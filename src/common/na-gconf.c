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

#include <gconf/gconf.h>
#include <gconf/gconf-client.h>

#include "na-action.h"
#include "na-action-profile.h"
#include "na-gconf.h"
#include "na-gconf-keys.h"
#include "na-iio-provider.h"
#include "na-utils.h"

/* private class data
 */
struct NAGConfClassPrivate {
};

/* private instance data
 */
struct NAGConfPrivate {
	gboolean     dispose_has_run;

	/* instance to be notified of an action modification
	 */
	gpointer  notified;

	GConfClient *gconf;
	guint        notify_id;
};

/* private instance properties
 */
enum {
	PROP_NOTIFIED = 1
};

#define PROP_NOTIFIED_STR				"to-be-notified"

static GObjectClass *st_parent_class = NULL;

static GType            register_type( void );
static void             class_init( NAGConfClass *klass );
static void             iio_provider_iface_init( NAIIOProviderInterface *iface );
static void             instance_init( GTypeInstance *instance, gpointer klass );
static void             instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void             instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void             instance_dispose( GObject *object );
static void             instance_finalize( GObject *object );

static GSList          *do_read_actions( NAIIOProvider *provider );
static void             load_action_properties( NAGConf *gconf, NAAction *action );
static GSList          *load_profiles( NAGConf *gconf, NAAction *action );
static void             load_profile_properties( NAGConf *gconf, NAActionProfile *profile );
static GSList          *load_subdirs( const NAGConf *gconf, const gchar *path );
static GSList          *load_keys_values( const NAGConf *gconf, const gchar *path );
static void             free_keys_values( GSList *keys );
static gchar           *path_to_key( const gchar *path );
static void             set_item_properties( NAObject *object, GSList *properties );
static NAPivotNotify *entry_to_notify( const GConfEntry *entry );

static guint            install_gconf_watch( NAGConf *gconf );
static void             remove_gconf_watch( NAGConf *gconf );
static void             action_changed_cb( GConfClient *client, guint cnxn_id, GConfEntry *entry, gpointer user_data );

GType
na_gconf_get_type( void )
{
	static GType object_type = 0;

	if( !object_type ){
		object_type = register_type();
	}

	return( object_type );
}

static GType
register_type( void )
{
	static GTypeInfo info = {
		sizeof( NAGConfClass ),
		NULL,
		NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NAGConf ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	GType type = g_type_register_static( G_TYPE_OBJECT, "NAGConf", &info, 0 );

	static const GInterfaceInfo iio_provider_iface_info = {
		( GInterfaceInitFunc ) iio_provider_iface_init,
		NULL,
		NULL
	};

	g_type_add_interface_static( type, NA_IIO_PROVIDER_TYPE, &iio_provider_iface_info );

	return( type );
}

static void
class_init( NAGConfClass *klass )
{
	static const gchar *thisfn = "na_gconf_class_init";
	g_debug( "%s: klass=%p", thisfn, klass );

	st_parent_class = g_type_class_peek_parent( klass );

	GObjectClass *object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
	object_class->set_property = instance_set_property;
	object_class->get_property = instance_get_property;

	GParamSpec *spec;
	spec = g_param_spec_pointer(
			PROP_NOTIFIED_STR,
			PROP_NOTIFIED_STR,
			"A pointer to a GObject which will receive action_changed notifications",
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_NOTIFIED, spec );

	klass->private = g_new0( NAGConfClassPrivate, 1 );
}

static void
iio_provider_iface_init( NAIIOProviderInterface *iface )
{
	static const gchar *thisfn = "na_gconf_iio_provider_iface_init";
	g_debug( "%s: iface=%p", thisfn, iface );

	iface->read_actions = do_read_actions;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "na_gconf_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );

	g_assert( NA_IS_GCONF( instance ));
	NAGConf *self = NA_GCONF( instance );

	self->private = g_new0( NAGConfPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->gconf = gconf_client_get_default();
	self->private->notify_id = install_gconf_watch( self );
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	g_assert( NA_IS_GCONF( object ));
	NAGConf *self = NA_GCONF( object );

	switch( property_id ){
		case PROP_NOTIFIED:
			g_value_set_pointer( value, self->private->notified );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	g_assert( NA_IS_GCONF( object ));
	NAGConf *self = NA_GCONF( object );

	switch( property_id ){
		case PROP_NOTIFIED:
			self->private->notified = g_value_get_pointer( value );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_dispose( GObject *object )
{
	g_assert( NA_IS_GCONF( object ));
	NAGConf *self = NA_GCONF( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* release the GConf connexion */
		remove_gconf_watch( self );
		g_object_unref( self->private->gconf );

		/* chain up to the parent class */
		G_OBJECT_CLASS( st_parent_class )->dispose( object );
	}
}

static void
instance_finalize( GObject *object )
{
	g_assert( NA_IS_GCONF( object ));
	/*NAGConf *self = NA_GCONF( object );*/

	/* chain call to parent class */
	if( st_parent_class->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * Allocate a new GConf object.
 *
 * @handler: the GObject which is to be notified when an action is
 * added, modified or removed in underlying GConf system.
 *
 * The object to be notified will receive a
 * "notify_pivot_of_action_changed" message for each detected
 * modification, with a pointer to a newly allocated NAPivotNotify
 * structure describing the change.
 */
NAGConf *
na_gconf_new( const GObject *handler )
{
	return( g_object_new( NA_GCONF_TYPE, PROP_NOTIFIED_STR, handler, NULL ));
}

/*
 * NAIIOProviderInterface implementation
 * load the list of actions and returns them as a GSList
 */
static GSList *
do_read_actions( NAIIOProvider *provider )
{
	static const gchar *thisfn = "nacf_gconf_do_read_actions";
	g_debug( "%s: provider=%p", thisfn, provider );

	g_assert( NA_IS_IIO_PROVIDER( provider ));
	g_assert( NA_IS_GCONF( provider ));
	NAGConf *self = NA_GCONF( provider );

	GSList *items = NULL;
	GSList *ip;
	GSList *listpath = load_subdirs( self, NA_GCONF_CONFIG_PATH );
	GSList *profiles;

	for( ip = listpath ; ip ; ip = ip->next ){

		gchar *key = path_to_key(( const gchar * ) ip->data );

		NAAction *action = na_action_new( key );

		load_action_properties( self, action );

		profiles = load_profiles( self, action );
		na_action_set_profiles( action, profiles );
		na_action_free_profiles( profiles );

#ifdef NACT_MAINTAINER_MODE
		na_object_dump( NA_OBJECT( action ));
#endif

		items = g_slist_prepend( items, action );
		g_free( key );
	}

	na_utils_free_string_list( listpath );

	return( items );
}

/*
 * load and set the properties of the specified action
 */
static void
load_action_properties( NAGConf *gconf, NAAction *action )
{
	/*static const gchar *thisfn = "nacf_gconf_load_action_properties";
	g_debug( "%s: gconf=%p, action=%p", thisfn, gconf, action );*/

	g_assert( NA_IS_GCONF( gconf ));
	g_assert( NA_IS_ACTION( action ));

	gchar *uuid = na_action_get_uuid( action );
	gchar *path = g_strdup_printf( "%s/%s", NA_GCONF_CONFIG_PATH, uuid );

	GSList *properties = load_keys_values( gconf, path );

	set_item_properties( NA_OBJECT( action ), properties );

	free_keys_values( properties );
	g_free( uuid );
	g_free( path );
}

/*
 * load the list of profiles for an action and returns them as a GSList
 */
static GSList *
load_profiles( NAGConf *gconf, NAAction *action )
{
	/*static const gchar *thisfn = "nacf_gconf_load_profiles";
	g_debug( "%s: gconf=%p, action=%p", thisfn, gconf, action );*/

	g_assert( NA_IS_GCONF( gconf ));
	g_assert( NA_IS_ACTION( action ));

	gchar *uuid = na_action_get_uuid( action );
	gchar *path = g_strdup_printf( "%s/%s", NA_GCONF_CONFIG_PATH, uuid );

	GSList *ip;
	GSList *items = NULL;
	GSList *listpath = load_subdirs( gconf, path );

	for( ip = listpath ; ip ; ip = ip->next ){

		gchar *key = path_to_key(( const gchar * ) ip->data );
		NAActionProfile *profile = na_action_profile_new( NA_OBJECT( action ), key );
		load_profile_properties( gconf, profile );

		items = g_slist_prepend( items, profile );
	}

	na_utils_free_string_list( listpath );
	g_free( path );
	g_free( uuid );

	return( items );
}

/*
 * load and set the properties of the specified profile
 */
static void
load_profile_properties( NAGConf *gconf, NAActionProfile *profile )
{
	/*static const gchar *thisfn = "nacf_gconf_load_profile_properties";
	g_debug( "%s: gconf=%p, profile=%p", thisfn, gconf, profile );*/

	g_assert( NA_IS_GCONF( gconf ));
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	NAAction *action =
		NA_ACTION( na_action_profile_get_action( NA_ACTION_PROFILE( profile )));
	g_assert( NA_IS_ACTION( action ));

	gchar *uuid = na_action_get_uuid( action );
	gchar *path = g_strdup_printf(
			"%s/%s/%s", NA_GCONF_CONFIG_PATH, uuid, na_action_profile_get_name( profile ));

	GSList *properties = load_keys_values( gconf, path );

	set_item_properties( NA_OBJECT( profile ), properties );

	free_keys_values( properties );
	g_free( path );
	g_free( uuid );
}

/*
 * load the keys which are the subdirs of the given path
 * returns a list of keys as full path
 */
GSList *
load_subdirs( const NAGConf *gconf, const gchar *path )
{
	static const gchar *thisfn = "na_gconf_load_subdirs";

	GError *error = NULL;
	GSList *list = gconf_client_all_dirs( gconf->private->gconf, path, &error );
	if( error ){
		g_error( "%s: path=%s, error=%s", thisfn, path, error->message );
		g_error_free( error );
		return(( GSList * ) NULL );
	}

	return( list );
}

/*
 * load all the key=value pairs of this key (specified as a full path)
 * The list is not recursive, it contains only the immediate children of
 * path.
 * To free the returned list, call free_key_values
 */
static GSList *
load_keys_values( const NAGConf *gconf, const gchar *path )
{
	static const gchar *thisfn = "na_gconf_load_keys_values";

	GError *error = NULL;
	GSList *list_path = gconf_client_all_entries( gconf->private->gconf, path, &error );
	if( error ){
		g_error( "%s: path=%s, error=%s", thisfn, path, error->message );
		g_error_free( error );
		return(( GSList * ) NULL );
	}

	return( list_path );
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
 * set the item properties into the object
 * properties is a list of path to entry
 */
static void
set_item_properties( NAObject *object, GSList *properties )
{
	static const gchar *thisfn = "na_gconf_set_item_properties";
	g_assert( NA_IS_OBJECT( object ));

	GSList *item;
	for( item = properties ; item != NULL ; item = item->next ){

		GConfEntry *entry = ( GConfEntry * ) item->data;
		NAPivotNotify *npn = entry_to_notify( entry );
		if( npn->type ){

			switch( npn->type ){
				case NA_PIVOT_STR:
				case NA_PIVOT_BOOL:
				case NA_PIVOT_STRLIST:
					g_object_set( G_OBJECT( object ), npn->parm, npn->data, NULL );
					break;

				default:
					g_debug( "%s: uuid='%s', profile='%s', parm='%s', type=%d, data=%p",
							thisfn, npn->uuid, npn->profile, npn->parm, npn->type, npn->data );
					g_assert_not_reached();
					break;
			}
		}

		na_pivot_free_notify( npn );
	}
}

/*
 * convert a GConfEntry to a structure suitable to notify NAPivot
 *
 * when created or modified, the entry can be of the forms :
 *  key/parm
 *  key/profile/parm with a not null value
 * but when removing an entry, it will be of the form :
 *  key
 *  key/parm
 *  key/profile
 *  key/profile/parm with a null value
 *
 * I don't know any way to choose between key/parm and key/profile
 * as the entry no more exists in GConf and thus cannot be tested
 * -> we will set this as key/parm, letting pivot try to interpret it
 */
static NAPivotNotify *
entry_to_notify( const GConfEntry *entry )
{
	/*static const gchar *thisfn = "na_gconf_entry_to_notify";*/
	GSList *listvalues, *iv, *strings;

	g_assert( entry );

	const gchar *path = gconf_entry_get_key( entry );
	g_assert( path );

	NAPivotNotify *npn = g_new0( NAPivotNotify, 1 );

	const gchar *subpath = path + strlen( NA_GCONF_CONFIG_PATH ) + 1;
	gchar **split = g_strsplit( subpath, "/", -1 );
	/*g_debug( "%s: [0]=%s, [1]=%s", thisfn, split[0], split[1] );*/
	npn->uuid = g_strdup( split[0] );
	if( g_strv_length( split ) == 2 ){
		npn->parm = g_strdup( split[1] );
	} else if( g_strv_length( split ) == 3 ){
		npn->profile = g_strdup( split[1] );
		npn->parm = g_strdup( split[2] );
	}
	g_strfreev( split );

	const GConfValue *value = gconf_entry_get_value( entry );
	if( value ){
		switch( value->type ){

			case GCONF_VALUE_STRING:
				npn->type = NA_PIVOT_STR;
				npn->data = ( gpointer ) g_strdup( gconf_value_get_string( value ));
				break;

			case GCONF_VALUE_BOOL:
				npn->type = NA_PIVOT_BOOL;
				npn->data = ( gpointer ) gconf_value_get_bool( value );
				break;

			case GCONF_VALUE_LIST:
				listvalues = gconf_value_get_list( value );
				strings = NULL;
				for( iv = listvalues ; iv != NULL ; iv = iv->next ){
					strings = g_slist_prepend( strings,
							( gpointer ) gconf_value_get_string(( GConfValue * ) iv->data ));
				}

				npn->type = NA_PIVOT_STRLIST;
				npn->data = ( gpointer ) na_utils_duplicate_string_list( strings );
				/*na_utils_free_string_list( strings );*/
				break;

			default:
				g_assert_not_reached();
				break;
		}
	}
	return( npn );
}

/*
 * note that we need the NAPivot object in action_changed_cb handler
 * but it is initialized as a construction property, and this watch is
 * installed from instance_init, i.e. before properties are set..
 * we so pass NAGConf pointer which is already valid at this time.
 */
static guint
install_gconf_watch( NAGConf *gconf )
{
	static const gchar *thisfn = "na_gconf_install_gconf_watch";
	GError *error = NULL;

	gconf_client_add_dir(
			gconf->private->gconf, NA_GCONF_CONFIG_PATH, GCONF_CLIENT_PRELOAD_RECURSIVE, &error );
	if( error ){
		g_error( "%s: error=%s", thisfn, error->message );
		g_error_free( error );
		return( 0 );
	}

	guint notify_id =
		gconf_client_notify_add(
			gconf->private->gconf,
			NA_GCONF_CONFIG_PATH,
			( GConfClientNotifyFunc ) action_changed_cb,
			gconf,
			NULL,
			&error
		);
	if( error ){
		g_error( "%s: error=%s", thisfn, error->message );
		g_error_free( error );
		return( 0 );
	}

	return( notify_id );
}

static void
remove_gconf_watch( NAGConf *gconf )
{
	static const gchar *thisfn = "na_gconf_remove_gconf_watch";
	GError *error = NULL;

	if( gconf->private->notify_id ){
		gconf_client_notify_remove( gconf->private->gconf, gconf->private->notify_id );
	}

	gconf_client_remove_dir( gconf->private->gconf, NA_GCONF_CONFIG_PATH, &error );
	if( error ){
		g_error( "%s: error=%s", thisfn, error->message );
		g_error_free( error );
	}
}

/*
 * this callback is triggered each time a value is changed under our
 * actions directory
 *
 * if the modification is made from nautilus-actions-config ui, then
 * the callback is triggered several times (one time for each rewritten
 * property) as action/profile are edited as blocs of data ; in this
 * case, the ui takes care (aso of 1.10) of also writing at last a
 * particular key of the form xxx:yyyyyyyy-yyyy-yyyy-..., where :
 *    xxx is a sequential number (inside of the ui session)
 *    yyyyyyyy-yyyy-yyyy-... is the uuid of the involved action
 *
 * this is so a sort of hack which simplifies a lot the notification
 * system (take the new action, replace it in the current global list)
 * but doesn't work if the modification is made from outside of the ui
 *
 * if the modification is made elsewhere (an action is imported as a
 * xml file in gconf, or gconf is directly edited), we'd have to rely
 * only on the standard watch mechanism
 */
static void
action_changed_cb( GConfClient *client, guint cnxn_id, GConfEntry *entry, gpointer user_data )
{
	/*static const gchar *thisfn = "action_changed_cb";
	g_debug( "%s: client=%p, cnxnid=%u, entry=%p, user_data=%p", thisfn, client, cnxn_id, entry, user_data );*/

	g_assert( NA_IS_GCONF( user_data ));
	NAGConf *gconf = NA_GCONF( user_data );

	NAPivotNotify *npn = entry_to_notify( entry );
	g_signal_emit_by_name( gconf->private->notified, "notify_pivot_of_action_changed", npn );
}
