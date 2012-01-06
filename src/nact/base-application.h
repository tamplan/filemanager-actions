/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011, 2012 Pierre Wieser and others (see AUTHORS)
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
 *         appli = my_application_new();
 *         code = base_appliction_run( BASE_APPLICATION( appli ), argc, argv );
 *         g_object_unref( appli );
 *
 *         return( code );
 *     }
 *   </programlisting>
 * </example>
 *
 * main                 BaseApplication      NactApplication
 * ===================  ===================  ===================
 * appli = nact_application_new()
 *                                           appli = g_object_new()
 *                                           set properties
 *                                             application name
 *                                             icon name
 *                                             description
 *                                             command-line definitions
 *                                             unique name (if apply)
 * ret = base_application_run( appli, argc, argv )
 *                      init i18n
 *                      init application name
 *                      init gtk with command-line options
 *                      manage command-line options
 *                      init unique manager
 *                      init session manager
 *                      init icon name
 *                      init common builder
 *                      foreach window to display
 *                        create window
 *                        create gtk toplevel
 *                        unique watch toplevel
 *                        run the window
 *                      ret = last window return code
 * g_object_unref( appli )
 * return( ret )
 *
 * At any time, a function may preset the exit code of the application just by
 * setting the @BASE_PROP_CODE property. Unless it also asks to quit immediately
 * by returning %FALSE, another function may always set another exit code after
 * that.
 */

#include "base-builder.h"

G_BEGIN_DECLS

#define BASE_APPLICATION_TYPE           ( base_application_get_type())
#define BASE_APPLICATION( o )           ( G_TYPE_CHECK_INSTANCE_CAST( o, BASE_APPLICATION_TYPE, BaseApplication ))
#define BASE_APPLICATION_CLASS( k )     ( G_TYPE_CHECK_CLASS_CAST( k, BASE_APPLICATION_TYPE, BaseApplicationClass ))
#define BASE_IS_APPLICATION( o )        ( G_TYPE_CHECK_INSTANCE_TYPE( o, BASE_APPLICATION_TYPE ))
#define BASE_IS_APPLICATION_CLASS( k )  ( G_TYPE_CHECK_CLASS_TYPE(( k ), BASE_APPLICATION_TYPE ))
#define BASE_APPLICATION_GET_CLASS( o ) ( G_TYPE_INSTANCE_GET_CLASS(( o ), BASE_APPLICATION_TYPE, BaseApplicationClass ))

typedef struct _BaseApplicationPrivate      BaseApplicationPrivate;

typedef struct {
	/*< private >*/
	GObject                 parent;
	BaseApplicationPrivate *private;
}
	BaseApplication;

typedef struct _BaseApplicationClassPrivate BaseApplicationClassPrivate;

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
	 *
	 * This is invoked by the BaseApplication base class, after arguments
	 * in the command-line have been processed by gtk_init_with_args()
	 * function.
	 *
	 * This let the derived class an opportunity to manage command-line
	 * arguments. Unless it decides to stop the execution of the program,
	 * the derived class should call the parent class method in order to
	 * let it manage its own options.
	 *
	 * The derived class may set the exit code of the application by
	 * setting the @BASE_PROP_CODE property of @appli.
	 *
	 * Returns: %TRUE to continue execution, %FALSE to stop it.
	 */
	gboolean  ( *manage_options ) ( const BaseApplication *appli );

	/**
	 * main_window_new:
	 * @appli: this #BaseApplication -derived instance.
	 *
	 * This is invoked by the BaseApplication base class to let the derived
	 * class do its own initializations and create its main window.
	 *
	 * This is a pure virtual method. Only the most derived class
	 * main_window_new() method is invoked.
	 *
	 * Returns: the main window of the application, as a #BaseWindow
	 * -derived object. It may or may not have already been initialized.
	 */
	GObject * ( *main_window_new )( const BaseApplication *appli, int *code );
}
	BaseApplicationClass;

/**
 * Properties defined by the BaseApplication class.
 * They may be provided at object instantiation time, either in the derived-
 * application constructor, or in the main() function, but in all cases
 * before calling base_application_run().
 *
 * @BASE_PROP_ARGC:             count of arguments in command-line.
 * @BASE_PROP_ARGV:             array of command-line arguments.
 * @BASE_PROP_OPTIONS:          array of command-line options descriptions.
 * @BASE_PROP_APPLICATION_NAME: application name.
 * @BASE_PROP_DESCRIPTION:      short description.
 * @BASE_PROP_ICON_NAME:        icon name.
 * @BASE_PROP_UNIQUE_NAME:      unique name of the application (if not empty)
 * @BASE_PROP_CODE:             return code of the application
 */
#define BASE_PROP_ARGC						"base-application-argc"
#define BASE_PROP_ARGV						"base-application-argv"
#define BASE_PROP_OPTIONS					"base-application-options"
#define BASE_PROP_APPLICATION_NAME			"base-application-name"
#define BASE_PROP_DESCRIPTION				"base-application-description"
#define BASE_PROP_ICON_NAME					"base-application-icon-name"
#define BASE_PROP_UNIQUE_NAME				"base-application-unique-name"
#define BASE_PROP_CODE						"base-application-code"

typedef enum {
	BASE_EXIT_CODE_START_FAIL = -1,
	BASE_EXIT_CODE_OK = 0,
	BASE_EXIT_CODE_NO_APPLICATION_NAME,		/* no application name has been set by the derived class */
	BASE_EXIT_CODE_ARGS,					/* unable to interpret command-line options */
	BASE_EXIT_CODE_UNIQUE_APP,				/* another instance is already running */
	BASE_EXIT_CODE_MAIN_WINDOW,
	BASE_EXIT_CODE_INIT_FAIL,
	BASE_EXIT_CODE_PROGRAM,
	/*
	 * BaseApplication -derived class may use program return codes
	 * starting with this value
	 */
	BASE_EXIT_CODE_USER_APP = 32
}
	BaseExitCode;

GType        base_application_get_type( void );

int          base_application_run( BaseApplication *application, int argc, GStrv argv );

gchar       *base_application_get_application_name( const BaseApplication *application );
BaseBuilder *base_application_get_builder         ( const BaseApplication *application );

G_END_DECLS

#endif /* __BASE_APPLICATION_H__ */
