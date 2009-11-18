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

#include "nact-application.h"

static void na_log_handler( const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data );

int
main( int argc, char *argv[] )
{
	NactApplication *app;
	int ret;

	g_log_set_handler( NA_LOGDOMAIN_NACT, G_LOG_LEVEL_DEBUG, na_log_handler, NULL );
	g_log_set_handler( NA_LOGDOMAIN_PRIVATE, G_LOG_LEVEL_DEBUG, na_log_handler, NULL );
	g_log_set_handler( NA_LOGDOMAIN_RUNTIME, G_LOG_LEVEL_DEBUG, na_log_handler, NULL );

	app = nact_application_new_with_args( argc, argv );

	ret = base_application_run( BASE_APPLICATION( app ));

	g_object_unref( app );

	return( ret );
}

static void
na_log_handler( const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data )
{
#ifdef NA_MAINTAINER_MODE
	g_log_default_handler( log_domain, log_level, message, user_data );
#else
	/* do nothing */
#endif
}
