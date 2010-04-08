/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010 Pierre Wieser and others (see AUTHORS)
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
#include <api/na-ifactory-object-data.h>
#include <api/na-ifactory-provider.h>
#include <api/na-object-api.h>

#include "nadp-desktop-provider.h"
#include "nadp-keys.h"
#include "nadp-reader.h"
#include "nadp-xdg-dirs.h"

typedef struct {
	gchar *path;
	gchar *id;
}
	DesktopPath;

static GList            *get_list_of_desktop_paths( const NadpDesktopProvider *provider, GSList **mesages );
static void              get_list_of_desktop_files( const NadpDesktopProvider *provider, GList **files, const gchar *dir, GSList **messages );
static gboolean          is_already_loaded( const NadpDesktopProvider *provider, GList *files, const gchar *desktop_id );
static GList            *desktop_path_from_id( const NadpDesktopProvider *provider, GList *files, const gchar *dir, const gchar *id );
static NAIFactoryObject *item_from_desktop_path( const NadpDesktopProvider *provider, DesktopPath *dps, GSList **messages );
static void              desktop_weak_notify( NadpDesktopFile *ndf, GObject *item );
static void              free_desktop_paths( GList *paths );

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

	desktop_paths = get_list_of_desktop_paths( NADP_DESKTOP_PROVIDER( provider ), messages );
	for( ip = desktop_paths ; ip ; ip = ip->next ){

		item = item_from_desktop_path( NADP_DESKTOP_PROVIDER( provider ), ( DesktopPath * ) ip->data, messages );

		if( item ){
			items = g_list_prepend( items, item );
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
get_list_of_desktop_paths( const NadpDesktopProvider *provider, GSList **messages )
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

		/* explore chaque N-A candidate subdirectory for each XDG dir
		 */
		for( isub = subdirs ; isub ; isub = isub->next ){

			dir = g_build_filename(( gchar * ) idir->data, ( gchar * ) isub->data, NULL );
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
	static const gchar *thisfn = "nadp_reader_item_from_desktop_path";
	NAIFactoryObject *item;
	NadpDesktopFile *ndf;
	gchar *type;
	NadpReaderData *reader_data;
	gchar *id;

	ndf = nadp_desktop_file_new_from_path( dps->path );
	if( !ndf ){
		return( NULL );
	}

	item = NULL;
	type = nadp_desktop_file_get_file_type( ndf );

	if( !type || !strlen( type ) || !strcmp( type, NADP_VALUE_TYPE_ACTION )){
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
