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

#ifndef __NADP_KEYS_H__
#define __NADP_KEYS_H__

#include <api/na-data-def.h>

G_BEGIN_DECLS

#define NADP_GROUP_DESKTOP							G_KEY_FILE_DESKTOP_GROUP
#define NADP_GROUP_PROFILE							"X-Action-Profile"

#define NADP_KEY_TYPE								G_KEY_FILE_DESKTOP_KEY_TYPE
#define NADP_VALUE_TYPE_ACTION						"ExtensionAction"
#define NADP_VALUE_TYPE_MENU						"ExtensionMenu"
#define NADP_KEY_NAME								G_KEY_FILE_DESKTOP_KEY_NAME
#define NADP_KEY_TOOLTIP							"Tooltip"
#define NADP_KEY_ICON								G_KEY_FILE_DESKTOP_KEY_ICON
#define NADP_KEY_DESCRIPTION						"Description"
#define NADP_KEY_SUGGESTED_SHORTCUT					"SuggestedShortcut"

#define NADP_KEY_TARGET_SELECTION					"TargetSelection"
#define NADP_KEY_TARGET_BACKGROUND					"TargetBackground"
#define NADP_KEY_TARGET_CONTEXT						"TargetContext"
#define NADP_KEY_TARGET_TOOLBAR						"TargetToolbar"
#define NADP_KEY_TOOLBAR_LABEL						"ToolbarLabel"
#define NADP_KEY_PROFILES							"Profiles"

#define NADP_KEY_EXEC								G_KEY_FILE_DESKTOP_KEY_EXEC
#define NADP_KEY_PATH								G_KEY_FILE_DESKTOP_KEY_PATH
#define NADP_KEY_EXECUTION_MODE						"ExecutionMode"
#define NADP_VALUE_EXECUTION_MODE_NORMAL			"Normal"
#define NADP_VALUE_EXECUTION_MODE_MINIMIZED			"Minimized"
#define NADP_VALUE_EXECUTION_MODE_MAXIMIZED			"Maximized"
#define NADP_VALUE_EXECUTION_MODE_TERMINAL			"Terminal"
#define NADP_VALUE_EXECUTION_MODE_EMBEDDED			"Embedded"
#define NADP_VALUE_EXECUTION_MODE_DISPLAY_OUTPUT	"DisplayOutput"
#define NADP_KEY_STARTUP_NOTIFY						G_KEY_FILE_DESKTOP_KEY_STARTUP_NOTIFY
#define NADP_KEY_STARTUP_WM_CLASS					G_KEY_FILE_DESKTOP_KEY_STARTUP_WM_CLASS
#define NADP_KEY_EXECUTE_AS							"ExecuteAs"
#define NADP_KEY_FOR_EACH							"ForEach"

#define NADP_KEY_ITEMS_LIST							"ItemsList"
#define NADP_KEY_GET_ITEMS_LIST						"GetItemsList"

#define NADP_KEY_ONLY_SHOW_IN						G_KEY_FILE_DESKTOP_KEY_ONLY_SHOW_IN
#define NADP_KEY_NOT_SHOW_IN						G_KEY_FILE_DESKTOP_KEY_NOT_SHOW_IN
#define NADP_KEY_NO_DISPLAY							"NoDisplay"
#define NADP_KEY_HIDDEN								G_KEY_FILE_DESKTOP_KEY_HIDDEN
#define NADP_KEY_TRY_EXEC							G_KEY_FILE_DESKTOP_KEY_TRY_EXEC
#define NADP_KEY_SHOW_IF_REGISTERED					"ShowIfRegistered"
#define NADP_KEY_SHOW_IF_TRUE						"ShowIfTrue"
#define NADP_KEY_SHOW_IF_RUNNING					"ShowIfRunning"
#define NADP_KEY_MIMETYPES							"MimeTypes"
#define NADP_KEY_BASENAMES							"Basenames"
#define NADP_KEY_MATCHCASE							"Matchcase"
#define NADP_KEY_SELECTION_COUNT					"SelectionCount"
#define NADP_KEY_SCHEMES							"Schemes"
#define NADP_KEY_FOLDERS							"Folders"
#define NADP_KEY_CAPABILITIES						"Capabilities"
#define NADP_VALUE_CAPABILITY_OWNER					"Owner"
#define NADP_VALUE_CAPABILITY_READABLE				"Readable"
#define NADP_VALUE_CAPABILITY_WRITABLE				"Writable"
#define NADP_VALUE_CAPABILITY_EXECUTABLE			"Executable"
#define NADP_VALUE_CAPABILITY_LOCAL					"Local"

G_END_DECLS

#endif /* __NADP_KEYS_H__ */
