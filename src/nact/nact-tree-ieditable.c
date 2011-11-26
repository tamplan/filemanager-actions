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
	gulong         modified_handler_id;
	gulong         valid_handler_id;
	guint          count_modified;
	gboolean       level_zero_changed;
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
static void           on_main_selection_changed( BaseWindow *window, GList *selected_items, gpointer user_data );
static void           on_object_modified_status_changed( NactTreeIEditable *view, NAObject *object, gboolean new_status, BaseWindow *window );
static void           on_object_valid_status_changed( NactTreeIEditable *view, NAObject *object, gboolean new_status, BaseWindow *window );
static void           on_tree_view_level_zero_changed( BaseWindow *window, gboolean is_modified, gpointer user_data );
static void           on_tree_view_modified_status_changed( BaseWindow *window, gboolean is_modified, gpointer user_data );
static void           add_to_deleted_rec( IEditableData *ied, NAObject *object );
static void           decrement_counters( NactTreeIEditable *view, IEditableData *ialid, GList *items );
static GtkTreePath   *do_insert_before( IEditableData *ied, GList *items, GtkTreePath *insert_path );
static GtkTreePath   *do_insert_into( IEditableData *ied, GList *items, GtkTreePath *insert_path );
static void           increment_counters( NactTreeIEditable *view, IEditableData *ied, GList *items );
static GtkTreePath   *get_selection_first_path( GtkTreeView *treeview );
static gboolean       get_modification_status( IEditableData *ied );
static IEditableData *get_instance_data( NactTreeIEditable *view );
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

	/* inline label edition with F2 */
	base_window_signal_connect( window,
			G_OBJECT( treeview ), "key-press-event", G_CALLBACK( on_key_pressed_event ));

	/* label edition: inform the corresponding tab */
	column = gtk_tree_view_get_column( treeview, TREE_COLUMN_LABEL );
	renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( column ));
	base_window_signal_connect( window,
			G_OBJECT( renderers->data ), "edited", G_CALLBACK( on_label_edited ));

	/* monitors status changes to refresh the display */
	na_iduplicable_register_consumer( G_OBJECT( instance ));
	ied->modified_handler_id = base_window_signal_connect( window,
			G_OBJECT( instance ), IDUPLICABLE_SIGNAL_MODIFIED_CHANGED, G_CALLBACK( on_object_modified_status_changed ));
	ied->valid_handler_id = base_window_signal_connect( window,
			G_OBJECT( instance ), IDUPLICABLE_SIGNAL_VALID_CHANGED, G_CALLBACK( on_object_valid_status_changed ));

	/* monitors the reloading of the tree */
	base_window_signal_connect( window,
			G_OBJECT( window ), TREE_SIGNAL_MODIFIED_STATUS_CHANGED, G_CALLBACK( on_tree_view_modified_status_changed ));

	/* monitors the level zero of the tree */
	base_window_signal_connect( window,
			G_OBJECT( window ), TREE_SIGNAL_LEVEL_ZERO_CHANGED, G_CALLBACK( on_tree_view_level_zero_changed ));

	/* monitors the main selection */
	base_window_signal_connect( window,
			G_OBJECT( window ), MAIN_SIGNAL_SELECTION_CHANGED, G_CALLBACK( on_main_selection_changed ));
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

	na_object_free_items( ied->deleted );

	base_window_signal_disconnect( ied->window, ied->modified_handler_id );
	base_window_signal_disconnect( ied->window, ied->valid_handler_id );

	g_free( ied );
	g_object_set_data( G_OBJECT( instance ), VIEW_DATA_IEDITABLE, NULL );
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

	items_view = NACT_TREE_VIEW( g_object_get_data( G_OBJECT( window ), WINDOW_DATA_TREE_VIEW ));

	if( nact_tree_view_are_notify_allowed( items_view )){
		ied = ( IEditableData * ) g_object_get_data( G_OBJECT( items_view ), VIEW_DATA_IEDITABLE );
		path = gtk_tree_path_new_from_string( path_str );
		object = nact_tree_model_object_at_path( ied->model, path );
		na_object_set_label( object, text );

		g_signal_emit_by_name( window, MAIN_SIGNAL_ITEM_UPDATED, object );
	}
}

static void
on_main_selection_changed( BaseWindow *window, GList *selected_items, gpointer user_data )
{
	NAObject *object;
	gboolean object_editable;
	NactTreeView *items_view;
	IEditableData *ied;
	GtkTreeViewColumn *column;
	GList *renderers;
	gboolean editable;

	g_object_get( window, MAIN_PROP_ITEM, &object, MAIN_PROP_EDITABLE, &object_editable, NULL );
	editable = NA_IS_OBJECT( object ) && object_editable;
	items_view = NACT_TREE_VIEW( g_object_get_data( G_OBJECT( window ), WINDOW_DATA_TREE_VIEW ));
	ied = ( IEditableData * ) g_object_get_data( G_OBJECT( items_view ), VIEW_DATA_IEDITABLE );
	column = gtk_tree_view_get_column( ied->treeview, TREE_COLUMN_LABEL );
	renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( column ));
	g_object_set( G_OBJECT( renderers->data ), "editable", editable, "editable-set", TRUE, NULL );
}

/*
 * modification status of an edited object has changed
 * - refresh the display
 */
static void
on_object_modified_status_changed( NactTreeIEditable *instance, NAObject *object, gboolean is_modified, BaseWindow *window )
{
	static const gchar *thisfn = "nact_tree_ieditable_on_object_modified_status_changed";
	IEditableData *ied;
	gboolean prev_status, status;

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){
		g_debug( "%s: instance=%p, object=%p (%s), is_modified=%s, window=%p",
				thisfn, ( void * ) instance,
				( void * ) object, G_OBJECT_TYPE_NAME( object ), is_modified ? "True":"False",
				( void * ) window );

		ied = ( IEditableData * ) g_object_get_data( G_OBJECT( instance ), VIEW_DATA_IEDITABLE );
		prev_status = get_modification_status( ied );
		gtk_tree_model_filter_refilter( GTK_TREE_MODEL_FILTER( ied->model ));

		if( NA_IS_OBJECT_ITEM( object )){
			if( is_modified ){
				ied->count_modified += 1;
			} else {
				ied->count_modified -= 1;
			}
		}

		if( nact_tree_view_are_notify_allowed( NACT_TREE_VIEW( instance ))){
			status = get_modification_status( ied );
			if( status != prev_status ){
				g_signal_emit_by_name( window, TREE_SIGNAL_MODIFIED_STATUS_CHANGED, status );
			}
		}
	}
}

/*
 * validity status of the edited object has changed
 * - refresh the display
 */
static void
on_object_valid_status_changed( NactTreeIEditable *instance, NAObject *object, gboolean new_status, BaseWindow *window )
{
	static const gchar *thisfn = "nact_tree_ieditable_on_object_valid_status_changed";
	IEditableData *ied;

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){
		g_debug( "%s: instance=%p, new_status=%s, window=%p",
				thisfn, ( void * ) instance, new_status ? "True":"False", ( void * ) window );

		ied = ( IEditableData * ) g_object_get_data( G_OBJECT( instance ), VIEW_DATA_IEDITABLE );
		gtk_tree_model_filter_refilter( GTK_TREE_MODEL_FILTER( ied->model ));
	}
}

/*
 * order or list of items at level zero of the tree has changed
 */
static void
on_tree_view_level_zero_changed( BaseWindow *window, gboolean is_modified, gpointer user_data )
{
	NactTreeView *items_view;
	IEditableData *ied;
	gboolean prev_status, status;

	items_view = NACT_TREE_VIEW( g_object_get_data( G_OBJECT( window ), WINDOW_DATA_TREE_VIEW ));
	ied = get_instance_data( NACT_TREE_IEDITABLE( items_view ));
	prev_status = get_modification_status( ied );
	ied->level_zero_changed = is_modified;

	if( nact_tree_view_are_notify_allowed( items_view )){
		status = get_modification_status( ied );
		if( prev_status != status ){
			g_signal_emit_by_name( window, TREE_SIGNAL_MODIFIED_STATUS_CHANGED, status );
		}
	}
}

/*
 * the tree has been reloaded
 */
static void
on_tree_view_modified_status_changed( BaseWindow *window, gboolean is_modified, gpointer user_data )
{
	NactTreeView *items_view;
	IEditableData *ied;

	if( !is_modified ){
		items_view = NACT_TREE_VIEW( g_object_get_data( G_OBJECT( window ), WINDOW_DATA_TREE_VIEW ));
		ied = get_instance_data( NACT_TREE_IEDITABLE( items_view ));
		ied->count_modified = 0;
		ied->deleted = na_object_free_items( ied->deleted );
		ied->level_zero_changed = FALSE;
	}
}

/**
 * nact_tree_ieditable_delete:
 * @instance: this #NactTreeIEditable instance.
 * @list: list of #NAObject to be deleted.
 * @ope: whether the objects are actually to be deleted, or just moved
 *  to another path of the tree.
 *
 * Deletes the specified list from the underlying tree store.
 *
 * If the items are to be actually deleted, then this function takes care
 * of repositionning a new selection if possible, and refilter the display
 * model.
 * Deleted NAObjectItems are added to the 'deleted' list attached to the
 * view, so that they will be actually deleted from the storage subsystem
 * on save.
 * Deleted NAObjectProfiles are not added to the list, because the deletion
 * will be recorded when saving the action.
 *
 * If the items are to be actually moved to another path of the tree, then
 * neither the 'deleted' list nor the 'count_modified' counter are modified.
 */
void
nact_tree_ieditable_delete( NactTreeIEditable *instance, GList *items, TreeIEditableDeleteOpe ope )
{
	static const gchar *thisfn = "nact_tree_ieditable_delete";
	IEditableData *ied;
	gboolean prev_status, status;
	GtkTreePath *path;
	GList *it;
	NAObjectItem *parent;

	g_return_if_fail( NACT_IS_TREE_IEDITABLE( instance ));

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){
		g_debug( "%s: instance=%p, items=%p (count=%d), ope=%u",
				thisfn, ( void * ) instance, ( void * ) items, g_list_length( items ), ope );

		nact_tree_view_set_notify_allowed( NACT_TREE_VIEW( instance ), FALSE );
		ied = get_instance_data( instance );
		prev_status = get_modification_status( ied );

		decrement_counters( instance, ied, items );
		path = NULL;

		for( it = items ; it ; it = it->next ){
			if( path ){
				gtk_tree_path_free( path );
			}

			/* check the status of the parent of the deleted object
			 * so potentially update the modification status
			 */
			parent = na_object_get_parent( it->data );
			path = nact_tree_model_delete( ied->model, NA_OBJECT( it->data ));
			if( parent ){
				na_object_check_status( parent );
			} else {
				g_signal_emit_by_name( ied->window, TREE_SIGNAL_LEVEL_ZERO_CHANGED, TRUE );
			}

			/* record the deleted item in the 'deleted' list,
			 * incrementing the 'modified' count if the deleted item was not yet modified
			 */
			if( ope == TREE_OPE_DELETE ){
				add_to_deleted_rec( ied, NA_OBJECT( it->data ));
			}

			g_debug( "%s: object=%p (%s, ref_count=%d)", thisfn,
					( void * ) it->data, G_OBJECT_TYPE_NAME( it->data ), G_OBJECT( it->data )->ref_count );
		}

		gtk_tree_model_filter_refilter( GTK_TREE_MODEL_FILTER( ied->model ));

		nact_tree_view_set_notify_allowed( NACT_TREE_VIEW( instance ), TRUE );

		if( path ){
			if( ope == TREE_OPE_DELETE ){
				nact_tree_view_select_row_at_path( NACT_TREE_VIEW( instance ), path );
			}
			gtk_tree_path_free( path );
		}

		status = get_modification_status( ied );
		if( status != prev_status ){
			g_signal_emit_by_name( G_OBJECT( ied->window ), TREE_SIGNAL_MODIFIED_STATUS_CHANGED, status );

		}
	}
}

/**
 * add_to_deleted_rec:
 * @list: list of deleted objects.
 * @object: the object to be added from the list.
 *
 * Recursively adds to the 'deleted' list the NAObjectItem object currently being deleted.
 *
 * Increments the 'modified' count with the deleted items which were not modified.
 */
static void
add_to_deleted_rec( IEditableData *ied, NAObject *object )
{
	GList *subitems, *it;

	if( NA_IS_OBJECT_ITEM( object )){

		if( !g_list_find( ied->deleted, object )){
			ied->deleted = g_list_prepend( ied->deleted, object );

			if( na_object_is_modified( object )){
				ied->count_modified -= 1;
			}
		}

		if( NA_IS_OBJECT_MENU( object )){
			subitems = na_object_get_items( object );
			for( it = subitems ; it ; it = it->next ){
				add_to_deleted_rec( ied, it->data );
			}
		}
	}
}

/*
 * update the total count of elements
 */
static void
decrement_counters( NactTreeIEditable *view, IEditableData *ied, GList *items )
{
	static const gchar *thisfn = "nact_tree_editable_decrement_counters";
	gint menus, actions, profiles;

	g_debug( "%s: view=%p, ied=%p, items=%p (count=%u)",
			thisfn, ( void * ) view, ( void * ) ied, ( void * ) items, g_list_length( items ));

	na_object_count_items( items, &menus, &actions, &profiles );
	menus *= -1;
	actions *= -1;
	profiles *= -1;
	g_signal_emit_by_name( G_OBJECT( ied->window ), TREE_SIGNAL_COUNT_CHANGED, FALSE, menus, actions, profiles );
}

/**
 * nact_tree_ieditable_remove_deleted:
 * @instance: this #NactTreeIEditable *instance.
 * @messages: a pointer to a #GSList of error messages.
 *
 * Actually removes the deleted items from the underlying I/O storage subsystem.
 *
 * @messages #GSList is only filled up in case of an error has occured.
 * If there is no error (nact_tree_ieditable_remove_deleted() returns %TRUE),
 * then the caller may safely assume that @messages is returned in the
 * same state that it has been provided.
 *
 * Returns: %TRUE if all candidate items have been successfully deleted,
 * %FALSE else.
 */
gboolean
nact_tree_ieditable_remove_deleted( NactTreeIEditable *instance, GSList **messages )
{
	static const gchar *thisfn = "nact_tree_ieditable_remove_deleted";
	gboolean delete_ok;
	IEditableData *ied;
	GList *it;
	NAObjectItem *item;
	GList *not_deleted;
	guint delete_ret;

	g_return_val_if_fail( NACT_IS_TREE_IEDITABLE( instance ), TRUE );

	delete_ok = TRUE;

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){

		ied = get_instance_data( instance );
		not_deleted = NULL;

		for( it = ied->deleted ; it ; it = it->next ){
			item = NA_OBJECT_ITEM( it->data );
			g_debug( "%s: item=%p (%s)", thisfn, ( void * ) item, G_OBJECT_TYPE_NAME( item ));
			na_object_dump_norec( item );

			delete_ret = na_updater_delete_item( ied->updater, item, messages );
			delete_ok = ( delete_ret == NA_IIO_PROVIDER_CODE_OK );

			if( !delete_ok ){
				not_deleted = g_list_prepend( not_deleted, na_object_ref( item ));

#if 0
			} else {
				origin = ( NAObject * ) na_object_get_origin( item );
				if( origin ){
					na_updater_remove_item( updater, origin );
					g_object_unref( origin );
				}
#endif
			}
		}

		ied->deleted = na_object_free_items( ied->deleted );

		/* items that we cannot delete are reinserted in the tree view
		 * in the state they were when they were deleted
		 * (i.e. possibly modified)
		 */
		if( not_deleted ){
			nact_tree_ieditable_insert_items( instance, not_deleted, NULL );
			na_object_free_items( not_deleted );
		}
	}

	return( delete_ok );
}

/**
 * nact_tree_ieditable_get_deleted:
 * @instance: this #NactTreeIEditable *instance.
 *
 * Returns: a copy of the 'deleted' list, which should be na_object_free_items()
 * by the caller.
 */
GList *
nact_tree_ieditable_get_deleted( NactTreeIEditable *instance )
{
	GList *deleted;
	IEditableData *ied;

	g_return_val_if_fail( NACT_IS_TREE_IEDITABLE( instance ), NULL );

	deleted = NULL;

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){

		ied = get_instance_data( instance );
		deleted = na_object_copyref_items( ied->deleted );
	}

	return( deleted );
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
	g_return_if_fail( items );

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
	GtkTreePath *actual_path;
	gboolean prev_status, status;
	NAObjectItem *parent;
	GList *it;

	g_return_if_fail( NACT_IS_TREE_IEDITABLE( instance ));

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){
		g_debug( "%s: instance=%p, items=%p (count=%d)",
			thisfn, ( void * ) instance, ( void * ) items, g_list_length( items ));

		nact_tree_view_set_notify_allowed( NACT_TREE_VIEW( instance ), FALSE );
		ied = get_instance_data( instance );
		prev_status = get_modification_status( ied );

		actual_path = do_insert_before( ied, items, insert_path );

		parent = na_object_get_parent( items->data );
		if( parent ){
			na_object_check_status( parent );
		} else {
			for( it = items ; it ; it = it->next ){
				na_object_check_status( it->data );
			}
			g_signal_emit_by_name( ied->window, TREE_SIGNAL_LEVEL_ZERO_CHANGED, TRUE );
		}

		/* post insertion
		 */
		status = get_modification_status( ied );
		if( prev_status != status ){
			g_signal_emit_by_name( ied->window, TREE_SIGNAL_MODIFIED_STATUS_CHANGED, status );
		}
		nact_tree_view_set_notify_allowed( NACT_TREE_VIEW( instance ), TRUE );

		increment_counters( instance, ied, items );
		gtk_tree_model_filter_refilter( GTK_TREE_MODEL_FILTER( ied->model ));
		nact_tree_view_select_row_at_path( NACT_TREE_VIEW( instance ), actual_path );
		gtk_tree_path_free( actual_path );
	}
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
	NAObjectItem *parent;

	g_return_if_fail( NACT_IS_TREE_IEDITABLE( instance ));

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){
		g_debug( "%s: instance=%p, items=%p (count=%d)",
			thisfn, ( void * ) instance, ( void * ) items, g_list_length( items ));

		ied = get_instance_data( instance );
		insert_path = get_selection_first_path( ied->treeview );

		new_path = do_insert_into( ied, items, insert_path );

		parent = na_object_get_parent( items->data );
		na_object_check_status( parent );

		/* post insertion
		 */
		increment_counters( instance, ied, items );
		gtk_tree_model_filter_refilter( GTK_TREE_MODEL_FILTER( ied->model ));
		nact_tree_view_select_row_at_path( NACT_TREE_VIEW( instance ), new_path );
		gtk_tree_path_free( new_path );
		gtk_tree_path_free( insert_path );
	}
}

/*
 * inserting a list of objects at path is:
 * - for each item starting from the end, do
 *   > insert the item at path
 *   > recursively insert item children into the insertion path
 *
 * Returns the actual insertion path suitable for the next selection.
 * This path should be gtk_tree_path_free() by the caller.
 */
static GtkTreePath *
do_insert_before( IEditableData *ied, GList *items, GtkTreePath *asked_path )
{
	static const gchar *thisfn = "nact_tree_ieditable_do_insert_before";
	GList *reversed;
	GList *it;
	GtkTreePath *path;
	GtkTreePath *actual_path;

	reversed = g_list_reverse( items );
	actual_path = NULL;

	for( it = reversed ; it ; it = it->next ){
		if( actual_path ){
			path = gtk_tree_path_copy( actual_path );
			gtk_tree_path_free( actual_path );
		} else {
			path = gtk_tree_path_copy( asked_path );
		}
		actual_path = nact_tree_model_insert_before( ied->model, NA_OBJECT( it->data ), path );
		gtk_tree_path_free( path );
		g_debug( "%s: object=%p (%s, ref_count=%d)", thisfn,
				( void * ) it->data, G_OBJECT_TYPE_NAME( it->data ), G_OBJECT( it->data )->ref_count );

		/* recursively insert subitems
		 */
		if( NA_IS_OBJECT_ITEM( it->data )){
			path = do_insert_into( ied, na_object_get_items( it->data ), actual_path );
			gtk_tree_path_free( path );
		}
	}

	/* an l-value here is useless, but makes gcc happy */
	items = g_list_reverse( reversed );

	return( actual_path );
}

/*
 * inserting a list of objects into an object is as:
 * - inserting the last child into the object
 * - then inserting other children just before the last insertion path
 *
 * Returns the actual insertion path suitable for the next selection.
 * This path should be gtk_tree_path_free() by the caller.
 */
static GtkTreePath *
do_insert_into( IEditableData *ied, GList *items, GtkTreePath *asked_path )
{
	GList *copy;
	GList *last;
	GtkTreePath *actual_path;
	GtkTreePath *insertion_path;

	actual_path = NULL;

	if( items ){
		copy = g_list_copy( items );
		last = g_list_last( copy );
		copy = g_list_remove_link( copy, last );

		/* insert the last child into the wished path
		 * and recursively all its children
		 */
		actual_path = nact_tree_model_insert_into( ied->model, NA_OBJECT( last->data ), asked_path );
		gtk_tree_view_expand_to_path( ied->treeview, actual_path );

		if( NA_IS_OBJECT_ITEM( last->data )){
			insertion_path = do_insert_into( ied, na_object_get_items( last->data ), actual_path );
			gtk_tree_path_free( insertion_path );
		}

		/* then insert other objects
		 */
		if( copy ){
			insertion_path = do_insert_before( ied, copy, actual_path );
			gtk_tree_path_free( actual_path );
			actual_path = insertion_path;
			g_list_free( copy );
		}
	}

	return( actual_path );
}

/*
 * we pass here after each insertion operation (apart initial fill)
 */
static void
increment_counters( NactTreeIEditable *view, IEditableData *ied, GList *items )
{
	static const gchar *thisfn = "nact_tree_ieditable_increment_counters";
	gint menus, actions, profiles;

	g_debug( "%s: view=%p, ied=%p, items=%p (count=%d)",
			thisfn, ( void * ) view, ( void * ) ied, ( void * ) items, items ? g_list_length( items ) : 0 );

	na_object_count_items( items, &menus, &actions, &profiles );
	g_signal_emit_by_name( G_OBJECT( ied->window ), TREE_SIGNAL_COUNT_CHANGED, FALSE, menus, actions, profiles );
}

/**
 * nact_tree_ieditable_dump_modified:
 * @instance: this #NactTreeIEditable *instance.
 *
 * Dump informations about modified items.
 */
void
nact_tree_ieditable_dump_modified( const NactTreeIEditable *instance )
{
	static const gchar *thisfn = "nact_tree_ieditable_dump_modified";
	IEditableData *ied;

	g_return_if_fail( NACT_IS_TREE_IEDITABLE( instance ));

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){

		ied = get_instance_data(( NactTreeIEditable * ) instance );

		g_debug( "%s:      count_deleted=%u", thisfn, g_list_length( ied->deleted ));
		g_debug( "%s:     count_modified=%u", thisfn, ied->count_modified );
		g_debug( "%s: level_zero_changed=%s", thisfn, ied->level_zero_changed ? "True":"False" );
	}
}

/**
 * nact_tree_ieditable_is_level_zero_modified:
 * @instance: this #NactTreeIEditable *instance.
 *
 * Returns: %TRUE if the level zero must be saved, %FALSE else.
 */
gboolean
nact_tree_ieditable_is_level_zero_modified( const NactTreeIEditable *instance )
{
	IEditableData *ied;
	gboolean is_modified;

	g_return_val_if_fail( NACT_IS_TREE_IEDITABLE( instance ), FALSE );

	is_modified = FALSE;

	if( st_tree_ieditable_initialized && !st_tree_ieditable_finalized ){

		ied = get_instance_data(( NactTreeIEditable * ) instance );
		is_modified = ied->level_zero_changed;
	}

	return( is_modified );
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

static gboolean
get_modification_status( IEditableData *ied )
{
	gboolean modified;

	modified = ( ied->count_modified > 0 ) ||
				( g_list_length( ied->deleted ) > 0 ) || ( ied->level_zero_changed );

	return( modified );
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

/*
 * triggered by 'F2' key
 * let the label be edited
 */
static void
inline_edition( BaseWindow *window )
{
	NactTreeView *items_view;
	IEditableData *ied;
	GtkTreeSelection *selection;
	GList *listrows;
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeViewColumn *column;

	items_view = NACT_TREE_VIEW( g_object_get_data( G_OBJECT( window ), WINDOW_DATA_TREE_VIEW ));
	ied = ( IEditableData * ) g_object_get_data( G_OBJECT( items_view ), VIEW_DATA_IEDITABLE );
	selection = gtk_tree_view_get_selection( ied->treeview );
	listrows = gtk_tree_selection_get_selected_rows( selection, &model );

	if( g_list_length( listrows ) == 1 ){
		path = ( GtkTreePath * ) listrows->data;
		column = gtk_tree_view_get_column( ied->treeview, TREE_COLUMN_LABEL );
		gtk_tree_view_set_cursor( ied->treeview, path, column, TRUE );
	}

	g_list_foreach( listrows, ( GFunc ) gtk_tree_path_free, NULL );
	g_list_free( listrows );
}
