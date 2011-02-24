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
#include <stdlib.h>

#include <api/na-object-api.h>
#include <api/na-timeout.h>

#include <core/na-iprefs.h>
#include <core/na-pivot.h>

#include "base-marshal.h"
#include "nact-iaction-tab.h"
#include "nact-icommand-tab.h"
#include "nact-ibasenames-tab.h"
#include "nact-imimetypes-tab.h"
#include "nact-ifolders-tab.h"
#include "nact-ischemes-tab.h"
#include "nact-icapabilities-tab.h"
#include "nact-ienvironment-tab.h"
#include "nact-iexecution-tab.h"
#include "nact-iproperties-tab.h"
#include "nact-main-tab.h"
#include "nact-main-statusbar.h"
#include "nact-main-window.h"
#include "nact-menubar.h"
#include "nact-tree-view.h"
#include "nact-confirm-logout.h"
#include "nact-sort-buttons.h"

/* private class data
 */
struct _NactMainWindowClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _NactMainWindowPrivate {
	gboolean         dispose_has_run;

	NAUpdater       *updater;

	/**
	 * Current action or menu.
	 *
	 * This is the action or menu which is displayed in tabs Action/Menu
	 * and Properties ; it may be different of the exact row being currently
	 * selected, e.g. when a sub-profile is edited.
	 *
	 * Can be null, and this implies that @current_profile is also null,
	 * e.g. when the list is empty or in the case of a multiple selection.
	 *
	 * 'editable' property is set on selection change;
	 * This is the actual current writability status of the item at this time.
	 */
	NAObjectItem    *current_item;
	gboolean         editable;
	guint            reason;

	/**
	 * Current profile.
	 *
	 * This is the profile which is displayed in tab Command;
	 * it may be different of the exact row being currently selected,
	 * e.g. when an action with only one profile is selected.
	 *
	 * Can be null if @current_item is a menu, or an action with more
	 * than one profile is selected, or the list is empty, or in the
	 * case of a multiple selection.
	 *
	 * In other words, it is not null if:
	 * a) a profile is selected,
	 * b) an action is selected and it has exactly one profile.
	 */
	NAObjectProfile *current_profile;

	/**
	 * Current context.
	 *
	 * This is the #NAIContext data which corresponds to @current_profile
	 * or @current_item, depending of which one is actually selected.
	 */
	NAIContext      *current_context;

	/**
	 * Some convenience objects and data.
	 */
	NactTreeView    *items_view;
	gboolean         is_tree_modified;
	NactClipboard   *clipboard;
	NactMenubar     *menubar;

	gulong           pivot_handler_id;
	NATimeout        pivot_timeout;
};

/* properties set against the main window
 * these are set on selection changes
 */
enum {
	MAIN_PROP_0 = 0,

	MAIN_PROP_ITEM_ID,
	MAIN_PROP_PROFILE_ID,
	MAIN_PROP_CONTEXT_ID,
	MAIN_PROP_EDITABLE_ID,
	MAIN_PROP_REASON_ID,

	MAIN_PROP_N_PROPERTIES
};

/* signals
 */
enum {
	MAIN_ITEM_UPDATED,
	TAB_ITEM_UPDATED,
	SELECTION_CHANGED,
	LAST_SIGNAL
};

static const gchar     *st_xmlui_filename         = PKGDATADIR "/nautilus-actions-config-tool.ui";
static const gchar     *st_toplevel_name          = "MainWindow";
static const gchar     *st_wsp_name               = NA_IPREFS_MAIN_WINDOW_WSP;

static gint             st_burst_timeout          = 2500;		/* burst timeout in msec */
static BaseWindowClass *st_parent_class           = NULL;
static gint             st_signals[ LAST_SIGNAL ] = { 0 };

static GType      register_type( void );
static void       class_init( NactMainWindowClass *klass );
static void       iaction_tab_iface_init( NactIActionTabInterface *iface );
static void       icommand_tab_iface_init( NactICommandTabInterface *iface );
static void       ibasenames_tab_iface_init( NactIBasenamesTabInterface *iface );
static void       imimetypes_tab_iface_init( NactIMimetypesTabInterface *iface );
static void       ifolders_tab_iface_init( NactIFoldersTabInterface *iface );
static void       ischemes_tab_iface_init( NactISchemesTabInterface *iface );
static void       icapabilities_tab_iface_init( NactICapabilitiesTabInterface *iface );
static void       ienvironment_tab_iface_init( NactIEnvironmentTabInterface *iface );
static void       iexecution_tab_iface_init( NactIExecutionTabInterface *iface );
static void       iproperties_tab_iface_init( NactIPropertiesTabInterface *iface );
static void       instance_init( GTypeInstance *instance, gpointer klass );
static void       instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void       instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void       instance_constructed( GObject *application );
static void       instance_dispose( GObject *application );
static void       instance_finalize( GObject *application );

static void       on_base_initialize_gtk_toplevel( NactMainWindow *window, GtkWindow *toplevel, gpointer user_data );
static void       on_base_initialize_base_window( NactMainWindow *window, gpointer user_data );
static void       on_base_all_widgets_showed( NactMainWindow *window, gpointer user_data );

static void       on_block_items_changed_timeout( NactMainWindow *window );
static void       on_tree_view_modified_status_changed( NactMainWindow *window, gboolean is_modified, gpointer user_data );
static void       on_tree_view_selection_changed( NactMainWindow *window, GList *selected_items, gpointer user_data );
static void       on_selection_changed_cleanup_handler( BaseWindow *window, GList *selected_items );
static void       on_tab_updatable_item_updated( NactMainWindow *window, NAIContext *context, guint data, gpointer user_data );
static void       raz_selection_properties( NactMainWindow *window );
static void       setup_current_selection( NactMainWindow *window, NAObjectId *selected_row );
static void       setup_dialog_title( NactMainWindow *window );
static void       setup_writability_status( NactMainWindow *window );

/* items have changed */
static void       on_pivot_items_changed( NAUpdater *updater, NactMainWindow *window );
static gboolean   confirm_for_giveup_from_pivot( const NactMainWindow *window );
static gboolean   confirm_for_giveup_from_menu( const NactMainWindow *window );
static void       load_or_reload_items( NactMainWindow *window );

/* application termination */
static gboolean   on_base_is_willing_to_quit( const BaseWindow *window, gconstpointer user_data );
static gboolean   on_delete_event( GtkWidget *toplevel, GdkEvent *event, NactMainWindow *window );
static gboolean   warn_modified( NactMainWindow *window );


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
	GType type;

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

	static const GInterfaceInfo iaction_tab_iface_info = {
		( GInterfaceInitFunc ) iaction_tab_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo icommand_tab_iface_info = {
		( GInterfaceInitFunc ) icommand_tab_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo ibasenames_tab_iface_info = {
		( GInterfaceInitFunc ) ibasenames_tab_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo imimetypes_tab_iface_info = {
		( GInterfaceInitFunc ) imimetypes_tab_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo ifolders_tab_iface_info = {
		( GInterfaceInitFunc ) ifolders_tab_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo ischemes_tab_iface_info = {
		( GInterfaceInitFunc ) ischemes_tab_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo icapabilities_tab_iface_info = {
		( GInterfaceInitFunc ) icapabilities_tab_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo ienvironment_tab_iface_info = {
		( GInterfaceInitFunc ) ienvironment_tab_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo iexecution_tab_iface_info = {
		( GInterfaceInitFunc ) iexecution_tab_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo iproperties_tab_iface_info = {
		( GInterfaceInitFunc ) iproperties_tab_iface_init,
		NULL,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( BASE_WINDOW_TYPE, "NactMainWindow", &info, 0 );

	g_type_add_interface_static( type, NACT_IACTION_TAB_TYPE, &iaction_tab_iface_info );

	g_type_add_interface_static( type, NACT_ICOMMAND_TAB_TYPE, &icommand_tab_iface_info );

	g_type_add_interface_static( type, NACT_IBASENAMES_TAB_TYPE, &ibasenames_tab_iface_info );

	g_type_add_interface_static( type, NACT_IMIMETYPES_TAB_TYPE, &imimetypes_tab_iface_info );

	g_type_add_interface_static( type, NACT_IFOLDERS_TAB_TYPE, &ifolders_tab_iface_info );

	g_type_add_interface_static( type, NACT_ISCHEMES_TAB_TYPE, &ischemes_tab_iface_info );

	g_type_add_interface_static( type, NACT_ICAPABILITIES_TAB_TYPE, &icapabilities_tab_iface_info );

	g_type_add_interface_static( type, NACT_IENVIRONMENT_TAB_TYPE, &ienvironment_tab_iface_info );

	g_type_add_interface_static( type, NACT_IEXECUTION_TAB_TYPE, &iexecution_tab_iface_info );

	g_type_add_interface_static( type, NACT_IPROPERTIES_TAB_TYPE, &iproperties_tab_iface_info );

	return( type );
}

static void
class_init( NactMainWindowClass *klass )
{
	static const gchar *thisfn = "nact_main_window_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->set_property = instance_set_property;
	object_class->get_property = instance_get_property;
	object_class->constructed = instance_constructed;
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	g_object_class_install_property( object_class, MAIN_PROP_ITEM_ID,
			g_param_spec_pointer(
					MAIN_PROP_ITEM,
					"Current NAObjectItem",
					"A pointer to the currently edited NAObjectItem, an action or a menu",
					G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	g_object_class_install_property( object_class, MAIN_PROP_PROFILE_ID,
			g_param_spec_pointer(
					MAIN_PROP_PROFILE,
					"Current NAObjectProfile",
					"A pointer to the currently edited NAObjectProfile",
					G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	g_object_class_install_property( object_class, MAIN_PROP_CONTEXT_ID,
			g_param_spec_pointer(
					MAIN_PROP_CONTEXT,
					"Current NAIContext",
					"A pointer to the currently edited NAIContext",
					G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	g_object_class_install_property( object_class, MAIN_PROP_EDITABLE_ID,
			g_param_spec_boolean(
					MAIN_PROP_EDITABLE,
					"Editable item ?",
					"Whether the item will be able to be updated against its I/O provider", FALSE,
					G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	g_object_class_install_property( object_class, MAIN_PROP_REASON_ID,
			g_param_spec_int(
					MAIN_PROP_REASON,
					"No edition reason",
					"Why is this item not editable", 0, 255, 0,
					G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	klass->private = g_new0( NactMainWindowClassPrivate, 1 );

	/**
	 * NactMainWindow::main-item-updated:
	 *
	 * This signal is emitted on the BaseWindow when the item has been modified
	 * elsewhere that in a tab. The tabs should so update accordingly their
	 * widgets.
	 *
	 * Args:
	 * - an OR-ed list of the modified data, or 0 if not relevant.
	 */
	st_signals[ MAIN_ITEM_UPDATED ] = g_signal_new(
			MAIN_SIGNAL_ITEM_UPDATED,
			G_TYPE_OBJECT,
			G_SIGNAL_RUN_LAST,
			0,					/* no default handler */
			NULL,
			NULL,
			nact_cclosure_marshal_VOID__POINTER_UINT,
			G_TYPE_NONE,
			2,
			G_TYPE_POINTER,
			G_TYPE_UINT );

	/**
	 * nact-tab-updatable-item-updated:
	 *
	 * This signal is emitted by the notebook tabs, when any property
	 * of an item has been modified.
	 *
	 * Args:
	 * - an OR-ed list of the modified data, or 0 if not relevant.
	 *
	 * This main window is rather the only consumer of this message,
	 * does its tricks (title, etc.), and then reforward an item-updated
	 * message to IActionsList.
	 */
	st_signals[ TAB_ITEM_UPDATED ] = g_signal_new(
			TAB_UPDATABLE_SIGNAL_ITEM_UPDATED,
			G_TYPE_OBJECT,
			G_SIGNAL_RUN_LAST,
			0,					/* no default handler */
			NULL,
			NULL,
			nact_cclosure_marshal_VOID__POINTER_UINT,
			G_TYPE_NONE,
			2,
			G_TYPE_POINTER,
			G_TYPE_UINT );

	/**
	 * NactMainWindow::main-selection-changed:
	 *
	 * This signal is emitted on the window parent each time the selection
	 * has changed in the treeview, after having set the current item/profile/
	 * context properties.
	 *
	 * This way, we are sure that notebook edition tabs which required to
	 * have a current item/profile/context will have it, whenever they have
	 * connected to the 'selection-changed' signal.
	 *
	 * Signal args:
	 * - a #GList of currently selected #NAObjectItems.
	 *
	 * Handler prototype:
	 * void ( *handler )( BaseWindow *window, GList *selected, gpointer user_data );
	 */
	st_signals[ SELECTION_CHANGED ] = g_signal_new_class_handler(
			MAIN_SIGNAL_SELECTION_CHANGED,
			G_TYPE_OBJECT,
			G_SIGNAL_RUN_CLEANUP,
			G_CALLBACK( on_selection_changed_cleanup_handler ),
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER );
}

static void
iaction_tab_iface_init( NactIActionTabInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_iaction_tab_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );
}

static void
icommand_tab_iface_init( NactICommandTabInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_icommand_tab_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );
}

static void
ibasenames_tab_iface_init( NactIBasenamesTabInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_ibasenames_tab_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );
}

static void
imimetypes_tab_iface_init( NactIMimetypesTabInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_imimetypes_tab_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );
}

static void
ifolders_tab_iface_init( NactIFoldersTabInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_ifolders_tab_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );
}

static void
ischemes_tab_iface_init( NactISchemesTabInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_ischemes_tab_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );
}

static void
icapabilities_tab_iface_init( NactICapabilitiesTabInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_icapabilities_tab_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );
}

static void
ienvironment_tab_iface_init( NactIEnvironmentTabInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_ienvironment_tab_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );
}

static void
iexecution_tab_iface_init( NactIExecutionTabInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_iexecution_tab_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );
}

static void
iproperties_tab_iface_init( NactIPropertiesTabInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_iproperties_tab_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_main_window_instance_init";
	NactMainWindow *self;

	g_return_if_fail( NACT_IS_MAIN_WINDOW( instance ));

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self = NACT_MAIN_WINDOW( instance );

	self->private = g_new0( NactMainWindowPrivate, 1 );

	self->private->dispose_has_run = FALSE;

	/* initialize timeout parameters when blocking 'pivot-items-changed' handler
	 */
	self->private->pivot_timeout.timeout = st_burst_timeout;
	self->private->pivot_timeout.handler = ( NATimeoutFunc ) on_block_items_changed_timeout;
	self->private->pivot_timeout.user_data = self;
	self->private->pivot_timeout.source_id = 0;
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	NactMainWindow *self;

	g_return_if_fail( NACT_IS_MAIN_WINDOW( object ));
	self = NACT_MAIN_WINDOW( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case MAIN_PROP_ITEM_ID:
				g_value_set_pointer( value, self->private->current_item );
				break;

			case MAIN_PROP_PROFILE_ID:
				g_value_set_pointer( value, self->private->current_profile );
				break;

			case MAIN_PROP_CONTEXT_ID:
				g_value_set_pointer( value, self->private->current_context );
				break;

			case MAIN_PROP_EDITABLE_ID:
				g_value_set_boolean( value, self->private->editable );
				break;

			case MAIN_PROP_REASON_ID:
				g_value_set_int( value, self->private->reason );
				break;

			default:
				G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
				break;
		}
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	NactMainWindow *self;

	g_return_if_fail( NACT_IS_MAIN_WINDOW( object ));
	self = NACT_MAIN_WINDOW( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case MAIN_PROP_ITEM_ID:
				self->private->current_item = g_value_get_pointer( value );
				break;

			case MAIN_PROP_PROFILE_ID:
				self->private->current_profile = g_value_get_pointer( value );
				break;

			case MAIN_PROP_CONTEXT_ID:
				self->private->current_context = g_value_get_pointer( value );
				break;

			case MAIN_PROP_EDITABLE_ID:
				self->private->editable = g_value_get_boolean( value );
				break;

			case MAIN_PROP_REASON_ID:
				self->private->reason = g_value_get_int( value );
				break;

			default:
				G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
				break;
		}
	}
}

static void
instance_constructed( GObject *window )
{
	static const gchar *thisfn = "nact_main_window_instance_constructed";
	NactMainWindow *self;
	NactApplication *application;

	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	self = NACT_MAIN_WINDOW( window );

	if( !self->private->dispose_has_run ){
		g_debug( "%s: window=%p (%s)", thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ));

		/* first connect to BaseWindow signals
		 * so that convenience objects instanciated later will have this same signal
		 * triggered after the one of NactMainWindow
		 */
		base_window_signal_connect( BASE_WINDOW( window ),
				G_OBJECT( window ), BASE_SIGNAL_INITIALIZE_GTK, G_CALLBACK( on_base_initialize_gtk_toplevel ));

		base_window_signal_connect( BASE_WINDOW( window ),
				G_OBJECT( window ), BASE_SIGNAL_INITIALIZE_WINDOW, G_CALLBACK( on_base_initialize_base_window ));

		base_window_signal_connect( BASE_WINDOW( window ),
				G_OBJECT( window ), BASE_SIGNAL_ALL_WIDGETS_SHOWED, G_CALLBACK( on_base_all_widgets_showed ));

		application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
		self->private->updater = nact_application_get_updater( application );

		self->private->pivot_handler_id = base_window_signal_connect( BASE_WINDOW( window ),
				G_OBJECT( self->private->updater ), PIVOT_SIGNAL_ITEMS_CHANGED, G_CALLBACK( on_pivot_items_changed ));

		base_window_signal_connect( BASE_WINDOW( window ),
				G_OBJECT( window ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, G_CALLBACK( on_tab_updatable_item_updated ));

		/* create the tree view which will create itself its own tree model
		 */
		self->private->items_view = nact_tree_view_new( BASE_WINDOW( window ), "ActionsList", TREE_MODE_EDITION );

		base_window_signal_connect( BASE_WINDOW( window ),
				G_OBJECT( window ), TREE_SIGNAL_SELECTION_CHANGED, G_CALLBACK( on_tree_view_selection_changed ));

		base_window_signal_connect( BASE_WINDOW( window ),
				G_OBJECT( window ), TREE_SIGNAL_MODIFIED_STATUS_CHANGED, G_CALLBACK( on_tree_view_modified_status_changed ));

		/* create the menubar and other convenience objects
		 */
		self->private->menubar = nact_menubar_new( BASE_WINDOW( window ));
		self->private->clipboard = nact_clipboard_new( BASE_WINDOW( window ));

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->constructed ){
			G_OBJECT_CLASS( st_parent_class )->constructed( window );
		}
	}
}

static void
instance_dispose( GObject *window )
{
	static const gchar *thisfn = "nact_main_window_instance_dispose";
	NactMainWindow *self;
	GtkWidget *pane;
	gint pos;
	NASettings *settings;

	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	self = NACT_MAIN_WINDOW( window );

	if( !self->private->dispose_has_run ){
		g_debug( "%s: window=%p (%s)", thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ));

		self->private->dispose_has_run = TRUE;

		g_object_unref( self->private->clipboard );
		g_object_unref( self->private->menubar );

		settings = na_pivot_get_settings( NA_PIVOT( self->private->updater ));

		pane = base_window_get_widget( BASE_WINDOW( window ), "MainPaned" );
		pos = gtk_paned_get_position( GTK_PANED( pane ));
		na_settings_set_uint( settings, NA_IPREFS_MAIN_PANED, pos );

		nact_iaction_tab_dispose( NACT_IACTION_TAB( window ));
		nact_icommand_tab_dispose( NACT_ICOMMAND_TAB( window ));
		nact_ibasenames_tab_dispose( NACT_IBASENAMES_TAB( window ));
		nact_imimetypes_tab_dispose( NACT_IMIMETYPES_TAB( window ));
		nact_ifolders_tab_dispose( NACT_IFOLDERS_TAB( window ));
		nact_ischemes_tab_dispose( NACT_ISCHEMES_TAB( window ));
		nact_icapabilities_tab_dispose( NACT_ICAPABILITIES_TAB( window ));
		nact_ienvironment_tab_dispose( NACT_IENVIRONMENT_TAB( window ));
		nact_iexecution_tab_dispose( NACT_IEXECUTION_TAB( window ));
		nact_iproperties_tab_dispose( NACT_IPROPERTIES_TAB( window ));

		/* unref items view at last as gtk_tree_model_store_clear() will
		 * finalize all objects, thus invaliditing all our references
		 */
		g_object_unref( self->private->items_view );

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( window );
		}
	}
}

static void
instance_finalize( GObject *window )
{
	static const gchar *thisfn = "nact_main_window_instance_finalize";
	NactMainWindow *self;

	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	g_debug( "%s: window=%p (%s)", thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ));

	self = NACT_MAIN_WINDOW( window );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( window );
	}
}

/**
 * Returns a newly allocated NactMainWindow object.
 */
NactMainWindow *
nact_main_window_new( const NactApplication *application )
{
	NactMainWindow *window;

	g_return_val_if_fail( NACT_IS_APPLICATION( application ), NULL );

	window = g_object_new( NACT_MAIN_WINDOW_TYPE,
			BASE_PROP_APPLICATION,    application,
			BASE_PROP_XMLUI_FILENAME, st_xmlui_filename,
			BASE_PROP_TOPLEVEL_NAME,  st_toplevel_name,
			BASE_PROP_WSP_NAME,       st_wsp_name,
			NULL );

	return( window );
}

/*
 * note that for this NactMainWindow, on_base_initialize_gtk_toplevel() and
 * on_base_initialize_base_window() are roughly equivalent, as there is only
 * one occurrence on this window in the application: closing this window
 * is the same than quitting the application
 */
static void
on_base_initialize_gtk_toplevel( NactMainWindow *window, GtkWindow *toplevel, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_base_initialize_gtk_toplevel";

	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	if( !window->private->dispose_has_run ){
		g_debug( "%s: window=%p, toplevel=%p, user_data=%p",
				thisfn, ( void * ) window, ( void * ) toplevel, ( void * ) user_data );

		nact_iaction_tab_initial_load_toplevel( NACT_IACTION_TAB( window ));
		nact_icommand_tab_initial_load_toplevel( NACT_ICOMMAND_TAB( window ));
		nact_ibasenames_tab_initial_load_toplevel( NACT_IBASENAMES_TAB( window ));
		nact_imimetypes_tab_initial_load_toplevel( NACT_IMIMETYPES_TAB( window ));
		nact_ifolders_tab_initial_load_toplevel( NACT_IFOLDERS_TAB( window ));
		nact_ischemes_tab_initial_load_toplevel( NACT_ISCHEMES_TAB( window ));
		nact_icapabilities_tab_initial_load_toplevel( NACT_ICAPABILITIES_TAB( window ));
		nact_ienvironment_tab_initial_load_toplevel( NACT_IENVIRONMENT_TAB( window ));
		nact_iexecution_tab_initial_load_toplevel( NACT_IEXECUTION_TAB( window ));
		nact_iproperties_tab_initial_load_toplevel( NACT_IPROPERTIES_TAB( window ));

		nact_main_statusbar_initialize_gtk_toplevel( window );
	}
}

static void
on_base_initialize_base_window( NactMainWindow *window, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_base_initialize_base_window";
	NASettings *settings;
	guint pos;
	GtkWidget *pane;

	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	if( !window->private->dispose_has_run ){
		g_debug( "%s: window=%p, user_data=%p", thisfn, ( void * ) window, ( void * ) user_data );

		settings = na_pivot_get_settings( NA_PIVOT( window->private->updater ));

		pos = na_settings_get_uint( settings, NA_IPREFS_MAIN_PANED, NULL, NULL );
		if( pos ){
			pane = base_window_get_widget( BASE_WINDOW( window ), "MainPaned" );
			gtk_paned_set_position( GTK_PANED( pane ), pos );
		}

		nact_iaction_tab_runtime_init_toplevel( NACT_IACTION_TAB( window ));
		nact_icommand_tab_runtime_init_toplevel( NACT_ICOMMAND_TAB( window ));
		nact_ibasenames_tab_runtime_init_toplevel( NACT_IBASENAMES_TAB( window ));
		nact_imimetypes_tab_runtime_init_toplevel( NACT_IMIMETYPES_TAB( window ));
		nact_ifolders_tab_runtime_init_toplevel( NACT_IFOLDERS_TAB( window ));
		nact_ischemes_tab_runtime_init_toplevel( NACT_ISCHEMES_TAB( window ));
		nact_icapabilities_tab_runtime_init_toplevel( NACT_ICAPABILITIES_TAB( window ));
		nact_ienvironment_tab_runtime_init_toplevel( NACT_IENVIRONMENT_TAB( window ));
		nact_iexecution_tab_runtime_init_toplevel( NACT_IEXECUTION_TAB( window ));
		nact_iproperties_tab_runtime_init_toplevel( NACT_IPROPERTIES_TAB( window ));

		/* terminate the application by clicking the top right [X] button
		 */
		base_window_signal_connect( BASE_WINDOW( window ),
				G_OBJECT( base_window_get_gtk_toplevel( BASE_WINDOW( window ))),
				"delete-event", G_CALLBACK( on_delete_event ));

		/* is willing to quit ?
		 */
		base_window_signal_connect( BASE_WINDOW( window ),
				G_OBJECT( window ), BASE_SIGNAL_WILLING_TO_QUIT, G_CALLBACK( on_base_is_willing_to_quit ));
	}
}

static void
on_base_all_widgets_showed( NactMainWindow *window, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_base_all_widgets_showed";

	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));
	g_return_if_fail( NACT_IS_IACTION_TAB( window ));
	g_return_if_fail( NACT_IS_ICOMMAND_TAB( window ));
	g_return_if_fail( NACT_IS_IBASENAMES_TAB( window ));
	g_return_if_fail( NACT_IS_IMIMETYPES_TAB( window ));
	g_return_if_fail( NACT_IS_IFOLDERS_TAB( window ));
	g_return_if_fail( NACT_IS_ISCHEMES_TAB( window ));
	g_return_if_fail( NACT_IS_IENVIRONMENT_TAB( window ));
	g_return_if_fail( NACT_IS_IEXECUTION_TAB( window ));
	g_return_if_fail( NACT_IS_IPROPERTIES_TAB( window ));

	if( !window->private->dispose_has_run ){
		g_debug( "%s: window=%p, user_data=%p", thisfn, ( void * ) window, ( void * ) user_data );

		nact_iaction_tab_all_widgets_showed( NACT_IACTION_TAB( window ));
		nact_icommand_tab_all_widgets_showed( NACT_ICOMMAND_TAB( window ));
		nact_ibasenames_tab_all_widgets_showed( NACT_IBASENAMES_TAB( window ));
		nact_imimetypes_tab_all_widgets_showed( NACT_IMIMETYPES_TAB( window ));
		nact_ifolders_tab_all_widgets_showed( NACT_IFOLDERS_TAB( window ));
		nact_ischemes_tab_all_widgets_showed( NACT_ISCHEMES_TAB( window ));
		nact_icapabilities_tab_all_widgets_showed( NACT_ICAPABILITIES_TAB( window ));
		nact_ienvironment_tab_all_widgets_showed( NACT_IENVIRONMENT_TAB( window ));
		nact_iexecution_tab_all_widgets_showed( NACT_IEXECUTION_TAB( window ));
		nact_iproperties_tab_all_widgets_showed( NACT_IPROPERTIES_TAB( window ));

		load_or_reload_items( window );
	}
}

/**
 * nact_main_window_get_clipboard:
 * @window: this #NactMainWindow instance.
 *
 * Returns: the #NactClipboard convenience object.
 */
NactClipboard *
nact_main_window_get_clipboard( const NactMainWindow *window )
{
	NactClipboard *clipboard;

	g_return_val_if_fail( NACT_IS_MAIN_WINDOW( window ), NULL );

	clipboard = NULL;

	if( !window->private->dispose_has_run ){

		clipboard = window->private->clipboard;
	}

	return( clipboard );
}

/**
 * nact_main_window_get_items_view:
 * @window: this #NactMainWindow instance.
 *
 * Returns: The #NactTreeView convenience object.
 */
NactTreeView *
nact_main_window_get_items_view( const NactMainWindow *window )
{
	NactTreeView *view;

	g_return_val_if_fail( NACT_IS_MAIN_WINDOW( window ), NULL );

	view = NULL;

	if( !window->private->dispose_has_run ){

		view = window->private->items_view;
	}

	return( view );
}

/**
 * nact_main_window_reload:
 * @window: this #NactMainWindow instance.
 *
 * Refresh the list of items.
 * If there is some non-yet saved modifications, a confirmation is
 * required before giving up with them.
 */
void
nact_main_window_reload( NactMainWindow *window )
{
	gboolean reload_ok;

	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	if( !window->private->dispose_has_run ){

		reload_ok = confirm_for_giveup_from_menu( window );

		if( reload_ok ){
			load_or_reload_items( window );
		}
	}
}

/**
 * nact_main_window_block_reload:
 * @window: this #NactMainWindow instance.
 *
 * Temporarily blocks the handling of pivot-items-changed signal.
 */
void
nact_main_window_block_reload( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_window_block_reload";

	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	if( !window->private->dispose_has_run ){

		g_debug( "%s: blocking %s signal", thisfn, PIVOT_SIGNAL_ITEMS_CHANGED );
		g_signal_handler_block( window->private->updater, window->private->pivot_handler_id );
		na_timeout_event( &window->private->pivot_timeout );
	}
}

static void
on_block_items_changed_timeout( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_window_on_block_items_changed_timeout";

	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	g_debug( "%s: unblocking %s signal", thisfn, PIVOT_SIGNAL_ITEMS_CHANGED );
	g_signal_handler_unblock( window->private->updater, window->private->pivot_handler_id );
}

/*
 * the modification status of the items view has changed
 */
static void
on_tree_view_modified_status_changed( NactMainWindow *window, gboolean is_modified, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_tree_view_modified_status_changed";

	g_debug( "%s: window=%p, is_modified=%s, user_data=%p",
			thisfn, ( void * ) window, is_modified ? "True":"False", ( void * ) user_data );

	if( !window->private->dispose_has_run ){

		window->private->is_tree_modified = is_modified;
		setup_dialog_title( window );
	}
}

/*
 * tree view selection has changed
 */
static void
on_tree_view_selection_changed( NactMainWindow *window, GList *selected_items, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_tree_view_selection_changed";
	guint count;

	count = g_list_length( selected_items );

	if( !window->private->dispose_has_run ){
		g_debug( "%s: window=%p, selected_items=%p (count=%d), user_data=%p",
				thisfn, ( void * ) window,
				( void * ) selected_items, count, ( void * ) user_data );

		raz_selection_properties( window );

		if( count == 1 ){
			g_return_if_fail( NA_IS_OBJECT_ID( selected_items->data ));
			setup_current_selection( window, NA_OBJECT_ID( selected_items->data ));
			setup_writability_status( window );
		}

		setup_dialog_title( window );

		g_signal_emit_by_name( G_OBJECT( window ),
				MAIN_SIGNAL_SELECTION_CHANGED, na_object_copyref_items( selected_items ));
	}
}

/*
 * cleanup handler for our MAIN_SIGNAL_SELECTION_CHANGED signal
 */
static void
on_selection_changed_cleanup_handler( BaseWindow *window, GList *selected_items )
{
	static const gchar *thisfn = "nact_main_window_on_selection_changed_cleanup_handler";

	g_debug( "%s: window=%p, selected_items=%p (count=%u)",
			thisfn, ( void * ) window,
			( void * ) selected_items, g_list_length( selected_items ));

	na_object_free_items( selected_items );
}

static void
on_tab_updatable_item_updated( NactMainWindow *window, NAIContext *context, guint data, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_tab_updatable_item_updated";

	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	if( !window->private->dispose_has_run ){
		g_debug( "%s: window=%p, context=%p (%s), data=%u, user_data=%p",
				thisfn, ( void * ) window, ( void * ) context, G_OBJECT_TYPE_NAME( context ),
				data, ( void * ) user_data );

		if( context ){
			na_object_check_status( context );
		}
	}
}

static void
raz_selection_properties( NactMainWindow *window )
{
	window->private->current_item = NULL;
	window->private->current_profile = NULL;
	window->private->current_context = NULL;
	window->private->editable = FALSE;
	window->private->reason = 0;

	nact_main_statusbar_set_locked( window, FALSE, 0 );
}

/*
 * enter after raz_properties
 * only called when only one selected row
 */
static void
setup_current_selection( NactMainWindow *window, NAObjectId *selected_row )
{
	guint nb_profiles;
	GList *profiles;

	if( NA_IS_OBJECT_PROFILE( selected_row )){
		window->private->current_profile = NA_OBJECT_PROFILE( selected_row );
		window->private->current_item = NA_OBJECT_ITEM( na_object_get_parent( selected_row ));

	} else {
		g_return_if_fail( NA_IS_OBJECT_ITEM( selected_row ));
		window->private->current_item = NA_OBJECT_ITEM( selected_row );
		window->private->current_context = NA_ICONTEXT( selected_row );

		if( NA_IS_OBJECT_ACTION( selected_row )){
			nb_profiles = na_object_get_items_count( selected_row );

			if( nb_profiles == 1 ){
				profiles = na_object_get_items( selected_row );
				window->private->current_profile = NA_OBJECT_PROFILE( profiles->data );
				window->private->current_context = NA_ICONTEXT( profiles->data );
			}
		}
	}
}

/*
 * the title bar of the main window brings up three informations:
 * - the name of the application
 * - the name of the currently selected item if there is only one
 * - an asterisk if anything has been modified
 */
static void
setup_dialog_title( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_window_setup_dialog_title";
	GtkWindow *toplevel;
	NactApplication *application;
	gchar *title;
	gchar *label;
	gchar *tmp;
	gboolean is_modified;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	title = base_application_get_application_name( BASE_APPLICATION( application ));

	if( window->private->current_item ){
		label = na_object_get_label( window->private->current_item );
		is_modified = na_object_is_modified( window->private->current_item );
		tmp = g_strdup_printf( "%s%s - %s", is_modified ? "*" : "", label, title );
		g_free( label );
		g_free( title );
		title = tmp;
	}

	toplevel = base_window_get_gtk_toplevel( BASE_WINDOW( window ));
	gtk_window_set_title( toplevel, title );
	g_free( title );
}

static void
setup_writability_status( NactMainWindow *window )
{
	g_return_if_fail( NA_IS_OBJECT_ITEM( window->private->current_item ));

	window->private->editable = na_object_is_finally_writable( window->private->current_item, &window->private->reason );
	nact_main_statusbar_set_locked( window, !window->private->editable, window->private->reason );
}

/*
 * The handler of the signal sent by NAPivot when items have been modified
 * in the underlying storage subsystems
 */
static void
on_pivot_items_changed( NAUpdater *updater, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_window_on_pivot_items_changed";
	gboolean reload_ok;

	g_return_if_fail( NA_IS_UPDATER( updater ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	if( !window->private->dispose_has_run ){
		g_debug( "%s: updater=%p (%s), window=%p (%s)", thisfn,
				( void * ) updater, G_OBJECT_TYPE_NAME( updater ),
				( void * ) window, G_OBJECT_TYPE_NAME( window ));

		reload_ok = confirm_for_giveup_from_pivot( window );

		if( reload_ok ){
			load_or_reload_items( window );
		}
	}
}

/*
 * informs the user that the actions in underlying storage subsystem
 * have changed, and propose for reloading
 *
 */
static gboolean
confirm_for_giveup_from_pivot( const NactMainWindow *window )
{
	gboolean reload_ok;
	gchar *first, *second;

	first = g_strdup(
				_( "One or more actions have been modified in the filesystem.\n"
					"You could keep to work with your current list of actions, "
					"or you may want to reload a fresh one." ));

	if( window->private->is_tree_modified){

		gchar *tmp = g_strdup_printf( "%s\n\n%s", first,
				_( "Note that reloading a fresh list of actions requires "
					"that you give up with your current modifications." ));
		g_free( first );
		first = tmp;
	}

	second = g_strdup( _( "Do you want to reload a fresh list of actions ?" ));

	reload_ok = base_window_display_yesno_dlg( BASE_WINDOW( window ), first, second );

	g_free( second );
	g_free( first );

	return( reload_ok );
}

/*
 * requires a confirmation from the user when is has asked for reloading
 * the actions via the Edit menu
 */
static gboolean
confirm_for_giveup_from_menu( const NactMainWindow *window )
{
	gboolean reload_ok = TRUE;
	gchar *first, *second;

	if( window->private->is_tree_modified ){

		first = g_strdup(
					_( "Reloading a fresh list of actions requires "
						"that you give up with your current modifications." ));

		second = g_strdup( _( "Do you really want to do this ?" ));

		reload_ok = base_window_display_yesno_dlg( BASE_WINDOW( window ), first, second );

		g_free( second );
		g_free( first );
	}

	return( reload_ok );
}

static void
load_or_reload_items( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_window_load_or_reload_items";
	GList *tree;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );

	raz_selection_properties( window );

	tree = na_updater_load_items( window->private->updater );
	nact_tree_view_fill( window->private->items_view, tree );

	g_debug( "%s: end of tree view filling", thisfn );
}

/**
 * nact_main_window_quit:
 * @window: the #NactMainWindow main window.
 *
 * Quit the window, thus terminating the application.
 *
 * Returns: %TRUE if the application will terminate, and the @window object
 * is no more valid; %FALSE if the application will continue to run.
 */
gboolean
nact_main_window_quit( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_window_quit";
	gboolean terminated;

	g_return_val_if_fail( NACT_IS_MAIN_WINDOW( window ), FALSE );

	terminated = FALSE;

	if( !window->private->dispose_has_run ){
		g_debug( "%s: window=%p (%s)", thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ));

		if( !window->private->is_tree_modified  || warn_modified( window )){
			g_object_unref( window );
			terminated = TRUE;
		}
	}

	return( terminated );
}

/*
 * signal handler
 * should return %FALSE if it is not willing to quit
 * this will also stop the emission of the signal (i.e. the first FALSE wins)
 */
static gboolean
on_base_is_willing_to_quit( const BaseWindow *window, gconstpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_base_is_willing_to_quit";
	gboolean willing_to;

	g_return_val_if_fail( NACT_IS_MAIN_WINDOW( window ), TRUE );

	willing_to = TRUE;

	if( !NACT_MAIN_WINDOW( window )->private->dispose_has_run ){
		g_debug( "%s (virtual): window=%p", thisfn, ( void * ) window );

		if( NACT_MAIN_WINDOW( window )->private->is_tree_modified ){
			willing_to = nact_confirm_logout_run( NACT_MAIN_WINDOW( window ));
		}
	}

	return( willing_to );
}

/*
 * triggered when the user clicks on the top right [X] button
 * returns %TRUE to stop the signal to be propagated (which would cause the
 * window to be destroyed); instead we gracefully quit the application
 */
static gboolean
on_delete_event( GtkWidget *toplevel, GdkEvent *event, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_window_on_delete_event";

	g_debug( "%s: toplevel=%p, event=%p, window=%p",
			thisfn, ( void * ) toplevel, ( void * ) event, ( void * ) window );

	nact_main_window_quit( window );

	return( TRUE );
}

/*
 * warn_modified:
 * @window: this #NactWindow instance.
 *
 * Emits a warning if at least one item has been modified.
 *
 * Returns: %TRUE if the user confirms he wants to quit.
 */
static gboolean
warn_modified( NactMainWindow *window )
{
	gboolean confirm = FALSE;
	gchar *first;
	gchar *second;

	first = g_strdup_printf( _( "Some items have been modified." ));
	second = g_strdup( _( "Are you sure you want to quit without saving them ?" ));

	confirm = base_window_display_yesno_dlg( BASE_WINDOW( window ), first, second );

	g_free( second );
	g_free( first );

	return( confirm );
}
