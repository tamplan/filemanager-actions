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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "na-gconf-migration.h"

#define MIGRATION_COMMAND				PKGLIBEXECDIR "/na-gconf2key.sh -delete -nodummy"

/**
 * na_gconf_migration_run:
 *
 * Migrate users actions and menus from GConf to .desktop files.
 * Disable GConf I/O provider both for reading and writing.
 * Migrate users preferences to NASettings.
 *
 * Since: 3.1.0
 */
void
na_gconf_migration_run( void )
{
	static const gchar *thisfn = "na_gconf_migration_run";
	gchar *out, *err;
	GError *error;

	g_debug( "%s: running %s", thisfn, MIGRATION_COMMAND );

	error = NULL;
	if( !g_spawn_command_line_sync( MIGRATION_COMMAND, &out, &err, NULL, &error )){
		g_warning( "%s: %s", thisfn, error->message );
		g_error_free( error );
		error = NULL;

	} else {
		g_debug( "%s: out=%s", thisfn, out );
		g_debug( "%s: err=%s", thisfn, err );
		g_free( out );
		g_free( err );
	}
}
