/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010 Pierre Wieser and others (see AUTHORS)
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

#include <api/na-core-utils.h>

#include <core/na-pivot.h>
#include <core/na-importer.h>

int
main( int argc, char **argv )
{
	g_type_init();

	NAPivot *pivot = na_pivot_new();
	GSList *msg = NULL;
	gchar *uri = "file:///net/pierre/eclipse/nautilus-actions/exports/config_0af5a47e-96d9-441c-a3b8-d1185ced0351.schemas";
	NAObjectItem *item = na_importer_import( pivot, uri, IMPORTER_MODE_ASK, NULL, NULL, &msg );
	if( item ){
		na_object_dump( item );
		g_object_unref( item );
	}
	na_core_utils_slist_dump( msg );
	return( 0 );
}
