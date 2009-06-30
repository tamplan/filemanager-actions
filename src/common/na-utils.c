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
#include <glib-object.h>
#include "na-utils.h"

/**
 * Search for a string in a string list.
 *
 * @list: the GSList of strings to be searched.
 *
 * @str: the searched string.
 *
 * Returns TRUE if the string has been found in list.
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
 * Compare two string lists.
 *
 * @first: a GSList of strings.
 *
 * @second: another GSList of strings to be compared with @first.
 *
 * Returns TRUE if the two lists have same content.
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
 * Duplicates a GSList of strings.
 *
 * @list: the GSList to be duplicated.
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
	return( duplist );
}

/**
 * Frees a GSList of strings.
 *
 * @list: the GSList to be freed.
 */
void
na_utils_free_string_list( GSList *list )
{
	GSList *item;
	for( item = list ; item != NULL ; item = item->next ){
		g_free(( gchar * ) item->data );
	}
	g_slist_free( list );
}

/**
 * Concatenates a gchar **list of strings to a GString.
 */
gchar *
na_utils_gstring_joinv( const gchar *start, const gchar *separator, gchar **list )
{
	GString *tmp_string = g_string_new( "" );
	int i;

	g_return_val_if_fail( list != NULL, NULL );

	if( start != NULL ){
		tmp_string = g_string_append( tmp_string, start );
	}

	if( list[0] != NULL ){
		tmp_string = g_string_append( tmp_string, list[0] );
	}

	for( i = 1 ; list[i] != NULL ; i++ ){
		if( separator ){
			tmp_string = g_string_append( tmp_string, separator );
		}
		tmp_string = g_string_append( tmp_string, list[i] );
	}

	return( g_string_free( tmp_string, FALSE ));
}
