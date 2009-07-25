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

/**
 * SECTION: na_action
 * @short_description: #NAAction class definition.
 * @include: common/na-action.h
 *
 * This is the class which maintains data and properties of an Nautilus
 * action.
 */

#include "na-action-class.h"
#include "na-action-profile-class.h"
#include "na-iio-provider.h"

G_BEGIN_DECLS

/* TODO: move this declaration elsewhere */
/* export formats
 * used to be only GConf schemas ('gconfschemafile' XML document)
 */
enum {
	EXPORT_FORMAT_GCONFSCHEMAFILE = 1
};

NAAction        *na_action_new( void );
NAAction        *na_action_new_with_profile( void );

gchar           *na_action_get_uuid( const NAAction *action );
gchar           *na_action_get_label( const NAAction *action );
gchar           *na_action_get_version( const NAAction *action );
gchar           *na_action_get_tooltip( const NAAction *action );
gchar           *na_action_get_icon( const NAAction *action );
gchar           *na_action_get_verified_icon_name( const NAAction *action );
gboolean         na_action_is_readonly( const NAAction *action );
NAIIOProvider   *na_action_get_provider( const NAAction *action );

void             na_action_set_new_uuid( NAAction *action );
void             na_action_set_uuid( NAAction *action, const gchar *uuid );
void             na_action_set_label( NAAction *action, const gchar *label );
void             na_action_set_version( NAAction *action, const gchar *version );
void             na_action_set_tooltip( NAAction *action, const gchar *tooltip );
void             na_action_set_icon( NAAction *action, const gchar *icon_name );
void             na_action_set_readonly( NAAction *action, gboolean readonly );
void             na_action_set_provider( NAAction *action, const NAIIOProvider *provider );

gchar           *na_action_get_new_profile_name( const NAAction *action );
NAActionProfile *na_action_get_profile( const NAAction *action, const gchar *name );
GSList          *na_action_get_profiles( const NAAction *action );
void             na_action_attach_profile( NAAction *action, NAActionProfile *profile );
void             na_action_remove_profile( NAAction *action, NAActionProfile *profile );
void             na_action_set_profiles( NAAction *action, GSList *list );
void             na_action_free_profiles( GSList *list );
guint            na_action_get_profiles_count( const NAAction *action );

G_END_DECLS

#endif /* __NA_ACTION_H__ */
