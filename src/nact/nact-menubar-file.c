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

#include <glib/gi18n.h>
#include <libintl.h>

#include <api/na-core-utils.h>
#include <api/na-timeout.h>

#include <core/na-io-provider.h>
#include <core/na-iprefs.h>

#include "nact-application.h"
#include "nact-main-statusbar.h"
#include "nact-main-tab.h"
#include "nact-menubar-priv.h"
#include "nact-tree-ieditable.h"

static NATimeout st_autosave_prefs_timeout = { 0 };
static guint     st_event_autosave         = 0;

static gchar *st_save_error       = N_( "Save error" );
static gchar *st_save_warning     = N_( "Some items may not have been saved" );
static gchar *st_level_zero_write = N_( "Unable to rewrite the level-zero items list" );
static gchar *st_delete_error     = N_( "Some items have not been deleted" );

static gboolean save_item( BaseWindow *window, NAUpdater *updater, NAObjectItem *item, GSList **messages );
static void     install_autosave( NactMenubar *bar );
static void     on_autosave_prefs_changed( const gchar *group, const gchar *key, gconstpointer new_value, gpointer user_data );
static void     on_autosave_prefs_timeout( NactMenubar *bar );
static gboolean autosave_callback( NactMenubar *bar );
static void     autosave_destroyed( NactMenubar *bar );

/*
 * nact_menubar_file_initialize:
 * @bar: this #NactMenubar object.
 */
void
nact_menubar_file_initialize( NactMenubar *bar )
{
	install_autosave( bar );
}

/**
 * nact_menubar_file_on_update_sensitivities:
 * @bar: this #NactMenubar object.
 *
 * Update sensitivity of items of the File menu.
 */
void
nact_menubar_file_on_update_sensitivities( const NactMenubar *bar )
{
	gboolean new_item_enabled;

	/* new menu / new action
	 * new item will be inserted just before the beginning of selection
	 * parent of the first selected row must be writable
	 * we must have at least one writable provider
	 */
	new_item_enabled = bar->private->is_parent_writable && bar->private->has_writable_providers;
	nact_menubar_enable_item( bar, "NewMenuItem", new_item_enabled );
	nact_menubar_enable_item( bar, "NewActionItem", new_item_enabled );

	/* new profile enabled if selection is relative to only one writable action
	 * i.e. contains profile(s) of the same action, or only contains one action
	 * action must be writable
	 */
	nact_menubar_enable_item( bar, "NewProfileItem",
			bar->private->enable_new_profile && bar->private->is_action_writable );

	/* save enabled if at least one item has been modified
	 * or level-zero has been resorted and is writable
	 */
	nact_menubar_enable_item( bar, "SaveItem", ( bar->private->is_tree_modified ));

	/* quit always enabled */
}

/**
 * nact_menubar_file_on_new_menu:
 * @gtk_action: the #GtkAction action.
 * @window: the #BaseWindow main window.
 *
 * Triggers File / New menu item.
 */
void
nact_menubar_file_on_new_menu( GtkAction *gtk_action, BaseWindow *window )
{
	NAObjectMenu *menu;
	NactApplication *application;
	NAUpdater *updater;
	NactTreeView *items_view;
	GList *items;

	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	menu = na_object_menu_new_with_defaults();
	na_object_check_status( menu );
	application = NACT_APPLICATION( base_window_get_application( window ));
	updater = nact_application_get_updater( application );
	na_updater_check_item_writability_status( updater, NA_OBJECT_ITEM( menu ));
	items = g_list_prepend( NULL, menu );
	items_view = nact_main_window_get_items_view( NACT_MAIN_WINDOW( window ));
	nact_tree_ieditable_insert_items( NACT_TREE_IEDITABLE( items_view ), items, NULL );
	na_object_free_items( items );
}

/**
 * nact_menubar_file_on_new_action:
 * @gtk_action: the #GtkAction action.
 * @window: the #BaseWindow main window.
 *
 * Triggers File / New action item.
 */
void
nact_menubar_file_on_new_action( GtkAction *gtk_action, BaseWindow *window )
{
	NAObjectAction *action;
	NactApplication *application;
	NAUpdater *updater;
	NactTreeView *items_view;
	GList *items;

	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	action = na_object_action_new_with_defaults();
	na_object_check_status( action );
	application = NACT_APPLICATION( base_window_get_application( window ));
	updater = nact_application_get_updater( application );
	na_updater_check_item_writability_status( updater, NA_OBJECT_ITEM( action ));
	items = g_list_prepend( NULL, action );
	items_view = nact_main_window_get_items_view( NACT_MAIN_WINDOW( window ));
	nact_tree_ieditable_insert_items( NACT_TREE_IEDITABLE( items_view ), items, NULL );
	na_object_free_items( items );
}

/**
 * nact_menubar_file_on_new_profile:
 * @gtk_action: the #GtkAction action.
 * @window: the #BaseWindow main window.
 *
 * Triggers File / New profile item.
 */
void
nact_menubar_file_on_new_profile( GtkAction *gtk_action, BaseWindow *window )
{
	NAObjectAction *action;
	NAObjectProfile *profile;
	NactTreeView *items_view;
	gchar *name;
	GList *items;

	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	g_object_get(
			G_OBJECT( window ),
			MAIN_PROP_ITEM, &action,
			NULL );

	profile = na_object_profile_new_with_defaults();
	na_object_set_label( profile, _( "New profile" ));

	name = na_object_action_get_new_profile_name( action );
	na_object_set_id( profile, name );
	g_free( name );

	/*na_object_attach_profile( action, profile );*/

	na_object_check_status( profile );

	items = g_list_prepend( NULL, profile );
	items_view = nact_main_window_get_items_view( NACT_MAIN_WINDOW( window ));
	nact_tree_ieditable_insert_items( NACT_TREE_IEDITABLE( items_view ), items, NULL );
	na_object_free_items( items );
}

/**
 * nact_menubar_file_on_save:
 * @gtk_action: the #GtkAction action.
 * @window: the #BaseWindow main window.
 *
 * Triggers File /Save item.
 *
 * Saving is not only saving modified items, but also saving hierarchy
 * (and order if alpha order is not set).
 *
 * This is the same function that nact_menubar_file_save_items(), just with
 * different arguments.
 */
void
nact_menubar_file_on_save( GtkAction *gtk_action, BaseWindow *window )
{
	static const gchar *thisfn = "nact_menubar_file_on_save";

	g_debug( "%s: gtk_action=%p, window=%p", thisfn, ( void * ) gtk_action, ( void * ) window );
	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	nact_menubar_file_save_items( window );
}

/**
 * nact_menubar_file_save_items:
 * @gtk_action: the #GtkAction action.
 * @window: the #BaseWindow main window.
 *
 * Save items.
 * This is the same function that nact_menubar_file_on_save(), just
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
nact_menubar_file_save_items( BaseWindow *window )
{
	static const gchar *thisfn = "nact_menubar_file_save_items";
	NactTreeView *items_view;
	GList *items, *it;
	GList *new_pivot;
	NAObjectItem *duplicate;
	GSList *messages;
	gchar *msg;

	BAR_WINDOW_VOID( window );

	g_debug( "%s: window=%p", thisfn, ( void * ) window );

	/* always write the level zero list of items as the first save phase
	 * and reset the corresponding modification flag
	 */
	items_view = nact_main_window_get_items_view( NACT_MAIN_WINDOW( window ));
	items = nact_tree_view_get_items( items_view );
	na_object_dump_tree( items );
	messages = NULL;

	if( nact_tree_ieditable_is_level_zero_modified( NACT_TREE_IEDITABLE( items_view ))){
		if( !na_iprefs_write_level_zero( items, &messages )){
			if( g_slist_length( messages )){
				msg = na_core_utils_slist_join_at_end( messages, "\n" );
			} else {
				msg = g_strdup( gettext( st_level_zero_write ));
			}
			base_window_display_error_dlg( window, gettext( st_save_error ), msg );
			g_free( msg );
			na_core_utils_slist_free( messages );
			messages = NULL;
		}

	} else {
		g_signal_emit_by_name( window, TREE_SIGNAL_LEVEL_ZERO_CHANGED, FALSE );
	}

	/* remove deleted items
	 * so that new actions with same id do not risk to be deleted later
	 * not deleted items are reinserted in the tree
	 */
	if( !nact_tree_ieditable_remove_deleted( NACT_TREE_IEDITABLE( items_view ), &messages )){
		if( g_slist_length( messages )){
			msg = na_core_utils_slist_join_at_end( messages, "\n" );
		} else {
			msg = g_strdup( gettext( st_delete_error ));
		}
		base_window_display_error_dlg( window, gettext( st_save_error ), msg );
		g_free( msg );
		na_core_utils_slist_free( messages );
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
		save_item( window, bar->private->updater, NA_OBJECT_ITEM( it->data ), &messages );
		duplicate = NA_OBJECT_ITEM( na_object_duplicate( it->data ));
		na_object_reset_origin( it->data, duplicate );
		na_object_check_status( it->data );
		new_pivot = g_list_prepend( new_pivot, duplicate );
	}

	if( g_slist_length( messages )){
		msg = na_core_utils_slist_join_at_end( messages, "\n" );
		base_window_display_error_dlg( window, gettext( st_save_warning ), msg );
		g_free( msg );
		na_core_utils_slist_free( messages );
		messages = NULL;
	}

	na_pivot_set_new_items( NA_PIVOT( bar->private->updater ), g_list_reverse( new_pivot ));
	na_object_free_items( items );
	nact_main_window_block_reload( NACT_MAIN_WINDOW( window ));
	g_signal_emit_by_name( window, TREE_SIGNAL_MODIFIED_STATUS_CHANGED, FALSE );
}

/*
 * iterates here on each and every NAObjectItem row stored in the tree
 */
static gboolean
save_item( BaseWindow *window, NAUpdater *updater, NAObjectItem *item, GSList **messages )
{
	static const gchar *thisfn = "nact_menubar_file_save_item";
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
		}
	}

	return( ret );
}

/**
 * nact_menubar_file_on_quit:
 * @gtk_action: the #GtkAction action.
 * @window: the #BaseWindow main window.
 *
 * Triggers the File / Quit item.
 */
void
nact_menubar_file_on_quit( GtkAction *gtk_action, BaseWindow *window )
{
	static const gchar *thisfn = "nact_menubar_file_on_quit";

	g_debug( "%s: item=%p, window=%p", thisfn, ( void * ) gtk_action, ( void * ) window );
	g_return_if_fail( GTK_IS_ACTION( gtk_action ) || gtk_action == NULL );

	nact_main_window_quit( NACT_MAIN_WINDOW( window ));
}

/*
 * nact_menubar_file_install_autosave:
 * @bar: this #NactMenubar instance.
 *
 * Setup the autosave feature and initialize its monitoring.
 */
static void
install_autosave( NactMenubar *bar )
{
	st_autosave_prefs_timeout.timeout = 100;
	st_autosave_prefs_timeout.handler = ( NATimeoutFunc ) on_autosave_prefs_timeout;
	st_autosave_prefs_timeout.user_data = bar;

	na_settings_register_key_callback( NA_IPREFS_MAIN_SAVE_AUTO, G_CALLBACK( on_autosave_prefs_changed ), NULL );
	na_settings_register_key_callback( NA_IPREFS_MAIN_SAVE_PERIOD, G_CALLBACK( on_autosave_prefs_changed ), NULL );

	on_autosave_prefs_timeout( bar );
}

static void
on_autosave_prefs_changed( const gchar *group, const gchar *key, gconstpointer new_value, gpointer user_data )
{
	na_timeout_event( &st_autosave_prefs_timeout );
}

static void
on_autosave_prefs_timeout( NactMenubar *bar )
{
	static const gchar *thisfn = "nact_menubar_file_on_autosave_prefs_timeout";
	gboolean autosave_on;
	guint autosave_period;

	g_return_if_fail( NACT_IS_MENUBAR( bar ));

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
				bar,
				( GDestroyNotify ) autosave_destroyed );
	}
}

static gboolean
autosave_callback( NactMenubar *bar )
{
	const gchar *context = "autosave-context";
	g_debug( "nact_menubar_file_autosave_callback" );

	nact_main_statusbar_display_status( NACT_MAIN_WINDOW( bar->private->window ), context, _( "Automatically saving pending modifications..." ));
	nact_menubar_file_save_items( bar->private->window );
	nact_main_statusbar_hide_status( NACT_MAIN_WINDOW( bar->private->window ), context );

	return( TRUE );
}

static void
autosave_destroyed( NactMenubar *bar )
{
	g_debug( "nact_menubar_file_autosave_destroyed" );
}
