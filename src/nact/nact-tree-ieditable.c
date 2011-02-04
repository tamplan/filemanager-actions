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

#include <api/na-object-api.h>

#include <core/na-updater.h>

#include "nact-application.h"
#include "nact-tree-ieditable.h"
#include "nact-tree-model.h"

/* private interface data
 */
struct _NactTreeIEditableInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* data attached to the NactTreeView
 */
typedef struct {
	NAUpdater   *updater;
	GtkTreeView *treeview;
	guint        count_modified;
}
	IEditableData;

#define VIEW_DATA_IEDITABLE				"view-data-ieditable"

static gboolean st_tree_ieditable_initialized = FALSE;
static gboolean st_tree_ieditable_finalized   = FALSE;

static GType          register_type( void );
static void           interface_base_init( NactTreeIEditableInterface *klass );
static void           interface_base_finalize( NactTreeIEditableInterface *klass );

static void           do_insert_items( GtkTreeView *treeview, GtkTreeModel *model, GList *items, GtkTreePath *insert_path, GList **list_parents );
static NAObject      *do_insert_into_first( GtkTreeView *treeview, GtkTreeModel *model, GList *items, GtkTreePath *insert_path, GtkTreePath **new_path );
static GtkTreePath   *get_selection_first_path( GtkTreeView *treeview );
static IEditableData *get_instance_data( NactTreeIEditable *view );
static void           decrement_counters( NactTreeIEditable *instance, IEditableData *ialid, GList *items );
static void           increment_counters( NactTreeIEditable *view, IEditableData *ied, GList *items );
static void           increment_counters_modified( NAObject *object, IEditableData *ied );

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
 * nact_tree_ieditable_initialize:
 * @instance: the #NactTreeView which implements this interface.
 * @treeview: the embedded #GtkTreeView tree view.
 * @window: the #BaseWindow which embeds the @instance.
 *
 * Initialize the interface, mainly connecting to signals of interest.
 */
void
nact_tree_ieditable_initialize( NactTreeIEditable *instance, GtkTreeView *treeview, BaseWindow *window )
{
	static const gchar *thisfn = "nact_tree_ieditable_initialize";
	IEditableData *ied;
	NactApplication *application;

	g_debug( "%s: instance=%p, window=%p", thisfn, ( void * ) instance, ( void * ) window );

	ied = get_instance_data( instance );
	ied->treeview = treeview;

	application = NACT_APPLICATION( base_window_get_application( window ));
	ied->updater = nact_application_get_updater( application );
}

/**
 * nact_tree_ieditable_delete:
 * @instance: this #NactTreeIEditable instance.
 * @list: list of #NAObject to be deleted.
 * @select_at_end: whether a row should be selected after delete.
 *
 * Deletes the specified list from the underlying tree store.
 *
 * This function takes care of repositionning a new selection if
 * possible, and refilter the display model.
 */
void
nact_tree_ieditable_delete( NactTreeIEditable *instance, GList *items, gboolean select_at_end )
{
	static const gchar *thisfn = "nact_tree_ieditable_delete";
	IEditableData *ied;
	GtkTreeModel *model;
	GtkTreePath *path = NULL;
	GList *it;

	g_return_if_fail( NACT_IS_TREE_IEDITABLE( instance ));

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){
		g_debug( "%s: instance=%p, items=%p (count=%d), select_at_end=%s",
				thisfn, ( void * ) instance, ( void * ) items, g_list_length( items ), select_at_end ? "True":"False" );

		ied = get_instance_data( instance );
		model = gtk_tree_view_get_model( ied->treeview );

		decrement_counters( instance, ied, items );

		for( it = items ; it ; it = it->next ){
			if( path ){
				gtk_tree_path_free( path );
			}

			path = nact_tree_model_delete( NACT_TREE_MODEL( model ), NA_OBJECT( it->data ));

			/*ialid->modified_items = nact_iactions_list_remove_rec( ialid->modified_items, NA_OBJECT( it->data ));*/

			g_debug( "%s: object=%p (%s, ref_count=%d)", thisfn,
					( void * ) it->data, G_OBJECT_TYPE_NAME( it->data ), G_OBJECT( it->data )->ref_count );
		}

		gtk_tree_model_filter_refilter( GTK_TREE_MODEL_FILTER( model ));

		if( path ){
			if( select_at_end ){
				nact_tree_view_select_row_at_path( NACT_TREE_VIEW( instance ), path );
			}
			gtk_tree_path_free( path );
		}
	}
}

/**
 * nact_tree_ieditable_insert_at_path:
 * @instance: this #NactTreeIEditable instance.
 * @items: a list of items to be inserted (e.g. from a paste).
 * @path: insertion path.
 *
 * Inserts the provided @items list in the treeview.
 *
 * This function takes care of repositionning a new selection if
 * possible, and refilter the display model.
 */
void
nact_tree_ieditable_insert_at_path( NactTreeIEditable *instance, GList *items, GtkTreePath *insert_path )
{
	static const gchar *thisfn = "nact_tree_ieditable_insert_at_path";
	IEditableData *ied;
	GtkTreeModel *model;
	GList *parents;

	g_return_if_fail( NACT_IS_TREE_IEDITABLE( instance ));

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){
		g_debug( "%s: instance=%p, items=%p (count=%d)",
			thisfn, ( void * ) instance, ( void * ) items, g_list_length( items ));

		ied = get_instance_data( instance );
		model = gtk_tree_view_get_model( ied->treeview );
		g_return_if_fail( NACT_IS_TREE_MODEL( model ));

		increment_counters( instance, ied, items );
		do_insert_items( ied->treeview, model, items, insert_path, &parents );

		g_list_foreach( parents, ( GFunc ) na_object_object_check_status, NULL );
		g_list_free( parents );

		gtk_tree_model_filter_refilter( GTK_TREE_MODEL_FILTER( model ));
		nact_tree_view_select_row_at_path( NACT_TREE_VIEW( instance ), insert_path );
	}
}

/**
 * nact_tree_ieditable_insert_items:
 * @instance: this #NactTreeIEditable instance.
 * @items: a list of items to be inserted (e.g. from a paste).
 * @sibling: the #NAObject besides which the insertion should occurs;
 *  this is mainly used for inserting duplicated items besides each of
 *  original ones; may be %NULL.
 *
 * Inserts the provided @items list just before the @sibling object,
 * or at the current position in the treeview if @sibling is %NULL.
 *
 * The provided @items list is supposed to be homogeneous, i.e. referes
 * to only profiles, or only actions or menus.
 *
 * If the @items list contains profiles, they can only be inserted
 * into an action, or before another profile.
 *
 * If new item is a #NAObjectMenu or a #NAObjectAction, it will be inserted
 * before the current action or menu.
 *
 * This function takes care of repositionning a new selection (if possible)
 * on the lastly inserted item, and to refresh the display.
 */
void
nact_tree_ieditable_insert_items( NactTreeIEditable *instance, GList *items, NAObject *sibling )
{
	static const gchar *thisfn = "nact_tree_ieditable_insert_items";
	IEditableData *ied;
	GtkTreeModel *model;
	GtkTreePath *insert_path;
	NAObject *object, *parent;

	g_return_if_fail( NACT_IS_TREE_IEDITABLE( instance ));

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){
		g_debug( "%s: instance=%p, items=%p (count=%d), sibling=%p",
				thisfn, ( void * ) instance, ( void * ) items, g_list_length( items ), ( void * ) sibling );

		insert_path = NULL;
		ied = get_instance_data( instance );
		model = gtk_tree_view_get_model( ied->treeview );
		g_return_if_fail( NACT_IS_TREE_MODEL( model ));

		if( sibling ){
			insert_path = nact_tree_model_object_to_path( NACT_TREE_MODEL( model ), sibling );

		} else {
			insert_path = get_selection_first_path( ied->treeview );
			object = nact_tree_model_object_at_path( NACT_TREE_MODEL( model ), insert_path );
			g_debug( "%s: current object at insertion path is %p", thisfn, ( void * ) object );

			/* as a particular case, insert a new profile _into_ current action
			 */
			if( NA_IS_OBJECT_ACTION( object ) && NA_IS_OBJECT_PROFILE( items->data )){
				nact_tree_ieditable_insert_items_into( instance, items );
				gtk_tree_path_free( insert_path );
				return;
			}

			/* insert a new NAObjectItem before current NAObjectItem
			 */
			if( NA_IS_OBJECT_PROFILE( object ) && NA_IS_OBJECT_ITEM( items->data )){
				parent = ( NAObject * ) na_object_get_parent( object );
				gtk_tree_path_free( insert_path );
				insert_path = nact_tree_model_object_to_path( NACT_TREE_MODEL( model ), parent );
			}
		}

		nact_tree_ieditable_insert_at_path( instance, items, insert_path );
		gtk_tree_path_free( insert_path );
	}
}

/**
 * nact_tree_ieditable_insert_items_into:
 * @instance: this #NactTreeIEditable instance.
 * @items: a list of items to be inserted (e.g. from a paste).
 *
 * Inserts the provided @items list as first children of the current item.
 *
 * The provided @items list is supposed to be homogeneous, i.e. referes
 * to only profiles, or only actions or menus.
 *
 * If the @items list contains only profiles, they can only be inserted
 * into an action, and the profiles will eventually be renumbered.
 *
 * If new item is a #NAObjectMenu or a #NAObjectAction, it will be inserted
 * as first children of the current menu.
 *
 * This function takes care of repositionning a new selection as the
 * last inserted item, and refilter the display model.
 */
void
nact_tree_ieditable_insert_items_into( NactTreeIEditable *instance, GList *items )
{
	static const gchar *thisfn = "nact_tree_ieditable_insert_items_into";
	IEditableData *ied;
	GtkTreeModel *model;
	GtkTreePath *insert_path;
	GtkTreePath *new_path;
	NAObject *parent;

	g_return_if_fail( NACT_IS_TREE_IEDITABLE( instance ));

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){
		g_debug( "%s: instance=%p, items=%p (count=%d)",
			thisfn, ( void * ) instance, ( void * ) items, g_list_length( items ));

		insert_path = NULL;
		ied = get_instance_data( instance );
		model = gtk_tree_view_get_model( ied->treeview );
		insert_path = get_selection_first_path( ied->treeview );

		increment_counters( instance, ied, items );
		parent = do_insert_into_first( ied->treeview, model, items, insert_path, &new_path );
		gtk_tree_model_filter_refilter( GTK_TREE_MODEL_FILTER( model ));
		nact_tree_view_select_row_at_path( NACT_TREE_VIEW( instance ), new_path );

		gtk_tree_path_free( new_path );
		gtk_tree_path_free( insert_path );
	}
}

static void
do_insert_items( GtkTreeView *treeview, GtkTreeModel *model, GList *items, GtkTreePath *insert_path, GList **list_parents )
{
	static const gchar *thisfn = "nact_tree_ieditable_do_insert_items";
	GList *reversed;
	GList *it;
	GList *subitems;
	NAObject *obj_parent;
	gpointer updatable;
	GtkTreePath *inserted_path;

	obj_parent = NULL;
	if( list_parents ){
		*list_parents = NULL;
	}

	reversed = g_list_reverse( g_list_copy( items ));

	for( it = reversed ; it ; it = it->next ){
		inserted_path = nact_tree_model_insert( NACT_TREE_MODEL( model ), NA_OBJECT( it->data ), insert_path, &obj_parent );

		g_debug( "%s: object=%p (%s, ref_count=%d)", thisfn,
				( void * ) it->data, G_OBJECT_TYPE_NAME( it->data ), G_OBJECT( it->data )->ref_count );

		if( list_parents ){
			updatable = obj_parent ? obj_parent : it->data;
			if( !g_list_find( *list_parents, updatable )){
				*list_parents = g_list_prepend( *list_parents, updatable );
			}
		}

		/* recursively insert subitems
		 */
		if( NA_IS_OBJECT_ITEM( it->data ) && na_object_get_items_count( it->data )){
			subitems = na_object_get_items( it->data );
			do_insert_into_first( treeview, model, subitems, inserted_path, NULL );
		}

		gtk_tree_path_free( inserted_path );
	}

	g_list_free( reversed );
}

static NAObject *
do_insert_into_first( GtkTreeView *treeview, GtkTreeModel *model, GList *items, GtkTreePath *insert_path, GtkTreePath **new_path )
{
	static const gchar *thisfn = "nact_tree_ieditable_do_insert_into_first";
	GList *copy;
	GList *last;
	NAObject *parent;
	gchar *insert_path_str;
	GtkTreePath *inserted_path;
	GList *subitems;

	insert_path_str = gtk_tree_path_to_string( insert_path );
	g_debug( "%s: treeview=%p, model=%p, items=%p (count=%d), insert_path=%p (%s), new_path=%p",
			thisfn,
			( void * ) treeview, ( void * ) model, ( void * ) items, g_list_length( items ),
			( void * ) insert_path, insert_path_str, ( void * ) new_path );
	g_free( insert_path_str );

	parent = NULL;
	copy = g_list_copy( items );
	last = g_list_last( copy );
	copy = g_list_remove_link( copy, last );

	inserted_path = nact_tree_model_insert_into( NACT_TREE_MODEL( model ), NA_OBJECT( last->data ), insert_path, &parent );
	gtk_tree_view_expand_to_path( treeview, inserted_path );

	/* recursively insert subitems
	 */
	if( NA_IS_OBJECT_ITEM( last->data ) && na_object_get_items_count( last->data )){
		subitems = na_object_get_items( last->data );
		do_insert_into_first( treeview, model, subitems, inserted_path, NULL );
	}

	do_insert_items( treeview, model, copy, inserted_path, NULL );

	if( new_path ){
		*new_path = gtk_tree_path_copy( inserted_path );
	}

	g_list_free( copy );
	gtk_tree_path_free( inserted_path );

	return( parent );
}

static GtkTreePath *
get_selection_first_path( GtkTreeView *treeview )
{
	GtkTreeSelection *selection;
	GList *selected;
	GtkTreePath *path;

	path = NULL;
	selection = gtk_tree_view_get_selection( treeview );
	selected = gtk_tree_selection_get_selected_rows( selection, NULL );

	if( selected ){
		path = gtk_tree_path_copy(( GtkTreePath * ) selected->data );

	} else {
		path = gtk_tree_path_new_from_string( "0" );
	}

	g_list_foreach( selected, ( GFunc ) gtk_tree_path_free, NULL );
	g_list_free( selected );

	return( path );
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

static void
decrement_counters( NactTreeIEditable *view, IEditableData *ied, GList *items )
{
	static const gchar *thisfn = "nact_tree_editable_decrement_counters";
	gint menus, actions, profiles;
	BaseWindow *window;

	g_debug( "%s: view=%p, ied=%p, items=%p",
			thisfn, ( void * ) view, ( void * ) ied, ( void * ) items );

	menus = 0;
	actions = 0;
	profiles = 0;
	na_object_item_count_items( items, &menus, &actions, &profiles, TRUE );
	menus *= -1;
	actions *= -1;
	profiles *= -1;
	window = nact_tree_view_get_window( NACT_TREE_VIEW( view ));
	g_signal_emit_by_name( G_OBJECT( window ), TREE_SIGNAL_COUNT_CHANGED, view, FALSE, menus, actions, profiles );
}

/*
 * we pass here before each insertion operation (apart initial fill)
 */
static void
increment_counters( NactTreeIEditable *view, IEditableData *ied, GList *items )
{
	static const gchar *thisfn = "nact_tre_ieditablet_increment_counters";
	guint modified_prev;
	gint menus, actions, profiles;
	BaseWindow *window;

	g_debug( "%s: view=%p, ied=%p, items=%p (count=%d)",
			thisfn, ( void * ) view, ( void * ) ied, ( void * ) items, items ? g_list_length( items ) : 0 );

	menus = 0;
	actions = 0;
	profiles = 0;
	na_object_item_count_items( items, &menus, &actions, &profiles, TRUE );
	window = nact_tree_view_get_window( NACT_TREE_VIEW( view ));
	g_signal_emit_by_name( G_OBJECT( window ), TREE_SIGNAL_COUNT_CHANGED, view, FALSE, menus, actions, profiles );

	modified_prev = ied->count_modified;
	g_list_foreach( items, ( GFunc ) increment_counters_modified, ied );
	if( ied->count_modified != modified_prev ){
		g_signal_emit_by_name( G_OBJECT( window ), TREE_SIGNAL_MODIFIED_COUNT_CHANGED, view, ied->count_modified );
	}
}

static void
increment_counters_modified( NAObject *object, IEditableData *ied )
{
	gboolean writable;
	guint reason;

	if( NA_IS_OBJECT_ITEM( object )){

		if( !na_object_get_provider( object )){
			ied->count_modified += 1;
		}

		writable = na_updater_is_item_writable( ied->updater, NA_OBJECT_ITEM( object ), &reason );
		na_object_set_writability_status( object, writable, reason );
	}
}

#if 0

/* iter on selection prototype
 */
typedef gboolean ( *FnIterOnSelection )( NactIActionsList *, GtkTreeView *, GtkTreeModel *, GtkTreeIter *, NAObject *, gpointer );

/**
 * nact_iactions_list_bis_clear_selection:
 * @instance: this instance of the #NactIActionsList interface.
 * @treeview: the #GtkTreeView.
 *
 * Clears the current selection.
 */
void
nact_iactions_list_bis_clear_selection( NactIActionsList *instance, GtkTreeView *treeview )
{
	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection( treeview );
	gtk_tree_selection_unselect_all( selection );
}

/**
 * nact_iactions_list_bis_collapse_to_parent:
 * @instance: this instance of the #NactIActionsList interface.
 *
 * On left arrow, if we are on a first child, then collapse and go to
 * the parent.
 */
void
nact_iactions_list_bis_collapse_to_parent( NactIActionsList *instance )
{
	static const gchar *thisfn = "nact_iactions_list_bis_collapse_to_parent";
	IActionsListInstanceData *ialid;
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *listrows;
	GtkTreePath *path;
	gint *indices;
	GtkTreePath *parent_path;

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_IACTIONS_LIST( instance ));

	ialid = nact_iactions_list_priv_get_instance_data( instance );
	if( ialid->management_mode == IACTIONS_LIST_MANAGEMENT_MODE_EDITION ){

		treeview = nact_iactions_list_priv_get_actions_list_treeview( instance );
		selection = gtk_tree_view_get_selection( treeview );
		listrows = gtk_tree_selection_get_selected_rows( selection, &model );
		if( g_list_length( listrows ) == 1 ){

			path = ( GtkTreePath * ) listrows->data;
			/*g_debug( "%s: path_depth=%d", thisfn, gtk_tree_path_get_depth( path ));*/
			if( gtk_tree_path_get_depth( path ) > 1 ){

				indices = gtk_tree_path_get_indices( path );
				if( indices[ gtk_tree_path_get_depth( path )-1 ] == 0 ){

					parent_path = gtk_tree_path_copy( path );
					gtk_tree_path_up( parent_path );
					nact_iactions_list_bis_select_row_at_path( instance, treeview, model, parent_path );
					gtk_tree_view_collapse_row( treeview, parent_path );
					gtk_tree_path_free( parent_path );
				}
			}
		}

		g_list_foreach( listrows, ( GFunc ) gtk_tree_path_free, NULL );
		g_list_free( listrows );
	}
}

/**
 * nact_iactions_list_bis_expand_to_first_child:
 * @instance: this #NactIActionsList interface.
 *
 * On right arrow, expand the parent if it has childs, and select the
 * first child.
 */
void
nact_iactions_list_bis_expand_to_first_child( NactIActionsList *instance )
{
	static const gchar *thisfn = "nact_iactions_list_bis_expand_to_first_child";
	IActionsListInstanceData *ialid;
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *listrows;
	GtkTreePath *path;
	GtkTreeIter iter;
	GtkTreePath *child_path;

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_IACTIONS_LIST( instance ));

	ialid = nact_iactions_list_priv_get_instance_data( instance );
	if( ialid->management_mode == IACTIONS_LIST_MANAGEMENT_MODE_EDITION ){
		treeview = nact_iactions_list_priv_get_actions_list_treeview( instance );
		selection = gtk_tree_view_get_selection( treeview );
		listrows = gtk_tree_selection_get_selected_rows( selection, &model );

		if( g_list_length( listrows ) == 1 ){
			path = ( GtkTreePath * ) listrows->data;
			gtk_tree_model_get_iter( model, &iter, path );
			if( gtk_tree_model_iter_has_child( model, &iter )){
				child_path = gtk_tree_path_copy( path );
				gtk_tree_path_append_index( child_path, 0 );
				gtk_tree_view_expand_row( treeview, child_path, FALSE );
				nact_iactions_list_bis_select_row_at_path( instance, treeview, model, child_path );
				gtk_tree_path_free( child_path );
			}
		}

		g_list_foreach( listrows, ( GFunc ) gtk_tree_path_free, NULL );
		g_list_free( listrows );
	}
}

/**
 * nact_iactions_list_bis_get_selected_items:
 * @window: this #NactIActionsList instance.
 *
 * Returns: the currently selected rows as a list of #NAObjects.
 *
 * We acquire here a new reference on objects corresponding to actually
 * selected rows, and their childs.
 *
 * The caller may safely call na_object_free_items() on the
 * returned list.
 */
GList *
nact_iactions_list_bis_get_selected_items( NactIActionsList *instance )
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

	if( st_iactions_list_initialized && !st_iactions_list_finalized ){

		treeview = nact_iactions_list_priv_get_actions_list_treeview( instance );
		selection = gtk_tree_view_get_selection( treeview );
		listrows = gtk_tree_selection_get_selected_rows( selection, &model );

		for( it = listrows ; it ; it = it->next ){
			path = ( GtkTreePath * ) it->data;
			gtk_tree_model_get_iter( model, &iter, path );
			gtk_tree_model_get( model, &iter, IACTIONS_LIST_NAOBJECT_COLUMN, &object, -1 );
			g_debug( "nact_iactions_list_get_selected_items: object=%p", ( void * ) object );
			items = g_list_prepend( items, na_object_ref( object ));
			g_object_unref( object );
		}

		g_list_foreach( listrows, ( GFunc ) gtk_tree_path_free, NULL );
		g_list_free( listrows );

		items = g_list_reverse( items );
	}

	return( items );
}

/**
 * nact_iactions_list_bis_list_modified_items:
 * @instance: this #NactIActionsList instance.
 *
 * Dumps the modified items list.
 */
void
nact_iactions_list_bis_list_modified_items( NactIActionsList *instance )
{
	static const gchar *thisfn = "nact_iactions_list_bis_list_modified_items";
	IActionsListInstanceData *ialid;
	GList *it;

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_IACTIONS_LIST( instance ));

	if( st_iactions_list_initialized && !st_iactions_list_finalized ){

		ialid = nact_iactions_list_priv_get_instance_data( instance );

		g_debug( "%s: raw list", thisfn );
		for( it = ialid->modified_items ; it ; it = it->next ){
			g_debug( "%s: %p (%s)", thisfn, ( void * ) it->data, G_OBJECT_TYPE_NAME( it->data ));
		}

		g_debug( "%s: detailed list", thisfn );
		for( it = ialid->modified_items ; it ; it = it->next ){
			na_object_dump( it->data );
		}
	}
}

/**
 * nact_iactions_list_bis_remove_modified:
 * @instance: this #NactIActionsList instance.
 *
 * Removes the saved item from the modified items list.
 */
void
nact_iactions_list_bis_remove_modified( NactIActionsList *instance, const NAObjectItem *item )
{
	IActionsListInstanceData *ialid;

	g_return_if_fail( NACT_IS_IACTIONS_LIST( instance ));

	if( st_iactions_list_initialized && !st_iactions_list_finalized ){

		ialid = nact_iactions_list_priv_get_instance_data( instance );
		ialid->modified_items = g_list_remove( ialid->modified_items, item );

		if( g_list_length( ialid->modified_items ) == 0 ){
			g_list_free( ialid->modified_items );
			ialid->modified_items = NULL;
		}
	}
}

/**
 * nact_iactions_list_bis_toggle_collapse:
 * @instance: this #NactIActionsList interface.
 *
 * Toggle or collapse the current subtree.
 */
void
nact_iactions_list_bis_toggle_collapse( NactIActionsList *instance )
{
	int toggle = TOGGLE_UNDEFINED;

	iter_on_selection( instance, ( FnIterOnSelection ) toggle_collapse_iter, &toggle );
}

/**
 * nact_iactions_list_bis_toggle_collapse_object:
 * @instance: the current instance of the #NactIActionsList interface.
 * @item: the item to be toggled/collapsed.
 *
 * Collapse / expand if actions has more than one profile.
 */
void
nact_iactions_list_bis_toggle_collapse_object( NactIActionsList *instance, const NAObject *item )
{
	static const gchar *thisfn = "nact_iactions_list_bis_toggle_collapse";
	GtkTreeView *treeview;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean iterok, stop;
	NAObject *iter_object;

	g_return_if_fail( NACT_IS_IACTIONS_LIST( instance ));
	g_return_if_fail( NA_IS_OBJECT_ITEM( item ));

	if( st_iactions_list_initialized && !st_iactions_list_finalized ){

		treeview = nact_iactions_list_priv_get_actions_list_treeview( instance );
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

	treeview = nact_iactions_list_priv_get_actions_list_treeview( instance );
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
#endif
