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
#include <libintl.h>

#include "naxml-formats.h"

typedef struct {
	gchar *format;
	gchar *label;
	gchar *description;
	gchar *image;
}
	NaxmlExportFormat;

static NaxmlExportFormat naxml_formats[] = {

	/* GCONF_SCHEMA_V1: a schema with owner, short and long descriptions;
	 * each action has its own schema addressed by the id
	 * (historical format up to v1.10.x serie)
	 */
	{ NAXML_FORMAT_GCONF_SCHEMA_V1,
			N_( "Export as a _full GConf schema file" ),
			N_( "This used to be the historical export format.\n" \
				"The exported schema file may later be imported via :\n" \
				"- Import assistant of the Nautilus-Actions Configuration Tool,\n" \
				"- drag-n-drop into the Nautilus-Actions Configuration Tool,\n" \
				"- or via the gconftool-2 --import-schema-file command-line tool." ),
			"export-schemas-v1.png" },

	/* GCONF_SCHEMA_V2: the lightest schema still compatible with gconftool-2 --install-schema-file
	 * (no owner, no short nor long descriptions) - introduced in v 1.11
	 */
	{ NAXML_FORMAT_GCONF_SCHEMA_V2,
			N_( "Export as a _light GConf schema (v2) file" ),
			N_( "This format has been introduced in v 1.11 serie.\n" \
				"This is the lightest schema still compatible with GConf command-line tools, " \
				"while keeping backward compatibility with the Nautilus-Actions Configuration " \
				"Tool oldest versions.\n"
				"The exported schema file may later be imported via :\n" \
				"- Import assistant of the Nautilus-Actions Configuration Tool,\n" \
				"- drag-n-drop into the Nautilus-Actions Configuration Tool,\n" \
				"- or via the gconftool-2 --import-schema-file command-line tool." ),
			"export-schemas-v2.png" },

	/* GCONF_ENTRY: not a schema, but a dump of the GConf entry
	 * introduced in v 1.11
	 */
	{ NAXML_FORMAT_GCONF_ENTRY,
			N_( "Export as a GConf _dump file" ),
			N_( "This format has been introduced in v 1.11 serie.\n" \
				"Tough not backward compatible with Nautilus-Actions " \
				"Configuration Tool versions previous to 1.11, " \
				"it may still be imported via standard GConf command-line tools.\n" \
				"The exported dump file may later be imported via :\n" \
				"- Import assistant of the Nautilus-Actions Configuration Tool (1.11 and above),\n" \
				"- drag-n-drop into the Nautilus-Actions Configuration Tool (1.11 and above),\n" \
				"- or via the gconftool-2 --load command-line tool." ),
			"export-dump.png" },

	{ NULL }
};

#if 0
static void on_pixbuf_finalized( const NAIExporter* exporter, GObject *pixbuf );
#endif

/**
 * naxml_formats_get_formats:
 * @exporter: this #NAIExporter provider.
 *
 * Returns: a #GList of the #NAIExporterFormatExt supported export formats.
 *
 * This list should be naxml_formats_free_formats() by the caller.
 *
 * Since: 3.2
 */
GList *
naxml_formats_get_formats( const NAIExporter* exporter )
{
#if 0
	static const gchar *thisfn = "naxml_formats_get_formats";
#endif
	GList *str_list;
	NAIExporterFormatExt *str;
	guint i;
	gint width, height;
	gchar *fname;

	str_list = NULL;

	if( !gtk_icon_size_lookup( GTK_ICON_SIZE_DIALOG, &width, &height )){
		width = height = 48;
	}

	for( i = 0 ; naxml_formats[i].format ; ++i ){
		str = g_new0( NAIExporterFormatExt, 1 );
		str->version = 2;
		str->provider = NA_IEXPORTER( exporter );
		str->format = g_strdup( naxml_formats[i].format );
		str->label = g_strdup( gettext( naxml_formats[i].label ));
		str->description = g_strdup( gettext( naxml_formats[i].description ));
		if( naxml_formats[i].image ){
			fname = g_strdup_printf( "%s/%s", PKGDATADIR, naxml_formats[i].image );
			str->pixbuf = gdk_pixbuf_new_from_file_at_size( fname, width, height, NULL );
			g_free( fname );
#if 0
			/* do not set weak reference on a graphical object provided by a plugin
			 * if the windows does not have its own builder, it may happens that the
			 * graphical object be finalized when destroying toplevels at common
			 * builder finalization time, and so after the plugins have been shutdown
			 */
			if( str->pixbuf ){
				g_debug( "%s: allocating pixbuf at %p", thisfn, str->pixbuf );
				g_object_weak_ref( G_OBJECT( str->pixbuf ), ( GWeakNotify ) on_pixbuf_finalized, ( gpointer ) exporter );
			}
#endif
		}
		str_list = g_list_prepend( str_list, str );
	}

	return( str_list );
}

#if 0
static void
on_pixbuf_finalized( const NAIExporter* exporter, GObject *pixbuf )
{
	g_debug( "naxml_formats_on_pixbuf_finalized: exporter=%p, pixbuf=%p", ( void * ) exporter, ( void * ) pixbuf );
}
#endif

/**
 * naxml_formats_free_formats:
 * @formats: a #GList to be freed.
 *
 * Releases the list of managed formats.
 *
 * Since: 3.2
 */
void
naxml_formats_free_formats( GList *formats )
{
	GList *is;
	NAIExporterFormatExt *str;

	for( is = formats ; is ; is = is->next ){
		str = ( NAIExporterFormatExt * ) is->data;
		g_free( str->format );
		g_free( str->label );
		g_free( str->description );
		if( str->pixbuf ){
			g_object_unref( str->pixbuf );
		}
		g_free( str );
	}

	g_list_free( formats );
}
