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

#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <glib/gi18n.h>

#include "base-application.h"
#include "nact-assistant.h"

/* private class data
 */
struct NactAssistantClassPrivate {
};

/* private instance data
 */
struct NactAssistantPrivate {
	gboolean dispose_has_run;
	gboolean warn_on_cancel;
};

/* instance properties
 */
enum {
	PROP_ASSIST_WARN_ON_CANCEL = 1
};

#define PROP_ASSIST_WARN_ON_CANCEL_STR		"nact-assist-warn-on-cancel"

static GObjectClass *st_parent_class = NULL;

static GType    register_type( void );
static void     class_init( NactAssistantClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void     instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void     instance_dispose( GObject *application );
static void     instance_finalize( GObject *application );

static GtkWindow * get_dialog( BaseWindow *window, const gchar *name );

static void     v_assistant_apply( GtkAssistant *assistant, gpointer user_data );
static void     v_assistant_cancel( GtkAssistant *assistant, gpointer user_data );
static void     v_assistant_close( GtkAssistant *assistant, gpointer user_data );
static void     v_assistant_prepare( GtkAssistant *assistant, GtkWidget *page, gpointer user_data );

static void     on_runtime_init_toplevel( BaseWindow *window );
static gboolean on_key_pressed_event( GtkWidget *widget, GdkEventKey *event, gpointer user_data );
static gboolean on_escape_key_pressed( GtkWidget *widget, GdkEventKey *event, gpointer user_data );
static void     do_assistant_apply( NactAssistant *window, GtkAssistant *assistant );
static void     do_assistant_cancel( NactAssistant *window, GtkAssistant *assistant );
static void     do_assistant_close( NactAssistant *window, GtkAssistant *assistant );
static void     do_assistant_prepare( NactAssistant *window, GtkAssistant *assistant, GtkWidget *page );

GType
nact_assistant_get_type( void )
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
	static const gchar *thisfn = "nact_assistant_register_type";
	g_debug( "%s", thisfn );

	g_type_init();

	static GTypeInfo info = {
		sizeof( NactAssistantClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactAssistant ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	GType type = g_type_register_static( NACT_WINDOW_TYPE, "NactAssistant", &info, 0 );

	return( type );
}

static void
class_init( NactAssistantClass *klass )
{
	static const gchar *thisfn = "nact_assistant_class_init";
	g_debug( "%s: klass=%p", thisfn, klass );

	st_parent_class = g_type_class_peek_parent( klass );

	GObjectClass *object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
	object_class->get_property = instance_get_property;
	object_class->set_property = instance_set_property;

	GParamSpec *spec;
	spec = g_param_spec_boolean(
			PROP_ASSIST_WARN_ON_CANCEL_STR,
			PROP_ASSIST_WARN_ON_CANCEL_STR,
			"Does the user should confirm when exiting the assistant ?", FALSE,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_ASSIST_WARN_ON_CANCEL, spec );

	klass->private = g_new0( NactAssistantClassPrivate, 1 );

	BaseWindowClass *base_class = BASE_WINDOW_CLASS( klass );
	base_class->get_dialog = get_dialog;
	base_class->runtime_init_toplevel = on_runtime_init_toplevel;

	klass->on_escape_key_pressed = on_escape_key_pressed;
	klass->on_assistant_apply = do_assistant_apply;
	klass->on_assistant_close = do_assistant_close;
	klass->on_assistant_cancel = do_assistant_cancel;
	klass->on_assistant_prepare = do_assistant_prepare;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_assistant_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );

	g_assert( NACT_IS_ASSISTANT( instance ));
	NactAssistant *self = NACT_ASSISTANT( instance );

	self->private = g_new0( NactAssistantPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	g_assert( NACT_IS_ASSISTANT( object ));
	NactAssistant *self = NACT_ASSISTANT( object );

	switch( property_id ){
		case PROP_ASSIST_WARN_ON_CANCEL:
			g_value_set_boolean( value, self->private->warn_on_cancel );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	g_assert( NACT_IS_ASSISTANT( object ));
	NactAssistant *self = NACT_ASSISTANT( object );

	switch( property_id ){
		case PROP_ASSIST_WARN_ON_CANCEL:
			self->private->warn_on_cancel = g_value_get_boolean( value );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_dispose( GObject *window )
{
	static const gchar *thisfn = "nact_assistant_instance_dispose";
	g_debug( "%s: window=%p", thisfn, window );

	g_assert( NACT_IS_ASSISTANT( window ));
	NactAssistant *self = NACT_ASSISTANT( window );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		G_OBJECT_CLASS( st_parent_class )->dispose( window );
	}
}

static void
instance_finalize( GObject *window )
{
	static const gchar *thisfn = "nact_assistant_instance_finalize";
	g_debug( "%s: window=%p", thisfn, window );

	g_assert( NACT_IS_ASSISTANT( window ));
	NactAssistant *self = ( NactAssistant * ) window;

	g_free( self->private );

	/* chain call to parent class */
	if( st_parent_class->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( window );
	}
}

/*
 * cf. http://bugzilla.gnome.org/show_bug.cgi?id=589746
 * a GtkFileChooseWidget embedded in a GtkAssistant is not displayed
 * when run more than once
 *
 * as a work-around, reload the XML ui each time we run an assistant !
 */
static GtkWindow *
get_dialog( BaseWindow *window, const gchar *name )
{
	GtkBuilder *builder = gtk_builder_new();

	BaseApplication *appli = base_window_get_application( window );

	gchar *fname = base_application_get_ui_filename( appli );

	gtk_builder_add_from_file( builder, fname, NULL );

	g_free( fname );

	GtkWindow *dialog = GTK_WINDOW( gtk_builder_get_object( builder, name ));

	return( dialog );
}

/**
 * Set 'warn on close' property.
 */

void
nact_assistant_set_warn_on_cancel( NactAssistant *window, gboolean warn )
{
	g_assert( NACT_IS_ASSISTANT( window ));
	g_object_set( G_OBJECT( window ), PROP_ASSIST_WARN_ON_CANCEL_STR, warn, NULL );
}

static void
v_assistant_apply( GtkAssistant *assistant, gpointer user_data )
{
	g_assert( NACT_IS_ASSISTANT( user_data ));

	if( NACT_ASSISTANT_GET_CLASS( user_data )->on_assistant_apply ){
		NACT_ASSISTANT_GET_CLASS( user_data )->on_assistant_apply( NACT_ASSISTANT( user_data ), assistant );
	} else {
		do_assistant_apply( NACT_ASSISTANT( user_data ), assistant );
	}
}

static void
v_assistant_cancel( GtkAssistant *assistant, gpointer user_data )
{
	g_assert( NACT_IS_ASSISTANT( user_data ));

	if( NACT_ASSISTANT_GET_CLASS( user_data )->on_assistant_cancel ){
		NACT_ASSISTANT_GET_CLASS( user_data )->on_assistant_cancel( NACT_ASSISTANT( user_data ), assistant );
	} else {
		do_assistant_cancel( NACT_ASSISTANT( user_data ), assistant );
	}
}

static void
v_assistant_close( GtkAssistant *assistant, gpointer user_data )
{
	g_assert( NACT_IS_ASSISTANT( user_data ));

	if( NACT_ASSISTANT_GET_CLASS( user_data )->on_assistant_close ){
		NACT_ASSISTANT_GET_CLASS( user_data )->on_assistant_close( NACT_ASSISTANT( user_data ), assistant );
	} else {
		do_assistant_close( NACT_ASSISTANT( user_data ), assistant );
	}
}

static void
v_assistant_prepare( GtkAssistant *assistant, GtkWidget *page, gpointer user_data )
{
	g_assert( NACT_IS_ASSISTANT( user_data ));

	if( NACT_ASSISTANT_GET_CLASS( user_data )->on_assistant_prepare ){
		NACT_ASSISTANT_GET_CLASS( user_data )->on_assistant_prepare( NACT_ASSISTANT( user_data ), assistant, page );
	} else {
		do_assistant_prepare( NACT_ASSISTANT( user_data ), assistant, page );
	}
}

static void
on_runtime_init_toplevel( BaseWindow *window )
{
	static const gchar *thisfn = "nact_assistant_on_runtime_init_toplevel";

	/* call parent class at the very beginning */
	if( BASE_WINDOW_CLASS( st_parent_class )->runtime_init_toplevel ){
		BASE_WINDOW_CLASS( st_parent_class )->runtime_init_toplevel( window );
	}

	g_debug( "%s: window=%p", thisfn, window );
	g_assert( NACT_IS_ASSISTANT( window ));

	GtkWindow *toplevel = base_window_get_toplevel_dialog( window );
	g_assert( GTK_IS_ASSISTANT( toplevel ));

	nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( toplevel ), "key-press-event", G_CALLBACK( on_key_pressed_event ));
	nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( toplevel ), "cancel", G_CALLBACK( v_assistant_cancel ));
	nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( toplevel ), "close", G_CALLBACK( v_assistant_close ));
	nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( toplevel ), "prepare", G_CALLBACK( v_assistant_prepare ));
	nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( toplevel ), "apply", G_CALLBACK( v_assistant_apply ));

	nact_assistant_set_warn_on_cancel( NACT_ASSISTANT( window ), TRUE );
}

static gboolean
on_key_pressed_event( GtkWidget *widget, GdkEventKey *event, gpointer user_data )
{
	/*static const gchar *thisfn = "nact_assistant_on_key_pressed_event";
	g_debug( "%s: widget=%p, event=%p, user_data=%p", thisfn, widget, event, user_data );*/

	gboolean stop = FALSE;

	if( event->keyval == GDK_Escape ){
		if( NACT_ASSISTANT_GET_CLASS( user_data )->on_escape_key_pressed ){
			stop = NACT_ASSISTANT_GET_CLASS( user_data )->on_escape_key_pressed( widget, event, user_data );
		}
	}

	return( stop );
}

static gboolean
on_escape_key_pressed( GtkWidget *widget, GdkEventKey *event, gpointer user_data )
{
	static const gchar *thisfn = "nact_assistant_on_escape_key_pressed";
	g_debug( "%s: widget=%p, event=%p, user_data=%p", thisfn, widget, event, user_data );

	GtkWindow *toplevel = base_window_get_toplevel_dialog( BASE_WINDOW( user_data ));
	v_assistant_cancel( GTK_ASSISTANT( toplevel ), user_data );

	return( TRUE );
}

static void
do_assistant_apply( NactAssistant *window, GtkAssistant *assistant )
{
	static const gchar *thisfn = "nact_assistant_do_assistant_apply";
	g_debug( "%s: window=%p, assistant=%p", thisfn, window, assistant );
}

/*
 * the 'Cancel' button is clicked
 */
static void
do_assistant_cancel( NactAssistant *window, GtkAssistant *assistant )
{
	static const gchar *thisfn = "nact_assistant_do_assistant_cancel";
	g_debug( "%s: window=%p, assistant=%p", thisfn, window, assistant );

	gboolean ok = TRUE;

	if( window->private->warn_on_cancel ){
		gchar *first = g_strdup( _( "Are you sure you want to quit this assistant ?" ));
		ok = base_window_yesno_dlg( BASE_WINDOW( window ), GTK_MESSAGE_QUESTION, first, NULL );
		g_free( first );
	}

	if( ok ){
		do_assistant_close( window, assistant );
	}
}

static void
do_assistant_close( NactAssistant *window, GtkAssistant *assistant )
{
	static const gchar *thisfn = "nact_assistant_do_assistant_close";
	g_debug( "%s: window=%p, assistant=%p", thisfn, window, assistant );

	g_object_unref( window );
}

static void
do_assistant_prepare( NactAssistant *window, GtkAssistant *assistant, GtkWidget *page )
{
	static const gchar *thisfn = "nact_assistant_do_assistant_prepare";
	g_debug( "%s: window=%p, assistant=%p, page=%p", thisfn, window, assistant, page );
}
