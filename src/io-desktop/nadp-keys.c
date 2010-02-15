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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <api/na-idata-factory-enum.h>

#include "nadp-keys.h"

static NadpIdKey id_key [] = {
	{ NADF_DATA_LABEL,   NADP_GROUP_DESKTOP, NADP_KEY_NAME },
	{ NADF_DATA_TOOLTIP, NADP_GROUP_DESKTOP, NADP_KEY_TOOLTIP },
	{ NADF_DATA_ICON,    NADP_GROUP_DESKTOP, NADP_KEY_ICON },
	{ 0, NULL, NULL }
};

/**
 * nadp_keys_get_group_and_key:
 * @iddef:
 *
 * Set: the group and the key to be used for this @iddef.
 *
 * Returns: %TRUE if the data has been found, %FALSE else.
 */
gboolean
nadp_keys_get_group_and_key( const NadfIdType *iddef, gchar **group, gchar **key )
{
	gboolean found;
	int i;

	found = FALSE;
	*group = NULL;
	*key = NULL;

	for( i = 0 ; id_key[i].data_id && !found ; ++i ){

		if( id_key[i].data_id == iddef->id ){

			*group = g_strdup( id_key[i].group );
			*key = g_strdup( id_key[i].key );
			found = TRUE;
		}
	}

	return( found );
}
