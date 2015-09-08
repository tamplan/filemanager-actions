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
#include <libintl.h>

#include <api/fma-core-utils.h>
#include <api/fma-timeout.h>

#include <core/fma-io-provider.h>
#include <core/fma-iprefs.h>

#include "fma-application.h"
#include "fma-status-bar.h"
#include "fma-main-tab.h"
#include "fma-main-window.h"
#include "fma-menu.h"
#include "fma-menu-file.h"
#include "fma-tree-ieditable.h"

static FMATimeout st_autosave_prefs_timeout = { 0 };
static guint     st_event_autosave         = 0;

static gchar *st_save_error       = N_( "Save error" );
static gchar *st_save_warning     = N_( "Some items may not have been saved" );
static gchar *st_level_zero_write = N_( "Unable to rewrite the level-zero items list" );
static gchar *st_delete_error     = N_( "Some items have not been deleted" );

static gboolean save_item( FMAMainWindow *window, FMAUpdater *updater, FMAObjectItem *item, GSList **messages );
static void     install_autosave( FMAMainWindow *main_window );
static void     on_autosave_prefs_changed( const gchar *group, const gchar *key, gconstpointer new_value, gpointer user_data );
static void     on_autosave_prefs_timeout( FMAMainWindow *main_window );
static gboolean autosave_callback( FMAMainWindow *main_window );
static void     autosave_destroyed( FMAMainWindow *main_window );

/*
 * fma_menu_file_init:
 * @main_window: the #FMAMainWindow main window.
 */
void
fma_menu_file_init( FMAMainWindow *main_window )
{
	install_autosave( main_window );
}

/**
 * fma_menu_file_update_sensitivities:
 * @main_window: the #FMAMainWindow main window.
 *
 * Update sensitivity of items of the File menu.
 */
void
fma_menu_file_update_sensitivities( FMAMainWindow *main_window )
{
	static const gchar *thisfn = "fma_menu_file_update_sensitivities";
	sMenuData *sdata;
	gboolean new_item_enabled;

	sdata = fma_menu_get_data( main_window );

	/* new menu / new action
	 * new item will be inserted just before the beginning of selection
	 * parent of the first selected row must be writable
	 * we must have at least one writable provider
	 */
	new_item_enabled = sdata->is_parent_writable && sdata->has_writable_providers;
	g_debug( "%s: is_parent_writable=%s, has_writable_providers=%s, new_item_enabled=%s",
			thisfn,
			sdata->is_parent_writable ? "True":"False",
			sdata->has_writable_providers ? "True":"False",
			new_item_enabled ? "True":"False" );
	fma_menu_enable_item( main_window, "new-menu", new_item_enabled );
	fma_menu_enable_item( main_window, "new-action", new_item_enabled );

	/* new profile enabled if selection is relative to only one writable action
	 * i.e. contains profile(s) of the same action, or only contains one action
	 * action must be writable
	 */
	fma_menu_enable_item( main_window, "new-profile",
			sdata->enable_new_profile && sdata->is_action_writable );

	/* save enabled if at least one item has been modified
	 * or level-zero has been resorted and is writable
	 */
	fma_menu_enable_item( main_window, "save", ( sdata->is_tree_modified ));
}

/**
 * fma_menu_file_new_menu:
 * @main_window: the #FMAMainWindow main window.
 *
 * Triggers File / New menu item.
 */
void
fma_menu_file_new_menu( FMAMainWindow *main_window )
{
	sMenuData *sdata;
	FMAObjectMenu *menu;
	NactTreeView *items_view;
	GList *items;

	sdata = fma_menu_get_data( main_window );
	menu = fma_object_menu_new_with_defaults();
	fma_object_check_status( menu );
	fma_updater_check_item_writability_status( sdata->updater, FMA_OBJECT_ITEM( menu ));
	items = g_list_prepend( NULL, menu );
	items_view = fma_main_window_get_items_view( main_window );
	fma_tree_ieditable_insert_items( FMA_TREE_IEDITABLE( items_view ), items, NULL );
	fma_object_free_items( items );
}

/**
 * fma_menu_file_new_action:
 * @main_window: the #FMAMainWindow main window.
 *
 * Triggers File / New action item.
 */
void
fma_menu_file_new_action( FMAMainWindow *main_window )
{
	sMenuData *sdata;
	FMAObjectAction *action;
	NactTreeView *items_view;
	GList *items;

	sdata = fma_menu_get_data( main_window );
	action = fma_object_action_new_with_defaults();
	fma_object_check_status( action );
	fma_updater_check_item_writability_status( sdata->updater, FMA_OBJECT_ITEM( action ));
	items = g_list_prepend( NULL, action );
	items_view = fma_main_window_get_items_view( main_window );
	fma_tree_ieditable_insert_items( FMA_TREE_IEDITABLE( items_view ), items, NULL );
	fma_object_free_items( items );
}

/**
 * fma_menu_file_new_profile:
 * @main_window: the #FMAMainWindow main window.
 *
 * Triggers File / New profile item.
 */
void
fma_menu_file_new_profile( FMAMainWindow *main_window )
{
	FMAObjectAction *action;
	FMAObjectProfile *profile;
	NactTreeView *items_view;
	GList *items;

	g_object_get(
			G_OBJECT( main_window ),
			MAIN_PROP_ITEM, &action,
			NULL );

	profile = fma_object_profile_new_with_defaults();
	fma_object_attach_profile( action, profile );

	fma_object_set_label( profile, _( "New profile" ));
	fma_object_set_new_id( profile, action );

	fma_object_check_status( profile );

	items = g_list_prepend( NULL, profile );
	items_view = fma_main_window_get_items_view( main_window );
	fma_tree_ieditable_insert_items( FMA_TREE_IEDITABLE( items_view ), items, NULL );
	fma_object_free_items( items );
}

/**
 * fma_menu_file_save_items:
 * @window: the #FMAMainWindow main window.
 *
 * Save items.
 * This is the same function that fma_menu_file_save(), just
 * with different arguments.
 *
 * Synopsis:
 * - rewrite the level-zero items list
 * - delete the items which are marked to be deleted
 * - rewrite (i.e. delete/write) updated items
 *
 * The difficulty here is that some sort of pseudo-transactionnal process
 * must be setup:
 *
 * - if the level-zero items list cannot be updated, then an error message
 *   is displayed, and we abort the whole processus
 *
 * - if some items cannot be actually deleted, then an error message is
 *   displayed, and the whole processus is aborted;
 *   plus:
 *   a/ items which have not been deleted must be restored (maybe marked
 *      as deleted ?) -> so these items are modified
 *   b/ the level-zero list must be updated with these restored items
 *      and reset modified
 *
 * - idem if some items cannot be actually rewritten...
 */
void
fma_menu_file_save_items( FMAMainWindow *window )
{
	static const gchar *thisfn = "fma_menu_file_save_items";
	sMenuData *sdata;
	NactTreeView *items_view;
	GList *items, *it;
	GList *new_pivot;
	FMAObjectItem *duplicate;
	GSList *messages;
	gchar *msg;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );

	sdata = fma_menu_get_data( window );

	/* always write the level zero list of items as the first save phase
	 * and reset the corresponding modification flag
	 */
	items_view = fma_main_window_get_items_view( window );
	items = nact_tree_view_get_items( items_view );
	fma_object_dump_tree( items );
	messages = NULL;

	if( fma_tree_ieditable_is_level_zero_modified( FMA_TREE_IEDITABLE( items_view ))){
		if( !fma_iprefs_write_level_zero( items, &messages )){
			if( g_slist_length( messages )){
				msg = fma_core_utils_slist_join_at_end( messages, "\n" );
			} else {
				msg = g_strdup( gettext( st_level_zero_write ));
			}
			base_window_display_error_dlg( NULL, gettext( st_save_error ), msg );
			g_free( msg );
			fma_core_utils_slist_free( messages );
			messages = NULL;
		}

	} else {
		g_signal_emit_by_name( items_view, TREE_SIGNAL_LEVEL_ZERO_CHANGED, FALSE );
	}

	/* remove deleted items
	 * so that new actions with same id do not risk to be deleted later
	 * not deleted items are reinserted in the tree
	 */
	if( !fma_tree_ieditable_remove_deleted( FMA_TREE_IEDITABLE( items_view ), &messages )){
		if( g_slist_length( messages )){
			msg = fma_core_utils_slist_join_at_end( messages, "\n" );
		} else {
			msg = g_strdup( gettext( st_delete_error ));
		}
		base_window_display_error_dlg( NULL, gettext( st_save_error ), msg );
		g_free( msg );
		fma_core_utils_slist_free( messages );
		messages = NULL;

	} else {
		fma_object_free_items( items );
		items = nact_tree_view_get_items( items_view );
	}

	/* recursively save the modified items
	 * check is useless here if item was not modified, but not very costly;
	 * above all, it is less costly to check the status here, than to check
	 * recursively each and every modified item
	 */
	new_pivot = NULL;

	for( it = items ; it ; it = it->next ){
		save_item( window, sdata->updater, FMA_OBJECT_ITEM( it->data ), &messages );
		duplicate = FMA_OBJECT_ITEM( fma_object_duplicate( it->data, DUPLICATE_REC ));
		fma_object_reset_origin( it->data, duplicate );
		fma_object_check_status( it->data );
		new_pivot = g_list_prepend( new_pivot, duplicate );
	}

	if( g_slist_length( messages )){
		msg = fma_core_utils_slist_join_at_end( messages, "\n" );
		base_window_display_error_dlg( NULL, gettext( st_save_warning ), msg );
		g_free( msg );
		fma_core_utils_slist_free( messages );
		messages = NULL;
	}

	fma_pivot_set_new_items( FMA_PIVOT( sdata->updater ), g_list_reverse( new_pivot ));
	fma_object_free_items( items );
	fma_main_window_block_reload( window );
	g_signal_emit_by_name( items_view, TREE_SIGNAL_MODIFIED_STATUS_CHANGED, FALSE );
}

/*
 * iterates here on each and every FMAObjectItem row stored in the tree
 */
static gboolean
save_item( FMAMainWindow *window, FMAUpdater *updater, FMAObjectItem *item, GSList **messages )
{
	static const gchar *thisfn = "fma_menu_file_save_item";
	gboolean ret;
	FMAIOProvider *provider_before;
	FMAIOProvider *provider_after;
	GList *subitems, *it;
	gchar *label;
	guint save_ret;

	g_return_val_if_fail( FMA_IS_MAIN_WINDOW( window ), FALSE );
	g_return_val_if_fail( FMA_IS_UPDATER( updater ), FALSE );
	g_return_val_if_fail( FMA_IS_OBJECT_ITEM( item ), FALSE );

	ret = TRUE;

	if( FMA_IS_OBJECT_MENU( item )){
		subitems = fma_object_get_items( item );
		for( it = subitems ; it ; it = it->next ){
			ret &= save_item( window, updater, FMA_OBJECT_ITEM( it->data ), messages );
		}
	}

	provider_before = fma_object_get_provider( item );

	if( fma_object_is_modified( item )){
		label = fma_object_get_label( item );
		g_debug( "%s: saving %p (%s) '%s'", thisfn, ( void * ) item, G_OBJECT_TYPE_NAME( item ), label );
		g_free( label );

		save_ret = fma_updater_write_item( updater, item, messages );
		ret = ( save_ret == IIO_PROVIDER_CODE_OK );

		if( ret ){
			if( FMA_IS_OBJECT_ACTION( item )){
				fma_object_reset_last_allocated( item );
			}

			provider_after = fma_object_get_provider( item );
			if( provider_after != provider_before ){
				g_signal_emit_by_name( window, MAIN_SIGNAL_ITEM_UPDATED, item, MAIN_DATA_PROVIDER );
			}

		} else {
			g_warning( "%s: unable to write item: save_ret=%d", thisfn, save_ret );
		}
	}

	return( ret );
}

/*
 * fma_menu_file_install_autosave:
 * @main_window: this #FMAMainWindow main window.
 *
 * Setup the autosave feature and initialize its monitoring.
 */
static void
install_autosave( FMAMainWindow *main_window )
{
	st_autosave_prefs_timeout.timeout = 100;
	st_autosave_prefs_timeout.handler = ( FMATimeoutFunc ) on_autosave_prefs_timeout;
	st_autosave_prefs_timeout.user_data = main_window;

	fma_settings_register_key_callback( IPREFS_MAIN_SAVE_AUTO, G_CALLBACK( on_autosave_prefs_changed ), NULL );
	fma_settings_register_key_callback( IPREFS_MAIN_SAVE_PERIOD, G_CALLBACK( on_autosave_prefs_changed ), NULL );

	on_autosave_prefs_timeout( main_window );
}

static void
on_autosave_prefs_changed( const gchar *group, const gchar *key, gconstpointer new_value, gpointer user_data )
{
	na_timeout_event( &st_autosave_prefs_timeout );
}

static void
on_autosave_prefs_timeout( FMAMainWindow *main_window )
{
	static const gchar *thisfn = "fma_menu_file_autosave_prefs_timeout";
	gboolean autosave_on;
	guint autosave_period;

	autosave_on = fma_settings_get_boolean( IPREFS_MAIN_SAVE_AUTO, NULL, NULL );
	autosave_period = fma_settings_get_uint( IPREFS_MAIN_SAVE_PERIOD, NULL, NULL );

	if( st_event_autosave ){
		if( !g_source_remove( st_event_autosave )){
			g_warning( "%s: unable to remove autosave event source", thisfn );
		}
		st_event_autosave = 0;
	}

	if( autosave_on ){
		st_event_autosave = g_timeout_add_seconds_full(
				G_PRIORITY_DEFAULT,
				autosave_period * 60,
				( GSourceFunc ) autosave_callback,
				main_window,
				( GDestroyNotify ) autosave_destroyed );
	}
}

static gboolean
autosave_callback( FMAMainWindow *main_window )
{
	FMAStatusBar *bar;
	const gchar *context = "autosave-context";
	g_debug( "fma_menu_file_autosave_callback" );

	bar = fma_main_window_get_statusbar( main_window );
	fma_status_bar_display_status( bar, context, _( "Automatically saving pending modifications..." ));
	fma_menu_file_save_items( main_window );
	fma_status_bar_hide_status( bar, context );

	return( TRUE );
}

static void
autosave_destroyed( FMAMainWindow *main_window )
{
	g_debug( "fma_menu_file_autosave_destroyed" );
}
