/*
 * FileManager-Actions
 * A file-manager extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
 *
 * FileManager-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * FileManager-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FileManager-Actions; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Frederic Ruaudel <grumz@grumz.net>
 *   Rodrigo Moya <rodrigo@gnome-db.org>
 *   Pierre Wieser <pwieser@trychlos.org>
 *   ... and many others (see AUTHORS)
 */

#ifndef __FILEMANAGER_ACTIONS_API_IFACTORY_OBJECT_DATA_H__
#define __FILEMANAGER_ACTIONS_API_IFACTORY_OBJECT_DATA_H__

/**
 * SECTION: data-name
 * @title: Constants
 * @short_description: The Data Factory Constant Definitions
 * @include: filemanager-actions/fma-ifactory-object-data.h
 *
 * Each elementary data get its own name here.
 *
 * Through #FMADataDef and #FMADataGroup definitions, each #FMAObjectItem
 * derived object which implement the #FMAIFactoryObject interface will
 * dynamically define a property for each attached elementary data.
 */

#include <glib.h>

G_BEGIN_DECLS

/**
 * FMA_FACTORY_OBJECT_ID_GROUP:
 *
 * #FMAObjectId common data.
 */
#define FMA_FACTORY_OBJECT_ID_GROUP          "factory-group-id"
#define FMAFO_DATA_ID                        "factory-data-id"
#define FMAFO_DATA_LABEL                     "factory-data-label"
#define FMAFO_DATA_PARENT                    "factory-data-parent"
#define FMAFO_DATA_CONDITIONS                "factory-data-conditions"

/**
 * FMA_FACTORY_OBJECT_ITEM_GROUP:
 *
 * #FMAObjectItem common data.
 */
#define FMA_FACTORY_OBJECT_ITEM_GROUP        "factory-group-item"
#define FMAFO_DATA_IVERSION                  "factory-data-iversion"
#define FMAFO_DATA_TYPE                      "factory-data-type"
#define FMAFO_DATA_TOOLTIP                   "factory-data-tooltip"
#define FMAFO_DATA_ICON                      "factory-data-icon"
#define FMAFO_DATA_ICON_NOLOC                "factory-data-unlocalized-icon"
#define FMAFO_DATA_DESCRIPTION               "factory-data-description"
#define FMAFO_DATA_SHORTCUT                  "factory-data-shortcut"
#define FMAFO_DATA_SUBITEMS                  "factory-data-items"
#define FMAFO_DATA_SUBITEMS_SLIST            "factory-data-items-slist"
#define FMAFO_DATA_ENABLED                   "factory-data-enabled"
#define FMAFO_DATA_READONLY                  "factory-data-readonly"
#define FMAFO_DATA_PROVIDER                  "factory-data-provider"
#define FMAFO_DATA_PROVIDER_DATA             "factory-data-provider-data"

/**
 * FMA_FACTORY_OBJECT_ACTION_GROUP:
 *
 * #FMAObjectAction specific datas.
 */
#define FMA_FACTORY_OBJECT_ACTION_GROUP      "factory-group-action"
#define FMAFO_DATA_VERSION                   "factory-data-version"
#define FMAFO_DATA_TARGET_SELECTION          "factory-data-target-selection"
#define FMAFO_DATA_TARGET_LOCATION           "factory-data-target-location"
#define FMAFO_DATA_TARGET_TOOLBAR            "factory-data-target-toolbar"
#define FMAFO_DATA_TOOLBAR_LABEL             "factory-data-toolbar-label"
#define FMAFO_DATA_TOOLBAR_SAME_LABEL        "factory-data-toolbar-same-label"
#define FMAFO_DATA_LAST_ALLOCATED            "factory-data-last-allocated"

/**
 * FMA_FACTORY_ACTION_V1_GROUP:
 *
 * A group of datas which are specific to v 1 actions. It happens to be
 * empty as all these datas have been alter embedded in #FMAObjectItem
 * data group.
 */
#define FMA_FACTORY_ACTION_V1_GROUP          "factory-group-action-v1"

/**
 * FMA_FACTORY_OBJECT_MENU_GROUP:
 *
 * #FMAObjectMenu specific datas. It happens to be empty as the definition
 * of a menu is very close of those of an action.
 */
#define FMA_FACTORY_OBJECT_MENU_GROUP        "factory-group-menu"

/**
 * FMA_FACTORY_OBJECT_PROFILE_GROUP:
 *
 * #FMAObjectProfile specific datas.
 */
#define FMA_FACTORY_OBJECT_PROFILE_GROUP     "factory-group-profile"
#define FMAFO_DATA_DESCNAME                  "factory-data-descname"
#define FMAFO_DATA_DESCNAME_NOLOC            "factory-data-unlocalized-descname"
#define FMAFO_DATA_PATH                      "factory-data-path"
#define FMAFO_DATA_PARAMETERS                "factory-data-parameters"
#define FMAFO_DATA_WORKING_DIR               "factory-data-working-dir"
#define FMAFO_DATA_EXECUTION_MODE            "factory-data-execution-mode"
#define FMAFO_DATA_STARTUP_NOTIFY            "factory-data-startup-notify"
#define FMAFO_DATA_STARTUP_WMCLASS           "factory-data-startup-wm-class"
#define FMAFO_DATA_EXECUTE_AS                "factory-data-execute-as"

/**
 * FMA_FACTORY_OBJECT_CONDITIONS_GROUP:
 *
 * The datas which determine the display conditions of a menu or an action.
 *
 * @see_also: #FMAIContext interface.
 */
#define FMA_FACTORY_OBJECT_CONDITIONS_GROUP  "factory-group-conditions"
#define FMAFO_DATA_BASENAMES                 "factory-data-basenames"
#define FMAFO_DATA_MATCHCASE                 "factory-data-matchcase"
#define FMAFO_DATA_MIMETYPES                 "factory-data-mimetypes"
#define FMAFO_DATA_MIMETYPES_IS_ALL          "factory-data-all-mimetypes"
#define FMAFO_DATA_ISFILE                    "factory-data-isfile"
#define FMAFO_DATA_ISDIR                     "factory-data-isdir"
#define FMAFO_DATA_MULTIPLE                  "factory-data-multiple"
#define FMAFO_DATA_SCHEMES                   "factory-data-schemes"
#define FMAFO_DATA_FOLDERS                   "factory-data-folders"
#define FMAFO_DATA_SELECTION_COUNT           "factory-data-selection-count"
#define FMAFO_DATA_ONLY_SHOW                 "factory-data-only-show-in"
#define FMAFO_DATA_NOT_SHOW                  "factory-data-not-show-in"
#define FMAFO_DATA_TRY_EXEC                  "factory-data-try-exec"
#define FMAFO_DATA_SHOW_IF_REGISTERED        "factory-data-show-if-registered"
#define FMAFO_DATA_SHOW_IF_TRUE              "factory-data-show-if-true"
#define FMAFO_DATA_SHOW_IF_RUNNING           "factory-data-show-if-running"
#define FMAFO_DATA_CAPABILITITES             "factory-data-capabilitites"

G_END_DECLS

#endif /* __FILEMANAGER_ACTIONS_API_IFACTORY_OBJECT_DATA_H__ */
