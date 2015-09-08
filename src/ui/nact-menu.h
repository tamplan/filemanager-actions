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

#ifndef __UI_NACT_MENU_H__
#define __UI_NACT_MENU_H__

/*
 * SECTION: nact-menu
 * @title: NactMenu
 * @short_description: The menu helpers functions
 * @include: nact-menu.h
 */

#include "core/fma-updater.h"

#include "fma-application.h"
#include "nact-main-window-def.h"

G_BEGIN_DECLS

/* This is private data, set against the main window
 * for exclusive use of menu functions
 */
typedef struct {

	/* set at initialization time
	 */
	gulong		update_sensitivities_handler_id;
	FMAUpdater  *updater;
	GMenuModel *maintainer;
	GMenuModel *popup;
	gboolean    is_level_zero_writable;
	gboolean    has_writable_providers;

	/* set when the selection changes
	 */
	guint       count_selected;
	GList      *selected_items;
	gboolean    is_parent_writable;		/* new menu/new action/paste menu or action */
	gboolean    enable_new_profile;		/* new profile/paste a profile */
	gboolean    is_action_writable;
	gboolean    are_parents_writable;	/* duplicate */
	gboolean    are_items_writable;		/* cut/delete */

	/* set when the count of modified or deleted FMAObjectItem changes
	 * or when the level zero is changed
	 */
	gboolean    is_tree_modified;

	/* set on focus in/out
	 */
	gboolean    treeview_has_focus;

	/* opening a contextual popup menu
	 */
	gulong      popup_handler;

	/* set when total count of items changes
	 */
	gint        count_menus;
	gint        count_actions;
	gint        count_profiles;
	gboolean    have_exportables;
	gint        selected_menus;
	gint        selected_actions;
	gint        selected_profiles;
	gint        clipboard_menus;
	gint        clipboard_actions;
	gint        clipboard_profiles;
}
	sMenuData;

/* Toolbars identifiers
 * they are listed here in the order they should be displayed
 */
enum {
	TOOLBAR_FILE_ID = 1,
	TOOLBAR_EDIT_ID,
	TOOLBAR_TOOLS_ID,
	TOOLBAR_HELP_ID,
};

void       nact_menu_app        ( FMAApplication *application );

void       nact_menu_win        ( NactMainWindow *main_window );

void       nact_menu_enable_item( NactMainWindow *main_window,
										const gchar *action_name,
										gboolean enable );

sMenuData *nact_menu_get_data   ( NactMainWindow *main_window );

G_END_DECLS

#endif /* __UI_NACT_MENU_H__ */
