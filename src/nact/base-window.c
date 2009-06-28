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
#include "base-window.h"

/* private class data
 */
struct BaseWindowClassPrivate {
};

/* private instance data
 */
struct BaseWindowPrivate {
	gboolean         dispose_has_run;
	BaseApplication *application;
	gchar           *toplevel_name;
	GtkWindow       *toplevel_widget;
	gboolean         initialized;
};

/* instance properties
 */
enum {
	PROP_WINDOW_APPLICATION = 1,
	PROP_WINDOW_TOPLEVEL_NAME,
	PROP_WINDOW_TOPLEVEL_WIDGET,
	PROP_WINDOW_INITIALIZED
};

static GObjectClass *st_parent_class = NULL;

static GType       register_type( void );
static void        class_init( BaseWindowClass *klass );
static void        instance_init( GTypeInstance *instance, gpointer klass );
static void        instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void        instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void        instance_dispose( GObject *application );
static void        instance_finalize( GObject *application );

static gchar      *v_get_toplevel_name( BaseWindow *window );
static void        v_initial_load_toplevel( BaseWindow *window );
static void        v_runtime_init_toplevel( BaseWindow *window );
static void        v_all_widgets_showed( BaseWindow *window );
static void        v_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window );

static void        do_init_window( BaseWindow *window );
static void        do_initial_load_toplevel( BaseWindow *window );
static void        do_runtime_init_toplevel( BaseWindow *window );
static void        do_all_widgets_showed( BaseWindow *window );
static void        do_run_window( BaseWindow *window );
static void        do_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window );
static GObject    *do_get_application( BaseWindow *window );
static GtkWindow  *do_get_toplevel_widget( BaseWindow *window );
static GtkWidget  *do_get_widget( BaseWindow *window, const gchar *name );

static gboolean    is_toplevel_initialized( BaseWindow *window );
static void        set_toplevel_initialized( BaseWindow *window );
static gboolean    is_main_window( BaseWindow *window );

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
	g_debug( "%s", thisfn );

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

	return( g_type_register_static( G_TYPE_OBJECT, "BaseWindow", &info, 0 ));
}

static void
class_init( BaseWindowClass *klass )
{
	static const gchar *thisfn = "base_window_class_init";
	g_debug( "%s: klass=%p", thisfn, klass );

	st_parent_class = g_type_class_peek_parent( klass );

	GObjectClass *object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
	object_class->get_property = instance_get_property;
	object_class->set_property = instance_set_property;

	GParamSpec *spec;
	spec = g_param_spec_pointer(
			PROP_WINDOW_APPLICATION_STR,
			PROP_WINDOW_APPLICATION_STR,
			"BaseApplication object pointer",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_WINDOW_APPLICATION, spec );

	spec = g_param_spec_string(
			PROP_WINDOW_TOPLEVEL_NAME_STR,
			PROP_WINDOW_TOPLEVEL_NAME_STR,
			"The internal name of the toplevel window", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_WINDOW_TOPLEVEL_NAME, spec );

	spec = g_param_spec_pointer(
			PROP_WINDOW_TOPLEVEL_WIDGET_STR,
			PROP_WINDOW_TOPLEVEL_WIDGET_STR,
			"The main GtkWindow attached to this object",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_WINDOW_TOPLEVEL_WIDGET, spec );

	spec = g_param_spec_boolean(
			PROP_WINDOW_INITIALIZED_STR,
			PROP_WINDOW_INITIALIZED_STR,
			"Has base_window_init be ran", FALSE,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_WINDOW_INITIALIZED, spec );

	klass->private = g_new0( BaseWindowClassPrivate, 1 );

	klass->init = do_init_window;
	klass->run = do_run_window;
	klass->initial_load_toplevel = do_initial_load_toplevel;
	klass->runtime_init_toplevel = do_runtime_init_toplevel;
	klass->all_widgets_showed = do_all_widgets_showed;
	klass->dialog_response = do_dialog_response;
	klass->get_application = do_get_application;
	klass->get_toplevel_name = NULL;
	klass->get_toplevel_widget = do_get_toplevel_widget;
	klass->get_widget = do_get_widget;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "base_window_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );

	g_assert( BASE_IS_WINDOW( instance ));
	BaseWindow *self = BASE_WINDOW( instance );

	self->private = g_new0( BaseWindowPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	g_assert( BASE_IS_WINDOW( object ));
	BaseWindow *self = BASE_WINDOW( object );

	switch( property_id ){
		case PROP_WINDOW_APPLICATION:
			g_value_set_pointer( value, self->private->application );
			break;

		case PROP_WINDOW_TOPLEVEL_NAME:
			g_value_set_string( value, self->private->toplevel_name );
			break;

		case PROP_WINDOW_TOPLEVEL_WIDGET:
			g_value_set_pointer( value, self->private->toplevel_widget );
			break;

		case PROP_WINDOW_INITIALIZED:
			g_value_set_boolean( value, self->private->initialized );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	g_assert( BASE_IS_WINDOW( object ));
	BaseWindow *self = BASE_WINDOW( object );

	switch( property_id ){
		case PROP_WINDOW_APPLICATION:
			self->private->application = g_value_get_pointer( value );
			break;

		case PROP_WINDOW_TOPLEVEL_NAME:
			g_free( self->private->toplevel_name );
			self->private->toplevel_name = g_value_dup_string( value );
			break;

		case PROP_WINDOW_TOPLEVEL_WIDGET:
			self->private->toplevel_widget = g_value_get_pointer( value );
			break;

		case PROP_WINDOW_INITIALIZED:
			self->private->initialized = g_value_get_boolean( value );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_dispose( GObject *window )
{
	static const gchar *thisfn = "base_window_instance_dispose";
	g_debug( "%s: window=%p", thisfn, window );

	g_assert( BASE_IS_WINDOW( window ));
	BaseWindow *self = BASE_WINDOW( window );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		g_debug( "%s: gtk_main_level=%d", thisfn, gtk_main_level());
		if( is_main_window( BASE_WINDOW( window ))){
			gtk_main_quit ();
			gtk_widget_destroy( GTK_WIDGET( self->private->toplevel_widget ));

		} else {
			gtk_widget_hide_all( GTK_WIDGET( self->private->toplevel_widget ));
		}


		/* chain up to the parent class */
		G_OBJECT_CLASS( st_parent_class )->dispose( window );
	}
}

static void
instance_finalize( GObject *window )
{
	static const gchar *thisfn = "base_window_instance_finalize";
	g_debug( "%s: window=%p", thisfn, window );

	g_assert( BASE_IS_WINDOW( window ));
	BaseWindow *self = ( BaseWindow * ) window;

	g_free( self->private->toplevel_name );

	g_free( self->private );

	/* chain call to parent class */
	if( st_parent_class->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( window );
	}
}

/**
 * Initializes the window.
 *
 * @window: this BaseWindow object.
 *
 * This is a one-time initialization just after the BaseWindow has been
 * allocated. For an every-time initialization, see base_window_run.
 */
void
base_window_init( BaseWindow *window )
{
	g_assert( BASE_IS_WINDOW( window ));

	if( BASE_WINDOW_GET_CLASS( window )->init ){
		BASE_WINDOW_GET_CLASS( window )->init( window );
	}
}

/**
 * Run the window.
 *
 * @window: this BaseWindow object.
 */
void
base_window_run( BaseWindow *window )
{
	g_assert( BASE_IS_WINDOW( window ));
	if( BASE_WINDOW_GET_CLASS( window )->run ){
		BASE_WINDOW_GET_CLASS( window )->run( window );
	}
}

/**
 * Returns a pointer on the BaseApplication object.
 *
 * @window: this BaseWindow object.
 */
GObject *
base_window_get_application( BaseWindow *window )
{
	g_assert( BASE_IS_WINDOW( window ));
	return( BASE_WINDOW_GET_CLASS( window )->get_application( window ));
}

/**
 * Returns the top-level GtkWindow attached to this BaseWindow object.
 *
 * @window: this BaseWindow object.
 */
GtkWindow *
base_window_get_toplevel_widget( BaseWindow *window )
{
	g_assert( BASE_IS_WINDOW( window ));
	return( BASE_WINDOW_GET_CLASS( window )->get_toplevel_widget( window ));
}

/**
 * Returns the GtkWidget which is a child of this parent.
 *
 * @window: this BaseWindow object.
 *
 * @name: the name of the searched child.
 */
GtkWidget *
base_window_get_widget( BaseWindow *window, const gchar *name )
{
	g_assert( BASE_IS_WINDOW( window ));
	return( BASE_WINDOW_GET_CLASS( window )->get_widget( window, name ));
}

/**
 * Connects a signal to a handler, assuring that the BaseWindow pointer
 * is passed as user data.
 */
void
base_window_connect( BaseWindow *window, const gchar *widget, const gchar *signal, GCallback handler )
{
	GtkWidget *target = base_window_get_widget( window, widget );
	g_signal_connect( G_OBJECT( target ), signal, handler, window );
}

static gchar *
v_get_toplevel_name( BaseWindow *window )
{
	g_assert( BASE_IS_WINDOW( window ));

	gchar *name;
	g_object_get( G_OBJECT( window ), PROP_WINDOW_TOPLEVEL_NAME_STR, &name, NULL );

	if( !name || !strlen( name )){
		name = BASE_WINDOW_GET_CLASS( window )->get_toplevel_name( window );
		if( name && strlen( name )){
			g_object_set( G_OBJECT( window ), PROP_WINDOW_TOPLEVEL_NAME_STR, name, NULL );
		}
	}

	return( name );
}

static void
v_initial_load_toplevel( BaseWindow *window )
{
	g_assert( BASE_IS_WINDOW( window ));

	GtkWindow *toplevel = window->private->toplevel_widget;
	g_assert( toplevel );
	g_assert( GTK_IS_WINDOW( toplevel ));

	if( window->private->toplevel_widget ){
		if( BASE_WINDOW_GET_CLASS( window )->initial_load_toplevel ){
			BASE_WINDOW_GET_CLASS( window )->initial_load_toplevel( window );
		}
	}
}

static void
v_runtime_init_toplevel( BaseWindow *window )
{
	g_assert( BASE_IS_WINDOW( window ));

	if( window->private->toplevel_widget ){
		if( BASE_WINDOW_GET_CLASS( window )->runtime_init_toplevel ){
			BASE_WINDOW_GET_CLASS( window )->runtime_init_toplevel( window );
		}
	}
}

static void
v_all_widgets_showed( BaseWindow *window )
{
	g_assert( BASE_IS_WINDOW( window ));

	if( window->private->toplevel_widget ){
		if( BASE_WINDOW_GET_CLASS( window )->all_widgets_showed ){
			BASE_WINDOW_GET_CLASS( window )->all_widgets_showed( window );
		}
	}
}

static void
v_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window )
{
	g_assert( BASE_IS_WINDOW( window ));

	if( BASE_WINDOW_GET_CLASS( window )->dialog_response ){
		BASE_WINDOW_GET_CLASS( window )->dialog_response( dialog, code, window );
	}
}

static void
do_init_window( BaseWindow *window )
{
	static const gchar *thisfn = "base_window_do_init_window";
	g_debug( "%s: window=%p", thisfn, window );

	g_assert( BASE_IS_WINDOW( window ));

	gchar *widget_name = v_get_toplevel_name( window );
	g_assert( widget_name && strlen( widget_name ));

	GtkWidget *toplevel = base_window_get_widget( window, widget_name );
	window->private->toplevel_widget = GTK_WINDOW( toplevel );

	if( toplevel ){
		g_assert( GTK_IS_WINDOW( toplevel ));

		if( !is_toplevel_initialized( window )){
			v_initial_load_toplevel( window );
			set_toplevel_initialized( window );
		}

		v_runtime_init_toplevel( window );
	}

	g_free( widget_name );
	window->private->initialized = TRUE;
}

static void
do_initial_load_toplevel( BaseWindow *window )
{
	static const gchar *thisfn = "base_window_do_initial_load_toplevel";
	g_debug( "%s: window=%p", thisfn, window );
}

static void
do_runtime_init_toplevel( BaseWindow *window )
{
	static const gchar *thisfn = "base_window_do_runtime_init_toplevel";
	g_debug( "%s: window=%p", thisfn, window );
}

static void
do_all_widgets_showed( BaseWindow *window )
{
	static const gchar *thisfn = "base_window_do_all_widgets_showed";
	g_debug( "%s: window=%p", thisfn, window );
}

static void
do_run_window( BaseWindow *window )
{
	if( !window->private->initialized ){
		base_window_init( window );
	}

	static const gchar *thisfn = "base_window_do_run_window";
	g_debug( "%s: window=%p", thisfn, window );

	GtkWidget *this_widget = GTK_WIDGET( window->private->toplevel_widget );
	gtk_widget_show_all( this_widget );
	v_all_widgets_showed( window );

	if( is_main_window( window )){
		g_signal_connect( G_OBJECT( this_widget ), "response", G_CALLBACK( v_dialog_response ), window );
		g_debug( "%s: starting gtk_main", thisfn );
		gtk_main();

	} else {
		g_debug( "%s: starting gtk_dialog_run", thisfn );
		gint code = gtk_dialog_run( GTK_DIALOG( this_widget ));
		v_dialog_response( GTK_DIALOG( this_widget ), code, window );
	}
}

static void
do_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window )
{
	static const gchar *thisfn = "base_window_do_dialog_response";
	g_debug( "%s: dialog=%p, code=%d, window=%p", thisfn, dialog, code, window );
}

static GObject *
do_get_application( BaseWindow *window )
{
	return( G_OBJECT( window->private->application ));
}

static GtkWindow *
do_get_toplevel_widget( BaseWindow *window )
{
	return( window->private->toplevel_widget );
}

static GtkWidget *
do_get_widget( BaseWindow *window, const gchar *name )
{
	g_assert( BASE_IS_WINDOW( window ));
	return( base_application_get_widget( window->private->application, name ));
}

static gboolean
is_toplevel_initialized( BaseWindow *window )
{
	GtkWindow *toplevel = window->private->toplevel_widget;
	g_assert( toplevel );
	g_assert( GTK_IS_WINDOW( toplevel ));

	gpointer data = g_object_get_data( G_OBJECT( toplevel ), "toplevel-initialized" );
	if( !data ){
		return( FALSE );
	}
	return(( gboolean ) data );
}

static void
set_toplevel_initialized( BaseWindow *window )
{
	GtkWindow *toplevel = window->private->toplevel_widget;
	g_assert( toplevel );
	g_assert( GTK_IS_WINDOW( toplevel ));

	g_object_set_data( G_OBJECT( toplevel ), "toplevel-initialized", ( gpointer ) TRUE );
}

static gboolean
is_main_window( BaseWindow *window )
{
	BaseApplication *appli = window->private->application;

	BaseWindow *main_window = BASE_WINDOW( base_application_get_main_window( appli ));

	GtkWidget *main_widget = GTK_WIDGET( base_window_get_toplevel_widget( main_window ));

	GtkWidget *this_widget = GTK_WIDGET( window->private->toplevel_widget );

	return( main_widget == this_widget );
}
