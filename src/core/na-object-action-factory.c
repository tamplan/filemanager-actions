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

extern NADataDef data_def_id[];			/* defined in na-object-id-factory.c */
extern NADataDef data_def_item [];		/* defined in na-object-item-factory.c */

static NADataDef data_def_action [] = {

	{ NAFO_DATA_VERSION,
				TRUE,
				"Action version",
				"Version of the action",
				NAFD_TYPE_STRING,
				"2.0",
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"version",
				NULL,
				FALSE },

	{ NAFO_DATA_TARGET_SELECTION,
				TRUE,
				"Target a selection context menu",
				"Does the action target the context menu when there is some selection ?",
				NAFD_TYPE_BOOLEAN,
				"TRUE",
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"target-selection",
				NULL,
				FALSE },

	{ NAFO_DATA_TARGET_BACKGROUND,
				TRUE,
				"Target the folder context menu",
				"Does the action target the context menu when there is no selection ?",
				NAFD_TYPE_BOOLEAN,
				"FALSE",
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"target-background",
				NULL,
				FALSE },

	{ NAFO_DATA_TARGET_TOOLBAR,
				TRUE,
				"Target the toolbar",
				"Does the action target the toolbar ? " \
				"Only an action may target the toolbar as Nautilus, as of 2.28, " \
				"doesn't support menus in toolbar.",
				NAFD_TYPE_BOOLEAN,
				"FALSE",
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"target-toolbar",
				NULL,
				FALSE },

	{ NAFO_DATA_TOOLBAR_LABEL,
				TRUE,
				"Toolbar label",
				"Label of the action in the toolbar. " \
				"Defaults to main label if empty or not set.",
				NAFD_TYPE_LOCALE_STRING,
				"",
				TRUE,
				TRUE,
				FALSE,
				TRUE,
				"toolbar-label",
				NULL,
				FALSE },

	{ NAFO_DATA_TOOLBAR_SAME_LABEL,
				FALSE,
				"Does the toolbar label is the same than the main one ?",
				"Does the toolbar label is the same than the main one ?",
				NAFD_TYPE_BOOLEAN,
				"true",
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"toolbar-same-label",
				NULL,
				FALSE },

	{ NAFO_DATA_LAST_ALLOCATED,
				FALSE,
				"Last allocated profile",
				"Last allocated profile number in na_object_action_get_new_profile_name(), " \
				"reset to zero when saving the action.",
				NAFD_TYPE_UINT,
				"0",
				TRUE,
				FALSE,
				FALSE,
				FALSE,
				NULL,
				NULL,
				FALSE },

	{ NULL },
};

static NADataDef data_def_obsoleted_action [] = {

	{ NAFO_DATA_PATH,
				TRUE,
				"Command path",
				"The path to the command.",
				NAFD_TYPE_STRING,
				"",
				TRUE,
				TRUE,
				TRUE,
				FALSE,
				"path",
				NULL,
				TRUE },

	{ NAFO_DATA_PARAMETERS,
				TRUE,
				"Command parameters",
				"The parameters of the command.",
				NAFD_TYPE_STRING,
				"",
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"parameters",
				NULL,
				TRUE },

	{ NAFO_DATA_BASENAMES,
				TRUE,
				"Basenames",
				"The basenames the selection must match. " \
				"Defaults to '*'.",
				NAFD_TYPE_STRING_LIST,
				"*",
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"basenames",
				NULL,
				TRUE },

	{ NAFO_DATA_MATCHCASE,
				TRUE,
				"Case sensitive",
				"Whether the specified basenames are case sensitive." \
				"Defaults to 'true'.",
				NAFD_TYPE_BOOLEAN,
				"TRUE",
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"matchcase",
				NULL,
				TRUE },

	{ NAFO_DATA_MIMETYPES,
				TRUE,
				"Mimetypes",
				"The mimetypes the selection must match." \
				"Defaults to '*'.",
				NAFD_TYPE_STRING_LIST,
				"*",
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"mimetypes",
				NULL,
				TRUE },

	{ NAFO_DATA_ISFILE,
				TRUE,
				"Applies to files only",
				"Whether the profile only applies to files." \
				"Defaults to 'true'",
				NAFD_TYPE_BOOLEAN,
				"TRUE",
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"isfile",
				NULL,
				TRUE },

	{ NAFO_DATA_ISDIR,
				TRUE,
				"Applies to directories only",
				"Whether the profile applies to directories only." \
				"Defaults to 'false'",
				NAFD_TYPE_BOOLEAN,
				"FALSE",
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"isdir",
				NULL,
				TRUE },

	{ NAFO_DATA_MULTIPLE,
				TRUE,
				"Multiple selection",
				"Whether the selection may be multiple." \
				"Defaults to 'false'.",
				NAFD_TYPE_BOOLEAN,
				"FALSE",
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"accept-multiple-files",
				NULL,
				TRUE },

	{ NAFO_DATA_SCHEMES,
				TRUE,
				"Schemes",
				"The list of schemes the selection must match." \
				"Defaults to 'file'.",
				NAFD_TYPE_STRING_LIST,
				"file",
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"schemes",
				NULL,
				TRUE },

	{ NULL },
};

NADataGroup action_data_groups [] = {
	{ NA_FACTORY_OBJECT_ID_GROUP,         data_def_id },
	{ NA_FACTORY_OBJECT_ITEM_GROUP,       data_def_item },
	{ NA_FACTORY_OBJECT_ACTION_GROUP,     data_def_action },
	{ NA_FACTORY_OBSOLETED_ACTION_GROUP,  data_def_obsoleted_action },
	{ NA_FACTORY_OBJECT_CONDITIONS_GROUP, NULL },
	{ NULL }
};
