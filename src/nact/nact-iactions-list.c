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

#include <gdk/gdkkeysyms.h>

#include <common/na-action.h>

#include "nact-application.h"
#include "nact-iactions-list.h"

/* private interface data
 */
struct NactIActionsListInterfacePrivate {
};

/* column ordering
 */
enum {
	IACTIONS_LIST_ICON_COLUMN = 0,
	IACTIONS_LIST_LABEL_COLUMN,
	IACTIONS_LIST_NAOBJECT_COLUMN,
	IACTIONS_LIST_N_COLUMN
};

/* data set against GObject
 */
#define ACCEPT_MULTIPLE_SELECTION		"iactions-list-accept-multiple-selection"
#define IS_FILLING_LIST					"iactions-list-is-filling-list"
#define SEND_SELECTION_CHANGED_MESSAGE	"iactions-list-send-selection-changed-message"

static GType      register_type( void );
static void       interface_base_init( NactIActionsListInterface *klass );
static void       interface_base_finalize( NactIActionsListInterface *klass );

static GSList    *v_get_actions( NactWindow *window );
static void       v_set_sorted_actions( NactWindow *window, GSList *actions );
static void       v_on_selection_changed( GtkTreeSelection *selection, gpointer user_data );
static gboolean   v_on_button_press_event( GtkWidget *widget, GdkEventButton *event, gpointer data );
static gboolean   v_on_key_pressed_event( GtkWidget *widget, GdkEventKey *event, gpointer data );

static gboolean   filter_visible( GtkTreeModel *model, GtkTreeIter *iter, gpointer data );
static GtkWidget *get_actions_list_widget( NactWindow *window );
static gint       sort_actions_by_label( gconstpointer a1, gconstpointer a2 );

GType
nact_iactions_list_get_type( void )
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
	static const gchar *thisfn = "nact_iactions_list_register_type";
	g_debug( "%s", thisfn );

	static const GTypeInfo info = {
		sizeof( NactIActionsListInterface ),
		( GBaseInitFunc ) interface_base_init,
		( GBaseFinalizeFunc ) interface_base_finalize,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL
	};

	GType type = g_type_register_static( G_TYPE_INTERFACE, "NactIActionsList", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( NactIActionsListInterface *klass )
{
	static const gchar *thisfn = "nact_iactions_list_interface_base_init";
	static gboolean initialized = FALSE;

	if( !initialized ){
		g_debug( "%s: klass=%p", thisfn, klass );

		klass->private = g_new0( NactIActionsListInterfacePrivate, 1 );

		initialized = TRUE;
	}
}

static void
interface_base_finalize( NactIActionsListInterface *klass )
{
	static const gchar *thisfn = "nact_iactions_list_interface_base_finalize";
	static gboolean finalized = FALSE ;

	if( !finalized ){
		g_debug( "%s: klass=%p", thisfn, klass );

		g_free( klass->private );

		finalized = TRUE;
	}
}

/**
 * Allocates and initializes the ActionsList widget.
 */
void
nact_iactions_list_initial_load( NactWindow *window )
{
	static const gchar *thisfn = "nact_iactions_list_initial_load";
	g_debug( "%s: window=%p", thisfn, window );

	g_assert( NACT_IS_IACTIONS_LIST( window ));
	g_assert( NACT_IS_WINDOW( window ));

	nact_iactions_list_set_send_selection_changed_on_fill_list( window, FALSE );
	nact_iactions_list_set_is_filling_list( window, FALSE );

	GtkWidget *widget = get_actions_list_widget( window );

	/* create the model */
	GtkTreeStore *ts_model = gtk_tree_store_new(
			IACTIONS_LIST_N_COLUMN, GDK_TYPE_PIXBUF, G_TYPE_STRING, NA_OBJECT_TYPE );

	GtkTreeModel *tmf_model = gtk_tree_model_filter_new( GTK_TREE_MODEL( ts_model ), NULL );

	gtk_tree_model_filter_set_visible_func(
			GTK_TREE_MODEL_FILTER( tmf_model ), ( GtkTreeModelFilterVisibleFunc ) filter_visible, window, NULL );

	gtk_tree_view_set_model( GTK_TREE_VIEW( widget ), tmf_model );
	gtk_tree_view_set_enable_tree_lines( GTK_TREE_VIEW( widget ), TRUE );

	/*g_object_unref( tmf_model );*/
	g_object_unref( ts_model );

	/* create visible columns on the tree view */
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
			"icon", gtk_cell_renderer_pixbuf_new(), "pixbuf", IACTIONS_LIST_ICON_COLUMN, NULL );
	gtk_tree_view_append_column( GTK_TREE_VIEW( widget ), column );

	column = gtk_tree_view_column_new_with_attributes(
			"label", gtk_cell_renderer_text_new(), "text", IACTIONS_LIST_LABEL_COLUMN, NULL );
	gtk_tree_view_append_column( GTK_TREE_VIEW( widget ), column );
}

/**
 * Allocates and initializes the ActionsList widget.
 */
void
nact_iactions_list_runtime_init( NactWindow *window )
{
	static const gchar *thisfn = "nact_iactions_list_runtime_init";
	g_debug( "%s: window=%p", thisfn, window );

	g_assert( NACT_IS_IACTIONS_LIST( window ));
	g_assert( NACT_IS_WINDOW( window ));

	GtkWidget *widget = get_actions_list_widget( window );
	g_assert( GTK_IS_WIDGET( widget ));

	nact_iactions_list_fill( window );

	/* set up selection */
	nact_window_signal_connect(
			window,
			G_OBJECT( gtk_tree_view_get_selection( GTK_TREE_VIEW( widget ))),
			"changed",
			G_CALLBACK( v_on_selection_changed ));

	/* catch press 'Enter' */
	nact_window_signal_connect(
			window,
			G_OBJECT( widget ),
			"key-press-event",
			G_CALLBACK( v_on_key_pressed_event ));

	/* catch double-click */
	nact_window_signal_connect(
			window,
			G_OBJECT( widget ),
			"button-press-event",
			G_CALLBACK( v_on_button_press_event ));

	/* clear the selection */
	GtkTreeSelection *selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( widget ));
	gtk_tree_selection_unselect_all( selection );
}

/**
 * Fill the listbox with current actions.
 */
void
nact_iactions_list_fill( NactWindow *window )
{
	static const gchar *thisfn = "nact_iactions_list_fill";
	g_debug( "%s: window=%p", thisfn, window );

	g_assert( NACT_IS_IACTIONS_LIST( window ));

	nact_iactions_list_set_is_filling_list( window, TRUE );

	GtkWidget *widget = get_actions_list_widget( window );
	GtkTreeModelFilter *tmf_model = GTK_TREE_MODEL_FILTER( gtk_tree_view_get_model( GTK_TREE_VIEW( widget )));
	GtkTreeStore *ts_model = GTK_TREE_STORE( gtk_tree_model_filter_get_model( tmf_model ));
	gtk_tree_store_clear( ts_model );

	GSList *actions = v_get_actions( window );
	actions = g_slist_sort( actions, ( GCompareFunc ) sort_actions_by_label );
	v_set_sorted_actions( window, actions );

	GSList *ia;
	/*g_debug( "%s: actions has %d elements", thisfn, g_slist_length( actions ));*/

	for( ia = actions ; ia != NULL ; ia = ia->next ){
		GtkTreeIter iter;
		GtkStockItem item;
		GdkPixbuf* icon = NULL;

		NAAction *action = NA_ACTION( ia->data );
		gchar *label = na_action_get_label( action );
		gchar *iconname = na_action_get_icon( action );

		/* TODO: use the same algorythm than Nautilus to find and
		 * display an icon + move the code to NAAction class +
		 * remove na_action_get_verified_icon_name
		 */
		if( iconname ){
			if( gtk_stock_lookup( iconname, &item )){
				icon = gtk_widget_render_icon( widget, iconname, GTK_ICON_SIZE_MENU, NULL );

			} else if( g_file_test( iconname, G_FILE_TEST_EXISTS )
				   && g_file_test( iconname, G_FILE_TEST_IS_REGULAR )){
				gint width;
				gint height;
				GError* error = NULL;

				gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &width, &height);
				icon = gdk_pixbuf_new_from_file_at_size( iconname, width, height, &error );
				if( error ){
					g_warning( "%s: iconname=%s, error=%s", thisfn, iconname, error->message );
					g_error_free( error );
					error = NULL;
					icon = NULL;
				}
			}
		}
		gtk_tree_store_append( ts_model, &iter, NULL );
		gtk_tree_store_set( ts_model, &iter,
				    IACTIONS_LIST_ICON_COLUMN, icon,
				    IACTIONS_LIST_LABEL_COLUMN, label,
				    IACTIONS_LIST_NAOBJECT_COLUMN, action,
				    -1);
		g_debug( "%s: action=%p", thisfn, action );

		GSList *profiles = na_action_get_profiles( action );
		GSList *ip;
		GtkTreeIter profile_iter;
		for( ip = profiles ; ip ; ip = ip->next ){
			NAActionProfile *profile = NA_ACTION_PROFILE( ip->data );
			gchar *profile_label = na_action_profile_get_label( profile );
			gtk_tree_store_append( ts_model, &profile_iter, &iter );
			gtk_tree_store_set( ts_model, &profile_iter,
					    IACTIONS_LIST_LABEL_COLUMN, profile_label,
					    IACTIONS_LIST_NAOBJECT_COLUMN, profile,
					    -1);
			g_free( profile_label );
		}

		g_free( iconname );
		g_free( label );
	}
	/*g_debug( "%s: at end, actions has %d elements", thisfn, g_slist_length( actions ));*/

	nact_iactions_list_set_is_filling_list( window, FALSE );
}

/**
 * Set the selection to the named action.
 * If not found, we select the first following, else the previous one.
 */
void
nact_iactions_list_set_selection( NactWindow *window, const gchar *uuid, const gchar *label )
{
	static const gchar *thisfn = "nact_iactions_list_set_selection";
	g_debug( "%s: window=%p, uuid=%s, label=%s", thisfn, window, uuid, label );

	GtkWidget *list = get_actions_list_widget( window );
	GtkTreeSelection *selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( list ));
	GtkTreeModel *model = gtk_tree_view_get_model( GTK_TREE_VIEW( list ));
	GtkTreeIter iter, previous;

	gtk_tree_selection_unselect_all( selection );

	gboolean iterok = gtk_tree_model_get_iter_first( model, &iter );
	gboolean found = FALSE;
	NAAction *action;
	gchar *iter_uuid, *iter_label;
	gint count = 0;

	while( iterok && !found ){
		count += 1;
		gtk_tree_model_get(
				model,
				&iter,
				IACTIONS_LIST_NAOBJECT_COLUMN, &action, IACTIONS_LIST_LABEL_COLUMN, &iter_label,
				-1 );

		iter_uuid = na_action_get_uuid( action );
		gint ret_uuid = g_ascii_strcasecmp( iter_uuid, uuid );
		gint ret_label = g_utf8_collate( iter_label, label );
		if(( ret_uuid == 0 && ret_label == 0 ) || ret_label > 0 ){
			gtk_tree_selection_select_iter( selection, &iter );
			found = TRUE;

		} else {
			previous = iter;
		}

		g_free( iter_uuid );
		g_free( iter_label );
		iterok = gtk_tree_model_iter_next( model, &iter );
	}

	if( !found && count ){
		gtk_tree_selection_select_iter( selection, &previous );
	}
}

/**
 * Reset the focus on the ActionsList listbox.
 */
void
nact_iactions_list_set_focus( NactWindow *window )
{
	GtkWidget *list = get_actions_list_widget( window );
	gtk_widget_grab_focus( list );
}

/**
 * Returns the currently selected action or profile.
 */
NAObject *
nact_iactions_list_get_selected_action( NactWindow *window )
{
	GSList *list = nact_iactions_list_get_selected_actions( window );

	NAObject *object = NA_OBJECT( list->data );

	g_assert( object );
	g_assert( NA_IS_OBJECT( object ));

	g_slist_free( list );

	return( object );
}

/**
 * Returns the currently selected actions when in export mode.
 *
 * The returned GSList should be freed by the caller (g_slist_free),
 * without freing not unref any of the contained objects.
 */
GSList *
nact_iactions_list_get_selected_actions( NactWindow *window )
{
	GSList *actions = NULL;

	GtkWidget *treeview = get_actions_list_widget( window );
	GtkTreeSelection *selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( treeview ));

	GtkTreeModel *model;
	GtkTreeIter iter;
	GList *it;
	NAAction *action;

	GList *listrows = gtk_tree_selection_get_selected_rows( selection, &model );
	for( it = listrows ; it ; it = it->next ){
		GtkTreePath *path = ( GtkTreePath * ) it->data;
		gtk_tree_model_get_iter( model, &iter, path );

		gtk_tree_model_get( model, &iter, IACTIONS_LIST_NAOBJECT_COLUMN, &action, -1 );
		actions = g_slist_prepend( actions, action );
	}

	g_list_foreach( listrows, ( GFunc ) gtk_tree_path_free, NULL );
	g_list_free( listrows );

	return( actions );
}


void
nact_iactions_list_set_modified( NactWindow *window, gboolean is_modified, gboolean can_save )
{
}

/**
 * Does the IActionsList box support multiple selection ?
 */
void
nact_iactions_list_set_multiple_selection( NactWindow *window, gboolean multiple )
{
	g_assert( NACT_IS_IACTIONS_LIST( window ));
	g_object_set_data( G_OBJECT( window ), ACCEPT_MULTIPLE_SELECTION, GINT_TO_POINTER( multiple ));

	GtkWidget *list = get_actions_list_widget( window );
	GtkTreeSelection *selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( list ));
	gtk_tree_selection_set_mode( selection, multiple ? GTK_SELECTION_MULTIPLE : GTK_SELECTION_SINGLE );
}

/**
 * Should the IActionsList interface trigger a 'on_selection_changed'
 * message when the ActionsList list is filled ?
 */
void
nact_iactions_list_set_send_selection_changed_on_fill_list( NactWindow *window, gboolean send_message )
{
	g_assert( NACT_IS_IACTIONS_LIST( window ));
	g_object_set_data( G_OBJECT( window ), SEND_SELECTION_CHANGED_MESSAGE, GINT_TO_POINTER( send_message ));
}

/**
 * Is the IActionsList interface currently filling the ActionsList ?
 */
void
nact_iactions_list_set_is_filling_list( NactWindow *window, gboolean is_filling )
{
	g_assert( NACT_IS_IACTIONS_LIST( window ));
	g_object_set_data( G_OBJECT( window ), IS_FILLING_LIST, GINT_TO_POINTER( is_filling ));
}

static GSList *
v_get_actions( NactWindow *window )
{
	g_assert( NACT_IS_IACTIONS_LIST( window ));
	NactIActionsList *instance = NACT_IACTIONS_LIST( window );

	if( NACT_IACTIONS_LIST_GET_INTERFACE( instance )->get_actions ){
		return( NACT_IACTIONS_LIST_GET_INTERFACE( instance )->get_actions( window ));
	}

	return( NULL );
}

static void
v_set_sorted_actions( NactWindow *window, GSList *actions )
{
	g_assert( NACT_IS_IACTIONS_LIST( window ));
	NactIActionsList *instance = NACT_IACTIONS_LIST( window );

	if( NACT_IACTIONS_LIST_GET_INTERFACE( instance )->set_sorted_actions ){
		NACT_IACTIONS_LIST_GET_INTERFACE( instance )->set_sorted_actions( window, actions );
	}
}

static void
v_on_selection_changed( GtkTreeSelection *selection, gpointer user_data )
{
	g_assert( NACT_IS_IACTIONS_LIST( user_data ));
	g_assert( BASE_IS_WINDOW( user_data ));

	NactIActionsList *instance = NACT_IACTIONS_LIST( user_data );

	g_assert( NACT_IS_WINDOW( user_data ));
	gboolean send_message = GPOINTER_TO_INT( g_object_get_data( G_OBJECT( user_data ), SEND_SELECTION_CHANGED_MESSAGE ));
	gboolean is_filling = GPOINTER_TO_INT( g_object_get_data( G_OBJECT( user_data ), IS_FILLING_LIST ));

	if( send_message || !is_filling ){
		if( NACT_IACTIONS_LIST_GET_INTERFACE( instance )->on_selection_changed ){
			NACT_IACTIONS_LIST_GET_INTERFACE( instance )->on_selection_changed( selection, user_data );
		}
	}
}

static gboolean
v_on_button_press_event( GtkWidget *widget, GdkEventButton *event, gpointer user_data )
{
	/*static const gchar *thisfn = "nact_iactions_list_v_on_button_pres_event";
	g_debug( "%s: widget=%p, event=%p, user_data=%p", thisfn, widget, event, user_data );*/

	g_assert( NACT_IS_IACTIONS_LIST( user_data ));
	g_assert( NACT_IS_WINDOW( user_data ));

	gboolean stop = FALSE;
	NactIActionsList *instance = NACT_IACTIONS_LIST( user_data );

	if( NACT_IACTIONS_LIST_GET_INTERFACE( instance )->on_button_press_event ){
		stop = NACT_IACTIONS_LIST_GET_INTERFACE( instance )->on_button_press_event( widget, event, user_data );
	}

	if( !stop ){
		if( event->type == GDK_2BUTTON_PRESS ){
			if( NACT_IACTIONS_LIST_GET_INTERFACE( instance )->on_double_click ){
				stop = NACT_IACTIONS_LIST_GET_INTERFACE( instance )->on_double_click( widget, event, user_data );
			}
		}
	}
	return( stop );
}

static gboolean
v_on_key_pressed_event( GtkWidget *widget, GdkEventKey *event, gpointer user_data )
{
	/*static const gchar *thisfn = "nact_iactions_list_v_on_key_pressed_event";
	g_debug( "%s: widget=%p, event=%p, user_data=%p", thisfn, widget, event, user_data );*/

	g_assert( NACT_IS_IACTIONS_LIST( user_data ));
	g_assert( NACT_IS_WINDOW( user_data ));
	g_assert( event->type == GDK_KEY_PRESS );

	gboolean stop = FALSE;
	NactIActionsList *instance = NACT_IACTIONS_LIST( user_data );

	if( NACT_IACTIONS_LIST_GET_INTERFACE( instance )->on_key_pressed_event ){
		stop = NACT_IACTIONS_LIST_GET_INTERFACE( instance )->on_key_pressed_event( widget, event, user_data );
	}

	if( !stop ){
		if( event->keyval == GDK_Return || event->keyval == GDK_KP_Enter ){
			if( NACT_IACTIONS_LIST_GET_INTERFACE( instance )->on_enter_key_pressed ){
				stop = NACT_IACTIONS_LIST_GET_INTERFACE( instance )->on_enter_key_pressed( widget, event, user_data );
			}
		}
	}
	return( stop );
}

static gboolean
filter_visible( GtkTreeModel *model, GtkTreeIter *iter, gpointer data )
{
	NAObject *object;
	gtk_tree_model_get( model, iter, IACTIONS_LIST_NAOBJECT_COLUMN, &object, -1 );

	if( object ){
		if( NA_IS_ACTION( object )){
			return( TRUE );
		}

		g_assert( NA_IS_ACTION_PROFILE( object ));
		NAAction *action = NA_ACTION( na_action_profile_get_action( NA_ACTION_PROFILE( object )));
		return( na_action_get_profiles_count( action ) > 1 );
	}

	return( FALSE );
}

static GtkWidget *
get_actions_list_widget( NactWindow *window )
{
	return( base_window_get_widget( BASE_WINDOW( window ), "ActionsList" ));
}

static gint
sort_actions_by_label( gconstpointer a1, gconstpointer a2 )
{
	NAAction *action1 = NA_ACTION( a1 );
	gchar *label1 = na_action_get_label( action1 );

	NAAction *action2 = NA_ACTION( a2 );
	gchar *label2 = na_action_get_label( action2 );

	gint ret = g_utf8_collate( label1, label2 );

	g_free( label1 );
	g_free( label2 );

	return( ret );
}
