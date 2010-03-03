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

#include <glib/gi18n.h>

#include <api/na-ifactory-object-data.h>
#include <api/na-data-def.h>
#include <api/na-data-types.h>

NADataDef data_def_item [] = {

	/* this data is marked as 'non-serializable'
	 * this means it will not be automatically readen/written or imported/exported
	 * the corresponding NADataBoxed is created at read_start() time
	 */
	{ NAFO_DATA_TYPE,
				FALSE,
				N_( "Type of the item" ),
				N_( "Defines if the item is an action or a menu. Possible values are :\n" \
					"- 'Action',\n" \
					"- 'Menu'.\n" \
					"The value is case sensitive and must not be localized." ),
				NAFD_TYPE_LOCALE_STRING,
				NULL,
				TRUE,
				FALSE,
				FALSE,
				FALSE,
				"type",
				FALSE },

	{ NAFO_DATA_LABEL,
				TRUE,
				N_( "Label of the context menu item" ),
				N_( "The label of the menu item that will appear in the file manager context " \
					"menu when the selection matches the appearance condition settings.\n" \
					"It is also used as a default for the toolbar label of an action." ),
				NAFD_TYPE_LOCALE_STRING,
				"",
				TRUE,
				TRUE,
				FALSE,
				TRUE,
				"label",
				FALSE },

	{ NAFO_DATA_TOOLTIP,
				TRUE,
				N_( "Tooltip of the context menu item" ),
				N_( "The tooltip of the menu item that will appear in the file manager " \
					"statusbar when the user points to the file manager context menu item " \
					"with his/her mouse." ),
				NAFD_TYPE_LOCALE_STRING,
				"",
				TRUE,
				TRUE,
				FALSE,
				TRUE,
				"tooltip",
				FALSE },

	{ NAFO_DATA_ICON,
				TRUE,
				N_( "Icon of the context menu item" ),
				N_( "The icon of the menu item that will appear next to the label " \
					"in the file manager context menu when the selection matches the appearance " \
					"conditions settings.\n" \
					"May be the localized name of a themed icon, or a full path to any appropriate image." ),
				NAFD_TYPE_LOCALE_STRING,
				"",
				TRUE,
				TRUE,
				FALSE,
				TRUE,
				"icon",
				FALSE },

	{ NAFO_DATA_DESCRIPTION,
				TRUE,
				N_( "Description relative to the item" ),
				N_( "Some text which explains the goal of the menu or the action.\n" \
					"May be used, e.g. when displaying available items on a web site." ),
				NAFD_TYPE_LOCALE_STRING,
				"",
				TRUE,
				TRUE,
				FALSE,
				TRUE,
				"description",
				FALSE },

	{ NAFO_DATA_SUBITEMS,
				FALSE,			/* not serializable */
				"Subitems",
				"List of subitems objects",
				NAFD_TYPE_POINTER,
				NULL,
				FALSE,			/* not copyable */
				FALSE,			/* not comparable */
				FALSE,			/* not mandatory */
				FALSE,			/* not localized */
				NULL,
				FALSE },

	{ NAFO_DATA_SUBITEMS_SLIST,
				TRUE,
				N_( "List of subitem ids" ),
				N_( "Ordered list of the IDs of the subitems. This may be actions or menus " \
					"if the item is a menu, or profiles if the item is an action.\n" \
					"If this list doesn't exist or is empty for an action or a menu, " \
					"subitems are attached in the order of the read operations." ),
				NAFD_TYPE_STRING_LIST,
				NULL,
				FALSE,
				FALSE,
				FALSE,
				FALSE,
				"items",
				FALSE },

	{ NAFO_DATA_ENABLED,
				TRUE,
				N_( "Whether the action or the menu is enabled" ),
				N_( "If the or the menu action is disabled, it will never appear in the " \
					"file manager context menu." ),
				NAFD_TYPE_BOOLEAN,
				"true",
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"enabled",
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
				FALSE },

	{ NULL },
};
