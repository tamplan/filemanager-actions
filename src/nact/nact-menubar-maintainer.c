/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011, 2012 Pierre Wieser and others (see AUTHORS)
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

#include <api/na-object-api.h>

#include "nact-clipboard.h"
#include "nact-menubar-priv.h"
#include "nact-main-window.h"
#include "nact-tree-ieditable.h"

/**
 * nact_menubar_maintainer_on_update_sensitivities:
 * @bar: this #NactMenubar object.
 *
 * Update sensitivities on the Maintainer menu.
 */
void
nact_menubar_maintainer_on_update_sensitivities( const NactMenubar *bar )
{
}

/**
 * nact_menubar_maintainer_on_dump_selection:
 * @action: the #GtkAction of the item.
 * @window: the #BaseWindow main application window.
 *
 * Triggers the "Maintainer/Dump selection" item.
 */
void
nact_menubar_maintainer_on_dump_selection( GtkAction *action, BaseWindow *window )
{
	GList *items;

	BAR_WINDOW_VOID( window );

	items = na_object_copyref_items( bar->private->selected_items );
	na_object_dump_tree( items );
	na_object_free_items( items );
}

/**
 * nact_menubar_maintainer_on_brief_tree_store_dump:
 * @action: the #GtkAction of the item.
 * @window: the #BaseWindow main application window.
 *
 * Triggers the "Maintainer/Brief treestore dump" item.
 */
void
nact_menubar_maintainer_on_brief_tree_store_dump( GtkAction *action, BaseWindow *window )
{
	NactTreeView *items_view;
	GList *items;

	items_view = nact_main_window_get_items_view( NACT_MAIN_WINDOW( window ));
	items = nact_tree_view_get_items( items_view );
	na_object_dump_tree( items );
	na_object_free_items( items );
}

/**
 * nact_menubar_maintainer_on_list_modified_items:
 * @action: the #GtkAction of the item.
 * @window: the #BaseWindow main application window.
 *
 * Triggers the "Maintainer/List modified items" item.
 */
void
nact_menubar_maintainer_on_list_modified_items( GtkAction *action, BaseWindow *window )
{
	NactTreeView *items_view;

	items_view = nact_main_window_get_items_view( NACT_MAIN_WINDOW( window ));
	nact_tree_ieditable_dump_modified( NACT_TREE_IEDITABLE( items_view ));
}

/**
 * nact_menubar_maintainer_on_dump_clipboard:
 * @action: the #GtkAction of the item.
 * @window: the #BaseWindow main application window.
 *
 * Triggers the "Maintainer/Dump clipboard" item.
 */
void
nact_menubar_maintainer_on_dump_clipboard( GtkAction *action, BaseWindow *window )
{
	nact_clipboard_dump( nact_main_window_get_clipboard( NACT_MAIN_WINDOW( window )));
}

/*
 * Test a miscellaneous function
 */
void
nact_menubar_maintainer_on_test_function( GtkAction *action, BaseWindow *window )
{
	gboolean is_willing = base_window_is_willing_to_quit( BASE_WINDOW( window ));
	g_debug( "nact_menubar_maintainer_on_test_function: willing_to=%s", is_willing ? "True":"False" );
}
