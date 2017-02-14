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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "api/fma-object-api.h"

#include "fma-clipboard.h"
#include "fma-main-window.h"
#include "fma-menu.h"
#include "fma-menu-maintainer.h"
#include "fma-tree-ieditable.h"

/**
 * fma_menu_maintainer_update_sensitivities:
 * @main_window: the #FMAMainWindow main window.
 *
 * Update sensitivities on the Maintainer menu.
 */
void
fma_menu_maintainer_update_sensitivities( FMAMainWindow *main_window )
{
}

/**
 * fma_menu_maintainer_dump_selection:
 * @main_window: the #FMAMainWindow main window.
 *
 * Triggers the "Maintainer/Dump selection" item.
 */
void
fma_menu_maintainer_dump_selection( FMAMainWindow *main_window )
{
	sMenuData *sdata;

	sdata = fma_menu_get_data( main_window );

	fma_object_dump_tree( sdata->selected_items );
}

/**
 * fma_menu_maintainer_brief_tree_store_dump:
 * @main_window: the #FMAMainWindow main window.
 *
 * Triggers the "Maintainer/Brief treestore dump" item.
 */
void
fma_menu_maintainer_brief_tree_store_dump( FMAMainWindow *main_window )
{
	FMATreeView *items_view;
	GList *items;

	items_view = fma_main_window_get_items_view( main_window );
	items = fma_tree_view_get_items( items_view );
	fma_object_dump_tree( items );
	fma_object_free_items( items );
}

/**
 * fma_menu_maintainer_list_modified_items:
 * @main_window: the #FMAMainWindow main window.
 *
 * Triggers the "Maintainer/List modified items" item.
 */
void
fma_menu_maintainer_list_modified_items( FMAMainWindow *main_window )
{
	FMATreeView *items_view;

	items_view = fma_main_window_get_items_view( main_window );
	fma_tree_ieditable_dump_modified( FMA_TREE_IEDITABLE( items_view ));
}

/**
 * fma_menu_maintainer_dump_clipboard:
 * @main_window: the #FMAMainWindow main window.
 *
 * Triggers the "Maintainer/Dump clipboard" item.
 */
void
fma_menu_maintainer_dump_clipboard( FMAMainWindow *main_window )
{
	fma_clipboard_dump( fma_main_window_get_clipboard( main_window ));
}

/**
 * fma_menu_maintainer_test_function:
 * @main_window: the #FMAMainWindow main window.
 *
 * Test a miscellaneous function.
 */
void
fma_menu_maintainer_test_function( FMAMainWindow *main_window )
{
}

