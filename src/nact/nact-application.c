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
#include <libintl.h>

#include <api/na-core-utils.h>

#include <core/na-iabout.h>
#include <core/na-ipivot-consumer.h>

#include "nact-application.h"
#include "nact-main-window.h"

/* private class data
 */
struct _NactApplicationClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _NactApplicationPrivate {
	gboolean   dispose_has_run;
	NAUpdater *updater;
};

/* private instance properties
 */
enum {
	NACT_APPLICATION_PROP_UPDATER_ID = 1
};

#define NACT_APPLICATION_PROP_UPDATER	"nact-application-prop-updater"

static gboolean     st_non_unique_opt = FALSE;
static gboolean     st_version_opt    = FALSE;

static const gchar *st_application_name = N_( "Nautilus-Actions Configuration Tool" );
static const gchar *st_description      = N_( "A user interface to edit your own contextual actions" );
static const gchar *st_unique_app_name  = "org.nautilus-actions.ConfigurationTool";

static GOptionEntry st_option_entries[] = {
	{ "non-unique", 'n', 0, G_OPTION_ARG_NONE, &st_non_unique_opt,
			N_( "Set it to run multiple instances of the program [unique]" ), NULL },
	{ "version"   , 'v', 0, G_OPTION_ARG_NONE, &st_version_opt,
			N_( "Output the version number, and exit gracefully [no]" ), NULL },
	{ NULL }
};

static BaseApplicationClass *st_parent_class = NULL;

static GType    register_type( void );
static void     class_init( NactApplicationClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void     instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void     instance_dispose( GObject *application );
static void     instance_finalize( GObject *application );

static gboolean appli_manage_options( const BaseApplication *application, int *code );
static GObject *appli_main_window_new( const BaseApplication *application, int *code );

static gchar   *appli_get_gtkbuilder_filename( BaseApplication *application );

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
	GType type;

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

	g_debug( "%s", thisfn );

	type = g_type_register_static( BASE_APPLICATION_TYPE, "NactApplication", &info, 0 );

	return( type );
}

static void
class_init( NactApplicationClass *klass )
{
	static const gchar *thisfn = "nact_application_class_init";
	GObjectClass *object_class;
	GParamSpec *spec;
	BaseApplicationClass *appli_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = BASE_APPLICATION_CLASS( g_type_class_peek_parent( klass ));

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
	object_class->get_property = instance_get_property;
	object_class->set_property = instance_set_property;

	spec = g_param_spec_pointer(
			NACT_APPLICATION_PROP_UPDATER,
			NACT_APPLICATION_PROP_UPDATER,
			"NAUpdater object pointer",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, NACT_APPLICATION_PROP_UPDATER_ID, spec );

	klass->private = g_new0( NactApplicationClassPrivate, 1 );

	appli_class = BASE_APPLICATION_CLASS( klass );
	appli_class->manage_options = appli_manage_options;
	appli_class->main_window_new = appli_main_window_new;

	appli_class->get_ui_filename = appli_get_gtkbuilder_filename;
}

static void
instance_init( GTypeInstance *application, gpointer klass )
{
	static const gchar *thisfn = "nact_application_instance_init";
	NactApplication *self;

	g_return_if_fail( NACT_IS_APPLICATION( application ));

	g_debug( "%s: application=%p (%s), klass=%p",
			thisfn, ( void * ) application, G_OBJECT_TYPE_NAME( application ), ( void * ) klass );

	self = NACT_APPLICATION( application );

	self->private = g_new0( NactApplicationPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	NactApplication *self;

	g_assert( NACT_IS_APPLICATION( object ));
	self = NACT_APPLICATION( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case NACT_APPLICATION_PROP_UPDATER_ID:
				g_value_set_pointer( value, self->private->updater );
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
	NactApplication *self;

	g_assert( NACT_IS_APPLICATION( object ));
	self = NACT_APPLICATION( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case NACT_APPLICATION_PROP_UPDATER_ID:
				self->private->updater = g_value_get_pointer( value );
				break;

			default:
				G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
				break;
		}
	}
}

static void
instance_dispose( GObject *application )
{
	static const gchar *thisfn = "nact_application_instance_dispose";
	NactApplication *self;

	g_return_if_fail( NACT_IS_APPLICATION( application ));

	self = NACT_APPLICATION( application );

	if( !self->private->dispose_has_run ){

		g_debug( "%s: application=%p (%s)", thisfn, ( void * ) application, G_OBJECT_TYPE_NAME( application ));

		self->private->dispose_has_run = TRUE;

		if( self->private->updater ){
			g_object_unref( self->private->updater );
		}

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( application );
		}
	}
}

static void
instance_finalize( GObject *application )
{
	static const gchar *thisfn = "nact_application_instance_finalize";
	NactApplication *self;

	g_return_if_fail( NACT_IS_APPLICATION( application ));

	g_debug( "%s: application=%p (%s)", thisfn, ( void * ) application, G_OBJECT_TYPE_NAME( application ));

	self = NACT_APPLICATION( application );

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
	NactApplication *application;
	gchar *icon_name;

	icon_name = na_iabout_get_icon_name();

	application = g_object_new( NACT_APPLICATION_TYPE,
			BASE_PROP_ARGC,             argc,
			BASE_PROP_ARGV,             argv,
			BASE_PROP_OPTIONS,          st_option_entries,
			BASE_PROP_APPLICATION_NAME, gettext( st_application_name ),
			BASE_PROP_DESCRIPTION,      gettext( st_description ),
			BASE_PROP_ICON_NAME,        icon_name,
			BASE_PROP_UNIQUE_APP_NAME,  st_unique_app_name,
			NULL );

	g_free( icon_name );

	return( application );
}

/*
 * overriden to manage command-line options
 */
static gboolean
appli_manage_options( const BaseApplication *application, int *code )
{
	static const gchar *thisfn = "nact_application_appli_manage_options";
	gboolean ret;

	g_return_val_if_fail( NACT_IS_APPLICATION( application ), FALSE );

	g_debug( "%s: application=%p, code=%p (%d)", thisfn, ( void * ) application, ( void * ) code, *code );

	ret = TRUE;

	if( st_version_opt ){
		na_core_utils_print_version();
		ret = FALSE;
	}
	if( ret && st_non_unique_opt ){
		g_object_set( G_OBJECT( application ), BASE_PROP_UNIQUE_APP_NAME, "", NULL );
	}

	/* call parent class */
	if( ret && BASE_APPLICATION_CLASS( st_parent_class )->manage_options ){
		ret = BASE_APPLICATION_CLASS( st_parent_class )->manage_options( application, code );
	}

	return( ret );
}

/*
 * create the main window
 */
static GObject *
appli_main_window_new( const BaseApplication *application, int *code )
{
	static const gchar *thisfn = "nact_application_appli_main_window_new";
	NactApplication *appli;
	NactMainWindow *main_window;

	g_return_val_if_fail( NACT_IS_APPLICATION( application ), NULL );

	g_debug( "%s: application=%p, code=%p (%d)", thisfn, ( void * ) application, ( void * ) code, *code );

	appli = NACT_APPLICATION( application );

	appli->private->updater = na_updater_new();
	na_pivot_set_loadable( NA_PIVOT( appli->private->updater ), PIVOT_LOAD_ALL );
	na_pivot_load_items( NA_PIVOT( appli->private->updater ));

	main_window = nact_main_window_new( appli );

	na_pivot_register_consumer(
			NA_PIVOT( appli->private->updater ),
			NA_IPIVOT_CONSUMER( main_window ));

	return( G_OBJECT( main_window ));
}

/**
 * nact_application_get_updater:
 * @application: this NactApplication object.
 *
 * Returns a pointer on the #NAUpdater object.
 *
 * The returned pointer is owned by the #NactApplication object.
 * It should not be g_free() not g_object_unref() by the caller.
 */
NAUpdater *
nact_application_get_updater( NactApplication *application )
{
	NAUpdater *updater = NULL;

	g_return_val_if_fail( NACT_IS_APPLICATION( application ), NULL );

	if( !application->private->dispose_has_run ){

		updater = application->private->updater;
	}

	return( updater );
}

static gchar *
appli_get_gtkbuilder_filename( BaseApplication *application )
{
	return( g_strdup( PKGDATADIR "/nautilus-actions-config-tool.ui" ));
}
