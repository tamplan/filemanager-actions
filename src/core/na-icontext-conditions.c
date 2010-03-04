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

#include <libnautilus-extension/nautilus-file-info.h>

#include <api/na-core-utils.h>
#include <api/na-object-api.h>

#include "na-icontext-conditions.h"
#include "na-dbus-tracker.h"
#include "na-gnome-vfs-uri.h"

/* private interface data
 */
struct NAIContextConditionsInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

static gboolean st_initialized = FALSE;
static gboolean st_finalized   = FALSE;

static GType    register_type( void );
static void     interface_base_init( NAIContextConditionsInterface *klass );
static void     interface_base_finalize( NAIContextConditionsInterface *klass );

static gboolean is_target_background_candidate( const NAIContextConditions *profile, NautilusFileInfo *current_folder );
static gboolean is_target_toolbar_candidate( const NAIContextConditions *profile, NautilusFileInfo *current_folder );
static gboolean is_current_folder_inside( const NAIContextConditions *profile, NautilusFileInfo *current_folder );
static gboolean is_target_selection_candidate( const NAIContextConditions *profile, GList *files, gboolean from_nautilus );

static gboolean tracked_is_directory( void *iter, gboolean from_nautilus );
static gchar   *tracked_to_basename( void *iter, gboolean from_nautilus );
static gchar   *tracked_to_mimetype( void *iter, gboolean from_nautilus );
static gchar   *tracked_to_scheme( void *iter, gboolean from_nautilus );
static int      validate_schemes( GSList *schemes2test, void *iter, gboolean from_nautilus );

/**
 * na_icontext_conditions_get_type:
 *
 * Returns: the #GType type of this interface.
 */
GType
na_icontext_conditions_get_type( void )
{
	static GType type = 0;

	if( !type ){
		type = register_type();
	}

	return( type );
}

/*
 * na_icontext_conditions_register_type:
 *
 * Registers this interface.
 */
static GType
register_type( void )
{
	static const gchar *thisfn = "na_icontext_conditions_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NAIContextConditionsInterface ),
		( GBaseInitFunc ) interface_base_init,
		( GBaseFinalizeFunc ) interface_base_finalize,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_INTERFACE, "NAIContextConditions", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( NAIContextConditionsInterface *klass )
{
	static const gchar *thisfn = "na_icontext_conditions_interface_base_init";

	if( !st_initialized ){

		g_debug( "%s: klass%p (%s)", thisfn, ( void * ) klass, G_OBJECT_CLASS_NAME( klass ));

		klass->private = g_new0( NAIContextConditionsInterfacePrivate, 1 );

		st_initialized = TRUE;
	}
}

static void
interface_base_finalize( NAIContextConditionsInterface *klass )
{
	static const gchar *thisfn = "na_icontext_conditions_interface_base_finalize";

	if( st_initialized && !st_finalized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		st_finalized = TRUE;

		g_free( klass->private );
	}
}

/**
 * na_object_profile_is_candidate:
 * @profile: the #NAObjectProfile to be checked.
 * @target: the current target.
 * @files: the currently selected items, as provided by Nautilus.
 *
 * Determines if the given profile is candidate to be displayed in the
 * Nautilus context menu, regarding the list of currently selected
 * items.
 *
 * Returns: %TRUE if this profile succeeds to all tests and is so a
 * valid candidate to be displayed in Nautilus context menu, %FALSE
 * else.
 *
 * This method could have been left outside of the #NAObjectProfile
 * class, as it is only called by the plugin. Nonetheless, it is much
 * more easier to code here (because we don't need all get methods, nor
 * free the parameters after).
 */
gboolean
na_icontext_conditions_is_candidate( const NAIContextConditions *object, guint target, GList *files )
{
	gboolean is_candidate;

	g_return_val_if_fail( NA_IS_ICONTEXT_CONDITIONS( object ), FALSE );

	if( !na_object_is_valid( object )){
		return( FALSE );
	}

	is_candidate = FALSE;

	switch( target ){
		case ITEM_TARGET_BACKGROUND:
			is_candidate = is_target_background_candidate( object, ( NautilusFileInfo * ) files->data );
			break;

		case ITEM_TARGET_TOOLBAR:
			is_candidate = is_target_toolbar_candidate( object, ( NautilusFileInfo * ) files->data );
			break;

		case ITEM_TARGET_SELECTION:
		default:
			is_candidate = is_target_selection_candidate( object, files, TRUE );
	}

	return( is_candidate );
}

static gboolean
is_target_background_candidate( const NAIContextConditions *object, NautilusFileInfo *current_folder )
{
	gboolean is_candidate;

	is_candidate = is_current_folder_inside( object, current_folder );

	return( is_candidate );
}

static gboolean
is_target_toolbar_candidate( const NAIContextConditions *object, NautilusFileInfo *current_folder )
{
	gboolean is_candidate;

	is_candidate = is_current_folder_inside( object, current_folder );

	return( is_candidate );
}

static gboolean
is_current_folder_inside( const NAIContextConditions *object, NautilusFileInfo *current_folder )
{
	gboolean is_inside;
	GSList *folders, *ifold;
	const gchar *path;
	gchar *current_folder_uri;

	is_inside = FALSE;
	current_folder_uri = nautilus_file_info_get_uri( current_folder );
	folders = na_object_get_folders( object );

	for( ifold = folders ; ifold && !is_inside ; ifold = ifold->next ){
		path = ( const gchar * ) ifold->data;
		if( path && g_utf8_strlen( path, -1 )){
			if( !strcmp( path, "*" )){
				is_inside = TRUE;
			} else {
				is_inside = g_str_has_prefix( current_folder_uri, path );
				g_debug( "na_object_object_is_current_folder_inside: current_folder_uri=%s, path=%s, is_inside=%s", current_folder_uri, path, is_inside ? "True":"False" );
			}
		}
	}

	na_core_utils_slist_free( folders );
	g_free( current_folder_uri );

	return( is_inside );
}

static gboolean
is_target_selection_candidate( const NAIContextConditions *object, GList *files, gboolean from_nautilus )
{
	gboolean retv = FALSE;
	GSList *basenames, *mimetypes, *schemes;
	gboolean matchcase, multiple, isdir, isfile;
	gboolean test_multiple_file = FALSE;
	gboolean test_file_type = FALSE;
	gboolean test_scheme = FALSE;
	gboolean test_basename = FALSE;
	gboolean test_mimetype = FALSE;
	GList* glob_patterns = NULL;
	GList* glob_mime_patterns = NULL;
	GSList* iter;
	GList* iter1;
	GList* iter2;
	guint dir_count = 0;
	guint file_count = 0;
	guint total_count = 0;
	guint scheme_ok_count = 0;
	guint glob_ok_count = 0;
	guint mime_glob_ok_count = 0;
	gboolean basename_match_ok = FALSE;
	gboolean mimetype_match_ok = FALSE;
	gchar *tmp_pattern, *tmp_filename, *tmp_filename2, *tmp_mimetype, *tmp_mimetype2;

	basenames = na_object_get_basenames( object );
	matchcase = na_object_is_matchcase( object );
	multiple = na_object_is_multiple( object );
	isdir = na_object_is_dir( object );
	isfile = na_object_is_file( object );
	mimetypes = na_object_get_mimetypes( object );
	schemes = na_object_get_schemes( object );

	if( basenames && basenames->next != NULL &&
			g_ascii_strcasecmp(( gchar * )( basenames->data ), "*" ) == 0 ){
		/* if the only pattern is '*' then all files will match, so it
		 * is not necessary to make the test for each of them
		 */
		test_basename = TRUE;

	} else {
		for (iter = basenames ; iter ; iter = iter->next ){

			tmp_pattern = ( gchar * ) iter->data;
			if( !matchcase ){
				/* --> if case-insensitive asked, lower all the string
				 * since the pattern matching function don't manage it
				 * itself.
				 */
				tmp_pattern = g_ascii_strdown(( gchar * ) iter->data, strlen(( gchar * ) iter->data ));
			}

			glob_patterns = g_list_append( glob_patterns, g_pattern_spec_new( tmp_pattern ));

			if( !matchcase ){
				g_free( tmp_pattern );
			}
		}
	}

	if( mimetypes && mimetypes->next != NULL &&
			( g_ascii_strcasecmp(( gchar * )( mimetypes->data ), "*" ) == 0 ||
			  g_ascii_strcasecmp(( gchar * )( mimetypes->data), "*/*") == 0 )){
		/* if the only pattern is '*' or * / * then all mimetypes will
		 * match, so it is not necessary to make the test for each of them
		 */
		test_mimetype = TRUE;

	} else {
		for( iter = mimetypes ; iter ; iter = iter->next ){
			glob_mime_patterns = g_list_append( glob_mime_patterns, g_pattern_spec_new(( gchar * ) iter->data ));
		}
	}

	for( iter1 = files; iter1; iter1 = iter1->next ){

		tmp_filename = tracked_to_basename( iter1->data, from_nautilus );

		if( tmp_filename ){
			tmp_mimetype = tracked_to_mimetype( iter1->data, from_nautilus );

			if( !matchcase ){
				/* --> if case-insensitive asked, lower all the string
				 * since the pattern matching function don't manage it
				 * itself.
				 */
				tmp_filename2 = g_ascii_strdown( tmp_filename, strlen( tmp_filename ));
				g_free( tmp_filename );
				tmp_filename = tmp_filename2;
			}

			/* --> for the moment we deal with all mimetypes case-insensitively */
			tmp_mimetype2 = g_ascii_strdown( tmp_mimetype, strlen( tmp_mimetype ));
			g_free( tmp_mimetype );
			tmp_mimetype = tmp_mimetype2;

			if( tracked_is_directory( iter1->data, from_nautilus )){
				dir_count++;
			} else {
				file_count++;
			}

			scheme_ok_count += validate_schemes( schemes, iter1->data, from_nautilus );

			if( !test_basename ){ /* if it is already ok, skip the test to improve performance */
				basename_match_ok = FALSE;
				iter2 = glob_patterns;
				while( iter2 && !basename_match_ok ){
					if( g_pattern_match_string(( GPatternSpec * ) iter2->data, tmp_filename )){
						basename_match_ok = TRUE;
					}
					iter2 = iter2->next;
				}

				if( basename_match_ok ){
					glob_ok_count++;
				}
			}

			if( !test_mimetype ){ /* if it is already ok, skip the test to improve performance */
				mimetype_match_ok = FALSE;
				iter2 = glob_mime_patterns;
				while( iter2 && !mimetype_match_ok ){
					if (g_pattern_match_string(( GPatternSpec * ) iter2->data, tmp_mimetype )){
						mimetype_match_ok = TRUE;
					}
					iter2 = iter2->next;
				}

				if( mimetype_match_ok ){
					mime_glob_ok_count++;
				}
			}

			g_free( tmp_mimetype );
			g_free( tmp_filename );
		}

		total_count++;
	}

	if(( files != NULL ) && ( files->next == NULL ) && ( !multiple )){
		test_multiple_file = TRUE;

	} else if( multiple ){
		test_multiple_file = TRUE;
	}

	if( isdir && isfile ){
		if( dir_count > 0 || file_count > 0 ){
			test_file_type = TRUE;
		}
	} else if( isdir && !isfile ){
		if( file_count == 0 ){
			test_file_type = TRUE;
		}
	} else if( !isdir && isfile ){
		if( dir_count == 0 ){
			test_file_type = TRUE;
		}
	}

	if( scheme_ok_count == total_count ){
		test_scheme = TRUE;
	}

	if( !test_basename ){ /* if not already tested */
		if( glob_ok_count == total_count ){
			test_basename = TRUE;
		}
	}

	if( !test_mimetype ){ /* if not already tested */
		if( mime_glob_ok_count == total_count ){
			test_mimetype = TRUE;
		}
	}

	if( test_basename && test_mimetype && test_file_type && test_scheme && test_multiple_file ){
		retv = TRUE;
	}

	g_list_foreach (glob_patterns, (GFunc) g_pattern_spec_free, NULL);
	g_list_free (glob_patterns);
	g_list_foreach (glob_mime_patterns, (GFunc) g_pattern_spec_free, NULL);
	g_list_free (glob_mime_patterns);
	na_core_utils_slist_free( schemes );
	na_core_utils_slist_free( mimetypes );
	na_core_utils_slist_free( basenames );

	return retv;
}

static gboolean
tracked_is_directory( void *iter, gboolean from_nautilus )
{
	gboolean is_dir;
	GFile *file;
	GFileType type;

	if( from_nautilus ){
		is_dir = nautilus_file_info_is_directory(( NautilusFileInfo * ) iter );

	} else {
		file = g_file_new_for_uri((( NATrackedItem * ) iter )->uri );
		type = g_file_query_file_type( file, G_FILE_QUERY_INFO_NONE, NULL );
		is_dir = ( type == G_FILE_TYPE_DIRECTORY );
		g_object_unref( file );
	}

	return( is_dir );
}

static gchar *
tracked_to_basename( void *iter, gboolean from_nautilus )
{
	gchar *bname;
	GFile *file;

	if( from_nautilus ){
		bname = nautilus_file_info_get_name(( NautilusFileInfo * ) iter );

	} else {
		file = g_file_new_for_uri((( NATrackedItem * ) iter )->uri );
		bname = g_file_get_basename( file );
		g_object_unref( file );
	}

	return( bname );
}

static gchar *
tracked_to_mimetype( void *iter, gboolean from_nautilus )
{
	gchar *type;
	NATrackedItem *tracked;
	GFile *file;
	GFileInfo *info;

	type = NULL;
	if( from_nautilus ){
		type = nautilus_file_info_get_mime_type(( NautilusFileInfo * ) iter );

	} else {
		tracked = ( NATrackedItem * ) iter;
		if( tracked->mimetype ){
			type = g_strdup( tracked->mimetype );

		} else {
			file = g_file_new_for_uri((( NATrackedItem * ) iter )->uri );
			info = g_file_query_info( file, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, G_FILE_QUERY_INFO_NONE, NULL, NULL );
			if( info ){
				type = g_strdup( g_file_info_get_content_type( info ));
				g_object_unref( info );
			}
			g_object_unref( file );
		}
	}

	return( type );
}

static gchar *
tracked_to_scheme( void *iter, gboolean from_nautilus )
{
	gchar *scheme;
	NAGnomeVFSURI *vfs;

	if( from_nautilus ){
		scheme = nautilus_file_info_get_uri_scheme(( NautilusFileInfo * ) iter );

	} else {
		vfs = g_new0( NAGnomeVFSURI, 1 );
		na_gnome_vfs_uri_parse( vfs, (( NATrackedItem * ) iter )->uri );
		scheme = g_strdup( vfs->scheme );
		na_gnome_vfs_uri_free( vfs );
	}

	return( scheme );
}

static int
validate_schemes( GSList* schemes2test, void* tracked_iter, gboolean from_nautilus )
{
	int retv = 0;
	GSList* iter;
	gboolean found = FALSE;
	gchar *scheme;

	iter = schemes2test;
	while( iter && !found ){

		scheme = tracked_to_scheme( tracked_iter, from_nautilus );

		if( g_ascii_strncasecmp( scheme, ( gchar * ) iter->data, strlen(( gchar * ) iter->data )) == 0 ){
			found = TRUE;
			retv = 1;
		}

		g_free( scheme );
		iter = iter->next;
	}

	return retv;
}
