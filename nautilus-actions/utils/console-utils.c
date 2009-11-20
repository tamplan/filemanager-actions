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

#include <glib.h>

#include <runtime/na-iabout.h>

#include "console-utils.h"

static void log_handler( const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data );

static GLogFunc st_default_log_func = NULL;

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

/**
 * console_print_version:
 *
 * Prints the current version of the program to stdout.
 */
/*
 * nautilus-actions-new (Nautilus-Actions) v 2.29.1
 * Copyright (C) 2005-2007 Frederic Ruaudel
 * Copyright (C) 2009 Pierre Wieser
 * Nautilus-Actions is free software, licensed under GPLv2 or later.
 */
void
console_print_version( void )
{
	gchar *copyright;

	g_print( "\n" );
	g_print( "%s (%s) v %s\n", g_get_prgname(), PACKAGE_NAME, PACKAGE_VERSION );
	copyright = na_iabout_get_copyright( TRUE );
	g_print( "%s\n", copyright );
	g_free( copyright );

	g_print( "%s is free software, and is provided without any warranty. You may\n", PACKAGE_NAME );
	g_print( "redistribute copies of %s under the terms of the GNU General Public\n", PACKAGE_NAME );
	g_print( "License (see COPYING).\n" );
	g_print( "\n" );
}

static void
log_handler( const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data )
{
#ifdef NA_MAINTAINER_MODE
	( *st_default_log_func )( log_domain, log_level, message, user_data );
#else
	/* do nothing */
#endif
}
