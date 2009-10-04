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
#include <common/na-iprefs.h>
#include <common/na-utils.h>
#include <common/na-xml-writer.h>

#include "egg-tree-multi-dnd.h"
#include "nact-application.h"
#include "nact-iactions-list.h"
#include "nact-main-statusbar.h"
#include "nact-main-window.h"
#include "nact-clipboard.h"
#include "nact-tree-model.h"
#include "nact-xml-reader.h"

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
 *   call once egg_tree_multi_dnd_on_drag_data_get( treeview, context, selection_data, info=0, time )
 *   call once nact_tree_model_imulti_drag_source_drag_data_get( drag_source, context, selection_data, path_list, atom=XdndDirectSave0 )
 *   call once nact_tree_model_idrag_dest_drag_data_received
 *   call once nact_clipboard_on_drag_end( treeview, context, main_window )
 *
 * when we drop in Nautilus Actions
 *   call once egg_tree_multi_dnd_on_drag_data_get( treeview, context, selection_data, info=0, time )
 *   call once nact_tree_model_imulti_drag_source_drag_data_get( drag_source, context, selection_data, path_list, atom=XdndNautilusActions )
 *   call once nact_clipboard_get_data_for_intern_use
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
	gboolean       dispose_has_run;
	BaseWindow    *window;
	GtkTreeView   *treeview;
	gboolean       have_dnd;
	gboolean       only_actions;
	NactClipboard *clipboard;
	gboolean       drag_has_profiles;
	gboolean       drag_highlight;		/* defined for on_drag_motion handler */
	gboolean       drag_drop;			/* defined for on_drag_motion handler */
};

#define MAX_XDS_ATOM_VAL_LEN			4096
#define TEXT_ATOM						gdk_atom_intern( "text/plain", FALSE )
#define XDS_ATOM						gdk_atom_intern( "XdndDirectSave0", FALSE )
#define XDS_FILENAME					"xds.txt"

#define NACT_ATOM						gdk_atom_intern( "XdndNautilusActions", FALSE )

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

/* as a dnd dest, we accept
 * - of course, the same special XdndNautilusAction format for internal move/copy
 * - a list of uris, to be imported
 * - a XML buffer, to be imported
 * - a plain text, which we are goint to try to import as a XML description
 */
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

typedef struct {
	GtkTreeModel *store;
	gchar        *id;
	gboolean      found;
	GtkTreeIter  *iter;
}
	ntmSearchIdStruct;

#define TREE_MODEL_ORDER_MODE			"nact-tree-model-order-mode"

#define TREE_MODEL_STATUSBAR_CONTEXT	"nact-tree-model-statusbar-context"

static GtkTreeModelFilterClass *st_parent_class = NULL;

static GType          register_type( void );
static void           class_init( NactTreeModelClass *klass );
static void           imulti_drag_source_init( EggTreeMultiDragSourceIface *iface );
static void           idrag_dest_init( GtkTreeDragDestIface *iface );
static void           instance_init( GTypeInstance *instance, gpointer klass );
static void           instance_dispose( GObject *application );
static void           instance_finalize( GObject *application );

static NactTreeModel *tree_model_new( BaseWindow *window, GtkTreeView *treeview );

static void           fill_tree_store( GtkTreeStore *model, GtkTreeView *treeview, GList *items, gboolean only_actions, GtkTreeIter *parent );
static void           remove_if_exists( NactTreeModel *model, GtkTreeModel *store, const NAObject *object );

static GList         *add_parent( GList *parents, GtkTreeModel *store, GtkTreeIter *obj_iter );
static void           append_item( GtkTreeStore *model, GtkTreeView *treeview, GtkTreeIter *parent, GtkTreeIter *iter, const NAObject *object );
static void           display_item( GtkTreeStore *model, GtkTreeView *treeview, GtkTreeIter *iter, const NAObject *object );
static gboolean       dump_store( NactTreeModel *model, GtkTreePath *path, NAObject *object, ntmDumpStruct *ntm );
static void           iter_on_store( NactTreeModel *model, GtkTreeModel *store, GtkTreeIter *parent, FnIterOnStore fn, gpointer user_data );
static gboolean       iter_on_store_item( NactTreeModel *model, GtkTreeModel *store, GtkTreeIter *iter, FnIterOnStore fn, gpointer user_data );
static gboolean       remove_items( GtkTreeStore *store, GtkTreeIter *iter );
static gboolean       search_for_object( NactTreeModel *model, GtkTreeModel *store, const NAObject *object, GtkTreeIter *iter );
static gboolean       search_for_objet_iter( NactTreeModel *model, GtkTreePath *path, NAObject *object, ntmSearchStruct *ntm );
static gboolean       search_for_object_id( NactTreeModel *model, GtkTreeModel *store, const NAObject *object, GtkTreeIter *iter );
static gboolean       search_for_object_id_iter( NactTreeModel *model, GtkTreePath *path, NAObject *object, ntmSearchIdStruct *ntm );

static gboolean       imulti_drag_source_row_draggable( EggTreeMultiDragSource *drag_source, GList *path_list );
static gboolean       imulti_drag_source_drag_data_get( EggTreeMultiDragSource *drag_source, GdkDragContext *context, GtkSelectionData *selection_data, GList *path_list, guint info );
static gboolean       imulti_drag_source_drag_data_delete( EggTreeMultiDragSource *drag_source, GList *path_list );
static GtkTargetList *imulti_drag_source_get_target_list( EggTreeMultiDragSource *drag_source );
static GdkDragAction  imulti_drag_source_get_drag_actions( EggTreeMultiDragSource *drag_source );

static gboolean       idrag_dest_drag_data_received( GtkTreeDragDest *drag_dest, GtkTreePath *dest, GtkSelectionData  *selection_data );
static gboolean       idrag_dest_row_drop_possible( GtkTreeDragDest *drag_dest, GtkTreePath *dest_path, GtkSelectionData *selection_data );

static void           on_drag_begin( GtkWidget *widget, GdkDragContext *context, BaseWindow *window );
/*static gboolean       on_drag_motion( GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, BaseWindow *window );*/
/*static gboolean       on_drag_drop( GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, BaseWindow *window );*/
static void           on_drag_end( GtkWidget *widget, GdkDragContext *context, BaseWindow *window );

static char          *get_xds_atom_value( GdkDragContext *context );
static guint          target_atom_to_id( GdkAtom atom );

static gint           sort_actions_list( GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data );
static gboolean       filter_visible( GtkTreeModel *store, GtkTreeIter *iter, NactTreeModel *model );

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
	g_return_if_fail( NACT_IS_TREE_MODEL( instance ));
	self = NACT_TREE_MODEL( instance );

	self->private = g_new0( NactTreeModelPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "nact_tree_model_instance_dispose";
	NactTreeModel *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_return_if_fail( NACT_IS_TREE_MODEL( object ));
	self = NACT_TREE_MODEL( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		g_object_unref( self->private->clipboard );

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
	g_return_if_fail( NACT_IS_TREE_MODEL( object ));
	self = NACT_TREE_MODEL( object );

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
	NactApplication *application;
	NAPivot *pivot;
	gint order_mode;

	g_debug( "%s: window=%p, treeview=%p", thisfn, ( void * ) window, ( void * ) treeview );
	g_return_val_if_fail( BASE_IS_WINDOW( window ), NULL );
	g_return_val_if_fail( NACT_IS_IACTIONS_LIST( window ), NULL );
	g_return_val_if_fail( GTK_IS_TREE_VIEW( treeview ), NULL );

	ts_model = gtk_tree_store_new(
			IACTIONS_LIST_N_COLUMN, GDK_TYPE_PIXBUF, G_TYPE_STRING, NA_OBJECT_TYPE );

	/* create the filter model
	 */
	model = g_object_new( NACT_TREE_MODEL_TYPE, "child-model", ts_model, NULL );

	gtk_tree_model_filter_set_visible_func(
			GTK_TREE_MODEL_FILTER( model ), ( GtkTreeModelFilterVisibleFunc ) filter_visible, model, NULL );

	/* initialize the sortable interface
	 */
	application = NACT_APPLICATION( base_window_get_application( window ));
	pivot = nact_application_get_pivot( application );
	order_mode = na_iprefs_get_order_mode( NA_IPREFS( pivot ));
	nact_tree_model_display_order_change( model, order_mode );

	model->private->window = window;
	model->private->treeview = treeview;
	model->private->clipboard = nact_clipboard_new();

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

	g_debug( "%s: model=%p, have_dnd=%s", thisfn, ( void * ) model, have_dnd ? "True":"False" );
	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	if( !model->private->dispose_has_run ){

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

			/* connect: implies that we have to do all hard stuff
			 * connect_after: no more triggers drag-drop signal
			 */
			/*base_window_signal_connect_after(
					BASE_WINDOW( model->private->window ),
					G_OBJECT( model->private->treeview ),
					"drag-motion",
					G_CALLBACK( on_drag_motion ));*/

			/*base_window_signal_connect(
					BASE_WINDOW( model->private->window ),
					G_OBJECT( model->private->treeview ),
					"drag-drop",
					G_CALLBACK( on_drag_drop ));*/

			base_window_signal_connect(
					BASE_WINDOW( model->private->window ),
					G_OBJECT( model->private->treeview ),
					"drag-end",
					G_CALLBACK( on_drag_end ));
		}
	}
}

void
nact_tree_model_dispose( NactTreeModel *model )
{
	static const gchar *thisfn = "nact_tree_model_dispose";
	GtkTreeStore *ts_model;

	g_debug( "%s: model=%p", thisfn, ( void * ) model );
	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	if( !model->private->dispose_has_run ){

		nact_tree_model_dump( model );

		ts_model = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));

		gtk_tree_store_clear( ts_model );
	}
}

/**
 * nact_tree_model_display:
 * @model: this #NactTreeModel instance.
 * @object: the object whose display is to be refreshed.
 *
 * Refresh the display of a #NAObject.
 */
void
nact_tree_model_display( NactTreeModel *model, NAObject *object )
{
	/*static const gchar *thisfn = "nact_tree_model_display";*/
	GtkTreeStore *store;
	GtkTreeIter iter;
	GtkTreePath *path;

	/*g_debug( "%s: model=%p (%s), object=%p (%s)", thisfn,
			( void * ) model, G_OBJECT_TYPE_NAME( model ),
			( void * ) object, G_OBJECT_TYPE_NAME( object ));*/
	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	if( !model->private->dispose_has_run ){

		store = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));

		if( search_for_object( model, GTK_TREE_MODEL( store ), object, &iter )){
			display_item( store, model->private->treeview, &iter, object );
			path = gtk_tree_model_get_path( GTK_TREE_MODEL( store ), &iter );
			gtk_tree_model_row_changed( GTK_TREE_MODEL( store ), path, &iter );
			gtk_tree_path_free( path );
		}

		/*gtk_tree_model_filter_refilter( GTK_TREE_MODEL_FILTER( model ));*/
	}
}

/**
 * Setup the new order mode.
 */
void
nact_tree_model_display_order_change( NactTreeModel *model, gint order_mode )
{
	GtkTreeStore *store;

	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	if( !model->private->dispose_has_run ){

		store = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));
		g_object_set_data( G_OBJECT( store ), TREE_MODEL_ORDER_MODE, GINT_TO_POINTER( order_mode ));

		switch( order_mode ){

			case IPREFS_ORDER_ALPHA_ASCENDING:

				gtk_tree_sortable_set_sort_column_id(
						GTK_TREE_SORTABLE( store ),
						IACTIONS_LIST_LABEL_COLUMN,
						GTK_SORT_ASCENDING );

				gtk_tree_sortable_set_sort_func(
						GTK_TREE_SORTABLE( store ),
						IACTIONS_LIST_LABEL_COLUMN,
						( GtkTreeIterCompareFunc ) sort_actions_list,
						NULL,
						NULL );
				break;

			case IPREFS_ORDER_ALPHA_DESCENDING:

				gtk_tree_sortable_set_sort_column_id(
						GTK_TREE_SORTABLE( store ),
						IACTIONS_LIST_LABEL_COLUMN,
						GTK_SORT_DESCENDING );

				gtk_tree_sortable_set_sort_func(
						GTK_TREE_SORTABLE( store ),
						IACTIONS_LIST_LABEL_COLUMN,
						( GtkTreeIterCompareFunc ) sort_actions_list,
						NULL,
						NULL );
				break;

			case IPREFS_ORDER_MANUAL:
			default:

				gtk_tree_sortable_set_sort_column_id(
						GTK_TREE_SORTABLE( store ),
						GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID,
						0 );
				break;
		}
	}
}

void
nact_tree_model_dump( NactTreeModel *model )
{
	static const gchar *thisfn = "nact_tree_model_dump";
	GtkTreeStore *store;
	ntmDumpStruct *ntm;

	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	if( !model->private->dispose_has_run ){

		store = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));

		g_debug( "%s: %s at %p, %s at %p", thisfn,
				G_OBJECT_TYPE_NAME( model ), ( void * ) model, G_OBJECT_TYPE_NAME( store ), ( void * ) store );

		ntm = g_new0( ntmDumpStruct, 1 );
		ntm->fname = g_strdup( thisfn );
		ntm->prefix = g_strdup( "" );

		nact_tree_model_iter( model, ( FnIterOnStore ) dump_store, ntm );

		g_free( ntm->prefix );
		g_free( ntm->fname );
		g_free( ntm );
	}
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

	if( !model->private->dispose_has_run ){

		model->private->only_actions = only_actions;
		ts_model = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));
		gtk_tree_store_clear( ts_model );
		fill_tree_store( ts_model, model->private->treeview, items, only_actions, NULL );
	}
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
		/*g_debug( "%s: object=%p(%s)", thisfn
				, ( void * ) object, G_OBJECT_TYPE_NAME( object ));*/

		if( NA_IS_OBJECT_MENU( object )){
			duplicate = object;
			if( !only_actions ){
				duplicate = parent ? g_object_ref( object ) : na_object_duplicate( object );
				/*g_debug( "%s: appending duplicate=%p (%s)", thisfn, ( void * ) duplicate, G_OBJECT_TYPE_NAME( duplicate ));*/
				append_item( model, treeview, parent, &iter, duplicate );
				g_object_unref( duplicate );
			}
			subitems = na_object_get_items( duplicate );
			fill_tree_store( model, treeview, subitems, only_actions, only_actions ? NULL : &iter );
			na_object_free_items( subitems );
		}

		if( NA_IS_OBJECT_ACTION( object )){
			duplicate = parent ? g_object_ref( object ) : na_object_duplicate( object );
			/*g_debug( "%s: appending duplicate=%p (%s)", thisfn, ( void * ) duplicate, G_OBJECT_TYPE_NAME( duplicate ));*/
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
			/*g_debug( "%s: appending object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));*/
			append_item( model, treeview, parent, &iter, object );
		}
	}
}

/**
 * nact_tree_model_insert:
 * @model: this #NactTreeModel instance.
 * @object: a #NAObject-derived object to be inserted.
 * @path: the #GtkTreePath which specifies the insertion path.
 * @parent: set to the parent or the object itself.
 *
 * Insert a new row at the given position.
 *
 * Gtk API uses to returns iter ; but at least when inserting a new
 * profile in an action, we may have store_iter_path="0:1" (good), but
 * iter_path="0:0" (bad) - so we'd rather return a string path.
 *
 * Note that we do not return anything here as the insertion path at
 * the beginning of the function is always valid when the function
 * returns; it points now to the newly inserted row.
 */
void
nact_tree_model_insert( NactTreeModel *model, const NAObject *object, GtkTreePath *path, NAObject **parent )
{
	/*static const gchar *thisfn = "nact_tree_model_insert";*/
	GtkTreeModel *store;
	GtkTreeIter iter;
	GtkTreeIter parent_iter;
	GtkTreePath *parent_path;
	NAObject *parent_obj;
	gboolean has_parent;
	GtkTreeIter sibling_iter;
	NAObject *sibling_obj;
	gboolean has_sibling;

	g_return_if_fail( NACT_IS_TREE_MODEL( model ));
	g_return_if_fail( NA_IS_OBJECT( object ));

	if( !model->private->dispose_has_run ){

		store = gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model ));
		has_parent = FALSE;
		parent_obj = NULL;
		sibling_obj = NULL;

		remove_if_exists( model, store, object );

		/* may be FALSE when store is empty */
		has_sibling = gtk_tree_model_get_iter( store, &sibling_iter, path );
		if( has_sibling ){
			gtk_tree_model_get( store, &sibling_iter, IACTIONS_LIST_NAOBJECT_COLUMN, &sibling_obj, -1 );
			g_object_unref( sibling_obj );
		}

		if( gtk_tree_path_get_depth( path ) > 1 ){

			has_parent = TRUE;
			parent_path = gtk_tree_path_copy( path );
			gtk_tree_path_up( parent_path );
			gtk_tree_model_get_iter( store, &parent_iter, parent_path );
			gtk_tree_path_free( parent_path );

			gtk_tree_model_get( store, &parent_iter, IACTIONS_LIST_NAOBJECT_COLUMN, &parent_obj, -1 );
			g_object_unref( parent_obj );

			if( parent && !*parent ){
				*parent = parent_obj;
			}

			if( has_sibling ){
				na_object_insert_item( parent_obj, object, sibling_obj );
			} else {
				na_object_append_item( parent_obj, object );
			}
		}

		gtk_tree_store_insert_before(
				GTK_TREE_STORE( store ), &iter,
				has_parent ? &parent_iter : NULL,
				has_sibling ? &sibling_iter : NULL );
		gtk_tree_store_set( GTK_TREE_STORE( store ), &iter, IACTIONS_LIST_NAOBJECT_COLUMN, object, -1 );
		display_item( GTK_TREE_STORE( store ), model->private->treeview, &iter, object );
	}
}

GtkTreePath *
nact_tree_model_insert_into( NactTreeModel *model, const NAObject *object, GtkTreePath *path, NAObject **parent )
{
	static const gchar *thisfn = "nact_tree_model_insert_into";
	GtkTreeModel *store;
	GtkTreeIter iter;
	GtkTreeIter parent_iter;
	GtkTreePath *new_path;
	gchar *path_str;

	g_return_val_if_fail( NACT_IS_TREE_MODEL( model ), NULL );
	g_return_val_if_fail( NA_IS_OBJECT( object ), NULL );

	new_path = NULL;

	if( !model->private->dispose_has_run ){

		store = gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model ));

		if( !gtk_tree_model_get_iter( store, &parent_iter, path )){
			path_str = gtk_tree_path_to_string( path );
			g_warning( "%s: unable to get iter at path %s", thisfn, path_str );
			g_free( path_str );
			return( NULL );
		}
		gtk_tree_model_get( store, &parent_iter, IACTIONS_LIST_NAOBJECT_COLUMN, parent, -1 );
		g_object_unref( *parent );

		na_object_insert_item( *parent, object, NULL );

		gtk_tree_store_insert_after( GTK_TREE_STORE( store ), &iter, &parent_iter, NULL );
		gtk_tree_store_set( GTK_TREE_STORE( store ), &iter, IACTIONS_LIST_NAOBJECT_COLUMN, object, -1 );
		display_item( GTK_TREE_STORE( store ), model->private->treeview, &iter, object );

		new_path = gtk_tree_model_get_path( store, &iter );
	}

	return( new_path );
}

/*
 * if the object, identified by its uuid, already exists, then remove it first
 */
static void
remove_if_exists( NactTreeModel *model, GtkTreeModel *store, const NAObject *object )
{
	GtkTreeIter iter;

	if( NA_IS_OBJECT_ITEM( object )){
		if( search_for_object_id( model, store, object, &iter )){
			gtk_tree_store_remove( GTK_TREE_STORE( store ), &iter );
		}
	}
}

void
nact_tree_model_iter( NactTreeModel *model, FnIterOnStore fn, gpointer user_data )
{
	GtkTreeStore *store;

	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	if( !model->private->dispose_has_run ){

		store = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));
		iter_on_store( model, GTK_TREE_MODEL( store ), NULL, fn, user_data );
	}
}

/**
 * nact_tree_model_object_at_path:
 * @model: this #NactTreeModel instance.
 * @path: the #GtkTreePath to be searched.
 *
 * Returns: the #NAObject at the given @path if any, or NULL.
 */
NAObject *
nact_tree_model_object_at_path( NactTreeModel *model, GtkTreePath *path )
{
	NAObject *object;
	GtkTreeModel *store;
	GtkTreeIter iter;

	g_return_val_if_fail( NACT_IS_TREE_MODEL( model ), NULL );

	object = NULL;

	if( !model->private->dispose_has_run ){

		store = gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model ));
		gtk_tree_model_get_iter( store, &iter, path );
		gtk_tree_model_get( store, &iter, IACTIONS_LIST_NAOBJECT_COLUMN, &object, -1 );
		g_object_unref( object );
	}

	return( object );
}

/**
 * nact_tree_model_remove:
 * @model: this #NactTreeModel instance.
 * @object: the #NAObject to be deleted.
 *
 * Recursively deletes the specified object.
 *
 * Returns: a path which may be suitable for the next selection.
 */
GtkTreePath *
nact_tree_model_remove( NactTreeModel *model, NAObject *object )
{
	GtkTreeIter iter;
	GtkTreeStore *store;
	GList *parents = NULL;
	GList *it;
	GtkTreePath *path = NULL;

	g_return_val_if_fail( NACT_IS_TREE_MODEL( model ), NULL );

	if( !model->private->dispose_has_run ){

		store = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));

		if( search_for_object( model, GTK_TREE_MODEL( store ), object, &iter )){
			parents = add_parent( parents, GTK_TREE_MODEL( store ), &iter );
			path = gtk_tree_model_get_path( GTK_TREE_MODEL( store ), &iter );
			remove_items( store, &iter );
		}

		for( it = parents ; it ; it = it->next ){
			na_object_check_edition_status( it->data );
		}
	}

	return( path );
}

/*
 * iter is positionned on the row which is going to be deleted
 * remove the object from the subitems list of parent (if any)
 * add parent to the list to check its status after remove will be done
 */
static GList *
add_parent( GList *parents, GtkTreeModel *store, GtkTreeIter *obj_iter )
{
	NAObject *object;
	GtkTreePath *path;
	GtkTreeIter iter;
	NAObject *parent;

	gtk_tree_model_get( store, obj_iter, IACTIONS_LIST_NAOBJECT_COLUMN, &object, -1 );
	path = gtk_tree_model_get_path( store, obj_iter );

	if( gtk_tree_path_get_depth( path ) > 1 ){
		gtk_tree_path_up( path );
		gtk_tree_model_get_iter( store, &iter, path );
		gtk_tree_model_get( store, &iter, IACTIONS_LIST_NAOBJECT_COLUMN, &parent, -1 );

		if( !g_list_find( parents, parent )){
			parents = g_list_prepend( parents, parent );
			na_object_remove_item( parent, object );
		}

		g_object_unref( parent );
	}

	gtk_tree_path_free( path );
	g_object_unref( object );

	return( parents );
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
	gchar *id, *label;

	depth = gtk_tree_path_get_depth( path );
	prefix = g_string_new( ntm->prefix );
	for( i=1 ; i<depth ; ++i ){
		g_string_append_printf( prefix, "  " );
	}

	id = na_object_get_id( object );
	label = na_object_get_label( object );
	g_debug( "%s: %s%s at %p \"[%s] %s\"",
			ntm->fname, prefix->str, G_OBJECT_TYPE_NAME( object ), ( void * ) object, id, label );
	g_free( label );
	g_free( id );

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

/*
 * recursively remove child items starting with iter
 * returns TRUE if iter is always valid after the remove
 */
static gboolean
remove_items( GtkTreeStore *store, GtkTreeIter *iter )
{
	GtkTreeIter child;
	gboolean valid;

	while( gtk_tree_model_iter_children( GTK_TREE_MODEL( store ), &child, iter )){
		remove_items( store, &child );
	}
	valid = gtk_tree_store_remove( store, iter );

	return( valid );
}

static gboolean
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
	}

	/* stop iteration when found */
	return( ntm->found );
}

static gboolean
search_for_object_id( NactTreeModel *model, GtkTreeModel *store, const NAObject *object, GtkTreeIter *result_iter )
{
	gboolean found = FALSE;
	ntmSearchIdStruct *ntm;
	GtkTreeIter iter;

	ntm = g_new0( ntmSearchIdStruct, 1 );
	ntm->store = store;
	ntm->id = na_object_get_id( object );
	ntm->found = FALSE;
	ntm->iter = &iter;

	iter_on_store( model, store, NULL, ( FnIterOnStore ) search_for_object_id_iter, ntm );

	if( ntm->found ){
		found = TRUE;
		memcpy( result_iter, ntm->iter, sizeof( GtkTreeIter ));
	}

	g_free( ntm->id );
	g_free( ntm );
	return( found );
}

static gboolean
search_for_object_id_iter( NactTreeModel *model, GtkTreePath *path, NAObject *object, ntmSearchIdStruct *ntm )
{
	gchar *id;

	id = na_object_get_id( object );

	if( !g_ascii_strcasecmp( id, ntm->id )){
		if( gtk_tree_model_get_iter( ntm->store, ntm->iter, path )){
			ntm->found = TRUE;
		}
	}

	g_free( id );

	/* stop iteration when found */
	return( ntm->found );
}

/*
 * all selectable rows are draggable
 * nonetheless, it's a good place to store the dragged row references
 * we only make use of them in on_drag_motion handler
 */
static gboolean
imulti_drag_source_row_draggable( EggTreeMultiDragSource *drag_source, GList *rows )
{
	static const gchar *thisfn = "nact_tree_model_imulti_drag_source_row_draggable";
	NactTreeModel *model;
	GtkTreeModel *store;
	GtkTreePath *path;
	GtkTreeIter iter;
	NAObject *object;
	GList *it;

	g_debug( "%s: drag_source=%p, rows=%p (%d items)",
			thisfn, ( void * ) drag_source, ( void * ) rows, g_list_length( rows ));

	g_return_val_if_fail( NACT_IS_TREE_MODEL( drag_source ), FALSE );
	model = NACT_TREE_MODEL( drag_source );

	if( !model->private->dispose_has_run ){

		model->private->drag_has_profiles = FALSE;
		store = gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model ));

		for( it = rows ; it && !model->private->drag_has_profiles ; it = it->next ){

			path = gtk_tree_row_reference_get_path(( GtkTreeRowReference * ) it->data );
			gtk_tree_model_get_iter( store, &iter, path );
			gtk_tree_model_get( store, &iter, IACTIONS_LIST_NAOBJECT_COLUMN, &object, -1 );

			if( NA_IS_OBJECT_PROFILE( object )){
				model->private->drag_has_profiles = TRUE;
			}

			g_object_unref( object );
			gtk_tree_path_free( path );
		}
	}

	return( TRUE );
}

/*
 * imulti_drag_source_drag_data_get:
 * @context: contains
 *  - the suggested action, as choosen by the drop site,
 *    between those we have provided in imulti_drag_source_get_drag_actions()
 *  - the target folder (XDS protocol)
 * @selection_data:
 * @rows: list of row references which are to be dropped
 * @info: the suggested format, as choosen by the drop site, between those
 *  we have provided in imulti_drag_source_get_target_list()
 *
 * This function is called when we release the selected items onto the
 * destination
 *
 * NACT_XCHANGE_FORMAT_NACT:
 * internal format for drag and drop inside the treeview:
 * - copy in the clipboard the list of row references
 * - selection data is empty
 *
 * NACT_XCHANGE_FORMAT_XDS:
 * exchange format to drop to outside:
 * - copy in the clipboard the list of row references
 * - set the destination folder
 * - selection data is 'success' or 'failure'
 *
 * NACT_XCHANGE_FORMAT_APPLICATION_XML:
 * NACT_XCHANGE_FORMAT_TEXT_PLAIN:
 * exchange format to export to outside:
 * - do not use dnd clipboard
 * - selection data receives the export in text format
 *
 * Returns: %TRUE if required data was actually provided by the source,
 * %FALSE else.
 */
static gboolean
imulti_drag_source_drag_data_get( EggTreeMultiDragSource *drag_source,
				   GdkDragContext         *context,
				   GtkSelectionData       *selection_data,
				   GList                  *rows,
				   guint                   info )
{
	static const gchar *thisfn = "nact_tree_model_imulti_drag_source_drag_data_get";
	gchar *atom_name;
	NactTreeModel *model;
	gchar *data;
	gboolean ret = FALSE;
	gchar *dest_folder, *folder;
	gboolean is_writable;
	gboolean copy_data;

	atom_name = gdk_atom_name( selection_data->target );
	g_debug( "%s: drag_source=%p, context=%p, action=%d, selection_data=%p, rows=%p, atom=%s",
			thisfn, ( void * ) drag_source, ( void * ) context, ( int ) context->suggested_action, ( void * ) selection_data, ( void * ) rows,
			atom_name );
	g_free( atom_name );

	model = NACT_TREE_MODEL( drag_source );
	g_return_val_if_fail( model->private->window, FALSE );

	if( !model->private->dispose_has_run ){

		if( !rows || !g_list_length( rows )){
			return( FALSE );
		}

		switch( info ){
			case NACT_XCHANGE_FORMAT_NACT:
				copy_data = ( context->action == GDK_ACTION_COPY );
				gtk_selection_data_set( selection_data, selection_data->target, 8, ( guchar * ) "", 0 );
				nact_clipboard_dnd_set( model->private->clipboard, info, rows, NULL, copy_data );
				ret = TRUE;
				break;

			case NACT_XCHANGE_FORMAT_XDS:
				folder = get_xds_atom_value( context );
				dest_folder = na_utils_remove_last_level_from_path( folder );
				g_free( folder );
				is_writable = na_utils_is_writable_dir( dest_folder );
				gtk_selection_data_set( selection_data, selection_data->target, 8, ( guchar * )( is_writable ? "S" : "F" ), 1 );
				if( is_writable ){
					nact_clipboard_dnd_set( model->private->clipboard, info, rows, dest_folder, TRUE );
				}
				g_free( dest_folder );
				ret = is_writable;
				break;

			case NACT_XCHANGE_FORMAT_APPLICATION_XML:
			case NACT_XCHANGE_FORMAT_TEXT_PLAIN:
				data = nact_clipboard_dnd_get_text( model->private->clipboard, rows );
				gtk_selection_data_set( selection_data, selection_data->target, 8, ( guchar * ) data, strlen( data ));
				g_free( data );
				ret = TRUE;
				break;

			default:
				break;
		}
	}

	return( ret );
}

static gboolean
imulti_drag_source_drag_data_delete( EggTreeMultiDragSource *drag_source, GList *rows )
{
	static const gchar *thisfn = "nact_tree_model_imulti_drag_source_drag_data_delete";

	g_debug( "%s: drag_source=%p, path_list=%p", thisfn, ( void * ) drag_source, ( void * ) rows );

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
 * called when a drop from the outside occurs in the treeview
 * this may be an import action, or a move/copy inside of the tree
 *
 * Returns: %TRUE if the specified rows were successfully inserted at
 * the given dest, %FALSE else.
 *
 * When importing:
 * - selection=XdndSelection
 * - target=text/uri-list
 * - type=text/uri-list
 *
 * When moving/copy from the treeview to the treeview:
 * - selection=XdndSelection
 * - target=XdndNautilusActions
 * - type=XdndNautilusActions
 */
static gboolean
idrag_dest_drag_data_received( GtkTreeDragDest *drag_dest, GtkTreePath *dest, GtkSelectionData  *selection_data )
{
	static const gchar *thisfn = "nact_tree_model_idrag_dest_drag_data_received";
	gboolean result = FALSE;
	NactApplication *application;
	NAPivot *pivot;
	gchar *atom_name;
	guint info;
	GList *rows;
	gboolean copy_data;
	GSList *uri_list, *is, *msg;
	gint import_mode;
	NAObjectAction *action;
	gchar *path_str;
	GtkTreeIter iter;
	NAObject *current;
	gboolean inside_an_action;
	GList *object_list, *it;
	NactMainWindow *main_window;
	gboolean drop_ok = TRUE;
	NAObjectAction *parent = NULL;
	GtkTreePath *path;

	g_debug( "%s: drag_dest=%p, dest=%p, selection_data=%p", thisfn, ( void * ) drag_dest, ( void * ) dest, ( void * ) selection_data );
	g_return_val_if_fail( NACT_IS_TREE_MODEL( drag_dest ), FALSE );

	atom_name = gdk_atom_name( selection_data->selection );
	g_debug( "%s: selection=%s", thisfn, atom_name );
	g_free( atom_name );

	atom_name = gdk_atom_name( selection_data->target );
	g_debug( "%s: target=%s", thisfn, atom_name );
	g_free( atom_name );

	atom_name = gdk_atom_name( selection_data->type );
	g_debug( "%s: type=%s", thisfn, atom_name );
	g_free( atom_name );

	g_debug( "%s: format=%d, length=%d", thisfn, selection_data->format, selection_data->length );

	info = target_atom_to_id( selection_data->type );
	g_debug( "%s: info=%u", thisfn, info );

	application = NACT_APPLICATION( base_window_get_application( NACT_TREE_MODEL( drag_dest )->private->window ));
	pivot = nact_application_get_pivot( application );
	main_window = NACT_MAIN_WINDOW( base_application_get_main_window( BASE_APPLICATION( application )));

	path_str = gtk_tree_path_to_string( dest );
	g_debug( "%s: dest_path=%s", thisfn, path_str );
	g_free( path_str );

	/*
	 * NACT format (may embed profiles, or not)
	 * 	with profiles: only valid dest is inside an action
	 *  without profile: only valid dest is outside (besides of) an action
	 * URI format only involves actions
	 *  ony valid dest in outside (besides of) an action
	 */
	inside_an_action = FALSE;
	if( gtk_tree_model_get_iter( GTK_TREE_MODEL( drag_dest ), &iter, dest )){
		gtk_tree_model_get( GTK_TREE_MODEL( drag_dest ), &iter, IACTIONS_LIST_NAOBJECT_COLUMN, &current, -1 );
		g_debug( "%s: current object at dest is %s", thisfn, G_OBJECT_TYPE_NAME( current ));
		if( NA_IS_OBJECT_PROFILE( current )){
			inside_an_action = TRUE;
			parent = na_object_profile_get_action( NA_OBJECT_PROFILE( current ));
		}
		g_object_unref( current );
	}

	switch( info ){
		case NACT_XCHANGE_FORMAT_NACT:
			if( NACT_TREE_MODEL( drag_dest )->private->drag_has_profiles ){
				if( !inside_an_action ){
					drop_ok = FALSE;
					nact_main_statusbar_display_with_timeout(
							main_window,
							TREE_MODEL_STATUSBAR_CONTEXT,
							_( "Unable to drop a profile outside of an action" ));
				}
			} else {
				if( inside_an_action ){
					drop_ok = FALSE;
					nact_main_statusbar_display_with_timeout(
							main_window,
							TREE_MODEL_STATUSBAR_CONTEXT,
							_( "Unable to drop an action or a menu inside of an action" ));
				}
			}
			if( drop_ok ){
				rows = nact_clipboard_dnd_get_data( NACT_TREE_MODEL( drag_dest )->private->clipboard, &copy_data );
				g_debug( "%s: rows has %d items, copy_data=%s", thisfn, g_list_length( rows ), copy_data ? "True":"False" );
				object_list = NULL;
				for( it = rows ; it ; it = it->next ){
					path = gtk_tree_row_reference_get_path(( GtkTreeRowReference * ) it->data );
					if( path ){
						if( gtk_tree_model_get_iter( GTK_TREE_MODEL( drag_dest ), &iter, path )){
							gtk_tree_model_get( GTK_TREE_MODEL( drag_dest ), &iter, IACTIONS_LIST_NAOBJECT_COLUMN, &current, -1 );
							if( copy_data ){
								nact_main_window_prepare_object_for_copy( main_window, current, parent );
							}
							object_list = g_list_prepend( object_list, current );
							g_object_unref( current );
						}
						gtk_tree_path_free( path );
					}
				}
				object_list = g_list_reverse( object_list );
				nact_iactions_list_insert_at_path( NACT_IACTIONS_LIST( main_window ), object_list, dest );

				if( !copy_data ){
					nact_iactions_list_delete( NACT_IACTIONS_LIST( main_window ), object_list );
				}

				g_list_free( object_list );
				g_list_foreach( rows, ( GFunc ) gtk_tree_row_reference_free, NULL );
				g_list_free( rows );
				result = TRUE;
			}
			break;

		/* drop some actions from outside
		 * most probably from the file manager as a list of uris
		 */
		case NACT_XCHANGE_FORMAT_URI_LIST:
			if( inside_an_action ){

				nact_main_statusbar_display_with_timeout(
						main_window,
						TREE_MODEL_STATUSBAR_CONTEXT,
						_( "Unable to drop an action inside of another one" ));

			} else {
				uri_list = na_utils_lines_to_string_list(( const gchar * ) selection_data->data );
				import_mode = na_iprefs_get_import_mode( NA_IPREFS( pivot ));
				for( is = uri_list ; is ; is = is->next ){

					action = nact_xml_reader_import(
							NACT_TREE_MODEL( drag_dest )->private->window,
							( const gchar * ) is->data,
							import_mode,
							&msg );

					if( msg ){
						nact_main_statusbar_display_with_timeout(
								main_window,
								TREE_MODEL_STATUSBAR_CONTEXT,
								msg->data );
						na_utils_free_string_list( msg );

					} else {
						g_return_val_if_fail( NA_IS_OBJECT_ACTION( action ), FALSE );
						object_list = g_list_prepend( NULL, action );
						nact_iactions_list_insert_at_path( NACT_IACTIONS_LIST( main_window ), object_list, dest );
						g_list_free( object_list );
					}

					if( action ){
						g_return_val_if_fail( NA_IS_OBJECT_ACTION( action ), FALSE );
						g_object_unref( action );
					}
				}
				na_utils_free_string_list( uri_list );
				result = TRUE;
			}
			break;

		default:
			break;
	}

	return( result );
}

/*
 * called when the source and the dest are not at the same tree level ?
 */
static gboolean
idrag_dest_row_drop_possible( GtkTreeDragDest *drag_dest, GtkTreePath *dest_path, GtkSelectionData *selection_data )
{
	static const gchar *thisfn = "nact_tree_model_idrag_dest_row_drop_possible";

	g_debug( "%s: drag_dest=%p, dest_path=%p, selection_data=%p", thisfn, ( void * ) drag_dest, ( void * ) dest_path, ( void * ) selection_data );

	return( TRUE );
}

static void
on_drag_begin( GtkWidget *widget, GdkDragContext *context, BaseWindow *window )
{
	static const gchar *thisfn = "nact_tree_model_on_drag_begin";
	NactTreeModel *model;

	g_debug( "%s: widget=%p, context=%p, window=%p",
			thisfn, ( void * ) widget, ( void * ) context, ( void * ) window );

	model = NACT_TREE_MODEL( gtk_tree_view_get_model( GTK_TREE_VIEW( widget )));
	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	if( !model->private->dispose_has_run ){

		model->private->drag_highlight = FALSE;
		model->private->drag_drop = FALSE;

		nact_clipboard_dnd_clear( model->private->clipboard );

		gdk_property_change(
				context->source_window,
				XDS_ATOM, TEXT_ATOM, 8, GDK_PROP_MODE_REPLACE, ( guchar * ) XDS_FILENAME, strlen( XDS_FILENAME ));
	}
}

/*
 * this function works well, but only called from on_drag_motion handler...
 */
/*static gboolean
is_row_drop_possible( NactTreeModel *model, GtkTreePath *path, GtkTreeViewDropPosition pos )
{
	gboolean ok = FALSE;
	GtkTreeModel *store;
	GtkTreeIter iter;
	NAObject *object;

	store = gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model ));
	gtk_tree_model_get_iter( store, &iter, path );
	gtk_tree_model_get( store, &iter, IACTIONS_LIST_NAOBJECT_COLUMN, &object, -1 );

	if( model->private->drag_has_profiles ){
		if( NA_IS_OBJECT_PROFILE( object )){
			ok = TRUE;
		} else if( NA_IS_OBJECT_ACTION( object )){
			ok = ( pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE || pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER );
		}
	} else {
		if( NA_IS_OBJECT_ITEM( object )){
			ok = TRUE;
		}
	}

	g_object_unref( object );
	return( ok );
}*/

/*
 * called when the user moves into the target widget
 * returns TRUE if a drop zone
 */
#if 0
static gboolean
on_drag_motion( GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, BaseWindow *window )
{
	NactTreeModel *model;
	gboolean ok = FALSE;
	GtkTreePath *path;
	GtkTreeViewDropPosition pos;

	model = NACT_TREE_MODEL( gtk_tree_view_get_model( GTK_TREE_VIEW( widget )));
	g_return_val_if_fail( NACT_IS_TREE_MODEL( model ), FALSE );

	if( !model->private->dispose_has_run ){

		if( !model->private->drag_highlight ){
			model->private->drag_highlight = TRUE;
			gtk_drag_highlight( widget );
		}

		/*target = gtk_drag_dest_find_target( widget, context, NULL );
		if( target == GDK_NONE ){
			gdk_drag_status( context, 0, time );
		} else {
			gtk_drag_get_data( widget, context, target, time );
		}*/

		if( gtk_tree_view_get_dest_row_at_pos( GTK_TREE_VIEW( widget ), x, y, &path, &pos )){
			ok = is_row_drop_possible( model, path, pos );
			if( ok ){
				gdk_drag_status( context, 0, time );
			}
		}

		gtk_tree_path_free( path );
		g_debug( "nact_tree_model_on_drag_motion: ok=%s, pos=%d", ok ? "True":"False", pos );
	}

	return( ok );
}
#endif

/*
 * called when the user drops the data
 * returns TRUE if a drop zone
 */
/*static gboolean
on_drag_drop( GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, BaseWindow *window )
{
	NactTreeModel *model;

	model = NACT_TREE_MODEL( gtk_tree_view_get_model( GTK_TREE_VIEW( widget )));
	g_return_val_if_fail( NACT_IS_TREE_MODEL( model ), FALSE );

	if( !model->private->dispose_has_run ){

		model->private->drag_drop = TRUE;
	}

	return( TRUE );
}*/

static void
on_drag_end( GtkWidget *widget, GdkDragContext *context, BaseWindow *window )
{
	static const gchar *thisfn = "nact_tree_model_on_drag_end";
	NactTreeModel *model;

	g_debug( "%s: widget=%p, context=%p, window=%p",
			thisfn, ( void * ) widget, ( void * ) context, ( void * ) window );

	model = NACT_TREE_MODEL( gtk_tree_view_get_model( GTK_TREE_VIEW( widget )));
	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	if( !model->private->dispose_has_run ){

		nact_clipboard_dnd_drag_end( model->private->clipboard );
		nact_clipboard_dnd_clear( model->private->clipboard );
		gdk_property_delete( context->source_window, XDS_ATOM );
	}
}

/* The following function taken from bugzilla
 * (http://bugzilla.gnome.org/attachment.cgi?id=49362&action=view)
 * Author: Christian Neumair
 * Copyright: 2005 Free Software Foundation, Inc
 * License: GPL
 */
static char *
get_xds_atom_value( GdkDragContext *context )
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

static guint
target_atom_to_id( GdkAtom atom )
{
	gint i;
	guint info = 0;
	gchar *atom_name;

	atom_name = gdk_atom_name( atom );
	for( i = 0 ; i < G_N_ELEMENTS( dnd_dest_targets ) ; ++i ){
		if( !g_ascii_strcasecmp( dnd_dest_targets[i].target, atom_name )){
			info = dnd_dest_targets[i].info;
			break;
		}
	}
	g_free( atom_name );
	return( info );
}

static gint
sort_actions_list( GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data )
{
	/*static const gchar *thisfn = "nact_tree_model_sort_actions_list";*/
	NAObjectId *obj_a, *obj_b;
	gint ret;

	/*g_debug( "%s: model=%p, a=%p, b=%p, window=%p", thisfn, ( void * ) model, ( void * ) a, ( void * ) b, ( void * ) window );*/

	gtk_tree_model_get( model, a, IACTIONS_LIST_NAOBJECT_COLUMN, &obj_a, -1 );
	gtk_tree_model_get( model, b, IACTIONS_LIST_NAOBJECT_COLUMN, &obj_b, -1 );

	ret = na_pivot_sort_alpha_asc( obj_a, obj_b );

	g_object_unref( obj_b );
	g_object_unref( obj_a );

	/*g_debug( "%s: ret=%d", thisfn, ret );*/
	return( ret );
}

static gboolean
filter_visible( GtkTreeModel *store, GtkTreeIter *iter, NactTreeModel *model )
{
	/*static const gchar *thisfn = "nact_tree_model_filter_visible";*/
	NAObject *object;
	NAObjectAction *action;
	gboolean only_actions;
	gint count;

	/*g_debug( "%s: model=%p, iter=%p, window=%p", thisfn, ( void * ) model, ( void * ) iter, ( void * ) window );*/
	/*g_debug( "%s at %p", G_OBJECT_TYPE_NAME( model ), ( void * ) model );*/
	/* is a GtkTreeStore */

	gtk_tree_model_get( store, iter, IACTIONS_LIST_NAOBJECT_COLUMN, &object, -1 );

	if( object ){
		/*na_object_dump( object );*/

		if( NA_IS_OBJECT_ACTION( object )){
			g_object_unref( object );
			return( TRUE );
		}

		only_actions = NACT_TREE_MODEL( model )->private->only_actions;

		if( !only_actions ){

			if( NA_IS_OBJECT_MENU( object )){
				g_object_unref( object );
				return( TRUE );
			}

			if( NA_IS_OBJECT_PROFILE( object )){
				action = na_object_profile_get_action( NA_OBJECT_PROFILE( object ));
				g_object_unref( object );
				count = na_object_get_items_count( action );
				/*g_debug( "action=%p: count=%d", ( void * ) action, count );*/
				/*return( TRUE );*/
				return( count > 1 );
			}

			g_assert_not_reached();
		}
	}

	return( FALSE );
}
