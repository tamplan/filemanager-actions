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

static NadfIdType profile_iddef [] = {

	{ NADF_DATA_PATH,
				"na-object-path",
				TRUE,
				"Command path",
				"The path to the command.",
				NADF_TYPE_STRING,
				"",
				TRUE,
				TRUE,
				TRUE,
				NULL },

	{ NADF_DATA_PARAMETERS,
				"na-object-parameters",
				TRUE,
				"Command parameters",
				"The parameters of the command.",
				NADF_TYPE_STRING,
				"",
				TRUE,
				TRUE,
				FALSE,
				NULL },

	{ NADF_DATA_BASENAMES,
				"na-object-basenames",
				TRUE,
				"Basenames",
				"The basenames the selection must match. " \
				"Defaults to '*'.",
				NADF_TYPE_STRING_LIST,
				"*",
				TRUE,
				TRUE,
				FALSE,
				NULL },

	{ NADF_DATA_MATCHCASE,
				"na-object-matchcase",
				TRUE,
				"Case sensitive",
				"Whether the specified basenames are case sensitive." \
				"Defaults to 'true'.",
				NADF_TYPE_BOOLEAN,
				"TRUE",
				TRUE,
				TRUE,
				FALSE,
				NULL },

	{ NADF_DATA_MIMETYPES,
				"na-object-mimetypes",
				TRUE,
				"Mimetypes",
				"The mimetypes the selection must match." \
				"Defaults to '*'.",
				NADF_TYPE_STRING_LIST,
				"*",
				TRUE,
				TRUE,
				FALSE,
				NULL },

	{ NADF_DATA_ISFILE,
				"na-object-isfile",
				TRUE,
				"Applies to files only",
				"Whether the profile only applies to files." \
				"Defaults to 'true'",
				NADF_TYPE_BOOLEAN,
				"TRUE",
				TRUE,
				TRUE,
				FALSE,
				NULL },

	{ NADF_DATA_ISDIR,
				"na-object-isdir",
				TRUE,
				"Applies to directories only",
				"Whether the profile applies to directories only." \
				"Defaults to 'false'",
				NADF_TYPE_BOOLEAN,
				"FALSE",
				TRUE,
				TRUE,
				FALSE,
				NULL },

	{ NADF_DATA_MULTIPLE,
				"na-object-multiple",
				TRUE,
				"Multiple selection",
				"Whether the selection may be multiple." \
				"Defaults to 'false'.",
				NADF_TYPE_BOOLEAN,
				"FALSE",
				TRUE,
				TRUE,
				FALSE,
				NULL },

	{ NADF_DATA_SCHEMES,
				"na-object-schemes",
				TRUE,
				"Schemes",
				"The list of schemes the selection must match." \
				"Defaults to 'file'.",
				NADF_TYPE_STRING_LIST,
				"file",
				TRUE,
				TRUE,
				FALSE,
				NULL },

	{ NADF_DATA_FOLDERS,
				"na-object-folders",
				TRUE,
				"Folders",
				"The list of folders which apply when there is no selection." \
				"Defaults to '/'.",
				NADF_TYPE_STRING_LIST,
				"/",
				TRUE,
				TRUE,
				FALSE,
				NULL },

	{ 0, NULL, FALSE, NULL, NULL, 0, NULL, FALSE, FALSE, FALSE, NULL },
};

NadfIdGroup profile_id_groups [] = {
	{ NA_DATA_FACTORY_ID_GROUP,         id_iddef },
	{ NA_DATA_FACTORY_PROFILE_GROUP,    profile_iddef },
	{ NA_DATA_FACTORY_CONDITIONS_GROUP, NULL },
	{ 0, NULL }
};
