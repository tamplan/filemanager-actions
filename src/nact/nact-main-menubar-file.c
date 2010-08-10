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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>

#include <core/na-io-provider.h>

#include "nact-application.h"
#include "nact-iactions-list.h"
#include "nact-main-statusbar.h"
#include "nact-main-tab.h"
#include "nact-main-menubar-file.h"

static guint st_event_autosave = 0;

static void     save_item( NactMainWindow *window, NAUpdater *updater, NAObjectItem *item );
static gboolean autosave_callback( NactMainWindow *window );
static void     autosave_destroyed( NactMainWindow *window );

/**
 * nact_main_menubar_file_on_update_sensitivities:
 * @window: the #NactMainWindow main window.
 * user_data: the data passed to the function via the signal.
 * @mis: the #MenubarIndicatorsStruct struct.
 *
 * Update sensitivity of items of the File menu.
 */
void
nact_main_menubar_file_on_update_sensitivities( NactMainWindow *window, gpointer user_data, MenubarIndicatorsStruct *mis )
{
	static const gchar *thisfn = "nact_main_menubar_file_on_update_sensitivities";
	gboolean new_item_enabled;
	gboolean new_profile_enabled;
	NAObject *first_parent;
	NAObject *selected_action;
	NAObject *parent_item;
	gboolean is_first_parent_writable;
	gboolean has_modified_items;
	GList *is;

	first_parent = mis->selected_items && g_list_length( mis->selected_items )
			? ( NAObject * ) na_object_get_parent( mis->selected_items->data )
			: NULL;
	is_first_parent_writable = first_parent
			? nact_window_is_item_writable( NACT_WINDOW( window ), NA_OBJECT_ITEM( first_parent ), NULL )
			: mis->is_level_zero_writable;

	has_modified_items = nact_main_window_has_modified_items( window );
	g_debug( "%s: has_modified_items=%s", thisfn, has_modified_items ? "True":"False" );

	/* new menu / new action
	 * new item will be inserted just before beginning of selection
	 * parent of the first selected row must be writable
	 * we must have at least one writable provider
	 */
	new_item_enabled = is_first_parent_writable && mis->has_writable_providers;
	nact_main_menubar_enable_item( window, "NewMenuItem", new_item_enabled );
	nact_main_menubar_enable_item( window, "NewActionItem", new_item_enabled );

	/* new profile enabled if selection is relative to only one writable action
	 * i.e. contains profile(s) of the same action, or only contains one action
	 * action must be writable
	 */
	new_profile_enabled = TRUE;
	selected_action = NULL;
	for( is = mis->selected_items ; is ; is = is->next ){

		if( NA_IS_OBJECT_MENU( is->data )){
			new_profile_enabled = FALSE;
			break;

		} else if( NA_IS_OBJECT_ACTION( is->data )){
			if( !selected_action ){
				selected_action = NA_OBJECT( is->data );
			} else if( selected_action != is->data ){
				new_profile_enabled = FALSE;
				break;
			}

		} else if( NA_IS_OBJECT_PROFILE( is->data )){
			parent_item = NA_OBJECT( na_object_get_parent( is->data ));
			if( !selected_action ){
				selected_action = parent_item;
			} else if( selected_action != parent_item ){
				new_profile_enabled = FALSE;
				break;
			}
		}
	}
	nact_main_menubar_enable_item( window, "NewProfileItem",
			new_profile_enabled &&
			selected_action != NULL &&
			nact_window_is_item_writable( NACT_WINDOW( window ), NA_OBJECT_ITEM( selected_action ), NULL ));

	/* save enabled if at least one item has been modified
	 * or level-zero has been resorted and is writable
	 */
	nact_main_menubar_enable_item( window, "SaveItem",
			has_modified_items || ( mis->level_zero_order_changed && mis->is_level_zero_writable ));

	/* quit always enabled */
}

/**
 * nact_main_menubar_file_on_new_menu:
 * @gtk_action: the #GtkAction action.
 * @window: the #NactMainWindow main window.
 *
 * Triggers File / New menu item.
 */
void
nact_main_menubar_file_on_new_menu( GtkAction *gtk_action, NactMainWindow *window )
{
	NAObjectMenu *menu;
	GList *items;

	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	menu = na_object_menu_new_with_defaults();
	na_object_check_status( menu );
	items = g_list_prepend( NULL, menu );
	nact_iactions_list_bis_insert_items( NACT_IACTIONS_LIST( window ), items, NULL );
	na_object_unref_items( items );
}

/**
 * nact_main_menubar_file_on_new_action:
 * @gtk_action: the #GtkAction action.
 * @window: the #NactMainWindow main window.
 *
 * Triggers File / New action item.
 */
void
nact_main_menubar_file_on_new_action( GtkAction *gtk_action, NactMainWindow *window )
{
	NAObjectAction *action;
	GList *items;

	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	action = na_object_action_new_with_defaults();
	na_object_check_status( action );
	items = g_list_prepend( NULL, action );
	nact_iactions_list_bis_insert_items( NACT_IACTIONS_LIST( window ), items, NULL );
	na_object_unref_items( items );
}

/**
 * nact_main_menubar_file_on_new_profile:
 * @gtk_action: the #GtkAction action.
 * @window: the #NactMainWindow main window.
 *
 * Triggers File / New profile item.
 */
void
nact_main_menubar_file_on_new_profile( GtkAction *gtk_action, NactMainWindow *window )
{
	NAObjectAction *action;
	NAObjectProfile *profile;
	gchar *name;
	GList *items;

	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	g_object_get(
			G_OBJECT( window ),
			TAB_UPDATABLE_PROP_SELECTED_ITEM, &action,
			NULL );

	profile = na_object_profile_new_with_defaults();
	na_object_set_label( profile, _( "New profile" ));

	name = na_object_action_get_new_profile_name( action );
	na_object_set_id( profile, name );
	g_free( name );

	/*na_object_attach_profile( action, profile );*/

	na_object_check_status( profile );

	items = g_list_prepend( NULL, profile );
	nact_iactions_list_bis_insert_items( NACT_IACTIONS_LIST( window ), items, NULL );
	na_object_unref_items( items );
}

/**
 * nact_main_menubar_file_on_save:
 * @gtk_action: the #GtkAction action.
 * @window: the #NactMainWindow main window.
 *
 * Triggers File /Save item.
 *
 * Saving is not only saving modified items, but also saving hierarchy
 * (and order if alpha order is not set).
 *
 * This is the same function that #nact_main_menubar_file_save_items(), just with
 * different arguments.
 */
void
nact_main_menubar_file_on_save( GtkAction *gtk_action, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_menubar_file_on_save";

	g_debug( "%s: gtk_action=%p, window=%p", thisfn, ( void * ) gtk_action, ( void * ) window );
	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	nact_main_menubar_file_save_items( window );
}

/**
 * nact_main_menubar_file_save_items:
 * @gtk_action: the #GtkAction action.
 * @window: the #NactMainWindow main window.
 *
 * Save items.
 * This is the same function that #nact_main_menubar_file_on_save(), just with
 * different arguments.
 */
void
nact_main_menubar_file_save_items( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_menubar_file_save_items";
	GList *items, *it;
	NactApplication *application;
	NAUpdater *updater;
	MenubarIndicatorsStruct *mis;
	gchar *label;
	GList *new_pivot;
	NAObjectItem *duplicate;

	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	g_debug( "%s: window=%p", thisfn, ( void * ) window );

	/* get ride of notification messages of IOProviders
	 */
	na_ipivot_consumer_allow_notify( NA_IPIVOT_CONSUMER( window ), FALSE, 0 );

	/* remove deleted items
	 * so that new actions with same id do not risk to be deleted later
	 */
	nact_main_window_remove_deleted( window );

	/* always write the level zero list of items
	 * and reset the corresponding modification flag
	 */
	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	updater = nact_application_get_updater( application );
	items = nact_iactions_list_bis_get_items( NACT_IACTIONS_LIST( window ));

	na_pivot_write_level_zero( NA_PIVOT( updater ), items );
	mis = ( MenubarIndicatorsStruct * ) g_object_get_data( G_OBJECT( window ), MENUBAR_PROP_INDICATORS );
	mis->level_zero_order_changed = FALSE;

	/* recursively save the modified items
	 * check is useless here if item was not modified, but not very costly
	 * above all, it is less costly to check the status here, than to check
	 * recursively each and every modified item
	 */
	new_pivot = NULL;

	for( it = items ; it ; it = it->next ){
		label = na_object_get_label( it->data );
		g_debug( "%s saving item %s %p (%s), modified=%s",
				thisfn,
				G_OBJECT_TYPE_NAME( it->data ),
				( void * ) it->data, label,
				na_object_is_modified( it->data ) ? "True":"False" );
		g_free( label );

		save_item( window, updater, NA_OBJECT_ITEM( it->data ));

		duplicate = NA_OBJECT_ITEM( na_object_duplicate( it->data ));
		na_object_reset_origin( it->data, duplicate );
		na_object_check_status( it->data );
		new_pivot = g_list_prepend( new_pivot, duplicate );
	}

	na_pivot_set_new_items( NA_PIVOT( updater ), g_list_reverse( new_pivot ));
	g_list_free( items );

	/* when new_pivot is empty, then there has been no chance of updating
	 * sensibilities on check status - so force it there
	 */
	if( !new_pivot ){
		g_signal_emit_by_name( window, MAIN_WINDOW_SIGNAL_UPDATE_ACTION_SENSITIVITIES, NULL );
	}

	/* restore NAPivot notifications
	 */
	na_ipivot_consumer_allow_notify( NA_IPIVOT_CONSUMER( window ), TRUE, 2000 );
}

/*
 * iterates here on each and every NAObjectItem row stored in the tree
 *
 * do not deal with profiles as they are directly managed by the action
 * they are attached to
 *
 * level zero order has already been saved from tree store order, so that
 * we actually do not care of the exact order of level zero NAPivot items
 *
 * saving means non-recursively save modified NAObjectItem, simultaneously
 * reproducing the new item in NAPivot
 *  +- A
 *  |  +- B
 *  |  |  +- C
 *  |  |  |  +- D
 *  |  |  |  +- E
 *  |  |  +- F
 *  |  +- G
 *  +- H
 *  |  +- ...
 *  save order: A-B-C-D-E-F-G-H (first parent, then children)
 */
static void
save_item( NactMainWindow *window, NAUpdater *updater, NAObjectItem *item )
{
	NAIOProvider *provider_before;
	NAIOProvider *provider_after;
	GList *subitems, *it;

	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));
	g_return_if_fail( NA_IS_UPDATER( updater ));
	g_return_if_fail( NA_IS_OBJECT_ITEM( item ));

	provider_before = na_object_get_provider( item );

	if( na_object_is_modified( item ) &&
		nact_window_save_item( NACT_WINDOW( window ), item )){

			if( NA_IS_OBJECT_ACTION( item )){
				na_object_reset_last_allocated( item );
			}

			nact_iactions_list_bis_remove_modified( NACT_IACTIONS_LIST( window ), item );

			provider_after = na_object_get_provider( item );
			if( provider_after != provider_before ){
				g_signal_emit_by_name( window, TAB_UPDATABLE_SIGNAL_PROVIDER_CHANGED, item );
			}
	}

	if( NA_IS_OBJECT_MENU( item )){
		subitems = na_object_get_items( item );
		for( it = subitems ; it ; it = it->next ){
			save_item( window, updater, NA_OBJECT_ITEM( it->data ));
		}
	}
}

/**
 * nact_main_menubar_file_on_quit:
 * @gtk_action: the #GtkAction action.
 * @window: the #NactMainWindow main window.
 *
 * Triggers the File / Quit item.
 */
void
nact_main_menubar_file_on_quit( GtkAction *gtk_action, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_menubar_file_on_quit";
	gboolean has_modified;

	g_debug( "%s: item=%p, window=%p", thisfn, ( void * ) gtk_action, ( void * ) window );
	g_return_if_fail( GTK_IS_ACTION( gtk_action ) || gtk_action == NULL );
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	has_modified = nact_main_window_has_modified_items( window );
	if( !has_modified || nact_window_warn_modified( NACT_WINDOW( window ))){
		g_object_unref( window );
	}
}

/**
 * nact_main_menubar_file_set_autosave:
 * @window: the #NactMainWindow main window.
 * @enabled: whether the autosave feature is enabled.
 * @period: autosave periodicity in minutes.
 *
 * Setup or disabled the autosave feature.
 */
void
nact_main_menubar_file_set_autosave( NactMainWindow *window, gboolean enabled, guint period )
{
	static const gchar *thisfn = "nact_main_menubar_file_set_autosave";

	if( st_event_autosave ){
		if( !g_source_remove( st_event_autosave )){
			g_warning( "%s: unable to remove autosave event source", thisfn );
		}
		st_event_autosave = 0;
	}

	if( enabled ){
		st_event_autosave = g_timeout_add_seconds_full(
				G_PRIORITY_DEFAULT,
				period * 60,
				( GSourceFunc ) autosave_callback,
				window,
				( GDestroyNotify ) autosave_destroyed );
	}
}

static gboolean
autosave_callback( NactMainWindow *window )
{
	const gchar *context = "autosave-context";
	g_debug( "nact_main_menubar_file_autosave_callback" );

	nact_main_statusbar_display_status( window, context, _( "Automatically saving pending modifications..." ));
	nact_main_menubar_file_save_items( window );
	nact_main_statusbar_hide_status( window, context );

	return( TRUE );
}

static void
autosave_destroyed( NactMainWindow *window )
{
	g_debug( "nact_main_menubar_file_autosave_destroyed" );
}
