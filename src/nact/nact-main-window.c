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
};

/* private instance data
 */
struct NactMainWindowPrivate {
	gboolean         dispose_has_run;
	GtkStatusbar    *status_bar;
	guint            status_context;
	gint             initial_count;
	GSList          *actions;
	NAAction        *edited_action;
	NAActionProfile *edited_profile;
	GSList          *deleted;
	GTimeVal         last_saved;
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
static void             instance_init( GTypeInstance *instance, gpointer klass );
static void             instance_dispose( GObject *application );
static void             instance_finalize( GObject *application );

static gchar           *get_iprefs_window_id( NactWindow *window );
static gchar           *get_toplevel_name( BaseWindow *window );
static GSList          *get_actions( NactWindow *window );
static GtkWidget       *get_status_bar( NactWindow *window );

static void             on_initial_load_toplevel( BaseWindow *window );
static void             on_runtime_init_toplevel( BaseWindow *window );
static void             setup_dialog_title( NactWindow *window );
/*static void             setup_dialog_menu( NactMainWindow *window );*/

static void             on_actions_list_selection_changed( GtkTreeSelection *selection, gpointer user_data );
static gboolean         on_actions_list_double_click( GtkWidget *widget, GdkEventButton *event, gpointer data );
static gboolean         on_actions_list_delete_key_pressed( GtkWidget *widget, GdkEventKey *event, gpointer data );
static gboolean         on_actions_list_enter_key_pressed( GtkWidget *widget, GdkEventKey *event, gpointer data );
static void             set_current_action( NactMainWindow *window );
static void             set_current_profile( NactMainWindow *window, gboolean set_action );
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

static void             add_action( NactWindow *window, NAAction *action );
static void             add_profile( NactWindow *window, NAActionProfile *profile );
static void             remove_action( NactWindow *window, NAAction *action );
static GSList          *get_deleted_actions( NactWindow *window );
static void             free_deleted_actions( NactWindow *window );
static void             push_removed_action( NactWindow *window, NAAction *action );

/*static void     on_new_button_clicked( GtkButton *button, gpointer user_data );
static void     on_edit_button_clicked( GtkButton *button, gpointer user_data );
static void     on_duplicate_button_clicked( GtkButton *button, gpointer user_data );
static void     on_delete_button_clicked( GtkButton *button, gpointer user_data );
static gboolean on_dialog_response( GtkDialog *dialog, gint response_id, BaseWindow *window );*/

static void             update_actions_list( NactWindow *window );
static gboolean         on_delete_event( BaseWindow *window, GtkWindow *toplevel, GdkEvent *event );
static gint             count_actions( NactWindow *window );
static gint             count_modified_actions( NactWindow *window );
static void             reload_actions( NactWindow *window );
static GSList          *free_actions( GSList *actions );
static void             on_save( NactWindow *window );
static void             on_actions_changed( NAIPivotConsumer *instance, gpointer user_data );

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

	/* implement IMenubar interface
	 */
	static const GInterfaceInfo imenubar_iface_info = {
		( GInterfaceInitFunc ) imenubar_iface_init,
		NULL,
		NULL
	};
	g_type_add_interface_static( type, NACT_IMENUBAR_TYPE, &imenubar_iface_info );

	/* implement IPivotConsumer interface
	 */
	static const GInterfaceInfo pivot_consumer_iface_info = {
		( GInterfaceInitFunc ) ipivot_consumer_iface_init,
		NULL,
		NULL
	};
	g_type_add_interface_static( type, NA_IPIVOT_CONSUMER_TYPE, &pivot_consumer_iface_info );

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
	iface->on_selection_changed = on_actions_list_selection_changed;
	iface->on_double_click = on_actions_list_double_click;
	iface->on_delete_key_pressed = on_actions_list_delete_key_pressed;
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
	g_debug( "%s: iface=%p", thisfn, iface );

	iface->get_status_bar = get_status_bar;
	iface->get_selected = nact_iactions_list_get_selected_object;
	iface->get_edited_action = get_edited_action;
	iface->field_modified = on_modified_field;
}

static void
icommand_tab_iface_init( NactICommandTabInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_icommand_tab_iface_init";
	g_debug( "%s: iface=%p", thisfn, iface );

	iface->get_status_bar = get_status_bar;
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
imenubar_iface_init( NactIMenubarInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_imenubar_iface_init";
	g_debug( "%s: iface=%p", thisfn, iface );

	iface->add_action = add_action;
	iface->add_profile = add_profile;
	iface->remove_action = remove_action;
	iface->get_deleted_actions = get_deleted_actions;
	iface->free_deleted_actions = free_deleted_actions;
	iface->push_removed_action = push_removed_action;
	iface->get_actions = get_actions;
	iface->get_selected = nact_iactions_list_get_selected_object;
	iface->get_status_bar = get_status_bar;
	iface->setup_dialog_title = setup_dialog_title;
	iface->update_actions_list = update_actions_list;
	iface->select_actions_list = nact_iactions_list_set_selection;
	iface->count_actions = count_actions;
	iface->count_modified_actions = count_modified_actions;
	iface->reload_actions = reload_actions;
	iface->on_save = on_save;
}

static void
ipivot_consumer_iface_init( NAIPivotConsumerInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_ipivot_consumer_iface_init";
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

static GtkWidget *
get_status_bar( NactWindow *window )
{
	g_assert( NACT_IS_MAIN_WINDOW( window ));
	return( GTK_WIDGET( NACT_MAIN_WINDOW( window )->private->status_bar ));
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
	NAPivot *pivot = nact_application_get_pivot( application );
	na_pivot_set_automatic_reload( pivot, FALSE );
	wnd->private->actions = na_pivot_get_duplicate_actions( pivot );
	wnd->private->initial_count = g_slist_length( wnd->private->actions );

	g_get_current_time( &wnd->private->last_saved );

	wnd->private->status_bar = GTK_STATUSBAR( base_window_get_widget( window, "StatusBar" ));
	wnd->private->status_context = gtk_statusbar_get_context_id( wnd->private->status_bar, "nact-main-window" );

	nact_imenubar_init( wnd );

	g_assert( NACT_IS_IACTIONS_LIST( window ));
	nact_iactions_list_initial_load( NACT_WINDOW( window ));
	nact_iactions_list_set_edition_mode( NACT_WINDOW( window ), TRUE );
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

	/* forces a no-selection when the list is initially empty
	 */
	if( !g_slist_length( NACT_MAIN_WINDOW( window )->private->actions )){
		set_current_action( NACT_MAIN_WINDOW( window ));
	}
}

static void
setup_dialog_title( NactWindow *window )
{
	BaseApplication *appli = BASE_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	gchar *title = base_application_get_name( appli );

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

	GtkWindow *toplevel = base_window_get_toplevel_dialog( BASE_WINDOW( window ));
	gtk_window_set_title( toplevel, title );

	g_free( title );
}

/*static void
setup_dialog_menu( NactMainWindow *window )
{
	GSList *ia;
	gboolean to_save = FALSE;
	for( ia = window->private->actions ; ia && !to_save ; ia = ia->next ){
		gboolean elt_to_save = is_valid_action( NACT_WINDOW( window ), NA_ACTION( ia->data ));
		to_save |= elt_to_save;
	}

	gtk_widget_set_sensitive(  window->private->new_profile_item, window->private->edited_action != NULL );

	gtk_widget_set_sensitive( window->private->save_item, to_save );
}*/

/*
 * note that the IActionsList tree store may return an action or a profile
 */
static void
on_actions_list_selection_changed( GtkTreeSelection *selection, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_actions_list_selection_changed";
	g_debug( "%s: selection=%p, user_data=%p", thisfn, selection, user_data );

	g_assert( NACT_IS_MAIN_WINDOW( user_data ));
	NactMainWindow *window = NACT_MAIN_WINDOW( user_data );

	NAObject *object = nact_iactions_list_get_selected_object( NACT_WINDOW( window ));
	g_debug( "%s: object=%p", thisfn, object );

	if( object ){
		if( NA_IS_ACTION( object )){
			window->private->edited_action = NA_ACTION( object );
			set_current_action( window );

		} else {
			g_assert( NA_IS_ACTION_PROFILE( object ));
			window->private->edited_profile = NA_ACTION_PROFILE( object );
			set_current_profile( window, TRUE );
		}

	} else {
		window->private->edited_action = NULL;
		set_current_action( window );
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
on_actions_list_delete_key_pressed( GtkWidget *widget, GdkEventKey *event, gpointer user_data )
{
	if( NACT_MAIN_WINDOW( user_data )->private->edited_action ){
		nact_imenubar_on_delete_key_pressed( NACT_WINDOW( user_data ));
		return( TRUE );
	}

	return( FALSE );
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
set_current_action( NactMainWindow *window )
{
	g_debug( "set_current_action: current=%p", window->private->edited_action );

	nact_iaction_tab_set_action( NACT_WINDOW( window ), window->private->edited_action );

	window->private->edited_profile = NULL;

	if( window->private->edited_action ){
		if( na_action_get_profiles_count( window->private->edited_action ) == 1 ){
			window->private->edited_profile = NA_ACTION_PROFILE( na_action_get_profiles( window->private->edited_action )->data );
		}
	}

	set_current_profile( window, FALSE );
}

static void
set_current_profile( NactMainWindow *window, gboolean set_action )
{
	if( window->private->edited_profile && set_action ){
		NAAction *action = NA_ACTION( na_action_profile_get_action( window->private->edited_profile ));
		window->private->edited_action = action;
		nact_iaction_tab_set_action( NACT_WINDOW( window ), window->private->edited_action );
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

static void
add_action( NactWindow *window, NAAction *action )
{
	NactMainWindow *wnd = NACT_MAIN_WINDOW( window );
	wnd->private->actions = g_slist_prepend( wnd->private->actions, ( gpointer ) action );
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
update_actions_list( NactWindow *window )
{
	nact_iactions_list_fill( window, TRUE );
}

static gboolean
on_delete_event( BaseWindow *window, GtkWindow *toplevel, GdkEvent *event )
{
	static const gchar *thisfn = "nact_main_window_on_delete_event";
	g_debug( "%s: window=%p, toplevel=%p, event=%p", thisfn, window, toplevel, event );

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
	if( g_slist_length( NACT_MAIN_WINDOW( window )->private->actions ) == 0 &&
		NACT_MAIN_WINDOW( window )->private->initial_count == 0 ){
			return( 0 );
	}

	GSList *ia;
	gint count = 0;

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
	self->private->actions = free_actions( self->private->actions );
	self->private->deleted = free_actions( self->private->deleted );

	NactApplication *application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	NAPivot *pivot = nact_application_get_pivot( application );
	na_pivot_reload_actions( pivot );
	self->private->actions = na_pivot_get_duplicate_actions( pivot );
	self->private->initial_count = g_slist_length( self->private->actions );
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
 * in initial_load_toplevel(), we have forbidden the automatic reload
 * of the list of actions in NAPivot, as we take care of updating it of
 * the modifications entered in the UI
 *
 * this doesn't prevent NAPivot to advertise us when it detects some
 * modifications in an I/O provider ; so that we are able to ask the
 * user for a reload
 *
 * but we don't want be advertized when this is our own save which
 * triggers the NAPivot advertising - so we forbids such advertisings
 * during one seconde after each save
 *
 * note that last_saved is initialized in initial_load_toplevel()
 * there is so a race condition if NAPivot detects a modification
 * in the seconde after this initialization - just ignore this case
 */
static void
on_save( NactWindow *window )
{
	g_get_current_time( &NACT_MAIN_WINDOW( window )->private->last_saved );
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
	g_debug( "%s: instance=%p, user_data=%p", thisfn, instance, user_data );

	g_assert( NACT_IS_MAIN_WINDOW( instance ));
	NactMainWindow *self = NACT_MAIN_WINDOW( instance );

	GTimeVal now;
	g_get_current_time( &now );
	glong ecart = 1000000 * ( now.tv_sec - self->private->last_saved.tv_sec );
	ecart += now.tv_usec - self->private->last_saved.tv_usec;
	if( ecart < 1000000 ){
		return;
	}

	NactApplication *application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( instance )));
	NAPivot *pivot = nact_application_get_pivot( application );

	gchar *first = g_strdup(_( "One or more actions have been modified in the filesystem.\n"
								"You could keep to work with your current list of actions, "
								"or you may want to reload a fresh one." ));

	if( count_modified_actions( NACT_WINDOW( instance )) > 0 ){
		gchar *tmp = g_strdup_printf( "%s\n\n%s", first,
				_( "Note that reloading a fresh list of actions requires "
					"that you give up with your current modifications." ));
		g_free( first );
		first = tmp;
	}

	gchar *second = g_strdup( _( "Do you want to reload a fresh list of actions ?" ));

	gboolean ok = base_window_yesno_dlg( BASE_WINDOW( instance ), GTK_MESSAGE_QUESTION, first, second );

	g_free( second );
	g_free( first );

	if( ok ){

		na_pivot_reload_actions( pivot );

		na_pivot_free_actions( self->private->actions );

		self->private->actions = na_pivot_get_duplicate_actions( pivot );

		nact_iactions_list_fill( NACT_WINDOW( instance ), TRUE );
	}
}
