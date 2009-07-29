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

#ifndef __NA_GCONF_H__
#define __NA_GCONF_H__

/**
 * SECTION: na_gconf
 * @short_description: #NAGConf class definition.
 * @include: common/na-gconf.h
 *
 * This class manages the GConf I/O storage subsystem.
 * It should only be used through the NAIIOProvider interface.
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define NA_GCONF_TYPE					( na_gconf_get_type())
#define NA_GCONF( object )				( G_TYPE_CHECK_INSTANCE_CAST( object, NA_GCONF_TYPE, NAGConf ))
#define NA_GCONF_CLASS( klass )			( G_TYPE_CHECK_CLASS_CAST( klass, NA_GCONF_TYPE, NAGConfClass ))
#define NA_IS_GCONF( object )			( G_TYPE_CHECK_INSTANCE_TYPE( object, NA_GCONF_TYPE ))
#define NA_IS_GCONF_CLASS( klass )		( G_TYPE_CHECK_CLASS_TYPE(( klass ), NA_GCONF_TYPE ))
#define NA_GCONF_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), NA_GCONF_TYPE, NAGConfClass ))

typedef struct NAGConfPrivate NAGConfPrivate;

typedef struct {
	GObject         parent;
	NAGConfPrivate *private;
}
	NAGConf;

typedef struct NAGConfClassPrivate NAGConfClassPrivate;

typedef struct {
	GObjectClass         parent;
	NAGConfClassPrivate *private;
}
	NAGConfClass;

GType    na_gconf_get_type( void );

NAGConf *na_gconf_new( const GObject *notified );

G_END_DECLS

#endif /* __NA_GCONF_H__ */
