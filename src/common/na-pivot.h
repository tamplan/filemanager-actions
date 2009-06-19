/*
 * Nautilus Actions
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

#ifndef __NA_PIVOT_H__
#define __NA_PIVOT_H__

/*
 * NAPivot class definition.
 *
 * A consuming program should allocate one new NAPivot object in its
 * startup phase. The class takes care of declaring the I/O interface,
 * while registering the known providers. The object will then load
 * itself the existing list of actions.
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define NA_PIVOT_TYPE					( na_pivot_get_type())
#define NA_PIVOT( object )				( G_TYPE_CHECK_INSTANCE_CAST( object, NA_PIVOT_TYPE, NAPivot ))
#define NA_PIVOT_CLASS( klass )			( G_TYPE_CHECK_CLASS_CAST( klass, NA_PIVOT_TYPE, NAPivotClass ))
#define NA_IS_PIVOT( object )			( G_TYPE_CHECK_INSTANCE_TYPE( object, NA_PIVOT_TYPE ))
#define NA_IS_PIVOT_CLASS( klass )		( G_TYPE_CHECK_CLASS_TYPE(( klass ), NA_PIVOT_TYPE ))
#define NA_PIVOT_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), NA_PIVOT_TYPE, NAPivotClass ))

typedef struct NAPivotPrivate NAPivotPrivate;

typedef struct {
	GObject         parent;
	NAPivotPrivate *private;
}
	NAPivot;

typedef struct NAPivotClassPrivate NAPivotClassPrivate;

typedef struct {
	GObjectClass         parent;
	NAPivotClassPrivate *private;
}
	NAPivotClass;

GType    na_pivot_get_type( void );

NAPivot *na_pivot_new( const GObject *notified );

GSList  *na_pivot_get_providers( const NAPivot *pivot, GType type );

GSList  *na_pivot_get_label_sorted_actions( const NAPivot *pivot );
GSList  *na_pivot_get_actions( const NAPivot *pivot );

void     na_pivot_free_actions( GSList *actions );

GObject *na_pivot_get_action( NAPivot *pivot, const gchar *uuid );

gboolean na_pivot_write_action( NAPivot *pivot, const GObject *action, gchar **message );

/* data passed from the storage subsystem when an action is changed
 */
enum {
	NA_PIVOT_STR = 1,
	NA_PIVOT_BOOL,
	NA_PIVOT_STRLIST
};

typedef struct {
	gchar   *uuid;
	gchar   *profile;
	gchar   *parm;
	guint    type;
	gpointer data;
}
	NAPivotNotify;

void       na_pivot_free_notify( NAPivotNotify *data );

G_END_DECLS

#endif /* __NA_PIVOT_H__ */
