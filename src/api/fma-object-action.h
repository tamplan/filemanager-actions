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

#ifndef __FILE_MANAGER_ACTIONS_API_OBJECT_ACTION_H__
#define __FILE_MANAGER_ACTIONS_API_OBJECT_ACTION_H__

/**
 * SECTION: object-action
 * @title: FMAObjectAction
 * @short_description: The Action Class Definition
 * @include: file-manager-actions/fma-object-action.h
 *
 * This is the class which maintains data and properties of a &prodname;
 * action.
 *
 * <note>
 *   <formalpara>
 *     <title>Edition status</title>
 *     <para>
 *       As a particular rule for a #FMAObjectItem -derived class,
 *       a #FMAObjectAction is considered modified as soon as any of
 *       its profiles has been modified itself
 *       (because they are saved as a whole).
 *     </para>
 *   </formalpara>
 * </note>
 */

#include "fma-object-item.h"
#include "fma-object-profile.h"

G_BEGIN_DECLS

#define FMA_TYPE_OBJECT_ACTION                ( fma_object_action_get_type())
#define FMA_OBJECT_ACTION( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_OBJECT_ACTION, FMAObjectAction ))
#define FMA_OBJECT_ACTION_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_OBJECT_ACTION, FMAObjectActionClass ))
#define FMA_IS_OBJECT_ACTION( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_OBJECT_ACTION ))
#define FMA_IS_OBJECT_ACTION_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_OBJECT_ACTION ))
#define FMA_OBJECT_ACTION_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_OBJECT_ACTION, FMAObjectActionClass ))

typedef struct _FMAObjectActionPrivate        FMAObjectActionPrivate;

typedef struct {
	/*< private >*/
	FMAObjectItem           parent;
	FMAObjectActionPrivate *private;
}
	FMAObjectAction;

typedef struct _FMAObjectActionClassPrivate   FMAObjectActionClassPrivate;

typedef struct {
	/*< private >*/
	FMAObjectItemClass           parent;
	FMAObjectActionClassPrivate *private;
}
	FMAObjectActionClass;

GType fma_object_action_get_type                       ( void );

FMAObjectAction *fma_object_action_new                 ( void );
FMAObjectAction *fma_object_action_new_with_profile    ( void );
FMAObjectAction *fma_object_action_new_with_defaults   ( void );

gchar           *fma_object_action_get_new_profile_name( const FMAObjectAction *action );

void             fma_object_action_attach_profile      ( FMAObjectAction *action, FMAObjectProfile *profile );
void             fma_object_action_set_last_version    ( FMAObjectAction *action );

G_END_DECLS

#endif /* __FILE_MANAGER_ACTIONS_API_OBJECT_ACTION_H__ */
