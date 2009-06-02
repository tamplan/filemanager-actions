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

typedef struct {
	gchar *key;
	gchar *path;
}
	NactGConfIO;

static GConfClient *st_gconf = NULL;

static void         free_keys_values( GSList *list );
static void         initialize( void );
static void         load_action_properties( NactStorage *action );
static GSList      *load_items( const gchar *root, GType type );
static GSList      *load_keys_values( const gchar *path );
static GSList      *load_list_actions( GType type );
static GSList      *load_list_profiles( NactStorage *action, GType type );
static void         load_profile_properties( NactStorage *profile );
static GSList      *load_subdirs( const gchar *path );
static gchar       *path_to_key( const gchar *path );
static NactGConfIO *path_to_struct( const gchar *path );

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

/*
 * allocate the whole list of items under the specified dir.
 * each item of the list is a NactStorage-derived initialized GObject
 */
static GSList *
load_items( const gchar *root, GType type )
{
	GSList *items = NULL;

	GSList *listpath = load_subdirs( root );
	GSList *path;
	for( path = listpath ; path != NULL ; path = path->next ){

		NactGConfIO *io = path_to_struct(( const gchar * ) path->data );

		GObject *object = g_object_new(
				type, PROP_ORIGIN_STR, ORIGIN_GCONF, PROP_SUBSYSTEM_STR, io, NULL  );

		items = g_slist_prepend( items, object );
	}
	nactuti_free_string_list( listpath );

	return( items );
}

/*
 * load all the key=value pairs of this key (specified as a full path)
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

/*
 * load the keys which are the subdirs of the given path
 * returns a list of keys as full path
 */
GSList *
load_subdirs( const gchar *path )
{
	static const gchar *thisfn = "nact_gconf_load_subdirs";

	if( !st_gconf ){
		initialize();
	}

	GError *error = NULL;
	GSList *list = gconf_client_all_dirs( st_gconf, path, &error );
	if( error ){
		g_error( "%s: %s", thisfn, error->message );
		g_error_free( error );
		return(( GSList * ) NULL );
	}

	return( list );
}

/*
 * extract the key part (the last part) of a full path
 * returns a newly allocated string which must be g_freed by the caller
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

/*
 * allocate a new NactGConfIO structure
 * to be freed via nact_gconf_dispose
 */
static NactGConfIO *
path_to_struct( const gchar *path )
{
	NactGConfIO *io = g_new0( NactGConfIO, 1 );
	io->key = path_to_key( path );
	io->path = g_strdup( path );
	return( io );
}

/**
 * to be called from NactStorage instance_finalize to free the allocated
 * NactGConfIO structure.
 */
void
nact_gconf_dispose( gpointer ptr )
{
	NactGConfIO *io = ( NactGConfIO * ) ptr;
	g_free( io->key );
	g_free( io->path );
	g_free( io );
}

/**
 * Dump the NactGConfIO structure.
 *
 * @pio: a gpointer to the NactGConfIO structure to be dumped.
 */
void
nact_gconf_dump( gpointer pio )
{
	static const gchar *thisfn = "nact_gconf_dump";
	NactGConfIO *io = ( NactGConfIO * ) pio;
	g_debug( "%s: path='%s'", thisfn, io->path );
	g_debug( "%s: key='%s'", thisfn, io->key );
}

/*
 * load the action properties from GConf repository and setup action
 * instance accordingly.
 */
static void
load_action_properties( NactStorage *action )
{
	g_assert( NACT_IS_STORAGE( action ));

	gpointer pio;
	g_object_get( G_OBJECT( action ), PROP_SUBSYSTEM_STR, &pio, NULL );
	NactGConfIO *io = ( NactGConfIO * ) pio;

	GSList *list = load_keys_values( io->path );

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
}

/*
 * allocate the whole list of the actions
 * each item of the list is an action GObject, with the NactStorage
 * stuff being initialized
 */
static GSList *
load_list_actions( GType type )
{
	return( load_items( NACT_GCONF_CONFIG, type ));
}

/**
 * Return the list of actions as NactStorage-initialized objects.
 *
 * @type: actual GObject type to be allocated.
 */
GSList *
nact_gconf_load_actions( GType type )
{
	static const gchar *thisfn = "nact_gconf_load_actions";
	g_debug( "%s", thisfn );

	GSList *list = load_list_actions( type );

	GSList *it;
	for( it = list ; it ; it = it->next ){
		load_action_properties( NACT_STORAGE( it->data ));
	}

	return( list );
}

/*
 * read the properties of the profile and fill-up the object accordingly.
 */
static void
load_profile_properties( NactStorage *profile )
{
	GSList *listvalues, *iv, *strings;

	g_assert( NACT_IS_STORAGE( profile ));

	gpointer pio;
	g_object_get( G_OBJECT( profile ), PROP_SUBSYSTEM_STR, &pio, NULL );
	NactGConfIO *io = ( NactGConfIO * ) pio;

	GSList *list = load_keys_values( io->path );

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
				listvalues = gconf_value_get_list( value );
				strings = NULL;
				for( iv = listvalues ; iv != NULL ; iv = iv->next ){
					/*g_debug( "get '%s'", gconf_value_get_string(( GConfValue * ) iv->data ));*/
					strings = g_slist_prepend( strings,
							( gpointer ) gconf_value_get_string(( GConfValue * ) iv->data ));
				}
				g_object_set( G_OBJECT( profile ), key, strings, NULL );
				/*g_slist_free( strings );*/
				break;

			default:
				break;
		}
	}

	free_keys_values( list );
}

/*
 * allocate the whole list of the profile for the action
 * each item of the list is a profile GObject, with the NactStorage
 * stuff being initialized
 */
static GSList *
load_list_profiles( NactStorage *action, GType type )
{
	gpointer pio;
	g_object_get( G_OBJECT( action ), PROP_SUBSYSTEM_STR, &pio, NULL );
	NactGConfIO *io = ( NactGConfIO * ) pio;

	return( load_items( io->path, type ));
}

/**
 * Return the list of profiles as NactStorage-initialized objects.
 *
 * @action: the action.
 *
 * @type: actual GObject type to be allocated.
 */
GSList *
nact_gconf_load_profiles( NactStorage *action, GType type )
{
	static const gchar *thisfn = "nact_gconf_load_profiles";
	g_debug( "%s", thisfn );

	GSList *list = load_list_profiles( action, type );

	GSList *it;
	for( it = list ; it ; it = it->next ){
		load_profile_properties( NACT_STORAGE( it->data ));
	}

	return( list );
}
