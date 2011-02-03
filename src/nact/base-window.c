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

#include <glib/gi18n.h>
#include <stdlib.h>
#include <string.h>

#include "base-application.h"
#include "base-window.h"
#include "base-gtk-utils.h"

/* private class data
 */
struct _BaseWindowClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _BaseWindowPrivate {
	gboolean         dispose_has_run;

	/* properties
	 */
	BaseWindow      *parent;
	BaseApplication *application;
	gchar           *xmlui_filename;
	gboolean         has_own_builder;
	gchar           *toplevel_name;
	gchar           *wsp_name;

	/* internals
	 */
	GtkWindow       *gtk_toplevel;
	gboolean         initialized;
	GList           *signals;
	BaseBuilder     *builder;
};

/* connected signal, to be disconnected at NactWindow dispose
 */
typedef struct {
	gpointer instance;
	gulong   handler_id;
}
	RecordedSignal;

/* instance properties
 */
enum {
	BASE_PROP_0,

	BASE_PROP_PARENT_ID,
	BASE_PROP_APPLICATION_ID,
	BASE_PROP_XMLUI_FILENAME_ID,
	BASE_PROP_HAS_OWN_BUILDER_ID,
	BASE_PROP_TOPLEVEL_NAME_ID,
	BASE_PROP_WSP_NAME_ID,

	BASE_PROP_N_PROPERTIES
};

/* signals defined in BaseWindow, to be used in all derived classes
 */
enum {
	INITIALIZE_GTK,
	INITIALIZE_BASE,
	ALL_WIDGETS_SHOWED,
	LAST_SIGNAL
};

static GObjectClass *st_parent_class           = NULL;
static gint          st_signals[ LAST_SIGNAL ] = { 0 };
static gboolean      st_debug_signal_connect   = FALSE;
static BaseWindow   *st_first_window           = NULL;

static GType    register_type( void );
static void     class_init( BaseWindowClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void     instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void     instance_constructed( GObject *window );
static void     instance_dispose( GObject *window );
static void     instance_finalize( GObject *window );

/* initialization process
 */
static gboolean setup_builder( const BaseWindow *window );
static gboolean load_gtk_toplevel( const BaseWindow *window );
static gboolean is_gtk_toplevel_initialized( const BaseWindow *window, GtkWindow *gtk_toplevel );
static void     set_gtk_toplevel_initialized( const BaseWindow *window, GtkWindow *gtk_toplevel, gboolean init );
static void     on_initialize_gtk_toplevel_class_handler( BaseWindow *window, GtkWindow *toplevel );
static void     do_initialize_gtk_toplevel( BaseWindow *window, GtkWindow *toplevel );

/* run
 */
static void     on_initialize_base_window_class_handler( BaseWindow *window );
static void     do_initialize_base_window( BaseWindow *window );
static void     on_all_widgets_showed_class_handler( BaseWindow *window );
static gboolean do_run( BaseWindow *window, GtkWindow *toplevel );
static gboolean is_main_window( BaseWindow *window );
static gboolean on_delete_event( GtkWidget *widget, GdkEvent *event, BaseWindow *window );

static void     record_connected_signal( BaseWindow *window, GObject *instance, gulong handler_id );
static gint     display_dlg( const BaseWindow *parent, GtkMessageType type_message, GtkButtonsType type_buttons, const gchar *primary, const gchar *secondary );

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

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "BaseWindow", &info, 0 );

	return( type );
}

static void
class_init( BaseWindowClass *klass )
{
	static const gchar *thisfn = "base_window_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->get_property = instance_get_property;
	object_class->set_property = instance_set_property;
	object_class->constructed = instance_constructed;
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	g_object_class_install_property( object_class, BASE_PROP_PARENT_ID,
			g_param_spec_pointer(
					BASE_PROP_PARENT,
					_( "Parent BaseWindow" ),
					_( "A pointer (not a reference) to the BaseWindow parent of this BaseWindow" ),
					G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	g_object_class_install_property( object_class, BASE_PROP_APPLICATION_ID,
			g_param_spec_pointer(
					BASE_PROP_APPLICATION,
					_( "BaseApplication" ),
					_( "A pointer (not a reference) to the BaseApplication instance" ),
					G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	g_object_class_install_property( object_class, BASE_PROP_XMLUI_FILENAME_ID,
			g_param_spec_string(
					BASE_PROP_XMLUI_FILENAME,
					_( "XML UI filename" ),
					_( "The filename which contains the XML UI definition" ),
					"",
					G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	g_object_class_install_property( object_class, BASE_PROP_HAS_OWN_BUILDER_ID,
			g_param_spec_boolean(
					BASE_PROP_HAS_OWN_BUILDER,
					_( "Has its own GtkBuilder" ),
					_( "Whether this BaseWindow reallocates a new GtkBuilder each time it is opened" ),
					FALSE,
					G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	g_object_class_install_property( object_class, BASE_PROP_TOPLEVEL_NAME_ID,
			g_param_spec_string(
					BASE_PROP_TOPLEVEL_NAME,
					_( "Toplevel name" ),
					_( "The internal GtkBuildable name of the toplevel window" ),
					"",
					G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	g_object_class_install_property( object_class, BASE_PROP_WSP_NAME_ID,
			g_param_spec_string(
					BASE_PROP_WSP_NAME,
					_( "WSP name" ),
					_( "The string which handles the window size and position in user preferences" ),
					"",
					G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	klass->private = g_new0( BaseWindowClassPrivate, 1 );

	klass->initialize_gtk_toplevel = do_initialize_gtk_toplevel;
	klass->initialize_base_window = do_initialize_base_window;
	klass->all_widgets_showed = NULL;
	klass->run = do_run;
	klass->is_willing_to_quit = NULL;

	/**
	 * base-window-initialize-gtk:
	 *
	 * The signal is emitted by and on the #BaseWindow instance when it has
	 * loaded for the first time the Gtk toplevel widget from the GtkBuilder.
	 *
	 * The toplevel GtkWindow is passed as a parameter to this signal.
	 */
	st_signals[ INITIALIZE_GTK ] =
		g_signal_new_class_handler(
				BASE_SIGNAL_INITIALIZE_GTK,
				G_TYPE_FROM_CLASS( klass ),
				G_SIGNAL_RUN_LAST,
				G_CALLBACK( on_initialize_gtk_toplevel_class_handler ),
				NULL,
				NULL,
				g_cclosure_marshal_VOID__POINTER,
				G_TYPE_NONE,
				1,
				G_TYPE_POINTER );

	/**
	 * base-window-initialize-window:
	 *
	 * The signal is emitted by the #BaseWindow instance after the toplevel
	 * GtkWindow has been initialized, before actually displaying the window.
	 * Is is so time to initialize it with runtime values.
	 */
	st_signals[ INITIALIZE_BASE ] =
		g_signal_new_class_handler(
				BASE_SIGNAL_INITIALIZE_WINDOW,
				G_TYPE_FROM_CLASS( klass ),
				G_SIGNAL_RUN_LAST,
				G_CALLBACK( on_initialize_base_window_class_handler ),
				NULL,
				NULL,
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE,
				0 );

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
				BASE_SIGNAL_ALL_WIDGETS_SHOWED,
				G_TYPE_FROM_CLASS( klass ),
				G_SIGNAL_RUN_LAST,
				G_CALLBACK( on_all_widgets_showed_class_handler ),
				NULL,
				NULL,
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE,
				0 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "base_window_instance_init";
	BaseWindow *self;

	g_return_if_fail( BASE_IS_WINDOW( instance ));

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self = BASE_WINDOW( instance );

	/* at a first glance, we may suppose that this first window is the main one
	 * if this is not the case, we would have to write some more code
	 */
	if( !st_first_window ){
		st_first_window = self;
	}

	self->private = g_new0( BaseWindowPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->signals = NULL;
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	BaseWindow *self;

	g_return_if_fail( BASE_IS_WINDOW( object ));
	self = BASE_WINDOW( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case BASE_PROP_PARENT_ID:
				g_value_set_pointer( value, self->private->parent );
				break;

			case BASE_PROP_APPLICATION_ID:
				g_value_set_pointer( value, self->private->application );
				break;

			case BASE_PROP_XMLUI_FILENAME_ID:
				g_value_set_string( value, self->private->xmlui_filename );
				break;

			case BASE_PROP_HAS_OWN_BUILDER_ID:
				g_value_set_boolean( value, self->private->has_own_builder );
				break;

			case BASE_PROP_TOPLEVEL_NAME_ID:
				g_value_set_string( value, self->private->toplevel_name );
				break;

			case BASE_PROP_WSP_NAME_ID:
				g_value_set_string( value, self->private->wsp_name );
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
			case BASE_PROP_PARENT_ID:
				self->private->parent = g_value_get_pointer( value );
				break;

			case BASE_PROP_APPLICATION_ID:
				self->private->application = g_value_get_pointer( value );
				break;

			case BASE_PROP_XMLUI_FILENAME_ID:
				g_free( self->private->xmlui_filename );
				self->private->xmlui_filename = g_value_dup_string( value );
				break;

			case BASE_PROP_HAS_OWN_BUILDER_ID:
				self->private->has_own_builder = g_value_get_boolean( value );
				break;

			case BASE_PROP_TOPLEVEL_NAME_ID:
				g_free( self->private->toplevel_name );
				self->private->toplevel_name = g_value_dup_string( value );
				break;

			case BASE_PROP_WSP_NAME_ID:
				g_free( self->private->wsp_name );
				self->private->wsp_name = g_value_dup_string( value );
				break;

			default:
				G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
				break;
		}
	}
}

static void
instance_constructed( GObject *window )
{
	static const gchar *thisfn = "base_window_instance_constructed";
	BaseWindow *self;

	g_return_if_fail( BASE_IS_WINDOW( window ));

	self = BASE_WINDOW( window );

	if( !self->private->dispose_has_run ){
		g_debug( "%s: window=%p (%s)", thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ));

		g_debug( "%s: application=%p", thisfn, ( void * ) self->private->application );

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->constructed ){
			G_OBJECT_CLASS( st_parent_class )->constructed( window );
		}
	}
}

static void
instance_dispose( GObject *window )
{
	static const gchar *thisfn = "base_window_instance_dispose";
	BaseWindow *self;
	GList *is;

	g_return_if_fail( BASE_IS_WINDOW( window ));

	self = BASE_WINDOW( window );

	if( !self->private->dispose_has_run ){
		g_debug( "%s: window=%p (%s)", thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ));

		base_gtk_utils_save_window_position( self, self->private->wsp_name );

		/* signals must be deconnected before quitting main loop
		 */
		for( is = self->private->signals ; is ; is = is->next ){
			RecordedSignal *str = ( RecordedSignal * ) is->data;
			if( g_signal_handler_is_connected( str->instance, str->handler_id )){
				g_signal_handler_disconnect( str->instance, str->handler_id );
				if( st_debug_signal_connect ){
					g_debug( "%s: disconnecting signal handler %p:%lu", thisfn, str->instance, str->handler_id );
				}
			}
			g_free( str );
		}
		g_list_free( self->private->signals );

		if( is_main_window( BASE_WINDOW( window ))){
			g_debug( "%s: quitting main window", thisfn );
			gtk_main_quit ();
			gtk_widget_destroy( GTK_WIDGET( self->private->gtk_toplevel ));

		} else if( GTK_IS_ASSISTANT( self->private->gtk_toplevel )){
			g_debug( "%s: quitting assistant", thisfn );
			gtk_main_quit();
			if( is_gtk_toplevel_initialized( self, self->private->gtk_toplevel )){
				gtk_widget_hide( GTK_WIDGET( self->private->gtk_toplevel ));
			}

		} else {
			g_debug( "%s: quitting dialog", thisfn );
			if( is_gtk_toplevel_initialized( self, self->private->gtk_toplevel )){
				gtk_widget_hide( GTK_WIDGET( self->private->gtk_toplevel ));
			}
		}

		/* must dispose _after_ quitting the loop
		 */
		self->private->dispose_has_run = TRUE;

		/* release the Gtkbuilder, if any
		 */
		if( self->private->has_own_builder ){
			if( self->private->builder ){
				g_object_unref( self->private->builder );
			}
		}

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

	g_return_if_fail( BASE_IS_WINDOW( window ));

	g_debug( "%s: window=%p (%s)", thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ));

	self = BASE_WINDOW( window );

	g_free( self->private->toplevel_name );
	g_free( self->private->xmlui_filename );

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
	gboolean initialized;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), FALSE );

	if( !window->private->dispose_has_run &&
		!window->private->initialized ){

		g_debug( "%s: window=%p (%s)", thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ));

		if( !window->private->application ){
			g_return_val_if_fail( window->private->parent, FALSE );
			g_return_val_if_fail( BASE_IS_WINDOW( window->private->parent ), FALSE );
			window->private->application = BASE_APPLICATION( base_window_get_application( window->private->parent ));
			g_debug( "%s: application=%p", thisfn, ( void * ) window->private->application );
		}

		g_return_val_if_fail( window->private->application, FALSE );
		g_return_val_if_fail( BASE_IS_APPLICATION( window->private->application ), FALSE );

		if( setup_builder( window ) &
			load_gtk_toplevel( window )){

				if( window->private->gtk_toplevel ){
					g_return_val_if_fail( GTK_IS_WINDOW( window->private->gtk_toplevel ), FALSE );

					initialized = is_gtk_toplevel_initialized( window, window->private->gtk_toplevel );
					g_debug( "%s: gtk_toplevel=%p, initialized=%s",
							thisfn, ( void * ) window->private->gtk_toplevel, initialized ? "True":"False" );

					if( !initialized ){
						g_signal_emit_by_name( window, BASE_SIGNAL_INITIALIZE_GTK, window->private->gtk_toplevel );
						set_gtk_toplevel_initialized( window, window->private->gtk_toplevel, TRUE );
					}

					window->private->initialized = TRUE;
				}
		}
	}

	return( window->private->initialized );
}

/*
 * setup the builder of the window as a new one, or use the global one
 *
 * A dialog may have its own builder ,sharing the common UI XML definition file
 * or a dialog may have its own UI XML definition file, sharing the common builder
 * or a dialog may have both its UI XML definition file with its own builder
 */
static gboolean
setup_builder( const BaseWindow *window )
{
	static const gchar *thisfn = "base_window_setup_builder";
	gboolean ret;
	GError *error = NULL;
	gchar *msg;

	ret = TRUE;

	/* allocate a dedicated BaseBuilder or use the common one
	 */
	g_debug( "%s: has_own_builder=%s", thisfn, window->private->has_own_builder ? "True":"False" );
	if( window->private->has_own_builder ){
		window->private->builder = base_builder_new();
	} else {
		g_return_val_if_fail( BASE_IS_APPLICATION( window->private->application ), FALSE );
		window->private->builder = base_application_get_builder( window->private->application );
	}

	/* load the XML definition from the UI file
	 */
	g_debug( "%s: xmlui_filename=%s", thisfn, window->private->xmlui_filename );
	if( window->private->xmlui_filename &&
		g_utf8_strlen( window->private->xmlui_filename, -1 ) &&
		!base_builder_add_from_file( window->private->builder, window->private->xmlui_filename, &error )){

			msg = g_strdup_printf( _( "Unable to load %s UI XML definition: %s" ), window->private->xmlui_filename, error->message );
			base_window_display_error_dlg( NULL, thisfn, msg );
			g_free( msg );
			g_error_free( error );
			ret = FALSE;
	}

	g_debug( "%s: ret=%s", thisfn, ret ? "True":"False" );
	return( ret );
}

static gboolean
load_gtk_toplevel( const BaseWindow *window )
{
	GtkWindow *gtk_toplevel;
	gchar *msg;

	gtk_toplevel = NULL;

	if( window->private->toplevel_name ){
		if( strlen( window->private->toplevel_name )){
			g_return_val_if_fail( BASE_IS_BUILDER( window->private->builder ), FALSE );
			gtk_toplevel = base_builder_get_toplevel_by_name( window->private->builder, window->private->toplevel_name );

			if( !gtk_toplevel ){
				msg = g_strdup_printf( _( "Unable to load %s dialog definition." ), window->private->toplevel_name );
				base_window_display_error_dlg( NULL, msg, NULL );
				g_free( msg );
			}
		}
	}

	window->private->gtk_toplevel = gtk_toplevel;

	return( gtk_toplevel != NULL );
}

static gboolean
is_gtk_toplevel_initialized( const BaseWindow *window, GtkWindow *gtk_toplevel )
{
	gboolean initialized;

	initialized = GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( gtk_toplevel ), "base-window-gtk-toplevel-initialized" ));

	return( initialized );
}

static void
set_gtk_toplevel_initialized( const BaseWindow *window, GtkWindow *gtk_toplevel, gboolean initialized )
{
	g_object_set_data( G_OBJECT( gtk_toplevel ), "base-window-gtk-toplevel-initialized", GUINT_TO_POINTER( initialized ));
}

/*
 * default class handler for "base-window-initialize-gtk" signal
 *
 * successively invokes the method of each derived class, starting from
 * the topmost derived up to this BaseWindow
 */
static void
on_initialize_gtk_toplevel_class_handler( BaseWindow *window, GtkWindow *toplevel )
{
	static const gchar *thisfn = "base_window_on_initialize_gtk_toplevel_class_handler";

	g_return_if_fail( BASE_IS_WINDOW( window ));

	if( !window->private->dispose_has_run ){
		g_debug( "%s: window=%p (%s), toplevel=%p (%s)", thisfn,
				( void * ) window, G_OBJECT_TYPE_NAME( window ),
				( void * ) toplevel, G_OBJECT_TYPE_NAME( toplevel ));

		if( BASE_WINDOW_GET_CLASS( window )->initialize_gtk_toplevel ){
			BASE_WINDOW_GET_CLASS( window )->initialize_gtk_toplevel( window, toplevel );
		}
	}
}

static void
do_initialize_gtk_toplevel( BaseWindow *window, GtkWindow *toplevel )
{
	static const gchar *thisfn = "base_window_do_initialize_gtk_toplevel";
	GtkWindow *parent_gtk_toplevel;

	g_return_if_fail( BASE_IS_WINDOW( window ));
	g_return_if_fail( GTK_IS_WINDOW( toplevel ));
	g_return_if_fail( toplevel == window->private->gtk_toplevel );

	if( !window->private->dispose_has_run ){
		g_debug( "%s: window=%p (%s), toplevel=%p (%s), parent=%p (%s)", thisfn,
				( void * ) window, G_OBJECT_TYPE_NAME( window ),
				( void * ) toplevel, G_OBJECT_TYPE_NAME( toplevel ),
				( void * ) window->private->parent,
				window->private->parent ? G_OBJECT_TYPE_NAME( window->private->parent ) : "null" );

		if( window->private->parent ){
			g_return_if_fail( BASE_IS_WINDOW( window->private->parent ));
			parent_gtk_toplevel = base_window_get_gtk_toplevel( BASE_WINDOW( window->private->parent ));
			gtk_window_set_transient_for( toplevel, parent_gtk_toplevel );
		}
	}
}

/**
 * base_window_run:
 * @window: this #BaseWindow object.
 *
 * Runs the window.
 *
 * Returns: the exit code of the program if this is the main window,
 *  the response ID of a dialog box
 */
int
base_window_run( BaseWindow *window )
{
	static const gchar *thisfn = "base_window_run";
	gboolean run_ok;
	int code;

	code = BASE_EXIT_CODE_START_FAIL;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), code );

	if( !window->private->dispose_has_run ){

		run_ok = window->private->initialized;

		if( !run_ok ){
			run_ok = base_window_init( window );
		}

		if( !run_ok ){
			code = BASE_EXIT_CODE_INIT_FAIL;

		} else {
			g_return_val_if_fail( GTK_IS_WINDOW( window->private->gtk_toplevel ), code );
			g_debug( "%s: window=%p (%s)", thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ));

			code = BASE_EXIT_CODE_OK;
			g_signal_emit_by_name( window, BASE_SIGNAL_INITIALIZE_WINDOW );

			gtk_widget_show_all( GTK_WIDGET( window->private->gtk_toplevel ));
			g_signal_emit_by_name( window, BASE_SIGNAL_ALL_WIDGETS_SHOWED );

			if( BASE_WINDOW_GET_CLASS( window )->run ){
				code = BASE_WINDOW_GET_CLASS( window )->run( window, window->private->gtk_toplevel );
			}
		}
	}

	return( code );
}

/*
 * default class handler for "nact-signal-base-window-runtime-init" message
 * -> does nothing here
 */
static void
on_initialize_base_window_class_handler( BaseWindow *window )
{
	static const gchar *thisfn = "base_window_on_initialize_base_window_class_handler";

	g_return_if_fail( BASE_IS_WINDOW( window ));

	if( !window->private->dispose_has_run ){
		g_debug( "%s: window=%p (%s)", thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ));

		if( BASE_WINDOW_GET_CLASS( window )->initialize_base_window ){
			BASE_WINDOW_GET_CLASS( window )->initialize_base_window( window );
		}
	}
}

static void
do_initialize_base_window( BaseWindow *window )
{
	static const gchar *thisfn = "base_window_do_initialize_base_window";

	g_return_if_fail( BASE_IS_WINDOW( window ));

	if( !window->private->dispose_has_run ){
		g_debug( "%s: window=%p (%s)", thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ));

		base_gtk_utils_restore_window_position( window, window->private->wsp_name );
	}
}

static void
on_all_widgets_showed_class_handler( BaseWindow *window )
{
	static const gchar *thisfn = "base_window_on_all_widgets_showed_class_handler";

	g_return_if_fail( BASE_IS_WINDOW( window ));

	if( !window->private->dispose_has_run ){
		g_debug( "%s: window=%p (%s)", thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ));

		if( BASE_WINDOW_GET_CLASS( window )->all_widgets_showed ){
			BASE_WINDOW_GET_CLASS( window )->all_widgets_showed( window );
		}
	}
}

static int
do_run( BaseWindow *window, GtkWindow *toplevel )
{
	static const gchar *thisfn = "base_window_do_run";
	int code;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), BASE_EXIT_CODE_PROGRAM );
	g_return_val_if_fail( GTK_IS_WINDOW( toplevel ), BASE_EXIT_CODE_PROGRAM );

	code = BASE_EXIT_CODE_INIT_FAIL;

	if( !window->private->dispose_has_run ){
		if( is_main_window( window )){
			g_signal_connect( G_OBJECT( toplevel ), "delete-event", G_CALLBACK( on_delete_event ), window );
			g_debug( "%s: window=%p (%s), toplevel=%p (%s), starting gtk_main",
					thisfn,
					( void * ) window, G_OBJECT_TYPE_NAME( window ),
					( void * ) toplevel, G_OBJECT_TYPE_NAME( toplevel ));
			gtk_main();
			code = BASE_EXIT_CODE_OK;
		}
	}

	return( code );
}

static gboolean
is_main_window( BaseWindow *window )
{
	gboolean is_main = FALSE;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), FALSE );

	if( !window->private->dispose_has_run ){

		is_main = ( window == st_first_window );
	}

	return( is_main );
}

/*
 * Handler of "delete-event" message connected on the main window Gtk toplevel
 *
 * Our own function does nothing, and let the signal be propagated
 * it so ends up in the default class handler for this signal
 * which just destroys the toplevel.
 *
 * The main window should really connect to this signal and stop its
 * propagation, at least if it does not want the user be able to abruptly
 * terminate the application.
 */
static gboolean
on_delete_event( GtkWidget *toplevel, GdkEvent *event, BaseWindow *window )
{
	static const gchar *thisfn = "base_window_on_delete_event";
	static gboolean stop = FALSE;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), FALSE );

	g_debug( "%s: toplevel=%p (%s), event=%p, window=%p (%s)",
			thisfn, ( void * ) toplevel, G_OBJECT_TYPE_NAME( toplevel ),
			( void * ) event, ( void * ) window, G_OBJECT_TYPE_NAME( window ));

	return( stop );
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
base_window_get_application( const BaseWindow *window )
{
	BaseApplication *application = NULL;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), NULL );

	if( !window->private->dispose_has_run ){

		application = window->private->application;
	}

	return( application );
}

/**
 * base_window_get_parent:
 * @window: this #BaseWindow instance..
 *
 * Returns the #BaseWindow parent of @window.
 *
 * The returned object is owned by @window, and should not be freed.
 */
BaseWindow *
base_window_get_parent( const BaseWindow *window )
{
	BaseWindow *parent = NULL;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), NULL );

	if( !window->private->dispose_has_run ){

		parent = window->private->parent;
	}

	return( parent );
}

/**
 * base_window_get_gtk_toplevel:
 * @window: this #BaseWindow instance..
 *
 * Returns the top-level GtkWindow attached to this BaseWindow object.
 *
 * The caller may close the window by g_object_unref()-ing the returned
 * #GtkWindow.
 */
GtkWindow *
base_window_get_gtk_toplevel( const BaseWindow *window )
{
	GtkWindow *gtk_toplevel = NULL;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), NULL );

	if( !window->private->dispose_has_run ){

		gtk_toplevel = window->private->gtk_toplevel;
	}

	return( gtk_toplevel );
}

/**
 * base_window_get_gtk_toplevel_by_name:
 * @window: this #BaseWindow instance.
 * @name: the name of the searched GtkWindow.
 *
 * Returns: the named top-level GtkWindow.
 *
 * This is just a convenience function to be able to open quickly a
 * window (e.g. Legend dialog).
 *
 * The caller may close the window by g_object_unref()-ing the returned
 * #GtkWindow.
 */
GtkWindow *
base_window_get_gtk_toplevel_by_name( const BaseWindow *window, const gchar *name )
{
	GtkWindow *gtk_toplevel = NULL;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), NULL );

	if( !window->private->dispose_has_run ){

		gtk_toplevel = base_builder_get_toplevel_by_name( window->private->builder, name );
	}

	return( gtk_toplevel );
}

/**
 * base_window_get_widget:
 * @window: this #BaseWindow instance.
 * @name: the name of the searched child.
 *
 * Returns a pointer to the named widget which is a child of the
 * toplevel #GtkWindow associated with @window.
 *
 * Returns: a pointer to the searched widget, or NULL.
 * This pointer is owned by GtkBuilder instance, and must not be
 * g_free() nor g_object_unref() by the caller.
 */
GtkWidget *
base_window_get_widget( const BaseWindow *window, const gchar *name )
{
	GtkWidget *widget = NULL;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), NULL );

	if( !window->private->dispose_has_run ){

		widget = base_gtk_utils_get_widget_by_name( window->private->gtk_toplevel, name );
	}

	return( widget );
}

/**
 * base_window_is_willing_to_quit:
 * @window: this #BaseWindow instance.
 *
 * Returns: %TRUE if the application is willing to quit, %FALSE else.
 *
 * This function is called when the session manager detects the end of
 * session and thus asks its client if they are willing to quit.
 */
gboolean
base_window_is_willing_to_quit( const BaseWindow *window )
{
	gboolean willing_to;

	willing_to = TRUE;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), TRUE );

	if( !window->private->dispose_has_run ){

		if( BASE_WINDOW_GET_CLASS( window )->is_willing_to_quit ){
			BASE_WINDOW_GET_CLASS( window )->is_willing_to_quit( window );
		}
	}

	return( willing_to );
}

/**
 * base_window_display_error_dlg:
 * @parent: the #BaseWindow parent, may be %NULL.
 * @primary: the primary message.
 * @secondary: the secondary message.
 *
 * Display an error dialog box, with a 'OK' button only.
 *
 * if @secondary is not null, then @primary is displayed as a bold title.
 */
void
base_window_display_error_dlg( const BaseWindow *parent, const gchar *primary, const gchar *secondary )
{
	display_dlg( parent, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, primary, secondary );
}

/**
 * base_window_display_yesno_dlg:
 * @parent: the #BaseWindow parent, may be %NULL.
 * @primary: the primary message.
 * @secondary: the secondary message.
 *
 * Display a warning dialog box, with a 'OK' button only.
 *
 * if @secondary is not null, then @primary is displayed as a bold title.
 *
 * Returns: %TRUE if the user has clicked 'Yes', %FALSE else.
 */
gboolean
base_window_display_yesno_dlg( const BaseWindow *parent, const gchar *primary, const gchar *secondary )
{
	gint result;

	result = display_dlg( parent, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, primary, secondary );

	return( result == GTK_RESPONSE_YES );
}

/**
 * base_window_display_message_dlg:
 * @parent: the #BaseWindow parent, may be %NULL.
 * @message: the message to be displayed.
 *
 * Displays an information dialog with only an OK button.
 */
void
base_window_display_message_dlg( const BaseWindow *parent, GSList *msg )
{
	GString *string;
	GSList *im;

	string = g_string_new( "" );
	for( im = msg ; im ; im = im->next ){
		if( g_utf8_strlen( string->str, -1 )){
			string = g_string_append( string, "\n" );
		}
		string = g_string_append( string, ( gchar * ) im->data );
	}
	display_dlg( parent, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, string->str, NULL );

	g_string_free( string, TRUE );
}

static gint
display_dlg( const BaseWindow *parent, GtkMessageType type_message, GtkButtonsType type_buttons, const gchar *primary, const gchar *secondary )
{
	GtkWindow *gtk_parent;
	GtkWidget *dialog;
	gint result;

	gtk_parent = NULL;
	if( parent ){
		gtk_parent = base_window_get_gtk_toplevel( parent );
	}

	dialog = gtk_message_dialog_new( gtk_parent, GTK_DIALOG_MODAL, type_message, type_buttons, "%s", primary );

	if( secondary && g_utf8_strlen( secondary, -1 )){
		gtk_message_dialog_format_secondary_text( GTK_MESSAGE_DIALOG( dialog ), "%s", secondary );
	}

	g_object_set( G_OBJECT( dialog ) , "title", g_get_application_name(), NULL );

	result = gtk_dialog_run( GTK_DIALOG( dialog ));

	gtk_widget_destroy( dialog );

	return( result );
}

/**
 * Records a connected signal, to be disconnected at NactWindow dispose.
 */
gulong
base_window_signal_connect( BaseWindow *window, GObject *instance, const gchar *signal, GCallback fn )
{
	gulong handler_id = 0;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), ( gulong ) 0 );

	if( !window->private->dispose_has_run ){

		handler_id = g_signal_connect( instance, signal, fn, window );
		record_connected_signal( window, instance, handler_id );
	}

	return( handler_id );
}

gulong
base_window_signal_connect_after( BaseWindow *window, GObject *instance, const gchar *signal, GCallback fn )
{
	gulong handler_id = 0;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), ( gulong ) 0 );

	if( !window->private->dispose_has_run ){

		handler_id = g_signal_connect_after( instance, signal, fn, window );
		record_connected_signal( window, instance, handler_id );
	}

	return( handler_id );
}

gulong
base_window_signal_connect_by_name( BaseWindow *window, const gchar *name, const gchar *signal, GCallback fn )
{
	gulong handler_id = 0;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), ( gulong ) 0 );

	if( !window->private->dispose_has_run ){

		GtkWidget *widget = base_window_get_widget( window, name );
		if( GTK_IS_WIDGET( widget )){

			handler_id = base_window_signal_connect( window, G_OBJECT( widget ), signal, fn );
		}
	}

	return( handler_id );
}

gulong
base_window_signal_connect_with_data( BaseWindow *window, GObject *instance, const gchar *signal, GCallback fn, void *user_data )
{
	gulong handler_id = 0;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), ( gulong ) 0 );

	if( !window->private->dispose_has_run ){

		handler_id = g_signal_connect( instance, signal, fn, user_data );
		record_connected_signal( window, instance, handler_id );
	}

	return( handler_id );
}

static void
record_connected_signal( BaseWindow *window, GObject *instance, gulong handler_id )
{
	static const gchar *thisfn = "base_window_record_connected_signal";

	RecordedSignal *str = g_new0( RecordedSignal, 1 );
	str->instance = instance;
	str->handler_id = handler_id;
	window->private->signals = g_list_prepend( window->private->signals, str );

	if( st_debug_signal_connect ){
		g_debug( "%s: connecting signal handler %p:%lu", thisfn, ( void * ) instance, handler_id );
	}
}
