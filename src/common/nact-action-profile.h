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

#ifndef __NACT_ACTION_PROFILE_H__
#define __NACT_ACTION_PROFILE_H__

/*
 * NactActionProfile class definition.
 *
 * This is a companion class of NactAction. It embeds the profile
 * definition of an action.
 *
 * As NactAction itself, NactActionProfile class is derived from
 * NactObject which takes care of i/o.
 */

#include "nact-object.h"

G_BEGIN_DECLS

#define NACT_ACTION_PROFILE_TYPE				( nact_action_profile_get_type())
#define NACT_ACTION_PROFILE( object )			( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_ACTION_PROFILE_TYPE, NactActionProfile ))
#define NACT_ACTION_PROFILE_CLASS( klass )		( G_TYPE_CHECK_CLASS_CAST( klass, NACT_ACTION_PROFILE_TYPE, NactActionProfileClass ))
#define NACT_IS_ACTION_PROFILE( object )		( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_ACTION_PROFILE_TYPE ))
#define NACT_IS_ACTION_PROFILE_CLASS( klass )	( G_TYPE_CHECK_CLASS_TYPE(( klass ), NACT_ACTION_PROFILE_TYPE ))
#define NACT_ACTION_PROFILE_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), NACT_ACTION_PROFILE_TYPE, NactActionProfileClass ))

typedef struct NactActionProfilePrivate NactActionProfilePrivate;

typedef struct {
	NactObject                parent;
	NactActionProfilePrivate *private;
}
	NactActionProfile;

typedef struct NactActionProfileClassPrivate NactActionProfileClassPrivate;

typedef struct {
	NactObjectClass                parent;
	NactActionProfileClassPrivate *private;
}
	NactActionProfileClass;

GType              nact_action_profile_get_type( void );

NactActionProfile *nact_action_profile_new( const NactObject *action, const gchar *name );

void               nact_action_profile_load( NactObject *profile );

gboolean           nact_action_profile_is_empty( const NactActionProfile *profile );

NactObject        *nact_action_profile_get_action( const NactActionProfile *profile );
gchar             *nact_action_profile_get_name( const NactActionProfile *profile );
gchar             *nact_action_profile_get_path( const NactActionProfile *profile );
gchar             *nact_action_profile_get_parameters( const NactActionProfile *profile );

gboolean           nact_action_profile_validate( const NactActionProfile *profile, GList *files );

G_END_DECLS

#endif /* __NACT_ACTION_PROFILE_H__ */
