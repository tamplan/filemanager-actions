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

#ifndef __NA_OBJECT_PROFILE_H__
#define __NA_OBJECT_PROFILE_H__

/**
 * SECTION: na_object_profile
 * @short_description: #NAObjectProfile class definition.
 * @include: common/na-obj-profile.h
 *
 * This is a companion class of NAObjectAction. It embeds the profile
 * definition of an action.
 *
 * As NAObjectAction itself, NAObjectProfile class is derived from
 * NAObject class, which takes care of IDuplicable interface management.
 */

#include "na-obj-action-class.h"
#include "na-obj-profile-class.h"

G_BEGIN_DECLS

/* internal identifier of profiles must begin with the following prefix
 * this let us identify a profile key versus an action key
 * corollarily, no action entry must begin with this same prefix
 */
#define OBJECT_PROFILE_PREFIX			"profile-"

NAObjectProfile *na_object_profile_new( void );

NAObjectAction  *na_object_profile_get_action( const NAObjectProfile *profile );
gchar           *na_object_profile_get_path( const NAObjectProfile *profile );
gchar           *na_object_profile_get_parameters( const NAObjectProfile *profile );
GSList          *na_object_profile_get_basenames( const NAObjectProfile *profile );
gboolean         na_object_profile_get_matchcase( const NAObjectProfile *profile );
GSList          *na_object_profile_get_mimetypes( const NAObjectProfile *profile );
gboolean         na_object_profile_get_is_file( const NAObjectProfile *profile );
gboolean         na_object_profile_get_is_dir( const NAObjectProfile *profile );
gboolean         na_object_profile_get_multiple( const NAObjectProfile *profile );
GSList          *na_object_profile_get_schemes( const NAObjectProfile *profile );

void             na_object_profile_set_action( NAObjectProfile *profile, const NAObjectAction *action );
void             na_object_profile_set_path( NAObjectProfile *profile, const gchar *path );
void             na_object_profile_set_parameters( NAObjectProfile *profile, const gchar *parameters );
void             na_object_profile_set_basenames( NAObjectProfile *profile, GSList *basenames );
void             na_object_profile_set_matchcase( NAObjectProfile *profile, gboolean matchcase );
void             na_object_profile_set_mimetypes( NAObjectProfile *profile, GSList *mimetypes );
void             na_object_profile_set_isfile( NAObjectProfile *profile, gboolean isfile );
void             na_object_profile_set_isdir( NAObjectProfile *profile, gboolean isdir );
void             na_object_profile_set_isfiledir( NAObjectProfile *profile, gboolean isfile, gboolean isdir );
void             na_object_profile_set_multiple( NAObjectProfile *profile, gboolean multiple );
void             na_object_profile_set_scheme( NAObjectProfile *profile, const gchar *scheme, gboolean selected );
void             na_object_profile_set_schemes( NAObjectProfile *profile, GSList *schemes );

gboolean         na_object_profile_is_candidate( const NAObjectProfile *profile, GList *files );
gchar           *na_object_profile_parse_parameters( const NAObjectProfile *profile, GList *files );

G_END_DECLS

#endif /* __NA_OBJECT_PROFILE_H__ */
