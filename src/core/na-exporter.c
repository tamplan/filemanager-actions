/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011, 2012 Pierre Wieser and others (see AUTHORS)
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

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <string.h>

#include "na-exporter.h"
#include "na-export-format.h"
#include "na-settings.h"

#define	Ask			"Ask"

typedef struct {
	const gchar *format;				/* export format saved in user's preferences */
	const gchar *label;					/* short label */
	const gchar *description;			/* full description */
	const gchar *image;					/* associated image */
}
	NAExporterFormatStr;

static NAExporterFormatStr st_format_ask = {

		Ask,
		N_( "_Ask me" ),
		N_( "You will be asked for the format to choose each time an item " \
			"is about to be exported." ),
		"export-format-ask.png"
};

static GList       *exporter_get_formats( const NAIExporter *exporter );
static void         exporter_free_formats( const NAIExporter *exporter, GList * str_list );
static gchar       *exporter_get_name( const NAIExporter *exporter );
static NAIExporter *find_exporter_for_format( const NAPivot *pivot, GQuark format );
static void         on_pixbuf_finalized( gpointer user_data, GObject *pixbuf );
static GQuark       id_from_string( const gchar *format_str );

/*
 * na_exporter_get_formats:
 * @pivot: the #NAPivot instance.
 *
 * Returns: a list of #NAExportFormat objects, each of them addressing an
 * available export format, i.e. a format provided by a module which
 * implement the #NAIExporter interface.
 *
 * The returned list should later be na_exporter_free_formats() by the caller.
 */
GList *
na_exporter_get_formats( const NAPivot *pivot )
{
	GList *iexporters, *imod;
	GList *formats;
	GList *str_list, *is;
	NAExportFormat *format;

	g_return_val_if_fail( NA_IS_PIVOT( pivot ), NULL );

	formats = NULL;
	iexporters = na_pivot_get_providers( pivot, NA_IEXPORTER_TYPE );

	for( imod = iexporters ; imod ; imod = imod->next ){
		str_list = exporter_get_formats( NA_IEXPORTER( imod->data ));

		for( is = str_list ; is ; is = is->next ){
			format = na_export_format_new(( NAIExporterFormatExt * ) is->data );
			formats = g_list_prepend( formats, format );
		}

		exporter_free_formats( NA_IEXPORTER( imod->data ), str_list );
	}

	na_pivot_free_providers( iexporters );

	return( formats );
}

/*
 * Returns a GList of NAIExporterFormatExt structures which describes
 * the export formats provided by the exporter
 * If the provider only implements the v1 interface, we dynamically
 * allocate a new structure and convert the v1 to the v2.
 */
static GList *
exporter_get_formats( const NAIExporter *exporter )
{
	GList *str_list;
	guint version;

	str_list = NULL;

	version = 1;
	if( NA_IEXPORTER_GET_INTERFACE( exporter )->get_version ){
		version = NA_IEXPORTER_GET_INTERFACE( exporter )->get_version( exporter );
	}

	if( NA_IEXPORTER_GET_INTERFACE( exporter )->get_formats ){
		if( version == 1 ){
#ifdef NA_ENABLE_DEPRECATED
			const NAIExporterFormat * strv1;
			strv1 = NA_IEXPORTER_GET_INTERFACE( exporter )->get_formats( exporter );
			while( strv1->format ){
				NAIExporterFormatExt *strv2 = g_new0( NAIExporterFormatExt, 1 );
				strv2->version = 1;
				strv2->provider = ( NAIExporter * ) exporter;
				strv2->format = strv1->format;
				strv2->label = strv1->label;
				strv2->description = strv1->description;
				strv2->pixbuf = NULL;
				str_list = g_list_prepend( str_list, strv2 );
				strv1++;
			}
#else
			;
#endif
		} else {
			str_list = NA_IEXPORTER_GET_INTERFACE( exporter )->get_formats( exporter );
		}
	}

	return( str_list );
}

/*
 * Free the list returned by exporter_get_formats() for this provider
 */
static void
exporter_free_formats( const NAIExporter *exporter, GList *str_list )
{
	guint version;

	version = 1;
	if( NA_IEXPORTER_GET_INTERFACE( exporter )->get_version ){
		version = NA_IEXPORTER_GET_INTERFACE( exporter )->get_version( exporter );
	}

	if( version == 1 ){
		g_list_foreach( str_list, ( GFunc ) g_free, NULL );
		g_list_free( str_list );

	} else {
		g_return_if_fail( NA_IEXPORTER_GET_INTERFACE( exporter )->free_formats );
		NA_IEXPORTER_GET_INTERFACE( exporter )->free_formats( exporter, str_list );
	}
}

/*
 * na_exporter_free_formats:
 * @formats: a list of available export formats, as returned by
 *  na_exporter_get_formats().
 *
 * Release the @formats #GList.
 */
void
na_exporter_free_formats( GList *formats )
{
	static const gchar *thisfn = "na_exporter_free_formats";

	g_debug( "%s: formats=%p (count=%d)", thisfn, ( void * ) formats, g_list_length( formats ));

	g_list_foreach( formats, ( GFunc ) g_object_unref, NULL );
	g_list_free( formats );
}

/*
 * na_exporter_get_ask_option:
 *
 * Returns the 'Ask me' option.
 *
 * Since: 3.2
 */
NAIOption *
na_exporter_get_ask_option( void )
{
	static const gchar *thisfn = "na_exporter_get_ask_option";
	NAIExporterFormatExt *str;
	gint width, height;
	gchar *fname;
	NAExportFormat *format;

	if( !gtk_icon_size_lookup( GTK_ICON_SIZE_DIALOG, &width, &height )){
		width = height = 48;
	}

	str = g_new0( NAIExporterFormatExt, 1 );
	str->version = 2;
	str->provider = NULL;
	str->format = g_strdup( st_format_ask.format );
	str->label = g_strdup( gettext( st_format_ask.label ));
	str->description = g_strdup( gettext( st_format_ask.description ));
	if( st_format_ask.image ){
		fname = g_strdup_printf( "%s/%s", PKGDATADIR, st_format_ask.image );
		str->pixbuf = gdk_pixbuf_new_from_file_at_size( fname, width, height, NULL );
		g_free( fname );
		if( str->pixbuf ){
			g_debug( "%s: allocating pixbuf at %p", thisfn, str->pixbuf );
			g_object_weak_ref( G_OBJECT( str->pixbuf ), ( GWeakNotify ) on_pixbuf_finalized, NULL );
		}
	}

	format = na_export_format_new( str );

	if( str->pixbuf ){
		g_object_unref( str->pixbuf );
	}
	g_free( str->description );
	g_free( str->label );
	g_free( str->format );
	g_free( str );

	return( NA_IOPTION( format ));
}

static void
on_pixbuf_finalized( gpointer user_data /* ==NULL */, GObject *pixbuf )
{
	g_debug( "na_exporter_on_pixbuf_finalized: pixbuf=%p", ( void * ) pixbuf );
}

/*
 * na_exporter_to_buffer:
 * @pivot: the #NAPivot pivot for the running application.
 * @item: a #NAObjectItem-derived object.
 * @format: the target format identifier.
 * @messages: a pointer to a #GSList list of strings; the provider
 *  may append messages to this list, but shouldn't reinitialize it.
 *
 * Exports the specified @item in the required @format.
 *
 * Returns: the output buffer, as a newly allocated string which should
 * be g_free() by the caller, or %NULL if an error has been detected.
 */
gchar *
na_exporter_to_buffer( const NAPivot *pivot,
		const NAObjectItem *item, GQuark format, GSList **messages )
{
	static const gchar *thisfn = "na_exporter_to_buffer";
	gchar *buffer;
	NAIExporterBufferParms parms;
	NAIExporter *exporter;
	gchar *name;
	gchar *msg;

	g_return_val_if_fail( NA_IS_PIVOT( pivot ), NULL );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), NULL );

	buffer = NULL;

	g_debug( "%s: pivot=%p, item=%p (%s), format=%u, messages=%p",
			thisfn,
			( void * ) pivot,
			( void * ) item, G_OBJECT_TYPE_NAME( item ),
			( guint ) format,
			( void * ) messages );

	exporter = find_exporter_for_format( pivot, format );
	g_debug( "%s: exporter=%p (%s)", thisfn, ( void * ) exporter, G_OBJECT_TYPE_NAME( exporter ));

	if( exporter ){
		parms.version = 1;
		parms.exported = ( NAObjectItem * ) item;
		parms.format = format;
		parms.buffer = NULL;
		parms.messages = messages ? *messages : NULL;

		if( NA_IEXPORTER_GET_INTERFACE( exporter )->to_buffer ){
			NA_IEXPORTER_GET_INTERFACE( exporter )->to_buffer( exporter, &parms );

			if( parms.buffer ){
				buffer = parms.buffer;
			}

		} else {
			name = exporter_get_name( exporter );
			msg = g_strdup_printf( _( "NAIExporter %s doesn't implement 'to_buffer' interface." ), name );
			*messages = g_slist_append( *messages, msg );
			g_free( name );
		}

	} else {
		msg = g_strdup_printf(
				_( "No NAIExporter implementation found for %s format." ), g_quark_to_string( format ));
		*messages = g_slist_append( *messages, msg );
	}

	return( buffer );
}

/*
 * na_exporter_to_file:
 * @pivot: the #NAPivot pivot for the running application.
 * @item: a #NAObjectItem-derived object.
 * @folder_uri: the URI of the target folder.
 * @format: the target format identifier.
 * @messages: a pointer to a #GSList list of strings; the provider
 *  may append messages to this list, but shouldn't reinitialize it.
 *
 * Exports the specified @item to the target @uri in the required @format.
 *
 * Returns: the URI of the exported file, as a newly allocated string which
 * should be g_free() by the caller, or %NULL if an error has been detected.
 */
gchar *
na_exporter_to_file( const NAPivot *pivot,
		const NAObjectItem *item, const gchar *folder_uri, GQuark format, GSList **messages )
{
	static const gchar *thisfn = "na_exporter_to_file";
	gchar *export_uri;
	NAIExporterFileParms parms;
	NAIExporter *exporter;
	gchar *msg;
	gchar *name;

	g_return_val_if_fail( NA_IS_PIVOT( pivot ), NULL );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), NULL );

	export_uri = NULL;

	g_debug( "%s: pivot=%p, item=%p (%s), folder_uri=%s, format=%u (%s), messages=%p",
			thisfn,
			( void * ) pivot,
			( void * ) item, G_OBJECT_TYPE_NAME( item ),
			folder_uri,
			( guint ) format, g_quark_to_string( format ),
			( void * ) messages );

	exporter = find_exporter_for_format( pivot, format );

	if( exporter ){
		parms.version = 1;
		parms.exported = ( NAObjectItem * ) item;
		parms.folder = ( gchar * ) folder_uri;
		parms.format = format;
		parms.basename = NULL;
		parms.messages = messages ? *messages : NULL;

		if( NA_IEXPORTER_GET_INTERFACE( exporter )->to_file ){
			NA_IEXPORTER_GET_INTERFACE( exporter )->to_file( exporter, &parms );

			if( parms.basename ){
				export_uri = g_strdup_printf( "%s%s%s", folder_uri, G_DIR_SEPARATOR_S, parms.basename );
			}

		} else {
			name = exporter_get_name( exporter );
			msg = g_strdup_printf( _( "NAIExporter %s doesn't implement 'to_file' interface." ), name );
			*messages = g_slist_append( *messages, msg );
			g_free( name );
		}

	} else {
		msg = g_strdup_printf(
				_( "No NAIExporter implementation found for %s format." ), g_quark_to_string( format ));
		*messages = g_slist_append( *messages, msg );
	}

	return( export_uri );
}

static gchar *
exporter_get_name( const NAIExporter *exporter )
{
	gchar *name;

	name = NULL;

	if( NA_IEXPORTER_GET_INTERFACE( exporter )->get_name ){
		name = NA_IEXPORTER_GET_INTERFACE( exporter )->get_name( exporter );
	}

	return( name );
}

static NAIExporter *
find_exporter_for_format( const NAPivot *pivot, GQuark format )
{
	NAIExporter *exporter;
	GList *formats, *ifmt;

	exporter = NULL;
	formats = na_exporter_get_formats( pivot );

	for( ifmt = formats ; ifmt && !exporter ; ifmt = ifmt->next ){

		if( na_export_format_get_quark( NA_EXPORT_FORMAT( ifmt->data )) == format ){
			exporter = na_export_format_get_provider( NA_EXPORT_FORMAT( ifmt->data ));
		}
	}

	na_exporter_free_formats( formats );

	return( exporter );
}

/*
 * na_exporter_get_export_format:
 * @pref: the name of the preference to be read.
 * @mandatory: if not %NULL, a pointer to a boolean which will receive the
 *  mandatory property.
 *
 * Returns: the export format currently set in user's preference..
 *
 * The returned integer is either:
 * - the corresponding enum value in the case of "NoImport" or "Ask";
 * - the #GQuark of the string for other export formats.
 */
GQuark
na_exporter_get_export_format( const gchar *pref, gboolean *mandatory )
{
	gchar *format_str;
	GQuark format_id;

	g_return_val_if_fail( pref && strlen( pref ), ( GQuark ) EXPORTER_FORMAT_NO_EXPORT );

	format_str = na_settings_get_string( pref, NULL, mandatory );
	g_return_val_if_fail( format_str && strlen( format_str ), ( GQuark ) EXPORTER_FORMAT_NO_EXPORT );

	format_id = id_from_string( format_str );
	g_free( format_str );

	return( format_id );
}

/*
 * na_exporter_id_from_string:
 * @format_str: the string which defines the export format in user's preferences.
 *
 * Returns: the integer which identifies the export format.
 *
 * The returned integer is either:
 * - the corresponding enum value in the case of "NoImport" or "Ask";
 * - the #GQuark of the string for other export formats.
 */
static GQuark
id_from_string( const gchar *format_str )
{
	if( !strcmp( format_str, Ask )){
		return(( GQuark ) EXPORTER_FORMAT_ASK );
	}

	return( g_quark_from_string( format_str ));
}
