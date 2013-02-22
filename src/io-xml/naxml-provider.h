/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2013 Pierre Wieser and others (see AUTHORS)
 *
 * Nautilus-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Nautilus-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nautilus-Actions; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Frederic Ruaudel <grumz@grumz.net>
 *   Rodrigo Moya <rodrigo@gnome-db.org>
 *   Pierre Wieser <pwieser@trychlos.org>
 *   ... and many others (see AUTHORS)
 */

#ifndef __NAXML_PROVIDER_H__
#define __NAXML_PROVIDER_H__

/**
 * SECTION: naxml_provider
 * @short_description: #NAXMLProvider class definition.
 * @include: naxml-provider.h
 *
 * This class manages I/O in XML formats.
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define NAXML_TYPE_PROVIDER                ( naxml_provider_get_type())
#define NAXML_PROVIDER( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, NAXML_TYPE_PROVIDER, NAXMLProvider ))
#define NAXML_PROVIDER_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, NAXML_TYPE_PROVIDER, NAXMLProviderClass ))
#define NA_IS_XML_PROVIDER( object )       ( G_TYPE_CHECK_INSTANCE_TYPE( object, NAXML_TYPE_PROVIDER ))
#define NA_IS_XML_PROVIDER_CLASS( klass )  ( G_TYPE_CHECK_CLASS_TYPE(( klass ), NAXML_TYPE_PROVIDER ))
#define NAXML_PROVIDER_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), NAXML_TYPE_PROVIDER, NAXMLProviderClass ))

typedef struct _NAXMLProviderPrivate       NAXMLProviderPrivate;

typedef struct {
	/*< private >*/
	GObject               parent;
	NAXMLProviderPrivate *private;
}
	NAXMLProvider;

typedef struct _NAXMLProviderClassPrivate  NAXMLProviderClassPrivate;

typedef struct {
	/*< private >*/
	GObjectClass               parent;
	NAXMLProviderClassPrivate *private;
}
	NAXMLProviderClass;

GType naxml_provider_get_type     ( void );
void  naxml_provider_register_type( GTypeModule *module );

G_END_DECLS

#endif /* __NAXML_PROVIDER_H__ */
