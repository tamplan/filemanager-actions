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

#ifndef __NAXML_READER_H__
#define __NAXML_READER_H__

/**
 * SECTION: naxml_reader
 * @short_description: #NAXMLReader class definition.
 * @include: naxml-reader.h
 *
 * This is the base class for importing items from XML files.
 */

#include <api/na-iimporter.h>
#include <api/na-object-item.h>

G_BEGIN_DECLS

#define NAXML_READER_TYPE					( naxml_reader_get_type())
#define NAXML_READER( object )				( G_TYPE_CHECK_INSTANCE_CAST( object, NAXML_READER_TYPE, NAXMLReader ))
#define NAXML_READER_CLASS( klass )			( G_TYPE_CHECK_CLASS_CAST( klass, NAXML_READER_TYPE, NAXMLReaderClass ))
#define NAXML_IS_READER( object )			( G_TYPE_CHECK_INSTANCE_TYPE( object, NAXML_READER_TYPE ))
#define NAXML_IS_READER_CLASS( klass )		( G_TYPE_CHECK_CLASS_TYPE(( klass ), NAXML_READER_TYPE ))
#define NAXML_READER_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), NAXML_READER_TYPE, NAXMLReaderClass ))

typedef struct NAXMLReaderPrivate      NAXMLReaderPrivate;

typedef struct {
	GObject             parent;
	NAXMLReaderPrivate *private;
}
	NAXMLReader;

typedef struct NAXMLReaderClassPrivate NAXMLReaderClassPrivate;

typedef struct {
	GObjectClass             parent;
	NAXMLReaderClassPrivate *private;
}
	NAXMLReaderClass;

GType         naxml_reader_get_type( void );

NAObjectItem *naxml_reader_import_uri( const NAIImporter *instance, const gchar *uri, guint mode, ImporterCheckFn fn, void *fn_data, GSList **messages );

G_END_DECLS

#endif /* __NAXML_READER_H__ */
