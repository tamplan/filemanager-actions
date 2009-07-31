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
#include <string.h>
#include <unique/unique.h>

#include "base-application.h"
#include "base-window.h"

/* private class data
 */
struct BaseApplicationClassPrivate {
};

/* private instance data
 */
struct BaseApplicationPrivate {
	gboolean    dispose_has_run;

	/**
	 * @argc: count of arguments in command-line.
	 * @argv: list of command-line arguments
	 *
	 * These two variables must be provided before running the
	 * initialization process ; they are required in order to correctly
	 * initialize the Gtk+ user interface.
	 */
	int         argc;
	gpointer    argv;

	/**
	 * @is_gtk_initialized: set to %TRUE after successfully returning
	 * from the application_initialize_gtk() virtual function.
	 *
	 * While this flag is not %TRUE, error messages are printed to
	 * stdout. When %TRUE, error messages are displayed with a dialog
	 * box.
	 */
	gboolean    is_gtk_initialized;

	/**
	 * @unique_app_handle: the UniqueApp object allocated if the
	 * derived-class has provided a @unique_app_name. Rather for
	 * internal use.
	 */
	UniqueApp  *unique_app_handle;

	/**
	 * @exit_code: the code which will be returned by the program to
	 * the operating system.
	 * @exit_message: the message which will be displayed at program
	 * terminaison if @exit_code is not zero.
	 *
	 * See @is_gtk_initialized for how the @exit_message is actually
	 * displayed.
	 */
	int         exit_code;
	gchar      *exit_message;

	/**
	 * @ui_xml: the GtkBuilder object allocated to handle the UI
	 * definition. Rather for internal use.
	 */
	GtkBuilder *ui_xml;

	/**
	 * @main_window: as its name says...
	 */
	BaseWindow *main_window;
};

/* instance properties
 */
enum {
	PROP_APPLICATION_ARGC_ID = 1,
	PROP_APPLICATION_ARGV_ID,
	PROP_APPLICATION_IS_GTK_INITIALIZED_ID,
	PROP_APPLICATION_UNIQUE_APP_HANDLE_ID,
	PROP_APPLICATION_EXIT_CODE_ID,
	PROP_APPLICATION_EXIT_MESSAGE_ID,
	PROP_APPLICATION_UI_XML_ID,
	PROP_APPLICATION_MAIN_WINDOW_ID
};

static GObjectClass *st_parent_class = NULL;

static GType          register_type( void );
static void           class_init( BaseApplicationClass *klass );
static void           instance_init( GTypeInstance *instance, gpointer klass );
static void           instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void           instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void           instance_dispose( GObject *application );
static void           instance_finalize( GObject *application );

static gboolean       v_initialize( BaseApplication *application );
static gboolean       v_initialize_i18n( BaseApplication *application );
static gboolean       v_initialize_gtk( BaseApplication *application );
static gboolean       v_initialize_application( BaseApplication *application );
static gboolean       v_initialize_unique_app( BaseApplication *application );
static gboolean       v_initialize_ui( BaseApplication *application );
static gchar         *v_get_unique_app_name( BaseApplication *application );
static gchar         *v_get_ui_filename( BaseApplication *application );

static int            do_application_run( BaseApplication *application );
static void           do_application_initialize( BaseApplication *application );
static void           do_application_initialize_i18n( BaseApplication *application );
static void           do_application_initialize_gtk( BaseApplication *application );
static void           do_application_initialize_application( BaseApplication *application );
static void           do_application_initialize_unique_app( BaseApplication *application );
static void           do_application_initialize_ui( BaseApplication *application );

static gboolean       check_for_unique_app( BaseApplication *application );
/*static UniqueResponse on_unique_message_received( UniqueApp *app, UniqueCommand command, UniqueMessageData *message, guint time, gpointer user_data );*/
static gint           display_dlg( BaseApplication *application, GtkMessageType type_message, GtkButtonsType type_buttons, const gchar *first, const gchar *second );
static GtkWidget     *recursive_search_for_child( BaseApplication *application, GtkWindow *toplevel, const gchar *name );
static GtkWidget     *search_for_child_widget( GtkContainer *container, const gchar *name );

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
	g_debug( "%s", thisfn );

	g_type_init();

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

	return( g_type_register_static( G_TYPE_OBJECT, "BaseApplication", &info, 0 ));
}

static void
class_init( BaseApplicationClass *klass )
{
	static const gchar *thisfn = "base_application_class_init";
	g_debug( "%s: klass=%p", thisfn, klass );

	st_parent_class = g_type_class_peek_parent( klass );

	GObjectClass *object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
	object_class->get_property = instance_get_property;
	object_class->set_property = instance_set_property;

	GParamSpec *spec;
	spec = g_param_spec_int(
			PROP_APPLICATION_ARGC,
			"Command-line arguments count",
			"Command-line arguments count", 0, 65535, 0,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_APPLICATION_ARGC_ID, spec );

	spec = g_param_spec_pointer(
			PROP_APPLICATION_ARGV,
			"Command-line arguments",
			"Command-line arguments",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_APPLICATION_ARGV_ID, spec );

	spec = g_param_spec_boolean(
			PROP_APPLICATION_IS_GTK_INITIALIZED,
			"Gtk+ initialization flag",
			"Has Gtk+ be initialized ?", FALSE,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_APPLICATION_IS_GTK_INITIALIZED_ID, spec );

	spec = g_param_spec_pointer(
			PROP_APPLICATION_UNIQUE_APP_HANDLE,
			"UniqueApp object pointer",
			"UniqueApp object pointer",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_APPLICATION_UNIQUE_APP_HANDLE_ID, spec );

	spec = g_param_spec_int(
			PROP_APPLICATION_EXIT_CODE,
			"Exit code",
			"Exit code of the application", 0, 65535, 0,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_APPLICATION_CODE_ID, spec );

	spec = g_param_spec_string(
			PROP_APPLICATION_EXIT_MESSAGE,
			"Error message",
			"Error message displayed when exit_code not nul", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_APPLICATION_EXIT_MESSAGE_ID, spec );

	spec = g_param_spec_pointer(
			PROP_APPLICATION_UI_XML,
			"UI object pointer",
			"UI definition object pointer",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_APPLICATION_UI_XML_ID, spec );

	spec = g_param_spec_pointer(
			PROP_APPLICATION_MAIN_WINDOW,
			"Main BaseWindow object",
			"Main BaseWindow object",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_APPLICATION_MAIN_WINDOW_ID, spec );

	klass->private = g_new0( BaseApplicationClassPrivate, 1 );

	klass->application_run = do_application_run;
	klass->application_initialize = do_application_initialize;
	klass->application_initialize_i18n = do_application_initialize_i18n;
	klass->application_initialize_gtk = do_application_initialize_gtk;
	klass->application_initialize_application = do_application_initialize_application;
	klass->application_initialize_unique_app = do_application_initialize_unique_app;
	klass->application_initialize_ui = do_application_initialize_ui;
	klass->application_get_application_name = NULL;
	klass->application_get_icon_name = NULL;
	klass->application_get_unique_app_name = NULL;
	klass->application_get_ui_filename = NULL;
	klass->application_get_main_window = NULL;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "base_application_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );

	g_assert( BASE_IS_APPLICATION( instance ));
	BaseApplication *self = BASE_APPLICATION( instance );

	self->private = g_new0( BaseApplicationPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->code = 0;
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	g_assert( BASE_IS_APPLICATION( object ));
	BaseApplication *self = BASE_APPLICATION( object );

	switch( property_id ){
		case PROP_APPLICATION_ARGC_ID:
			g_value_set_int( value, self->private->argc );
			break;

		case PROP_APPLICATION_ARGV_ID:
			g_value_set_pointer( value, self->private->argv );
			break;

		case PROP_APPLICATION_IS_GTK_INITIALIZED_ID:
			g_value_set_boolean( value, self->private->is_gtk_initialized );
			break;

		case PROP_APPLICATION_UNIQUE_APP_HANDLE_ID:
			g_value_set_pointer( value, self->private->unique_app_handle );
			break;

		case PROP_APPLICATION_EXIT_CODE_ID:
			g_value_set_int( value, self->private->exit_code );
			break;

		case PROP_APPLICATION_EXIT_MESSAGE_ID:
			g_value_set_string( value, self->private->exit_message );
			break;

		case PROP_APPLICATION_UI_XML_ID:
			g_value_set_pointer( value, self->private->ui_xml );
			break;

		case PROP_APPLICATION_MAIN_WINDOW_ID:
			g_value_set_pointer( value, self->private->main_window );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	g_assert( BASE_IS_APPLICATION( object ));
	BaseApplication *self = BASE_APPLICATION( object );

	switch( property_id ){
		case PROP_APPLICATION_ARGC_ID:
			self->private->argc = g_value_get_int( value );
			break;

		case PROP_APPLICATION_ARGV_ID:
			self->private->argv = g_value_get_pointer( value );
			break;

		case PROP_APPLICATION_IS_GTK_INITIALIZED_ID:
			self->private->argv = g_value_get_boolean( value );
			break;

		case PROP_APPLICATION_UNIQUE_APP_HANDLE_ID:
			self->private->unique_app_handle = g_value_get_pointer( value );
			break;

		case PROP_APPLICATION_EXIT_CODE_ID:
			self->private->exit_code = g_value_get_int( value );
			break;

		case PROP_APPLICATION_EXIT_MESSAGE_ID:
			g_free( self->private->exit_message );
			self->private->exit_message = g_value_dup_string( value );
			break;

		case PROP_APPLICATION_UI_XML_ID:
			self->private->ui_xml = g_value_get_pointer( value );
			break;

		case PROP_APPLICATION_MAIN_WINDOW_ID:
			self->private->main_window = g_value_get_pointer( value );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_dispose( GObject *application )
{
	static const gchar *thisfn = "base_application_instance_dispose";
	g_debug( "%s: application=%p", thisfn, application );

	g_assert( BASE_IS_APPLICATION( application ));
	BaseApplication *self = BASE_APPLICATION( application );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		if( UNIQUE_IS_APP( self->private->unique_app_handle )){
			g_object_unref( self->private->unique_app_handle );
		}

		g_object_unref( self->private->ui_xml );

		/* chain up to the parent class */
		G_OBJECT_CLASS( st_parent_class )->dispose( application );
	}
}

static void
instance_finalize( GObject *application )
{
	static const gchar *thisfn = "base_application_instance_finalize";
	g_debug( "%s: application=%p", thisfn, application );

	g_assert( BASE_IS_APPLICATION( application ));
	BaseApplication *self = ( BaseApplication * ) application;

	g_free( self->private->exit_message );

	g_free( self->private );

	/* chain call to parent class */
	if( st_parent_class->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( application );
	}
}
/**
 * base_application_run:
 * @application: this #BaseApplication instance.
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
	g_debug( "%s: application=%p", thisfn, application );
	g_assert( BASE_IS_APPLICATION( application ));

	if( BASE_APPLICATION_GET_CLASS( application )){
		return( BASE_APPLICATION_GET_CLASS( application )->application_run( application ));
	}

	g_assert_not_reached();
	return( 0 ); /* so that gcc is happy */
}

/**
 * base_application_get_application_name:
 * @application: this #BaseApplication instance.
 *
 * Asks the #BaseApplication-derived class for its localized
 * application name.
 *
 * Defaults to empty.
 *
 * Returns: a newly allocated string to be g_free() by the caller.
 */
gchar *
base_application_get_application_name( BaseApplication *application )
{
	/*static const gchar *thisfn = "base_application_get_application_name";
	g_debug( "%s: application=%p", thisfn, application );*/

	g_assert( BASE_IS_APPLICATION( application ));

	gchar *name = NULL;

	if( BASE_APPLICATION_GET_CLASS( application )->application_get_application_name( application )){
		name = BASE_APPLICATION_GET_CLASS( application )->application_get_application_name( application );

	} else {
		name = g_strdup( "" );
	}

	return( name );
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

	g_assert( BASE_IS_APPLICATION( application ));

	gchar *name = NULL;

	if( BASE_APPLICATION_GET_CLASS( application )->application_get_icon_name( application )){
		name = BASE_APPLICATION_GET_CLASS( application )->application_get_icon_name( application );

	} else {
		name = g_strdup( "" );
	}

	return( name );
}

/**
 * base_application_get_main_window:
 * @application: this #BaseApplication instance.
 *
 * Returns: a pointer to the #BaseWindow-derived object which serves as
 * the main window of the application.
 *
 * The returned pointer should not be g_free() nor g_object_unref() by
 * the caller.
 *
 * When first calling, #BaseApplication asks for its derived class to
 * allocate a new object. This same object is then returned on
 * subsequent calls.
 */
BaseWindow *
base_application_get_main_window( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_get_main_window";
	g_debug( "%s: icon=%p", thisfn, application );

	g_assert( BASE_IS_APPLICATION( application ));

	if( !application->private->main_window &&
		BASE_APPLICATION_GET_CLASS( application )->application_get_main_window( application )){
			application->private->main_window = BASE_WINDOW( BASE_APPLICATION_GET_CLASS( application )->application_get_main_window( application ));
	}

	return( application->private->main_window );
}

/**
 * base_application_get_dialog:
 * @application: this #BaseApplication instance.
 * @name: the name of the searched toplevel dialog.
 *
 * This function provides a pointer to the toplevel dialog associated
 * with the specified #BaseWindow.
 *
 * Returns: a pointer to the named dialog, or NULL.
 * This pointer is owned by GtkBuilder instance, and must not be
 * g_free() nor g_object_unref() by the caller.
 */
GtkWindow *
base_application_get_dialog( BaseApplication *application, const gchar *name )
{
	/*static const gchar *thisfn = "base_application_get_dialog";
	g_debug( "%s: application=%p, name=%s", thisfn, application, name );*/

	GtkWindow *dialog = GTK_WINDOW( gtk_builder_get_object( application->private->ui_xml, name ));

	if( !dialog ){
		gchar *msg = g_strdup_printf(
				_( "Unable to load %s dialog from %s glade file." ), name, application->private->ui_fname );
		base_application_error_dlg( application, GTK_MESSAGE_ERROR, msg, NULL );
		g_free( msg );
		g_object_set( G_OBJECT( application ), PROP_APPLICATION_CODE_STR, 1, NULL );

	} else {
		g_assert( GTK_IS_WINDOW( dialog ));
	}

	return( dialog );
}

/**
 * base_application_get_widget:
 * @application: this #BaseApplication instance.
 * @window: a #BaseWindow-derived document.
 * @name: the name of the searched widget.
 *
 * Returns a pointer to the named widget which is a child of the
 * toplevel #GtkWindow associated with @window.
 *
 * Returns: a pointer to the searched widget, or NULL.
 * This pointer is owned by GtkBuilder instance, and must not be
 * g_free() nor g_object_unref() by the caller.
 */
GtkWidget *
base_application_get_widget( BaseApplication *application, BaseWindow *window, const gchar *name )
{
	/*static const gchar *thisfn = "base_application_get_widget";
	g_debug( "%s: application=%p, name=%s", thisfn, application, name );*/

	GtkWindow *toplevel = base_window_get_toplevel_dialog( window );

	return( base_application_search_for_widget( application, toplevel, name ));
}

/**
 * base_application_search_for_widget:
 * @application: this #BaseApplication instance.
 * @window: a #GtkWindow toplevel dialog.
 * @name: the name of the searched widget.
 *
 * Returns a pointer to the named widget which is a child of @window.
 *
 * Returns: a pointer to the searched widget, or NULL.
 * This pointer is owned by GtkBuilder instance, and must not be
 * g_free() nor g_object_unref() by the caller.
 */
GtkWidget *
base_application_search_for_widget( BaseApplication *application, GtkWindow *window, const gchar *name )
{
	/*static const gchar *thisfn = "base_application_get_widget";
	g_debug( "%s: application=%p, name=%s", thisfn, application, name );*/

	GtkWidget *widget = recursive_search_for_child( application, window, name );

	if( widget ){
		g_assert( GTK_IS_WIDGET( widget ));
	}

	return( widget );
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
base_application_error_dlg(
		BaseApplication *application, GtkMessageType type, const gchar *primary, const gchar *secondary )
{
	display_dlg( application, type, GTK_BUTTONS_OK, primary, secondary );
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
	gint result = display_dlg( application, type, GTK_BUTTONS_YES_NO, first, second );
	return( result == GTK_RESPONSE_YES );
}

static gboolean
v_initialize( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_initialize";
	g_debug( "%s: application=%p", thisfn, application );

	if( BASE_APPLICATION_GET_CLASS( application )->application_initialize ){
		return( BASE_APPLICATION_GET_CLASS( application )->application_initialize( application ));
	}

	g_assert_not_reached();
	return( FALSE ); /* so that gcc is happy */
}

static gboolean
v_initialize_i18n( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_initialize_i18n";
	g_debug( "%s: application=%p", thisfn, application );

	if( BASE_APPLICATION_GET_CLASS( application )->application_initialize_i18n ){
		return( BASE_APPLICATION_GET_CLASS( application )->application_initialize_i18n( application ));
	}

	g_assert_not_reached();
	return( FALSE ); /* so that gcc is happy */
}

static gboolean
v_initialize_gtk( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_initialize_gtk";
	g_debug( "%s: application=%p", thisfn, application );

	if( BASE_APPLICATION_GET_CLASS( application )->application_initialize_gtk ){
		gboolean ret = BASE_APPLICATION_GET_CLASS( application )->application_initialize_gtk( application );
		if( ret ){
			application->private->is_gtk_initialized = TRUE;
		}
		return( ret );
	}

	g_assert_not_reached();
	return( FALSE ); /* so that gcc is happy */
}

static gboolean
v_initialize_application( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_initialize_application_name";
	g_debug( "%s: application=%p", thisfn, application );

	if( BASE_APPLICATION_GET_CLASS( application )->application_initialize_application ){
		return( BASE_APPLICATION_GET_CLASS( application )->application_initialize_application( application ));
	}

	g_assert_not_reached();
	return( FALSE ); /* so that gcc is happy */
}

static gboolean
v_initialize_unique_app( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_initialize_unique_app";
	g_debug( "%s: application=%p", thisfn, application );

	if( BASE_APPLICATION_GET_CLASS( application )->application_initialize_unique_app ){
		return( BASE_APPLICATION_GET_CLASS( application )->application_initialize_unique_app( application ));
	}

	g_assert_not_reached();
	return( FALSE ); /* so that gcc is happy */
}

static gboolean
v_initialize_ui( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_initialize_ui";
	g_debug( "%s: application=%p", thisfn, application );

	if( BASE_APPLICATION_GET_CLASS( application )->application_initialize_ui ){
		return( BASE_APPLICATION_GET_CLASS( application )->application_initialize_ui( application ));
	}

	g_assert_not_reached();
	return( FALSE ); /* so that gcc is happy */
}

static gchar *
v_get_unique_app_name( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_get_unique_name";
	g_debug( "%s: application=%p", thisfn, application );

	g_assert( BASE_IS_APPLICATION( application ));

	gchar *name = NULL;

	if( BASE_APPLICATION_GET_CLASS( application )->application_get_unique_app_name( application )){
		name = BASE_APPLICATION_GET_CLASS( application )->application_get_unique_app_name( application );

	} else {
		name = g_strdup( "" );
	}

	return( name );
}

static gchar *
v_get_ui_filename( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_get_ui_filename";
	g_debug( "%s: application=%p", thisfn, application );

	g_assert( BASE_IS_APPLICATION( application ));

	gchar *name = NULL;

	if( BASE_APPLICATION_GET_CLASS( application )->application_get_ui_filename( application )){
		name = BASE_APPLICATION_GET_CLASS( application )->application_get_ui_filename( application );

	} else {
		name = g_strdup( "" );
	}

	return( name );
}

static int
do_application_run( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_do_application_run";
	g_debug( "%s: application=%p", thisfn, application );

	if( v_initialize( application )){

		g_assert( application->private->main_window );
		g_assert( BASE_IS_WINDOW( application->private->main_window ));

		base_window_init( window );

		GtkWindow *wnd = base_window_get_toplevel_dialog( window );
		g_assert( wnd );
		g_assert( GTK_IS_WINDOW( wnd ));

		if( application->private->unique_app ){
			unique_app_watch_window( application->private->unique_app_handle, wnd );
		}

		base_window_run( window );
	}

	return( application->private->exit_code );
}

static gboolean
do_application_initialize( BaseApplication *application )
{
	return(
			v_initialize_i18n( application ) &&
			v_initialize_gtk( application ) &&
			v_initialize_application( application ) &&
			v_initialize_unique_app( application ) &&
			v_initialize_ui( application )
	);
}

static gboolean
do_application_initialize_i18n( BaseApplication *application )
{
#ifdef ENABLE_NLS
	bindtextdomain( GETTEXT_PACKAGE, GNOMELOCALEDIR );
# ifdef HAVE_BIND_TEXTDOMAIN_CODESET
	bind_textdomain_codeset( GETTEXT_PACKAGE, "UTF-8" );
# endif
	textdomain( GETTEXT_PACKAGE );
#endif
	return( TRUE );
}

static gboolean
do_application_initialize_gtk( BaseApplication *application )
{
	int argc;
	gpointer argv;
	gboolean ret;

	g_object_get( G_OBJECT( application ), PROP_APPLICATION_ARGC, &argc, PROP_APPLICATION_ARGV, &argv, NULL );

	ret = gtk_init_check( &argc, ( char *** ) &argv );

	if( ret ){
		g_object_set( G_OBJECT( application ), PROP_APPLICATION_ARGC, argc, PROP_APPLICATION_ARGV, argv, NULL );

	} else {
		application->private->exit_code = APPLICATION_ERROR_GTK;
		application->private->exit_message =
			g_strdup( _( "Unable to initialize the Gtk+ user interface." ));
	}

	return( ret );
}

static gboolean
do_application_initialize_application( BaseApplication *application )
{
	BaseWindow *window;
	gboolean ret = TRUE;

	window = base_application_get_main_window( application );
	if( !window ){
		ret = FALSE;
		application->private->exit_code = APPLICATION_ERROR_MAIN_WINDOW;
		application->private->exit_message =
			g_strdup( _( "Unable to get the main window of the application." ));
	}

	return( ret );
}

static gboolean
do_application_initialize_unique_app( BaseApplication *application )
{
	gboolean ret = TRUE;

	gchar *name = v_get_unique_app_name( application );
	if( name && strlen( name )){
		application->private->unique_app_handle = unique_app_new( name, NULL );
		if( !check_for_unique_app( application )){
			ret = FALSE;
			application->private->exit_code = APPLICATION_ERROR_UNIQUE_APP;
			application->private->exit_message =
				g_strdup( _( "Another instance of the application is already running." ));
		}
	}
	g_free( name );

	return( ret );
}

static gboolean
do_application_initialize_ui( BaseApplication *application )
{
	gboolean ret = TRUE;
	GError *error = NULL;

	gchar *name = v_get_ui_filename( application );
	if( !name || !strlen( name )){
		ret = FALSE;
		application->private->exit_code = APPLICATION_ERROR_UI_FNAME;
		application->private->exit_message =
			g_strdup( _( "No filename provided for the UI XML definition." ));

	} else {
		application->private->ui_xml = gtk_builder_new();
		gtk_builder_add_from_file( application->private->ui_xml, name, &error );
		if( error ){
			ret = FALSE;
			application->private->exit_code = APPLICATION_ERROR_UI_LOAD;
			application->private->exit_message =
				/* i18n: Unable to load the XML definition from <filename> */
				g_strdup( _( "Unable to load the XML definition from %s." ), name );
			g_error_free( error );
		}

	g_free( name );

	return( ret );
}

static gboolean
check_for_unique_app( BaseApplication *application )
{
	gboolean is_first = TRUE;

	g_assert( BASE_IS_APPLICATION( application ));

	if( unique_app_is_running( application->private->unique_app )){

		is_first = FALSE;

		unique_app_send_message( application->private->unique_app, UNIQUE_ACTIVATE, NULL );

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

static gint
display_dlg( BaseApplication *application, GtkMessageType type_message, GtkButtonsType type_buttons, const gchar *first, const gchar *second )
{
	g_assert( BASE_IS_APPLICATION( application ));

	GtkWidget *dialog = gtk_message_dialog_new( NULL, GTK_DIALOG_MODAL, type_message, type_buttons, first );

	if( second && strlen( second )){
		gtk_message_dialog_format_secondary_text( GTK_MESSAGE_DIALOG( dialog ), second );
	}

	const gchar *name = g_get_application_name();

	g_object_set( G_OBJECT( dialog ) , "title", name, NULL );

	gint result = gtk_dialog_run( GTK_DIALOG( dialog ));

	gtk_widget_destroy( dialog );

	return( result );
}

static GtkWidget *
recursive_search_for_child( BaseApplication *application, GtkWindow *toplevel, const gchar *name )
{
	return( search_for_child_widget( GTK_CONTAINER( toplevel ) , name ));
}

static GtkWidget *
search_for_child_widget( GtkContainer *container, const gchar *name )
{
	/*static const gchar *thisfn = "base_application_search_for_child_widget";
	g_debug( "%s: container=%p, name=%s", thisfn, container, name );*/

	GList *children = gtk_container_get_children( container );
	GList *ic;
	GtkWidget *found = NULL;

	for( ic = children ; ic ; ic = ic->next ){
		if( GTK_IS_WIDGET( ic->data )){
			GtkWidget *child = GTK_WIDGET( ic->data );
			if( child->name && strlen( child->name )){
				/*g_debug( "%s: child=%s", thisfn, child->name );*/
				if( !g_ascii_strcasecmp( name, child->name )){
					found = child;
					break;

				} else if( GTK_IS_CONTAINER( child )){
					found = search_for_child_widget( GTK_CONTAINER( child ), name );
					if( found ){
						break;
					}
				}
			}
		}
	}

	g_list_free( children );
	return( found );
}
