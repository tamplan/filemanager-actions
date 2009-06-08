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

#ifndef __NACT_OBJECT_H__
#define __NACT_OBJECT_H__

/*
 * NactObject class definition.
 *
 * This is the base class for NactAction and NactActionProfile.
 *
 * It takes care of common things such as ids, i/o, etc.
 * It uses the NactIIOProviderInterface for all storage subsystems
 * management.
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define NACT_OBJECT_TYPE				( nact_object_get_type())
#define NACT_OBJECT( object )			( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_OBJECT_TYPE, NactObject ))
#define NACT_OBJECT_CLASS( klass )		( G_TYPE_CHECK_CLASS_CAST( klass, NACT_OBJECT_TYPE, NactObjectClass ))
#define NACT_IS_OBJECT( object )		( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_OBJECT_TYPE ))
#define NACT_IS_OBJECT_CLASS( klass )	( G_TYPE_CHECK_CLASS_TYPE(( klass ), NACT_OBJECT_TYPE ))
#define NACT_OBJECT_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), NACT_OBJECT_TYPE, NactObjectClass ))

typedef struct NactObjectPrivate NactObjectPrivate;

typedef struct {
	GObject            parent;
	NactObjectPrivate *private;
}
	NactObject;

typedef struct NactObjectClassPrivate NactObjectClassPrivate;

typedef struct {
	GObjectClass            parent;
	NactObjectClassPrivate *private;

	/* virtual public functions */
	void    ( *dump )( const NactObject *object );
	void    ( *empty_property )( NactObject *object, const gchar *property );
}
	NactObjectClass;

GType    nact_object_get_type( void );

void     nact_object_dump( const NactObject *object );
void     nact_object_empty_property( NactObject *object, const gchar *property );

G_END_DECLS

#endif /* __NACT_OBJECT_H__ */
