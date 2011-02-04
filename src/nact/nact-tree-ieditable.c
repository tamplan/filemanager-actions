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

#include "nact-tree-ieditable.h"

/* private interface data
 */
struct _NactTreeIEditableInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* data attached to the NactTreeView
 */
typedef struct {
	guint nb_modified;
}
	IEditableData;

#define VIEW_DATA_IEDITABLE				"view-data-ieditable"

static gboolean st_tree_ieditable_initialized = FALSE;
static gboolean st_tree_ieditable_finalized   = FALSE;

static GType          register_type( void );
static void           interface_base_init( NactTreeIEditableInterface *klass );
static void           interface_base_finalize( NactTreeIEditableInterface *klass );

static IEditableData *get_instance_data( NactTreeIEditable *view );

GType
nact_tree_ieditable_get_type( void )
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
	static const gchar *thisfn = "nact_tree_ieditable_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NactTreeIEditableInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "NactTreeIEditable", &info, 0 );

	return( type );
}

static void
interface_base_init( NactTreeIEditableInterface *klass )
{
	static const gchar *thisfn = "nact_tree_ieditable_interface_base_init";

	if( !st_tree_ieditable_initialized ){
		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NactTreeIEditableInterfacePrivate, 1 );

		st_tree_ieditable_initialized = TRUE;
	}
}

static void
interface_base_finalize( NactTreeIEditableInterface *klass )
{
	static const gchar *thisfn = "nact_tree_ieditable_interface_base_finalize";

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){
		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		st_tree_ieditable_finalized = TRUE;

		g_free( klass->private );
	}
}

/**
 * nact_tree_ieditable_view_constructed:
 * @view: the #NactTreeView which implements this interface.
 * @window: the #BaseWindow which embeds the @view.
 *
 * Initialize the interface, mainly connecting to signals of interest.
 */
void
nact_tree_ieditable_on_view_constructed( NactTreeView *view, BaseWindow *window )
{
	static const gchar *thisfn = "nact_tree_ieditable_on_view_constructed";
	IEditableData *ied;

	g_debug( "%s: view=%p, window=%p", thisfn, ( void * ) view, ( void * ) window );

	ied = get_instance_data( NACT_TREE_IEDITABLE( view ));
}

static IEditableData *
get_instance_data( NactTreeIEditable *view )
{
	IEditableData *ied;

	ied = ( IEditableData * ) g_object_get_data( G_OBJECT( view ), VIEW_DATA_IEDITABLE );

	if( !ied ){
		ied = g_new0( IEditableData, 1 );
		g_object_set_data( G_OBJECT( view ), VIEW_DATA_IEDITABLE, ied );
	}

	return( ied );
}

#if 0
/* when iterating through a selection
 */
typedef struct {
	gboolean has_menu_or_action;
}
	SelectionIter;

/* signals
 */
enum {
	LIST_COUNT_UPDATED,
	SELECTION_CHANGED,
	FOCUS_IN,
	FOCUS_OUT,
	COLUMN_EDITED,
	STATUS_CHANGED,
	LAST_SIGNAL
};

static gint     st_signals[ LAST_SIGNAL ] = { 0 };

static void     free_items_callback( NactTreeIEditable *instance, GList *items );
static void     free_column_edited_callback( NactTreeIEditable *instance, NAObject *object, gchar *text, gint column );

static gboolean are_profiles_displayed( NactTreeIEditable *instance, TreeIEditableInstanceData *ialid );
static void     display_label( GtkTreeViewColumn *column, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, NactTreeIEditable *instance );
static gboolean filter_selection( GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path, gboolean path_currently_selected, NactTreeIEditable *instance );
static gboolean filter_selection_is_homogeneous( GtkTreeSelection *selection, NAObject *object );
static void     filter_selection_iter( GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, SelectionIter *str );
static gboolean filter_selection_has_menu_or_action( GtkTreeSelection *selection );
static gboolean filter_selection_is_implicitely_selected( NAObject *object );
static void     filter_selection_set_implicitely_selected_childs( NAObject *object, gboolean select );
static gboolean have_dnd_mode( NactTreeIEditable *instance, TreeIEditableInstanceData *ialid );
static gboolean have_filter_selection_mode( NactTreeIEditable *instance, TreeIEditableInstanceData *ialid );
static void     inline_edition( NactTreeIEditable *instance );
static gboolean is_iduplicable_proxy( NactTreeIEditable *instance, TreeIEditableInstanceData *ialid );
static gboolean on_button_press_event( GtkWidget *widget, GdkEventButton *event, NactTreeIEditable *instance );
static void     on_edition_status_changed( NactTreeIEditable *instance, NAIDuplicable *object );
static gboolean on_focus_in( GtkWidget *widget, GdkEventFocus *event, NactTreeIEditable *instance );
static gboolean on_focus_out( GtkWidget *widget, GdkEventFocus *event, NactTreeIEditable *instance );
static gboolean on_key_pressed_event( GtkWidget *widget, GdkEventKey *event, NactTreeIEditable *instance );
static void     on_label_edited( GtkCellRendererText *renderer, const gchar *path, const gchar *text, NactTreeIEditable *instance );
static void     on_tab_updatable_item_updated( NactTreeIEditable *instance, NAObject *object, gboolean force_display );
static void     open_popup( NactTreeIEditable *instance, GdkEventButton *event );

static void
free_items_callback( NactTreeIEditable *instance, GList *items )
{
	g_debug( "nact_tree_ieditable_free_items_callback: selection=%p (%d items)",
			( void * ) items, g_list_length( items ));

	na_object_free_items( items );
}

static void
free_column_edited_callback( NactTreeIEditable *instance, NAObject *object, gchar *text, gint column )
{
	static const gchar *thisfn = "nact_tree_ieditable_free_column_edited_callback";

	g_debug( "%s: instance=%p, object=%p (%s), text=%s, column=%d",
			thisfn, ( void * ) instance, ( void * ) object, G_OBJECT_TYPE_NAME( object ), text, column );

	g_free( text );
}

/**
 * nact_tree_ieditable_initial_load_toplevel:
 * @instance: this #NactTreeIEditable *instance.
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
nact_tree_ieditable_initial_load_toplevel( NactTreeIEditable *instance )
{
	static const gchar *thisfn = "nact_tree_ieditable_initial_load_toplevel";
	GtkWidget *label;
	GtkTreeView *treeview;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	TreeIEditableInstanceData *ialid;

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_TREE_IEDITABLE( instance ));

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){

		treeview = nact_tree_ieditable_priv_get_actions_list_treeview( instance );
		ialid = nact_tree_ieditable_priv_get_instance_data( instance );
		ialid->selection_changed_allowed = FALSE;

		/* associates the ActionsList to the label */
		label = base_window_get_widget( BASE_WINDOW( instance ), "ActionsListLabel" );
		gtk_label_set_mnemonic_widget( GTK_LABEL( label ), GTK_WIDGET( treeview ));

		nact_tree_model_initial_load( BASE_WINDOW( instance ), treeview );
		gtk_tree_view_set_enable_tree_lines( treeview, TRUE );

		/* create visible columns on the tree view
		 */
		/* icon: no header */
		column = gtk_tree_view_column_new_with_attributes(
				"icon",
				gtk_cell_renderer_pixbuf_new(),
				"pixbuf", TREE_IEDITABLE_ICON_COLUMN,
				NULL );
		gtk_tree_view_append_column( treeview, column );

		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(
				"label",
				renderer,
				"text", TREE_IEDITABLE_LABEL_COLUMN,
				NULL );
		gtk_tree_view_column_set_sort_column_id( column, TREE_IEDITABLE_LABEL_COLUMN );
		gtk_tree_view_column_set_cell_data_func(
				column, renderer, ( GtkTreeCellDataFunc ) display_label, instance, NULL );
		gtk_tree_view_append_column( treeview, column );
	}
}

/**
 * nact_tree_ieditable_runtime_init_toplevel:
 * @window: this #NactTreeIEditable *instance.
 * @items: list of #NAObject actions and menus as provided by #NAPivot.
 *
 * Allocates and initializes the ActionsList widget.
 */
void
nact_tree_ieditable_runtime_init_toplevel( NactTreeIEditable *instance, GList *items )
{
	static const gchar *thisfn = "nact_tree_ieditable_runtime_init_toplevel";
	GtkTreeView *treeview;
	NactTreeModel *model;
	gboolean have_dnd;
	gboolean have_filter_selection;
	gboolean is_proxy;
	GtkTreeSelection *selection;
	TreeIEditableInstanceData *ialid;
	GtkTreeViewColumn *column;
	GList *renderers;

	g_debug( "%s: instance=%p, items=%p (%d items)",
			thisfn, ( void * ) instance, ( void * ) items, g_list_length( items ));
	g_return_if_fail( NACT_IS_TREE_IEDITABLE( instance ));

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){

		treeview = nact_tree_ieditable_priv_get_actions_list_treeview( instance );
		model = NACT_TREE_MODEL( gtk_tree_view_get_model( treeview ));

		ialid = nact_tree_ieditable_priv_get_instance_data( instance );
		ialid->selection_changed_allowed = FALSE;
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
				G_CALLBACK( nact_tree_ieditable_on_treeview_selection_changed ));

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

		/* updates the treeview display when an item is modified */
		ialid->tab_updated_handler = base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( instance ),
				TAB_UPDATABLE_SIGNAL_ITEM_UPDATED,
				G_CALLBACK( on_tab_updatable_item_updated ));

		/* enable/disable edit menu item accelerators depending of
		 * which widget has the focus */
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( treeview ),
				"focus-in-event",
				G_CALLBACK( on_focus_in ));

		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( treeview ),
				"focus-out-event",
				G_CALLBACK( on_focus_out ));

		/* label edition: inform the corresponding tab */
		column = gtk_tree_view_get_column( treeview, TREE_IEDITABLE_LABEL_COLUMN );
		renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( column ));
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( renderers->data ),
				"edited",
				G_CALLBACK( on_label_edited ));

		/* records NactTreeIEditable as a proxy for edition status
		 * modification */
		is_proxy = is_iduplicable_proxy( instance, ialid );
		if( is_proxy ){
			na_iduplicable_register_consumer( G_OBJECT( instance ));

			base_window_signal_connect(
					BASE_WINDOW( instance ),
					G_OBJECT( instance ),
					NA_IDUPLICABLE_SIGNAL_STATUS_CHANGED,
					G_CALLBACK( on_edition_status_changed ));
		}

		/* fill the model after having connected the signals
		 * so that callbacks are triggered at last
		 */
		nact_tree_ieditable_fill( instance, items );

		/* force the treeview to have the focus at start
		 */
		gtk_widget_grab_focus( GTK_WIDGET( treeview ));
	}
}

/**
 * nact_tree_ieditable_all_widgets_showed:
 * @window: this #NactTreeIEditable *instance.
 */
void
nact_tree_ieditable_all_widgets_showed( NactTreeIEditable *instance )
{
	static const gchar *thisfn = "nact_tree_ieditable_all_widgets_showed";
	gboolean profiles_are_displayed;
	TreeIEditableInstanceData *ialid;

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_TREE_IEDITABLE( instance ));

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){

		ialid = nact_tree_ieditable_priv_get_instance_data( instance );
		profiles_are_displayed = are_profiles_displayed( instance, ialid );

		if( profiles_are_displayed ){
			nact_tree_ieditable_priv_send_list_count_updated_signal( instance, ialid );
		}

		nact_tree_ieditable_bis_select_first_row( instance );
	}
}

/**
 * nact_tree_ieditable_dispose:
 * @window: this #NactTreeIEditable *instance.
 */
void
nact_tree_ieditable_dispose( NactTreeIEditable *instance )
{
	static const gchar *thisfn = "nact_tree_ieditable_dispose";
	GtkTreeView *treeview;
	NactTreeModel *model;
	TreeIEditableInstanceData *ialid;

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_TREE_IEDITABLE( instance ));

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){

		treeview = nact_tree_ieditable_priv_get_actions_list_treeview( instance );
		model = NACT_TREE_MODEL( gtk_tree_view_get_model( treeview ));

		ialid = nact_tree_ieditable_priv_get_instance_data( instance );
		g_list_free( ialid->modified_items );
		ialid->modified_items = NULL;

		ialid->selection_changed_allowed = FALSE;
		nact_tree_model_dispose( model );

		g_free( ialid );
	}
}

/**
 * nact_tree_ieditable_brief_tree_dump:
 * @instance: this #NactTreeIEditable implementation.
 *
 * Brief dump of the tree store content.
 */
void
nact_tree_ieditable_brief_tree_dump( NactTreeIEditable *instance )
{
	GtkTreeView *treeview;
	NactTreeModel *model;

	g_return_if_fail( NACT_IS_TREE_IEDITABLE( instance ));

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){

		treeview = nact_tree_ieditable_priv_get_actions_list_treeview( instance );
		model = NACT_TREE_MODEL( gtk_tree_view_get_model( treeview ));
		nact_tree_model_dump( model );
	}
}

/**
 * nact_tree_ieditable_collapse_all:
 * @instance: this #NactTreeIEditable implementation.
 *
 * Collapse all the tree hierarchy.
 */
void
nact_tree_ieditable_collapse_all( NactTreeIEditable *instance )
{
	static const gchar *thisfn = "nact_tree_ieditable_collapse_all";
	GtkTreeView *treeview;

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_TREE_IEDITABLE( instance ));

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){

		treeview = nact_tree_ieditable_priv_get_actions_list_treeview( instance );
		gtk_tree_view_collapse_all( treeview );
	}
}

/**
 * nact_tree_ieditable_display_order_change:
 * @instance: this #NactTreeIEditable implementation.
 * @order_mode: the new order mode.
 *
 * Setup the new order mode.
 */
void
nact_tree_ieditable_display_order_change( NactTreeIEditable *instance, gint order_mode )
{
	GtkTreeView *treeview;
	NactTreeModel *model;

	g_return_if_fail( NACT_IS_TREE_IEDITABLE( instance ));

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){

		treeview = nact_tree_ieditable_priv_get_actions_list_treeview( instance );
		model = NACT_TREE_MODEL( gtk_tree_view_get_model( treeview ));
		nact_tree_model_display_order_change( model, order_mode );
	}
}

/**
 * nact_tree_ieditable_expand_all:
 * @instance: this #NactTreeIEditable implementation.
 *
 * Expand all the tree hierarchy.
 */
void
nact_tree_ieditable_expand_all( NactTreeIEditable *instance )
{
	static const gchar *thisfn = "nact_tree_ieditable_expand_all";
	GtkTreeView *treeview;

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_TREE_IEDITABLE( instance ));

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){

		treeview = nact_tree_ieditable_priv_get_actions_list_treeview( instance );
		gtk_tree_view_expand_all( treeview );
	}
}

/**
 * nact_tree_ieditable_fill:
 * @instance: this #NactTreeIEditable instance.
 *
 * Fill the listbox with the provided list of items.
 *
 * Menus are expanded, profiles are not.
 * The selection is reset to the first line of the tree, if there is one.
 */
void
nact_tree_ieditable_fill( NactTreeIEditable *instance, GList *items )
{
	static const gchar *thisfn = "nact_tree_ieditable_fill";
	GtkTreeView *treeview;
	NactTreeModel *model;
	gboolean profiles_are_displayed;
	TreeIEditableInstanceData *ialid;

	g_debug( "%s: instance=%p, items=%p", thisfn, ( void * ) instance, ( void * ) items );
	g_return_if_fail( NACT_IS_TREE_IEDITABLE( instance ));

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){

		treeview = nact_tree_ieditable_priv_get_actions_list_treeview( instance );
		model = NACT_TREE_MODEL( gtk_tree_view_get_model( treeview ));

		nact_tree_ieditable_bis_clear_selection( instance, treeview );

		ialid = nact_tree_ieditable_priv_get_instance_data( instance );
		profiles_are_displayed = are_profiles_displayed( instance, ialid );
		g_debug( "%s: profiles_are_displayed:%s", thisfn, profiles_are_displayed ? "True":"False" );

		ialid->selection_changed_allowed = FALSE;
		nact_tree_model_fill( model, items, profiles_are_displayed );
		ialid->selection_changed_allowed = TRUE;

		g_list_free( ialid->modified_items );
		ialid->modified_items = NULL;

		g_signal_emit_by_name(
				instance,
				MAIN_WINDOW_SIGNAL_LEVEL_ZERO_ORDER_CHANGED,
				GINT_TO_POINTER( FALSE ));

		ialid->menus = 0;
		ialid->actions = 0;
		ialid->profiles = 0;

		if( profiles_are_displayed ){
			na_object_item_count_items( items, &ialid->menus, &ialid->actions, &ialid->profiles, TRUE );
			nact_tree_ieditable_priv_send_list_count_updated_signal( instance, ialid );
		}
	}
}

/**
 * nact_tree_ieditable_get_management_mode:
 * @instance: this #NactTreeIEditable instance.
 *
 * Returns: the current management mode of the list.
 */
gint
nact_tree_ieditable_get_management_mode( NactTreeIEditable *instance )
{
	gint mode = 0;
	TreeIEditableInstanceData *ialid;

	g_return_val_if_fail( NACT_IS_TREE_IEDITABLE( instance ), 0 );

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){

		ialid = nact_tree_ieditable_priv_get_instance_data( instance );
		mode = ialid->management_mode;
	}

	return( mode );
}

/**
 * nact_tree_ieditable_has_modified_items:
 * @window: this #NactTreeIEditable instance.
 *
 * Returns: %TRUE if at least there is one modified item in the list.
 */
gboolean
nact_tree_ieditable_has_modified_items( NactTreeIEditable *instance )
{
	gboolean has_modified = FALSE;
	/*GtkTreeView *treeview;
	NactTreeModel *model;*/
	TreeIEditableInstanceData *ialid;

	g_return_val_if_fail( NACT_IS_TREE_IEDITABLE( instance ), FALSE );

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){

		ialid = nact_tree_ieditable_priv_get_instance_data( instance );
		has_modified = ( g_list_length( ialid->modified_items ) > 0 );

		/*treeview = get_actions_list_treeview( instance );
		model = NACT_TREE_MODEL( gtk_tree_view_get_model( treeview ));
		nact_tree_model_iter( model, ( FnIterOnStore ) has_modified_iter, &has_modified );*/
	}

	return( has_modified );
}

/**
 * nact_tree_ieditable_on_treeview_selection_changed:
 * @selection: current selection.
 * @instance: this instance of the #NactTreeIEditable interface.
 *
 * This is our handler for "changed" signal emitted by the treeview.
 * The handler is inhibited while filling the list (usually only at
 * runtime init), and while deleting a selection.
 */
void
nact_tree_ieditable_on_treeview_selection_changed( GtkTreeSelection *selection, NactTreeIEditable *instance )
{
	GList *selected_items;
	TreeIEditableInstanceData *ialid;

	ialid = nact_tree_ieditable_priv_get_instance_data( instance );
	if( ialid->selection_changed_allowed ){

		g_debug( "nact_tree_ieditable_on_treeview_selection_changed" );
		g_signal_handler_block( instance, ialid->tab_updated_handler );

		selected_items = nact_tree_ieditable_bis_get_selected_items( instance );
		g_debug( "nact_tree_ieditable_on_treeview_selection_changed: selection=%p (%d items)", ( void * ) selected_items, g_list_length( selected_items ));
		g_signal_emit_by_name( instance, TREE_IEDITABLE_SIGNAL_SELECTION_CHANGED, selected_items );

		g_signal_handler_unblock( instance, ialid->tab_updated_handler );

		/* selection list is freed in cleanup handler for the signal */
	}
}

/**
 * nact_tree_ieditable_remove_rec:
 * @list: list of modified objects.
 * @object: the object to be removed from the list.
 *
 * When removing from modified list an object which is no more modified,
 * then all subitems of the object have also to be removed
 *
 * Returns: the updated list.
 */
GList *
nact_tree_ieditable_remove_rec( GList *list, NAObject *object )
{
	GList *subitems, *it;

	if( NA_IS_OBJECT_ITEM( object )){
		subitems = na_object_get_items( object );
		for( it = subitems ; it ; it = it->next ){
			list = nact_tree_ieditable_remove_rec( list, it->data );
		}
	}

	list = g_list_remove( list, object );

	return( list );
}

/**
 * nact_tree_ieditable_set_management_mode:
 * @instance: this #NactTreeIEditable instance.
 * @mode: management mode.
 *
 * Set the management mode for this @instance.
 *
 * For the two known modes (edition mode, export mode), we also allow
 * multiple selection in the list.
 */
void
nact_tree_ieditable_set_management_mode( NactTreeIEditable *instance, gint mode )
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	gboolean multiple;
	TreeIEditableInstanceData *ialid;

	g_return_if_fail( NACT_IS_TREE_IEDITABLE( instance ));

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){

		ialid = nact_tree_ieditable_priv_get_instance_data( instance );
		ialid->management_mode = mode;

		multiple = ( mode == TREE_IEDITABLE_MANAGEMENT_MODE_EDITION ||
						mode == TREE_IEDITABLE_MANAGEMENT_MODE_EXPORT );

		treeview = nact_tree_ieditable_priv_get_actions_list_treeview( instance );
		selection = gtk_tree_view_get_selection( treeview );
		gtk_tree_selection_set_mode( selection, multiple ? GTK_SELECTION_MULTIPLE : GTK_SELECTION_SINGLE );
	}
}

static gboolean
are_profiles_displayed( NactTreeIEditable *instance, TreeIEditableInstanceData *ialid )
{
	gboolean display;

	display = ( ialid->management_mode == TREE_IEDITABLE_MANAGEMENT_MODE_EDITION );

	return( display );
}

/*
 * item modified: italic
 * item not saveable (invalid): red
 */
static void
display_label( GtkTreeViewColumn *column, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, NactTreeIEditable *instance )
{
	NAObject *object;
	gchar *label;
	gboolean modified = FALSE;
	gboolean valid = TRUE;
	TreeIEditableInstanceData *ialid;
	NAObjectItem *item;
#if 0
	gboolean writable_item;
	NactApplication *application;
	NAUpdater *updater;
#endif

	gtk_tree_model_get( model, iter, TREE_IEDITABLE_NAOBJECT_COLUMN, &object, -1 );
	g_object_unref( object );
	g_return_if_fail( NA_IS_OBJECT( object ));

	ialid = nact_tree_ieditable_priv_get_instance_data( instance );
	label = na_object_get_label( object );
	g_object_set( cell, "style-set", FALSE, NULL );
	g_object_set( cell, "foreground-set", FALSE, NULL );
	/*g_debug( "nact_tree_ieditable_display_label: %s %s", G_OBJECT_TYPE_NAME( object ), label );*/

	if( ialid->management_mode == TREE_IEDITABLE_MANAGEMENT_MODE_EDITION ){

		modified = na_object_is_modified( object );
		valid = na_object_is_valid( object );
		item = NA_IS_OBJECT_PROFILE( object ) ? na_object_get_parent( object ) : NA_OBJECT_ITEM( object );

		if( modified ){
			g_object_set( cell, "style", PANGO_STYLE_ITALIC, "style-set", TRUE, NULL );
		}

		if( !valid ){
			g_object_set( cell, "foreground", "Red", "foreground-set", TRUE, NULL );
		}

#if 0
		application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( instance )));
		updater = nact_application_get_updater( application );
		writable_item = na_updater_is_item_writable( updater, item, NULL );
		g_object_set( cell, "editable", writable_item, NULL );
#endif
	}

	g_object_set( cell, "text", label, NULL );
	g_free( label );
}

/*
 * rationale: it is very difficult to copy anything in the clipboard,
 * and to hope that this will be easily copyable anywhere after.
 * We know how to insert profiles, or how to insert actions or menus,
 * but not how nor where to insert e.g. a mix selection.
 *
 * So a selection must first be homogeneous, i.e. it only contains
 * explicitely selected profiles _or_ menus or actions (and their childs).
 *
 * To simplify the selection management while letting the user actually
 * select almost anything, we are doing following assumptions:
 * - when the user selects one row, all childs are also automatically
 *   selected ; visible childs are setup so that they are known as
 *   'indirectly' selected
 * - when a row is set as 'indirectly' selected, user cannot select
 *   nor unselect it (sort of readonly or mandatory implied selection)
 *   while the parent stays itself selected
 */
static gboolean
filter_selection( GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path, gboolean path_currently_selected, NactTreeIEditable *instance )
{
	static const gchar *thisfn = "nact_tree_ieditable_filter_selection";
	GList *selected_paths;
	GtkTreeIter iter;
	NAObject *object;

	gtk_tree_model_get_iter( model, &iter, path );
	gtk_tree_model_get( model, &iter, TREE_IEDITABLE_NAOBJECT_COLUMN, &object, -1 );
	g_return_val_if_fail( object, FALSE );
	g_return_val_if_fail( NA_IS_OBJECT_ID( object ), FALSE );
	g_object_unref( object );

	/* if there is not yet any selection, then anything is allowed
	 */
	selected_paths = gtk_tree_selection_get_selected_rows( selection, NULL );
	if( !selected_paths || !g_list_length( selected_paths )){
		/*g_debug( "%s: current selection is empty: allowing this one", thisfn );*/
		filter_selection_set_implicitely_selected_childs( object, !path_currently_selected );
		return( TRUE );
	}

	/* if the object at the row is already 'implicitely' selected, i.e.
	 * selected because of the selection of one of its parents, then
	 * nothing is allowed
	 */
	if( filter_selection_is_implicitely_selected( object )){
		g_debug( "%s: implicitely selected item: selection not allowed", thisfn );
		return( FALSE );
	}

	/* object at the row is not 'implicitely' selected: we may so select
	 * or unselect it while the selection stays homogeneous
	 * (rather we set its childs to the corresponding implied status)
	 */
	if( path_currently_selected ||
		filter_selection_is_homogeneous( selection, object )){

			filter_selection_set_implicitely_selected_childs( object, !path_currently_selected );
	}
	return( TRUE );
}

/*
 * does the selection stay homogeneous when adding this object ?
 */
static gboolean
filter_selection_is_homogeneous( GtkTreeSelection *selection, NAObject *object )
{
	gboolean homogeneous;

	if( filter_selection_has_menu_or_action( selection )){
		homogeneous = !NA_IS_OBJECT_PROFILE( object );
	} else {
		homogeneous = NA_IS_OBJECT_PROFILE( object );
	}

	return( homogeneous );
}

static gboolean
filter_selection_has_menu_or_action( GtkTreeSelection *selection )
{
	gboolean has_menu_or_action;
	SelectionIter *str;

	has_menu_or_action = FALSE;
	str = g_new0( SelectionIter, 1 );
	str->has_menu_or_action = has_menu_or_action;
	gtk_tree_selection_selected_foreach( selection, ( GtkTreeSelectionForeachFunc ) filter_selection_iter, str );
	has_menu_or_action = str->has_menu_or_action;
	g_free( str );

	return( has_menu_or_action );
}

static void
filter_selection_iter( GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, SelectionIter *str )
{
	NAObject *object;

	gtk_tree_model_get( model, iter, TREE_IEDITABLE_NAOBJECT_COLUMN, &object, -1 );

	if( NA_IS_OBJECT_ITEM( object )){
		str->has_menu_or_action = TRUE;
	}

	g_object_unref( object );
}

static gboolean
filter_selection_is_implicitely_selected( NAObject *object )
{
	gboolean selected;

	selected = ( gboolean ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( object), "nact-implicit-selection" ));

	return( selected );
}

/*
 * the object is being selected (resp. unselected)
 * recursively set the 'implicit selection' flag for all its childs
 */
static void
filter_selection_set_implicitely_selected_childs( NAObject *object, gboolean select )
{
	GList *childs, *ic;

	if( NA_IS_OBJECT_ITEM( object )){
		childs = na_object_get_items( object );
		for( ic = childs ; ic ; ic = ic->next ){
			g_object_set_data( G_OBJECT( ic->data ), "nact-implicit-selection", GUINT_TO_POINTER(( guint ) select ));
			filter_selection_set_implicitely_selected_childs( NA_OBJECT( ic->data ), select );
		}
	}
}

static gboolean
have_dnd_mode( NactTreeIEditable *instance, TreeIEditableInstanceData *ialid )
{
	gboolean have_dnd;

	have_dnd = ( ialid->management_mode == TREE_IEDITABLE_MANAGEMENT_MODE_EDITION );

	return( have_dnd );
}

static gboolean
have_filter_selection_mode( NactTreeIEditable *instance, TreeIEditableInstanceData *ialid )
{
	gboolean have_filter;

	have_filter = ( ialid->management_mode == TREE_IEDITABLE_MANAGEMENT_MODE_EDITION );

	return( have_filter );
}

/*
 * triggered by 'F2' key
 * only in edition mode
 */
static void
inline_edition( NactTreeIEditable *instance )
{
	static const gchar *thisfn = "nact_tree_ieditable_inline_edition";
	TreeIEditableInstanceData *ialid;
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GList *listrows;
	GtkTreePath *path;
	GtkTreeViewColumn *column;

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_TREE_IEDITABLE( instance ));

	ialid = nact_tree_ieditable_priv_get_instance_data( instance );
	if( ialid->management_mode == TREE_IEDITABLE_MANAGEMENT_MODE_EDITION ){

		ialid->selection_changed_allowed = FALSE;

		treeview = nact_tree_ieditable_priv_get_actions_list_treeview( instance );
		selection = gtk_tree_view_get_selection( treeview );
		listrows = gtk_tree_selection_get_selected_rows( selection, NULL );

		if( g_list_length( listrows ) == 1 ){
			path = ( GtkTreePath * ) listrows->data;
			column = gtk_tree_view_get_column( treeview, TREE_IEDITABLE_LABEL_COLUMN );
			gtk_tree_view_set_cursor( treeview, path, column, TRUE );
		}

		g_list_foreach( listrows, ( GFunc ) gtk_tree_path_free, NULL );
		g_list_free( listrows );

		ialid->selection_changed_allowed = TRUE;
	}
}

static gboolean
is_iduplicable_proxy( NactTreeIEditable *instance, TreeIEditableInstanceData *ialid )
{
	gboolean is_proxy;

	is_proxy = ( ialid->management_mode == TREE_IEDITABLE_MANAGEMENT_MODE_EDITION );
	g_debug( "nact_tree_ieditable_is_iduplicable_proxy: is_proxy=%s", is_proxy ? "True":"False" );

	return( is_proxy );
}

static gboolean
on_button_press_event( GtkWidget *widget, GdkEventButton *event, NactTreeIEditable *instance )
{
	/*static const gchar *thisfn = "nact_tree_ieditable_v_on_button_pres_event";
	g_debug( "%s: widget=%p, event=%p, user_data=%p", thisfn, widget, event, user_data );*/

	gboolean stop = FALSE;

	/* double-click of left button */
	if( event->type == GDK_2BUTTON_PRESS && event->button == 1 ){
		nact_tree_ieditable_bis_toggle_collapse( instance );
		stop = TRUE;
	}

	/* single click on right button */
	if( event->type == GDK_BUTTON_PRESS && event->button == 3 ){
		open_popup( instance, event );
		stop = TRUE;
	}

	return( stop );
}

static void
on_edition_status_changed( NactTreeIEditable *instance, NAIDuplicable *object )
{
	GtkTreeView *treeview;
	NactTreeModel *model;
	TreeIEditableInstanceData *ialid;

	ialid = nact_tree_ieditable_priv_get_instance_data( instance );

	g_debug( "nact_tree_ieditable_on_edition_status_changed: instance=%p, object=%p (%s)",
			( void * ) instance,
			( void * ) object, G_OBJECT_TYPE_NAME( object ));

	g_return_if_fail( NA_IS_OBJECT( object ));

	treeview = nact_tree_ieditable_priv_get_actions_list_treeview( instance );
	model = NACT_TREE_MODEL( gtk_tree_view_get_model( treeview ));
	nact_tree_model_display( model, NA_OBJECT( object ));

	if( na_object_is_modified( object )){
		if( !g_list_find( ialid->modified_items, object )){
			ialid->modified_items = g_list_prepend( ialid->modified_items, object );
		}
	} else {
		ialid->modified_items = nact_tree_ieditable_remove_rec( ialid->modified_items, NA_OBJECT( object ));
	}

	/* do not send status-changed signal while filling the tree
	 */
	if( ialid->selection_changed_allowed ){
		g_signal_emit_by_name( instance, TREE_IEDITABLE_SIGNAL_STATUS_CHANGED, NULL );
	}
}

/*
 * focus is monitored to avoid an accelerator being pressed while on a tab
 * triggers an unwaited operation on the list
 * e.g. when editing an entry field on the tab, pressing Del should _not_
 * delete current row in the list !
 */
static gboolean
on_focus_in( GtkWidget *widget, GdkEventFocus *event, NactTreeIEditable *instance )
{
	/*static const gchar *thisfn = "nact_tree_ieditable_on_focus_in";*/
	gboolean stop = FALSE;

	/*g_debug( "%s: widget=%p, event=%p, instance=%p", thisfn, ( void * ) widget, ( void * ) event, ( void * ) instance );*/
	g_signal_emit_by_name( instance, TREE_IEDITABLE_SIGNAL_FOCUS_IN, instance );

	return( stop );
}

static gboolean
on_focus_out( GtkWidget *widget, GdkEventFocus *event, NactTreeIEditable *instance )
{
	/*static const gchar *thisfn = "nact_tree_ieditable_on_focus_out";*/
	gboolean stop = FALSE;

	/*g_debug( "%s: widget=%p, event=%p, instance=%p", thisfn, ( void * ) widget, ( void * ) event, ( void * ) instance );*/
	g_signal_emit_by_name( instance, TREE_IEDITABLE_SIGNAL_FOCUS_OUT, instance );

	return( stop );
}

static gboolean
on_key_pressed_event( GtkWidget *widget, GdkEventKey *event, NactTreeIEditable *instance )
{
	/*static const gchar *thisfn = "nact_tree_ieditable_v_on_key_pressed_event";
	g_debug( "%s: widget=%p, event=%p, user_data=%p", thisfn, widget, event, user_data );*/
	gboolean stop = FALSE;

	if( event->keyval == NACT_KEY_Return || event->keyval == NACT_KEY_KP_Enter ){
		nact_tree_ieditable_bis_toggle_collapse( instance );
		stop = TRUE;
	}

	if( event->keyval == NACT_KEY_F2 ){
		inline_edition( instance );
		stop = TRUE;
	}

	if( event->keyval == NACT_KEY_Right ){
		nact_tree_ieditable_bis_expand_to_first_child( instance );
		stop = TRUE;
	}

	if( event->keyval == NACT_KEY_Left ){
		nact_tree_ieditable_bis_collapse_to_parent( instance );
		stop = TRUE;
	}

	return( stop );
}

/*
 * path: path of the edited row, as a string
 * text: new text
 *
 * - inform tabs so that they can update their fields
 *   data = object_at_row + new_label
 *   this will trigger set the object content, and other updates
 */
static void
on_label_edited( GtkCellRendererText *renderer, const gchar *path_str, const gchar *text, NactTreeIEditable *instance )
{
	GtkTreeView *treeview;
	NactTreeModel *model;
	NAObject *object;
	GtkTreePath *path;
	gchar *new_text;

	treeview = nact_tree_ieditable_priv_get_actions_list_treeview( instance );
	model = NACT_TREE_MODEL( gtk_tree_view_get_model( treeview ));
	path = gtk_tree_path_new_from_string( path_str );
	object = nact_tree_model_object_at_path( model, path );
	new_text = g_strdup( text );

	g_signal_emit_by_name( instance, TREE_IEDITABLE_SIGNAL_COLUMN_EDITED, object, new_text, TREE_IEDITABLE_LABEL_COLUMN );
}

/*
 * an item has been updated in one of the tabs
 * update the treeview to reflects its new edition status
 */
static void
on_tab_updatable_item_updated( NactTreeIEditable *instance, NAObject *object, gboolean force_display )
{
	static const gchar *thisfn = "nact_tree_ieditable_on_tab_updatable_item_updated";
	GtkTreeView *treeview;
	GtkTreeModel *model;

	g_debug( "%s: instance=%p, object=%p (%s), force_display=%s", thisfn,
			( void * ) instance, ( void * ) object, G_OBJECT_TYPE_NAME( object ),
			force_display ? "True":"False" );
	g_return_if_fail( NACT_IS_TREE_IEDITABLE( instance ));
	g_return_if_fail( NA_IS_OBJECT( object ));
	g_return_if_fail( NA_IS_IDUPLICABLE( object ));

	if( object ){
		treeview = nact_tree_ieditable_priv_get_actions_list_treeview( instance );
		model = gtk_tree_view_get_model( treeview );
		if( !na_object_check_status_up( object ) && force_display ){
			on_edition_status_changed( instance, NA_IDUPLICABLE( object ));
		}
	}
}

static void
open_popup( NactTreeIEditable *instance, GdkEventButton *event )
{
	GtkTreeView *treeview;
	GtkTreeModel *model;
	GtkTreePath *path;

	treeview = nact_tree_ieditable_priv_get_actions_list_treeview( instance );

	if( gtk_tree_view_get_path_at_pos( treeview, event->x, event->y, &path, NULL, NULL, NULL )){
		model = gtk_tree_view_get_model( treeview );
		nact_tree_ieditable_bis_select_row_at_path( instance, treeview, model, path );
		gtk_tree_path_free( path );
		nact_menubar_open_popup( BASE_WINDOW( instance ), event );
	}
}
#endif
