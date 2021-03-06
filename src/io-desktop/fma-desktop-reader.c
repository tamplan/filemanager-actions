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

#include <glib/gi18n.h>
#include <stdlib.h>
#include <string.h>

#include <api/fma-core-utils.h>
#include <api/fma-data-types.h>
#include <api/fma-ifactory-object-data.h>
#include <api/fma-ifactory-provider.h>
#include <api/fma-object-api.h>

#include "fma-desktop-provider.h"
#include "fma-desktop-keys.h"
#include "fma-desktop-reader.h"
#include "fma-desktop-utils.h"
#include "fma-desktop-xdg-dirs.h"

typedef struct {
	gchar *path;
	gchar *id;
}
	sDesktopPath;

/* the structure passed as reader data to FMAIFactoryObject
 */
typedef struct {
	FMADesktopFile  *ndf;
	FMAObjectAction *action;
}
	sReaderData;

#define ERR_NOT_DESKTOP		_( "The Desktop I/O Provider is not able to handle the URI" )

static GList             *get_list_of_desktop_paths( FMADesktopProvider *provider, GSList **mesages );
static void               get_list_of_desktop_files( const FMADesktopProvider *provider, GList **files, const gchar *dir, GSList **messages );
static gboolean           is_already_loaded( const FMADesktopProvider *provider, GList *files, const gchar *desktop_id );
static GList             *desktop_path_from_id( const FMADesktopProvider *provider, GList *files, const gchar *dir, const gchar *id );
static FMAIFactoryObject *item_from_desktop_path( const FMADesktopProvider *provider, sDesktopPath *dps, GSList **messages );
static FMAIFactoryObject *item_from_desktop_file( const FMADesktopProvider *provider, FMADesktopFile *ndf, GSList **messages );
static void               desktop_weak_notify( FMADesktopFile *ndf, GObject *item );
static void               free_desktop_paths( GList *paths );

static void               read_start_read_subitems_key( const FMAIFactoryProvider *provider, FMAObjectItem *item, sReaderData *reader_data, GSList **messages );
static void               read_start_profile_attach_profile( const FMAIFactoryProvider *provider, FMAObjectProfile *profile, sReaderData *reader_data, GSList **messages );

static gboolean           read_done_item_is_writable( const FMAIFactoryProvider *provider, FMAObjectItem *item, sReaderData *reader_data, GSList **messages );
static void               read_done_action_read_profiles( const FMAIFactoryProvider *provider, FMAObjectAction *action, sReaderData *data, GSList **messages );
static void               read_done_action_load_profile( const FMAIFactoryProvider *provider, sReaderData *reader_data, const gchar *profile_id, GSList **messages );

/*
 * Returns an unordered list of FMAIFactoryObject-derived objects
 *
 * This is implementation of FMAIIOProvider::read_items method
 */
GList *
fma_desktop_reader_iio_provider_read_items( const FMAIIOProvider *provider, GSList **messages )
{
	static const gchar *thisfn = "fma_desktop_reader_iio_provider_read_items";
	GList *items;
	GList *desktop_paths, *ip;
	FMAIFactoryObject *item;

	g_debug( "%s: provider=%p (%s), messages=%p",
			thisfn, ( void * ) provider, G_OBJECT_TYPE_NAME( provider ), ( void * ) messages );

	g_return_val_if_fail( FMA_IS_IIO_PROVIDER( provider ), NULL );

	items = NULL;
	fma_desktop_provider_release_monitors( FMA_DESKTOP_PROVIDER( provider ));

	desktop_paths = get_list_of_desktop_paths( FMA_DESKTOP_PROVIDER( provider ), messages );
	for( ip = desktop_paths ; ip ; ip = ip->next ){

		item = item_from_desktop_path( FMA_DESKTOP_PROVIDER( provider ), ( sDesktopPath * ) ip->data, messages );

		if( item ){
			items = g_list_prepend( items, item );
			fma_object_dump( item );
		}
	}

	free_desktop_paths( desktop_paths );

	g_debug( "%s: count=%d", thisfn, g_list_length( items ));
	return( items );
}

/*
 * returns a list of sDesktopPath items
 *
 * we get the ordered list of XDG_DATA_DIRS, and the ordered list of
 *  subdirs to add; then for each item of each list, we search for
 *  .desktop files in the resulted built path
 *
 * the returned list is so a list of sDesktopPath struct, in
 * the ordered of preference (most preferred first)
 */
static GList *
get_list_of_desktop_paths( FMADesktopProvider *provider, GSList **messages )
{
	GList *files;
	GSList *xdg_dirs, *idir;
	GSList *subdirs, *isub;
	gchar *dir;

	files = NULL;
	xdg_dirs = fma_desktop_xdg_dirs_get_data_dirs();
	subdirs = fma_core_utils_slist_from_split( FMA_DESKTOP_PROVIDER_SUBDIRS, G_SEARCHPATH_SEPARATOR_S );

	/* explore each directory from XDG_DATA_DIRS
	 */
	for( idir = xdg_dirs ; idir ; idir = idir->next ){

		/* explore each FMA candidate subdirectory for each XDG dir
		 */
		for( isub = subdirs ; isub ; isub = isub->next ){

			dir = g_build_filename(( gchar * ) idir->data, ( gchar * ) isub->data, NULL );
			fma_desktop_provider_add_monitor( provider, dir );
			get_list_of_desktop_files( provider, &files, dir, messages );
			g_free( dir );
		}
	}

	fma_core_utils_slist_free( subdirs );
	fma_core_utils_slist_free( xdg_dirs );

	return( files );
}

/*
 * scans the directory for .desktop files
 * only adds to the list those which have not been yet loaded
 */
static void
get_list_of_desktop_files( const FMADesktopProvider *provider, GList **files, const gchar *dir, GSList **messages )
{
	static const gchar *thisfn = "fma_desktop_reader_get_list_of_desktop_files";
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
			if( g_str_has_suffix( name, FMA_DESKTOP_FILE_SUFFIX )){
				desktop_id = fma_core_utils_str_remove_suffix( name, FMA_DESKTOP_FILE_SUFFIX );
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
is_already_loaded( const FMADesktopProvider *provider, GList *files, const gchar *desktop_id )
{
	gboolean found;
	GList *ip;
	sDesktopPath *dps;

	found = FALSE;
	for( ip = files ; ip && !found ; ip = ip->next ){
		dps = ( sDesktopPath * ) ip->data;
		if( !g_ascii_strcasecmp( dps->id, desktop_id )){
			found = TRUE;
		}
	}

	return( found );
}

static GList *
desktop_path_from_id( const FMADesktopProvider *provider, GList *files, const gchar *dir, const gchar *id )
{
	sDesktopPath *dps;
	gchar *bname;
	GList *list;

	dps = g_new0( sDesktopPath, 1 );

	bname = g_strdup_printf( "%s%s", id, FMA_DESKTOP_FILE_SUFFIX );
	dps->path = g_build_filename( dir, bname, NULL );
	g_free( bname );

	dps->id = g_strdup( id );

	list = g_list_prepend( files, dps );

	return( list );
}

/*
 * Returns a newly allocated FMAIFactoryObject-derived object, initialized
 * from the .desktop file pointed to by sDesktopPath struct
 */
static FMAIFactoryObject *
item_from_desktop_path( const FMADesktopProvider *provider, sDesktopPath *dps, GSList **messages )
{
	FMADesktopFile *ndf;

	ndf = fma_desktop_file_new_from_path( dps->path );
	if( !ndf ){
		return( NULL );
	}

	return( item_from_desktop_file( provider, ndf, messages ));
}

/*
 * Returns a newly allocated FMAIFactoryObject-derived object, initialized
 * from the .desktop file
 */
static FMAIFactoryObject *
item_from_desktop_file( const FMADesktopProvider *provider, FMADesktopFile *ndf, GSList **messages )
{
	/*static const gchar *thisfn = "fma_desktop_reader_item_from_desktop_file";*/
	FMAIFactoryObject *item;
	gchar *type;
	sReaderData *reader_data;
	gchar *id;

	item = NULL;
	type = fma_desktop_file_get_file_type( ndf );

	if( !strcmp( type, FMA_DESKTOP_VALUE_TYPE_ACTION )){
		item = FMA_IFACTORY_OBJECT( fma_object_action_new());

	} else if( !strcmp( type, FMA_DESKTOP_VALUE_TYPE_MENU )){
		item = FMA_IFACTORY_OBJECT( fma_object_menu_new());

	} else {
		/* i18n: “type” is the nature of the item: Action or Menu */
		fma_core_utils_slist_add_message( messages, _( "unknown type: %s" ), type );
	}

	if( item ){
		id = fma_desktop_file_get_id( ndf );
		fma_object_set_id( item, id );
		g_free( id );

		reader_data = g_new0( sReaderData, 1 );
		reader_data->ndf = ndf;

		fma_ifactory_provider_read_item( FMA_IFACTORY_PROVIDER( provider ), reader_data, item, messages );

		fma_object_set_provider_data( item, ndf );
		g_object_weak_ref( G_OBJECT( item ), ( GWeakNotify ) desktop_weak_notify, ndf );

		g_free( reader_data );
	}

	g_free( type );

	return( item );
}

static void
desktop_weak_notify( FMADesktopFile *ndf, GObject *item )
{
	static const gchar *thisfn = "fma_desktop_reader_desktop_weak_notify";

	g_debug( "%s: ndf=%p (%s), item=%p (%s)",
			thisfn, ( void * ) ndf, G_OBJECT_TYPE_NAME( ndf ),
			( void * ) item, G_OBJECT_TYPE_NAME( item ));

	g_object_unref( ndf );
}

static void
free_desktop_paths( GList *paths )
{
	GList *ip;
	sDesktopPath *dps;

	for( ip = paths ; ip ; ip = ip->next ){
		dps = ( sDesktopPath * ) ip->data;
		g_free( dps->path );
		g_free( dps->id );
		g_free( dps );
	}

	g_list_free( paths );
}

/**
 * fma_desktop_reader_iimporter_import_from_uri:
 * @instance: the #FMAIImporter provider.
 * @parms: a #FMAIImporterUriParms structure.
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
 *
 * Starting with FMA 3.2, we only honor the version 2 of #FMAIImporter interface,
 * thus no more checking here against possible duplicate identifiers.
 */
guint
fma_desktop_reader_iimporter_import_from_uri( const FMAIImporter *instance, void *parms_ptr )
{
	static const gchar *thisfn = "fma_desktop_reader_iimporter_import_from_uri";
	guint code;
	FMAIImporterImportFromUriParmsv2 *parms;
	FMADesktopFile *ndf;

	g_debug( "%s: instance=%p, parms=%p", thisfn, ( void * ) instance, parms_ptr );

	g_return_val_if_fail( FMA_IS_IIMPORTER( instance ), IMPORTER_CODE_PROGRAM_ERROR );
	g_return_val_if_fail( FMA_IS_DESKTOP_PROVIDER( instance ), IMPORTER_CODE_PROGRAM_ERROR );

	parms = ( FMAIImporterImportFromUriParmsv2 * ) parms_ptr;

	if( !fma_core_utils_file_is_loadable( parms->uri )){
		code = IMPORTER_CODE_NOT_LOADABLE;
		return( code );
	}

	code = IMPORTER_CODE_NOT_WILLING_TO;

	ndf = fma_desktop_file_new_from_uri( parms->uri );
	if( ndf ){
		parms->imported = ( FMAObjectItem * ) item_from_desktop_file(
				( const FMADesktopProvider * ) FMA_DESKTOP_PROVIDER( instance ),
				ndf, &parms->messages );

		if( parms->imported ){
			g_return_val_if_fail( FMA_IS_OBJECT_ITEM( parms->imported ), IMPORTER_CODE_NOT_WILLING_TO );

			/* remove the weak reference on desktop file set by 'item_from_desktop_file'
			 * as we must consider this #FMAObjectItem as a new one
			 */
			fma_object_set_provider_data( parms->imported, NULL );
			g_object_weak_unref( G_OBJECT( parms->imported ), ( GWeakNotify ) desktop_weak_notify, ndf );
			g_object_unref( ndf );

			/* also remove the 'writable' status'
			 */
			fma_object_set_readonly( parms->imported, FALSE );

			code = IMPORTER_CODE_OK;
		}
	}

	if( code == IMPORTER_CODE_NOT_WILLING_TO ){
		fma_core_utils_slist_add_message( &parms->messages, ERR_NOT_DESKTOP );
	}

	return( code );
}

/*
 * at this time, the object has been allocated and its id has been set
 * read here the subitems key, which may be 'Profiles' or 'ItemsList'
 * depending of the exact class of the FMAObjectItem
 */
void
fma_desktop_reader_ifactory_provider_read_start( const FMAIFactoryProvider *reader, void *reader_data, const FMAIFactoryObject *serializable, GSList **messages )
{
	static const gchar *thisfn = "fma_desktop_reader_ifactory_provider_read_start";

	g_return_if_fail( FMA_IS_IFACTORY_PROVIDER( reader ));
	g_return_if_fail( FMA_IS_DESKTOP_PROVIDER( reader ));
	g_return_if_fail( FMA_IS_IFACTORY_OBJECT( serializable ));

	if( !FMA_DESKTOP_PROVIDER( reader )->private->dispose_has_run ){

		g_debug( "%s: reader=%p (%s), reader_data=%p, serializable=%p (%s), messages=%p",
				thisfn,
				( void * ) reader, G_OBJECT_TYPE_NAME( reader ),
				( void * ) reader_data,
				( void * ) serializable, G_OBJECT_TYPE_NAME( serializable ),
				( void * ) messages );

		if( FMA_IS_OBJECT_ITEM( serializable )){
			read_start_read_subitems_key( reader, FMA_OBJECT_ITEM( serializable ), ( sReaderData * ) reader_data, messages );
			fma_object_set_iversion( serializable, 3 );
		}

		if( FMA_IS_OBJECT_PROFILE( serializable )){
			read_start_profile_attach_profile( reader, FMA_OBJECT_PROFILE( serializable ), ( sReaderData * ) reader_data, messages );
		}
	}
}

static void
read_start_read_subitems_key( const FMAIFactoryProvider *provider, FMAObjectItem *item, sReaderData *reader_data, GSList **messages )
{
	GSList *subitems;
	gboolean key_found;

	subitems = fma_desktop_file_get_string_list( reader_data->ndf,
			FMA_DESKTOP_GROUP_DESKTOP,
			FMA_IS_OBJECT_ACTION( item ) ? FMA_DESTOP_KEY_PROFILES : FMA_DESTOP_KEY_ITEMS_LIST,
			&key_found,
			NULL );

	if( key_found ){
		fma_object_set_items_slist( item, subitems );
	}

	fma_core_utils_slist_free( subitems );
}

static void
read_start_profile_attach_profile( const FMAIFactoryProvider *provider, FMAObjectProfile *profile, sReaderData *reader_data, GSList **messages )
{
	fma_object_attach_profile( reader_data->action, profile );
}

/*
 * reading any data from a desktop file requires:
 * - a FMADesktopFile object which has been initialized with the .desktop file
 *   -> has been attached to the FMAObjectItem in get_item() above
 * - the data type (+ reading default value)
 * - group and key names
 *
 * Returns: NULL if the key has not been found
 * letting the caller deal with default values
 */
FMADataBoxed *
fma_desktop_reader_ifactory_provider_read_data( const FMAIFactoryProvider *reader, void *reader_data, const FMAIFactoryObject *object, const FMADataDef *def, GSList **messages )
{
	static const gchar *thisfn = "fma_desktop_reader_ifactory_provider_read_data";
	FMADataBoxed *boxed;
	gboolean found;
	sReaderData *nrd;
	gchar *group, *id;
	gchar *msg;
	gchar *str_value;
	gboolean bool_value;
	GSList *slist_value;
	guint uint_value;

	g_return_val_if_fail( FMA_IS_IFACTORY_PROVIDER( reader ), NULL );
	g_return_val_if_fail( FMA_IS_DESKTOP_PROVIDER( reader ), NULL );
	g_return_val_if_fail( FMA_IS_IFACTORY_OBJECT( object ), NULL );

	boxed = NULL;

	if( !FMA_DESKTOP_PROVIDER( reader )->private->dispose_has_run ){

		nrd = ( sReaderData * ) reader_data;
		g_return_val_if_fail( FMA_IS_DESKTOP_FILE( nrd->ndf ), NULL );

		if( def->desktop_entry ){

			if( FMA_IS_OBJECT_ITEM( object )){
				group = g_strdup( FMA_DESKTOP_GROUP_DESKTOP );

			} else {
				g_return_val_if_fail( FMA_IS_OBJECT_PROFILE( object ), NULL );
				id = fma_object_get_id( object );
				group = g_strdup_printf( "%s %s", FMA_DESKTOP_GROUP_PROFILE, id );
				g_free( id );
			}

			switch( def->type ){

				case FMA_DATA_TYPE_LOCALE_STRING:
					str_value = fma_desktop_file_get_locale_string( nrd->ndf, group, def->desktop_entry, &found, def->default_value );
					if( found ){
						boxed = fma_data_boxed_new( def );
						fma_boxed_set_from_void( FMA_BOXED( boxed ), str_value );
					}
					g_free( str_value );
					break;

				case FMA_DATA_TYPE_STRING:
					str_value = fma_desktop_file_get_string( nrd->ndf, group, def->desktop_entry, &found, def->default_value );
					if( found ){
						boxed = fma_data_boxed_new( def );
						fma_boxed_set_from_void( FMA_BOXED( boxed ), str_value );
					}
					g_free( str_value );
					break;

				case FMA_DATA_TYPE_BOOLEAN:
					bool_value = fma_desktop_file_get_boolean( nrd->ndf, group, def->desktop_entry, &found, fma_core_utils_boolean_from_string( def->default_value ));
					if( found ){
						boxed = fma_data_boxed_new( def );
						fma_boxed_set_from_void( FMA_BOXED( boxed ), GUINT_TO_POINTER( bool_value ));
					}
					break;

				case FMA_DATA_TYPE_STRING_LIST:
					slist_value = fma_desktop_file_get_string_list( nrd->ndf, group, def->desktop_entry, &found, def->default_value );
					if( found ){
						boxed = fma_data_boxed_new( def );
						fma_boxed_set_from_void( FMA_BOXED( boxed ), slist_value );
					}
					fma_core_utils_slist_free( slist_value );
					break;

				case FMA_DATA_TYPE_UINT:
					uint_value = fma_desktop_file_get_uint( nrd->ndf, group, def->desktop_entry, &found, atoi( def->default_value ));
					if( found ){
						boxed = fma_data_boxed_new( def );
						fma_boxed_set_from_void( FMA_BOXED( boxed ), GUINT_TO_POINTER( uint_value ));
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
 * called when each FMAIFactoryObject object has been read
 */
void
fma_desktop_reader_ifactory_provider_read_done( const FMAIFactoryProvider *reader, void *reader_data, const FMAIFactoryObject *serializable, GSList **messages )
{
	static const gchar *thisfn = "fma_desktop_reader_ifactory_provider_read_done";
	gboolean writable;

	g_return_if_fail( FMA_IS_IFACTORY_PROVIDER( reader ));
	g_return_if_fail( FMA_IS_DESKTOP_PROVIDER( reader ));
	g_return_if_fail( FMA_IS_IFACTORY_OBJECT( serializable ));

	if( !FMA_DESKTOP_PROVIDER( reader )->private->dispose_has_run ){

		g_debug( "%s: reader=%p (%s), reader_data=%p, serializable=%p (%s), messages=%p",
				thisfn,
				( void * ) reader, G_OBJECT_TYPE_NAME( reader ),
				( void * ) reader_data,
				( void * ) serializable, G_OBJECT_TYPE_NAME( serializable ),
				( void * ) messages );

		if( FMA_IS_OBJECT_ITEM( serializable )){
			writable = read_done_item_is_writable( reader, FMA_OBJECT_ITEM( serializable ), ( sReaderData * ) reader_data, messages );
			fma_object_set_readonly( serializable, !writable );
		}

		if( FMA_IS_OBJECT_ACTION( serializable )){
			read_done_action_read_profiles( reader, FMA_OBJECT_ACTION( serializable ), ( sReaderData * ) reader_data, messages );
		}

		g_debug( "%s: quitting for %s at %p", thisfn, G_OBJECT_TYPE_NAME( serializable ), ( void * ) serializable );
	}
}

static gboolean
read_done_item_is_writable( const FMAIFactoryProvider *provider, FMAObjectItem *item, sReaderData *reader_data, GSList **messages )
{
	FMADesktopFile *ndf;
	gchar *uri;
	gboolean writable;

	ndf = reader_data->ndf;
	uri = fma_desktop_file_get_key_file_uri( ndf );
	writable = fma_desktop_utils_uri_is_writable( uri );
	g_free( uri );

	return( writable );
}

/*
 * Read and attach profiles in the specified order
 * - profiles which may exist in .desktop files, but are not referenced
 *   in the 'Profiles' string list are just ignored
 * - profiles which may be referenced in the action string list, but are not
 *   found in the .desktop file are recreated with default values (plus a warning)
 * - ensure that there is at least one profile attached to the action
 */
static void
read_done_action_read_profiles( const FMAIFactoryProvider *provider, FMAObjectAction *action, sReaderData *reader_data, GSList **messages )
{
	static const gchar *thisfn = "fma_desktop_reader_read_done_action_read_profiles";
	GSList *order;
	GSList *ip;
	gchar *profile_id;
	FMAObjectId *found;
	FMAObjectProfile *profile;

	reader_data->action = action;
	order = fma_object_get_items_slist( action );

	for( ip = order ; ip ; ip = ip->next ){
		profile_id = ( gchar * ) ip->data;
		found = fma_object_get_item( action, profile_id );
		if( !found ){
			read_done_action_load_profile( provider, reader_data, profile_id, messages );
		}
	}

	fma_core_utils_slist_free( order );

	if( !fma_object_get_items_count( action )){
		g_warning( "%s: no profile found in .desktop file", thisfn );
		profile = fma_object_profile_new_with_defaults();
		fma_object_attach_profile( action, profile );
	}
}

static void
read_done_action_load_profile( const FMAIFactoryProvider *provider, sReaderData *reader_data, const gchar *profile_id, GSList **messages )
{
	static const gchar *thisfn = "fma_desktop_reader_read_done_action_load_profile";
	FMAObjectProfile *profile;

	g_debug( "%s: loading profile=%s", thisfn, profile_id );

	profile = fma_object_profile_new_with_defaults();
	fma_object_set_id( profile, profile_id );

	if( fma_desktop_file_has_profile( reader_data->ndf, profile_id )){
		fma_ifactory_provider_read_item(
				FMA_IFACTORY_PROVIDER( provider ),
				reader_data,
				FMA_IFACTORY_OBJECT( profile ),
				messages );

	} else {
		g_warning( "%s: profile '%s' not found in .desktop file", thisfn, profile_id );
		fma_object_attach_profile( reader_data->action, profile );
	}
}
