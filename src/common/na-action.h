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

#ifndef __NA_ACTION_H__
#define __NA_ACTION_H__

/*
 * NAAction class definition.
 *
 * This is the class which maintains an action.
 *
 * NAAction class is derived from NAObject.
 */

#include "na-object.h"
#include "na-pivot.h"

G_BEGIN_DECLS

#define NA_ACTION_TYPE					( na_action_get_type())
#define NA_ACTION( object )				( G_TYPE_CHECK_INSTANCE_CAST( object, NA_ACTION_TYPE, NAAction ))
#define NA_ACTION_CLASS( klass )		( G_TYPE_CHECK_CLASS_CAST( klass, NA_ACTION_TYPE, NAActionClass ))
#define NA_IS_ACTION( object )			( G_TYPE_CHECK_INSTANCE_TYPE( object, NA_ACTION_TYPE ))
#define NA_IS_ACTION_CLASS( klass )		( G_TYPE_CHECK_CLASS_TYPE(( klass ), NA_ACTION_TYPE ))
#define NA_ACTION_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), NA_ACTION_TYPE, NAActionClass ))

typedef struct NAActionPrivate NAActionPrivate;

typedef struct {
	NAObject         parent;
	NAActionPrivate *private;
}
	NAAction;

typedef struct NAActionClassPrivate NAActionClassPrivate;

typedef struct {
	NAObjectClass         parent;
	NAActionClassPrivate *private;
}
	NAActionClass;

/* instance properties
 */
#define PROP_ACTION_UUID_STR			"action-uuid"
#define PROP_ACTION_VERSION_STR			"action-version"
#define PROP_ACTION_LABEL_STR			"action-label"
#define PROP_ACTION_TOOLTIP_STR			"action-tooltip"
#define PROP_ACTION_ICON_STR			"action-icon"
#define PROP_ACTION_READONLY_STR		"action-read-only"
#define PROP_ACTION_PROVIDER_STR		"action-provider"

GType     na_action_get_type( void );

NAAction *na_action_new( const gchar *uuid );
NAAction *na_action_new_with_profile( void );
NAAction *na_action_duplicate( const NAAction *action );

gchar    *na_action_get_uuid( const NAAction *action );
gchar    *na_action_get_version( const NAAction *action );
gchar    *na_action_get_label( const NAAction *action );
gchar    *na_action_get_tooltip( const NAAction *action );
gchar    *na_action_get_icon( const NAAction *action );
gchar    *na_action_get_verified_icon_name( const NAAction *action );
gboolean  na_action_is_readonly( const NAAction *action );
gpointer  na_action_get_provider( const NAAction *action );

void      na_action_set_new_uuid( NAAction *action );
void      na_action_set_label( NAAction *action, const gchar *label );
void      na_action_set_tooltip( NAAction *action, const gchar *tooltip );
void      na_action_set_icon( NAAction *action, const gchar *icon_name );

gboolean  na_action_are_equal( NAAction *first, NAAction *second );

GSList   *na_action_get_profiles( const NAAction *action );
void      na_action_set_profiles( NAAction *action, GSList *list );
void      na_action_free_profiles( GSList *list );

guint     na_action_get_profiles_count( const NAAction *action );

G_END_DECLS

#endif /* __NA_ACTION_H__ */
