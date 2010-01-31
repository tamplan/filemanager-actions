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

#include <errno.h>
#include <gio/gio.h>
#include <glib/gstdio.h>
#include <uuid/uuid.h>

#include "nadp-desktop-provider.h"
#include "nadp-utils.h"

static GSList *text_to_string_list( const gchar *text, const gchar *separator, const gchar *default_value );

/**
 * nadp_utils_split_path_list:
 * @path_list: a ':'-separated list of paths.
 *
 * Returns: an ordered GSList of paths, as newly allocated strings.
 *
 * The returned GSList should be freed by the caller.
 */
GSList *
nadp_utils_split_path_list( const gchar *path_list )
{
	return( text_to_string_list( path_list, ":", NULL ));
}

/*
 * split a text buffer in lines
 */
static GSList *
text_to_string_list( const gchar *text, const gchar *separator, const gchar *default_value )
{
	GSList *strlist = NULL;
	gchar **tokens;
	gchar *tmp;
	gchar *source = g_strdup( text );

	tmp = g_strstrip( source );
	if( !g_utf8_strlen( tmp, -1 ) && default_value ){
		strlist = g_slist_append( strlist, g_strdup( default_value ));

	} else {
		tokens = g_strsplit( tmp, separator, -1 );
		strlist = nadp_utils_to_slist(( const gchar ** ) tokens );
		g_strfreev( tokens );
	}

	g_free( source );
	return( strlist );
}

/**
 * nadp_utils_to_slist:
 * @list: a gchar ** list of strings.
 *
 * Returns: a #GSList.
 */
GSList *
nadp_utils_to_slist( const gchar **list )
{
	GSList *strlist = NULL;
	gchar **iter;
	gchar *tmp;

	iter = ( gchar ** ) list;

	while( *iter ){
		tmp = g_strstrip( *iter );
		strlist = g_slist_append( strlist, g_strdup( tmp ));
		iter++;
	}

	return( strlist );
}

/**
 * nadp_utils_gslist_free:
 * @list: the GSList to be freed.
 *
 * Frees a GSList of strings.
 */
void
nadp_utils_gslist_free( GSList *list )
{
	g_slist_foreach( list, ( GFunc ) g_free, NULL );
	g_slist_free( list );
}

/**
 * nadp_utils_gslist_remove_from:
 * @list: the #GSList from which remove the @string.
 * @string: the string to be removed.
 *
 * Removes a @string from a string list, then frees the removed @string.
 */
GSList *
nadp_utils_gslist_remove_from( GSList *list, const gchar *string )
{
	GSList *is;

	for( is = list ; is ; is = is->next ){
		const gchar *istr = ( const gchar * ) is->data;
		if( !g_utf8_collate( string, istr )){
			g_free( is->data );
			list = g_slist_delete_link( list, is );
			break;
		}
	}

	return( list );
}

/**
 * nadp_utils_remove_suffix:
 * @string: source string.
 * @suffix: suffix to be removed from @string.
 *
 * Returns: a newly allocated string, which is a copy of the source @string,
 * minus the removed @suffix if present. If @strings doesn't terminate with
 * @suffix, then the returned string is equal to source @string.
 *
 * The returned string should be g_free() by the caller.
 */
gchar *
nadp_utils_remove_suffix( const gchar *string, const gchar *suffix )
{
	gchar *removed;
	gchar *ptr;

	removed = g_strdup( string );

	if( g_str_has_suffix( string, suffix )){
		ptr = g_strrstr( removed, suffix );
		ptr[0] = '\0';
	}

	return( removed );
}

/**
 * nadp_utils_is_writable_dir:
 * @path: the path of the directory to be tested.
 *
 * Returns: %TRUE if the directory is writable, %FALSE else.
 *
 * Please note that this type of test is subject to race conditions,
 * as the directory may become unwritable after a successfull test,
 * but before the caller has been able to actually write into it.
 *
 * There is no "super-test". Just try...
 */
gboolean
nadp_utils_is_writable_dir( const gchar *path )
{
	static const gchar *thisfn = "nadp_utils_is_writable_dir";
	GFile *file;
	GError *error = NULL;
	GFileInfo *info;
	GFileType type;
	gboolean writable;

	if( !path || !g_utf8_strlen( path, -1 )){
		return( FALSE );
	}

	file = g_file_new_for_path( path );
	info = g_file_query_info( file,
			G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE "," G_FILE_ATTRIBUTE_STANDARD_TYPE,
			G_FILE_QUERY_INFO_NONE, NULL, &error );

	if( error ){
		g_warning( "%s: g_file_query_info error: %s", thisfn, error->message );
		g_error_free( error );
		g_object_unref( file );
		return( FALSE );
	}

	type = g_file_info_get_file_type( info );
	if( type != G_FILE_TYPE_DIRECTORY ){
		g_debug( "%s: %s is not a directory", thisfn, path );
		g_object_unref( info );
		return( FALSE );
	}

	writable = g_file_info_get_attribute_boolean( info, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE );
	if( !writable ){
		g_debug( "%s: %s is not writable", thisfn, path );
	}
	g_object_unref( info );

	return( writable );
}

/**
 * nadp_utils_path2id:
 * @path: a full pathname.
 *
 * Returns: the id of the file, as a newly allocated string which
 * should be g_free() by the caller.
 *
 * The id of the file is equal to the basename, minus the suffix.
 */
gchar *
nadp_utils_path2id( const gchar *path )
{
	gchar *bname;
	gchar *id;

	bname = g_path_get_basename( path );
	id = nadp_utils_remove_suffix( bname, NADP_DESKTOP_SUFFIX );
	g_free( bname );

	return( id );
}

/**
 * nadp_utils_is_writable_file:
 * @path: the path of the file to be tested.
 *
 * Returns: %TRUE if the file is writable, %FALSE else.
 *
 * Please note that this type of test is subject to race conditions,
 * as the file may become unwritable after a successfull test,
 * but before the caller has been able to actually write into it.
 *
 * There is no "super-test". Just try...
 */
gboolean
nadp_utils_is_writable_file( const gchar *path )
{
	static const gchar *thisfn = "nadp_utils_is_writable_file";
	GFile *file;
	GError *error = NULL;
	GFileInfo *info;
	gboolean writable;

	if( !path || !g_utf8_strlen( path, -1 )){
		return( FALSE );
	}

	file = g_file_new_for_path( path );
	info = g_file_query_info( file,
			G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE "," G_FILE_ATTRIBUTE_STANDARD_TYPE,
			G_FILE_QUERY_INFO_NONE, NULL, &error );

	if( error ){
		g_warning( "%s: g_file_query_info error: %s", thisfn, error->message );
		g_error_free( error );
		g_object_unref( file );
		return( FALSE );
	}

	writable = g_file_info_get_attribute_boolean( info, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE );
	if( !writable ){
		g_debug( "%s: %s is not writable", thisfn, path );
	}
	g_object_unref( info );

	return( writable );
}

/**
 * nadp_utils_delete_file:
 * @path: the path of the file to be deleted.
 *
 * Returns: %TRUE if the file is successfully deleted, %FALSE else.
 */
gboolean
nadp_utils_delete_file( const gchar *path )
{
	static const gchar *thisfn = "nadp_utils_delete_file";
	gboolean deleted = FALSE;

	if( !path || !g_utf8_strlen( path, -1 )){
		return( FALSE );
	}

	if( g_unlink( path ) == 0 ){
		deleted = TRUE;
	} else {
		g_warning( "%s: %s: %s", thisfn, path, g_strerror( errno ));
	}

	return( deleted );
}
