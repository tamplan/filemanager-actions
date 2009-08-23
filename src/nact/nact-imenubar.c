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
#include <common/na-ipivot-consumer.h>

#include "nact-application.h"
#include "nact-assistant-export.h"
#include "nact-assistant-import.h"
#include "nact-preferences-editor.h"
#include "nact-statusbar.h"
#include "nact-imenubar.h"

/* private interface data
 */
struct NactIMenubarInterfacePrivate {
};

#define PROP_IMENUBAR_STATUS_CONTEXT	"nact-imenubar-status-context"

static GType      register_type( void );
static void       interface_base_init( NactIMenubarInterface *klass );
static void       interface_base_finalize( NactIMenubarInterface *klass );

static void       on_file_menu_selected( GtkMenuItem *item, NactWindow *window );
static void       on_new_action_activated( GtkMenuItem *item, NactWindow *window );
static void       on_new_profile_activated( GtkMenuItem *item, NactWindow *window );
static void       on_save_activated( GtkMenuItem *item, NactWindow *window );
static void       on_quit_activated( GtkMenuItem *item, NactWindow *window );
static void       add_action( NactWindow *window, NAAction *action );
static void       add_profile( NactWindow *window, NAAction *action, NAActionProfile *profile );

static void       on_edit_menu_selected( GtkMenuItem *item, NactWindow *window );
static void       on_duplicate_activated( GtkMenuItem *item, NactWindow *window );
static void       on_delete_activated( GtkMenuItem *item, NactWindow *window );
static void       on_reload_activated( GtkMenuItem *item, NactWindow *window );
static void       on_preferences_activated( GtkMenuItem *item, NactWindow *window );

static void       on_tools_menu_selected( GtkMenuItem *item, NactWindow *window );
static void       on_import_activated( GtkMenuItem *item, NactWindow *window );
static void       on_export_activated( GtkMenuItem *item, NactWindow *window );

static void       on_help_menu_selected( GtkMenuItem *item, NactWindow *window );
static void       on_help_activated( GtkMenuItem *item, NactWindow *window );
static void       on_about_activated( GtkMenuItem *item, NactWindow *window );

static void       on_menu_item_selected( GtkItem *item, NactWindow *window );
static void       on_menu_item_deselected( GtkItem *item, NactWindow *window );

static void       v_add_action( NactWindow *window, NAAction *action );
static void       v_add_profile( NactWindow *window, NAActionProfile *profile );
static void       v_remove_action( NactWindow *window, NAAction *action );
static GSList    *v_get_deleted_actions( NactWindow *window );
static void       v_free_deleted_actions( NactWindow *window );
static void       v_push_removed_action( NactWindow *window, NAAction *action );
static GSList    *v_get_actions( NactWindow *window );
static NAObject  *v_get_selected( NactWindow *window );
static void       v_setup_dialog_title( NactWindow *window );
static void       v_update_actions_list( NactWindow *window );
static void       v_select_actions_list( NactWindow *window, GType type, const gchar *uuid, const gchar *label );
static gint       v_count_actions( NactWindow *window );
static gint       v_count_modified_actions( NactWindow *window );
static void       v_reload_actions( NactWindow *window );

static const GtkActionEntry entries[] = {

		{ "FileMenu", NULL, N_( "_File" ) },
		{ "EditMenu", NULL, N_( "_Edit" ) },
		{ "ToolsMenu", NULL, N_( "_Tools" ) },
		{ "HelpMenu", NULL, N_( "_Help" ) },

		{ "NewActionItem", GTK_STOCK_NEW, N_( "_New action" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the 'New action' item */
				N_( "Define a new action." ),
				G_CALLBACK( on_new_action_activated ) },
		{ "NewProfileItem", NULL, N_( "_New _profile" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the 'New profile' item */
				N_( "Define a new profile attached to the current action." ),
				G_CALLBACK( on_new_profile_activated ) },
		{ "SaveItem", GTK_STOCK_SAVE, NULL, NULL,
				/* i18n: tooltip displayed in the status bar when selecting 'Save' item */
				N_( "Record all the modified actions. Invalid actions will be silently ignored." ),
				G_CALLBACK( on_save_activated ) },
		{ "QuitItem", GTK_STOCK_QUIT, NULL, NULL,
				/* i18n: tooltip displayed in the status bar when selecting 'Quit' item */
				N_( "Quit the application." ),
				G_CALLBACK( on_quit_activated ) },
		{ "DuplicateItem" , GTK_STOCK_COPY, N_( "D_uplicate" ), "",
				/* i18n: tooltip displayed in the status bar when selecting the Duplicate item */
				N_( "Create a copy of the selected action or profile." ),
				G_CALLBACK( on_duplicate_activated ) },
		{ "DeleteItem", GTK_STOCK_DELETE, NULL, "Del",
				/* i18n: tooltip displayed in the status bar when selecting the Delete item */
				N_( "Remove the selected action or profile from your configuration." ),
				G_CALLBACK( on_delete_activated ) },
		{ "ReloadActionsItem", NULL, N_( "_Reload the list of actions" ), "<Ctrl>R",
				/* i18n: tooltip displayed in the status bar when selecting the 'Reload' item */
				N_( "Cancel your current modifications and reload the list of actions." ),
				G_CALLBACK( on_reload_activated ) },
		{ "PreferencesItem", GTK_STOCK_PREFERENCES, NULL, "<Ctrl>P",
				/* i18n: tooltip displayed in the status bar when selecting the 'Preferences' item */
				N_( "Edit your preferences." ),
				G_CALLBACK( on_preferences_activated ) },
		{ "ImportItem" , GTK_STOCK_CONVERT, N_( "_Import assistant..." ), "",
				/* i18n: tooltip displayed in the status bar when selecting the Import item */
				N_( "Import one or more actions from external (XML) files into your configuration." ),
				G_CALLBACK( on_import_activated ) },
		{ "ExportItem", NULL, N_( "E_xport assistant..." ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the Export item */
				N_( "Export one or more actions from your configuration to external XML files." ),
				G_CALLBACK( on_export_activated ) },
		{ "HelpItem" , GTK_STOCK_HELP, NULL, NULL,
				/* i18n: tooltip displayed in the status bar when selecting the Help item */
				N_( "Display help about this program." ),
				G_CALLBACK( on_help_activated ) },
		{ "AboutItem", GTK_STOCK_ABOUT, NULL, NULL,
				/* i18n: tooltip displayed in the status bar when selecting the About item */
				N_( "Display informations about this program." ),
				G_CALLBACK( on_about_activated ) },
};

/* list of menus and on_selected corresponding functions
 */
typedef struct {
	char     *menu;
	GCallback on_selected;
}
	MenuOnSelectedStruct;

static const MenuOnSelectedStruct menu_callback[] = {
		{ "FileMenu" , G_CALLBACK( on_file_menu_selected ) },
		{ "EditMenu" , G_CALLBACK( on_edit_menu_selected ) },
		{ "ToolsMenu", G_CALLBACK( on_tools_menu_selected ) },
		{ "HelpMenu" , G_CALLBACK( on_help_menu_selected ) },
};

/* associates action with menu to be able to build their path
 */
typedef struct {
	char *menu;
	char *action;
}
	MenuActionStruct;

static const MenuActionStruct menu_actions[] = {
		{ "FileMenu", "NewActionItem" },
		{ "FileMenu", "NewProfileItem" },
		{ "FileMenu", "SaveItem" },
		{ "FileMenu", "QuitItem" },
		{ "EditMenu", "DuplicateItem" },
		{ "EditMenu", "DeleteItem" },
		{ "EditMenu", "ReloadActionsItem" },
		{ "EditMenu", "PreferencesItem" },
		{ "ToolsMenu", "ImportItem" },
		{ "ToolsMenu", "ExportItem" },
		{ "HelpMenu", "HelpItem" },
		{ "HelpMenu", "AboutItem" },
};

GType
nact_imenubar_get_type( void )
{
	static GType iface_type = 0;

	if( !iface_type ){
		iface_type = register_type();
	}

	return( iface_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "nact_imenubar_register_type";
	g_debug( "%s", thisfn );

	static const GTypeInfo info = {
		sizeof( NactIMenubarInterface ),
		( GBaseInitFunc ) interface_base_init,
		( GBaseFinalizeFunc ) interface_base_finalize,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL
	};

	GType type = g_type_register_static( G_TYPE_INTERFACE, "NactIMenubar", &info, 0 );

	g_type_interface_add_prerequisite( type, NACT_WINDOW_TYPE );

	return( type );
}

static void
interface_base_init( NactIMenubarInterface *klass )
{
	static const gchar *thisfn = "nact_imenubar_interface_base_init";
	static gboolean initialized = FALSE;

	if( !initialized ){
		g_debug( "%s: klass=%p", thisfn, klass );

		klass->private = g_new0( NactIMenubarInterfacePrivate, 1 );

		initialized = TRUE;
	}
}

static void
interface_base_finalize( NactIMenubarInterface *klass )
{
	static const gchar *thisfn = "nact_imenubar_interface_base_finalize";
	static gboolean finalized = FALSE ;

	if( !finalized ){
		g_debug( "%s: klass=%p", thisfn, klass );

		g_free( klass->private );

		finalized = TRUE;
	}
}

/**
 * nact_imenubar_init:
 * @window: the #NactMainWindow to which the menubar is attached.
 *
 * Creates the menubar.
 */
void
nact_imenubar_init( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_imenubar_init";

	g_assert( NACT_IS_MAIN_WINDOW( window ));
	g_assert( NACT_IS_IMENUBAR( window ));

	/* create the menubar:
	 * - create action group, and insert list of actions in it
	 * - create ui_manager, and insert action group in it
	 * - merge inserted actions group with ui xml definition
	 * - install accelerators in the main window
	 * - pack menu bar in the main window
	 */
	GtkActionGroup *action_group = gtk_action_group_new( "MenubarActions" );
	gtk_action_group_set_translation_domain( action_group, GETTEXT_PACKAGE );
	gtk_action_group_add_actions( action_group, entries, G_N_ELEMENTS( entries ), window );

	GtkUIManager *ui_manager = gtk_ui_manager_new();
	gtk_ui_manager_insert_action_group( ui_manager, action_group, 0 );
	g_object_unref( action_group );

	GError *error = NULL;
	guint merge_id = gtk_ui_manager_add_ui_from_file( ui_manager, PKGDATADIR "/nautilus-actions-config-tool.actions", &error );
	if( merge_id == 0 || error ){
		g_warning( "%s: error=%s", thisfn, error->message );
		g_error_free( error );
	}

	GtkWindow *wnd = base_window_get_toplevel_dialog( BASE_WINDOW( window ));
	GtkAccelGroup *accel_group = gtk_ui_manager_get_accel_group( ui_manager );
	gtk_window_add_accel_group( wnd, accel_group );

	GtkWidget *menubar = gtk_ui_manager_get_widget( ui_manager, "/ui/MainMenubar" );
	GtkWidget *vbox = base_window_get_widget( BASE_WINDOW( window ), "MenubarVBox" );
	gtk_box_pack_start( GTK_BOX( vbox ), menubar, FALSE, FALSE, 0 );

	g_object_set_data( G_OBJECT( window ), "nact-imenubar-ui-manager", ui_manager );

	/* menu and accelerators are ok
	 * but tooltip are no more displayed
	 */
	int i;
	for( i = 0 ; i < G_N_ELEMENTS( menu_actions ) ; ++ i ){
		gchar *path = g_build_path( "/", "ui", "MainMenubar", menu_actions[i].menu, menu_actions[i].action, NULL );
		GtkAction *action = gtk_ui_manager_get_action( ui_manager, path );
		GtkWidget *widget = gtk_ui_manager_get_widget( ui_manager, path );
		g_object_set_data( G_OBJECT( widget ), "nact-imenubar-action", action );
		nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( widget ), "select", G_CALLBACK( on_menu_item_selected ));
		nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( widget ), "deselect", G_CALLBACK( on_menu_item_deselected ));
		g_free( path );
	}

	/* we don't use property synchronization to update the items
	 * instead we update items just before opening the menu
	 */
	for( i = 0 ; i < G_N_ELEMENTS( menu_callback ) ; ++ i ){
		gchar *path = g_build_path( "/", "ui", "MainMenubar", menu_callback[i].menu, NULL );
		GtkWidget *widget = gtk_ui_manager_get_widget( ui_manager, path );
		nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( widget ), "select", menu_callback[i].on_selected );
		g_free( path );
	}
}

/**
 * nact_imenubar_on_delete_key_pressed:
 * @window: the #NactMainWindow to which the menubar is attached.
 *
 * Deletes the currently selected item.
 */
void
nact_imenubar_on_delete_key_pressed( NactWindow *window )
{
	static const gchar *thisfn = "nact_imenubar_on_delete_key_pressed";
	g_debug( "%s: window=%p", thisfn, window );

	g_assert( NACT_IS_MAIN_WINDOW( window ));
	g_assert( NACT_IS_IMENUBAR( window ));

	on_delete_activated( NULL, window );
}

/**
 * nact_imenubar_on_delete_event:
 * @window: the #NactMainWindow to which the menubar is attached.
 *
 * Closes the main window via the standard 'Quit' item of the menubar.
 *
 * This functions is triggered by the main window when it receives the
 * 'delete-event' event.
 */
void
nact_imenubar_on_delete_event( NactWindow *window )
{
	static const gchar *thisfn = "nact_imenubar_on_delete_event";
	g_debug( "%s: window=%p", thisfn, window );

	g_assert( NACT_IS_MAIN_WINDOW( window ));
	g_assert( NACT_IS_IMENUBAR( window ));

	on_quit_activated( NULL, window );
}

static void
on_file_menu_selected( GtkMenuItem *item, NactWindow *window )
{
	GtkUIManager *ui_manager = GTK_UI_MANAGER( g_object_get_data( G_OBJECT( window ), "nact-imenubar-ui-manager" ));

	NAObject *object = v_get_selected( window );
	gboolean new_profile_enabled = NA_IS_ACTION( object ) || NA_IS_ACTION_PROFILE( object );
	GtkAction *new_profile_action = gtk_ui_manager_get_action( ui_manager, "/ui/MainMenubar/FileMenu/NewProfileItem" );
	gtk_action_set_sensitive( new_profile_action, new_profile_enabled );

	gint modified = v_count_modified_actions( window );
	gboolean save_enabled = ( modified > 0 );
	GtkAction *save_action = gtk_ui_manager_get_action( ui_manager, "/ui/MainMenubar/FileMenu/SaveItem" );
	gtk_action_set_sensitive( save_action, save_enabled );
}

static void
on_new_action_activated( GtkMenuItem *item, NactWindow *window )
{
	NAAction *action = na_action_new_with_profile();
	add_action( window, action );
}

static void
on_new_profile_activated( GtkMenuItem *item, NactWindow *window )
{
	NAObject *object = v_get_selected( window );
	g_assert( NA_IS_OBJECT( object ));

	NAAction *action = NA_IS_ACTION( object ) ?
			NA_ACTION( object ) : na_action_profile_get_action( NA_ACTION_PROFILE( object ));

	NAActionProfile *new_profile = na_action_profile_new();
	g_debug( "nact_imenubar_on_new_profile_activated: action=%p, profile=%p", action, new_profile );

	gchar *name = na_action_get_new_profile_name( action );
	na_action_profile_set_name( new_profile, name );
	g_free( name );

	add_profile( window, action, new_profile );
}

static void
on_save_activated( GtkMenuItem *item, NactWindow *window )
{
	static const gchar *thisfn = "nact_imenubar_on_save_activated";
	g_debug( "%s: item=%p, window=%p", thisfn, item, window );

	NactApplication *application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	NAPivot *pivot = nact_application_get_pivot( application );
	GSList *actions = v_get_actions( window );
	g_debug( "%s: %d actions", thisfn, g_slist_length( actions ));
	GSList *ia;

	/* keep the current selection
	 * to be able to restore it at the end of the operation
	 */
	GType type = 0;
	gchar *uuid = NULL;
	gchar *label = NULL;

	NAObject *current = v_get_selected( window );
	if( current ){
		if( NA_IS_ACTION( current )){
			uuid = na_action_get_uuid( NA_ACTION( current ));
			label = na_action_get_label( NA_ACTION( current ));
			type = NA_ACTION_TYPE;

		} else {
			g_assert( NA_IS_ACTION_PROFILE( current ));
			NAAction *action = na_action_profile_get_action( NA_ACTION_PROFILE( current ));
			uuid = na_action_get_uuid( action );
			label = na_action_profile_get_label( NA_ACTION_PROFILE( current ));
			type = NA_ACTION_PROFILE_TYPE;
		}
	}

	/* save the valid modified actions
	 * - add a duplicate of this action to pivot or update the origin
	 * - set the duplicate as the origin of this action
	 * - reset the status
	 */
	for( ia = actions ; ia ; ia = ia->next ){

		NAAction *current = NA_ACTION( ia->data );

		if( na_object_get_modified_status( NA_OBJECT( current )) &&
			na_object_get_valid_status( NA_OBJECT( current )) &&
			nact_window_save_action( window, current )){

				NAAction *origin = NA_ACTION( na_object_get_origin( NA_OBJECT( current )));

				if( origin ){
					na_object_copy( NA_OBJECT( origin ), NA_OBJECT( current ));

				} else {
					NAAction *dup_pivot = NA_ACTION( na_object_duplicate( NA_OBJECT( current )));
					na_object_set_origin( NA_OBJECT( dup_pivot ), NULL );
					na_object_set_origin( NA_OBJECT( current ), NA_OBJECT( dup_pivot ));
					na_pivot_add_action( pivot, dup_pivot );
				}

				na_object_check_edited_status( NA_OBJECT( current ));
		}
	}

	/* delete the removed actions
	 * - remove the origin of pivot if any
	 */
	GSList *deleted = v_get_deleted_actions( window );
	for( ia = deleted ; ia ; ia = ia->next ){
		NAAction *current = NA_ACTION( ia->data );
		if( nact_window_delete_action( window, current )){

			NAAction *origin = NA_ACTION( na_object_get_origin( NA_OBJECT( current )));
			if( origin ){
				na_pivot_remove_action( pivot, origin );
			}
		}
	}
	v_free_deleted_actions( window );

	v_setup_dialog_title( window );

	if( current ){
		v_update_actions_list( window );
		v_select_actions_list( window, type, uuid, label );
		g_free( label );
		g_free( uuid );
	}

	na_ipivot_consumer_delay_notify( NA_IPIVOT_CONSUMER( window ));
}

static void
on_quit_activated( GtkMenuItem *item, NactWindow *window )
{
	static const gchar *thisfn = "nact_imenubar_on_quit_activated";
	g_debug( "%s: item=%p, window=%p", thisfn, item, window );

	gint count = v_count_modified_actions( NACT_WINDOW( window ));
	if( !count || nact_window_warn_count_modified( NACT_WINDOW( window ), count )){
		g_object_unref( window );
	}
}

static void
add_action( NactWindow *window, NAAction *action )
{
	na_object_check_edited_status( NA_OBJECT( action ));
	v_add_action( window, action );

	v_update_actions_list( window );

	gchar *uuid = na_action_get_uuid( action );
	gchar *label = na_action_get_label( action );
	v_select_actions_list( window, NA_ACTION_TYPE, uuid, label );
	g_free( label );
	g_free( uuid );

	v_setup_dialog_title( window );
}

static void
add_profile( NactWindow *window, NAAction *action, NAActionProfile *profile )
{
	na_action_attach_profile( action, profile );
	na_object_check_edited_status( NA_OBJECT( profile ));

	v_add_profile( window, profile );

	v_update_actions_list( window );

	gchar *uuid = na_action_get_uuid( action );
	gchar *label = na_action_profile_get_label( profile );
	v_select_actions_list( window, NA_ACTION_PROFILE_TYPE, uuid, label );
	g_free( label );
	g_free( uuid );

	v_setup_dialog_title( window );
}

static void
on_edit_menu_selected( GtkMenuItem *item, NactWindow *window )
{
	GtkUIManager *ui_manager = GTK_UI_MANAGER( g_object_get_data( G_OBJECT( window ), "nact-imenubar-ui-manager" ));

	NAObject *object = v_get_selected( window );
	gboolean delete_enabled = FALSE;
	if( object ){
		if( NA_IS_ACTION( object )){
			delete_enabled = TRUE;
		} else {
			g_assert( NA_IS_ACTION_PROFILE( object ));
			NAAction *action = na_action_profile_get_action( NA_ACTION_PROFILE( object ));
			delete_enabled = ( na_action_get_profiles_count( action ) > 1 );
		}
	}

	gboolean duplicate_enabled = delete_enabled;

	GtkAction *delete_action = gtk_ui_manager_get_action( ui_manager, "/ui/MainMenubar/EditMenu/DeleteItem" );
	gtk_action_set_sensitive( delete_action, delete_enabled );

	GtkAction *duplicate_action = gtk_ui_manager_get_action( ui_manager, "/ui/MainMenubar/EditMenu/DuplicateItem" );
	gtk_action_set_sensitive( duplicate_action, duplicate_enabled );
}

static void
on_duplicate_activated( GtkMenuItem *item, NactWindow *window )
{
	static const gchar *thisfn = "nact_imenubar_on_duplicate_activated";
	g_debug( "%s: item=%p, window=%p", thisfn, item, window );

	NAObject *object = v_get_selected( window );

	NAObject *dup = na_object_duplicate( object );

	if( NA_IS_ACTION( object )){

		gchar *label = na_action_get_label( NA_ACTION( object ));
		/* i18n: label of a duplicated action */
		gchar *dup_label = g_strdup_printf( _( "Copy of %s" ), label );
		na_action_set_label( NA_ACTION( dup ), dup_label );
		na_action_set_new_uuid( NA_ACTION( dup ));
		g_free( dup_label );
		g_free( label );

		add_action( window, NA_ACTION( dup ));

	} else {
		g_assert( NA_IS_ACTION_PROFILE( object ));
		NAAction *action = na_action_profile_get_action( NA_ACTION_PROFILE( object ));

		gchar *label = na_action_profile_get_label( NA_ACTION_PROFILE( object ));
		/* i18n: label of a duplicated profile */
		gchar *dup_label = g_strdup_printf( _( "Copy of %s" ), label );
		na_action_profile_set_label( NA_ACTION_PROFILE( dup ), dup_label );
		g_free( dup_label );
		g_free( label );

		add_profile( window, action, NA_ACTION_PROFILE( dup ));
	}
}

static void
on_delete_activated( GtkMenuItem *item, NactWindow *window )
{
	static const gchar *thisfn = "nact_imenubar_on_delete_activated";
	g_debug( "%s: item=%p, window=%p", thisfn, item, window );

	NAObject *object = v_get_selected( window );
	gchar *uuid, *label;
	GType type;

	if( NA_IS_ACTION( object )){
		uuid = na_action_get_uuid( NA_ACTION( object ));
		label = na_action_get_label( NA_ACTION( object ));
		type = NA_ACTION_TYPE;
		v_remove_action( window, NA_ACTION( object ));
		v_push_removed_action( window, NA_ACTION( object ));

	} else {
		g_assert( NA_IS_ACTION_PROFILE( object ));
		NAAction *action = na_action_profile_get_action( NA_ACTION_PROFILE( object ));
		uuid = na_action_get_uuid( action );
		label = na_action_profile_get_label( NA_ACTION_PROFILE( object ));
		type = NA_ACTION_PROFILE_TYPE;
		g_assert( na_action_get_profiles_count( action ) > 1 );
		na_action_remove_profile( action, NA_ACTION_PROFILE( object ));
		na_object_check_edited_status( NA_OBJECT( action ));
	}

	v_update_actions_list( window );

	v_select_actions_list( window, type, uuid, label );
	g_free( label );
	g_free( uuid );

	v_setup_dialog_title( window );
}

static void
on_reload_activated( GtkMenuItem *item, NactWindow *window )
{
	static const gchar *thisfn = "nact_imenubar_on_reload_activated";
	g_debug( "%s: item=%p, window=%p", thisfn, item, window );

	gboolean ok = TRUE;

	if( v_count_modified_actions( window )){

		/* i18n: message before reloading the list of actions */
		gchar *first = g_strdup( _( "This will cancel your current modifications." ));
		gchar *second = g_strdup( _( "Are you sure this is what you want ?" ));

		ok = base_window_yesno_dlg( BASE_WINDOW( window ), GTK_MESSAGE_QUESTION, first, second );

		g_free( second );
		g_free( first );
	}

	if( ok ){
		v_reload_actions( window );
	}
}

static void
on_preferences_activated( GtkMenuItem *item, NactWindow *window )
{
	nact_preferences_editor_run( window );
}

static void
on_tools_menu_selected( GtkMenuItem *item, NactWindow *window )
{
	GtkUIManager *ui_manager = GTK_UI_MANAGER( g_object_get_data( G_OBJECT( window ), "nact-imenubar-ui-manager" ));

	gboolean export_enabled = v_count_actions( window );
	GtkAction *export_action = gtk_ui_manager_get_action( ui_manager, "/ui/MainMenubar/ToolsMenu/ExportItem" );
	gtk_action_set_sensitive( export_action, export_enabled );
}

static void
on_import_activated( GtkMenuItem *item, NactWindow *window )
{
	GSList *list = nact_assistant_import_run( window );
	GSList *ia;
	for( ia = list ; ia ; ia = ia->next ){
		add_action( window, NA_ACTION( ia->data ));
	}
}

static void
on_export_activated( GtkMenuItem *item, NactWindow *window )
{
	static const gchar *thisfn = "nact_imenubar_on_export_activated";
	g_debug( "%s: item=%p, window=%p", thisfn, item, window );

	nact_assistant_export_run( NACT_WINDOW( window ));
}

static void
on_help_menu_selected( GtkMenuItem *item, NactWindow *window )
{
	GtkUIManager *ui_manager = GTK_UI_MANAGER( g_object_get_data( G_OBJECT( window ), "nact-imenubar-ui-manager" ));

	gboolean help_enabled = FALSE;
	GtkAction *help_action = gtk_ui_manager_get_action( ui_manager, "/ui/MainMenubar/HelpMenu/HelpItem" );
	gtk_action_set_sensitive( help_action, help_enabled );
}

static void
on_help_activated( GtkMenuItem *item, NactWindow *window )
{
}

/* TODO: make the website url and the mail addresses clickables
 */
static void
on_about_activated( GtkMenuItem *item, NactWindow *window )
{
	static const gchar *thisfn = "nact_imenubar_on_about_activated";
	g_debug( "%s: item=%p, window=%p", thisfn, item, window );

	GtkWindow *toplevel = base_window_get_toplevel_dialog( BASE_WINDOW( window ));

	na_about_display( G_OBJECT( toplevel ));
}

static void
on_menu_item_selected( GtkItem *item, NactWindow *window )
{
	GtkAction *action = GTK_ACTION( g_object_get_data( G_OBJECT( item ), "nact-imenubar-action" ));
	const gchar *tooltip = gtk_action_get_tooltip( action );
	nact_statusbar_display_status( NACT_MAIN_WINDOW( window ), PROP_IMENUBAR_STATUS_CONTEXT, tooltip );
}

static void
on_menu_item_deselected( GtkItem *item, NactWindow *window )
{
	nact_statusbar_hide_status( NACT_MAIN_WINDOW( window ), PROP_IMENUBAR_STATUS_CONTEXT );
}

static void
v_add_action( NactWindow *window, NAAction *action )
{
	if( NACT_IMENUBAR_GET_INTERFACE( window )->add_action ){
		return( NACT_IMENUBAR_GET_INTERFACE( window )->add_action( window, action ));
	}
}

static void
v_add_profile( NactWindow *window, NAActionProfile *profile )
{
	if( NACT_IMENUBAR_GET_INTERFACE( window )->add_profile ){
		return( NACT_IMENUBAR_GET_INTERFACE( window )->add_profile( window, profile ));
	}
}

static void
v_remove_action( NactWindow *window, NAAction *action )
{
	if( NACT_IMENUBAR_GET_INTERFACE( window )->remove_action ){
		return( NACT_IMENUBAR_GET_INTERFACE( window )->remove_action( window, action ));
	}
}

static GSList *
v_get_deleted_actions( NactWindow *window )
{
	if( NACT_IMENUBAR_GET_INTERFACE( window )->get_deleted_actions ){
		return( NACT_IMENUBAR_GET_INTERFACE( window )->get_deleted_actions( window ));
	}

	return( NULL );
}

static void
v_free_deleted_actions( NactWindow *window )
{
	if( NACT_IMENUBAR_GET_INTERFACE( window )->free_deleted_actions ){
		NACT_IMENUBAR_GET_INTERFACE( window )->free_deleted_actions( window );
	}
}

static void
v_push_removed_action( NactWindow *window, NAAction *action )
{
	if( NACT_IMENUBAR_GET_INTERFACE( window )->push_removed_action ){
		return( NACT_IMENUBAR_GET_INTERFACE( window )->push_removed_action( window, action ));
	}
}

static GSList *
v_get_actions( NactWindow *window )
{
	if( NACT_IMENUBAR_GET_INTERFACE( window )->get_actions ){
		return( NACT_IMENUBAR_GET_INTERFACE( window )->get_actions( window ));
	}

	return( NULL );
}

static NAObject *
v_get_selected( NactWindow *window )
{
	if( NACT_IMENUBAR_GET_INTERFACE( window )->get_selected ){
		return( NACT_IMENUBAR_GET_INTERFACE( window )->get_selected( window ));
	}

	return( NULL );
}

static void
v_setup_dialog_title( NactWindow *window )
{
	if( NACT_IMENUBAR_GET_INTERFACE( window )->setup_dialog_title ){
		return( NACT_IMENUBAR_GET_INTERFACE( window )->setup_dialog_title( window ));
	}
}

static void
v_update_actions_list( NactWindow *window )
{
	if( NACT_IMENUBAR_GET_INTERFACE( window )->update_actions_list ){
		return( NACT_IMENUBAR_GET_INTERFACE( window )->update_actions_list( window ));
	}
}

static void
v_select_actions_list( NactWindow *window, GType type, const gchar *uuid, const gchar *label )
{
	if( NACT_IMENUBAR_GET_INTERFACE( window )->select_actions_list ){
		return( NACT_IMENUBAR_GET_INTERFACE( window )->select_actions_list( window, type, uuid, label ));
	}
}

static gint
v_count_actions( NactWindow *window )
{
	if( NACT_IMENUBAR_GET_INTERFACE( window )->count_actions ){
		return( NACT_IMENUBAR_GET_INTERFACE( window )->count_actions( window ));
	}

	return( 0 );
}

static gint
v_count_modified_actions( NactWindow *window )
{
	if( NACT_IMENUBAR_GET_INTERFACE( window )->count_modified_actions ){
		return( NACT_IMENUBAR_GET_INTERFACE( window )->count_modified_actions( window ));
	}

	return( 0 );
}

static void
v_reload_actions( NactWindow *window )
{
	if( NACT_IMENUBAR_GET_INTERFACE( window )->reload_actions ){
		NACT_IMENUBAR_GET_INTERFACE( window )->reload_actions( window );
	}
}
