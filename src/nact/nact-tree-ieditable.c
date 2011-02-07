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

#include "base-keysyms.h"
#include "nact-application.h"
#include "nact-main-window.h"
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
	NAUpdater     *updater;
	BaseWindow    *window;
	GtkTreeView   *treeview;
	NactTreeModel *model;
	guint          count_modified;
	GList         *deleted;
}
	IEditableData;

#define VIEW_DATA_IEDITABLE				"view-data-ieditable"

static gboolean st_tree_ieditable_initialized = FALSE;
static gboolean st_tree_ieditable_finalized   = FALSE;

static GType          register_type( void );
static void           interface_base_init( NactTreeIEditableInterface *klass );
static void           interface_base_finalize( NactTreeIEditableInterface *klass );

static gboolean       on_key_pressed_event( GtkWidget *widget, GdkEventKey *event, BaseWindow *window );
static void           on_label_edited( GtkCellRendererText *renderer, const gchar *path_str, const gchar *text, BaseWindow *window );
static void           do_insert_before( NactTreeIEditable *view, IEditableData *ied, GList *items, GtkTreePath *insert_path );
static GtkTreePath   *do_insert_into( NactTreeIEditable *view, IEditableData *ied, GList *items, GtkTreePath *insert_path );
static void           increment_count_items( NactTreeIEditable *view, IEditableData *ied, GList *items );
static GtkTreePath   *get_selection_first_path( GtkTreeView *treeview );
static IEditableData *get_instance_data( NactTreeIEditable *view );
static void           decrement_counters( NactTreeIEditable *instance, IEditableData *ialid, GList *items );
static void           inline_edition( BaseWindow *window );

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
	GtkTreeViewColumn *column;
	GList *renderers;

	g_debug( "%s: instance=%p, treeview=%p, window=%p",
			thisfn, ( void * ) instance, ( void * ) treeview, ( void * ) window );

	ied = get_instance_data( instance );
	ied->window = window;
	ied->treeview = treeview;
	ied->model = NACT_TREE_MODEL( gtk_tree_view_get_model( treeview ));

	application = NACT_APPLICATION( base_window_get_application( window ));
	ied->updater = nact_application_get_updater( application );

	/* expand/collapse with the keyboard */
	base_window_signal_connect( window,
			G_OBJECT( treeview ), "key-press-event", G_CALLBACK( on_key_pressed_event ));

	/* label edition: inform the corresponding tab */
	column = gtk_tree_view_get_column( treeview, TREE_COLUMN_LABEL );
	renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( column ));
	base_window_signal_connect( window,
			G_OBJECT( renderers->data ), "edited", G_CALLBACK( on_label_edited ));
}

/**
 * nact_tree_ieditable_terminate:
 * @instance: the #NactTreeView which implements this interface.
 *
 * Free the data when application quits.
 */
void
nact_tree_ieditable_terminate( NactTreeIEditable *instance )
{
	static const gchar *thisfn = "nact_tree_ieditable_terminate";
	IEditableData *ied;

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

	ied = get_instance_data( instance );

	g_list_free( ied->deleted );
}

static gboolean
on_key_pressed_event( GtkWidget *widget, GdkEventKey *event, BaseWindow *window )
{
	gboolean stop = FALSE;

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){

		if( event->keyval == NACT_KEY_F2 ){
			inline_edition( window );
			stop = TRUE;
		}
	}

	return( stop );
}

/*
 * path: path of the edited row, as a string
 * text: new text
 *
 * - inform tabs so that they can update their fields
 *   data = object_at_row + new_label
 */
static void
on_label_edited( GtkCellRendererText *renderer, const gchar *path_str, const gchar *text, BaseWindow *window )
{
	NactTreeView *items_view;
	IEditableData *ied;
	NAObject *object;
	GtkTreePath *path;
	gchar *new_text;

	items_view = nact_main_window_get_items_view( NACT_MAIN_WINDOW( window ));
	ied = ( IEditableData * ) g_object_get_data( G_OBJECT( items_view ), VIEW_DATA_IEDITABLE );
	path = gtk_tree_path_new_from_string( path_str );
	object = nact_tree_model_object_at_path( ied->model, path );
	new_text = g_strdup( text );

	g_signal_emit_by_name( window, TREE_SIGNAL_CONTENT_CHANGED, items_view, object );
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
 *
 * Deleted NAObjectItems are added to the 'deleted' list attached to the
 * view, so that they will be actually deleted from the storage subsystem
 * on save.
 *
 * Deleted NAObjectProfiles are not added to the list, because the deletion
 * will be recorded when saving the action.
 */
void
nact_tree_ieditable_delete( NactTreeIEditable *instance, GList *items, gboolean select_at_end )
{
	static const gchar *thisfn = "nact_tree_ieditable_delete";
	IEditableData *ied;
	GtkTreePath *path = NULL;
	GList *it;

	g_return_if_fail( NACT_IS_TREE_IEDITABLE( instance ));

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){
		g_debug( "%s: instance=%p, items=%p (count=%d), select_at_end=%s",
				thisfn, ( void * ) instance, ( void * ) items, g_list_length( items ), select_at_end ? "True":"False" );

		ied = get_instance_data( instance );

		decrement_counters( instance, ied, items );

		for( it = items ; it ; it = it->next ){
			if( path ){
				gtk_tree_path_free( path );
			}

			path = nact_tree_model_delete( ied->model, NA_OBJECT( it->data ));

			/*ialid->modified_items = nact_iactions_list_remove_rec( ialid->modified_items, NA_OBJECT( it->data ));*/

			g_debug( "%s: object=%p (%s, ref_count=%d)", thisfn,
					( void * ) it->data, G_OBJECT_TYPE_NAME( it->data ), G_OBJECT( it->data )->ref_count );
		}

		gtk_tree_model_filter_refilter( GTK_TREE_MODEL_FILTER( ied->model ));

		if( path ){
			if( select_at_end ){
				nact_tree_view_select_row_at_path( NACT_TREE_VIEW( instance ), path );
			}
			gtk_tree_path_free( path );
		}
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
	GtkTreePath *insert_path;
	NAObject *object, *parent;

	g_return_if_fail( NACT_IS_TREE_IEDITABLE( instance ));

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){
		g_debug( "%s: instance=%p, items=%p (count=%d), sibling=%p",
				thisfn, ( void * ) instance, ( void * ) items, g_list_length( items ), ( void * ) sibling );

		insert_path = NULL;
		ied = get_instance_data( instance );

		if( sibling ){
			insert_path = nact_tree_model_object_to_path( ied->model, sibling );

		} else {
			insert_path = get_selection_first_path( ied->treeview );
			object = nact_tree_model_object_at_path( ied->model, insert_path );
			g_debug( "%s: current object at insertion path is %p", thisfn, ( void * ) object );

			/* as a particular case, insert a new profile _into_ current action
			 */
			if( NA_IS_OBJECT_ACTION( object ) && NA_IS_OBJECT_PROFILE( items->data )){
				nact_tree_ieditable_insert_into( instance, items );
				gtk_tree_path_free( insert_path );
				return;
			}

			/* insert a new NAObjectItem before current NAObjectItem
			 */
			if( NA_IS_OBJECT_PROFILE( object ) && NA_IS_OBJECT_ITEM( items->data )){
				parent = ( NAObject * ) na_object_get_parent( object );
				gtk_tree_path_free( insert_path );
				insert_path = nact_tree_model_object_to_path( ied->model, parent );
			}
		}

		nact_tree_ieditable_insert_at_path( instance, items, insert_path );
		gtk_tree_path_free( insert_path );
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
	guint prev_count;

	g_return_if_fail( NACT_IS_TREE_IEDITABLE( instance ));

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){
		g_debug( "%s: instance=%p, items=%p (count=%d)",
			thisfn, ( void * ) instance, ( void * ) items, g_list_length( items ));

		ied = get_instance_data( instance );
		prev_count = ied->count_modified;

		do_insert_before( instance, ied, items, insert_path );

		/* post insertion
		 */
		increment_count_items( instance, ied, items );
		if( ied->count_modified != prev_count ){
			g_signal_emit_by_name( G_OBJECT( ied->window ),
					TREE_SIGNAL_MODIFIED_COUNT_CHANGED, NACT_TREE_VIEW( instance ), ied->count_modified );
		}
		gtk_tree_model_filter_refilter( GTK_TREE_MODEL_FILTER( ied->model ));
		nact_tree_view_select_row_at_path( NACT_TREE_VIEW( instance ), insert_path );
	}
}

static void
do_insert_before( NactTreeIEditable *view, IEditableData *ied, GList *items, GtkTreePath *insert_path )
{
	static const gchar *thisfn = "nact_tree_ieditable_do_insert_before";
	GList *reversed;
	GList *it;
	GtkTreePath *inserted_path;

	reversed = g_list_reverse( g_list_copy( items ));

	for( it = reversed ; it ; it = it->next ){
		inserted_path = nact_tree_model_insert_before( ied->model, NA_OBJECT( it->data ), insert_path );
		g_debug( "%s: object=%p (%s, ref_count=%d)", thisfn,
				( void * ) it->data, G_OBJECT_TYPE_NAME( it->data ), G_OBJECT( it->data )->ref_count );

		/* after the item has been inserted, and attached to its new parent
		 * increment modified counters
		 */

		/* recursively insert subitems
		 */
		if( NA_IS_OBJECT_ITEM( it->data )){
			do_insert_into( view, ied, na_object_get_items( it->data ), inserted_path );
		}

		na_object_check_status( it->data );
		gtk_tree_path_free( inserted_path );
	}

	g_list_free( reversed );
}

/**
 * nact_tree_ieditable_insert_into:
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
nact_tree_ieditable_insert_into( NactTreeIEditable *instance, GList *items )
{
	static const gchar *thisfn = "nact_tree_ieditable_insert_into";
	IEditableData *ied;
	GtkTreePath *insert_path;
	GtkTreePath *new_path;
	guint prev_count;

	g_return_if_fail( NACT_IS_TREE_IEDITABLE( instance ));

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){
		g_debug( "%s: instance=%p, items=%p (count=%d)",
			thisfn, ( void * ) instance, ( void * ) items, g_list_length( items ));

		insert_path = NULL;
		ied = get_instance_data( instance );
		prev_count = ied->count_modified;
		insert_path = get_selection_first_path( ied->treeview );

		new_path = do_insert_into( instance, ied, items, insert_path );

		/* post insertion
		 */
		increment_count_items( instance, ied, items );
		if( ied->count_modified != prev_count ){
			g_signal_emit_by_name( G_OBJECT( ied->window ),
					TREE_SIGNAL_MODIFIED_COUNT_CHANGED, NACT_TREE_VIEW( instance ), ied->count_modified );
		}
		gtk_tree_model_filter_refilter( GTK_TREE_MODEL_FILTER( ied->model ));
		nact_tree_view_select_row_at_path( NACT_TREE_VIEW( instance ), new_path );

		gtk_tree_path_free( new_path );
		gtk_tree_path_free( insert_path );
	}
}

/*
 * inserting a list of children into an object is as:
 * - inserting the last child into the object
 * - then inserting other children just before the last insertion path
 *
 * Returns the actual insertion path suitable for the next selection.
 * This path should be gtk_tree_path_free() by the caller.
 */
static GtkTreePath *
do_insert_into( NactTreeIEditable *view, IEditableData *ied, GList *items, GtkTreePath *insert_path )
{
	static const gchar *thisfn = "nact_tree_ieditable_do_insert_into";
	GList *copy;
	GList *last;
	gchar *insert_path_str;
	GtkTreePath *inserted_path;

	insert_path_str = gtk_tree_path_to_string( insert_path );
	g_debug( "%s: view=%p, ied=%p, items=%p (count=%d), insert_path=%p (%s)",
			thisfn,
			( void * ) view, ( void * ) ied, ( void * ) items, g_list_length( items ),
			( void * ) insert_path, insert_path_str );
	g_free( insert_path_str );

	copy = g_list_copy( items );
	last = g_list_last( copy );
	copy = g_list_remove_link( copy, last );

	/* insert the last child into the wished path
	 * and recursively all its children
	 */
	inserted_path = nact_tree_model_insert_into( ied->model, NA_OBJECT( last->data ), insert_path );
	gtk_tree_view_expand_to_path( ied->treeview, inserted_path );

	if( NA_IS_OBJECT_ITEM( last->data )){
		do_insert_into( view, ied, na_object_get_items( last->data ), inserted_path );
		na_object_check_status( last->data );
	}

	/* then insert other objects
	 */
	do_insert_before( view, ied, copy, inserted_path );

	g_list_free( copy );

	return( inserted_path );
}

/*
 * we pass here after each insertion operation (apart initial fill)
 */
static void
increment_count_items( NactTreeIEditable *view, IEditableData *ied, GList *items )
{
	static const gchar *thisfn = "nact_tre_ieditablet_increment_count_items";
	gint menus, actions, profiles;

	g_debug( "%s: view=%p, ied=%p, items=%p (count=%d)",
			thisfn, ( void * ) view, ( void * ) ied, ( void * ) items, items ? g_list_length( items ) : 0 );

	na_object_count_items( items, &menus, &actions, &profiles );
	g_signal_emit_by_name( G_OBJECT( ied->window ), TREE_SIGNAL_COUNT_CHANGED, view, FALSE, menus, actions, profiles );
}

#if 0
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
#endif

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

	g_debug( "%s: view=%p, ied=%p, items=%p",
			thisfn, ( void * ) view, ( void * ) ied, ( void * ) items );

	na_object_count_items( items, &menus, &actions, &profiles );
	menus *= -1;
	actions *= -1;
	profiles *= -1;
	g_signal_emit_by_name( G_OBJECT( ied->window ), TREE_SIGNAL_COUNT_CHANGED, view, FALSE, menus, actions, profiles );
}

/*
 * triggered by 'F2' key
 * let thelabel be edited
 */
static void
inline_edition( BaseWindow *window )
{
	NactTreeView *items_view;
	IEditableData *ied;
	GtkTreeSelection *selection;
	GList *listrows;
	GtkTreePath *path;
	GtkTreeViewColumn *column;

	items_view = nact_main_window_get_items_view( NACT_MAIN_WINDOW( window ));
	ied = ( IEditableData * ) g_object_get_data( G_OBJECT( items_view ), VIEW_DATA_IEDITABLE );
	selection = gtk_tree_view_get_selection( ied->treeview );
	listrows = gtk_tree_selection_get_selected_rows( selection, NULL );

	if( g_list_length( listrows ) == 1 ){
		path = ( GtkTreePath * ) listrows->data;
		column = gtk_tree_view_get_column( ied->treeview, TREE_COLUMN_LABEL );
		gtk_tree_view_set_cursor( ied->treeview, path, column, TRUE );
	}

	g_list_foreach( listrows, ( GFunc ) gtk_tree_path_free, NULL );
	g_list_free( listrows );
}
