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

#include <string.h>

#include <common/na-object-api.h>
#include <common/na-obj-action.h>
#include <common/na-obj-menu.h>
#include <common/na-obj-profile.h>
#include <common/na-iprefs.h>
#include <common/na-utils.h>

#include "egg-tree-multi-dnd.h"
#include "nact-iactions-list.h"
#include "nact-clipboard.h"
#include "nact-tree-model.h"

/*
 * call once egg_tree_multi_drag_add_drag_support( treeview ) at init time (before gtk_main)
 *
 * when we start with drag
 * 	 call once egg_tree_multi_dnd_on_button_press_event( treeview, event, drag_source )
 *   call many egg_tree_multi_dnd_on_motion_event( treeview, event, drag_source )
 *     until mouse quits the selected area
 *
 * as soon as mouse has quitted the selected area
 *   call once egg_tree_multi_dnd_stop_drag_check( treeview )
 *   call once nact_tree_model_imulti_drag_source_row_draggable: drag_source=0x92a0d70, path_list=0x9373c90
 *   call once nact_clipboard_on_drag_begin( treeview, context, main_window )
 *
 * when we drop (e.g. in Nautilus)
 *   call once egg_tree_multi_drag_drag_data_get( treeview, context, selection_data, info=0, time )
 *   call once nact_tree_model_imulti_drag_source_drag_data_get( drag_source, context, selection_data, path_list, atom=XdndDirectSave0 )
 *   call once nact_tree_model_idrag_dest_drag_data_received
 *   call once nact_clipboard_on_drag_end( treeview, context, main_window )
 */

/* private class data
 */
struct NactTreeModelClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NactTreeModelPrivate {
	gboolean     dispose_has_run;
	BaseWindow  *window;
	GtkTreeView *treeview;
	guint        count;
	gboolean     have_dnd;
	gchar       *drag_dest_uri;
	GList       *drag_items;
};

#define MAX_XDS_ATOM_VAL_LEN			4096
#define TEXT_ATOM						gdk_atom_intern( "text/plain", FALSE )
#define XDS_ATOM						gdk_atom_intern( "XdndDirectSave0", FALSE )
#define XDS_FILENAME					"xds.txt"

enum {
	NACT_XCHANGE_FORMAT_NACT = 0,
	NACT_XCHANGE_FORMAT_XDS,
	NACT_XCHANGE_FORMAT_APPLICATION_XML,
	NACT_XCHANGE_FORMAT_TEXT_PLAIN,
	NACT_XCHANGE_FORMAT_URI_LIST
};

/* as a dnd source, we provide
 * - a special XdndNautilusAction format for internal move/copy
 * - a XdndDirectSave, suitable for exporting to a file manager
 *   (note that Nautilus recognized the "XdndDirectSave0" format as XDS
 *   protocol)
 * - a text (xml) format, to go to clipboard or a text editor
 */
static GtkTargetEntry dnd_source_formats[] = {
	{ "XdndNautilusActions", GTK_TARGET_SAME_WIDGET, NACT_XCHANGE_FORMAT_NACT },
	{ "XdndDirectSave0",     GTK_TARGET_OTHER_APP,   NACT_XCHANGE_FORMAT_XDS },
	{ "application/xml",     GTK_TARGET_OTHER_APP,   NACT_XCHANGE_FORMAT_APPLICATION_XML },
	{ "text/plain",          GTK_TARGET_OTHER_APP,   NACT_XCHANGE_FORMAT_TEXT_PLAIN },
};

#define NACT_ATOM						gdk_atom_intern( "XdndNautilusActions", FALSE )

static GtkTargetEntry dnd_dest_targets[] = {
	{ "XdndNautilusActions", 0, NACT_XCHANGE_FORMAT_NACT },
	{ "text/uri-list",       0, NACT_XCHANGE_FORMAT_URI_LIST },
	{ "application/xml",     0, NACT_XCHANGE_FORMAT_APPLICATION_XML },
	{ "text/plain",          0, NACT_XCHANGE_FORMAT_TEXT_PLAIN },
};

typedef struct {
	gchar *fname;
	gchar *prefix;
}
	ntmDumpStruct;

typedef struct {
	GtkTreeModel   *store;
	const NAObject *object;
	gboolean        found;
	GtkTreeIter    *iter;
}
	ntmSearchStruct;

static GtkTreeModelFilterClass *st_parent_class = NULL;

static GType          register_type( void );
static void           class_init( NactTreeModelClass *klass );
static void           imulti_drag_source_init( EggTreeMultiDragSourceIface *iface );
static void           idrag_dest_init( GtkTreeDragDestIface *iface );
static void           instance_init( GTypeInstance *instance, gpointer klass );
static void           instance_dispose( GObject *application );
static void           instance_finalize( GObject *application );

static NactTreeModel *tree_model_new( BaseWindow *window, GtkTreeView *treeview );

static void           append_item( GtkTreeStore *model, GtkTreeView *treeview, GtkTreeIter *parent, GtkTreeIter *iter, const NAObject *object );
static void           display_item( GtkTreeStore *model, GtkTreeView *treeview, GtkTreeIter *iter, const NAObject *object );
static gboolean       dump_store( NactTreeModel *model, GtkTreePath *path, NAObject *object, ntmDumpStruct *ntm );
static void           fill_tree_store( GtkTreeStore *model, GtkTreeView *treeview, GList *items, gboolean only_actions, GtkTreeIter *parent );
static void           iter_on_store( NactTreeModel *model, GtkTreeModel *store, GtkTreeIter *parent, FnIterOnStore fn, gpointer user_data );
static gboolean       iter_on_store_item( NactTreeModel *model, GtkTreeModel *store, GtkTreeIter *iter, FnIterOnStore fn, gpointer user_data );
/*static gboolean       search_for_object( NactTreeModel *model, GtkTreeModel *store, const NAObject *object, GtkTreeIter *iter );
static gboolean       search_for_objet_iter( NactTreeModel *model, GtkTreePath *path, NAObject *object, ntmSearchStruct *ntm );*/

static gboolean       imulti_drag_source_row_draggable( EggTreeMultiDragSource *drag_source, GList *path_list );
static gboolean       imulti_drag_source_drag_data_get( EggTreeMultiDragSource *drag_source, GdkDragContext *context, GtkSelectionData *selection_data, GList *path_list, guint info );
static gboolean       imulti_drag_source_drag_data_delete( EggTreeMultiDragSource *drag_source, GList *path_list );
static GtkTargetList *imulti_drag_source_get_target_list( EggTreeMultiDragSource *drag_source );
static GdkDragAction  imulti_drag_source_get_drag_actions( EggTreeMultiDragSource *drag_source );

static gboolean       idrag_dest_drag_data_received( GtkTreeDragDest *drag_dest, GtkTreePath *dest, GtkSelectionData  *selection_data );
static gboolean       idrag_dest_row_drop_possible( GtkTreeDragDest *drag_dest, GtkTreePath *dest_path, GtkSelectionData *selection_data );

static gboolean       on_drag_begin( GtkWidget *widget, GdkDragContext *context, BaseWindow *window );
static void           on_drag_end( GtkWidget *widget, GdkDragContext *context, BaseWindow *window );
static void           on_row_deleted( GtkTreeModel *tree_model, GtkTreePath *path, NactTreeModel *model );
static void           on_row_inserted( GtkTreeModel *tree_model, GtkTreePath *path, GtkTreeIter *iter, NactTreeModel *model );

/*static gboolean       on_drag_drop( GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, BaseWindow *window );
static void           on_drag_data_received( GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y, GtkSelectionData *data, guint info, guint time, BaseWindow *window );*/

static gint           sort_actions_list( GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, BaseWindow *window );
static gboolean       filter_visible( GtkTreeModel *model, GtkTreeIter *iter, NactIActionsList *window );
static char          *get_xds_atom_value( GdkDragContext *context );

GType
nact_tree_model_get_type( void )
{
	static GType model_type = 0;

	if( !model_type ){
		model_type = register_type();
	}

	return( model_type );
}

static GType
register_type (void)
{
	static const gchar *thisfn = "nact_tree_model_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NactTreeModelClass ),
		NULL,							/* base_init */
		NULL,							/* base_finalize */
		( GClassInitFunc ) class_init,
		NULL,							/* class_finalize */
		NULL,							/* class_data */
		sizeof( NactTreeModel ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	static const GInterfaceInfo imulti_drag_source_info = {
		( GInterfaceInitFunc ) imulti_drag_source_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo idrag_dest_info = {
		( GInterfaceInitFunc ) idrag_dest_init,
		NULL,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( GTK_TYPE_TREE_MODEL_FILTER, "NactTreeModel", &info, 0 );

	g_type_add_interface_static( type, EGG_TYPE_TREE_MULTI_DRAG_SOURCE, &imulti_drag_source_info );

	g_type_add_interface_static( type, GTK_TYPE_TREE_DRAG_DEST, &idrag_dest_info );

	return( type );
}

static void
class_init( NactTreeModelClass *klass )
{
	static const gchar *thisfn = "nact_tree_model_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactTreeModelClassPrivate, 1 );
}

static void
imulti_drag_source_init( EggTreeMultiDragSourceIface *iface )
{
	static const gchar *thisfn = "nact_tree_model_imulti_drag_source_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->row_draggable = imulti_drag_source_row_draggable;
	iface->drag_data_get = imulti_drag_source_drag_data_get;
	iface->drag_data_delete = imulti_drag_source_drag_data_delete;
	iface->get_target_list = imulti_drag_source_get_target_list;
	iface->free_target_list = NULL;
	iface->get_drag_actions = imulti_drag_source_get_drag_actions;
}

static void
idrag_dest_init( GtkTreeDragDestIface *iface )
{
	static const gchar *thisfn = "nact_tree_model_idrag_dest_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->drag_data_received = idrag_dest_drag_data_received;
	iface->row_drop_possible = idrag_dest_row_drop_possible;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_tree_model_instance_init";
	NactTreeModel *self;

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );
	g_assert( NACT_IS_TREE_MODEL( instance ));
	self = NACT_TREE_MODEL( instance );

	self->private = g_new0( NactTreeModelPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->count = 0;
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "nact_tree_model_instance_dispose";
	NactTreeModel *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_assert( NACT_IS_TREE_MODEL( object ));
	self = NACT_TREE_MODEL( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "nact_tree_model_instance_finalize";
	NactTreeModel *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_assert( NACT_IS_TREE_MODEL( object ));
	self = NACT_TREE_MODEL( object );

	g_free( self->private->drag_dest_uri );
	g_list_free( self->private->drag_items );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/*
 * tree_model_new:
 * @window: a #BaseWindow window which must implement #NactIActionsList
 * interface.
 * @treeview: the #GtkTreeView widget.
 *
 * Creates a new #NactTreeModel model.
 *
 * This function should be called at widget initial load time. Is is so
 * too soon to make any assumption about sorting in the tree view.
 */
static NactTreeModel *
tree_model_new( BaseWindow *window, GtkTreeView *treeview )
{
	static const gchar *thisfn = "nact_tree_model_new";
	GtkTreeStore *ts_model;
	NactTreeModel *model;
	/*gboolean       alpha_order;*/

	g_debug( "%s: window=%p, treeview=%p", thisfn, ( void * ) window, ( void * ) treeview );
	g_return_val_if_fail( BASE_IS_WINDOW( window ), NULL );
	g_return_val_if_fail( NACT_IS_IACTIONS_LIST( window ), NULL );
	g_return_val_if_fail( GTK_IS_TREE_VIEW( treeview ), NULL );

	ts_model = gtk_tree_store_new(
			IACTIONS_LIST_N_COLUMN, GDK_TYPE_PIXBUF, G_TYPE_STRING, NA_OBJECT_TYPE );

	gtk_tree_sortable_set_default_sort_func(
			GTK_TREE_SORTABLE( ts_model ),
	        ( GtkTreeIterCompareFunc ) sort_actions_list, window, NULL );

	/*alpha_order = na_iprefs_get_alphabetical_order( NA_IPREFS( window ));

	if( alpha_order ){
		gtk_tree_sortable_set_sort_column_id(
				GTK_TREE_SORTABLE( ts_model ),
				IACTIONS_LIST_LABEL_COLUMN, GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID );
	}*/

	/* create the filter model
	 */
	model = g_object_new( NACT_TREE_MODEL_TYPE, "child-model", ts_model, NULL );

	gtk_tree_model_filter_set_visible_func(
			GTK_TREE_MODEL_FILTER( model ), ( GtkTreeModelFilterVisibleFunc ) filter_visible, window, NULL );

	model->private->window = window;
	model->private->treeview = treeview;

	return( model );
}

/**
 * nact_tree_model_initial_load:
 * @window: the #BaseWindow window.
 * @widget: the #GtkTreeView which will implement the #NactTreeModel.
 *
 * Creates a #NactTreeModel, and attaches it to the treeview.
 *
 * Please note that we cannot make any assumption here whether the
 * treeview, and so the tree model, must or not implement the drag and
 * drop interfaces.
 * This is because #NactIActionsList::on_initial_load() initializes these
 * properties to %FALSE. The actual values will be set by the main
 * program between #NactIActionsList::on_initial_load() returns and call
 * to #NactIActionsList::on_runtime_init().
 */
void
nact_tree_model_initial_load( BaseWindow *window, GtkTreeView *treeview )
{
	static const gchar *thisfn = "nact_tree_model_initial_load";
	NactTreeModel *model;

	g_debug( "%s: window=%p, treeview=%p", thisfn, ( void * ) window, ( void * ) treeview );
	g_return_if_fail( BASE_IS_WINDOW( window ));
	g_return_if_fail( NACT_IS_IACTIONS_LIST( window ));
	g_return_if_fail( GTK_IS_TREE_VIEW( treeview ));

	model = tree_model_new( window, treeview );

	gtk_tree_view_set_model( treeview, GTK_TREE_MODEL( model ));

	g_object_unref( model );
}

/**
 * nact_tree_model_runtime_init_dnd:
 * @model: this #NactTreeModel instance.
 * @have_dnd: whether the tree model must implement drag and drop
 * interfaces.
 *
 * Initializes the tree model.
 *
 * We use drag and drop:
 * - inside of treeview, for duplicating items, or moving items between
 *   menus
 * - from treeview to the outside world (e.g. Nautilus) to export actions
 * - from outside world (e.g. Nautilus) to import actions
 */
void
nact_tree_model_runtime_init( NactTreeModel *model, gboolean have_dnd )
{
	static const gchar *thisfn = "nact_tree_model_runtime_init";
	GtkTreeModel *ts_model;

	g_debug( "%s: model=%p, have_dnd=%s", thisfn, ( void * ) model, have_dnd ? "True":"False" );
	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	if( have_dnd ){
		egg_tree_multi_drag_add_drag_support( EGG_TREE_MULTI_DRAG_SOURCE( model ), model->private->treeview );

		gtk_tree_view_enable_model_drag_dest(
			model->private->treeview,
			dnd_dest_targets, G_N_ELEMENTS( dnd_dest_targets ), GDK_ACTION_COPY | GDK_ACTION_MOVE );

		base_window_signal_connect(
				BASE_WINDOW( model->private->window ),
				G_OBJECT( model->private->treeview ),
				"drag-begin",
				G_CALLBACK( on_drag_begin ));

		base_window_signal_connect(
				BASE_WINDOW( model->private->window ),
				G_OBJECT( model->private->treeview ),
				"drag-end",
				G_CALLBACK( on_drag_end ));

		/*nact_window_signal_connect(
				NACT_WINDOW( window ),
				G_OBJECT( treeview ),
				"drag_drop",
				G_CALLBACK( on_drag_drop ));

		nact_window_signal_connect(
				NACT_WINDOW( window ),
				G_OBJECT( treeview ),
				"drag_data-received",
				G_CALLBACK( on_drag_data_received ));*/
	}

	ts_model = gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model ));

	g_signal_connect(
			G_OBJECT( ts_model ),
			"row-deleted",
			G_CALLBACK( on_row_deleted ),
			model );

	g_signal_connect(
			G_OBJECT( ts_model ),
			"row-inserted",
			G_CALLBACK( on_row_inserted ),
			model );
}

void
nact_tree_model_dispose( NactTreeModel *model )
{
	static const gchar *thisfn = "nact_tree_model_dispose";
	GtkTreeStore *ts_model;

	g_debug( "%s: model=%p", thisfn, ( void * ) model );
	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

#ifdef NA_MAINTAINER_MODE
	nact_tree_model_dump( model );
#endif

	ts_model = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));

	gtk_tree_store_clear( ts_model );
}

void
nact_tree_model_dump( NactTreeModel *model )
{
	static const gchar *thisfn = "nact_tree_model_dump";
	GtkTreeStore *store;
	ntmDumpStruct *ntm;

	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	store = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));

	g_debug( "%s: %s at %p, %s at %p", thisfn,
			G_OBJECT_TYPE_NAME( model ), ( void * ) model, G_OBJECT_TYPE_NAME( store ), ( void * ) store );

	ntm = g_new0( ntmDumpStruct, 1 );
	ntm->fname = g_strdup( thisfn );
	ntm->prefix = g_strdup( "" );

	nact_tree_model_iter( model, ( FnIterOnStore ) dump_store, ntm );
	/*dump_store( GTK_TREE_MODEL( store ), NULL, thisfn, "" );*/

	g_free( ntm->prefix );
	g_free( ntm->fname );
	g_free( ntm );
}

/**
 * nact_tree_model_fill:
 * @model: this #NactTreeModel instance.
 * @ŧreeview: the #GtkTreeView widget.
 * @items: this list of items, usually from #NAPivot, which will be used
 * to fill up the tree store.
 * @only_actions: whether to store only actions, or all items.
 *
 * Fill up the tree store with specified items.
 *
 * We enter with the GSList owned by NAPivot which contains the ordered
 * list of level-zero items. We want have a duplicate of this list in
 * tree store, so that we are able to freely edit it.
 */
void
nact_tree_model_fill( NactTreeModel *model, GList *items, gboolean only_actions)
{
	static const gchar *thisfn = "nact_tree_model_fill";
	GtkTreeStore *ts_model;

	g_debug( "%s: model=%p, items=%p (%d items), only_actions=%s",
			thisfn, ( void * ) model, ( void * ) items, g_list_length( items ), only_actions ? "True":"False" );
	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	ts_model = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));

	gtk_tree_store_clear( ts_model );

	fill_tree_store( ts_model, model->private->treeview, items, only_actions, NULL );
}

static void
fill_tree_store( GtkTreeStore *model, GtkTreeView *treeview,
					GList *items, gboolean only_actions, GtkTreeIter *parent )
{
	/*static const gchar *thisfn = "nact_tree_model_fill_tree_store";*/
	GList *subitems, *it;
	NAObject *object;
	NAObject *duplicate;
	GtkTreeIter iter;

	for( it = items ; it ; it = it->next ){
		object = NA_OBJECT( it->data );
		/*g_debug( "%s: %s at %p", thisfn, G_OBJECT_TYPE_NAME( object ), ( void * ) object );*/

		if( NA_IS_OBJECT_MENU( object )){
			duplicate = object;
			if( !only_actions ){
				duplicate = parent ? g_object_ref( object ) : na_object_duplicate( object );
				append_item( model, treeview, parent, &iter, duplicate );
				g_object_unref( duplicate );
			}
			subitems = na_object_get_items( duplicate );
			fill_tree_store( model, treeview, subitems, only_actions, only_actions ? NULL : &iter );
			na_object_free_items( subitems );
		}

		if( NA_IS_OBJECT_ACTION( object )){
			duplicate = parent ? g_object_ref( object ) : na_object_duplicate( object );
			append_item( model, treeview, parent, &iter, duplicate );
			g_object_unref( duplicate );
			if( !only_actions ){
				subitems = na_object_get_items( duplicate );
				fill_tree_store( model, treeview, subitems, only_actions, &iter );
				na_object_free_items( subitems );
			}
			g_return_if_fail( NA_IS_OBJECT_ACTION( duplicate ));
			g_return_if_fail( na_object_get_items_count( duplicate ) >= 1 );
		}

		if( NA_IS_OBJECT_PROFILE( object )){
			g_assert( !only_actions );
			append_item( model, treeview, parent, &iter, object );
		}
	}
}

/**
 * nact_tree_model_get_items_count:
 * @model: this #NactTreeModel instance.
 *
 * Returns: the total count of rows, whether they are currently visible
 * or not.
 */
guint
nact_tree_model_get_items_count( NactTreeModel *model )
{
	g_return_val_if_fail( NACT_IS_TREE_MODEL( model ), 0 );

	return( model->private->count );
}

/**
 * nact_tree_model_insert:
 * @model: this #NactTreeModel instance.
 * @object: a #NAObject-derived object to be inserted.
 * @path: the #GtkTreePath of the beginning of the current selection,
 * or NULL.
 * @iter: set to the new row
 * @obj_parent: set to the parent or the object itself.
 *
 * Insert new rows starting at the given position.
 */
void
nact_tree_model_insert( NactTreeModel *model, const NAObject *object, GtkTreePath *path, GtkTreeIter *iter, NAObject **obj_parent )
{
	static const gchar *thisfn = "nact_tree_model_insert";
	gchar *path_str;
	GtkTreeModel *store;
	GtkTreeIter sibling;
	GtkTreeIter *parent_iter;
	GtkTreeIter select_iter;
	GtkTreeIter store_iter;
	NAObject *selected;

	path_str = path ? gtk_tree_path_to_string( path ) : NULL;
	g_debug( "%s: model=%p, object=%p (%s), path=%p (%s), iter=%p",
			thisfn, ( void * ) model,
			( void * ) object, G_OBJECT_TYPE_NAME( object ),
			( void * ) path, path_str,
			( void * ) iter );
	g_free( path_str );

	g_return_if_fail( NACT_IS_TREE_MODEL( model ));
	g_return_if_fail( NA_IS_OBJECT( object ));
	g_return_if_fail( iter );

	store = gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model ));
	parent_iter = NULL;
	*obj_parent = NA_OBJECT( object );

	if( path ){
		gtk_tree_model_get_iter( GTK_TREE_MODEL( model ), &select_iter, path );
		gtk_tree_model_get( GTK_TREE_MODEL( model ), &select_iter, IACTIONS_LIST_NAOBJECT_COLUMN, &selected, -1 );

		g_return_if_fail( selected );
		g_return_if_fail( NA_IS_OBJECT( selected ));

		if( NA_IS_OBJECT_ITEM( object )){
			if( !NA_IS_OBJECT_ITEM( selected )){
				gtk_tree_path_up( path );
			}
			gtk_tree_model_get_iter( store, &sibling, path );
			if( NA_IS_OBJECT_MENU( selected )){
				parent_iter = gtk_tree_iter_copy( &sibling );
				na_object_insert_item( selected, object );
				*obj_parent = selected;
			}
		}

		if( NA_IS_OBJECT_PROFILE( object )){
			if( NA_IS_OBJECT_ACTION( selected )){
				/*g_debug( "nact_tree_model_insert_item: object_is_action_profile, selected_is_action" );*/
				na_object_action_attach_profile( NA_OBJECT_ACTION( selected ), NA_OBJECT_PROFILE( object ));
				gtk_tree_model_get_iter( store, &sibling, path );
				parent_iter = gtk_tree_iter_copy( &sibling );
				*obj_parent = selected;
			} else {
				g_return_if_fail( NA_IS_OBJECT_PROFILE( selected ));
				*obj_parent = NA_OBJECT( na_object_profile_get_action( NA_OBJECT_PROFILE( object )));
				gtk_tree_path_down( path );
				gtk_tree_model_get_iter( store, &sibling, path );
			}
		}

		g_object_unref( selected );

	} else {
		g_return_if_fail( NA_IS_OBJECT_ITEM( object ));
	}

	gtk_tree_store_insert_before( GTK_TREE_STORE( store ), &store_iter, parent_iter, parent_iter ? NULL : ( path ? &sibling : NULL ));
	gtk_tree_store_set( GTK_TREE_STORE( store ), &store_iter, IACTIONS_LIST_NAOBJECT_COLUMN, object, -1 );
	display_item( GTK_TREE_STORE( store ), model->private->treeview, &store_iter, object );

	if( parent_iter ){
		gtk_tree_iter_free( parent_iter );
	}

	gtk_tree_model_filter_convert_child_iter_to_iter( GTK_TREE_MODEL_FILTER( model ), iter, &store_iter );
}

#if 0
void
nact_tree_model_insert_item( NactTreeModel *model, const NAObject *object, GtkTreePath *path, const NAObject *selected, GtkTreeIter *iter )
{
	static const gchar *thisfn = "nact_tree_model_insert_item";
	gchar *path_str;
	GtkTreeModel *store;
	GtkTreeIter sibling;
	GtkTreeIter *parent;
	GtkTreeIter store_iter;
	GtkTreeIter profile_iter;
	GList *profiles;

	path_str = path ? gtk_tree_path_to_string( path ) : NULL;
	g_debug( "%s: model=%p, object=%p, path=%p (%s), selected=%p, iter=%p",
			thisfn, ( void * ) model, ( void * ) object, ( void * ) path, path_str, ( void * ) selected, ( void * ) iter );
	g_free( path_str );

	g_return_if_fail( NACT_IS_TREE_MODEL( model ));
	g_return_if_fail( NA_IS_OBJECT( object ));
	g_return_if_fail( iter );

	store = gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model ));
	parent = NULL;

	if( path ){
		g_return_if_fail( selected );
		g_return_if_fail( NA_IS_OBJECT( selected ));

		if( NA_IS_OBJECT_ITEM( object )){
			if( !NA_IS_OBJECT_ITEM( selected )){
				gtk_tree_path_up( path );
			}
			gtk_tree_model_get_iter( store, &sibling, path );
			if( NA_IS_OBJECT_MENU( selected )){
				parent = gtk_tree_iter_copy( &sibling );
				na_object_insert_item( selected, object );
			}
		}

		if( NA_IS_OBJECT_PROFILE( object )){
			if( NA_IS_OBJECT_ACTION( selected )){
				gtk_tree_path_down( path );
				g_debug( "nact_tree_model_insert_item: object_is_action_profile, selected_is_action" );
			}
			gtk_tree_model_get_iter( store, &sibling, path );
		}

	} else {
		g_return_if_fail( NA_IS_OBJECT_ITEM( object ));
	}

	gtk_tree_store_insert_before( GTK_TREE_STORE( store ), &store_iter, parent, parent ? NULL : ( path ? &sibling : NULL ));
	gtk_tree_store_set( GTK_TREE_STORE( store ), &store_iter, IACTIONS_LIST_NAOBJECT_COLUMN, object, -1 );
	display_item( GTK_TREE_STORE( store ), model->private->treeview, &store_iter, object );

	if( parent ){
		gtk_tree_iter_free( parent );
	}

	if( NA_IS_OBJECT_ACTION( object )){
		g_return_if_fail( na_object_get_items_count( object ) == 1 );
		profiles = na_object_get_items( object );
		append_item( GTK_TREE_STORE( store ), model->private->treeview, &store_iter, &profile_iter, NA_OBJECT( profiles->data ));
		na_object_free_items( profiles );
	}

	nact_tree_model_update_parent( model, object );

	gtk_tree_model_filter_refilter( GTK_TREE_MODEL_FILTER( model ));
	gtk_tree_model_filter_convert_child_iter_to_iter( GTK_TREE_MODEL_FILTER( model ), iter, &store_iter );
}
#endif

void
nact_tree_model_iter( NactTreeModel *model, FnIterOnStore fn, gpointer user_data )
{
	GtkTreeStore *store;

	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	store = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));

	iter_on_store( model, GTK_TREE_MODEL( store ), NULL, fn, user_data );
}

/**
 * nact_tree_model_remove:
 * @model: this #NactTreeModel instance.
 * @selected: a list of #GtkTreePath as returned by
 * gtk_tree_selection_get_selected_rows().
 *
 * Deletes the selected rows, unref-ing the underlying objects if any.
 *
 * We begin by the end, so that we are almost sure that path remain
 * valid during the iteration.
 */
void
nact_tree_model_remove( NactTreeModel *model, GList *selected )
{
	GList *reversed, *item;
	GtkTreeIter iter;
	GtkTreeStore *store;
	gchar *path_str;

	reversed = g_list_reverse( selected );
	store = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));

	for( item = reversed ; item ; item = item->next ){

		path_str = gtk_tree_path_to_string(( GtkTreePath * ) item->data );
		g_debug( "nact_tree_model_remove: path=%s", path_str );
		g_free( path_str );

		gtk_tree_model_get_iter( GTK_TREE_MODEL( store ), &iter, ( GtkTreePath * ) item->data );
		gtk_tree_store_remove( store, &iter );
	}
}

static void
append_item( GtkTreeStore *model, GtkTreeView *treeview, GtkTreeIter *parent, GtkTreeIter *iter, const NAObject *object )
{
	gtk_tree_store_append( model, iter, parent );

	gtk_tree_store_set( model, iter, IACTIONS_LIST_NAOBJECT_COLUMN, object, -1 );

	display_item( model, treeview, iter, object );
}

static void
display_item( GtkTreeStore *model, GtkTreeView *treeview, GtkTreeIter *iter, const NAObject *object )
{
	gchar *label = na_object_get_label( object );
	gtk_tree_store_set( model, iter, IACTIONS_LIST_LABEL_COLUMN, label, -1 );
	g_free( label );

	if( NA_IS_OBJECT_ITEM( object )){
		GdkPixbuf *icon = na_object_item_get_pixbuf( NA_OBJECT_ITEM( object ), GTK_WIDGET( treeview ));
		gtk_tree_store_set( model, iter, IACTIONS_LIST_ICON_COLUMN, icon, -1 );
	}
}

static gboolean
dump_store( NactTreeModel *model, GtkTreePath *path, NAObject *object, ntmDumpStruct *ntm )
{
	gint depth;
	gint i;
	GString *prefix;

	depth = gtk_tree_path_get_depth( path );
	prefix = g_string_new( ntm->prefix );
	for( i=1 ; i<depth ; ++i ){
		g_string_append_printf( prefix, "  " );
	}

	g_debug( "%s: %s%s at %p", ntm->fname, prefix->str, G_OBJECT_TYPE_NAME( object ), ( void * ) object );

	g_string_free( prefix, TRUE );

	/* don't stop iteration */
	return( FALSE );
}

static void
iter_on_store( NactTreeModel *model, GtkTreeModel *store, GtkTreeIter *parent, FnIterOnStore fn, gpointer user_data )
{
	GtkTreeIter iter;
	gboolean stop;

	if( gtk_tree_model_iter_children( store, &iter, parent )){
		stop = iter_on_store_item( model, store, &iter, fn, user_data );
		while( !stop && gtk_tree_model_iter_next( store, &iter )){
			stop = iter_on_store_item( model, store, &iter, fn, user_data );
		}
	}
}

static gboolean
iter_on_store_item( NactTreeModel *model, GtkTreeModel *store, GtkTreeIter *iter, FnIterOnStore fn, gpointer user_data )
{
	NAObject *object;
	GtkTreePath *path;
	gboolean stop;

	gtk_tree_model_get( store, iter, IACTIONS_LIST_NAOBJECT_COLUMN, &object, -1 );
	path = gtk_tree_model_get_path( store, iter );

	stop = ( *fn )( model, path, object, user_data );

	gtk_tree_path_free( path );
	g_object_unref( object );

	if( !stop ){
		iter_on_store( model, store, iter, fn, user_data );
	}

	return( stop );
}

/*static gboolean
search_for_object( NactTreeModel *model, GtkTreeModel *store, const NAObject *object, GtkTreeIter *result_iter )
{
	gboolean found = FALSE;
	ntmSearchStruct *ntm;
	GtkTreeIter iter;

	ntm = g_new0( ntmSearchStruct, 1 );
	ntm->store = store;
	ntm->object = object;
	ntm->found = FALSE;
	ntm->iter = &iter;

	iter_on_store( model, store, NULL, ( FnIterOnStore ) search_for_objet_iter, ntm );

	if( ntm->found ){
		found = TRUE;
		memcpy( result_iter, ntm->iter, sizeof( GtkTreeIter ));
	}

	g_free( ntm );
	return( found );
}

static gboolean
search_for_objet_iter( NactTreeModel *model, GtkTreePath *path, NAObject *object, ntmSearchStruct *ntm )
{
	if( object == ntm->object ){
		if( gtk_tree_model_get_iter( ntm->store, ntm->iter, path )){
			ntm->found = TRUE;
		}
	}*/
	/* stop iteration when found */
	/*return( ntm->found );
}*/

/*
 * all rows are draggable
 */
static gboolean
imulti_drag_source_row_draggable( EggTreeMultiDragSource *drag_source, GList *path_list )
{
	static const gchar *thisfn = "nact_tree_model_imulti_drag_source_row_draggable";
	/*FrWindow     *window;
	GtkTreeModel *model;
	GList        *scan;*/

	g_debug( "%s: drag_source=%p, path_list=%p", thisfn, ( void * ) drag_source, ( void * ) path_list );

	/*window = g_object_get_data (G_OBJECT (drag_source), "FrWindow");
	g_return_val_if_fail (window != NULL, FALSE);

	model = fr_window_get_list_store (window);

	for (scan = path_list; scan; scan = scan->next) {
		GtkTreeRowReference *reference = scan->data;
		GtkTreePath         *path;
		GtkTreeIter          iter;
		FileData            *fdata;

		path = gtk_tree_row_reference_get_path (reference);
		if (path == NULL)
			continue;

		if (! gtk_tree_model_get_iter (model, &iter, path))
			continue;

		gtk_tree_model_get (model, &iter,
				    COLUMN_FILE_DATA, &fdata,
				    -1);

		if (fdata != NULL)
			return TRUE;
	}*/

	return( TRUE );
}

/*
 * drag_data_get is called when we release the selected items onto the
 * destination
 *
 * if some rows are selected
 * here, we only provide id. of dragged rows :
 * 		M:uuid
 * 		A:uuid
 * 		P:uuid/name
 * this is suitable and sufficient for the internal clipboard
 *
 * when exporting to the outside, we should prepare to export the items
 */
static gboolean
imulti_drag_source_drag_data_get( EggTreeMultiDragSource *drag_source,
				   GdkDragContext         *context,
				   GtkSelectionData       *selection_data,
				   GList                  *path_list,
				   guint                   info )
{
	static const gchar *thisfn = "nact_tree_model_imulti_drag_source_drag_data_get";
	gchar *atom_name;
	NactTreeModel *model;
	GList *selected_items;
	gchar *data;
	gboolean ret = FALSE;
	gchar *dest_folder, *folder;
	gboolean is_writable;
	gboolean copy_data;

	atom_name = gdk_atom_name( selection_data->target );
	g_debug( "%s: drag_source=%p, context=%p, action=%d, selection_data=%p, path_list=%p, atom=%s",
			thisfn, ( void * ) drag_source, ( void * ) context, ( int ) context->suggested_action, ( void * ) selection_data, ( void * ) path_list,
			atom_name );
	g_free( atom_name );

	model = NACT_TREE_MODEL( drag_source );
	g_assert( model->private->window );

	selected_items = nact_iactions_list_get_selected_items( NACT_IACTIONS_LIST( model->private->window ));
	if( !selected_items ){
		return( FALSE );
	}
	if( !g_list_length( selected_items )){
		g_list_free( selected_items );
		return( FALSE );
	}

	switch( info ){
		case NACT_XCHANGE_FORMAT_NACT:
			copy_data = ( context->suggested_action == GDK_ACTION_COPY );
			nact_clipboard_get_data_for_intern_use( selected_items, copy_data );
			gtk_selection_data_set( selection_data, selection_data->target, 8, ( guchar * ) "", 0 );
			ret = TRUE;
			break;

		case NACT_XCHANGE_FORMAT_XDS:
			folder = get_xds_atom_value( context );
			dest_folder = na_utils_remove_last_level_from_path( folder );
			g_free( folder );
			is_writable = na_utils_is_writable_dir( dest_folder );
			gtk_selection_data_set( selection_data, selection_data->target, 8, ( guchar * )( is_writable ? "S" : "F" ), 1 );
			if( is_writable ){
				model->private->drag_dest_uri = g_strdup( dest_folder );
				model->private->drag_items = g_list_copy( selected_items );
			}
			g_free( dest_folder );
			ret = TRUE;
			break;

		case NACT_XCHANGE_FORMAT_APPLICATION_XML:
		case NACT_XCHANGE_FORMAT_TEXT_PLAIN:
			data = nact_clipboard_get_data_for_extern_use( selected_items );
			gtk_selection_data_set( selection_data, selection_data->target, 8, ( guchar * ) data, strlen( data ));
			g_free( data );
			ret = TRUE;
			break;

		default:
			break;
	}

	g_list_free( selected_items );
	return( ret );
}

static gboolean
imulti_drag_source_drag_data_delete( EggTreeMultiDragSource *drag_source, GList *path_list )
{
	static const gchar *thisfn = "nact_tree_model_imulti_drag_source_drag_data_delete";

	g_debug( "%s: drag_source=%p, path_list=%p", thisfn, ( void * ) drag_source, ( void * ) path_list );

	return( TRUE );
}

static GtkTargetList *
imulti_drag_source_get_target_list( EggTreeMultiDragSource *drag_source )
{
	GtkTargetList *target_list;

	target_list = gtk_target_list_new( dnd_source_formats, G_N_ELEMENTS( dnd_source_formats ));

	return( target_list );
}

static GdkDragAction
imulti_drag_source_get_drag_actions( EggTreeMultiDragSource *drag_source )
{
	return( GDK_ACTION_COPY | GDK_ACTION_MOVE );
}

/*
 * TODO: empty the internal clipboard at drop time
 */
static gboolean
idrag_dest_drag_data_received( GtkTreeDragDest *drag_dest, GtkTreePath *dest, GtkSelectionData  *selection_data )
{
	static const gchar *thisfn = "nact_tree_model_idrag_dest_drag_data_received";

	g_debug( "%s: drag_dest=%p, dest=%p, selection_data=%p", thisfn, ( void * ) drag_dest, ( void * ) dest, ( void * ) selection_data );

	return( FALSE );
}

static gboolean
idrag_dest_row_drop_possible( GtkTreeDragDest *drag_dest, GtkTreePath *dest_path, GtkSelectionData *selection_data )
{
	static const gchar *thisfn = "nact_tree_model_idrag_dest_row_drop_possible";

	g_debug( "%s: drag_dest=%p, dest_path=%p, selection_data=%p", thisfn, ( void * ) drag_dest, ( void * ) dest_path, ( void * ) selection_data );

	return( TRUE );
}

static gboolean
on_drag_begin( GtkWidget *widget, GdkDragContext *context, BaseWindow *window )
{
	static const gchar *thisfn = "nact_clipboard_on_drag_begin";
	NactTreeModel *model;

	g_debug( "%s: widget=%p, context=%p, window=%p",
			thisfn, ( void * ) widget, ( void * ) context, ( void * ) window );

	model = NACT_TREE_MODEL( gtk_tree_view_get_model( GTK_TREE_VIEW( widget )));

	g_free( model->private->drag_dest_uri );
	model->private->drag_dest_uri = NULL;

	g_list_free( model->private->drag_items );
	model->private->drag_items = NULL;

	gdk_property_change(
			context->source_window,
			XDS_ATOM, TEXT_ATOM, 8, GDK_PROP_MODE_REPLACE, ( guchar * ) XDS_FILENAME, strlen( XDS_FILENAME ));

	return( FALSE );
}

static void
on_drag_end( GtkWidget *widget, GdkDragContext *context, BaseWindow *window )
{
	static const gchar *thisfn = "nact_clipboard_on_drag_end";
	NactTreeModel *model;

	g_debug( "%s: widget=%p, context=%p, window=%p",
			thisfn, ( void * ) widget, ( void * ) context, ( void * ) window );

	model = NACT_TREE_MODEL( gtk_tree_view_get_model( GTK_TREE_VIEW( widget )));

	if( model->private->drag_dest_uri && model->private->drag_items && g_list_length( model->private->drag_items )){
		nact_clipboard_export_items( model->private->drag_dest_uri, model->private->drag_items );
	}

	g_free( model->private->drag_dest_uri );
	model->private->drag_dest_uri = NULL;

	g_list_free( model->private->drag_items );
	model->private->drag_items = NULL;

	gdk_property_delete( context->source_window, XDS_ATOM );
}

/*static gboolean
on_drag_drop( GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, BaseWindow *window )
{
	static const gchar *thisfn = "nact_clipboard_on_drag_drop";

	g_debug( "%s: widget=%p, context=%p, x=%d, y=%d, time=%d, window=%p",
			thisfn, ( void * ) widget, ( void * ) context, x, y, time, ( void * ) window );
*/
	/*No!*/
	/*gtk_drag_get_data( widget, context, NACT_ATOM, time );*/

	/* return TRUE is the mouse pointer is on a drop zone, FALSE else */
	/*return( TRUE );
}*/

/*static void
on_drag_data_received( GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y, GtkSelectionData *data, guint info, guint time, BaseWindow *window )
{
	static const gchar *thisfn = "nact_tree_model_on_drag_data_received";

	g_debug( "%s: widget=%p, drag_context=%p, x=%d, y=%d, selection_data=%p, info=%d, time=%d, window=%p",
			thisfn, ( void * ) widget, ( void * ) drag_context, x, y, ( void * ) data, info, time, ( void * ) window );
}*/

static void
on_row_deleted( GtkTreeModel *tree_model, GtkTreePath *path, NactTreeModel *model )
{
	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	model->private->count -= 1;
}

static void
on_row_inserted( GtkTreeModel *tree_model, GtkTreePath *path, GtkTreeIter *iter, NactTreeModel *model )
{
	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	model->private->count += 1;
}

static gint
sort_actions_list( GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, BaseWindow *window )
{
	static const gchar *thisfn = "nact_tree_model_sort_actions_list";
	gchar *labela, *labelb;
	gint ret;

	g_debug( "%s: model=%p, a=%p, b=%p, window=%p", thisfn, ( void * ) model, ( void * ) a, ( void * ) b, ( void * ) window );

	gtk_tree_model_get( model, a, IACTIONS_LIST_LABEL_COLUMN, &labela, -1 );
	gtk_tree_model_get( model, b, IACTIONS_LIST_LABEL_COLUMN, &labelb, -1 );

	ret = g_utf8_collate( labela, labelb );

	g_free( labela );
	g_free( labelb );

	return( ret );
}

static gboolean
filter_visible( GtkTreeModel *model, GtkTreeIter *iter, NactIActionsList *window )
{
	/*static const gchar *thisfn = "nact_tree_model_filter_visible";*/
	NAObject *object;
	NAObjectAction *action;
	gboolean only_actions;

	/*g_debug( "%s: model=%p, iter=%p, window=%p", thisfn, ( void * ) model, ( void * ) iter, ( void * ) window );*/
	/*g_debug( "%s at %p", G_OBJECT_TYPE_NAME( model ), ( void * ) model );*/
	/* is a GtkTreeStore */

	gtk_tree_model_get( model, iter, IACTIONS_LIST_NAOBJECT_COLUMN, &object, -1 );

	if( object ){
		/*na_object_dump( object );*/

		if( NA_IS_OBJECT_ACTION( object )){
			g_object_unref( object );
			return( TRUE );
		}

		only_actions = nact_iactions_list_is_only_actions_mode( window );

		if( !only_actions ){

			if( NA_IS_OBJECT_MENU( object )){
				g_object_unref( object );
				return( TRUE );
			}

			if( NA_IS_OBJECT_PROFILE( object )){
				action = na_object_profile_get_action( NA_OBJECT_PROFILE( object ));
				g_object_unref( object );
				/*return( TRUE );*/
				return( na_object_get_items_count( action ) > 1 );
			}

			g_assert_not_reached();
		}
	}

	return( FALSE );
}

/* The following function taken from bugzilla
 * (http://bugzilla.gnome.org/attachment.cgi?id=49362&action=view)
 * Author: Christian Neumair
 * Copyright: 2005 Free Software Foundation, Inc
 * License: GPL
 */
static char *
get_xds_atom_value (GdkDragContext *context)
{
	char *ret;

	g_return_val_if_fail (context != NULL, NULL);
	g_return_val_if_fail (context->source_window != NULL, NULL);

	gdk_property_get (context->source_window,
						XDS_ATOM, TEXT_ATOM,
						0, MAX_XDS_ATOM_VAL_LEN,
						FALSE, NULL, NULL, NULL,
						(unsigned char **) &ret);

	return ret;
}
