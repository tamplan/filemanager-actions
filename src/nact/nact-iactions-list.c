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

#include <common/na-object-api.h>
#include <common/na-obj-action.h>
#include <common/na-obj-menu.h>
#include <common/na-obj-profile.h>
#include <common/na-iprefs.h>

#include "nact-application.h"
#include "nact-main-tab.h"
#include "nact-tree-model.h"
#include "nact-iactions-list.h"

/* private interface data
 */
struct NactIActionsListInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* signals
 */
enum {
	SELECTION_CHANGED,
	ITEM_UPDATED,
	LAST_SIGNAL
};

/* iter on selection prototype
 */
typedef gboolean ( *FnIterOnSelection )( NactIActionsList *, GtkTreeView *, GtkTreeModel *, GtkTreeIter *, NAObject *, gpointer );

/* when toggle collapse/expand rows, we want all rows have the same
 * behavior, e.g. all rows collapse, or all rows expand
 * this behavior is fixed by the first rows which will actually be
 * toggled
 */
enum {
	TOGGLE_UNDEFINED,
	TOGGLE_COLLAPSE,
	TOGGLE_EXPAND
};

/* data set against GObject
 */
#define SHOW_ONLY_ACTIONS_MODE			"nact-iactions-list-show-only-actions-mode"
#define IS_FILLING_LIST					"nact-iactions-list-is-filling-list"
#define HAVE_DND_MODE					"nact-iactions-list-dnd-mode"

static gint         st_signals[ LAST_SIGNAL ] = { 0 };

static GType        register_type( void );
static void         interface_base_init( NactIActionsListInterface *klass );
static void         interface_base_finalize( NactIActionsListInterface *klass );

static void         display_label( GtkTreeViewColumn *column, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, NactIActionsList *instance );
static void         extend_selection_to_childs( NactIActionsList *instance, GtkTreeView *treeview, GtkTreeModel *model, GtkTreeIter *parent );
static void         free_items_flat_list_callback( NactIActionsList *instance, GSList *items );
static GtkTreeView *get_actions_list_treeview( NactIActionsList *instance );
static gboolean     get_item( NactTreeModel *model, GtkTreePath *path, NAObject *object, GSList **items );
static gboolean     have_dnd_mode( NactIActionsList *instance );
static void         insert_item( NactIActionsList *instance, NAObject *item );
static gboolean     is_modified_item( NactTreeModel *model, GtkTreePath *path, NAObject *object, GSList **items );
static void         iter_on_selection( NactIActionsList *instance, FnIterOnSelection fn_iter, gpointer user_data );
static gboolean     on_button_press_event( GtkWidget *widget, GdkEventButton *event, NactIActionsList *instance );
static gboolean     on_key_pressed_event( GtkWidget *widget, GdkEventKey *event, NactIActionsList *instance );
static void         on_treeview_selection_changed( GtkTreeSelection *selection, NactIActionsList *instance );
static void         on_iactions_list_item_updated( NactIActionsList *instance, GSList *updated_items );
static void         on_iactions_list_item_updated_treeview( NactIActionsList *instance, GSList *updated_items );
static void         on_iactions_list_selection_changed( NactIActionsList *instance, GSList *selected_items );
static void         select_first_row( NactIActionsList *instance );
static void         set_is_filling_list( NactIActionsList *instance, gboolean is_filling );
static void         toggle_collapse( NactIActionsList *instance );
static gboolean     toggle_collapse_iter( NactIActionsList *instance, GtkTreeView *treeview, GtkTreeModel *model, GtkTreeIter *iter, NAObject *object, gpointer user_data );
static void         toggle_collapse_row( GtkTreeView *treeview, GtkTreePath *path, guint *toggle );

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
	GType type;

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

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_INTERFACE, "NactIActionsList", &info, 0 );

	g_type_interface_add_prerequisite( type, BASE_WINDOW_TYPE );

	return( type );
}

static void
interface_base_init( NactIActionsListInterface *klass )
{
	static const gchar *thisfn = "nact_iactions_list_interface_base_init";
	static gboolean initialized = FALSE;

	if( !initialized ){
		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NactIActionsListInterfacePrivate, 1 );

		/**
		 * "nact-iactions-list-selection-changed":
		 *
		 * This signal is emitted each time the selection is changed in
		 * the treeview.
		 *
		 * User_data is a flat GSList of newly selected #NAObjects.
		 * It is owned by the #IActionsList interface, and will be
		 * released in the cleanup handler.
		 */
		st_signals[ SELECTION_CHANGED ] = g_signal_new_class_handler(
				IACTIONS_LIST_SIGNAL_SELECTION_CHANGED,
				G_TYPE_OBJECT,
				G_SIGNAL_RUN_CLEANUP,
				G_CALLBACK( free_items_flat_list_callback ),
				NULL,
				NULL,
				g_cclosure_marshal_VOID__POINTER,
				G_TYPE_NONE,
				1,
				G_TYPE_POINTER );

		/**
		 * "nact-iactions-list-item-updated":
		 *
		 * This signal is emitted to inform all widgets that an item
		 * has been updated.
		 * This typically results in a modification of the status of
		 * this item, and of all its hierarchy in the #IActionsList
		 * view.
		 *
		 * User_data is a flat GSList of modified #NAObjects. This
		 * list is owned by the emitter, and will be released in the
		 * cleanup handler.
		 */
		st_signals[ ITEM_UPDATED ] = g_signal_new_class_handler(
				IACTIONS_LIST_SIGNAL_ITEM_UPDATED,
				G_TYPE_OBJECT,
				G_SIGNAL_RUN_CLEANUP,
				G_CALLBACK( free_items_flat_list_callback ),
				NULL,
				NULL,
				g_cclosure_marshal_VOID__POINTER,
				G_TYPE_NONE,
				1,
				G_TYPE_POINTER );

		initialized = TRUE;
	}
}

static void
interface_base_finalize( NactIActionsListInterface *klass )
{
	static const gchar *thisfn = "nact_iactions_list_interface_base_finalize";
	static gboolean finalized = FALSE ;

	if( !finalized ){
		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );

		finalized = TRUE;
	}
}

/**
 * nact_iactions_list_initial_load_toplevel:
 * @window: this #NactIActionsList *instance.
 *
 * Allocates and initializes the ActionsList widget.
 *
 * GtkTreeView is created with NactTreeModel model
 * NactTreeModel
 *   implements EggTreeMultiDragSourceIface
 *   is derived from GtkTreeModelFilter
 *     GtkTreeModelFilter is built on top of GtkTreeStore
 */
void
nact_iactions_list_initial_load_toplevel( NactIActionsList *instance )
{
	static const gchar *thisfn = "nact_iactions_list_initial_load_toplevel";
	GtkWidget *label;
	GtkTreeView *treeview;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_IACTIONS_LIST( instance ));

	treeview = get_actions_list_treeview( instance );

	/* associates the ActionsList to the label */
	label = base_window_get_widget( BASE_WINDOW( instance ), "ActionsListLabel" );
	gtk_label_set_mnemonic_widget( GTK_LABEL( label ), GTK_WIDGET( treeview ));

	nact_iactions_list_set_dnd_mode( instance, FALSE );
	nact_iactions_list_set_multiple_selection_mode( instance, FALSE );
	nact_iactions_list_set_only_actions_mode( instance, FALSE );
	set_is_filling_list( instance, FALSE );

	nact_tree_model_initial_load( BASE_WINDOW( instance ), treeview );

	gtk_tree_view_set_enable_tree_lines( treeview, TRUE );

	/* create visible columns on the tree view
	 */
	column = gtk_tree_view_column_new_with_attributes(
			"icon", gtk_cell_renderer_pixbuf_new(), "pixbuf", IACTIONS_LIST_ICON_COLUMN, NULL );
	gtk_tree_view_append_column( treeview, column );

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title( column, "label" );
	gtk_tree_view_column_set_sort_column_id( column, IACTIONS_LIST_LABEL_COLUMN );
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start( column, renderer, TRUE );
	gtk_tree_view_column_set_cell_data_func(
			column, renderer, ( GtkTreeCellDataFunc ) display_label, instance, NULL );
	gtk_tree_view_append_column( treeview, column );
}

/**
 * nact_iactions_list_runtime_init_toplevel:
 * @window: this #NactIActionsList *instance.
 * @items: list of #NAObject actions and menus as provided by #NAPivot.
 *
 * Allocates and initializes the ActionsList widget.
 */
void
nact_iactions_list_runtime_init_toplevel( NactIActionsList *instance, GSList *items )
{
	static const gchar *thisfn = "nact_iactions_list_runtime_init_toplevel";
	GtkTreeView *treeview;
	NactTreeModel *model;
	gboolean have_dnd;

	g_debug( "%s: instance=%p, items=%p (%d items)",
			thisfn, ( void * ) instance, ( void * ) items, g_slist_length( items ));
	g_return_if_fail( NACT_IS_IACTIONS_LIST( instance ));

	treeview = get_actions_list_treeview( instance );
	model = NACT_TREE_MODEL( gtk_tree_view_get_model( treeview ));
	have_dnd = have_dnd_mode( instance );

	nact_tree_model_runtime_init( model, have_dnd );

	/* set up selection control */
	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( gtk_tree_view_get_selection( treeview )),
			"changed",
			G_CALLBACK( on_treeview_selection_changed ));

	/* catch press 'Enter' */
	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( treeview ),
			"key-press-event",
			G_CALLBACK( on_key_pressed_event ));

	/* catch double-click */
	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( treeview ),
			"button-press-event",
			G_CALLBACK( on_button_press_event ));

	/* install a virtual function as 'selection-changed' handler */
	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			IACTIONS_LIST_SIGNAL_SELECTION_CHANGED,
			G_CALLBACK( on_iactions_list_selection_changed ));

	/* updates the treeview display when an item is modified */
	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			IACTIONS_LIST_SIGNAL_ITEM_UPDATED,
			G_CALLBACK( on_iactions_list_item_updated_treeview ));

	/* install a virtual function as 'item-updated' handler */
	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			IACTIONS_LIST_SIGNAL_ITEM_UPDATED,
			G_CALLBACK( on_iactions_list_item_updated ));

	/* fill the model after having connected the signals
	 * so that callbacks are triggered at last
	 */
	nact_iactions_list_fill( instance, items );
}

/**
 * nact_iactions_list_all_widgets_showed:
 * @window: this #NactIActionsList *instance.
 */
void
nact_iactions_list_all_widgets_showed( NactIActionsList *instance )
{
	static const gchar *thisfn = "nact_iactions_list_all_widgets_showed";

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
}

/**
 * nact_iactions_list_dispose:
 * @window: this #NactIActionsList *instance.
 */
void
nact_iactions_list_dispose( NactIActionsList *instance )
{
	static const gchar *thisfn = "nact_iactions_list_dispose";
	GtkTreeView *treeview;
	NactTreeModel *model;

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

	treeview = get_actions_list_treeview( instance );
	model = NACT_TREE_MODEL( gtk_tree_view_get_model( treeview ));

	nact_tree_model_dispose( model );
}

/**
 * nact_iactions_list_delete_selection:
 * @window: this #NactIActionsList instance.
 * @path: a #GtkTreePath allocated here to point to the new row to select.
 *
 * Deletes the current selection from the underlying tree store.
 *
 * Returns the list of deleted #NAObject items.
 *
 * the returned @path sould be gtk_tree_path_free() by the caller.
 */
GSList *
nact_iactions_list_delete_selection( NactIActionsList *instance, GtkTreePath **path )
{
	GtkTreeView *treeview;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GList *selected;
	GSList *deleted = NULL;

	g_return_val_if_fail( NACT_IS_IACTIONS_LIST( instance ), NULL );

	treeview = get_actions_list_treeview( instance );
	selection = gtk_tree_view_get_selection( treeview );
	selected = gtk_tree_selection_get_selected_rows( selection, &model );

	if( g_list_length( selected )){
		*path = gtk_tree_path_copy(( GtkTreePath * ) selected->data );
		deleted = nact_tree_model_remove( NACT_TREE_MODEL( model ), selected );
	}

	g_list_foreach( selected, ( GFunc ) gtk_tree_path_free, NULL );
	g_list_free( selected );

	return( deleted );
}

/**
 * nact_iactions_list_fill:
 * @window: this #NactIActionsList instance.
 *
 * Fill the listbox with the provided list of items.
 *
 * Menus are expanded, profiles are not.
 * The selection is reset to the first line of the tree, if there is one.
 */
void
nact_iactions_list_fill( NactIActionsList *instance, GSList *items )
{
	static const gchar *thisfn = "nact_iactions_list_fill";
	GtkTreeView *treeview;
	NactTreeModel *model;
	gboolean only_actions;

	g_debug( "%s: instance=%p, items=%p", thisfn, ( void * ) instance, ( void * ) items );

	treeview = get_actions_list_treeview( instance );
	model = NACT_TREE_MODEL( gtk_tree_view_get_model( treeview ));
	only_actions = nact_iactions_list_is_only_actions_mode( instance );

	set_is_filling_list( instance, TRUE );
	nact_tree_model_fill( model, items, only_actions );
	set_is_filling_list( instance, FALSE );

	select_first_row( instance );
}

GSList *
nact_iactions_list_free_items_list( NactIActionsList *instance, GSList *items )
{
	g_return_val_if_fail( NACT_IS_IACTIONS_LIST( instance ), NULL );

	free_items_flat_list_callback( instance, items );

	return( NULL );
}

/**
 * nact_iactions_list_get_items_count:
 * @window: this #NactIActionsList instance.
 *
 * Returns: the current count of items in the list, whether they are
 * currently visible or not.
 */
guint
nact_iactions_list_get_items_count( NactIActionsList *instance )
{
	GtkTreeView *treeview;
	NactTreeModel *model;

	g_return_val_if_fail( NACT_IS_IACTIONS_LIST( instance ), 0 );

	treeview = get_actions_list_treeview( instance );

	model = NACT_TREE_MODEL( gtk_tree_view_get_model( treeview ));

	g_return_val_if_fail( NACT_IS_TREE_MODEL( model ), 0 );

	return( nact_tree_model_get_items_count( model ));
}

/**
 * nact_iactions_list_get_items:
 * @window: this #NactIActionsList instance.
 *
 * Returns: the current tree.
 *
 * The returned #GSList content is owned by the underlying tree model,
 * and should only be g_slist_free() by the caller.
 */
GSList *
nact_iactions_list_get_items( NactIActionsList *instance )
{
	GSList *items = NULL;
	GtkTreeView *treeview;
	NactTreeModel *model;

	g_return_val_if_fail( NACT_IS_IACTIONS_LIST( instance ), NULL );

	treeview = get_actions_list_treeview( instance );
	model = NACT_TREE_MODEL( gtk_tree_view_get_model( treeview ));

	nact_tree_model_iter( model, ( FnIterOnStore ) get_item, &items );

	/*g_debug( "nact_iactions_list_get_modified_items: count=%d", g_slist_length( items ));*/

	return( g_slist_reverse( items ));
}

/**
 * nact_iactions_list_get_modified_items:
 * @window: this #NactIActionsList instance.
 *
 * Returns a flat list of modified items.
 *
 * The returned GSList should be nact_iaction_list_free_items_list()-ed.
 */
GSList *
nact_iactions_list_get_modified_items( NactIActionsList *instance )
{
	GSList *items = NULL;
	GtkTreeView *treeview;
	NactTreeModel *model;

	g_return_val_if_fail( NACT_IS_IACTIONS_LIST( instance ), NULL );

	treeview = get_actions_list_treeview( instance );
	model = NACT_TREE_MODEL( gtk_tree_view_get_model( treeview ));

	nact_tree_model_iter( model, ( FnIterOnStore ) is_modified_item, &items );

	/*g_debug( "nact_iactions_list_get_modified_items: count=%d", g_slist_length( items ));*/

	return( g_slist_reverse( items ));
}

/**
 * nact_iactions_list_get_selected_items:
 * @window: this #NactIActionsList instance.
 *
 * Returns the currently selected actions when in export mode.
 *
 * The returned GSList should be nact_iaction_list_free_items_list()-ed.
 */
GSList *
nact_iactions_list_get_selected_items( NactIActionsList *instance )
{
	GSList *items = NULL;
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GList *it, *listrows;
	NAObject *object;
	GtkTreePath *path;

	treeview = get_actions_list_treeview( instance );
	selection = gtk_tree_view_get_selection( treeview );
	listrows = gtk_tree_selection_get_selected_rows( selection, &model );

	for( it = listrows ; it ; it = it->next ){
		path = ( GtkTreePath * ) it->data;
		gtk_tree_model_get_iter( model, &iter, path );
		gtk_tree_model_get( model, &iter, IACTIONS_LIST_NAOBJECT_COLUMN, &object, -1 );
		items = g_slist_prepend( items, object );
	}

	g_list_foreach( listrows, ( GFunc ) gtk_tree_path_free, NULL );
	g_list_free( listrows );

	return( g_slist_reverse( items ));
}

/**
 * nact_iactions_list_insert_items:
 * @instance: this #NactIActionsList instance.
 * @items: a list of items to be inserted (e.g. from a paste).
 *
 * Inserts the provided @items list in the treeview.
 *
 * The provided @items list is supposed to be homogeneous, i.e. referes
 * to a whole subtree.
 *
 * If the @items list contains only profiles, they can only be inserted
 * into an action, and the profiles will eventually be renumbered.
 * The insertion position is current, or alpha-sorted, depending on
 *
 * If the list is not sorted, the new item is inserted just before the
 * current position.
 *
 * If new item is a #NAActionMenu or a #NAAction, it will be inserted
 * before the current action or menu.
 *
 * If new item is a #NAActionProfile, it will be inserted before the
 * current profile, or as a new profile at the beginning of the list
 * of profiles of the current action.
 */
void
nact_iactions_list_insert_items( NactIActionsList *instance, GSList *items )
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *list_selected;
	GtkTreePath *path = NULL;
	NAObject *item;
	NAObject *obj_selected = NULL;
	GtkTreeIter iter;

	g_return_if_fail( NACT_IS_IACTIONS_LIST( instance ));
	/*g_return_if_fail( NA_IS_OBJECT( item ));*/

	item = NA_OBJECT( items->data );
	if( FALSE ){
		insert_item( instance, item );
	}

	treeview = get_actions_list_treeview( instance );
	model = gtk_tree_view_get_model( treeview );
	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	selection = gtk_tree_view_get_selection( treeview );
	list_selected = gtk_tree_selection_get_selected_rows( selection, NULL );
	gtk_tree_selection_unselect_all( selection );

	/* a NAObjectItem (action or menu) is inserted just before the
	 * beginning of the selection, at the same level that this beginning
	 */
	if( NA_IS_OBJECT_ITEM( item )){
		if( g_list_length( list_selected )){
			path = ( GtkTreePath * ) list_selected->data;
			gtk_tree_model_get_iter( model, &iter, path );
			gtk_tree_model_get( model, &iter, IACTIONS_LIST_NAOBJECT_COLUMN, &obj_selected, -1 );
		}
	}

	/* a NAObjectProfile is inserted just after the action if it has
	 * only one profile (it is currently selected)
	 * if current selection is a profile, then the insertion occurs
	 * just before this profile
	 */
	if( NA_IS_OBJECT_PROFILE( item )){
		g_return_if_fail( g_list_length( list_selected ));
		path = ( GtkTreePath * ) list_selected->data;
		gtk_tree_model_get_iter( model, &iter, path );
		gtk_tree_model_get( model, &iter, IACTIONS_LIST_NAOBJECT_COLUMN, &obj_selected, -1 );
	}

	nact_tree_model_insert_item( NACT_TREE_MODEL( model ), item, path, obj_selected, &iter );

	if( NA_IS_OBJECT_PROFILE( item )){
		na_object_dump( na_object_profile_get_action( NA_OBJECT_PROFILE( item )));
	} else {
		g_return_if_fail( NA_IS_OBJECT_ITEM( item ));
		na_object_dump( NA_OBJECT( item ));
	}

	path = gtk_tree_model_get_path( model, &iter );
	gtk_tree_view_expand_to_path( treeview, path );
	gtk_tree_view_set_cursor_on_cell( treeview, path, NULL, NULL, FALSE );
	gtk_tree_path_free( path );

	if( obj_selected ){
		g_object_unref( obj_selected );
	}

	g_list_foreach( list_selected, ( GFunc ) gtk_tree_path_free, NULL );
	g_list_free( list_selected );
}

/*
 * nact_iactions_list_insert_item:
 * @instance: this #NactIActionsList instance.
 * @item: a #NAActionMenu, #NAAction or #NAActionProfile to be added.
 *
 * If the list is not sorted, the new item is inserted just before the
 * current position.
 *
 * If new item is a #NAActionMenu or a #NAAction, it will be inserted
 * before the current action or menu.
 *
 * If new item is a #NAActionProfile, it will be inserted before the
 * current profile, or as a new profile at the beginning of the list
 * of profiles of the current action.
 */
static void
insert_item( NactIActionsList *instance, NAObject *item )
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *list_selected;
	GtkTreePath *path = NULL;
	NAObject *obj_selected = NULL;
	GtkTreeIter iter;

	g_return_if_fail( NACT_IS_IACTIONS_LIST( instance ));
	g_return_if_fail( NA_IS_OBJECT( item ));

	treeview = get_actions_list_treeview( instance );
	model = gtk_tree_view_get_model( treeview );
	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	selection = gtk_tree_view_get_selection( treeview );
	list_selected = gtk_tree_selection_get_selected_rows( selection, NULL );
	gtk_tree_selection_unselect_all( selection );

	/* a NAObjectItem (action or menu) is inserted just before the
	 * beginning of the selection, at the same level that this beginning
	 */
	if( NA_IS_OBJECT_ITEM( item )){
		if( g_list_length( list_selected )){
			path = ( GtkTreePath * ) list_selected->data;
			gtk_tree_model_get_iter( model, &iter, path );
			gtk_tree_model_get( model, &iter, IACTIONS_LIST_NAOBJECT_COLUMN, &obj_selected, -1 );
		}
	}

	/* a NAObjectProfile is inserted just after the action if it has
	 * only one profile (it is currently selected)
	 * if current selection is a profile, then the insertion occurs
	 * just before this profile
	 */
	if( NA_IS_OBJECT_PROFILE( item )){
		g_return_if_fail( g_list_length( list_selected ));
		path = ( GtkTreePath * ) list_selected->data;
		gtk_tree_model_get_iter( model, &iter, path );
		gtk_tree_model_get( model, &iter, IACTIONS_LIST_NAOBJECT_COLUMN, &obj_selected, -1 );
	}

	nact_tree_model_insert_item( NACT_TREE_MODEL( model ), item, path, obj_selected, &iter );

	if( NA_IS_OBJECT_PROFILE( item )){
		na_object_dump( na_object_profile_get_action( NA_OBJECT_PROFILE( item )));
	} else {
		g_return_if_fail( NA_IS_OBJECT_ITEM( item ));
		na_object_dump( NA_OBJECT( item ));
	}

	path = gtk_tree_model_get_path( model, &iter );
	gtk_tree_view_expand_to_path( treeview, path );
	gtk_tree_view_set_cursor_on_cell( treeview, path, NULL, NULL, FALSE );
	gtk_tree_path_free( path );

	if( obj_selected ){
		g_object_unref( obj_selected );
	}

	g_list_foreach( list_selected, ( GFunc ) gtk_tree_path_free, NULL );
	g_list_free( list_selected );
}

/**
 * nact_iactions_list_is_filling_list:
 * @instance: this #NactIActionsList instance.
 *
 * Returns %TRUE if currently being filling the list.
 */
gboolean
nact_iactions_list_is_filling_list( NactIActionsList *instance )
{
	return(( gboolean ) GPOINTER_TO_INT( g_object_get_data( G_OBJECT( instance ), IS_FILLING_LIST )));
}

/**
 * nact_iactions_list_is_filling_list:
 * @instance: this #NactIActionsList instance.
 * @action: a #NAAction action.
 *
 * Returns %TRUE if the action is expanded (i.e. if its profiles are
 * visible).
 */
gboolean
nact_iactions_list_is_expanded( NactIActionsList *instance, const NAObject *item )
{
	GtkTreeView *treeview;
	GtkTreeModel *model;
	gboolean is_expanded;
	GtkTreeIter iter;
	gboolean iterok, stop;
	NAObject *iter_object;

	g_return_val_if_fail( NACT_IS_IACTIONS_LIST( instance ), FALSE );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), FALSE );

	treeview = get_actions_list_treeview( instance );
	model = gtk_tree_view_get_model( treeview );
	iterok = gtk_tree_model_get_iter_first( model, &iter );
	stop = FALSE;
	is_expanded = FALSE;

	while( iterok && !stop ){
		gtk_tree_model_get( model, &iter, IACTIONS_LIST_NAOBJECT_COLUMN, &iter_object, -1 );

		if( iter_object == item ){
			GtkTreePath *path = gtk_tree_model_get_path( model, &iter );
			is_expanded = gtk_tree_view_row_expanded( treeview, path );
			gtk_tree_path_free( path );
			stop = TRUE;
		}

		g_object_unref( iter_object );
		iterok = gtk_tree_model_iter_next( model, &iter );
	}

	return( is_expanded );
}

/**
 * nact_iactions_list_is_only_actions_mode:
 * @window: this #NactIActionsList instance.
 *
 * Returns %TRUE if only actions should be displayed in the treeview.
 */
gboolean
nact_iactions_list_is_only_actions_mode( NactIActionsList *instance )
{
	return(( gboolean ) GPOINTER_TO_INT( g_object_get_data( G_OBJECT( instance ), SHOW_ONLY_ACTIONS_MODE )));
}

/**
 * nact_iactions_list_select_row:
 * @window: this #NactIActionsList instance.
 * @path: a #GtkTreePath.
 */
void
nact_iactions_list_select_row( NactIActionsList *instance, GtkTreePath *path )
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;

	treeview = get_actions_list_treeview( instance );
	selection = gtk_tree_view_get_selection( treeview );
	model = gtk_tree_view_get_model( treeview );

	if( gtk_tree_model_get_iter( model, &iter, path )){
		gtk_tree_selection_select_iter( selection, &iter );
	}
}

/**
 * nact_iactions_list_set_dnd_mode:
 * @window: this #NactIActionsList instance.
 * @have_dnd: whether the treeview implements drag and drop ?
 *
 * When set to %TRUE, the corresponding tree model will implement the
 * GtkTreeDragSource and the GtkTreeDragDest interfaces.
 *
 * This property defaults to %FALSE.
 */
void
nact_iactions_list_set_dnd_mode( NactIActionsList *instance, gboolean have_dnd )
{
	g_object_set_data( G_OBJECT( instance ), HAVE_DND_MODE, GINT_TO_POINTER( have_dnd ));
}

/**
 * nact_iactions_list_set_multiple_selection_mode:
 * @window: this #NactIActionsList instance.
 * @multiple: whether the treeview does support multiple selection ?
 *
 * If %FALSE, only one item can selected at same time. Set to %TRUE to
 * be able to have multiple items simultaneously selected.
 *
 * This property defaults to %FALSE.
 */
void
nact_iactions_list_set_multiple_selection_mode( NactIActionsList *instance, gboolean multiple )
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;

	treeview = get_actions_list_treeview( instance );
	selection = gtk_tree_view_get_selection( treeview );
	gtk_tree_selection_set_mode( selection, multiple ? GTK_SELECTION_MULTIPLE : GTK_SELECTION_SINGLE );
}

/**
 * nact_iactions_list_set_only_actions_mode:
 * @window: this #NactIActionsList instance.
 * @only_actions: whether the treeview must only display actions ?
 *
 * When @only_actions is %TRUE, then the treeview will only display the
 * list of actions in alphabetical order of their label. In this mode,
 * the actual value of the 'Display in alphabetical order' preference
 * is ignored.
 *
 * If @only_actions is %FALSE, the the treeview display all the tree
 * of menus, submenus, actions and profiles.
 *
 * This property defaults to %FALSE.
 */
void
nact_iactions_list_set_only_actions_mode( NactIActionsList *instance, gboolean only_actions )
{
	g_object_set_data( G_OBJECT( instance ), SHOW_ONLY_ACTIONS_MODE, GINT_TO_POINTER( only_actions ));
}

/*
 * Collapse / expand if actions has more than one profile
 */
void
nact_iactions_list_toggle_collapse( NactIActionsList *instance, const NAObject *item )
{
	static const gchar *thisfn = "nact_iactions_list_toggle_collapse";
	GtkTreeView *treeview;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean iterok, stop;
	NAObject *iter_object;

	g_return_if_fail( NACT_IS_IACTIONS_LIST( instance ));
	g_return_if_fail( NA_IS_OBJECT_ITEM( item ));

	treeview = get_actions_list_treeview( instance );
	model = gtk_tree_view_get_model( treeview );
	iterok = gtk_tree_model_get_iter_first( model, &iter );
	stop = FALSE;

	while( iterok && !stop ){

		gtk_tree_model_get( model, &iter, IACTIONS_LIST_NAOBJECT_COLUMN, &iter_object, -1 );
		if( iter_object == item ){

			if( na_object_get_items_count( item ) > 1 ){

				GtkTreePath *path = gtk_tree_model_get_path( model, &iter );

				if( gtk_tree_view_row_expanded( GTK_TREE_VIEW( treeview ), path )){
					gtk_tree_view_collapse_row( GTK_TREE_VIEW( treeview ), path );
					g_debug( "%s: action=%p collapsed", thisfn, ( void * ) item );

				} else {
					gtk_tree_view_expand_row( GTK_TREE_VIEW( treeview ), path, TRUE );
					g_debug( "%s: action=%p expanded", thisfn, ( void * ) item );
				}

				gtk_tree_path_free( path );
			}
			stop = TRUE;
		}

		g_object_unref( iter_object );
		iterok = gtk_tree_model_iter_next( model, &iter );
	}
}

/*
 * item modified: italic
 * item not saveable (invalid): red
 */
static void
display_label( GtkTreeViewColumn *column, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, NactIActionsList *instance )
{
	NAObject *object;
	gchar *label;
	gboolean modified = FALSE;
	gboolean valid = TRUE;

	gtk_tree_model_get( model, iter, IACTIONS_LIST_NAOBJECT_COLUMN, &object, -1 );

	if( object ){
		g_assert( NA_IS_OBJECT( object ));
		label = na_object_get_label( object );
		modified = na_object_is_modified( object );
		valid = na_object_is_valid( object );

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
		g_object_unref( object );
	}
}

/*
 * when expanding a selected row which has childs
 */
static void
extend_selection_to_childs( NactIActionsList *instance, GtkTreeView *treeview, GtkTreeModel *model, GtkTreeIter *parent )
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	gboolean ok;

	selection = gtk_tree_view_get_selection( treeview );

	ok = gtk_tree_model_iter_children( model, &iter, parent );

	while( ok ){
		GtkTreePath *path = gtk_tree_model_get_path( model, &iter );
		gtk_tree_selection_select_path( selection, path );
		gtk_tree_path_free( path );
		ok = gtk_tree_model_iter_next( model, &iter );
	}
}

static void
free_items_flat_list_callback( NactIActionsList *instance, GSList *items )
{
	g_slist_foreach( items, ( GFunc ) g_object_unref, NULL );
	g_slist_free( items );
}

static GtkTreeView *
get_actions_list_treeview( NactIActionsList *instance )
{
	GtkTreeView *treeview;

	treeview = GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( instance ), "ActionsList" ));
	g_assert( GTK_IS_TREE_VIEW( treeview ));

	return( treeview );
}

/*
 * builds the tree
 */
static gboolean
get_item( NactTreeModel *model, GtkTreePath *path, NAObject *object, GSList **items )
{
	if( gtk_tree_path_get_depth( path ) == 1 ){
		*items = g_slist_prepend( *items, object );
	}

	/* don't stop iteration */
	return( FALSE );
}

static gboolean
have_dnd_mode( NactIActionsList *instance )
{
	return(( gboolean ) GPOINTER_TO_INT( g_object_get_data( G_OBJECT( instance ), HAVE_DND_MODE )));
}

/*
 * count _all_ modified rows, including profiles
 */
static gboolean
is_modified_item( NactTreeModel *model, GtkTreePath *path, NAObject *object, GSList **items )
{
	/*if( !NA_IS_ACTION_PROFILE( object )){*/
		if( na_object_is_modified( object )){
			*items = g_slist_prepend( *items, g_object_ref( object ));
		}
	/*}*/

	/* don't stop iteration */
	return( FALSE );
}

static void
iter_on_selection( NactIActionsList *instance, FnIterOnSelection fn_iter, gpointer user_data )
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *listrows, *ipath;
	GtkTreePath *path;
	GtkTreeIter iter;
	NAObject *object;
	gboolean stop = FALSE;

	treeview = get_actions_list_treeview( instance );
	selection = gtk_tree_view_get_selection( treeview );
	listrows = gtk_tree_selection_get_selected_rows( selection, &model );
	listrows = g_list_reverse( listrows );

	for( ipath = listrows ; !stop && ipath ; ipath = ipath->next ){

		path = ( GtkTreePath * ) ipath->data;
		gtk_tree_model_get_iter( model, &iter, path );
		gtk_tree_model_get( model, &iter, IACTIONS_LIST_NAOBJECT_COLUMN, &object, -1 );

		stop = fn_iter( instance, treeview, model, &iter, object, user_data );

		g_object_unref( object );
	}

	g_list_foreach( listrows, ( GFunc ) gtk_tree_path_free, NULL );
	g_list_free( listrows );
}

static gboolean
on_button_press_event( GtkWidget *widget, GdkEventButton *event, NactIActionsList *instance )
{
	/*static const gchar *thisfn = "nact_iactions_list_v_on_button_pres_event";
	g_debug( "%s: widget=%p, event=%p, user_data=%p", thisfn, widget, event, user_data );*/

	gboolean stop = FALSE;

	if( event->type == GDK_2BUTTON_PRESS ){
		toggle_collapse( instance );
		stop = TRUE;
	}

	return( stop );
}

static gboolean
on_key_pressed_event( GtkWidget *widget, GdkEventKey *event, NactIActionsList *instance )
{
	/*static const gchar *thisfn = "nact_iactions_list_v_on_key_pressed_event";
	g_debug( "%s: widget=%p, event=%p, user_data=%p", thisfn, widget, event, user_data );*/

	gboolean stop = FALSE;

	if( event->keyval == GDK_Return || event->keyval == GDK_KP_Enter ){
		toggle_collapse( instance );
		stop = TRUE;
	}

	return( stop );
}

/*
 * this is our handler of "changed" signal emitted by the treeview
 */
static void
on_treeview_selection_changed( GtkTreeSelection *selection, NactIActionsList *instance )
{
	GSList *selected_items;

	selected_items = nact_iactions_list_get_selected_items( instance );

	if( !nact_iactions_list_is_filling_list( instance )){
		g_signal_emit_by_name( instance, IACTIONS_LIST_SIGNAL_SELECTION_CHANGED, selected_items );
	}
}

/*
 * our handler for "item-updated" emitted whan an item is modified
 * this let us transform the signal in a virtual function
 * so that our implementors have the best of two worlds ;-)
 */
static void
on_iactions_list_item_updated( NactIActionsList *instance, GSList *updated_items )
{
	if( NACT_IACTIONS_LIST_GET_INTERFACE( instance )->item_updated ){
		NACT_IACTIONS_LIST_GET_INTERFACE( instance )->item_updated( instance, updated_items );
	}
}

/*
 * rewind all the parent hierarchy of each modified item to update its status
 */
static void
on_iactions_list_item_updated_treeview( NactIActionsList *instance, GSList *updated_items )
{
	GtkTreeView *treeview;
	NactTreeModel *model;
	GSList *it;

	treeview = get_actions_list_treeview( instance );
	model = NACT_TREE_MODEL( gtk_tree_view_get_model( treeview ));

	for( it = updated_items ; it ; it=it->next ){
		nact_tree_model_update_parent( model, NA_OBJECT( it->data ));
	}
}

/*
 * our handler for "selection-changed" emitted by the interface
 * this let us transform the signal in a virtual function
 * so that our implementors have the best of two worlds ;-)
 */
static void
on_iactions_list_selection_changed( NactIActionsList *instance, GSList *selected_items )
{
	if( NACT_IACTIONS_LIST_GET_INTERFACE( instance )->selection_changed ){
		NACT_IACTIONS_LIST_GET_INTERFACE( instance )->selection_changed( instance, selected_items );
	}
}

static void
select_first_row( NactIActionsList *instance )
{
	static const gchar *thisfn = "nact_iactions_list_select_first_row";
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean iterok;

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

	treeview = get_actions_list_treeview( instance );
	selection = gtk_tree_view_get_selection( treeview );
	model = gtk_tree_view_get_model( treeview );

	iterok = gtk_tree_model_get_iter_first( model, &iter );
	if( !iterok ){
		g_debug( "%s: empty actions list: unselect all", thisfn );
		gtk_tree_selection_unselect_all( selection );
	} else {
		gtk_tree_selection_select_iter( selection, &iter );
	}
}

static void
set_is_filling_list( NactIActionsList *instance, gboolean is_filling )
{
	g_object_set_data( G_OBJECT( instance ), IS_FILLING_LIST, GINT_TO_POINTER( is_filling ));
}

static void
toggle_collapse( NactIActionsList *instance )
{
	int toggle = TOGGLE_UNDEFINED;

	iter_on_selection( instance, ( FnIterOnSelection ) toggle_collapse_iter, &toggle );
}

static gboolean
toggle_collapse_iter( NactIActionsList *instance,
						GtkTreeView *treeview,
						GtkTreeModel *model,
						GtkTreeIter *iter,
						NAObject *object,
						gpointer user_data )
{
	guint count;
	guint *toggle;

	toggle = ( guint * ) user_data;

	if( NA_IS_OBJECT_ITEM( object )){

		GtkTreePath *path = gtk_tree_model_get_path( model, iter );

		if( NA_IS_OBJECT_ITEM( object )){
			count = na_object_get_items_count( object );
			if( count > 1 ){
				toggle_collapse_row( treeview, path, toggle );
			}
		}

		gtk_tree_path_free( path );

		/* do not extend selection */
		if( *toggle == TOGGLE_EXPAND && FALSE ){
			extend_selection_to_childs( instance, treeview, model, iter );
		}
	}

	/* do not stop iteration */
	return( FALSE );
}

/*
 * toggle mode can be undefined, collapse or expand
 * it is set on the first row
 */
static void
toggle_collapse_row( GtkTreeView *treeview, GtkTreePath *path, guint *toggle )
{
	if( *toggle == TOGGLE_UNDEFINED ){
		*toggle = gtk_tree_view_row_expanded( treeview, path ) ? TOGGLE_COLLAPSE : TOGGLE_EXPAND;
	}

	if( *toggle == TOGGLE_COLLAPSE ){
		if( gtk_tree_view_row_expanded( treeview, path )){
			gtk_tree_view_collapse_row( treeview, path );
		}
	} else {
		if( !gtk_tree_view_row_expanded( treeview, path )){
			gtk_tree_view_expand_row( treeview, path, TRUE );
		}
	}
}
