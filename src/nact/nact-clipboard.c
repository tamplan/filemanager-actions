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
#include <common/na-obj-action-class.h>
#include <common/na-obj-profile.h>
#include <common/na-xml-names.h>
#include <common/na-xml-writer.h>

#include "nact-clipboard.h"

#define NACT_CLIPBOARD_ATOM				gdk_atom_intern( "_NACT_CLIPBOARD", FALSE )

#define CLIPBOARD_PROP_PRIMAY_USED		"nact-clipboard-primary-used"

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
	gboolean only_profiles;
	GSList  *items;
}
	NactClipboardData;

static GtkClipboard *get_clipboard( void );
static gboolean      have_only_profiles( GSList *items );
static void          add_item_to_clipboard0( NAObject *object, gboolean copy_data, gboolean only_profiles, GSList **copied );
static void          add_item_to_clipboard( NAObject *object, GSList **copied );
static void          export_action( const gchar *uri, const NAObject *action, GSList **exported );
static gchar        *get_action_xml_buffer( const NAObject *action, GSList **exported );
static void          get_from_clipboard_callback( GtkClipboard *clipboard, GtkSelectionData *selection_data, guint info, NactClipboardData *data );
static void          clear_clipboard_callback( GtkClipboard *clipboard, NactClipboardData *data );

/**
 * nact_clipboard_get_data_for_intern_use:
 *
 * Set the selected items into our custom clipboard.
 *
 * Note that we take a copy of the selected items, so that we will be
 * able to paste them when needed.
 */
void
nact_clipboard_get_data_for_intern_use( GSList *selected_items, gboolean copy_data )
{
	static const gchar *thisfn = "nact_clipboard_get_data_for_intern_use";
	GtkClipboard *clipboard;
	NactClipboardData *data;
	GSList *item;

	clipboard = get_clipboard();

	data = g_new0( NactClipboardData, 1 );
	data->only_profiles = have_only_profiles( selected_items );

	for( item = selected_items ; item ; item = item->next ){
		NAObject *item_object = NA_OBJECT( item->data );
		add_item_to_clipboard0( item_object, copy_data, data->only_profiles, &data->items );
	}
	data->items = g_slist_reverse( data->items );

	gtk_clipboard_set_with_data( clipboard,
			clipboard_formats, G_N_ELEMENTS( clipboard_formats ),
			( GtkClipboardGetFunc ) get_from_clipboard_callback,
			( GtkClipboardClearFunc ) clear_clipboard_callback,
			data );

	g_object_set_data( G_OBJECT( clipboard ), CLIPBOARD_PROP_PRIMAY_USED, GINT_TO_POINTER( TRUE ));
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
nact_clipboard_get_data_for_extern_use( GSList *selected_items )
{
	GSList *item;
	GSList *exported = NULL;
	GString *data;
	gchar *chunk;

	data = g_string_new( "" );

	for( item = selected_items ; item ; item = item->next ){
		NAObject *item_object = NA_OBJECT( item->data );
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
nact_clipboard_export_items( const gchar *uri, GSList *items )
{
	GSList *item;
	GSList *exported = NULL;

	for( item = items ; item ; item = item->next ){
		NAObject *item_object = NA_OBJECT( item->data );

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
 * nact_clipboard_is_empty:
 *
 * This is called to know if we can enable paste item in the menubar.
 */
gboolean
nact_clipboard_is_empty( void )
{
	GtkClipboard *clipboard;
	gboolean used;
	gpointer data;

	clipboard = get_clipboard();
	data = g_object_get_data( G_OBJECT( clipboard ), CLIPBOARD_PROP_PRIMAY_USED );
	used = ( gboolean ) GPOINTER_TO_INT( data );
	/*g_debug( "nact_clipboard_is_empty: used=%s", used ? "True":"False" );*/

	return( !used );
}

/**
 * nact_clipboard_get:
 *
 * Returns: the list of items previously referenced in the internal
 * clipboard.
 *
 * The list is owned by the clipboard, and should not be g_free() nor
 * g_object_unref() by the caller.
 */
GSList *
nact_clipboard_get( void )
{
	GtkClipboard *clipboard;
	GtkSelectionData *selection;
	NactClipboardData *data;
	GSList *items;

	if( nact_clipboard_is_empty()){
		return( NULL );
	}

	clipboard = get_clipboard();
	selection = gtk_clipboard_wait_for_contents( clipboard, GDK_SELECTION_PRIMARY );
	data = ( NactClipboardData * ) selection->data;

	/* prepare the next paste by renumeroting the ids */
	/*for( it = items ; it ; it = it->next ){
		na_object_set_new_id( it->data );
	}*/

	return( data->items );
}

/**
 * nact_clipboard_set:
 * @items: a list of #NAObject items
 *
 * Takes a reference on the specified list of items, and installs them
 * in the internal clipboard.
 */
void
nact_clipboard_set( GSList *items )
{
	GtkClipboard *clipboard;
	NactClipboardData *data;
	GSList *it;

	clipboard = get_clipboard();

	gtk_clipboard_clear( clipboard );

	data = g_new0( NactClipboardData, 1 );
	data->only_profiles = have_only_profiles( items );

	for( it = items ; it ; it = it->next ){
		add_item_to_clipboard( NA_OBJECT( it->data ), &data->items );
	}
	data->items = g_slist_reverse( data->items );

	gtk_clipboard_set_with_data( clipboard,
			clipboard_formats, G_N_ELEMENTS( clipboard_formats ),
			( GtkClipboardGetFunc ) get_from_clipboard_callback,
			( GtkClipboardClearFunc ) clear_clipboard_callback,
			data );

	g_object_set_data( G_OBJECT( clipboard ), CLIPBOARD_PROP_PRIMAY_USED, GINT_TO_POINTER( TRUE ));
}

static GtkClipboard *
get_clipboard( void )
{
	GdkDisplay *display;
	GtkClipboard *clipboard;

	display = gdk_display_get_default();
	clipboard = gtk_clipboard_get_for_display( display, GDK_SELECTION_PRIMARY );

	return( clipboard );
}

static gboolean
have_only_profiles( GSList *items )
{
	GSList *item;
	gboolean only_profiles = TRUE;

	for( item = items ; item ; item = item->next ){
		if( !NA_IS_OBJECT_PROFILE( item->data )){
			only_profiles = FALSE;
			break;
		}
	}

	return( only_profiles );
}

static void
add_item_to_clipboard0( NAObject *object, gboolean copy_data, gboolean only_profiles, GSList **items_list )
{
	NAObject *source;
	gint index;

	source = object;
	if( !only_profiles && NA_IS_OBJECT_PROFILE( object )){
		source = NA_OBJECT( na_object_profile_get_action( NA_OBJECT_PROFILE( object )));
	}

	index = g_slist_index( *items_list, ( gconstpointer ) source );
	if( index != -1 ){
		return;
	}

	*items_list = g_slist_prepend( *items_list, na_object_ref( source ));
}

static void
add_item_to_clipboard( NAObject *object, GSList **items_list )
{
	*items_list = g_slist_prepend( *items_list, na_object_ref( object ));
}

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
get_from_clipboard_callback( GtkClipboard *clipboard, GtkSelectionData *selection_data, guint info, NactClipboardData *data )
{
	static const gchar *thisfn = "nact_clipboard_get_from_clipboard_callback";

	g_debug( "%s: clipboard=%p, selection_data=%p, target=%s, info=%d, data=%p",
			thisfn, ( void * ) clipboard,
			( void * ) selection_data, gtk_atom_name( selection_data->target ), info, ( void * ) data );

	gtk_selection_data_set( selection_data, selection_data->target, 8, data, sizeof( NactClipboardData ));
}

static void
clear_clipboard_callback( GtkClipboard *clipboard, NactClipboardData *data )
{
	static const gchar *thisfn = "nact_clipboard_clear_clipboard_callback";

	g_debug( "%s: clipboard=%p, data=%p", thisfn, ( void * ) clipboard, ( void * ) data );

	g_slist_foreach( data->items, ( GFunc ) g_object_unref, NULL );
	g_slist_free( data->items );
	g_free( data );

	g_object_set_data( G_OBJECT( clipboard ), CLIPBOARD_PROP_PRIMAY_USED, GINT_TO_POINTER( FALSE ));
}
