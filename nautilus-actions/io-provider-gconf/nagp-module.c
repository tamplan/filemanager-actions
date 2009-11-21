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

#include <nautilus-actions/api/na-api.h>

#include "nagp-gconf-provider.h"

/*
 * A Nautilus-Actions extension must implement four functions :
 *
 * - na_api_module_init
 * - na_api_module_list_types
 * - na_api_module_get_name
 * - na_api_module_shutdown
 *
 * The first two functions are called at Nautilus-Actions startup.
 *
 * The prototypes for these functions are defined in
 * nautilus-actions/api/na-api.h
 */

gboolean
na_api_module_init( GTypeModule *module )
{
	static const gchar *thisfn = "nagp_module_na_api_module_initialize";
	static const gchar *name = "NagpGConfIOProvider";

	g_debug( "%s: module=%p", thisfn, ( void * ) module );

	g_type_module_set_name( module, name );

	nagp_gconf_provider_register_type( module );

	return( TRUE );
}

gint
na_api_module_list_types( const GType **types )
{
	static const gchar *thisfn = "nagp_module_na_api_module_list_types";
	#define count 1
	static GType type_list[count];

	g_debug( "%s: types=%p", thisfn, ( void * ) types );

	type_list[0] = NAGP_GCONF_PROVIDER_TYPE;
	*types = type_list;

	return( count );
}

const gchar *
na_api_module_get_name( GType type )
{
	static const gchar *thisfn = "nagp_module_na_api_module_get_name";

	g_debug( "%s: type=%ld", thisfn, ( gulong ) type );

	if( type == NAGP_GCONF_PROVIDER_TYPE ){
		return( "Nautilus-Actions GConf IO Provider" );
	}

	return( NULL );
}

void
na_api_module_shutdown( void )
{
	static const gchar *thisfn = "nagp_module_na_api_module_shutdown";

	g_debug( "%s", thisfn );
}
