/*
 * FileManager-Actions
 * A file-manager extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
 *
 * FileManager-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * FileManager-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FileManager-Actions; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Frederic Ruaudel <grumz@grumz.net>
 *   Rodrigo Moya <rodrigo@gnome-db.org>
 *   Pierre Wieser <pwieser@trychlos.org>
 *   ... and many others (see AUTHORS)
 */

#ifndef __NACT_APPLICATION_H__
#define __NACT_APPLICATION_H__

/**
 * SECTION: nact_application
 * @short_description: #NactApplication class definition.
 * @include: nact/nact-application.h
 *
 * This is the main class for file-manager-actions-config-tool program.
 *
 * The #NactApplication object is instanciated from main() function,
 * then later #g_object_unref() after nact_application_run() has
 * returned.
 *
 * NactApplication is a non-unique application e.g. the user is able
 * to run several instance of the applications, all pointing to the
 * same actions set, potentially being able to simultaneously have
 * different views or to simultaneously act on different subsets.
 */

#include <gtk/gtk.h>

#include "core/fma-updater.h"

G_BEGIN_DECLS

#define NACT_TYPE_APPLICATION                ( nact_application_get_type())
#define NACT_APPLICATION( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_TYPE_APPLICATION, NactApplication ))
#define NACT_APPLICATION_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, NACT_TYPE_APPLICATION, NactApplicationClass ))
#define NACT_IS_APPLICATION( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_TYPE_APPLICATION ))
#define NACT_IS_APPLICATION_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), NACT_TYPE_APPLICATION ))
#define NACT_APPLICATION_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), NACT_TYPE_APPLICATION, NactApplicationClass ))

typedef struct _NactApplicationPrivate       NactApplicationPrivate;

typedef struct {
	/*< private >*/
	GtkApplicationClass     parent;
}
	NactApplicationClass;

typedef struct {
	/*< private >*/
	GtkApplication          parent;
	NactApplicationPrivate *private;
}
	NactApplication;

/**
 * NactExitCode:
 *
 * The code returned by the application.
 *
 * @NACT_EXIT_CODE_PROGRAM = -1: this is a program error code.
 * @NACT_EXIT_CODE_OK = 0:       the program has successfully run, and returns zero.
 * @NACT_EXIT_CODE_ARGS = 1:     unable to interpret command-line options
 * @NACT_EXIT_CODE_WINDOW = 2:   unable to create a window
 */
typedef enum {
	NACT_EXIT_CODE_PROGRAM = -1,
	NACT_EXIT_CODE_OK = 0,
	NACT_EXIT_CODE_ARGS,
	NACT_EXIT_CODE_WINDOW,
}
	NactExitCode;

GType            nact_application_get_type            ( void );

NactApplication *nact_application_new                 ( void );

int              nact_application_run_with_args       ( NactApplication *application,
																int argc,
																GStrv argv );

gchar           *nact_application_get_application_name( const NactApplication *application );

FMAUpdater       *nact_application_get_updater         ( const NactApplication *application );

gboolean         nact_application_is_willing_to_quit  ( const NactApplication *application );

G_END_DECLS

#endif /* __NACT_APPLICATION_H__ */
