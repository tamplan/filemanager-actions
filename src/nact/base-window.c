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
#include <glade/glade-xml.h>
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
	GtkWindow       *toplevel_window;
	gchar           *glade_fname;
};

/* instance properties
 */
enum {
	PROP_WINDOW_APPLICATION = 1,
	PROP_WINDOW_TOPLEVEL_NAME,
	PROP_WINDOW_TOPLEVEL_WINDOW,
	PROP_WINDOW_GLADE_FILENAME
};

static GObjectClass *st_parent_class = NULL;

static GType       register_type( void );
static void        class_init( BaseWindowClass *klass );
static void        instance_init( GTypeInstance *instance, gpointer klass );
static void        instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void        instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void        instance_dispose( GObject *application );
static void        instance_finalize( GObject *application );

static gchar      *v_get_glade_file( BaseWindow *window );
static void        do_init_window( BaseWindow *window );
static GtkWidget  *do_load_widget( BaseWindow *window, const gchar *name );
static gchar      *do_get_toplevel_name( BaseWindow *window );
static GtkWindow  *do_get_toplevel_window( BaseWindow *window );
static gchar      *do_get_glade_file( BaseWindow *window );
static GtkWidget  *do_get_widget( BaseWindow *window, const gchar *name );

static GHashTable *get_glade_hashtable( void );
static GladeXML   *get_glade_xml_object( const gchar *glade_fname, const gchar *widget_name );
static void        destroy_glade_objects( void );

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

	g_type_init();

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
			PROP_WINDOW_TOPLEVEL_WINDOW_STR,
			PROP_WINDOW_TOPLEVEL_WINDOW_STR,
			"The main GtkWindow attached to this object",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_WINDOW_TOPLEVEL_WINDOW, spec );

	spec = g_param_spec_string(
			PROP_WINDOW_GLADE_FILENAME_STR,
			PROP_WINDOW_GLADE_FILENAME_STR,
			"Glade full pathname", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_WINDOW_GLADE_FILENAME, spec );

	klass->private = g_new0( BaseWindowClassPrivate, 1 );

	klass->init_window = do_init_window;
	klass->load_widget = do_load_widget;
	klass->get_toplevel_name = do_get_toplevel_name;
	klass->get_toplevel_window = do_get_toplevel_window;
	klass->get_glade_file = do_get_glade_file;
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

		case PROP_WINDOW_TOPLEVEL_WINDOW:
			g_value_set_pointer( value, self->private->toplevel_window );
			break;

		case PROP_WINDOW_GLADE_FILENAME:
			g_value_set_string( value, self->private->glade_fname );
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

		case PROP_WINDOW_TOPLEVEL_WINDOW:
			self->private->toplevel_window = g_value_get_pointer( value );
			break;

		case PROP_WINDOW_GLADE_FILENAME:
			g_free( self->private->glade_fname );
			self->private->glade_fname = g_value_dup_string( value );
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

		gtk_widget_destroy( GTK_WIDGET( self->private->toplevel_window ));
		destroy_glade_objects();
		gtk_main_quit ();

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
	g_free( self->private->glade_fname );

	/* chain call to parent class */
	if( st_parent_class->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( window );
	}
}

/**
 * Returns a newly allocated BaseWindow object.
 */
BaseWindow *
base_window_new( void )
{
	return( g_object_new( BASE_WINDOW_TYPE, NULL ));
}

/**
 * Initializes the window.
 *
 * @window: this BaseWindow object.
 */
void
base_window_init_window( BaseWindow *window )
{
	g_assert( BASE_IS_WINDOW( window ));
	BASE_WINDOW_GET_CLASS( window )->init_window( window );
}

/**
 * Loads the required widget from a glade file.
 * Uses preferences for sizing and positionning the window.
 *
 * @window: this BaseWindow object.
 *
 * @name: name of the widget
 */
GtkWidget *
base_window_load_widget( BaseWindow *window, const gchar *name )
{
	g_assert( BASE_IS_WINDOW( window ));
	return( BASE_WINDOW_GET_CLASS( window )->load_widget( window, name ));
}

/**
 * Returns the internal name of the GtkWindow attached to this
 * BaseWindow object.
 *
 * @window: this BaseWindow object.
 */
gchar *
base_window_get_toplevel_name( BaseWindow *window )
{
	g_assert( BASE_IS_WINDOW( window ));
	return( BASE_WINDOW_GET_CLASS( window )->get_toplevel_name( window ));
}

/**
 * Returns the GtkWindow attached to this BaseWindow object.
 *
 * @window: this BaseWindow object.
 */
GtkWindow *
base_window_get_toplevel_window( BaseWindow *window )
{
	g_assert( BASE_IS_WINDOW( window ));
	return( BASE_WINDOW_GET_CLASS( window )->get_toplevel_window( window ));
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
v_get_glade_file( BaseWindow *window )
{
	g_assert( BASE_IS_WINDOW( window ));

	gchar *name;
	g_object_get( G_OBJECT( window ), PROP_WINDOW_GLADE_FILENAME_STR, &name, NULL );

	if( !name || !strlen( name )){
		name = BASE_WINDOW_GET_CLASS( window )->get_glade_file( window );
		if( name && strlen( name )){
			g_object_set( G_OBJECT( window ), PROP_WINDOW_GLADE_FILENAME_STR, name, NULL );
		}
	}

	return( name );
}

static void
do_init_window( BaseWindow *window )
{
	/* do nothing */
}

static GtkWidget *
do_load_widget( BaseWindow *window, const gchar *name )
{
	GtkWidget *widget = NULL;

	gchar *glade_file = v_get_glade_file( window );

	if( glade_file && strlen( glade_file )){
		GladeXML *xml = get_glade_xml_object( glade_file, name );
		if( xml ){
			glade_xml_signal_autoconnect( xml );
			widget = glade_xml_get_widget( xml, name );
			g_object_unref( xml );
		}
	}

	if( widget ){
		g_object_set( G_OBJECT( window ),
				PROP_WINDOW_TOPLEVEL_NAME_STR, name,
				PROP_WINDOW_TOPLEVEL_WINDOW_STR, widget,
				NULL );

		g_object_set_data( G_OBJECT( widget ), "base-window", window );

	} else {
		gchar *msg = g_strdup_printf( _( "Unable to load %s widget from %s glade file." ), name, glade_file );
		base_application_error_dlg( window->private->application, GTK_MESSAGE_ERROR, msg, NULL );
		g_free( msg );
		exit( 1 );
	}

	g_free( glade_file );
	return( widget );
}

static gchar *
do_get_toplevel_name( BaseWindow *window )
{
	return( g_strdup( window->private->toplevel_name ));
}

static GtkWindow *
do_get_toplevel_window( BaseWindow *window )
{
	return( window->private->toplevel_window );
}

static gchar *
do_get_glade_file( BaseWindow *window )
{
	return( NULL );
}

static GtkWidget *
do_get_widget( BaseWindow *window, const gchar *name )
{
	gchar *glade_file = v_get_glade_file( window );
	gchar *parent_name = base_window_get_toplevel_name( window );
	GladeXML *xml = get_glade_xml_object( glade_file, parent_name );
	GtkWidget *widget = glade_xml_get_widget( xml, name );
	g_free( parent_name );
	g_object_unref( xml );
	return( widget );
}

static GHashTable *
get_glade_hashtable( void )
{
	static GHashTable* glade_hash = NULL;

	if( !glade_hash ){
		glade_hash = g_hash_table_new_full( g_str_hash, g_str_equal, g_free, g_object_unref );
	}

	return( glade_hash );
}

static GladeXML *
get_glade_xml_object( const gchar *glade_fname, const gchar *widget_name )
{
	GHashTable *glade_hash = get_glade_hashtable();
	GladeXML *xml = NULL;

	xml = ( GladeXML * ) g_hash_table_lookup( glade_hash, widget_name );
	if( !xml ){
		xml = glade_xml_new( glade_fname, widget_name, NULL );
		g_hash_table_insert( glade_hash, g_strdup( widget_name ), xml );
	}

	return( GLADE_XML( g_object_ref( xml )));
}

static void
destroy_glade_objects( void )
{
	GHashTable *glade_hash = get_glade_hashtable();
	g_hash_table_destroy( glade_hash );
}
