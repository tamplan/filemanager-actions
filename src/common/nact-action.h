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

#ifndef __NACT_ACTION_H__
#define __NACT_ACTION_H__

/*
 * NactAction class definition.
 *
 * This is the class which maintains an action.
 *
 * NactAction class is derived from NactObject.
 */

#include "nact-object.h"

G_BEGIN_DECLS

#define NACT_ACTION_TYPE				( nact_action_get_type())
#define NACT_ACTION( object )			( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_ACTION_TYPE, NactAction ))
#define NACT_ACTION_CLASS( klass )		( G_TYPE_CHECK_CLASS_CAST( klass, NACT_ACTION_TYPE, NactActionClass ))
#define NACT_IS_ACTION( object )		( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_ACTION_TYPE ))
#define NACT_IS_ACTION_CLASS( klass )	( G_TYPE_CHECK_CLASS_TYPE(( klass ), NACT_ACTION_TYPE ))
#define NACT_ACTION_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), NACT_ACTION_TYPE, NactActionClass ))

typedef struct NactActionPrivate NactActionPrivate;

typedef struct {
	NactObject         parent;
	NactActionPrivate *private;
}
	NactAction;

typedef struct NactActionClassPrivate NactActionClassPrivate;

typedef struct {
	NactObjectClass         parent;
	NactActionClassPrivate *private;
}
	NactActionClass;

GType       nact_action_get_type( void );

NactAction *nact_action_new( gpointer provider, gpointer data );

void        nact_action_load( NactAction *action );

gchar      *nact_action_get_uuid( const NactAction *action );
gchar      *nact_action_get_label( const NactAction *action );
gchar      *nact_action_get_tooltip( const NactAction *action );
gchar      *nact_action_get_verified_icon_name( const NactAction *action );

GSList     *nact_action_get_profiles( const NactAction *action );
void        nact_action_set_profiles( NactAction *action, GSList *list );

guint       nact_action_get_profiles_count( const NactAction *action );
GSList     *nact_action_get_profile_ids( const NactAction *action );
void        nact_action_free_profile_ids( GSList *list );

NactObject *nact_action_get_profile( const NactAction *action, const gchar *name );

G_END_DECLS

#endif /* __NACT_ACTION_H__ */
