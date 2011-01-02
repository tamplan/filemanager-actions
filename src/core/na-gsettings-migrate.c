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

#include "na-gsettings-migrate.h"

static void migrate_configurations( const NAPivot *pivot, GConfClient *gconf );
static void migrate_io_providers( const NAPivot *pivot, GConfClient *gconf );
static void migrate_preferences( const NAPivot *pivot, GConfClient *gconf );

/*
 * This function is called at NAPivot construction, after plugins have been
 * loaded. As both GConf and GSettings are enabled, it is time to migrate
 * runtime parameters and existing NAObjectItem objects if this does not have
 * already done.
 */
void
na_pivot_gsettings_migrate( const NAPivot *pivot )
{
	static const gchar *thisfn = "na_pivot_gsettings_migrate";
	GConfClient *gconf_client;
	GSList *root, *ir;
	const gchar *bname;

	gconf_client = gconf_client_get_default();
	if( !gconf_client ){
		return;
	}

	/* root is the first level of subdirectories:
	 * configurations, io-providers and preferences
	 */
	root = na_gconf_utils_get_subdirs( gconf_client, IPREFS_GCONF_BASEDIR );

	if( root && g_slist_length( root )){
		for( ir = root ; ir ; ir = ir->next ){

			const gchar *branch = ( const gchar * ) ir->data;
			bname = g_path_get_basename( branch );

			if( !strcmp( bname, "configurations" )){
				migrate_configurations( pivot, gconf_client );

			} else if( !strcmp( bname, "io-providers")){
				migrate_io_providers( pivot, gconf_client );

			} else if( !strcmp( bname, "preferences")){
				migrate_preferences( pivot, gconf_client );

			} else {
				g_warning( "%s: unknown branch: %s", thisfn, branch );
			}

			g_free( bname );
		}

		na_core_utils_slist_free( root );
	}

	g_object_unref( gconf_client );
}

static void
migrate_configurations( const NAPivot *pivot, GConfClient *gconf )
{
}

static void
migrate_io_providers( const NAPivot *pivot, GConfClient *gconf )
{
}

static void
migrate_preferences( const NAPivot *pivot, GConfClient *gconf )
{
}
