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

#include "nact-action.h"
#include "nact-action-profile.h"
#include "nact-gconf.h"
#include "nact-gconf-keys.h"
#include "nact-iio-client.h"
#include "nact-iio-provider.h"
#include "nact-pivot.h"
#include "uti-lists.h"

struct NactGConfPrivate {
	gboolean     dispose_has_run;
	NactPivot   *pivot;
	GConfClient *gconf;
	guint        notify_id;
};

struct NactGConfClassPrivate {
};

typedef struct {
	gchar *key;
	gchar *path;
}
	NactGConfIO;

enum {
	PROP_PIVOT = 1
};

static GObjectClass *st_parent_class = NULL;

static GType          register_type( void );
static void           class_init( NactGConfClass *klass );
static void           iio_provider_iface_init( NactIIOProviderInterface *iface );
static void           instance_init( GTypeInstance *instance, gpointer klass );
static void           instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void           instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void           instance_dispose( GObject *object );
static void           instance_finalize( GObject *object );
static guint          install_gconf_watch( NactGConf *gconf );
static void           remove_gconf_watch( NactGConf *gconf );

static void           actions_changed_cb( GConfClient *client, guint cnxn_id, GConfEntry *entry, gpointer user_data );

static void           free_keys_values( GSList *keys );
static GSList        *load_keys_values( const NactGConf *gconf, const gchar *path );
static GSList        *load_subdirs( const NactGConf *gconf, const gchar *path );
static gchar         *path_to_key( const gchar *path );
static NactGConfIO   *path_to_struct( const gchar *path );
static void           set_item_properties( NactObject *object, GSList *properties );
static NactPivotValue *value_to_pivot( const GConfValue *value );

static GSList        *do_load_actions( NactIIOProvider *provider );
static   void         do_load_action_properties( NactIIOClient *client );
static GSList        *do_load_profiles( NactIIOClient *client );
static void           do_load_profile_properties( NactObject *profile );
static void           do_release_data( NactIIOClient *client );

NactGConf *
nact_gconf_new( const GObject *pivot )
{
	g_assert( NACT_IS_PIVOT( pivot ));
	return( g_object_new( NACT_GCONF_TYPE, "pivot", pivot, NULL ));
}

GType
nact_gconf_get_type( void )
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
		sizeof( NactGConfClass ),
		NULL,
		NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactGConf ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	GType type = g_type_register_static( G_TYPE_OBJECT, "NactGConf", &info, 0 );

	static const GInterfaceInfo iio_provider_iface_info = {
		( GInterfaceInitFunc ) iio_provider_iface_init,
		NULL,
		NULL
	};

	g_type_add_interface_static( type, NACT_IIO_PROVIDER_TYPE, &iio_provider_iface_info );

	return( type );
}

static void
class_init( NactGConfClass *klass )
{
	static const gchar *thisfn = "nact_gconf_class_init";
	g_debug( "%s: klass=%p", thisfn, klass );

	st_parent_class = g_type_class_peek_parent( klass );

	GObjectClass *object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
	object_class->set_property = instance_set_property;
	object_class->get_property = instance_get_property;

	GParamSpec *spec;
	spec = g_param_spec_pointer(
			"pivot",
			"pivot",
			"A pointer to NactPivot object",
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_PIVOT, spec );

	klass->private = g_new0( NactGConfClassPrivate, 1 );
}

static void
iio_provider_iface_init( NactIIOProviderInterface *iface )
{
	static const gchar *thisfn = "nact_gconf_iio_provider_iface_init";
	g_debug( "%s: iface=%p", thisfn, iface );

	iface->load_actions = do_load_actions;
	iface->load_action_properties = do_load_action_properties;
	iface->load_profiles = do_load_profiles;
	iface->load_profile_properties = do_load_profile_properties;
	iface->release_data = do_release_data;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_gconf_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );

	g_assert( NACT_IS_GCONF( instance ));
	NactGConf *self = NACT_GCONF( instance );

	self->private = g_new0( NactGConfPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->gconf = gconf_client_get_default();
	self->private->notify_id = install_gconf_watch( self );
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	g_assert( NACT_IS_GCONF( object ));
	NactGConf *self = NACT_GCONF( object );

	switch( property_id ){
		case PROP_PIVOT:
			g_value_set_pointer( value, self->private->pivot );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	g_assert( NACT_IS_GCONF( object ));
	NactGConf *self = NACT_GCONF( object );

	switch( property_id ){
		case PROP_PIVOT:
			self->private->pivot = g_value_get_pointer( value );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_dispose( GObject *object )
{
	g_assert( NACT_IS_GCONF( object ));
	NactGConf *self = NACT_GCONF( object );

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
	g_assert( NACT_IS_GCONF( object ));
	/*NactGConf *self = NACT_GCONF( object );*/

	/* chain call to parent class */
	if( st_parent_class->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

static guint
install_gconf_watch( NactGConf *gconf )
{
	static const gchar *thisfn = "install_gconf_watch";
	GError *error = NULL;

	gconf_client_add_dir(
			gconf->private->gconf, NACT_GCONF_CONFIG_PATH, GCONF_CLIENT_PRELOAD_RECURSIVE, &error );
	if( error ){
		g_error( "%s: error=%s", thisfn, error->message );
		g_error_free( error );
		return( 0 );
	}

	guint notify_id =
		gconf_client_notify_add(
			gconf->private->gconf,
			NACT_GCONF_CONFIG_PATH,
			( GConfClientNotifyFunc ) actions_changed_cb,
			gconf->private->pivot,
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
remove_gconf_watch( NactGConf *gconf )
{
	static const gchar *thisfn = "remove_gconf_watch";
	GError *error = NULL;

	if( gconf->private->notify_id ){
		gconf_client_notify_remove( gconf->private->gconf, gconf->private->notify_id );
	}

	gconf_client_remove_dir( gconf->private->gconf, NACT_GCONF_CONFIG_PATH, &error );
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
 * xml file in gconf, of gconf is directly edited), we'd have to rely
 * on the key of the value
 *
 * user_data is a ptr to NactPivot object.
 */
static void
actions_changed_cb( GConfClient *client, guint cnxn_id, GConfEntry *entry, gpointer user_data )
{
	static const gchar *thisfn = "actions_changed_cb";
	g_debug( "%s: client=%p, cnxnid=%u, entry=%p, user_data=%p", thisfn, client, cnxn_id, entry, user_data );

	g_assert( NACT_IS_PIVOT( user_data ));

	const GConfValue* value = gconf_entry_get_value( entry );
	const gchar *path = gconf_entry_get_key( entry );
	const gchar *subpath = path + strlen( NACT_GCONF_CONFIG_PATH ) + 1;

	gchar **split = g_strsplit( subpath, "/", 2 );
	gchar *key = g_strdup( split[0] );
	g_strfreev( split );

	/*nact_pivot_on_action_changed( NACT_PIVOT( user_data ), key, parm, value );*/
	g_free( key );

	/*NactGConf *gconf = NACT_GCONF( user_data );*/

	/* The Key value format is XXX:YYYY-YYYY-... where XXX is an number incremented to make key
	 * effectively changed each time and YYYY-YYYY-... is the modified uuid */
	const gchar* notify_value = gconf_value_get_string (value);
	g_debug( "%s: notify_value='%s', key='%s'", thisfn, notify_value, key );
	/*const gchar* uuid = notify_value + 4;*/

	/* Get the modified action from the internal list if any */
	/*NautilusActionsConfigAction *old_action = nautilus_actions_config_get_action (config, uuid);*/

	/* Get the new version from GConf if any */
	/*NautilusActionsConfigAction *new_action = nautilus_actions_config_gconf_get_action (NAUTILUS_ACTIONS_CONFIG_GCONF (config), uuid);*/

	/* If modified action is unknown internally */
	/*if (old_action == NULL)
	{
		if (new_action != NULL)
		{*/
			/* new action */
			/*nautilus_actions_config_add_action (config, new_action, NULL);
		}
		else
		{*/
			/* This case should not happen */
			/*g_assert_not_reached ();
		}
	}
	else
	{
		if (new_action != NULL)
		{*/
			/* action modified */
			/*nautilus_actions_config_update_action (config, new_action);
		}
		else
		{*/
			/* action removed */
			/*nautilus_actions_config_remove_action (config, uuid);
		}
	}*/

	/* Add & change handler duplicate actions before adding them,
	 * so we can free the new action
	 */
	/*nautilus_actions_config_action_free (new_action);*/
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
 * load all the key=value pairs of this key (specified as a full path)
 * The list is not recursive, it contains only the immediate children of
 * path.
 * To free the returned hash table, call free_key_values
 */
static GSList *
load_keys_values( const NactGConf *gconf, const gchar *path )
{
	static const gchar *thisfn = "nact_gconf_load_keys_values";

	GError *error = NULL;
	GSList *list_path = gconf_client_all_entries( gconf->private->gconf, path, &error );
	if( error ){
		g_error( "%s: path=%s, error=%s", thisfn, path, error->message );
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
load_subdirs( const NactGConf *gconf, const gchar *path )
{
	static const gchar *thisfn = "nact_gconf_load_subdirs";

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

/*
 * set the item properties into the object
 */
static void
set_item_properties( NactObject *object, GSList *properties )
{
	g_assert( NACT_IS_OBJECT( object ));

	GSList *item;
	for( item = properties ; item != NULL ; item = item->next ){

		GConfEntry *entry = ( GConfEntry * ) item->data;
		const char *key = gconf_entry_get_key( entry );
		NactPivotValue *value = value_to_pivot( gconf_entry_get_value( entry ));

		if( value ){
			switch( value->type ){

				case NACT_PIVOT_STR:
				case NACT_PIVOT_BOOL:
				case NACT_PIVOT_STRLIST:
					g_object_set( G_OBJECT( object ), key, value->data, NULL );
					break;

				default:
					g_assert_not_reached();
					break;
			}
			nact_pivot_free_pivot_value( value );
		}
	}
}

/*
 * convert a GConfValue to our internal data type
 */
static NactPivotValue *
value_to_pivot( const GConfValue *value )
{
	NactPivotValue *pivot_value = NULL;
	GSList *listvalues, *iv, *strings;

	switch( value->type ){

		case GCONF_VALUE_STRING:
			pivot_value = g_new0( NactPivotValue, 1 );
			pivot_value->type = NACT_PIVOT_STR;
			pivot_value->data = ( gpointer ) g_strdup( gconf_value_get_string( value ));
			break;

		case GCONF_VALUE_BOOL:
			pivot_value = g_new0( NactPivotValue, 1 );
			pivot_value->type = NACT_PIVOT_BOOL;
			pivot_value->data = ( gpointer ) gconf_value_get_bool( value );
			break;

		case GCONF_VALUE_LIST:
			listvalues = gconf_value_get_list( value );
			strings = NULL;
			for( iv = listvalues ; iv != NULL ; iv = iv->next ){
				strings = g_slist_prepend( strings,
						( gpointer ) gconf_value_get_string(( GConfValue * ) iv->data ));
			}

			pivot_value = g_new0( NactPivotValue, 1 );
			pivot_value->type = NACT_PIVOT_STRLIST;
			pivot_value->data = ( gpointer ) nactuti_duplicate_string_list( strings );
			/*nactuti_free_string_list( strings );*/
			break;

		default:
			g_assert_not_reached();
			break;
	}

	return( pivot_value );
}

/*
 * NactIIOProviderInterface implementation
 * load the list of actions and returns them as a GSList
 */
static GSList *
do_load_actions( NactIIOProvider *provider )
{
	static const gchar *thisfn = "nacf_gconf_do_load_actions";
	g_debug( "%s: provider=%p", thisfn, provider );

	g_assert( NACT_IS_IIO_PROVIDER( provider ));
	g_assert( NACT_IS_GCONF( provider ));
	NactGConf *self = NACT_GCONF( provider );

	GSList *items = NULL;

	GSList *listpath = load_subdirs( self, NACT_GCONF_CONFIG_PATH );
	GSList *ip;
	for( ip = listpath ; ip ; ip = ip->next ){

		NactGConfIO *io = path_to_struct(( const gchar * ) ip->data );

		NactAction *action = nact_action_new( self, io );
		nact_action_load( action );

		items = g_slist_prepend( items, action );
	}
	nactuti_free_string_list( listpath );

	return( items );
}

/*
 * NactIIOProviderInterface implementation
 * load and set the properties of the specified action
 */
static void
do_load_action_properties( NactIIOClient *client )
{
	static const gchar *thisfn = "nacf_gconf_do_load_action_properties";
	g_debug( "%s: client=%p", thisfn, client );

	g_assert( NACT_IS_IIO_CLIENT( client ));
	g_assert( NACT_IS_ACTION( client ));
	NactAction *action = NACT_ACTION( client );

	NactGConfIO *io = ( NactGConfIO * ) nact_iio_client_get_provider_data( client );
	NactGConf *self = NACT_GCONF( nact_iio_client_get_provider_id( client ));

	GSList *properties = load_keys_values( self, io->path );

	set_item_properties( NACT_OBJECT( action ), properties );

	free_keys_values( properties );
}

/*
 * NactIIOProviderInterface implementation
 * load the list of profiles for an action and returns them as a GSList
 */
static GSList *
do_load_profiles( NactIIOClient *client )
{
	static const gchar *thisfn = "nacf_gconf_do_load_profiles";
	g_debug( "%s: client=%p", thisfn, client );

	g_assert( NACT_IS_IIO_CLIENT( client ));
	g_assert( NACT_IS_ACTION( client ));
	NactAction *action = NACT_ACTION( client );

	NactGConfIO *io = ( NactGConfIO * ) nact_iio_client_get_provider_data( client );
	NactGConf *self = NACT_GCONF( nact_iio_client_get_provider_id( client ));

	GSList *items = NULL;

	GSList *listpath = load_subdirs( self, io->path );
	GSList *ip;
	for( ip = listpath ; ip ; ip = ip->next ){

		gchar *key = path_to_key(( const gchar * ) ip->data );
		NactActionProfile *profile = nact_action_profile_new( NACT_OBJECT( action ), key );
		nact_action_profile_load( NACT_OBJECT( profile ));

		items = g_slist_prepend( items, profile );
	}
	nactuti_free_string_list( listpath );

	return( items );
}

/*
 * NactIIOProviderInterface implementation
 * load and set the properties of the specified profile
 */
static void
do_load_profile_properties( NactObject *profile )
{
	static const gchar *thisfn = "nacf_gconf_do_load_profile_properties";
	g_debug( "%s: profile=%p", thisfn, profile );

	g_assert( NACT_IS_ACTION_PROFILE( profile ));

	NactAction *action =
		NACT_ACTION( nact_action_profile_get_action( NACT_ACTION_PROFILE( profile )));

	g_assert( NACT_IS_ACTION( action ));
	g_assert( NACT_IS_IIO_CLIENT( action ));

	NactIIOClient *client = NACT_IIO_CLIENT( action );

	NactGConfIO *io = ( NactGConfIO * ) nact_iio_client_get_provider_data( client );
	NactGConf *self = NACT_GCONF( nact_iio_client_get_provider_id( client ));

	gchar *path = g_strdup_printf(
			"%s/%s", io->path, nact_action_profile_get_id( NACT_ACTION_PROFILE( profile )));

	GSList *properties = load_keys_values( self, path );

	g_free( path );

	set_item_properties( profile, properties );

	free_keys_values( properties );
}

/*
 * NactIIOProviderInterface implementation
 */
static void
do_release_data( NactIIOClient *client )
{
	g_assert( NACT_IS_IIO_CLIENT( client ));

	NactGConfIO *io = ( NactGConfIO * ) nact_iio_client_get_provider_data( client );

	g_free( io->key );
	g_free( io->path );
	g_free( io );
}
