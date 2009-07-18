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

#include <stdlib.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <common/na-pivot.h>
#include <common/na-iio-provider.h>
#include <common/na-ipivot-container.h>

#include "nact-application.h"
#include "nact-action-conditions-editor.h"
#include "nact-action-profiles-editor.h"
#include "nact-assist-export.h"
#include "nact-assist-import.h"
#include "nact-iactions-list.h"
#include "nact-iaction-tab.h"
#include "nact-icommand-tab.h"
#include "nact-iconditions-tab.h"
#include "nact-iadvanced-tab.h"
#include "nact-iprefs.h"
#include "nact-main-window.h"

/* private class data
 */
struct NactMainWindowClassPrivate {
};

/* private instance data
 */
struct NactMainWindowPrivate {
	gboolean         dispose_has_run;
	GtkStatusbar    *status_bar;
	guint            status_context;
	GtkWidget       *new_profile_item;
	GtkWidget       *save_item;
	GSList          *actions;
	NAAction        *edited_action;
	NAActionProfile *edited_profile;
};

/* the GConf key used to read/write size and position of auxiliary dialogs
 */
#define IPREFS_IMPORT_ACTIONS		"main-window-import-actions"

static GObjectClass *st_parent_class = NULL;

static GType            register_type( void );
static void             class_init( NactMainWindowClass *klass );
static void             iactions_list_iface_init( NactIActionsListInterface *iface );
static void             iaction_tab_iface_init( NactIActionTabInterface *iface );
static void             icommand_tab_iface_init( NactICommandTabInterface *iface );
static void             iconditions_tab_iface_init( NactIConditionsTabInterface *iface );
static void             iadvanced_tab_iface_init( NactIAdvancedTabInterface *iface );
static void             ipivot_container_iface_init( NAIPivotContainerInterface *iface );
static void             instance_init( GTypeInstance *instance, gpointer klass );
static void             instance_dispose( GObject *application );
static void             instance_finalize( GObject *application );

static gchar           *get_iprefs_window_id( NactWindow *window );
static gchar           *get_toplevel_name( BaseWindow *window );
static GSList          *get_actions( NactWindow *window );
static void             set_sorted_actions( NactWindow *window, GSList *actions );

static void             on_initial_load_toplevel( BaseWindow *window );
static void             create_file_menu( BaseWindow *window, GtkMenuBar *menubar );
static void             create_tools_menu( BaseWindow *window, GtkMenuBar *menubar );
static void             create_help_menu( BaseWindow *window, GtkMenuBar *menubar );
static void             on_runtime_init_toplevel( BaseWindow *window );
static void             setup_dialog_title( NactMainWindow *window );
static void             setup_dialog_menu( NactMainWindow *window );

static void             on_actions_list_selection_changed( GtkTreeSelection *selection, gpointer user_data );
static gboolean         on_actions_list_double_click( GtkWidget *widget, GdkEventButton *event, gpointer data );
static gboolean         on_actions_list_enter_key_pressed( GtkWidget *widget, GdkEventKey *event, gpointer data );
static void             set_current_action( NactMainWindow *window );
static void             set_current_profile( NactMainWindow *window );
static NAAction        *get_edited_action( NactWindow *window );
static NAActionProfile *get_edited_profile( NactWindow *window );
static void             on_modified_field( NactWindow *window );
static void             check_edited_status( NactWindow *window, const NAAction *action );
static gboolean         is_action_modified( const NAAction *action );
static gboolean         is_action_to_save( const NAAction *action );

static void             get_isfiledir( NactWindow *window, gboolean *isfile, gboolean *isdir );
static gboolean         get_multiple( NactWindow *window );
static GSList          *get_schemes( NactWindow *window );

static void             on_new_action_activated( GtkMenuItem *item, gpointer user_data );
static void             on_new_action_selected( GtkItem *item, gpointer user_data );
static void             on_new_profile_activated( GtkMenuItem *item, gpointer user_data );
static void             on_new_profile_selected( GtkItem *item, gpointer user_data );

static void             on_import_activated( GtkMenuItem *item, gpointer user_data );
static void             on_import_selected( GtkItem *item, gpointer user_data );
static void             on_export_activated( GtkMenuItem *item, gpointer user_data );
static void             on_export_selected( GtkItem *item, gpointer user_data );
static void             on_about_activated( GtkMenuItem *item, gpointer user_data );
static void             on_about_selected( GtkItem *item, gpointer user_data );
static void             on_menu_item_deselected( GtkItem *item, gpointer user_data );
/*static void     on_new_button_clicked( GtkButton *button, gpointer user_data );
static void     on_edit_button_clicked( GtkButton *button, gpointer user_data );
static void     on_duplicate_button_clicked( GtkButton *button, gpointer user_data );
static void     on_delete_button_clicked( GtkButton *button, gpointer user_data );
static gboolean on_dialog_response( GtkDialog *dialog, gint response_id, BaseWindow *window );*/
static void             on_close( GtkMenuItem *item, gpointer user_data );
static gboolean         on_delete_event( BaseWindow *window, GtkWindow *toplevel, GdkEvent *event );

static gint             count_modified_actions( NactMainWindow *window );
static void             on_actions_changed( NAIPivotContainer *instance, gpointer user_data );

GType
nact_main_window_get_type( void )
{
	static GType window_type = 0;

	if( !window_type ){
		window_type = register_type();
	}

	return( window_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "nact_main_window_register_type";
	g_debug( "%s", thisfn );

	static GTypeInfo info = {
		sizeof( NactMainWindowClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactMainWindow ),
		0,
		( GInstanceInitFunc ) instance_init
	};
	GType type = g_type_register_static( NACT_WINDOW_TYPE, "NactMainWindow", &info, 0 );

	/* implement IActionsList interface
	 */
	static const GInterfaceInfo iactions_list_iface_info = {
		( GInterfaceInitFunc ) iactions_list_iface_init,
		NULL,
		NULL
	};
	g_type_add_interface_static( type, NACT_IACTIONS_LIST_TYPE, &iactions_list_iface_info );

	/* implement IActionTab interface
	 */
	static const GInterfaceInfo iaction_tab_iface_info = {
		( GInterfaceInitFunc ) iaction_tab_iface_init,
		NULL,
		NULL
	};
	g_type_add_interface_static( type, NACT_IACTION_TAB_TYPE, &iaction_tab_iface_info );

	/* implement ICommandTab interface
	 */
	static const GInterfaceInfo icommand_tab_iface_info = {
		( GInterfaceInitFunc ) icommand_tab_iface_init,
		NULL,
		NULL
	};
	g_type_add_interface_static( type, NACT_ICOMMAND_TAB_TYPE, &icommand_tab_iface_info );

	/* implement IConditionsTab interface
	 */
	static const GInterfaceInfo iconditions_tab_iface_info = {
		( GInterfaceInitFunc ) iconditions_tab_iface_init,
		NULL,
		NULL
	};
	g_type_add_interface_static( type, NACT_ICONDITIONS_TAB_TYPE, &iconditions_tab_iface_info );

	/* implement IAdvancedTab interface
	 */
	static const GInterfaceInfo iadvanced_tab_iface_info = {
		( GInterfaceInitFunc ) iadvanced_tab_iface_init,
		NULL,
		NULL
	};
	g_type_add_interface_static( type, NACT_IADVANCED_TAB_TYPE, &iadvanced_tab_iface_info );

	/* implement IPivotContainer interface
	 */
	static const GInterfaceInfo pivot_container_iface_info = {
		( GInterfaceInitFunc ) ipivot_container_iface_init,
		NULL,
		NULL
	};
	g_type_add_interface_static( type, NA_IPIVOT_CONTAINER_TYPE, &pivot_container_iface_info );

	return( type );
}

static void
class_init( NactMainWindowClass *klass )
{
	static const gchar *thisfn = "nact_main_window_class_init";
	g_debug( "%s: klass=%p", thisfn, klass );

	st_parent_class = g_type_class_peek_parent( klass );

	GObjectClass *object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactMainWindowClassPrivate, 1 );

	BaseWindowClass *base_class = BASE_WINDOW_CLASS( klass );
	base_class->get_toplevel_name = get_toplevel_name;
	base_class->initial_load_toplevel = on_initial_load_toplevel;
	base_class->runtime_init_toplevel = on_runtime_init_toplevel;
	base_class->delete_event = on_delete_event;

	NactWindowClass *nact_class = NACT_WINDOW_CLASS( klass );
	nact_class->get_iprefs_window_id = get_iprefs_window_id;
}

static void
iactions_list_iface_init( NactIActionsListInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_iactions_list_iface_init";
	g_debug( "%s: iface=%p", thisfn, iface );

	iface->get_actions = get_actions;
	iface->set_sorted_actions = set_sorted_actions;
	iface->on_selection_changed = on_actions_list_selection_changed;
	iface->on_double_click = on_actions_list_double_click;
	iface->on_enter_key_pressed = on_actions_list_enter_key_pressed;
}

static void
iaction_tab_iface_init( NactIActionTabInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_iaction_tab_iface_init";
	g_debug( "%s: iface=%p", thisfn, iface );

	iface->get_edited_action = get_edited_action;
	iface->field_modified = on_modified_field;
}

static void
icommand_tab_iface_init( NactICommandTabInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_icommand_tab_iface_init";
	g_debug( "%s: iface=%p", thisfn, iface );

	iface->get_edited_profile = get_edited_profile;
	iface->field_modified = on_modified_field;
	iface->get_isfiledir = get_isfiledir;
	iface->get_multiple = get_multiple;
	iface->get_schemes = get_schemes;
}

static void
iconditions_tab_iface_init( NactIConditionsTabInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_iconditions_tab_iface_init";
	g_debug( "%s: iface=%p", thisfn, iface );

	iface->get_edited_profile = get_edited_profile;
	iface->field_modified = on_modified_field;
}

static void
iadvanced_tab_iface_init( NactIAdvancedTabInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_iadvanced_tab_iface_init";
	g_debug( "%s: iface=%p", thisfn, iface );

	iface->get_edited_profile = get_edited_profile;
	iface->field_modified = on_modified_field;
}

static void
ipivot_container_iface_init( NAIPivotContainerInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_ipivot_container_iface_init";
	g_debug( "%s: iface=%p", thisfn, iface );

	iface->on_actions_changed = on_actions_changed;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_main_window_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );

	g_assert( NACT_IS_MAIN_WINDOW( instance ));
	NactMainWindow *self = NACT_MAIN_WINDOW( instance );

	self->private = g_new0( NactMainWindowPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *window )
{
	static const gchar *thisfn = "nact_main_window_instance_dispose";
	g_debug( "%s: window=%p", thisfn, window );

	g_assert( NACT_IS_MAIN_WINDOW( window ));
	NactMainWindow *self = NACT_MAIN_WINDOW( window );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		GtkWidget *pane = base_window_get_widget( BASE_WINDOW( window ), "MainPaned" );
		gint pos = gtk_paned_get_position( GTK_PANED( pane ));
		nact_iprefs_set_int( NACT_WINDOW( window ), "main-paned", pos );

		GSList *ia;
		for( ia = self->private->actions ; ia ; ia = ia->next ){
			g_object_unref( NA_ACTION( ia->data ));
		}
		g_slist_free( self->private->actions );

		nact_iaction_tab_dispose( NACT_WINDOW( window ));
		nact_icommand_tab_dispose( NACT_WINDOW( window ));
		nact_iconditions_tab_dispose( NACT_WINDOW( window ));
		nact_iadvanced_tab_dispose( NACT_WINDOW( window ));

		/* chain up to the parent class */
		G_OBJECT_CLASS( st_parent_class )->dispose( window );
	}
}

static void
instance_finalize( GObject *window )
{
	static const gchar *thisfn = "nact_main_window_instance_finalize";
	g_debug( "%s: window=%p", thisfn, window );

	g_assert( NACT_IS_MAIN_WINDOW( window ));
	NactMainWindow *self = ( NactMainWindow * ) window;

	/*g_free( self->private->current_uuid );
	g_free( self->private->current_label );*/

	g_free( self->private );

	/* chain call to parent class */
	if( st_parent_class->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( window );
	}
}

/**
 * Returns a newly allocated NactMainWindow object.
 */
NactMainWindow *
nact_main_window_new( GObject *application )
{
	g_assert( NACT_IS_APPLICATION( application ));

	return( g_object_new( NACT_MAIN_WINDOW_TYPE, PROP_WINDOW_APPLICATION_STR, application, NULL ));
}

static gchar *
get_iprefs_window_id( NactWindow *window )
{
	return( g_strdup( "main-window" ));
}

static gchar *
get_toplevel_name( BaseWindow *window )
{
	return( g_strdup( "MainWindow" ));
}

static GSList *
get_actions( NactWindow *window )
{
	g_assert( NACT_IS_MAIN_WINDOW( window ));
	return( NACT_MAIN_WINDOW( window )->private->actions );
}

static void
set_sorted_actions( NactWindow *window, GSList *actions )
{
	g_assert( NACT_IS_MAIN_WINDOW( window ));
	NACT_MAIN_WINDOW( window )->private->actions = actions;
}

/*
 * note that for this NactMainWindow, on_initial_load_toplevel and
 * on_runtime_init_toplevel are equivalent, as there is only one
 * occurrence on this window in the application : closing this window
 * is the same than quitting the application
 */
static void
on_initial_load_toplevel( BaseWindow *window )
{
	static const gchar *thisfn = "nact_main_window_on_initial_load_toplevel";

	/* call parent class at the very beginning */
	if( BASE_WINDOW_CLASS( st_parent_class )->initial_load_toplevel ){
		BASE_WINDOW_CLASS( st_parent_class )->initial_load_toplevel( window );
	}

	g_debug( "%s: window=%p", thisfn, window );
	g_assert( NACT_IS_MAIN_WINDOW( window ));
	NactMainWindow *wnd = NACT_MAIN_WINDOW( window );

	NactApplication *application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( wnd )));
	NAPivot *pivot = NA_PIVOT( nact_application_get_pivot( application ));
	GSList *origin = na_pivot_get_actions( pivot );
	GSList *ia;
	for( ia = origin ; ia ; ia = ia->next ){
		wnd->private->actions = g_slist_prepend( wnd->private->actions, na_action_duplicate( NA_ACTION( ia->data )));
	}

	GtkWidget *vbox = base_window_get_widget( window, "MenuBarVBox" );
	GtkWidget *menubar = gtk_menu_bar_new();
	gtk_container_add( GTK_CONTAINER( vbox ), menubar );

	create_file_menu( window, GTK_MENU_BAR( menubar ));
	create_tools_menu( window, GTK_MENU_BAR( menubar ));
	create_help_menu( window, GTK_MENU_BAR( menubar ));

	wnd->private->status_bar = GTK_STATUSBAR( base_window_get_widget( window, "StatusBar" ));
	wnd->private->status_context = gtk_statusbar_get_context_id( wnd->private->status_bar, "NautilusActionsConfigurationTool" );

	g_assert( NACT_IS_IACTIONS_LIST( window ));
	nact_iactions_list_initial_load( NACT_WINDOW( window ));
	nact_iactions_list_set_multiple_selection( NACT_WINDOW( window ), FALSE );
	nact_iactions_list_set_send_selection_changed_on_fill_list( NACT_WINDOW( window ), FALSE );

	g_assert( NACT_IS_IACTION_TAB( window ));
	nact_iaction_tab_initial_load( NACT_WINDOW( window ));

	g_assert( NACT_IS_ICOMMAND_TAB( window ));
	nact_icommand_tab_initial_load( NACT_WINDOW( window ));

	g_assert( NACT_IS_ICONDITIONS_TAB( window ));
	nact_iconditions_tab_initial_load( NACT_WINDOW( window ));

	g_assert( NACT_IS_IADVANCED_TAB( window ));
	nact_iadvanced_tab_initial_load( NACT_WINDOW( window ));

	gint pos = nact_iprefs_get_int( NACT_WINDOW( window ), "main-paned" );
	if( pos ){
		GtkWidget *pane = base_window_get_widget( window, "MainPaned" );
		gtk_paned_set_position( GTK_PANED( pane ), pos );
	}
}

static void
create_file_menu( BaseWindow *window, GtkMenuBar *menubar )
{
	/* i18n: File menu */
	GtkWidget *file = gtk_menu_item_new_with_label( _( "_File" ));
	gtk_menu_item_set_use_underline( GTK_MENU_ITEM( file ), TRUE );
	gtk_menu_shell_append( GTK_MENU_SHELL( menubar ), file );
	GtkWidget *menu = gtk_menu_new();
	gtk_menu_item_set_submenu( GTK_MENU_ITEM( file ), menu );

	/* i18n: 'New action' item in 'File' menu */
	GtkWidget *item = gtk_image_menu_item_new_with_label( _( "New _action" ));
	gtk_menu_shell_append( GTK_MENU_SHELL( menu ), item );
	gtk_menu_item_set_use_underline( GTK_MENU_ITEM( item ), TRUE );
	nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( item ), "activate", G_CALLBACK( on_new_action_activated ));
	nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( item ), "select", G_CALLBACK( on_new_action_selected ));
	nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( item ), "deselect", G_CALLBACK( on_menu_item_deselected ));

	/* i18n: 'New profile' item in 'File' menu */
	item = gtk_image_menu_item_new_with_label( _( "New _profile" ));
	gtk_menu_shell_append( GTK_MENU_SHELL( menu ), item );
	gtk_menu_item_set_use_underline( GTK_MENU_ITEM( item ), TRUE );
	NACT_MAIN_WINDOW( window )->private->new_profile_item = item;
	nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( item ), "activate", G_CALLBACK( on_new_profile_activated ));
	nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( item ), "select", G_CALLBACK( on_new_profile_selected ));
	nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( item ), "deselect", G_CALLBACK( on_menu_item_deselected ));

	item = gtk_image_menu_item_new_from_stock( GTK_STOCK_SAVE, NULL );
	gtk_menu_shell_append( GTK_MENU_SHELL( menu ), item );
	NACT_MAIN_WINDOW( window )->private->save_item = item;

	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append( GTK_MENU_SHELL( menu ), item );

	item = gtk_image_menu_item_new_from_stock( GTK_STOCK_CLOSE, NULL );
	gtk_menu_shell_append( GTK_MENU_SHELL( menu ), item );
	nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( item ), "activate", G_CALLBACK( on_close ));
}

static void
create_tools_menu( BaseWindow *window, GtkMenuBar *menubar )
{
	/* i18n: Tools menu */
	GtkWidget *tools = gtk_menu_item_new_with_label( _( "_Tools" ));
	gtk_menu_item_set_use_underline( GTK_MENU_ITEM( tools ), TRUE );
	gtk_menu_shell_append( GTK_MENU_SHELL( menubar ), tools );
	GtkWidget *menu = gtk_menu_new();
	gtk_menu_item_set_submenu( GTK_MENU_ITEM( tools ), menu );

	/* i18n: Import item in Tools menu */
	GtkWidget *item = gtk_image_menu_item_new_with_label( _( "_Import" ));
	gtk_menu_item_set_use_underline( GTK_MENU_ITEM( item ), TRUE );
	gtk_menu_shell_append( GTK_MENU_SHELL( menu ), item );
	nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( item ), "activate", G_CALLBACK( on_import_activated ));
	nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( item ), "select", G_CALLBACK( on_import_selected ));
	nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( item ), "deselect", G_CALLBACK( on_menu_item_deselected ));

	/* i18n: Export item in Tools menu */
	item = gtk_image_menu_item_new_with_label( _( "_Export" ));
	gtk_menu_item_set_use_underline( GTK_MENU_ITEM( item ), TRUE );
	gtk_menu_shell_append( GTK_MENU_SHELL( menu ), item );
	nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( item ), "activate", G_CALLBACK( on_export_activated ));
	nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( item ), "select", G_CALLBACK( on_export_selected ));
	nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( item ), "deselect", G_CALLBACK( on_menu_item_deselected ));
}

static void
create_help_menu( BaseWindow *window, GtkMenuBar *menubar )
{
	/* i18n: Help menu */
	GtkWidget *help = gtk_menu_item_new_with_label( _( "_Help" ));
	gtk_menu_item_set_use_underline( GTK_MENU_ITEM( help ), TRUE );
	gtk_menu_shell_append( GTK_MENU_SHELL( menubar ), help );
	GtkWidget *menu = gtk_menu_new();
	gtk_menu_item_set_submenu( GTK_MENU_ITEM( help ), menu );

	GtkWidget *item = gtk_image_menu_item_new_from_stock( GTK_STOCK_ABOUT, NULL );
	gtk_menu_shell_append( GTK_MENU_SHELL( menu ), item );
	nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( item ), "activate", G_CALLBACK( on_about_activated ));
	nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( item ), "select", G_CALLBACK( on_about_selected ));
	nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( item ), "deselect", G_CALLBACK( on_menu_item_deselected ));
}

static void
on_runtime_init_toplevel( BaseWindow *window )
{
	static const gchar *thisfn = "nact_main_window_on_runtime_init_toplevel";

	/* call parent class at the very beginning */
	if( BASE_WINDOW_CLASS( st_parent_class )->runtime_init_toplevel ){
		BASE_WINDOW_CLASS( st_parent_class )->runtime_init_toplevel( window );
	}

	g_debug( "%s: window=%p", thisfn, window );
	g_assert( NACT_IS_MAIN_WINDOW( window ));
	/*NactMainWindow *wnd = NACT_MAIN_WINDOW( window );*/

	g_assert( NACT_IS_IACTIONS_LIST( window ));
	nact_iactions_list_runtime_init( NACT_WINDOW( window ));

	g_assert( NACT_IS_IACTION_TAB( window ));
	nact_iaction_tab_runtime_init( NACT_WINDOW( window ));

	g_assert( NACT_IS_ICOMMAND_TAB( window ));
	nact_icommand_tab_runtime_init( NACT_WINDOW( window ));

	g_assert( NACT_IS_ICONDITIONS_TAB( window ));
	nact_iconditions_tab_runtime_init( NACT_WINDOW( window ));

	g_assert( NACT_IS_IADVANCED_TAB( window ));
	nact_iadvanced_tab_runtime_init( NACT_WINDOW( window ));
}

static void
setup_dialog_title( NactMainWindow *window )
{
	BaseApplication *appli = BASE_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	gchar *title = base_application_get_name( appli );

	if( window->private->edited_action ){
		gchar *label = na_action_get_label( window->private->edited_action );
		gchar *tmp = g_strdup_printf( "%s - %s", title, label );
		g_free( label );
		g_free( title );
		title = tmp;
	}

	if( count_modified_actions( window )){
		gchar *tmp = g_strdup_printf( "*%s", title );
		g_free( title );
		title = tmp;
	}

	GtkWindow *toplevel = base_window_get_toplevel_dialog( BASE_WINDOW( window ));
	gtk_window_set_title( toplevel, title );

	g_free( title );
}

static void
setup_dialog_menu( NactMainWindow *window )
{
	GSList *ia;
	gboolean to_save = FALSE;
	for( ia = window->private->actions ; ia && !to_save ; ia = ia->next ){
		gboolean elt_to_save = is_action_to_save( NA_ACTION( ia->data ));
		to_save |= elt_to_save;
	}

	gtk_widget_set_sensitive(  window->private->new_profile_item, window->private->edited_action != NULL );

	gtk_widget_set_sensitive( window->private->save_item, to_save );
}

/*
 * note that the IActionsList tree store may return an action or a profile
 */
static void
on_actions_list_selection_changed( GtkTreeSelection *selection, gpointer user_data )
{
	g_assert( NACT_IS_MAIN_WINDOW( user_data ));
	NactMainWindow *window = NACT_MAIN_WINDOW( user_data );

	NAObject *object = nact_iactions_list_get_selected_action( NACT_WINDOW( window ));

	if( NA_IS_ACTION( object )){
		window->private->edited_action = NA_ACTION( object );
		set_current_action( window );

	} else {
		g_assert( NA_IS_ACTION_PROFILE( object ));
		window->private->edited_profile = NA_ACTION_PROFILE( object );
		set_current_profile( window );
	}
}

static gboolean
on_actions_list_double_click( GtkWidget *widget, GdkEventButton *event, gpointer user_data )
{
	g_assert( event->type == GDK_2BUTTON_PRESS );

	/*on_edit_button_clicked( NULL, user_data );*/

	return( TRUE );
}

static gboolean
on_actions_list_enter_key_pressed( GtkWidget *widget, GdkEventKey *event, gpointer user_data )
{
	/*on_edit_button_clicked( NULL, user_data );*/

	return( TRUE );
}

/*
 * update the notebook when selection changes in IActionsList
 * if there is only one profile, we also setup the profile
 */
static void
set_current_action( NactMainWindow *window )
{
	g_debug( "set_current_action: current=%p", window->private->edited_action );

	nact_iaction_tab_set_action( NACT_WINDOW( window ), window->private->edited_action );

	window->private->edited_profile = NULL;
	if( na_action_get_profiles_count( window->private->edited_action ) == 1 ){
		window->private->edited_profile = NA_ACTION_PROFILE( na_action_get_profiles( window->private->edited_action )->data );
	}

	set_current_profile( window );
}

static void
set_current_profile( NactMainWindow *window )
{
	if( window->private->edited_profile ){
		NAAction *action = NA_ACTION( na_action_profile_get_action( window->private->edited_profile ));
		if( action != window->private->edited_action ){
			window->private->edited_action = action;
			nact_iaction_tab_set_action( NACT_WINDOW( window ), window->private->edited_action );
		}
	}

	nact_icommand_tab_set_profile( NACT_WINDOW( window ), window->private->edited_profile );
	nact_iconditions_tab_set_profile( NACT_WINDOW( window ), window->private->edited_profile );
	nact_iadvanced_tab_set_profile( NACT_WINDOW( window ), window->private->edited_profile );
}

/*
 * update the currently edited NAAction when a field is modified
 * (called as a virtual function by each interface tab)
 */
static NAAction *
get_edited_action( NactWindow *window )
{
	g_assert( NACT_IS_MAIN_WINDOW( window ));
	return( NACT_MAIN_WINDOW( window )->private->edited_action );
}

static NAActionProfile *
get_edited_profile( NactWindow *window )
{
	g_assert( NACT_IS_MAIN_WINDOW( window ));
	return( NACT_MAIN_WINDOW( window )->private->edited_profile );
}

/*
 * called as a virtual function by each interface tab when a field
 * has been modified : time to set the 'modified' flag in the
 * IActionsList box
 */
static void
on_modified_field( NactWindow *window )
{
	g_assert( NACT_IS_MAIN_WINDOW( window ));

	check_edited_status( window, NACT_MAIN_WINDOW( window )->private->edited_action );

	setup_dialog_title( NACT_MAIN_WINDOW( window ));
	setup_dialog_menu( NACT_MAIN_WINDOW( window ));
}

static void
check_edited_status( NactWindow *window, const NAAction *edited )
{
	g_assert( edited );

	NactApplication *application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	NAPivot *pivot = NA_PIVOT( nact_application_get_pivot( application ));
	gchar *uuid = na_action_get_uuid( edited );
	NAAction *original = NA_ACTION( na_pivot_get_action( pivot, uuid ));
	g_free( uuid );
	gboolean is_modified = !na_action_are_equal( edited, original );
	g_object_set_data( G_OBJECT( edited ), "nact-main-window-action-modified", GINT_TO_POINTER( is_modified ));

	/*gboolean check = is_action_modified( edited );
	g_debug( "check_edited_status: edited=%p, is_modified=%s, check=%s", edited, is_modified ? "True":"False", check ? "True":"False" );*/

	gboolean can_save = FALSE;
	if( is_modified ){
		gchar *label = na_action_get_label( edited );
		if( label && g_utf8_strlen( label, -1 )){
			GSList *ip;
			GSList *profiles = na_action_get_profiles( edited );
			for( ip = profiles ; ip ; ip = ip->next ){
				NAActionProfile *prof = NA_ACTION_PROFILE( ip->data );
				gchar *prof_label = na_action_profile_get_label( prof );
				if( prof_label && g_utf8_strlen( prof_label, -1 )){
					can_save = TRUE;
				}
				g_free( prof_label );
			}
			g_free( label );
		}
	}
	g_object_set_data( G_OBJECT( edited ), "nact-main-window-action-can-save", GINT_TO_POINTER( can_save));
}

static gboolean
is_action_modified( const NAAction *action )
{
	return( GPOINTER_TO_INT( g_object_get_data( G_OBJECT( action ), "nact-main-window-action-modified" )));
}

static gboolean
is_action_to_save( const NAAction *action )
{
	return( GPOINTER_TO_INT( g_object_get_data( G_OBJECT( action ), "nact-main-window-action-can-save" )));
}

static void
get_isfiledir( NactWindow *window, gboolean *isfile, gboolean *isdir )
{

}

static gboolean
get_multiple( NactWindow *window )
{
	return( FALSE );
}

static GSList *
get_schemes( NactWindow *window )
{
	return( NULL );
}

static void
on_new_action_activated( GtkMenuItem *item, gpointer user_data )
{

}

static void
on_new_action_selected( GtkItem *item, gpointer user_data )
{
	g_assert( NACT_IS_MAIN_WINDOW( user_data ));
	NactMainWindow *window = NACT_MAIN_WINDOW( user_data );
	gtk_statusbar_push(
			window->private->status_bar,
			window->private->status_context,
			/* i18n: tooltip displayed in the status bar when selecting the 'New action' item */
			_( "Define a new action" ));
}

static void
on_new_profile_activated( GtkMenuItem *item, gpointer user_data )
{

}

static void
on_new_profile_selected( GtkItem *item, gpointer user_data )
{
	g_assert( NACT_IS_MAIN_WINDOW( user_data ));
	NactMainWindow *window = NACT_MAIN_WINDOW( user_data );
	gtk_statusbar_push(
			window->private->status_bar,
			window->private->status_context,
			/* i18n: tooltip displayed in the status bar when selecting the 'New profile' item */
			_( "Define a new profile attached to the current action" ));
}

static void
on_import_activated( GtkMenuItem *item, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_import_activated";
	g_debug( "%s: item=%p, user_data=%p", thisfn, item, user_data );

	nact_assist_import_run( NACT_WINDOW( user_data ));

	/*g_assert( NACT_IS_MAIN_WINDOW( user_data ));
	NactWindow *wndmain = NACT_WINDOW( user_data );
	nact_iactions_list_set_focus( wndmain );*/
}

static void
on_import_selected( GtkItem *item, gpointer user_data )
{
	g_assert( NACT_IS_MAIN_WINDOW( user_data ));
	NactMainWindow *window = NACT_MAIN_WINDOW( user_data );
	gtk_statusbar_push(
			window->private->status_bar,
			window->private->status_context,
			/* i18n: tooltip displayed in the status bar when selecting the Import item */
			_( "Import one or more actions from external (XML) files into your configuration" ));
}

static void
on_export_activated( GtkMenuItem *item, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_export_activated";
	g_debug( "%s: item=%p, user_data=%p", thisfn, item, user_data );

	nact_assist_export_run( NACT_WINDOW( user_data ));

	/*g_assert( NACT_IS_MAIN_WINDOW( user_data ));
	NactWindow *wndmain = NACT_WINDOW( user_data );
	nact_iactions_list_set_focus( wndmain );*/
}

static void
on_export_selected( GtkItem *item, gpointer user_data )
{
	g_assert( NACT_IS_MAIN_WINDOW( user_data ));
	NactMainWindow *window = NACT_MAIN_WINDOW( user_data );
	gtk_statusbar_push(
			window->private->status_bar,
			window->private->status_context,
			/* i18n: tooltip displayed in the status bar when selecting the Export item */
			_( "Export one or more actions from your configuration to external XML files" ));
}

/* TODO: make the website url and the mail addresses clickables
 */
static void
on_about_activated( GtkMenuItem *item, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_about_activated";
	g_debug( "%s: item=%p, user_data=%p", thisfn, item, user_data );

	g_assert( NACT_IS_MAIN_WINDOW( user_data ));
	NactMainWindow *window = NACT_MAIN_WINDOW( user_data );

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
			"comments", _( "A graphical tool to create and edit your Nautilus actions." ),
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
on_about_selected( GtkItem *item, gpointer user_data )
{
	g_assert( NACT_IS_MAIN_WINDOW( user_data ));
	NactMainWindow *window = NACT_MAIN_WINDOW( user_data );
	gtk_statusbar_push(
			window->private->status_bar,
			window->private->status_context,
			/* i18n: tooltip displayed in the status bar when selecting the About item */
			_( "Display informations about this program" ));
}

static void
on_menu_item_deselected( GtkItem *item, gpointer user_data )
{
	g_assert( NACT_IS_MAIN_WINDOW( user_data ));
	NactMainWindow *window = NACT_MAIN_WINDOW( user_data );
	gtk_statusbar_pop( window->private->status_bar, window->private->status_context );
}

/*
 * creating a new action
 * pwi 2009-05-19
 * I don't want the profiles feature spread wide while I'm not convinced
 * that it is useful and actually used.
 * so the new action is silently created with a default profile name
 */
/*static void
on_new_button_clicked( GtkButton *button, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_new_button_clicked";
	g_debug( "%s: button=%p, user_data=%p", thisfn, button, user_data );

	g_assert( NACT_IS_MAIN_WINDOW( user_data ));
	NactWindow *wndmain = NACT_WINDOW( user_data );

	nact_action_conditions_editor_run_editor( wndmain, NULL );

	nact_iactions_list_set_focus( wndmain );
}*/

/*
 * editing an existing action
 * pwi 2009-05-19
 * I don't want the profiles feature spread wide while I'm not convinced
 * that it is useful and actually used.
 * so :
 * - if there is only one profile, the user will be directed to a dialog
 *   box which includes all needed fields, but without any profile notion
 * - if there are more than one profile, one can assume that the user has
 *   found a use to the profiles, and let him edit them
 */
/*static void
on_edit_button_clicked( GtkButton *button, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_edit_button_clicked";
	g_debug( "%s: button=%p, user_data=%p", thisfn, button, user_data );

	g_assert( NACT_IS_MAIN_WINDOW( user_data ));
	NactWindow *wndmain = NACT_WINDOW( user_data );

	NAAction *action = NA_ACTION( nact_iactions_list_get_selected_action( wndmain ));
	g_assert( action );
	g_assert( NA_IS_ACTION( action ));

	if( na_action_get_profiles_count( action ) > 1 ){
		nact_action_profiles_editor_run_editor( wndmain, action );
	} else {
		nact_action_conditions_editor_run_editor( wndmain, action );
	}

	nact_iactions_list_set_focus( wndmain );
}*/

/*static void
on_duplicate_button_clicked( GtkButton *button, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_duplicate_button_clicked";
	g_debug( "%s: button=%p, user_data=%p", thisfn, button, user_data );

	g_assert( NACT_IS_MAIN_WINDOW( user_data ));
	NactWindow *wndmain = NACT_WINDOW( user_data );

	NAAction *action = NA_ACTION( nact_iactions_list_get_selected_action( wndmain ));
	if( action ){

		NAAction *duplicate = na_action_duplicate( action );
		na_action_set_new_uuid( duplicate );
		gchar *label = na_action_get_label( action );
		gchar *label2 = g_strdup_printf( _( "Copy of %s"), label );
		na_action_set_label( duplicate, label2 );
		g_free( label2 );

		gchar *msg = NULL;
		NAPivot *pivot = NA_PIVOT( nact_window_get_pivot( wndmain ));
		if( na_pivot_write_action( pivot, G_OBJECT( duplicate ), &msg ) != NA_IIO_PROVIDER_WRITE_OK ){

			gchar *first = g_strdup_printf( _( "Unable to duplicate \"%s\" action." ), label );
			base_window_error_dlg( BASE_WINDOW( wndmain ), GTK_MESSAGE_ERROR, first, msg );
			g_free( first );
			g_free( msg );

		} else {
			do_set_current_action( NACT_WINDOW( wndmain ), duplicate );
		}

		g_object_unref( duplicate );
		g_free( label );

	} else {
		g_assert_not_reached();
	}

	nact_iactions_list_set_focus( wndmain );
}*/

/*static void
on_delete_button_clicked( GtkButton *button, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_delete_button_clicked";
	g_debug( "%s: button=%p, user_data=%p", thisfn, button, user_data );

	g_assert( NACT_IS_MAIN_WINDOW( user_data ));
	NactWindow *wndmain = NACT_WINDOW( user_data );

	NAAction *action = NA_ACTION( nact_iactions_list_get_selected_action( wndmain ));
	if( action ){

		gchar *label = na_action_get_label( action );
		gchar *sure = g_strdup_printf( _( "Are you sure you want to delete \"%s\" action ?" ), label );
		if( base_window_yesno_dlg( BASE_WINDOW( wndmain ), GTK_MESSAGE_WARNING, sure, NULL )){

			gchar *msg = NULL;
			NAPivot *pivot = NA_PIVOT( nact_window_get_pivot( wndmain ));
			if( na_pivot_delete_action( pivot, G_OBJECT( action ), &msg ) != NA_IIO_PROVIDER_WRITE_OK ){

				gchar *first = g_strdup_printf( _( "Unable to delete \"%s\" action." ), label );
				base_window_error_dlg( BASE_WINDOW( wndmain ), GTK_MESSAGE_ERROR, first, msg );
				g_free( first );
				g_free( msg );
			}
		}
		g_free( sure );
		g_free( label );

	} else {
		g_assert_not_reached();
	}

	nact_iactions_list_set_focus( wndmain );
}*/

/*static gboolean
on_dialog_response( GtkDialog *dialog, gint response_id, BaseWindow *window )
{
	static const gchar *thisfn = "nact_main_window_on_dialog_response";
	g_debug( "%s: dialog=%p, response_id=%d, window=%p", thisfn, dialog, response_id, window );
	g_assert( NACT_IS_MAIN_WINDOW( window ));
*/
	/*GtkWidget *paste_button = nact_get_glade_widget_from ("PasteProfileButton", GLADE_EDIT_DIALOG_WIDGET);*/

	/*switch( response_id ){
		case GTK_RESPONSE_NONE:
		case GTK_RESPONSE_DELETE_EVENT:
		case GTK_RESPONSE_CLOSE:*/
			/* Free any profile in the clipboard */
			/*nautilus_actions_config_action_profile_free (g_object_steal_data (G_OBJECT (paste_button), "profile"));*/

			/*g_object_unref( window );
			return( TRUE );
			break;
	}

	return( FALSE );
}*/

static void
on_close( GtkMenuItem *item, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_close";
	g_debug( "%s: item=%p, user_data=%p", thisfn, item, user_data );

	g_assert( NACT_IS_MAIN_WINDOW( user_data ));
	NactMainWindow *window = NACT_MAIN_WINDOW( user_data );

	gint count = count_modified_actions( window );
	if( !count || nact_window_warn_count_modified( NACT_WINDOW( window ), count )){
		g_object_unref( window );
	}
}

static gboolean
on_delete_event( BaseWindow *window, GtkWindow *toplevel, GdkEvent *event )
{
	static const gchar *thisfn = "nact_main_window_on_delete_event";
	g_debug( "%s: window=%p, toplevel=%p, event=%p", thisfn, window, toplevel, event );

	g_assert( NACT_IS_MAIN_WINDOW( window ));

	on_close( NULL, window );

	return( TRUE );
}

static gint
count_modified_actions( NactMainWindow *window )
{
	gint count = 0;
	GSList *ia;
	for( ia = window->private->actions ; ia ; ia = ia->next ){
		gboolean is_modified = is_action_modified( NA_ACTION( ia->data ));
		if( is_modified ){
			count += 1;
		}
		/*g_debug( "count_modified_actions: action=%p, is_modified=%s", ia->data, is_modified ? "True":"False" );*/
	}
	/*g_debug( "count_modified_actions: length=%d, count=%d", g_slist_length( window->private->actions ), count );*/
	return( count );
}

/*
 * called by NAPivot because this window implements the IIOContainer
 * interface, i.e. it wish to be advertised when the list of actions
 * changes in the underlying I/O storage subsystem (typically, when we
 * save the modifications)
 */
static void
on_actions_changed( NAIPivotContainer *instance, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_actions_changed";
	g_debug( "%s: instance=%p, user_data=%p", thisfn, instance, user_data );

	g_assert( NACT_IS_MAIN_WINDOW( instance ));
	NactMainWindow *self = NACT_MAIN_WINDOW( instance );

	if( !self->private->dispose_has_run ){
		nact_iactions_list_fill( NACT_WINDOW( instance ));
	}

	/*nact_iactions_list_set_selection(
			NACT_WINDOW( self ), self->private->current_uuid, self->private->current_label );*/
}
