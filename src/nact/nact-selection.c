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

#include <common/na-action.h>
#include <common/na-action-profile.h>
#include <common/na-xml-names.h>
#include <common/na-xml-writer.h>

#include "nact-selection.h"

static void   export_action( const gchar *uri, const NAAction *action, GSList **exported );
static gchar *get_action_xml_buffer( const NAAction *action, GSList **exported );

/**
 *
 */
char *
nact_selection_get_data_for_intern_use( GSList *selected_items )
{
	GString *data;
	GSList *item;

	data = g_string_new( "" );

	for( item = selected_items ; item ; item = item->next ){
		gchar *chunk = na_object_get_clipboard_id( NA_OBJECT( item->data ));
		if( chunk && strlen( chunk )){
			data = g_string_append( data, chunk );
			data = g_string_append( data, "\n" );
		}
		g_free( chunk );
	}

	return( g_string_free( data, FALSE ));
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
nact_selection_get_data_for_extern_use( GSList *selected_items )
{
	GSList *item;
	GSList *exported = NULL;
	GString *data;
	gchar *chunk;

	data = g_string_new( "" );

	for( item = selected_items ; item ; item = item->next ){
		NAObject *item_object = NA_OBJECT( item->data );
		chunk = NULL;

		if( NA_IS_ACTION( item_object )){
			chunk = get_action_xml_buffer( NA_ACTION( item_object ), &exported );

		} else if( NA_IS_ACTION_PROFILE( item_object )){
			NAAction *action = na_action_profile_get_action( NA_ACTION_PROFILE( item_object ));
			chunk = get_action_xml_buffer( action, &exported );
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
nact_selection_export_items( const gchar *uri, GSList *items )
{
	GSList *item;
	GSList *exported = NULL;

	for( item = items ; item ; item = item->next ){
		NAObject *item_object = NA_OBJECT( item->data );

		if( NA_IS_ACTION( item_object )){
			export_action( uri, NA_ACTION( item_object ), &exported );

		} else if( NA_IS_ACTION_PROFILE( item_object )){
			NAAction *action = na_action_profile_get_action( NA_ACTION_PROFILE( item_object ));
			export_action( uri, action, &exported );
		}
	}

	g_slist_free( exported );
}

static void
export_action( const gchar *uri, const NAAction *action, GSList **exported )
{
	gint index;
	gchar *fname, *buffer;

	index = g_slist_index( *exported, ( gconstpointer ) action );
	if( index != -1 ){
		return;
	}

	fname = na_xml_writer_get_output_fname( action, uri, FORMAT_GCONFENTRY );
	buffer = na_xml_writer_get_xml_buffer( action, FORMAT_GCONFENTRY );

	na_xml_writer_output_xml( buffer, fname );

	g_free( buffer );
	g_free( fname );

	*exported = g_slist_prepend( *exported, ( gpointer ) action );
}

static gchar *
get_action_xml_buffer( const NAAction *action, GSList **exported )
{
	gint index;
	gchar *buffer;

	index = g_slist_index( *exported, ( gconstpointer ) action );
	if( index != -1 ){
		return( NULL );
	}

	buffer = na_xml_writer_get_xml_buffer( action, FORMAT_GCONFENTRY );

	*exported = g_slist_prepend( *exported, ( gpointer ) action );

	return( buffer );
}
