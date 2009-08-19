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

#include "nact-application.h"
#include "nact-assistant-export.h"
#include "nact-assistant-import.h"
#include "nact-statusbar.h"
#include "nact-imenubar.h"

/* private interface data
 */
struct NactIMenubarInterfacePrivate {
};

/* menubar properties, set on the main window
 */
#define PROP_IMENUBAR_STATUS_CONTEXT	"nact-imenubar-status-context"
#define PROP_IMENUBAR_DELETED_ACTIONS	"nact-imenubar-deleted-actions"
#define PROP_IMENUBAR_NEW_PROFILE_ITEM	"nact-imenubar-new-profile-item"
#define PROP_IMENUBAR_SAVE_ITEM			"nact-imenubar-save-item"
#define PROP_IMENUBAR_DUPLICATE_ITEM	"nact-imenubar-duplicate-item"
#define PROP_IMENUBAR_DELETE_ITEM		"nact-imenubar-delete-item"
#define PROP_IMENUBAR_EXPORT_ITEM		"nact-imenubar-export-item"

static GType      register_type( void );
static void       interface_base_init( NactIMenubarInterface *klass );
static void       interface_base_finalize( NactIMenubarInterface *klass );

static void       create_file_menu( NactMainWindow *window, GtkMenuBar *bar );
static void       create_edit_menu( NactMainWindow *window, GtkMenuBar *bar );
static void       create_tools_menu( NactMainWindow *window, GtkMenuBar *bar );
static void       create_help_menu( NactMainWindow *window, GtkMenuBar *bar );
static void       signal_connect( NactMainWindow *window, GtkWidget *item, GCallback on_activated, GCallback on_selected );

static void       on_file_selected( GtkMenuItem *item, NactWindow *window );
static void       on_new_action_activated( GtkMenuItem *item, NactWindow *window );
static void       on_new_action_selected( GtkItem *item, NactWindow *window );
static void       on_new_profile_activated( GtkMenuItem *item, NactWindow *window );
static void       on_new_profile_selected( GtkItem *item, NactWindow *window );
static void       on_save_activated( GtkMenuItem *item, NactWindow *window );
static void       on_save_selected( GtkMenuItem *item, NactWindow *window );
static void       on_quit_activated( GtkMenuItem *item, NactWindow *window );
static void       on_quit_selected( GtkMenuItem *item, NactWindow *window );
static void       add_action( NactWindow *window, NAAction *action );
static void       add_profile( NactWindow *window, NAAction *action, NAActionProfile *profile );

static void       on_edit_selected( GtkMenuItem *item, NactWindow *window );
static void       on_duplicate_activated( GtkMenuItem *item, NactWindow *window );
static void       on_duplicate_selected( GtkItem *item, NactWindow *window );
static void       on_delete_activated( GtkMenuItem *item, NactWindow *window );
static void       on_delete_selected( GtkItem *item, NactWindow *window );
static void       on_reload_activated( GtkMenuItem *item, NactWindow *window );
static void       on_reload_selected( GtkItem *item, NactWindow *window );

static void       on_tools_selected( GtkMenuItem *item, NactWindow *window );
static void       on_import_activated( GtkMenuItem *item, NactWindow *window );
static void       on_import_selected( GtkItem *item, NactWindow *window );
static void       on_export_activated( GtkMenuItem *item, NactWindow *window );
static void       on_export_selected( GtkItem *item, NactWindow *window );

static void       on_help_activated( GtkMenuItem *item, NactWindow *window );
static void       on_help_selected( GtkItem *item, NactWindow *window );
static void       on_about_activated( GtkMenuItem *item, NactWindow *window );
static void       on_about_selected( GtkItem *item, NactWindow *window );

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
static void       v_on_save( NactWindow *window );

static GtkWidget *get_new_profile_item( NactWindow *window );
static void       set_new_profile_item( NactWindow *window, GtkWidget *item );
static GtkWidget *get_save_item( NactWindow *window );
static void       set_save_item( NactWindow *window, GtkWidget *item );
static GtkWidget *get_duplicate_item( NactWindow *window );
static void       set_duplicate_item( NactWindow *window, GtkWidget *item );
static GtkWidget *get_delete_item( NactWindow *window );
static void       set_delete_item( NactWindow *window, GtkWidget *item );
static GtkWidget *get_export_item( NactWindow *window );
static void       set_export_item( NactWindow *window, GtkWidget *item );

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
	g_assert( NACT_IS_MAIN_WINDOW( window ));
	g_assert( NACT_IS_IMENUBAR( window ));

	/*static const gchar *menubar[] =
			"<ui>"
			"    <menubar>"
			"        <menu action=\"FileMenu\">"
			"            <menuitem action=\"NewActionItem\" />"
			"            <menuitem action=\"NewProfileItem\" />"
			"            <menuitem action=\"SaveItem\" />"
			"            <menuitem action=\"QuitItem\" />"
			"        </menu>"
			"        <menu action=\"EditMenu\">"
			"            <menuitem action=\"DuplicateItem\" />"
			"            <menuitem action=\"DeleteItem\" />"
			"            <menuitem action=\"ReloadActionsItem\" />"
			"        </menu>"
			"        <menu action=\"ToolsMenu\">"
			"            <menuitem action=\"ImportItem\" />"
			"            <menuitem action=\"ExportItem\" />"
			"        </menu>"
			"        <menu action=\"HelpMenu\">"
			"            <menuitem action=\"HelpItem\" />"
			"            <menuitem action=\"AboutItem\" />"
			"        </menu>"
			"    </menubar>"
			"</ui>";*/

	GtkWidget *vbox = base_window_get_widget( BASE_WINDOW( window ), "MenuBarVBox" );
	GtkWidget *menubar= gtk_menu_bar_new();
	gtk_container_add( GTK_CONTAINER( vbox ), menubar );

	create_file_menu( window, GTK_MENU_BAR( menubar ));
	create_edit_menu( window, GTK_MENU_BAR( menubar ));
	create_tools_menu( window, GTK_MENU_BAR( menubar ));
	create_help_menu( window, GTK_MENU_BAR( menubar ));
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
create_file_menu( NactMainWindow *window, GtkMenuBar *menubar )
{
	/* i18n: File menu */
	GtkWidget *file = gtk_menu_item_new_with_mnemonic( _( "_File" ));
	gtk_menu_shell_append( GTK_MENU_SHELL( menubar ), file );
	GtkWidget *menu = gtk_menu_new();
	gtk_menu_item_set_submenu( GTK_MENU_ITEM( file ), menu );
	nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( file ), "select", G_CALLBACK( on_file_selected ));

	/* i18n: 'New action' item in 'File' menu */
	GtkWidget *item = gtk_image_menu_item_new_with_mnemonic( _( "_New action" ));
	gtk_image_menu_item_set_image( GTK_IMAGE_MENU_ITEM( item ), gtk_image_new_from_stock( GTK_STOCK_NEW, GTK_ICON_SIZE_MENU ));
	gtk_menu_shell_append( GTK_MENU_SHELL( menu ), item );
	signal_connect( window, item, G_CALLBACK( on_new_action_activated ), G_CALLBACK( on_new_action_selected ));

	/* i18n: 'New profile' item in 'File' menu */
	item = gtk_image_menu_item_new_with_mnemonic( _( "New _profile" ));
	gtk_menu_shell_append( GTK_MENU_SHELL( menu ), item );
	set_new_profile_item( NACT_WINDOW( window ), item );
	signal_connect( window, item, G_CALLBACK( on_new_profile_activated ), G_CALLBACK( on_new_profile_selected ));

	item = gtk_image_menu_item_new_from_stock( GTK_STOCK_SAVE, NULL );
	gtk_menu_shell_append( GTK_MENU_SHELL( menu ), item );
	set_save_item( NACT_WINDOW( window ), item );
	signal_connect( window, item, G_CALLBACK( on_save_activated ), G_CALLBACK( on_save_selected ));

	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append( GTK_MENU_SHELL( menu ), item );

	item = gtk_image_menu_item_new_from_stock( GTK_STOCK_QUIT, NULL );
	gtk_menu_shell_append( GTK_MENU_SHELL( menu ), item );
	signal_connect( window, item, G_CALLBACK( on_quit_activated ), G_CALLBACK( on_quit_selected ));
}

static void
create_edit_menu( NactMainWindow *window, GtkMenuBar *menubar )
{
	/* i18n: Edit menu */
	GtkWidget *edit = gtk_menu_item_new_with_mnemonic( _( "_Edit" ));
	gtk_menu_shell_append( GTK_MENU_SHELL( menubar ), edit );
	GtkWidget *menu = gtk_menu_new();
	gtk_menu_item_set_submenu( GTK_MENU_ITEM( edit ), menu );
	nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( edit ), "select", G_CALLBACK( on_edit_selected ));

	/* i18n: Duplicate item in Edit menu */
	GtkWidget *item = gtk_image_menu_item_new_with_mnemonic( _( "D_uplicate" ));
	gtk_image_menu_item_set_image( GTK_IMAGE_MENU_ITEM( item ), gtk_image_new_from_stock( GTK_STOCK_COPY, GTK_ICON_SIZE_MENU ));
	gtk_menu_shell_append( GTK_MENU_SHELL( menu ), item );
	set_duplicate_item( NACT_WINDOW( window ), item );
	signal_connect( window, item, G_CALLBACK( on_duplicate_activated ), G_CALLBACK( on_duplicate_selected ));

	/* i18n: Delete item in Edit menu */
	item = gtk_image_menu_item_new_from_stock( GTK_STOCK_DELETE, NULL );
	gtk_menu_shell_append( GTK_MENU_SHELL( menu ), item );
	set_delete_item( NACT_WINDOW( window ), item );
	signal_connect( window, item, G_CALLBACK( on_delete_activated ), G_CALLBACK( on_delete_selected ));

	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append( GTK_MENU_SHELL( menu ), item );

	item = gtk_image_menu_item_new_with_mnemonic( _( "_Reload the list of actions" ));
	gtk_menu_shell_append( GTK_MENU_SHELL( menu ), item );
	signal_connect( window, item, G_CALLBACK( on_reload_activated ), G_CALLBACK( on_reload_selected ));
}

static void
create_tools_menu( NactMainWindow *window, GtkMenuBar *menubar )
{
	/* i18n: Tools menu */
	GtkWidget *tools = gtk_menu_item_new_with_mnemonic( _( "_Tools" ));
	gtk_menu_shell_append( GTK_MENU_SHELL( menubar ), tools );
	GtkWidget *menu = gtk_menu_new();
	gtk_menu_item_set_submenu( GTK_MENU_ITEM( tools ), menu );
	nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( tools ), "select", G_CALLBACK( on_tools_selected ));

	/* i18n: Import item in Tools menu */
	GtkWidget *item = gtk_image_menu_item_new_with_mnemonic( _( "_Import assistant..." ));
	gtk_image_menu_item_set_image( GTK_IMAGE_MENU_ITEM( item ), gtk_image_new_from_stock( GTK_STOCK_CONVERT, GTK_ICON_SIZE_MENU ));
	gtk_menu_shell_append( GTK_MENU_SHELL( menu ), item );
	signal_connect( window, item, G_CALLBACK( on_import_activated ), G_CALLBACK( on_import_selected ));

	/* i18n: Export item in Tools menu */
	item = gtk_image_menu_item_new_with_mnemonic( _( "E_xport assistant..." ));
	gtk_menu_shell_append( GTK_MENU_SHELL( menu ), item );
	set_export_item( NACT_WINDOW( window ), item );
	signal_connect( window, item, G_CALLBACK( on_export_activated ), G_CALLBACK( on_export_selected ));
}

static void
create_help_menu( NactMainWindow *window, GtkMenuBar *menubar )
{
	/* i18n: Help menu */
	GtkWidget *help = gtk_menu_item_new_with_mnemonic( _( "_Help" ));
	gtk_menu_shell_append( GTK_MENU_SHELL( menubar ), help );
	GtkWidget *menu = gtk_menu_new();
	gtk_menu_item_set_submenu( GTK_MENU_ITEM( help ), menu );

	GtkWidget *item = gtk_image_menu_item_new_from_stock( GTK_STOCK_HELP, NULL );
	gtk_menu_shell_append( GTK_MENU_SHELL( menu ), item );
	signal_connect( window, item, G_CALLBACK( on_help_activated ), G_CALLBACK( on_help_selected ));
	gtk_widget_set_sensitive( item, FALSE );

	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append( GTK_MENU_SHELL( menu ), item );

	item = gtk_image_menu_item_new_from_stock( GTK_STOCK_ABOUT, NULL );
	gtk_menu_shell_append( GTK_MENU_SHELL( menu ), item );
	signal_connect( window, item, G_CALLBACK( on_about_activated ), G_CALLBACK( on_about_selected ));
}

static void
signal_connect( NactMainWindow *window, GtkWidget *item, GCallback on_activated, GCallback on_selected )
{
	if( on_activated ){
		nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( item ), "activate", on_activated );
	}

	if( on_selected ){
		nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( item ), "select", on_selected );
		nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( item ), "deselect", G_CALLBACK( on_menu_item_deselected ));
	}
}

static void
on_file_selected( GtkMenuItem *item, NactWindow *window )
{
	NAObject *object = v_get_selected( window );
	gboolean new_profile_enabled = NA_IS_ACTION( object ) || NA_IS_ACTION_PROFILE( object );
	GtkWidget *new_profile_item = get_new_profile_item( window );
	gtk_widget_set_sensitive( new_profile_item, new_profile_enabled );

	gint modified = v_count_modified_actions( window );
	gboolean save_enabled = ( modified > 0 );
	GtkWidget *save_item = get_save_item( window );
	gtk_widget_set_sensitive( save_item, save_enabled );
}

static void
on_new_action_activated( GtkMenuItem *item, NactWindow *window )
{
	NAAction *action = na_action_new_with_profile();
	add_action( window, action );
}

static void
on_new_action_selected( GtkItem *item, NactWindow *window )
{
	/* i18n: tooltip displayed in the status bar when selecting the 'New action' item */
	nact_statusbar_display_status( NACT_MAIN_WINDOW( window ), PROP_IMENUBAR_STATUS_CONTEXT, _( "Define a new action." ));
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
on_new_profile_selected( GtkItem *item, NactWindow *window )
{
	/* i18n: tooltip displayed in the status bar when selecting the 'New profile' item */
	nact_statusbar_display_status( NACT_MAIN_WINDOW( window ), PROP_IMENUBAR_STATUS_CONTEXT, _( "Define a new profile attached to the current action." ));
}

static void
on_save_activated( GtkMenuItem *item, NactWindow *window )
{
	NactApplication *application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	NAPivot *pivot = nact_application_get_pivot( application );
	GSList *actions = v_get_actions( window );
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
	 * - remove the origin of pivot if any
	 * - add a duplicate of this action to pivot
	 * - set the duplicate as the origin of this action
	 * - reset the status
	 */
	for( ia = actions ; ia ; ia = ia->next ){
		NAAction *current = NA_ACTION( ia->data );
		if( na_object_get_modified_status( NA_OBJECT( current )) &&
			na_object_get_valid_status( NA_OBJECT( current ))){

			if( nact_window_save_action( window, current )){

				NAAction *origin = NA_ACTION( na_object_get_origin( NA_OBJECT( current )));
				if( origin ){
					na_pivot_remove_action( pivot, origin );
				}

				NAAction *dup_pivot = NA_ACTION( na_object_duplicate( NA_OBJECT( current )));
				na_pivot_add_action( pivot, dup_pivot );

				v_remove_action( window, current );
				g_object_unref( current );

				NAAction *dup_window = NA_ACTION( na_object_duplicate( NA_OBJECT( dup_pivot )));
				v_add_action( window, dup_window );
				na_object_check_edited_status( NA_OBJECT( dup_window ));
			}
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
	v_on_save( window );

	v_setup_dialog_title( window );

	if( current ){
		v_update_actions_list( window );
		v_select_actions_list( window, type, uuid, label );
		g_free( label );
		g_free( uuid );
	}
}

static void
on_save_selected( GtkMenuItem *item, NactWindow *window )
{
	/* i18n: tooltip displayed in the status bar when selecting 'Save' item */
	nact_statusbar_display_status( NACT_MAIN_WINDOW( window ), PROP_IMENUBAR_STATUS_CONTEXT, _( "Record all the modified actions. Invalid actions will be silently ignored." ));
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
on_quit_selected( GtkMenuItem *item, NactWindow *window )
{
	/* i18n: tooltip displayed in the status bar when selecting 'Quit' item */
	nact_statusbar_display_status( NACT_MAIN_WINDOW( window ), PROP_IMENUBAR_STATUS_CONTEXT, _( "Quit the application." ));
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
on_edit_selected( GtkMenuItem *item, NactWindow *window )
{
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

	GtkWidget *delete_item = get_delete_item( window );
	gtk_widget_set_sensitive( delete_item, delete_enabled );

	GtkWidget *duplicate_item = get_duplicate_item( window );
	gtk_widget_set_sensitive( duplicate_item, duplicate_enabled );
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
on_duplicate_selected( GtkItem *item, NactWindow *window )
{
	/* i18n: tooltip displayed in the status bar when selecting the Duplicate item */
	nact_statusbar_display_status( NACT_MAIN_WINDOW( window ), PROP_IMENUBAR_STATUS_CONTEXT, _( "Create a copy of the selected action or profile." ));
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
on_delete_selected( GtkItem *item, NactWindow *window )
{
	/* i18n: tooltip displayed in the status bar when selecting the Delete item */
	nact_statusbar_display_status( NACT_MAIN_WINDOW( window ), PROP_IMENUBAR_STATUS_CONTEXT, _( "Remove the selected action or profile from your configuration." ));
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
on_reload_selected( GtkItem *item, NactWindow *window )
{
	/* i18n: tooltip displayed in the status bar when selecting the 'Reload' item */
	nact_statusbar_display_status( NACT_MAIN_WINDOW( window ), PROP_IMENUBAR_STATUS_CONTEXT, _( " Cancel your current modifications and reload the list of actions." ));
}

static void
on_tools_selected( GtkMenuItem *item, NactWindow *window )
{
	gboolean export_enabled = v_count_actions( window );
	GtkWidget *export_item = get_export_item( window );
	gtk_widget_set_sensitive( export_item, export_enabled );
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
on_import_selected( GtkItem *item, NactWindow *window )
{
	/* i18n: tooltip displayed in the status bar when selecting the Import item */
	nact_statusbar_display_status( NACT_MAIN_WINDOW( window ), PROP_IMENUBAR_STATUS_CONTEXT, _( "Import one or more actions from external (XML) files into your configuration." ));
}

static void
on_export_activated( GtkMenuItem *item, NactWindow *window )
{
	static const gchar *thisfn = "nact_imenubar_on_export_activated";
	g_debug( "%s: item=%p, window=%p", thisfn, item, window );

	nact_assistant_export_run( NACT_WINDOW( window ));

	/*g_assert( NACT_IS_MAIN_WINDOW( user_data ));
	NactWindow *wndmain = NACT_WINDOW( user_data );
	nact_iactions_list_set_focus( wndmain );*/
}

static void
on_export_selected( GtkItem *item, NactWindow *window )
{
	/* i18n: tooltip displayed in the status bar when selecting the Export item */
	nact_statusbar_display_status( NACT_MAIN_WINDOW( window ), PROP_IMENUBAR_STATUS_CONTEXT, _( "Export one or more actions from your configuration to external XML files." ));
}

static void
on_help_activated( GtkMenuItem *item, NactWindow *window )
{
}

static void
on_help_selected( GtkItem *item, NactWindow *window )
{
	/* i18n: tooltip displayed in the status bar when selecting the Help item */
	nact_statusbar_display_status( NACT_MAIN_WINDOW( window ), PROP_IMENUBAR_STATUS_CONTEXT, _( "Display help about this program." ));
}

/* TODO: make the website url and the mail addresses clickables
 */
static void
on_about_activated( GtkMenuItem *item, NactWindow *window )
{
	static const gchar *thisfn = "nact_imenubar_on_about_activated";
	g_debug( "%s: item=%p, window=%p", thisfn, item, window );

	BaseApplication *appli;
	g_object_get( G_OBJECT( window ), PROP_WINDOW_APPLICATION_STR, &appli, NULL );
	gchar *icon_name = base_application_get_icon_name( appli );

	static const gchar *artists[] = {
		N_( "Ulisse Perusin <uli.peru@gmail.com>" ),
		NULL
	};

	static const gchar *authors[] = {
		N_( "Frederic Ruaudel <grumz@grumz.net>" ),
		N_( "Rodrigo Moya <rodrigo@gnome-db.org>" ),
		N_( "Pierre Wieser <pwieser@trychlos.org>" ),
		NULL
	};

	static const gchar *documenters[] = {
		NULL
	};

	static gchar *license[] = {
		N_( "Nautilus Actions Configuration Tool is free software; you can "
			"redistribute it and/or modify it under the terms of the GNU General "
			"Public License as published by the Free Software Foundation; either "
			"version 2 of the License, or (at your option) any later version." ),
		N_( "Nautilus Actions Configuration Tool is distributed in the hope that it "
			"will be useful, but WITHOUT ANY WARRANTY; without even the implied "
			"warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See "
			"the GNU General Public License for more details." ),
		N_( "You should have received a copy of the GNU General Public License along "
			"with Nautilus Actions Configuration Tool ; if not, write to the Free "
			"Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, "
			"MA 02110-1301, USA." ),
		NULL
	};
	gchar *license_i18n = g_strjoinv( "\n\n", license );

	GtkWindow *toplevel = base_window_get_toplevel_dialog( BASE_WINDOW( window ));

	gtk_show_about_dialog( toplevel,
			"artists", artists,
			"authors", authors,
			"comments", _( "A graphical interface to create and edit your Nautilus actions." ),
			"copyright", _( "Copyright \xc2\xa9 2005-2007 Frederic Ruaudel <grumz@grumz.net>\nCopyright \xc2\xa9 2009 Pierre Wieser <pwieser@trychlos.org>" ),
			"documenters", documenters,
			"translator-credits", _( "The GNOME Translation Project <gnome-i18n@gnome.org>" ),
			"license", license_i18n,
			"wrap-license", TRUE,
			"logo-icon-name", icon_name,
			"version", PACKAGE_VERSION,
			"website", "http://www.nautilus-actions.org",
			NULL );

	g_free( license_i18n );
	g_free( icon_name );

	/*nact_iactions_list_set_focus( NACT_WINDOW( wndmain ));*/
}

static void
on_about_selected( GtkItem *item, NactWindow *window )
{
	/* i18n: tooltip displayed in the status bar when selecting the About item */
	nact_statusbar_display_status( NACT_MAIN_WINDOW( window ), PROP_IMENUBAR_STATUS_CONTEXT, _( "Display informations about this program." ));
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

static void
v_on_save( NactWindow *window )
{
	if( NACT_IMENUBAR_GET_INTERFACE( window )->on_save ){
		NACT_IMENUBAR_GET_INTERFACE( window )->on_save( window );
	}
}

static GtkWidget *
get_new_profile_item( NactWindow *window )
{
	return( GTK_WIDGET( g_object_get_data( G_OBJECT( window ), PROP_IMENUBAR_NEW_PROFILE_ITEM )));
}

static void
set_new_profile_item( NactWindow *window, GtkWidget *item )
{
	g_object_set_data( G_OBJECT( window ), PROP_IMENUBAR_NEW_PROFILE_ITEM, item );
}

static GtkWidget *
get_save_item( NactWindow *window )
{
	return( GTK_WIDGET( g_object_get_data( G_OBJECT( window ), PROP_IMENUBAR_SAVE_ITEM )));
}

static void
set_save_item( NactWindow *window, GtkWidget *item )
{
	g_object_set_data( G_OBJECT( window ), PROP_IMENUBAR_SAVE_ITEM, item );
}

static GtkWidget *
get_duplicate_item( NactWindow *window )
{
	return( GTK_WIDGET( g_object_get_data( G_OBJECT( window ), PROP_IMENUBAR_DUPLICATE_ITEM )));
}

static void
set_duplicate_item( NactWindow *window, GtkWidget *item )
{
	g_object_set_data( G_OBJECT( window ), PROP_IMENUBAR_DUPLICATE_ITEM, item );
}

static GtkWidget *
get_delete_item( NactWindow *window )
{
	return( GTK_WIDGET( g_object_get_data( G_OBJECT( window ), PROP_IMENUBAR_DELETE_ITEM )));
}

static void
set_delete_item( NactWindow *window, GtkWidget *item )
{
	g_object_set_data( G_OBJECT( window ), PROP_IMENUBAR_DELETE_ITEM, item );
}

static GtkWidget *
get_export_item( NactWindow *window )
{
	return( GTK_WIDGET( g_object_get_data( G_OBJECT( window ), PROP_IMENUBAR_EXPORT_ITEM )));
}

static void
set_export_item( NactWindow *window, GtkWidget *item )
{
	g_object_set_data( G_OBJECT( window ), PROP_IMENUBAR_EXPORT_ITEM, item );
}
