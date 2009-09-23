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

#include <string.h>

#include "na-utils.h"
#include "na-gconf-utils.h"

static gboolean sync_gconf( GConfClient *gconf, gchar **message );

/**
 * na_gconf_utils_get_subdirs:
 * @gconf: a  #GConfClient instance.
 * @path: a full path to be readen.
 *
 * Loads the subdirs of the given path.
 *
 * Returns: a GSList of full path subdirectories.
 *
 * The returned list should be na_gconf_utils_free_subdirs() by the
 * caller.
 */
GSList *
na_gconf_utils_get_subdirs( GConfClient *gconf, const gchar *path )
{
	static const gchar *thisfn = "na_gconf_utils_get_subdirs";
	GError *error = NULL;
	GSList *list_subdirs;

	list_subdirs = gconf_client_all_dirs( gconf, path, &error );

	if( error ){
		g_warning( "%s: path=%s, error=%s", thisfn, path, error->message );
		g_error_free( error );
		return(( GSList * ) NULL );
	}

	return( list_subdirs );
}

/**
 * na_gconf_utils_free_subdirs:
 * @subdirs: a list of subdirs as returned by na_gconf_utils_get_subdirs().
 *
 * Release the list of subdirs.
 */
void
na_gconf_utils_free_subdirs( GSList *subdirs )
{
	na_utils_free_string_list( subdirs );
}

/**
 * na_gconf_utils_have_subdir:
 * @gconf: a  #GConfClient instance.
 * @path: a full path to be readen.
 *
 * Returns: %TRUE if the specified path has at least one subdirectory,
 * %FALSE else.
 */
gboolean
na_gconf_utils_have_subdir( GConfClient *gconf, const gchar *path )
{
	GSList *listpath;
	gboolean have_subdir;

	listpath = na_gconf_utils_get_subdirs( gconf, path );
	have_subdir = ( listpath && g_slist_length( listpath ));
	na_gconf_utils_free_subdirs( listpath );

	return( have_subdir );
}

/**
 * na_gconf_utils_get_entries:
 * @gconf: a  #GConfClient instance.
 * @path: a full path to be readen.
 *
 * Loads all the key=value pairs of the specified key.
 *
 * Returns: a list of #GConfEntry.
 *
 * The returned list is not recursive : it contains only the immediate
 * children of @path. To free the returned list, call
 * na_gconf_utils_free_entries().
 */
GSList *
na_gconf_utils_get_entries( GConfClient *gconf, const gchar *path )
{
	static const gchar *thisfn = "na_gconf_utils_get_entries";
	GError *error = NULL;
	GSList *list_entries;

	list_entries = gconf_client_all_entries( gconf, path, &error );

	if( error ){
		g_warning( "%s: path=%s, error=%s", thisfn, path, error->message );
		g_error_free( error );
		return(( GSList * ) NULL );
	}

	return( list_entries );
}

/**
 * na_gconf_utils_free_entries:
 * @list: a list of #GConfEntry as returned by na_gconf_utils_get_entries().
 *
 * Releases the provided list.
 */
void
na_gconf_utils_free_entries( GSList *list )
{
	g_slist_foreach( list, ( GFunc ) gconf_entry_unref, NULL );
	g_slist_free( list );
}

/**
 * na_gconf_utils_have_entry:
 * @gconf: a  #GConfClient instance.
 * @path: the full path of a key.
 * @entry: the entry to be tested.
 *
 * Returns: %TRUE if the given @entry exists for the given @path,
 * %FALSE else.
 */
gboolean
na_gconf_utils_have_entry( GConfClient *gconf, const gchar *path, const gchar *entry )
{
	static const gchar *thisfn = "na_gconf_utils_have_entry";
	gboolean have_entry = FALSE;
	GError *error = NULL;
	gchar *key;
	GConfValue *value;

	key = g_strdup_printf( "%s/%s", path, entry );

	value = gconf_client_get_without_default( gconf, key, &error );

	if( error ){
		g_warning( "%s: key=%s, error=%s", thisfn, key, error->message );
		g_error_free( error );
		if( value ){
			gconf_value_free( value );
			value = NULL;
		}
	}

	if( value ){
		have_entry = TRUE;
		gconf_value_free( value );
	}

	g_free( key );

	return( have_entry );
}

/**
 * na_gconf_utils_get_bool_from_entries:
 * @entries: a list of #GConfEntry as returned by na_gconf_utils_get_entries().
 * @entry: the searched entry.
 * @value: a pointer to a gboolean to be set to the found value.
 *
 * Returns: %TRUE if the entry was found, %FALSE else.
 *
 * If the entry was not found, or was not of boolean type, @value is set
 * to %FALSE.
 */
gboolean
na_gconf_utils_get_bool_from_entries( GSList *entries, const gchar *entry, gboolean *value )
{
	GSList *ip;
	GConfEntry *gconf_entry;
	GConfValue *gconf_value;
	gchar *key;
	gboolean found;

	g_return_val_if_fail( value, FALSE );

	*value = FALSE;
	found = FALSE;

	for( ip = entries ; ip && !found ; ip = ip->next ){
		gconf_entry = ( GConfEntry * ) ip->data;
		key = na_gconf_utils_path_to_key( gconf_entry_get_key( gconf_entry ));

		if( !strcmp( key, entry )){
			gconf_value = gconf_entry_get_value( gconf_entry );

			if( gconf_value &&
				gconf_value->type == GCONF_VALUE_BOOL ){

					found = TRUE;
					*value = gconf_value_get_bool( gconf_value );
			}
		}
		g_free( key );
	}

	return( found );
}

/**
 * na_gconf_utils_get_string_from_entries:
 * @entries: a list of #GConfEntry as returned by na_gconf_utils_get_entries().
 * @entry: the searched entry.
 * @value: a pointer to a gchar * to be set to the found value.
 *
 * Returns: %TRUE if the entry was found, %FALSE else.
 *
 * If the entry was not found, or was not of string type, @value is set
 * to %NULL.
 *
 * If @value is returned not NULL, it should be g_free() by the caller.
 */
gboolean
na_gconf_utils_get_string_from_entries( GSList *entries, const gchar *entry, gchar **value )
{
	GSList *ip;
	GConfEntry *gconf_entry;
	GConfValue *gconf_value;
	gchar *key;
	gboolean found;

	g_return_val_if_fail( value, FALSE );

	*value = NULL;
	found = FALSE;

	for( ip = entries ; ip && !found ; ip = ip->next ){
		gconf_entry = ( GConfEntry * ) ip->data;
		key = na_gconf_utils_path_to_key( gconf_entry_get_key( gconf_entry ));

		if( !strcmp( key, entry )){
			gconf_value = gconf_entry_get_value( gconf_entry );

			if( gconf_value &&
				gconf_value->type == GCONF_VALUE_STRING ){

					found = TRUE;
					*value = g_strdup( gconf_value_get_string( gconf_value ));
			}
		}
		g_free( key );
	}

	return( found );
}

/**
 * na_gconf_utils_get_string_list_from_entries:
 * @entries: a list of #GConfEntry as returned by na_gconf_utils_get_entries().
 * @entry: the searched entry.
 * @value: a pointer to a GSList * to be set to the found value.
 *
 * Returns: %TRUE if the entry was found, %FALSE else.
 *
 * If the entry was not found, or was not of string list type, @value
 * is set to %NULL.
 *
 * If @value is returned not NULL, it should be na_utils_free_string_list()
 * by the caller.
 */
gboolean
na_gconf_utils_get_string_list_from_entries( GSList *entries, const gchar *entry, GSList **value )
{
	GSList *ip, *iv;
	GConfEntry *gconf_entry;
	GConfValue *gconf_value;
	gchar *key;
	gboolean found;
	GSList *list_values;

	g_return_val_if_fail( value, FALSE );

	*value = NULL;
	found = FALSE;

	for( ip = entries ; ip && !found ; ip = ip->next ){
		gconf_entry = ( GConfEntry * ) ip->data;
		key = na_gconf_utils_path_to_key( gconf_entry_get_key( gconf_entry ));

		if( !strcmp( key, entry )){
			gconf_value = gconf_entry_get_value( gconf_entry );

			if( gconf_value &&
				gconf_value->type == GCONF_VALUE_LIST ){

					found = TRUE;
					list_values = gconf_value_get_list( gconf_value );
					for( iv = list_values ; iv ; iv = iv->next ){
						*value = g_slist_append( *value, g_strdup( gconf_value_get_string(( GConfValue * ) iv->data )));
					}
			}
		}
		g_free( key );
	}

	return( found );
}

/**
 * na_gconf_utils_path_to_key:
 * @path: the full path of a key.
 *
 * Returns: the key itself, i.e. the last part of the @path.
 *
 * The returned string should be g_free() by the caller.
 */
gchar *
na_gconf_utils_path_to_key( const gchar *path )
{
	return( na_utils_path_extract_last_dir( path ));
}

/**
 * na_gconf_utils_read_bool:
 * @gconf: a #GConfClient instance.
 * @path: the full path to the key.
 * @use_schema: whether to use a default value from schema, or not.
 * @default_value: default value to be used if @use_schema is %FALSE.
 *
 * Returns: the boolean value read at the specified @path, taking into
 * account a default value from schema (if @use_schema if %TRUE), or
 * the specified @default_value.
 */
gboolean
na_gconf_utils_read_bool( GConfClient *gconf, const gchar *path, gboolean use_schema, gboolean default_value )
{
	static const gchar *thisfn = "na_gconf_utils_read_bool";
	GError *error = NULL;
	GConfValue *value = NULL;
	gboolean ret;

	g_return_val_if_fail( GCONF_IS_CLIENT( gconf ), FALSE );

	ret = default_value;

	if( use_schema ){
		ret = gconf_client_get_bool( gconf, path, &error );
	} else {
		value = gconf_client_get_without_default( gconf, path, &error );
	}

	if( error ){
		g_warning( "%s: path=%s, error=%s", thisfn, path, error->message );
		g_error_free( error );
		if( value ){
			gconf_value_free( value );
			value = NULL;
		}
	}

	if( value ){
		ret = gconf_value_get_bool( value );
		gconf_value_free( value );
	}

	return( ret );
}

/**
 * na_gconf_utils_read_string_list:
 * @gconf: a #GConfClient instance.
 * @path: the full path to the key to be read.
 *
 * Returns: a list of strings,
 * or %NULL if the entry was not found or was not of string list type.
 *
 * The returned list must be released with na_utils_free_string_list().
 */
GSList *
na_gconf_utils_read_string_list( GConfClient *gconf, const gchar *path )
{
	static const gchar *thisfn = "na_gconf_utils_read_string_list";
	GError *error = NULL;
	GSList *list_strings;

	g_return_val_if_fail( GCONF_IS_CLIENT( gconf ), NULL );

	list_strings = gconf_client_get_list( gconf, path, GCONF_VALUE_STRING, &error );

	if( error ){
		g_warning( "%s: path=%s, error=%s", thisfn, path, error->message );
		g_error_free( error );
		return( NULL );
	}

	return( list_strings );
}

/**
 * na_gconf_utils_write_bool:
 * @gconf: a #GConfClient instance.
 * @path: the full path to the key.
 * @value: the value to be written.
 * @message: a pointer to a gchar * which will be allocated if needed.
 *
 * Writes a boolean at the given @path.
 *
 * Returns: %TRUE if the writing has been successfull, %FALSE else.
 *
 * If returned not NULL, the @message contains an error message.
 * It should be g_free() by the caller.
 */
gboolean
na_gconf_utils_write_bool( GConfClient *gconf, const gchar *path, gboolean value, gchar **message )
{
	static const gchar *thisfn = "na_gconf_utils_write_bool";
	gboolean ret = TRUE;
	GError *error = NULL;

	g_return_val_if_fail( GCONF_IS_CLIENT( gconf ), FALSE );

	if( !gconf_client_set_bool( gconf, path, value, &error )){
		if( message ){
			*message = g_strdup( error->message );
		}
		g_warning( "%s: path=%s, value=%s, error=%s", thisfn, path, value ? "True":"False", error->message );
		g_error_free( error );
		ret = FALSE;
	}

	return( ret );
}

/**
 * na_gconf_utils_write_string:
 * @gconf: a #GConfClient instance.
 * @path: the full path to the key.
 * @value: the value to be written.
 * @message: a pointer to a gchar * which will be allocated if needed.
 *
 * Writes a string at the given @path.
 *
 * Returns: %TRUE if the writing has been successfull, %FALSE else.
 *
 * If returned not NULL, the @message contains an error message.
 * It should be g_free() by the caller.
 */
gboolean
na_gconf_utils_write_string( GConfClient *gconf, const gchar *path, const gchar *value, gchar **message )
{
	static const gchar *thisfn = "na_gconf_utils_write_string";
	gboolean ret = TRUE;
	GError *error = NULL;

	g_return_val_if_fail( GCONF_IS_CLIENT( gconf ), FALSE );

	if( !gconf_client_set_string( gconf, path, value, &error )){
		if( message ){
			*message = g_strdup( error->message );
		}
		g_warning( "%s: path=%s, value=%s, error=%s", thisfn, path, value ? "True":"False", error->message );
		g_error_free( error );
		ret = FALSE;
	}

	return( ret );
}

/**
 * na_gconf_utils_write_string_list:
 * @gconf: a #GConfClient instance.
 * @path: the full path to the key.
 * @value: the list of values to be written.
 * @message: a pointer to a gchar * which will be allocated if needed.
 *
 * Writes a list of strings at the given @path.
 *
 * Returns: %TRUE if the writing has been successfull, %FALSE else.
 *
 * If returned not NULL, the @message contains an error message.
 * It should be g_free() by the caller.
 */
gboolean
na_gconf_utils_write_string_list( GConfClient *gconf, const gchar *path, GSList *value, gchar **message )
{
	static const gchar *thisfn = "na_gconf_utils_write_string_list";
	gboolean ret = TRUE;
	GError *error = NULL;

	g_return_val_if_fail( GCONF_IS_CLIENT( gconf ), FALSE );

	if( !gconf_client_set_list( gconf, path, GCONF_VALUE_STRING, value, &error )){
		if( message ){
			*message = g_strdup( error->message );
		}
		g_warning( "%s: path=%s, value=%s, error=%s", thisfn, path, value ? "True":"False", error->message );
		g_error_free( error );
		ret = FALSE;
	}

	if( ret ){
		ret = sync_gconf( gconf, message );
	}

	return( ret );
}

static gboolean
sync_gconf( GConfClient *gconf, gchar **message )
{
	static const gchar *thisfn = "na_gconf_utils_sync_gconf";
	gboolean ret = TRUE;
	GError *error = NULL;

	gconf_client_suggest_sync( gconf, &error );
	if( error ){
		if( message ){
			*message = g_strdup( error->message );
		}
		g_warning( "%s: error=%s", thisfn, error->message );
		g_error_free( error );
		ret = FALSE;
	}

	return( ret );
}
