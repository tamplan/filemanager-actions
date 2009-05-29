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
#include <syslog.h>
#include <libnautilus-extension/nautilus-extension-types.h>
#include "nautilus-actions.h"

static guint st_log_handler = 0;

#ifdef NACT_MAINTAINER_MODE
static void nact_log_handler( const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data );
#endif

/*
 * A nautilus extension must implement three functions :
 *
 * - nautilus_module_initialize
 * - nautilus_module_list_types
 * - nautilus_module_shutdown
 *
 * The first two functions are called at nautilus startup.
 *
 * The prototypes for these functions are defined in nautilus-extension-types.h
 */

void
nautilus_module_initialize( GTypeModule *module )
{
#ifdef NACT_MAINTAINER_MODE
	/*
	 *  install a debug log handler
	 * (if development mode and not already done)
	 */
	if( !st_log_handler ){
		openlog( G_LOG_DOMAIN, LOG_PID, LOG_USER );
		st_log_handler = g_log_set_handler( G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, nact_log_handler, NULL );
	}
#endif

	static const gchar *thisfn = "nautilus_module_initialize";
	g_debug( "%s: module=%p", thisfn, module );

	nautilus_actions_register_type( module );
}

void
nautilus_module_list_types( const GType **types, int *num_types )
{
	static const gchar *thisfn = "nautilus_module_list_types";
	g_debug( "%s: types=%p, num_types=%p", thisfn, types, num_types );

	static GType type_list[1];

	type_list[0] = NAUTILUS_ACTIONS_TYPE;
	*types = type_list;

	*num_types = 1;
}

void
nautilus_module_shutdown( void )
{
	static const gchar *thisfn = "nautilus_module_shutdown";
	g_debug( "%s", thisfn );

	/* remove the log handler
	 * almost useless as the process is nonetheless terminating at this time
	 * but this is the art of coding...
	 */
	if( st_log_handler ){
		g_log_remove_handler( G_LOG_DOMAIN, st_log_handler );
		st_log_handler = 0;
	}
}

/*
 * a log handler that we install when in development mode in order to be
 * able to log plugin runtime
 * TODO: the debug flag should be dynamic, so that an advanced user could
 * setup a given key and obtain a full log to send to Bugzilla..
 * For now, is always install when compiled in maintainer mode, never else
 */
#ifdef NACT_MAINTAINER_MODE
static void
nact_log_handler( const gchar *log_domain,
					GLogLevelFlags log_level,
					const gchar *message,
					gpointer user_data )
{
	syslog( LOG_USER | LOG_DEBUG, "%s", message );
}
#endif
