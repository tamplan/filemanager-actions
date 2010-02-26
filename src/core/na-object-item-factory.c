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

#include <api/na-ifactory-object-data.h>
#include <api/na-data-def.h>
#include <api/na-data-types.h>

static void free_items_list( void * list );

NADataDef data_def_item [] = {

	{ NAFO_DATA_LABEL,
				TRUE,
				"NAObjectItem label",
				"Main label of the NAObjectItem object. " \
				"Serves as a default for the toolbar label of an action.",
				NAFD_TYPE_LOCALE_STRING,
				"",
				TRUE,
				TRUE,
				FALSE,
				TRUE,
				"label",
				NULL,
				FALSE },

	{ NAFO_DATA_TOOLTIP,
				TRUE,
				"Item tooltip",
				"Tooltip associated to the item in the context menu or in the toolbar.",
				NAFD_TYPE_LOCALE_STRING,
				"",
				TRUE,
				TRUE,
				FALSE,
				TRUE,
				"tooltip",
				NULL,
				FALSE },

	{ NAFO_DATA_ICON,
				TRUE,
				"Icon name",
				"Icon displayed in the context menu and in the toolbar. " \
				"May be the name of a themed icon, or the full path to any appropriate image.",
				NAFD_TYPE_LOCALE_STRING,
				"",
				TRUE,
				TRUE,
				FALSE,
				TRUE,
				"icon",
				NULL,
				FALSE },

	{ NAFO_DATA_DESCRIPTION,
				TRUE,
				"Description",
				"Some text which explains the goal of the menu or the action. " \
				"Will be used, e.g. when displaying available items on a web site.",
				NAFD_TYPE_LOCALE_STRING,
				"",
				TRUE,
				TRUE,
				FALSE,
				TRUE,
				"description",
				NULL,
				FALSE },

	{ NAFO_DATA_SUBITEMS,
				FALSE,
				"Subitems",
				"List of subitems objects",
				NAFD_TYPE_POINTER,
				NULL,
				FALSE,
				FALSE,
				FALSE,
				FALSE,
				NULL,
				free_items_list,
				FALSE },

	{ NAFO_DATA_SUBITEMS_SLIST,
				TRUE,
				"Subitems",
				"List of subitems ids, " \
				"as readen from corresponding entry from the storage subsystem.",
				NAFD_TYPE_STRING_LIST,
				NULL,
				FALSE,
				FALSE,
				FALSE,
				FALSE,
				"items",
				NULL,
				FALSE },

	{ NAFO_DATA_ENABLED,
				TRUE,
				"Enabled",
				"Is the item enabled ? " \
				"When FALSE, the item will never be candidate to the context menu," \
				"nor to the toolbar.",
				NAFD_TYPE_BOOLEAN,
				"true",
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"enabled",
				NULL,
				FALSE },

	{ NAFO_DATA_READONLY,
				FALSE,
				"Read-only",
				"Is the item only readable ? " \
				"This is an intrinsic property, dynamically set when the item is unserialized. " \
				"This property being FALSE doesn't mean that the item will actually be updatable, " \
				"as this also depend of parameters set by user and administrator. " \
				"Also, a property initially set to FALSE when first unserializing may be set to" \
				"TRUE if an eccor occurs on a later write operation.",
				NAFD_TYPE_BOOLEAN,
				"false",
				TRUE,
				FALSE,
				FALSE,
				FALSE,
				NULL,
				NULL,
				FALSE },

	{ NAFO_DATA_PROVIDER,
				FALSE,
				"I/O provider",
				"A pointer to the NAIOProvider object.",
				NAFD_TYPE_POINTER,
				NULL,
				TRUE,
				FALSE,
				FALSE,
				FALSE,
				NULL,
				NULL,
				FALSE },

	{ NAFO_DATA_PROVIDER_DATA,
				FALSE,
				"I/O provider data",
				"A pointer to some NAIOProvider specific data.",
				NAFD_TYPE_POINTER,
				NULL,
				TRUE,
				FALSE,
				FALSE,
				FALSE,
				NULL,
				NULL,
				FALSE },

	{ NULL },
};

static void
free_items_list( void *list )
{
	static const gchar *thisfn = "na_object_item_factory_free_items_list";

	g_debug( "%s: list=%p (count=%d)", thisfn, list, g_list_length(( GList * ) list ));

	if( list ){
		/*g_list_free(( GList * ) list );*/
	}
}
