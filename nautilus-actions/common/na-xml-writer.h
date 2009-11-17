/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009 Pierre Wieser and others (see AUTHORS)
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

#ifndef __NA_COMMON_XML_WRITER_H__
#define __NA_COMMON_XML_WRITER_H__

/**
 * SECTION: na_xml_writer
 * @short_description: #NAXMLWriter class definition.
 * @include: common/na-xml-writer.h
 *
 * This class exports Nautilus Actions as XML files.
 *
 * This class is embedded in libnact library as it is used by
 * nautilus-actions-new utility.
 */

#include <runtime/na-object-action-class.h>

G_BEGIN_DECLS

#define NA_XML_WRITER_TYPE					( na_xml_writer_get_type())
#define NA_XML_WRITER( object )				( G_TYPE_CHECK_INSTANCE_CAST( object, NA_XML_WRITER_TYPE, NAXMLWriter ))
#define NA_XML_WRITER_CLASS( klass )		( G_TYPE_CHECK_CLASS_CAST( klass, NA_XML_WRITER_TYPE, NAXMLWriterClass ))
#define NA_IS_XML_WRITER( object )			( G_TYPE_CHECK_INSTANCE_TYPE( object, NA_XML_WRITER_TYPE ))
#define NA_IS_XML_WRITER_CLASS( klass )		( G_TYPE_CHECK_CLASS_TYPE(( klass ), NA_XML_WRITER_TYPE ))
#define NA_XML_WRITER_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), NA_XML_WRITER_TYPE, NAXMLWriterClass ))

typedef struct NAXMLWriterPrivate NAXMLWriterPrivate;

typedef struct {
	GObject             parent;
	NAXMLWriterPrivate *private;
}
	NAXMLWriter;

typedef struct NAXMLWriterClassPrivate NAXMLWriterClassPrivate;

typedef struct {
	GObjectClass             parent;
	NAXMLWriterClassPrivate *private;
}
	NAXMLWriterClass;

GType  na_xml_writer_get_type( void );

gchar *na_xml_writer_export( const NAObjectAction *action, const gchar *folder, gint format, GSList **msg );
gchar *na_xml_writer_get_output_fname( const NAObjectAction *action, const gchar *folder, gint format );
gchar *na_xml_writer_get_xml_buffer( const NAObjectAction *action, gint format );
void   na_xml_writer_output_xml( const gchar *xml, const gchar *filename, GSList **msg );

G_END_DECLS

#endif /* __NA_COMMON_XML_WRITER_H__ */
