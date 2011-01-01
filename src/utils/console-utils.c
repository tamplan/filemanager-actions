/*
 * Nautilus Actions
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

#include <glib.h>
#include <glib/gi18n.h>

#include "console-utils.h"

static void log_handler( const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data );

static GLogFunc st_default_log_func = NULL;

/**
 * console_cmdline_get_description:
 *
 * Returns: a newly allocated string to be displayed as a description
 * of the usage message. The returned string should be g_free() by the
 * caller.
 */
gchar *
console_cmdline_get_description( void ){
	return( g_strdup_printf( "%s.\n%s", PACKAGE_STRING,
			_( "Bug reports are welcomed at http://bugzilla.gnome.org,"
				" or you may prefer to mail to <maintainer@nautilus-actions.org>.\n" )));
}

/**
 * console_init_log_handler:
 *
 * Initialize log handler so that debug messages are not outputed when
 * not in maintainer mode.
 */
void
console_init_log_handler( void )
{
	st_default_log_func = g_log_set_default_handler(( GLogFunc ) log_handler, NULL );
}

static void
log_handler( const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data )
{
#ifdef NA_MAINTAINER_MODE
	( *st_default_log_func )( log_domain, log_level, message, user_data );
#else
	if( g_getenv( NAUTILUS_ACTIONS_DEBUG )){
		( *st_default_log_func )( log_domain, log_level, message, user_data );
	}
#endif
}
