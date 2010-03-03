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

#ifndef __NAUTILUS_ACTIONS_API_NA_ICONDITIONS_H__
#define __NAUTILUS_ACTIONS_API_NA_ICONDITIONS_H__

/**
 * SECTION: na_iconditions
 * @short_description: #NAIConditions interface definition.
 * @include: nautilus-actions/na-iconditions.h
 *
 * This interface is implemented by all #NAObject-derived objects
 * which must met some conditions in order to be displayed in the
 * Nautilus context menu.
 *
 * Implementors, typically actions, profiles and menus, host the required
 * data as #NADataBoxed in a dedicated NA_FACTORY_CONDITIONS_GROUP
 * data group.
 */

#include "na-object.h"

G_BEGIN_DECLS

#define NA_ICONDITIONS_TYPE							( na_iobject_conditions_get_type())
#define NA_ICONDITIONS( instance )					( G_TYPE_CHECK_INSTANCE_CAST( instance, NA_ICONDITIONS_TYPE, NAIConditions ))
#define NA_IS_ICONDITIONS( instance )				( G_TYPE_CHECK_INSTANCE_TYPE( instance, NA_ICONDITIONS_TYPE ))
#define NA_ICONDITIONS_GET_INTERFACE( instance )	( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NA_ICONDITIONS_TYPE, NAIConditionsInterface ))

typedef struct NAIConditions                 NAIConditions;

typedef struct NAIConditionsInterfacePrivate NAIConditionsInterfacePrivate;

typedef struct {
	GTypeInterface                 parent;
	NAIConditionsInterfacePrivate *private;
}
	NAIConditionsInterface;

GType na_iconditions_get_type( void );

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_API_NA_ICONDITIONS_H__ */
