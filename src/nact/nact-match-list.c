/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010 Pierre Wieser and others (see AUTHORS)
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
#include <glib/gi18n.h>

#include <api/na-object-api.h>
#include <api/na-core-utils.h>

#include "nact-gtk-utils.h"
#include "nact-main-tab.h"
#include "nact-match-list.h"

/* column ordering
 */
enum {
	ITEM_COLUMN = 0,
	MUST_MATCH_COLUMN,
	MUST_NOT_MATCH_COLUMN,
	N_COLUMN
};

static gboolean st_on_selection_change = FALSE;

static void     on_add_filter_clicked( GtkButton *button, MatchListStr *data );
static void     on_filter_clicked( GtkTreeViewColumn *treeviewcolumn, MatchListStr *data );
static void     on_filter_edited( GtkCellRendererText *renderer, const gchar *path, const gchar *text, MatchListStr *data );
static gboolean on_key_pressed_event( GtkWidget *widget, GdkEventKey *event, MatchListStr *data );
static void     on_must_match_clicked( GtkTreeViewColumn *treeviewcolumn, MatchListStr *data );
static void     on_must_match_toggled( GtkCellRendererToggle *cell_renderer, gchar *path, MatchListStr *data );
static void     on_must_not_match_clicked( GtkTreeViewColumn *treeviewcolumn, MatchListStr *data );
static void     on_must_not_match_toggled( GtkCellRendererToggle *cell_renderer, gchar *path, MatchListStr *data );
static void     on_remove_filter_clicked( GtkButton *button, MatchListStr *data );
static void     on_selection_changed( GtkTreeSelection *selection, MatchListStr *data );

static void     add_filter( MatchListStr *data, const gchar *filter, const gchar *prefix );
static void     delete_current_row( MatchListStr *data );
static void     edit_inline( MatchListStr *data );
static void     insert_new_row( MatchListStr *data );
static void     iter_for_setup( gchar *filter, GtkTreeModel *model );
static void     sort_on_column( GtkTreeViewColumn *treeviewcolumn, MatchListStr *data, guint colid );
static gboolean tab_set_sensitive( MatchListStr *data );

/**
 * nact_match_list_create_model:
 * @window: the #BaseWindow window which contains the view.
 * @tab_name: a string constant which identifies this page.
 * @tab_id: our id for this page.
 * @listview: the #GtkTreeView widget.
 * @addbutton: the #GtkButton widget.
 * @removebutton: the #GtkButton widget.
 * @pget: a pointer to the function to get the list of filters.
 * @pset: a pointer to the function to set the list of filters.
 * @pon_add: an optional pointer to a function which handles the Add button.
 * @item_header: the title of the item header.
 *
 * Creates the tree model.
 */
void
nact_match_list_create_model( BaseWindow *window,
		const gchar *tab_name, guint tab_id,
		GtkWidget *listview, GtkWidget *addbutton, GtkWidget *removebutton,
		pget_filters pget, pset_filters pset, pon_add_callback pon_add,
		const gchar *item_header )
{
	MatchListStr *data;
	GtkListStore *model;
	GtkCellRenderer *text_cell, *radio_cell;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;

	data = g_new0( MatchListStr, 1 );
	data->window = window;
	data->tab_id = tab_id;
	data->listview = GTK_TREE_VIEW( listview );
	data->addbutton = addbutton;
	data->removebutton = removebutton;
	data->pget = pget;
	data->pset = pset;
	data->pon_add = pon_add;
	data->item_header = g_strdup( item_header );
	data->editable = FALSE;
	data->sort_column = 0;
	data->sort_order = 0;
	g_object_set_data( G_OBJECT( window ), tab_name, data );

	model = gtk_list_store_new( N_COLUMN, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN );
	gtk_tree_view_set_model( data->listview, GTK_TREE_MODEL( model ));
	g_object_unref( model );

	text_cell = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
			data->item_header,
			text_cell,
			"text", ITEM_COLUMN,
			NULL );
	gtk_tree_view_append_column( data->listview, column );

	radio_cell = gtk_cell_renderer_toggle_new();
	gtk_cell_renderer_toggle_set_radio( GTK_CELL_RENDERER_TOGGLE( radio_cell ), TRUE );
	column = gtk_tree_view_column_new_with_attributes(
			/* i18n: label of the header of a column which let the user select a positive filter
			 */
			_( "Must match one of" ),
			radio_cell,
			"active", MUST_MATCH_COLUMN,
			NULL );
	gtk_tree_view_append_column( data->listview, column );

	radio_cell = gtk_cell_renderer_toggle_new();
	gtk_cell_renderer_toggle_set_radio( GTK_CELL_RENDERER_TOGGLE( radio_cell ), TRUE );
	column = gtk_tree_view_column_new_with_attributes(
			/* i18n: label of the header of a column which let the user select a negative filter
			 */
			_( "Must not match any of" ),
			radio_cell,
			"active", MUST_NOT_MATCH_COLUMN,
			NULL );
	gtk_tree_view_append_column( data->listview, column );

	/* an empty column to fill out the view
	 */
	column = gtk_tree_view_column_new();
	gtk_tree_view_append_column( data->listview, column );

	gtk_tree_view_set_headers_visible( data->listview, TRUE );
	gtk_tree_view_set_headers_clickable( data->listview, TRUE );

	selection = gtk_tree_view_get_selection( data->listview );
	gtk_tree_selection_set_mode( selection, GTK_SELECTION_BROWSE );
}

/**
 * nact_match_list_init_view:
 * @window: the #BaseWindow window which contains the view.
 * @tab_name: a string constant which identifies this page.
 *
 * Initializes the tab widget at each time the widget will be displayed.
 * Connect signals.
 */
void
nact_match_list_init_view( BaseWindow *window, const gchar *tab_name )
{
	MatchListStr *data;
	GtkTreeViewColumn *column;
	GList *renderers;
	GtkTreeModel *model;

	data = ( MatchListStr * ) g_object_get_data( G_OBJECT( window ), tab_name );
	g_return_if_fail( data != NULL );

	column = gtk_tree_view_get_column( data->listview, ITEM_COLUMN );
	base_window_signal_connect_with_data(
			window,
			G_OBJECT( column ),
			"clicked",
			G_CALLBACK( on_filter_clicked ),
			data );

	renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( column ));
	base_window_signal_connect_with_data(
			window,
			G_OBJECT( renderers->data ),
			"edited",
			G_CALLBACK( on_filter_edited ),
			data );

	column = gtk_tree_view_get_column( data->listview, MUST_MATCH_COLUMN );
	base_window_signal_connect_with_data(
			window,
			G_OBJECT( column ),
			"clicked",
			G_CALLBACK( on_must_match_clicked ),
			data );

	renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( column ));
	base_window_signal_connect_with_data(
			window,
			G_OBJECT( renderers->data ),
			"toggled",
			G_CALLBACK( on_must_match_toggled ),
			data );

	column = gtk_tree_view_get_column( data->listview, MUST_NOT_MATCH_COLUMN );
	base_window_signal_connect_with_data(
			window,
			G_OBJECT( column ),
			"clicked",
			G_CALLBACK( on_must_not_match_clicked ),
			data );

	renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( column ));
	base_window_signal_connect_with_data(
			window,
			G_OBJECT( renderers->data ),
			"toggled",
			G_CALLBACK( on_must_not_match_toggled ),
			data );

	base_window_signal_connect_with_data(
			window,
			G_OBJECT( data->addbutton ),
			"clicked",
			data->pon_add ? G_CALLBACK( data->pon_add ) : G_CALLBACK( on_add_filter_clicked ),
			data );

	base_window_signal_connect_with_data(
			window,
			G_OBJECT( data->removebutton ),
			"clicked",
			G_CALLBACK( on_remove_filter_clicked ),
			data );

	base_window_signal_connect_with_data(
			window,
			G_OBJECT( gtk_tree_view_get_selection( data->listview )),
			"changed",
			G_CALLBACK( on_selection_changed ),
			data );

	base_window_signal_connect_with_data(
			window,
			G_OBJECT( data->listview ),
			"key-press-event",
			G_CALLBACK( on_key_pressed_event ),
			data );

	model = gtk_tree_view_get_model( data->listview );
	gtk_tree_sortable_set_sort_column_id( GTK_TREE_SORTABLE( model ), ITEM_COLUMN, GTK_SORT_ASCENDING );
	data->sort_column = ITEM_COLUMN;
	data->sort_order = GTK_SORT_ASCENDING;
}

/**
 * nact_match_list_on_selection_changed:
 * @window: the #BaseWindow window which contains the view.
 * @tab_name: a string constant which identifies this page.
 * @count_selected: count of selected items in the #NactIActionsList list view.
 *
 * Called at instance_dispose time.
 *
 * Basically we are using here a rather common scheme:
 * - object has a GSList of strings, each of one being a filter description,
 *   which may be negated
 * - the list is displayed in a listview with radio toggle buttons
 *   so that a user cannot have both positive and negative assertions
 *   for the same basename filter
 * - update the object with a summary of the listbox contents
 */
void
nact_match_list_on_selection_changed( BaseWindow *window, const gchar *tab_name, guint count_selected )
{
	static const gchar *thisfn = "nact_match_list_on_selection_changed";
	MatchListStr *data;
	NAObjectItem *item;
	NAObjectProfile *profile;
	gboolean editable;
	NAIContext *context;
	GSList *filters;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
	GtkTreePath *path;

	g_debug( "%s: window=%p, tab=%s, count_selected=%d", thisfn, ( void * ) window, tab_name, count_selected );

	data = ( MatchListStr * ) g_object_get_data( G_OBJECT( window ), tab_name );
	g_return_if_fail( data != NULL );

	g_object_get(
			G_OBJECT( window ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, &item,
			TAB_UPDATABLE_PROP_EDITED_PROFILE, &profile,
			TAB_UPDATABLE_PROP_EDITABLE, &editable,
			NULL );

	context = ( profile ? NA_ICONTEXT( profile ) : ( NAIContext * ) item );
	data->editable = editable;
	filters = ( *data->pget )( context );

	st_on_selection_change = TRUE;

	model = gtk_tree_view_get_model( data->listview );
	selection = gtk_tree_view_get_selection( data->listview );
	gtk_tree_selection_unselect_all( selection );
	gtk_list_store_clear( GTK_LIST_STORE( model ));

	if( filters ){
		g_slist_foreach( filters, ( GFunc ) iter_for_setup, model );
	}

	column = gtk_tree_view_get_column( data->listview, ITEM_COLUMN );
	nact_gtk_utils_set_editable( GTK_OBJECT( column ), data->editable );

	nact_gtk_utils_set_editable( GTK_OBJECT( data->addbutton ), data->editable );
	nact_gtk_utils_set_editable( GTK_OBJECT( data->removebutton ), data->editable );

	st_on_selection_change = FALSE;

	path = gtk_tree_path_new_first();
	if( path ){
		selection = gtk_tree_view_get_selection( data->listview );
		gtk_tree_selection_select_path( selection, path );
		gtk_tree_path_free( path );
	}
}

/**
 * nact_match_list_on_enable_tab:
 * @window: the #BaseWindow window which contains the view.
 * @tab_name: a string constant which identifies this page.
 * @item: the currently selected #NAObjectItem.
 *
 * Enable/disable this page of the notebook.
 */
void
nact_match_list_on_enable_tab( BaseWindow *window, const gchar *tab_name, NAObjectItem *item )
{
	static const gchar *thisfn = "nact_match_list_on_tab_updatable_enable_tab";
	MatchListStr *data;

	g_debug( "%s: window=%p, tab=%s, item=%p", thisfn, ( void * ) window, tab_name, ( void * ) item );

	data = ( MatchListStr * ) g_object_get_data( G_OBJECT( window ), tab_name );
	g_return_if_fail( data != NULL );

	tab_set_sensitive( data );
}

/**
 * nact_match_list_insert_row:
 * @data: the #MatchListStr structure.
 * @filter: the item to add.
 * @match: whether the 'must match' column is checked.
 * @not_match: whether the 'must not match' column is checked.
 *
 * Add a new row to the list view.
 */
void
nact_match_list_insert_row( MatchListStr *data, const gchar *filter, gboolean match, gboolean not_match )
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;
	GtkTreeViewColumn *column;

	g_return_if_fail( !( match && not_match ));

	model = gtk_tree_view_get_model( data->listview );

	gtk_list_store_insert_with_values( GTK_LIST_STORE( model ), &iter, 0,
			/* i18n notes : new filter for a new row in a match/no matchlist */
			ITEM_COLUMN, filter,
			MUST_MATCH_COLUMN, match,
			MUST_NOT_MATCH_COLUMN, not_match,
			-1 );

	path = gtk_tree_model_get_path( model, &iter );
	column = gtk_tree_view_get_column( data->listview, ITEM_COLUMN );
	gtk_tree_view_set_cursor( data->listview, path, column, TRUE );
	gtk_tree_path_free( path );

	if( match ){
		add_filter( data, filter, "" );
	}

	if( not_match ){
		add_filter( data, filter, "!" );
	}
}

/**
 * nact_match_list_dispose:
 * @window: the #BaseWindow window which contains the view.
 * @tab_name: a string constant which identifies this page.
 *
 * Called at instance_dispose time.
 */
void
nact_match_list_dispose( BaseWindow *window, const gchar *tab_name )
{
	MatchListStr *data;
	GtkTreeModel *model;
	GtkTreeSelection *selection;

	data = ( MatchListStr * ) g_object_get_data( G_OBJECT( window ), tab_name );
	g_return_if_fail( data != NULL );

	model = gtk_tree_view_get_model( data->listview );
	selection = gtk_tree_view_get_selection( data->listview );
	gtk_tree_selection_unselect_all( selection );
	gtk_list_store_clear( GTK_LIST_STORE( model ));

	g_free( data->item_header );

	g_free( data );
	g_object_set_data( G_OBJECT( window ), tab_name, NULL );
}

static void
on_add_filter_clicked( GtkButton *button, MatchListStr *data )
{
	insert_new_row( data );
}

static void
on_filter_clicked( GtkTreeViewColumn *treeviewcolumn, MatchListStr *data )
{
	sort_on_column( treeviewcolumn, data, ITEM_COLUMN );
}

static void
on_filter_edited( GtkCellRendererText *renderer, const gchar *path_str, const gchar *text, MatchListStr *data )
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;
	gchar *old_text;
	NAObjectItem *item;
	NAObjectProfile *profile;
	NAIContext *context;
	gboolean must_match, must_not_match;
	gchar *to_add, *to_remove;
	GSList *filters;

	model = gtk_tree_view_get_model( data->listview );
	path = gtk_tree_path_new_from_string( path_str );
	gtk_tree_model_get_iter( model, &iter, path );
	gtk_tree_path_free( path );

	gtk_tree_model_get( model, &iter,
			ITEM_COLUMN, &old_text,
			MUST_MATCH_COLUMN, &must_match,
			MUST_NOT_MATCH_COLUMN, &must_not_match,
			-1 );

	gtk_list_store_set( GTK_LIST_STORE( model ), &iter, ITEM_COLUMN, text, -1 );

	g_object_get(
			G_OBJECT( data->window ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, &item,
			TAB_UPDATABLE_PROP_EDITED_PROFILE, &profile,
			NULL );

	context = ( profile ? NA_ICONTEXT( profile ) : ( NAIContext * ) item );

	if( context ){
		filters = ( *data->pget )( context );

		if( filters ){
			to_remove = g_strdup( old_text );
			filters = na_core_utils_slist_remove_ascii( filters, to_remove );
			g_free( to_remove );
			to_remove = g_strdup_printf( "!%s", old_text );
			filters = na_core_utils_slist_remove_ascii( filters, to_remove );
			g_free( to_remove );
		}

		if( must_match ){
			filters = g_slist_prepend( filters, g_strdup( text ));

		} else if( must_not_match ){
			to_add = g_strdup_printf( "!%s", text );
			filters = g_slist_prepend( filters, to_add );
		}

		( *data->pset )( context, filters );
		na_core_utils_slist_free( filters );

		g_signal_emit_by_name( G_OBJECT( data->window ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, context, FALSE );
	}

	g_free( old_text );
}

static gboolean
on_key_pressed_event( GtkWidget *widget, GdkEventKey *event, MatchListStr *data )
{
	gboolean stop;

	stop = FALSE;

	if( event->keyval == GDK_F2 ){
		edit_inline( data );
		stop = TRUE;
	}

	if( event->keyval == GDK_Insert || event->keyval == GDK_KP_Insert ){
		insert_new_row( data );
		stop = TRUE;
	}

	if( event->keyval == GDK_Delete || event->keyval == GDK_KP_Delete ){
		delete_current_row( data );
		stop = TRUE;
	}

	return( stop );
}

static void
on_must_match_clicked( GtkTreeViewColumn *treeviewcolumn, MatchListStr *data )
{
	sort_on_column( treeviewcolumn, data, MUST_MATCH_COLUMN );
}

/*
 * clicking on an already active toggle button has no effect
 * clicking on an inactive toggle button has a double effect:
 * - the other toggle button becomes inactive
 * - this toggle button becomes active
 * the corresponding strings must be respectively removed/added to the
 * filters list
 */
static void
on_must_match_toggled( GtkCellRendererToggle *cell_renderer, gchar *path_str, MatchListStr *data )
{
	/*static const gchar *thisfn = "nact_ibasenames_tab_on_must_match_toggled";*/
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	gchar *filter;
	NAObjectItem *item;
	NAObjectProfile *profile;
	NAIContext *context;
	GSList *filters;
	gchar *to_remove;

	/*gboolean is_active = gtk_cell_renderer_toggle_get_active( cell_renderer );
	g_debug( "%s: is_active=%s", thisfn, is_active ? "True":"False" );*/

	if( !gtk_cell_renderer_toggle_get_active( cell_renderer )){

		model = gtk_tree_view_get_model( data->listview );
		path = gtk_tree_path_new_from_string( path_str );
		gtk_tree_model_get_iter( model, &iter, path );
		gtk_tree_path_free( path );

		gtk_tree_model_get( model, &iter, ITEM_COLUMN, &filter, -1 );
		gtk_list_store_set( GTK_LIST_STORE( model ), &iter, MUST_MATCH_COLUMN, TRUE, MUST_NOT_MATCH_COLUMN, FALSE, -1 );

		g_object_get(
				G_OBJECT( data->window ),
				TAB_UPDATABLE_PROP_EDITED_ACTION, &item,
				TAB_UPDATABLE_PROP_EDITED_PROFILE, &profile,
				NULL );

		context = ( profile ? NA_ICONTEXT( profile ) : ( NAIContext * ) item );

		if( context ){
			filters = ( *data->pget )( context );
			if( filters ){
				to_remove = g_strdup_printf( "!%s", filter );
				filters = na_core_utils_slist_remove_ascii( filters, to_remove );
				g_free( to_remove );
			}
			filters = g_slist_prepend( filters, filter );
			( *data->pset )( context, filters );
			na_core_utils_slist_free( filters );

			g_signal_emit_by_name( G_OBJECT( data->window ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, context, FALSE );
		}
	}
}

static void
on_must_not_match_clicked( GtkTreeViewColumn *treeviewcolumn, MatchListStr *data )
{
	sort_on_column( treeviewcolumn, data, MUST_MATCH_COLUMN );
}

static void
on_must_not_match_toggled( GtkCellRendererToggle *cell_renderer, gchar *path_str, MatchListStr *data )
{
	/*static const gchar *thisfn = "nact_ibasenames_tab_on_must_not_match_toggled";*/
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	gchar *filter;
	NAObjectItem *item;
	NAObjectProfile *profile;
	NAIContext *context;
	GSList *filters;
	gchar *to_add;

	/*gboolean is_active = gtk_cell_renderer_toggle_get_active( cell_renderer );
	g_debug( "%s: is_active=%s", thisfn, is_active ? "True":"False" );*/

	if( !gtk_cell_renderer_toggle_get_active( cell_renderer )){

		model = gtk_tree_view_get_model( data->listview );
		path = gtk_tree_path_new_from_string( path_str );
		gtk_tree_model_get_iter( model, &iter, path );
		gtk_tree_path_free( path );

		gtk_tree_model_get( model, &iter, ITEM_COLUMN, &filter, -1 );
		gtk_list_store_set( GTK_LIST_STORE( model ), &iter, MUST_MATCH_COLUMN, FALSE, MUST_NOT_MATCH_COLUMN, TRUE, -1 );

		g_object_get(
				G_OBJECT( data->window ),
				TAB_UPDATABLE_PROP_EDITED_ACTION, &item,
				TAB_UPDATABLE_PROP_EDITED_PROFILE, &profile,
				NULL );

		context = ( profile ? NA_ICONTEXT( profile ) : ( NAIContext * ) item );

		if( context ){
			filters = ( *data->pget )( context );
			if( filters ){
				filters = na_core_utils_slist_remove_ascii( filters, filter );
			}
			to_add = g_strdup_printf( "!%s", filter );
			filters = g_slist_prepend( filters, to_add );
			( *data->pset )( context, filters );
			na_core_utils_slist_free( filters );

			g_signal_emit_by_name( G_OBJECT( data->window ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, context, FALSE );
		}

		g_free( filter );
	}
}

static void
on_remove_filter_clicked( GtkButton *button, MatchListStr *data )
{
	delete_current_row( data );
}

static void
on_selection_changed( GtkTreeSelection *selection, MatchListStr *data )
{
	gtk_widget_set_sensitive( data->removebutton,
			data->editable && gtk_tree_selection_count_selected_rows( selection ) > 0 );
}

static void
add_filter( MatchListStr *data, const gchar *filter, const gchar *prefix )
{
	NAObjectItem *item;
	NAObjectProfile *profile;
	NAIContext *context;
	GSList *filters;
	gchar *to_add;

	g_object_get(
			G_OBJECT( data->window ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, &item,
			TAB_UPDATABLE_PROP_EDITED_PROFILE, &profile,
			NULL );

	context = ( profile ? NA_ICONTEXT( profile ) : ( NAIContext * ) item );

	if( context ){
		filters = ( *data->pget )( context );
		to_add = g_strdup_printf( "%s%s", prefix, filter );
		filters = g_slist_prepend( filters, to_add );
		( *data->pset )( context, filters );
		na_core_utils_slist_free( filters );

		g_signal_emit_by_name( G_OBJECT( data->window ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, context, FALSE );
	}
}

static void
delete_current_row( MatchListStr *data )
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *rows;
	GtkTreePath *path;
	GtkTreeIter iter;
	gchar *filter;
	NAObjectItem *item;
	NAObjectProfile *profile;
	NAIContext *context;
	GSList *filters;
	gchar *to_remove;

	selection = gtk_tree_view_get_selection( data->listview );
	model = gtk_tree_view_get_model( data->listview );
	rows = gtk_tree_selection_get_selected_rows( selection, NULL );

	if( g_list_length( rows ) == 1 ){
		path = ( GtkTreePath * ) rows->data;
		gtk_tree_model_get_iter( model, &iter, path );
		gtk_tree_model_get( model, &iter, ITEM_COLUMN, &filter, -1 );
		gtk_list_store_remove( GTK_LIST_STORE( model ), &iter );

		g_object_get(
				G_OBJECT( data->window ),
				TAB_UPDATABLE_PROP_EDITED_ACTION, &item,
				TAB_UPDATABLE_PROP_EDITED_PROFILE, &profile,
				NULL );

		context = ( profile ? NA_ICONTEXT( profile ) : ( NAIContext * ) item );

		if( context ){
			filters = ( *data->pget )( context );

			if( filters ){
				to_remove = g_strdup_printf( "!%s", filter );
				filters = na_core_utils_slist_remove_ascii( filters, to_remove );
				g_free( to_remove );
				filters = na_core_utils_slist_remove_ascii( filters, filter );
				( *data->pset )( context, filters );
				na_core_utils_slist_free( filters );

				g_signal_emit_by_name( G_OBJECT( data->window ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, context, FALSE );
			}
		}

		g_free( filter );

		if( gtk_tree_model_get_iter( model, &iter, path ) ||
			gtk_tree_path_prev( path )){
			gtk_tree_view_set_cursor( data->listview, path, NULL, FALSE );
		}
	}

	g_list_foreach( rows, ( GFunc ) gtk_tree_path_free, NULL );
	g_list_free( rows );
}

static void
edit_inline( MatchListStr *data )
{
	GtkTreeSelection *selection;
	GList *rows;
	GtkTreePath *path;
	GtkTreeViewColumn *column;

	selection = gtk_tree_view_get_selection( data->listview );
	rows = gtk_tree_selection_get_selected_rows( selection, NULL );

	if( g_list_length( rows ) == 1 ){
		gtk_tree_view_get_cursor( data->listview, &path, &column );
		gtk_tree_view_set_cursor( data->listview, path, column, TRUE );
		gtk_tree_path_free( path );
	}

	g_list_foreach( rows, ( GFunc ) gtk_tree_path_free, NULL );
	g_list_free( rows );
}

static void
insert_new_row( MatchListStr *data )
{
	nact_match_list_insert_row( data, _( "new-filter" ), FALSE, FALSE );
}

static void
iter_for_setup( gchar *filter_orig, GtkTreeModel *model )
{
	GtkTreeIter iter;
	gchar *tmp, *filter;
	gboolean positive;
	gboolean negative;

	filter = g_strstrip( g_strdup( filter_orig ));
	positive = FALSE;
	negative = FALSE;

	if( filter[0] == '!' ){
		tmp = g_strstrip( g_strdup( filter+1 ));
		g_free( filter );
		filter = tmp;
		negative = TRUE;

	} else {
		positive = TRUE;
	}

	gtk_list_store_append( GTK_LIST_STORE( model ), &iter );
	gtk_list_store_set(
			GTK_LIST_STORE( model ),
			&iter,
			ITEM_COLUMN, filter,
			MUST_MATCH_COLUMN, positive,
			MUST_NOT_MATCH_COLUMN, negative,
			-1 );

	g_free( filter );
}

static void
sort_on_column( GtkTreeViewColumn *treeviewcolumn, MatchListStr *data, guint new_col_id )
{
	guint prev_col_id;
	guint prev_order, new_order;
	GtkTreeModel *model;

	prev_col_id = data->sort_column;
	prev_order = data->sort_order;

	if( new_col_id == prev_col_id ){
		new_order = ( prev_order == GTK_SORT_ASCENDING ? GTK_SORT_DESCENDING : GTK_SORT_ASCENDING );
	} else {
		new_order = GTK_SORT_ASCENDING;
	}

	data->sort_column = new_col_id;
	data->sort_order = new_order;

	model = gtk_tree_view_get_model( data->listview );
	gtk_tree_sortable_set_sort_column_id( GTK_TREE_SORTABLE( model ), new_col_id, new_order );
}

static gboolean
tab_set_sensitive( MatchListStr *data )
{
	NAObjectItem *item;
	NAObjectProfile *profile;
	gboolean enable_tab;

	g_object_get(
			G_OBJECT( data->window ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, &item,
			TAB_UPDATABLE_PROP_EDITED_PROFILE, &profile,
			NULL );

	enable_tab = ( profile != NULL || item != NULL );
	nact_main_tab_enable_page( NACT_MAIN_WINDOW( data->window ), data->tab_id, enable_tab );

	return( enable_tab );
}
