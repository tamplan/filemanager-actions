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

#include "na-importer.h"

extern gboolean iimporter_initialized;		/* defined in na-iimporter.c */
extern gboolean iimporter_finalized;		/* defined in na-iimporter.c */

/**
 * na_importer_import:
 * @items: a #GList of already loaded items.
 * @uri: the source filename URI.
 * @mode: the import mode.
 * @messages: a pointer to a #GSList list of strings; the provider
 *  may append messages to this list, but shouldn't reinitialize it.
 *
 * Exports the specified @item to the target @uri in the required
 * @format.
 *
 * Returns: a newly allocated #NAObjectItem-derived object, or %NULL
 * if an error has been detected.
 */
NAObjectItem *
na_importer_import( GList *items, const gchar *uri, guint mode, GSList **messages )
{
	NAObjectItem *item;

	item = NULL;

	if( iimporter_initialized && !iimporter_finalized ){

	}

	return( item );
}
