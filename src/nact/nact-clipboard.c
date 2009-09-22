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

#include <common/na-object-api.h>
#include <common/na-object-action-class.h>
#include <common/na-object-menu.h>
#include <common/na-object-profile.h>
#include <common/na-xml-names.h>
#include <common/na-xml-writer.h>

#include "nact-clipboard.h"

/* private class data
 */
struct NactClipboardClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NactClipboardPrivate {
	gboolean      dispose_has_run;
	GtkClipboard *primary;
};

#define NACT_CLIPBOARD_ATOM				gdk_atom_intern( "_NACT_CLIPBOARD", FALSE )
#define NACT_CLIPBOARD_NACT_ATOM		gdk_atom_intern( "ClipboardNautilusActions", FALSE )

enum {
	NACT_CLIPBOARD_FORMAT_NACT = 0,
	NACT_CLIPBOARD_FORMAT_APPLICATION_XML,
	NACT_CLIPBOARD_FORMAT_TEXT_PLAIN
};

/* clipboard formats
 * - a special XdndNautilusAction format for internal move/copy
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

typedef struct {
	guint  nb_actions;
	guint  nb_profiles;
	guint  nb_menus;
	GList *items;
}
	NactClipboardData;

static GObjectClass *st_parent_class = NULL;

static GType         register_type( void );
static void          class_init( NactClipboardClass *klass );
static void          instance_init( GTypeInstance *instance, gpointer klass );
static void          instance_dispose( GObject *application );
static void          instance_finalize( GObject *application );

static GtkClipboard *get_nact_clipboard( void );
static GtkClipboard *get_primary_clipboard( void );
static void          add_item_to_clipboard0( NAObject *object, gboolean copy_data, gboolean only_profiles, GList **copied );
/*static void          add_item_to_clipboard( NAObject *object, GList **copied );*/
static void          export_action( const gchar *uri, const NAObject *action, GSList **exported );
static gchar        *get_action_xml_buffer( const NAObject *action, GSList **exported );
static void          get_from_clipboard_callback( GtkClipboard *clipboard, GtkSelectionData *selection_data, guint info, guchar *data );
static void          clear_clipboard_callback( GtkClipboard *clipboard, NactClipboardData *data );
static void          renumber_items( GList *items );

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

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );
	g_assert( NACT_IS_CLIPBOARD( instance ));
	self = NACT_CLIPBOARD( instance );

	self->private = g_new0( NactClipboardPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->primary = get_primary_clipboard();
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
nact_clipboard_new( void )
{
	NactClipboard *clipboard;

	clipboard = g_object_new( NACT_CLIPBOARD_TYPE, NULL );

	return( clipboard );
}

/**
 * nact_clipboard_get_data_for_intern_use:
 *
 * Set the selected items into our custom clipboard.
 *
 * Note that we take a copy of the selected items, so that we will be
 * able to paste them when needed.
 */
void
nact_clipboard_get_data_for_intern_use( GList *selected_items, gboolean copy_data )
{
	static const gchar *thisfn = "nact_clipboard_get_data_for_intern_use";
	GtkClipboard *clipboard;
	NactClipboardData *data;
	GList *it;

	clipboard = get_nact_clipboard();

	data = g_new0( NactClipboardData, 1 );
	/*data->only_profiles = have_only_profiles( selected_items );*/

	for( it = selected_items ; it ; it = it->next ){
		NAObject *item_object = NA_OBJECT( it->data );
		add_item_to_clipboard0( item_object, copy_data, FALSE /*data->only_profiles*/, &data->items );
	}
	data->items = g_list_reverse( data->items );

	gtk_clipboard_set_with_data( clipboard,
			clipboard_formats, G_N_ELEMENTS( clipboard_formats ),
			( GtkClipboardGetFunc ) get_from_clipboard_callback,
			( GtkClipboardClearFunc ) clear_clipboard_callback,
			data );

	g_debug( "%s: clipboard=%p, data=%p", thisfn, ( void * ) clipboard, ( void * ) data );
}

/**
 * Get text/plain from selected actions.
 *
 * This is called when we drop or paste a selection onto an application
 * willing to deal with Xdnd protocol, for text/plain or application/xml
 * mime types.
 *
 * Selected items may include menus, actions and profiles.
 * For now, we only exports actions as XML files.
 */
char *
nact_clipboard_get_data_for_extern_use( GList *selected_items )
{
	GList *it;
	GSList *exported = NULL;
	GString *data;
	gchar *chunk;

	data = g_string_new( "" );

	for( it = selected_items ; it ; it = it->next ){
		NAObject *item_object = NA_OBJECT( it->data );
		chunk = NULL;

		if( NA_IS_OBJECT_ACTION( item_object )){
			chunk = get_action_xml_buffer( item_object, &exported );

		} else if( NA_IS_OBJECT_PROFILE( item_object )){
			NAObjectAction *action = na_object_profile_get_action( NA_OBJECT_PROFILE( item_object ));
			chunk = get_action_xml_buffer( NA_OBJECT( action ), &exported );
		}

		if( chunk && strlen( chunk )){
			data = g_string_append( data, chunk );
		}
		g_free( chunk );
	}

	g_slist_free( exported );
	return( g_string_free( data, FALSE ));
}

/**
 * Exports selected actions.
 *
 * This is called when we drop or paste a selection onto an application
 * willing to deal with XdndDirectSave (XDS) protocol.
 *
 * Selected items may include menus, actions and profiles.
 * For now, we only exports actions as XML files.
 */
void
nact_clipboard_export_items( const gchar *uri, GList *items )
{
	GList *it;
	GSList *exported = NULL;

	for( it = items ; it ; it = it->next ){
		NAObject *item_object = NA_OBJECT( it->data );

		if( NA_IS_OBJECT_ACTION( item_object )){
			export_action( uri, item_object, &exported );

		} else if( NA_IS_OBJECT_PROFILE( item_object )){
			NAObjectAction *action = na_object_profile_get_action( NA_OBJECT_PROFILE( item_object ));
			export_action( uri, NA_OBJECT( action ), &exported );
		}
	}

	g_slist_free( exported );
}

/**
 * nact_clipboard_primary_set:
 * @items: a list of #NAObject items
 * @renumber_items: whether the actions or menus items should be
 * renumbered when copied in the clipboard ?
 *
 * Installs a copy of provided items in the clipboard.
 *
 * Rationale: when cutting an item to the clipboard, the next paste
 * will keep its same original id, and it is safe because this is
 * actually what we we want when we cut/paste.
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
nact_clipboard_primary_set( GList *items, gboolean renumber )
{
	GtkClipboard *clipboard;
	NactClipboardData *data;
	GList *it;

	clipboard = get_primary_clipboard();
	data = g_new0( NactClipboardData, 1 );

	for( it = items ; it ; it = it->next ){
		data->items = g_list_prepend( data->items, na_object_duplicate( it->data ));

		if( NA_IS_OBJECT_ACTION( it->data )){
			data->nb_actions += 1;

		} else if( NA_IS_OBJECT_MENU( it->data )){
			data->nb_menus += 1;

		} else if( NA_IS_OBJECT_PROFILE( it->data )){
			data->nb_profiles += 1;
		}
	}
	data->items = g_list_reverse( data->items );

	if( renumber ){
		renumber_items( data->items );
	}

	gtk_clipboard_set_with_data( clipboard,
			clipboard_formats, G_N_ELEMENTS( clipboard_formats ),
			( GtkClipboardGetFunc ) get_from_clipboard_callback,
			( GtkClipboardClearFunc ) clear_clipboard_callback,
			data );
}

/**
 * nact_clipboard_primary_get:
 *
 * Returns: a copy of the list of items previously referenced in the
 * internal clipboard.
 *
 * We allocate a new id for items in order to be ready to paste another
 * time.
 */
GList *
nact_clipboard_primary_get( void )
{
	GtkClipboard *clipboard;
	GtkSelectionData *selection;
	NactClipboardData *data;
	GList *items, *it;
	NAObject *obj;

	clipboard = get_primary_clipboard();

	items = NULL;

	selection = gtk_clipboard_wait_for_contents( clipboard, NACT_CLIPBOARD_NACT_ATOM );

	if( selection ){
		data = ( NactClipboardData * ) selection->data;

		for( it = data->items ; it ; it = it->next ){
			obj = na_object_duplicate( it->data );
			na_object_set_origin( obj, NULL );
			items = g_list_prepend( items, obj );
		}
		items = g_list_reverse( items );

		renumber_items( data->items );
	}

	return( items );
}

/**
 * nact_clipboard_primary_counts:
 *
 * Returns some counters on content of primary clipboard.
 */
void
nact_clipboard_primary_counts( guint *actions, guint *profiles, guint *menus )
{
	GtkClipboard *clipboard;
	GtkSelectionData *selection;
	NactClipboardData *data;

	g_return_if_fail( actions && profiles && menus );
	*actions = 0;
	*profiles = 0;
	*menus = 0;

	clipboard = get_primary_clipboard();

	selection = gtk_clipboard_wait_for_contents( clipboard, NACT_CLIPBOARD_NACT_ATOM );

	if( selection ){
		data = ( NactClipboardData * ) selection->data;

		*actions = data->nb_actions;
		*profiles = data->nb_profiles;
		*menus = data->nb_menus;
	}
}

static GtkClipboard *
get_nact_clipboard( void )
{
	GdkDisplay *display;
	GtkClipboard *clipboard;

	display = gdk_display_get_default();
	clipboard = gtk_clipboard_get_for_display( display, NACT_CLIPBOARD_ATOM );

	return( clipboard );
}

static GtkClipboard *
get_primary_clipboard( void )
{
	GdkDisplay *display;
	GtkClipboard *clipboard;

	display = gdk_display_get_default();
	clipboard = gtk_clipboard_get_for_display( display, GDK_SELECTION_PRIMARY );

	return( clipboard );
}

static void
add_item_to_clipboard0( NAObject *object, gboolean copy_data, gboolean only_profiles, GList **items_list )
{
	NAObject *source;
	gint index;

	source = object;
	if( !only_profiles && NA_IS_OBJECT_PROFILE( object )){
		source = NA_OBJECT( na_object_profile_get_action( NA_OBJECT_PROFILE( object )));
	}

	index = g_list_index( *items_list, ( gconstpointer ) source );
	if( index != -1 ){
		return;
	}

	*items_list = g_list_prepend( *items_list, na_object_ref( source ));
}

/*static void
add_item_to_clipboard( NAObject *object, GList **items_list )
{
	*items_list = g_list_prepend( *items_list, na_object_ref( object ));
}*/

static void
export_action( const gchar *uri, const NAObject *action, GSList **exported )
{
	gint index;
	gchar *fname, *buffer;

	index = g_slist_index( *exported, ( gconstpointer ) action );
	if( index != -1 ){
		return;
	}

	fname = na_xml_writer_get_output_fname( NA_OBJECT_ACTION( action ), uri, FORMAT_GCONFENTRY );
	buffer = na_xml_writer_get_xml_buffer( NA_OBJECT_ACTION( action ), FORMAT_GCONFENTRY );

	na_xml_writer_output_xml( buffer, fname );

	g_free( buffer );
	g_free( fname );

	*exported = g_slist_prepend( *exported, ( gpointer ) action );
}

static gchar *
get_action_xml_buffer( const NAObject *action, GSList **exported )
{
	gint index;
	gchar *buffer;

	index = g_slist_index( *exported, ( gconstpointer ) action );
	if( index != -1 ){
		return( NULL );
	}

	buffer = na_xml_writer_get_xml_buffer( NA_OBJECT_ACTION( action ), FORMAT_GCONFENTRY );

	*exported = g_slist_prepend( *exported, ( gpointer ) action );

	return( buffer );
}

static void
get_from_clipboard_callback( GtkClipboard *clipboard, GtkSelectionData *selection_data, guint info, guchar *data )
{
	static const gchar *thisfn = "nact_clipboard_get_from_clipboard_callback";

	g_debug( "%s: clipboard=%p, selection_data=%p, target=%s, info=%d, data=%p",
			thisfn, ( void * ) clipboard,
			( void * ) selection_data, gdk_atom_name( selection_data->target ), info, ( void * ) data );

	gtk_selection_data_set( selection_data, selection_data->target, 8, data, sizeof( NactClipboardData ));
}

static void
clear_clipboard_callback( GtkClipboard *clipboard, NactClipboardData *data )
{
	static const gchar *thisfn = "nact_clipboard_clear_clipboard_callback";

	g_debug( "%s: clipboard=%p, data=%p", thisfn, ( void * ) clipboard, ( void * ) data );

	g_list_foreach( data->items, ( GFunc ) g_object_unref, NULL );
	g_list_free( data->items );
	g_free( data );
}

static void
renumber_items( GList *items )
{
	GList *it;

	for( it = items ; it ; it = it->next ){
		if( NA_IS_OBJECT_ITEM( it->data )){
			na_object_set_new_id( NA_OBJECT_ITEM( it->data ));
		}
	}
}
