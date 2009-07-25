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
#include <string.h>

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
static void       v_on_selection_changed( GtkTreeSelection *selection, gpointer user_data );
static gboolean   v_on_button_press_event( GtkWidget *widget, GdkEventButton *event, gpointer data );
static gboolean   v_on_key_pressed_event( GtkWidget *widget, GdkEventKey *event, gpointer data );
static gboolean   v_is_modified_action( NactWindow *window, const NAAction *action );
static gboolean   v_is_valid_action( NactWindow *window, const NAAction *action );
static gboolean   v_is_modified_profile( NactWindow *window, const NAActionProfile *profile );
static gboolean   v_is_valid_profile( NactWindow *window, const NAActionProfile *profile );

static void       display_label( GtkTreeViewColumn *column, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, NactWindow *window );
static void       setup_action( GtkWidget *treeview, GtkTreeStore *model, GtkTreeIter *iter, NAAction *action );
static void       setup_profile( GtkWidget *treeview, GtkTreeStore *model, GtkTreeIter *iter, NAActionProfile *profile );
static gint       sort_actions_list( GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, NactWindow *window );
static gboolean   filter_visible( GtkTreeModel *model, GtkTreeIter *iter, gpointer data );
static GtkWidget *get_actions_list_widget( NactWindow *window );
static GSList    *get_expanded_rows( NactWindow *window );
static void       expand_rows( NactWindow *window, GSList *expanded );
static void       free_expanded_list( GSList *expanded );

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

	gtk_tree_sortable_set_default_sort_func(
			GTK_TREE_SORTABLE( ts_model ),
	        ( GtkTreeIterCompareFunc ) sort_actions_list, window, NULL );

	gtk_tree_sortable_set_sort_column_id(
			GTK_TREE_SORTABLE( ts_model ),
			IACTIONS_LIST_LABEL_COLUMN, GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID );

	GtkTreeModel *tmf_model = gtk_tree_model_filter_new( GTK_TREE_MODEL( ts_model ), NULL );

	gtk_tree_model_filter_set_visible_func(
			GTK_TREE_MODEL_FILTER( tmf_model ), ( GtkTreeModelFilterVisibleFunc ) filter_visible, window, NULL );

	gtk_tree_view_set_model( GTK_TREE_VIEW( widget ), tmf_model );
	gtk_tree_view_set_enable_tree_lines( GTK_TREE_VIEW( widget ), TRUE );

	g_object_unref( tmf_model );
	g_object_unref( ts_model );

	/* create visible columns on the tree view */
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
			"icon", gtk_cell_renderer_pixbuf_new(), "pixbuf", IACTIONS_LIST_ICON_COLUMN, NULL );
	gtk_tree_view_append_column( GTK_TREE_VIEW( widget ), column );

	/*column = gtk_tree_view_column_new_with_attributes(
			"label", gtk_cell_renderer_text_new(), "text", IACTIONS_LIST_LABEL_COLUMN, NULL );*/
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title( column, "label" );
	gtk_tree_view_column_set_sort_column_id( column, IACTIONS_LIST_LABEL_COLUMN );
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start( column, renderer, TRUE );
	gtk_tree_view_column_set_cell_data_func(
			column, renderer, ( GtkTreeCellDataFunc ) display_label, window, NULL );
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

	nact_iactions_list_fill( window, TRUE );

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
nact_iactions_list_fill( NactWindow *window, gboolean keep_expanded )
{
	static const gchar *thisfn = "nact_iactions_list_fill";
	g_debug( "%s: window=%p", thisfn, window );

	g_assert( NACT_IS_IACTIONS_LIST( window ));

	nact_iactions_list_set_is_filling_list( window, TRUE );

	GSList *expanded = NULL;
	if( keep_expanded ){
		expanded = get_expanded_rows( window );
	}

	GtkWidget *widget = get_actions_list_widget( window );
	GtkTreeModelFilter *tmf_model = GTK_TREE_MODEL_FILTER( gtk_tree_view_get_model( GTK_TREE_VIEW( widget )));
	GtkTreeStore *ts_model = GTK_TREE_STORE( gtk_tree_model_filter_get_model( tmf_model ));
	gtk_tree_store_clear( ts_model );

	GSList *actions = v_get_actions( window );
	GSList *ia;
	g_debug( "%s: actions has %d elements", thisfn, g_slist_length( actions ));

	for( ia = actions ; ia != NULL ; ia = ia->next ){
		GtkTreeIter iter;
		NAAction *action = NA_ACTION( ia->data );
		gtk_tree_store_append( ts_model, &iter, NULL );
		gtk_tree_store_set( ts_model, &iter, IACTIONS_LIST_NAOBJECT_COLUMN, action, -1);
		setup_action( widget, ts_model, &iter, action );
		g_debug( "%s: action=%p", thisfn, action );

		GSList *profiles = na_action_get_profiles( action );
		GSList *ip;
		GtkTreeIter profile_iter;
		for( ip = profiles ; ip ; ip = ip->next ){
			NAActionProfile *profile = NA_ACTION_PROFILE( ip->data );
			gtk_tree_store_append( ts_model, &profile_iter, &iter );
			gtk_tree_store_set( ts_model, &profile_iter, IACTIONS_LIST_NAOBJECT_COLUMN, profile, -1 );
			setup_profile( widget, ts_model, &profile_iter, profile );
		}
	}
	/*g_debug( "%s: at end, actions has %d elements", thisfn, g_slist_length( actions ));*/

	if( keep_expanded ){
		expand_rows( window, expanded );
		free_expanded_list( expanded );
	}

	nact_iactions_list_set_is_filling_list( window, FALSE );
}

/**
 * Set the selection to the specified object.
 *
 * @type: select a profile or an action
 * @uuid: uuid of the relevant action
 * @label: label of the action, or of the profile
 *
 * if we want select an action:
 * - set type = NA_ACTION_TYPE
 * - set uuid = uuid of the action
 * - set label = label of the action
 *
 * we are going to select:
 * - an action
 * - whose uuid is the requested uuid
 * - or whose label is the most close of the required label (if uuid is not found)
 *
 * if we want select a profile
 * - set type = NA_ACTION_PROFILE_TYPE
 * - set uuid = uuid of the parent action
 * - set label = label of the profile
 *
 * we are going to select:
 * a) if the action is found
 *    a.1) if the action has more than one profile
 *         - a profile of the action
 *         - whose label is the most close of the required one
 *    a.2) if the action has only one profile
 *         - the action
 * b) if the action is not found
 *    the last action of the list (this case should not appear)
 */
void
nact_iactions_list_set_selection( NactWindow *window, GType type, const gchar *uuid, const gchar *label )
{
	static const gchar *thisfn = "nact_iactions_list_set_selection";
	g_debug( "%s: window=%p, type=%s, uuid=%s, label=%s", thisfn, window, type == NA_ACTION_TYPE ? "action":"profile", uuid, label );

	GtkWidget *list = get_actions_list_widget( window );
	GtkTreeSelection *selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( list ));
	/*GtkTreeModelFilter *tmf_model = GTK_TREE_MODEL_FILTER( gtk_tree_view_get_model( GTK_TREE_VIEW( list )));
	GtkTreeStore *ts_model = GTK_TREE_STORE( gtk_tree_model_filter_get_model( tmf_model ));*/
	GtkTreeModel *model = gtk_tree_view_get_model( GTK_TREE_VIEW( list ));

	if( uuid && strlen( uuid )){
		gboolean found = FALSE;
		GtkTreeIter iter, previous;
		/*gboolean iterok = gtk_tree_model_get_iter_first( GTK_TREE_MODEL( ts_model ), &iter );*/
		gboolean iterok = gtk_tree_model_get_iter_first( model, &iter );
		NAObject *iter_object;
		gchar *iter_uuid, *iter_label;

		while( iterok && !found ){
			/*gtk_tree_model_get( GTK_TREE_MODEL( ts_model ), &iter, IACTIONS_LIST_NAOBJECT_COLUMN, &iter_object, -1 );*/
			gtk_tree_model_get( model, &iter, IACTIONS_LIST_NAOBJECT_COLUMN, &iter_object, IACTIONS_LIST_LABEL_COLUMN, &iter_label, -1 );
			g_debug( "%s: iter_object=%p", thisfn, iter_object );
			g_assert( NA_IS_ACTION( iter_object ));

			iter_uuid = na_action_get_uuid( NA_ACTION( iter_object ));
			gint ret_uuid = g_ascii_strcasecmp( iter_uuid, uuid );
			guint nb_profiles = na_action_get_profiles_count( NA_ACTION( iter_object ));

			if( type == NA_ACTION_TYPE || ( ret_uuid == 0 && nb_profiles == 1 )){
				gint ret_label = g_utf8_collate( iter_label, label );

				if( ret_uuid == 0 || ret_label > 0 ){
					gtk_tree_selection_select_iter( selection, &iter );
					found = TRUE;
					break;
				}

			} else if( ret_uuid == 0 && gtk_tree_model_iter_has_child( model, &iter )){
				GtkTreeIter iter_child, previous_child;
				gboolean iter_child_ok = gtk_tree_model_iter_children( model, &iter_child, &iter );

				while( iter_child_ok ){
					gtk_tree_model_get( model, &iter_child, IACTIONS_LIST_NAOBJECT_COLUMN, &iter_object, IACTIONS_LIST_LABEL_COLUMN, &iter_label, -1 );
					gint ret_label = g_utf8_collate( iter_label, label );

					if( ret_label >= 0 ){
						gtk_tree_selection_select_iter( selection, &iter_child );
						found = TRUE;
						break;
					}
					previous_child = iter_child;
					iter_child_ok = gtk_tree_model_iter_next( model, &iter_child );
				}
				if( !found ){
					gtk_tree_selection_select_iter( selection, &previous_child );
					found = TRUE;
				}
			}

			previous = iter;
			/*iterok = gtk_tree_model_iter_next( GTK_TREE_MODEL( ts_model ), &iter );*/
			iterok = gtk_tree_model_iter_next( model, &iter );
		}
		if( !found ){
			gtk_tree_selection_select_iter( selection, &previous );
		}

	} else {
		gtk_tree_selection_unselect_all( selection );
	}
	/*
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
	 */
}

/**
 * Reset the focus on the ActionsList listbox.
 */
/*void
nact_iactions_list_set_focus( NactWindow *window )
{
	GtkWidget *list = get_actions_list_widget( window );
	gtk_widget_grab_focus( list );
}*/

/**
 * Returns the currently selected action or profile.
 */
NAObject *
nact_iactions_list_get_selected_object( NactWindow *window )
{
	NAObject *object = NULL;

	GtkWidget *treeview = get_actions_list_widget( window );
	GtkTreeSelection *selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( treeview ));

	GtkTreeModel *tm_model;
	GtkTreeIter iter;
	if( gtk_tree_selection_get_selected( selection, &tm_model, &iter )){

		gtk_tree_model_get( tm_model, &iter, IACTIONS_LIST_NAOBJECT_COLUMN, &object, -1 );

		g_assert( object );
		g_assert( NA_IS_OBJECT( object ));
	}

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

/*void
nact_iactions_list_set_modified( NactWindow *window, gboolean is_modified, gboolean can_save )
{
}*/

gboolean
nact_iactions_list_is_expanded( NactWindow *window, const NAAction *action )
{
	GtkWidget *treeview = get_actions_list_widget( window );
	GtkTreeModel *model = gtk_tree_view_get_model( GTK_TREE_VIEW( treeview ));

	gboolean is_expanded = FALSE;
	GtkTreeIter iter;
	gboolean iterok = gtk_tree_model_get_iter_first( model, &iter );
	NAObject *iter_object;

	while( iterok ){
		gtk_tree_model_get( model, &iter, IACTIONS_LIST_NAOBJECT_COLUMN, &iter_object, -1 );

		if( iter_object == NA_OBJECT( action )){
			GtkTreePath *path = gtk_tree_model_get_path( model, &iter );
			is_expanded = gtk_tree_view_row_expanded( GTK_TREE_VIEW( treeview ), path );
			gtk_tree_path_free( path );
			break;
		}

		iterok = gtk_tree_model_iter_next( model, &iter );
	}

	return( is_expanded );
}

/*
 * Collapse / expand if actions has more than one profile
 */
void
nact_iactions_list_toggle_collapse( NactWindow *window, const NAAction *action )
{
	static const gchar *thisfn = "nact_iactions_list_toggle_collapse";

	GtkWidget *treeview = get_actions_list_widget( window );
	GtkTreeModel *model = gtk_tree_view_get_model( GTK_TREE_VIEW( treeview ));

	GtkTreeIter iter;
	gboolean iterok = gtk_tree_model_get_iter_first( model, &iter );

	while( iterok ){

		NAObject *iter_object;
		gtk_tree_model_get( model, &iter, IACTIONS_LIST_NAOBJECT_COLUMN, &iter_object, -1 );
		if( iter_object == NA_OBJECT( action )){

			if( na_action_get_profiles_count( action ) > 1 ){

				GtkTreePath *path = gtk_tree_model_get_path( model, &iter );

				if( gtk_tree_view_row_expanded( GTK_TREE_VIEW( treeview ), path )){
					gtk_tree_view_collapse_row( GTK_TREE_VIEW( treeview ), path );
					g_debug( "%s: action=%p collapsed", thisfn, action );

				} else {
					gtk_tree_view_expand_row( GTK_TREE_VIEW( treeview ), path, TRUE );
					g_debug( "%s: action=%p expanded", thisfn, action );
				}

				gtk_tree_path_free( path );
			}
			break;
		}
		iterok = gtk_tree_model_iter_next( model, &iter );
	}
}

/**
 * Update the listbox when a field has been modified in one of the tabs.
 *
 * @action: the NAAction whose one field has been modified ; the field
 * can be a profile field or an action field, whatever be the currently
 * selected row.
 *
 * e.g. while the currently selected row is a profile, user may have
 * modified the action label or the action icon : we wish report this
 * modification on the listbox
 */
void
nact_iactions_list_update_selected( NactWindow *window, NAAction *action )
{
	GtkWidget *treeview = get_actions_list_widget( window );
	GtkTreeSelection *selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( treeview ));

	GtkTreeModel *tm_model;
	GtkTreeIter iter;
	if( gtk_tree_selection_get_selected( selection, &tm_model, &iter )){

		NAObject *object;
		gtk_tree_model_get( tm_model, &iter, IACTIONS_LIST_NAOBJECT_COLUMN, &object, -1 );
		g_assert( object );

		GtkTreePath *path = gtk_tree_model_get_path( tm_model, &iter );

		GtkTreeModelFilter *tmf_model = GTK_TREE_MODEL_FILTER( gtk_tree_view_get_model( GTK_TREE_VIEW( treeview )));
		GtkTreeStore *ts_model = GTK_TREE_STORE( gtk_tree_model_filter_get_model( tmf_model ));
		GtkTreeIter ts_iter;
		gtk_tree_model_get_iter( GTK_TREE_MODEL( ts_model ), &ts_iter, path );

		if( NA_IS_ACTION( object )){
			g_assert( object == NA_OBJECT( action ));
			/*g_debug( "nact_iactions_list_update_selected: selected action=%p", object );*/
			setup_action( treeview, ts_model, &ts_iter, action );

		} else {
			g_assert( NA_IS_ACTION_PROFILE( object ));
			/*g_debug( "nact_iactions_list_update_selected: selected profile=%p", object );*/
			GSList *profiles = na_action_get_profiles( action );
			GSList *ip;
			for( ip = profiles ; ip ; ip = ip->next ){
				if( object == NA_OBJECT( ip->data )){
					setup_profile( treeview, ts_model, &ts_iter, NA_ACTION_PROFILE( ip->data ));
					break;
				}
			}

			if( gtk_tree_path_up( path )){
				gtk_tree_model_get_iter( GTK_TREE_MODEL( ts_model ), &ts_iter, path );
				setup_action( treeview, ts_model, &ts_iter, action );
			}
		}
	}
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

/*static void
v_set_sorted_actions( NactWindow *window, GSList *actions )
{
	g_assert( NACT_IS_IACTIONS_LIST( window ));
	NactIActionsList *instance = NACT_IACTIONS_LIST( window );

	if( NACT_IACTIONS_LIST_GET_INTERFACE( instance )->set_sorted_actions ){
		NACT_IACTIONS_LIST_GET_INTERFACE( instance )->set_sorted_actions( window, actions );
	}
}*/

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
		if( event->keyval == GDK_Delete || event->keyval == GDK_KP_Delete ){
			if( NACT_IACTIONS_LIST_GET_INTERFACE( instance )->on_delete_key_pressed ){
				stop = NACT_IACTIONS_LIST_GET_INTERFACE( instance )->on_delete_key_pressed( widget, event, user_data );
			}
		}
	}
	return( stop );
}

static gboolean
v_is_modified_action( NactWindow *window, const NAAction *action )
{
	if( NACT_IACTIONS_LIST_GET_INTERFACE( window )->is_modified_action ){
		return( NACT_IACTIONS_LIST_GET_INTERFACE( window )->is_modified_action( window, action ));
	}

	return( FALSE );
}

static gboolean
v_is_valid_action( NactWindow *window, const NAAction *action )
{
	if( NACT_IACTIONS_LIST_GET_INTERFACE( window )->is_valid_action ){
		return( NACT_IACTIONS_LIST_GET_INTERFACE( window )->is_valid_action( window, action ));
	}

	return( FALSE );
}

static gboolean
v_is_modified_profile( NactWindow *window, const NAActionProfile *profile )
{
	if( NACT_IACTIONS_LIST_GET_INTERFACE( window )->is_modified_profile ){
		return( NACT_IACTIONS_LIST_GET_INTERFACE( window )->is_modified_profile( window, profile ));
	}

	return( FALSE );
}

static gboolean
v_is_valid_profile( NactWindow *window, const NAActionProfile *profile )
{
	if( NACT_IACTIONS_LIST_GET_INTERFACE( window )->is_valid_profile ){
		return( NACT_IACTIONS_LIST_GET_INTERFACE( window )->is_valid_profile( window, profile ));
	}

	return( FALSE );
}

/*
 * action modified: italic
 * action not saveable: red
 */
static void
display_label( GtkTreeViewColumn *column, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, NactWindow *window )
{
	NAObject *object;
	gtk_tree_model_get( model, iter, IACTIONS_LIST_NAOBJECT_COLUMN, &object, -1 );

	if( object ){
		g_assert( NA_IS_OBJECT( object ));
		gchar *label = na_object_get_label( object );
		gboolean modified = FALSE;
		gboolean valid = TRUE;

		if( NA_IS_ACTION( object )){
			modified = v_is_modified_action( window, NA_ACTION( object ));
			valid = v_is_valid_action( window, NA_ACTION( object ));

		} else {
			g_assert( NA_IS_ACTION_PROFILE( object ));
			modified = v_is_modified_profile( window, NA_ACTION_PROFILE( object ));
			valid = v_is_valid_profile( window, NA_ACTION_PROFILE( object ));
		}

		if( modified ){
			g_object_set( cell, "style", PANGO_STYLE_ITALIC, "style-set", TRUE, NULL );
		} else {
			g_object_set( cell, "style-set", FALSE, NULL );
		}
		if( valid ){
			g_object_set( cell, "foreground-set", FALSE, NULL );
		} else {
			g_object_set( cell, "foreground", "Red", "foreground-set", TRUE, NULL );
		}

		g_object_set( cell, "text", label, NULL );

		g_free( label );
	}
}

static void
setup_action( GtkWidget *treeview, GtkTreeStore *model, GtkTreeIter *iter, NAAction *action )
{
	static const gchar *thisfn = "nact_iactions_list_setup_action";
	GtkStockItem item;
	GdkPixbuf* icon = NULL;

	gchar *label = na_action_get_label( action );
	gchar *iconname = na_action_get_icon( action );

	/* TODO: use the same algorythm than Nautilus to find and
	 * display an icon + move the code to NAAction class +
	 * remove na_action_get_verified_icon_name
	 */
	if( iconname ){
		if( gtk_stock_lookup( iconname, &item )){
			icon = gtk_widget_render_icon( treeview, iconname, GTK_ICON_SIZE_MENU, NULL );

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

	gtk_tree_store_set( model, iter, IACTIONS_LIST_ICON_COLUMN, icon, IACTIONS_LIST_LABEL_COLUMN, label, -1 );

	g_free( iconname );
	g_free( label );
}

static void
setup_profile( GtkWidget *treeview, GtkTreeStore *model, GtkTreeIter *iter, NAActionProfile *profile )
{
	gchar *label = na_action_profile_get_label( profile );
	gtk_tree_store_set( model, iter, IACTIONS_LIST_LABEL_COLUMN, label, -1);
	g_free( label );
}

static gint
sort_actions_list( GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, NactWindow *window )
{
	g_debug( "sort_actions_list" );
	gchar *labela, *labelb;

	gtk_tree_model_get( model, a, IACTIONS_LIST_LABEL_COLUMN, &labela, -1 );
	gtk_tree_model_get( model, b, IACTIONS_LIST_LABEL_COLUMN, &labelb, -1 );

	gint ret = g_utf8_collate( labela, labelb );

	g_free( labela );
	g_free( labelb );

	return( ret );
}

static gboolean
filter_visible( GtkTreeModel *model, GtkTreeIter *iter, gpointer data )
{
	NAObject *object;
	gtk_tree_model_get( model, iter, IACTIONS_LIST_NAOBJECT_COLUMN, &object, -1 );
	/*g_debug( "nact_main_window_filer_visible: model=%p, iter=%p, data=%p, object=%p", model, iter, data, object );*/

	if( object ){
		if( NA_IS_ACTION( object )){
			return( TRUE );
		}

		g_assert( NA_IS_ACTION_PROFILE( object ));
		NAAction *action = na_action_profile_get_action( NA_ACTION_PROFILE( object ));
		return( na_action_get_profiles_count( action ) > 1 );
	}

	return( FALSE );
}

static GtkWidget *
get_actions_list_widget( NactWindow *window )
{
	return( base_window_get_widget( BASE_WINDOW( window ), "ActionsList" ));
}

static GSList *
get_expanded_rows( NactWindow *window )
{
	GSList *expanded = NULL;
	GtkWidget *treeview = get_actions_list_widget( window );
	GtkTreeModel *model = gtk_tree_view_get_model( GTK_TREE_VIEW( treeview ));

	GtkTreeIter iter;
	gboolean iterok = gtk_tree_model_get_iter_first( model, &iter );
	NAObject *object;

	while( iterok ){
		gtk_tree_model_get( model, &iter, IACTIONS_LIST_NAOBJECT_COLUMN, &object, -1 );

		GtkTreePath *path = gtk_tree_model_get_path( model, &iter );

		if( gtk_tree_view_row_expanded( GTK_TREE_VIEW( treeview ), path )){
			expanded = g_slist_prepend( expanded, object );
		}

		gtk_tree_path_free( path );

		iterok = gtk_tree_model_iter_next( model, &iter );
	}

	return( expanded );
}

static void
expand_rows( NactWindow *window, GSList *expanded )
{
	GtkWidget *treeview = get_actions_list_widget( window );
	GtkTreeModel *model = gtk_tree_view_get_model( GTK_TREE_VIEW( treeview ));

	GtkTreeIter iter;
	gboolean iterok = gtk_tree_model_get_iter_first( model, &iter );
	NAObject *object;

	while( iterok ){
		gtk_tree_model_get( model, &iter, IACTIONS_LIST_NAOBJECT_COLUMN, &object, -1 );

		GSList *is;
		for( is = expanded ; is ; is = is->next ){
			if( object == NA_OBJECT( is->data )){

				GtkTreePath *path = gtk_tree_model_get_path( model, &iter );
				gtk_tree_view_expand_row( GTK_TREE_VIEW( treeview ), path, TRUE );
				gtk_tree_path_free( path );

				break;
			}
		}

		iterok = gtk_tree_model_iter_next( model, &iter );
	}
}

static void
free_expanded_list( GSList *expanded )
{
	g_slist_free( expanded );
}
