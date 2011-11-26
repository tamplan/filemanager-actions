/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011 Pierre Wieser and others (see AUTHORS)
 *
 * This Program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This Program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this Library; see the file COPYING.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors:
 *   Frederic Ruaudel <grumz@grumz.net>
 *   Rodrigo Moya <rodrigo@gnome-db.org>
 *   Pierre Wieser <pwieser@trychlos.org>
 *   ... and many others (see AUTHORS)
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <api/na-core-utils.h>
#include <api/na-object-api.h>

#include "na-iprefs.h"
#include "na-importer.h"
#include "na-importer-ask.h"

typedef struct {
	GList             *just_imported;
	NAIImporterCheckFn check_fn;
	void              *check_fn_data;
}
	ImporterExistsStr;

extern gboolean iimporter_initialized;		/* defined in na-iimporter.c */
extern gboolean iimporter_finalized;		/* defined in na-iimporter.c */

static guint         import_from_uri( const NAPivot *pivot, GList *modules, NAImporterParms *parms, const gchar *uri, NAImporterResult **result );
static NAObjectItem *is_importing_already_exists( const NAObjectItem *importing, ImporterExistsStr *parms );
static guint         ask_user_for_mode( const NAObjectItem *importing, const NAObjectItem *existing, NAImporterAskUserParms *parms );

/*
 * na_importer_import_from_list:
 * @pivot: the #NAPivot pivot for this application.
 * @parms: a #NAImporterParms structure.
 *
 * Imports a list of URIs.
 *
 * For each URI to import, we search through the available #NAIImporter
 * providers until the first which respond something different from
 * "not_willing_to" code.
 *
 * #parms.uris contains a list of URIs to import.
 *
 * Each import operation will have its corresponding newly allocated
 * #NAImporterResult structure which will contain:
 * - the imported URI
 * - a #NAObjectItem item if import was successful, or %NULL
 * - a list of error messages, or %NULL.
 *
 * If asked mode is 'ask', then ask the user at least the first time;
 * the 'keep my choice' is active or not, depending of the last time used,
 * then the 'keep my chose' is kept for other times.
 * So preferences are:
 * - asked import mode (may be 'ask') -> import-mode
 * - keep my choice                   -> import-keep-choice
 * - last chosen import mode          -> import-ask-user-last-mode
 *
 * Returns: the last import operation code.
 *
 * Since: 2.30
 */
guint
na_importer_import_from_list( const NAPivot *pivot, NAImporterParms *parms )
{
	static const gchar *thisfn = "na_importer_import_from_list";
	GList *modules;
	GSList *iuri;
	NAImporterResult *result;
	guint code;

	g_return_val_if_fail( NA_IS_PIVOT( pivot ), IMPORTER_CODE_PROGRAM_ERROR );

	code = IMPORTER_CODE_NOT_WILLING_TO;
	parms->results = NULL;

	if( iimporter_initialized && !iimporter_finalized ){

		g_debug( "%s: pivot=%p, parms=%p", thisfn, ( void * ) pivot, ( void * ) parms );

		modules = na_pivot_get_providers( pivot, NA_IIMPORTER_TYPE );

		for( iuri = parms->uris ; iuri ; iuri = iuri->next ){
			code = import_from_uri( pivot, modules, parms, ( const gchar * ) iuri->data, &result );
			parms->results = g_list_prepend( parms->results, result );
		}

		na_pivot_free_providers( modules );
		parms->results = g_list_reverse( parms->results );
	}

	return( code );
}

/*
 * na_importer_free_result:
 * @result: the #NAImporterResult structure to be released.
 *
 * Release the structure.
 */
void
na_importer_free_result( NAImporterResult *result )
{
	g_free( result->uri );
	na_core_utils_slist_free( result->messages );

	g_free( result );
}

static guint
import_from_uri( const NAPivot *pivot, GList *modules, NAImporterParms *parms, const gchar *uri, NAImporterResult **result )
{
	guint code;
	GList *im;
	NAIImporterImportFromUriParms provider_parms;
	ImporterExistsStr exists_parms;
	NAImporterAskUserParms ask_parms;

	code = IMPORTER_CODE_NOT_WILLING_TO;

	memset( &exists_parms, '\0', sizeof( ImporterExistsStr ));
	exists_parms.just_imported = parms->results;
	exists_parms.check_fn = parms->check_fn;
	exists_parms.check_fn_data = parms->check_fn_data;

	memset( &ask_parms, '\0', sizeof( NAImporterAskUserParms ));
	ask_parms.parent = parms->parent;
	ask_parms.uri = ( gchar * ) uri;
	ask_parms.count = g_list_length( parms->results );
	ask_parms.keep_choice = na_settings_get_boolean( NA_IPREFS_IMPORT_ASK_USER_KEEP_LAST_CHOICE, NULL, NULL );
	ask_parms.pivot = pivot;

	memset( &provider_parms, '\0', sizeof( NAIImporterImportFromUriParms ));
	provider_parms.version = 1;
	provider_parms.uri = ( gchar * ) uri;
	provider_parms.asked_mode = parms->mode;
	provider_parms.check_fn = ( NAIImporterCheckFn ) is_importing_already_exists;
	provider_parms.check_fn_data = &exists_parms;
	provider_parms.ask_fn = ( NAIImporterAskUserFn ) ask_user_for_mode;
	provider_parms.ask_fn_data = &ask_parms;

	for( im = modules ; im && code == IMPORTER_CODE_NOT_WILLING_TO ; im = im->next ){
		code = na_iimporter_import_from_uri( NA_IIMPORTER( im->data ), &provider_parms );
	}

	*result = g_new0( NAImporterResult, 1 );
	( *result )->uri = g_strdup( uri );
	( *result )->mode = provider_parms.import_mode;
	( *result )->exist = provider_parms.exist;
	( *result )->imported = provider_parms.imported;
	( *result )->messages = provider_parms.messages;

	return( code );
}

/*
 * to see if an imported item already exists, we have to check
 * - the current list of just imported items
 * - the main window (if any), which contains the in-memory list of items
 * - the tree in pivot which contains the 'actual' items
 */
static NAObjectItem *
is_importing_already_exists( const NAObjectItem *importing, ImporterExistsStr *parms )
{
	static const gchar *thisfn = "na_importer_is_importing_already_exists";
	NAObjectItem *exists;
	GList *ip;

	exists = NULL;
	gchar *importing_id = na_object_get_id( importing );
	g_debug( "%s: importing=%p, id=%s", thisfn, ( void * ) importing, importing_id );

	/* is the importing item already in the current importation list ?
	 */
	for( ip = parms->just_imported ; ip && !exists ; ip = ip->next ){
		NAImporterResult *result = ( NAImporterResult * ) ip->data;

		if( result->imported ){
			gchar *id = na_object_get_id( result->imported );
			if( !strcmp( importing_id, id )){
				exists = NA_OBJECT_ITEM( result->imported );
			}
			g_free( id );
		}
	}

	g_free( importing_id );

	/* if not found in our current importation list,
	 * then check the existence via provided function and data
	 */
	if( !exists ){
		exists = parms->check_fn( importing, parms->check_fn_data );
	}

	return( exists );
}

static guint
ask_user_for_mode( const NAObjectItem *importing, const NAObjectItem *existing, NAImporterAskUserParms *parms )
{
	guint mode;

	if( parms->count == 0 || !parms->keep_choice ){
		mode = na_importer_ask_user( importing, existing, parms );

	} else {
		mode = na_iprefs_get_import_mode( NA_IPREFS_IMPORT_ASK_USER_LAST_MODE, NULL );
	}

	return( mode );
}
