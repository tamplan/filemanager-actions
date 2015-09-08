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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <api/fma-extension.h>

#include "naxml-provider.h"

/* the count of GType types provided by this extension
 * each new GType type must
 * - be registered in fma_extension_startup()
 * - be addressed in fma_extension_list_types().
 */
#define FMA_TYPES_COUNT	1

/*
 * fma_extension_startup:
 *
 * mandatory starting with API v. 1.
 */
gboolean
fma_extension_startup( GTypeModule *module )
{
	static const gchar *thisfn = "fma_xml_module_fma_extension_startup";

	g_debug( "%s: module=%p", thisfn, ( void * ) module );

	naxml_provider_register_type( module );

	return( TRUE );
}

/*
 * fma_extension_get_version:
 *
 * optional, defaults to 1.
 */
guint
fma_extension_get_version( void )
{
	static const gchar *thisfn = "fma_xml_module_fma_extension_get_version";
	guint version;

	version = 1;

	g_debug( "%s: version=%d", thisfn, version );

	return( version );
}

/*
 * fma_extension_list_types:
 *
 * mandatory starting with v. 1.
 */
guint
fma_extension_list_types( const GType **types )
{
	static const gchar *thisfn = "fma_xml_module_fma_extension_list_types";
	static GType types_list [1+FMA_TYPES_COUNT];

	g_debug( "%s: types=%p", thisfn, ( void * ) types );

	types_list[0] = NAXML_TYPE_PROVIDER;

	types_list[FMA_TYPES_COUNT] = 0;
	*types = types_list;

	return( FMA_TYPES_COUNT );
}

/*
 * fma_extension_shutdown:
 *
 * mandatory starting with v. 1.
 */
void
fma_extension_shutdown( void )
{
	static const gchar *thisfn = "fma_xml_module_fma_extension_shutdown";

	g_debug( "%s", thisfn );
}
