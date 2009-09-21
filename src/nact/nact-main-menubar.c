/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009 Pierre Wieser and others (see AUTHORS)
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

#include <common/na-about.h>
#include <common/na-object-api.h>
#include <common/na-object-action.h>
#include <common/na-object-menu.h>
#include <common/na-ipivot-consumer.h>

#include "nact-application.h"
#include "nact-assistant-export.h"
#include "nact-assistant-import.h"
#include "nact-preferences-editor.h"
#include "nact-iactions-list.h"
#include "nact-clipboard.h"
#include "nact-main-statusbar.h"
#include "nact-main-tab.h"
#include "nact-main-menubar.h"

#define MENUBAR_PROP_STATUS_CONTEXT		"nact-menubar-status-context"
#define MENUBAR_PROP_UI_MANAGER			"nact-menubar-ui-manager"
#define MENUBAR_PROP_ACTIONS_GROUP		"nact-menubar-actions-group"

static void     on_tab_updatable_selection_changed( NactMainWindow *window, gint count_selected );

static void     on_new_menu_activated( GtkAction *action, NactMainWindow *window );
static void     on_new_action_activated( GtkAction *action, NactMainWindow *window );
static void     on_new_profile_activated( GtkAction *action, NactMainWindow *window );
static void     on_save_activated( GtkAction *action, NactMainWindow *window );
static void     save_object_item( NactMainWindow *window, NAPivot *pivot, NAObjectItem *object );
static void     on_quit_activated( GtkAction *action, NactMainWindow *window );

static void     on_cut_activated( GtkAction *action, NactMainWindow *window );
static void     on_copy_activated( GtkAction *action, NactMainWindow *window );
static void     on_paste_activated( GtkAction *action, NactMainWindow *window );
static void     on_duplicate_activated( GtkAction *action, NactMainWindow *window );
static void     on_delete_activated( GtkAction *action, NactMainWindow *window );
static void     on_reload_activated( GtkAction *action, NactMainWindow *window );
static void     on_preferences_activated( GtkAction *action, NactMainWindow *window );

static void     on_import_activated( GtkAction *action, NactMainWindow *window );
static void     on_export_activated( GtkAction *action, NactMainWindow *window );

static void     on_help_activated( GtkAction *action, NactMainWindow *window );
static void     on_about_activated( GtkAction *action, NactMainWindow *window );

static void     enable_item( NactMainWindow *window, const gchar *name, gboolean enabled );
static gboolean on_delete_event( GtkWidget *toplevel, GdkEvent *event, NactMainWindow *window );
static void     on_destroy_callback( gpointer data );
static void     on_menu_item_selected( GtkMenuItem *proxy, NactMainWindow *window );
static void     on_menu_item_deselected( GtkMenuItem *proxy, NactMainWindow *window );
static void     on_proxy_connect( GtkActionGroup *action_group, GtkAction *action, GtkWidget *proxy, NactMainWindow *window );
static void     on_proxy_disconnect( GtkActionGroup *action_group, GtkAction *action, GtkWidget *proxy, NactMainWindow *window );
static void     refresh_actions_sensitivity_with_count( NactMainWindow *window, gint count_selected );

static const GtkActionEntry entries[] = {

		{ "FileMenu", NULL, N_( "_File" ) },
		{ "EditMenu", NULL, N_( "_Edit" ) },
		{ "ToolsMenu", NULL, N_( "_Tools" ) },
		{ "HelpMenu", NULL, N_( "_Help" ) },

		{ "NewMenuItem", NULL, N_( "New _menu" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the 'New menu' item */
				N_( "Insert a new menu at the current position" ),
				G_CALLBACK( on_new_menu_activated ) },
		{ "NewActionItem", GTK_STOCK_NEW, N_( "_New action" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the 'New action' item */
				N_( "Define a new action" ),
				G_CALLBACK( on_new_action_activated ) },
		{ "NewProfileItem", NULL, N_( "New _profile" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the 'New profile' item */
				N_( "Define a new profile attached to the current action" ),
				G_CALLBACK( on_new_profile_activated ) },
		{ "SaveItem", GTK_STOCK_SAVE, NULL, NULL,
				/* i18n: tooltip displayed in the status bar when selecting 'Save' item */
				N_( "Record all the modified actions. Invalid actions will be silently ignored" ),
				G_CALLBACK( on_save_activated ) },
		{ "QuitItem", GTK_STOCK_QUIT, NULL, NULL,
				/* i18n: tooltip displayed in the status bar when selecting 'Quit' item */
				N_( "Quit the application" ),
				G_CALLBACK( on_quit_activated ) },
		{ "CutItem" , GTK_STOCK_CUT, NULL, NULL,
				/* i18n: tooltip displayed in the status bar when selecting the Cut item */
				N_( "Cut the selected item(s) to the clipboard" ),
				G_CALLBACK( on_cut_activated ) },
		{ "CopyItem" , GTK_STOCK_COPY, NULL, NULL,
				/* i18n: tooltip displayed in the status bar when selecting the Copy item */
				N_( "Copy the selected item(s) to the clipboard" ),
				G_CALLBACK( on_copy_activated ) },
		{ "PasteItem" , GTK_STOCK_PASTE, NULL, NULL,
				/* i18n: tooltip displayed in the status bar when selecting the Paste item */
				N_( "Insert the content of the clipboard at the current position" ),
				G_CALLBACK( on_paste_activated ) },
		{ "DuplicateItem" , NULL, N_( "D_uplicate" ), "",
				/* i18n: tooltip displayed in the status bar when selecting the Duplicate item */
				N_( "Duplicate the selected item(s)" ),
				G_CALLBACK( on_duplicate_activated ) },
		{ "DeleteItem", GTK_STOCK_DELETE, NULL, "Delete",
				/* i18n: tooltip displayed in the status bar when selecting the Delete item */
				N_( "Delete the selected item(s)" ),
				G_CALLBACK( on_delete_activated ) },
		{ "ReloadActionsItem", NULL, N_( "_Reload the list of actions" ), "<Ctrl>R",
				/* i18n: tooltip displayed in the status bar when selecting the 'Reload' item */
				N_( "Cancel your current modifications and reload the list of actions" ),
				G_CALLBACK( on_reload_activated ) },
		{ "PreferencesItem", GTK_STOCK_PREFERENCES, NULL, NULL,
				/* i18n: tooltip displayed in the status bar when selecting the 'Preferences' item */
				N_( "Edit your preferences" ),
				G_CALLBACK( on_preferences_activated ) },
		{ "ImportItem" , GTK_STOCK_CONVERT, N_( "_Import assistant.." ), "",
				/* i18n: tooltip displayed in the status bar when selecting the Import item */
				N_( "Import one or more actions from external (XML) files into your configuration" ),
				G_CALLBACK( on_import_activated ) },
		{ "ExportItem", NULL, N_( "E_xport assistant.." ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the Export item */
				N_( "Export one or more actions from your configuration to external XML files" ),
				G_CALLBACK( on_export_activated ) },
		{ "HelpItem" , GTK_STOCK_HELP, NULL, NULL,
				/* i18n: tooltip displayed in the status bar when selecting the Help item */
				N_( "Display help about this program" ),
				G_CALLBACK( on_help_activated ) },
		{ "AboutItem", GTK_STOCK_ABOUT, NULL, NULL,
				/* i18n: tooltip displayed in the status bar when selecting the About item */
				N_( "Display informations about this program" ),
				G_CALLBACK( on_about_activated ) },
};

/**
 * nact_main_menubar_runtime_init:
 * @window: the #NactMainWindow to which the menubar is attached.
 *
 * Creates the menubar.
 */
void
nact_main_menubar_runtime_init( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_menubar_init";
	GtkActionGroup *action_group;
	GtkUIManager *ui_manager;
	GError *error = NULL;
	guint merge_id;
	GtkWindow *wnd;
	GtkAccelGroup *accel_group;
	GtkWidget *menubar, *vbox;
	GtkWindow *toplevel;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );

	/* create the menubar:
	 * - create action group, and insert list of actions in it
	 * - create ui_manager, and insert action group in it
	 * - merge inserted actions group with ui xml definition
	 * - install accelerators in the main window
	 * - pack menu bar in the main window
	 */
	ui_manager = gtk_ui_manager_new();
	g_object_set_data_full( G_OBJECT( window ), MENUBAR_PROP_UI_MANAGER, ui_manager, ( GDestroyNotify ) on_destroy_callback  );
	g_debug( "%s: ui_manager=%p", thisfn, ( void * ) ui_manager );

	base_window_signal_connect(
			BASE_WINDOW( window ),
			G_OBJECT( ui_manager ),
			"connect-proxy",
			G_CALLBACK( on_proxy_connect ));

	base_window_signal_connect(
			BASE_WINDOW( window ),
			G_OBJECT( ui_manager ),
			"disconnect-proxy",
			G_CALLBACK( on_proxy_disconnect ));

	action_group = gtk_action_group_new( "MenubarActions" );
	g_object_set_data_full( G_OBJECT( window ), MENUBAR_PROP_ACTIONS_GROUP, action_group, ( GDestroyNotify ) on_destroy_callback );
	g_debug( "%s: action_group=%p", thisfn, ( void * ) action_group );

	gtk_action_group_set_translation_domain( action_group, GETTEXT_PACKAGE );
	gtk_action_group_add_actions( action_group, entries, G_N_ELEMENTS( entries ), window );
	gtk_ui_manager_insert_action_group( ui_manager, action_group, 0 );

	merge_id = gtk_ui_manager_add_ui_from_file( ui_manager, PKGDATADIR "/nautilus-actions-config-tool.actions", &error );
	if( merge_id == 0 || error ){
		g_warning( "%s: error=%s", thisfn, error->message );
		g_error_free( error );
	}

	wnd = base_window_get_toplevel_window( BASE_WINDOW( window ));
	accel_group = gtk_ui_manager_get_accel_group( ui_manager );
	gtk_window_add_accel_group( wnd, accel_group );

	menubar = gtk_ui_manager_get_widget( ui_manager, "/ui/MainMenubar" );
	vbox = base_window_get_widget( BASE_WINDOW( window ), "MenubarVBox" );
	gtk_box_pack_start( GTK_BOX( vbox ), menubar, FALSE, FALSE, 0 );

	toplevel = base_window_get_toplevel_window( BASE_WINDOW( window ));
	base_window_signal_connect(
			BASE_WINDOW( window ),
			G_OBJECT( toplevel ),
			"delete-event",
			G_CALLBACK( on_delete_event ));

	base_window_signal_connect(
			BASE_WINDOW( window ),
			G_OBJECT( window ),
			TAB_UPDATABLE_SIGNAL_SELECTION_CHANGED,
			G_CALLBACK( on_tab_updatable_selection_changed ));
}

/**
 * nact_main_menubar_refresh_actions_sensitivity:
 *
 * Sensitivity of items in the menubar is primarily refreshed when
 * "tab-updatable-selection-updated" signal is received.
 * This signal itself is sent on new selection in IActionsList.
 * E.g in "cut" action, this happens before having stored the items
 * in the clipboard.
 * We so have to refresh the menubar items on demand.
 */
void
nact_main_menubar_refresh_actions_sensitivity( NactMainWindow *window )
{
	GList *selected;
	guint count;

	g_return_if_fail( NACT_MAIN_WINDOW( window ));
	g_return_if_fail( NACT_IS_IACTIONS_LIST( window ));

	selected = nact_iactions_list_get_selected_items( NACT_IACTIONS_LIST( window ));
	count = g_list_length( selected );
	na_object_free_items( selected );
	refresh_actions_sensitivity_with_count( window, count );
}

static void
on_tab_updatable_selection_changed( NactMainWindow *window, gint count_selected )
{
	refresh_actions_sensitivity_with_count( window, count_selected );
}

static void
on_new_menu_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	NAObjectMenu *menu;
	GList *items;

	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	menu = na_object_menu_new();
	items = g_list_prepend( NULL, menu );
	nact_iactions_list_insert_items( NACT_IACTIONS_LIST( window ), items, NULL );
	na_object_free_items( items );
}

static void
on_new_action_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	NAObjectAction *action;
	GList *items;

	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	action = na_object_action_new_with_profile();
	items = g_list_prepend( NULL, action );
	nact_iactions_list_insert_items( NACT_IACTIONS_LIST( window ), items, NULL );
	na_object_free_items( items );
}

static void
on_new_profile_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	NAObjectAction *action;
	NAObjectProfile *profile;
	gchar *name;
	GList *items;

	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	g_object_get(
			G_OBJECT( window ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, &action,
			NULL );

	profile = na_object_profile_new();

	name = na_object_action_get_new_profile_name( action );
	/*na_object_action_attach_profile( action, profile );*/
	na_object_profile_set_action( profile, action );
	na_object_set_id( profile, name );
	na_object_check_edition_status( profile );

	items = g_list_prepend( NULL, profile );
	nact_iactions_list_insert_items( NACT_IACTIONS_LIST( window ), items, NULL );

	na_object_free_items( items );
	g_free( name );
}

/*
 * saving is not only saving modified items, but also saving hierarchy
 * (and order if alpha order is not set)
 */
static void
on_save_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	GList *items, *it;
	NactApplication *application;
	NAPivot *pivot;

	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));
	g_return_if_fail( NACT_IS_WINDOW( window ));

	items = nact_iactions_list_get_items( NACT_IACTIONS_LIST( window ));
	nact_window_write_level_zero( NACT_WINDOW( window ), items );

	/* recursively save the valid modified items
	 */
	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	pivot = nact_application_get_pivot( application );

	for( it = items ; it ; it = it->next ){
		g_return_if_fail( NA_IS_OBJECT_ITEM( it->data ));
		save_object_item( window, pivot, NA_OBJECT_ITEM( it->data ));
	}

	/* doesn't unref object owned by the tree store
	 */
	g_list_free( items );

	/* delete the removed actions
	 */
	nact_main_window_remove_deleted( window );

	nact_main_menubar_refresh_actions_sensitivity( window );
	na_ipivot_consumer_delay_notify( NA_IPIVOT_CONSUMER( window ));
}

/*
 * recursively saving an object item
 * - add a duplicate of this action to pivot or update the origin
 * - set the duplicate as the origin of this action
 * - reset the status
 */
static void
save_object_item( NactMainWindow *window, NAPivot *pivot, NAObjectItem *object )
{
	GList *items, *it;
	NAObjectItem *origin;
	NAObjectItem *dup_pivot;

	if( na_object_is_modified( NA_OBJECT( object )) &&
		na_object_is_valid( NA_OBJECT( object )) &&
		nact_window_save_item( NACT_WINDOW( window ), object )){

			g_debug( "save_object_item: about to get origin" );
			/* do not use OA_OBJECT_ITEM macro as this may return a (valid)
			 * NULL value
			 */
			origin = ( NAObjectItem * ) na_object_get_origin( object );

			if( origin ){
				g_debug( "save_object_item: about to copy to origin" );
				na_object_copy( origin, object );

			} else {
				g_debug( "save_object_item: about to duplicate" );
				dup_pivot = NA_OBJECT_ITEM( na_object_duplicate( object ));
				na_object_set_origin( dup_pivot, NULL );
				na_object_set_origin( object, dup_pivot );
				na_pivot_add_item( pivot, NA_OBJECT( dup_pivot ));
			}

			g_debug( "save_object_item: about to check edition status" );
			na_object_check_edition_status( object );
	}

	/* Profiles are saved with their actions
	 * so we only have to take care of menus
	 */
	if( NA_IS_OBJECT_MENU( object )){
		items = na_object_get_items( NA_OBJECT_MENU( object ));
		for( it = items ; it ; it = it->next ){
			g_return_if_fail( NA_IS_OBJECT_ITEM( it->data ));
			save_object_item( window, pivot, NA_OBJECT_ITEM( it->data ));
		}
		na_object_free_items( items );
	}
}

static void
on_quit_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_menubar_on_quit_activated";
	gboolean has_modified;

	g_debug( "%s: item=%p, window=%p", thisfn, ( void * ) gtk_action, ( void * ) window );
	g_return_if_fail( GTK_IS_ACTION( gtk_action ) || gtk_action == NULL );
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	has_modified = nact_main_window_has_modified_items( window );
	if( !has_modified || nact_window_warn_modified( NACT_WINDOW( window ))){
		g_object_unref( window );
	}
}

/*
 * cuts the visible selection
 * - (tree) get new refs on selected items
 * - (tree) remove selected items, unreffing objects
 * - (main) add selected items to main list of deleted,
 *          moving newref from list_from_tree to main_list_of_deleted
 * - (menu) install in clipboard a copy of selected objects
 * - (tree) select next row (if any, or previous if any, or none)
 */
static void
on_cut_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	GList *items;

	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	items = nact_iactions_list_get_selected_items( NACT_IACTIONS_LIST( window ));
	nact_main_window_move_to_deleted( window, items );
	nact_clipboard_primary_set( items, FALSE );
	nact_iactions_list_delete_selection( NACT_IACTIONS_LIST( window ));

	/* do not unref selected items as the ref has been moved to main_deleted
	 */
	/*g_list_free( items );*/
}

/*
 * copies the visible selection
 * - (tree) get new refs on selected items
 * - (menu) install in clipboard a copy of selected objects
 *          renumbering actions/menus id to ensure unicity at paste time
 * - (menu) release refs on selected items
 * - (menu) refresh actions sensitivy (as selection doesn't change)
 */
static void
on_copy_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	GList *items;

	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	items = nact_iactions_list_get_selected_items( NACT_IACTIONS_LIST( window ));
	nact_clipboard_primary_set( items, TRUE );
	na_object_free_items( items );
	nact_main_menubar_refresh_actions_sensitivity( window );
}

/*
 * pastes the current content of the clipboard
 * - (menu) get from clipboard a copy of installed items
 *          the clipboard will return a new copy
 *          and renumber its own data for allowing a new paste
 * - (tree) insert new items, the tree store will ref them
 *          attaching each item to its parent
 *          checking edition status of the topmost parent
 *          selecting the first item at end
 * - (menu) unreffing the copy got from clipboard
 */
static void
on_paste_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	GList *items;

	items = nact_clipboard_primary_get();
	nact_iactions_list_insert_items( NACT_IACTIONS_LIST( window ), items, NULL );
	na_object_free_items( items );
}

/*
 * duplicate is just as paste, with the difference that content comes
 * from the current selection, instead of coming from the clipboard
 *
 * this is nonetheless a bit more complicated because when we duplicate
 * some items (e.g. a multiple selection), we expect to see the new
 * items just besides the original ones...
 */
static void
on_duplicate_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	GList *items, *it;
	GList *dup;
	NAObject *obj;

	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	items = nact_iactions_list_get_selected_items( NACT_IACTIONS_LIST( window ));
	for( it = items ; it ; it = it->next ){
		obj = NA_OBJECT( na_object_duplicate( it->data ));
		if( NA_IS_OBJECT_ITEM( obj )){
			na_object_set_new_id( obj );
		}
		na_object_set_origin_rec( obj, NULL );
		dup = g_list_prepend( NULL, obj );
		nact_iactions_list_insert_items( NACT_IACTIONS_LIST( window ), dup, it->data );
		na_object_free_items( dup );
	}

	na_object_free_items( items );
}

/*
 * deletes the visible selection
 * - (tree) get new refs on selected items
 * - (tree) remove selected items, unreffing objects
 * - (main) add selected items to main list of deleted,
 *          moving newref from list_from_tree to main_list_of_deleted
 * - (tree) select next row (if any, or previous if any, or none)
 */
static void
on_delete_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	GList *items;

	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	items = nact_iactions_list_get_selected_items( NACT_IACTIONS_LIST( window ));
	nact_main_window_move_to_deleted( window, items );
	nact_iactions_list_delete_selection( NACT_IACTIONS_LIST( window ));

	/* do not unref selected items as the ref has been moved to main_deleted
	 */
	/*g_list_free( items );*/
}

static void
on_reload_activated( GtkAction *gtk_action, NactMainWindow *window )
{
}

static void
on_preferences_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	nact_preferences_editor_run( BASE_WINDOW( window ));
}

static void
on_import_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	nact_assistant_import_run( BASE_WINDOW( window ));
}

static void
on_export_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	nact_assistant_export_run( BASE_WINDOW( window ));
}

static void
on_help_activated( GtkAction *gtk_action, NactMainWindow *window )
{
}

static void
on_about_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	GtkWindow *toplevel;

	toplevel = base_window_get_toplevel_window( BASE_WINDOW( window ));

	na_about_display( G_OBJECT( toplevel ));
}

static void
enable_item( NactMainWindow *window, const gchar *name, gboolean enabled )
{
	GtkActionGroup *group;
	GtkAction *action;

	group = g_object_get_data( G_OBJECT( window ), MENUBAR_PROP_ACTIONS_GROUP );
	action = gtk_action_group_get_action( group, name );
	gtk_action_set_sensitive( action, enabled );
}

static gboolean
on_delete_event( GtkWidget *toplevel, GdkEvent *event, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_menubar_on_delete_event";

	g_debug( "%s: toplevel=%p, event=%p, window=%p",
			thisfn, ( void * ) toplevel, ( void * ) event, ( void * ) window );

	on_quit_activated( NULL, window );

	return( TRUE );
}

/*
 * this callback is declared when attaching ui_manager and actions_group
 * as data to the window ; it is so triggered on NactMainWindow::finalize()
 */
static void
on_destroy_callback( gpointer data )
{
	g_debug( "nact_main_menubar_on_destroy_callback: data=%p", ( void * ) data );
	g_object_unref( G_OBJECT( data ));
}

static void
on_menu_item_selected( GtkMenuItem *proxy, NactMainWindow *window )
{
	GtkAction *action;
	const gchar *tooltip;

	action = gtk_activatable_get_related_action( GTK_ACTIVATABLE( proxy ));
	if( !action ){
		return;
	}

	tooltip = gtk_action_get_tooltip( action );
	if( !tooltip ){
		return;
	}

	nact_main_statusbar_display_status( window, MENUBAR_PROP_STATUS_CONTEXT, tooltip );
}

static void
on_menu_item_deselected( GtkMenuItem *proxy, NactMainWindow *window )
{
	nact_main_statusbar_hide_status( window, MENUBAR_PROP_STATUS_CONTEXT );
}

static void
on_proxy_connect( GtkActionGroup *action_group, GtkAction *action, GtkWidget *proxy, NactMainWindow *window )
{
	g_debug( "on_proxy_connect: action_group=%p, action=%p, proxy=%p, window=%p",
			( void * ) action_group, ( void * ) action, ( void * ) proxy, ( void * ) window );

	if( GTK_IS_MENU_ITEM( proxy )){

		base_window_signal_connect(
				BASE_WINDOW( window ),
				G_OBJECT( proxy ),
				"select",
				G_CALLBACK( on_menu_item_selected ));

		base_window_signal_connect(
				BASE_WINDOW( window ),
				G_OBJECT( proxy ),
				"deselect",
				G_CALLBACK( on_menu_item_deselected ));
	}
}

static void
on_proxy_disconnect( GtkActionGroup *action_group, GtkAction *action, GtkWidget *proxy, NactMainWindow *window )
{
	/* signal handlers will be automagically disconnected on NactWindow::dispose */
}

static void
refresh_actions_sensitivity_with_count( NactMainWindow *window, gint count_selected )
{
	static const gchar *thisfn = "nact_main_menubar_refresh_actions_sensitivity_with_count";
	NAObjectItem *item;
	NAObjectProfile *profile;
	gboolean has_exportable;
	gboolean has_modified;
	guint nb_actions, nb_profiles, nb_menus;
	gboolean paste_enabled;

	g_debug( "%s: window=%p, count_selected=%d", thisfn, ( void * ) window, count_selected );
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	g_object_get(
			G_OBJECT( window ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, &item,
			TAB_UPDATABLE_PROP_EDITED_PROFILE, &profile,
			NULL );

	has_exportable = nact_iactions_list_has_exportable( NACT_IACTIONS_LIST( window ));
	has_modified = nact_main_window_has_modified_items( window );

	paste_enabled = FALSE;
	nact_clipboard_primary_counts( &nb_actions, &nb_profiles, &nb_menus );
	g_debug( "%s: actions=%d, profiles=%d, menus=%d", thisfn, nb_actions, nb_profiles, nb_menus );
	if( nb_profiles ){
		paste_enabled = NA_IS_OBJECT_ACTION( item );
	} else {
		paste_enabled = ( nb_actions + nb_menus > 0 );
	}

	/* new menu always enabled */
	/* new action always enabled */
	/* new profile enabled if selection is relative to only one action */
	enable_item( window, "NewProfileItem", item != NULL && !NA_IS_OBJECT_MENU( item ));
	/* save enabled if at least one item has been modified */
	enable_item( window, "SaveItem", has_modified );
	/* quit always enabled */
	/* cut/copy enabled if selection not empty */
	enable_item( window, "CutItem", count_selected > 0 );
	enable_item( window, "CopyItem", count_selected > 0 );
	/* paste enabled if
	 * - clipboard contains only profiles, and current selection is action or profile
	 * - clipboard contains actions or menus */
	enable_item( window, "PasteItem", paste_enabled );
	/* duplicate/delete enabled if selection not empty */
	enable_item( window, "DuplicateItem", count_selected > 0 );
	enable_item( window, "DeleteItem", count_selected > 0 );
	/* reload items always enabled */
	/* preferences always enabled */
	/* import item always enabled */
	/* export item enabled if IActionsList store contains actions */
	enable_item( window, "ExportItem", has_exportable );
	/* TODO: help temporarily disabled */
	enable_item( window, "HelpItem", FALSE );
	/* about always enabled */
}
