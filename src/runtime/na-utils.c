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

#include <gio/gio.h>
#include <glib-object.h>
#include <string.h>

#include "na-utils.h"

/**
 * na_utils_duplicate_string_list:
 * @list: the GSList to be duplicated.
 *
 * Returns: a #GSList of strings.
 *
 * The returned list should be na_utils_free_string_list() by the caller.
 */
GSList *
na_utils_duplicate_string_list( GSList *list )
{
	GSList *duplist = NULL;
	GSList *it;

	for( it = list ; it != NULL ; it = it->next ){
		gchar *dupstr = g_strdup(( gchar * ) it->data );
		duplist = g_slist_prepend( duplist, dupstr );
	}
	duplist = g_slist_reverse( duplist );

	return( duplist );
}

/**
 * na_utils_find_in_list:
 * @list: the GSList of strings to be searched.
 * @str: the searched string.
 *
 * Search for a string in a string list.
 *
 * Returns: %TRUE if the string has been found in list.
 */
gboolean
na_utils_find_in_list( GSList *list, const gchar *str )
{
	GSList *il;

	for( il = list ; il ; il = il->next ){
		const gchar *istr = ( const gchar * ) il->data;
		if( !g_utf8_collate( str, istr )){
			return( TRUE );
		}
	}

	return( FALSE );
}

/**
 * na_utils_free_string_list:
 * @list: the GSList to be freed.
 *
 * Frees a GSList of strings.
 */
void
na_utils_free_string_list( GSList *list )
{
	g_slist_foreach( list, ( GFunc ) g_free, NULL );
	g_slist_free( list );
}

/**
 * na_utils_remove_from_string_list:
 * @list: the GSList to be updated.
 * @str: the string to be removed.
 *
 * Removes from the @list the item which has a string which is equal to
 * @str.
 *
 * Returns: the new @list start position.
 */
GSList *
na_utils_remove_from_string_list( GSList *list, const gchar *str )
{
	GSList *is;

	for( is = list ; is ; is = is->next ){
		const gchar *istr = ( const gchar * ) is->data;
		if( !g_utf8_collate( str, istr )){
			g_free( is->data );
			list = g_slist_delete_link( list, is );
			break;
		}
	}

	return( list );
}

/**
 * na_utils_string_lists_are_equal:
 * @first: a GSList of strings.
 * @second: another GSList of strings to be compared with @first.
 *
 * Compare two string lists, without regards to the order.
 *
 * Returns: %TRUE if the two lists have same content.
 */
gboolean
na_utils_string_lists_are_equal( GSList *first, GSList *second )
{
	GSList *il;

	for( il = first ; il ; il = il->next ){
		const gchar *str = ( const gchar * ) il->data;
		if( !na_utils_find_in_list( second, str )){
			return( FALSE );
		}
	}

	for( il = second ; il ; il = il->next ){
		const gchar *str = ( const gchar * ) il->data;
		if( !na_utils_find_in_list( first, str )){
			return( FALSE );
		}
	}

	return( TRUE );
}

/**
 * na_utils_gslist_to_schema:
 * @list: a list of strings.
 *
 * Converts a list of strings to a comma-separated list of strings,
 * enclosed by brackets (dump format, GConf export format).
 *
 * Returns: a newly allocated string which should be g_free() by the caller.
 */
gchar *
na_utils_gslist_to_schema( GSList *list )
{
	GSList *ib;
	gchar *tmp;
	gchar *text = g_strdup( "" );

	for( ib = list ; ib ; ib = ib->next ){
		if( strlen( text )){
			tmp = g_strdup_printf( "%s,", text );
			g_free( text );
			text = tmp;
		}
		tmp = g_strdup_printf( "%s%s", text, ( gchar * ) ib->data );
		g_free( text );
		text = tmp;
	}

	tmp = g_strdup_printf( "[%s]", text );
	g_free( text );
	text = tmp;

	return( text );
}

/**
 * na_utils_get_first_word:
 * @string: a space-separated string.
 *
 * Returns: the first word of @string, as a newly allocated string which
 * should be g_free() by the caller.
 */
gchar *
na_utils_get_first_word( const gchar *string )
{
	gchar **splitted, **iter;
	gchar *word, *tmp;

	splitted = g_strsplit( string, " ", 0 );
	iter = splitted;
	word = NULL;

	while( *iter ){
		tmp = g_strstrip( *iter );
		if( g_utf8_strlen( tmp, -1 )){
			word = g_strdup( tmp );
			break;
		}
		iter++;
	}

	g_strfreev( splitted );
	return( word );
}

/**
 * na_utils_path_extract_last_dir:
 * @path: a full path.
 *
 * Extracts the last part of a full path.
 *
 * Returns: a newly allocated string which should be g_free() by the caller.
 */
gchar *
na_utils_path_extract_last_dir( const gchar *path )
{
	gchar **split = g_strsplit( path, "/", -1 );
	guint count = g_strv_length( split );
	gchar *lastdir = g_strdup( split[count-1] );
	g_strfreev( split );
	return( lastdir );
}
