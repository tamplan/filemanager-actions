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
#include <glib/gi18n.h>
#include <string.h>

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
	GConfClient *gconf;
	gpointer     notified;
	guint        watch_id;
};

/* private instance properties
 */
enum {
	PROP_NAGCONF_GCONF = 1,
	PROP_NAGCONF_NOTIFIED,
	PROP_NAGCONF_WATCH_ID
};

#define PROP_NAGCONF_GCONF_STR			"na-gconf-client"
#define PROP_NAGCONF_NOTIFIED_STR		"na-gconf-to-be-notified"
#define PROP_NAGCONF_WATCH_ID_STR		"na-gconf-watch-id"

/* correspondance table for association of GConf entry keys to action
 * and profile properties
 */
/*typedef struct {
	gchar *gconfkey;
	gchar *property;
}
	KeyToPropertyStruct;

static KeyToPropertyStruct st_key_property[] = {
	{ ACTION_VERSION_ENTRY      , PROP_NAACTION_VERSION_STR          },
	{ ACTION_LABEL_ENTRY        , PROP_NAACTION_LABEL_STR            },
	{ ACTION_TOOLTIP_ENTRY      , PROP_NAACTION_TOOLTIP_STR          },
	{ ACTION_ICON_ENTRY         , PROP_NAACTION_ICON_STR             },
	{ ACTION_PROFILE_LABEL_ENTRY, PROP_NAPROFILE_LABEL_STR           },
	{ ACTION_PATH_ENTRY         , PROP_NAPROFILE_PATH_STR            },
	{ ACTION_PARAMETERS_ENTRY   , PROP_NAPROFILE_PARAMETERS_STR      },
	{ ACTION_BASENAMES_ENTRY    , PROP_NAPROFILE_BASENAMES_STR       },
	{ ACTION_MATCHCASE_ENTRY    , PROP_NAPROFILE_MATCHCASE_STR       },
	{ ACTION_MIMETYPES_ENTRY    , PROP_NAPROFILE_MIMETYPES_STR       },
	{ ACTION_ISFILE_ENTRY       , PROP_NAPROFILE_ISFILE_STR          },
	{ ACTION_ISDIR_ENTRY        , PROP_NAPROFILE_ISDIR_STR           },
	{ ACTION_MULTIPLE_ENTRY     , PROP_NAPROFILE_ACCEPT_MULTIPLE_STR },
	{ ACTION_SCHEMES_ENTRY      , PROP_NAPROFILE_SCHEMES_STR         },
	{                       NULL, NULL }
};*/

static GObjectClass *st_parent_class = NULL;

static GType          register_type( void );
static void           class_init( NAGConfClass *klass );
static void           iio_provider_iface_init( NAIIOProviderInterface *iface );
static void           instance_init( GTypeInstance *instance, gpointer klass );
static void           instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void           instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void           instance_dispose( GObject *object );
static void           instance_finalize( GObject *object );

static GSList        *iio_provider_read_actions( const NAIIOProvider *provider );
static gboolean       iio_provider_is_willing_to_write( const NAIIOProvider *provider );
static gboolean       iio_provider_is_writable( const NAIIOProvider *provider, const NAAction *action );
static guint          iio_provider_write_action( const NAIIOProvider *provider, NAAction *action, gchar **message );
static guint          iio_provider_delete_action( const NAIIOProvider *provider, const NAAction *action, gchar **message );

static void           read_action( NAGConf *gconf, NAAction *action, const gchar *path );
static void           read_profile( NAGConf *gconf, NAActionProfile *profile, const gchar *path );
static void           fill_action_core_properties( NAGConf *gconf, NAAction *action, GSList *notifies );
static void           fill_action_v1_properties( NAGConf *gconf, NAAction *action, GSList *notifies );
static void           fill_profile_properties( NAGConf *gconf, NAActionProfile *profile, GSList *notifies );

static GSList        *get_path_subdirs( const NAGConf *gconf, const gchar *path );
static GSList        *get_list_entries( const NAGConf *gconf, const gchar *path );
static void           free_list_entries( GSList *entries );
static NAPivotNotify *entry_to_notify( const GConfEntry *entry );
static GSList        *entries_to_notifies( GSList *entries );
static void           free_list_notifies( GSList *notifies );
static gboolean       search_for_str( GSList *notifies, const gchar *profile, const gchar *key, gchar **value );
static gboolean       search_for_bool( GSList *notifies, const gchar *profile, const gchar *key, gboolean *value );
static gboolean       search_for_list( GSList *notifies, const gchar *profile, const gchar *key, GSList **value );

/*static gboolean       load_action( NAGConf *gconf, NAAction *action, const gchar *path );
static void           load_action_properties( NAGConf *gconf, NAAction *action );
static GSList        *load_profiles( NAGConf *gconf, NAAction *action );
static void           load_profile_properties( NAGConf *gconf, NAActionProfile *profile );
static GSList        *load_keys_values( const NAGConf *gconf, const gchar *path );
static void           free_keys_values( GSList *entries );
static void           set_item_properties( NAObject *object, GSList *properties );
static const gchar   *key_to_property( const gchar *key );
static gboolean       set_action_properties( NAGConf *gconf, NAAction *action, GSList *properties );
static void           load_v1_properties( NAAction *action, const gchar *version, GSList *properties );
static gchar         *search_for_str( GSList *properties, const gchar *profile, const gchar *key );
static gboolean       search_for_bool( GSList *properties, const gchar *profile, const gchar *key );
static GSList        *search_for_list( GSList *properties, const gchar *profile, const gchar *key );
static GSList        *keys_to_notify( GSList *entries );
static void           free_list_notify( GSList *list );
static gboolean       remove_v1_keys( NAGConf *gconf, const NAAction *action, gchar **message );
static gboolean       remove_key( NAGConf *gconf, const gchar *uuid, const gchar *key, gchar **message );*/

static gboolean       key_is_writable( NAGConf *gconf, const gchar *path );
static gboolean       write_v2_keys( NAGConf *gconf, const NAAction *action, gchar **message );
static gboolean       write_str( NAGConf *gconf, const gchar *uuid, const gchar *key, gchar *value, gchar **message );
static gboolean       write_str2( NAGConf *gconf, const gchar *uuid, const gchar *name, const gchar *key, gchar *value, gchar **message );
static gboolean       write_key( NAGConf *gconf, const gchar *path, const gchar *value, gchar **message );
static gboolean       write_bool( NAGConf *gconf, const gchar *uuid, const gchar *name, const gchar *key, gboolean value, gchar **message );
static gboolean       write_list( NAGConf *gconf, const gchar *uuid, const gchar *name, const gchar *key, GSList *value, gchar **message );

static guint          install_gconf_watch( NAGConf *gconf );
static void           install_gconf_watched_dir( NAGConf *gconf );
static void           remove_gconf_watch( NAGConf *gconf );
static void           remove_gconf_watched_dir( NAGConf *gconf );
static void           action_changed_cb( GConfClient *client, guint cnxn_id, GConfEntry *entry, gpointer user_data );

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
	static const gchar *thisfn = "na_gconf_register_type";
	g_debug( "%s", thisfn );

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

	/* implements IIOProvider interface
	 */
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
			PROP_NAGCONF_GCONF_STR,
			"GConfClient object",
			"A pointer to the GConfClient object",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_NAGCONF_GCONF, spec );

	spec = g_param_spec_pointer(
			PROP_NAGCONF_NOTIFIED_STR,
			"Object to be notified",
			"A pointer to a GObject which will receive action_changed notifications",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_NAGCONF_NOTIFIED, spec );

	spec = g_param_spec_uint(
			PROP_NAGCONF_WATCH_ID_STR,
			"Watch ID",
			"The unique ID got when installing the watch monitor on GConf", 0, 0, 0,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_NAGCONF_WATCH_ID, spec );

	klass->private = g_new0( NAGConfClassPrivate, 1 );
}

static void
iio_provider_iface_init( NAIIOProviderInterface *iface )
{
	static const gchar *thisfn = "na_gconf_iio_provider_iface_init";
	g_debug( "%s: iface=%p", thisfn, iface );

	iface->read_actions = iio_provider_read_actions;
	iface->is_willing_to_write = iio_provider_is_willing_to_write;
	iface->is_writable = iio_provider_is_writable;
	iface->write_action = iio_provider_write_action;
	iface->delete_action = iio_provider_delete_action;
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
	self->private->watch_id = install_gconf_watch( self );
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	g_assert( NA_IS_GCONF( object ));
	NAGConf *self = NA_GCONF( object );

	switch( property_id ){
		case PROP_NAGCONF_GCONF:
			g_value_set_pointer( value, self->private->gconf );
			break;

		case PROP_NAGCONF_NOTIFIED:
			g_value_set_pointer( value, self->private->notified );
			break;

		case PROP_NAGCONF_WATCH_ID:
			g_value_set_uint( value, self->private->watch_id );
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
		case PROP_NAGCONF_GCONF:
			self->private->gconf = g_value_get_pointer( value );
			break;

		case PROP_NAGCONF_NOTIFIED:
			self->private->notified = g_value_get_pointer( value );
			break;

		case PROP_NAGCONF_WATCH_ID:
			self->private->watch_id = g_value_get_uint( value );
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
	NAGConf *self = NA_GCONF( object );

	g_free( self->private );

	/* chain call to parent class */
	if( st_parent_class->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * na_gconf_new:
 * @handler: a #GObject-derived object which is to be notified when an
 * action is added, modified or removed in underlying GConf system.
 *
 * Allocates a new #NAGConf object.
 *
 * Is specified, the #GObject-derived object to be notified will
 * receive a "notify_pivot_of_action_changed" message for each detected
 * modification, with a pointer to a newly allocated #NAPivotNotify
 * structure describing the change.
 */
NAGConf *
na_gconf_new( const GObject *handler )
{
	return( g_object_new( NA_GCONF_TYPE, PROP_NAGCONF_NOTIFIED_STR, handler, NULL ));
}

/*
 * Note that whatever be the version of the readen action, it will be
 * stored as a #NAAction and its set of #NAActionProfile of the same,
 * latest, version of these classes.
 */
static GSList *
iio_provider_read_actions( const NAIIOProvider *provider )
{
	static const gchar *thisfn = "nacf_gconf_iio_provider_read_actions";
	g_debug( "%s: provider=%p", thisfn, provider );

	g_assert( NA_IS_IIO_PROVIDER( provider ));
	g_assert( NA_IS_GCONF( provider ));
	NAGConf *self = NA_GCONF( provider );

	GSList *items = NULL;
	GSList *ip;
	GSList *listpath = get_path_subdirs( self, NA_GCONF_CONFIG_PATH );

	for( ip = listpath ; ip ; ip = ip->next ){

		const gchar *path = ( const gchar * ) ip->data;

		NAAction *action = na_action_new();

		read_action( self, action, path );

		items = g_slist_prepend( items, action );

		/*gchar *uuid = path_to_key( path );

		NAAction *action = na_action_new( uuid );

		if( load_action( self, action, path )){
			items = g_slist_prepend( items, action );

		} else {
			g_object_unref( action );
		}

		g_free( uuid );*/
	}

	na_utils_free_string_list( listpath );

	return( items );
}

static gboolean
iio_provider_is_willing_to_write( const NAIIOProvider *provider )
{
	/* TODO: na_gconf_iio_provider_is_willing_to_write */
	return( TRUE );
}

static gboolean
iio_provider_is_writable( const NAIIOProvider *provider, const NAAction *action )
{
	/* TODO: na_gconf_iio_provider_is_writable */
	return( TRUE );
}

static guint
iio_provider_write_action( const NAIIOProvider *provider, NAAction *action, gchar **message )
{
	static const gchar *thisfn = "na_gconf_iio_provider_write_action";
	g_debug( "%s: provider=%p, action=%p, message=%p", thisfn, provider, action, message );

	g_assert( NA_IS_IIO_PROVIDER( provider ));
	g_assert( NA_IS_GCONF( provider ));
	NAGConf *self = NA_GCONF( provider );

	message = NULL;

	g_assert( NA_IS_ACTION( action ));

	if( !write_v2_keys( self, action, message )){
		return( NA_IIO_PROVIDER_WRITE_ERROR );

	} else {
		gconf_client_suggest_sync( self->private->gconf, NULL );
	}

	na_action_set_provider( action, provider );

	return( NA_IIO_PROVIDER_WRITE_OK );
}

/*
 * also delete the schema which may be directly attached to this action
 * cf. http://bugzilla.gnome.org/show_bug.cgi?id=325585
 */
static guint
iio_provider_delete_action( const NAIIOProvider *provider, const NAAction *action, gchar **message )
{
	static const gchar *thisfn = "na_gconf_iio_provider_delete_action";
	g_debug( "%s: provider=%p, action=%p, message=%p", thisfn, provider, action, message );

	g_assert( NA_IS_IIO_PROVIDER( provider ));
	g_assert( NA_IS_GCONF( provider ));
	NAGConf *self = NA_GCONF( provider );

	message = NULL;
	guint ret = NA_IIO_PROVIDER_WRITE_OK;

	g_assert( NA_IS_ACTION( action ));

	gchar *uuid = na_action_get_uuid( action );

	GError *error = NULL;
	gchar *path = g_strdup_printf( "%s%s/%s", NA_GCONF_SCHEMA_PREFIX, NA_GCONF_CONFIG_PATH, uuid );
	gconf_client_recursive_unset( self->private->gconf, path, 0, &error );
	if( error ){
		g_debug( "%s: error=%s for path=%s", thisfn, error->message, path );
		g_error_free( error );
		error = NULL;
	}
	g_free( path );


	path = g_strdup_printf( "%s/%s", NA_GCONF_CONFIG_PATH, uuid );
	if( !gconf_client_recursive_unset( self->private->gconf, path, 0, &error )){
		g_debug( "%s: error=%s", thisfn, error->message );
		*message = g_strdup( error->message );
		g_error_free( error );
		ret = NA_IIO_PROVIDER_WRITE_ERROR;

	} else {
		gconf_client_suggest_sync( self->private->gconf, NULL );
		g_debug( "%s: ok for %s", thisfn, path );
	}

	g_free( path );
	g_free( uuid );
	return( ret );
}

/*
 * load and set the properties of the specified action
 * at least we must have a label, as all other entries can have
 * suitable default values
 *
 * we have to deal with successive versions of action schema :
 *
 * - version = '1.0'
 *   action+= uuid+label+tooltip+icon
 *   action+= path+parameters+basenames+isdir+isfile+multiple+schemes
 *
 * - version > '1.0'
 *   action+= matchcase+mimetypes
 *
 * - version = '2.0' which introduces the 'profile' notion
 *   profile += name+label
 */
/*static gboolean
load_action( NAGConf *gconf, NAAction *action, const gchar *path )
{
	static const gchar *thisfn = "nacf_gconf_load_action";
	g_debug( "%s: gconf=%p, action=%p, uuid=%s", thisfn, gconf, action, uuid );

	g_assert( NA_IS_GCONF( gconf ));
	g_assert( NA_IS_ACTION( action ));

	gchar *path = g_strdup_printf( "%s/%s", NA_GCONF_CONFIG_PATH, uuid );

	GSList *entries = load_keys_values( gconf, path );
	GSList *properties = keys_to_notify( entries );
	free_keys_values( entries );

	gboolean ok = set_action_properties( gconf, action, properties );

	if( !key_is_writable( gconf, path )){
		g_object_set( G_OBJECT( action ), PROP_NAGCONF_ACTION_READONLY_STR, TRUE, NULL );
	}

	free_list_notify( properties );
	g_free( path );
	return( ok );
}*/
static void
read_action( NAGConf *gconf, NAAction *action, const gchar *path )
{
	static const gchar *thisfn = "na_gconf_read_action";
	g_debug( "%s: gconf=%p, action=%p, path=%s", thisfn, gconf, action, path );

	g_assert( NA_IS_GCONF( gconf ));
	g_assert( NA_IS_ACTION( action ));

	gchar *uuid = na_utils_path_to_key( path );
	na_action_set_uuid( action, uuid );
	g_free( uuid );

	GSList *entries = get_list_entries( gconf, path );
	GSList *notifies = entries_to_notifies( entries );
	free_list_entries( entries );

	fill_action_core_properties( gconf, action, notifies  );

	GSList *ip;
	GSList *list_profiles = get_path_subdirs( gconf, path );
	if( list_profiles ){
		for( ip = list_profiles ; ip ; ip = ip->next ){

			const gchar *profile_path = ( const gchar * ) ip->data;
			NAActionProfile *profile = na_action_profile_new();

			read_profile( gconf, profile, profile_path );

			na_action_attach_profile( action, profile );
		}

	} else {
		fill_action_v1_properties( gconf, action, notifies );
	}

	free_list_notifies( notifies );

	na_action_set_readonly( action, !key_is_writable( gconf, path ));
}

static void
read_profile( NAGConf *gconf, NAActionProfile *profile, const gchar *path )
{
	static const gchar *thisfn = "na_gconf_read_profile";
	g_debug( "%s: gconf=%p, profile=%p, path=%s", thisfn, gconf, profile, path );

	g_assert( NA_IS_GCONF( gconf ));
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	gchar *name = na_utils_path_to_key( path );
	na_action_profile_set_name( profile, name );
	g_free( name );

	GSList *entries = get_list_entries( gconf, path );
	GSList *notifies = entries_to_notifies( entries );
	free_list_entries( entries );

	fill_profile_properties( gconf, profile, notifies  );

	free_list_notifies( notifies );
}

/*
 * set the item properties into the action, dealing with successive
 * versions
 */
static void
fill_action_core_properties( NAGConf *gconf, NAAction *action, GSList *notifies )
{
	static const gchar *thisfn = "na_gconf_fill_action_properties";

	gchar *label;
	if( !search_for_str( notifies, NULL, ACTION_LABEL_ENTRY, &label )){
		gchar *uuid = na_action_get_uuid( action );
		g_warning( "%s: no label found for action '%s'", thisfn, uuid );
		g_free( uuid );
		label = g_strdup( "" );
	}
	na_action_set_label( action, label );
	g_free( label );

	gchar *version;
	if( search_for_str( notifies, NULL, ACTION_VERSION_ENTRY, &version )){
		na_action_set_version( action, version );
		g_free( version );
	}

	gchar *tooltip;
	if( search_for_str( notifies, NULL, ACTION_TOOLTIP_ENTRY, &tooltip )){
		na_action_set_tooltip( action, tooltip );
		g_free( tooltip );
	}

	gchar *icon;
	if( search_for_str( notifies, NULL, ACTION_ICON_ENTRY, &icon )){
		na_action_set_icon( action, icon );
		g_free( icon );
	}
}

/*
 * version is marked as less than "2.0"
 * we handle so only one profile, which is already loaded
 * action+= path+parameters+basenames+isdir+isfile+multiple+schemes
 * if version greater than "1.0", we have also matchcase+mimetypes
 */
static void
fill_action_v1_properties( NAGConf *gconf, NAAction *action, GSList *notifies )
{
	NAActionProfile *profile = na_action_profile_new();

	na_action_attach_profile( action, profile );

	fill_profile_properties( gconf, profile, notifies );
}

static void
fill_profile_properties( NAGConf *gconf, NAActionProfile *profile, GSList *notifies )
{
	gchar *label;
	if( !search_for_str( notifies, NULL, ACTION_PROFILE_LABEL_ENTRY, &label )){
		/* i18n: default profile label */
		label = g_strdup( _( "Default profile" ));
	}
	na_action_profile_set_label( profile, label );
	g_free( label );

	gchar *path;
	if( search_for_str( notifies, NULL, ACTION_PATH_ENTRY, &path )){
		na_action_profile_set_path( profile, path );
		g_free( path );
	}

	gchar *parameters;
	if( search_for_str( notifies, NULL, ACTION_PARAMETERS_ENTRY, &parameters )){
		na_action_profile_set_parameters( profile, parameters );
		g_free( parameters );
	}

	GSList *basenames;
	if( search_for_list( notifies, NULL, ACTION_BASENAMES_ENTRY, &basenames )){
		na_action_profile_set_basenames( profile, basenames );
		na_utils_free_string_list( basenames );
	}

	gboolean isfile;
	if( search_for_bool( notifies, NULL, ACTION_ISFILE_ENTRY, &isfile )){
		na_action_profile_set_isfile( profile, isfile );
	}

	gboolean isdir;
	if( search_for_bool( notifies, NULL, ACTION_ISDIR_ENTRY, &isdir )){
		na_action_profile_set_isdir( profile, isdir );
	}

	gboolean multiple;
	if( search_for_bool( notifies, NULL, ACTION_MULTIPLE_ENTRY, &multiple )){
		na_action_profile_set_multiple( profile, multiple );
	}

	GSList *schemes;
	if( search_for_list( notifies, NULL, ACTION_SCHEMES_ENTRY, &schemes )){
		na_action_profile_set_schemes( profile, schemes );
		na_utils_free_string_list( schemes );
	}

	/* handle matchcase+mimetypes
	 * note that default values for 1.0 version have been set
	 * in na_action_profile_instance_init
	 */
	gboolean matchcase;
	if( search_for_bool( notifies, NULL, ACTION_MATCHCASE_ENTRY, &matchcase )){
		na_action_profile_set_matchcase( profile, matchcase );
	}

	GSList *mimetypes;
	if( search_for_list( notifies, NULL, ACTION_MIMETYPES_ENTRY, &mimetypes )){
		na_action_profile_set_mimetypes( profile, mimetypes );
		na_utils_free_string_list( mimetypes );
	}
}

/*
 * load the keys which are the subdirs of the given path
 * returns a list of keys as full path
 */
/*static GSList *
load_subdirs( const NAGConf *gconf, const gchar *path )
{
	static const gchar *thisfn = "na_gconf_load_subdirs";

	GError *error = NULL;
	GSList *list = gconf_client_all_dirs( gconf->private->gconf, path, &error );
	if( error ){
		g_warning( "%s: path=%s, error=%s", thisfn, path, error->message );
		g_error_free( error );
		return(( GSList * ) NULL );
	}

	return( list );
}*/
static GSList *
get_path_subdirs( const NAGConf *gconf, const gchar *path )
{
	static const gchar *thisfn = "na_gconf_get_path_subdirs";

	GError *error = NULL;
	GSList *list = gconf_client_all_dirs( gconf->private->gconf, path, &error );
	if( error ){
		g_warning( "%s: path=%s, error=%s", thisfn, path, error->message );
		g_error_free( error );
		return(( GSList * ) NULL );
	}

	return( list );
}

/*
 * load all the key=value pairs of this key (specified as a full path),
 * returning them as a list of GConfEntry.
 *
 * The list is not recursive, it contains only the immediate children of
 * path. To free the returned list, call free_list_entries().
 */
static GSList *
get_list_entries( const NAGConf *gconf, const gchar *path )
{
	static const gchar *thisfn = "na_gconf_get_list_values";

	GError *error = NULL;
	GSList *list_path = gconf_client_all_entries( gconf->private->gconf, path, &error );
	if( error ){
		g_warning( "%s: path=%s, error=%s", thisfn, path, error->message );
		g_error_free( error );
		return(( GSList * ) NULL );
	}

	return( list_path );
}

static void
free_list_entries( GSList *list )
{
	GSList *item;
	for( item = list ; item != NULL ; item = item->next ){
		GConfEntry *entry = ( GConfEntry * ) item->data;
		gconf_entry_unref( entry );
	}
	g_slist_free( list );
}

/*
 * convert a GConfEntry to a structure suitable to notify NAPivot
 *
 * when created or modified, the entry can be of the forms :
 *  key=path/uuid/parm
 *  key=path/uuid/profile/parm with a not null value
 *
 * but when removing an entry, it will be of the form :
 *  key=path/uuid
 *  key=path/uuid/parm
 *  key=path/uuid/profile
 *  key=path/uuid/profile/parm with a null value
 *
 * I don't know any way to choose between key/parm and key/profile (*)
 * as the entry no more exists in GConf and thus cannot be tested
 * -> we will set this as key/parm, letting pivot try to interpret it
 *
 * (*) other than assuming that a profile name begins with 'profile-'
 * (see action-profile.h)
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
 * convert a list of GConfEntry to a list of NAPivotNotify.
 */
static GSList *
entries_to_notifies( GSList *entries )
{
	GSList *item;
	GSList *notifies = NULL;

	for( item = entries ; item ; item = item->next ){

		const GConfEntry *entry = ( const GConfEntry * ) item->data;
		NAPivotNotify *npn = entry_to_notify( entry );
		notifies = g_slist_prepend( notifies, npn );
	}

	return( notifies );
}

static void
free_list_notifies( GSList *list )
{
	GSList *il;
	for( il = list ; il ; il = il->next ){
		na_pivot_free_notify(( NAPivotNotify *) il->data );
	}
}

static gboolean
search_for_str( GSList *properties, const gchar *profile, const gchar *key, gchar **value )
{
	*value = NULL;
	GSList *ip;
	for( ip = properties ; ip ; ip = ip->next ){
		NAPivotNotify *npn = ( NAPivotNotify * ) ip->data;
		if( npn->type == NA_PIVOT_STR &&
		  ( !profile || !g_ascii_strcasecmp( profile, npn->profile )) &&
		    !g_ascii_strcasecmp( key, npn->parm )){
				*value = g_strdup(( gchar * ) npn->data );
				return( TRUE );
		}
	}
	return( FALSE );
}

static gboolean
search_for_bool( GSList *properties, const gchar *profile, const gchar *key, gboolean *value )
{
	*value = FALSE;
	GSList *ip;
	for( ip = properties ; ip ; ip = ip->next ){
		NAPivotNotify *npn = ( NAPivotNotify * ) ip->data;
		if( npn->type == NA_PIVOT_BOOL &&
		  ( !profile || !g_ascii_strcasecmp( profile, npn->profile )) &&
			!g_ascii_strcasecmp( key, npn->parm )){
				*value = ( gboolean ) npn->data;
				return( TRUE );
		}
	}
	return( FALSE );
}

static gboolean
search_for_list( GSList *properties, const gchar *profile, const gchar *key, GSList **value )
{
	*value = NULL;
	GSList *ip;
	for( ip = properties ; ip ; ip = ip->next ){
		NAPivotNotify *npn = ( NAPivotNotify * ) ip->data;
		if( npn->type == NA_PIVOT_STRLIST &&
		  ( !profile || !g_ascii_strcasecmp( profile, npn->profile )) &&
			!g_ascii_strcasecmp( key, npn->parm )){
				*value = na_utils_duplicate_string_list(( GSList * ) npn->data );
				return( TRUE );
		}
	}
	return( FALSE );
}

/*
 * load and set the properties of the specified action
 */
/*static void
load_action_properties( NAGConf *gconf, NAAction *action )
{
	static const gchar *thisfn = "nacf_gconf_load_action_properties";
	g_debug( "%s: gconf=%p, action=%p", thisfn, gconf, action );

	g_assert( NA_IS_GCONF( gconf ));
	g_assert( NA_IS_ACTION( action ));

	gchar *uuid = na_action_get_uuid( action );
	gchar *path = g_strdup_printf( "%s/%s", NA_GCONF_CONFIG_PATH, uuid );

	GSList *properties = load_keys_values( gconf, path );

	set_item_properties( NA_OBJECT( action ), properties );

	free_keys_values( properties );
	g_free( uuid );
	g_free( path );
}*/

/*
 * load the list of profiles for an action and returns them as a GSList
 */
/*static GSList *
load_profiles( NAGConf *gconf, NAAction *action )
{*/
	/*static const gchar *thisfn = "nacf_gconf_load_profiles";
	g_debug( "%s: gconf=%p, action=%p", thisfn, gconf, action );*/

	/*g_assert( NA_IS_GCONF( gconf ));
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
}*/

/*
 * load and set the properties of the specified profile
 */
/*static void
load_profile_properties( NAGConf *gconf, NAActionProfile *profile )
{
	*//*static const gchar *thisfn = "nacf_gconf_load_profile_properties";
	g_debug( "%s: gconf=%p, profile=%p", thisfn, gconf, profile );*/

/*	g_assert( NA_IS_GCONF( gconf ));
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
}*/

/*
 * load all the key=value pairs of this key (specified as a full path),
 * returning them as a list of GConfEntry.
 * The list is not recursive, it contains only the immediate children of
 * path. To free the returned list, call free_key_values.
 */
/*static GSList *
load_keys_values( const NAGConf *gconf, const gchar *path )
{
	static const gchar *thisfn = "na_gconf_load_keys_values";

	GError *error = NULL;
	GSList *list_path = gconf_client_all_entries( gconf->private->gconf, path, &error );
	if( error ){
		g_warning( "%s: path=%s, error=%s", thisfn, path, error->message );
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
}*/

/*
 * set the item properties into the object
 * properties is a list of path to entry
 */
/*static void
set_item_properties( NAObject *object, GSList *properties )
{
	static const gchar *thisfn = "na_gconf_set_item_properties";
	g_assert( NA_IS_OBJECT( object ));

	GSList *item;
	for( item = properties ; item != NULL ; item = item->next ){

		GConfEntry *entry = ( GConfEntry * ) item->data;
		NAPivotNotify *npn = entry_to_notify( entry );
		if( npn->type ){
			const gchar *property_str = key_to_property( npn->parm );

			if( property_str ){
				switch( npn->type ){
					case NA_PIVOT_STR:
					case NA_PIVOT_BOOL:
					case NA_PIVOT_STRLIST:
						g_object_set( G_OBJECT( object ), property_str, npn->data, NULL );
						break;

					default:
						g_debug( "%s: uuid='%s', profile='%s', parm='%s', type=%d, data=%p",
								thisfn, npn->uuid, npn->profile, npn->parm, npn->type, npn->data );
						g_assert_not_reached();
						break;
				}
			} else {
				g_warning( "%s: no property found for %s GConf key", thisfn, npn->parm );
			}
		}

		na_pivot_free_notify( npn );
	}
}*/

/*static const gchar *
key_to_property( const gchar *key )
{
	int i;
	gchar *property = NULL;

	for( i=0 ; st_key_property[i].gconfkey && !property ; ++i ){
		if( !g_ascii_strcasecmp( st_key_property[i].gconfkey, key )){
			property = st_key_property[i].property;
			break;
		}
	}

	return( property );
}*/

/*
 * set the item properties into the action, dealing with successive
 * versions
 */
/*static gboolean
set_action_properties( NAGConf *gconf, NAAction *action, GSList *properties )
{
	*//*static const gchar *thisfn = "na_gconf_set_action_properties";*//*

	gchar *label = search_for_str( properties, NULL, ACTION_LABEL_ENTRY );
	if( !label ){
		return( FALSE );
	}

	gchar *version = search_for_str( properties, NULL, ACTION_VERSION_ENTRY );
	gchar *tooltip = search_for_str( properties, NULL, ACTION_TOOLTIP_ENTRY );
	gchar *icon = search_for_str( properties, NULL, ACTION_ICON_ENTRY );

	g_object_set( G_OBJECT( action ),
			PROP_NAACTION_VERSION_STR, version,
			PROP_NAACTION_LABEL_STR, label,
			PROP_NAACTION_TOOLTIP_STR, tooltip,
			PROP_NAACTION_ICON_STR, icon,
			PROP_NAACTION_PROVIDER_STR, gconf,
			NULL );

	g_free( icon );
	g_free( tooltip );
	g_free( label );

	if( g_ascii_strcasecmp( version, "2.0" ) < 0 ){

		load_v1_properties( action, version, properties );

		gchar *uuid = na_action_get_uuid( action );
		gchar *path = g_strdup_printf( "%s/%s", NA_GCONF_CONFIG_PATH, uuid );
		if( gconf_client_key_is_writable( gconf->private->gconf, path, NULL )){
			remove_v1_keys( gconf, action, NULL );
		}
		g_free( path );
		g_free( uuid );

	} else {

		GSList *profiles = load_profiles( gconf, action );
		na_action_set_profiles( action, profiles );
		na_action_free_profiles( profiles );
	}

	g_free( version );
	return( TRUE );
}*/

/*
 * only handle one profile, which is already loaded
 * action+= path+parameters+basenames+isdir+isfile+multiple+schemes
 */
/*static void
load_v1_properties( NAAction *action, const gchar *version, GSList *properties )
{
	GSList *profiles = NULL;
	NAActionProfile *profile = na_action_profile_new( NA_OBJECT( action ), "default" );

	gchar *path = search_for_str( properties, NULL, ACTION_PATH_ENTRY );
	gchar *parameters = search_for_str( properties, NULL, ACTION_PARAMETERS_ENTRY );
	GSList *basenames = search_for_list( properties, NULL, ACTION_BASENAMES_ENTRY );
	gboolean isdir = search_for_bool( properties, NULL, ACTION_ISDIR_ENTRY );
	gboolean isfile = search_for_bool( properties, NULL, ACTION_ISFILE_ENTRY );
	gboolean multiple = search_for_bool( properties, NULL, ACTION_MULTIPLE_ENTRY );
	GSList *schemes = search_for_list( properties, NULL, ACTION_SCHEMES_ENTRY );

	g_object_set( G_OBJECT( profile ),
			PROP_NAPROFILE_PATH_STR, path,
			PROP_NAPROFILE_PARAMETERS_STR, parameters,
			PROP_NAPROFILE_BASENAMES_STR, basenames,
			PROP_NAPROFILE_ISDIR_STR, isdir,
			PROP_NAPROFILE_ISFILE_STR, isfile,
			PROP_NAPROFILE_ACCEPT_MULTIPLE_STR, multiple,
			PROP_NAPROFILE_SCHEMES_STR, schemes,
			NULL );

	g_free( path );
	g_free( parameters );
	na_utils_free_string_list( basenames );
	na_utils_free_string_list( schemes );

	if( g_ascii_strcasecmp( version, "1.0" ) > 0 ){
*/
		/* handle matchcase+mimetypes
		 * note that default values for 1.0 version have been set
		 * in na_action_profile_instance_init
		 */
		/*gboolean matchcase = search_for_bool( properties, "", ACTION_MATCHCASE_ENTRY );
		GSList *mimetypes = search_for_list( properties, "", ACTION_MIMETYPES_ENTRY );

		g_object_set( G_OBJECT( profile ),
				PROP_NAGCONF_PROFILE_MATCHCASE_STR, matchcase,
				PROP_NAGCONF_PROFILE_MIMETYPES_STR, mimetypes,
				NULL );

		na_utils_free_string_list( mimetypes );
	}

	profiles = g_slist_prepend( profiles, profile );
	na_action_set_profiles( action, profiles );
	na_action_free_profiles( profiles );
}*/

/*static gchar *
search_for_str( GSList *properties, const gchar *profile, const gchar *key )
{
	GSList *ip;
	for( ip = properties ; ip ; ip = ip->next ){
		NAPivotNotify *npn = ( NAPivotNotify * ) ip->data;
		if( npn->type == NA_PIVOT_STR &&
		  ( !profile || !g_ascii_strcasecmp( profile, npn->profile )) &&
		    !g_ascii_strcasecmp( key, npn->parm )){
				return( g_strdup(( gchar * ) npn->data ));
		}
	}
	return(( gchar * ) NULL );
}

static gboolean
search_for_bool( GSList *properties, const gchar *profile, const gchar *key )
{
	GSList *ip;
	for( ip = properties ; ip ; ip = ip->next ){
		NAPivotNotify *npn = ( NAPivotNotify * ) ip->data;
		if( npn->type == NA_PIVOT_BOOL &&
		  ( !profile || !g_ascii_strcasecmp( profile, npn->profile )) &&
			!g_ascii_strcasecmp( key, npn->parm )){
				return(( gboolean ) npn->data );
		}
	}
	return( FALSE );
}

static GSList *
search_for_list( GSList *properties, const gchar *profile, const gchar *key )
{
	GSList *ip;
	for( ip = properties ; ip ; ip = ip->next ){
		NAPivotNotify *npn = ( NAPivotNotify * ) ip->data;
		if( npn->type == NA_PIVOT_STRLIST &&
		  ( !profile || !g_ascii_strcasecmp( profile, npn->profile )) &&
			!g_ascii_strcasecmp( key, npn->parm )){
				return( na_utils_duplicate_string_list(( GSList * ) npn->data ));
		}
	}
	return(( GSList * ) NULL );
}*/

/*
 * convet a list of GConfEntry to a list of NAPivotNotify.
 */
/*static GSList *
keys_to_notify( GSList *entries )
{
	GSList *item;
	GSList *properties = NULL;

	for( item = entries ; item ; item = item->next ){
		GConfEntry *entry = ( GConfEntry * ) item->data;
		NAPivotNotify *npn = entry_to_notify( entry );
		properties = g_slist_prepend( properties, npn );
	}

	return( properties );
}*/

/*
 * convert a GConfEntry to a structure suitable to notify NAPivot
 *
 * when created or modified, the entry can be of the forms :
 *  key/parm
 *  key/profile/parm with a not null value
 *
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
/*static NAPivotNotify *
entry_to_notify( const GConfEntry *entry )
{
	*//*static const gchar *thisfn = "na_gconf_entry_to_notify";*//*
	GSList *listvalues, *iv, *strings;

	g_assert( entry );

	const gchar *path = gconf_entry_get_key( entry );
	g_assert( path );

	NAPivotNotify *npn = g_new0( NAPivotNotify, 1 );

	const gchar *subpath = path + strlen( NA_GCONF_CONFIG_PATH ) + 1;
	gchar **split = g_strsplit( subpath, "/", -1 );
	*//*g_debug( "%s: [0]=%s, [1]=%s", thisfn, split[0], split[1] );*//*
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
				*//*na_utils_free_string_list( strings );*//*
				break;

			default:
				g_assert_not_reached();
				break;
		}
	}
	return( npn );
}

static void
free_list_notify( GSList *list )
{
	GSList *il;
	for( il = list ; il ; il = il->next ){
		na_pivot_free_notify(( NAPivotNotify *) il->data );
	}
}*/

/*static gboolean
remove_v1_keys( NAGConf *gconf, const NAAction *action, gchar **message )
{
	gchar *uuid = na_action_get_uuid( action );

	gboolean ret =
		remove_key( gconf, uuid, ACTION_PATH_ENTRY, message ) &&
		remove_key( gconf, uuid, ACTION_PARAMETERS_ENTRY, message ) &&
		remove_key( gconf, uuid, ACTION_BASENAMES_ENTRY, message ) &&
		remove_key( gconf, uuid, ACTION_MATCHCASE_ENTRY, message ) &&
		remove_key( gconf, uuid, ACTION_MIMETYPES_ENTRY, message ) &&
		remove_key( gconf, uuid, ACTION_ISFILE_ENTRY, message ) &&
		remove_key( gconf, uuid, ACTION_ISDIR_ENTRY, message ) &&
		remove_key( gconf, uuid, ACTION_MULTIPLE_ENTRY, message ) &&
		remove_key( gconf, uuid, ACTION_SCHEMES_ENTRY, message );

	g_free( uuid );
	return( ret );
}*/

/*static gboolean
remove_key( NAGConf *gconf, const gchar *uuid, const gchar *key, gchar **message )
{
	gboolean ret = TRUE;
	GError *error = NULL;

	gchar *path = g_strdup_printf( "%s/%s/%s", NA_GCONF_CONFIG_PATH, uuid, key );

	if( !gconf_client_unset( gconf->private->gconf, path, &error )){
		if( message ){
			*message = g_strdup( error->message );
		}
		g_error_free( error );
		ret = FALSE;
	}

	g_free( path );
	return( ret );
}*/

/*
 * gconf_client_key_is_writable doesn't work as I expect: it returns
 * FALSE without error for our keys !
 * So I have to actually try a fake write to the key to get the real
 * writability status...
 *
 * TODO: having a NAPivot as Nautilus extension and another NAPivot in
 * the UI leads to a sort of notification loop: extension's pivot is
 * not aware of UI's one, and so get notifications from GConf, reloads
 * the list of actions, retrying to test the writability.
 * In the meanwhile, UI's pivot has reauthorized notifications, and is
 * notified of extension's pivot tests, and so on......
 *
 * -> Nautilus extension's pivot should not test for writability, as it
 *    uses actions as read-only, reloading the whole list when one
 *    action is modified ; this can break the loop
 *
 * -> the UI may use the pivot inside of Nautilus extension via a sort
 *    of API, falling back to its own pivot, when the extension is not
 *    present.
 */
static gboolean
key_is_writable( NAGConf *gconf, const gchar *path )
{
	/*static const gchar *thisfn = "na_gconf_key_is_writable";
	GError *error = NULL;

	remove_gconf_watched_dir( gconf );

	gboolean ret_gconf = gconf_client_key_is_writable( gconf->private->gconf, path, &error );
	if( error ){
		g_warning( "%s: gconf_client_key_is_writable: %s", thisfn, error->message );
		g_error_free( error );
		error = NULL;
	}
	gboolean ret_try = FALSE;
	gchar *path_try = g_strdup_printf( "%s/%s", path, "fake_key" );
	ret_try = gconf_client_set_string( gconf->private->gconf, path_try, "fake_value", &error );
	if( error ){
		g_warning( "%s: gconf_client_set_string: %s", thisfn, error->message );
		g_error_free( error );
		error = NULL;
	}
	if( ret_try ){
		gconf_client_unset( gconf->private->gconf, path_try, NULL );
	}
	g_free( path_try );
	g_debug( "%s: ret_gconf=%s, ret_try=%s", thisfn, ret_gconf ? "True":"False", ret_try ? "True":"False" );

	install_gconf_watched_dir( gconf );
	return( ret_try );*/

	return( TRUE );
}

static gboolean
write_v2_keys( NAGConf *gconf, const NAAction *action, gchar **message )
{
	gchar *uuid = na_action_get_uuid( action );

	gboolean ret =
		write_str( gconf, uuid, ACTION_VERSION_ENTRY, na_action_get_version( action ), message ) &&
		write_str( gconf, uuid, ACTION_LABEL_ENTRY, na_action_get_label( action ), message ) &&
		write_str( gconf, uuid, ACTION_TOOLTIP_ENTRY, na_action_get_tooltip( action ), message ) &&
		write_str( gconf, uuid, ACTION_ICON_ENTRY, na_action_get_icon( action ), message );

	GSList *ip;
	GSList *profiles = na_action_get_profiles( action );

	for( ip = profiles ; ip && ret ; ip = ip->next ){

		NAActionProfile *profile = NA_ACTION_PROFILE( ip->data );
		gchar *name = na_action_profile_get_name( profile );

		ret =
			write_str2( gconf, uuid, name, ACTION_PROFILE_LABEL_ENTRY, na_action_profile_get_label( profile ), message ) &&
			write_str2( gconf, uuid, name, ACTION_PATH_ENTRY, na_action_profile_get_path( profile ), message ) &&
			write_str2( gconf, uuid, name, ACTION_PARAMETERS_ENTRY, na_action_profile_get_parameters( profile ), message ) &&
			write_list( gconf, uuid, name, ACTION_BASENAMES_ENTRY, na_action_profile_get_basenames( profile ), message ) &&
			write_bool( gconf, uuid, name, ACTION_MATCHCASE_ENTRY, na_action_profile_get_matchcase( profile ), message ) &&
			write_list( gconf, uuid, name, ACTION_MIMETYPES_ENTRY, na_action_profile_get_mimetypes( profile ), message ) &&
			write_bool( gconf, uuid, name, ACTION_ISFILE_ENTRY, na_action_profile_get_is_file( profile ), message ) &&
			write_bool( gconf, uuid, name, ACTION_ISDIR_ENTRY, na_action_profile_get_is_dir( profile ), message ) &&
			write_bool( gconf, uuid, name, ACTION_MULTIPLE_ENTRY, na_action_profile_get_multiple( profile ), message ) &&
			write_list( gconf, uuid, name, ACTION_SCHEMES_ENTRY, na_action_profile_get_schemes( profile ), message );

		g_free( name );
	}

	g_free( uuid );
	return( ret );
}

static gboolean
write_str( NAGConf *gconf, const gchar *uuid, const gchar *key, gchar *value, gchar **message )
{
	gchar *path = g_strdup_printf( "%s/%s/%s", NA_GCONF_CONFIG_PATH, uuid, key );
	gboolean ret = write_key( gconf, path, value, message );
	g_free( value );
	g_free( path );
	return( ret );
}

static gboolean
write_str2( NAGConf *gconf, const gchar *uuid, const gchar *name, const gchar *key, gchar *value, gchar **message )
{
	gchar *path = g_strdup_printf( "%s/%s/%s/%s", NA_GCONF_CONFIG_PATH, uuid, name, key );
	gboolean ret = write_key( gconf, path, value, message );
	g_free( value );
	g_free( path );
	return( ret );
}

static gboolean
write_key( NAGConf *gconf, const gchar *path, const gchar *value, gchar **message )
{
	gboolean ret = TRUE;
	GError *error = NULL;

	if( !gconf_client_set_string( gconf->private->gconf, path, value, &error )){
		*message = g_strdup( error->message );
		g_error_free( error );
		ret = FALSE;
	}

	return( ret );
}

static gboolean
write_bool( NAGConf *gconf, const gchar *uuid, const gchar *name, const gchar *key, gboolean value, gchar **message )
{
	gboolean ret = TRUE;
	GError *error = NULL;

	gchar *path = g_strdup_printf( "%s/%s/%s/%s", NA_GCONF_CONFIG_PATH, uuid, name, key );

	if( !gconf_client_set_bool( gconf->private->gconf, path, value, &error )){
		*message = g_strdup( error->message );
		g_error_free( error );
		ret = FALSE;
	}

	g_free( path );
	return( ret );
}

static gboolean
write_list( NAGConf *gconf, const gchar *uuid, const gchar *name, const gchar *key, GSList *value, gchar **message )
{
	gboolean ret = TRUE;
	GError *error = NULL;

	gchar *path = g_strdup_printf( "%s/%s/%s/%s", NA_GCONF_CONFIG_PATH, uuid, name, key );

	if( !gconf_client_set_list( gconf->private->gconf, path, GCONF_VALUE_STRING, value, &error )){
		*message = g_strdup( error->message );
		g_error_free( error );
		ret = FALSE;
	}

	na_utils_free_string_list( value );
	g_free( path );
	return( ret );
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

	install_gconf_watched_dir( gconf );

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
		g_warning( "%s: error=%s", thisfn, error->message );
		g_error_free( error );
		return( 0 );
	}

	return( notify_id );
}

static void
install_gconf_watched_dir( NAGConf *gconf )
{
	static const gchar *thisfn = "na_gconf_install_gconf_watched_dir";
	GError *error = NULL;

	gconf_client_add_dir(
			gconf->private->gconf, NA_GCONF_CONFIG_PATH, GCONF_CLIENT_PRELOAD_RECURSIVE, &error );
	if( error ){
		g_warning( "%s: error=%s", thisfn, error->message );
		g_error_free( error );
	}
}

static void
remove_gconf_watch( NAGConf *gconf )
{
	if( gconf->private->watch_id ){
		gconf_client_notify_remove( gconf->private->gconf, gconf->private->watch_id );
	}

	remove_gconf_watched_dir( gconf );
}

static void
remove_gconf_watched_dir( NAGConf *gconf )
{
	static const gchar *thisfn = "na_gconf_remove_gconf_watched_dir";
	GError *error = NULL;

	gconf_client_remove_dir( gconf->private->gconf, NA_GCONF_CONFIG_PATH, &error );
	if( error ){
		g_warning( "%s: error=%s", thisfn, error->message );
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
 * case, the ui takes care (as of of 1.10) of also writing at last a
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
	/*static const gchar *thisfn = "action_changed_cb";*/
	/*g_debug( "%s: client=%p, cnxnid=%u, entry=%p, user_data=%p", thisfn, client, cnxn_id, entry, user_data );*/

	g_assert( NA_IS_GCONF( user_data ));
	NAGConf *gconf = NA_GCONF( user_data );
	g_assert( NA_IS_IIO_PROVIDER( gconf ));

	NAPivotNotify *npn = entry_to_notify( entry );
	g_signal_emit_by_name( gconf->private->notified, NA_IIO_PROVIDER_SIGNAL_ACTION_CHANGED, npn );
}
