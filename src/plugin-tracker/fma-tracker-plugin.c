/*
 * FileManager-Actions
 * A file-manager extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
 *
 * FileManager-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * FileManager-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FileManager-Actions; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
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

#include <api/fma-dbus.h>
#include <api/fma-fm-defines.h>

#include "fma-tracker-plugin.h"
#include "fma-tracker-gdbus.h"

/* private class data
 */
struct _FMATrackerPluginClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _FMATrackerPluginPrivate {
	gboolean                  dispose_has_run;
	guint                     owner_id;	/* the identifier returns by g_bus_own_name */
	GDBusObjectManagerServer *manager;
	GList                    *selected;
};

static GObjectClass *st_parent_class = NULL;
static GType         st_module_type = 0;

static void     class_init( FMATrackerPluginClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     initialize_dbus_connection( FMATrackerPlugin *tracker );
static void     on_bus_acquired( GDBusConnection *connection, const gchar *name, FMATrackerPlugin *tracker );
static void     on_name_acquired( GDBusConnection *connection, const gchar *name, FMATrackerPlugin *tracker );
static void     on_name_lost( GDBusConnection *connection, const gchar *name, FMATrackerPlugin *tracker );
static gboolean on_properties1_get_selected_paths( FMATrackerGDBusProperties1 *properties, GDBusMethodInvocation *invocation, FMATrackerPlugin *tracker );
static void     instance_dispose( GObject *object );
static void     instance_finalize( GObject *object );

static void     menu_provider_iface_init( FileManagerMenuProviderIface *iface );
static GList   *menu_provider_get_background_items( FileManagerMenuProvider *provider, GtkWidget *window, FileManagerFileInfo *folder );
static GList   *menu_provider_get_file_items( FileManagerMenuProvider *provider, GtkWidget *window, GList *files );

static void     set_uris( FMATrackerPlugin *tracker, GList *files );
static gchar  **get_selected_paths( FMATrackerPlugin *tracker );
static GList   *free_selected( GList *selected );

GType
fma_tracker_plugin_get_type( void )
{
	g_assert( st_module_type );
	return( st_module_type );
}

void
fma_tracker_plugin_register_type( GTypeModule *module )
{
	static const gchar *thisfn = "fma_tracker_plugin_register_type";

	static const GTypeInfo info = {
		sizeof( FMATrackerPluginClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( FMATrackerPlugin ),
		0,
		( GInstanceInitFunc ) instance_init,
	};

	static const GInterfaceInfo menu_provider_iface_info = {
		( GInterfaceInitFunc ) menu_provider_iface_init,
		NULL,
		NULL
	};

	g_debug( "%s: module=%p", thisfn, ( void * ) module );
	g_assert( st_module_type == 0 );

	st_module_type = g_type_module_register_type( module, G_TYPE_OBJECT, "FMATrackerPlugin", &info, 0 );

	g_type_module_add_interface( module, st_module_type, FILE_MANAGER_TYPE_MENU_PROVIDER, &menu_provider_iface_info );
}

static void
class_init( FMATrackerPluginClass *klass )
{
	static const gchar *thisfn = "fma_tracker_plugin_class_init";
	GObjectClass *gobject_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	gobject_class = G_OBJECT_CLASS( klass );
	gobject_class->dispose = instance_dispose;
	gobject_class->finalize = instance_finalize;

	klass->private = g_new0( FMATrackerPluginClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "fma_tracker_plugin_instance_init";
	FMATrackerPlugin *self;

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );
	g_return_if_fail( FMA_IS_TRACKER_PLUGIN( instance ));

	self = FMA_TRACKER_PLUGIN( instance );

	self->private = g_new0( FMATrackerPluginPrivate, 1 );
	self->private->dispose_has_run = FALSE;

	initialize_dbus_connection( self );
}

/*
 * initialize the DBus connection at instanciation time
 * & instantiate the object which will do effective tracking
 */
static void
initialize_dbus_connection( FMATrackerPlugin *tracker )
{
	FMATrackerPluginPrivate *priv = tracker->private;

	priv->owner_id = g_bus_own_name(
			G_BUS_TYPE_SESSION,
			FILEMANAGER_ACTIONS_DBUS_SERVICE,
			G_BUS_NAME_OWNER_FLAGS_REPLACE,
			( GBusAcquiredCallback ) on_bus_acquired,
			( GBusNameAcquiredCallback ) on_name_acquired,
			( GBusNameLostCallback ) on_name_lost,
			tracker,
			NULL );
}

static void
on_bus_acquired( GDBusConnection *connection, const gchar *name, FMATrackerPlugin *tracker )
{
	static const gchar *thisfn = "fma_tracker_plugin_on_bus_acquired";
	FMATrackerGDBusObjectSkeleton *tracker_object;
	FMATrackerGDBusProperties1 *tracker_properties1;

	/*FMATrackerGDBusDBus *tracker_object;*/

	g_debug( "%s: connection=%p, name=%s, tracker=%p",
			thisfn,
			( void * ) connection,
			name,
			( void * ) tracker );

	/* create a new org.freedesktop.DBus.ObjectManager rooted at
	 *  /org/filemanager_actions/DBus/Tracker
	 */
	tracker->private->manager = g_dbus_object_manager_server_new( FILEMANAGER_ACTIONS_DBUS_TRACKER_PATH );

	/* create a new D-Bus object at the path
	 *  /org/filemanager_actions/DBus/Tracker
	 *  (which must be same or below than that of object manager server)
	 */
	tracker_object = fma_tracker_gdbus_object_skeleton_new( FILEMANAGER_ACTIONS_DBUS_TRACKER_PATH "/0" );

	/* make a newly created object export the interface
	 *  org.filemanager_actions.DBus.Tracker.Properties1
	 *  and attach it to the D-Bus object, which takes its own reference on it
	 */
	tracker_properties1 = fma_tracker_gdbus_properties1_skeleton_new();
	fma_tracker_gdbus_object_skeleton_set_properties1( tracker_object, tracker_properties1 );
	g_object_unref( tracker_properties1 );

	/* handle GetSelectedPaths method invocation on the .Properties1 interface
	 */
	g_signal_connect(
			tracker_properties1,
			"handle-get-selected-paths",
			G_CALLBACK( on_properties1_get_selected_paths ),
			tracker );

	/* and export the DBus object on the object manager server
	 * (which takes its own reference on it)
	 */
	g_dbus_object_manager_server_export( tracker->private->manager, G_DBUS_OBJECT_SKELETON( tracker_object ));
	g_object_unref( tracker_object );

	/* and connect the object manager server to the D-Bus session
	 * exporting all attached objects
	 */
	g_dbus_object_manager_server_set_connection( tracker->private->manager, connection );
}

static void
on_name_acquired( GDBusConnection *connection, const gchar *name, FMATrackerPlugin *tracker )
{
	static const gchar *thisfn = "fma_tracker_plugin_on_name_acquired";

	g_debug( "%s: connection=%p, name=%s, tracker=%p",
			thisfn,
			( void * ) connection,
			name,
			( void * ) tracker );
}

static void
on_name_lost( GDBusConnection *connection, const gchar *name, FMATrackerPlugin *tracker )
{
	static const gchar *thisfn = "fma_tracker_plugin_on_name_lost";

	g_debug( "%s: connection=%p, name=%s, tracker=%p",
			thisfn,
			( void * ) connection,
			name,
			( void * ) tracker );
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "fma_tracker_plugin_instance_dispose";
	FMATrackerPluginPrivate *priv;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_return_if_fail( FMA_IS_TRACKER_PLUGIN( object ));

	priv = FMA_TRACKER_PLUGIN( object )->private;

	if( !priv->dispose_has_run ){

		priv->dispose_has_run = TRUE;

		if( priv->owner_id ){
			g_bus_unown_name( priv->owner_id );
		}
		if( priv->manager ){
			g_object_unref( priv->manager );
		}

		priv->selected = free_selected( priv->selected );

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "fma_tracker_plugin_instance_finalize";
	FMATrackerPlugin *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_return_if_fail( FMA_IS_TRACKER_PLUGIN( object ));
	self = FMA_TRACKER_PLUGIN( object );

	g_free( self->private );

	/* chain up to the parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

static void
menu_provider_iface_init( FileManagerMenuProviderIface *iface )
{
	static const gchar *thisfn = "fma_tracker_plugin_menu_provider_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->get_background_items = menu_provider_get_background_items;
	iface->get_file_items = menu_provider_get_file_items;
}

static GList *
menu_provider_get_background_items( FileManagerMenuProvider *provider, GtkWidget *window, FileManagerFileInfo *folder )
{
	static const gchar *thisfn = "fma_tracker_plugin_menu_provider_get_background_items";
	FMATrackerPlugin *tracker;
	gchar *uri;
	GList *selected;

	g_return_val_if_fail( FMA_IS_TRACKER_PLUGIN( provider ), NULL );

	tracker = FMA_TRACKER_PLUGIN( provider );

	if( !tracker->private->dispose_has_run ){

		uri = file_manager_file_info_get_uri( folder );
		g_debug( "%s: provider=%p, window=%p, folder=%s",
				thisfn,
				( void * ) provider,
				( void * ) window,
				uri );
		g_free( uri );

		selected = g_list_prepend( NULL, folder );
		set_uris( tracker, selected );
		g_list_free( selected );
	}

	return( NULL );
}

/*
 * this function is called each time the selection changed
 * menus items are available :
 * a) in Edit menu while the selection stays unchanged
 * b) in contextual menu while the selection stays unchanged
 */
static GList *
menu_provider_get_file_items( FileManagerMenuProvider *provider, GtkWidget *window, GList *files )
{
	static const gchar *thisfn = "fma_tracker_plugin_menu_provider_get_file_items";
	FMATrackerPlugin *tracker;

	g_return_val_if_fail( FMA_IS_TRACKER_PLUGIN( provider ), NULL );

	tracker = FMA_TRACKER_PLUGIN( provider );

	if( !tracker->private->dispose_has_run ){

		g_debug( "%s: provider=%p, window=%p, files=%p, count=%d",
				thisfn,
				( void * ) provider,
				( void * ) window,
				( void * ) files, g_list_length( files ));

		set_uris( tracker, files );
	}

	return( NULL );
}

/*
 * set_uris:
 * @tracker: this #FMATrackerPlugin instance.
 * @files: the list of currently selected items.
 *
 * Maintains our own list of uris.
 */
static void
set_uris( FMATrackerPlugin *tracker, GList *files )
{
	FMATrackerPluginPrivate *priv;

	priv = tracker->private;

	priv->selected = free_selected( tracker->private->selected );
	priv->selected = file_manager_file_info_list_copy( files );
}

/*
 * Returns: %TRUE if the method has been handled.
 */
static gboolean
on_properties1_get_selected_paths( FMATrackerGDBusProperties1 *properties, GDBusMethodInvocation *invocation, FMATrackerPlugin *tracker )
{
	gchar **paths;

	g_return_val_if_fail( FMA_IS_TRACKER_PLUGIN( tracker ), FALSE );

	paths = get_selected_paths( tracker );

	fma_tracker_gdbus_properties1_complete_get_selected_paths(
			properties,
			invocation,
			( const gchar * const * ) paths );

	return( TRUE );
}

/*
 * get_selected_paths:
 * @tracker: this #FMATrackerPlugin object.
 *
 * Sends on session D-Bus the list of currently selected items, as two
 * strings for each item :
 * - the uri
 * - the mimetype as returned by NautilusFileInfo.
 *
 * This is required as some particular items are only known by Nautilus
 * (e.g. computer), and standard GLib functions are not able to retrieve
 * their mimetype.
 *
 * Exported as GetSelectedPaths method on Tracker.Properties1 interface.
 */
static gchar **
get_selected_paths( FMATrackerPlugin *tracker )
{
	static const gchar *thisfn = "fma_tracker_plugin_get_selected_paths";
	FMATrackerPluginPrivate *priv;
	gchar **paths;
	GList *it;
	int count;
	gchar **iter;

	paths = NULL;
	priv = tracker->private;

	g_debug( "%s: tracker=%p", thisfn, ( void * ) tracker );

	count = 2 * g_list_length( priv->selected );
	paths = ( char ** ) g_new0( gchar *, 1+count );
	iter = paths;

	for( it = priv->selected ; it ; it = it->next ){
		*iter = file_manager_file_info_get_uri(( FileManagerFileInfo * ) it->data );
		iter++;
		*iter = file_manager_file_info_get_mime_type(( FileManagerFileInfo * ) it->data );
		iter++;
	}

	return( paths );
}

static GList *
free_selected( GList *selected )
{
	file_manager_file_info_list_free( selected );

	return( NULL );
}
