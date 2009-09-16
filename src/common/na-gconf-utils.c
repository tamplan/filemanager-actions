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

/*
 * load the keys which are the subdirs of the given path
 * returns a list of keys as full path
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

void
na_gconf_utils_free_subdirs( GSList *subdirs )
{
	na_utils_free_string_list( subdirs );
}

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

/*
 * load all the key=value pairs of this key specified as a full path,
 * returning them as a list of GConfEntry.
 *
 * The list is not recursive, it contains only the immediate children of
 * path. To free the returned list, call na_gconf_utils_free_entries().
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

void
na_gconf_utils_free_entries( GSList *list )
{
	g_slist_foreach( list, ( GFunc ) gconf_entry_unref, NULL );
	g_slist_free( list );
}

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

gchar *
na_gconf_utils_path_to_key( const gchar *path )
{
	return( na_utils_path_extract_last_dir( path ));
}

gboolean
na_gconf_utils_read_bool( GConfClient *gconf, const gchar *path, gboolean use_schema, gboolean default_value )
{
	static const gchar *thisfn = "na_gconf_utils_read_bool";
	GError *error = NULL;
	GConfValue *value = NULL;
	gboolean ret;

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
 * Returns: a list of strings.
 *
 * The returned list must be released with na_utils_free_string_list().
 */
GSList *
na_gconf_utils_read_string_list( GConfClient *gconf, const gchar *path )
{
	static const gchar *thisfn = "na_gconf_utils_read_string_list";
	GError *error = NULL;
	GSList *list_strings;

	list_strings = gconf_client_get_list( gconf, path, GCONF_VALUE_STRING, &error );

	if( error ){
		g_warning( "%s: path=%s, error=%s", thisfn, path, error->message );
		g_error_free( error );
		return( NULL );
	}

	return( list_strings );
}

gboolean
na_gconf_utils_write_bool( GConfClient *gconf, const gchar *path, gboolean value, gchar **message )
{
	static const gchar *thisfn = "na_gconf_utils_write_bool";
	gboolean ret = TRUE;
	GError *error = NULL;

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

gboolean
na_gconf_utils_write_string( GConfClient *gconf, const gchar *path, const gchar *value, gchar **message )
{
	static const gchar *thisfn = "na_gconf_utils_write_string";
	gboolean ret = TRUE;
	GError *error = NULL;

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

gboolean
na_gconf_utils_write_string_list( GConfClient *gconf, const gchar *path, GSList *value, gchar **message )
{
	static const gchar *thisfn = "na_gconf_utils_write_string_list";
	gboolean ret = TRUE;
	GError *error = NULL;

	if( !gconf_client_set_list( gconf, path, GCONF_VALUE_STRING, value, &error )){
		if( message ){
			*message = g_strdup( error->message );
		}
		g_warning( "%s: path=%s, value=%s, error=%s", thisfn, path, value ? "True":"False", error->message );
		g_error_free( error );
		ret = FALSE;
	}

	return( ret );
}
