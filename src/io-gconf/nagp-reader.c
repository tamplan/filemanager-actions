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

#include <api/na-data-def.h>
#include <api/na-data-types.h>
#include <api/na-ifactory-provider.h>
#include <api/na-iio-provider.h>
#include <api/na-object-api.h>
#include <api/na-core-utils.h>
#include <api/na-gconf-utils.h>

#include "nagp-gconf-provider.h"
#include "nagp-keys.h"
#include "nagp-reader.h"

typedef struct {
	gchar        *path;
	GSList       *entries;
	NAObjectItem *parent;
}
	ReaderData;

static NAObjectItem *read_item( NagpGConfProvider *provider, const gchar *path, GSList **messages );

static void          read_done_item_is_writable( const NAIFactoryProvider *provider, NAObjectItem *item, ReaderData *data, GSList **messages );
static void          read_done_action_load_profiles_from_list( const NAIFactoryProvider *provider, NAObjectAction *action, ReaderData *data, GSList **messages );
static void          read_done_action_load_profile( const NAIFactoryProvider *provider, ReaderData *data, const gchar *path, GSList **messages );
static void          read_done_profile_attach_profile( const NAIFactoryProvider *provider, NAObjectProfile *profile, ReaderData *data, GSList **messages );

static void          convert_pre_v3_parameters( NAObjectProfile *profile );
static void          convert_pre_v3_parameters_str( gchar *str );
static NADataBoxed  *get_boxed_from_path( const NagpGConfProvider *provider, const gchar *path, ReaderData *reader_data, const NADataDef *def );
static gboolean      is_key_writable( NagpGConfProvider *gconf, const gchar *key );

/*
 * nagp_iio_provider_read_items:
 *
 * Note that whatever be the version of the readen action, it will be
 * stored as a #NAObjectAction and its set of #NAObjectProfile of the same,
 * latest, version of these classes.
 */
GList *
nagp_iio_provider_read_items( const NAIIOProvider *provider, GSList **messages )
{
	static const gchar *thisfn = "nagp_gconf_provider_iio_provider_read_items";
	NagpGConfProvider *self;
	GList *items_list = NULL;
	GSList *listpath, *ip;
	NAObjectItem *item;

	g_debug( "%s: provider=%p, messages=%p", thisfn, ( void * ) provider, ( void * ) messages );

	g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider ), NULL );
	g_return_val_if_fail( NAGP_IS_GCONF_PROVIDER( provider ), NULL );
	self = NAGP_GCONF_PROVIDER( provider );

	if( !self->private->dispose_has_run ){

		listpath = na_gconf_utils_get_subdirs( self->private->gconf, NAGP_CONFIGURATIONS_PATH );

		for( ip = listpath ; ip ; ip = ip->next ){

			item = read_item( self, ( const gchar * ) ip->data, messages );
			if( item ){
				items_list = g_list_prepend( items_list, item );
			}
		}

		na_gconf_utils_free_subdirs( listpath );
	}

	g_debug( "%s: count=%d", thisfn, g_list_length( items_list ));
	return( items_list );
}

/*
 * path is here the full path to an item
 */
static NAObjectItem *
read_item( NagpGConfProvider *provider, const gchar *path, GSList **messages )
{
	static const gchar *thisfn = "nagp_gconf_provider_read_item";
	NAObjectItem *item;
	gchar *full_path;
	gchar *type;
	gchar *id;
	ReaderData *data;

	g_debug( "%s: provider=%p, path=%s", thisfn, ( void * ) provider, path );
	g_return_val_if_fail( NAGP_IS_GCONF_PROVIDER( provider ), NULL );
	g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider ), NULL );
	g_return_val_if_fail( !provider->private->dispose_has_run, NULL );

	full_path = gconf_concat_dir_and_key( path, NAGP_ENTRY_TYPE );
	type = na_gconf_utils_read_string( provider->private->gconf, full_path, TRUE, NAGP_VALUE_TYPE_ACTION );
	g_free( full_path );
	item = NULL;

	/* a menu may have 'Action' or 'Menu' type ; defaults to Action
	 */
	if( !type || !strlen( type ) || !strcmp( type, NAGP_VALUE_TYPE_ACTION )){
		item = NA_OBJECT_ITEM( na_object_action_new());

	} else if( !strcmp( type, NAGP_VALUE_TYPE_MENU )){
		item = NA_OBJECT_ITEM( na_object_menu_new());

	} else {
		g_warning( "%s: unknown type '%s' at %s", thisfn, type, path );
	}

	g_free( type );

	if( item ){
		id = g_path_get_basename( path );
		na_object_set_id( item, id );
		g_free( id );

		data = g_new0( ReaderData, 1 );
		data->path = ( gchar * ) path;
		data->entries = na_gconf_utils_get_entries( provider->private->gconf, path );
		na_gconf_utils_dump_entries( data->entries );

		na_ifactory_provider_read_item(
				NA_IFACTORY_PROVIDER( provider ),
				data,
				NA_IFACTORY_OBJECT( item ),
				messages );

		na_gconf_utils_free_entries( data->entries );
		g_free( data );
	}

	return( item );
}

NADataBoxed *
nagp_reader_read_data( const NAIFactoryProvider *provider, void *reader_data, const NAIFactoryObject *object, const NADataDef *def, GSList **messages )
{
	static const gchar *thisfn = "nagp_reader_read_data";
	NADataBoxed *boxed;

	g_return_val_if_fail( NA_IS_IFACTORY_PROVIDER( provider ), NULL );
	g_return_val_if_fail( NA_IS_IFACTORY_OBJECT( object ), NULL );

	/*g_debug( "%s: reader_data=%p, object=%p (%s), data=%s",
			thisfn,
			( void * ) reader_data,
			( void * ) object, G_OBJECT_TYPE_NAME( object ),
			def->name );*/

	if( !def->gconf_entry || !strlen( def->gconf_entry )){
		g_warning( "%s: GConf entry is not set for NADataDef %s", thisfn, def->name );
		return( NULL );
	}

	boxed = get_boxed_from_path(
			NAGP_GCONF_PROVIDER( provider ), (( ReaderData * ) reader_data )->path, reader_data, def );

	return( boxed );
}

void
nagp_reader_read_done( const NAIFactoryProvider *provider, void *reader_data, const NAIFactoryObject *object, GSList **messages  )
{
	static const gchar *thisfn = "nagp_reader_read_done";

	g_return_if_fail( NA_IS_IFACTORY_PROVIDER( provider ));
	g_return_if_fail( NA_IS_IFACTORY_OBJECT( object ));

	g_debug( "%s: provider=%p, reader_data=%p, object=%p (%s), messages=%p",
			thisfn,
			( void * ) provider,
			( void * ) reader_data,
			( void * ) object, G_OBJECT_TYPE_NAME( object ),
			( void * ) messages );

	if( NA_IS_OBJECT_ITEM( object )){
		read_done_item_is_writable( provider, NA_OBJECT_ITEM( object ), ( ReaderData * ) reader_data, messages );
	}

	if( NA_IS_OBJECT_ACTION( object )){
		read_done_action_load_profiles_from_list( provider, NA_OBJECT_ACTION( object ), ( ReaderData * ) reader_data, messages );
	}

	if( NA_IS_OBJECT_PROFILE( object )){
		read_done_profile_attach_profile( provider, NA_OBJECT_PROFILE( object ), ( ReaderData * ) reader_data, messages );
	}

	g_debug( "quitting nagp_read_done for %s at %p", G_OBJECT_TYPE_NAME( object ), ( void * ) object );
}

static void
read_done_item_is_writable( const NAIFactoryProvider *provider, NAObjectItem *item, ReaderData *data, GSList **messages )
{
	GSList *ie;
	gboolean writable;
	GConfEntry *gconf_entry;
	const gchar *key;

	/* check for writability of this item
	 * item is writable if and only if all entries are themselves writable
	 */
	writable = TRUE;
	for( ie = data->entries ; ie && writable ; ie = ie->next ){
		gconf_entry = ( GConfEntry * ) ie->data;
		key = gconf_entry_get_key( gconf_entry );
		writable = is_key_writable( NAGP_GCONF_PROVIDER( provider ), key );
	}

	g_debug( "nagp_reader_read_done_item: writable=%s", writable ? "True":"False" );
	na_object_set_readonly( item, !writable );
}

static void
read_done_action_load_profiles_from_list( const NAIFactoryProvider *provider, NAObjectAction *action, ReaderData *data, GSList **messages )
{
	GSList *order;
	GSList *list_profiles;
	GSList *ip;
	gchar *profile_id;
	gchar *profile_path;
	NAObjectId *found;

	data->parent = NA_OBJECT_ITEM( action );
	order = na_object_get_items_slist( action );
	list_profiles = na_gconf_utils_get_subdirs( NAGP_GCONF_PROVIDER( provider )->private->gconf, data->path );

	/* read profiles in the specified order
	 * as a protection against bugs in NACT, we check that profile has not
	 * already been loaded
	 */
	for( ip = order ; ip ; ip = ip->next ){
		profile_id = ( gchar * ) ip->data;
		found = na_object_get_item( action, profile_id );
		if( !found ){
			g_debug( "nagp_reader_read_done_action: loading profile=%s", profile_id );
			profile_path = gconf_concat_dir_and_key( data->path, profile_id );
			read_done_action_load_profile( provider, data, profile_path, messages );
			g_free( profile_path );
		}
	}

	/* append other profiles
	 * this is mandatory for pre-2.29 actions which introduced order of profiles
	 */
	for( ip = list_profiles ; ip ; ip = ip->next ){
		profile_id = g_path_get_basename(( const gchar * ) ip->data );
		found = na_object_get_item( action, profile_id );
		if( !found ){
			g_debug( "nagp_reader_read_done_action: loading profile=%s", profile_id );
			read_done_action_load_profile( provider, data, ( const gchar * ) ip->data, messages );
		}
		g_free( profile_id );
	}
}

static void
read_done_action_load_profile( const NAIFactoryProvider *provider, ReaderData *data, const gchar *path, GSList **messages )
{
	gchar *id;
	ReaderData *profile_data;

	NAObjectProfile *profile = na_object_profile_new();

	id = g_path_get_basename( path );
	na_object_set_id( profile, id );
	g_free( id );

	profile_data = g_new0( ReaderData, 1 );
	profile_data->parent = data->parent;
	profile_data->path = ( gchar * ) path;
	profile_data->entries = na_gconf_utils_get_entries( NAGP_GCONF_PROVIDER( provider )->private->gconf, path );

	na_ifactory_provider_read_item(
			NA_IFACTORY_PROVIDER( provider ),
			profile_data,
			NA_IFACTORY_OBJECT( profile ),
			messages );

	na_gconf_utils_free_entries( profile_data->entries );
	g_free( profile_data );
}

static void
read_done_profile_attach_profile( const NAIFactoryProvider *provider, NAObjectProfile *profile, ReaderData *data, GSList **messages )
{
	guint iversion;
	gchar *version;

	g_debug( "nagp_reader_read_done_attach_profile: profile=%p", ( void * ) profile );

	na_object_attach_profile( data->parent, profile );

	/* converts pre-v3 parameters
	 */
	version = na_object_get_version( data->parent );
	iversion = na_object_get_iversion( data->parent );
	g_debug( "nagp_reader_read_done_attach_profile: version=%s, iversion=%d", version, iversion );
	if( iversion < 3 ){
		convert_pre_v3_parameters( profile );
	}
}

/*
 * starting wih v3, parameters are relabeled
 *   pre-v3 parameters					post-v3 parameters
 *   ----------------------------		-----------------------------------
 *   									%b: (first) basename	(new)
 *   									%B: list of basenames	(was %m)
 *   									%c: count				(new)
 * 	 %d: (first) base directory			...................		(unchanged)
 * 										%D: list of base dir	(new)
 *   %f: (first) pathname				...................		(unchanged)
 *   									%F: list of pathnames	(was %M)
 *   %h: (first) hostname				...................		(unchanged)
 *   %m: list of basenames	-> %B		-						(removed)
 *   %M: list of pathnames	-> %F		-						(removed)
 *   									%n: (first) username	(was %U)
 *   %p: (first) port number			...................		(unchanged)
 *   %R: list of URIs		-> %U		-						(removed)
 *   %s: (first) scheme					...................		(unchanged)
 *   %u: (first) URI					...................		(unchanged)
 *   %U: (first) username	-> %n		%U: list of URIs		(was %R)
 *   									%w: (first) basename w/o ext.	(new)
 *   									%W: list of basenames w/o ext.	(new)
 *   									%x: (first) extension	(new)
 *   									%X: list of extensions	(new)
 *   %%: %								...................		(unchanged)
 *
 * For pre-v3 items,
 * - substitute %m with %B
 * - substitute %M with %F
 * - substitute %U with %n
 * - substitute %R with %U
 *
 * Note that pre-v3 items only have parameters in the command and path fields.
 * Are only located in 'profile' objects.
 * Are only found in GConf provider, as .desktop files have been simultaneously
 * introduced.
 */
static void
convert_pre_v3_parameters( NAObjectProfile *profile )
{
	gchar *path = na_object_get_path( profile );
	convert_pre_v3_parameters_str( path );
	na_object_set_path( profile, path );
	g_free( path );

	gchar *parms = na_object_get_parameters( profile );
	convert_pre_v3_parameters_str( parms );
	na_object_set_parameters( profile, parms );
	g_free( parms );
}

static void
convert_pre_v3_parameters_str( gchar *str )
{
	gchar *iter = str;

	while(( iter = g_strstr_len( iter, strlen( iter ), "%" )) != NULL ){

		switch( iter[1] ){

			/* %m (list of basenames) becomes %B
			 */
			case 'm':
				iter[1] = 'B';
				break;

			/* %M (list of filenames) becomes %F
			 */
			case 'M':
				iter[1] = 'F';
				break;

			/* %U ((first) username) becomes %n
			 */
			case 'U':
				iter[1] = 'n';
				break;

			/* %R (list of URIs) becomes %U
			 */
			case 'R':
				iter[1] = 'U';
				break;
		}

		iter += 2;
	}
}

static NADataBoxed *
get_boxed_from_path( const NagpGConfProvider *provider, const gchar *path, ReaderData *reader_data, const NADataDef *def )
{
	static const gchar *thisfn = "nagp_reader_get_boxed_from_path";
	NADataBoxed *boxed;
	gboolean have_entry;
	gchar *str_value;
	gboolean bool_value;
	GSList *slist_value;
	gint int_value;

	boxed = NULL;
	have_entry = na_gconf_utils_has_entry( reader_data->entries, def->gconf_entry );
	g_debug( "%s: entry=%s, have_entry=%s", thisfn, def->gconf_entry, have_entry ? "True":"False" );

	if( have_entry ){
		boxed = na_data_boxed_new( def );
		gchar *entry_path = gconf_concat_dir_and_key( path, def->gconf_entry );

		switch( def->type ){

			case NAFD_TYPE_STRING:
			case NAFD_TYPE_LOCALE_STRING:
				str_value = na_gconf_utils_read_string( provider->private->gconf, entry_path, TRUE, NULL );
				g_debug( "%s: entry=%s, value=%s", thisfn, def->gconf_entry, str_value );
				na_data_boxed_set_from_string( boxed, str_value );
				g_free( str_value );
				break;

			case NAFD_TYPE_BOOLEAN:
				bool_value = na_gconf_utils_read_bool( provider->private->gconf, entry_path, TRUE, FALSE );
				na_data_boxed_set_from_void( boxed, GUINT_TO_POINTER( bool_value ));
				break;

			case NAFD_TYPE_STRING_LIST:
				slist_value = na_gconf_utils_read_string_list( provider->private->gconf, entry_path );
				na_data_boxed_set_from_void( boxed, slist_value );
				na_core_utils_slist_free( slist_value );
				break;

			case NAFD_TYPE_UINT:
				int_value = na_gconf_utils_read_int( provider->private->gconf, entry_path, TRUE, 0 );
				na_data_boxed_set_from_void( boxed, GUINT_TO_POINTER( int_value ));
				break;

			default:
				g_warning( "%s: unknown type=%u for %s", thisfn, def->type, def->name );
				g_free( boxed );
				boxed = NULL;
		}

		g_free( entry_path );
	}

	return( boxed );
}

/*
 * key must be an existing entry (not a dir) to get a relevant return
 * value ; else we get FALSE
 */
static gboolean
is_key_writable( NagpGConfProvider *gconf, const gchar *key )
{
	static const gchar *thisfn = "nagp_read_is_key_writable";
	GError *error = NULL;
	gboolean is_writable;

	is_writable = gconf_client_key_is_writable( gconf->private->gconf, key, &error );
	if( error ){
		g_warning( "%s: gconf_client_key_is_writable: %s", thisfn, error->message );
		g_error_free( error );
		error = NULL;
		is_writable = FALSE;
	}

	return( is_writable );
}
