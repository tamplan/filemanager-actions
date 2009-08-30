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

/*
 * Adapted from File-Roller:fr-list-model.c
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <common/na-action.h>
#include <common/na-iprefs.h>
#include <common/na-utils.h>

#include "egg-tree-multi-dnd.h"
#include "nact-iactions-list.h"
#include "nact-selection.h"
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
 *   call once nact_selection_on_drag_begin( treeview, context, main_window )
 *
 * when we drop (e.g. in Nautilus)
 *   call once egg_tree_multi_drag_drag_data_get( treeview, context, selection_data, info=0, time )
 *   call once nact_tree_model_imulti_drag_source_drag_data_get( drag_source, context, selection_data, path_list, atom=XdndDirectSave0 )
 *   call once nact_selection_on_drag_end( treeview, context, main_window )
 */

/* private class data
 */
struct NactTreeModelClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NactTreeModelPrivate {
	gboolean        dispose_has_run;
	NactMainWindow *window;
	gchar          *drag_dest_uri;
	GSList         *drag_items;
};

#define MAX_XDS_ATOM_VAL_LEN			4096
#define TEXT_ATOM						gdk_atom_intern( "text/plain", FALSE )
#define XDS_ATOM						gdk_atom_intern( "XdndDirectSave0", FALSE )
#define XDS_FILENAME					"xds.txt"
#define XNACT_ATOM						gdk_atom_intern( "XdndNautilusActions", FALSE )

enum {
	NACT_XCHANGE_FORMAT_NACT = 0,
	NACT_XCHANGE_FORMAT_XDS,
	NACT_XCHANGE_FORMAT_APPLICATION_XML,
	NACT_XCHANGE_FORMAT_TEXT_PLAIN

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

/*static GtkTargetEntry dnd_dest_targets[] = {
	{ "XdndNautilusActions0", 0, 0 },
	{ "XdndDirectSave0", 0, 2 }
};*/

static GtkTreeModelFilterClass *st_parent_class = NULL;

static GType          register_type( void );
static void           class_init( NactTreeModelClass *klass );
static void           imulti_drag_source_init( EggTreeMultiDragSourceIface *iface );
static void           idrag_dest_init( GtkTreeDragDestIface *iface );
static void           instance_init( GTypeInstance *instance, gpointer klass );
static void           instance_dispose( GObject *application );
static void           instance_finalize( GObject *application );

static gboolean       imulti_drag_source_row_draggable( EggTreeMultiDragSource *drag_source, GList *path_list );
static gboolean       imulti_drag_source_drag_data_get( EggTreeMultiDragSource *drag_source, GdkDragContext *context, GtkSelectionData *selection_data, GList *path_list, guint info );
static gboolean       imulti_drag_source_drag_data_delete( EggTreeMultiDragSource *drag_source, GList *path_list );
static GtkTargetList *imulti_drag_source_get_target_list( EggTreeMultiDragSource *drag_source );
static GdkDragAction  imulti_drag_source_get_drag_actions( EggTreeMultiDragSource *drag_source );

static gboolean       idrag_dest_drag_data_received( GtkTreeDragDest *drag_dest, GtkTreePath *dest, GtkSelectionData  *selection_data );
static gboolean       idrag_dest_row_drop_possible( GtkTreeDragDest *drag_dest, GtkTreePath *dest_path, GtkSelectionData *selection_data );

static gboolean       on_drag_begin( GtkWidget *widget, GdkDragContext *context, NactWindow *window );
static void           on_drag_end( GtkWidget *widget, GdkDragContext *context, NactWindow *window );

static gint           sort_actions_list( GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, NactWindow *window );
static gboolean       filter_visible( GtkTreeModel *model, GtkTreeIter *iter, gpointer data );
/*static gboolean       nautilus_xds_dnd_is_valid_xds_context( GdkDragContext *context );
static gboolean       context_offers_target( GdkDragContext *context, GdkAtom target );*/
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
		NULL,		/* base_init */
		NULL,		/* base_finalize */
		( GClassInitFunc ) class_init,
		NULL,		/* class_finalize */
		NULL,		/* class_data */
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
	/*static const gchar *thisfn = "nact_tree_model_instance_init";*/
	NactTreeModel *self;

	/*g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );*/
	g_assert( NACT_IS_TREE_MODEL( instance ));
	self = NACT_TREE_MODEL( instance );

	self->private = g_new0( NactTreeModelPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *object )
{
	/*static const gchar *thisfn = "nact_tree_model_instance_dispose";*/
	NactTreeModel *self;

	/*g_debug( "%s: object=%p", thisfn, ( void * ) object );*/
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
	/*static const gchar *thisfn = "nact_tree_model_instance_finalize";*/
	NactTreeModel *self;

	/*g_debug( "%s: object=%p", thisfn, ( void * ) object );*/
	g_assert( NACT_IS_TREE_MODEL( object ));
	self = NACT_TREE_MODEL( object );

	g_free( self->private->drag_dest_uri );
	g_slist_free( self->private->drag_items );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

NactTreeModel *
nact_tree_model_new( NactMainWindow *window )
{
	GtkTreeStore  *ts_model;
	NactTreeModel *model;
	gboolean       alpha_order;

	ts_model = gtk_tree_store_new(
			IACTIONS_LIST_N_COLUMN, GDK_TYPE_PIXBUF, G_TYPE_STRING, NA_OBJECT_TYPE );

	gtk_tree_sortable_set_default_sort_func(
			GTK_TREE_SORTABLE( ts_model ),
	        ( GtkTreeIterCompareFunc ) sort_actions_list, window, NULL );

	alpha_order = na_iprefs_get_alphabetical_order( NA_IPREFS( window ));

	if( alpha_order ){
		gtk_tree_sortable_set_sort_column_id(
				GTK_TREE_SORTABLE( ts_model ),
				IACTIONS_LIST_LABEL_COLUMN, GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID );
	}

	/* create the filter model */

	model = g_object_new( NACT_TREE_MODEL_TYPE, "child-model", ts_model, NULL );

	gtk_tree_model_filter_set_visible_func(
			GTK_TREE_MODEL_FILTER( model ), ( GtkTreeModelFilterVisibleFunc ) filter_visible, window, NULL );

	model->private->window = window;

	return( model );
}

/**
 * nact_tree_model_runtime_init_dnd:
 * @window: the #NactMainWindow window.
 * @widget: the #GtkTreeView which implements this #NactTreeModel.
 *
 * Initializes the drag & drop features.
 *
 * We use drag and drop:
 * - inside of treeview, for duplicating items, or moving items between
 *   menus
 * - from treeview to the outside world (e.g. Nautilus) to export actions
 * - from outside world (e.g. Nautilus) to import actions
 */
void
nact_tree_model_runtime_init_dnd( NactMainWindow *window, GtkTreeView *widget )
{
	NactTreeModel *model;

	model = NACT_TREE_MODEL( gtk_tree_view_get_model( widget ));
	g_assert( NACT_IS_TREE_MODEL( model ));

	nact_window_signal_connect(
			NACT_WINDOW( window ),
			G_OBJECT( widget ),
			"drag_begin",
			G_CALLBACK( on_drag_begin ));

	nact_window_signal_connect(
			NACT_WINDOW( window ),
			G_OBJECT( widget ),
			"drag_end",
			G_CALLBACK( on_drag_end ));

	egg_tree_multi_drag_add_drag_support( EGG_TREE_MULTI_DRAG_SOURCE( model ), GTK_TREE_VIEW( widget ));

	/*gtk_drag_source_set(
		GTK_WIDGET( widget ),
		GDK_BUTTON1_MASK,
		dnd_source_formats, G_N_ELEMENTS( dnd_source_formats ),
		GDK_ACTION_COPY | GDK_ACTION_MOVE
	);*/

	/*gtk_drag_dest_set(
		GTK_WIDGET( widget ), GTK_DEST_DEFAULT_ALL,
		dnd_dest_targets, G_N_ELEMENTS( dnd_dest_targets ), GDK_ACTION_COPY | GDK_ACTION_MOVE );*/
}

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
	GSList *selected_items;
	gchar *data;
	gboolean ret = FALSE;
	gchar *dest_folder, *folder;
	gboolean is_writable;

	atom_name = gdk_atom_name( selection_data->target );
	g_debug( "%s: drag_source=%p, context=%p, selection_data=%p, path_list=%p, atom=%s",
			thisfn, ( void * ) drag_source, ( void * ) context, ( void * ) selection_data, ( void * ) path_list,
			atom_name );
	g_free( atom_name );

	model = NACT_TREE_MODEL( drag_source );
	g_assert( model->private->window );

	selected_items = nact_iactions_list_get_selected_items( NACT_IACTIONS_LIST( model->private->window ));
	if( !selected_items ){
		return( FALSE );
	}
	if( !g_slist_length( selected_items )){
		g_slist_free( selected_items );
		return( FALSE );
	}

	switch( info ){
		case NACT_XCHANGE_FORMAT_NACT:
			data = nact_selection_get_data_for_intern_use( selected_items );
			gtk_selection_data_set( selection_data, selection_data->target, 8, ( guchar * ) data, strlen( data ));
			g_free( data );
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
				model->private->drag_items = g_slist_copy( selected_items );
			}
			g_free( dest_folder );
			ret = TRUE;
			break;

		case NACT_XCHANGE_FORMAT_APPLICATION_XML:
		case NACT_XCHANGE_FORMAT_TEXT_PLAIN:
			data = nact_selection_get_data_for_extern_use( selected_items );
			gtk_selection_data_set( selection_data, selection_data->target, 8, ( guchar * ) data, strlen( data ));
			g_free( data );
			ret = TRUE;
			break;

		default:
			break;
	}

	g_slist_free( selected_items );
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
on_drag_begin( GtkWidget *widget, GdkDragContext *context, NactWindow *window )
{
	static const gchar *thisfn = "nact_selection_on_drag_begin";
	NactTreeModel *model;

	g_debug( "%s: widget=%p, context=%p, window=%p",
			thisfn, ( void * ) widget, ( void * ) context, ( void * ) window );

	model = NACT_TREE_MODEL( gtk_tree_view_get_model( GTK_TREE_VIEW( widget )));

	g_free( model->private->drag_dest_uri );
	model->private->drag_dest_uri = NULL;

	g_slist_free( model->private->drag_items );
	model->private->drag_items = NULL;

	gdk_property_change(
			context->source_window,
			XDS_ATOM, TEXT_ATOM, 8, GDK_PROP_MODE_REPLACE, ( guchar * ) XDS_FILENAME, strlen( XDS_FILENAME ));

	return( FALSE );
}

static void
on_drag_end( GtkWidget *widget, GdkDragContext *context, NactWindow *window )
{
	static const gchar *thisfn = "nact_selection_on_drag_end";
	NactTreeModel *model;

	g_debug( "%s: widget=%p, context=%p, window=%p",
			thisfn, ( void * ) widget, ( void * ) context, ( void * ) window );

	model = NACT_TREE_MODEL( gtk_tree_view_get_model( GTK_TREE_VIEW( widget )));

	if( model->private->drag_dest_uri && model->private->drag_items && g_slist_length( model->private->drag_items )){
		nact_selection_export_items( model->private->drag_dest_uri, model->private->drag_items );
	}

	g_free( model->private->drag_dest_uri );
	model->private->drag_dest_uri = NULL;

	g_slist_free( model->private->drag_items );
	model->private->drag_items = NULL;

	gdk_property_delete( context->source_window, XDS_ATOM );
}

static gint
sort_actions_list( GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, NactWindow *window )
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
filter_visible( GtkTreeModel *model, GtkTreeIter *iter, gpointer data )
{
	/*static const gchar *thisfn = "nact_tree_model_filter_visible";*/
	NAObject *object;
	NAAction *action;

	/*g_debug( "%s: model=%p, iter=%p, data=%p", thisfn, ( void * ) model, ( void * ) iter, ( void * ) data );*/

	gtk_tree_model_get( model, iter, IACTIONS_LIST_NAOBJECT_COLUMN, &object, -1 );

	if( object ){
		if( NA_IS_ACTION( object )){
			return( TRUE );
		}

		g_assert( NA_IS_ACTION_PROFILE( object ));
		action = na_action_profile_get_action( NA_ACTION_PROFILE( object ));
		return( na_action_get_profiles_count( action ) > 1 );
	}

	return( FALSE );
}

/*
 * from FileRoller
 */
/* The following three functions taken from bugzilla
 * (http://bugzilla.gnome.org/attachment.cgi?id=49362&action=view)
 * Author: Christian Neumair
 * Copyright: 2005 Free Software Foundation, Inc
 * License: GPL
 */
/*static gboolean
nautilus_xds_dnd_is_valid_xds_context (GdkDragContext *context)
{
	char *tmp;
	gboolean ret;

	g_return_val_if_fail (context != NULL, FALSE);

	tmp = NULL;
	if (context_offers_target (context, XDS_ATOM)) {
		tmp = get_xds_atom_value (context);
	}

	ret = (tmp != NULL);
	g_free (tmp);

	return ret;
}

static gboolean
context_offers_target (GdkDragContext *context,
                       GdkAtom target)
{
	return (g_list_find (context->targets, target) != NULL);
}*/

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
