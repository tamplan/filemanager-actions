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
#include <common/na-iprefs.h>

#include "nact-application.h"
#include "nact-marshal.h"
#include "nact-main-tab.h"
#include "nact-tree-model.h"
#include "nact-window.h"
#include "nact-iactions-list.h"

/* private interface data
 */
struct NactIActionsListInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* data set against the GObject instance
 */
typedef struct {

	/* management mode
	 */
	gint     management_mode;

	/* counters
	 * initialized when filling the list, updated on insert/delete
	 */
	gint     menus;
	gint     actions;
	gint     profiles;

	/* signal management
	 */
	gulong   tab_updated_handler;
	gboolean selection_changed_send_allowed;
}
	IActionsListInstanceData;

#define IACTIONS_LIST_DATA_INSTANCE		"nact-iactions-list-instance-data"

/* signals
 */
enum {
	LIST_COUNT_UPDATED,
	SELECTION_CHANGED,
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

/* when iterating through a selection
 */
typedef struct {
	guint nb_profiles;
	guint nb_actions;
	guint nb_menus;
}
	SelectionIter;

/* when iterating while searching for the path of an object
 */
typedef struct {
	NAObject    *object;
	gboolean     found;
	GtkTreePath *path;
}
	ObjectToPathIter;

/* when iterating while searching for an object
 */
typedef struct {
	NAObject *object;
	gchar    *uuid;
}
	IdToObjectIter;

static gint         st_signals[ LAST_SIGNAL ] = { 0 };
static gboolean     st_initialized = FALSE;
static gboolean     st_finalized = FALSE;

static GType        register_type( void );
static void         interface_base_init( NactIActionsListInterface *klass );
static void         interface_base_finalize( NactIActionsListInterface *klass );

static void         free_items_callback( NactIActionsList *instance, GList *items );
static void         decrement_counters( NactIActionsList *instance, IActionsListInstanceData *ialid, GList *items );
static GtkTreePath *get_selection_first_path( GtkTreeView *treeview );
static void         do_insert_items( GtkTreeView *treeview, GtkTreeModel *model, GList *items, GtkTreePath *path, GList **parents );
static NAObject    *do_insert_into_first( GtkTreeView *treeview, GtkTreeModel *model, GList *items, GtkTreePath *insert_path, GtkTreePath **new_path );
static GtkTreePath *do_insert_into_second( GtkTreeView *treeview, GtkTreeModel *model, NAObject *object, GtkTreePath *insert_path, NAObject **parent );
static void         increment_counters( NactIActionsList *instance, IActionsListInstanceData *ialid, GList *items );
static void         update_parents_edition_status( GList *parents, GList *items );

static gchar       *v_get_treeview_name( NactIActionsList *instance );

static void         display_label( GtkTreeViewColumn *column, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, NactIActionsList *instance );
static void         extend_selection_to_childs( NactIActionsList *instance, GtkTreeView *treeview, GtkTreeModel *model, GtkTreeIter *parent );
static gboolean     filter_selection( GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path, gboolean path_currently_selected, NactIActionsList *instance );
static void         filter_selection_iter( GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, SelectionIter *str );
static GtkTreeView *get_actions_list_treeview( NactIActionsList *instance );
static gboolean     get_item_iter( NactTreeModel *model, GtkTreePath *path, NAObject *object, IdToObjectIter *ito );
static gboolean     get_items_iter( NactTreeModel *model, GtkTreePath *path, NAObject *object, GList **items );
static gboolean     has_modified_iter( NactTreeModel *model, GtkTreePath *path, NAObject *object, gboolean *has_modified );
static gboolean     have_dnd_mode( NactIActionsList *instance, IActionsListInstanceData *ialid );
static gboolean     have_filter_selection_mode( NactIActionsList *instance, IActionsListInstanceData *ialid );
static gboolean     have_only_actions( NactIActionsList *instance, IActionsListInstanceData *ialid );
static gboolean     is_iduplicable_proxy( NactIActionsList *instance, IActionsListInstanceData *ialid );
static void         iter_on_selection( NactIActionsList *instance, FnIterOnSelection fn_iter, gpointer user_data );
static GtkTreePath *object_to_path( NactIActionsList *instance, NactTreeModel *model, NAObject *object );
static gboolean     object_to_path_iter( NactTreeModel *model, GtkTreePath *path, NAObject *object, ObjectToPathIter *otp );
static gboolean     on_button_press_event( GtkWidget *widget, GdkEventButton *event, NactIActionsList *instance );
static void         on_edition_status_changed( NactIActionsList *instance, NAIDuplicable *object );
static gboolean     on_key_pressed_event( GtkWidget *widget, GdkEventKey *event, NactIActionsList *instance );
static void         on_treeview_selection_changed( GtkTreeSelection *selection, NactIActionsList *instance );
static void         on_tab_updatable_item_updated( NactIActionsList *instance, NAObject *object );
static void         on_iactions_list_selection_changed( NactIActionsList *instance, GSList *selected_items );
static void         select_first_row( NactIActionsList *instance );
static void         select_row_at_path( NactIActionsList *instance, GtkTreeView *treeview, GtkTreeModel *model, GtkTreePath *path );
static void         send_list_count_updated_signal( NactIActionsList *instance, IActionsListInstanceData *ialid );
static void         toggle_collapse( NactIActionsList *instance );
static gboolean     toggle_collapse_iter( NactIActionsList *instance, GtkTreeView *treeview, GtkTreeModel *model, GtkTreeIter *iter, NAObject *object, gpointer user_data );
static void         toggle_collapse_row( GtkTreeView *treeview, GtkTreePath *path, guint *toggle );

static IActionsListInstanceData *get_instance_data( NactIActionsList *instance );

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

	if( !st_initialized ){
		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NactIActionsListInterfacePrivate, 1 );

		/**
		 * nact-iactions-list-count-updated:
		 *
		 * This signal is emitted byIActionsList to its implementor when
		 * it has been asked to refill the list.
		 *
		 * It sends as arguments to the connected handlers the total
		 * count of menus, actions and profiles in the stored list.
		 */
		st_signals[ LIST_COUNT_UPDATED ] = g_signal_new(
				IACTIONS_LIST_SIGNAL_LIST_COUNT_UPDATED,
				G_TYPE_OBJECT,
				G_SIGNAL_RUN_LAST,
				0,						/* no default handler */
				NULL,
				NULL,
				nact_marshal_VOID__INT_INT_INT,
				G_TYPE_NONE,
				3,
				G_TYPE_INT,
				G_TYPE_INT,
				G_TYPE_INT );

		/**
		 * nact-iactions-list-selection-changed:
		 *
		 * This signal is emitted byIActionsList to its implementor,
		 * in response to the "changed" Gtk signal, each time the
		 * selection has changed in the treeview.
		 *
		 * It is not just a proxy, as we add a list of currently selected
		 * objects as user_data (see #on_treeview_selection_changed()).
		 *
		 * Note that IActionsList is itself connected to this signal,
		 * in order to convert the signal to an interface API
		 * (see #on_iactions_list_selection_changed()).
		 *
		 * The main window is typically the only interested. It will
		 * setup current item and profiles, before emitting another
		 * signal targeting the notebook tabs
		 * (see. TAB_UPDATABLE_SIGNAL_SELECTION_CHANGED signal).
		 */
		st_signals[ SELECTION_CHANGED ] = g_signal_new_class_handler(
				IACTIONS_LIST_SIGNAL_SELECTION_CHANGED,
				G_TYPE_OBJECT,
				G_SIGNAL_RUN_CLEANUP,
				G_CALLBACK( free_items_callback ),
				NULL,
				NULL,
				g_cclosure_marshal_VOID__POINTER,
				G_TYPE_NONE,
				1,
				G_TYPE_POINTER );

		st_initialized = TRUE;
	}
}

static void
free_items_callback( NactIActionsList *instance, GList *items )
{
	g_debug( "free_items_callback: selection=%p (%d items)", ( void * ) items, g_list_length( items ));
	na_object_free_items( items );
}

static void
interface_base_finalize( NactIActionsListInterface *klass )
{
	static const gchar *thisfn = "nact_iactions_list_interface_base_finalize";

	if( !st_finalized ){
		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );

		st_finalized = TRUE;
	}
}

/**
 * nact_iactions_list_initial_load_toplevel:
 * @instance: this #NactIActionsList *instance.
 *
 * Allocates and initializes the ActionsList widget.
 *
 * GtkTreeView is created with NactTreeModel model
 * NactTreeModel
 *   implements EggTreeMultiDragSourceIface
 *   is derived from GtkTreeModelFilter
 *     GtkTreeModelFilter is built on top of GtkTreeStore
 *
 * Please note that management mode for the list should have been set
 * before calling this function.
 */
void
nact_iactions_list_initial_load_toplevel( NactIActionsList *instance )
{
	static const gchar *thisfn = "nact_iactions_list_initial_load_toplevel";
	GtkWidget *label;
	GtkTreeView *treeview;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	IActionsListInstanceData *ialid;

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_IACTIONS_LIST( instance ));

	if( st_initialized && !st_finalized ){

		treeview = get_actions_list_treeview( instance );

		ialid = get_instance_data( instance );
		ialid->selection_changed_send_allowed = FALSE;

		/* associates the ActionsList to the label */
		label = base_window_get_widget( BASE_WINDOW( instance ), "ActionsListLabel" );
		gtk_label_set_mnemonic_widget( GTK_LABEL( label ), GTK_WIDGET( treeview ));

		nact_tree_model_initial_load( BASE_WINDOW( instance ), treeview );

		gtk_tree_view_set_enable_tree_lines( treeview, TRUE );

		/* create visible columns on the tree view
		 */
		/* icon: no header */
		column = gtk_tree_view_column_new_with_attributes(
				"", gtk_cell_renderer_pixbuf_new(), "pixbuf", IACTIONS_LIST_ICON_COLUMN, NULL );
		gtk_tree_view_append_column( treeview, column );

		column = gtk_tree_view_column_new();
		/* i18n: header of the 'label' column in the treeview */
		gtk_tree_view_column_set_title( column, _( "Label" ));
		gtk_tree_view_column_set_sort_column_id( column, IACTIONS_LIST_LABEL_COLUMN );
		renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start( column, renderer, TRUE );
		gtk_tree_view_column_set_cell_data_func(
				column, renderer, ( GtkTreeCellDataFunc ) display_label, instance, NULL );
		gtk_tree_view_append_column( treeview, column );
	}
}

/**
 * nact_iactions_list_runtime_init_toplevel:
 * @window: this #NactIActionsList *instance.
 * @items: list of #NAObject actions and menus as provided by #NAPivot.
 *
 * Allocates and initializes the ActionsList widget.
 */
void
nact_iactions_list_runtime_init_toplevel( NactIActionsList *instance, GList *items )
{
	static const gchar *thisfn = "nact_iactions_list_runtime_init_toplevel";
	GtkTreeView *treeview;
	NactTreeModel *model;
	gboolean have_dnd;
	gboolean have_filter_selection;
	gboolean is_proxy;
	GtkTreeSelection *selection;
	IActionsListInstanceData *ialid;

	g_debug( "%s: instance=%p, items=%p (%d items)",
			thisfn, ( void * ) instance, ( void * ) items, g_list_length( items ));
	g_return_if_fail( NACT_IS_IACTIONS_LIST( instance ));

	if( st_initialized && !st_finalized ){

		treeview = get_actions_list_treeview( instance );
		model = NACT_TREE_MODEL( gtk_tree_view_get_model( treeview ));

		ialid = get_instance_data( instance );
		have_dnd = have_dnd_mode( instance, ialid );
		have_filter_selection = have_filter_selection_mode( instance, ialid );

		if( have_filter_selection ){
			selection = gtk_tree_view_get_selection( treeview );
			gtk_tree_selection_set_select_function( selection, ( GtkTreeSelectionFunc ) filter_selection, instance, NULL );
		}

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
		ialid->tab_updated_handler = base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( instance ),
				TAB_UPDATABLE_SIGNAL_ITEM_UPDATED,
				G_CALLBACK( on_tab_updatable_item_updated ));

		/* records NactIActionsList as a proxy for edition status
		 * modification */
		is_proxy = is_iduplicable_proxy( instance, ialid );
		if( is_proxy ){
			na_iduplicable_register_consumer( G_OBJECT( instance ));

			base_window_signal_connect(
					BASE_WINDOW( instance ),
					G_OBJECT( instance ),
					NA_IDUPLICABLE_SIGNAL_MODIFIED_CHANGED,
					G_CALLBACK( on_edition_status_changed ));

			base_window_signal_connect(
					BASE_WINDOW( instance ),
					G_OBJECT( instance ),
					NA_IDUPLICABLE_SIGNAL_VALID_CHANGED,
					G_CALLBACK( on_edition_status_changed ));
		}

		/* fill the model after having connected the signals
		 * so that callbacks are triggered at last
		 */
		nact_iactions_list_fill( instance, items );
	}
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
	g_return_if_fail( NACT_IS_IACTIONS_LIST( instance ));

	if( st_initialized && !st_finalized ){
	}
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
	IActionsListInstanceData *ialid;

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_IACTIONS_LIST( instance ));

	if( st_initialized && !st_finalized ){

		treeview = get_actions_list_treeview( instance );
		model = NACT_TREE_MODEL( gtk_tree_view_get_model( treeview ));

		nact_tree_model_dispose( model );

		ialid = get_instance_data( instance );
		g_free( ialid );
	}
}

/**
 * nact_iactions_list_collapse_all:
 * @instance: this #NactIActionsList implementation.
 *
 * Collapse all the tree hierarchy.
 */
void
nact_iactions_list_collapse_all( NactIActionsList *instance )
{
	static const gchar *thisfn = "nact_iactions_list_collapse_all";
	GtkTreeView *treeview;

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_IACTIONS_LIST( instance ));

	if( st_initialized && !st_finalized ){

		treeview = get_actions_list_treeview( instance );
		gtk_tree_view_collapse_all( treeview );
	}
}

/**
 * nact_iactions_list_delete:
 * @window: this #NactIActionsList instance.
 * @list: list of #NAObject to be deleted.
 *
 * Deletes the specified list from the underlying tree store.
 *
 * This function takes care of repositionning a new selection if
 * possible, and refilter the display model.
 */
void
nact_iactions_list_delete( NactIActionsList *instance, GList *items )
{
	GtkTreeView *treeview;
	GtkTreeModel *model;
	GtkTreePath *path = NULL;
	GList *it;
	IActionsListInstanceData *ialid;

	g_return_if_fail( NACT_IS_IACTIONS_LIST( instance ));

	if( st_initialized && !st_finalized ){

		treeview = get_actions_list_treeview( instance );
		model = gtk_tree_view_get_model( treeview );

		ialid = get_instance_data( instance );
		ialid->selection_changed_send_allowed = FALSE;

		decrement_counters( instance, ialid, items );

		for( it = items ; it ; it = it->next ){
			if( path ){
				gtk_tree_path_free( path );
			}
			path = nact_tree_model_remove( NACT_TREE_MODEL( model ), NA_OBJECT( it->data ));
		}

		gtk_tree_model_filter_refilter( GTK_TREE_MODEL_FILTER( model ));

		ialid->selection_changed_send_allowed = TRUE;

		if( path ){
			select_row_at_path( instance, treeview, model, path );
			gtk_tree_path_free( path );
		}
	}
}

static void
decrement_counters( NactIActionsList *instance, IActionsListInstanceData *ialid, GList *items )
{
	static const gchar *thisfn = "nact_iactions_list_decrement_counters";
	gint menus, actions, profiles;

	g_debug( "%s: instance=%p, ialid=%p, items=%p",
			thisfn, ( void * ) instance, ( void * ) ialid, ( void * ) items );

	menus = 0;
	actions = 0;
	profiles = 0;
	na_object_item_count_items( items, &menus, &actions, &profiles, TRUE );

	ialid->menus -= menus;
	ialid->actions -= actions;
	ialid->profiles -= profiles;

	send_list_count_updated_signal( instance, ialid );
}

/**
 * nact_iactions_list_display_order_change:
 * @instance: this #NactIActionsList implementation.
 * @order_mode: the new order mode.
 *
 * Setup the new order mode.
 */
void
nact_iactions_list_display_order_change( NactIActionsList *instance, gint order_mode )
{
	GtkTreeView *treeview;
	NactTreeModel *model;

	g_return_if_fail( NACT_IS_IACTIONS_LIST( instance ));

	if( st_initialized && !st_finalized ){

		treeview = get_actions_list_treeview( instance );
		model = NACT_TREE_MODEL( gtk_tree_view_get_model( treeview ));
		nact_tree_model_display_order_change( model, order_mode );
	}
}

/**
 * nact_iactions_list_expand_all:
 * @instance: this #NactIActionsList implementation.
 *
 * Expand all the tree hierarchy.
 */
void
nact_iactions_list_expand_all( NactIActionsList *instance )
{
	static const gchar *thisfn = "nact_iactions_list_expand_all";
	GtkTreeView *treeview;

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_IACTIONS_LIST( instance ));

	if( st_initialized && !st_finalized ){

		treeview = get_actions_list_treeview( instance );
		gtk_tree_view_expand_all( treeview );
	}
}

/**
 * nact_iactions_list_fill:
 * @instance: this #NactIActionsList instance.
 *
 * Fill the listbox with the provided list of items.
 *
 * Menus are expanded, profiles are not.
 * The selection is reset to the first line of the tree, if there is one.
 */
void
nact_iactions_list_fill( NactIActionsList *instance, GList *items )
{
	static const gchar *thisfn = "nact_iactions_list_fill";
	GtkTreeView *treeview;
	NactTreeModel *model;
	gboolean only_actions;
	IActionsListInstanceData *ialid;

	g_debug( "%s: instance=%p, items=%p", thisfn, ( void * ) instance, ( void * ) items );
	g_return_if_fail( NACT_IS_IACTIONS_LIST( instance ));

	if( st_initialized && !st_finalized ){

		treeview = get_actions_list_treeview( instance );
		model = NACT_TREE_MODEL( gtk_tree_view_get_model( treeview ));

		ialid = get_instance_data( instance );
		only_actions = have_only_actions( instance, ialid );

		ialid->selection_changed_send_allowed = FALSE;
		nact_tree_model_fill( model, items, only_actions );
		ialid->selection_changed_send_allowed = TRUE;

		ialid->menus = 0;
		ialid->actions = 0;
		ialid->profiles = 0;
		na_object_item_count_items( items, &ialid->menus, &ialid->actions, &ialid->profiles, TRUE );
		send_list_count_updated_signal( instance, ialid );

		select_first_row( instance );
	}
}

/**
 * nact_iactions_list_get_item:
 * @window: this #NactIActionsList instance.
 * @uuid: the id of the searched object.
 *
 * Returns: a pointer on the #NAObject which has this id, or NULL.
 *
 * The returned pointer is owned by IActionsList (actually by the
 * underlying tree store), and should not be released by the caller.
 */
NAObject *
nact_iactions_list_get_item( NactIActionsList *instance, const gchar *uuid )
{
	NAObject *item = NULL;
	GtkTreeView *treeview;
	NactTreeModel *model;
	IdToObjectIter *ito;

	g_return_val_if_fail( NACT_IS_IACTIONS_LIST( instance ), NULL );

	if( st_initialized && !st_finalized ){

		treeview = get_actions_list_treeview( instance );
		model = NACT_TREE_MODEL( gtk_tree_view_get_model( treeview ));

		ito = g_new0( IdToObjectIter, 1 );
		ito->uuid = ( gchar * ) uuid;

		nact_tree_model_iter( model, ( FnIterOnStore ) get_item_iter, ito );

		item = ito->object;

		g_free( ito );
	}

	return( item );
}

/**
 * nact_iactions_list_get_items:
 * @window: this #NactIActionsList instance.
 *
 * Returns: the current tree.
 *
 * The returned #GList content is owned by the underlying tree model,
 * and should only be g_list_free() by the caller.
 */
GList *
nact_iactions_list_get_items( NactIActionsList *instance )
{
	GList *items = NULL;
	GtkTreeView *treeview;
	NactTreeModel *model;

	g_return_val_if_fail( NACT_IS_IACTIONS_LIST( instance ), NULL );

	if( st_initialized && !st_finalized ){

		treeview = get_actions_list_treeview( instance );
		model = NACT_TREE_MODEL( gtk_tree_view_get_model( treeview ));

		nact_tree_model_iter( model, ( FnIterOnStore ) get_items_iter, &items );

		items = g_list_reverse( items );
	}

	return( items );
}

/**
 * nact_iactions_list_get_management_mode:
 * @instance: this #NactIActionsList instance.
 *
 * Returns: the current management mode of the list.
 */
gint
nact_iactions_list_get_management_mode( NactIActionsList *instance )
{
	gint mode = 0;
	IActionsListInstanceData *ialid;

	g_return_val_if_fail( NACT_IS_IACTIONS_LIST( instance ), 0 );

	if( st_initialized && !st_finalized ){

		ialid = get_instance_data( instance );
		mode = ialid->management_mode;
	}

	return( mode );
}

/**
 * nact_iactions_list_get_selected_items:
 * @window: this #NactIActionsList instance.
 *
 * Returns: the currently selected rows as a list of #NAObjects.
 *
 * We acquire here a new reference on objects corresponding to actually
 * selected rows. It is supposed that their subitems are also concerned,
 * but this may be caller-dependant.
 *
 * The caller may safely call na_object_free_items() on the returned
 * list, or g_list_free() if it wants keep the references somewhere.
 */
GList *
nact_iactions_list_get_selected_items( NactIActionsList *instance )
{
	GList *items = NULL;
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GList *it, *listrows;
	NAObject *object;
	GtkTreePath *path;

	g_return_val_if_fail( NACT_IS_IACTIONS_LIST( instance ), NULL );

	if( st_initialized && !st_finalized ){

		treeview = get_actions_list_treeview( instance );
		selection = gtk_tree_view_get_selection( treeview );
		listrows = gtk_tree_selection_get_selected_rows( selection, &model );

		for( it = listrows ; it ; it = it->next ){
			path = ( GtkTreePath * ) it->data;
			gtk_tree_model_get_iter( model, &iter, path );
			gtk_tree_model_get( model, &iter, IACTIONS_LIST_NAOBJECT_COLUMN, &object, -1 );
			items = g_list_prepend( items, object );
		}

		g_list_foreach( listrows, ( GFunc ) gtk_tree_path_free, NULL );
		g_list_free( listrows );

		items = g_list_reverse( items );
	}

	return( items );
}

/**
 * nact_iactions_list_has_modified_items:
 * @window: this #NactIActionsList instance.
 *
 * Returns: %TRUE if at least there is one modified item in the list.
 */
gboolean
nact_iactions_list_has_modified_items( NactIActionsList *instance )
{
	gboolean has_modified = FALSE;
	GtkTreeView *treeview;
	NactTreeModel *model;

	g_return_val_if_fail( NACT_IS_IACTIONS_LIST( instance ), FALSE );

	if( st_initialized && !st_finalized ){

		treeview = get_actions_list_treeview( instance );
		model = NACT_TREE_MODEL( gtk_tree_view_get_model( treeview ));

		nact_tree_model_iter( model, ( FnIterOnStore ) has_modified_iter, &has_modified );
	}

	return( has_modified );
}

/**
 * nact_iactions_list_insert_at_path:
 * @instance: this #NactIActionsList instance.
 * @items: a list of items to be inserted (e.g. from a paste).
 * @path: insertion path.
 *
 * Inserts the provided @items list in the treeview.
 *
 * This function takes care of repositionning a new selection if
 * possible, and refilter the display model.
 */
void
nact_iactions_list_insert_at_path( NactIActionsList *instance, GList *items, GtkTreePath *insert_path )
{
	static const gchar *thisfn = "nact_iactions_list_insert_at_path";
	GtkTreeView *treeview;
	GtkTreeModel *model;
	GList *parents;
	IActionsListInstanceData *ialid;

	g_debug( "%s: instance=%p, items=%p (%d items)",
			thisfn, ( void * ) instance, ( void * ) items, g_list_length( items ));
	g_return_if_fail( NACT_IS_IACTIONS_LIST( instance ));
	g_return_if_fail( NACT_IS_WINDOW( instance ));

	if( st_initialized && !st_finalized ){

		treeview = get_actions_list_treeview( instance );
		model = gtk_tree_view_get_model( treeview );
		g_return_if_fail( NACT_IS_TREE_MODEL( model ));

		do_insert_items( treeview, model, items, insert_path, &parents );

		update_parents_edition_status( parents, items );
		ialid = get_instance_data( instance );
		increment_counters( instance, ialid, items );

		gtk_tree_model_filter_refilter( GTK_TREE_MODEL_FILTER( model ));

		select_row_at_path( instance, treeview, model, insert_path );
	}
}

/**
 * nact_iactions_list_insert_items:
 * @instance: this #NactIActionsList instance.
 * @items: a list of items to be inserted (e.g. from a paste).
 * @sibling: the #NAObject besides which the insertion should occurs ;
 * this may prevent e.g. to go inside a menu.
 *
 * Inserts the provided @items list in the treeview.
 *
 * The provided @items list is supposed to be homogeneous, i.e. referes
 * to only profiles, or only actions or menus.
 *
 * If the @items list contains profiles, they can only be inserted
 * into an action, or before another profile.
 *
 * If new item is a #NAActionMenu or a #NAAction, it will be inserted
 * before the current action or menu.
 *
 * This function takes care of repositionning a new selection if
 * possible, and refilter the display model.
 */
void
nact_iactions_list_insert_items( NactIActionsList *instance, GList *items, NAObject *sibling )
{
	static const gchar *thisfn = "nact_iactions_list_insert_items";
	GtkTreeView *treeview;
	GtkTreeModel *model;
	GtkTreePath *insert_path;
	NAObject *object;

	g_debug( "%s: instance=%p, items=%p (%d items)",
			thisfn, ( void * ) instance, ( void * ) items, g_list_length( items ));
	g_return_if_fail( NACT_IS_IACTIONS_LIST( instance ));
	g_return_if_fail( NACT_IS_WINDOW( instance ));

	if( st_initialized && !st_finalized ){

		insert_path = NULL;
		treeview = get_actions_list_treeview( instance );
		model = gtk_tree_view_get_model( treeview );
		g_return_if_fail( NACT_IS_TREE_MODEL( model ));

		if( sibling ){
			insert_path = object_to_path( instance, NACT_TREE_MODEL( model ), sibling );

		} else {
			insert_path = get_selection_first_path( treeview );

			/* as a particular case, insert profiles into current action
			 */
			object = nact_tree_model_object_at_path( NACT_TREE_MODEL( model ), insert_path );
			/*g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));
			g_debug( "%s: items->data is %s", thisfn, G_OBJECT_TYPE_NAME( items->data ));*/
			if( NA_IS_OBJECT_ACTION( object ) &&
				NA_IS_OBJECT_PROFILE( items->data )){

				nact_iactions_list_insert_into( instance, items );
				gtk_tree_path_free( insert_path );
				return;
			}
		}

		nact_iactions_list_insert_at_path( instance, items, insert_path );
		gtk_tree_path_free( insert_path );
	}
}

/**
 * nact_iactions_list_insert_into:
 * @instance: this #NactIActionsList instance.
 * @items: a list of items to be inserted (e.g. from a paste).
 *
 * Inserts the provided @items list as first childs of the current item.
 *
 * The provided @items list is supposed to be homogeneous, i.e. referes
 * to only profiles, or only actions or menus.
 *
 * If the @items list contains only profiles, they can only be inserted
 * into an action, and the profiles will eventually be renumbered.
 *
 * If new item is a #NAActionMenu or a #NAAction, it will be inserted
 * as first childs of the current menu.
 *
 * This function takes care of repositionning a new selection as the
 * last inserted item, and refilter the display model.
 */
void
nact_iactions_list_insert_into( NactIActionsList *instance, GList *items )
{
	static const gchar *thisfn = "nact_iactions_list_insert_into";
	GtkTreeView *treeview;
	GtkTreeModel *model;
	GtkTreePath *insert_path;
	GtkTreePath *new_path;
	NAObject *parent;
	IActionsListInstanceData *ialid;

	g_debug( "%s: instance=%p, items=%p (%d items)",
			thisfn, ( void * ) instance, ( void * ) items, g_list_length( items ));
	g_return_if_fail( NACT_IS_IACTIONS_LIST( instance ));
	g_return_if_fail( NACT_IS_WINDOW( instance ));

	if( st_initialized && !st_finalized ){

		insert_path = NULL;
		treeview = get_actions_list_treeview( instance );
		model = gtk_tree_view_get_model( treeview );
		insert_path = get_selection_first_path( treeview );

		ialid = get_instance_data( instance );
		increment_counters( instance, ialid, items );

		parent = do_insert_into_first( treeview, model, items, insert_path, &new_path );

		na_object_check_edition_status( parent );

		gtk_tree_model_filter_refilter( GTK_TREE_MODEL_FILTER( model ));

		select_row_at_path( instance, treeview, model, new_path );

		gtk_tree_path_free( new_path );
		gtk_tree_path_free( insert_path );
	}
}

static GtkTreePath *
get_selection_first_path( GtkTreeView *treeview )
{
	GtkTreeSelection *selection;
	GList *list_selected;
	GtkTreePath *path;

	path = NULL;
	selection = gtk_tree_view_get_selection( treeview );
	list_selected = gtk_tree_selection_get_selected_rows( selection, NULL );

	if( g_list_length( list_selected )){
		path = gtk_tree_path_copy(( GtkTreePath * ) list_selected->data );

	} else {
		path = gtk_tree_path_new_from_string( "0" );
	}

	g_list_foreach( list_selected, ( GFunc ) gtk_tree_path_free, NULL );
	g_list_free( list_selected );

	return( path );
}

static void
do_insert_items( GtkTreeView *treeview, GtkTreeModel *model, GList *items, GtkTreePath *insert_path, GList **list_parents )
{
	/*static const gchar *thisfn = "nact_iactions_list_do_insert_items";*/
	GList *reversed;
	GList *it;
	GList *subitems;
	NAObject *obj_parent;

	obj_parent = NULL;
	if( list_parents ){
		*list_parents = NULL;
	}

	reversed = g_list_reverse( items );

	for( it = reversed ; it ; it = it->next ){

		nact_tree_model_insert( NACT_TREE_MODEL( model ), NA_OBJECT( it->data ), insert_path, &obj_parent );

		if( list_parents && obj_parent ){
			if( !g_list_find( *list_parents, obj_parent )){
				*list_parents = g_list_prepend( *list_parents, obj_parent );
			}
		}

		/* recursively insert subitems
		 */
		if( NA_IS_OBJECT_ITEM( it->data ) && na_object_get_items_count( it->data )){

			subitems = na_object_get_items( it->data );
			do_insert_into_first( treeview, model, subitems, insert_path, NULL );
			na_object_free_items( subitems );
		}
	}

	/*g_list_free( reversed );*/
}

static NAObject *
do_insert_into_first( GtkTreeView *treeview, GtkTreeModel *model, GList *items, GtkTreePath *insert_path, GtkTreePath **new_path )
{
	GList *last;
	NAObject *parent;
	GtkTreePath *inserted_path;

	parent = NULL;
	last = g_list_last( items );
	items = g_list_remove_link( items, last );

	inserted_path = do_insert_into_second( treeview, model, NA_OBJECT( last->data ), insert_path, &parent );
	do_insert_items( treeview, model, items, inserted_path, NULL );

	if( new_path ){
		*new_path = gtk_tree_path_copy( inserted_path );
	}

	gtk_tree_path_free( inserted_path );

	return( parent );
}

static GtkTreePath *
do_insert_into_second( GtkTreeView *treeview, GtkTreeModel *model, NAObject *object, GtkTreePath *insert_path, NAObject **parent )
{
	/*static const gchar *thisfn = "nact_iactions_list_do_insert_into";*/
	GtkTreePath *new_path;

	new_path = nact_tree_model_insert_into( NACT_TREE_MODEL( model ), object, insert_path, parent );

	gtk_tree_view_expand_to_path( treeview, new_path );

	return( new_path );
}

static void
increment_counters( NactIActionsList *instance, IActionsListInstanceData *ialid, GList *items )
{
	static const gchar *thisfn = "nact_iactions_list_increment_counters";
	gint menus, actions, profiles;

	g_debug( "%s: instance=%p, ialid=%p, items=%p",
			thisfn, ( void * ) instance, ( void * ) ialid, ( void * ) items );

	menus = 0;
	actions = 0;
	profiles = 0;
	na_object_item_count_items( items, &menus, &actions, &profiles, TRUE );
	/*g_debug( "increment_counters: counted: menus=%d, actions=%d, profiles=%d", menus, actions, profiles );*/

	/*g_debug( "incremente_counters: cs before: menus=%d, actions=%d, profiles=%d", cs->menus, cs->actions, cs->profiles );*/
	ialid->menus += menus;
	ialid->actions += actions;
	ialid->profiles += profiles;
	/*g_debug( "increment_counters: cs after: menus=%d, actions=%d, profiles=%d", cs->menus, cs->actions, cs->profiles );*/

	send_list_count_updated_signal( instance, ialid );
}

static void
update_parents_edition_status( GList *parents, GList *items )
{
	static const gchar *thisfn = "nact_iactions_list_update_parents_edition_status";
	GList *it;

	g_debug( "%s: parents=%p (count=%d), items=%p (count=%d)", thisfn,
			( void * ) parents, g_list_length( parents ),
			( void * ) items, g_list_length( items ));

	/*if( !parents || !g_list_length( parents )){
		parents = g_list_copy( items );
	}*/

	for( it = parents ; it ; it = it->next ){
		na_object_check_edition_status( it->data );
	}

	g_list_free( parents );
}

/**
 * nact_iactions_list_is_expanded:
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
	gboolean is_expanded = FALSE;
	GtkTreeIter iter;
	gboolean iterok, stop;
	NAObject *iter_object;

	g_return_val_if_fail( NACT_IS_IACTIONS_LIST( instance ), FALSE );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), FALSE );

	if( st_initialized && !st_finalized ){

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
	}

	return( is_expanded );
}

/**
 * nact_iactions_list_set_management_mode:
 * @instance: this #NactIActionsList instance.
 * @mode: management mode.
 *
 * Set the management mode for this @instance.
 *
 * For the two known modes (edition mode, export mode), we also allow
 * multiple selection in the list.
 */
void
nact_iactions_list_set_management_mode( NactIActionsList *instance, gint mode )
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	gboolean multiple;
	IActionsListInstanceData *ialid;

	g_return_if_fail( NACT_IS_IACTIONS_LIST( instance ));

	if( st_initialized && !st_finalized ){

		ialid = get_instance_data( instance );
		ialid->management_mode = mode;

		multiple = ( mode == IACTIONS_LIST_MANAGEMENT_MODE_EDITION ||
						mode == IACTIONS_LIST_MANAGEMENT_MODE_EXPORT );

		treeview = get_actions_list_treeview( instance );
		selection = gtk_tree_view_get_selection( treeview );
		gtk_tree_selection_set_mode( selection, multiple ? GTK_SELECTION_MULTIPLE : GTK_SELECTION_SINGLE );
	}
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

	if( st_initialized && !st_finalized ){

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
}

static gchar *
v_get_treeview_name( NactIActionsList *instance )
{
	gchar *name = NULL;

	if( st_initialized && !st_finalized ){
		if( NACT_IACTIONS_LIST_GET_INTERFACE( instance )->get_treeview_name ){
			name = NACT_IACTIONS_LIST_GET_INTERFACE( instance )->get_treeview_name( instance );
		}
	}

	return( name );
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

/*
 * rationale: it is very difficult to copy anything in the clipboard,
 * and to hope that this will be easily copyable anywhere after.
 * We know how to insert profiles, or how to insert actions or menus,
 * but not how nor where to insert e.g. a mix selection.
 * So the selection allows :
 * - only profiles, maybe from different actions
 * - or only actions or menus.
 *
 * Note that we do not allow here a selection to be made or not. What
 * we actually allow is only toggle the selection state. And so, we
 * only deal with "do we allow to toggle from non-selected ?"
 */
static gboolean
filter_selection( GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path, gboolean path_currently_selected, NactIActionsList *instance )
{
	SelectionIter *str;
	GtkTreeIter iter;
	NAObject *object;
	gboolean select_ok;

	if( path_currently_selected ){
		return( TRUE );
	}

	/* iter through the selection: does it contain profiles ? actions or
	 * menus ? or both ?
	 */
	str = g_new0( SelectionIter, 1 );
	str->nb_profiles = 0;
	str->nb_actions = 0;
	str->nb_menus = 0;

	gtk_tree_selection_selected_foreach( selection, ( GtkTreeSelectionForeachFunc ) filter_selection_iter, str );

	/* if there is not yet any selection, then anything is allowed
	 */
	if( str->nb_profiles + str->nb_actions + str->nb_menus == 0 ){
		return( TRUE );
	}

	gtk_tree_model_get_iter( model, &iter, path );
	gtk_tree_model_get( model, &iter, IACTIONS_LIST_NAOBJECT_COLUMN, &object, -1 );

	g_return_val_if_fail( object, FALSE );
	g_return_val_if_fail( NA_IS_OBJECT_ID( object ), FALSE );

	select_ok = FALSE;

	/* selecting a profile is only ok if a profile is already selected
	 */
	if( NA_IS_OBJECT_PROFILE( object )){
		select_ok = ( str->nb_actions + str->nb_menus == 0 );
	}

	/* selecting an action or a menu is only ok if no profile is selected
	 */
	if( NA_IS_OBJECT_ITEM( object )){
		select_ok = ( str->nb_profiles == 0 );
	}

	g_free( str );
	g_object_unref( object );

	return( select_ok );
}

static void
filter_selection_iter( GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, SelectionIter *str )
{
	NAObject *object;

	gtk_tree_model_get( model, iter, IACTIONS_LIST_NAOBJECT_COLUMN, &object, -1 );

	g_return_if_fail( object );
	g_return_if_fail( NA_IS_OBJECT_ID( object ));

	if( NA_IS_OBJECT_PROFILE( object )){
		str->nb_profiles += 1;

	} else if( NA_IS_OBJECT_ACTION( object )){
		str->nb_actions += 1;

	} else if( NA_IS_OBJECT_MENU( object )){
		str->nb_menus += 1;
	}

	g_object_unref( object );
}

static GtkTreeView *
get_actions_list_treeview( NactIActionsList *instance )
{
	gchar *widget_name;
	GtkTreeView *treeview = NULL;

	widget_name = v_get_treeview_name( instance );
	if( widget_name ){
		treeview = GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( instance ), widget_name ));
		g_return_val_if_fail( GTK_IS_TREE_VIEW( treeview ), NULL );
		g_free( widget_name );
	}

	return( treeview );
}

/*
 * search for an object, given its uuid
 */
static gboolean
get_item_iter( NactTreeModel *model, GtkTreePath *path, NAObject *object, IdToObjectIter *ito )
{
	gchar *id;
	gboolean found = FALSE;

	id = na_object_get_id( object );
	found = ( g_ascii_strcasecmp( id, ito->uuid ) == 0 );
	g_free( id );

	if( found ){
		ito->object = object;
	}

	/* stop iteration if found */
	return( found );
}

/*
 * builds the tree
 */
static gboolean
get_items_iter( NactTreeModel *model, GtkTreePath *path, NAObject *object, GList **items )
{
	if( gtk_tree_path_get_depth( path ) == 1 ){
		*items = g_list_prepend( *items, object );
	}

	/* don't stop iteration */
	return( FALSE );
}

/*
 * stop as soon as we have found a modified item
 */
static gboolean
has_modified_iter( NactTreeModel *model, GtkTreePath *path, NAObject *object, gboolean *has_modified )
{
	if( na_object_is_modified( object )){
		*has_modified = TRUE;
		return( TRUE );
	}

	/* don't stop iteration if not modified */
	return( FALSE );
}

static gboolean
have_dnd_mode( NactIActionsList *instance, IActionsListInstanceData *ialid )
{
	gboolean have_dnd;

	have_dnd = ( ialid->management_mode == IACTIONS_LIST_MANAGEMENT_MODE_EDITION );

	return( have_dnd );
}

static gboolean
have_filter_selection_mode( NactIActionsList *instance, IActionsListInstanceData *ialid )
{
	gboolean have_filter;

	have_filter = ( ialid->management_mode == IACTIONS_LIST_MANAGEMENT_MODE_EDITION );

	return( have_filter );
}

static gboolean
have_only_actions( NactIActionsList *instance, IActionsListInstanceData *ialid )
{
	gboolean only_actions;

	only_actions = ( ialid->management_mode == IACTIONS_LIST_MANAGEMENT_MODE_EXPORT );

	return( only_actions );
}

static gboolean
is_iduplicable_proxy( NactIActionsList *instance, IActionsListInstanceData *ialid )
{
	gboolean is_proxy;

	is_proxy = ( ialid->management_mode == IACTIONS_LIST_MANAGEMENT_MODE_EDITION );

	return( is_proxy );
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

static GtkTreePath *
object_to_path( NactIActionsList *instance, NactTreeModel *model, NAObject *object )
{
	ObjectToPathIter *otp;
	GtkTreePath *path = NULL;

	otp = g_new0( ObjectToPathIter, 1 );
	otp->object = object;
	otp->found = FALSE;
	otp->path = NULL;

	nact_tree_model_iter( model, ( FnIterOnStore ) object_to_path_iter, otp );

	if( otp->found ){
		path = gtk_tree_path_copy( otp->path );
		gtk_tree_path_free( otp->path );
	}

	g_free( otp );

	return( path );
}

static gboolean
object_to_path_iter( NactTreeModel *model, GtkTreePath *path, NAObject *object, ObjectToPathIter *otp )
{
	if( object == otp->object ){
		otp->found = TRUE;
		otp->path = gtk_tree_path_copy( path );
	}

	return( otp->found );
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

static void
on_edition_status_changed( NactIActionsList *instance, NAIDuplicable *object )
{
	GtkTreeView *treeview;
	NactTreeModel *model;
	IActionsListInstanceData *ialid;

	ialid = get_instance_data( instance );
	if( ialid->selection_changed_send_allowed ){

		g_debug( "nact_iactions_list_on_edition_status_changed: instance=%p, object=%p (%s)",
				( void * ) instance,
				( void * ) object, G_OBJECT_TYPE_NAME( object ));

		g_return_if_fail( NA_IS_OBJECT( object ));

		treeview = get_actions_list_treeview( instance );
		model = NACT_TREE_MODEL( gtk_tree_view_get_model( treeview ));
		nact_tree_model_display( model, NA_OBJECT( object ));
	}
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
 * it is inhibited while filling the list (usually only at runtime init)
 * and while deleting a selection
 */
static void
on_treeview_selection_changed( GtkTreeSelection *selection, NactIActionsList *instance )
{
	GList *selected_items;
	IActionsListInstanceData *ialid;

	ialid = get_instance_data( instance );
	if( ialid->selection_changed_send_allowed ){

		g_signal_handler_block( instance, ialid->tab_updated_handler );

		selected_items = nact_iactions_list_get_selected_items( instance );
		g_debug( "on_treeview_selection_changed: selection=%p (%d items)", ( void * ) selected_items, g_list_length( selected_items ));
		g_signal_emit_by_name( instance, IACTIONS_LIST_SIGNAL_SELECTION_CHANGED, selected_items );

		g_signal_handler_unblock( instance, ialid->tab_updated_handler );
	}

	/* selection list if free in cleanup handler for the signal */
}

/*
 * an item has been updated in one of the tabs
 * update the treeview to reflects its new edition status
 */
static void
on_tab_updatable_item_updated( NactIActionsList *instance, NAObject *object )
{
	static const gchar *thisfn = "nact_iactions_list_on_tab_updatable_item_updated";
	NAObject *item;
	GtkTreeView *treeview;
	GtkTreeModel *model;

	g_debug( "%s: instance=%p, object=%p (%s)", thisfn,
			( void * ) instance, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	if( object ){
		treeview = get_actions_list_treeview( instance );
		model = gtk_tree_view_get_model( treeview );

		item = object;
		if( NA_IS_OBJECT_PROFILE( object )){
			item = NA_OBJECT( na_object_profile_get_action( NA_OBJECT_PROFILE( object )));
		}

		na_object_check_edition_status( item );

		nact_tree_model_display( NACT_TREE_MODEL( model ), object );
		if( NA_IS_OBJECT_PROFILE( object )){
			nact_tree_model_display( NACT_TREE_MODEL( model ), NA_OBJECT( item ));
		}
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
	GtkTreeView *treeview;
	GtkTreeModel *model;
	GtkTreePath *path;

	treeview = get_actions_list_treeview( instance );
	model = gtk_tree_view_get_model( treeview );

	path = gtk_tree_path_new_from_string( "0" );
	select_row_at_path( instance, treeview, model, path );
	gtk_tree_path_free( path );
}

/*
 * select_row_at_path:
 * @window: this #NactIActionsList instance.
 * @path: a #GtkTreePath.
 *
 * Select the row at the required path, or the next following, or
 * the immediate previous.
 */
static void
select_row_at_path( NactIActionsList *instance, GtkTreeView *treeview, GtkTreeModel *model, GtkTreePath *path )
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	gboolean anything = FALSE;

	selection = gtk_tree_view_get_selection( treeview );
	gtk_tree_selection_unselect_all( selection );

	if( path ){
		g_debug( "nact_iactions_list_select_row_at_path: path=%s", gtk_tree_path_to_string( path ));

		if( gtk_tree_model_get_iter( model, &iter, path )){
			gtk_tree_view_set_cursor( treeview, path, NULL, FALSE );
			anything = TRUE;

		} else if( gtk_tree_path_prev( path ) && gtk_tree_model_get_iter( model, &iter, path )){
			gtk_tree_view_set_cursor( treeview, path, NULL, FALSE );
			anything = TRUE;

		} else {
			gtk_tree_path_next( path );
			if( gtk_tree_model_get_iter( model, &iter, path )){
				gtk_tree_view_set_cursor( treeview, path, NULL, FALSE );
				anything = TRUE;

			} else if( gtk_tree_path_get_depth( path ) > 1 &&
						gtk_tree_path_up( path ) &&
						gtk_tree_model_get_iter( model, &iter, path )){

							gtk_tree_view_set_cursor( treeview, path, NULL, FALSE );
							anything = TRUE;
			}
		}
	}

	/* if nothing can be selected, at least send a message with empty
	 *  selection
	 */
	if( !anything ){
		on_treeview_selection_changed( NULL, instance );
	}
}

/*
 * send a 'fill' signal with count of items
 */
static void
send_list_count_updated_signal( NactIActionsList *instance, IActionsListInstanceData *ialid )
{
	g_signal_emit_by_name( instance,
							IACTIONS_LIST_SIGNAL_LIST_COUNT_UPDATED,
							ialid->menus, ialid->actions, ialid->profiles );
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

			if(( count > 1 && NA_IS_OBJECT_ACTION( object )) ||
				( count > 0 && NA_IS_OBJECT_MENU( object ))){

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

static IActionsListInstanceData *
get_instance_data( NactIActionsList *instance )
{
	IActionsListInstanceData *ialid;

	ialid = ( IActionsListInstanceData * ) g_object_get_data( G_OBJECT( instance ), IACTIONS_LIST_DATA_INSTANCE );

	if( !ialid ){
		ialid = g_new0( IActionsListInstanceData, 1 );
		g_object_set_data( G_OBJECT( instance ), IACTIONS_LIST_DATA_INSTANCE, ialid );
	}

	return( ialid );
}
