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

#ifndef __NAXML_PROVIDER_H__
#define __NAXML_PROVIDER_H__

/**
 * SECTION: naxml_provider
 * @short_description: #NaxmlProvider class definition.
 * @include: naxml-provider.h
 *
 * This class manages I/O in XML formats.
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define NAXML_PROVIDER_TYPE						( naxml_provider_get_type())
#define NAXML_PROVIDER( object )				( G_TYPE_CHECK_INSTANCE_CAST( object, NAXML_PROVIDER_TYPE, NaxmlProvider ))
#define NAXML_PROVIDER_CLASS( klass )			( G_TYPE_CHECK_CLASS_CAST( klass, NAXML_PROVIDER_TYPE, NaxmlProviderClass ))
#define NAGP_IS_GCONF_PROVIDER( object )		( G_TYPE_CHECK_INSTANCE_TYPE( object, NAXML_PROVIDER_TYPE ))
#define NAGP_IS_GCONF_PROVIDER_CLASS( klass )	( G_TYPE_CHECK_CLASS_TYPE(( klass ), NAXML_PROVIDER_TYPE ))
#define NAXML_PROVIDER_GET_CLASS( object )		( G_TYPE_INSTANCE_GET_CLASS(( object ), NAXML_PROVIDER_TYPE, NaxmlProviderClass ))

typedef struct NaxmlProviderPrivate      NaxmlProviderPrivate;

typedef struct {
	GObject               parent;
	NaxmlProviderPrivate *private;
}
	NaxmlProvider;

typedef struct NaxmlProviderClassPrivate NaxmlProviderClassPrivate;

typedef struct {
	GObjectClass               parent;
	NaxmlProviderClassPrivate *private;
}
	NaxmlProviderClass;

GType naxml_provider_get_type     ( void );
void  naxml_provider_register_type( GTypeModule *module );

G_END_DECLS

#endif /* __NAXML_PROVIDER_H__ */
