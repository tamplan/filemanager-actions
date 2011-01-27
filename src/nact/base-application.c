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
#include <string.h>
#include <unique/unique.h>

#include "base-application.h"
#include "base-window.h"
#include "egg-sm-client.h"

/* private class data
 */
struct _BaseApplicationClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _BaseApplicationPrivate {
	gboolean      dispose_has_run;

	/* properties
	 */
	int           argc;
	GStrv         argv;
	GOptionEntry *options;
	gchar        *application_name;
	gchar        *description;
	gchar        *icon_name;
	gchar        *unique_app_name;

	/* internals
	 */
	EggSMClient  *sm_client;
	gulong        sm_client_quit_handler_id;
	gulong        sm_client_quit_requested_handler_id;
	BaseBuilder  *builder;
	BaseWindow   *main_window;
	UniqueApp    *unique_app_handle;
};

/* instance properties
 */
enum {
	BASE_PROP_0,

	BASE_PROP_ARGC_ID,
	BASE_PROP_ARGV_ID,
	BASE_PROP_OPTIONS_ID,
	BASE_PROP_APPLICATION_NAME_ID,
	BASE_PROP_DESCRIPTION_ID,
	BASE_PROP_ICON_NAME_ID,
	BASE_PROP_UNIQUE_APP_NAME_ID,

	BASE_PROP_N_PROPERTIES
};

static GObjectClass *st_parent_class = NULL;

static GType          register_type( void );
static void           class_init( BaseApplicationClass *klass );
static void           instance_init( GTypeInstance *instance, gpointer klass );
static void           instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void           instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void           instance_dispose( GObject *application );
static void           instance_finalize( GObject *application );

static gboolean       appli_initialize_gtk( BaseApplication *application, int *code );
static gboolean       appli_initialize_manage_options( const BaseApplication *application, int *code );
static gboolean       appli_initialize_unique_app( BaseApplication *application, int *code );
#if 0
static UniqueResponse on_unique_message_received( UniqueApp *app, UniqueCommand command, UniqueMessageData *message, guint time, gpointer user_data );
#endif
static gboolean       appli_initialize_session_manager( BaseApplication *application, int *code );
static void           session_manager_client_quit_cb( EggSMClient *client, BaseApplication *application );
static void           session_manager_client_quit_requested_cb( EggSMClient *client, BaseApplication *application );
static gboolean       appli_initialize_base_application( BaseApplication *application, int *code );

GType
base_application_get_type( void )
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
	static const gchar *thisfn = "base_application_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( BaseApplicationClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( BaseApplication ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	g_type_init();

	type = g_type_register_static( G_TYPE_OBJECT, "BaseApplication", &info, 0 );

	return( type );
}

static void
class_init( BaseApplicationClass *klass )
{
	static const gchar *thisfn = "base_application_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
	object_class->get_property = instance_get_property;
	object_class->set_property = instance_set_property;

	g_object_class_install_property( object_class, BASE_PROP_ARGC_ID,
			g_param_spec_int(
					BASE_PROP_ARGC,
					_( "Arguments count" ),
					_( "The count of command-line arguments" ),
					0, 65535, 0,
					G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	g_object_class_install_property( object_class, BASE_PROP_ARGV_ID,
			g_param_spec_boxed(
					BASE_PROP_ARGV,
					_( "Arguments" ),
					_( "The array of command-line arguments" ),
					G_TYPE_STRV,
					G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	g_object_class_install_property( object_class, BASE_PROP_OPTIONS_ID,
			g_param_spec_pointer(
					BASE_PROP_OPTIONS,
					_( "Option entries" ),
					_( "The array of command-line option definitions" ),
					G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	g_object_class_install_property( object_class, BASE_PROP_APPLICATION_NAME_ID,
			g_param_spec_string(
					BASE_PROP_APPLICATION_NAME,
					_( "Application name" ),
					_( "The name of the application" ),
					"",
					G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	g_object_class_install_property( object_class, BASE_PROP_DESCRIPTION_ID,
			g_param_spec_string(
					BASE_PROP_DESCRIPTION,
					_( "Description" ),
					_( "A short description to be displayed in the first line of --help output" ),
					"",
					G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	g_object_class_install_property( object_class, BASE_PROP_ICON_NAME_ID,
			g_param_spec_string(
					BASE_PROP_ICON_NAME,
					_( "Icon name" ),
					_( "The name of the icon of the application" ),
					"",
					G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	g_object_class_install_property( object_class, BASE_PROP_UNIQUE_APP_NAME_ID,
			g_param_spec_string(
					BASE_PROP_UNIQUE_APP_NAME,
					_( "UniqueApp name" ),
					_( "The Unique name of the application" ),
					"",
					G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	klass->private = g_new0( BaseApplicationClassPrivate, 1 );

	klass->manage_options = NULL;
	klass->main_window_new = NULL;
}

static void
instance_init( GTypeInstance *application, gpointer klass )
{
	static const gchar *thisfn = "base_application_instance_init";
	BaseApplication *self;

	g_return_if_fail( BASE_IS_APPLICATION( application ));

	g_debug( "%s: application=%p (%s), klass=%p",
			thisfn, ( void * ) application, G_OBJECT_TYPE_NAME( application ), ( void * ) klass );

	self = BASE_APPLICATION( application );

	self->private = g_new0( BaseApplicationPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	BaseApplication *self;

	g_return_if_fail( BASE_IS_APPLICATION( object ));
	self = BASE_APPLICATION( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case BASE_PROP_ARGC_ID:
				g_value_set_int( value, self->private->argc );
				break;

			case BASE_PROP_ARGV_ID:
				g_value_set_boxed( value, self->private->argv );
				break;

			case BASE_PROP_OPTIONS_ID:
				g_value_set_pointer( value, self->private->options );
				break;

			case BASE_PROP_APPLICATION_NAME_ID:
				g_value_set_string( value, self->private->application_name );
				break;

			case BASE_PROP_DESCRIPTION_ID:
				g_value_set_string( value, self->private->description );
				break;

			case BASE_PROP_ICON_NAME_ID:
				g_value_set_string( value, self->private->icon_name );
				break;

			case BASE_PROP_UNIQUE_APP_NAME_ID:
				g_value_set_string( value, self->private->unique_app_name );
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
	BaseApplication *self;

	g_return_if_fail( BASE_IS_APPLICATION( object ));
	self = BASE_APPLICATION( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case BASE_PROP_ARGC_ID:
				self->private->argc = g_value_get_int( value );
				break;

			case BASE_PROP_ARGV_ID:
				if( self->private->argv ){
					g_boxed_free( G_TYPE_STRV, self->private->argv );
				}
				self->private->argv = g_value_dup_boxed( value );
				break;

			case BASE_PROP_OPTIONS_ID:
				self->private->options = g_value_get_pointer( value );
				break;

			case BASE_PROP_APPLICATION_NAME_ID:
				g_free( self->private->application_name );
				self->private->application_name = g_value_dup_string( value );
				break;

			case BASE_PROP_DESCRIPTION_ID:
				g_free( self->private->description );
				self->private->description = g_value_dup_string( value );
				break;

			case BASE_PROP_ICON_NAME_ID:
				g_free( self->private->icon_name );
				self->private->icon_name = g_value_dup_string( value );
				break;

			case BASE_PROP_UNIQUE_APP_NAME_ID:
				g_free( self->private->unique_app_name );
				self->private->unique_app_name = g_value_dup_string( value );
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
	static const gchar *thisfn = "base_application_instance_dispose";
	BaseApplication *self;

	g_return_if_fail( BASE_IS_APPLICATION( application ));

	self = BASE_APPLICATION( application );

	if( !self->private->dispose_has_run ){

		g_debug( "%s: application=%p (%s)", thisfn, ( void * ) application, G_OBJECT_TYPE_NAME( application ));

		self->private->dispose_has_run = TRUE;

		if( UNIQUE_IS_APP( self->private->unique_app_handle )){
			g_object_unref( self->private->unique_app_handle );
		}

		if( GTK_IS_BUILDER( self->private->builder )){
			g_object_unref( self->private->builder );
		}

		if( self->private->sm_client_quit_handler_id &&
			g_signal_handler_is_connected( self->private->sm_client, self->private->sm_client_quit_handler_id )){
				g_signal_handler_disconnect( self->private->sm_client, self->private->sm_client_quit_handler_id  );
		}

		if( self->private->sm_client_quit_requested_handler_id &&
			g_signal_handler_is_connected( self->private->sm_client, self->private->sm_client_quit_requested_handler_id )){
				g_signal_handler_disconnect( self->private->sm_client, self->private->sm_client_quit_requested_handler_id  );
		}

		if( self->private->sm_client ){
			g_object_unref( self->private->sm_client );
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
	static const gchar *thisfn = "base_application_instance_finalize";
	BaseApplication *self;

	g_return_if_fail( BASE_IS_APPLICATION( application ));

	g_debug( "%s: application=%p (%s)", thisfn, ( void * ) application, G_OBJECT_TYPE_NAME( application ));

	self = BASE_APPLICATION( application );

	g_free( self->private->application_name );
	g_free( self->private->description );
	g_free( self->private->icon_name );
	g_free( self->private->unique_app_name );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( application );
	}
}

/**
 * base_application_run:
 * @application: this #BaseApplication -derived instance.
 *
 * Starts and runs the application.
 * Takes care of creating, initializing, and running the main window.
 *
 * All steps are implemented as virtual functions which provide some
 * suitable defaults, and may be overriden by a derived class.
 *
 * Returns: an %int code suitable as an exit code for the program.
 *
 * Though it is defined as a virtual function itself, it should be very
 * seldomly needed to override this in a derived class.
 */
int
base_application_run( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_run";
	int code;
	GtkWindow *gtk_toplevel;

	g_return_val_if_fail( BASE_IS_APPLICATION( application ), BASE_EXIT_CODE_START_FAIL );

	code = BASE_EXIT_CODE_START_FAIL;

	if( !application->private->dispose_has_run ){
		g_debug( "%s: application=%p", thisfn, ( void * ) application );

		code = BASE_EXIT_CODE_OK;
		application->private->main_window = NULL;

		if( appli_initialize_gtk( application, &code ) &&
			appli_initialize_manage_options( application, &code ) &&
			appli_initialize_unique_app( application, &code ) &&
			appli_initialize_session_manager( application, &code ) &&
			appli_initialize_base_application( application, &code )){


			if( BASE_APPLICATION_GET_CLASS( application )->main_window_new ){
				application->private->main_window =
						( BaseWindow * ) BASE_APPLICATION_GET_CLASS( application )->main_window_new( application, &code );
			} else {
				code = BASE_EXIT_CODE_MAIN_WINDOW;
			}

			if( application->private->main_window ){
				g_return_val_if_fail( BASE_IS_WINDOW( application->private->main_window ), BASE_EXIT_CODE_START_FAIL );

				if( base_window_init( application->private->main_window )){
					gtk_toplevel = base_window_get_gtk_toplevel( application->private->main_window );
					g_return_val_if_fail( gtk_toplevel, BASE_EXIT_CODE_START_FAIL );
					g_return_val_if_fail( GTK_IS_WINDOW( gtk_toplevel ), BASE_EXIT_CODE_START_FAIL );

					if( application->private->unique_app_handle ){
						unique_app_watch_window( application->private->unique_app_handle, gtk_toplevel );
					}

					g_debug( "%s: invoking base_window_run", thisfn );
					code = base_window_run( application->private->main_window );

				} else {
					g_debug( "%s: base_window_init has returned FALSE", thisfn );
					code = BASE_EXIT_CODE_INIT_FAIL;
				}
			}
		}
	}

	return( code );
}

/*
 * pre-gtk initialization
 *
 * Returns: %TRUE to continue the execution, %FALSE to temrinate the program.
 * The program exit code will be taken from @code.
 */
static gboolean
appli_initialize_gtk( BaseApplication *application, int *code )
{
	static const gchar *thisfn = "base_application_appli_initialize_gtk";
	gchar *name;
	gboolean ret;
	char *parameter_string;
	GError *error;

	g_debug( "%s: application=%p, code=%p (%d)", thisfn, ( void * ) application, ( void * ) code, *code );

	/* initialize i18n
	 */
#ifdef ENABLE_NLS
	bindtextdomain( GETTEXT_PACKAGE, GNOMELOCALEDIR );
# ifdef HAVE_BIND_TEXTDOMAIN_CODESET
	bind_textdomain_codeset( GETTEXT_PACKAGE, "UTF-8" );
# endif
	textdomain( GETTEXT_PACKAGE );
#endif

	gtk_set_locale();

	/* setup default Gtk+ application name
	 * must have been set at instanciationtime by the derived class
	 */
	name = base_application_get_application_name( application );
	if( name && g_utf8_strlen( name, -1 )){
		g_set_application_name( name );
	}
	g_free( name );

	/* manage command-line arguments
	 */
	if( application->private->options ){
		parameter_string = g_strdup( g_get_application_name());
		error = NULL;
		ret = gtk_init_with_args(
				&application->private->argc, ( char *** ) &application->private->argv,
				parameter_string, application->private->options, GETTEXT_PACKAGE, &error );
		if( error ){
			g_warning( "%s: %s", thisfn, error->message );
			g_error_free( error );
			ret = FALSE;
			*code = BASE_EXIT_CODE_ARGS;
		}
		g_free( parameter_string );

	} else {
		ret = gtk_init_check( &application->private->argc, ( char *** ) &application->private->argv );
	}

	return( ret );
}

static gboolean
appli_initialize_manage_options( const BaseApplication *application, int *code )
{
	static const gchar *thisfn = "base_application_appli_initialize_manage_options";
	gboolean ret;

	g_debug( "%s: application=%p, code=%p (%d)", thisfn, ( void * ) application, ( void * ) code, *code );

	ret = TRUE;

	if( BASE_APPLICATION_GET_CLASS( application )->manage_options ){
		ret = BASE_APPLICATION_GET_CLASS( application )->manage_options( application, code );
	}

	return( ret );
}

static gboolean
appli_initialize_unique_app( BaseApplication *application, int *code )
{
	static const gchar *thisfn = "base_application_appli_initialize_unique_app";
	gboolean ret;
	gboolean is_first;
	gchar *msg;

	g_debug( "%s: application=%p, code=%p (%d)", thisfn, ( void * ) application, ( void * ) code, *code );

	ret = TRUE;

	if( application->private->unique_app_name &&
		strlen( application->private->unique_app_name )){

			application->private->unique_app_handle = unique_app_new( application->private->unique_app_name, NULL );
			is_first = !unique_app_is_running( application->private->unique_app_handle );

			if( !is_first ){
				unique_app_send_message( application->private->unique_app_handle, UNIQUE_ACTIVATE, NULL );
				/* i18n: application name */
				msg = g_strdup_printf(
						_( "Another instance of %s is already running.\n"
							"Please switch back to it." ), application->private->application_name );
				base_window_display_error_dlg( NULL, _( "The application is not unique" ), msg );
				g_free( msg );
				ret = FALSE;
				*code = BASE_EXIT_CODE_UNIQUE_APP;
#if 0
			/* default from libunique is actually to activate the first window
			 * so we rely on the default..
			 */
			} else {
				g_signal_connect( application->private->unique_app_handle,
						"message-received", G_CALLBACK( on_unique_message_received ), application );
#endif
			}
	}

	return( ret );
}

#if 0
static UniqueResponse
on_unique_message_received(
		UniqueApp *app, UniqueCommand command, UniqueMessageData *message, guint time, gpointer user_data )
{
	static const gchar *thisfn = "base_application_check_for_unique_app";
	UniqueResponse resp = UNIQUE_RESPONSE_OK;

	switch( command ){
		case UNIQUE_ACTIVATE:
			g_debug( "%s: received message UNIQUE_ACTIVATE", thisfn );
			break;
		default:
			resp = UNIQUE_RESPONSE_PASSTHROUGH;
			break;
	}

	return( resp );
}
#endif

static gboolean
appli_initialize_session_manager( BaseApplication *application, int *code )
{
	static const gchar *thisfn = "base_application_do_initialize_session_manager";

	g_debug( "%s: application=%p, code=%p (%d)", thisfn, ( void * ) application, ( void * ) code, *code );

	egg_sm_client_set_mode( EGG_SM_CLIENT_MODE_NO_RESTART );
	application->private->sm_client = egg_sm_client_get();
	egg_sm_client_startup();
	g_debug( "%s: sm_client=%p", thisfn, ( void * ) application->private->sm_client );

	application->private->sm_client_quit_handler_id =
			g_signal_connect( application->private->sm_client,
					"quit-requested", G_CALLBACK( session_manager_client_quit_requested_cb ), application );

	application->private->sm_client_quit_requested_handler_id =
			g_signal_connect( application->private->sm_client,
					"quit", G_CALLBACK( session_manager_client_quit_cb ), application );

	return( TRUE );
}

static void
session_manager_client_quit_cb( EggSMClient *client, BaseApplication *application )
{
	static const gchar *thisfn = "base_application_session_manager_client_quit_cb";

	g_return_if_fail( BASE_IS_APPLICATION( application ));

	g_debug( "%s: client=%p, application=%p", thisfn, ( void * ) client, ( void * ) application );

	if( application->private->main_window ){

			g_return_if_fail( BASE_IS_WINDOW( application->private->main_window ));
			g_object_unref( application->private->main_window );
			application->private->main_window = NULL;
	}
}

static void
session_manager_client_quit_requested_cb( EggSMClient *client, BaseApplication *application )
{
	static const gchar *thisfn = "base_application_session_manager_client_quit_requested_cb";
	gboolean willing_to = TRUE;

	g_return_if_fail( BASE_IS_APPLICATION( application ));

	g_debug( "%s: client=%p, application=%p", thisfn, ( void * ) client, ( void * ) application );

	if( application->private->main_window ){

			g_return_if_fail( BASE_IS_WINDOW( application->private->main_window ));
			willing_to = base_window_is_willing_to_quit( application->private->main_window );
	}

	egg_sm_client_will_quit( client, willing_to );
}

static gboolean
appli_initialize_base_application( BaseApplication *application, int *code )
{
	static const gchar *thisfn = "base_application_appli_initialize_base_application";

	g_debug( "%s: application=%p, code=%p (%d)", thisfn, ( void * ) application, ( void * ) code, *code );

	/* setup default application icon
	 */
	if( application->private->icon_name &&
		g_utf8_strlen( application->private->icon_name, -1 )){

			gtk_window_set_default_icon_name( application->private->icon_name );
	}

	/* allocate the common BaseBuilder instance
	 */
	application->private->builder = base_builder_new();

	return( TRUE );
}

/**
 * base_application_get_application_name:
 * @application: this #BaseApplication instance.
 *
 * Returns: the application name as a newly allocated string which should
 * be be g_free() by the caller.
 */
gchar *
base_application_get_application_name( const BaseApplication *application )
{
	gchar *name = NULL;

	g_return_val_if_fail( BASE_IS_APPLICATION( application ), NULL );

	if( !application->private->dispose_has_run ){

		name = g_strdup( application->private->application_name );
	}

	return( name );
}

/**
 * base_application_get_builder:
 * @application: this #BaseApplication instance.
 *
 * Returns: the default #BaseBuilder object for the application.
 */
BaseBuilder *
base_application_get_builder( const BaseApplication *application )
{
	BaseBuilder *builder = NULL;

	g_return_val_if_fail( BASE_IS_APPLICATION( application ), NULL );

	if( !application->private->dispose_has_run ){

		builder = application->private->builder;
	}

	return( builder );
}
