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
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include <string.h>
#include "nact-gconf.h"
#include "nact-gconf-keys.h"
#include "uti-lists.h"

static GConfClient *st_gconf = NULL;

static void     initialize( void );
static gchar   *get_object_path( NactStorage *object );
static gchar   *path_to_key( const gchar *path );
static GSList  *load_subdirs( const gchar *path );
static GSList  *load_keys_values( const gchar *path );
static void     free_keys_values( GSList *list );
static GSList  *duplicate_list( GSList *list );

/*
 * we have to initialize this early in the process as nautilus-actions
 * will try to load actions before even any NactAction has been created
 */
static void
initialize( void )
{
	static const gchar *thisfn = "nact_gconf_initialize";
	g_debug( "%s", thisfn );

	st_gconf = gconf_client_get_default();
}

gchar *
get_object_path( NactStorage *object )
{
	g_assert( NACT_IS_STORAGE( object ));

	gchar *id = nact_storage_get_id( object );
	gchar *path = g_strdup_printf( "%s/%s", NACT_GCONF_CONFIG, id );
	g_free( id );

	return( path );
}

/*
 * Returns a list of the keys which appear as subdirectories
 *
 * The returned list contains allocated strings. Each string is the
 * relative path of a subdirectory. You should g_free() each string
 * in the list, then g_slist_free() the list itself.
 */
static gchar *
path_to_key( const gchar *path )
{
	gchar **split = g_strsplit( path, "/", -1 );
	guint count = g_strv_length( split );
	gchar *key = g_strdup( split[count-1] );
	g_strfreev( split );

	return( key );
}

GSList *
load_subdirs( const gchar *path )
{
	static const gchar *thisfn = "nact_gconf_load_subdirs";
	g_debug( "%s: path=%s", thisfn, path );

	if( !st_gconf ){
		initialize();
	}

	GError *error = NULL;
	GSList *list_path = gconf_client_all_dirs( st_gconf, path, &error );
	if( error ){
		g_error( "%s: %s", thisfn, error->message );
		g_error_free( error );
		return(( GSList * ) NULL );
	}

	GSList *list_keys = NULL;
	GSList *item;
	for( item = list_path ; item != NULL ; item = item->next ){
		gchar *key = path_to_key(( gchar * ) item->data );
		list_keys = g_slist_prepend( list_keys, key );
	}

	nactuti_free_string_list( list_path );

	return( list_keys );
}

/**
 * Return the list of uuids.
 *
 * The list of UUIDs is returned as a GSList of newly allocation strings.
 * This list should be freed by calling nact_gconf_free_uuids.
 */
GSList *
nact_gconf_load_uuids( void )
{
	static const gchar *thisfn = "nact_gconf_load_uuids";
	g_debug( "%s", thisfn );

	return( load_subdirs( NACT_GCONF_CONFIG ));
}

/**
 * Free a previously allocated list of UUIDs.
 *
 * @list: list of UUIDs to be freed.
 */
void
nact_gconf_free_uuids( GSList *list )
{
	static const gchar *thisfn = "nact_gconf_free_uuids";
	g_debug( "%s: list=%p", thisfn, list );
	nactuti_free_string_list( list );
}

/*
 * Load all the key=value pairs of this key
 *
 * The list is not recursive, it contains only the immediate children of
 * path. To free the returned list, gconf_entry_free() each list element,
 * then g_slist_free() the list itself.
 */
static GSList *
load_keys_values( const gchar *path )
{
	static const gchar *thisfn = "nact_gconf_load_keys_values";

	if( !st_gconf ){
		initialize();
	}

	GError *error = NULL;
	GSList *list_path = gconf_client_all_entries( st_gconf, path, &error );
	if( error ){
		g_error( "%s: %s", thisfn, error->message );
		g_error_free( error );
		return(( GSList * ) NULL );
	}

	GSList *list_keys = NULL;
	GSList *item;
	for( item = list_path ; item != NULL ; item = item->next ){
		GConfEntry *entry = ( GConfEntry * ) item->data;
		gchar *key = path_to_key( gconf_entry_get_key( entry ));
		GConfEntry *entry_new = gconf_entry_new( key, gconf_entry_get_value( entry ));
		g_free( key );
		list_keys = g_slist_prepend( list_keys, entry_new );
	}

	free_keys_values( list_path );

	return( list_keys );
}

static void
free_keys_values( GSList *list )
{
	GSList *item;
	for( item = list ; item != NULL ; item = item->next ){
		GConfEntry *entry = ( GConfEntry * ) item->data;
		gconf_entry_unref( entry );
	}
	g_slist_free( list );
}

/*
 * load the action properties from GConf repository and setup action
 * instance accordingly
 */
gboolean
nact_gconf_load_action_properties( NactStorage *action )
{
	g_object_set( G_OBJECT( action ), "origin", ORIG_GCONF, NULL );

	gchar *path = get_object_path( action );

	g_object_set( G_OBJECT( action ), "gconf-path", path, NULL );

	GSList *list = load_keys_values( path );

	g_free( path );

	if( !list ){
		return( FALSE );
	}

	GSList *item;
	for( item = list ; item != NULL ; item = item->next ){
		GConfEntry *entry = ( GConfEntry * ) item->data;
		const char *key = gconf_entry_get_key( entry );
		GConfValue *value = gconf_entry_get_value( entry );
		switch( value->type ){
			case GCONF_VALUE_STRING:
				g_object_set( G_OBJECT( action ), key, gconf_value_get_string( value ), NULL );
				break;
			default:
				g_assert_not_reached();
				break;
		}
	}

	free_keys_values( list );

	return( TRUE );
}

GSList *
nact_gconf_load_profile_names( NactStorage *action )
{
	gchar *path = get_object_path( action );

	GSList *list = load_subdirs( path );

	g_free( path );

	return( list );
}

static GSList *
duplicate_list( GSList *list )
{
	GSList *newlist = NULL;
	GSList *item;
	for( item = list ; item != NULL ; item = item->next ){
		gchar *value = g_strdup(( gchar * ) item->data );
		g_debug( "duplicate '%s'", value );
		newlist = g_slist_prepend( newlist, value );
	}
	return( newlist );
}

gboolean
nact_gconf_load_profile_properties( NactStorage *profile )
{
	g_object_set( G_OBJECT( profile ), "origin", ORIG_GCONF, NULL );

	NactStorage *action;
	g_object_get( G_OBJECT( profile ), "action", &action, NULL );
	gchar *path_action = get_object_path( action );

	gchar *profile_name;
	g_object_get( G_OBJECT( profile ), "name", &profile_name, NULL );
	gchar *profile_path = g_strdup_printf( "%s/%s", path_action, profile_name );
	g_free( profile_name );
	g_free( path_action );

	g_object_set( G_OBJECT( profile ), "gconf-path", profile_path, NULL );

	GSList *list = load_keys_values( profile_path );

	g_free( profile_path );

	if( !list ){
		return( FALSE );
	}

	GSList *item;
	for( item = list ; item != NULL ; item = item->next ){

		GConfEntry *entry = ( GConfEntry * ) item->data;

		const char *key = gconf_entry_get_key( entry );
		GConfValue *value = gconf_entry_get_value( entry );

		switch( value->type ){

			case GCONF_VALUE_STRING:
				g_object_set( G_OBJECT( profile ), key, gconf_value_get_string( value ), NULL );
				break;

			case GCONF_VALUE_BOOL:
				g_object_set( G_OBJECT( profile ), key, gconf_value_get_bool( value ), NULL );
				break;

			case GCONF_VALUE_LIST:
				g_object_set( G_OBJECT( profile ), key, duplicate_list( gconf_value_get_list( value )), NULL );
				break;

			default:
				break;
		}
	}

	free_keys_values( list );

	return( TRUE );
}

void
nact_gconf_free_profile_names( GSList *list )
{
	nactuti_free_string_list( list );
}
