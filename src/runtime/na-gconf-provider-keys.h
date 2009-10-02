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

#ifndef __NA_RUNTIME_GCONF_PROVIDER_KEYS_H__
#define __NA_RUNTIME_GCONF_PROVIDER_KEYS_H__

#include "na-gconf-keys.h"

/* GConf general information
 */
#define NA_GCONF_CONFIG_PATH			NAUTILUS_ACTIONS_GCONF_BASEDIR "/configurations"

/* GConf key names (common to menu and actions)
 */
#define OBJECT_ITEM_LABEL_ENTRY			"label"
#define OBJECT_ITEM_TOOLTIP_ENTRY		"tooltip"
#define OBJECT_ITEM_ICON_ENTRY			"icon"
#define OBJECT_ITEM_ENABLED_ENTRY		"enabled"

/* GConf key names (specific to menu)
 */
#define MENU_ITEMS_ENTRY				"items"

/* GConf key names (specific to action)
 */
#define ACTION_VERSION_ENTRY			"version"

/* GConf key names (specific to profile)
 */
#define ACTION_PROFILE_LABEL_ENTRY		"desc-name"
#define ACTION_PATH_ENTRY				"path"
#define ACTION_PARAMETERS_ENTRY			"parameters"
#define ACTION_BASENAMES_ENTRY			"basenames"
#define ACTION_MATCHCASE_ENTRY			"matchcase"
#define ACTION_MIMETYPES_ENTRY			"mimetypes"
#define ACTION_ISFILE_ENTRY				"isfile"
#define ACTION_ISDIR_ENTRY				"isdir"
#define ACTION_MULTIPLE_ENTRY			"accept-multiple-files"
#define ACTION_SCHEMES_ENTRY			"schemes"

#endif /* __NA_RUNTIME_GCONF_PROVIDER_KEYS_H__ */
