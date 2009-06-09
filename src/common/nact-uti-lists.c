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
#include <glib-object.h>
#include "nact-uti-lists.h"

/**
 * Duplicates a GSList of strings.
 *
 * @list: the GSList to be duplicated.
 */
GSList *
nactuti_duplicate_string_list( GSList *list )
{
	GSList *duplist = NULL;
	GSList *it;
	for( it = list ; it != NULL ; it = it->next ){
		gchar *dupstr = g_strdup(( gchar * ) it->data );
		duplist = g_slist_prepend( duplist, dupstr );
	}
	return( duplist );
}

/**
 * Frees a GSList of strings.
 *
 * @list: the GSList to be freed.
 */
void
nactuti_free_string_list( GSList *list )
{
	GSList *item;
	for( item = list ; item != NULL ; item = item->next ){
		g_free(( gchar * ) item->data );
	}
	g_slist_free( list );
}
