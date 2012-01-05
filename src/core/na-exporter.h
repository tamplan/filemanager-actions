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

#ifndef __CORE_NA_EXPORTER_H__
#define __CORE_NA_EXPORTER_H__

/* @title: NAIExporter
 * @short_description: The #NAIExporter Internal Functions
 * @include: core/na-exporter.h
 */

#include <api/na-object-api.h>

#include "na-ioption.h"
#include "na-pivot.h"

G_BEGIN_DECLS

/*
 * NAExporterExportFormat:
 * @EXPORTER_FORMAT_NO_EXPORT:
 * @EXPORTER_FORMAT_ASK:
 *
 * This enum defines some special export formats, which are typically used
 * in switch statements. Standard export formats, as provided by I/O providers,
 * are just #GQuark of their format identifier string.
 *
 * When the user chooses to not export an item, this value is not written in
 * user's preferences.
 */
typedef enum {
	EXPORTER_FORMAT_NO_EXPORT = 1,
	EXPORTER_FORMAT_ASK,
}
	NAExporterExportFormat;

GList     *na_exporter_get_formats      ( const NAPivot *pivot );
void       na_exporter_free_formats     ( GList *formats );

NAIOption *na_exporter_get_ask_option   ( void );

gchar     *na_exporter_to_buffer        ( const NAPivot *pivot,
                                          const NAObjectItem *item,
                                          GQuark format,
                                          GSList **messages );

gchar     *na_exporter_to_file          ( const NAPivot *pivot,
                                          const NAObjectItem *item,
                                          const gchar *folder_uri,
                                          GQuark format,
                                          GSList **messages );

GQuark     na_exporter_get_export_format( const gchar *pref, gboolean *mandatory );

G_END_DECLS

#endif /* __CORE_NA_EXPORTER_H__ */
