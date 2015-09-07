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

#ifndef __CORE_FMA_EXPORT_FORMAT_H__
#define __CORE_FMA_EXPORT_FORMAT_H__

/* @title: FMAExportFormat
 * @short_description: The #FMAExportFormat Class Definition
 * @include: core/fma-export-format.h
 */

#include <api/fma-iexporter.h>

G_BEGIN_DECLS

#define FMA_TYPE_EXPORT_FORMAT                ( fma_export_format_get_type())
#define FMA_EXPORT_FORMAT( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_EXPORT_FORMAT, FMAExportFormat ))
#define FMA_EXPORT_FORMAT_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_EXPORT_FORMAT, FMAExportFormatClass ))
#define FMA_IS_EXPORT_FORMAT( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_EXPORT_FORMAT ))
#define FMA_IS_EXPORT_FORMAT_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_EXPORT_FORMAT ))
#define FMA_EXPORT_FORMAT_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_EXPORT_FORMAT, FMAExportFormatClass ))

typedef struct _FMAExportFormatPrivate        FMAExportFormatPrivate;

typedef struct {
	GObject                 parent;
	FMAExportFormatPrivate *private;
}
	FMAExportFormat;

typedef struct _FMAExportFormatClassPrivate   FMAExportFormatClassPrivate;

typedef struct {
	GObjectClass                 parent;
	FMAExportFormatClassPrivate *private;
}
	FMAExportFormatClass;

GType            fma_export_format_get_type    ( void );

FMAExportFormat *fma_export_format_new         ( const FMAIExporterFormatv2 *exporter_format );

FMAIExporter    *fma_export_format_get_provider( const FMAExportFormat *format );

G_END_DECLS

#endif /* __CORE_FMA_EXPORT_FORMAT_H__ */
