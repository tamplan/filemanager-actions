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

#include "na-gconf-monitor.h"

/* private class data
 */
struct NAGConfMonitorClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NAGConfMonitorPrivate {
	gboolean              dispose_has_run;
	GConfClient          *gconf;
	gchar                *path;
	gint                  preload;
	GConfClientNotifyFunc handler;
	gpointer              user_data;
	guint                 monitor_id;
};

static GObjectClass *st_parent_class = NULL;

static GType register_type( void );
static void  class_init( NAGConfMonitorClass *klass );
static void  instance_init( GTypeInstance *instance, gpointer klass );
static void  instance_dispose( GObject *object );
static void  instance_finalize( GObject *object );

static guint install_monitor( NAGConfMonitor *monitor );
static void  release_monitor( NAGConfMonitor *monitor );

GType
na_gconf_monitor_get_type( void )
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
	static const gchar *thisfn = "na_gconf_monitor_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NAGConfMonitorClass ),
		NULL,
		NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NAGConfMonitor ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "NAGConfMonitor", &info, 0 );

	return( type );
}

static void
class_init( NAGConfMonitorClass *klass )
{
	static const gchar *thisfn = "na_gconf_monitor_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NAGConfMonitorClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "na_gconf_monitor_instance_init";
	NAGConfMonitor *self;

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );
	g_return_if_fail( NA_IS_GCONF_MONITOR( instance ));
	self = NA_GCONF_MONITOR( instance );

	self->private = g_new0( NAGConfMonitorPrivate, 1 );
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "na_gconf_monitor_instance_dispose";
	NAGConfMonitor *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_return_if_fail( NA_IS_GCONF_MONITOR( object ));
	self = NA_GCONF_MONITOR( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* release the installed monitor */
		release_monitor( self );

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	NAGConfMonitor *self;

	g_return_if_fail( NA_IS_GCONF_MONITOR( object ));
	self = NA_GCONF_MONITOR( object );

	g_free( self->private->path );
	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * na_gconf_monitor_new:
 * @client: a #GConfClient object already initialized by the caller.
 * @path: the absolute path to monitor.
 * @preload: a #GConfClientPreloadType for this monitoring.
 * @handler: the function to be triggered by the monitor.
 * @user_data: data to pass to the @handler.
 *
 * Initializes the monitoring of a GConf path.
 */
NAGConfMonitor *
na_gconf_monitor_new( GConfClient *client, const gchar *path, gint preload, GConfClientNotifyFunc handler, gpointer user_data )
{
	static const gchar *thisfn = "na_gconf_monitor_new";
	NAGConfMonitor *monitor;

	g_debug( "%s: client=%p, path=%s, preload=%d, user_data=%p",
			thisfn, ( void * ) client, path, preload, ( void * ) user_data );

	monitor = g_object_new( NA_GCONF_MONITOR_TYPE, NULL );

	monitor->private->gconf = client;
	monitor->private->path = g_strdup( path );
	monitor->private->preload = preload;
	monitor->private->handler = handler;
	monitor->private->user_data = user_data;

	monitor->private->monitor_id = install_monitor( monitor );

	return( monitor );
}

static guint
install_monitor( NAGConfMonitor *monitor )
{
	static const gchar *thisfn = "na_gconf_monitor_install_monitor";
	GError *error = NULL;
	guint notify_id;

	g_return_val_if_fail( NA_IS_GCONF_MONITOR( monitor ), 0 );
	g_return_val_if_fail( !monitor->private->dispose_has_run, 0 );

	gconf_client_add_dir(
			monitor->private->gconf,
			monitor->private->path,
			monitor->private->preload,
			&error );

	if( error ){
		g_warning( "%s[gconf_client_add_dir] path=%s, error=%s", thisfn, monitor->private->path, error->message );
		g_error_free( error );
		return( 0 );
	}

	notify_id = gconf_client_notify_add(
			monitor->private->gconf,
			monitor->private->path,
			monitor->private->handler,
			monitor->private->user_data,
			NULL,
			&error );

	if( error ){
		g_warning( "%s[gconf_client_notify_add] path=%s, error=%s", thisfn, monitor->private->path, error->message );
		g_error_free( error );
		return( 0 );
	}

	return( notify_id );
}

/**
 * na_gconf_monitor_release_monitors:
 * @monitors: a list of #NAGConfMonitors.
 *
 * Release allocated monitors.
 */
void
na_gconf_monitor_release_monitors( GSList *monitors )
{
	g_slist_foreach( monitors, ( GFunc ) g_object_unref, NULL );
	g_slist_free( monitors );
}

static void
release_monitor( NAGConfMonitor *monitor )
{
	static const gchar *thisfn = "na_gconf_monitor_release_monitor";
	GError *error = NULL;

	g_debug( "%s: monitor=%p", thisfn, ( void * ) monitor );

	g_return_if_fail( NA_IS_GCONF_MONITOR( monitor ));

	if( monitor->private->monitor_id ){
		gconf_client_notify_remove( monitor->private->gconf, monitor->private->monitor_id );
	}

	gconf_client_remove_dir( monitor->private->gconf, monitor->private->path, &error );

	if( error ){
		g_warning( "%s[gconf_client_remove_dir] path=%s, error=%s", thisfn, monitor->private->path, error->message );
		g_error_free( error );
	}
}
