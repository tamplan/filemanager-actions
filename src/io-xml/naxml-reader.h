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

#ifndef __NAXML_READER_H__
#define __NAXML_READER_H__

/**
 * SECTION: naxml_reader
 * @short_description: #NAXMLReader class definition.
 * @include: naxml-reader.h
 *
 * This is the base class for importing items from XML files.
 *
 * If the imported file is not an XML one, with a known document root,
 * then we returned IMPORTER_CODE_NOT_WILLING_TO.
 * In all other cases, errors or inconsistancies are signaled, but
 * we do our best to actually import the file and produce a valuable
 * #NAObjectItem-derived object.
 */

#include <api/fma-data-boxed.h>
#include <api/fma-iimporter.h>
#include <api/fma-ifactory-provider.h>

G_BEGIN_DECLS

#define NAXML_READER_TYPE                ( naxml_reader_get_type())
#define NAXML_READER( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, NAXML_READER_TYPE, NAXMLReader ))
#define NAXML_READER_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, NAXML_READER_TYPE, NAXMLReaderClass ))
#define NAXML_IS_READER( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, NAXML_READER_TYPE ))
#define NAXML_IS_READER_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), NAXML_READER_TYPE ))
#define NAXML_READER_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), NAXML_READER_TYPE, NAXMLReaderClass ))

typedef struct _NAXMLReaderPrivate       NAXMLReaderPrivate;

typedef struct {
	/*< private >*/
	GObject             parent;
	NAXMLReaderPrivate *private;
}
	NAXMLReader;

typedef struct _NAXMLReaderClassPrivate  NAXMLReaderClassPrivate;

typedef struct {
	/*< private >*/
	GObjectClass             parent;
	NAXMLReaderClassPrivate *private;
}
	NAXMLReaderClass;

GType        naxml_reader_get_type( void );

guint        naxml_reader_import_from_uri( const FMAIImporter *instance, void *parms_ptr );

void         naxml_reader_read_start( const FMAIFactoryProvider *provider, void *reader_data, const FMAIFactoryObject *object, GSList **messages  );
FMADataBoxed *naxml_reader_read_data ( const FMAIFactoryProvider *provider, void *reader_data, const FMAIFactoryObject *object, const FMADataDef *def, GSList **messages );
void         naxml_reader_read_done ( const FMAIFactoryProvider *provider, void *reader_data, const FMAIFactoryObject *object, GSList **messages  );

G_END_DECLS

#endif /* __NAXML_READER_H__ */
