/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009 Pierre Wieser and others (see AUTHORS)
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

#include <nautilus-actions/api/na-object-api.h>

#include "nadp-desktop-file.h"
#include "nadp-desktop-provider.h"
#include "nadp-read.h"
#include "nadp-utils.h"
#include "nadp-xdg-data-dirs.h"

typedef struct {
	gchar *path;
	gchar *id;
}
	DesktopPath;

static GList          *get_list_of_desktop_paths( const NadpDesktopProvider *provider, GSList **mesages );
static void            get_list_of_desktop_files( const NadpDesktopProvider *provider, GList **files, const gchar *dir, GSList **messages );
static gboolean        is_already_loaded( const NadpDesktopProvider *provider, GList *files, const gchar *desktop_id );
static GList          *desktop_path_from_id( const NadpDesktopProvider *provider, GList *files, const gchar *dir, const gchar *id );
static NAObjectAction *action_from_desktop_path( const NadpDesktopProvider *provider, DesktopPath *dps, GSList **messages );
static void            read_action_properties( const NadpDesktopProvider *provider, NAObjectAction *action, NadpDesktopFile *ndf, GSList **messages );
static void            read_item_properties( const NadpDesktopProvider *provider, NAObjectItem *item, NadpDesktopFile *ndf, GSList **messages );
static void            free_desktop_paths( GList *paths );

/*
 * Returns an unordered list of NAObjectItem-derived objects
 */
GList *
nadp_iio_provider_read_items( const NAIIOProvider *provider, GSList **messages )
{
	static const gchar *thisfn = "nadp_read_iio_provider_read_items";
	GList *items;
	GList *paths, *ip;
	DesktopPath *dps;
	NAObjectAction *action;

	g_debug( "%s: provider=%p, messages=%p", thisfn, ( void * ) provider, ( void * ) messages );

	items = NULL;

	paths = get_list_of_desktop_paths( NADP_DESKTOP_PROVIDER( provider ), messages );
	for( ip = paths ; ip ; ip = ip->next ){
		dps = ( DesktopPath * ) ip->data;
		action = action_from_desktop_path( NADP_DESKTOP_PROVIDER( provider ), dps, messages );
		if( action ){
			items = g_list_prepend( items, action );
		}
	}

	free_desktop_paths( paths );

	return( items );
}

/*
 * returns a list of DesktopPath items
 */
static GList *
get_list_of_desktop_paths( const NadpDesktopProvider *provider, GSList **messages )
{
	GList *files;
	GSList *xdg_dirs, *idir;
	GSList *subdirs, *isub;
	gchar *dir;

	files = NULL;
	xdg_dirs = nadp_xdg_data_dirs_get_dirs( provider, messages );
	subdirs = nadp_utils_split_path_list( NADP_DESKTOP_PROVIDER_SUBDIRS );

	for( idir = xdg_dirs ; idir ; idir = idir->next ){
		for( isub = subdirs ; isub ; isub = isub->next ){
			dir = g_build_filename(( gchar * ) idir->data, ( gchar * ) isub->data, NULL );
			get_list_of_desktop_files( provider, &files, dir, messages );
			g_free( dir );
		}
	}

	nadp_utils_gslist_free( subdirs );
	nadp_utils_gslist_free( xdg_dirs );

	return( files );
}

/*
 * scans the directory for a list of not yet loaded .desktop files
 */
static void
get_list_of_desktop_files( const NadpDesktopProvider *provider, GList **files, const gchar *dir, GSList **messages )
{
	static const gchar *thisfn = "nadp_read_get_list_of_desktop_files";
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
			if( g_str_has_suffix( name, NADP_DESKTOP_SUFFIX )){
				desktop_id = nadp_utils_remove_suffix( name, NADP_DESKTOP_SUFFIX );
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

	bname = g_strdup_printf( "%s%s", id, NADP_DESKTOP_SUFFIX );
	dps->path = g_build_filename( dir, bname, NULL );
	g_free( bname );

	dps->id = g_strdup( id );

	list = g_list_append( files, dps );

	return( list );
}

/*
 * returns a newly allocated NAObjectAction object, initialized
 * from the .desktop file pointed to by DesktopPath struct
 */
static NAObjectAction *
action_from_desktop_path( const NadpDesktopProvider *provider, DesktopPath *dps, GSList **messages )
{
	NadpDesktopFile *ndf;
	NAObjectAction *action;

	ndf = nadp_desktop_file_new_from_path( dps->path );
	if( !ndf ){
		return( NULL );
	}

	action = na_object_action_new();
	read_action_properties( provider, action, ndf, messages );

	g_object_set_data( G_OBJECT( action ), "nadp-desktop-file", ndf );
	g_object_weak_ref( G_OBJECT( action ), ( GWeakNotify ) g_object_unref, ndf );

	return( action );
}

static void
read_action_properties( const NadpDesktopProvider *provider, NAObjectAction *action, NadpDesktopFile *ndf, GSList **messages )
{
	read_item_properties( provider, NA_OBJECT_ITEM( action ), ndf, messages );
}

static void
read_item_properties( const NadpDesktopProvider *provider, NAObjectItem *item, NadpDesktopFile *ndf, GSList **messages )
{
	static const gchar *thisfn = "nadp_read_read_item_properties";
	gchar *id;
	gchar *label;
	gchar *tooltip;
	gchar *path;
	gboolean writable;

	id = nadp_desktop_file_get_id( ndf );

	label = ( gchar * ) nadp_desktop_file_get_label( ndf );
	if( !label || !g_utf8_strlen( label, -1 )){
		g_warning( "%s: id=%s, label not found or empty", thisfn, id );
		g_free( label );
		label = g_strdup( "" );
	}
	na_object_set_label( item, label );
	g_free( label );

	tooltip = ( gchar * ) nadp_desktop_file_get_tooltip( ndf );
	na_object_set_tooltip( item, tooltip );
	g_free( tooltip );

	path = nadp_desktop_file_get_key_file_path( ndf );
	writable = nadp_utils_is_writable_file( path );
	g_free( path );
	na_object_set_readonly( item, !writable );

	g_free( id );
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
