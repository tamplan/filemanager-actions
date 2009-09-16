/*
 * Nautilus ObjectActions
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

#ifndef __NA_OBJECT_ACTION_H__
#define __NA_OBJECT_ACTION_H__

/**
 * SECTION: na_object_action
 * @short_description: #NAObjectAction class definition.
 * @include: common/na-obj-action.h
 *
 * This is the class which maintains data and properties of an Nautilus
 * action.
 *
 * Note about the UUID:
 *
 * The uuid is only required when writing the action to GConf in order
 * to ensure unicity of subdirectories.
 *
 * UUID is transfered through import/export operations.
 *
 * Note that a user may import an action, translate it and then
 * reexport it : we so may have two different actions with the same
 * uuid. The user has so to modify the UUID before import.
 */

#include <glib/gi18n.h>

#include "na-obj-action-class.h"
#include "na-obj-profile-class.h"

G_BEGIN_DECLS

/* i18n: default label for a newly created action */
#define NA_OBJECT_ACTION_DEFAULT_LABEL		_( "New Nautilus action" )

/* i18n: default label for a newly created profile */
#define NA_OBJECT_PROFILE_DEFAULT_LABEL		_( "New profile" )

NAObjectAction *na_object_action_new( void );
NAObjectAction *na_object_action_new_with_profile( void );

gchar          *na_object_action_get_version( const NAObjectAction *action );
gboolean        na_object_action_is_readonly( const NAObjectAction *action );

void            na_object_action_set_version( NAObjectAction *action, const gchar *version );
void            na_object_action_set_readonly( NAObjectAction *action, gboolean readonly );

gchar          *na_object_action_get_new_profile_name( const NAObjectAction *action );
void            na_object_action_attach_profile( NAObjectAction *action, NAObjectProfile *profile );

G_END_DECLS

#endif /* __NA_OBJECT_ACTION_H__ */
