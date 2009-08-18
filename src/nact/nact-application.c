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

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "nact-application.h"
#include "nact-main-window.h"

#define GLADE_FILENAME				GLADEDIR "/nautilus-actions-config-tool.ui"

/* private class data
 */
struct NactApplicationClassPrivate {
};

/* private instance data
 */
struct NactApplicationPrivate {
	gboolean  dispose_has_run;
	NAPivot  *pivot;
};

/* private instance properties
 */
enum {
	PROP_PIVOT = 1
};

#define PROP_PIVOT_STR					"nact-application-pivot"

static BaseApplicationClass *st_parent_class = NULL;

static GType    register_type( void );
static void     class_init( NactApplicationClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void     instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void     instance_dispose( GObject *application );
static void     instance_finalize( GObject *application );

static gboolean appli_initialize_unique_app( BaseApplication *application );
static gboolean appli_initialize_application( BaseApplication *application );
static gchar   *appli_get_application_name( BaseApplication *application );
static gchar   *appli_get_icon_name( BaseApplication *application );
static gchar   *appli_get_unique_app_name( BaseApplication *application );
static gchar   *appli_get_gtkbuilder_filename( BaseApplication *application );
static GObject *appli_get_main_window( BaseApplication *application );

GType
nact_application_get_type( void )
{
	static GType application_type = 0;

	if( !application_type ){
		application_type = register_type();
	}

	return( application_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "nact_application_register_type";
	g_debug( "%s", thisfn );

	static GTypeInfo info = {
		sizeof( NactApplicationClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactApplication ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	return( g_type_register_static( BASE_APPLICATION_TYPE, "NactApplication", &info, 0 ));
}

static void
class_init( NactApplicationClass *klass )
{
	static const gchar *thisfn = "nact_application_class_init";
	g_debug( "%s: klass=%p", thisfn, klass );

	st_parent_class = BASE_APPLICATION_CLASS( g_type_class_peek_parent( klass ));

	GObjectClass *object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
	object_class->get_property = instance_get_property;
	object_class->set_property = instance_set_property;

	GParamSpec *spec;
	spec = g_param_spec_pointer(
			PROP_PIVOT_STR,
			PROP_PIVOT_STR,
			"NAPivot object pointer",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_PIVOT, spec );

	klass->private = g_new0( NactApplicationClassPrivate, 1 );

	BaseApplicationClass *appli_class = BASE_APPLICATION_CLASS( klass );

	appli_class->application_initialize_unique_app = appli_initialize_unique_app;
	appli_class->application_initialize_application = appli_initialize_application;
	appli_class->application_get_application_name = appli_get_application_name;
	appli_class->application_get_icon_name = appli_get_icon_name;
	appli_class->application_get_unique_app_name = appli_get_unique_app_name;
	appli_class->application_get_ui_filename = appli_get_gtkbuilder_filename;
	appli_class->application_get_main_window = appli_get_main_window;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_application_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );

	g_assert( NACT_IS_APPLICATION( instance ));
	NactApplication *self = NACT_APPLICATION( instance );

	self->private = g_new0( NactApplicationPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	g_assert( NACT_IS_APPLICATION( object ));
	NactApplication *self = NACT_APPLICATION( object );

	switch( property_id ){
		case PROP_PIVOT:
			g_value_set_pointer( value, self->private->pivot );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	g_assert( NACT_IS_APPLICATION( object ));
	NactApplication *self = NACT_APPLICATION( object );

	switch( property_id ){
		case PROP_PIVOT:
			self->private->pivot = g_value_get_pointer( value );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_dispose( GObject *application )
{
	static const gchar *thisfn = "nact_application_instance_dispose";
	g_debug( "%s: application=%p", thisfn, application );

	g_assert( NACT_IS_APPLICATION( application ));
	NactApplication *self = NACT_APPLICATION( application );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		if( self->private->pivot ){
			g_object_unref( self->private->pivot );
		}

		/* chain up to the parent class */
		G_OBJECT_CLASS( st_parent_class )->dispose( application );
	}
}

static void
instance_finalize( GObject *application )
{
	static const gchar *thisfn = "nact_application_instance_finalize";
	g_debug( "%s: application=%p", thisfn, application );

	g_assert( NACT_IS_APPLICATION( application ));
	NactApplication *self = ( NactApplication * ) application;

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( application );
	}
}

/**
 * Returns a newly allocated NactApplication object.
 *
 * @argc: count of command-line arguments.
 *
 * @argv: command-line arguments.
 */
NactApplication *
nact_application_new_with_args( int argc, char **argv )
{
	return(
			g_object_new(
					NACT_APPLICATION_TYPE,
					PROP_APPLICATION_ARGC, argc,
					PROP_APPLICATION_ARGV, argv,
					NULL )
	);
}

/**
 * Returns a pointer on the NAPivot object.
 *
 * @application: this NactApplication object.
 *
 * The returned pointer is owned by the NactApplication object.
 * It should not be freed not unref by the caller.
 */
NAPivot *
nact_application_get_pivot( NactApplication *application )
{
	g_assert( NACT_IS_APPLICATION( application ));
	return( NA_PIVOT( application->private->pivot ));
}

/*
 * overrided to provide a personalized error message
 */
static gboolean
appli_initialize_unique_app( BaseApplication *application )
{
	/* call parent class */
	gboolean ok = st_parent_class->application_initialize_unique_app( application );

	if( !ok ){
		gchar *msg1 = g_strdup( _( "Another instance of Nautilus Actions Configuration Tool is already running." ));
		/* i18n: another instance is already running: second line of error message */
		gchar *msg2 = g_strdup( _( "Please switch back to it." ));

		g_object_set( G_OBJECT( application ),
				PROP_APPLICATION_EXIT_MESSAGE1, msg1,
				PROP_APPLICATION_EXIT_MESSAGE2, msg2,
				NULL );

		g_free( msg2 );
		g_free( msg1 );
	}

	return( ok );
}

/*
 * overrided to complete the initialization of the application
 * note that pivot must be initialized _before_ the main window be created
 */
static gboolean
appli_initialize_application( BaseApplication *application )
{
	NACT_APPLICATION( application )->private->pivot = na_pivot_new( NULL );

	/* call parent class */
	gboolean ok = st_parent_class->application_initialize_application( application );

	return( ok );
}

static gchar *
appli_get_application_name( BaseApplication *application )
{
	static const gchar *thisfn = "nact_application_appli_get_application_name";
	g_debug( "%s: application=%p", thisfn, application );

	/* i18n: this is the application name, used in window title */
	return( g_strdup( _( "Nautilus Actions Configuration Tool" )));
}

static gchar *
appli_get_icon_name( BaseApplication *application )
{
	static const gchar *thisfn = "nact_application_appli_get_icon_name";
	g_debug( "%s: application=%p", thisfn, application );

	return( g_strdup( PACKAGE ));
}

static gchar *
appli_get_unique_app_name( BaseApplication *application )
{
	static const gchar *thisfn = "nact_application_appli_get_unique_app_name";
	g_debug( "%s: application=%p", thisfn, application );

	return( g_strdup( "org.nautilus-actions.ConfigurationTool" ));
}

static gchar *
appli_get_gtkbuilder_filename( BaseApplication *application )
{
	return( g_strdup( GLADE_FILENAME ));
}

/*
 * this should be called only once as base class is supposed to keep
 * the pointer to the main BaseWindow window as a property
 */
static GObject *
appli_get_main_window( BaseApplication *application )
{
	static const gchar *thisfn = "nact_application_appli_get_main_window";
	g_debug( "%s: application=%p", thisfn, application );

	BaseWindow *window = BASE_WINDOW( nact_main_window_new( application ));

	na_pivot_add_consumer(
			NA_PIVOT( nact_application_get_pivot( NACT_APPLICATION( application ))),
			G_OBJECT( window ));

	return( G_OBJECT( window ));
}
