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
#include <api/na-idata-factory-enum.h>
#include <api/na-iio-factory.h>
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

static GList          *get_list_of_desktop_paths( const NadpDesktopProvider *provider, GSList **mesages );
static void            get_list_of_desktop_files( const NadpDesktopProvider *provider, GList **files, const gchar *dir, GSList **messages );
static gboolean        is_already_loaded( const NadpDesktopProvider *provider, GList *files, const gchar *desktop_id );
static GList          *desktop_path_from_id( const NadpDesktopProvider *provider, GList *files, const gchar *dir, const gchar *id );
static NAIDataFactory *item_from_desktop_path( const NadpDesktopProvider *provider, DesktopPath *dps, GSList **messages );
#if 0
static void            read_menu_from_desktop_file( const NadpDesktopProvider *provider, NAObjectMenu *menu, NadpDesktopFile *ndf, GSList **messages );
static void            read_action_from_desktop_file( const NadpDesktopProvider *provider, NAObjectAction *action, NadpDesktopFile *ndf, GSList **messages );
static void            read_item_properties_from_ndf( const NadpDesktopProvider *provider, NAObjectItem *item, NadpDesktopFile *ndf, GSList **messages );
static void            append_profile( NAObjectAction *action, NadpDesktopFile *ndf, const gchar *profile_id, GSList **messages );
static void            exec_to_path_parameters( const gchar *command, gchar **path, gchar **parameters );
#endif
static void            free_desktop_paths( GList *paths );

/*
 * Returns an unordered list of NAIDataFactory-derived objects
 *
 * This is implementation of NAIIOProvider::read_items method
 */
GList *
nadp_iio_provider_read_items( const NAIIOProvider *provider, GSList **messages )
{
	static const gchar *thisfn = "nadp_iio_provider_read_items";
	GList *items;
	GList *desktop_paths, *ip;
	NAIDataFactory *item;

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
 * Returns a newly allocated NAIDataFactory-derived object, initialized
 * from the .desktop file pointed to by DesktopPath struct
 */
static NAIDataFactory *
item_from_desktop_path( const NadpDesktopProvider *provider, DesktopPath *dps, GSList **messages )
{
	static const gchar *thisfn = "nadp_reader_item_from_desktop_path";
	NAIDataFactory *item;
	NadpDesktopFile *ndf;
	gchar *type;
	GType reader_type;
	NadpReaderData *reader_data;
	gchar *id;

	ndf = nadp_desktop_file_new_from_path( dps->path );
	if( !ndf ){
		return( NULL );
	}

	item = NULL;
	reader_type = 0;
	type = nadp_desktop_file_get_file_type( ndf );

	if( !strcmp( type, NADP_VALUE_TYPE_MENU )){
		reader_type = NA_OBJECT_MENU_TYPE;

	} else if( !type || !strlen( type ) || !strcmp( type, NADP_VALUE_TYPE_ACTION )){
		reader_type = NA_OBJECT_ACTION_TYPE;

	} else {
		g_warning( "%s: unknown type=%s", thisfn, type );
	}

	if( reader_type ){

		reader_data = g_new0( NadpReaderData, 1 );
		reader_data->ndf = ndf;

		item = na_iio_factory_read_item( NA_IIO_FACTORY( provider ), reader_data, reader_type, messages );

		if( item ){
			id = nadp_desktop_file_get_id( ndf );
			na_object_set_id( item, id );
			g_free( id );
			na_object_set_provider_data( item, ndf );
			g_object_weak_ref( G_OBJECT( item ), ( GWeakNotify ) g_object_unref, ndf );
		}

		g_free( reader_data );
	}

	return( item );
}

#if 0
static void
read_menu_from_desktop_file( const NadpDesktopProvider *provider, NAObjectMenu *menu, NadpDesktopFile *ndf, GSList **messages )
{
	GSList *items_list;

	read_item_properties_from_ndf( provider, NA_OBJECT_ITEM( menu ), ndf, messages );

	items_list = nadp_desktop_file_get_items_list( ndf );
	na_object_item_set_items_string_list( NA_OBJECT_ITEM( menu ), items_list );
	nadp_utils_gslist_free( items_list );
}

static void
read_action_from_desktop_file( const NadpDesktopProvider *provider, NAObjectAction *action, NadpDesktopFile *ndf, GSList **messages )
{
	gboolean target_context;
	gboolean target_toolbar;
	gchar *action_label;
	gchar *toolbar_label;
	gboolean same_label;
	GSList *profiles_list, *ip;
	GSList *group_list;
	NAObjectProfile *profile;

	read_item_properties_from_ndf( provider, NA_OBJECT_ITEM( action ), ndf, messages );

	target_context = nadp_desktop_file_get_target_context( ndf );
	na_object_action_set_target_selection( action, target_context );

	target_toolbar = nadp_desktop_file_get_target_toolbar( ndf );
	na_object_action_set_target_toolbar( action, target_toolbar );

	action_label = na_object_get_label( action );
	toolbar_label = nadp_desktop_file_get_toolbar_label( ndf );
	same_label = FALSE;
	if( !toolbar_label || !g_utf8_strlen( toolbar_label, -1 ) || !g_utf8_collate( toolbar_label, action_label )){
		same_label = TRUE;
		g_free( toolbar_label );
		toolbar_label = g_strdup( action_label );
	}
	na_object_action_toolbar_set_label( action, toolbar_label );
	na_object_action_toolbar_set_same_label( action, same_label );
	g_free( toolbar_label );
	g_free( action_label );

	profiles_list = nadp_desktop_file_get_profiles_list( ndf );
	na_object_item_set_items_string_list( NA_OBJECT_ITEM( action ), profiles_list );

	/* trying to load all profiles found in .desktop files
	 * starting with those correctly recorded in profiles_list
	 * appending those not listed in the 'Profiles' entry
	 */
	group_list = nadp_desktop_file_get_profile_group_list( ndf );
	if( group_list && g_slist_length( group_list )){

		/* read profiles in the specified order
		 */
		for( ip = profiles_list ; ip ; ip = ip->next ){
			append_profile( action, ndf, ( const gchar * ) ip->data, messages );
			group_list = nadp_utils_gslist_remove_from( group_list, ( const gchar * ) ip->data );
		}

		/* append other profiles
		 * but this may be an inconvenient for the runtime plugin ?
		 */
		for( ip = group_list ; ip ; ip = ip->next ){
			append_profile( action, ndf, ( const gchar * ) ip->data, messages );
		}
	}
	nadp_utils_gslist_free( group_list );
	nadp_utils_gslist_free( profiles_list );

	/* have at least one profile */
	if( !na_object_get_items_count( action )){
		profile = na_object_profile_new();
		na_object_action_attach_profile( action, profile );
	}
}

static void
read_item_properties_from_ndf( const NadpDesktopProvider *provider, NAObjectItem *item, NadpDesktopFile *ndf, GSList **messages )
{
	static const gchar *thisfn = "nadp_read_read_item_properties_from_ndf";
	gchar *id;
	gchar *label;
	gchar *tooltip;
	gchar *icon;
	gboolean enabled;
	gboolean writable;

	na_object_set_provider_data( item, ndf );
	g_object_weak_ref( G_OBJECT( item ), ( GWeakNotify ) g_object_unref, ndf );

	id = nadp_desktop_file_get_id2( ndf );
	na_object_set_id( item, id );

	label = nadp_desktop_file_get_name( ndf );
	if( !label || !g_utf8_strlen( label, -1 )){
		g_warning( "%s: id=%s, label not found or empty", thisfn, id );
		g_free( label );
		label = g_strdup( "" );
	}
	na_object_set_label( item, label );
	g_free( label );

	tooltip = nadp_desktop_file_get_tooltip( ndf );
	na_object_set_tooltip( item, tooltip );
	g_free( tooltip );

	icon = nadp_desktop_file_get_icon( ndf );
	na_object_set_icon( item, icon );
	g_free( icon );

	/*description = nadp_desktop_file_get_description( ndf );
	na_object_set_description( item, description );
	g_free( description );*/

	/*shortcut = nadp_desktop_file_get_shortcut( ndf );
	na_object_set_shortcut( item, shortcut );
	g_free( shortcut );*/

	enabled = nadp_desktop_file_get_enabled( ndf );
	na_object_set_enabled( item, enabled );

	writable = nadp_iio_provider_is_writable( NA_IIO_PROVIDER( provider ), item );
	na_object_set_readonly( item, !writable );

	g_free( id );
}

static void
append_profile( NAObjectAction *action, NadpDesktopFile *ndf, const gchar *profile_id, GSList **messages )
{
	NAObjectProfile *profile;
	gchar *name;
	gchar *exec, *path, *parameters;
	GSList *basenames, *mimetypes, *schemes, *folders;
	gboolean matchcase;
	gboolean isfile, isdir;
	gboolean multiple;

	profile = na_object_profile_new();
	na_object_set_id( profile, profile_id );

	name = nadp_desktop_file_get_profile_name( ndf, profile_id );
	na_object_set_label( profile, name );
	g_free( name );

	exec = nadp_desktop_file_get_profile_exec( ndf, profile_id );
	exec_to_path_parameters( exec, &path, &parameters );
	na_object_profile_set_path( profile, path );
	na_object_profile_set_parameters( profile, parameters );
	g_free( parameters );
	g_free( path );
	g_free( exec );

	basenames = nadp_desktop_file_get_basenames( ndf, profile_id );
	na_object_profile_set_basenames( profile, basenames );
	nadp_utils_gslist_free( basenames );

	matchcase = nadp_desktop_file_get_matchcase( ndf, profile_id );
	na_object_profile_set_matchcase( profile, matchcase );

	mimetypes = nadp_desktop_file_get_mimetypes( ndf, profile_id );
	na_object_profile_set_mimetypes( profile, mimetypes );
	nadp_utils_gslist_free( mimetypes );

	schemes = nadp_desktop_file_get_schemes( ndf, profile_id );
	na_object_profile_set_schemes( profile, schemes );
	nadp_utils_gslist_free( schemes );

	folders = nadp_desktop_file_get_folders( ndf, profile_id );
	na_object_profile_set_folders( profile, folders );
	nadp_utils_gslist_free( folders );

	isfile = TRUE;
	isdir = TRUE;
	na_object_profile_set_isfiledir( profile, isfile, isdir );

	multiple = TRUE;
	na_object_profile_set_multiple( profile, multiple );

	na_object_action_attach_profile( action, profile );
}

/*
 * suppose here that command is only one word,
 * also 'normalize' space characters (bad, but temporary!)
 */
static void
exec_to_path_parameters( const gchar *command, gchar **path, gchar **parameters )
{
	gchar **tokens, **iter;
	gchar *tmp;
	gchar *source = g_strdup( command );
	GString *string;

	tmp = g_strstrip( source );
	if( tmp && g_utf8_strlen( tmp, -1 )){

		tokens = g_strsplit_set( tmp, " ", -1 );
		*path = g_strdup( *tokens );

		iter = tokens;
		iter++;
		string = g_string_new( "" );
		while( *iter ){
			if( !string->len ){
				string = g_string_append( string, " " );
			}
			string = g_string_append( string, g_strstrip( *iter ));
			iter++;
		}
		*parameters = g_string_free( string, FALSE );
		g_strfreev( tokens );

	} else {
		*path = g_strdup( "" );
		*parameters = g_strdup( "" );
	}

	g_free( source );
}
#endif

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
