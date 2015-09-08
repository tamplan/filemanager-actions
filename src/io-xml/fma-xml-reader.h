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

#ifndef __IO_XML_FMA_XML_READER_H__
#define __IO_XML_FMA_XML_READER_H__

/**
 * SECTION: fma_xml_reader
 * @short_description: #FMAXMLReader class definition.
 * @include: fma-xml-reader.h
 *
 * This is the base class for importing items from XML files.
 *
 * If the imported file is not an XML one, with a known document root,
 * then we returned IMPORTER_CODE_NOT_WILLING_TO.
 * In all other cases, errors or inconsistancies are signaled, but
 * we do our best to actually import the file and produce a valuable
 * #FMAObjectItem-derived object.
 */

#include <api/fma-data-boxed.h>
#include <api/fma-iimporter.h>
#include <api/fma-ifactory-provider.h>

G_BEGIN_DECLS

#define FMA_XML_READER_TYPE                ( fma_xml_reader_get_type())
#define FMA_XML_READER( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_XML_READER_TYPE, FMAXMLReader ))
#define FMA_XML_READER_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_XML_READER_TYPE, FMAXMLReaderClass ))
#define FMA_IS_XML_READER( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_XML_READER_TYPE ))
#define FMA_IS_XML_READER_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_XML_READER_TYPE ))
#define FMA_XML_READER_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_XML_READER_TYPE, FMAXMLReaderClass ))

typedef struct _FMAXMLReaderPrivate        FMAXMLReaderPrivate;

typedef struct {
	/*< private >*/
	GObject              parent;
	FMAXMLReaderPrivate *private;
}
	FMAXMLReader;

typedef struct _FMAXMLReaderClassPrivate   FMAXMLReaderClassPrivate;

typedef struct {
	/*< private >*/
	GObjectClass              parent;
	FMAXMLReaderClassPrivate *private;
}
	FMAXMLReaderClass;

GType         fma_xml_reader_get_type       ( void );

guint         fma_xml_reader_import_from_uri( const FMAIImporter *instance, void *parms_ptr );

void          fma_xml_reader_read_start     ( const FMAIFactoryProvider *provider, void *reader_data, const FMAIFactoryObject *object, GSList **messages  );
FMADataBoxed *fma_xml_reader_read_data      ( const FMAIFactoryProvider *provider, void *reader_data, const FMAIFactoryObject *object, const FMADataDef *def, GSList **messages );
void          fma_xml_reader_read_done      ( const FMAIFactoryProvider *provider, void *reader_data, const FMAIFactoryObject *object, GSList **messages  );

G_END_DECLS

#endif /* __IO_XML_FMA_XML_READER_H__ */
