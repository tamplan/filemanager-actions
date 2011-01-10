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

#include <api/na-data-types.h>

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
	GKeyFile     *global_conf;
	GFileMonitor *global_monitor;
	gulong        global_handler;
	GKeyFile     *user_conf;
	GFileMonitor *user_monitor;
	gulong        user_handler;
	GList        *consumers;
};

#define GROUP_NACT						"nact"
#define GROUP_RUNTIME					"runtime"
#define GROUP_IO_PROVIDER				"io-provider"

typedef struct {
	const gchar *key;
	const gchar *group;
	guint        data_type;				/* picked up from NADataFactory */
	const gchar *default_value;
}
	KeyDef;

static const KeyDef st_def_keys[] = {
		{ NA_SETTINGS_RUNTIME_ITEMS_ADD_ABOUT_ITEM,   GROUP_RUNTIME, NAFD_TYPE_BOOLEAN, "true" },
		{ NA_SETTINGS_RUNTIME_ITEMS_CREATE_ROOT_MENU, GROUP_RUNTIME, NAFD_TYPE_BOOLEAN, "true" },
		{ 0 }
};

typedef struct {
	gchar    *key;
	GCallback callback;
	gpointer  user_data;
}
	Consumer;

static GObjectClass *st_parent_class = NULL;

static GType     register_type( void );
static void      class_init( NASettingsClass *klass );
static void      instance_init( GTypeInstance *instance, gpointer klass );
static void      instance_dispose( GObject *object );
static void      instance_finalize( GObject *object );

static GKeyFile *initialize_settings( NASettings* settings, const gchar *dir, GFileMonitor **monitor, gulong *handler );
static void      on_conf_changed( GFileMonitor *monitor, GFile *file, GFile *other_file, GFileMonitorEvent event_type, NASettings *settings );
static void      release_consumer( Consumer *consumer );

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
 * na_settings_register:
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
na_settings_register( NASettings *settings, const gchar *key, GCallback callback, gpointer user_data )
{
	g_return_if_fail( NA_IS_SETTINGS( settings ));

	if( !settings->private->dispose_has_run ){

		Consumer *consumer = g_new0( Consumer, 1 );

		consumer->key = g_strdup( key );
		consumer->callback = callback;
		consumer->user_data = user_data;
		settings->private->consumers = g_list_prepend( settings->private->consumers, consumer );
	}

}

/**
 * na_settings_register_global:
 * @settings: this #NASettings instance.
 * @callback: the function to be called when the value of the key changes.
 * @user_data: data to be passed to the @callback function.
 *
 * Registers a new consumer of the monitoring of the configuration files.
 *
 * Since: 3.1.0
 */
void
na_settings_register_global( NASettings *settings, GCallback callback, gpointer user_data )
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
	gboolean value;

	g_return_val_if_fail( NA_IS_SETTINGS( settings ), FALSE );

	value = FALSE;
	if( found ){
		*found = FALSE;
	}
	if( global ){
		*global = FALSE;
	}

	if( !settings->private->dispose_has_run ){

	}

	return( value );
}

/**
 * na_settings_get_value:
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
gpointer
na_settings_get_value( NASettings *settings, const gchar *key, gboolean *found, gboolean *global )
{
	gpointer value;

	g_return_val_if_fail( NA_IS_SETTINGS( settings ), NULL );

	value = NULL;
	if( found ){
		*found = FALSE;
	}
	if( global ){
		*global = FALSE;
	}

	if( !settings->private->dispose_has_run ){

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

static void
on_conf_changed( GFileMonitor *monitor,
		GFile *file, GFile *other_file, GFileMonitorEvent event_type, NASettings *settings )
{
	g_return_if_fail( NA_IS_SETTINGS( settings ));
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
