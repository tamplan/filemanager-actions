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

extern NADataDef data_def_id [];			/* defined in na-object-id-factory.c */

static NADataDef data_def_profile [] = {

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
				FALSE },

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
				FALSE },

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
				FALSE },

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
				FALSE },

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
				FALSE },

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
				FALSE },

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
				FALSE },

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
				FALSE },

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
				FALSE },

	{ NAFO_DATA_FOLDERS,
				TRUE,
				"Folders",
				"The list of folders which apply when there is no selection." \
				"Defaults to '/'.",
				NAFD_TYPE_STRING_LIST,
				"/",
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"folders",
				NULL,
				FALSE },

	{ NULL },
};

NADataGroup profile_data_groups [] = {
	{ NA_FACTORY_OBJECT_ID_GROUP,         data_def_id },
	{ NA_FACTORY_OBJECT_PROFILE_GROUP,    data_def_profile },
	{ NA_FACTORY_OBJECT_CONDITIONS_GROUP, NULL },
	{ NULL }
};
