/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011 Pierre Wieser and others (see AUTHORS)
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
#include <string.h>

#include <api/na-data-types.h>
#include <api/na-core-utils.h>

#include "na-settings.h"

/* private class data
 */
struct _NASettingsClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _NASettingsPrivate {
	gboolean  dispose_has_run;

	/* the global configuration file, which handles mandatory preferences
	 */
	GKeyFile     *global_conf;
	GFileMonitor *global_monitor;
	gulong        global_handler;

	/* the user configuration file
	 */
	GKeyFile     *user_conf;
	GFileMonitor *user_monitor;
	gulong        user_handler;

	/* the registered consumers of monitoring events
	 * as a list of 'Consumer' structs
	 */
	GList        *consumers;

	/* for each monitoring key, we keep here the last known value
	 * so that we are able to detect changes when the configuration
	 * files is 'globally' changed
	 * as a list of 'Entry' structs
	 */
	GList        *entries;
};

#define GROUP_NACT						"nact"
#define GROUP_RUNTIME					"runtime"
#define GROUP_IO_PROVIDER				"io-provider"

#define IO_PROVIDER_READABLE			"readable"

typedef struct {
	const gchar *key;
	const gchar *group;
	guint        data_type;				/* picked up from NADataFactory */
	const gchar *default_value;
}
	KeyDef;

static const KeyDef st_def_keys[] = {
	{ NA_SETTINGS_RUNTIME_IO_PROVIDERS_READ_ORDER, GROUP_RUNTIME, NAFD_TYPE_STRING_LIST, "" },
	{ NA_SETTINGS_RUNTIME_ITEMS_ADD_ABOUT_ITEM,    GROUP_RUNTIME, NAFD_TYPE_BOOLEAN,     "true" },
	{ NA_SETTINGS_RUNTIME_ITEMS_CREATE_ROOT_MENU,  GROUP_RUNTIME, NAFD_TYPE_BOOLEAN,     "true" },
	{ NA_SETTINGS_RUNTIME_ITEMS_LEVEL_ZERO_ORDER,  GROUP_RUNTIME, NAFD_TYPE_STRING_LIST, "" },
	{ NA_SETTINGS_RUNTIME_ITEMS_LIST_ORDER_MODE,   GROUP_RUNTIME, NAFD_TYPE_MAP,         "AscendingOrder" },
	{ 0 }
};

typedef void ( *global_fn )( gboolean global, void *user_data );

typedef struct {
	gchar    *key;
	GCallback callback;
	gpointer  user_data;
}
	Consumer;

typedef struct {
	gchar   *group;
	gchar   *key;
	void    *value;
	gboolean global;
	guint    type;
}
	Entry;

static GObjectClass *st_parent_class = NULL;

static GType     register_type( void );
static void      class_init( NASettingsClass *klass );
static void      instance_init( GTypeInstance *instance, gpointer klass );
static void      instance_dispose( GObject *object );
static void      instance_finalize( GObject *object );

static KeyDef   *get_key_def( const gchar *key );
static gchar    *get_string_ex( NASettings *settings, const gchar *group, const gchar *key, const gchar *default_value, gboolean *found, gboolean *global );
static GSList   *get_string_list_ex( NASettings *settings, const gchar *group, const gchar *key, const gchar *default_value, gboolean *found, gboolean *global );
static GKeyFile *initialize_settings( NASettings* settings, const gchar *dir, GFileMonitor **monitor, gulong *handler );
static void      monitor_io_provider_read_status( NASettings *settings );
static void      monitor_io_provider_read_status_conf( NASettings *settings, GKeyFile *key_file, gboolean global );
static void      monitor_key( NASettings *settings, const gchar *key );
static void      monitor_key_add( NASettings *settings, const gchar *group, const gchar *key, gpointer value, gboolean global, guint type );
static void      on_conf_changed( GFileMonitor *monitor, GFile *file, GFile *other_file, GFileMonitorEvent event_type, NASettings *settings );
static void      release_consumer( Consumer *consumer );
static void      release_entry( Entry *entry );

GType
na_settings_get_type( void )
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
	static const gchar *thisfn = "na_settings_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NASettingsClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NASettings ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "NASettings", &info, 0 );

	return( type );
}

static void
class_init( NASettingsClass *klass )
{
	static const gchar *thisfn = "na_settings_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NASettingsClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "na_settings_instance_init";
	NASettings *self;

	g_return_if_fail( NA_IS_SETTINGS( instance ));

	self = NA_SETTINGS( instance );

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self->private = g_new0( NASettingsPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->global_conf = NULL;
	self->private->global_monitor = NULL;
	self->private->global_handler = 0;
	self->private->user_conf = NULL;
	self->private->user_monitor = NULL;
	self->private->user_handler = 0;
	self->private->consumers = NULL;
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "na_settings_instance_dispose";
	NASettings *self;

	g_return_if_fail( NA_IS_SETTINGS( object ));

	self = NA_SETTINGS( object );

	if( !self->private->dispose_has_run ){

		g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

		self->private->dispose_has_run = TRUE;

		g_key_file_free( self->private->global_conf );
		if( self->private->global_monitor ){
			if( self->private->global_handler ){
				g_signal_handler_disconnect( self->private->global_monitor, self->private->global_handler );
			}
			g_file_monitor_cancel( self->private->global_monitor );
			g_object_unref( self->private->global_monitor );
		}

		g_key_file_free( self->private->user_conf );
		if( self->private->user_monitor ){
			if( self->private->user_handler ){
				g_signal_handler_disconnect( self->private->user_monitor, self->private->user_handler );
			}
			g_file_monitor_cancel( self->private->user_monitor );
			g_object_unref( self->private->user_monitor );
		}

		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "na_settings_instance_finalize";
	NASettings *self;

	g_return_if_fail( NA_IS_SETTINGS( object ));

	self = NA_SETTINGS( object );

	g_debug( "%s: object=%p", thisfn, ( void * ) object );

	g_list_foreach( self->private->consumers, ( GFunc ) release_consumer, NULL );
	g_list_free( self->private->consumers );

	g_list_foreach( self->private->entries, ( GFunc ) release_entry, NULL );
	g_list_free( self->private->entries );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * na_settings_new:
 *
 * Returns: a new #NASettings object which should be g_object_unref()
 * by the caller.
 */
NASettings *
na_settings_new( void )
{
	NASettings *settings;
	gchar *dir;

	settings = g_object_new( NA_SETTINGS_TYPE, NULL );

	dir = g_build_filename( SYSCONFDIR, "xdg", NULL );
	settings->private->global_conf = initialize_settings(
			settings, dir,
			&settings->private->global_monitor,
			&settings->private->global_handler );
	g_free( dir );

	dir = g_build_filename( g_get_home_dir(), ".config", NULL );
	settings->private->user_conf = initialize_settings(
			settings, dir,
			&settings->private->user_monitor,
			&settings->private->user_handler );
	g_free( dir );

	return( settings );
}

/**
 * na_settings_register_key_callback:
 * @settings: this #NASettings instance.
 * @key: the key to be monitored.
 * @callback: the function to be called when the value of the key changes.
 * @user_data: data to be passed to the @callback function.
 *
 * Registers a new consumer of the monitoring of the @key.
 *
 * Since: 3.1.0
 */
void
na_settings_register_key_callback( NASettings *settings, const gchar *key, GCallback callback, gpointer user_data )
{
	g_return_if_fail( NA_IS_SETTINGS( settings ));

	if( !settings->private->dispose_has_run ){

		Consumer *consumer = g_new0( Consumer, 1 );

		consumer->key = g_strdup( key );
		consumer->callback = callback;
		consumer->user_data = user_data;
		settings->private->consumers = g_list_prepend( settings->private->consumers, consumer );

		if( !strcmp( key, NA_SETTINGS_RUNTIME_IO_PROVIDER_READ_STATUS )){
			monitor_io_provider_read_status( settings );

		} else if( !strcmp( key, NA_SETTINGS_RUNTIME_IO_PROVIDERS_READ_ORDER ) ||
					!strcmp( key, NA_SETTINGS_RUNTIME_ITEMS_ADD_ABOUT_ITEM ) ||
					!strcmp( key, NA_SETTINGS_RUNTIME_ITEMS_CREATE_ROOT_MENU ) ||
					!strcmp( key, NA_SETTINGS_RUNTIME_ITEMS_LEVEL_ZERO_ORDER )){
			monitor_key( settings, key );
		}
	}

}

/**
 * na_settings_register_global_callback:
 * @settings: this #NASettings instance.
 * @callback: the function to be called when the value of the key changes.
 * @user_data: data to be passed to the @callback function.
 *
 * Registers a new consumer of the monitoring of the configuration files.
 *
 * Since: 3.1.0
 */
void
na_settings_register_global_callback( NASettings *settings, GCallback callback, gpointer user_data )
{
	g_return_if_fail( NA_IS_SETTINGS( settings ));

	if( !settings->private->dispose_has_run ){

		Consumer *consumer = g_new0( Consumer, 1 );

		consumer->key = NULL;
		consumer->callback = callback;
		consumer->user_data = user_data;
		settings->private->consumers = g_list_prepend( settings->private->consumers, consumer );
	}

}

/**
 * na_settings_get_boolean:
 * @settings: this #NASettings instance.
 * @key: the key whose value is to be returned.
 * @found: if not %NULL, a pointer to a gboolean in which we will store
 *  whether the searched @key has been found (%TRUE), or if the returned
 *  value comes from default (%FALSE).
 * @global: if not %NULL, a pointer to a gboolean in which we will store
 *  whether the returned value has been readen from global preferences
 *  (%TRUE), or from the user preferences (%FALSE). Global preferences
 *  are usually read-only. When the @key has not been found, @global
 *  is set to %FALSE.
 *
 * Returns: the value of the key, of its default value if not found.
 *
 * Since: 3.1.0
 */
gboolean
na_settings_get_boolean( NASettings *settings, const gchar *key, gboolean *found, gboolean *global )
{
	static const gchar *thisfn = "na_settings_get_boolean";
	gboolean value;
	KeyDef *key_def;

	g_return_val_if_fail( NA_IS_SETTINGS( settings ), FALSE );

	value = FALSE;
	if( found ){
		*found = FALSE;
	}
	if( global ){
		*global = FALSE;
	}

	if( !settings->private->dispose_has_run ){

		key_def = get_key_def( key );
		if( key_def ){
			value = na_settings_get_boolean_ex( settings, key_def->group, key, key_def->default_value, found, global );

		} else {
			g_warning( "%s: no KeyDef found for key=%s", thisfn, key );
		}
	}

	return( value );
}

/**
 * na_settings_get_boolean_ex:
 * @settings: this #NASettings instance.
 * @group: the group where the @key is to be searched for.
 * @key: the key whose value is to be returned.
 * @default_value: as 'true' or 'false', may be %NULL which defaults to %FALSE.
 * @found: if not %NULL, a pointer to a gboolean in which we will store
 *  whether the searched @key has been found (%TRUE), or if the returned
 *  value comes from default (%FALSE).
 * @global: if not %NULL, a pointer to a gboolean in which we will store
 *  whether the returned value has been readen from global preferences
 *  (%TRUE), or from the user preferences (%FALSE). Global preferences
 *  are usually read-only. When the @key has not been found, @global
 *  is set to %FALSE.
 *
 * Returns: the value of the key, of its default value if not found.
 *
 * Since: 3.1.0
 */
gboolean
na_settings_get_boolean_ex( NASettings *settings, const gchar *group, const gchar *key, const gchar *default_value, gboolean *found, gboolean *global )
{
	static const gchar *thisfn = "na_settings_get_boolean_ex";
	gboolean value;
	GError *error;
	gboolean has_entry;

	g_return_val_if_fail( NA_IS_SETTINGS( settings ), FALSE );

	value = FALSE;
	if( found ){
		*found = FALSE;
	}
	if( global ){
		*global = FALSE;
	}

	if( !settings->private->dispose_has_run ){

		error = NULL;
		has_entry = TRUE;
		value = g_key_file_get_boolean( settings->private->global_conf, group, key, &error );
		if( error ){
			has_entry = FALSE;
			if( error->code != G_KEY_FILE_ERROR_KEY_NOT_FOUND && error->code != G_KEY_FILE_ERROR_GROUP_NOT_FOUND ){
				g_warning( "%s: (global) %s", thisfn, error->message );
			}
			g_error_free( error );
			error = NULL;

		} else {
			if( found ){
				*found = TRUE;
			}
			if( global ){
				*global = TRUE;
			}
		}
		if( !has_entry ){
			value = g_key_file_get_boolean( settings->private->user_conf, group, key, &error );
			if( error ){
				if( error->code != G_KEY_FILE_ERROR_KEY_NOT_FOUND && error->code != G_KEY_FILE_ERROR_GROUP_NOT_FOUND ){
					g_warning( "%s: (user) %s", thisfn, error->message );
				}
				g_error_free( error );
				error = NULL;
			} else {
				has_entry = TRUE;
				if( found ){
					*found = TRUE;
				}
			}
		}
		if( !has_entry ){
			if( default_value ){
				value = ( strcmp( default_value, "true" ) == 0 );
			}
		}
	}

	return( value );
}

/**
 * na_settings_get_string_list:
 * @settings: this #NASettings instance.
 * @key: the key whose value is to be returned.
 * @found: if not %NULL, a pointer to a gboolean in which we will store
 *  whether the searched @key has been found (%TRUE), or if the returned
 *  value comes from default (%FALSE).
 * @global: if not %NULL, a pointer to a gboolean in which we will store
 *  whether the returned value has been readen from global preferences
 *  (%TRUE), or from the user preferences (%FALSE). Global preferences
 *  are usually read-only. When the @key has not been found, @global
 *  is set to %FALSE.
 *
 * Returns: the value of the key as a newly allocated list of strings.
 * The returned list should be na_core_utils_slist_free() by the caller.
 *
 * Since: 3.1.0
 */
GSList *
na_settings_get_string_list( NASettings *settings, const gchar *key, gboolean *found, gboolean *global )
{
	static const gchar *thisfn = "na_settings_get_string_list";
	GSList *value;
	KeyDef *key_def;

	g_return_val_if_fail( NA_IS_SETTINGS( settings ), FALSE );

	value = NULL;
	if( found ){
		*found = FALSE;
	}
	if( global ){
		*global = FALSE;
	}

	if( !settings->private->dispose_has_run ){

		key_def = get_key_def( key );

		if( key_def ){
			value = get_string_list_ex( settings, key_def->group, key, key_def->default_value, found, global );

		} else {
			g_warning( "%s: no KeyDef found for key=%s", thisfn, key );
		}
	}

	return( value );
}

static KeyDef *
get_key_def( const gchar *key )
{
	KeyDef *found = NULL;
	KeyDef *idef;

	idef = ( KeyDef * ) st_def_keys;
	while( idef && !found ){
		if( !strcmp( idef->key, key )){
			found = idef;
		}
		idef++;
	}

	return( found );
}

/*
 * get_string_ex:
 * @settings: this #NASettings instance.
 * @key: the key whose value is to be returned.
 * @found: if not %NULL, a pointer to a gboolean in which we will store
 *  whether the searched @key has been found (%TRUE), or if the returned
 *  value comes from default (%FALSE).
 * @global: if not %NULL, a pointer to a gboolean in which we will store
 *  whether the returned value has been readen from global preferences
 *  (%TRUE), or from the user preferences (%FALSE). Global preferences
 *  are usually read-only. When the @key has not been found, @global
 *  is set to %FALSE.
 *
 * Returns: the value of the key as a newly allocated string which should
 * be g_free() by the caller.
 *
 * Since: 3.1.0
 */
static gchar *
get_string_ex( NASettings *settings, const gchar *group, const gchar *key, const gchar *default_value, gboolean *found, gboolean *global )
{
	static const gchar *thisfn = "na_settings_get_string_ex";
	gchar *value;
	GError *error;
	gboolean has_entry;

	g_return_val_if_fail( NA_IS_SETTINGS( settings ), NULL );

	value = NULL;
	if( found ){
		*found = FALSE;
	}
	if( global ){
		*global = FALSE;
	}

	if( !settings->private->dispose_has_run ){
		error = NULL;
		has_entry = TRUE;
		value = g_key_file_get_string( settings->private->global_conf, group, key, &error );
		if( error ){
			has_entry = FALSE;
			if( error->code != G_KEY_FILE_ERROR_KEY_NOT_FOUND && error->code != G_KEY_FILE_ERROR_GROUP_NOT_FOUND ){
				g_warning( "%s: (global) %s", thisfn, error->message );
			}
			g_error_free( error );
			error = NULL;

		} else {
			if( found ){
				*found = TRUE;
			}
			if( global ){
				*global = TRUE;
			}
		}
		if( !has_entry ){
			value = g_key_file_get_string( settings->private->user_conf, group, key, &error );
			if( error ){
				if( error->code != G_KEY_FILE_ERROR_KEY_NOT_FOUND && error->code != G_KEY_FILE_ERROR_GROUP_NOT_FOUND ){
					g_warning( "%s: (user) %s", thisfn, error->message );
				}
				g_error_free( error );
				error = NULL;
			} else {
				has_entry = TRUE;
				if( found ){
					*found = TRUE;
				}
			}
		}
		if( !has_entry ){
			value = g_strdup( default_value );
		}
	}

	return( value );
}

/*
 * get_string_list_ex:
 * @settings: this #NASettings instance.
 * @key: the key whose value is to be returned.
 * @found: if not %NULL, a pointer to a gboolean in which we will store
 *  whether the searched @key has been found (%TRUE), or if the returned
 *  value comes from default (%FALSE).
 * @global: if not %NULL, a pointer to a gboolean in which we will store
 *  whether the returned value has been readen from global preferences
 *  (%TRUE), or from the user preferences (%FALSE). Global preferences
 *  are usually read-only. When the @key has not been found, @global
 *  is set to %FALSE.
 *
 * Returns: the value of the key as a newly allocated list of strings.
 * The returned list should be na_core_utils_slist_free() by the caller.
 *
 * Since: 3.1.0
 */
static GSList *
get_string_list_ex( NASettings *settings, const gchar *group, const gchar *key, const gchar *default_value, gboolean *found, gboolean *global )
{
	static const gchar *thisfn = "na_settings_get_string_list_ex";
	GSList *value;
	gchar **array;
	GError *error;
	gboolean has_entry;

	g_return_val_if_fail( NA_IS_SETTINGS( settings ), NULL );

	value = NULL;
	if( found ){
		*found = FALSE;
	}
	if( global ){
		*global = FALSE;
	}

	if( !settings->private->dispose_has_run ){
		error = NULL;
		has_entry = TRUE;
		array = g_key_file_get_string_list( settings->private->global_conf, group, key, NULL, &error );
		if( error ){
			has_entry = FALSE;
			if( error->code != G_KEY_FILE_ERROR_KEY_NOT_FOUND && error->code != G_KEY_FILE_ERROR_GROUP_NOT_FOUND ){
				g_warning( "%s: (global) %s", thisfn, error->message );
			}
			g_error_free( error );
			error = NULL;

		} else {
			if( found ){
				*found = TRUE;
			}
			if( global ){
				*global = TRUE;
			}
		}
		if( !has_entry ){
			array = g_key_file_get_string_list( settings->private->user_conf, group, key, NULL, &error );
			if( error ){
				if( error->code != G_KEY_FILE_ERROR_KEY_NOT_FOUND && error->code != G_KEY_FILE_ERROR_GROUP_NOT_FOUND ){
					g_warning( "%s: (user) %s", thisfn, error->message );
				}
				g_error_free( error );
				error = NULL;
			} else {
				has_entry = TRUE;
				if( found ){
					*found = TRUE;
				}
			}
		}
		if( !has_entry ){
			value = g_slist_append( NULL, g_strdup( default_value ));
		} else {
			value = na_core_utils_slist_from_array(( const gchar ** ) array );
			g_strfreev( array );
		}
	}

	return( value );
}

/*
 * called from na_settings_new
 * allocate and load the key files for global and user preferences
 */
static GKeyFile *
initialize_settings( NASettings* settings, const gchar *dir, GFileMonitor **monitor, gulong *handler )
{
	static const gchar *thisfn = "na_settings_initialize_settings";
	GKeyFile *key_file;
	GError *error;
	gchar *path;
	GFile *file;

	key_file = g_key_file_new();

	error = NULL;
	path = g_strdup_printf( "%s/%s.conf", dir, PACKAGE );
	if( !g_key_file_load_from_file( key_file, path, G_KEY_FILE_KEEP_COMMENTS, &error )){
		g_warning( "%s: %s", thisfn, error->message );
		g_error_free( error );
		error = NULL;
	}

	file = g_file_new_for_path( path );
	*monitor = g_file_monitor_file( file, 0, NULL, &error );
	if( error ){
		g_warning( "%s: %s", thisfn, error->message );
		g_error_free( error );
		error = NULL;

	} else {
		*handler = g_signal_connect( *monitor, "changed", ( GCallback ) on_conf_changed, settings );
	}
	g_object_unref( file );

	g_free( path );
	return( key_file );
}

/*
 * this is a fake key, as we actually monitor the 'readable' status
 * of all io-providers
 * add to the list of monitored keys the found io-providers
 */
static void
monitor_io_provider_read_status( NASettings *settings )
{
	monitor_io_provider_read_status_conf( settings, settings->private->global_conf, TRUE );

	monitor_io_provider_read_status_conf( settings, settings->private->user_conf, FALSE );
}

static void
monitor_io_provider_read_status_conf( NASettings *settings, GKeyFile *key_file, gboolean global )
{
	gchar **array;
	gchar **idx;
	gboolean readable;
	gboolean found;

	array = g_key_file_get_groups( key_file, NULL );
	if( array ){
		idx = array;
		while( idx ){
			if( g_str_has_prefix( *idx, "io-provider " )){
				readable = na_settings_get_boolean_ex( settings, *idx, IO_PROVIDER_READABLE, NULL, &found, NULL );
				if( found ){
					monitor_key_add( settings, *idx, IO_PROVIDER_READABLE, GUINT_TO_POINTER(( guint ) readable ), global, NAFD_TYPE_BOOLEAN );
				}
			}
			idx++;
		}
		g_strfreev( array );
	}
}

static void
monitor_key( NASettings *settings, const gchar *key )
{
	static const gchar *thisfn = "na_settings_monitor_key";
	KeyDef *key_def;
	gpointer value;
	gboolean found, global;

	found = FALSE;
	key_def = get_key_def( key );
	if( key_def ){
		switch( key_def->data_type ){

			case NAFD_TYPE_STRING:
			case NAFD_TYPE_MAP:
				value = get_string_ex( settings, key_def->group, key, NULL, &found, &global );
				break;

			case NAFD_TYPE_BOOLEAN:
				value = GUINT_TO_POINTER( na_settings_get_boolean_ex( settings, key_def->group, key, NULL, &found, &global ));
				break;

			case NAFD_TYPE_STRING_LIST:
				value = get_string_list_ex( settings, key_def->group, key, NULL, &found, &global );
				break;

			case NAFD_TYPE_LOCALE_STRING:
			case NAFD_TYPE_POINTER:
			case NAFD_TYPE_UINT:
				g_warning( "%s: unmanaged data type: %s", thisfn, na_data_types_get_label( key_def->data_type ));
				break;

			default:
				g_warning( "%s: unknown data type: %d", thisfn, key_def->data_type );
		}

	} else {
		g_warning( "%s: no KeyDef found for key=%s", thisfn, key );
	}

	if( found ){
		monitor_key_add( settings, key_def->group, key, value, global, key_def->data_type );
	}
}

/* add the key if not already monitored
 */
static void
monitor_key_add( NASettings *settings, const gchar *group, const gchar *key, gpointer value, gboolean global, guint type )
{
	GList *it;
	gboolean found;
	Entry *entry;

	for( it=settings->private->entries, found=FALSE ; it && !found ; it=it->next ){
		entry = ( Entry * ) it->data;
		if( !strcmp( entry->group, group ) &&
			!strcmp( entry->key, key ) &&
			entry->global == global ){
				found = TRUE;
		}
	}

	if( !found ){
		entry = g_new0( Entry, 1 );
		entry->group = g_strdup( group );
		entry->key = g_strdup( key );
		entry->value = na_data_types_copy( value, type );
		entry->global = global;
		entry->type = type;

		settings->private->entries = g_list_prepend( settings->private->entries, entry );
	}
}

/*
 * one of the two monitored configuration files have changed on the disk
 * we do not try to identify which keys have actually change
 * instead we trigger each registered consumer for the 'global' event
 *
 * consumers which register for the 'global_conf' event are recorded
 * with a NULL key
 */
static void
on_conf_changed( GFileMonitor *monitor,
		GFile *file, GFile *other_file, GFileMonitorEvent event_type, NASettings *settings )
{
	GList *ic;
	Consumer *consumer;
	gchar *path;
	GFile *prefix;
	gboolean global;

	g_return_if_fail( NA_IS_SETTINGS( settings ));

	if( !settings->private->dispose_has_run ){

		path = g_build_filename( SYSCONFDIR, "xdg", NULL );
		prefix = g_file_new_for_path( path );
		global = g_file_has_prefix( file, prefix );
		g_object_unref( prefix );
		g_free( path );

		for( ic = settings->private->consumers ; ic ; ic = ic->next ){
			consumer = ( Consumer * ) ic->data;
			if( !consumer->key ){
				( *( global_fn ) consumer->callback )( global, consumer->user_data );
			}
		}
	}
}

/*
 * called from instance_finalize
 * release the list of registered consumers
 */
static void
release_consumer( Consumer *consumer )
{
	g_free( consumer->key );
	g_free( consumer );
}

/*
 * called from instance_finalize
 * release the list of monitored entries
 */
static void
release_entry( Entry *entry )
{
	g_free( entry->group );
	g_free( entry->key );
	na_data_types_free( entry->value, entry->type );
	g_free( entry );
}
