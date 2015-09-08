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

#ifndef __IO_XML_FMA_XML_WRITER_H__
#define __IO_XML_FMA_XML_WRITER_H__

/**
 * SECTION: fma_xml_writer
 * @short_description: #FMAXMLWriter class definition.
 * @include: io-xml/fma-xml-writer.h
 *
 * This class exports FileManager-Actions actions and menus as XML files.
 */

#include <api/fma-data-boxed.h>
#include <api/fma-iexporter.h>
#include <api/fma-ifactory-provider.h>

G_BEGIN_DECLS

#define FMA_XML_WRITER_TYPE                ( fma_xml_writer_get_type())
#define FMA_XML_WRITER( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_XML_WRITER_TYPE, FMAXMLWriter ))
#define FMA_XML_WRITER_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_XML_WRITER_TYPE, FMAXMLWriterClass ))
#define FMA_IS_XML_WRITER( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_XML_WRITER_TYPE ))
#define FMA_IS_XML_WRITER_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_XML_WRITER_TYPE ))
#define FMA_XML_WRITER_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_XML_WRITER_TYPE, FMAXMLWriterClass ))

typedef struct _FMAXMLWriterPrivate        FMAXMLWriterPrivate;

typedef struct {
	/*< private >*/
	GObject              parent;
	FMAXMLWriterPrivate *private;
}
	FMAXMLWriter;

typedef struct _FMAXMLWriterClassPrivate   FMAXMLWriterClassPrivate;

typedef struct {
	/*< private >*/
	GObjectClass              parent;
	FMAXMLWriterClassPrivate *private;
}
	FMAXMLWriterClass;

GType  fma_xml_writer_get_type        ( void );

guint  fma_xml_writer_export_to_buffer( const FMAIExporter *instance, FMAIExporterBufferParmsv2 *parms );
guint  fma_xml_writer_export_to_file  ( const FMAIExporter *instance, FMAIExporterFileParmsv2 *parms );

guint  fma_xml_writer_write_start     ( const FMAIFactoryProvider *writer, void *writer_data, const FMAIFactoryObject *object, GSList **messages  );
guint  fma_xml_writer_write_data      ( const FMAIFactoryProvider *writer, void *writer_data, const FMAIFactoryObject *object, const FMADataBoxed *boxed, GSList **messages );
guint  fma_xml_writer_write_done      ( const FMAIFactoryProvider *writer, void *writer_data, const FMAIFactoryObject *object, GSList **messages  );

G_END_DECLS

#endif /* __IO_XML_FMA_XML_WRITER_H__ */
