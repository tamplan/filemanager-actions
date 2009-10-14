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
#include <common/na-ipivot-consumer.h>
#include <common/na-iprefs.h>

#include "nact-application.h"
#include "nact-iactions-list.h"
#include "nact-iaction-tab.h"
#include "nact-icommand-tab.h"
#include "nact-iconditions-tab.h"
#include "nact-iadvanced-tab.h"
#include "nact-imenubar.h"
#include "nact-iprefs.h"
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
	gint             initial_count;
	GSList          *actions;
	NAAction        *edited_action;
	NAActionProfile *edited_profile;
	GSList          *deleted;
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
static void             imenubar_iface_init( NactIMenubarInterface *iface );
static void             ipivot_consumer_iface_init( NAIPivotConsumerInterface *iface );
static void             iprefs_iface_init( NAIPrefsInterface *iface );
static void             instance_init( GTypeInstance *instance, gpointer klass );
static void             instance_dispose( GObject *application );
static void             instance_finalize( GObject *application );

static gchar           *get_iprefs_window_id( NactWindow *window );
static gchar           *get_toplevel_name( BaseWindow *window );
static GSList          *get_actions( NactWindow *window );
static NAObject        *get_selected_object( NactWindow *window );

static void             on_initial_load_toplevel( BaseWindow *window );
static void             on_runtime_init_toplevel( BaseWindow *window );
static void             setup_dialog_title( NactWindow *window );

static void             on_actions_list_selection_changed( NactIActionsList *instance, GSList *selected_items );
static gboolean         on_actions_list_double_click( GtkWidget *widget, GdkEventButton *event, gpointer data );
static gboolean         on_actions_list_enter_key_pressed( GtkWidget *widget, GdkEventKey *event, gpointer data );
static void             set_current_action( NactMainWindow *window, GSList *selected_items );
static void             set_current_profile( NactMainWindow *window, gboolean set_action, GSList *selected_items );
static NAAction        *get_edited_action( NactWindow *window );
static NAActionProfile *get_edited_profile( NactWindow *window );
static void             on_modified_field( NactWindow *window );
static gboolean         is_modified_action( NactWindow *window, const NAAction *action );
static gboolean         is_valid_action( NactWindow *window, const NAAction *action );
static gboolean         is_modified_profile( NactWindow *window, const NAActionProfile *profile );
static gboolean         is_valid_profile( NactWindow *window, const NAActionProfile *profile );

static void             get_isfiledir( NactWindow *window, gboolean *isfile, gboolean *isdir );
static gboolean         get_multiple( NactWindow *window );
static GSList          *get_schemes( NactWindow *window );

static void             insert_item( NactWindow *window, NAAction *action );
static void             add_profile( NactWindow *window, NAActionProfile *profile );
static void             remove_action( NactWindow *window, NAAction *action );
static GSList          *get_deleted_actions( NactWindow *window );
static void             free_deleted_actions( NactWindow *window );
static void             push_removed_action( NactWindow *window, NAAction *action );

static void             update_actions_list( NactWindow *window );
static gboolean         on_delete_event( BaseWindow *window, GtkWindow *toplevel, GdkEvent *event );
static gint             count_actions( NactWindow *window );
static gint             count_modified_actions( NactWindow *window );
static void             reload_actions( NactWindow *window );
static GSList          *free_actions( GSList *actions );
static void             on_actions_changed( NAIPivotConsumer *instance, gpointer user_data );
static void             on_display_order_changed( NAIPivotConsumer *instance, gpointer user_data );
static void             sort_actions_list( NactMainWindow *window );

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

	static const GInterfaceInfo imenubar_iface_info = {
		( GInterfaceInitFunc ) imenubar_iface_init,
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

	g_type_add_interface_static( type, NACT_IMENUBAR_TYPE, &imenubar_iface_info );

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
	NactWindowClass *nact_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactMainWindowClassPrivate, 1 );

	base_class = BASE_WINDOW_CLASS( klass );
	base_class->get_toplevel_name = get_toplevel_name;
	base_class->initial_load_toplevel = on_initial_load_toplevel;
	base_class->runtime_init_toplevel = on_runtime_init_toplevel;
	base_class->delete_event = on_delete_event;

	nact_class = NACT_WINDOW_CLASS( klass );
	nact_class->get_iprefs_window_id = get_iprefs_window_id;
}

static void
iactions_list_iface_init( NactIActionsListInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_iactions_list_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->get_actions = get_actions;
	iface->on_selection_changed = on_actions_list_selection_changed;
	iface->on_double_click = on_actions_list_double_click;
	iface->on_delete_key_pressed = NULL;
	iface->on_enter_key_pressed = on_actions_list_enter_key_pressed;
	iface->is_modified_action = is_modified_action;
	iface->is_valid_action = is_valid_action;
	iface->is_modified_profile = is_modified_profile;
	iface->is_valid_profile = is_valid_profile;
}

static void
iaction_tab_iface_init( NactIActionTabInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_iaction_tab_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->get_edited_action = get_edited_action;
	iface->field_modified = on_modified_field;
}

static void
icommand_tab_iface_init( NactICommandTabInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_icommand_tab_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

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

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->get_edited_profile = get_edited_profile;
	iface->field_modified = on_modified_field;
}

static void
iadvanced_tab_iface_init( NactIAdvancedTabInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_iadvanced_tab_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->get_edited_profile = get_edited_profile;
	iface->field_modified = on_modified_field;
}

static void
imenubar_iface_init( NactIMenubarInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_imenubar_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->insert_item = insert_item;
	iface->add_profile = add_profile;
	iface->remove_action = remove_action;
	iface->get_deleted_actions = get_deleted_actions;
	iface->free_deleted_actions = free_deleted_actions;
	iface->push_removed_action = push_removed_action;
	iface->get_actions = get_actions;
	iface->get_selected = get_selected_object;
	iface->setup_dialog_title = setup_dialog_title;
	iface->update_actions_list = update_actions_list;
	iface->select_actions_list = nact_iactions_list_set_selection;
	iface->count_actions = count_actions;
	iface->count_modified_actions = count_modified_actions;
	iface->reload_actions = reload_actions;
}

static void
ipivot_consumer_iface_init( NAIPivotConsumerInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_ipivot_consumer_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->on_actions_changed = on_actions_changed;
	iface->on_display_order_changed = on_display_order_changed;
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

	g_assert( NACT_IS_MAIN_WINDOW( instance ));
	self = NACT_MAIN_WINDOW( instance );

	self->private = g_new0( NactMainWindowPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *window )
{
	static const gchar *thisfn = "nact_main_window_instance_dispose";
	NactMainWindow *self;
	GtkWidget *pane;
	gint pos;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );
	g_assert( NACT_IS_MAIN_WINDOW( window ));
	self = NACT_MAIN_WINDOW( window );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		pane = base_window_get_widget( BASE_WINDOW( window ), "MainPaned" );
		pos = gtk_paned_get_position( GTK_PANED( pane ));
		nact_iprefs_set_int( NACT_WINDOW( window ), "main-paned", pos );

		self->private->actions = free_actions( self->private->actions );
		self->private->deleted = free_actions( self->private->deleted );

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
	NactMainWindow *self;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );

	g_assert( NACT_IS_MAIN_WINDOW( window ));
	self = ( NactMainWindow * ) window;

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
nact_main_window_new( BaseApplication *application )
{
	g_assert( NACT_IS_APPLICATION( application ));

	return( g_object_new( NACT_MAIN_WINDOW_TYPE, PROP_WINDOW_APPLICATION_STR, application, NULL ));
}

/**
 * Returns the current list of actions
 */
GSList *
nact_main_window_get_actions( const NactMainWindow *window )
{
	return( window->private->actions );
}

/**
 * The specified action does already exist in the list ?
 */
gboolean
nact_main_window_action_exists( const NactMainWindow *window, const gchar *uuid )
{
	GSList *ia;

	for( ia = window->private->actions ; ia ; ia = ia->next ){
		NAAction *action = NA_ACTION( ia->data );
		gchar *action_uuid = na_action_get_uuid( action );
		gboolean ok = ( g_ascii_strcasecmp( action_uuid, uuid ) == 0 );
		g_free( action_uuid );
		if( ok ){
			return( TRUE );
		}
	}

	return( FALSE );
}

/**
 * Returns the status bar widget
 */
GtkStatusbar *
nact_main_window_get_statusbar( const NactMainWindow *window )
{
	GtkWidget *statusbar;

	g_assert( NACT_IS_MAIN_WINDOW( window ));

	statusbar = base_window_get_widget( BASE_WINDOW( window ), "StatusBar" );

	return( GTK_STATUSBAR( statusbar ));
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

static NAObject *
get_selected_object( NactWindow *window )
{
	NAObject *object;
	GSList *items;

	object = NULL;
	items = nact_iactions_list_get_selected_items( NACT_IACTIONS_LIST( window ));
	if( g_slist_length( items ) == 1 ){
		object = NA_OBJECT( items->data );
	}

	return( object );
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
	NactMainWindow *wnd;
	gint pos;
	GtkWidget *pane;
	/*gboolean alpha_order;*/

	/* call parent class at the very beginning */
	if( BASE_WINDOW_CLASS( st_parent_class )->initial_load_toplevel ){
		BASE_WINDOW_CLASS( st_parent_class )->initial_load_toplevel( window );
	}

	g_debug( "%s: window=%p", thisfn, ( void * ) window );
	g_assert( NACT_IS_MAIN_WINDOW( window ));
	wnd = NACT_MAIN_WINDOW( window );

	nact_imenubar_init( wnd );

	g_assert( NACT_IS_IACTIONS_LIST( window ));
	nact_iactions_list_initial_load( NACT_WINDOW( window ));
	nact_iactions_list_set_edition_mode( NACT_WINDOW( window ), TRUE );
	nact_iactions_list_set_send_selection_changed_on_fill_list( NACT_WINDOW( window ), FALSE );
	nact_iactions_list_set_multiple_selection( NACT_WINDOW( window ), TRUE );

	/*alpha_order = na_iprefs_get_alphabetical_order( NA_IPREFS( window ));
	nact_iactions_list_set_multiple_selection( NACT_WINDOW( window ), !alpha_order );
	nact_iactions_list_set_dnd_mode( NACT_WINDOW( window ), !alpha_order );*/

	g_assert( NACT_IS_IACTION_TAB( window ));
	nact_iaction_tab_initial_load( NACT_WINDOW( window ));

	g_assert( NACT_IS_ICOMMAND_TAB( window ));
	nact_icommand_tab_initial_load( NACT_WINDOW( window ));

	g_assert( NACT_IS_ICONDITIONS_TAB( window ));
	nact_iconditions_tab_initial_load( NACT_WINDOW( window ));

	g_assert( NACT_IS_IADVANCED_TAB( window ));
	nact_iadvanced_tab_initial_load( NACT_WINDOW( window ));

	pos = nact_iprefs_get_int( NACT_WINDOW( window ), "main-paned" );
	if( pos ){
		pane = base_window_get_widget( window, "MainPaned" );
		gtk_paned_set_position( GTK_PANED( pane ), pos );
	}
}

static void
on_runtime_init_toplevel( BaseWindow *window )
{
	static const gchar *thisfn = "nact_main_window_on_runtime_init_toplevel";
	NactMainWindow *wnd;
	NactApplication *application;
	NAPivot *pivot;
	GSList *ia;

	/* call parent class at the very beginning */
	if( BASE_WINDOW_CLASS( st_parent_class )->runtime_init_toplevel ){
		BASE_WINDOW_CLASS( st_parent_class )->runtime_init_toplevel( window );
	}

	g_assert( NACT_IS_MAIN_WINDOW( window ));
	g_debug( "%s: window=%p", thisfn, ( void * ) window );
	wnd = NACT_MAIN_WINDOW( window );

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( wnd )));
	pivot = nact_application_get_pivot( application );
	na_pivot_set_automatic_reload( pivot, FALSE );
	wnd->private->actions = na_pivot_get_duplicate_actions( pivot );
	wnd->private->initial_count = g_slist_length( wnd->private->actions );

	/* initialize the current edition status as a loaded action may be
	 * invalid (without having been modified)
	 */
	for( ia = wnd->private->actions ; ia ; ia = ia->next ){
		na_object_check_edited_status( NA_OBJECT( ia->data ));
	}

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

	/* forces a no-selection when the list is initially empty
	 */
	if( !wnd->private->initial_count ){
		set_current_action( NACT_MAIN_WINDOW( window ), NULL );
	} else {
		nact_iactions_list_select_first( NACT_WINDOW( window ));
	}
}

static void
setup_dialog_title( NactWindow *window )
{
	GtkWindow *toplevel;
	BaseApplication *appli = BASE_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	gchar *title = base_application_get_application_name( appli );

	if( NACT_MAIN_WINDOW( window )->private->edited_action ){
		gchar *label = na_action_get_label( NACT_MAIN_WINDOW( window )->private->edited_action );
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

	toplevel = base_window_get_toplevel_dialog( BASE_WINDOW( window ));
	gtk_window_set_title( toplevel, title );

	g_free( title );
}

static void
on_actions_list_selection_changed( NactIActionsList *instance, GSList *selected_items )
{
	static const gchar *thisfn = "nact_main_window_on_actions_list_selection_changed";
	NactMainWindow *window;
	NAObject *object;
	gint count;

	g_debug( "%s: instance=%p, selected_items=%p", thisfn, ( void * ) instance, ( void * ) selected_items );

	g_assert( NACT_IS_MAIN_WINDOW( instance ));
	window = NACT_MAIN_WINDOW( instance );

	count = g_slist_length( selected_items );

	if( count == 1 ){
		object = NA_OBJECT( selected_items->data );
		if( NA_IS_ACTION( object )){
			window->private->edited_action = NA_ACTION( object );
			set_current_action( window, selected_items );

		} else {
			g_assert( NA_IS_ACTION_PROFILE( object ));
			window->private->edited_profile = NA_ACTION_PROFILE( object );
			set_current_profile( window, TRUE, selected_items );
		}

	} else {
		window->private->edited_action = NULL;
		set_current_action( window, selected_items );
	}
}

static gboolean
on_actions_list_double_click( GtkWidget *widget, GdkEventButton *event, gpointer user_data )
{
	g_assert( event->type == GDK_2BUTTON_PRESS );

	nact_iactions_list_toggle_collapse(
			NACT_WINDOW( user_data ), NACT_MAIN_WINDOW( user_data )->private->edited_action );

	return( TRUE );
}

static gboolean
on_actions_list_enter_key_pressed( GtkWidget *widget, GdkEventKey *event, gpointer user_data )
{
	nact_iactions_list_toggle_collapse(
			NACT_WINDOW( user_data ), NACT_MAIN_WINDOW( user_data )->private->edited_action );

	return( TRUE );
}

/*
 * update the notebook when selection changes in IActionsList
 * if there is only one profile, we also setup the profile
 */
static void
set_current_action( NactMainWindow *window, GSList *selected_items )
{
	static const gchar *thisfn = "nact_main_window_set_current_action";

	g_debug( "%s: window=%p, current=%p, selected_items=%p",
			thisfn, ( void * ) window, ( void * ) window->private->edited_action, ( void * ) selected_items );

	nact_iaction_tab_set_action( NACT_WINDOW( window ), window->private->edited_action, selected_items );

	window->private->edited_profile = NULL;

	if( window->private->edited_action ){
		if( na_action_get_profiles_count( window->private->edited_action ) == 1 ){
			window->private->edited_profile = NA_ACTION_PROFILE( na_action_get_profiles( window->private->edited_action )->data );
		}
	}

	set_current_profile( window, FALSE, selected_items );
}

static void
set_current_profile( NactMainWindow *window, gboolean set_action, GSList *selected_items )
{
	static const gchar *thisfn = "nact_main_window_set_current_profile";

	g_debug( "%s: window=%p, set_action=%s, selected_items=%p",
			thisfn, ( void * ) window, set_action ? "True":"False", ( void * ) selected_items );

	if( window->private->edited_profile && set_action ){
		NAAction *action = NA_ACTION( na_action_profile_get_action( window->private->edited_profile ));
		window->private->edited_action = action;
		nact_iaction_tab_set_action( NACT_WINDOW( window ), window->private->edited_action, selected_items );
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
 * has been modified
 * - if the label has been modified, the IActionsList must reflect this
 * - setup dialog title
 */
static void
on_modified_field( NactWindow *window )
{
	g_assert( NACT_IS_MAIN_WINDOW( window ));

	na_object_check_edited_status( NA_OBJECT( NACT_MAIN_WINDOW( window )->private->edited_action ));

	setup_dialog_title( window );

	nact_iactions_list_update_selected( window, NACT_MAIN_WINDOW( window )->private->edited_action );
}

static gboolean
is_modified_action( NactWindow *window, const NAAction *action )
{
	return( na_object_get_modified_status( NA_OBJECT( action )));
}

static gboolean
is_valid_action( NactWindow *window, const NAAction *action )
{
	return( na_object_get_valid_status( NA_OBJECT( action )));
}

static gboolean
is_modified_profile( NactWindow *window, const NAActionProfile *profile )
{
	return( na_object_get_modified_status( NA_OBJECT( profile )));
}

static gboolean
is_valid_profile( NactWindow *window, const NAActionProfile *profile )
{
	return( na_object_get_valid_status( NA_OBJECT( profile )));
}

static void
get_isfiledir( NactWindow *window, gboolean *isfile, gboolean *isdir )
{
	nact_iconditions_tab_get_isfiledir( window, isfile, isdir );
}

static gboolean
get_multiple( NactWindow *window )
{
	return( nact_iconditions_tab_get_multiple( window ));
}

static GSList *
get_schemes( NactWindow *window )
{
	return( nact_iadvanced_tab_get_schemes( window ));
}

/*
 * insert an item (action or menu) in the list:
 * - the last position if the list is sorted, and sort it
 * - at the current position if the list is not sorted
 *
 * set the selection on the new item
 */
static void
insert_item( NactWindow *window, NAAction *item )
{
	NactMainWindow *wnd = NACT_MAIN_WINDOW( window );
	gboolean alpha_order;
	gchar *uuid;
	NAAction *current;
	gint index;

	alpha_order = na_iprefs_get_alphabetical_order( NA_IPREFS( window ));
	if( alpha_order ){
		wnd->private->actions = g_slist_prepend( wnd->private->actions, ( gpointer ) item );
		sort_actions_list( wnd );

	} else {
		current = get_edited_action( window );
		if( current ){
			index = g_slist_index( wnd->private->actions, current );
			g_assert( index >= 0 );
			wnd->private->actions = g_slist_insert( wnd->private->actions, item, index );
		} else {
			g_assert( g_slist_length( wnd->private->actions ) == 0 );
			wnd->private->actions = g_slist_prepend( wnd->private->actions, ( gpointer ) item );
		}
	}

	nact_iactions_list_fill( window, TRUE );

	uuid = na_action_get_uuid( item );
	nact_iactions_list_set_selection( window, NA_ACTION_TYPE, uuid, NULL );
	g_free( uuid );
}

static void
add_profile( NactWindow *window, NAActionProfile *profile )
{
	NAAction *action = na_action_profile_get_action( profile );

	if( !nact_iactions_list_is_expanded( window, action )){
		nact_iactions_list_toggle_collapse( window, action );
	}
}

static void
remove_action( NactWindow *window, NAAction *action )
{
	NactMainWindow *wnd = NACT_MAIN_WINDOW( window );
	wnd->private->actions = g_slist_remove( wnd->private->actions, ( gconstpointer ) action );
}

static GSList *
get_deleted_actions( NactWindow *window )
{
	return( NACT_MAIN_WINDOW( window )->private->deleted );
}

static void
free_deleted_actions( NactWindow *window )
{
	NactMainWindow *self = NACT_MAIN_WINDOW( window );

	self->private->deleted = free_actions( self->private->deleted );
}

static void
push_removed_action( NactWindow *window, NAAction *action )
{
	NactMainWindow *wnd = NACT_MAIN_WINDOW( window );
	wnd->private->deleted = g_slist_append( wnd->private->deleted, ( gpointer ) action );
}

static void
update_actions_list( NactWindow *window )
{
	nact_iactions_list_fill( window, TRUE );
}

static gboolean
on_delete_event( BaseWindow *window, GtkWindow *toplevel, GdkEvent *event )
{
	static const gchar *thisfn = "nact_main_window_on_delete_event";

	g_debug( "%s: window=%p, toplevel=%p, event=%p",
			thisfn, ( void * ) window, ( void * ) toplevel, ( void * ) event );
	g_assert( NACT_IS_MAIN_WINDOW( window ));

	nact_imenubar_on_delete_event( NACT_WINDOW( window ));

	return( TRUE );
}

static gint
count_actions( NactWindow *window )
{
	return( g_slist_length( NACT_MAIN_WINDOW( window )->private->actions ));
}

/*
 * exact count of modified actions is subject to some approximation
 * 1. counting the actions currently in the list is ok
 * 2. what about deleted actions ?
 *    we can create any new actions, deleting them, and so on
 *    if we have eventually deleted all newly created actions, then the
 *    final count of modified actions should be zero... don't it ?
 */
static gint
count_modified_actions( NactWindow *window )
{
	GSList *ia;
	gint count = 0;

	if( g_slist_length( NACT_MAIN_WINDOW( window )->private->actions ) == 0 &&
		NACT_MAIN_WINDOW( window )->private->initial_count == 0 ){
			return( 0 );
	}

	for( ia = NACT_MAIN_WINDOW( window )->private->deleted ; ia ; ia = ia->next ){
		if( na_object_get_origin( NA_OBJECT( ia->data )) != NULL ){
			count += 1;
		}
	}

	for( ia = NACT_MAIN_WINDOW( window )->private->actions ; ia ; ia = ia->next ){
		if( is_modified_action( window, NA_ACTION( ia->data ))){
			count += 1;
		}
	}

	return( count );
}

static void
reload_actions( NactWindow *window )
{
	NactMainWindow *self = NACT_MAIN_WINDOW( window );
	NactApplication *application;
	NAPivot *pivot;

	self->private->actions = free_actions( self->private->actions );
	self->private->deleted = free_actions( self->private->deleted );

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	pivot = nact_application_get_pivot( application );
	na_pivot_reload_actions( pivot );
	self->private->actions = na_pivot_get_duplicate_actions( pivot );
	self->private->initial_count = g_slist_length( self->private->actions );

	nact_iactions_list_fill( window, FALSE );

	if( self->private->initial_count ){
		nact_iactions_list_select_first( window );
	}
}

static GSList *
free_actions( GSList *actions )
{
	GSList *ia;
	for( ia = actions ; ia ; ia = ia->next ){
		g_object_unref( NA_ACTION( ia->data ));
	}
	g_slist_free( actions );
	return( NULL );
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
on_actions_changed( NAIPivotConsumer *instance, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_actions_changed";
	NactMainWindow *self;
	NactApplication *application;
	NAPivot *pivot;
	gchar *first, *second;
	gboolean ok;

	g_debug( "%s: instance=%p, user_data=%p", thisfn, ( void * ) instance, ( void * ) user_data );
	g_assert( NACT_IS_MAIN_WINDOW( instance ));
	self = NACT_MAIN_WINDOW( instance );

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( instance )));
	pivot = nact_application_get_pivot( application );

	first = g_strdup(_( "One or more actions have been modified in the filesystem.\n"
								"You could keep to work with your current list of actions, "
								"or you may want to reload a fresh one." ));

	if( count_modified_actions( NACT_WINDOW( instance )) > 0 ){
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

		na_pivot_reload_actions( pivot );

		na_pivot_free_actions( self->private->actions );

		self->private->actions = na_pivot_get_duplicate_actions( pivot );

		nact_iactions_list_fill( NACT_WINDOW( instance ), TRUE );
	}
}

/*
 * called by NAPivot via NAIPivotConsumer whenever the
 * "sort in alphabetical order" preference is modified.
 */
static void
on_display_order_changed( NAIPivotConsumer *instance, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_display_order_changed";
	/*NactMainWindow *self;*/
	gboolean alpha_order;

	g_debug( "%s: instance=%p, user_data=%p", thisfn, ( void * ) instance, ( void * ) user_data );
	g_assert( NACT_IS_MAIN_WINDOW( instance ));
	/*self = NACT_MAIN_WINDOW( instance );*/

	alpha_order = na_iprefs_get_alphabetical_order( NA_IPREFS( instance ));

	nact_iactions_list_set_multiple_selection( NACT_WINDOW( instance ), !alpha_order );
	nact_iactions_list_set_dnd_mode( NACT_WINDOW( instance ), !alpha_order );
}

static void
sort_actions_list( NactMainWindow *window )
{
	NactApplication *application;
	NAPivot *pivot;

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	pivot = nact_application_get_pivot( application );
	window->private->actions = na_iio_provider_sort_actions( pivot, window->private->actions );
}
