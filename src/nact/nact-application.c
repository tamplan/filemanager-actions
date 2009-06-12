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
#include <unique/unique.h>

#include "nact.h"
#include "nact-application.h"

/* private class data
 */
struct NactApplicationClassPrivate {
};

/* private instance data
 */
struct NactApplicationPrivate {
	gboolean   dispose_has_run;
	int        argc;
	gpointer   argv;
	UniqueApp *unique;
	GtkWindow *main;
};

/* private instance properties
 */
enum {
	PROP_ARGC = 1,
	PROP_ARGV
};

#define PROP_ARGC_STR		"argc"
#define PROP_ARGV_STR		"argv"

static GObjectClass *st_parent_class = NULL;

static GType          register_type( void );
static void           class_init( NactApplicationClass *klass );
static void           instance_init( GTypeInstance *instance, gpointer klass );
static void           instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void           instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void           instance_dispose( GObject *application );
static void           instance_finalize( GObject *application );

/*static UniqueResponse on_unique_message_received( UniqueApp *app, UniqueCommand command, UniqueMessageData *message, guint time, gpointer user_data );*/
static void           warn_other_instance( NactApplication *application );
static gboolean       check_for_unique_app( NactApplication *application );
static void           initialize_i18n( NactApplication *application );
static gboolean       startup_appli( NactApplication *application );
static int            run_appli( NactApplication *application );
static void           finish_appli( NactApplication *application );

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

	return( g_type_register_static( G_TYPE_OBJECT, "NactApplication", &info, 0 ));
}

static void
class_init( NactApplicationClass *klass )
{
	static const gchar *thisfn = "nact_application_class_init";
	g_debug( "%s: klass=%p", thisfn, klass );

	st_parent_class = g_type_class_peek_parent( klass );

	GObjectClass *object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
	object_class->get_property = instance_get_property;
	object_class->set_property = instance_set_property;

	GParamSpec *spec;
	spec = g_param_spec_int(
			PROP_ARGC_STR,
			PROP_ARGC_STR,
			"Command-line arguments count", 0, 65535, 0,
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_ARGC, spec );

	spec = g_param_spec_pointer(
			PROP_ARGV_STR,
			PROP_ARGV_STR,
			"Command-line arguments",
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_ARGV, spec );

	klass->private = g_new0( NactApplicationClassPrivate, 1 );
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

	self->private->unique = unique_app_new( "org.nautilus-actions.Config", NULL );
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	g_assert( NACT_IS_APPLICATION( object ));
	NactApplication *self = NACT_APPLICATION( object );

	switch( property_id ){
		case PROP_ARGC:
			g_value_set_int( value, self->private->argc );
			break;

		case PROP_ARGV:
			g_value_set_pointer( value, self->private->argv );
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
		case PROP_ARGC:
			self->private->argc = g_value_get_int( value );
			break;

		case PROP_ARGV:
			self->private->argv = g_value_get_pointer( value );
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

		g_object_unref( self->private->unique );

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
	/*NactApplication *self = ( NactApplication * ) application;*/

	/* chain call to parent class */
	if( st_parent_class->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( application );
	}
}

/**
 * Returns a newly allocated NactApplication object.
 */
NactApplication *
nact_application_new( void )
{
	return( g_object_new( NACT_APPLICATION_TYPE, NULL ));
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
	return( g_object_new( NACT_APPLICATION_TYPE, PROP_ARGC_STR, argc, PROP_ARGV_STR, argv, NULL ));
}

/*static UniqueResponse
on_unique_message_received(
		UniqueApp *app, UniqueCommand command, UniqueMessageData *message, guint time, gpointer user_data )
{
	static const gchar *thisfn = "nact_application_check_for_unique_app";
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
warn_other_instance( NactApplication *application )
{
	g_assert( NACT_IS_APPLICATION( application ));

	GtkWidget *dialog = gtk_message_dialog_new_with_markup(
			NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
			_( "<b>Another instance of Nautilus Actions Configurator is already running.</b>\n\nPlease switch back to it." ));

	g_object_set( G_OBJECT( dialog ) , "title", _( "Nautilus Actions" ), NULL );

	gtk_dialog_run( GTK_DIALOG( dialog ));
	gtk_widget_destroy( dialog );
}

/*
 * returns TRUE if we are the first instance
 */
static gboolean
check_for_unique_app( NactApplication *application )
{
	gboolean is_first = TRUE;

	g_assert( NACT_IS_APPLICATION( application ));

	if( unique_app_is_running( application->private->unique )){

		is_first = FALSE;

		unique_app_send_message( application->private->unique, UNIQUE_ACTIVATE, NULL );

		/* the screen is not actually modified, nor the main window is
		 * switched back to the current screen ; the icon in the deskbar
		 * applet is just highlighted
		 * so a message is not too much !
		 */
		warn_other_instance( application );

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
initialize_i18n( NactApplication *application )
{
#ifdef ENABLE_NLS
        bindtextdomain( GETTEXT_PACKAGE, GNOMELOCALEDIR );
# ifdef HAVE_BIND_TEXTDOMAIN_CODESET
        bind_textdomain_codeset( GETTEXT_PACKAGE, "UTF-8" );
# endif
        textdomain( GETTEXT_PACKAGE );
#endif
}

static gboolean
startup_appli( NactApplication *application )
{
	int ret;

	initialize_i18n( application );

	g_set_application_name( PACKAGE );

	gtk_window_set_default_icon_name( PACKAGE );

	ret = check_for_unique_app( application );

	return( ret );
}

static int
run_appli( NactApplication *application )
{
	int code = 0;

	application->private->main = nact_init_dialog();
	unique_app_watch_window( application->private->unique, application->private->main );

	gtk_main();

	return( code );
}

static void
finish_appli( NactApplication *application )
{
}

/**
 * This a the whole run management for the NactApplication Object.
 *
 * @app: the considered NactApplication object.
 *
 * The returned integer should be returned to the OS.
 */
int
nact_application_run( NactApplication *application )
{
	static const gchar *thisfn = "nact_application_run";
	g_debug( "%s: application=%p", thisfn, application );

	g_assert( NACT_IS_APPLICATION( application ));

	int code = 0;

	if( startup_appli( application )){
		code = run_appli( application );
		finish_appli( application );
	}

	return( code );
}
