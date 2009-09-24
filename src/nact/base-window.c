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

#include <glib.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include <string.h>

#include "base-application.h"
#include "base-iprefs.h"
#include "base-window.h"

/* private class data
 */
struct BaseWindowClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct BaseWindowPrivate {
	gboolean         dispose_has_run;
	BaseWindow      *parent;
	BaseApplication *application;
	gchar           *toplevel_name;
	GtkWindow       *toplevel_window;
	gboolean         initialized;
	GSList          *signals;
	gboolean         save_window_position;
};

/* connected signal, to be disconnected at NactWindow dispose
 */
typedef struct {
	gpointer instance;
	gulong   handler_id;
}
	BaseWindowRecordedSignal;

/* instance properties
 */
enum {
	BASE_WINDOW_PROP_PARENT_ID = 1,
	BASE_WINDOW_PROP_APPLICATION_ID,
	BASE_WINDOW_PROP_TOPLEVEL_NAME_ID,
	BASE_WINDOW_PROP_TOPLEVEL_WIDGET_ID,
	BASE_WINDOW_PROP_INITIALIZED_ID,
	BASE_WINDOW_PROP_SAVE_WINDOW_POSITION_ID
};

/* signals defined in BaseWindow, to be used in all derived classes
 */
enum {
	INITIAL_LOAD,
	RUNTIME_INIT,
	ALL_WIDGETS_SHOWED,
	LAST_SIGNAL
};

static GObjectClass *st_parent_class = NULL;
static gint          st_signals[ LAST_SIGNAL ] = { 0 };
static gboolean      st_debug_signal_connect = FALSE;

static GType            register_type( void );
static void             class_init( BaseWindowClass *klass );
static void             iprefs_iface_init( BaseIPrefsInterface *iface );
static void             instance_init( GTypeInstance *instance, gpointer klass );
static void             instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void             instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void             instance_dispose( GObject *application );
static void             instance_finalize( GObject *application );

static gboolean         on_delete_event( GtkWidget *widget, GdkEvent *event, BaseWindow *window );

static void             v_initial_load_toplevel( BaseWindow *window, gpointer user_data );
static void             v_runtime_init_toplevel( BaseWindow *window, gpointer user_data );
static void             v_all_widgets_showed( BaseWindow *window, gpointer user_data );
static gboolean         v_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window );
static BaseApplication *v_get_application( BaseWindow *window );
static GtkWindow       *v_get_toplevel_window( BaseWindow *window );
static GtkWindow       *v_get_window( BaseWindow *window, const gchar *name );
static GtkWidget       *v_get_widget( BaseWindow *window, const gchar *name );
static gchar           *v_get_iprefs_window_id( BaseWindow *window );

static void             on_runtime_init_toplevel( BaseWindow *window, gpointer user_data );

static void             window_do_initial_load_toplevel( BaseWindow *window, gpointer user_data );
static void             window_do_runtime_init_toplevel( BaseWindow *window, gpointer user_data );
static void             window_do_all_widgets_showed( BaseWindow *window, gpointer user_data );
static gboolean         window_do_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window );
static gboolean         window_do_delete_event( BaseWindow *window, GtkWindow *toplevel, GdkEvent *event );
static BaseApplication *window_do_get_application( BaseWindow *window );
static gchar           *window_do_get_toplevel_name( BaseWindow *window );
static GtkWindow       *window_do_get_toplevel_window( BaseWindow *window );
static GtkWindow       *window_do_get_window( BaseWindow *window, const gchar *name );
static GtkWidget       *window_do_get_widget( BaseWindow *window, const gchar *name );

static gboolean         is_main_window( BaseWindow *window );
static gboolean         is_toplevel_initialized( BaseWindow *window, GtkWindow *toplevel );
static void             set_toplevel_initialized( BaseWindow *window, GtkWindow *toplevel, gboolean init );

GType
base_window_get_type( void )
{
	static GType window_type = 0;

	if( !window_type ){
		window_type = register_type();
	}

	return( window_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "base_window_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( BaseWindowClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( BaseWindow ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	static const GInterfaceInfo prefs_iface_info = {
		( GInterfaceInitFunc ) iprefs_iface_init,
		NULL,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "BaseWindow", &info, 0 );

	g_type_add_interface_static( type, BASE_IPREFS_TYPE, &prefs_iface_info );

	return( type );
}

static void
class_init( BaseWindowClass *klass )
{
	static const gchar *thisfn = "base_window_class_init";
	GObjectClass *object_class;
	GParamSpec *spec;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
	object_class->get_property = instance_get_property;
	object_class->set_property = instance_set_property;

	spec = g_param_spec_pointer(
			BASE_WINDOW_PROP_PARENT,
			"BaseWindow parent pointer",
			"A pointer (not a reference) to the BaseWindow parent of this BaseWindow",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, BASE_WINDOW_PROP_PARENT_ID, spec );

	spec = g_param_spec_pointer(
			BASE_WINDOW_PROP_APPLICATION,
			"BaseApplication pointer",
			"A pointer (not a reference) to the BaseApplication object",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, BASE_WINDOW_PROP_APPLICATION_ID, spec );

	spec = g_param_spec_string(
			BASE_WINDOW_PROP_TOPLEVEL_NAME,
			"Internal toplevel name",
			"The internal name in GtkBuilder of the toplevel window", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, BASE_WINDOW_PROP_TOPLEVEL_NAME_ID, spec );

	spec = g_param_spec_pointer(
			BASE_WINDOW_PROP_TOPLEVEL_WIDGET,
			"Main GtkWindow pointer",
			"A pointer to the main GtkWindow toplevel managed by this BaseWindow instance",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, BASE_WINDOW_PROP_TOPLEVEL_WIDGET_ID, spec );

	spec = g_param_spec_boolean(
			BASE_WINDOW_PROP_INITIALIZED,
			"Has base_window_init be run",
			"Has base_window_init be run", FALSE,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, BASE_WINDOW_PROP_INITIALIZED_ID, spec );

	spec = g_param_spec_boolean(
			BASE_WINDOW_PROP_SAVE_WINDOW_POSITION,
			"Save window size and position on dispose",
			"Whether the size and position of the window must be saved as a GConf preference", FALSE,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, BASE_WINDOW_PROP_SAVE_WINDOW_POSITION_ID, spec );

	klass->private = g_new0( BaseWindowClassPrivate, 1 );

	klass->initial_load_toplevel = window_do_initial_load_toplevel;
	klass->runtime_init_toplevel = window_do_runtime_init_toplevel;
	klass->all_widgets_showed = window_do_all_widgets_showed;
	klass->dialog_response = window_do_dialog_response;
	klass->delete_event = window_do_delete_event;
	klass->get_application = window_do_get_application;
	klass->get_toplevel_name = NULL;
	klass->get_toplevel_window = window_do_get_toplevel_window;
	klass->get_window = window_do_get_window;
	klass->get_widget = window_do_get_widget;
	klass->get_iprefs_window_id = NULL;

	/**
	 * nact-signal-base-window-initial-load:
	 *
	 * The signal is emitted by the #BaseWindow instance when it loads
	 * the toplevel widget for the first time from the GtkBuilder.
	 */
	st_signals[ INITIAL_LOAD ] =
		g_signal_new_class_handler(
				BASE_WINDOW_SIGNAL_INITIAL_LOAD,
				G_TYPE_FROM_CLASS( klass ),
				G_SIGNAL_RUN_LAST,
				G_CALLBACK( v_initial_load_toplevel ),
				NULL,
				NULL,
				g_cclosure_marshal_VOID__POINTER,
				G_TYPE_NONE,
				1,
				G_TYPE_POINTER );

	/**
	 * nact-signal-base-window-runtime-init:
	 *
	 * The signal is emitted by the #BaseWindow instance when it is
	 * about to display the toplevel widget. Is is so time to initialize
	 * it with runtime values.
	 */
	st_signals[ RUNTIME_INIT ] =
		g_signal_new_class_handler(
				BASE_WINDOW_SIGNAL_RUNTIME_INIT,
				G_TYPE_FROM_CLASS( klass ),
				G_SIGNAL_RUN_LAST,
				G_CALLBACK( v_runtime_init_toplevel ),
				NULL,
				NULL,
				g_cclosure_marshal_VOID__POINTER,
				G_TYPE_NONE,
				1,
				G_TYPE_POINTER );

	/**
	 * nact-signal-base-window-all-widgets-showed:
	 *
	 * The signal is emitted by the #BaseWindow instance when the
	 * toplevel widget has been initialized with its runtime values,
	 * just after showing it and all its descendants.
	 *
	 * It is typically used by notebooks, to select the first visible
	 * page.
	 */
	st_signals[ ALL_WIDGETS_SHOWED ] =
		g_signal_new_class_handler(
				BASE_WINDOW_SIGNAL_ALL_WIDGETS_SHOWED,
				G_TYPE_FROM_CLASS( klass ),
				G_SIGNAL_RUN_LAST,
				G_CALLBACK( v_all_widgets_showed ),
				NULL,
				NULL,
				g_cclosure_marshal_VOID__POINTER,
				G_TYPE_NONE,
				1,
				G_TYPE_POINTER );
}

static void
iprefs_iface_init( BaseIPrefsInterface *iface )
{
	static const gchar *thisfn = "base_window_iprefs_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->iprefs_get_window_id = v_get_iprefs_window_id;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "base_window_instance_init";
	BaseWindow *self;

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );
	g_return_if_fail( BASE_IS_WINDOW( instance ));
	self = BASE_WINDOW( instance );

	self->private = g_new0( BaseWindowPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->signals = NULL;
	self->private->save_window_position = TRUE;

	base_window_signal_connect(
			self,
			G_OBJECT( instance ),
			BASE_WINDOW_SIGNAL_RUNTIME_INIT,
			G_CALLBACK( on_runtime_init_toplevel ));
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	BaseWindow *self;

	g_return_if_fail( BASE_IS_WINDOW( object ));
	self = BASE_WINDOW( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case BASE_WINDOW_PROP_PARENT_ID:
				g_value_set_pointer( value, self->private->parent );
				break;

			case BASE_WINDOW_PROP_APPLICATION_ID:
				g_value_set_pointer( value, self->private->application );
				break;

			case BASE_WINDOW_PROP_TOPLEVEL_NAME_ID:
				g_value_set_string( value, self->private->toplevel_name );
				break;

			case BASE_WINDOW_PROP_TOPLEVEL_WIDGET_ID:
				g_value_set_pointer( value, self->private->toplevel_window );
				break;

			case BASE_WINDOW_PROP_INITIALIZED_ID:
				g_value_set_boolean( value, self->private->initialized );
				break;

			case BASE_WINDOW_PROP_SAVE_WINDOW_POSITION_ID:
				g_value_set_boolean( value, self->private->save_window_position );
				break;

			default:
				G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
				break;
		}
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	BaseWindow *self;

	g_return_if_fail( BASE_IS_WINDOW( object ));
	self = BASE_WINDOW( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case BASE_WINDOW_PROP_PARENT_ID:
				self->private->parent = g_value_get_pointer( value );
				break;

			case BASE_WINDOW_PROP_APPLICATION_ID:
				self->private->application = g_value_get_pointer( value );
				break;

			case BASE_WINDOW_PROP_TOPLEVEL_NAME_ID:
				g_free( self->private->toplevel_name );
				self->private->toplevel_name = g_value_dup_string( value );
				break;

			case BASE_WINDOW_PROP_TOPLEVEL_WIDGET_ID:
				self->private->toplevel_window = g_value_get_pointer( value );
				break;

			case BASE_WINDOW_PROP_INITIALIZED_ID:
				self->private->initialized = g_value_get_boolean( value );
				break;

			case BASE_WINDOW_PROP_SAVE_WINDOW_POSITION_ID:
				self->private->save_window_position = g_value_get_boolean( value );
				break;

			default:
				G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
				break;
		}
	}
}

static void
instance_dispose( GObject *window )
{
	static const gchar *thisfn = "base_window_instance_dispose";
	BaseWindow *self;
	GSList *is;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );
	g_return_if_fail( BASE_IS_WINDOW( window ));
	self = BASE_WINDOW( window );

	if( !self->private->dispose_has_run ){

		if( self->private->save_window_position ){
			base_iprefs_save_window_position( self );
		}

		/* signals must be deconnected before quitting main loop
		 */
		for( is = self->private->signals ; is ; is = is->next ){
			BaseWindowRecordedSignal *str = ( BaseWindowRecordedSignal * ) is->data;
			if( g_signal_handler_is_connected( str->instance, str->handler_id )){
				g_signal_handler_disconnect( str->instance, str->handler_id );
				if( st_debug_signal_connect ){
					g_debug( "%s: disconnecting signal handler %p:%lu", thisfn, str->instance, str->handler_id );
				}
			}
			g_free( str );
		}
		g_slist_free( self->private->signals );

		if( is_main_window( BASE_WINDOW( window ))){
			g_debug( "%s: quitting main window", thisfn );
			gtk_main_quit ();
			gtk_widget_destroy( GTK_WIDGET( self->private->toplevel_window ));

		} else if( GTK_IS_ASSISTANT( self->private->toplevel_window )){
			g_debug( "%s: quitting assistant", thisfn );
			gtk_main_quit();
			gtk_widget_hide_all( GTK_WIDGET( self->private->toplevel_window ));

		} else {
			g_debug( "%s: quitting dialog", thisfn );
			gtk_widget_hide_all( GTK_WIDGET( self->private->toplevel_window ));
		}

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( window );
		}
	}
}

static void
instance_finalize( GObject *window )
{
	static const gchar *thisfn = "base_window_instance_finalize";
	BaseWindow *self;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );
	g_return_if_fail( BASE_IS_WINDOW( window ));
	self = BASE_WINDOW( window );

	g_free( self->private->toplevel_name );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( window );
	}
}

/**
 * base_window_init:
 * @window: this #BaseWindow object.
 *
 * Initializes the window.
 *
 * This is a one-time initialization just after the BaseWindow has been
 * allocated. This should leave the BaseWindow object with a valid
 * toplevel GtkWindow dialog. This is also time to make one-time
 * initialization on this toplevel dialog.
 *
 * For an every-time initialization, see base_window_run().
 *
 * Note that the BaseWindow itself should be initialized each time
 * the user opens the dialog, though the GtkWindow itself needs only
 * be initialized the first time it is loaded.
 *
 * Returns: %TRUE if the window has been successfully initialized,
 * %FALSE else.
 */
gboolean
base_window_init( BaseWindow *window )
{
	static const gchar *thisfn = "base_window_init";
	gboolean initialized = FALSE;
	gchar *dialog_name;
	GtkWindow *toplevel;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), FALSE );

	if( !window->private->dispose_has_run &&
		!window->private->initialized ){

		g_debug( "%s: window=%p", thisfn, ( void * ) window );

		if( !window->private->application ){
			g_return_val_if_fail( window->private->parent, FALSE );
			g_return_val_if_fail( BASE_IS_WINDOW( window->private->parent ), FALSE );
			window->private->application = BASE_APPLICATION( base_window_get_application( window->private->parent ));
			g_debug( "%s: application=%p", thisfn, ( void * ) window->private->application );
		}

		g_assert( window->private->application );
		g_assert( BASE_IS_APPLICATION( window->private->application ));

		dialog_name = window_do_get_toplevel_name( window );
		g_assert( dialog_name && strlen( dialog_name ));

		toplevel = base_window_get_toplevel( window, dialog_name );
		window->private->toplevel_window = toplevel;

		if( toplevel ){
			g_assert( GTK_IS_WINDOW( toplevel ));

			if( !is_toplevel_initialized( window, toplevel )){

				g_signal_emit_by_name( window, BASE_WINDOW_SIGNAL_INITIAL_LOAD, NULL );

				set_toplevel_initialized( window, toplevel, TRUE );
			}
			window->private->initialized = TRUE;
			initialized = TRUE;
		}

		g_free( dialog_name );
	}

	return( initialized );
}

/**
 * base_window_run:
 * @window: this #BaseWindow object.
 *
 * Runs the window.
 */
void
base_window_run( BaseWindow *window )
{
	static const gchar *thisfn = "base_window_run";
	GtkWidget *this_dialog;
	gint code;

	g_return_if_fail( BASE_IS_WINDOW( window ));

	if( !window->private->dispose_has_run ){

		if( !window->private->initialized ){
			base_window_init( window );
		}

		g_debug( "%s: window=%p", thisfn, ( void * ) window );

		g_signal_emit_by_name( window, BASE_WINDOW_SIGNAL_RUNTIME_INIT, NULL );

		this_dialog = GTK_WIDGET( window->private->toplevel_window );

		gtk_widget_show_all( this_dialog );

		g_signal_emit_by_name( window, BASE_WINDOW_SIGNAL_ALL_WIDGETS_SHOWED, NULL );

		if( is_main_window( window )){

			if( GTK_IS_DIALOG( this_dialog )){
				g_signal_connect( G_OBJECT( this_dialog ), "response", G_CALLBACK( v_dialog_response ), window );
			} else {
				g_signal_connect( G_OBJECT( this_dialog ), "delete-event", G_CALLBACK( on_delete_event ), window );
			}

			g_debug( "%s: application=%p, starting gtk_main", thisfn, ( void * ) window->private->application );
			gtk_main();

		} else if( GTK_IS_ASSISTANT( this_dialog )){
			g_debug( "%s: starting gtk_main", thisfn );
			gtk_main();

		} else {
			g_assert( GTK_IS_DIALOG( this_dialog ));
			g_debug( "%s: starting gtk_dialog_run", thisfn );
			do {
				code = gtk_dialog_run( GTK_DIALOG( this_dialog ));
			}
			while( !v_dialog_response( GTK_DIALOG( this_dialog ), code, window ));
		}
	}
}

/**
 * base_window_get_application:
 * @window: this #BaseWindow object.
 *
 * Returns: a pointer on the #BaseApplication object.
 *
 * The returned pointer is owned by the primary allocator of the
 * application ; it should not be g_free() nor g_object_unref() by the
 * caller.
 */
BaseApplication *
base_window_get_application( BaseWindow *window )
{
	BaseApplication *application = NULL;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), NULL );

	if( !window->private->dispose_has_run ){
		application = v_get_application( window );
	}

	return( application );
}

/**
 * base_window_get_toplevel_window:
 * @window: this #BaseWindow instance..
 *
 * Returns the top-level GtkWindow attached to this BaseWindow object.
 *
 * The caller may close the window by g_object_unref()-ing the returned
 * #GtkWindow.
 */
GtkWindow *
base_window_get_toplevel_window( BaseWindow *window )
{
	GtkWindow *toplevel = NULL;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), NULL );

	if( !window->private->dispose_has_run ){
		toplevel = v_get_toplevel_window( window );
	}

	return( toplevel );
}

/**
 * base_window_get_toplevel:
 * @window: this #BaseWindow instance.
 * @name: the name of the searched GtkWindow.
 *
 * Returns: the named top-level GtkWindow.
 *
 * The caller may close the window by g_object_unref()-ing the returned
 * #GtkWindow.
 */
GtkWindow *
base_window_get_toplevel( BaseWindow *window, const gchar *name )
{
	GtkWindow *toplevel = NULL;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), NULL );

	if( !window->private->dispose_has_run ){
		toplevel = v_get_window( window, name );
	}

	return( toplevel );
}

/**
 * base_window_get_widget:
 * @window: this BaseWindow object.
 * @name: the name of the searched child.
 *
 * Returns: the GtkWidget which is a child of this parent.
 *
 * The returned #GtkWidget is owned by the #GtkBuilder UI. It should
 * not be g_free nor g_object_unref() by the caller.
 */
GtkWidget *
base_window_get_widget( BaseWindow *window, const gchar *name )
{
	GtkWidget *widget = NULL;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), NULL );

	if( !window->private->dispose_has_run ){
		widget = v_get_widget( window, name );
	}

	return( widget );
}

/*
 * handler of "delete-event" message
 * let a chance to derived class to handle it
 * our own function does nothing, and let the signal be propagated.
 */
static gboolean
on_delete_event( GtkWidget *toplevel, GdkEvent *event, BaseWindow *window )
{
	static const gchar *thisfn = "base_window_on_delete_event";
	static gboolean stop = FALSE;

	g_debug( "%s: toplevel=%p, event=%p, window=%p",
			thisfn, ( void * ) toplevel, ( void * ) event, ( void * ) window );
	g_return_val_if_fail( BASE_IS_WINDOW( window ), FALSE );

	if( !window->private->dispose_has_run ){

		if( BASE_WINDOW_GET_CLASS( window )->delete_event ){
			stop = BASE_WINDOW_GET_CLASS( window )->delete_event( window, GTK_WINDOW( toplevel ), event );

		} else {
			stop = window_do_delete_event( window, GTK_WINDOW( toplevel ), event );
		}
	}

	return( stop );
}

/*
 * default class handler for "nact-signal-base-window-initial-load" message
 * -> does nothing here
 */
static void
v_initial_load_toplevel( BaseWindow *window, gpointer user_data )
{
	static const gchar *thisfn = "base_window_v_initial_load_toplevel";

	g_debug( "%s: window=%p, user_data=%p", thisfn, ( void * ) window, ( void * ) user_data );
	g_return_if_fail( BASE_IS_WINDOW( window ));

	if( !window->private->dispose_has_run ){

		if( BASE_WINDOW_GET_CLASS( window )->initial_load_toplevel ){
			BASE_WINDOW_GET_CLASS( window )->initial_load_toplevel( window, user_data );

		} else {
			window_do_initial_load_toplevel( window, user_data );
		}
	}
}

/*
 * default class handler for "nact-signal-base-window-runtime-init" message
 * -> does nothing here
 */
static void
v_runtime_init_toplevel( BaseWindow *window, gpointer user_data )
{
	static const gchar *thisfn = "base_window_v_runtime_init_toplevel";

	g_debug( "%s: window=%p, user_data=%p", thisfn, ( void * ) window, ( void * ) user_data );
	g_return_if_fail( BASE_IS_WINDOW( window ));

	if( !window->private->dispose_has_run ){

		if( BASE_WINDOW_GET_CLASS( window )->runtime_init_toplevel ){
			BASE_WINDOW_GET_CLASS( window )->runtime_init_toplevel( window, user_data );

		} else {
			window_do_runtime_init_toplevel( window, user_data );
		}
	}
}

static void
v_all_widgets_showed( BaseWindow *window, gpointer user_data )
{
	static const gchar *thisfn = "base_window_v_all_widgets_showed";

	g_debug( "%s: window=%p, user_data=%p", thisfn, ( void * ) window, ( void * ) user_data );
	g_return_if_fail( BASE_IS_WINDOW( window ));

	if( !window->private->dispose_has_run ){

		if( BASE_WINDOW_GET_CLASS( window )->all_widgets_showed ){
			BASE_WINDOW_GET_CLASS( window )->all_widgets_showed( window, user_data );

		} else {
			window_do_all_widgets_showed( window, user_data );
		}
	}
}

static gboolean
v_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window )
{
	static const gchar *thisfn = "base_window_v_dialog_response";
	gboolean stop = FALSE;

	g_debug( "%s: dialog=%p, code=%d, window=%p", thisfn, ( void * ) dialog, code, ( void * ) window );
	g_return_val_if_fail( BASE_IS_WINDOW( window ), FALSE );

	if( !window->private->dispose_has_run ){

		if( BASE_WINDOW_GET_CLASS( window )->dialog_response ){
			stop = BASE_WINDOW_GET_CLASS( window )->dialog_response( dialog, code, window );

		} else {
			stop = window_do_dialog_response( dialog, code, window );
		}
	}

	return( stop );
}

static BaseApplication *
v_get_application( BaseWindow *window )
{
	BaseApplication *application = NULL;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), NULL );

	if( !window->private->dispose_has_run ){

		if( BASE_WINDOW_GET_CLASS( window )->get_application ){
			application = BASE_WINDOW_GET_CLASS( window )->get_application( window );

		} else {
			application = window_do_get_application( window );
		}
	}

	return( application );
}

static GtkWindow *
v_get_toplevel_window( BaseWindow *window )
{
	g_assert( BASE_IS_WINDOW( window ));

	if( BASE_WINDOW_GET_CLASS( window )->get_toplevel_window ){
		return( BASE_WINDOW_GET_CLASS( window )->get_toplevel_window( window ));
	}

	return( window_do_get_toplevel_window( window ));
}

static GtkWindow *
v_get_window( BaseWindow *window, const gchar *name )
{
	GtkWindow *toplevel = NULL;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), NULL );

	if( !window->private->dispose_has_run ){

		if( BASE_WINDOW_GET_CLASS( window )->get_window ){
			toplevel = BASE_WINDOW_GET_CLASS( window )->get_window( window, name );

		} else {
			toplevel = window_do_get_window( window, name );
		}
	}

	return( toplevel );
}

static GtkWidget *
v_get_widget( BaseWindow *window, const gchar *name )
{
	GtkWidget *widget = NULL;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), NULL );

	if( !window->private->dispose_has_run ){

		if( BASE_WINDOW_GET_CLASS( window )->get_widget ){
			widget = BASE_WINDOW_GET_CLASS( window )->get_widget( window, name );

		} else {
			widget = window_do_get_widget( window, name );
		}
	}

	return( widget );
}

static gchar *
v_get_iprefs_window_id( BaseWindow *window )
{
	gchar *id = NULL;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), NULL );

	if( !window->private->dispose_has_run ){

		if( BASE_WINDOW_GET_CLASS( window )->get_iprefs_window_id ){
			id = BASE_WINDOW_GET_CLASS( window )->get_iprefs_window_id( window );
		}
	}

	return( id );
}

static void
on_runtime_init_toplevel( BaseWindow *window, gpointer user_data )
{
	static const gchar *thisfn = "base_window_on_runtime_init_toplevel";
	GtkWindow *parent_toplevel;

	g_debug( "%s: window=%p, user_data=%p, parent_window=%p",
			thisfn, ( void * ) window, ( void * ) user_data, ( void * ) window->private->parent );
	g_return_if_fail( BASE_IS_WINDOW( window ));

	if( !window->private->dispose_has_run ){

		if( window->private->parent ){
			g_assert( BASE_IS_WINDOW( window->private->parent ));
			parent_toplevel = base_window_get_toplevel_window( BASE_WINDOW( window->private->parent ));
			gtk_window_set_transient_for( window->private->toplevel_window, parent_toplevel );
		}

		base_iprefs_position_window( window );
	}
}

static void
window_do_initial_load_toplevel( BaseWindow *window, gpointer user_data )
{
	static const gchar *thisfn = "base_window_do_initial_load_toplevel";

	g_debug( "%s: window=%p, user_data=%p", thisfn, ( void * ) window, ( void * ) user_data );
	g_return_if_fail( BASE_IS_WINDOW( window ));

	if( !window->private->dispose_has_run ){
		/* nothing to do here */
	}
}

static void
window_do_runtime_init_toplevel( BaseWindow *window, gpointer user_data )
{
	static const gchar *thisfn = "base_window_do_runtime_init_toplevel";

	g_debug( "%s: window=%p, user_data=%p", thisfn, ( void * ) window, ( void * ) user_data );
	g_return_if_fail( BASE_IS_WINDOW( window ));

	if( !window->private->dispose_has_run ){
		/* nothing to do here */
	}
}

static void
window_do_all_widgets_showed( BaseWindow *window, gpointer user_data )
{
	static const gchar *thisfn = "base_window_do_all_widgets_showed";

	g_debug( "%s: window=%p, user_data=%p", thisfn, ( void * ) window, ( void * ) user_data );
	g_return_if_fail( BASE_IS_WINDOW( window ));

	if( !window->private->dispose_has_run ){
		/* nothing to do here */
	}
}

/*
 * return TRUE to quit the dialog loop
 */
static gboolean
window_do_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window )
{
	static const gchar *thisfn = "base_window_do_dialog_response";

	g_debug( "%s: dialog=%p, code=%d, window=%p", thisfn, ( void * ) dialog, code, ( void * ) window );
	g_return_val_if_fail( GTK_IS_DIALOG( dialog ), FALSE );
	g_return_val_if_fail( BASE_IS_WINDOW( window ), FALSE );

	return( TRUE );
}

/*
 * return TRUE to stop other handlers from being invoked for the event.
 * note that default handler will destroy this window
 */
static gboolean
window_do_delete_event( BaseWindow *window, GtkWindow *toplevel, GdkEvent *event )
{
	static const gchar *thisfn = "base_window_do_delete_event";

	g_debug( "%s: window=%p, toplevel=%p, event=%p",
			thisfn, ( void * ) window, ( void * ) toplevel, ( void * ) event );
	g_return_val_if_fail( BASE_IS_WINDOW( window ), FALSE );
	g_return_val_if_fail( GTK_IS_WINDOW( toplevel ), FALSE );

	return( FALSE );
}

static BaseApplication *
window_do_get_application( BaseWindow *window )
{
	BaseApplication *application = NULL;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), NULL );

	if( !window->private->dispose_has_run ){
		application = window->private->application;
	}

	return( application );
}

static gchar *
window_do_get_toplevel_name( BaseWindow *window )
{
	gchar *name = NULL;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), NULL );

	if( !window->private->dispose_has_run ){

		g_object_get( G_OBJECT( window ), BASE_WINDOW_PROP_TOPLEVEL_NAME, &name, NULL );

		if( !name || !strlen( name )){
			name = BASE_WINDOW_GET_CLASS( window )->get_toplevel_name( window );
			if( name && strlen( name )){
				g_object_set( G_OBJECT( window ), BASE_WINDOW_PROP_TOPLEVEL_NAME, name, NULL );
			}
		}
	}

	return( name );
}

static GtkWindow *
window_do_get_toplevel_window( BaseWindow *window )
{
	GtkWindow *toplevel = NULL;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), NULL );

	if( !window->private->dispose_has_run ){
		toplevel = window->private->toplevel_window;
	}

	return( toplevel );
}

static GtkWindow *
window_do_get_window( BaseWindow *window, const gchar *name )
{
	GtkWindow *toplevel = NULL;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), NULL );

	if( !window->private->dispose_has_run ){
		toplevel = base_application_get_toplevel( window->private->application, name );
	}

	return( toplevel );
}

static GtkWidget *
window_do_get_widget( BaseWindow *window, const gchar *name )
{
	GtkWidget *widget = NULL;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), NULL );

	if( !window->private->dispose_has_run ){
		widget = base_application_get_widget( window->private->application, window, name );
	}

	return( widget );
}

static gboolean
is_main_window( BaseWindow *window )
{
	gboolean is_main = FALSE;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), FALSE );

	if( !window->private->dispose_has_run ){

		BaseApplication *appli = window->private->application;

		BaseWindow *main_window = BASE_WINDOW( base_application_get_main_window( appli ));

		GtkWidget *main_dialog = GTK_WIDGET( base_window_get_toplevel_window( main_window ));

		GtkWidget *this_dialog = GTK_WIDGET( window->private->toplevel_window );

		is_main = ( main_dialog == this_dialog );
	}

	return( is_main );
}

static gboolean
is_toplevel_initialized( BaseWindow *window, GtkWindow *toplevel )
{
	gboolean initialized;

	initialized = GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( toplevel ), "base-window-toplevel-initialized" ));

	return( initialized );
}

static void
set_toplevel_initialized( BaseWindow *window, GtkWindow *toplevel, gboolean initialized )
{
	g_object_set_data( G_OBJECT( toplevel ), "base-window-toplevel-initialized", GUINT_TO_POINTER( initialized ));
}

void
base_window_error_dlg( BaseWindow *window, GtkMessageType type, const gchar *primary, const gchar *secondary )
{
	g_return_if_fail( BASE_IS_WINDOW( window ));

	if( !window->private->dispose_has_run ){
		base_application_error_dlg( window->private->application, type, primary, secondary );
	}
}

gboolean
base_window_yesno_dlg( BaseWindow *window, GtkMessageType type, const gchar *first, const gchar *second )
{
	gboolean yesno = FALSE;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), FALSE );

	if( !window->private->dispose_has_run ){
		yesno = base_application_yesno_dlg( window->private->application, type, first, second );
	}

	return( yesno );
}

/**
 * Records a connected signal, to be disconnected at NactWindow dispose.
 */
void
base_window_signal_connect( BaseWindow *window, GObject *instance, const gchar *signal, GCallback fn )
{
	static const gchar *thisfn = "base_window_signal_connect";

	g_return_if_fail( BASE_IS_WINDOW( window ));

	if( !window->private->dispose_has_run ){

		gulong handler_id = g_signal_connect( instance, signal, fn, window );

		BaseWindowRecordedSignal *str = g_new0( BaseWindowRecordedSignal, 1 );
		str->instance = instance;
		str->handler_id = handler_id;
		window->private->signals = g_slist_prepend( window->private->signals, str );

		if( st_debug_signal_connect ){
			g_debug( "%s: connecting signal handler %p:%lu", thisfn, ( void * ) instance, handler_id );
		}
	}
}

void
base_window_signal_connect_by_name( BaseWindow *window, const gchar *name, const gchar *signal, GCallback fn )
{
	g_return_if_fail( BASE_IS_WINDOW( window ));

	if( !window->private->dispose_has_run ){

		GtkWidget *widget = base_window_get_widget( window, name );
		if( GTK_IS_WIDGET( widget )){

			base_window_signal_connect( window, G_OBJECT( widget ), signal, fn );
		}
	}
}
