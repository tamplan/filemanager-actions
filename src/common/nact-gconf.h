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

#ifndef __NACT_GCONF_H__
#define __NACT_GCONF_H__

/*
 * NactGConf class definition.
 *
 * Implements the NactIIOProvider (I/O storage subsystem) interface.
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define NACT_GCONF_TYPE					( nact_gconf_get_type())
#define NACT_GCONF( object )			( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_GCONF_TYPE, NactGConf ))
#define NACT_GCONF_CLASS( klass )		( G_TYPE_CHECK_CLASS_CAST( klass, NACT_GCONF_TYPE, NactGConfClass ))
#define NACT_IS_GCONF( object )			( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_GCONF_TYPE ))
#define NACT_IS_GCONF_CLASS( klass )	( G_TYPE_CHECK_CLASS_TYPE(( klass ), NACT_GCONF_TYPE ))
#define NACT_GCONF_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), NACT_GCONF_TYPE, NactGConfClass ))

typedef struct NactGConfPrivate NactGConfPrivate;

typedef struct {
	GObject           parent;
	NactGConfPrivate *private;
}
	NactGConf;

typedef struct NactGConfClassPrivate NactGConfClassPrivate;

typedef struct {
	GObjectClass           parent;
	NactGConfClassPrivate *private;
}
	NactGConfClass;

GType      nact_gconf_get_type( void );

NactGConf *nact_gconf_new( const GObject *notified );

G_END_DECLS

#endif /* __NACT_GCONF_H__ */
