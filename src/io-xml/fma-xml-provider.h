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

#ifndef __IO_XML_FMA_XML_PROVIDER_H__
#define __IO_XML_FMA_XML_PROVIDER_H__

/**
 * SECTION: fma_xml_provider
 * @short_description: #FMAXMLProvider class definition.
 * @include: fma-xml-provider.h
 *
 * This class manages I/O in XML formats.
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define FMA_TYPE_XML_PROVIDER                ( fma_xml_provider_get_type())
#define FMA_XML_PROVIDER( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_XML_PROVIDER, FMAXMLProvider ))
#define FMA_XML_PROVIDER_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_XML_PROVIDER, FMAXMLProviderClass ))
#define FMA_IS_XML_PROVIDER( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_XML_PROVIDER ))
#define FMA_IS_XML_PROVIDER_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_XML_PROVIDER ))
#define FMA_XML_PROVIDER_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_XML_PROVIDER, FMAXMLProviderClass ))

typedef struct _FMAXMLProviderPrivate        FMAXMLProviderPrivate;

typedef struct {
	/*< private >*/
	GObject                parent;
	FMAXMLProviderPrivate *private;
}
	FMAXMLProvider;

typedef struct _FMAXMLProviderClassPrivate   FMAXMLProviderClassPrivate;

typedef struct {
	/*< private >*/
	GObjectClass                parent;
	FMAXMLProviderClassPrivate *private;
}
	FMAXMLProviderClass;

GType fma_xml_provider_get_type     ( void );
void  fma_xml_provider_register_type( GTypeModule *module );

G_END_DECLS

#endif /* __IO_XML_FMA_XML_PROVIDER_H__ */
