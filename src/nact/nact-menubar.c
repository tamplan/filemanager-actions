/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011 Pierre Wieser and others (see AUTHORS)
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

#include "nact-menubar.h"

/* *** */
#include <api/na-object-api.h>
#include <api/na-core-utils.h>

#include <core/na-factory-object.h>
#include <core/na-iprefs.h>
#include <core/na-ipivot-consumer.h>
#include <core/na-io-provider.h>

#include "nact-application.h"
#include "nact-iactions-list.h"
#include "nact-clipboard.h"
#include "nact-main-statusbar.h"
#include "nact-main-toolbar.h"
#include "nact-main-tab.h"
#include "nact-menubar.h"
#include "nact-main-menubar-file.h"
#include "nact-main-menubar-edit.h"
#include "nact-main-menubar-view.h"
#include "nact-main-menubar-tools.h"
#include "nact-main-menubar-maintainer.h"
#include "nact-main-menubar-help.h"
#include "nact-sort-buttons.h"

enum {
	MENUBAR_FILE_TOOLBAR_POS = 0,
	MENUBAR_EDIT_TOOLBAR_POS,
	MENUBAR_TOOLS_TOOLBAR_POS,
	MENUBAR_HELP_TOOLBAR_POS
};

static void     on_iactions_list_count_updated( NactMainWindow *window, gint menus, gint actions, gint profiles );
static void     on_iactions_list_selection_changed( NactMainWindow *window, GList *selected );
static void     on_iactions_list_focus_in( NactMainWindow *window, gpointer user_data );
static void     on_iactions_list_focus_out( NactMainWindow *window, gpointer user_data );
static void     on_iactions_list_status_changed( NactMainWindow *window, gpointer user_data );
static void     on_level_zero_order_changed( NactMainWindow *window, gpointer user_data );
static void     on_update_sensitivities( NactMainWindow *window, gpointer user_data );

static void     on_popup_selection_done(GtkMenuShell *menushell, NactMainWindow *window );
/* *** */

/* private class data
 */
struct _NactMenubarClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

struct _NactMenubarPrivate {
	gboolean        dispose_has_run;
	BaseWindow     *window;
	GtkUIManager   *ui_manager;
	GtkActionGroup *action_group;
};

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
				G_CALLBACK( nact_main_menubar_file_on_new_menu ) },
		{ "NewActionItem", GTK_STOCK_NEW, N_( "_New action" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the 'New action' item */
				N_( "Define a new action" ),
				G_CALLBACK( nact_main_menubar_file_on_new_action ) },
		{ "NewProfileItem", NULL, N_( "New _profile" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the 'New profile' item */
				N_( "Define a new profile attached to the current action" ),
				G_CALLBACK( nact_main_menubar_file_on_new_profile ) },
		{ "SaveItem", GTK_STOCK_SAVE, NULL, NULL,
				/* i18n: tooltip displayed in the status bar when selecting 'Save' item */
				N_( "Record all the modified actions. Invalid actions will be silently ignored" ),
				G_CALLBACK( nact_main_menubar_file_on_save ) },
		{ "QuitItem", GTK_STOCK_QUIT, NULL, NULL,
				/* i18n: tooltip displayed in the status bar when selecting 'Quit' item */
				N_( "Quit the application" ),
				G_CALLBACK( nact_main_menubar_file_on_quit ) },
		{ "CutItem" , GTK_STOCK_CUT, NULL, NULL,
				/* i18n: tooltip displayed in the status bar when selecting the Cut item */
				N_( "Cut the selected item(s) to the clipboard" ),
				G_CALLBACK( nact_main_menubar_edit_on_cut ) },
		{ "CopyItem" , GTK_STOCK_COPY, NULL, NULL,
				/* i18n: tooltip displayed in the status bar when selecting the Copy item */
				N_( "Copy the selected item(s) to the clipboard" ),
				G_CALLBACK( nact_main_menubar_edit_on_copy ) },
		{ "PasteItem" , GTK_STOCK_PASTE, NULL, NULL,
				/* i18n: tooltip displayed in the status bar when selecting the Paste item */
				N_( "Insert the content of the clipboard just before the current position" ),
				G_CALLBACK( nact_main_menubar_edit_on_paste ) },
		{ "PasteIntoItem" , NULL, N_( "Paste _into" ), "<Shift><Ctrl>V",
				/* i18n: tooltip displayed in the status bar when selecting the Paste Into item */
				N_( "Insert the content of the clipboard as first child of the current item" ),
				G_CALLBACK( nact_main_menubar_edit_on_paste_into ) },
		{ "DuplicateItem" , NULL, N_( "D_uplicate" ), "",
				/* i18n: tooltip displayed in the status bar when selecting the Duplicate item */
				N_( "Duplicate the selected item(s)" ),
				G_CALLBACK( nact_main_menubar_edit_on_duplicate ) },
		{ "DeleteItem", GTK_STOCK_DELETE, NULL, "Delete",
				/* i18n: tooltip displayed in the status bar when selecting the Delete item */
				N_( "Delete the selected item(s)" ),
				G_CALLBACK( nact_main_menubar_edit_on_delete ) },
		{ "ReloadActionsItem", GTK_STOCK_REFRESH, N_( "_Reload the items" ), "F5",
				/* i18n: tooltip displayed in the status bar when selecting the 'Reload' item */
				N_( "Cancel your current modifications and reload the initial list of menus and actions" ),
				G_CALLBACK( nact_main_menubar_edit_on_reload ) },
		{ "PreferencesItem", GTK_STOCK_PREFERENCES, NULL, NULL,
				/* i18n: tooltip displayed in the status bar when selecting the 'Preferences' item */
				N_( "Edit your preferences" ),
				G_CALLBACK( nact_main_menubar_edit_on_prefererences ) },
		{ "ExpandAllItem" , NULL, N_( "_Expand all" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the Expand all item */
				N_( "Entirely expand the items hierarchy" ),
				G_CALLBACK( nact_main_menubar_view_on_expand_all ) },
		{ "CollapseAllItem" , NULL, N_( "_Collapse all" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the Collapse all item */
				N_( "Entirely collapse the items hierarchy" ),
				G_CALLBACK( nact_main_menubar_view_on_collapse_all ) },

		{ "ImportItem" , GTK_STOCK_CONVERT, N_( "_Import assistant..." ), "",
				/* i18n: tooltip displayed in the status bar when selecting the Import item */
				N_( "Import one or more actions from external (XML) files into your configuration" ),
				G_CALLBACK( nact_main_menubar_tools_on_import ) },
		{ "ExportItem", NULL, N_( "E_xport assistant..." ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the Export item */
				N_( "Export one or more actions from your configuration to external XML files" ),
				G_CALLBACK( nact_main_menubar_tools_on_export ) },

		{ "DumpSelectionItem", NULL, N_( "_Dump the selection" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the Dump selection item */
				N_( "Recursively dump selected items" ),
				G_CALLBACK( nact_main_menubar_maintainer_on_dump_selection ) },
		{ "BriefTreeStoreDumpItem", NULL, N_( "_Brief tree store dump" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the BriefTreeStoreDump item */
				N_( "Briefly dump the tree store" ),
				G_CALLBACK( nact_main_menubar_maintainer_on_brief_tree_store_dump ) },
		{ "ListModifiedItems", NULL, N_( "_List modified items" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the ListModifiedItems item */
				N_( "List the modified items" ),
				G_CALLBACK( nact_main_menubar_maintainer_on_list_modified_items ) },
		{ "DumpClipboard", NULL, N_( "_Dump the clipboard" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the DumpClipboard item */
				N_( "Dump the content of the clipboard object" ),
				G_CALLBACK( nact_main_menubar_maintainer_on_dump_clipboard ) },

		{ "HelpItem" , GTK_STOCK_HELP, N_( "Contents" ), "F1",
				/* i18n: tooltip displayed in the status bar when selecting the Help item */
				N_( "Display help about this program" ),
				G_CALLBACK( nact_main_menubar_help_on_help ) },
		{ "AboutItem", GTK_STOCK_ABOUT, NULL, NULL,
				/* i18n: tooltip displayed in the status bar when selecting the About item */
				N_( "Display informations about this program" ),
				G_CALLBACK( nact_main_menubar_help_on_about ) },
};

static const GtkToggleActionEntry toolbar_entries[] = {

		{ "ViewFileToolbarItem", NULL, N_( "_File" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the 'View File toolbar' item */
				N_( "Display the File toolbar" ),
				G_CALLBACK( nact_main_menubar_view_on_toolbar_file ), FALSE },
		{ "ViewEditToolbarItem", NULL, N_( "_Edit" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the 'View Edit toolbar' item */
				N_( "Display the Edit toolbar" ),
				G_CALLBACK( nact_main_menubar_view_on_toolbar_edit ), FALSE },
		{ "ViewToolsToolbarItem", NULL, N_( "_Tools" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting 'View Tools toolbar' item */
				N_( "Display the Tools toolbar" ),
				G_CALLBACK( nact_main_menubar_view_on_toolbar_tools ), FALSE },
		{ "ViewHelpToolbarItem", NULL, N_( "_Help" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting 'View Help toolbar' item */
				N_( "Display the Help toolbar" ),
				G_CALLBACK( nact_main_menubar_view_on_toolbar_help ), FALSE },
};

/* GtkActivatable
 * gtk_action_get_tooltip() is only available starting with Gtk 2.16
 * until this is a required level, we must have some code to do the
 * same thing
 */
#undef NA_HAS_GTK_ACTIVATABLE
#if GTK_CHECK_VERSION( 2,16,0 )
	#define NA_HAS_GTK_ACTIVATABLE
#endif

#ifndef NA_HAS_GTK_ACTIVATABLE
#define MENUBAR_PROP_ITEM_ACTION			"menubar-item-action"
#endif

#define MENUBAR_PROP_STATUS_CONTEXT			"menubar-status-context"
#define MENUBAR_PROP_MAIN_STATUS_CONTEXT	"menubar-main-status-context"

static const gchar  *st_ui_menubar_actions    = PKGDATADIR "/nautilus-actions-config-tool.actions";
static const gchar  *st_ui_maintainer_actions = PKGDATADIR "/nautilus-actions-maintainer.actions";

static GObjectClass *st_parent_class          = NULL;

static GType    register_type( void );
static void     class_init( NactMenubarClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_dispose( GObject *application );
static void     instance_finalize( GObject *application );

static void     on_base_initialize_window( BaseWindow *window, gpointer user_data );
static void     on_ui_manager_proxy_connect( GtkUIManager *ui_manager, GtkAction *action, GtkWidget *proxy, BaseWindow *window );
static void     on_menu_item_selected( GtkMenuItem *proxy, NactMainWindow *window );
static void     on_menu_item_deselected( GtkMenuItem *proxy, NactMainWindow *window );
static void     on_finalize_window( NactMenubar *bar, GObject *window );

GType
nact_menubar_get_type( void )
{
	static GType type = 0;

	if( !type ){
		type = register_type();
	}

	return( type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "nact_menubar_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NactMenubarClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactMenubar ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "NactMenubar", &info, 0 );

	return( type );
}

static void
class_init( NactMenubarClass *klass )
{
	static const gchar *thisfn = "nact_menubar_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactMenubarClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_menubar_instance_init";
	NactMenubar *self;

	g_return_if_fail( NACT_IS_MENUBAR( instance ));

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self = NACT_MENUBAR( instance );

	self->private = g_new0( NactMenubarPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "nact_menubar_instance_dispose";
	NactMenubar *self;

	g_return_if_fail( NACT_IS_MENUBAR( object ));

	self = NACT_MENUBAR( object );

	if( !self->private->dispose_has_run ){
		g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

		self->private->dispose_has_run = TRUE;

		g_object_unref( self->private->action_group );
		g_object_unref( self->private->ui_manager );

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *instance )
{
	static const gchar *thisfn = "nact_menubar_instance_finalize";
	NactMenubar *self;
	MenubarIndicatorsStruct *mis;

	g_return_if_fail( NACT_IS_MENUBAR( instance ));

	g_debug( "%s: instance=%p (%s)", thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	self = NACT_MENUBAR( instance );

	mis = ( MenubarIndicatorsStruct * ) g_object_get_data( G_OBJECT( self->private->window ), MENUBAR_PROP_INDICATORS );
	g_free( mis );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( instance );
	}
}

/**
 * nact_menubar_new:
 * @window: the main window which embeds the menubar, usually the #NactMainWindow.
 *
 * The created menubar attachs itself to the @window; it also connect a weak
 * reference to this same @window, thus automatically g_object_unref() -ing
 * itself at @window finalization time.
 *
 * The menubar also takes advantage of #BaseWindow messages to initialize
 * its Gtk widgets.
 *
 * Returns: a new #NactMenubar object.
 */
NactMenubar *
nact_menubar_new( BaseWindow *window )
{
	NactMenubar *bar;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), NULL );

	bar = g_object_new( NACT_MENUBAR_TYPE, NULL );

	bar->private->window = window;

	base_window_signal_connect( window,
			G_OBJECT( window ), BASE_SIGNAL_INITIALIZE_WINDOW, G_CALLBACK( on_base_initialize_window ));

	g_object_weak_ref( G_OBJECT( window ), ( GWeakNotify ) on_finalize_window, bar );

	g_object_set_data( G_OBJECT( window ), WINDOW_DATA_MENUBAR, bar );

	return( bar );
}

static void
on_base_initialize_window( BaseWindow *window, gpointer user_data )
{
	static const gchar *thisfn = "nact_menubar_on_base_initialize_window";
	GError *error;
	guint merge_id;
	GtkAccelGroup *accel_group;
	GtkWidget *menubar, *vbox;
	GtkWindow *toplevel;
	MenubarIndicatorsStruct *mis;
	gboolean has_maintainer_menu;

	BAR_WINDOW_VOID( window );

	if( !bar->private->dispose_has_run ){
		g_debug( "%s: window=%p (%s), user_data=%p", thisfn,
				( void * ) window, G_OBJECT_TYPE_NAME( window ), ( void * ) user_data );

		/* create the menubar:
		 * - create action group, and insert list of actions in it
		 * - create the ui manager, and insert action group in it
		 * - merge inserted actions group with ui xml definition
		 * - install accelerators in the main window
		 * - pack menu bar in the main window
		 *
		 * "disconnect-proxy" signal is never triggered.
		 */
		bar->private->ui_manager = gtk_ui_manager_new();
		g_debug( "%s: ui_manager=%p", thisfn, ( void * ) bar->private->ui_manager );

		base_window_signal_connect( window,
				G_OBJECT( bar->private->ui_manager ),
				"connect-proxy", G_CALLBACK( on_ui_manager_proxy_connect ));

		bar->private->action_group = gtk_action_group_new( "MenubarActions" );
		g_debug( "%s: action_group=%p", thisfn, ( void * ) bar->private->action_group );

		gtk_action_group_set_translation_domain( bar->private->action_group, GETTEXT_PACKAGE );
		gtk_action_group_add_actions( bar->private->action_group, entries, G_N_ELEMENTS( entries ), window );
		gtk_action_group_add_toggle_actions( bar->private->action_group, toolbar_entries, G_N_ELEMENTS( toolbar_entries ), window );
		gtk_ui_manager_insert_action_group( bar->private->ui_manager, bar->private->action_group, 0 );

		error = NULL;
		merge_id = gtk_ui_manager_add_ui_from_file( bar->private->ui_manager, st_ui_menubar_actions, &error );
		if( merge_id == 0 || error ){
			g_warning( "%s: error=%s", thisfn, error->message );
			g_error_free( error );
		}

		has_maintainer_menu = FALSE;
#ifdef NA_MAINTAINER_MODE
		has_maintainer_menu = TRUE;
#endif

		if( has_maintainer_menu ){
			error = NULL;
			merge_id = gtk_ui_manager_add_ui_from_file( bar->private->ui_manager, st_ui_maintainer_actions, &error );
			if( merge_id == 0 || error ){
				g_warning( "%s: error=%s", thisfn, error->message );
				g_error_free( error );
			}
		}

		toplevel = base_window_get_gtk_toplevel( window );
		accel_group = gtk_ui_manager_get_accel_group( bar->private->ui_manager );
		gtk_window_add_accel_group( toplevel, accel_group );

		menubar = gtk_ui_manager_get_widget( bar->private->ui_manager, "/ui/MainMenubar" );
		vbox = base_window_get_widget( window, "MenubarVBox" );
		gtk_box_pack_start( GTK_BOX( vbox ), menubar, FALSE, FALSE, 0 );

		/* this creates a submenu in the toolbar */
		/*gtk_container_add( GTK_CONTAINER( vbox ), toolbar );*/

		base_window_signal_connect( window,
				G_OBJECT( window ), IACTIONS_LIST_SIGNAL_LIST_COUNT_UPDATED, G_CALLBACK( on_iactions_list_count_updated ));

		base_window_signal_connect( window,
				G_OBJECT( window ), IACTIONS_LIST_SIGNAL_SELECTION_CHANGED, G_CALLBACK( on_iactions_list_selection_changed ));

		base_window_signal_connect( window,
				G_OBJECT( window ), IACTIONS_LIST_SIGNAL_FOCUS_IN, G_CALLBACK( on_iactions_list_focus_in ));

		base_window_signal_connect( window,
				G_OBJECT( window ), IACTIONS_LIST_SIGNAL_FOCUS_OUT, G_CALLBACK( on_iactions_list_focus_out ));

		base_window_signal_connect( window,
				G_OBJECT( window ), IACTIONS_LIST_SIGNAL_STATUS_CHANGED, G_CALLBACK( on_iactions_list_status_changed ));

		base_window_signal_connect( window,
				G_OBJECT( window ), MAIN_WINDOW_SIGNAL_UPDATE_ACTION_SENSITIVITIES, G_CALLBACK( on_update_sensitivities ));

		base_window_signal_connect( window,
				G_OBJECT( window ), MAIN_WINDOW_SIGNAL_LEVEL_ZERO_ORDER_CHANGED, G_CALLBACK( on_level_zero_order_changed ));

		mis = g_new0( MenubarIndicatorsStruct, 1 );
		g_object_set_data( G_OBJECT( window ), MENUBAR_PROP_INDICATORS, mis );

		nact_main_toolbar_init(( NactMainWindow * ) window, bar->private->action_group );
	}
}

/*
 * action: GtkAction, GtkToggleAction
 * proxy:  GtkImageMenuItem, GtkCheckMenuItem, GtkToolButton
 */
static void
on_ui_manager_proxy_connect( GtkUIManager *ui_manager, GtkAction *action, GtkWidget *proxy, BaseWindow *window )
{
	static const gchar *thisfn = "nact_menubar_on_ui_manager_proxy_connect";

	g_debug( "%s: ui_manager=%p (%s), action=%p (%s), proxy=%p (%s), window=%p (%s)",
			thisfn,
			( void * ) ui_manager, G_OBJECT_TYPE_NAME( ui_manager ),
			( void * ) action, G_OBJECT_TYPE_NAME( action ),
			( void * ) proxy, G_OBJECT_TYPE_NAME( proxy ),
			( void * ) window, G_OBJECT_TYPE_NAME( window ));

	if( GTK_IS_MENU_ITEM( proxy )){

		base_window_signal_connect( window,
				G_OBJECT( proxy ), "select", G_CALLBACK( on_menu_item_selected ));

		base_window_signal_connect( window,
				G_OBJECT( proxy ), "deselect", G_CALLBACK( on_menu_item_deselected ));

#ifndef NA_HAS_GTK_ACTIVATABLE
		g_object_set_data( G_OBJECT( proxy ), MENUBAR_PROP_ITEM_ACTION, action );
#endif
	}
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

#ifdef NA_HAS_GTK_ACTIVATABLE
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

#ifndef NA_HAS_GTK_ACTIVATABLE
	g_free( tooltip );
#endif
}

static void
on_menu_item_deselected( GtkMenuItem *proxy, NactMainWindow *window )
{
	nact_main_statusbar_hide_status( window, MENUBAR_PROP_STATUS_CONTEXT );
}

/*
 * triggered just before the NactMainWindow is finalized
 */
static void
on_finalize_window( NactMenubar *bar, GObject *window )
{
	static const gchar *thisfn = "nact_menubar_on_finalize_window";

	g_return_if_fail( NACT_IS_MENUBAR( bar ));

	g_debug( "%s: bar=%p (%s), window=%p", thisfn,
			( void * ) bar, G_OBJECT_TYPE_NAME( bar ), ( void * ) window );

	g_object_unref( bar );
}

/**
 * nact_menubar_get_ui_manager:
 * @bar: this #NactMenubar instance.
 *
 * Returns: the attached GtkUIManager.
 */
GtkUIManager *
nact_menubar_get_ui_manager( const NactMenubar *bar )
{
	g_return_val_if_fail( NACT_IS_MENUBAR( bar ), NULL );

	return( bar->private->dispose_has_run ? NULL : bar->private->ui_manager );
}

/**
 * nact_main_menubar_is_level_zero_order_changed:
 * @window: the #NactMainWindow main window.
 *
 * Returns: %TRUE if the level zero has changed, %FALSE else.
 */
gboolean
nact_main_menubar_is_level_zero_order_changed( const NactMainWindow *window )
{
	MenubarIndicatorsStruct *mis;

	mis = ( MenubarIndicatorsStruct * ) g_object_get_data( G_OBJECT( window ), MENUBAR_PROP_INDICATORS );

	return( mis->level_zero_order_changed );
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
	GtkWidget *menu;
	MenubarIndicatorsStruct *mis;

	BAR_WINDOW_VOID( instance );

	menu = gtk_ui_manager_get_widget( bar->private->ui_manager, "/ui/Popup" );

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
	mis->have_exportables = ( mis->list_menus + mis->list_actions > 0 );

	nact_sort_buttons_enable_buttons( window, mis->list_menus + mis->list_actions > 0 );

	/* i18n: note the space at the beginning of the sentence */
	status = g_strdup_printf( _( " %d menu(s), %d action(s), %d profile(s) are currently loaded" ), menus, actions, profiles );
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

	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	g_debug( "nact_main_menubar_on_level_zero_order_changed: change=%s", user_data ? "True":"False" );

	mis = ( MenubarIndicatorsStruct * ) g_object_get_data( G_OBJECT( window ), MENUBAR_PROP_INDICATORS );
	mis->level_zero_order_changed = GPOINTER_TO_INT( user_data );
	g_signal_emit_by_name( window, MAIN_WINDOW_SIGNAL_UPDATE_ACTION_SENSITIVITIES, NULL );
}

static void
on_update_sensitivities( NactMainWindow *window, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_menubar_on_update_sensitivities";
	NactApplication *application;
	MenubarIndicatorsStruct *mis;

	g_debug( "%s: window=%p, user_data=%p", thisfn, ( void * ) window, ( void * ) user_data );
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	mis = ( MenubarIndicatorsStruct * ) g_object_get_data( G_OBJECT( window ), MENUBAR_PROP_INDICATORS );

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	mis->updater = nact_application_get_updater( application );
	mis->is_level_zero_writable = na_iprefs_is_level_zero_writable( NA_PIVOT( mis->updater ));

	mis->has_writable_providers = nact_window_has_writable_providers( NACT_WINDOW( window ));
	g_debug( "%s: has_writable_providers=%s", thisfn, mis->has_writable_providers ? "True":"False" );

	mis->selected_items = nact_iactions_list_bis_get_selected_items( NACT_IACTIONS_LIST( window ));
	mis->count_selected = mis->selected_items ? g_list_length( mis->selected_items ) : 0;
	g_debug( "%s: count_selected=%d", thisfn, mis->count_selected );

	nact_main_menubar_file_on_update_sensitivities( window, user_data, mis );
	nact_main_menubar_edit_on_update_sensitivities( window, user_data, mis );
	nact_main_menubar_view_on_update_sensitivities( window, user_data, mis );
	nact_main_menubar_tools_on_update_sensitivities( window, user_data, mis );
	nact_main_menubar_maintainer_on_update_sensitivities( window, user_data, mis );
	nact_main_menubar_help_on_update_sensitivities( window, user_data, mis );

	na_object_unref_selected_items( mis->selected_items );
	mis->selected_items = NULL;
}

/**
 * nact_main_menubar_enable_item:
 * @window: the #NactMainWindow main application window.
 * @name: the name of the item in a menu.
 * @enabled: whether this item should be enabled or not.
 *
 * Enable/disable an item in an menu.
 */
void
nact_main_menubar_enable_item( NactMainWindow *window, const gchar *name, gboolean enabled )
{
	GtkAction *action;

	BAR_WINDOW_VOID( window );

	action = gtk_action_group_get_action( bar->private->action_group, name );
	gtk_action_set_sensitive( action, enabled );
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