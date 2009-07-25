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

#ifndef __NA_ACTION_PROFILE_H__
#define __NA_ACTION_PROFILE_H__

/**
 * SECTION: na_action_profile
 * @short_description: #NAActionProfile class definition.
 * @include: common/na-action-profile.h
 *
 * This is a companion class of NAAction. It embeds the profile
 * definition of an action.
 *
 * As NAAction itself, NAActionProfile class is derived from
 * NAObject class, which takes care of IDuplicable interface management.
 */

#include "na-action-class.h"
#include "na-action-profile-class.h"

G_BEGIN_DECLS

/* internal identifier of profiles must begin with the following prefix
 * this let us identify a profile key versus an action key
 * corollarily, no action entry must begin with this same prefix
 */
#define ACTION_PROFILE_PREFIX			"profile-"

NAActionProfile *na_action_profile_new( void );

NAAction        *na_action_profile_get_action( const NAActionProfile *profile );
gchar           *na_action_profile_get_name( const NAActionProfile *profile );
gchar           *na_action_profile_get_label( const NAActionProfile *profile );
gchar           *na_action_profile_get_path( const NAActionProfile *profile );
gchar           *na_action_profile_get_parameters( const NAActionProfile *profile );
GSList          *na_action_profile_get_basenames( const NAActionProfile *profile );
gboolean         na_action_profile_get_matchcase( const NAActionProfile *profile );
GSList          *na_action_profile_get_mimetypes( const NAActionProfile *profile );
gboolean         na_action_profile_get_is_file( const NAActionProfile *profile );
gboolean         na_action_profile_get_is_dir( const NAActionProfile *profile );
gboolean         na_action_profile_get_multiple( const NAActionProfile *profile );
GSList          *na_action_profile_get_schemes( const NAActionProfile *profile );

void             na_action_profile_set_action( NAActionProfile *profile, const NAAction *action );
void             na_action_profile_set_name( NAActionProfile *profile, const gchar *name );
void             na_action_profile_set_label( NAActionProfile *profile, const gchar *label );
void             na_action_profile_set_path( NAActionProfile *profile, const gchar *path );
void             na_action_profile_set_parameters( NAActionProfile *profile, const gchar *parameters );
void             na_action_profile_set_basenames( NAActionProfile *profile, GSList *basenames );
void             na_action_profile_set_matchcase( NAActionProfile *profile, gboolean matchcase );
void             na_action_profile_set_mimetypes( NAActionProfile *profile, GSList *mimetypes );
void             na_action_profile_set_isfile( NAActionProfile *profile, gboolean isfile );
void             na_action_profile_set_isdir( NAActionProfile *profile, gboolean isdir );
void             na_action_profile_set_isfiledir( NAActionProfile *profile, gboolean isfile, gboolean isdir );
void             na_action_profile_set_multiple( NAActionProfile *profile, gboolean multiple );
void             na_action_profile_set_scheme( NAActionProfile *profile, const gchar *scheme, gboolean selected );
void             na_action_profile_set_schemes( NAActionProfile *profile, GSList *schemes );

gboolean         na_action_profile_is_candidate( const NAActionProfile *profile, GList *files );
gchar           *na_action_profile_parse_parameters( const NAActionProfile *profile, GList *files );

G_END_DECLS

#endif /* __NA_ACTION_PROFILE_H__ */
