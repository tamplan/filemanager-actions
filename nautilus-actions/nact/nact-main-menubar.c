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

#include <api/na-object-api.h>

#include <runtime/na-iabout.h>
#include <runtime/na-iprefs.h>
#include <runtime/na-ipivot-consumer.h>

#include "nact-application.h"
#include "nact-assistant-export.h"
#include "nact-assistant-import.h"
#include "nact-preferences-editor.h"
#include "nact-iactions-list.h"
#include "nact-clipboard.h"
#include "nact-main-statusbar.h"
#include "nact-main-toolbar.h"
#include "nact-main-tab.h"
#include "nact-main-menubar.h"

#define MENUBAR_PROP_STATUS_CONTEXT			"nact-menubar-status-context"
#define MENUBAR_PROP_MAIN_STATUS_CONTEXT	"nact-menubar-main-status-context"
#define MENUBAR_PROP_UI_MANAGER				"nact-menubar-ui-manager"
#define MENUBAR_PROP_ACTIONS_GROUP			"nact-menubar-actions-group"
#define MENUBAR_IPREFS_FILE_TOOLBAR			"main-file-toolbar"
#define MENUBAR_IPREFS_EDIT_TOOLBAR			"main-edit-toolbar"
#define MENUBAR_IPREFS_TOOLS_TOOLBAR		"main-tools-toolbar"
#define MENUBAR_IPREFS_HELP_TOOLBAR			"main-help-toolbar"

enum {
	MENUBAR_FILE_TOOLBAR_POS = 0,
	MENUBAR_EDIT_TOOLBAR_POS,
	MENUBAR_TOOLS_TOOLBAR_POS,
	MENUBAR_HELP_TOOLBAR_POS
};

/* GtkActivatable
 * gtk_action_get_tooltip are only available starting with Gtk 2.16
 * until this is a required level, we must have some code to do the
 * same thing
 */
#undef GTK_HAS_ACTIVATABLE
#if(( GTK_MAJOR_VERSION > 2 ) || ( GTK_MINOR_VERSION >= 16 ))
	#define GTK_HAS_ACTIVATABLE
#endif

#ifndef GTK_HAS_ACTIVATABLE
#define MENUBAR_PROP_ITEM_ACTION			"nact-menubar-item-action"
#endif

/* this structure is updated each time the user interacts in the
 * interface ; it is then used to update action sensitivities
 */
typedef struct {
	gint     selected_menus;
	gint     selected_actions;
	gint     selected_profiles;
	gint     clipboard_menus;
	gint     clipboard_actions;
	gint     clipboard_profiles;
	gint     list_menus;
	gint     list_actions;
	gint     list_profiles;
	gboolean is_modified;
	gboolean have_exportables;
	gboolean treeview_has_focus;
	gboolean level_zero_order_changed;
	gulong   popup_handler;
}
	MenubarIndicatorsStruct;

#define MENUBAR_PROP_INDICATORS			"nact-menubar-indicators"

static void     on_iactions_list_count_updated( NactMainWindow *window, gint menus, gint actions, gint profiles );
static void     on_iactions_list_selection_changed( NactMainWindow *window, GList *selected );
static void     on_iactions_list_focus_in( NactMainWindow *window, gpointer user_data );
static void     on_iactions_list_focus_out( NactMainWindow *window, gpointer user_data );
static void     on_iactions_list_status_changed( NactMainWindow *window, gpointer user_data );
static void     on_level_zero_order_changed( NactMainWindow *window, gpointer user_data );
static void     on_update_sensitivities( NactMainWindow *window, gpointer user_data );

static void     on_new_menu_activated( GtkAction *action, NactMainWindow *window );
static void     on_new_action_activated( GtkAction *action, NactMainWindow *window );
static void     on_new_profile_activated( GtkAction *action, NactMainWindow *window );
static void     on_save_activated( GtkAction *action, NactMainWindow *window );
static void     save_item( NactMainWindow *window, NAPivot *pivot, NAObjectItem *item );
static void     on_quit_activated( GtkAction *action, NactMainWindow *window );

static void     on_cut_activated( GtkAction *action, NactMainWindow *window );
static void     on_copy_activated( GtkAction *action, NactMainWindow *window );
static void     on_paste_activated( GtkAction *action, NactMainWindow *window );
static void     on_paste_into_activated( GtkAction *action, NactMainWindow *window );
static GList   *prepare_for_paste( NactMainWindow *window );
static void     on_duplicate_activated( GtkAction *action, NactMainWindow *window );
static void     on_delete_activated( GtkAction *action, NactMainWindow *window );
static void     update_clipboard_counters( NactMainWindow *window );
static void     on_reload_activated( GtkAction *action, NactMainWindow *window );
static void     on_preferences_activated( GtkAction *action, NactMainWindow *window );

static void     on_expand_all_activated( GtkAction *action, NactMainWindow *window );
static void     on_collapse_all_activated( GtkAction *action, NactMainWindow *window );
static void     on_view_file_toolbar_activated( GtkToggleAction *action, NactMainWindow *window );
static void     on_view_edit_toolbar_activated( GtkToggleAction *action, NactMainWindow *window );
static void     on_view_tools_toolbar_activated( GtkToggleAction *action, NactMainWindow *window );
static void     on_view_help_toolbar_activated( GtkToggleAction *action, NactMainWindow *window );
static void     on_view_toolbar_activated( GtkToggleAction *action, NactMainWindow *window, int toolbar_id );

static void     on_import_activated( GtkAction *action, NactMainWindow *window );
static void     on_export_activated( GtkAction *action, NactMainWindow *window );

static void     on_dump_selection_activated( GtkAction *action, NactMainWindow *window );
static void     on_brief_tree_store_dump_activated( GtkAction *action, NactMainWindow *window );
static void     on_list_modified_items_activated( GtkAction *action, NactMainWindow *window );
static void     on_dump_clipboard_activated( GtkAction *action, NactMainWindow *window );

static void     on_help_activated( GtkAction *action, NactMainWindow *window );
static void     on_about_activated( GtkAction *action, NactMainWindow *window );

static void     enable_item( NactMainWindow *window, const gchar *name, gboolean enabled );
static gboolean on_delete_event( GtkWidget *toplevel, GdkEvent *event, NactMainWindow *window );
static void     on_destroy_callback( gpointer data );
static void     on_menu_item_selected( GtkMenuItem *proxy, NactMainWindow *window );
static void     on_menu_item_deselected( GtkMenuItem *proxy, NactMainWindow *window );
static void     on_popup_selection_done(GtkMenuShell *menushell, NactMainWindow *window );
static void     on_proxy_connect( GtkActionGroup *action_group, GtkAction *action, GtkWidget *proxy, NactMainWindow *window );
static void     on_proxy_disconnect( GtkActionGroup *action_group, GtkAction *action, GtkWidget *proxy, NactMainWindow *window );

static const GtkActionEntry entries[] = {

		{ "FileMenu", NULL, N_( "_File" ) },
		{ "EditMenu", NULL, N_( "_Edit" ) },
		{ "ViewMenu", NULL, N_( "_View" ) },
		{ "ViewToolbarMenu", NULL, N_( "_Toolbars" ) },
		{ "ToolsMenu", NULL, N_( "_Tools" ) },
		{ "MaintainerMenu", NULL, N_( "_Maintainer" ) },
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
				N_( "Insert the content of the clipboard just before the current position" ),
				G_CALLBACK( on_paste_activated ) },
		{ "PasteIntoItem" , NULL, N_( "Paste _into" ), "<Shift><Ctrl>V",
				/* i18n: tooltip displayed in the status bar when selecting the Paste Into item */
				N_( "Insert the content of the clipboard as first child of the current item" ),
				G_CALLBACK( on_paste_into_activated ) },
		{ "DuplicateItem" , NULL, N_( "D_uplicate" ), "",
				/* i18n: tooltip displayed in the status bar when selecting the Duplicate item */
				N_( "Duplicate the selected item(s)" ),
				G_CALLBACK( on_duplicate_activated ) },
		{ "DeleteItem", GTK_STOCK_DELETE, NULL, "Delete",
				/* i18n: tooltip displayed in the status bar when selecting the Delete item */
				N_( "Delete the selected item(s)" ),
				G_CALLBACK( on_delete_activated ) },
		{ "ReloadActionsItem", GTK_STOCK_REFRESH, N_( "_Reload the items" ), "F5",
				/* i18n: tooltip displayed in the status bar when selecting the 'Reload' item */
				N_( "Cancel your current modifications and reload the initial list of menus and actions" ),
				G_CALLBACK( on_reload_activated ) },
		{ "PreferencesItem", GTK_STOCK_PREFERENCES, NULL, NULL,
				/* i18n: tooltip displayed in the status bar when selecting the 'Preferences' item */
				N_( "Edit your preferences" ),
				G_CALLBACK( on_preferences_activated ) },
		{ "ExpandAllItem" , NULL, N_( "_Expand all" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the Expand all item */
				N_( "Entirely expand the items hierarchy" ),
				G_CALLBACK( on_expand_all_activated ) },
		{ "CollapseAllItem" , NULL, N_( "_Collapse all" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the Collapse all item */
				N_( "Entirely collapse the items hierarchy" ),
				G_CALLBACK( on_collapse_all_activated ) },
		{ "ImportItem" , GTK_STOCK_CONVERT, N_( "_Import assistant..." ), "",
				/* i18n: tooltip displayed in the status bar when selecting the Import item */
				N_( "Import one or more actions from external (XML) files into your configuration" ),
				G_CALLBACK( on_import_activated ) },
		{ "ExportItem", NULL, N_( "E_xport assistant..." ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the Export item */
				N_( "Export one or more actions from your configuration to external XML files" ),
				G_CALLBACK( on_export_activated ) },
		{ "DumpSelectionItem", NULL, N_( "_Dump the selection" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the Dump selection item */
				N_( "Recursively dump selected items" ),
				G_CALLBACK( on_dump_selection_activated ) },
		{ "BriefTreeStoreDumpItem", NULL, N_( "_Brief tree store dump" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the BriefTreeStoreDump item */
				N_( "Briefly dump the tree store" ),
				G_CALLBACK( on_brief_tree_store_dump_activated ) },
		{ "ListModifiedItems", NULL, N_( "_List modified items" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the ListModifiedItems item */
				N_( "List the modified items" ),
				G_CALLBACK( on_list_modified_items_activated ) },
		{ "DumpClipboard", NULL, N_( "_Dump the clipboard" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the DumpClipboard item */
				N_( "Dump the content of the clipboard object" ),
				G_CALLBACK( on_dump_clipboard_activated ) },
		{ "HelpItem" , GTK_STOCK_HELP, NULL, NULL,
				/* i18n: tooltip displayed in the status bar when selecting the Help item */
				N_( "Display help about this program" ),
				G_CALLBACK( on_help_activated ) },
		{ "AboutItem", GTK_STOCK_ABOUT, NULL, NULL,
				/* i18n: tooltip displayed in the status bar when selecting the About item */
				N_( "Display informations about this program" ),
				G_CALLBACK( on_about_activated ) },
};

static const GtkToggleActionEntry toolbar_entries[] = {

		{ "ViewFileToolbarItem", NULL, N_( "_File" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the 'View File toolbar' item */
				N_( "Display the File toolbar" ),
				G_CALLBACK( on_view_file_toolbar_activated ), FALSE },
		{ "ViewEditToolbarItem", NULL, N_( "_Edit" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the 'View Edit toolbar' item */
				N_( "Display the Edit toolbar" ),
				G_CALLBACK( on_view_edit_toolbar_activated ), FALSE },
		{ "ViewToolsToolbarItem", NULL, N_( "_Tools" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting 'View Tools toolbar' item */
				N_( "Display the Tools toolbar" ),
				G_CALLBACK( on_view_tools_toolbar_activated ), FALSE },
		{ "ViewHelpToolbarItem", NULL, N_( "_Help" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting 'View Help toolbar' item */
				N_( "Display the Help toolbar" ),
				G_CALLBACK( on_view_help_toolbar_activated ), FALSE },
};

/**
 * nact_main_menubar_runtime_init:
 * @window: the #NactMainWindow to which the menubar is attached.
 *
 * Creates the menubar.
 * Connects to all possible signals which may have an impact on action
 * sensitivities.
 */
void
nact_main_menubar_runtime_init( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_menubar_init";
	GtkActionGroup *action_group;
	GtkUIManager *ui_manager;
	GError *error = NULL;
	guint merge_id;
	GtkAccelGroup *accel_group;
	GtkWidget *menubar, *vbox;
	GtkWindow *toplevel;
	MenubarIndicatorsStruct *mis;
	gboolean has_maintainer_menu;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );

	/* create the menubar:
	 * - create action group, and insert list of actions in it
	 * - create ui_manager, and insert action group in it
	 * - merge inserted actions group with ui xml definition
	 * - install accelerators in the main window
	 * - pack menu bar in the main window
	 */
	ui_manager = gtk_ui_manager_new();
	g_object_set_data_full( G_OBJECT( window ), MENUBAR_PROP_UI_MANAGER, ui_manager, ( GDestroyNotify ) on_destroy_callback );
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
	gtk_action_group_add_toggle_actions( action_group, toolbar_entries, G_N_ELEMENTS( toolbar_entries ), window );
	gtk_ui_manager_insert_action_group( ui_manager, action_group, 0 );

	merge_id = gtk_ui_manager_add_ui_from_file( ui_manager, PKGDATADIR "/nautilus-actions-config-tool.actions", &error );
	if( merge_id == 0 || error ){
		g_warning( "%s: error=%s", thisfn, error->message );
		g_error_free( error );
	}

	has_maintainer_menu = FALSE;
#ifdef NA_MAINTAINER_MODE
	has_maintainer_menu = TRUE;
#endif

	if( has_maintainer_menu ){
		merge_id = gtk_ui_manager_add_ui_from_file( ui_manager, PKGDATADIR "/nautilus-actions-maintainer.actions", &error );
		if( merge_id == 0 || error ){
			g_warning( "%s: error=%s", thisfn, error->message );
			g_error_free( error );
		}
	}

	toplevel = base_window_get_toplevel( BASE_WINDOW( window ));
	accel_group = gtk_ui_manager_get_accel_group( ui_manager );
	gtk_window_add_accel_group( toplevel, accel_group );

	menubar = gtk_ui_manager_get_widget( ui_manager, "/ui/MainMenubar" );
	vbox = base_window_get_widget( BASE_WINDOW( window ), "MenubarVBox" );
	gtk_box_pack_start( GTK_BOX( vbox ), menubar, FALSE, FALSE, 0 );

	/* this creates a submenu in the toolbar */
	/*gtk_container_add( GTK_CONTAINER( vbox ), toolbar );*/

	base_window_signal_connect(
			BASE_WINDOW( window ),
			G_OBJECT( toplevel ),
			"delete-event",
			G_CALLBACK( on_delete_event ));

	base_window_signal_connect(
			BASE_WINDOW( window ),
			G_OBJECT( window ),
			IACTIONS_LIST_SIGNAL_LIST_COUNT_UPDATED,
			G_CALLBACK( on_iactions_list_count_updated ));

	base_window_signal_connect(
			BASE_WINDOW( window ),
			G_OBJECT( window ),
			IACTIONS_LIST_SIGNAL_SELECTION_CHANGED,
			G_CALLBACK( on_iactions_list_selection_changed ));

	base_window_signal_connect(
			BASE_WINDOW( window ),
			G_OBJECT( window ),
			IACTIONS_LIST_SIGNAL_FOCUS_IN,
			G_CALLBACK( on_iactions_list_focus_in ));

	base_window_signal_connect(
			BASE_WINDOW( window ),
			G_OBJECT( window ),
			IACTIONS_LIST_SIGNAL_FOCUS_OUT,
			G_CALLBACK( on_iactions_list_focus_out ));

	base_window_signal_connect(
			BASE_WINDOW( window ),
			G_OBJECT( window ),
			IACTIONS_LIST_SIGNAL_STATUS_CHANGED,
			G_CALLBACK( on_iactions_list_status_changed ));

	base_window_signal_connect(
			BASE_WINDOW( window ),
			G_OBJECT( window ),
			MAIN_WINDOW_SIGNAL_UPDATE_ACTION_SENSITIVITIES,
			G_CALLBACK( on_update_sensitivities ));

	base_window_signal_connect(
			BASE_WINDOW( window ),
			G_OBJECT( window ),
			MAIN_WINDOW_SIGNAL_LEVEL_ZERO_ORDER_CHANGED,
			G_CALLBACK( on_level_zero_order_changed ));

	mis = g_new0( MenubarIndicatorsStruct, 1 );
	g_object_set_data( G_OBJECT( window ), MENUBAR_PROP_INDICATORS, mis );

	nact_main_toolbar_init( window, action_group );
}

/**
 * nact_main_menubar_dispose:
 * @window: this #NactMainWindow window.
 *
 * Release internally allocated resources.
 */
void
nact_main_menubar_dispose( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_menubar_dispose";
	MenubarIndicatorsStruct *mis;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	mis = ( MenubarIndicatorsStruct * ) g_object_get_data( G_OBJECT( window ), MENUBAR_PROP_INDICATORS );
	g_free( mis );
}

/**
 * Whether the specified object should be relabeled when pasted ?
 * @object: the considered #NAObject-derived object.
 * @pivot: the #NAPivot instance.
 *
 * Returns: %TRUE if the object should be relabeled, %FALSE else.
 */
gboolean
nact_main_menubar_is_pasted_object_relabeled( NAObject *object, NAPivot *pivot )
{
	static const gchar *thisfn = "nact_main_menubar_is_pasted_object_relabeled";
	gboolean relabel;

	if( NA_IS_OBJECT_MENU( object )){
		relabel = na_iprefs_read_bool( NA_IPREFS( pivot ), IPREFS_RELABEL_MENUS, FALSE );
	} else if( NA_IS_OBJECT_ACTION( object )){
		relabel = na_iprefs_read_bool( NA_IPREFS( pivot ), IPREFS_RELABEL_ACTIONS, FALSE );
	} else if( NA_IS_OBJECT_PROFILE( object )){
		relabel = na_iprefs_read_bool( NA_IPREFS( pivot ), IPREFS_RELABEL_PROFILES, FALSE );
	} else {
		g_warning( "%s: unknown object type at %p", thisfn, ( void * ) object );
		g_return_val_if_reached( FALSE );
	}

	return( relabel );
}

/**
 * nact_main_menubar_open_popup:
 * @window: this #NactMainWindow window.
 * @event: the mouse event.
 *
 * Opens a popup menu.
 */
void
nact_main_menubar_open_popup( NactMainWindow *instance, GdkEventButton *event )
{
	GtkUIManager *ui_manager;
	GtkWidget *menu;
	MenubarIndicatorsStruct *mis;

	ui_manager = ( GtkUIManager * ) g_object_get_data( G_OBJECT( instance ), MENUBAR_PROP_UI_MANAGER );
	menu = gtk_ui_manager_get_widget( ui_manager, "/ui/Popup" );

	mis = ( MenubarIndicatorsStruct * ) g_object_get_data( G_OBJECT( instance ), MENUBAR_PROP_INDICATORS );
	mis->popup_handler = g_signal_connect( menu, "selection-done", G_CALLBACK( on_popup_selection_done ), instance );

	g_signal_emit_by_name( instance, MAIN_WINDOW_SIGNAL_UPDATE_ACTION_SENSITIVITIES, NULL );

	gtk_menu_popup( GTK_MENU( menu ), NULL, NULL, NULL, NULL, event->button, event->time );
}

/*
 * when the IActionsList is refilled, update our internal counters so
 * that we are knowing if we have some exportables
 */
static void
on_iactions_list_count_updated( NactMainWindow *window, gint menus, gint actions, gint profiles )
{
	MenubarIndicatorsStruct *mis;
	gchar *status;

	g_debug( "nact_main_menubar_on_iactions_list_count_updated: menus=%u, actions=%u, profiles=%u", menus, actions, profiles );
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	mis = ( MenubarIndicatorsStruct * ) g_object_get_data( G_OBJECT( window ), MENUBAR_PROP_INDICATORS );
	mis->list_menus = menus;
	mis->list_actions = actions;
	mis->list_profiles = profiles;
	mis->have_exportables = ( mis->list_actions > 0 );

	/* i18n: note the space at the beginning of the sentence */
	status = g_strdup_printf( _( " %d menus, %d actions, %d profiles are currently displayed" ), menus, actions, profiles );
	nact_main_statusbar_display_status( window, MENUBAR_PROP_MAIN_STATUS_CONTEXT, status );
	g_free( status );

	g_signal_emit_by_name( window, MAIN_WINDOW_SIGNAL_UPDATE_ACTION_SENSITIVITIES, NULL );
}

/*
 * when the selection changes in IActionsList, see what is selected
 */
static void
on_iactions_list_selection_changed( NactMainWindow *window, GList *selected )
{
	MenubarIndicatorsStruct *mis;

	g_debug( "nact_main_menubar_on_iactions_list_selection_changed: selected=%p (count=%d)",
			( void * ) selected, g_list_length( selected ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	mis = ( MenubarIndicatorsStruct * ) g_object_get_data( G_OBJECT( window ), MENUBAR_PROP_INDICATORS );
	mis->selected_menus = 0;
	mis->selected_actions = 0;
	mis->selected_profiles = 0;
	na_object_item_count_items( selected, &mis->selected_menus, &mis->selected_actions, &mis->selected_profiles, FALSE );
	g_debug( "nact_main_menubar_on_iactions_list_selection_changed: menus=%d, actions=%d, profiles=%d",
			mis->selected_menus, mis->selected_actions, mis->selected_profiles );

	g_signal_emit_by_name( window, MAIN_WINDOW_SIGNAL_UPDATE_ACTION_SENSITIVITIES, NULL );
}

static void
on_iactions_list_focus_in( NactMainWindow *window, gpointer user_data )
{
	MenubarIndicatorsStruct *mis;

	g_debug( "nact_main_menubar_on_iactions_list_focus_in" );
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	mis = ( MenubarIndicatorsStruct * ) g_object_get_data( G_OBJECT( window ), MENUBAR_PROP_INDICATORS );
	mis->treeview_has_focus = TRUE;
	g_signal_emit_by_name( window, MAIN_WINDOW_SIGNAL_UPDATE_ACTION_SENSITIVITIES, NULL );
}

static void
on_iactions_list_focus_out( NactMainWindow *window, gpointer user_data )
{
	MenubarIndicatorsStruct *mis;

	g_debug( "nact_main_menubar_on_iactions_list_focus_out" );
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	mis = ( MenubarIndicatorsStruct * ) g_object_get_data( G_OBJECT( window ), MENUBAR_PROP_INDICATORS );
	mis->treeview_has_focus = FALSE;
	g_signal_emit_by_name( window, MAIN_WINDOW_SIGNAL_UPDATE_ACTION_SENSITIVITIES, NULL );
}

static void
on_iactions_list_status_changed( NactMainWindow *window, gpointer user_data )
{
	g_debug( "nact_main_menubar_on_iactions_list_status_changed" );
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	g_signal_emit_by_name( window, MAIN_WINDOW_SIGNAL_UPDATE_ACTION_SENSITIVITIES, NULL );
}

static void
on_level_zero_order_changed( NactMainWindow *window, gpointer user_data )
{
	MenubarIndicatorsStruct *mis;

	g_debug( "nact_main_menubar_on_level_zero_order_changed" );
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	mis = ( MenubarIndicatorsStruct * ) g_object_get_data( G_OBJECT( window ), MENUBAR_PROP_INDICATORS );
	mis->level_zero_order_changed = GPOINTER_TO_INT( user_data );
	g_signal_emit_by_name( window, MAIN_WINDOW_SIGNAL_UPDATE_ACTION_SENSITIVITIES, NULL );
}

static void
on_update_sensitivities( NactMainWindow *window, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_menubar_on_update_sensitivities";
	NactApplication *application;
	NAPivot *pivot;
	gboolean is_level_zero_writable;
	gboolean has_writable_providers;
	gboolean has_modified_items;
	GList *selected_items, *is;
	NAObject *first_parent;
	gboolean is_first_parent_writable;
	gboolean new_item_enabled;
	gboolean new_profile_enabled;
	NAObject *selected_action;
	NAObject *parent_item;
	MenubarIndicatorsStruct *mis;
	gboolean is_clipboard_empty;
	gint count_selected;
	gboolean are_parents_writable;
	NAObject *selected_item;
	gboolean cut_enabled;
	gboolean copy_enabled;
	gboolean paste_enabled;
	gboolean paste_into_enabled;
	gboolean duplicate_enabled;
	gboolean delete_enabled;
	gint count_list;

	g_debug( "%s: window=%p, user_data=%p", thisfn, ( void * ) window, ( void * ) user_data );
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	pivot = nact_application_get_pivot( application );
	is_level_zero_writable = na_pivot_is_level_zero_writable( pivot );

	has_writable_providers = nact_window_has_writable_providers( NACT_WINDOW( window ));
	g_debug( "%s: has_writable_providers=%s", thisfn, has_writable_providers ? "True":"False" );

	has_modified_items = nact_main_window_has_modified_items( window );
	g_debug( "%s: has_modified_items=%s", thisfn, has_modified_items ? "True":"False" );

	selected_items = nact_iactions_list_bis_get_selected_items( NACT_IACTIONS_LIST( window ));
	first_parent = selected_items && g_list_length( selected_items )
			? ( NAObject * ) na_object_get_parent( selected_items->data )
			: NULL;
	is_first_parent_writable = first_parent
			? na_pivot_is_item_writable( pivot, NA_OBJECT_ITEM( first_parent ), NULL )
			: is_level_zero_writable;

	mis = ( MenubarIndicatorsStruct * ) g_object_get_data( G_OBJECT( window ), MENUBAR_PROP_INDICATORS );
	is_clipboard_empty = ( mis->clipboard_menus + mis->clipboard_actions + mis->clipboard_profiles == 0 );
	count_selected = selected_items ? g_list_length( selected_items ) : 0;
	g_debug( "%s: count_selected=%d", thisfn, count_selected );

	/* new menu / new action
	 * new item will be inserted just before beginning of selection
	 * parent of the first selected row must be writable
	 * we must have at least one writable provider
	 */
	new_item_enabled = is_first_parent_writable && has_writable_providers;
	enable_item( window, "NewMenuItem", new_item_enabled );
	enable_item( window, "NewActionItem", new_item_enabled );

	/* new profile enabled if selection is relative to only one writable action
	 * i.e. contains profile(s) of the same action, or only contains one action
	 * action must be writable
	 */
	new_profile_enabled = TRUE;
	selected_action = NULL;
	for( is = selected_items ; is ; is = is->next ){

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
	enable_item( window, "NewProfileItem",
			new_profile_enabled &&
			selected_action != NULL &&
			na_pivot_is_item_writable( pivot, NA_OBJECT_ITEM( selected_action ), NULL ));

	/* save enabled if at least one item has been modified
	 * or level-zero has been resorted and is writable
	 */
	enable_item( window, "SaveItem",
			has_modified_items || ( mis->level_zero_order_changed && is_level_zero_writable ));

	/* quit always enabled */

	/* cut requires a non-empty selection
	 * and that all parents are writable (as implies a delete operation)
	 */
	cut_enabled = mis->treeview_has_focus || mis->popup_handler;
	cut_enabled &= count_selected > 0;
	are_parents_writable = TRUE;
	for( is = selected_items ; is ; is = is->next ){
		parent_item = ( NAObject * ) na_object_get_parent( is->data );
		if( parent_item ){
			if( !na_pivot_is_item_writable( pivot, NA_OBJECT_ITEM( parent_item ), NULL )){
				are_parents_writable = FALSE;
				break;
			}
		} else if( !is_level_zero_writable ){
			are_parents_writable = FALSE;
			break;
		}
	}
	cut_enabled &= are_parents_writable;
	enable_item( window, "CutItem", cut_enabled );

	/* copy only requires a non-empty selection */
	copy_enabled = mis->treeview_has_focus || mis->popup_handler;
	copy_enabled &= count_selected > 0;
	enable_item( window, "CopyItem", copy_enabled );

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
	paste_enabled = mis->treeview_has_focus || mis->popup_handler;
	paste_enabled &= !is_clipboard_empty;
	paste_enabled &= count_selected <= 1;
	if( mis->clipboard_profiles ){
		paste_enabled &= count_selected == 1;
		if( paste_enabled ){
			selected_action = NA_OBJECT(
					NA_IS_OBJECT_PROFILE( selected_items->data )
							? na_object_get_parent( selected_items->data )
							: selected_items->data );
			paste_enabled &= NA_IS_OBJECT_ACTION( selected_action );
			paste_enabled &= na_pivot_is_item_writable( pivot, NA_OBJECT_ITEM( selected_action ), NULL );
		}
	} else {
		paste_enabled &= has_writable_providers;
		if( count_selected ){
			selected_item = NA_OBJECT( selected_items->data );
			paste_enabled &= NA_IS_OBJECT_ITEM( selected_item );
			if( paste_enabled ){
				parent_item = ( NAObject * ) na_object_get_parent( selected_item );
				paste_enabled &= parent_item
						? na_pivot_is_item_writable( pivot, NA_OBJECT_ITEM( parent_item ), NULL )
						: is_level_zero_writable;
			}
		} else {
			paste_enabled &= is_level_zero_writable;
		}
	}
	enable_item( window, "PasteItem", paste_enabled );

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
	paste_into_enabled = mis->treeview_has_focus || mis->popup_handler;
	paste_into_enabled &= !is_clipboard_empty;
	paste_into_enabled &= count_selected <= 1;
	if( mis->clipboard_profiles ){
		paste_enabled &= count_selected == 1;
		if( paste_enabled ){
			selected_action = NA_OBJECT( selected_items->data );
			paste_enabled &= NA_IS_OBJECT_ACTION( selected_action );
			paste_enabled &= na_pivot_is_item_writable( pivot, NA_OBJECT_ITEM( selected_action ), NULL );
		}
	} else {
		paste_enabled &= has_writable_providers;
		if( count_selected ){
			selected_item = NA_OBJECT( selected_items->data );
			paste_enabled &= NA_IS_OBJECT_MENU( selected_item );
			if( paste_enabled ){
				parent_item = ( NAObject * ) na_object_get_parent( selected_item );
				paste_enabled &= parent_item
						? na_pivot_is_item_writable( pivot, NA_OBJECT_ITEM( parent_item ), NULL )
						: is_level_zero_writable;
			}
		} else {
			paste_enabled &= is_level_zero_writable;
		}
	}
	enable_item( window, "PasteIntoItem", paste_into_enabled );

	/* duplicate items will be duplicated besides each one
	 * selection must be non-empty
	 * each parent must be writable
	 * -> so this is the same than cut
	 */
	duplicate_enabled = cut_enabled;
	enable_item( window, "DuplicateItem", duplicate_enabled );

	/* delete is same that cut */
	delete_enabled = cut_enabled;
	enable_item( window, "DeleteItem", delete_enabled );

	/* reload items always enabled */

	/* preferences always enabled */

	/* expand all/collapse all requires at least one item in the list */
	count_list = mis->list_menus + mis->list_actions + mis->list_profiles;
	enable_item( window, "ExpandAllItem", count_list > 0 );
	enable_item( window, "CollapseAllItem", count_list > 0 );

	/* import item enabled if at least one writable provider */
	enable_item( window, "ImportItem", has_writable_providers );

	/* export item enabled if IActionsList store contains actions */
	enable_item( window, "ExportItem", mis->have_exportables );

	/* TODO: help temporarily disabled */
	enable_item( window, "HelpItem", FALSE );

	/* about always enabled */

	na_object_free_items_list( selected_items );
}

static void
on_new_menu_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	NAObjectMenu *menu;
	GList *items;

	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	menu = na_object_menu_new();
	na_object_check_status( menu );
	items = g_list_prepend( NULL, menu );
	nact_iactions_list_bis_insert_items( NACT_IACTIONS_LIST( window ), items, NULL );
	na_object_free_items_list( items );
}

static void
on_new_action_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	NAObjectAction *action;
	GList *items;

	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	action = na_object_action_new_with_profile();
	na_object_check_status( action );
	items = g_list_prepend( NULL, action );
	nact_iactions_list_bis_insert_items( NACT_IACTIONS_LIST( window ), items, NULL );
	na_object_free_items_list( items );
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
	na_object_set_parent( profile, action );
	na_object_set_id( profile, name );
	na_object_check_status( profile );

	items = g_list_prepend( NULL, profile );
	nact_iactions_list_bis_insert_items( NACT_IACTIONS_LIST( window ), items, NULL );

	na_object_free_items_list( items );
	g_free( name );
}

/*
 * saving is not only saving modified items, but also saving hierarchy
 * (and order if alpha order is not set)
 */
static void
on_save_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_menubar_on_save_activated";

	g_debug( "%s: gtk_action=%p, window=%p", thisfn, ( void * ) gtk_action, ( void * ) window );
	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	nact_main_menubar_save_items( window );
}

/*
 * saving is not only saving modified items, but also saving hierarchy
 * (and order if alpha order is not set)
 */
void
nact_main_menubar_save_items( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_menubar_save_items";
	GList *items, *it;
	NactApplication *application;
	NAPivot *pivot;
	MenubarIndicatorsStruct *mis;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	/* remove deleted items
	 * so that new actions with same id do not risk to be deleted later
	 */
	nact_main_window_remove_deleted( window );

	/* always write the level zero list of items
	 * and reset the corresponding modification flag
	 */
	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	pivot = nact_application_get_pivot( application );
	items = nact_iactions_list_bis_get_items( NACT_IACTIONS_LIST( window ));
	na_pivot_write_level_zero( pivot, items );

	mis = ( MenubarIndicatorsStruct * ) g_object_get_data( G_OBJECT( window ), MENUBAR_PROP_INDICATORS );
	mis->level_zero_order_changed = FALSE;

	/* recursively save the modified items
	 * check is useless here if item was not modified, but not very costly
	 * above all, it is cheaper to check the status here, than to check
	 * recursively each and every modified item
	 */
	for( it = items ; it ; it = it->next ){
		save_item( window, pivot, NA_OBJECT_ITEM( it->data ));
		na_object_check_status( it->data );
	}
	g_list_free( items );

	/* get ride of notification messages of IOProviders
	 */
	na_ipivot_consumer_delay_notify( NA_IPIVOT_CONSUMER( window ));
}

/*
 * iterates here on each and every row stored in the tree
 * - do not deal with profiles as they are directly managed by their
 *   action parent
 * - do not deal with not modified, or not valid, items, but allow
 *   for save their subitems
 */
static void
save_item( NactMainWindow *window, NAPivot *pivot, NAObjectItem *item )
{
	NAObjectItem *origin;
	NAObjectItem *dup_pivot;
	GList *subitems, *it;
	NAObjectItem *parent;
	gint pos;
	NAIOProvider *provider_before;
	NAIOProvider *provider_after;

	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));
	g_return_if_fail( NA_IS_PIVOT( pivot ));
	g_return_if_fail( NA_IS_OBJECT_ITEM( item ));

	if( NA_IS_OBJECT_MENU( item )){
		subitems = na_object_get_items_list( item );
		for( it = subitems ; it ; it = it->next ){
			save_item( window, pivot, NA_OBJECT_ITEM( it->data ));
		}
	}

	provider_before = na_object_get_provider( item );

	if( na_object_is_modified( item ) &&
		nact_window_save_item( NACT_WINDOW( window ), item )){

			if( NA_IS_OBJECT_ACTION( item )){
				na_object_action_reset_last_allocated( NA_OBJECT_ACTION( item ));
			}

			/* do not use NA_OBJECT_ITEM macro as this may return a
			 * (valid) NULL value
			 */
			origin = ( NAObjectItem * ) na_object_get_origin( item );
			parent = NULL;
			pos = -1;

			if( origin ){
				parent = na_object_get_parent( origin );
				if( parent ){
					pos = na_object_get_position( parent, origin );
					na_object_remove_item( parent, origin );
				} else {
					na_pivot_remove_item( pivot, NA_OBJECT( origin ));
				}
			}

			dup_pivot = NA_OBJECT_ITEM( na_object_duplicate( item ));
			na_object_reset_origin( item, dup_pivot );
			na_object_set_parent( dup_pivot, parent );
			if( parent ){
				if( pos == -1 ){
					na_object_append_item( parent, dup_pivot );
				} else {
					na_object_insert_at( parent, dup_pivot, pos );
				}
			} else {
				na_pivot_add_item( pivot, dup_pivot );
			}

			nact_iactions_list_bis_removed_modified( NACT_IACTIONS_LIST( window ), item );

			provider_after = na_object_get_provider( item );
			if( provider_after != provider_before ){
				g_signal_emit_by_name( window, TAB_UPDATABLE_SIGNAL_PROVIDER_CHANGED, item );
			}
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
 * - (main) add selected items to main list of deleted,
 *          moving newref from list_from_tree to main_list_of_deleted
 * - (menu) install in clipboard a copy of selected objects
 * - (tree) remove selected items, unreffing objects
 */
static void
on_cut_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_menubar_on_cut_activated";
	GList *items;
	NactClipboard *clipboard;

	g_debug( "%s: gtk_action=%p, window=%p", thisfn, ( void * ) gtk_action, ( void * ) window );
	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	items = nact_iactions_list_bis_get_selected_items( NACT_IACTIONS_LIST( window ));
	nact_main_window_move_to_deleted( window, items );
	clipboard = nact_main_window_get_clipboard( window );
	nact_clipboard_primary_set( clipboard, items, CLIPBOARD_MODE_CUT );
	update_clipboard_counters( window );
	nact_iactions_list_bis_delete( NACT_IACTIONS_LIST( window ), items );

	/* do not unref selected items as the list has been concatenated
	 * to main_deleted
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
	static const gchar *thisfn = "nact_main_menubar_on_copy_activated";
	GList *items;
	NactClipboard *clipboard;

	g_debug( "%s: gtk_action=%p, window=%p", thisfn, ( void * ) gtk_action, ( void * ) window );
	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	items = nact_iactions_list_bis_get_selected_items( NACT_IACTIONS_LIST( window ));
	clipboard = nact_main_window_get_clipboard( window );
	nact_clipboard_primary_set( clipboard, items, CLIPBOARD_MODE_COPY );
	update_clipboard_counters( window );
	na_object_free_items_list( items );

	g_signal_emit_by_name( window, MAIN_WINDOW_SIGNAL_UPDATE_ACTION_SENSITIVITIES, NULL );
}

/*
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
static void
on_paste_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_menubar_on_paste_activated";
	GList *items;

	g_debug( "%s: gtk_action=%p, window=%p", thisfn, ( void * ) gtk_action, ( void * ) window );

	items = prepare_for_paste( window );
	if( items ){
		nact_iactions_list_bis_insert_items( NACT_IACTIONS_LIST( window ), items, NULL );
		na_object_free_items_list( items );
	}
}

/*
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
static void
on_paste_into_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_menubar_on_paste_into_activated";
	GList *items;

	g_debug( "%s: gtk_action=%p, window=%p", thisfn, ( void * ) gtk_action, ( void * ) window );

	items = prepare_for_paste( window );
	if( items ){
		nact_iactions_list_bis_insert_into( NACT_IACTIONS_LIST( window ), items );
		na_object_free_items_list( items );
	}
}

static GList *
prepare_for_paste( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_menubar_prepare_for_paste";
	GList *items, *it;
	NactClipboard *clipboard;
	NAObjectAction *action;
	gboolean relabel;
	gboolean renumber;
	NactApplication *application;
	NAPivot *pivot;

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	pivot = nact_application_get_pivot( application );

	clipboard = nact_main_window_get_clipboard( window );
	items = nact_clipboard_primary_get( clipboard, &renumber );
	action = NULL;

	/* if pasted items are profiles, then setup the target action
	 */
	for( it = items ; it ; it = it->next ){

		if( NA_IS_OBJECT_PROFILE( it->data )){
			if( !action ){
				g_object_get( G_OBJECT( window ), TAB_UPDATABLE_PROP_EDITED_ACTION, &action, NULL );
				g_return_val_if_fail( NA_IS_OBJECT_ACTION( action ), NULL );
			}
		}

		relabel = nact_main_menubar_is_pasted_object_relabeled( NA_OBJECT( it->data ), pivot );
		na_object_prepare_for_paste( it->data, relabel, renumber, action );
		na_object_check_status( it->data );
	}

	g_debug( "%s: action=%p (%s)",
			thisfn, ( void * ) action, action ? G_OBJECT_TYPE_NAME( action ): "(null)" );

	return( items );
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
	static const gchar *thisfn = "nact_main_menubar_on_duplicate_activated";
	NactApplication *application;
	NAPivot *pivot;
	NAObjectAction *action;
	GList *items, *it;
	GList *dup;
	NAObject *obj;
	gboolean relabel;

	g_debug( "%s: gtk_action=%p, window=%p", thisfn, ( void * ) gtk_action, ( void * ) window );
	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	pivot = nact_application_get_pivot( application );

	items = nact_iactions_list_bis_get_selected_items( NACT_IACTIONS_LIST( window ));
	for( it = items ; it ; it = it->next ){
		obj = NA_OBJECT( na_object_duplicate( it->data ));
		action = NULL;

		/* duplicating a profile
		 * as we insert in sibling mode, the parent doesn't change
		 */
		if( NA_IS_OBJECT_PROFILE( obj )){
			action = NA_OBJECT_ACTION( na_object_get_parent( it->data ));
		}

		relabel = nact_main_menubar_is_pasted_object_relabeled( obj, pivot );
		na_object_prepare_for_paste( obj, relabel, TRUE, action );
		na_object_set_origin( obj, NULL );
		na_object_check_status( obj );
		dup = g_list_prepend( NULL, obj );
		nact_iactions_list_bis_insert_items( NACT_IACTIONS_LIST( window ), dup, it->data );
		na_object_free_items_list( dup );
	}

	na_object_free_items_list( items );
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
	static const gchar *thisfn = "nact_main_menubar_on_delete_activated";
	GList *items;
	GList *it;

	g_debug( "%s: gtk_action=%p, window=%p", thisfn, ( void * ) gtk_action, ( void * ) window );
	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	items = nact_iactions_list_bis_get_selected_items( NACT_IACTIONS_LIST( window ));
	for( it = items ; it ; it = it->next ){
		g_debug( "%s: item=%p (%s)", thisfn, ( void * ) it->data, G_OBJECT_TYPE_NAME( it->data ));
	}
	nact_main_window_move_to_deleted( window, items );
	nact_iactions_list_bis_delete( NACT_IACTIONS_LIST( window ), items );

	/* do not unref selected items as the list has been concatenated
	 * to main_deleted
	 */
	/*g_list_free( items );*/
}

/*
 * as we are coming from cut or copy to clipboard, report selection
 * counters to clipboard ones
 */
static void
update_clipboard_counters( NactMainWindow *window )
{
	MenubarIndicatorsStruct *mis;

	mis = ( MenubarIndicatorsStruct * ) g_object_get_data( G_OBJECT( window ), MENUBAR_PROP_INDICATORS );

	mis->clipboard_menus = mis->selected_menus;
	mis->clipboard_actions = mis->selected_actions;
	mis->clipboard_profiles = mis->selected_profiles;

	g_debug( "nact_main_menubar_update_clipboard_counters: menus=%d, actions=%d, profiles=%d",
			mis->clipboard_menus, mis->clipboard_actions, mis->clipboard_profiles );

	g_signal_emit_by_name( window, MAIN_WINDOW_SIGNAL_UPDATE_ACTION_SENSITIVITIES, NULL );
}

static void
on_reload_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	nact_main_window_reload( window );
}

static void
on_preferences_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	nact_preferences_editor_run( BASE_WINDOW( window ));
}

static void
on_expand_all_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	nact_iactions_list_expand_all( NACT_IACTIONS_LIST( window ));
}

static void
on_collapse_all_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	nact_iactions_list_collapse_all( NACT_IACTIONS_LIST( window ));
}

static void
on_view_file_toolbar_activated( GtkToggleAction *action, NactMainWindow *window )
{
	/*on_view_toolbar_activated( action, window, MENUBAR_IPREFS_FILE_TOOLBAR, "/ui/FileToolbar", MENUBAR_FILE_TOOLBAR_POS );*/
	on_view_toolbar_activated( action, window, MAIN_TOOLBAR_FILE_ID );
}

static void
on_view_edit_toolbar_activated( GtkToggleAction *action, NactMainWindow *window )
{
	/*on_view_toolbar_activated( action, window, MENUBAR_IPREFS_EDIT_TOOLBAR, "/ui/EditToolbar", MENUBAR_EDIT_TOOLBAR_POS );*/
	on_view_toolbar_activated( action, window, MAIN_TOOLBAR_EDIT_ID );
}

static void
on_view_tools_toolbar_activated( GtkToggleAction *action, NactMainWindow *window )
{
	/*on_view_toolbar_activated( action, window, MENUBAR_IPREFS_TOOLS_TOOLBAR, "/ui/ToolsToolbar", MENUBAR_TOOLS_TOOLBAR_POS );*/
	on_view_toolbar_activated( action, window, MAIN_TOOLBAR_TOOLS_ID );
}

static void
on_view_help_toolbar_activated( GtkToggleAction *action, NactMainWindow *window )
{
	/*on_view_toolbar_activated( action, window, MENUBAR_IPREFS_HELP_TOOLBAR, "/ui/HelpToolbar", MENUBAR_HELP_TOOLBAR_POS );*/
	on_view_toolbar_activated( action, window, MAIN_TOOLBAR_HELP_ID );
}

static void
on_view_toolbar_activated( GtkToggleAction *action, NactMainWindow *window, int toolbar_id )
{
	gboolean is_active;
	GtkUIManager *ui_manager;

	is_active = gtk_toggle_action_get_active( action );
	ui_manager = ( GtkUIManager * ) g_object_get_data( G_OBJECT( window ), MENUBAR_PROP_UI_MANAGER );

	nact_main_toolbar_activate( window, toolbar_id, ui_manager, is_active );
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
on_dump_selection_activated( GtkAction *action, NactMainWindow *window )
{
	GList *items, *it;

	items = nact_iactions_list_bis_get_selected_items( NACT_IACTIONS_LIST( window ));
	for( it = items ; it ; it = it->next ){
		na_object_dump( it->data );
	}

	na_object_free_items_list( items );
}

static void
on_brief_tree_store_dump_activated( GtkAction *action, NactMainWindow *window )
{
	nact_iactions_list_brief_tree_dump( NACT_IACTIONS_LIST( window ));
}

static void
on_list_modified_items_activated( GtkAction *action, NactMainWindow *window )
{
	nact_iactions_list_bis_list_modified_items( NACT_IACTIONS_LIST( window ));
}

static void
on_dump_clipboard_activated( GtkAction *action, NactMainWindow *window )
{
	nact_clipboard_dump( nact_main_window_get_clipboard( window ));
}

static void
on_help_activated( GtkAction *gtk_action, NactMainWindow *window )
{
}

static void
on_about_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	na_iabout_display( NA_IABOUT( window ));
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
	static const gchar *thisfn = "nact_main_menubar_on_destroy_callback";

	g_debug( "%s: data=%p (%s)", thisfn,
			( void * ) data, G_OBJECT_TYPE_NAME( data ));

	g_object_unref( G_OBJECT( data ));
}

/*
 * gtk_activatable_get_related_action() and gtk_action_get_tooltip()
 * are only available starting with Gtk 2.16
 */
static void
on_menu_item_selected( GtkMenuItem *proxy, NactMainWindow *window )
{
	GtkAction *action;
	gchar *tooltip;

	/*g_debug( "nact_main_menubar_on_menu_item_selected: proxy=%p (%s), window=%p (%s)",
			( void * ) proxy, G_OBJECT_TYPE_NAME( proxy ),
			( void * ) window, G_OBJECT_TYPE_NAME( window ));*/

	tooltip = NULL;

#ifdef GTK_HAS_ACTIVATABLE
	action = gtk_activatable_get_related_action( GTK_ACTIVATABLE( proxy ));
	if( action ){
		tooltip = ( gchar * ) gtk_action_get_tooltip( action );
	}
#else
	action = GTK_ACTION( g_object_get_data( G_OBJECT( proxy ), MENUBAR_PROP_ITEM_ACTION ));
	if( action ){
		g_object_get( G_OBJECT( action ), "tooltip", &tooltip, NULL );
	}
#endif

	if( tooltip ){
		nact_main_statusbar_display_status( window, MENUBAR_PROP_STATUS_CONTEXT, tooltip );
	}

#ifndef GTK_HAS_ACTIVATABLE
	g_free( tooltip );
#endif
}

static void
on_menu_item_deselected( GtkMenuItem *proxy, NactMainWindow *window )
{
	nact_main_statusbar_hide_status( window, MENUBAR_PROP_STATUS_CONTEXT );
}

static void
on_popup_selection_done(GtkMenuShell *menushell, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_menubar_on_popup_selection_done";
	MenubarIndicatorsStruct *mis;

	g_debug( "%s", thisfn );

	mis = ( MenubarIndicatorsStruct * ) g_object_get_data( G_OBJECT( window ), MENUBAR_PROP_INDICATORS );
	g_signal_handler_disconnect( menushell, mis->popup_handler );
	mis->popup_handler = ( gulong ) 0;
}

static void
on_proxy_connect( GtkActionGroup *action_group, GtkAction *action, GtkWidget *proxy, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_menubar_on_proxy_connect";

	g_debug( "%s: action_group=%p (%s), action=%p (%s), proxy=%p (%s), window=%p (%s)",
			thisfn,
			( void * ) action_group, G_OBJECT_TYPE_NAME( action_group ),
			( void * ) action, G_OBJECT_TYPE_NAME( action ),
			( void * ) proxy, G_OBJECT_TYPE_NAME( proxy ),
			( void * ) window, G_OBJECT_TYPE_NAME( window ));

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

#ifndef GTK_HAS_ACTIVATABLE
		g_object_set_data( G_OBJECT( proxy ), MENUBAR_PROP_ITEM_ACTION, action );
#endif
	}
}

static void
on_proxy_disconnect( GtkActionGroup *action_group, GtkAction *action, GtkWidget *proxy, NactMainWindow *window )
{
	/* signal handlers will be automagically disconnected on BaseWindow::dispose */
}
