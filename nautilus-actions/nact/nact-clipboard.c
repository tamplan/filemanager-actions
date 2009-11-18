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

#include <gtk/gtk.h>
#include <string.h>

#include <private/na-object-api.h>

#include <runtime/na-iprefs.h>
#include <runtime/na-pivot.h>
#include <runtime/na-utils.h>
#include <runtime/na-xml-names.h>
#include <runtime/na-xml-writer.h>

#include "nact-application.h"
#include "nact-assistant-export-ask.h"
#include "nact-tree-model.h"
#include "nact-clipboard.h"

/* private class data
 */
struct NactClipboardClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
typedef struct {
	guint    target;
	gchar   *folder;
	GList   *rows;
	gboolean copy;
}
	NactClipboardDndData;

typedef struct {
	guint    nb_actions;
	guint    nb_profiles;
	guint    nb_menus;
	GList   *items;
	gint     mode;
}
	NactClipboardPrimaryData;

struct NactClipboardPrivate {
	gboolean                  dispose_has_run;
	BaseWindow               *window;
	GtkClipboard             *dnd;
	GtkClipboard             *primary;
	NactClipboardPrimaryData *primary_data;
	gboolean                  primary_got;
};

#define NACT_CLIPBOARD_ATOM				gdk_atom_intern( "_NACT_CLIPBOARD", FALSE )
#define NACT_CLIPBOARD_NACT_ATOM		gdk_atom_intern( "ClipboardNautilusActions", FALSE )

enum {
	NACT_CLIPBOARD_FORMAT_NACT = 0,
	NACT_CLIPBOARD_FORMAT_APPLICATION_XML,
	NACT_CLIPBOARD_FORMAT_TEXT_PLAIN
};

/* clipboard formats
 * - a special ClipboardNautilusAction format for internal move/copy
 *   and also used by drag and drop operations
 * - a XdndDirectSave, suitable for exporting to a file manager
 *   (note that Nautilus recognized the "XdndDirectSave0" format as XDS
 *   protocol)
 * - a text (xml) format, to go to clipboard or a text editor
 */
static GtkTargetEntry clipboard_formats[] = {
	{ "ClipboardNautilusActions", 0, NACT_CLIPBOARD_FORMAT_NACT },
	{ "application/xml",          0, NACT_CLIPBOARD_FORMAT_APPLICATION_XML },
	{ "text/plain",               0, NACT_CLIPBOARD_FORMAT_TEXT_PLAIN },
};

static GObjectClass *st_parent_class = NULL;

static GType  register_type( void );
static void   class_init( NactClipboardClass *klass );
static void   instance_init( GTypeInstance *instance, gpointer klass );
static void   instance_dispose( GObject *application );
static void   instance_finalize( GObject *application );

static void   get_from_dnd_clipboard_callback( GtkClipboard *clipboard, GtkSelectionData *selection_data, guint info, guchar *data );
static void   clear_dnd_clipboard_callback( GtkClipboard *clipboard, NactClipboardDndData *data );
static gchar *export_rows( NactClipboard *clipboard, GList *rows, const gchar *dest_folder );
static gchar *export_objects( NactClipboard *clipboard, GList *objects, const gchar *dest_folder );
static gchar *export_row_object( NactClipboard *clipboard, NAObject *object, const gchar *dest_folder, GList **exported );

static void   get_from_primary_clipboard_callback( GtkClipboard *gtk_clipboard, GtkSelectionData *selection_data, guint info, NactClipboard *clipboard );
static void   clear_primary_clipboard_callback( GtkClipboard *gtk_clipboard, NactClipboard *clipboard );

GType
nact_clipboard_get_type( void )
{
	static GType type = 0;

	if( !type ){
		type = register_type();
	}

	return( type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "nact_clipboard_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NactClipboardClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactClipboard ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "NactClipboard", &info, 0 );

	return( type );
}

static void
class_init( NactClipboardClass *klass )
{
	static const gchar *thisfn = "nact_clipboard_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactClipboardClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_clipboard_instance_init";
	NactClipboard *self;
	GdkDisplay *display;

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );
	g_assert( NACT_IS_CLIPBOARD( instance ));
	self = NACT_CLIPBOARD( instance );

	self->private = g_new0( NactClipboardPrivate, 1 );

	self->private->dispose_has_run = FALSE;

	display = gdk_display_get_default();
	self->private->dnd = gtk_clipboard_get_for_display( display, NACT_CLIPBOARD_ATOM );
	self->private->primary = gtk_clipboard_get_for_display( display, GDK_SELECTION_CLIPBOARD );
}

static void
instance_dispose( GObject *window )
{
	static const gchar *thisfn = "nact_clipboard_instance_dispose";
	NactClipboard *self;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );
	g_assert( NACT_IS_CLIPBOARD( window ));
	self = NACT_CLIPBOARD( window );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		gtk_clipboard_clear( self->private->dnd );
		gtk_clipboard_clear( self->private->primary );

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( window );
		}
	}
}

static void
instance_finalize( GObject *window )
{
	static const gchar *thisfn = "nact_clipboard_instance_finalize";
	NactClipboard *self;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );
	g_assert( NACT_IS_CLIPBOARD( window ));
	self = NACT_CLIPBOARD( window );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( window );
	}
}

/**
 * nact_clipboard_new:
 *
 * Returns: a new #NactClipboard object.
 */
NactClipboard *
nact_clipboard_new( BaseWindow *window )
{
	NactClipboard *clipboard;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), NULL );

	clipboard = g_object_new( NACT_CLIPBOARD_TYPE, NULL );

	clipboard->private->window = window;

	return( clipboard );
}

/**
 * nact_clipboard_dnd_set:
 * @clipboard: this #NactClipboard instance.
 * @rows: the list of row references of dragged items.
 * @folder: the target folder if any (XDS protocol to outside).
 * @copy_data: %TRUE if data is to be copied, %FALSE else
 *  (only relevant when drag and drop occurs inside of the tree view).
 *
 * Set the selected items into our dnd clipboard.
 */
void
nact_clipboard_dnd_set( NactClipboard *clipboard, guint target, GList *rows, const gchar *folder, gboolean copy_data )
{
	static const gchar *thisfn = "nact_clipboard_dnd_set";
	NactClipboardDndData *data;
	GtkTreeModel *model;
	GList *it;

	g_return_if_fail( NACT_IS_CLIPBOARD( clipboard ));
	g_return_if_fail( rows && g_list_length( rows ));

	if( !clipboard->private->dispose_has_run ){

		data = g_new0( NactClipboardDndData, 1 );

		data->target = target;
		data->folder = g_strdup( folder );
		data->rows = NULL;
		data->copy = copy_data;

		model = gtk_tree_row_reference_get_model(( GtkTreeRowReference * ) rows->data );

		for( it = rows ; it ; it = it->next ){
			data->rows = g_list_append(
					data->rows,
					gtk_tree_row_reference_copy(( GtkTreeRowReference * ) it->data ));
		}

		gtk_clipboard_set_with_data( clipboard->private->dnd,
				clipboard_formats, G_N_ELEMENTS( clipboard_formats ),
				( GtkClipboardGetFunc ) get_from_dnd_clipboard_callback,
				( GtkClipboardClearFunc ) clear_dnd_clipboard_callback,
				data );

		g_debug( "%s: clipboard=%p, data=%p", thisfn, ( void * ) clipboard, ( void * ) data );
	}
}

/**
 * nact_clipboard_dnd_get_data:
 * @clipboard: this #NactClipboard instance.
 * @copy_data: will be set to the original value of the drag and drop.
 *
 * Returns the list of rows references privously stored.
 *
 * The returned list should be gtk_tree_row_reference_free() by the
 * caller.
 */
GList *
nact_clipboard_dnd_get_data( NactClipboard *clipboard, gboolean *copy_data )
{
	static const gchar *thisfn = "nact_clipboard_dnd_get_data";
	GList *rows = NULL;
	GtkSelectionData *selection;
	NactClipboardDndData *data;
	GtkTreeModel *model;
	GList *it;

	g_debug( "%s: clipboard=%p", thisfn, ( void * ) clipboard );
	g_return_val_if_fail( NACT_IS_CLIPBOARD( clipboard ), NULL );

	if( copy_data ){
		*copy_data = FALSE;
	}

	if( !clipboard->private->dispose_has_run ){

		selection = gtk_clipboard_wait_for_contents( clipboard->private->dnd, NACT_CLIPBOARD_NACT_ATOM );
		if( selection ){

			data = ( NactClipboardDndData * ) selection->data;
			if( data->target == NACT_XCHANGE_FORMAT_NACT ){

				model = gtk_tree_row_reference_get_model(( GtkTreeRowReference * ) data->rows->data );

				for( it = data->rows ; it ; it = it->next ){
					rows = g_list_append( rows,
							gtk_tree_row_reference_copy(( GtkTreeRowReference * ) it->data ));
				}
				*copy_data = data->copy;
			}
		}
	}

	return( rows );
}

/*
 * Get text/plain from selected actions.
 *
 * This is called when we drop or paste a selection onto an application
 * willing to deal with Xdnd protocol, for text/plain or application/xml
 * mime types.
 *
 * Selected items may include menus, actions and profiles.
 * For now, we only exports actions (and profiles) as XML files.
 *
 * FIXME: na_xml_writer_get_xml_buffer() returns a valid XML document,
 * which includes the <?xml ...?> header. Concatenating several valid
 * XML documents doesn't provide a valid global XML doc, because of
 * the presence of several <?xml ?> headers inside.
 */
gchar *
nact_clipboard_dnd_get_text( NactClipboard *clipboard, GList *rows )
{
	gchar *buffer;

	g_return_val_if_fail( NACT_IS_CLIPBOARD( clipboard ), NULL );

	buffer = NULL;

	if( !clipboard->private->dispose_has_run ){
		buffer = export_rows( clipboard, rows, NULL );
	}

	return( buffer );
}

/**
 * nact_clipboard_dnd_drag_end:
 * @clipboard: this #NactClipboard instance.
 *
 * On drag-end, exports the objects if needed.
 */
void
nact_clipboard_dnd_drag_end( NactClipboard *clipboard )
{
	static const gchar *thisfn = "nact_clipboard_dnd_drag_end";
	GtkSelectionData *selection;
	NactClipboardDndData *data;

	g_debug( "%s: clipboard=%p", thisfn, ( void * ) clipboard );
	g_return_if_fail( NACT_IS_CLIPBOARD( clipboard ));

	if( !clipboard->private->dispose_has_run ){

		selection = gtk_clipboard_wait_for_contents( clipboard->private->dnd, NACT_CLIPBOARD_NACT_ATOM );
		if( selection ){

			data = ( NactClipboardDndData * ) selection->data;
			g_debug( "%s: data=%p (NactClipboardDndData)", thisfn, ( void * ) data );
			if( data->target == NACT_XCHANGE_FORMAT_XDS ){
				export_rows( clipboard, data->rows, data->folder );
			}
		}
	}
}

/**
 * nact_clipboard_dnd_clear:
 * @clipboard: this #NactClipboard instance.
 *
 * Clears the drag-and-drop clipboard.
 *
 * At least called on drag-begin.
 */
void
nact_clipboard_dnd_clear( NactClipboard *clipboard )
{
	gtk_clipboard_clear( clipboard->private->dnd );
}

static void
get_from_dnd_clipboard_callback( GtkClipboard *clipboard, GtkSelectionData *selection_data, guint info, guchar *data )
{
	static const gchar *thisfn = "nact_clipboard_get_from_dnd_clipboard_callback";

	g_debug( "%s: clipboard=%p, selection_data=%p, target=%s, info=%d, data=%p",
			thisfn, ( void * ) clipboard,
			( void * ) selection_data, gdk_atom_name( selection_data->target ), info, ( void * ) data );

	gtk_selection_data_set( selection_data, selection_data->target, 8, data, sizeof( NactClipboardDndData ));
}

static void
clear_dnd_clipboard_callback( GtkClipboard *clipboard, NactClipboardDndData *data )
{
	static const gchar *thisfn = "nact_clipboard_clear_dnd_clipboard_callback";

	g_debug( "%s: clipboard=%p, data=%p", thisfn, ( void * ) clipboard, ( void * ) data );

	g_free( data->folder );
	g_list_foreach( data->rows, ( GFunc ) gtk_tree_row_reference_free, NULL );
	g_list_free( data->rows );
	g_free( data );
}

static gchar *
export_rows( NactClipboard *clipboard, GList *rows, const gchar *dest_folder )
{
	GString *data;
	GtkTreeModel *model;
	GList *exported, *irow;
	GtkTreePath *path;
	GtkTreeIter iter;
	NAObject *object;
	gchar *buffer;

	buffer = NULL;
	exported = NULL;
	data = g_string_new( "" );
	model = gtk_tree_row_reference_get_model(( GtkTreeRowReference * ) rows->data );

	for( irow = rows ; irow ; irow = irow->next ){
		path = gtk_tree_row_reference_get_path(( GtkTreeRowReference * ) irow->data );
		if( path ){
			gtk_tree_model_get_iter( model, &iter, path );
			gtk_tree_path_free( path );
			gtk_tree_model_get( model, &iter, IACTIONS_LIST_NAOBJECT_COLUMN, &object, -1 );
			buffer = export_row_object( clipboard, object, dest_folder, &exported );
			if( buffer && strlen( buffer )){
				data = g_string_append( data, buffer );
				g_free( buffer );
			}
			g_object_unref( object );
		}
	}

	g_list_free( exported );
	return( g_string_free( data, FALSE ));
}

static gchar *
export_objects( NactClipboard *clipboard, GList *objects, const gchar *dest_folder )
{
	gchar *buffer;
	GString *data;
	GList *exported;
	GList *iobj;
	NAObject *object;

	buffer = NULL;
	exported = NULL;
	data = g_string_new( "" );

	for( iobj = objects ; iobj ; iobj = iobj->next ){
		object = NA_OBJECT( iobj->data );
		buffer = export_row_object( clipboard, object, dest_folder, &exported );
		if( buffer && strlen( buffer )){
			data = g_string_append( data, buffer );
			g_free( buffer );
		}
		g_object_unref( object );
	}

	g_list_free( exported );
	return( g_string_free( data, FALSE ));
}

static gchar *
export_row_object( NactClipboard *clipboard, NAObject *object, const gchar *dest_folder, GList **exported )
{
	GList *subitems, *isub;
	NAObjectAction *action;
	gint index;
	GString *data;
	gchar *buffer;
	NactApplication *application;
	NAPivot *pivot;
	gint format;
	gchar *fname;
	GSList *msg;

	if( NA_IS_OBJECT_MENU( object )){
		subitems = na_object_get_items_list( object );
		data = g_string_new( "" );

		for( isub = subitems ; isub ; isub = isub->next ){
			buffer = export_row_object( clipboard, isub->data, dest_folder, exported );
			if( buffer && strlen( buffer )){
				data = g_string_append( data, buffer );
				g_free( buffer );
			}
		}

		return( g_string_free( data, FALSE ));
	}

	msg = NULL;
	buffer = NULL;
	action = ( NAObjectAction * ) object;
	if( NA_IS_OBJECT_PROFILE( object )){
		action = NA_OBJECT_ACTION( na_object_get_parent( object ));
	}

	index = g_list_index( *exported, ( gconstpointer ) action );
	if( index == -1 ){
		*exported = g_list_prepend( *exported, ( gpointer ) action );

		application = NACT_APPLICATION( base_window_get_application( clipboard->private->window ));
		pivot = nact_application_get_pivot( application );
		format = na_iprefs_get_export_format( NA_IPREFS( pivot ), IPREFS_EXPORT_FORMAT );

		if( format == IPREFS_EXPORT_FORMAT_ASK ){
			format = nact_assistant_export_ask_user( clipboard->private->window, action );
		}

		if( format != IPREFS_EXPORT_NO_EXPORT ){
			buffer = na_xml_writer_get_xml_buffer( action, format );

			if( buffer && dest_folder ){
				fname = na_xml_writer_get_output_fname( action, dest_folder, format );
				na_xml_writer_output_xml( buffer, fname, &msg );
				g_free( fname );
				g_free( buffer );
				buffer = NULL;
			}
		}
	}

	return( buffer );
}

/**
 * nact_clipboard_primary_set:
 * @clipboard: this #NactClipboard object.
 * @items: a list of #NAObject items
 * @mode: where do these items come from ?
 *  Or what is the operation which has led the items to the clipboard?
 *
 * Installs a copy of provided items in the clipboard.
 *
 * Rationale: when cutting an item to the clipboard, the next paste
 * will keep its same original id, and it is safe because this is
 * actually what we want when we cut/paste.
 *
 * Contrarily, when we copy/paste, we are waiting for a new element
 * which has the same characteristics that the previous one ; we so
 * have to renumber actions/menus items when copying into the clipboard.
 *
 * Note that we use NAIDuplicable interface without actually taking care
 * of what is origin or so, as origin will be reinitialized when getting
 * data out of the clipboard.
 */
void
nact_clipboard_primary_set( NactClipboard *clipboard, GList *items, gint mode )
{
	static const gchar *thisfn = "nact_clipboard_primary_set";
	NactClipboardPrimaryData *data;
	GList *it;

	g_debug( "%s: clipboard=%p, items=%p (count=%d), mode=%d",
			thisfn, ( void * ) clipboard, ( void * ) items, g_list_length( items ), mode );
	g_return_if_fail( NACT_IS_CLIPBOARD( clipboard ));

	if( !clipboard->private->dispose_has_run ){

		g_return_if_fail( clipboard->private->primary_data == NULL );

		data = g_new0( NactClipboardPrimaryData, 1 );

		na_object_item_count_items( items,
				( gint * ) &data->nb_menus, ( gint * ) &data->nb_actions, ( gint * ) &data->nb_profiles, FALSE );

		for( it = items ; it ; it = it->next ){
			data->items = g_list_prepend( data->items, na_object_duplicate( it->data ));
		}
		data->items = g_list_reverse( data->items );

		data->mode = mode;

		clipboard->private->primary_data = data;
		clipboard->private->primary_got = FALSE;
		g_debug( "%s: data=%p (NactClipboardPrimaryData)", thisfn, ( void * ) data );

		gtk_clipboard_set_with_data( clipboard->private->primary,
				clipboard_formats, G_N_ELEMENTS( clipboard_formats ),
				( GtkClipboardGetFunc ) get_from_primary_clipboard_callback,
				( GtkClipboardClearFunc ) clear_primary_clipboard_callback,
				clipboard );
	}
}

/**
 * nact_clipboard_primary_get:
 * @clipboard: this #NactClipboard object.
 *
 * Returns: a copy of the list of items previously referenced in the
 * internal clipboard.
 *
 * We allocate a new id for items in order to be ready to paste another
 * time.
 */
GList *
nact_clipboard_primary_get( NactClipboard *clipboard, gboolean *relabel )
{
	static const gchar *thisfn = "nact_clipboard_primary_get";
	GtkSelectionData *selection;
	NactClipboardPrimaryData *data;
	GList *items = NULL;
	GList *it;
	NAObject *obj;

	g_debug( "%s: clipboard=%p", thisfn, ( void * ) clipboard );
	g_return_val_if_fail( NACT_IS_CLIPBOARD( clipboard ), NULL );
	g_return_val_if_fail( relabel, NULL );

	if( !clipboard->private->dispose_has_run ){

		selection = gtk_clipboard_wait_for_contents( clipboard->private->primary, NACT_CLIPBOARD_NACT_ATOM );

		if( selection ){
			data = ( NactClipboardPrimaryData * ) selection->data;
			g_debug( "%s: data=%p (NactClipboardPrimaryData)", thisfn, ( void * ) data );

			for( it = data->items ; it ; it = it->next ){
				obj = na_object_duplicate( it->data );
				na_object_set_origin( obj, NULL );
				items = g_list_prepend( items, obj );
			}
			items = g_list_reverse( items );

			*relabel = (( data->mode == CLIPBOARD_MODE_CUT && clipboard->private->primary_got ) ||
							data->mode == CLIPBOARD_MODE_COPY );

			clipboard->private->primary_got = TRUE;
		}
	}

	return( items );
}

/**
 * nact_clipboard_primary_counts:
 * @clipboard: this #NactClipboard object.
 *
 * Returns some counters on content of primary clipboard.
 */
void
nact_clipboard_primary_counts( NactClipboard *clipboard, guint *actions, guint *profiles, guint *menus )
{
	GtkSelectionData *selection;
	NactClipboardPrimaryData *data;

	g_return_if_fail( NACT_IS_CLIPBOARD( clipboard ));
	g_return_if_fail( actions && profiles && menus );

	if( !clipboard->private->dispose_has_run ){

		*actions = 0;
		*profiles = 0;
		*menus = 0;

		selection = gtk_clipboard_wait_for_contents( clipboard->private->primary, NACT_CLIPBOARD_NACT_ATOM );

		if( selection ){
			data = ( NactClipboardPrimaryData * ) selection->data;

			*actions = data->nb_actions;
			*profiles = data->nb_profiles;
			*menus = data->nb_menus;
		}
	}
}

static void
get_from_primary_clipboard_callback( GtkClipboard *gtk_clipboard, GtkSelectionData *selection_data, guint info, NactClipboard *clipboard )
{
	static const gchar *thisfn = "nact_clipboard_get_from_primary_clipboard_callback";
	NactClipboardPrimaryData *data;
	gchar *buffer;

	g_debug( "%s: gtk_clipboard=%p, selection_data=%p, target=%s, info=%d, clipboard=%p",
			thisfn, ( void * ) gtk_clipboard,
			( void * ) selection_data, gdk_atom_name( selection_data->target ), info, ( void * ) clipboard );

	data = clipboard->private->primary_data;

	if( info == NACT_CLIPBOARD_FORMAT_TEXT_PLAIN ){
		buffer = export_objects( clipboard, data->items, NULL );
		gtk_selection_data_set( selection_data, selection_data->target, 8, ( const guchar * ) buffer, strlen( buffer ));
		g_free( buffer );

	} else {
		gtk_selection_data_set( selection_data, selection_data->target, 8, ( const guchar * ) data, sizeof( NactClipboardPrimaryData ));
	}
}

static void
clear_primary_clipboard_callback( GtkClipboard *gtk_clipboard, NactClipboard *clipboard )
{
	static const gchar *thisfn = "nact_clipboard_clear_primary_clipboard_callback";
	NactClipboardPrimaryData *data;

	g_debug( "%s: gtk_clipboard=%p, clipboard=%p",
			thisfn, ( void * ) gtk_clipboard, ( void * ) clipboard );

	data = clipboard->private->primary_data;
	g_list_foreach( data->items, ( GFunc ) g_object_unref, NULL );
	g_list_free( data->items );
	g_free( data );
	clipboard->private->primary_data = NULL;
}
