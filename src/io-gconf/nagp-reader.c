/*
 * FileManager-Actions
 * A file-manager extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
 *
 * FileManager-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * FileManager-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FileManager-Actions; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
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

#include <api/fma-data-def.h>
#include <api/fma-data-types.h>
#include <api/fma-ifactory-provider.h>
#include <api/fma-iio-provider.h>
#include <api/fma-object-api.h>
#include <api/fma-core-utils.h>
#include <api/fma-gconf-utils.h>

#include "fma-gconf-provider.h"
#include "fma-gconf-keys.h"
#include "nagp-reader.h"

typedef struct {
	gchar        *path;
	GSList       *entries;
	FMAObjectItem *parent;
}
	ReaderData;

static FMAObjectItem *read_item( FMAGConfProvider *provider, const gchar *path, GSList **messages );

static void          read_start_profile_attach_profile( const FMAIFactoryProvider *provider, FMAObjectProfile *profile, ReaderData *data, GSList **messages );

static gboolean      read_done_item_is_writable( const FMAIFactoryProvider *provider, FMAObjectItem *item, ReaderData *data, GSList **messages );
static void          read_done_action_read_profiles( const FMAIFactoryProvider *provider, FMAObjectAction *action, ReaderData *data, GSList **messages );
static void          read_done_action_load_profile( const FMAIFactoryProvider *provider, ReaderData *data, const gchar *path, GSList **messages );

static FMADataBoxed  *get_boxed_from_path( const FMAGConfProvider *provider, const gchar *path, ReaderData *reader_data, const FMADataDef *def );
static gboolean      is_key_writable( FMAGConfProvider *gconf, const gchar *key );

/*
 * nagp_iio_provider_read_items:
 *
 * Note that whatever be the version of the read action, it will be
 * stored as a #FMAObjectAction and its set of #FMAObjectProfile of the same,
 * latest, version of these classes.
 */
GList *
nagp_iio_provider_read_items( const FMAIIOProvider *provider, GSList **messages )
{
	static const gchar *thisfn = "nagp_reader_nagp_iio_provider_read_items";
	FMAGConfProvider *self;
	GList *items_list = NULL;
	GSList *listpath, *ip;
	FMAObjectItem *item;

	g_debug( "%s: provider=%p, messages=%p", thisfn, ( void * ) provider, ( void * ) messages );

	g_return_val_if_fail( FMA_IS_IIO_PROVIDER( provider ), NULL );
	g_return_val_if_fail( FMA_IS_GCONF_PROVIDER( provider ), NULL );
	self = FMA_GCONF_PROVIDER( provider );

	if( !self->private->dispose_has_run ){

		listpath = fma_gconf_utils_get_subdirs( self->private->gconf, FMA_GCONF_CONFIGURATIONS_PATH );

		for( ip = listpath ; ip ; ip = ip->next ){

			item = read_item( self, ( const gchar * ) ip->data, messages );
			if( item ){
				items_list = g_list_prepend( items_list, item );
				fma_object_dump( item );
			}
		}

		fma_gconf_utils_free_subdirs( listpath );
	}

	g_debug( "%s: count=%d", thisfn, g_list_length( items_list ));
	return( items_list );
}

/*
 * path is here the full path to an item
 */
static FMAObjectItem *
read_item( FMAGConfProvider *provider, const gchar *path, GSList **messages )
{
	static const gchar *thisfn = "nagp_reader_read_item";
	FMAObjectItem *item;
	gchar *full_path;
	gchar *type;
	gchar *id;
	ReaderData *data;

	g_debug( "%s: provider=%p, path=%s", thisfn, ( void * ) provider, path );
	g_return_val_if_fail( FMA_IS_GCONF_PROVIDER( provider ), NULL );
	g_return_val_if_fail( FMA_IS_IIO_PROVIDER( provider ), NULL );
	g_return_val_if_fail( !provider->private->dispose_has_run, NULL );

	full_path = gconf_concat_dir_and_key( path, FMA_GCONF_ENTRY_TYPE );
	type = fma_gconf_utils_read_string( provider->private->gconf, full_path, TRUE, FMA_GCONF_VALUE_TYPE_ACTION );
	g_free( full_path );
	item = NULL;

	/* an item may have 'Action' or 'Menu' type; defaults to Action
	 */
	if( !type || !strlen( type ) || !strcmp( type, FMA_GCONF_VALUE_TYPE_ACTION )){
		item = FMA_OBJECT_ITEM( fma_object_action_new());

	} else if( !strcmp( type, FMA_GCONF_VALUE_TYPE_MENU )){
		item = FMA_OBJECT_ITEM( fma_object_menu_new());

	} else {
		g_warning( "%s: unknown type '%s' at %s", thisfn, type, path );
	}

	g_free( type );

	if( item ){
		id = g_path_get_basename( path );
		fma_object_set_id( item, id );
		g_free( id );

		data = g_new0( ReaderData, 1 );
		data->path = ( gchar * ) path;
		data->entries = fma_gconf_utils_get_entries( provider->private->gconf, path );
		fma_gconf_utils_dump_entries( data->entries );

		fma_ifactory_provider_read_item(
				FMA_IFACTORY_PROVIDER( provider ),
				data,
				FMA_IFACTORY_OBJECT( item ),
				messages );

		fma_gconf_utils_free_entries( data->entries );
		g_free( data );
	}

	return( item );
}

void
nagp_reader_read_start( const FMAIFactoryProvider *provider, void *reader_data, const FMAIFactoryObject *object, GSList **messages  )
{
	static const gchar *thisfn = "nagp_reader_read_start";

	g_return_if_fail( FMA_IS_IFACTORY_PROVIDER( provider ));
	g_return_if_fail( FMA_IS_GCONF_PROVIDER( provider ));
	g_return_if_fail( FMA_IS_IFACTORY_OBJECT( object ));

	if( !FMA_GCONF_PROVIDER( provider )->private->dispose_has_run ){

		g_debug( "%s: provider=%p (%s), reader_data=%p, object=%p (%s), messages=%p",
				thisfn,
				( void * ) provider, G_OBJECT_TYPE_NAME( provider ),
				( void * ) reader_data,
				( void * ) object, G_OBJECT_TYPE_NAME( object ),
				( void * ) messages );

		if( FMA_IS_OBJECT_PROFILE( object )){
			read_start_profile_attach_profile( provider, FMA_OBJECT_PROFILE( object ), ( ReaderData * ) reader_data, messages );
		}
	}
}

static void
read_start_profile_attach_profile( const FMAIFactoryProvider *provider, FMAObjectProfile *profile, ReaderData *data, GSList **messages )
{
	fma_object_attach_profile( data->parent, profile );
}

FMADataBoxed *
nagp_reader_read_data( const FMAIFactoryProvider *provider, void *reader_data, const FMAIFactoryObject *object, const FMADataDef *def, GSList **messages )
{
	static const gchar *thisfn = "nagp_reader_read_data";
	FMADataBoxed *boxed;

	g_return_val_if_fail( FMA_IS_IFACTORY_PROVIDER( provider ), NULL );
	g_return_val_if_fail( FMA_IS_IFACTORY_OBJECT( object ), NULL );

	/*g_debug( "%s: reader_data=%p, object=%p (%s), data=%s",
			thisfn,
			( void * ) reader_data,
			( void * ) object, G_OBJECT_TYPE_NAME( object ),
			def->name );*/

	if( !def->gconf_entry || !strlen( def->gconf_entry )){
		g_warning( "%s: GConf entry is not set for FMADataDef %s", thisfn, def->name );
		return( NULL );
	}

	boxed = get_boxed_from_path(
			FMA_GCONF_PROVIDER( provider ), (( ReaderData * ) reader_data )->path, reader_data, def );

	return( boxed );
}

void
nagp_reader_read_done( const FMAIFactoryProvider *provider, void *reader_data, const FMAIFactoryObject *object, GSList **messages  )
{
	static const gchar *thisfn = "nagp_reader_read_done";
	gboolean writable;

	g_return_if_fail( FMA_IS_IFACTORY_PROVIDER( provider ));
	g_return_if_fail( FMA_IS_GCONF_PROVIDER( provider ));
	g_return_if_fail( FMA_IS_IFACTORY_OBJECT( object ));

	if( !FMA_GCONF_PROVIDER( provider )->private->dispose_has_run ){

		g_debug( "%s: provider=%p (%s), reader_data=%p, object=%p (%s), messages=%p",
				thisfn,
				( void * ) provider, G_OBJECT_TYPE_NAME( provider ),
				( void * ) reader_data,
				( void * ) object, G_OBJECT_TYPE_NAME( object ),
				( void * ) messages );

		if( FMA_IS_OBJECT_ITEM( object )){
			writable = read_done_item_is_writable( provider, FMA_OBJECT_ITEM( object ), ( ReaderData * ) reader_data, messages );
			fma_object_set_readonly( object, !writable );
		}

		if( FMA_IS_OBJECT_ACTION( object )){
			read_done_action_read_profiles( provider, FMA_OBJECT_ACTION( object ), ( ReaderData * ) reader_data, messages );
		}

		g_debug( "%s: quitting for %s at %p", thisfn, G_OBJECT_TYPE_NAME( object ), ( void * ) object );
	}
}

static gboolean
read_done_item_is_writable( const FMAIFactoryProvider *provider, FMAObjectItem *item, ReaderData *data, GSList **messages )
{
	GSList *ie;
	gboolean writable;
	GConfEntry *gconf_entry;
	const gchar *key;

	/* check for writability of this item
	 * item is writable if and only if all entries are themselves writable
	 */
	writable = TRUE;
	for( ie = data->entries ; ie && writable ; ie = ie->next ){
		gconf_entry = ( GConfEntry * ) ie->data;
		key = gconf_entry_get_key( gconf_entry );
		writable = is_key_writable( FMA_GCONF_PROVIDER( provider ), key );
	}

	g_debug( "nagp_reader_read_done_item: writable=%s", writable ? "True":"False" );
	return( writable );
}

static void
read_done_action_read_profiles( const FMAIFactoryProvider *provider, FMAObjectAction *action, ReaderData *data, GSList **messages )
{
	static const gchar *thisfn = "nagp_reader_read_done_action_read_profiles";
	GSList *order;
	GSList *list_profiles;
	GSList *ip;
	gchar *profile_id;
	gchar *profile_path;
	FMAObjectId *found;
	FMAObjectProfile *profile;

	data->parent = FMA_OBJECT_ITEM( action );
	order = fma_object_get_items_slist( action );
	list_profiles = fma_gconf_utils_get_subdirs( FMA_GCONF_PROVIDER( provider )->private->gconf, data->path );

	/* read profiles in the specified order
	 * as a protection against bugs in NACT, we check that profile has not
	 * already been loaded
	 */
	for( ip = order ; ip ; ip = ip->next ){
		profile_id = ( gchar * ) ip->data;
		found = fma_object_get_item( action, profile_id );
		if( !found ){
			g_debug( "nagp_reader_read_done_action: loading profile=%s", profile_id );
			profile_path = gconf_concat_dir_and_key( data->path, profile_id );
			read_done_action_load_profile( provider, data, profile_path, messages );
			g_free( profile_path );
		}
	}

	/* append other profiles
	 * this is mandatory for pre-2.29 actions which introduced order of profiles
	 */
	for( ip = list_profiles ; ip ; ip = ip->next ){
		profile_id = g_path_get_basename(( const gchar * ) ip->data );
		found = fma_object_get_item( action, profile_id );
		if( !found ){
			g_debug( "nagp_reader_read_done_action: loading profile=%s", profile_id );
			read_done_action_load_profile( provider, data, ( const gchar * ) ip->data, messages );
		}
		g_free( profile_id );
	}

	/* make sure we have at least one profile
	 */
	if( !fma_object_get_items_count( action )){
		g_warning( "%s: no profile found in GConf backend", thisfn );
		profile = fma_object_profile_new_with_defaults();
		fma_object_attach_profile( action, profile );
	}
}

static void
read_done_action_load_profile( const FMAIFactoryProvider *provider, ReaderData *data, const gchar *path, GSList **messages )
{
	gchar *id;
	ReaderData *profile_data;

	FMAObjectProfile *profile = fma_object_profile_new();

	id = g_path_get_basename( path );
	fma_object_set_id( profile, id );
	g_free( id );

	profile_data = g_new0( ReaderData, 1 );
	profile_data->parent = data->parent;
	profile_data->path = ( gchar * ) path;
	profile_data->entries = fma_gconf_utils_get_entries( FMA_GCONF_PROVIDER( provider )->private->gconf, path );

	fma_ifactory_provider_read_item(
			FMA_IFACTORY_PROVIDER( provider ),
			profile_data,
			FMA_IFACTORY_OBJECT( profile ),
			messages );

	fma_gconf_utils_free_entries( profile_data->entries );
	g_free( profile_data );
}

static FMADataBoxed *
get_boxed_from_path( const FMAGConfProvider *provider, const gchar *path, ReaderData *reader_data, const FMADataDef *def )
{
	static const gchar *thisfn = "nagp_reader_get_boxed_from_path";
	FMADataBoxed *boxed;
	gboolean have_entry;
	gchar *str_value;
	gboolean bool_value;
	GSList *slist_value;
	gint int_value;

	boxed = NULL;
	have_entry = fma_gconf_utils_has_entry( reader_data->entries, def->gconf_entry );
	g_debug( "%s: entry=%s, have_entry=%s", thisfn, def->gconf_entry, have_entry ? "True":"False" );

	if( have_entry ){
		gchar *entry_path = gconf_concat_dir_and_key( path, def->gconf_entry );
		boxed = fma_data_boxed_new( def );

		switch( def->type ){

			case FMA_DATA_TYPE_STRING:
			case FMA_DATA_TYPE_LOCALE_STRING:
				str_value = fma_gconf_utils_read_string( provider->private->gconf, entry_path, TRUE, NULL );
				fma_boxed_set_from_string( FMA_BOXED( boxed ), str_value );
				g_free( str_value );
				break;

			case FMA_DATA_TYPE_BOOLEAN:
				bool_value = fma_gconf_utils_read_bool( provider->private->gconf, entry_path, TRUE, FALSE );
				fma_boxed_set_from_void( FMA_BOXED( boxed ), GUINT_TO_POINTER( bool_value ));
				break;

			case FMA_DATA_TYPE_STRING_LIST:
				slist_value = fma_gconf_utils_read_string_list( provider->private->gconf, entry_path );
				fma_boxed_set_from_void( FMA_BOXED( boxed ), slist_value );
				fma_core_utils_slist_free( slist_value );
				break;

			case FMA_DATA_TYPE_UINT:
				int_value = fma_gconf_utils_read_int( provider->private->gconf, entry_path, TRUE, 0 );
				fma_boxed_set_from_void( FMA_BOXED( boxed ), GUINT_TO_POINTER( int_value ));
				break;

			default:
				g_warning( "%s: unknown type=%u for %s", thisfn, def->type, def->name );
				g_free( boxed );
				boxed = NULL;
		}

		g_free( entry_path );
	}

	return( boxed );
}

/*
 * key must be an existing entry (not a dir) to get a relevant return
 * value ; else we get FALSE
 */
static gboolean
is_key_writable( FMAGConfProvider *gconf, const gchar *key )
{
	static const gchar *thisfn = "nagp_read_is_key_writable";
	GError *error = NULL;
	gboolean is_writable;

	is_writable = gconf_client_key_is_writable( gconf->private->gconf, key, &error );
	if( error ){
		g_warning( "%s: gconf_client_key_is_writable: %s", thisfn, error->message );
		g_error_free( error );
		error = NULL;
		is_writable = FALSE;
	}

	return( is_writable );
}
