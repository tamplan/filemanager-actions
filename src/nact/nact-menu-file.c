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
#include <api/na-timeout.h>

#include <core/na-io-provider.h>
#include <core/na-iprefs.h>

#include "nact-application.h"
#include "nact-statusbar.h"
#include "nact-main-tab.h"
#include "nact-main-window.h"
#include "nact-menu.h"
#include "nact-menu-file.h"
#include "nact-tree-ieditable.h"

static NATimeout st_autosave_prefs_timeout = { 0 };
static guint     st_event_autosave         = 0;

static gchar *st_save_error       = N_( "Save error" );
static gchar *st_save_warning     = N_( "Some items may not have been saved" );
static gchar *st_level_zero_write = N_( "Unable to rewrite the level-zero items list" );
static gchar *st_delete_error     = N_( "Some items have not been deleted" );

static gboolean save_item( NactMainWindow *window, NAUpdater *updater, NAObjectItem *item, GSList **messages );
static void     install_autosave( NactMainWindow *main_window );
static void     on_autosave_prefs_changed( const gchar *group, const gchar *key, gconstpointer new_value, gpointer user_data );
static void     on_autosave_prefs_timeout( NactMainWindow *main_window );
static gboolean autosave_callback( NactMainWindow *main_window );
static void     autosave_destroyed( NactMainWindow *main_window );

/*
 * nact_menu_file_init:
 * @main_window: the #NactMainWindow main window.
 */
void
nact_menu_file_init( NactMainWindow *main_window )
{
	install_autosave( main_window );
}

/**
 * nact_menu_file_update_sensitivities:
 * @main_window: the #NactMainWindow main window.
 *
 * Update sensitivity of items of the File menu.
 */
void
nact_menu_file_update_sensitivities( NactMainWindow *main_window )
{
	static const gchar *thisfn = "nact_menu_file_update_sensitivities";
	sMenuData *sdata;
	gboolean new_item_enabled;

	sdata = nact_menu_get_data( main_window );

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
	nact_menu_enable_item( main_window, "new-menu", new_item_enabled );
	nact_menu_enable_item( main_window, "new-action", new_item_enabled );

	/* new profile enabled if selection is relative to only one writable action
	 * i.e. contains profile(s) of the same action, or only contains one action
	 * action must be writable
	 */
	nact_menu_enable_item( main_window, "new-profile",
			sdata->enable_new_profile && sdata->is_action_writable );

	/* save enabled if at least one item has been modified
	 * or level-zero has been resorted and is writable
	 */
	nact_menu_enable_item( main_window, "save", ( sdata->is_tree_modified ));
}

/**
 * nact_menu_file_new_menu:
 * @main_window: the #NactMainWindow main window.
 *
 * Triggers File / New menu item.
 */
void
nact_menu_file_new_menu( NactMainWindow *main_window )
{
	sMenuData *sdata;
	NAObjectMenu *menu;
	NactTreeView *items_view;
	GList *items;

	sdata = nact_menu_get_data( main_window );
	menu = na_object_menu_new_with_defaults();
	na_object_check_status( menu );
	na_updater_check_item_writability_status( sdata->updater, NA_OBJECT_ITEM( menu ));
	items = g_list_prepend( NULL, menu );
	items_view = nact_main_window_get_items_view( main_window );
	nact_tree_ieditable_insert_items( NACT_TREE_IEDITABLE( items_view ), items, NULL );
	na_object_free_items( items );
}

/**
 * nact_menu_file_new_action:
 * @main_window: the #NactMainWindow main window.
 *
 * Triggers File / New action item.
 */
void
nact_menu_file_new_action( NactMainWindow *main_window )
{
	sMenuData *sdata;
	NAObjectAction *action;
	NactTreeView *items_view;
	GList *items;

	sdata = nact_menu_get_data( main_window );
	action = na_object_action_new_with_defaults();
	na_object_check_status( action );
	na_updater_check_item_writability_status( sdata->updater, NA_OBJECT_ITEM( action ));
	items = g_list_prepend( NULL, action );
	items_view = nact_main_window_get_items_view( main_window );
	nact_tree_ieditable_insert_items( NACT_TREE_IEDITABLE( items_view ), items, NULL );
	na_object_free_items( items );
}

/**
 * nact_menu_file_new_profile:
 * @main_window: the #NactMainWindow main window.
 *
 * Triggers File / New profile item.
 */
void
nact_menu_file_new_profile( NactMainWindow *main_window )
{
	NAObjectAction *action;
	NAObjectProfile *profile;
	NactTreeView *items_view;
	GList *items;

	g_object_get(
			G_OBJECT( main_window ),
			MAIN_PROP_ITEM, &action,
			NULL );

	profile = na_object_profile_new_with_defaults();
	na_object_attach_profile( action, profile );

	na_object_set_label( profile, _( "New profile" ));
	na_object_set_new_id( profile, action );

	na_object_check_status( profile );

	items = g_list_prepend( NULL, profile );
	items_view = nact_main_window_get_items_view( main_window );
	nact_tree_ieditable_insert_items( NACT_TREE_IEDITABLE( items_view ), items, NULL );
	na_object_free_items( items );
}

/**
 * nact_menu_file_save_items:
 * @window: the #NactMainWindow main window.
 *
 * Save items.
 * This is the same function that nact_menu_file_save(), just
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
nact_menu_file_save_items( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_menu_file_save_items";
	sMenuData *sdata;
	NactTreeView *items_view;
	GList *items, *it;
	GList *new_pivot;
	NAObjectItem *duplicate;
	GSList *messages;
	gchar *msg;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );

	sdata = nact_menu_get_data( window );

	/* always write the level zero list of items as the first save phase
	 * and reset the corresponding modification flag
	 */
	items_view = nact_main_window_get_items_view( window );
	items = nact_tree_view_get_items( items_view );
	na_object_dump_tree( items );
	messages = NULL;

	if( nact_tree_ieditable_is_level_zero_modified( NACT_TREE_IEDITABLE( items_view ))){
		if( !na_iprefs_write_level_zero( items, &messages )){
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
	if( !nact_tree_ieditable_remove_deleted( NACT_TREE_IEDITABLE( items_view ), &messages )){
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
		na_object_free_items( items );
		items = nact_tree_view_get_items( items_view );
	}

	/* recursively save the modified items
	 * check is useless here if item was not modified, but not very costly;
	 * above all, it is less costly to check the status here, than to check
	 * recursively each and every modified item
	 */
	new_pivot = NULL;

	for( it = items ; it ; it = it->next ){
		save_item( window, sdata->updater, NA_OBJECT_ITEM( it->data ), &messages );
		duplicate = NA_OBJECT_ITEM( na_object_duplicate( it->data, DUPLICATE_REC ));
		na_object_reset_origin( it->data, duplicate );
		na_object_check_status( it->data );
		new_pivot = g_list_prepend( new_pivot, duplicate );
	}

	if( g_slist_length( messages )){
		msg = fma_core_utils_slist_join_at_end( messages, "\n" );
		base_window_display_error_dlg( NULL, gettext( st_save_warning ), msg );
		g_free( msg );
		fma_core_utils_slist_free( messages );
		messages = NULL;
	}

	na_pivot_set_new_items( NA_PIVOT( sdata->updater ), g_list_reverse( new_pivot ));
	na_object_free_items( items );
	nact_main_window_block_reload( window );
	g_signal_emit_by_name( items_view, TREE_SIGNAL_MODIFIED_STATUS_CHANGED, FALSE );
}

/*
 * iterates here on each and every NAObjectItem row stored in the tree
 */
static gboolean
save_item( NactMainWindow *window, NAUpdater *updater, NAObjectItem *item, GSList **messages )
{
	static const gchar *thisfn = "nact_menu_file_save_item";
	gboolean ret;
	NAIOProvider *provider_before;
	NAIOProvider *provider_after;
	GList *subitems, *it;
	gchar *label;
	guint save_ret;

	g_return_val_if_fail( NACT_IS_MAIN_WINDOW( window ), FALSE );
	g_return_val_if_fail( NA_IS_UPDATER( updater ), FALSE );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), FALSE );

	ret = TRUE;

	if( NA_IS_OBJECT_MENU( item )){
		subitems = na_object_get_items( item );
		for( it = subitems ; it ; it = it->next ){
			ret &= save_item( window, updater, NA_OBJECT_ITEM( it->data ), messages );
		}
	}

	provider_before = na_object_get_provider( item );

	if( na_object_is_modified( item )){
		label = na_object_get_label( item );
		g_debug( "%s: saving %p (%s) '%s'", thisfn, ( void * ) item, G_OBJECT_TYPE_NAME( item ), label );
		g_free( label );

		save_ret = na_updater_write_item( updater, item, messages );
		ret = ( save_ret == NA_IIO_PROVIDER_CODE_OK );

		if( ret ){
			if( NA_IS_OBJECT_ACTION( item )){
				na_object_reset_last_allocated( item );
			}

			provider_after = na_object_get_provider( item );
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
 * nact_menu_file_install_autosave:
 * @main_window: this #NactMainWindow main window.
 *
 * Setup the autosave feature and initialize its monitoring.
 */
static void
install_autosave( NactMainWindow *main_window )
{
	st_autosave_prefs_timeout.timeout = 100;
	st_autosave_prefs_timeout.handler = ( NATimeoutFunc ) on_autosave_prefs_timeout;
	st_autosave_prefs_timeout.user_data = main_window;

	na_settings_register_key_callback( NA_IPREFS_MAIN_SAVE_AUTO, G_CALLBACK( on_autosave_prefs_changed ), NULL );
	na_settings_register_key_callback( NA_IPREFS_MAIN_SAVE_PERIOD, G_CALLBACK( on_autosave_prefs_changed ), NULL );

	on_autosave_prefs_timeout( main_window );
}

static void
on_autosave_prefs_changed( const gchar *group, const gchar *key, gconstpointer new_value, gpointer user_data )
{
	na_timeout_event( &st_autosave_prefs_timeout );
}

static void
on_autosave_prefs_timeout( NactMainWindow *main_window )
{
	static const gchar *thisfn = "nact_menu_file_autosave_prefs_timeout";
	gboolean autosave_on;
	guint autosave_period;

	autosave_on = na_settings_get_boolean( NA_IPREFS_MAIN_SAVE_AUTO, NULL, NULL );
	autosave_period = na_settings_get_uint( NA_IPREFS_MAIN_SAVE_PERIOD, NULL, NULL );

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
autosave_callback( NactMainWindow *main_window )
{
	NactStatusbar *bar;
	const gchar *context = "autosave-context";
	g_debug( "nact_menu_file_autosave_callback" );

	bar = nact_main_window_get_statusbar( main_window );
	nact_statusbar_display_status( bar, context, _( "Automatically saving pending modifications..." ));
	nact_menu_file_save_items( main_window );
	nact_statusbar_hide_status( bar, context );

	return( TRUE );
}

static void
autosave_destroyed( NactMainWindow *main_window )
{
	g_debug( "nact_menu_file_autosave_destroyed" );
}