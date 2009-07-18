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
	BaseWindow      *parent;
	BaseApplication *application;
	gchar           *toplevel_name;
	GtkWindow       *toplevel_dialog;
	gboolean         initialized;
};

/* instance properties
 */
enum {
	PROP_WINDOW_PARENT = 1,
	PROP_WINDOW_APPLICATION,
	PROP_WINDOW_TOPLEVEL_NAME,
	PROP_WINDOW_TOPLEVEL_DIALOG,
	PROP_WINDOW_INITIALIZED
};

static GObjectClass *st_parent_class = NULL;

static GType      register_type( void );
static void       class_init( BaseWindowClass *klass );
static void       instance_init( GTypeInstance *instance, gpointer klass );
static void       instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void       instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void       instance_dispose( GObject *application );
static void       instance_finalize( GObject *application );

static gchar     *v_get_toplevel_name( BaseWindow *window );
static void       v_initial_load_toplevel( BaseWindow *window );
static void       v_runtime_init_toplevel( BaseWindow *window );
static void       v_all_widgets_showed( BaseWindow *window );
static gboolean   v_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window );
static gboolean   v_delete_event( GtkWidget *widget, GdkEvent *event, BaseWindow *window );

static void       do_init_window( BaseWindow *window );
static void       do_initial_load_toplevel( BaseWindow *window );
static void       do_runtime_init_toplevel( BaseWindow *window );
static void       do_all_widgets_showed( BaseWindow *window );
static void       do_run_window( BaseWindow *window );
static gboolean   do_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window );
static GObject   *do_get_application( BaseWindow *window );
static GtkWindow *do_get_toplevel_dialog( BaseWindow *window );
static GtkWindow *do_get_dialog( BaseWindow *window, const gchar *name );
static GtkWidget *do_get_widget( BaseWindow *window, const gchar *name );

static gboolean   is_main_window( BaseWindow *window );
static gboolean   is_toplevel_initialized( BaseWindow *window, GtkWindow *toplevel );
static void       set_toplevel_initialized( BaseWindow *window, GtkWindow *toplevel, gboolean init );

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
			PROP_WINDOW_PARENT_STR,
			PROP_WINDOW_PARENT_STR,
			"Parent BaseWindow object pointer",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_WINDOW_PARENT, spec );

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
			PROP_WINDOW_TOPLEVEL_DIALOG_STR,
			PROP_WINDOW_TOPLEVEL_DIALOG_STR,
			"The main GtkWindow attached to this object",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_WINDOW_TOPLEVEL_DIALOG, spec );

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
	klass->get_toplevel_dialog = do_get_toplevel_dialog;
	klass->get_dialog = do_get_dialog;
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
		case PROP_WINDOW_PARENT:
			g_value_set_pointer( value, self->private->parent );
			break;

		case PROP_WINDOW_APPLICATION:
			g_value_set_pointer( value, self->private->application );
			break;

		case PROP_WINDOW_TOPLEVEL_NAME:
			g_value_set_string( value, self->private->toplevel_name );
			break;

		case PROP_WINDOW_TOPLEVEL_DIALOG:
			g_value_set_pointer( value, self->private->toplevel_dialog );
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
		case PROP_WINDOW_PARENT:
			self->private->parent = g_value_get_pointer( value );
			break;

		case PROP_WINDOW_APPLICATION:
			self->private->application = g_value_get_pointer( value );
			break;

		case PROP_WINDOW_TOPLEVEL_NAME:
			g_free( self->private->toplevel_name );
			self->private->toplevel_name = g_value_dup_string( value );
			break;

		case PROP_WINDOW_TOPLEVEL_DIALOG:
			self->private->toplevel_dialog = g_value_get_pointer( value );
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

		if( is_main_window( BASE_WINDOW( window ))){
			g_debug( "%s: quitting main window", thisfn );
			gtk_main_quit ();
			gtk_widget_destroy( GTK_WIDGET( self->private->toplevel_dialog ));

		} else if( GTK_IS_ASSISTANT( self->private->toplevel_dialog )){
			g_debug( "%s: quitting assistant", thisfn );
			gtk_main_quit();
			gtk_widget_hide_all( GTK_WIDGET( self->private->toplevel_dialog ));

		} else {
			g_debug( "%s: quitting dialog", thisfn );
			gtk_widget_hide_all( GTK_WIDGET( self->private->toplevel_dialog ));
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
 * allocated. This should leave the BaseWindow object with a valid
 * toplevel GtkWindow dialog. This is also time to make one-time
 * initialization on this toplevel dialog.
 *
 * For an every-time initialization, see base_window_run.
 *
 * Note that the BaseWindow itself should be initialized each time
 * the user opens the dialog, though the GtkWindow itself needs only
 * be initialized the first time it is loaded.
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
base_window_get_toplevel_dialog( BaseWindow *window )
{
	g_assert( BASE_IS_WINDOW( window ));
	return( BASE_WINDOW_GET_CLASS( window )->get_toplevel_dialog( window ));
}

/**
 * Returns a top-level GtkWindow.
 *
 * @window: this BaseWindow object.
 *
 * @name: the name of the searched GtkWindow.
 */
GtkWindow *
base_window_get_dialog( BaseWindow *window, const gchar *name )
{
	g_assert( BASE_IS_WINDOW( window ));
	return( BASE_WINDOW_GET_CLASS( window )->get_dialog( window, name ));
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

	GtkWindow *toplevel = window->private->toplevel_dialog;
	g_assert( toplevel );
	g_assert( GTK_IS_WINDOW( toplevel ));

	if( window->private->toplevel_dialog ){
		if( BASE_WINDOW_GET_CLASS( window )->initial_load_toplevel ){
			BASE_WINDOW_GET_CLASS( window )->initial_load_toplevel( window );
		}
	}
}

static void
v_runtime_init_toplevel( BaseWindow *window )
{
	g_assert( BASE_IS_WINDOW( window ));

	if( window->private->toplevel_dialog ){
		if( BASE_WINDOW_GET_CLASS( window )->runtime_init_toplevel ){
			BASE_WINDOW_GET_CLASS( window )->runtime_init_toplevel( window );
		}
	}
}

static void
v_all_widgets_showed( BaseWindow *window )
{
	g_assert( BASE_IS_WINDOW( window ));

	if( window->private->toplevel_dialog ){
		if( BASE_WINDOW_GET_CLASS( window )->all_widgets_showed ){
			BASE_WINDOW_GET_CLASS( window )->all_widgets_showed( window );
		}
	}
}

/*
 * return TRUE to quit the dialog loop
 */
static gboolean
v_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window )
{
	g_assert( BASE_IS_WINDOW( window ));

	if( BASE_WINDOW_GET_CLASS( window )->dialog_response ){
		return( BASE_WINDOW_GET_CLASS( window )->dialog_response( dialog, code, window ));
	}

	return( TRUE );
}

/*
 * return TRUE to quit the toplevel window loop
 */
static gboolean
v_delete_event( GtkWidget *widget, GdkEvent *event, BaseWindow *window )
{
	g_assert( BASE_IS_WINDOW( window ));
	g_assert( GTK_IS_WINDOW( widget ));

	if( BASE_WINDOW_GET_CLASS( window )->delete_event ){
		return( BASE_WINDOW_GET_CLASS( window )->delete_event( window, GTK_WINDOW( widget ), event ));
	}

	return( TRUE );
}

static void
do_init_window( BaseWindow *window )
{
	static const gchar *thisfn = "base_window_do_init_window";
	g_assert( BASE_IS_WINDOW( window ));

	if( !window->private->initialized ){
		g_debug( "%s: window=%p", thisfn, window );

		if( !window->private->application ){
			g_assert( window->private->parent );
			g_assert( BASE_IS_WINDOW( window->private->parent ));
			window->private->application = BASE_APPLICATION( base_window_get_application( window->private->parent ));
			g_debug( "%s: application=%p", thisfn, window->private->application );
		}

		g_assert( window->private->application );
		g_assert( BASE_IS_APPLICATION( window->private->application ));

		gchar *dialog_name = v_get_toplevel_name( window );
		g_assert( dialog_name && strlen( dialog_name ));

		GtkWindow *toplevel = base_window_get_dialog( window, dialog_name );
		window->private->toplevel_dialog = toplevel;

		if( toplevel ){
			g_assert( GTK_IS_WINDOW( toplevel ));

			if( !is_toplevel_initialized( window, toplevel )){

				v_initial_load_toplevel( window );
				set_toplevel_initialized( window, toplevel, TRUE );
			}
		}

		g_free( dialog_name );
		window->private->initialized = TRUE;
	}
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

	if( window->private->parent ){
		g_assert( BASE_IS_WINDOW( window->private->parent ));
		GtkWindow *parent_toplevel = base_window_get_toplevel_dialog( BASE_WINDOW( window->private->parent ));
		gtk_window_set_transient_for( window->private->toplevel_dialog, parent_toplevel );
	}
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

	v_runtime_init_toplevel( window );

	static const gchar *thisfn = "base_window_do_run_window";
	g_debug( "%s: window=%p", thisfn, window );

	GtkWidget *this_dialog = GTK_WIDGET( window->private->toplevel_dialog );
	gtk_widget_show_all( this_dialog );
	v_all_widgets_showed( window );

	if( is_main_window( window )){

		if( GTK_IS_DIALOG( this_dialog )){
			g_signal_connect( G_OBJECT( this_dialog ), "response", G_CALLBACK( v_dialog_response ), window );
		} else {
			g_signal_connect( G_OBJECT( this_dialog ), "delete-event", G_CALLBACK( v_delete_event ), window );
		}

		g_debug( "%s: application=%p, starting gtk_main", thisfn, window->private->application );
		gtk_main();

	} else if( GTK_IS_ASSISTANT( this_dialog )){
		g_debug( "%s: starting gtk_main", thisfn );
		gtk_main();

	} else {
		g_assert( GTK_IS_DIALOG( this_dialog ));
		g_debug( "%s: starting gtk_dialog_run", thisfn );
		gint code;
		do {
			code = gtk_dialog_run( GTK_DIALOG( this_dialog ));
		}
		while( !v_dialog_response( GTK_DIALOG( this_dialog ), code, window ));
	}
}

static gboolean
do_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window )
{
	static const gchar *thisfn = "base_window_do_dialog_response";
	g_debug( "%s: dialog=%p, code=%d, window=%p", thisfn, dialog, code, window );

	return( TRUE );
}

static GObject *
do_get_application( BaseWindow *window )
{
	return( G_OBJECT( window->private->application ));
}

static GtkWindow *
do_get_toplevel_dialog( BaseWindow *window )
{
	return( window->private->toplevel_dialog );
}

static GtkWindow *
do_get_dialog( BaseWindow *window, const gchar *name )
{
	g_assert( BASE_IS_WINDOW( window ));
	return( base_application_get_dialog( window->private->application, name ));
}

static GtkWidget *
do_get_widget( BaseWindow *window, const gchar *name )
{
	g_assert( BASE_IS_WINDOW( window ));
	return( base_application_get_widget( window->private->application, window, name ));
}

static gboolean
is_main_window( BaseWindow *window )
{
	BaseApplication *appli = window->private->application;

	BaseWindow *main_window = BASE_WINDOW( base_application_get_main_window( appli ));

	GtkWidget *main_dialog = GTK_WIDGET( base_window_get_toplevel_dialog( main_window ));

	GtkWidget *this_dialog = GTK_WIDGET( window->private->toplevel_dialog );

	return( main_dialog == this_dialog );
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
	g_assert( BASE_IS_WINDOW( window ));
	base_application_error_dlg( window->private->application, type, primary, secondary );
}

gboolean
base_window_yesno_dlg( BaseWindow *window, GtkMessageType type, const gchar *first, const gchar *second )
{
	g_assert( BASE_IS_WINDOW( window ));
	return( base_application_yesno_dlg( window->private->application, type, first, second ));
}
