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

static NadfIdType action_iddef [] = {

	{ NADF_DATA_VERSION,
				"na-object-version",
				TRUE,
				"Action version",
				"Version of the action",
				NADF_TYPE_STRING,
				"2.0",
				TRUE,
				TRUE,
				FALSE,
				NULL },

	{ NADF_DATA_TARGET_SELECTION,
				"na-object-target-selection",
				TRUE,
				"Target a selection context menu",
				"Does the action target the context menu when there is some selection ?",
				NADF_TYPE_BOOLEAN,
				"TRUE",
				TRUE,
				TRUE,
				FALSE,
				NULL },

	{ NADF_DATA_TARGET_BACKGROUND,
				"na-object-target-background",
				TRUE,
				"Target the folder context menu",
				"Does the action target the context menu when there is no selection ?",
				NADF_TYPE_BOOLEAN,
				"FALSE",
				TRUE,
				TRUE,
				FALSE,
				NULL },

	{ NADF_DATA_TARGET_TOOLBAR,
				"na-object-target-toolbar",
				TRUE,
				"Target the toolbar",
				"Does the action target the toolbar ? " \
				"Only an action may target the toolbar as Nautilus, as of 2.28, " \
				"doesn't support menus in toolbar.",
				NADF_TYPE_BOOLEAN,
				"FALSE",
				TRUE,
				TRUE,
				FALSE,
				NULL },

	{ NADF_DATA_TOOLBAR_LABEL,
				"na-object-toolbar-label",
				TRUE,
				"Toolbar label",
				"Label of the action in the toolbar. " \
				"Defaults to main label if empty or not set.",
				NADF_TYPE_LOCALE_STRING,
				"",
				TRUE,
				TRUE,
				FALSE,
				NULL },

	{ NADF_DATA_TOOLBAR_SAME_LABEL,
				"na-object-toolbar-same-label",
				FALSE,
				"Does the toolbar label is the same than the main one ?",
				"Does the toolbar label is the same than the main one ?",
				NADF_TYPE_BOOLEAN,
				"true",
				TRUE,
				TRUE,
				FALSE,
				NULL },

	{ NADF_DATA_LAST_ALLOCATED,
				"na-object-last-allocated",
				FALSE,
				"Last allocated profile",
				"Last allocated profile number in na_object_action_get_new_profile_name(), " \
				"reset to zero when saving the action.",
				NADF_TYPE_UINT,
				"0",
				TRUE,
				FALSE,
				FALSE,
				NULL },

	{ 0, NULL, FALSE, NULL, NULL, 0, NULL, FALSE, FALSE, FALSE, NULL },
};

NadfIdGroup action_id_groups [] = {
	{ NA_DATA_FACTORY_ID_GROUP,         id_iddef },
	{ NA_DATA_FACTORY_ITEM_GROUP,       item_iddef },
	{ NA_DATA_FACTORY_ACTION_GROUP,     action_iddef },
	{ NA_DATA_FACTORY_CONDITIONS_GROUP, NULL },
	{ 0, NULL }
};
