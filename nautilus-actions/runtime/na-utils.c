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

#include "na-iabout.h"
#include "na-utils.h"

static GSList *text_to_string_list( const gchar *text, const gchar *separator, const gchar *default_value );

/**
 * na_utils_dump_string_list:
 * @list: a list of strings.
 *
 * Dumps the content of a list of strings.
 */
void
na_utils_dump_string_list( GSList *list )
{
	static const gchar *thisfn = "na_utils_dump_string_list";
	GSList *i;
	int c;

	g_debug( "%s: list at %p has %d elements", thisfn, ( void * ) list, g_slist_length( list ));
	for( i=list, c=0 ; i ; i=i->next, c++ ){
		gchar *s = ( gchar * ) i->data;
		g_debug( "%s: %2d - %s", thisfn, c, s );
	}
}

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
 * na_utils_lines_to_string_list:
 * @text: a buffer which contains embedded newlines.
 *
 * Returns: a list of strings from the buffer.
 *
 * The returned list should be na_utils_free_string_list() by the caller.
 */
GSList *
na_utils_lines_to_string_list( const gchar *text )
{
	return( text_to_string_list( text, "\n", NULL ));
}

/**
 * na_utils_remove_ascii_from_string_list:
 * @list: the GSList to be updated.
 * @text: string to remove.
 *
 * Removes a string from a GSList of strings.
 *
 * Returns the new list after update.
 */
GSList *
na_utils_remove_ascii_from_string_list( GSList *list, const gchar *text )
{
	GSList *il;
	for( il = list ; il ; il = il->next ){
		const gchar *istr = ( const gchar * ) il->data;
		if( !g_ascii_strcasecmp( text, istr )){
			list = g_slist_remove( list, ( gconstpointer ) istr );
			return( list );
		}
	}
	return( list );
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
 * na_utils_string_list_to_text:
 * @strlist: a list of strings.
 *
 * Concatenates a string list to a semi-colon-separated text
 * suitable for an entry in the user interface
 *
 * Returns: a newly allocated string, which should be g_free() by the
 * caller.
 */
gchar *
na_utils_string_list_to_text( GSList *strlist )
{
	GSList *ib;
	gchar *tmp;
	gchar *text = g_strdup( "" );

	for( ib = strlist ; ib ; ib = ib->next ){
		if( strlen( text )){
			tmp = g_strdup_printf( "%s; ", text );
			g_free( text );
			text = tmp;
		}
		tmp = g_strdup_printf( "%s%s", text, ( gchar * ) ib->data );
		g_free( text );
		text = tmp;
	}

	return( text );
}

/**
 * na_utils_text_to_string_list:
 * @text: a semi-colon-separated string.
 *
 * Returns: a list of strings from a semi-colon-separated text
 * (entry text in the user interface).
 *
 * The returned list should be na_utils_free_string_list() by the caller.
 */
GSList *
na_utils_text_to_string_list( const gchar *text )
{
	return( text_to_string_list( text, ";", NULL ));
}

/*
 * split a text buffer in lines
 */
static GSList *
text_to_string_list( const gchar *text, const gchar *separator, const gchar *default_value )
{
	GSList *strlist = NULL;
	gchar **tokens, **iter;
	gchar *tmp;
	gchar *source = g_strdup( text );

	tmp = g_strstrip( source );
	if( !strlen( tmp ) && default_value ){
		strlist = g_slist_append( strlist, g_strdup( default_value ));

	} else {
		tokens = g_strsplit( source, separator, -1 );
		iter = tokens;

		while( *iter ){
			tmp = g_strstrip( *iter );
			strlist = g_slist_append( strlist, g_strdup( tmp ));
			iter++;
		}

		g_strfreev( tokens );
	}

	g_free( source );
	return( strlist );
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
 * na_utils_schema_to_gslist:
 * @value: a string of the form [xxx,yyy,...] as read from GConf.
 *
 * Converts a string representing a list of strings in a GConf format
 * to a list of strings.
 *
 * Returns: a newly allocated list of strings, which should be
 * na_utils_free_string_list() by the caller.
 */
GSList *
na_utils_schema_to_gslist( const gchar *value )
{
	GSList *list = NULL;
	const gchar *ptr = value;
	const gchar *start = NULL;
	gchar *str_list = NULL;
	gchar **str_list_splited = NULL;
	int i;

	/* first remove the surrounding brackets [] */
	while( *ptr != '[' ){
		ptr++;
	}

	if( *ptr == '[' ){
		ptr++;
		start = ptr;
		i = 0;
		while( *ptr != ']' ){
			i++;
			ptr++;
		}
		if( *ptr == ']' ){
			str_list = g_strndup( start, i );
		}
	}

	/* split the result and fill the list */
	if( str_list != NULL ){

		str_list_splited = g_strsplit( str_list, ",", -1 );
		i = 0;
		while( str_list_splited[i] != NULL ){
			list = g_slist_append( list, g_strdup( str_list_splited[i] ));
			i++;
		}
		g_strfreev( str_list_splited );
	}

	return( list );
}

/**
 * na_utils_boolean_to_schema:
 * @b: a boolean to be written.
 *
 * Converts a boolean to the suitable string for a GConf schema
 *
 * Returns: a newly allocated string which should be g_free() by the caller.
 */
gchar *
na_utils_boolean_to_schema( gboolean b )
{
	gchar *text = g_strdup_printf( "%s", b ? "true" : "false" );
	return( text );
}

/**
 * na_utils_schema_to_boolean:
 * @value: a string which should contains a boolean value.
 * @default_value: the default value to be used.
 *
 * Converts a string to a boolean.
 *
 * The conversion is not case sensitive, and accepts abbreviations.
 * The default value is used if we cannot parse the provided string.
 *
 * Returns: the boolean.
 *
 */
gboolean
na_utils_schema_to_boolean( const gchar *value, gboolean default_value )
{
	if( !g_ascii_strcasecmp( value, "true" )){
		/*g_debug( "na_utils_schema_to_boolean: value=%s, returning TRUE", value );*/
		return( TRUE );
	}
	if( !g_ascii_strcasecmp( value, "false" )){
		/*g_debug( "na_utils_schema_to_boolean: value=%s, returning FALSE", value );*/
		return( FALSE );
	}
	/*g_debug( "na_utils_schema_to_boolean: value=%s, returning default_value", value );*/
	return( default_value );
}

/**
 * na_utils_gstring_joinv:
 * @start: a prefix to be written at the beginning of the output string.
 * @separator: a string to be used as separator.
 * @list: the list of strings to be concatenated.
 *
 * Concatenates a gchar **list of strings to a new string.
 *
 * Returns: a newly allocated string which should be g_free() by the caller.
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
 * na_utils_prefix_strings:
 * @prefix: the prefix to be prepended.
 * @str: a multiline string.
 *
 * Appends a prefix to each line of the string.
 *
 * Returns: a new string which should be g_free() by the caller.
 */
gchar *
na_utils_prefix_strings( const gchar *prefix, const gchar *str )
{
	GSList *list, *il;
	GString *result;

	list = text_to_string_list( str, "\n", NULL );
	result = g_string_new( "" );

	for( il = list ; il ; il = il->next ){
		g_string_append_printf( result, "%s%s\n", prefix, ( gchar * ) il->data );
	}

	na_utils_free_string_list( list );

	return( g_string_free( result, FALSE ));
}

/**
 * na_utils_exist_file:
 * @uri: an uri which points to a file.
 *
 * Returns: %TRUE if the specified file exists, %FALSE else.
 *
 * Race condition: cf. na_utils_is_writable_dir() comment.
 */
gboolean
na_utils_exist_file( const gchar *uri )
{
	GFile *file;
	gboolean exists;

	file = g_file_new_for_uri( uri );
	exists = g_file_query_exists( file, NULL );
	g_object_unref( file );

	return( exists );
}

/**
 * na_utils_is_writable_dir:
 * @uri: an uri which points to a directory.
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
na_utils_is_writable_dir( const gchar *uri )
{
	static const gchar *thisfn = "na_utils_is_writable_dir";
	GFile *file;
	GError *error = NULL;
	GFileInfo *info;
	GFileType type;
	gboolean writable;

	if( !uri || !strlen( uri )){
		return( FALSE );
	}

	file = g_file_new_for_uri( uri );
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
		g_warning( "%s: %s is not a directory", thisfn, uri );
		g_object_unref( info );
		return( FALSE );
	}

	writable = g_file_info_get_attribute_boolean( info, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE );
	if( !writable ){
		g_warning( "%s: %s is not writable", thisfn, uri );
	}
	g_object_unref( info );

	return( writable );
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

/**
 * na_utils_remove_last_level_from_path:
 * @path: a full path.
 *
 * Removes last level from path (mostly a 'dirname').
 *
 * Returns: a newly allocated string which should be g_free() by the caller.
 */
gchar *
na_utils_remove_last_level_from_path( const gchar *path )
{
	int p;
	const char *ptr = path;
	char *new_path;

	if( path == NULL ){
		return( NULL );
	}

	p = strlen( path ) - 1;
	if( p < 0 ){
		return( NULL );
	}

	while(( p > 0 ) && ( ptr[p] != '/' )){
		p--;
	}

	if(( p == 0 ) && ( ptr[p] == '/' )){
		p++;
	}

	new_path = g_strndup( path, ( guint ) p );

	return( new_path );
}

/**
 * na_utils_print_version:
 *
 * Print a version message on the console
 *
 * nautilus-actions-new (Nautilus-Actions) v 2.29.1
 * Copyright (C) 2005-2007 Frederic Ruaudel
 * Copyright (C) 2009 Pierre Wieser
 * Nautilus-Actions is free software, licensed under GPLv2 or later.
 */
void
na_utils_print_version( void )
{
	gchar *copyright;

	g_print( "\n" );
	g_print( "%s (%s) v %s\n", g_get_prgname(), PACKAGE_NAME, PACKAGE_VERSION );
	copyright = na_iabout_get_copyright( TRUE );
	g_print( "%s\n", copyright );
	g_free( copyright );

	g_print( "%s is free software, and is provided without any warranty. You may\n", PACKAGE_NAME );
	g_print( "redistribute copies of %s under the terms of the GNU General Public\n", PACKAGE_NAME );
	g_print( "License (see COPYING).\n" );
	g_print( "\n" );

	g_debug( "Current system runs Glib %d.%d.%d, Gtk+ %d.%d.%d",
			glib_major_version, glib_minor_version, glib_micro_version,
			gtk_major_version, gtk_minor_version, gtk_micro_version );
	g_debug( "%s", "" );
}
