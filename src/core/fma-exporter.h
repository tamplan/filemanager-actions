/*
 * FileManager-Actions
 * A file-manager extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
 *
 * FileManager-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * FileManager-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FileManager-Actions; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Frederic Ruaudel <grumz@grumz.net>
 *   Rodrigo Moya <rodrigo@gnome-db.org>
 *   Pierre Wieser <pwieser@trychlos.org>
 *   ... and many others (see AUTHORS)
 */

#ifndef __CORE_FMA_EXPORTER_H__
#define __CORE_FMA_EXPORTER_H__

/* @title: FMAIExporter
 * @short_description: The #FMAIExporter Internal Functions
 * @include: core/fma-exporter.h
 */

#include <api/fma-iexporter.h>
#include <api/fma-object-api.h>

#include "fma-ioption.h"
#include "fma-pivot.h"

G_BEGIN_DECLS

#define EXPORTER_FORMAT_ASK				"Ask"
#define EXPORTER_FORMAT_NOEXPORT		"NoExport"

GList        *fma_exporter_get_formats    ( const FMAPivot *pivot );
void          fma_exporter_free_formats   ( GList *formats );
FMAIOption   *fma_exporter_get_ask_option ( void );

gchar        *fma_exporter_to_buffer      ( const FMAPivot *pivot,
                                            const FMAObjectItem *item,
                                            const gchar *format,
                                            GSList **messages );

gchar        *fma_exporter_to_file        ( const FMAPivot *pivot,
                                            const FMAObjectItem *item,
                                            const gchar *folder_uri,
                                            const gchar *format,
                                            GSList **messages );

FMAIExporter *fma_exporter_find_for_format( const FMAPivot *pivot,
		                                    const gchar *format );

G_END_DECLS

#endif /* __CORE_FMA_EXPORTER_H__ */
