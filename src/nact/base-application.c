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
	int         argc;
	gpointer    argv;
	gchar      *unique_name;
	UniqueApp  *unique_app;
	gchar      *application_name;
	gchar      *icon_name;
	BaseWindow *main_window;
};

/* instance properties
 */
enum {
	PROP_APPLICATION_ARGC = 1,
	PROP_APPLICATION_ARGV,
	PROP_APPLICATION_UNIQUE_NAME,
	PROP_APPLICATION_UNIQUE_APP,
	PROP_APPLICATION_MAIN_WINDOW,
	PROP_APPLICATION_NAME,
	PROP_APPLICATION_ICON_NAME
};

static GObjectClass *st_parent_class = NULL;

static GType          register_type( void );
static void           class_init( BaseApplicationClass *klass );
static void           instance_init( GTypeInstance *instance, gpointer klass );
static void           instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void           instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void           instance_dispose( GObject *application );
static void           instance_finalize( GObject *application );

static int            do_run( BaseApplication *application );

static void           v_initialize( BaseApplication *application );
static void           v_initialize_i18n( BaseApplication *application );
static void           v_initialize_gtk( BaseApplication *application );
static void           v_initialize_application_name( BaseApplication *application );
static void           v_initialize_icon_name( BaseApplication *application );
static void           v_initialize_unique( BaseApplication *application );
static gboolean       v_is_willing_to_run( BaseApplication *application );
static void           v_advertise_willing_to_run( BaseApplication *application );
static void           v_advertise_not_willing_to_run( BaseApplication *application );
static void           v_start( BaseApplication *application );
static int            v_finish( BaseApplication *application );
static gchar         *v_get_unique_name( BaseApplication *application );
static gchar         *v_get_application_name( BaseApplication *application );
static gchar         *v_get_icon_name( BaseApplication *application );
static GObject       *v_get_main_window( BaseApplication *application );

static void           do_initialize( BaseApplication *application );
static void           do_initialize_i18n( BaseApplication *application );
static void           do_initialize_gtk( BaseApplication *application );
static void           do_initialize_application_name( BaseApplication *application );
static void           do_initialize_icon_name( BaseApplication *application );
static void           do_initialize_unique( BaseApplication *application );
static gboolean       is_willing_to_run( BaseApplication *application );
static gboolean       check_for_unique_app( BaseApplication *application );
static void           do_advertise_willing_to_run( BaseApplication *application );
static void           do_advertise_not_willing_to_run( BaseApplication *application );
static void           do_start( BaseApplication *application );
static int            do_finish( BaseApplication *application );
static gchar         *do_get_unique_name( BaseApplication *application );
static gchar         *do_get_application_name( BaseApplication *application );
static gchar         *do_get_icon_name( BaseApplication *application );
static GObject       *do_get_main_window( BaseApplication *application );

/*static UniqueResponse on_unique_message_received( UniqueApp *app, UniqueCommand command, UniqueMessageData *message, guint time, gpointer user_data );*/

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
			PROP_APPLICATION_ARGC_STR,
			PROP_APPLICATION_ARGC_STR,
			"Command-line arguments count", 0, 65535, 0,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_APPLICATION_ARGC, spec );

	spec = g_param_spec_pointer(
			PROP_APPLICATION_ARGV_STR,
			PROP_APPLICATION_ARGV_STR,
			"Command-line arguments",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_APPLICATION_ARGV, spec );

	spec = g_param_spec_string(
			PROP_APPLICATION_UNIQUE_NAME_STR,
			PROP_APPLICATION_UNIQUE_NAME_STR,
			"DBUS name for unique application", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_APPLICATION_UNIQUE_NAME, spec );

	spec = g_param_spec_pointer(
			PROP_APPLICATION_UNIQUE_APP_STR,
			PROP_APPLICATION_UNIQUE_APP_STR,
			"UniqueApp object pointer",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_APPLICATION_UNIQUE_APP, spec );

	spec = g_param_spec_pointer(
			PROP_APPLICATION_MAIN_WINDOW_STR,
			PROP_APPLICATION_MAIN_WINDOW_STR,
			"The BaseWindow object which encapsulates the main GtkWindow",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_APPLICATION_MAIN_WINDOW, spec );

	spec = g_param_spec_string(
			PROP_APPLICATION_NAME_STR,
			PROP_APPLICATION_NAME_STR,
			"Localized application name", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_APPLICATION_NAME, spec );

	spec = g_param_spec_string(
			PROP_APPLICATION_ICON_NAME_STR,
			PROP_APPLICATION_ICON_NAME_STR,
			"Default themed icon name", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_APPLICATION_ICON_NAME, spec );

	klass->private = g_new0( BaseApplicationClassPrivate, 1 );

	klass->run = do_run;
	klass->initialize = do_initialize;
	klass->initialize_i18n = do_initialize_i18n;
	klass->initialize_gtk = do_initialize_gtk;
	klass->initialize_application_name = do_initialize_application_name;
	klass->initialize_icon_name = do_initialize_icon_name;
	klass->initialize_unique = do_initialize_unique;
	klass->is_willing_to_run = is_willing_to_run;
	klass->advertise_willing_to_run = do_advertise_willing_to_run;
	klass->advertise_not_willing_to_run = do_advertise_not_willing_to_run;
	klass->start = do_start;
	klass->finish = do_finish;
	klass->get_unique_name = do_get_unique_name;
	klass->get_application_name = do_get_application_name;
	klass->get_icon_name = do_get_icon_name;
	klass->get_main_window = do_get_main_window;
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
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	g_assert( BASE_IS_APPLICATION( object ));
	BaseApplication *self = BASE_APPLICATION( object );

	switch( property_id ){
		case PROP_APPLICATION_ARGC:
			g_value_set_int( value, self->private->argc );
			break;

		case PROP_APPLICATION_ARGV:
			g_value_set_pointer( value, self->private->argv );
			break;

		case PROP_APPLICATION_UNIQUE_NAME:
			g_value_set_string( value, self->private->unique_name );
			break;

		case PROP_APPLICATION_UNIQUE_APP:
			g_value_set_pointer( value, self->private->unique_app );
			break;

		case PROP_APPLICATION_MAIN_WINDOW:
			g_value_set_pointer( value, self->private->main_window );
			break;

		case PROP_APPLICATION_NAME:
			g_value_set_string( value, self->private->application_name );
			break;

		case PROP_APPLICATION_ICON_NAME:
			g_value_set_string( value, self->private->icon_name );
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
		case PROP_APPLICATION_ARGC:
			self->private->argc = g_value_get_int( value );
			break;

		case PROP_APPLICATION_ARGV:
			self->private->argv = g_value_get_pointer( value );
			break;

		case PROP_APPLICATION_UNIQUE_NAME:
			g_free( self->private->unique_name );
			self->private->unique_name = g_value_dup_string( value );
			break;

		case PROP_APPLICATION_UNIQUE_APP:
			self->private->unique_app = g_value_get_pointer( value );
			break;

		case PROP_APPLICATION_MAIN_WINDOW:
			self->private->main_window = g_value_get_pointer( value );
			break;

		case PROP_APPLICATION_NAME:
			g_free( self->private->application_name );
			self->private->application_name = g_value_dup_string( value );
			break;

		case PROP_APPLICATION_ICON_NAME:
			g_free( self->private->icon_name );
			self->private->icon_name = g_value_dup_string( value );
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

		if( UNIQUE_IS_APP( self->private->unique_app )){
			g_object_unref( self->private->unique_app );
		}
		if( GTK_IS_WINDOW( self->private->main_window )){
			g_object_unref( self->private->main_window );
		}

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

	g_free( self->private->unique_name );
	g_free( self->private->application_name );
	g_free( self->private->icon_name );

	/* chain call to parent class */
	if( st_parent_class->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( application );
	}
}

/**
 * Returns a newly allocated BaseApplication object.
 */
BaseApplication *
base_application_new( void )
{
	return( g_object_new( BASE_APPLICATION_TYPE, NULL ));
}

/**
 * Returns a newly allocated BaseApplication object.
 *
 * @argc: count of command-line arguments.
 *
 * @argv: command-line arguments.
 */
BaseApplication *
base_application_new_with_args( int argc, char **argv )
{
	return(
			g_object_new(
					BASE_APPLICATION_TYPE,
					PROP_APPLICATION_ARGC_STR, argc,
					PROP_APPLICATION_ARGV_STR, argv,
					NULL )
	);
}

/**
 * Executes the application.
 *
 * @application: the considered BaseApplication object.
 *
 * The returned integer should be returned to the OS.
 *
 * This a the main function management of the application. We iniialize
 * it, test command line options, if it is willing to run, then start
 * it and finally finish it.
 *
 * All these steps are implemented by virtual functions which provider
 * some suitable defaults, and can be overriden by a derived class.
 */
int
base_application_run( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_run";
	g_debug( "%s: application=%p", thisfn, application );

	g_assert( BASE_IS_APPLICATION( application ));

	return( BASE_APPLICATION_GET_CLASS( application )->run( application ));
}

static int
do_run( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_do_run";
	g_debug( "%s: application=%p", thisfn, application );

	g_assert( BASE_IS_APPLICATION( application ));

	int code = 0;

	v_initialize( application );

	if( v_is_willing_to_run( application )){

		v_advertise_willing_to_run( application );
		v_start( application );
		code = v_finish( application );

	} else {

		v_advertise_not_willing_to_run( application );
	}

	return( code );
}

static void
v_initialize( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_initialize";
	g_debug( "%s: application=%p", thisfn, application );

	g_assert( BASE_IS_APPLICATION( application ));

	BASE_APPLICATION_GET_CLASS( application )->initialize( application );
}

static void
v_initialize_i18n( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_initialize_i18n";
	g_debug( "%s: application=%p", thisfn, application );

	g_assert( BASE_IS_APPLICATION( application ));

	BASE_APPLICATION_GET_CLASS( application )->initialize_i18n( application );
}

static void
v_initialize_gtk( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_initialize_gtk";
	g_debug( "%s: application=%p", thisfn, application );

	g_assert( BASE_IS_APPLICATION( application ));

	BASE_APPLICATION_GET_CLASS( application )->initialize_gtk( application );
}

static void
v_initialize_application_name( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_initialize_application_name";
	g_debug( "%s: application=%p", thisfn, application );

	g_assert( BASE_IS_APPLICATION( application ));

	BASE_APPLICATION_GET_CLASS( application )->initialize_application_name( application );
}

static void
v_initialize_icon_name( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_initialize_icon_name";
	g_debug( "%s: application=%p", thisfn, application );

	g_assert( BASE_IS_APPLICATION( application ));

	BASE_APPLICATION_GET_CLASS( application )->initialize_icon_name( application );
}

static void
v_initialize_unique( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_initialize_unique";
	g_debug( "%s: application=%p", thisfn, application );

	g_assert( BASE_IS_APPLICATION( application ));

	BASE_APPLICATION_GET_CLASS( application )->initialize_unique( application );
}

static gboolean
v_is_willing_to_run( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_is_willing_to_run";
	g_debug( "%s: application=%p", thisfn, application );

	g_assert( BASE_IS_APPLICATION( application ));

	return( BASE_APPLICATION_GET_CLASS( application )->is_willing_to_run( application ));
}

static void
v_advertise_willing_to_run( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_advertise_willing_to_run";
	g_debug( "%s: application=%p", thisfn, application );

	g_assert( BASE_IS_APPLICATION( application ));

	return( BASE_APPLICATION_GET_CLASS( application )->advertise_willing_to_run( application ));
}

static void
v_advertise_not_willing_to_run( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_advertise_not_willing_to_run";
	g_debug( "%s: application=%p", thisfn, application );

	g_assert( BASE_IS_APPLICATION( application ));

	return( BASE_APPLICATION_GET_CLASS( application )->advertise_not_willing_to_run( application ));
}

static void
v_start( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_start";
	g_debug( "%s: application=%p", thisfn, application );

	g_assert( BASE_IS_APPLICATION( application ));

	BASE_APPLICATION_GET_CLASS( application )->start( application );
}

static int
v_finish( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_finish";
	g_debug( "%s: application=%p", thisfn, application );

	g_assert( BASE_IS_APPLICATION( application ));

	return( BASE_APPLICATION_GET_CLASS( application )->finish( application ));
}

static gchar *
v_get_unique_name( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_get_unique_name";
	g_debug( "%s: application=%p", thisfn, application );

	g_assert( BASE_IS_APPLICATION( application ));

	gchar *name;
	g_object_get( G_OBJECT( application ), PROP_APPLICATION_UNIQUE_NAME_STR, &name, NULL );

	if( !name || !strlen( name )){
		name = BASE_APPLICATION_GET_CLASS( application )->get_unique_name( application );
		if( name && strlen( name )){
			g_object_set( G_OBJECT( application ), PROP_APPLICATION_UNIQUE_NAME_STR, name, NULL );
		}
	}

	return( name );
}

static gchar *
v_get_application_name( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_get_application_name";
	g_debug( "%s: application=%p", thisfn, application );

	g_assert( BASE_IS_APPLICATION( application ));

	gchar *name;
	g_object_get( G_OBJECT( application ), PROP_APPLICATION_NAME_STR, &name, NULL );

	if( !name || !strlen( name )){
		name = BASE_APPLICATION_GET_CLASS( application )->get_application_name( application );
		if( name && strlen( name )){
			g_object_set( G_OBJECT( application ), PROP_APPLICATION_NAME_STR, name, NULL );
		}
	}

	return( name );
}

static gchar *
v_get_icon_name( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_get_icon_name";
	g_debug( "%s: icon=%p", thisfn, application );

	g_assert( BASE_IS_APPLICATION( application ));

	gchar *name;
	g_object_get( G_OBJECT( application ), PROP_APPLICATION_ICON_NAME_STR, &name, NULL );

	if( !name || !strlen( name )){
		name = BASE_APPLICATION_GET_CLASS( application )->get_icon_name( application );
		if( name && strlen( name )){
			g_object_set( G_OBJECT( application ), PROP_APPLICATION_ICON_NAME_STR, name, NULL );
		}
	}

	return( name );
}

static GObject *
v_get_main_window( BaseApplication *application )
{
	static const gchar *thisfn = "base_application_v_get_main_window";
	g_debug( "%s: icon=%p", thisfn, application );

	g_assert( BASE_IS_APPLICATION( application ));

	return( BASE_APPLICATION_GET_CLASS( application )->get_main_window( application ));
}

static void
do_initialize( BaseApplication *application )
{
	v_initialize_i18n( application );
	v_initialize_gtk( application );
	v_initialize_application_name( application );
	v_initialize_icon_name( application );
	v_initialize_unique( application );
}

static void
do_initialize_i18n( BaseApplication *application )
{
#ifdef ENABLE_NLS
        bindtextdomain( GETTEXT_PACKAGE, GNOMELOCALEDIR );
# ifdef HAVE_BIND_TEXTDOMAIN_CODESET
        bind_textdomain_codeset( GETTEXT_PACKAGE, "UTF-8" );
# endif
        textdomain( GETTEXT_PACKAGE );
#endif
}

static void
do_initialize_gtk( BaseApplication *application )
{
	int argc;
	gpointer argv;
	g_object_get( G_OBJECT( application ), PROP_APPLICATION_ARGC_STR, &argc, PROP_APPLICATION_ARGV_STR, &argv, NULL );
	gtk_init( &argc, ( char *** ) &argv );
	g_object_set( G_OBJECT( application ), PROP_APPLICATION_ARGC_STR, argc, PROP_APPLICATION_ARGV_STR, argv, NULL );
}

static void
do_initialize_application_name( BaseApplication *application )
{
	gchar *name = v_get_application_name( application );
	if( name && strlen( name )){
		g_set_application_name( name );
	}
	g_free( name );
}

static void
do_initialize_icon_name( BaseApplication *application )
{
	gchar *name = v_get_icon_name( application );
	if( name && strlen( name )){
		gtk_window_set_default_icon_name( name );
	}
	g_free( name );
}

static void
do_initialize_unique( BaseApplication *application )
{
	gchar *unique_name = v_get_unique_name( application );

	if( unique_name && strlen( unique_name )){
		application->private->unique_app = unique_app_new( unique_name, NULL );
	}

	g_free( unique_name );
}

static gboolean
is_willing_to_run( BaseApplication *application )
{
	gboolean is_willing = TRUE;

	if( application->private->unique_app ){
		is_willing = check_for_unique_app( application );
	}

	return( is_willing );
}

/*
 * returns TRUE if we are the first instance
 */
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

static void
do_advertise_willing_to_run( BaseApplication *application )
{

}

static void
do_advertise_not_willing_to_run( BaseApplication *application )
{

}

/*
 * this should be typically called by the derived class after it has
 * created the main window
 */
static void
do_start( BaseApplication *application )
{
	BaseWindow *window = BASE_WINDOW( v_get_main_window( application ));

	if( window ){
		g_assert( BASE_IS_WINDOW( window ));

		application->private->main_window = window;

		base_window_init_window( window );

		GtkWindow *wnd = base_window_get_toplevel_window( window );
		if( application->private->unique_app ){
			unique_app_watch_window( application->private->unique_app, wnd );
		}

		gtk_main();
	}
}

static int
do_finish( BaseApplication *application )
{
	int code = 0;
	return( code );
}

static gchar *
do_get_unique_name( BaseApplication *application )
{
	return( NULL );
}

static gchar *
do_get_application_name( BaseApplication *application )
{
	return( NULL );
}

static gchar *
do_get_icon_name( BaseApplication *application )
{
	return( NULL );
}

static GObject *
do_get_main_window( BaseApplication *application )
{
	return( NULL );
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

void
base_application_error_dlg(
		BaseApplication *application, GtkMessageType type, const gchar *primary, const gchar *secondary )
{
	g_assert( BASE_IS_APPLICATION( application ));

	GtkWidget *dialog = gtk_message_dialog_new(
			NULL, GTK_DIALOG_MODAL, type, GTK_BUTTONS_OK, primary );

	if( secondary && strlen( secondary )){
		gtk_message_dialog_format_secondary_text( GTK_MESSAGE_DIALOG( dialog ), secondary );
	}

	const gchar *name = g_get_application_name();

	g_object_set( G_OBJECT( dialog ) , "title", name, NULL );

	gtk_dialog_run( GTK_DIALOG( dialog ));

	gtk_widget_destroy( dialog );
}
