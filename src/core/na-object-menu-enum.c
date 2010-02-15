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
#include <api/na-idata-factory-str.h>

extern NadfIdType id_iddef [];			/* defined in na-object-id-enum.c */
extern NadfIdType item_iddef [];		/* defined in na-object-item-enum.c */

static NadfIdType menu_iddef [] = {

	{ 0, NULL, FALSE, NULL, NULL, 0, NULL, FALSE, FALSE },
};

NadfIdGroup menu_id_groups [] = {
	{ NA_DATA_FACTORY_ID_GROUP,         id_iddef },
	{ NA_DATA_FACTORY_ITEM_GROUP,       item_iddef },
	{ NA_DATA_FACTORY_MENU_GROUP,       menu_iddef },
	{ NA_DATA_FACTORY_CONDITIONS_GROUP, NULL },
	{ 0, NULL }
};
