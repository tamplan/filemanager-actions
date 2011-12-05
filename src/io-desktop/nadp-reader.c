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

#include <glib/gi18n.h>
#include <stdlib.h>
#include <string.h>

#include <api/na-core-utils.h>
#include <api/na-data-types.h>
#include <api/na-ifactory-object-data.h>
#include <api/na-ifactory-provider.h>
#include <api/na-object-api.h>

#include "nadp-desktop-provider.h"
#include "nadp-keys.h"
#include "nadp-reader.h"
#include "nadp-utils.h"
#include "nadp-xdg-dirs.h"

typedef struct {
	gchar *path;
	gchar *id;
}
	DesktopPath;

/* the structure passed as reader data to NAIFactoryObject
 */
typedef struct {
	NadpDesktopFile *ndf;
	NAObjectAction  *action;
}
	NadpReaderData;

#define ERR_NOT_DESKTOP		_( "The Desktop I/O Provider is not able to handle the URI" )

static GList            *get_list_of_desktop_paths( NadpDesktopProvider *provider, GSList **mesages );
static void              get_list_of_desktop_files( const NadpDesktopProvider *provider, GList **files, const gchar *dir, GSList **messages );
static gboolean          is_already_loaded( const NadpDesktopProvider *provider, GList *files, const gchar *desktop_id );
static GList            *desktop_path_from_id( const NadpDesktopProvider *provider, GList *files, const gchar *dir, const gchar *id );
static NAIFactoryObject *item_from_desktop_path( const NadpDesktopProvider *provider, DesktopPath *dps, GSList **messages );
static NAIFactoryObject *item_from_desktop_file( const NadpDesktopProvider *provider, NadpDesktopFile *ndf, GSList **messages );
static void              desktop_weak_notify( NadpDesktopFile *ndf, GObject *item );
static void              free_desktop_paths( GList *paths );

static void              read_start_read_subitems_key( const NAIFactoryProvider *provider, NAObjectItem *item, NadpReaderData *reader_data, GSList **messages );
static void              read_start_profile_attach_profile( const NAIFactoryProvider *provider, NAObjectProfile *profile, NadpReaderData *reader_data, GSList **messages );

static gboolean          read_done_item_is_writable( const NAIFactoryProvider *provider, NAObjectItem *item, NadpReaderData *reader_data, GSList **messages );
static void              read_done_action_read_profiles( const NAIFactoryProvider *provider, NAObjectAction *action, NadpReaderData *data, GSList **messages );
static void              read_done_action_load_profile( const NAIFactoryProvider *provider, NadpReaderData *reader_data, const gchar *profile_id, GSList **messages );

/*
 * Returns an unordered list of NAIFactoryObject-derived objects
 *
 * This is implementation of NAIIOProvider::read_items method
 */
GList *
nadp_iio_provider_read_items( const NAIIOProvider *provider, GSList **messages )
{
	static const gchar *thisfn = "nadp_iio_provider_read_items";
	GList *items;
	GList *desktop_paths, *ip;
	NAIFactoryObject *item;

	g_debug( "%s: provider=%p (%s), messages=%p",
			thisfn, ( void * ) provider, G_OBJECT_TYPE_NAME( provider ), ( void * ) messages );

	g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider ), NULL );

	items = NULL;
	nadp_desktop_provider_release_monitors( NADP_DESKTOP_PROVIDER( provider ));

	desktop_paths = get_list_of_desktop_paths( NADP_DESKTOP_PROVIDER( provider ), messages );
	for( ip = desktop_paths ; ip ; ip = ip->next ){

		item = item_from_desktop_path( NADP_DESKTOP_PROVIDER( provider ), ( DesktopPath * ) ip->data, messages );

		if( item ){
			items = g_list_prepend( items, item );
			na_object_dump( item );
		}
	}

	free_desktop_paths( desktop_paths );

	g_debug( "%s: count=%d", thisfn, g_list_length( items ));
	return( items );
}

/*
 * returns a list of DesktopPath items
 *
 * we get the ordered list of XDG_DATA_DIRS, and the ordered list of
 *  subdirs to add; then for each item of each list, we search for
 *  .desktop files in the resulted built path
 *
 * the returned list is so a list of DesktopPath struct, in
 * the ordered of preference (most preferred first)
 */
static GList *
get_list_of_desktop_paths( NadpDesktopProvider *provider, GSList **messages )
{
	GList *files;
	GSList *xdg_dirs, *idir;
	GSList *subdirs, *isub;
	gchar *dir;

	files = NULL;
	xdg_dirs = nadp_xdg_dirs_get_data_dirs();
	subdirs = na_core_utils_slist_from_split( NADP_DESKTOP_PROVIDER_SUBDIRS, G_SEARCHPATH_SEPARATOR_S );

	/* explore each directory from XDG_DATA_DIRS
	 */
	for( idir = xdg_dirs ; idir ; idir = idir->next ){

		/* explore each N-A candidate subdirectory for each XDG dir
		 */
		for( isub = subdirs ; isub ; isub = isub->next ){

			dir = g_build_filename(( gchar * ) idir->data, ( gchar * ) isub->data, NULL );
			nadp_desktop_provider_add_monitor( provider, dir );
			get_list_of_desktop_files( provider, &files, dir, messages );
			g_free( dir );
		}
	}

	na_core_utils_slist_free( subdirs );
	na_core_utils_slist_free( xdg_dirs );

	return( files );
}

/*
 * scans the directory for .desktop files
 * only adds to the list those which have not been yet loaded
 */
static void
get_list_of_desktop_files( const NadpDesktopProvider *provider, GList **files, const gchar *dir, GSList **messages )
{
	static const gchar *thisfn = "nadp_reader_get_list_of_desktop_files";
	GDir *dir_handle;
	GError *error;
	const gchar *name;
	gchar *desktop_id;

	g_debug( "%s: provider=%p, files=%p (count=%d), dir=%s, messages=%p",
			thisfn, ( void * ) provider, ( void * ) files, g_list_length( *files ), dir, ( void * ) messages );

	error = NULL;
	dir_handle = NULL;

	/* do not warn when the directory just doesn't exist
	 */
	if( g_file_test( dir, G_FILE_TEST_IS_DIR )){
		dir_handle = g_dir_open( dir, 0, &error );
		if( error ){
			g_warning( "%s: %s: %s", thisfn, dir, error->message );
			g_error_free( error );
			goto close_dir_handle;
		}
	} else {
		g_debug( "%s: %s: directory doesn't exist", thisfn, dir );
	}

	if( dir_handle ){
		while(( name = g_dir_read_name( dir_handle ))){
			if( g_str_has_suffix( name, NADP_DESKTOP_FILE_SUFFIX )){
				desktop_id = na_core_utils_str_remove_suffix( name, NADP_DESKTOP_FILE_SUFFIX );
				if( !is_already_loaded( provider, *files, desktop_id )){
					*files = desktop_path_from_id( provider, *files, dir, desktop_id );
				}
				g_free( desktop_id );
			}
		}
	}

close_dir_handle:
	if( dir_handle ){
		g_dir_close( dir_handle );
	}
}

static gboolean
is_already_loaded( const NadpDesktopProvider *provider, GList *files, const gchar *desktop_id )
{
	gboolean found;
	GList *ip;
	DesktopPath *dps;

	found = FALSE;
	for( ip = files ; ip && !found ; ip = ip->next ){
		dps = ( DesktopPath * ) ip->data;
		if( !g_ascii_strcasecmp( dps->id, desktop_id )){
			found = TRUE;
		}
	}

	return( found );
}

static GList *
desktop_path_from_id( const NadpDesktopProvider *provider, GList *files, const gchar *dir, const gchar *id )
{
	DesktopPath *dps;
	gchar *bname;
	GList *list;

	dps = g_new0( DesktopPath, 1 );

	bname = g_strdup_printf( "%s%s", id, NADP_DESKTOP_FILE_SUFFIX );
	dps->path = g_build_filename( dir, bname, NULL );
	g_free( bname );

	dps->id = g_strdup( id );

	list = g_list_prepend( files, dps );

	return( list );
}

/*
 * Returns a newly allocated NAIFactoryObject-derived object, initialized
 * from the .desktop file pointed to by DesktopPath struct
 */
static NAIFactoryObject *
item_from_desktop_path( const NadpDesktopProvider *provider, DesktopPath *dps, GSList **messages )
{
	NadpDesktopFile *ndf;

	ndf = nadp_desktop_file_new_from_path( dps->path );
	if( !ndf ){
		return( NULL );
	}

	return( item_from_desktop_file( provider, ndf, messages ));
}

/*
 * Returns a newly allocated NAIFactoryObject-derived object, initialized
 * from the .desktop file
 */
static NAIFactoryObject *
item_from_desktop_file( const NadpDesktopProvider *provider, NadpDesktopFile *ndf, GSList **messages )
{
	static const gchar *thisfn = "nadp_reader_item_from_desktop_file";
	NAIFactoryObject *item;
	gchar *type;
	NadpReaderData *reader_data;
	gchar *id;

	item = NULL;
	type = nadp_desktop_file_get_file_type( ndf );

	if( !strcmp( type, NADP_VALUE_TYPE_ACTION )){
		item = NA_IFACTORY_OBJECT( na_object_action_new());

	} else if( !strcmp( type, NADP_VALUE_TYPE_MENU )){
		item = NA_IFACTORY_OBJECT( na_object_menu_new());

	} else {
		g_warning( "%s: unknown type=%s", thisfn, type );
	}

	if( item ){
		id = nadp_desktop_file_get_id( ndf );
		na_object_set_id( item, id );
		g_free( id );

		reader_data = g_new0( NadpReaderData, 1 );
		reader_data->ndf = ndf;

		na_ifactory_provider_read_item( NA_IFACTORY_PROVIDER( provider ), reader_data, item, messages );

		na_object_set_provider_data( item, ndf );
		g_object_weak_ref( G_OBJECT( item ), ( GWeakNotify ) desktop_weak_notify, ndf );

		g_free( reader_data );
	}

	return( item );
}

static void
desktop_weak_notify( NadpDesktopFile *ndf, GObject *item )
{
	static const gchar *thisfn = "nadp_reader_desktop_weak_notify";

	g_debug( "%s: ndf=%p (%s), item=%p (%s)",
			thisfn, ( void * ) ndf, G_OBJECT_TYPE_NAME( ndf ),
			( void * ) item, G_OBJECT_TYPE_NAME( item ));

	g_object_unref( ndf );
}

static void
free_desktop_paths( GList *paths )
{
	GList *ip;
	DesktopPath *dps;

	for( ip = paths ; ip ; ip = ip->next ){
		dps = ( DesktopPath * ) ip->data;
		g_free( dps->path );
		g_free( dps->id );
		g_free( dps );
	}

	g_list_free( paths );
}

/**
 * nadp_reader_iimporter_import_from_uri:
 * @instance: the #NAIImporter provider.
 * @parms: a #NAIImporterUriParms structure.
 *
 * Imports an item.
 *
 * Returns: the import operation code.
 *
 * As soon as we have a valid .desktop file, we are most probably willing
 * to successfully import an action or a menu of it.
 *
 * GLib does not have any primitive to load a key file from an uri.
 * So we have to load the file into memory, and then try to load the key
 * file from the memory data.
 */
guint
nadp_reader_iimporter_import_from_uri( const NAIImporter *instance, NAIImporterImportFromUriParms *parms )
{
	static const gchar *thisfn = "nadp_reader_iimporter_import_from_uri";
	guint code;
	NadpDesktopFile *ndf;
	NAIImporterManageImportModeParms manage_parms;

	g_debug( "%s: instance=%p, parms=%p", thisfn, ( void * ) instance, ( void * ) parms );

	g_return_val_if_fail( NA_IS_IIMPORTER( instance ), IMPORTER_CODE_PROGRAM_ERROR );
	g_return_val_if_fail( NADP_IS_DESKTOP_PROVIDER( instance ), IMPORTER_CODE_PROGRAM_ERROR );

	code = IMPORTER_CODE_NOT_WILLING_TO;

	ndf = nadp_desktop_file_new_from_uri( parms->uri );
	if( ndf ){
		parms->exist = FALSE;
		parms->import_mode = IMPORTER_MODE_NO_IMPORT;
		parms->imported = ( NAObjectItem * ) item_from_desktop_file(
				( const NadpDesktopProvider * ) NADP_DESKTOP_PROVIDER( instance ),
				ndf, &parms->messages );

		if( parms->imported ){
			g_return_val_if_fail( NA_IS_OBJECT_ITEM( parms->imported ), IMPORTER_CODE_NOT_WILLING_TO );

			/* remove the weak reference on desktop file set by 'item_from_desktop_file'
			 * as we must consider this #NAObjectItem as a new one
			 */
			na_object_set_provider_data( parms->imported, NULL );
			g_object_weak_unref( G_OBJECT( parms->imported ), ( GWeakNotify ) desktop_weak_notify, ndf );
			g_object_unref( ndf );

			manage_parms.version = 1;
			manage_parms.imported = parms->imported;
			manage_parms.check_fn = parms->check_fn;
			manage_parms.check_fn_data = parms->check_fn_data;
			manage_parms.ask_fn = parms->ask_fn;
			manage_parms.ask_fn_data = parms->ask_fn_data;
			manage_parms.asked_mode = parms->asked_mode;
			manage_parms.messages = parms->messages;

			code = na_iimporter_manage_import_mode( &manage_parms );

			parms->exist = manage_parms.exist;
			parms->import_mode = manage_parms.import_mode;
			parms->messages = manage_parms.messages;
		}

		if( code != IMPORTER_CODE_OK ){
			if( parms->imported ){
				g_debug( "%s: unreffing imported item %p as na_iimporter_manage_import_mode didn't return IMPORTER_CODE_OK", thisfn, parms->imported );
				g_object_unref( parms->imported );
				parms->imported = NULL;
			}
		}
	}

	if( code == IMPORTER_CODE_NOT_WILLING_TO ){
		na_core_utils_slist_add_message( &parms->messages, ERR_NOT_DESKTOP );
	}

	return( code );
}

/*
 * at this time, the object has been allocated and its id has been set
 * read here the subitems key, which may be 'Profiles' or 'ItemsList'
 * depending of the exact class of the NAObjectItem
 */
void
nadp_reader_ifactory_provider_read_start( const NAIFactoryProvider *reader, void *reader_data, const NAIFactoryObject *serializable, GSList **messages )
{
	static const gchar *thisfn = "nadp_reader_ifactory_provider_read_start";

	g_return_if_fail( NA_IS_IFACTORY_PROVIDER( reader ));
	g_return_if_fail( NADP_IS_DESKTOP_PROVIDER( reader ));
	g_return_if_fail( NA_IS_IFACTORY_OBJECT( serializable ));

	if( !NADP_DESKTOP_PROVIDER( reader )->private->dispose_has_run ){

		g_debug( "%s: reader=%p (%s), reader_data=%p, serializable=%p (%s), messages=%p",
				thisfn,
				( void * ) reader, G_OBJECT_TYPE_NAME( reader ),
				( void * ) reader_data,
				( void * ) serializable, G_OBJECT_TYPE_NAME( serializable ),
				( void * ) messages );

		if( NA_IS_OBJECT_ITEM( serializable )){
			read_start_read_subitems_key( reader, NA_OBJECT_ITEM( serializable ), ( NadpReaderData * ) reader_data, messages );
			na_object_set_iversion( serializable, 3 );
		}

		if( NA_IS_OBJECT_PROFILE( serializable )){
			read_start_profile_attach_profile( reader, NA_OBJECT_PROFILE( serializable ), ( NadpReaderData * ) reader_data, messages );
		}
	}
}

static void
read_start_read_subitems_key( const NAIFactoryProvider *provider, NAObjectItem *item, NadpReaderData *reader_data, GSList **messages )
{
	GSList *subitems;
	gboolean key_found;

	subitems = nadp_desktop_file_get_string_list( reader_data->ndf,
			NADP_GROUP_DESKTOP,
			NA_IS_OBJECT_ACTION( item ) ? NADP_KEY_PROFILES : NADP_KEY_ITEMS_LIST,
			&key_found,
			NULL );

	if( key_found ){
		na_object_set_items_slist( item, subitems );
	}

	na_core_utils_slist_free( subitems );
}

static void
read_start_profile_attach_profile( const NAIFactoryProvider *provider, NAObjectProfile *profile, NadpReaderData *reader_data, GSList **messages )
{
	na_object_attach_profile( reader_data->action, profile );
}

/*
 * reading any data from a desktop file requires:
 * - a NadpDesktopFile object which has been initialized with the .desktop file
 *   -> has been attached to the NAObjectItem in get_item() above
 * - the data type (+ reading default value)
 * - group and key names
 *
 * Returns: NULL if the key has not been found
 * letting the caller deal with default values
 */
NADataBoxed *
nadp_reader_ifactory_provider_read_data( const NAIFactoryProvider *reader, void *reader_data, const NAIFactoryObject *object, const NADataDef *def, GSList **messages )
{
	static const gchar *thisfn = "nadp_reader_ifactory_provider_read_data";
	NADataBoxed *boxed;
	gboolean found;
	NadpReaderData *nrd;
	gchar *group, *id;
	gchar *msg;
	gchar *str_value;
	gboolean bool_value;
	GSList *slist_value;
	guint uint_value;

	g_return_val_if_fail( NA_IS_IFACTORY_PROVIDER( reader ), NULL );
	g_return_val_if_fail( NADP_IS_DESKTOP_PROVIDER( reader ), NULL );
	g_return_val_if_fail( NA_IS_IFACTORY_OBJECT( object ), NULL );

	boxed = NULL;

	if( !NADP_DESKTOP_PROVIDER( reader )->private->dispose_has_run ){

		nrd = ( NadpReaderData * ) reader_data;
		g_return_val_if_fail( NADP_IS_DESKTOP_FILE( nrd->ndf ), NULL );

		if( def->desktop_entry ){

			if( NA_IS_OBJECT_ITEM( object )){
				group = g_strdup( NADP_GROUP_DESKTOP );

			} else {
				g_return_val_if_fail( NA_IS_OBJECT_PROFILE( object ), NULL );
				id = na_object_get_id( object );
				group = g_strdup_printf( "%s %s", NADP_GROUP_PROFILE, id );
				g_free( id );
			}

			switch( def->type ){

				case NA_DATA_TYPE_LOCALE_STRING:
					str_value = nadp_desktop_file_get_locale_string( nrd->ndf, group, def->desktop_entry, &found, def->default_value );
					if( found ){
						boxed = na_data_boxed_new( def );
						na_boxed_set_from_void( NA_BOXED( boxed ), str_value );
					}
					g_free( str_value );
					break;

				case NA_DATA_TYPE_STRING:
					str_value = nadp_desktop_file_get_string( nrd->ndf, group, def->desktop_entry, &found, def->default_value );
					if( found ){
						boxed = na_data_boxed_new( def );
						na_boxed_set_from_void( NA_BOXED( boxed ), str_value );
					}
					g_free( str_value );
					break;

				case NA_DATA_TYPE_BOOLEAN:
					bool_value = nadp_desktop_file_get_boolean( nrd->ndf, group, def->desktop_entry, &found, na_core_utils_boolean_from_string( def->default_value ));
					if( found ){
						boxed = na_data_boxed_new( def );
						na_boxed_set_from_void( NA_BOXED( boxed ), GUINT_TO_POINTER( bool_value ));
					}
					break;

				case NA_DATA_TYPE_STRING_LIST:
					slist_value = nadp_desktop_file_get_string_list( nrd->ndf, group, def->desktop_entry, &found, def->default_value );
					if( found ){
						boxed = na_data_boxed_new( def );
						na_boxed_set_from_void( NA_BOXED( boxed ), slist_value );
					}
					na_core_utils_slist_free( slist_value );
					break;

				case NA_DATA_TYPE_UINT:
					uint_value = nadp_desktop_file_get_uint( nrd->ndf, group, def->desktop_entry, &found, atoi( def->default_value ));
					if( found ){
						boxed = na_data_boxed_new( def );
						na_boxed_set_from_void( NA_BOXED( boxed ), GUINT_TO_POINTER( uint_value ));
					}
					break;

				default:
					msg = g_strdup_printf( "%s: %d: invalid data type.", thisfn, def->type );
					g_warning( "%s", msg );
					*messages = g_slist_append( *messages, msg );
			}

			g_free( group );
		}
	}

	return( boxed );
}

/*
 * called when each NAIFactoryObject object has been read
 */
void
nadp_reader_ifactory_provider_read_done( const NAIFactoryProvider *reader, void *reader_data, const NAIFactoryObject *serializable, GSList **messages )
{
	static const gchar *thisfn = "nadp_reader_ifactory_provider_read_done";
	gboolean writable;

	g_return_if_fail( NA_IS_IFACTORY_PROVIDER( reader ));
	g_return_if_fail( NADP_IS_DESKTOP_PROVIDER( reader ));
	g_return_if_fail( NA_IS_IFACTORY_OBJECT( serializable ));

	if( !NADP_DESKTOP_PROVIDER( reader )->private->dispose_has_run ){

		g_debug( "%s: reader=%p (%s), reader_data=%p, serializable=%p (%s), messages=%p",
				thisfn,
				( void * ) reader, G_OBJECT_TYPE_NAME( reader ),
				( void * ) reader_data,
				( void * ) serializable, G_OBJECT_TYPE_NAME( serializable ),
				( void * ) messages );

		if( NA_IS_OBJECT_ITEM( serializable )){
			writable = read_done_item_is_writable( reader, NA_OBJECT_ITEM( serializable ), ( NadpReaderData * ) reader_data, messages );
			na_object_set_readonly( serializable, !writable );
		}

		if( NA_IS_OBJECT_ACTION( serializable )){
			read_done_action_read_profiles( reader, NA_OBJECT_ACTION( serializable ), ( NadpReaderData * ) reader_data, messages );
		}

		g_debug( "%s: quitting for %s at %p", thisfn, G_OBJECT_TYPE_NAME( serializable ), ( void * ) serializable );
	}
}

static gboolean
read_done_item_is_writable( const NAIFactoryProvider *provider, NAObjectItem *item, NadpReaderData *reader_data, GSList **messages )
{
	NadpDesktopFile *ndf;
	gchar *uri;
	gboolean writable;

	ndf = reader_data->ndf;
	uri = nadp_desktop_file_get_key_file_uri( ndf );
	writable = nadp_utils_uri_is_writable( uri );
	g_free( uri );

	return( writable );
}

/*
 * read and attach profiles in the specified order
 * profiles which may exist in .desktop files, but are not referenced
 * in the 'Profiles' string list are just ignored
 */
static void
read_done_action_read_profiles( const NAIFactoryProvider *provider, NAObjectAction *action, NadpReaderData *reader_data, GSList **messages )
{
	GSList *order;
	GSList *ip;
	gchar *profile_id;
	NAObjectId *found;

	reader_data->action = action;
	order = na_object_get_items_slist( action );

	for( ip = order ; ip ; ip = ip->next ){
		profile_id = ( gchar * ) ip->data;
		found = na_object_get_item( action, profile_id );
		if( !found ){
			read_done_action_load_profile( provider, reader_data, profile_id, messages );
		}
	}

	na_core_utils_slist_free( order );
}

static void
read_done_action_load_profile( const NAIFactoryProvider *provider, NadpReaderData *reader_data, const gchar *profile_id, GSList **messages )
{
	NAObjectProfile *profile;

	g_debug( "nadp_reader_read_done_action_load_profile: loading profile=%s", profile_id );

	profile = na_object_profile_new();
	na_object_set_id( profile, profile_id );

	na_ifactory_provider_read_item(
			NA_IFACTORY_PROVIDER( provider ),
			reader_data,
			NA_IFACTORY_OBJECT( profile ),
			messages );
}
