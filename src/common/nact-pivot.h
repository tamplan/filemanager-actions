/*
 * Nautilus Pivots
 * A Nautilus extension which offers configurable context menu pivots.
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

#ifndef __NACT_PIVOT_H__
#define __NACT_PIVOT_H__

/*
 * NactPivot class definition.
 *
 * A consuming program should allocate one new NactPivot object in its
 * startup phase. The class takes care of declaring the I/O interface,
 * while registering the known providers. The object will then load
 * itself the existing list of actions.
 */

#include <glib-object.h>

G_BEGIN_DECLS

/*
 * We would want have a sort of GConfValue, but which is not named with
 * GConf, in order to propose this same structure to other storage
 * subsystems.
 * We so define this, with only the data types we need.
 */
enum {
	NACT_PIVOT_STR = 1,
	NACT_PIVOT_BOOL,
	NACT_PIVOT_STRLIST
};

typedef struct {
	guint    type;
	gpointer data;
}
	NactPivotValue;

#define NACT_PIVOT_TYPE					( nact_pivot_get_type())
#define NACT_PIVOT( object )			( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_PIVOT_TYPE, NactPivot ))
#define NACT_PIVOT_CLASS( klass )		( G_TYPE_CHECK_CLASS_CAST( klass, NACT_PIVOT_TYPE, NactPivotClass ))
#define NACT_IS_PIVOT( object )			( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_PIVOT_TYPE ))
#define NACT_IS_PIVOT_CLASS( klass )	( G_TYPE_CHECK_CLASS_TYPE(( klass ), NACT_PIVOT_TYPE ))
#define NACT_PIVOT_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), NACT_PIVOT_TYPE, NactPivotClass ))

typedef struct NactPivotPrivate NactPivotPrivate;

typedef struct {
	GObject           parent;
	NactPivotPrivate *private;
}
	NactPivot;

typedef struct NactPivotClassPrivate NactPivotClassPrivate;

typedef struct {
	GObjectClass           parent;
	NactPivotClassPrivate *private;
}
	NactPivotClass;

GType      nact_pivot_get_type( void );

NactPivot *nact_pivot_new( void );

GSList    *nact_pivot_get_providers( const NactPivot *pivot, GType type );

void       nact_pivot_on_action_changed( NactPivot *pivot, const gchar *uuid, const gchar *parm, NactPivotValue *value );

void       nact_pivot_free_pivot_value( NactPivotValue *value );

G_END_DECLS

#endif /* __NACT_PIVOT_H__ */
