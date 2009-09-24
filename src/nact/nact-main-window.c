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

#include <common/na-object-api.h>
#include <common/na-pivot.h>
#include <common/na-iio-provider.h>
#include <common/na-ipivot-consumer.h>
#include <common/na-iprefs.h>

#include "base-iprefs.h"
#include "nact-application.h"
#include "nact-clipboard.h"
#include "nact-iactions-list.h"
#include "nact-iaction-tab.h"
#include "nact-icommand-tab.h"
#include "nact-iconditions-tab.h"
#include "nact-iadvanced-tab.h"
#include "nact-main-tab.h"
#include "nact-main-menubar.h"
#include "nact-main-window.h"

/* private class data
 */
struct NactMainWindowClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NactMainWindowPrivate {
	gboolean         dispose_has_run;

	/* TODO: this will have to be replaced with undo-manager */
	GList           *deleted;

	/**
	 * Currently edited action or menu.
	 *
	 * This is the action or menu which is displayed in tab Action ;
	 * it may be different of the row being currently selected.
	 *
	 * Can be null, and this implies that edited_profile is also null.
	 */
	NAObjectItem    *edited_item;

	/**
	 * Currently edited profile.
	 *
	 * This is the profile which is displayed in tabs Command,
	 * Conditions and Advanced ; it may be different of the row being
	 * currently selected.
	 *
	 * Can be null and this implies that the edited item is a menu
	 * (because an action cannot have zero profile).
	 */
	NAObjectProfile *edited_profile;

	/**
	 * The convenience clipboard object.
	 */
	NactClipboard   *clipboard;
};

/* action properties
 */
enum {
	PROP_EDITED_ITEM = 1,
	PROP_ITEM_EDITION_ENABLED,
	PROP_EDITED_PROFILE
};

/* signals
 */
enum {
	SELECTION_CHANGED,
	ITEM_UPDATED,
	LAST_SIGNAL
};

static NactWindowClass *st_parent_class = NULL;
static gint             st_signals[ LAST_SIGNAL ] = { 0 };

static GType    register_type( void );
static void     class_init( NactMainWindowClass *klass );
static void     iactions_list_iface_init( NactIActionsListInterface *iface );
static void     iaction_tab_iface_init( NactIActionTabInterface *iface );
static void     icommand_tab_iface_init( NactICommandTabInterface *iface );
static void     iconditions_tab_iface_init( NactIConditionsTabInterface *iface );
static void     iadvanced_tab_iface_init( NactIAdvancedTabInterface *iface );
static void     ipivot_consumer_iface_init( NAIPivotConsumerInterface *iface );
static void     iprefs_iface_init( NAIPrefsInterface *iface );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void     instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void     instance_dispose( GObject *application );
static void     instance_finalize( GObject *application );

static void     actually_delete_item( NactMainWindow *window, NAObject *item, NAPivot *pivot );

static gchar   *base_get_toplevel_name( BaseWindow *window );
static gchar   *base_get_iprefs_window_id( BaseWindow *window );
static void     on_base_initial_load_toplevel( NactMainWindow *window, gpointer user_data );
static void     on_base_runtime_init_toplevel( NactMainWindow *window, gpointer user_data );
static void     on_base_all_widgets_showed( NactMainWindow *window, gpointer user_data );

static void     iactions_list_selection_changed( NactIActionsList *instance, GSList *selected_items );
static void     set_current_object_item( NactMainWindow *window, GSList *selected_items );
static void     set_current_profile( NactMainWindow *window, gboolean set_action, GSList *selected_items );

static void     on_tab_updatable_item_updated( NactMainWindow *window, gpointer user_data );

static void     ipivot_consumer_on_actions_changed( NAIPivotConsumer *instance, gpointer user_data );
static void     ipivot_consumer_on_display_order_changed( NAIPivotConsumer *instance, gpointer user_data );

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

	static const GInterfaceInfo iactions_list_iface_info = {
		( GInterfaceInitFunc ) iactions_list_iface_init,
		NULL,
		NULL
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

	static const GInterfaceInfo iconditions_tab_iface_info = {
		( GInterfaceInitFunc ) iconditions_tab_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo iadvanced_tab_iface_info = {
		( GInterfaceInitFunc ) iadvanced_tab_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo ipivot_consumer_iface_info = {
		( GInterfaceInitFunc ) ipivot_consumer_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo iprefs_iface_info = {
		( GInterfaceInitFunc ) iprefs_iface_init,
		NULL,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( NACT_WINDOW_TYPE, "NactMainWindow", &info, 0 );

	g_type_add_interface_static( type, NACT_IACTIONS_LIST_TYPE, &iactions_list_iface_info );

	g_type_add_interface_static( type, NACT_IACTION_TAB_TYPE, &iaction_tab_iface_info );

	g_type_add_interface_static( type, NACT_ICOMMAND_TAB_TYPE, &icommand_tab_iface_info );

	g_type_add_interface_static( type, NACT_ICONDITIONS_TAB_TYPE, &iconditions_tab_iface_info );

	g_type_add_interface_static( type, NACT_IADVANCED_TAB_TYPE, &iadvanced_tab_iface_info );

	g_type_add_interface_static( type, NA_IPIVOT_CONSUMER_TYPE, &ipivot_consumer_iface_info );

	g_type_add_interface_static( type, NA_IPREFS_TYPE, &iprefs_iface_info );

	return( type );
}

static void
class_init( NactMainWindowClass *klass )
{
	static const gchar *thisfn = "nact_main_window_class_init";
	GObjectClass *object_class;
	BaseWindowClass *base_class;
	GParamSpec *spec;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
	object_class->set_property = instance_set_property;
	object_class->get_property = instance_get_property;

	spec = g_param_spec_pointer(
			TAB_UPDATABLE_PROP_EDITED_ACTION,
			"Edited NAObjectItem",
			"A pointer to the edited NAObjectItem, an action or a menu",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_EDITED_ITEM, spec );

	spec = g_param_spec_pointer(
			TAB_UPDATABLE_PROP_EDITED_PROFILE,
			"Edited NAObjectProfile",
			"A pointer to the edited NAObjectProfile",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_EDITED_PROFILE, spec );

	klass->private = g_new0( NactMainWindowClassPrivate, 1 );

	base_class = BASE_WINDOW_CLASS( klass );
	base_class->get_toplevel_name = base_get_toplevel_name;
	base_class->get_iprefs_window_id = base_get_iprefs_window_id;

	/**
	 * nact-tab-updatable-selection-changed:
	 *
	 * This signal is emitted by this main window, in response of a
	 * change of the selection in IActionsList.
	 * Notebook tabs should connect to this signal and update their
	 * display to reflect the content of the new selection.
	 *
	 * Note also that, where this main window will receive from
	 * IActionsList the full list of currently selected items, this
	 * signal only carries to the tabs the count of selected items.
	 *
	 * See #iactions_list_selection_changed().
	 */
	st_signals[ SELECTION_CHANGED ] = g_signal_new(
			TAB_UPDATABLE_SIGNAL_SELECTION_CHANGED,
			G_TYPE_OBJECT,
			G_SIGNAL_RUN_LAST,
			0,					/* no default handler */
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER );

	/**
	 * nact-tab-updatable-item-updated:
	 *
	 * This signal is emitted by the notebook tabs, when any property
	 * of an item has been modified.
	 *
	 * This main window is rather the only consumer of this message,
	 * does its tricks (title, etc.), and then reforward an item-updated
	 * message to IActionsList.
	 */
	st_signals[ ITEM_UPDATED ] = g_signal_new(
			TAB_UPDATABLE_SIGNAL_ITEM_UPDATED,
			G_TYPE_OBJECT,
			G_SIGNAL_RUN_LAST,
			0,					/* no default handler */
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER );
}

static void
iactions_list_iface_init( NactIActionsListInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_iactions_list_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->selection_changed = iactions_list_selection_changed;
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
iconditions_tab_iface_init( NactIConditionsTabInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_iconditions_tab_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );
}

static void
iadvanced_tab_iface_init( NactIAdvancedTabInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_iadvanced_tab_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );
}

static void
ipivot_consumer_iface_init( NAIPivotConsumerInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_ipivot_consumer_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->on_actions_changed = ipivot_consumer_on_actions_changed;
	iface->on_display_order_changed = ipivot_consumer_on_display_order_changed;
}

static void
iprefs_iface_init( NAIPrefsInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_iprefs_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_main_window_instance_init";
	NactMainWindow *self;

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );
	g_return_if_fail( NACT_IS_MAIN_WINDOW( instance ));
	self = NACT_MAIN_WINDOW( instance );

	self->private = g_new0( NactMainWindowPrivate, 1 );

	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			BASE_WINDOW_SIGNAL_INITIAL_LOAD,
			G_CALLBACK( on_base_initial_load_toplevel ));

	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			BASE_WINDOW_SIGNAL_RUNTIME_INIT,
			G_CALLBACK( on_base_runtime_init_toplevel ));

	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			BASE_WINDOW_SIGNAL_ALL_WIDGETS_SHOWED,
			G_CALLBACK( on_base_all_widgets_showed ));

	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			TAB_UPDATABLE_SIGNAL_ITEM_UPDATED,
			G_CALLBACK( on_tab_updatable_item_updated ));

	self->private->dispose_has_run = FALSE;
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	NactMainWindow *self;

	g_return_if_fail( NACT_IS_MAIN_WINDOW( object ));
	self = NACT_MAIN_WINDOW( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case PROP_EDITED_ITEM:
				g_value_set_pointer( value, self->private->edited_item );
				break;

			case PROP_EDITED_PROFILE:
				g_value_set_pointer( value, self->private->edited_profile );
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
			case PROP_EDITED_ITEM:
				self->private->edited_item = g_value_get_pointer( value );
				break;

			case PROP_EDITED_PROFILE:
				self->private->edited_profile = g_value_get_pointer( value );
				break;

			default:
				G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
				break;
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

	g_debug( "%s: window=%p", thisfn, ( void * ) window );
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));
	self = NACT_MAIN_WINDOW( window );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		g_object_unref( self->private->clipboard );

		pane = base_window_get_widget( BASE_WINDOW( window ), "MainPaned" );
		pos = gtk_paned_get_position( GTK_PANED( pane ));
		base_iprefs_set_int( BASE_WINDOW( window ), "main-paned", pos );

		na_object_free_items( self->private->deleted );

		nact_iactions_list_dispose( NACT_IACTIONS_LIST( window ));
		nact_iaction_tab_dispose( NACT_IACTION_TAB( window ));
		nact_icommand_tab_dispose( NACT_ICOMMAND_TAB( window ));
		nact_iconditions_tab_dispose( NACT_ICONDITIONS_TAB( window ));
		nact_iadvanced_tab_dispose( NACT_IADVANCED_TAB( window ));

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

	g_debug( "%s: window=%p", thisfn, ( void * ) window );
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));
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
nact_main_window_new( BaseApplication *application )
{
	g_return_val_if_fail( NACT_IS_APPLICATION( application ), NULL );

	return( g_object_new( NACT_MAIN_WINDOW_TYPE, BASE_WINDOW_PROP_APPLICATION, application, NULL ));
}

/**
 * nact_main_window_action_exists:
 * @window: this #NactMainWindow instance.
 * @uuid: the uuid to check for existancy.
 *
 * Returns: %TRUE if the specified action already exists in the system,
 * %FALSE else.
 *
 * We have to check against existing actions in #NAPivot, and against
 * currently edited actions in #NactIActionsList.
 */
gboolean
nact_main_window_action_exists( const NactMainWindow *window, const gchar *uuid )
{
	gboolean exists = FALSE;
	NactApplication *application;
	NAPivot *pivot;
	NAObject *action;

	g_return_val_if_fail( NACT_IS_MAIN_WINDOW( window ), FALSE );

	if( !window->private->dispose_has_run ){

		application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
		pivot = nact_application_get_pivot( application );
		action = na_pivot_get_item( pivot, uuid );
		if( action ){
			exists = TRUE;
		}

		if( !exists ){
			action = nact_iactions_list_get_item( NACT_IACTIONS_LIST( window ), uuid );
			if( action ){
				exists = TRUE;
			}
		}
	}

	return( exists );
}

/**
 * nact_main_window_has_modified_items:
 * @window: this #NactMainWindow instance.
 *
 * Returns: %TRUE if there is at least one modified item in IActionsList.
 *
 * Note that exact count of modified actions is subject to some
 * approximation:
 * 1. counting the modified actions currently in the list is ok
 * 2. but what about deleted actions ?
 *    we can create any new actions, deleting them, and so on
 *    if we have eventually deleted all newly created actions, then the
 *    final count of modified actions should be zero... don't it ?
 */
gboolean
nact_main_window_has_modified_items( const NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_window_has_modified_items";
	GList *ia;
	gint count_deleted = 0;
	gboolean has_modified = FALSE;

	g_return_val_if_fail( NACT_IS_MAIN_WINDOW( window ), FALSE );
	g_return_val_if_fail( NACT_IS_IACTIONS_LIST( window ), FALSE );

	if( !window->private->dispose_has_run ){

		for( ia = window->private->deleted ; ia ; ia = ia->next ){
			if( na_object_get_origin( NA_OBJECT( ia->data )) != NULL ){
				count_deleted += 1;
			}
		}
		g_debug( "%s: count_deleted=%d", thisfn, count_deleted );

		has_modified = nact_iactions_list_has_modified_items( NACT_IACTIONS_LIST( window ));
		g_debug( "%s: has_modified=%s", thisfn, has_modified ? "True":"False" );
	}

	return( count_deleted > 0 || has_modified );
}

/**
 * nact_main_window_move_to_deleted:
 * @window: this #NactMainWindow instance.
 * @items: list of deleted objects.
 *
 * Adds the given list to the deleted one.
 *
 * Note that we move the ref from @items list to our own deleted list.
 * So that the caller should not try to na_object_free_items() the
 * provided list.
 */
void
nact_main_window_move_to_deleted( NactMainWindow *window, GList *items )
{
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	if( !window->private->dispose_has_run ){
		window->private->deleted = g_list_concat( window->private->deleted, items );
	}
}

/**
 * nact_main_window_remove_deleted:
 * @window: this #NactMainWindow instance.
 *
 * Removes the deleted items from the underlying I/O storage subsystem.
 */
void
nact_main_window_remove_deleted( NactMainWindow *window )
{
	NactApplication *application;
	NAPivot *pivot;
	GList *it;
	NAObject *item;

	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	if( !window->private->dispose_has_run ){

		application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
		pivot = nact_application_get_pivot( application );

		for( it = window->private->deleted ; it ; it = it->next ){
			item = NA_OBJECT( it->data );
			actually_delete_item( window, item, pivot );
		}

		na_object_free_items( window->private->deleted );
		window->private->deleted = NULL;
	}
}

/*
 * If the deleted item is a profile, then do nothing because the parent
 * action has been marked as modified when the profile has been deleted,
 * and thus updated in the storage subsystem as well as in the pivot
 */
static void
actually_delete_item( NactMainWindow *window, NAObject *item, NAPivot *pivot )
{
	GList *items, *it;
	NAObject *origin;

	g_debug( "nact_main_window_actually_delete_item: item=%p (%s)",
			( void * ) item, G_OBJECT_TYPE_NAME( item ));

	if( NA_IS_OBJECT_ITEM( item )){
		nact_window_delete_item( NACT_WINDOW( window ), NA_OBJECT_ITEM( item ));

		origin = na_object_get_origin( item );
		if( origin ){
			na_pivot_remove_item( pivot, origin );
		}

		if( NA_IS_OBJECT_MENU( item )){
			items = na_object_get_items( item );
			for( it = items ; it ; it = it->next ){
				actually_delete_item( window, NA_OBJECT( it->data ), pivot );
			}
			na_object_free_items( items );
		}
	}
}

static gchar *
base_get_toplevel_name( BaseWindow *window )
{
	return( g_strdup( "MainWindow" ));
}

static gchar *
base_get_iprefs_window_id( BaseWindow *window )
{
	return( g_strdup( "main-window" ));
}

/*
 * note that for this NactMainWindow, on_initial_load_toplevel and
 * on_runtime_init_toplevel are equivalent, as there is only one
 * occurrence on this window in the application : closing this window
 * is the same than quitting the application
 */
static void
on_base_initial_load_toplevel( NactMainWindow *window, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_base_initial_load_toplevel";
	gint pos;
	GtkWidget *pane;

	g_debug( "%s: window=%p, user_data=%p", thisfn, ( void * ) window, ( void * ) user_data );
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	if( !window->private->dispose_has_run ){

		window->private->clipboard = nact_clipboard_new();

		pos = base_iprefs_get_int( BASE_WINDOW( window ), "main-paned" );
		if( pos ){
			pane = base_window_get_widget( BASE_WINDOW( window ), "MainPaned" );
			gtk_paned_set_position( GTK_PANED( pane ), pos );
		}

		nact_iactions_list_initial_load_toplevel( NACT_IACTIONS_LIST( window ));
		nact_iactions_list_set_filter_selection_mode( NACT_IACTIONS_LIST( window ), TRUE );
		nact_iactions_list_set_multiple_selection_mode( NACT_IACTIONS_LIST( window ), TRUE );
		nact_iactions_list_set_dnd_mode( NACT_IACTIONS_LIST( window ), TRUE );

		nact_iaction_tab_initial_load_toplevel( NACT_IACTION_TAB( window ));
		nact_icommand_tab_initial_load_toplevel( NACT_ICOMMAND_TAB( window ));
		nact_iconditions_tab_initial_load_toplevel( NACT_ICONDITIONS_TAB( window ));
		nact_iadvanced_tab_initial_load_toplevel( NACT_IADVANCED_TAB( window ));
	}
}

static void
on_base_runtime_init_toplevel( NactMainWindow *window, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_base_runtime_init_toplevel";
	NactApplication *application;
	NAPivot *pivot;
	GList *tree;

	g_debug( "%s: window=%p, user_data=%p", thisfn, ( void * ) window, ( void * ) user_data );
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	if( !window->private->dispose_has_run ){

		application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
		pivot = nact_application_get_pivot( application );
		tree = na_pivot_get_items( pivot );
		g_debug( "%s: pivot_tree=%p", thisfn, ( void * ) tree );

		nact_iaction_tab_runtime_init_toplevel( NACT_IACTION_TAB( window ));
		nact_icommand_tab_runtime_init_toplevel( NACT_ICOMMAND_TAB( window ));
		nact_iconditions_tab_runtime_init_toplevel( NACT_ICONDITIONS_TAB( window ));
		nact_iadvanced_tab_runtime_init_toplevel( NACT_IADVANCED_TAB( window ));

		/* fill the IActionsList at last so that all signals are connected
		 */
		nact_iactions_list_runtime_init_toplevel( NACT_IACTIONS_LIST( window ), tree );
		nact_main_menubar_runtime_init( window );

		/* forces a no-selection when the list is initially empty
		 */
		/*if( !g_slist_length( wnd->private->actions )){
			set_current_action( NACT_MAIN_WINDOW( window ), NULL );
		}*/
	}
}

static void
on_base_all_widgets_showed( NactMainWindow *window, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_base_all_widgets_showed";

	g_debug( "%s: window=%p, user_data=%p", thisfn, ( void * ) window, ( void * ) user_data );
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));
	g_return_if_fail( NACT_IS_IACTIONS_LIST( window ));
	g_return_if_fail( NACT_IS_IACTION_TAB( window ));
	g_return_if_fail( NACT_IS_ICOMMAND_TAB( window ));
	g_return_if_fail( NACT_IS_ICONDITIONS_TAB( window ));
	g_return_if_fail( NACT_IS_IADVANCED_TAB( window ));

	if( !window->private->dispose_has_run ){

		nact_iactions_list_all_widgets_showed( NACT_IACTIONS_LIST( window ));
		nact_iaction_tab_all_widgets_showed( NACT_IACTION_TAB( window ));
		nact_icommand_tab_all_widgets_showed( NACT_ICOMMAND_TAB( window ));
		nact_iconditions_tab_all_widgets_showed( NACT_ICONDITIONS_TAB( window ));
		nact_iadvanced_tab_all_widgets_showed( NACT_IADVANCED_TAB( window ));

		nact_main_menubar_refresh_actions_sensitivity( window );
	}
}

/*
 * iactions_list_selection_changed:
 * @window: this #NactMainWindow instance.
 * @selected_items: the currently selected items in ActionsList
 */
static void
iactions_list_selection_changed( NactIActionsList *instance, GSList *selected_items )
{
	static const gchar *thisfn = "nact_main_window_iactions_list_selection_changed";
	NactMainWindow *window;
	NAObject *object;
	gint count;

	count = g_slist_length( selected_items );

	g_debug( "%s: instance=%p, selected_items=%p, count=%d",
			thisfn, ( void * ) instance, ( void * ) selected_items, count );

	window = NACT_MAIN_WINDOW( instance );

	if( window->private->dispose_has_run ){
		return;
	}

	if( count == 1 ){
		object = NA_OBJECT( selected_items->data );
		if( NA_IS_OBJECT_ITEM( object )){
			window->private->edited_item = NA_OBJECT_ITEM( object );
			set_current_object_item( window, selected_items );

		} else {
			g_assert( NA_IS_OBJECT_PROFILE( object ));
			window->private->edited_profile = NA_OBJECT_PROFILE( object );
			set_current_profile( window, TRUE, selected_items );
		}

	} else {
		window->private->edited_item = NULL;
		set_current_object_item( window, selected_items );
	}

	g_object_set(
			G_OBJECT( window ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, window->private->edited_item,
			TAB_UPDATABLE_PROP_EDITED_PROFILE, window->private->edited_profile,
			NULL );

	g_signal_emit_by_name( window, TAB_UPDATABLE_SIGNAL_SELECTION_CHANGED, GINT_TO_POINTER( count ));
}

/*
 * update the notebook when selection changes in ActionsList
 * if there is only one profile, we also setup the profile
 */
static void
set_current_object_item( NactMainWindow *window, GSList *selected_items )
{
	static const gchar *thisfn = "nact_main_window_set_current_object_item";
	gint count_profiles;
	GList *profiles;
	/*NAObject *current;*/

	g_debug( "%s: window=%p, current=%p, selected_items=%p",
			thisfn, ( void * ) window, ( void * ) window->private->edited_item, ( void * ) selected_items );

	/* set the profile to be displayed, if any
	 */
	window->private->edited_profile = NULL;

	if( window->private->edited_item &&
		NA_IS_OBJECT_ACTION( window->private->edited_item )){

			count_profiles = na_object_get_items_count( NA_OBJECT_ACTION( window->private->edited_item ));
			g_return_if_fail( count_profiles >= 1 );

			if( count_profiles == 1 ){
				profiles = na_object_get_items( window->private->edited_item );
				window->private->edited_profile = NA_OBJECT_PROFILE( profiles->data );
				na_object_free_items( profiles );
			}
	}

	/* do the profile tabs (ICommandTab, IConditionsTab and IAdvancedTab)
	 * will be editable ?
	 * yes if we have an action with only one profile, or the selected
	 * item is itself a profile
	 */
	/*window->private->edition_enabled = ( window->private->edited_item != NULL );

	if( window->private->edition_enabled ){
		g_assert( selected_items );
		if( g_slist_length( selected_items ) > 1 ){
			window->private->edition_enabled = FALSE;
		}
	}

	if( window->private->edition_enabled && NA_IS_OBJECT_MENU( window->private->edited_item )){
		window->private->edition_enabled = FALSE;
	}

	if( window->private->edition_enabled ){
		g_assert( NA_IS_ACTION( window->private->edited_item ));
		current = NA_OBJECT( selected_items->data );
		if( NA_IS_OBJECT_ACTION( current)){
			if( na_object_action_get_profiles_count( NA_ACTION( window->private->edited_item )) > 1 ){
				window->private->edition_enabled = FALSE;
			}
		}
	}*/

	set_current_profile( window, FALSE, selected_items );
}

static void
set_current_profile( NactMainWindow *window, gboolean set_action, GSList *selected_items )
{
	static const gchar *thisfn = "nact_main_window_set_current_profile";

	g_debug( "%s: window=%p, set_action=%s, selected_items=%p",
			thisfn, ( void * ) window, set_action ? "True":"False", ( void * ) selected_items );

	if( window->private->edited_profile && set_action ){
		NAObjectAction *action = NA_OBJECT_ACTION( na_object_profile_get_action( window->private->edited_profile ));
		window->private->edited_item = NA_OBJECT_ITEM( action );
	}
}

static void
on_tab_updatable_item_updated( NactMainWindow *window, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_tab_updatable_item_updated";

	g_debug( "%s: window=%p, user_data=%p", thisfn, ( void * ) window, ( void * ) user_data );
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	if( !window->private->dispose_has_run ){

		g_signal_emit_by_name( window, IACTIONS_LIST_SIGNAL_ITEM_UPDATED, user_data );
	}
}

/*
 * called by NAPivot because this window implements the IIOConsumer
 * interface, i.e. it wish to be advertised when the list of actions
 * changes in the underlying I/O storage subsystem (typically, when we
 * save the modifications)
 *
 * note that we only reload the full list of actions when asking for a
 * reset - saving is handled on a per-action basis.
 */
static void
ipivot_consumer_on_actions_changed( NAIPivotConsumer *instance, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_ipivot_consumer_on_actions_changed";
	NactApplication *application;
	NAPivot *pivot;
	gchar *first, *second;
	gboolean ok;

	g_debug( "%s: instance=%p, user_data=%p", thisfn, ( void * ) instance, ( void * ) user_data );
	g_assert( NACT_IS_MAIN_WINDOW( instance ));

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( instance )));
	pivot = nact_application_get_pivot( application );

	first = g_strdup(_( "One or more actions have been modified in the filesystem.\n"
								"You could keep to work with your current list of actions, "
								"or you may want to reload a fresh one." ));

	if( nact_main_window_has_modified_items( NACT_MAIN_WINDOW( instance ))){
		gchar *tmp = g_strdup_printf( "%s\n\n%s", first,
				_( "Note that reloading a fresh list of actions requires "
					"that you give up with your current modifications." ));
		g_free( first );
		first = tmp;
	}

	second = g_strdup( _( "Do you want to reload a fresh list of actions ?" ));

	ok = base_window_yesno_dlg( BASE_WINDOW( instance ), GTK_MESSAGE_QUESTION, first, second );

	g_free( second );
	g_free( first );

	if( ok ){
		na_pivot_reload_items( pivot );
		nact_iactions_list_fill( NACT_IACTIONS_LIST( instance ), na_pivot_get_items( pivot ));
	}
}

/*
 * called by NAPivot via NAIPivotConsumer whenever the
 * "sort in alphabetical order" preference is modified.
 */
static void
ipivot_consumer_on_display_order_changed( NAIPivotConsumer *instance, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_ipivot_consumer_on_display_order_changed";
	/*NactMainWindow *self;*/

	g_debug( "%s: instance=%p, user_data=%p", thisfn, ( void * ) instance, ( void * ) user_data );
	g_assert( NACT_IS_MAIN_WINDOW( instance ));
	/*self = NACT_MAIN_WINDOW( instance );*/
}
