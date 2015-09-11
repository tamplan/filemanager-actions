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

#include <glib/gi18n.h>

#include "api/fma-core-utils.h"

#include "core/fma-io-provider.h"

#include "fma-application.h"
#include "fma-clipboard.h"
#include "fma-main-tab.h"
#include "fma-main-window.h"
#include "fma-menu.h"
#include "fma-menu-edit.h"
#include "fma-tree-ieditable.h"
#include "fma-tree-view.h"

static GList  *prepare_for_paste( FMAMainWindow *window, sMenuData *sdata );
static GList  *get_deletables( FMAUpdater *updater, GList *tree, GSList **not_deletable );
static GSList *get_deletables_rec( FMAUpdater *updater, GList *tree );
static gchar  *add_ndeletable_msg( const FMAObjectItem *item, gint reason );
static void    update_clipboard_counters( FMAMainWindow *window, sMenuData *sdata );

/**
 * fma_menu_edit_update_sensitivities:
 * @main_window: the #FMAMainWindow main window.
 *
 * Update sensitivity of items of the Edit menu.
 *
 * Each action (cut, copy, delete, etc.) takes itself care of whether it
 * can safely apply to all the selection or not. The action can at least
 * assume that at least one item will be a valid candidate (this is the
 * Menubar rule).
 */
void
fma_menu_edit_update_sensitivities( FMAMainWindow *main_window )
{
	sMenuData *sdata;
	gboolean cut_enabled;
	gboolean copy_enabled;
	gboolean paste_enabled;
	gboolean paste_into_enabled;
	gboolean duplicate_enabled;
	gboolean delete_enabled;
	FMAObject *parent_item;
	FMAObject *selected_action;
	FMAObject *selected_item;
	gboolean is_clipboard_empty;

	sdata = fma_menu_get_data( main_window );
	is_clipboard_empty = ( sdata->clipboard_menus + sdata->clipboard_actions + sdata->clipboard_profiles == 0 );

	/* cut requires a non-empty selection
	 * and that the selection is writable (can be modified, i.e. is not read-only)
	 * and that all parents are writable (as implies a delete operation)
	 */
	duplicate_enabled = sdata->treeview_has_focus || sdata->popup_handler;
	duplicate_enabled &= sdata->count_selected > 0;
	duplicate_enabled &= sdata->are_parents_writable;
	cut_enabled = duplicate_enabled;
	cut_enabled &= sdata->are_items_writable;
	fma_menu_enable_item( main_window, "cut", cut_enabled );

	/* copy only requires a non-empty selection */
	copy_enabled = sdata->treeview_has_focus || sdata->popup_handler;
	copy_enabled &= sdata->count_selected > 0;
	fma_menu_enable_item( main_window, "copy", copy_enabled );

	/* paste enabled if
	 * - clipboard is not empty
	 * - current selection is not multiple
	 * - if clipboard contains only profiles,
	 *   then current selection must be a profile or an action
	 *   and the action must be writable
	 * - if clipboard contains actions or menus,
	 *   then current selection (if any) must be a menu or an action
	 *   and its parent must be writable
	 */
	paste_enabled = sdata->treeview_has_focus || sdata->popup_handler;
	paste_enabled &= !is_clipboard_empty;
	paste_enabled &= sdata->count_selected <= 1;
	if( sdata->clipboard_profiles ){
		paste_enabled &= sdata->count_selected == 1;
		paste_enabled &= sdata->is_action_writable;
	} else {
		paste_enabled &= sdata->has_writable_providers;
		if( sdata->count_selected ){
			paste_enabled &= sdata->is_parent_writable;
		} else {
			paste_enabled &= sdata->is_level_zero_writable;
		}
	}
	fma_menu_enable_item( main_window, "paste", paste_enabled );

	/* paste into enabled if
	 * - clipboard is not empty
	 * - current selection is not multiple
	 * - if clipboard contains only profiles,
	 *   then current selection must be an action
	 *   and the action must be writable
	 * - if clipboard contains actions or menus,
	 *   then current selection (if any) must be a menu
	 *   and its parent must be writable
	 */
	paste_into_enabled = sdata->treeview_has_focus || sdata->popup_handler;
	paste_into_enabled &= !is_clipboard_empty;
	paste_into_enabled &= sdata->count_selected <= 1;
	if( sdata->clipboard_profiles ){
		paste_into_enabled &= sdata->count_selected == 1;
		if( paste_into_enabled ){
			selected_action = FMA_OBJECT( sdata->selected_items->data );
			paste_into_enabled &= FMA_IS_OBJECT_ACTION( selected_action );
			if( paste_into_enabled ){
				paste_into_enabled &= fma_object_is_finally_writable( selected_action, NULL );
			}
		}
	} else {
		paste_into_enabled &= sdata->has_writable_providers;
		if( sdata->count_selected ){
			selected_item = FMA_OBJECT( sdata->selected_items->data );
			paste_into_enabled &= FMA_IS_OBJECT_MENU( selected_item );
			if( paste_into_enabled ){
				parent_item = ( FMAObject * ) fma_object_get_parent( selected_item );
				paste_into_enabled &= parent_item
						? fma_object_is_finally_writable( parent_item, NULL )
						: sdata->is_level_zero_writable;
			}
		} else {
			paste_into_enabled &= sdata->is_level_zero_writable;
		}
	}
	fma_menu_enable_item( main_window, "paste-into", paste_into_enabled );

	/* duplicate items will be duplicated besides each one
	 * selection must be non-empty
	 * each parent must be writable
	 */
	fma_menu_enable_item( main_window, "duplicate", duplicate_enabled );

	/* delete is same that cut
	 * but items themselves must be writable (because physically deleted)
	 * this will be checked on delete activated
	 */
	delete_enabled = cut_enabled;
	fma_menu_enable_item( main_window, "delete", delete_enabled );

	/* reload items always enabled */
}

/**
 * fma_menu_edit_cut:
 * @main_window: the #FMAMainWindow main window.
 *
 * Cut objects are installed both in the clipboard and in the deleted list.
 * Parent pointer is reset to %NULL.
 * Old parent status is re-checked by the tree model delete operation.
 * When pasting later these cut objects:
 * - the first time, we paste the very same object, removing it from the
 *   deleted list, attaching it to a new parent
 *   the object itself is not modified, but the parent is.
 * - the following times, we paste a copy of this object with a new identifier
 *
 * cuts the visible selection
 * - (tree) get new refs on selected items
 * - (main) add selected items to main list of deleted,
 *          moving newref from list_from_tree to main_list_of_deleted
 * - (menu) install in clipboard a copy of selected objects
 * - (tree) remove selected items, unreffing objects
 */
void
fma_menu_edit_cut( FMAMainWindow *main_window )
{
	static const gchar *thisfn = "fma_menu_edit_cut";
	sMenuData *sdata;
	GList *items;
	FMAClipboard *clipboard;
	GList *to_delete;
	GSList *ndeletables;
	FMATreeView *view;

	g_debug( "%s: main_window=%p", thisfn, ( void * ) main_window );
	g_return_if_fail( main_window && FMA_IS_MAIN_WINDOW( main_window ));

	sdata = fma_menu_get_data( main_window );
	items = fma_object_copyref_items( sdata->selected_items );
	ndeletables = NULL;
	to_delete = get_deletables( sdata->updater, items, &ndeletables );

	if( ndeletables ){
		gchar *second = fma_core_utils_slist_join_at_end( ndeletables, "\n" );
		base_window_display_error_dlg(
				BASE_WINDOW( main_window ),
				_( "Not all items have been cut as following ones are not modifiable:" ),
				second );
		g_free( second );
		fma_core_utils_slist_free( ndeletables );
	}

	if( to_delete ){
		clipboard = fma_main_window_get_clipboard( FMA_MAIN_WINDOW( main_window ));
		fma_clipboard_primary_set( clipboard, to_delete, CLIPBOARD_MODE_CUT );
		update_clipboard_counters( main_window, sdata );
		view = fma_main_window_get_items_view( main_window );
		fma_tree_ieditable_delete( FMA_TREE_IEDITABLE( view ), to_delete, TREE_OPE_DELETE );
	}

	fma_object_free_items( items );
}

/**
 * fma_menu_edit_copy:
 * @main_window: the #FMAMainWindow main window.
 *
 * copies the visible selection
 * - (tree) get new refs on selected items
 * - (menu) install in clipboard a copy of selected objects
 *          renumbering actions/menus id to ensure unicity at paste time
 * - (menu) release refs on selected items
 * - (menu) refresh actions sensitivy (as selection doesn't change)
 */
void
fma_menu_edit_copy( FMAMainWindow *main_window )
{
	static const gchar *thisfn = "fma_menu_edit_copy";
	sMenuData *sdata;
	FMAClipboard *clipboard;

	g_debug( "%s: main_window=%p", thisfn, ( void * ) main_window );
	g_return_if_fail( main_window && FMA_IS_MAIN_WINDOW( main_window ));

	sdata = fma_menu_get_data( main_window );

	clipboard = fma_main_window_get_clipboard( main_window );
	fma_clipboard_primary_set( clipboard, sdata->selected_items, CLIPBOARD_MODE_COPY );
	update_clipboard_counters( main_window, sdata );

	g_signal_emit_by_name( main_window, MAIN_SIGNAL_UPDATE_SENSITIVITIES );
}

/**
 * fma_menu_edit_paste:
 * @main_window: the #FMAMainWindow main window.
 *
 * pastes the current content of the clipboard at the current position
 * (same path, same level)
 * - (menu) get from clipboard a copy of installed items
 *          the clipboard will return a new copy
 *          and renumber its own data for allowing a new paste
 * - (tree) insert new items, the tree store will ref them
 *          attaching each item to its parent
 *          recursively checking edition status of the topmost parent
 *          selecting the first item at end
 * - (menu) unreffing the copy got from clipboard
 */
void
fma_menu_edit_paste( FMAMainWindow *main_window )
{
	static const gchar *thisfn = "fma_menu_edit_paste";
	sMenuData *sdata;
	GList *items;
	FMATreeView *view;

	g_debug( "%s: main_window=%p", thisfn, ( void * ) main_window );
	g_return_if_fail( main_window && FMA_IS_MAIN_WINDOW( main_window ));

	sdata = fma_menu_get_data( main_window );
	items = prepare_for_paste( main_window, sdata );

	if( items ){
		view = fma_main_window_get_items_view( main_window );
		fma_tree_ieditable_insert_items( FMA_TREE_IEDITABLE( view ), items, NULL );
		fma_object_free_items( items );
	}
}

/**
 * fma_menu_edit_paste_into:
 * @main_window: the #FMAMainWindow main window.
 *
 * pastes the current content of the clipboard as the first child of
 * currently selected item
 * - (menu) get from clipboard a copy of installed items
 *          the clipboard will return a new copy
 *          and renumber its own data for allowing a new paste
 * - (tree) insert new items, the tree store will ref them
 *          attaching each item to its parent
 *          recursively checking edition status of the topmost parent
 *          selecting the first item at end
 * - (menu) unreffing the copy got from clipboard
 */
void
fma_menu_edit_paste_into( FMAMainWindow *main_window )
{
	static const gchar *thisfn = "fma_menu_edit_paste_into";
	sMenuData *sdata;
	GList *items;
	FMATreeView *view;

	g_debug( "%s: main_window=%p", thisfn, ( void * ) main_window );
	g_return_if_fail( main_window && FMA_IS_MAIN_WINDOW( main_window ));

	sdata = fma_menu_get_data( main_window );
	items = prepare_for_paste( main_window, sdata );

	if( items ){
		view = fma_main_window_get_items_view( main_window );
		fma_tree_ieditable_insert_into( FMA_TREE_IEDITABLE( view ), items );
		fma_object_free_items( items );
	}
}

static GList *
prepare_for_paste( FMAMainWindow *window, sMenuData *sdata )
{
	static const gchar *thisfn = "fma_menu_edit_prepare_for_paste";
	GList *items, *it;
	FMAClipboard *clipboard;
	FMAObjectAction *action;
	gboolean relabel;
	gboolean renumber;

	clipboard = fma_main_window_get_clipboard( window );
	items = fma_clipboard_primary_get( clipboard, &renumber );
	action = NULL;

	/* if pasted items are profiles, then setup the target action
	 */
	for( it = items ; it ; it = it->next ){

		if( FMA_IS_OBJECT_PROFILE( it->data )){
			if( !action ){
				g_object_get( G_OBJECT( window ), MAIN_PROP_ITEM, &action, NULL );
				g_return_val_if_fail( FMA_IS_OBJECT_ACTION( action ), NULL );
			}
		}

		relabel = fma_updater_should_pasted_be_relabeled( sdata->updater, FMA_OBJECT( it->data ));
		fma_object_prepare_for_paste( it->data, relabel, renumber, action );
		fma_object_check_status( it->data );
	}

	g_debug( "%s: action=%p (%s)",
			thisfn, ( void * ) action, action ? G_OBJECT_TYPE_NAME( action ): "(null)" );

	return( items );
}

/**
 * fma_menu_edit_duplicate:
 * @main_window: the #FMAMainWindow main window.
 *
 * duplicate is just as paste, with the difference that content comes
 * from the current selection, instead of coming from the clipboard
 *
 * this is nonetheless a bit more complicated because when we duplicate
 * some items (e.g. a multiple selection), we expect to see the new
 * items just besides the original ones...
 */
void
fma_menu_edit_duplicate( FMAMainWindow *main_window )
{
	static const gchar *thisfn = "fma_menu_edit_duplicate";
	sMenuData *sdata;
	FMAObjectAction *action;
	GList *items, *it;
	GList *dup;
	FMAObject *obj;
	gboolean relabel;
	FMATreeView *view;

	g_debug( "%s: main_window=%p", thisfn, ( void * ) main_window );
	g_return_if_fail( main_window && FMA_IS_MAIN_WINDOW( main_window ));

	sdata = fma_menu_get_data( main_window );
	items = fma_object_copyref_items( sdata->selected_items );

	for( it = items ; it ; it = it->next ){
		obj = FMA_OBJECT( fma_object_duplicate( it->data, FMA_DUPLICATE_REC ));
		action = NULL;

		/* duplicating a profile
		 * as we insert in sibling mode, the parent doesn't change
		 */
		if( FMA_IS_OBJECT_PROFILE( obj )){
			action = FMA_OBJECT_ACTION( fma_object_get_parent( it->data ));
		}

		relabel = fma_updater_should_pasted_be_relabeled( sdata->updater, obj );
		fma_object_prepare_for_paste( obj, relabel, TRUE, action );
		fma_object_set_origin( obj, NULL );
		fma_object_check_status( obj );
		dup = g_list_prepend( NULL, obj );
		view = fma_main_window_get_items_view( FMA_MAIN_WINDOW( main_window ));
		fma_tree_ieditable_insert_items( FMA_TREE_IEDITABLE( view ), dup, it->data );
		fma_object_free_items( dup );
	}

	fma_object_free_items( items );
}

/**
 * fma_menu_edit_delete:
 * @main_window: the #FMAMainWindow main window.
 *
 * deletes the visible selection
 * - (tree) get new refs on selected items
 * - (tree) remove selected items, unreffing objects
 * - (main) add selected items to main list of deleted,
 *          moving newref from list_from_tree to main_list_of_deleted
 * - (tree) select next row (if any, or previous if any, or none)
 *
 * note that we get from selection a list of trees, but we don't have
 * yet ensured that each element of this tree is actually deletable
 * each branch of this list must be recursively deletable in order
 * this branch itself be deleted
 */
void
fma_menu_edit_delete( FMAMainWindow *main_window )
{
	static const gchar *thisfn = "fma_menu_edit_delete";
	sMenuData *sdata;
	GList *items;
	GList *to_delete;
	GSList *ndeletables;
	FMATreeView *view;

	g_debug( "%s: main_window=%p", thisfn, ( void * ) main_window );
	g_return_if_fail( main_window && FMA_IS_MAIN_WINDOW( main_window ));

	sdata = fma_menu_get_data( main_window );
	items = fma_object_copyref_items( sdata->selected_items );
	ndeletables = NULL;
	to_delete = get_deletables( sdata->updater, items, &ndeletables );

	if( ndeletables ){
		gchar *second = fma_core_utils_slist_join_at_end( ndeletables, "\n" );
		base_window_display_error_dlg(
				BASE_WINDOW( main_window ),
				_( "Not all items have been deleted as following ones are not modifiable:" ),
				second );
		g_free( second );
		fma_core_utils_slist_free( ndeletables );
	}

	if( to_delete ){
		view = fma_main_window_get_items_view( main_window );
		fma_tree_ieditable_delete( FMA_TREE_IEDITABLE( view ), to_delete, TREE_OPE_DELETE );
	}

	fma_object_free_items( items );
}

static GList *
get_deletables( FMAUpdater *updater, GList *selected, GSList **ndeletables )
{
	GList *to_delete;
	GList *it;
	GList *subitems;
	GSList *sub_deletables;
	guint reason;
	FMAObjectItem *item;

	to_delete = NULL;
	for( it = selected ; it ; it = it->next ){

		if( FMA_IS_OBJECT_PROFILE( it->data )){
			item = fma_object_get_parent( it->data );
		} else {
			item = FMA_OBJECT_ITEM( it->data );
		}

		if( !fma_object_is_finally_writable( item, &reason )){
			*ndeletables = g_slist_prepend(
					*ndeletables, add_ndeletable_msg( FMA_OBJECT_ITEM( it->data ), reason ));
			continue;
		}

		if( FMA_IS_OBJECT_MENU( it->data )){
			subitems = fma_object_get_items( it->data );
			sub_deletables = get_deletables_rec( updater, subitems );

			if( sub_deletables ){
				*ndeletables = g_slist_concat( *ndeletables, sub_deletables );
				continue;
			}
		}

		to_delete = g_list_prepend( to_delete, fma_object_ref( it->data ));
	}

	return( to_delete );
}

static GSList *
get_deletables_rec( FMAUpdater *updater, GList *tree )
{
	GSList *msgs;
	GList *it;
	GList *subitems;
	guint reason;

	msgs = NULL;
	for( it = tree ; it ; it = it->next ){

		if( !fma_object_is_finally_writable( it->data, &reason )){
			msgs = g_slist_prepend(
					msgs, add_ndeletable_msg( FMA_OBJECT_ITEM( it->data ), reason ));
			continue;
		}

		if( FMA_IS_OBJECT_MENU( it->data )){
			subitems = fma_object_get_items( it->data );
			msgs = g_slist_concat( msgs, get_deletables_rec( updater, subitems ));
		}
	}

	return( msgs );
}

static gchar *
add_ndeletable_msg( const FMAObjectItem *item, gint reason )
{
	gchar *msg;
	gchar *label;
	gchar *reasstr;

	label = fma_object_get_label( item );
	reasstr = fma_io_provider_get_readonly_tooltip( reason );

	msg = g_strdup_printf( "%s: %s", label, reasstr );

	g_free( reasstr );
	g_free( label );

	return( msg );
}

/*
 * as we are coming from cut or copy to clipboard, report selection
 * counters to clipboard ones
 */
static void
update_clipboard_counters( FMAMainWindow *main_window, sMenuData *sdata )
{
	sdata->clipboard_menus = sdata->selected_menus;
	sdata->clipboard_actions = sdata->selected_actions;
	sdata->clipboard_profiles = sdata->selected_profiles;

	g_debug( "fma_menu_update_clipboard_counters: menus=%d, actions=%d, profiles=%d",
			sdata->clipboard_menus, sdata->clipboard_actions, sdata->clipboard_profiles );

	g_signal_emit_by_name( main_window, MAIN_SIGNAL_UPDATE_SENSITIVITIES );
}
