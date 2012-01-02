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

#include "core/na-export-format.h"

#include "nact-export-format.h"

typedef struct {
	gchar *format;
	gchar *label;
	gchar *description;
	gchar *image;
}
	NactExportFormatStr;

static NactExportFormatStr st_format_ask = {
	IPREFS_EXPORT_FORMAT_ASK_ID,
		N_( "_Ask me" ),
		N_( "You will be asked for the format to choose each time an item " \
			"is about to be exported." ),
		"export-format-ask.png"
};

static void on_pixbuf_finalized( gpointer user_data, GObject *pixbuf );

/*
 * nact_export_format_get_ask_option:
 *
 * Returns the 'Ask me' option.
 *
 * Since: 3.2
 */
NAIOption *
nact_export_format_get_ask_option( void )
{
	static const gchar *thisfn = "nact_export_format_get_ask_option";
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
	g_debug( "nact_export_format_on_pixbuf_finalized: pixbuf=%p", ( void * ) pixbuf );
}
