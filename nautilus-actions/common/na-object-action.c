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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <runtime/na-object-api.h>
#include <runtime/na-object-action-priv.h>

#include "na-object-api.h"

/**
 * na_object_action_new_with_profile:
 *
 * Allocates a new #NAObjectAction object along with a default profile.
 *
 * Returns: the newly allocated #NAObjectAction action.
 */
NAObjectAction *
na_object_action_new_with_profile( void )
{
	NAObjectAction *action;
	NAObjectProfile *profile;

	action = na_object_action_new();

	profile = na_object_profile_new();

	/* i18n: name of the default profile when creating an action */
	na_object_set_label( profile, _( "Default profile" ));
	na_object_action_attach_profile( action, profile );

	return( action );
}

/**
 * na_object_action_is_readonly:
 * @action: the #NAObjectAction object to be requested.
 *
 * Is the specified action only readable ?
 * Or, in other words, may this action be edited and then saved to the
 * original I/O storage subsystem ?
 *
 * Returns: %TRUE if the action is read-only, %FALSE else.
 */
gboolean
na_object_action_is_readonly( const NAObjectAction *action )
{
	gboolean readonly = FALSE;

	g_return_val_if_fail( NA_IS_OBJECT_ACTION( action ), FALSE );

	if( !action->private->dispose_has_run ){
		g_object_get( G_OBJECT( action ), NAACTION_PROP_READONLY, &readonly, NULL );
	}

	return( readonly );
}

/**
 * na_object_action_reset_last_allocated:
 * @action: the #NAObjectAction object.
 *
 * Resets the last_allocated counter for computing new profile names.
 *
 * This should be called after having successfully saved the action.
 */
void
na_object_action_reset_last_allocated( NAObjectAction *action )
{
	g_return_if_fail( NA_IS_OBJECT_ACTION( action ));

	if( !action->private->dispose_has_run ){

		action->private->last_allocated = 0;
	}
}
