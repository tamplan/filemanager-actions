/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010 Pierre Wieser and others (see AUTHORS)
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

#include <api/na-iexporter.h>

#include "na-exporter.h"
#include "na-export-format.h"

extern gboolean iexporter_initialized;
extern gboolean iexporter_finalized;

static const NAExporterStr *exporter_get_formats( const NAIExporter *exporter );

/**
 * na_exporter_get_formats:
 * @pivot: the #NAPivot instance.
 *
 * Returns: a list of #NAExportFormat objects, each of them addressing an
 * available export format, i.e. a format provided by a module which
 * implement the #NAIExporter interface.
 */
GList *
na_exporter_get_formats( const NAPivot *pivot )
{
	GList *iexporters, *imod;
	GList *formats;
	const NAExporterStr *str;
	NAExportFormat *format;

	formats = NULL;

	if( iexporter_initialized && !iexporter_finalized ){

		iexporters = na_pivot_get_providers( pivot, NA_IEXPORTER_TYPE );
		for( imod = iexporters ; imod ; imod = imod->next ){

			str = exporter_get_formats( NA_IEXPORTER( imod->data ));
			while( str->format ){

				format = na_export_format_new( str, NA_IEXPORTER( imod->data ));
				formats = g_list_prepend( formats, format );
			}
		}

		na_pivot_free_providers( iexporters );
	}

	return( formats );
}

static const NAExporterStr *
exporter_get_formats( const NAIExporter *exporter )
{
	const NAExporterStr *str;

	str = NULL;

	if( NA_IEXPORTER_GET_INTERFACE( exporter )->get_formats ){
		str = NA_IEXPORTER_GET_INTERFACE( exporter )->get_formats( exporter );
	}

	return( str );
}

/**
 * na_exporter_free_formats:
 * @formats: a list of available export formats, as returned by
 *  #na_exporter_get_formats().
 *
 * Release the @formats #GList.
 */
void
na_exporter_free_formats( GList *formats )
{
	g_list_foreach( formats, ( GFunc ) g_object_unref, NULL );
	g_list_free( formats );
}

/**
 * na_exporter_to_file:
 * @item: a #NAObjectItem-derived object.
 * @uri: the target URI.
 * @format: the #GQuark target format.
 * @messages: a pointer to a #GSList list of strings; the provider
 *  may append messages to this list, but shouldn't reinitialize it.
 *
 * Exports the specified @item to the target @uri in the required @format.
 *
 * Returns: the URI of the exportered file, as a newly allocated string which
 * should be g_free() by the caller, or %NULL if an error has been detected.
 */
gchar *
na_exporter_to_file( const NAObjectItem *item, const gchar *uri, GQuark format, GSList **messages )
{
	gchar *fname;

	fname = NULL;

	if( iexporter_initialized && !iexporter_finalized ){
	}

	return( fname );
}

/**
 * na_exporter_to_buffer:
 * @item: a #NAObjectItem-derived object.
 * @format: the #GQuark target format.
 * @messages: a pointer to a #GSList list of strings; the provider
 *  may append messages to this list, but shouldn't reinitialize it.
 *
 * Exports the specified @item in the required @format.
 *
 * Returns: the output buffer, as a newly allocated string which should
 * be g_free() by the caller, or %NULL if an error has been detected.
 */
gchar *
na_exporter_to_buffer( const NAObjectItem *item, GQuark format, GSList **messages )
{
	gchar *buffer;

	buffer = NULL;

	if( iexporter_initialized && !iexporter_finalized ){
	}

	return( buffer );
}
