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

NadfIdType item_iddef [] = {

	{ NADF_DATA_TOOLTIP,
				"na-object-tooltip",
				TRUE,
				"Item tooltip",
				"Tooltip associated to the item in the context menu or in the toolbar.",
				NADF_TYPE_LOCALE_STRING,
				"",
				TRUE,
				TRUE },

	{ NADF_DATA_ICON,
				"na-object-icon",
				TRUE,
				"Icon name",
				"Icon displayed in the context menu and in the toolbar. " \
				"May be the name of a themed icon, or the full path to any appropriate image.",
				NADF_TYPE_LOCALE_STRING,
				"",
				TRUE,
				TRUE },

	{ NADF_DATA_DESCRIPTION,
				"na-object-description",
				TRUE,
				"Description",
				"Some text which explains the goal of the menu or the action. " \
				"Will be used, e.g. when displaying available items on a web site.",
				NADF_TYPE_LOCALE_STRING,
				"",
				TRUE,
				TRUE },

	{ NADF_DATA_SUBITEMS,
				"na-object-subitems",
				FALSE,
				"Subitems",
				"List of subitems objects",
				NADF_TYPE_POINTER,
				NULL,
				FALSE,
				FALSE },

	{ NADF_DATA_SUBITEMS_SLIST,
				"na-object-subitems-slist",
				TRUE,
				"Subitems",
				"List of subitems ids, " \
				"as readen from corresponding entry from the storage subsystem.",
				NADF_TYPE_STRING_LIST,
				"",
				FALSE,
				FALSE },

	{ NADF_DATA_ENABLED,
				"na-object-enabled",
				TRUE,
				"Enabled",
				"Is the item enabled ? " \
				"When FALSE, the item will never be candidate to the context menu," \
				"nor to the toolbar.",
				NADF_TYPE_BOOLEAN,
				"TRUE",
				TRUE,
				TRUE },

	{ NADF_DATA_READONLY,
				"na-object-readonly",
				FALSE,
				"Read-only",
				"Is the item only readable ? " \
				"This is an intrinsic property, dynamically set when the item is unserialized. " \
				"This property being FALSE doesn't mean that the item will actually be updatable, " \
				"as this also depend of parameters set by user and administrator. " \
				"Also, a property initially set to FALSE when first unserializing may be set to" \
				"TRUE if an eccor occurs on a later write operation.",
				NADF_TYPE_BOOLEAN,
				"FALSE",
				TRUE,
				FALSE },

	{ NADF_DATA_PROVIDER,
				"na-object-provider",
				FALSE,
				"I/O provider",
				"A pointer to the NAIOProvider object.",
				NADF_TYPE_POINTER,
				NULL,
				TRUE,
				FALSE },

	{ NADF_DATA_PROVIDER_DATA,
				"na-object-provider-data",
				FALSE,
				"I/O provider data",
				"A pointer to some NAIOProvider specific data.",
				NADF_TYPE_POINTER,
				NULL,
				TRUE,
				FALSE },

	{ 0, NULL, FALSE, NULL, NULL, 0, NULL, FALSE, FALSE },
};
