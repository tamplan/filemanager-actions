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

#ifndef __BASE_APPLICATION_H__
#define __BASE_APPLICATION_H__

/**
 * SECTION: base-application
 * @title: BaseApplication
 * @short_description: The Base Application application base class definition
 * @include: base-application.h
 *
 * #BaseApplication is the base class for the application part of Gtk programs.
 * It aims at providing all common services. It interacts with #BaseBuilder
 * and #BaseWindow classes.
 *
 * #BaseApplication is a pure virtual class. A Gtk program should derive
 * its own class from #BaseApplication, and instantiate it in its main()
 * program entry point.
 *
 * <example>
 *   <programlisting>
 *     #include "my-application.h"
 *
 *     int
 *     main( int argc, char **argv )
 *     {
 *         MyApplication *appli;
 *         int code;
 *
 *         appli = my_application_new_with_args( argc, argv );
 *         code = base_appliction_run( BASE_APPLICATION( appli ));
 *         g_object_unref( appli );
 *
 *         return( code );
 *     }
 *   </programlisting>
 * </example>
 */

#include "base-builder.h"

G_BEGIN_DECLS

#define BASE_APPLICATION_TYPE                ( base_application_get_type())
#define BASE_APPLICATION( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, BASE_APPLICATION_TYPE, BaseApplication ))
#define BASE_APPLICATION_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, BASE_APPLICATION_TYPE, BaseApplicationClass ))
#define BASE_IS_APPLICATION( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, BASE_APPLICATION_TYPE ))
#define BASE_IS_APPLICATION_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), BASE_APPLICATION_TYPE ))
#define BASE_APPLICATION_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), BASE_APPLICATION_TYPE, BaseApplicationClass ))

typedef struct _BaseApplicationPrivate       BaseApplicationPrivate;

typedef struct {
	/*< private >*/
	GObject                 parent;
	BaseApplicationPrivate *private;
}
	BaseApplication;

typedef struct _BaseApplicationClassPrivate  BaseApplicationClassPrivate;

/**
 * BaseApplicationClass:
 * @manage_options:  manage the command-line arguments.
 * @main_window_new: open and run the first (main) window of the application.
 *
 * This defines the virtual method a derived class may, should or must implement.
 */
typedef struct {
	/*< private >*/
	GObjectClass                 parent;
	BaseApplicationClassPrivate *private;

	/*< public >*/
	/**
	 * manage_options:
	 * @appli: this #BaseApplication -derived instance.
	 * @code:  a pointer to an integer that the derived application may
	 *         set as the exit code of the program if it chooses to stop
	 *         the execution; the code set here will be ignored if execution
	 *         is allowed to continue.
	 *
	 * This is invoked by the BaseApplication base class, after arguments
	 * in the command-line have been processed by gtk_init_with_args()
	 * function.
	 *
	 * This let the derived class an opportunity to manage command-line
	 * arguments.
	 *
	 * If it does not dected an error, the derived class should call the
	 * parent method, to give it a chance to manage its own options.
	 *
	 * Returns: %TRUE to continue execution, %FALSE to stop it.
	 * In this later case only, the exit code set in @code will be considered.
	 */
	gboolean  ( *manage_options ) ( const BaseApplication *appli, int *code );

	/**
	 * main_window_new:
	 * @appli: this #BaseApplication -derived instance.
	 *
	 * This is invoked by the BaseApplication base class to let the derived
	 * class do its own initializations and create its main window.
	 *
	 * This is a pure virtual method.
	 *
	 * Returns: the main window of the application, as a #BaseWindow
	 * -derived object. It may or may not having already been initialized.
	 */
	GObject * ( *main_window_new )( const BaseApplication *appli, int *code );

	/**
	 * initialize_application:
	 * @appli: this #BaseApplication instance.
	 *
	 * Initializes the derived-class application.
	 *
	 * When this function successfully returns, the #BaseApplication
	 * instance must have a valid pointer to the #BaseWindow-derived
	 * object which will be used as a main window for the application.
	 *
	 * Returns: %TRUE if at least all mandatory informations have been
	 * collected, %FALSE else.
	 *
	 * The base class implementation asks the derived class to
	 * allocates and provides the BaseWindow-derived object which will
	 * be the main window of the application
	 * (cf. get_main_window()). This step is mandatory.
	 *
	 * If failed, this function sets #exit_code to the value which is
	 * pertinent depending of the missing information, and prepares a
	 * short #exit_message to be displayed in a dialog box.
	 */
	gboolean  ( *initialize_application )     ( BaseApplication *appli );

	/**
	 * get_ui_filename:
	 * @appli: this #BaseApplication instance.
	 *
	 * Asks the derived class for the filename of the XML definition of
	 * the user interface. This XML definition must be suitable in order
	 * to be loaded via GtkBuilder.
	 *
	 * No default is provided by the base class. If the base class does
	 * not provide one, then the program stops and exits with the code
	 * %APPLICATION_ERROR_UI_FNAME.
	 *
	 * Returns: the filename of the XML definition, to be g_free() by
	 * the caller.
	 */
	gchar *   ( *get_ui_filename )                 ( BaseApplication *appli );
}
	BaseApplicationClass;

/**
 * Properties defined by the BaseApplication class.
 * They should be provided at object instantiation time.
 *
 * @BASE_PROP_ARGC:             count of arguments in command-line.
 * @BASE_PROP_ARGV:             array of command-line arguments.
 * @BASE_PROP_OPTIONS:          array of command-line options descriptions.
 * @BASE_PROP_APPLICATION_NAME: application name.
 * @BASE_PROP_ICON_NAME:        icon name.
 * @BASE_PROP_UNIQUE_APP_NAME:  unique name of the application (if apply)
 */
#define BASE_PROP_ARGC						"base-application-argc"
#define BASE_PROP_ARGV						"base-application-argv"
#define BASE_PROP_OPTIONS					"base-application-options"
#define BASE_PROP_APPLICATION_NAME			"base-application-name"
#define BASE_PROP_ICON_NAME					"base-application-icon-name"
#define BASE_PROP_UNIQUE_APP_NAME			"base-application-unique-app-name"

typedef enum {
	BASE_EXIT_CODE_START_FAIL = -1,
	BASE_EXIT_CODE_OK = 0,
	BASE_EXIT_CODE_UNIQUE_APP,
	BASE_EXIT_CODE_MAIN_WINDOW,

	BASE_APPLICATION_ERROR_UI_FNAME,		/* empty XML filename */
	BASE_APPLICATION_ERROR_UI_LOAD,			/* unable to load the XML definition of the UI */
	BASE_APPLICATION_ERROR_DEFAULT_ICON		/* unable to set default icon */
}
	BaseExitCode;

GType        base_application_get_type( void );

int          base_application_run( BaseApplication *application );

gchar       *base_application_get_application_name( const BaseApplication *application );

/**
 * @BASE_APPLICATION_PROP_IS_GTK_INITIALIZED: set to %TRUE after
 * successfully returning from the application_initialize_gtk() virtual
 * function.
 *
 * While this flag is not %TRUE, error messages are printed to
 * stdout. When %TRUE, error messages are displayed with a dialog
 * box.
 */
#define BASE_APPLICATION_PROP_IS_GTK_INITIALIZED	"base-application-is-gtk-initialized"

/**
 * @BASE_APPLICATION_PROP_UNIQUE_APP_HANDLE: the UniqueApp object allocated
 * if the derived-class has provided a UniqueApp name (see
 * #application_get_unique_app_name). Rather for internal use.
 */
#define BASE_APPLICATION_PROP_UNIQUE_APP_HANDLE		"base-application-unique-app-handle"

/**
 * @BASE_APPLICATION_PROP_EXIT_CODE: the code which will be returned by the
 * program to the operating system.
 * @BASE_APPLICATION_PROP_EXIT_MESSAGE1:
 * @BASE_APPLICATION_PROP_EXIT_MESSAGE2: the message which will be displayed
 * at program terminaison if @BASE_APPLICATION_PROP_EXIT_CODE is not zero.
 * When in graphical mode, the first line is displayed as bold.
 *
 * See @BASE_APPLICATION_PROP_IS_GTK_INITIALIZED for how the
 * @BASE_APPLICATION_PROP_EXIT_MESSAGE is actually displayed.
 */
#define BASE_APPLICATION_PROP_EXIT_CODE				"base-application-exit-code"
#define BASE_APPLICATION_PROP_EXIT_MESSAGE1			"base-application-exit-message1"
#define BASE_APPLICATION_PROP_EXIT_MESSAGE2			"base-application-exit-message2"

/**
 * @BASE_APPLICATION_PROP_BUILDER: the #BaseBuilder object allocated to
 * handle the user interface XML definition. Rather for internal use.
 */
#define BASE_APPLICATION_PROP_BUILDER				"base-application-builder"

/**
 * @BASE_APPLICATION_PROP_MAIN_WINDOW: as its name says: a pointer to the
 * #BaseWindow-derived main window of the application.
 */
#define BASE_APPLICATION_PROP_MAIN_WINDOW			"base-application-main-window"

gchar       *base_application_get_ui_filename( BaseApplication *application );
BaseBuilder *base_application_get_builder( BaseApplication *application );

void         base_application_message_dlg( BaseApplication *application, GSList *message );
void         base_application_error_dlg( BaseApplication *application, GtkMessageType type, const gchar *first, const gchar *second );
gboolean     base_application_yesno_dlg( BaseApplication *application, GtkMessageType type, const gchar *first, const gchar *second );

G_END_DECLS

#endif /* __BASE_APPLICATION_H__ */
