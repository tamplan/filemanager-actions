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
#include <glib/gprintf.h>
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
	gchar        *icon_name;
	gchar        *unique_app_name;

	gboolean      is_gtk_initialized;
	UniqueApp    *unique_app_handle;
	int           exit_code;
	gchar        *exit_message1;
	gchar        *exit_message2;
	BaseBuilder  *builder;
	BaseWindow   *main_window;
	EggSMClient  *sm_client;
};

/* instance properties
 */
enum {
	BASE_PROP_0,

	BASE_PROP_ARGC_ID,
	BASE_PROP_ARGV_ID,
	BASE_PROP_OPTIONS_ID,
	BASE_PROP_APPLICATION_NAME_ID,
	BASE_PROP_ICON_NAME_ID,
	BASE_PROP_UNIQUE_APP_NAME_ID,
	BASE_APPLICATION_PROP_IS_GTK_INITIALIZED_ID,
	BASE_APPLICATION_PROP_UNIQUE_APP_HANDLE_ID,
	BASE_APPLICATION_PROP_EXIT_CODE_ID,
	BASE_APPLICATION_PROP_EXIT_MESSAGE1_ID,
	BASE_APPLICATION_PROP_EXIT_MESSAGE2_ID,
	BASE_APPLICATION_PROP_BUILDER_ID,
	BASE_APPLICATION_PROP_MAIN_WINDOW_ID,

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

static gboolean       appli_initialize_i18n( BaseApplication *application, int *code );
static gboolean       appli_initialize_application_name( BaseApplication *application, int *code );

static gboolean       v_initialize( BaseApplication *application );
static gboolean       v_initialize_gtk( BaseApplication *application );
static gboolean       v_manage_options( const BaseApplication *application, int *code );
static gboolean       v_initialize_session_manager( BaseApplication *application );
static gboolean       v_initialize_unique_app( BaseApplication *application );
static gboolean       v_initialize_ui( BaseApplication *application );
static gboolean       v_initialize_default_icon( BaseApplication *application );
#if 0
static gboolean       v_initialize_application_name( BaseApplication *application );
static gboolean       v_initialize_application( BaseApplication *application );
static void           set_initialize_application_error( BaseApplication *application );
static gboolean       application_do_initialize_application( BaseApplication *application );
static void           set_initialize_i18n_error( BaseApplication *application );
static gboolean       application_do_initialize_application_name( BaseApplication *application );
#endif
static int            application_do_run( BaseApplication *application );
static gboolean       application_do_initialize( BaseApplication *application );
static gboolean       application_do_initialize_gtk( BaseApplication *application );
static gboolean       application_do_manage_options( const BaseApplication *application, int *code );
static gboolean       application_do_initialize_session_manager( BaseApplication *application );
static gboolean       application_do_initialize_unique_app( BaseApplication *application );
static gboolean       application_do_initialize_ui( BaseApplication *application );
static gboolean       application_do_initialize_default_icon( BaseApplication *application );

static gboolean       check_for_unique_app( BaseApplication *application );
/*static UniqueResponse on_unique_message_received( UniqueApp *app, UniqueCommand command, UniqueMessageData *message, guint time, gpointer user_data );*/

static void           client_quit_cb( EggSMClient *client, BaseApplication *application );
static void           client_quit_requested_cb( EggSMClient *client, BaseApplication *application );
static gint           display_dlg( BaseApplication *application, GtkMessageType type_message, GtkButtonsType type_buttons, const gchar *first, const gchar *second );
static void           display_error_message( BaseApplication *application );
static gboolean       init_with_args( BaseApplication *application, int *argc, char ***argv, GOptionEntry *entries );
static void           set_initialize_gtk_error( BaseApplication *application );
static void           set_initialize_unique_app_error( BaseApplication *application );
static void           set_initialize_ui_get_fname_error( BaseApplication *application );
static void           set_initialize_ui_add_xml_error( BaseApplication *application, const gchar *filename, GError *error );
static void           set_initialize_default_icon_error( BaseApplication *application );

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
	GParamSpec *spec;

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
					G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	g_object_class_install_property( object_class, BASE_PROP_APPLICATION_NAME_ID,
			g_param_spec_string(
					BASE_PROP_APPLICATION_NAME,
					_( "Application name" ),
					_( "The name of the application" ),
					"",
					G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	g_object_class_install_property( object_class, BASE_PROP_ICON_NAME_ID,
			g_param_spec_string(
					BASE_PROP_ICON_NAME,
					_( "Icon name" ),
					_( "The name of the icon of the application" ),
					"",
					G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	g_object_class_install_property( object_class, BASE_PROP_UNIQUE_APP_NAME_ID,
			g_param_spec_string(
					BASE_PROP_UNIQUE_APP_NAME,
					_( "UniqueApp name" ),
					_( "The Unique name of the application" ),
					"",
					G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	spec = g_param_spec_boolean(
			BASE_APPLICATION_PROP_IS_GTK_INITIALIZED,
			"Gtk+ initialization flag",
			"Has Gtk+ be initialized ?", FALSE,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, BASE_APPLICATION_PROP_IS_GTK_INITIALIZED_ID, spec );

	spec = g_param_spec_pointer(
			BASE_APPLICATION_PROP_UNIQUE_APP_HANDLE,
			"UniqueApp object pointer",
			"A reference to the UniqueApp object if any",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, BASE_APPLICATION_PROP_UNIQUE_APP_HANDLE_ID, spec );

	spec = g_param_spec_int(
			BASE_APPLICATION_PROP_EXIT_CODE,
			"Exit code",
			"Exit code of the application", 0, 65535, 0,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, BASE_APPLICATION_PROP_EXIT_CODE_ID, spec );

	spec = g_param_spec_string(
			BASE_APPLICATION_PROP_EXIT_MESSAGE1,
			"Error message",
			"First line of the error message displayed when exit_code not nul", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, BASE_APPLICATION_PROP_EXIT_MESSAGE1_ID, spec );

	spec = g_param_spec_string(
			BASE_APPLICATION_PROP_EXIT_MESSAGE2,
			"Error message",
			"Second line of the error message displayed when exit_code not nul", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, BASE_APPLICATION_PROP_EXIT_MESSAGE2_ID, spec );

	spec = g_param_spec_pointer(
			BASE_APPLICATION_PROP_BUILDER,
			"UI object pointer",
			"A reference to the UI definition from GtkBuilder",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, BASE_APPLICATION_PROP_BUILDER_ID, spec );

	spec = g_param_spec_pointer(
			BASE_APPLICATION_PROP_MAIN_WINDOW,
			"Main BaseWindow object",
			"A reference to the main BaseWindow object",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, BASE_APPLICATION_PROP_MAIN_WINDOW_ID, spec );

	klass->private = g_new0( BaseApplicationClassPrivate, 1 );

	klass->manage_options = application_do_manage_options;
	klass->main_window_new = NULL;

	klass->run = application_do_run;
	klass->initialize = application_do_initialize;
	klass->initialize_gtk = application_do_initialize_gtk;
	klass->initialize_session_manager = application_do_initialize_session_manager;
	klass->initialize_unique_app = application_do_initialize_unique_app;
	klass->initialize_ui = application_do_initialize_ui;
	klass->initialize_default_icon = application_do_initialize_default_icon;
	klass->initialize_application = NULL;
	klass->get_icon_name = NULL;
	klass->get_unique_app_name = NULL;
	klass->get_ui_filename = NULL;
	klass->get_main_window = NULL;
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
	self->private->exit_code = 0;
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

			case BASE_PROP_ICON_NAME_ID:
				g_value_set_string( value, self->private->icon_name );
				break;

			case BASE_PROP_UNIQUE_APP_NAME_ID:
				g_value_set_string( value, self->private->unique_app_name );
				break;

			case BASE_APPLICATION_PROP_IS_GTK_INITIALIZED_ID:
				g_value_set_boolean( value, self->private->is_gtk_initialized );
				break;

			case BASE_APPLICATION_PROP_UNIQUE_APP_HANDLE_ID:
				g_value_set_pointer( value, self->private->unique_app_handle );
				break;

			case BASE_APPLICATION_PROP_EXIT_CODE_ID:
				g_value_set_int( value, self->private->exit_code );
				break;

			case BASE_APPLICATION_PROP_EXIT_MESSAGE1_ID:
				g_value_set_string( value, self->private->exit_message1 );
				break;

			case BASE_APPLICATION_PROP_EXIT_MESSAGE2_ID:
				g_value_set_string( value, self->private->exit_message2 );
				break;

			case BASE_APPLICATION_PROP_BUILDER_ID:
				g_value_set_pointer( value, self->private->builder );
				break;

			case BASE_APPLICATION_PROP_MAIN_WINDOW_ID:
				g_value_set_pointer( value, self->private->main_window );
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

			case BASE_PROP_ICON_NAME_ID:
				g_free( self->private->icon_name );
				self->private->icon_name = g_value_dup_string( value );
				break;

			case BASE_PROP_UNIQUE_APP_NAME_ID:
				g_free( self->private->unique_app_name );
				self->private->unique_app_name = g_value_dup_string( value );
				break;

			case BASE_APPLICATION_PROP_IS_GTK_INITIALIZED_ID:
				self->private->is_gtk_initialized = g_value_get_boolean( value );
				break;

			case BASE_APPLICATION_PROP_UNIQUE_APP_HANDLE_ID:
				self->private->unique_app_handle = g_value_get_pointer( value );
				break;

			case BASE_APPLICATION_PROP_EXIT_CODE_ID:
				self->private->exit_code = g_value_get_int( value );
				break;

			case BASE_APPLICATION_PROP_EXIT_MESSAGE1_ID:
				g_free( self->private->exit_message1 );
				self->private->exit_message1 = g_value_dup_string( value );
				break;

			case BASE_APPLICATION_PROP_EXIT_MESSAGE2_ID:
				g_free( self->private->exit_message2 );
				self->private->exit_message2 = g_value_dup_string( value );
				break;

			case BASE_APPLICATION_PROP_BUILDER_ID:
				self->private->builder = g_value_get_pointer( value );
				break;

			case BASE_APPLICATION_PROP_MAIN_WINDOW_ID:
				self->private->main_window = g_value_get_pointer( value );
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
	g_free( self->private->icon_name );
	g_free( self->private->unique_app_name );

	g_free( self->private->exit_message1 );
	g_free( self->private->exit_message2 );

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

	g_return_val_if_fail( BASE_IS_APPLICATION( application ), BASE_EXIT_CODE_START_FAIL );

	code = BASE_EXIT_CODE_START_FAIL;

	if( !application->private->dispose_has_run ){
		g_debug( "%s: application=%p", thisfn, ( void * ) application );

		code = BASE_EXIT_CODE_OK;

		if( appli_initialize_i18n( application, &code ) &&
			appli_initialize_application_name( application, &code ) &&
				v_initialize( application ) /*
			appli_initialize_gtk( application, &code ) &&
			appli_initialize_manage_options( application, &code ) &&
			appli_initialize_session_manager( application, &code ) &&
			appli_initialize_unique_app( application, &code ) &&
			appli_initialize_default_icon( application, &code ) &&
			appli_initialize_builder( application, &code ) &&
			appli_initialize_first_window( application, &code )*/){

				code = application_do_run( application );
		}
	}

	return( code );
}

static gboolean
appli_initialize_i18n( BaseApplication *application, int *code )
{
	static const gchar *thisfn = "base_application_appli_initialize_i18n";

	g_debug( "%s: application=%p, code=%p (%d)", thisfn, ( void * ) application, ( void * ) code, *code );

#ifdef ENABLE_NLS
	bindtextdomain( GETTEXT_PACKAGE, GNOMELOCALEDIR );
# ifdef HAVE_BIND_TEXTDOMAIN_CODESET
	bind_textdomain_codeset( GETTEXT_PACKAGE, "UTF-8" );
# endif
	textdomain( GETTEXT_PACKAGE );
#endif

	gtk_set_locale();

	return( TRUE );
}

static gboolean
appli_initialize_application_name( BaseApplication *application, int *code )
{
	static const gchar *thisfn = "base_application_appli_initialize_application_name";
	gchar *name;

	g_debug( "%s: application=%p, code=%p (%d)", thisfn, ( void * ) application, ( void * ) code, *code );

	name = base_application_get_application_name( application );
	if( name && g_utf8_strlen( name, -1 )){
		g_set_application_name( name );
	}
	g_free( name );

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
base_application_get_builder( BaseApplication *application )
{
	/*static const gchar *thisfn = "base_application_get_application_name";
	g_debug( "%s: application=%p", thisfn, application );*/
	BaseBuilder *builder = NULL;

	g_return_val_if_fail( BASE_IS_APPLICATION( application ), NULL );

	if( !application->private->dispose_has_run ){
		builder = application->private->builder;
	}

	return( builder );
}

/**
 * base_application_get_icon_name:
 * @application: this #BaseApplication instance.
 *
 * Asks the #BaseApplication-derived class for its default icon name.
 *
 * Defaults to empty.
 *
 * Returns: a newly allocated string to be g_free() by the caller.
 */
gchar *
base_application_get_icon_name( BaseApplication *application )
{
	/*static const gchar *thisfn = "base_application_get_icon_name";
	g_debug( "%s: icon=%p", thisfn, application );*/
	gchar *name = NULL;

	g_return_val_if_fail( BASE_IS_APPLICATION( application ), NULL );

	if( !application->private->dispose_has_run ){
		if( BASE_APPLICATION_GET_CLASS( application )->get_icon_name ){
			name = BASE_APPLICATION_GET_CLASS( application )->get_icon_name( application );

		} else {
			name = g_strdup( "" );
		}
	}

	return( name );
}

/**
 * base_application_get_unique_app_name:
 * @application: this #BaseApplication instance.
 *
 * Asks the #BaseApplication-derived class for its UniqueApp name if any.
 *
 * Defaults to empty.
 *
 * Returns: a newly allocated string to be g_free() by the caller.
 */
gchar *
base_application_get_unique_app_name( BaseApplication *application )
{
	/*static const gchar *thisfn = "base_application_get_unique_app_name";
	g_debug( "%s: icon=%p", thisfn, application );*/
	gchar *name = NULL;

	g_return_val_if_fail( BASE_IS_APPLICATION( application ), NULL );

	if( !application->private->dispose_has_run ){
		if( BASE_APPLICATION_GET_CLASS( application )->get_unique_app_name ){
			name = BASE_APPLICATION_GET_CLASS( application )->get_unique_app_name( application );

		} else {
			name = g_strdup( "" );
		}
	}

	return( name );
}

/**
 * base_application_get_ui_filename:
 * @application: this #BaseApplication instance.
 *
 * Asks the #BaseApplication-derived class for the filename of the file
 * which contains the XML definition of the user interface.
 *
 * Defaults to empty.
 *
 * Returns: a newly allocated string to be g_free() by the caller.
 */
gchar *
base_application_get_ui_filename( BaseApplication *application )
{
	/*static const gchar *thisfn = "base_application_get_ui_filename";
	g_debug( "%s: icon=%p", thisfn, application );*/
	gchar *name = NULL;

	g_return_val_if_fail( BASE_IS_APPLICATION( application ), NULL );

	if( !application->private->dispose_has_run ){
		if( BASE_APPLICATION_GET_CLASS( application )->get_ui_filename ){
			name = BASE_APPLICATION_GET_CLASS( application )->get_ui_filename( application );

		} else {
			name = g_strdup( "" );
		}
	}

	return( name );
}

/**
 * base_application_message_dlg:
 * @application: this #BaseApplication instance.
 * @message: the message to be displayed.
 *
 * Displays a dialog with only an OK button.
 */
void
base_application_message_dlg( BaseApplication *application, GSList *msg )
{
	GString *string;
	GSList *im;

	if( !application->private->dispose_has_run ){

		string = g_string_new( "" );
		for( im = msg ; im ; im = im->next ){
			if( g_utf8_strlen( string->str, -1 )){
				string = g_string_append( string, "\n" );
			}
			string = g_string_append( string, ( gchar * ) im->data );
		}
		display_dlg( application, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, string->str, NULL );

		g_string_free( string, TRUE );
	}
}

/**
 * base_application_error_dlg:
 * @application: this #BaseApplication instance.
 * @type:
 * @primary: a first message.
 * @secondaru: a second message.
 *
 * Displays an error dialog with only an OK button.
 */
void
base_application_error_dlg( BaseApplication *application,
							GtkMessageType type,
							const gchar *first,
							const gchar *second )
{
	if( !application->private->dispose_has_run ){
		display_dlg( application, type, GTK_BUTTONS_OK, first, second );
	}
}

/**
 * base_application_yesno_dlg:
 * @application: this #BaseApplication instance.
 * @type:
 * @primary: a first message.
 * @secondaru: a second message.
 *
 * Displays a choice dialog, with Yes and No buttons.
 * No button is the default.
 *
 * Returns: %TRUE if user has clicked on Yes button, %FALSE else.
 */
gboolean
base_application_yesno_dlg( BaseApplication *application, GtkMessageType type, const gchar *first, const gchar *second )
{
	gboolean ret = FALSE;
	gint result;

	if( !application->private->dispose_has_run ){

		result = display_dlg( application, type, GTK_BUTTONS_YES_NO, first, second );
		ret = ( result == GTK_RESPONSE_YES );
	}

	return( ret );
}

static gboolean
v_initialize( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_initialize";

	g_debug( "%s: application=%p", thisfn, ( void * ) application );

	return( BASE_APPLICATION_GET_CLASS( application )->initialize( application ));
}

static gboolean
v_initialize_gtk( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_initialize_gtk";
	gboolean ok;

	g_debug( "%s: application=%p", thisfn, ( void * ) application );

	ok = BASE_APPLICATION_GET_CLASS( application )->initialize_gtk( application );

	if( ok ){
		application->private->is_gtk_initialized = TRUE;

	} else {
		set_initialize_gtk_error( application );
	}

	return( ok );
}

static gboolean
v_manage_options( const BaseApplication *application, int *code )
{
	static const gchar *thisfn = "base_application_v_manage_options";
	gboolean ok;

	g_debug( "%s: application=%p", thisfn, ( void * ) application );

	ok = BASE_APPLICATION_GET_CLASS( application )->manage_options( application, code );

	return( ok );
}

static gboolean
v_initialize_session_manager( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_initialize_session_manager";
	gboolean ok;

	g_debug( "%s: application=%p", thisfn, ( void * ) application );

	ok = BASE_APPLICATION_GET_CLASS( application )->initialize_session_manager( application );

	return( ok );
}

static gboolean
v_initialize_unique_app( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_initialize_unique_app";
	gboolean ok;

	g_debug( "%s: application=%p", thisfn, ( void * ) application );

	ok = BASE_APPLICATION_GET_CLASS( application )->initialize_unique_app( application );

	if( !ok ){
		set_initialize_unique_app_error( application );
	}

	return( ok );
}

static gboolean
v_initialize_ui( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_initialize_ui";

	g_debug( "%s: application=%p", thisfn, ( void * ) application );

	return( BASE_APPLICATION_GET_CLASS( application )->initialize_ui( application ));
}

static gboolean
v_initialize_default_icon( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_initialize_default_icon";
	gboolean ok;

	g_debug( "%s: application=%p", thisfn, ( void * ) application );

	ok = BASE_APPLICATION_GET_CLASS( application )->initialize_default_icon( application );

	if( !ok ){
		set_initialize_default_icon_error( application );
	}

	return( ok );
}

#if 0

static gboolean
v_initialize_application_name( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_initialize_application_name";
	gboolean ok;

	g_debug( "%s: application=%p", thisfn, ( void * ) application );

	ok = BASE_APPLICATION_GET_CLASS( application )->initialize_application_name( application );

	return( ok );
}

static gboolean
v_initialize_application( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_initialize_application";
	gboolean ok;

	g_debug( "%s: application=%p", thisfn, ( void * ) application );

	ok = BASE_APPLICATION_GET_CLASS( application )->initialize_application( application );

	if( !ok ){
		set_initialize_application_error( application );
	}

	return( ok );
}

static void
set_initialize_i18n_error( BaseApplication *application )
{
	application->private->exit_code = BASE_APPLICATION_ERROR_I18N;

	application->private->exit_message1 =
		g_strdup( _( "Unable to initialize the internationalization environment." ));
}

static void
set_initialize_application_error( BaseApplication *application )
{
	application->private->exit_code = BASE_APPLICATION_ERROR_MAIN_WINDOW;

	application->private->exit_message1 =
		g_strdup( _( "Unable to get the main window of the application." ));
}
#endif

static int
application_do_run( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_do_run";
	BaseWindow *main_window;
	GtkWindow *gtk_toplevel;

	g_debug( "%s: application=%p", thisfn, ( void * ) application );

	if( v_initialize( application )){

		main_window = NULL;

		if( BASE_APPLICATION_GET_CLASS( application )->main_window_new ){
			main_window = ( BaseWindow * ) BASE_APPLICATION_GET_CLASS( application )->main_window_new( application, &application->private->exit_code );
		}

		if( main_window ){
			g_return_val_if_fail( BASE_IS_WINDOW( main_window ), -1 );

			if( base_window_init( main_window )){

				gtk_toplevel = base_window_get_toplevel( main_window );
				g_assert( gtk_toplevel );
				g_assert( GTK_IS_WINDOW( gtk_toplevel ));

				if( application->private->unique_app_handle ){
					unique_app_watch_window( application->private->unique_app_handle, gtk_toplevel );
				}

				base_window_run( main_window );
			}
		}
	}

	display_error_message( application );

	return( application->private->exit_code );
}

static gboolean
application_do_initialize( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_do_initialize";
	int code;

	g_debug( "%s: application=%p", thisfn, ( void * ) application );

	return(
			v_initialize_gtk( application ) &&
			v_manage_options( application, &code ) &&
			v_initialize_session_manager( application ) &&
			v_initialize_unique_app( application ) &&
			v_initialize_ui( application ) &&
			v_initialize_default_icon( application )
	);
}

static gboolean
application_do_initialize_gtk( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_do_initialize_gtk";
	int argc;
	gpointer argv;
	gboolean ret;
	GOptionEntry *options;

	g_debug( "%s: application=%p", thisfn, ( void * ) application );

	g_object_get( G_OBJECT( application ),
			BASE_PROP_ARGC, &argc,
			BASE_PROP_ARGV, &argv,
			BASE_PROP_OPTIONS, &options,
			NULL );

	if( options ){
		ret = init_with_args( application, &argc, ( char *** ) &argv, options );
	} else {
		ret = gtk_init_check( &argc, ( char *** ) &argv );
	}

	if( ret ){
		application->private->argc = argc;
		application->private->argv = argv;
	}

	return( ret );
}

static gboolean
application_do_manage_options( const BaseApplication *application, int *code )
{
	static const gchar *thisfn = "base_application_do_manage_options";

	g_debug( "%s: application=%p", thisfn, ( void * ) application );

	return( TRUE );
}

static gboolean
application_do_initialize_session_manager( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_do_initialize_session_manager";
	gboolean ret = TRUE;

	g_debug( "%s: application=%p", thisfn, ( void * ) application );

	egg_sm_client_set_mode( EGG_SM_CLIENT_MODE_NO_RESTART );
	application->private->sm_client = egg_sm_client_get();
	egg_sm_client_startup();
	g_debug( "%s: sm_client=%p", thisfn, ( void * ) application->private->sm_client );

	g_signal_connect(
			application->private->sm_client,
	        "quit-requested",
	        G_CALLBACK( client_quit_requested_cb ),
	        application );

	g_signal_connect(
			application->private->sm_client,
	        "quit",
	        G_CALLBACK( client_quit_cb ),
	        application );

	return( ret );
}

static gboolean
application_do_initialize_unique_app( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_do_initialize_unique_app";
	gboolean ret = TRUE;
	gchar *name;

	g_debug( "%s: application=%p", thisfn, ( void * ) application );

	name = base_application_get_unique_app_name( application );

	if( name && strlen( name )){
		application->private->unique_app_handle = unique_app_new( name, NULL );
		ret = check_for_unique_app( application );
	}

	g_free( name );
	return( ret );
}

static gboolean
application_do_initialize_ui( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_do_initialize_ui";
	gboolean ret = TRUE;
	GError *error = NULL;
	gchar *name;

	g_debug( "%s: application=%p", thisfn, ( void * ) application );

	name = base_application_get_ui_filename( application );

	if( !name || !strlen( name )){
		ret = FALSE;
		set_initialize_ui_get_fname_error( application );

	} else {
		application->private->builder = base_builder_new();
		if( !base_builder_add_from_file( application->private->builder, name, &error )){
			ret = FALSE;
			set_initialize_ui_add_xml_error( application, name, error );
			if( error ){
				g_error_free( error );
			}
		}
	}

	g_free( name );
	return( ret );
}

static gboolean
application_do_initialize_default_icon( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_do_initialize_default_icon";
	gchar *name;

	g_debug( "%s: application=%p", thisfn, ( void * ) application );

	name = base_application_get_icon_name( application );

	if( name && strlen( name )){
		gtk_window_set_default_icon_name( name );
	}

	g_free( name );

	return( TRUE );
}

static gboolean
check_for_unique_app( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_check_for_unique_app";
	gboolean is_first = TRUE;

	g_debug( "%s: application=%p", thisfn, ( void * ) application );
	g_assert( BASE_IS_APPLICATION( application ));

	if( unique_app_is_running( application->private->unique_app_handle )){

		is_first = FALSE;

		unique_app_send_message( application->private->unique_app_handle, UNIQUE_ACTIVATE, NULL );

	/* default from libunique is actually to activate the first window
	 * so we rely on the default..
	 */
	/*} else {
		g_signal_connect(
				application->private->unique,
				"message-received",
				G_CALLBACK( on_unique_message_received ),
				application
		);*/
	}

	return( is_first );
}

/*static UniqueResponse
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
}*/

static void
client_quit_cb( EggSMClient *client, BaseApplication *application )
{
	static const gchar *thisfn = "base_application_client_quit_cb";

	g_debug( "%s: client=%p, application=%p", thisfn, ( void * ) client, ( void * ) application );

	if( BASE_IS_WINDOW( application->private->main_window )){
		g_object_unref( application->private->main_window );
		application->private->main_window = NULL;
	}
}

static void
client_quit_requested_cb( EggSMClient *client, BaseApplication *application )
{
	static const gchar *thisfn = "base_application_client_quit_requested_cb";
	gboolean willing_to = TRUE;

	g_debug( "%s: client=%p, application=%p", thisfn, ( void * ) client, ( void * ) application );

	if( BASE_IS_WINDOW( application->private->main_window )){
		willing_to = base_window_is_willing_to_quit( application->private->main_window );
	}

	egg_sm_client_will_quit( client, willing_to );
}

static gint
display_dlg( BaseApplication *application, GtkMessageType type_message, GtkButtonsType type_buttons, const gchar *first, const gchar *second )
{
	GtkWidget *dialog;
	const gchar *name;
	gint result;
	GtkWindow *parent;

	g_assert( BASE_IS_APPLICATION( application ));

	parent = NULL;
	if( application->private->main_window ){
		parent = base_window_get_toplevel( application->private->main_window );
	}

	dialog = gtk_message_dialog_new( parent, GTK_DIALOG_MODAL, type_message, type_buttons, "%s", first );

	if( second && g_utf8_strlen( second, -1 )){
		gtk_message_dialog_format_secondary_text( GTK_MESSAGE_DIALOG( dialog ), "%s", second );
	}

	name = g_get_application_name();

	g_object_set( G_OBJECT( dialog ) , "title", name, NULL );

	result = gtk_dialog_run( GTK_DIALOG( dialog ));

	gtk_widget_destroy( dialog );

	return( result );
}

static void
display_error_message( BaseApplication *application )
{
	if( application->private->exit_message1 && g_utf8_strlen( application->private->exit_message1, -1 )){

		if( application->private->is_gtk_initialized ){
			base_application_error_dlg(
					application,
					GTK_MESSAGE_INFO,
					application->private->exit_message1,
					application->private->exit_message2 );

		} else {
			g_printf( "%s\n", application->private->exit_message1 );
			if( application->private->exit_message2 && g_utf8_strlen( application->private->exit_message2, -1 )){
				g_printf( "%s\n", application->private->exit_message2 );
			}
		}
	}
}

static gboolean
init_with_args( BaseApplication *application, int *argc, char ***argv, GOptionEntry *entries )
{
	static const gchar *thisfn = "base_application_init_with_args";
	gboolean ret;
	char *parameter_string;
	GError *error;

	parameter_string = g_strdup( g_get_application_name());
	error = NULL;

	ret = gtk_init_with_args( argc, argv, parameter_string, entries, GETTEXT_PACKAGE, &error );
	if( error ){
		g_warning( "%s: %s", thisfn, error->message );
		g_error_free( error );
		ret = FALSE;
	}

	g_free( parameter_string );

	return( ret );
}

static void
set_initialize_gtk_error( BaseApplication *application )
{
	application->private->exit_code = BASE_APPLICATION_ERROR_GTK;

	application->private->exit_message1 =
		g_strdup( _( "Unable to initialize the Gtk+ user interface." ));
}

static void
set_initialize_unique_app_error( BaseApplication *application )
{
	application->private->exit_code = BASE_APPLICATION_ERROR_UNIQUE_APP;

	application->private->exit_message1 =
		g_strdup( _( "Another instance of the application is already running." ));
}

static void
set_initialize_ui_get_fname_error( BaseApplication *application )
{
	application->private->exit_code = BASE_APPLICATION_ERROR_UI_FNAME;

	application->private->exit_message1 =
		g_strdup( _( "No filename provided for the UI XML definition." ));
}

static void
set_initialize_ui_add_xml_error( BaseApplication *application, const gchar *filename, GError *error )
{
	application->private->exit_code = BASE_APPLICATION_ERROR_UI_LOAD;

	application->private->exit_message1 =
		/* i18n: Unable to load the XML definition from <filename> */
		g_strdup_printf( _( "Unable to load the XML definition from %s." ), filename );

	if( error && error->message ){
		application->private->exit_message2 = g_strdup( error->message );
	}
}

static void
set_initialize_default_icon_error( BaseApplication *application )
{
	application->private->exit_code = BASE_APPLICATION_ERROR_DEFAULT_ICON;

	application->private->exit_message1 =
		g_strdup( _( "Unable to set the default icon for the application." ));
}
