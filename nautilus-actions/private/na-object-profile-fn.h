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

#ifndef __NAUTILUS_ACTIONS_NA_PRIVATE_OBJECT_PROFILE_FN_H__
#define __NAUTILUS_ACTIONS_NA_PRIVATE_OBJECT_PROFILE_FN_H__

/**
 * SECTION: na_object_profile
 * @short_description: #NAObjectProfile public function declarations.
 * @include: nautilus-actions/private/na-object-profile.h
 */

#include <glib/gi18n.h>

#include "na-object-action-class.h"
#include "na-object-profile-class.h"

G_BEGIN_DECLS

/* internal identifier of profiles must begin with the following prefix
 * this let us identify a profile key versus an action key
 * corollarily, no action entry must begin with this same prefix
 */
#define OBJECT_PROFILE_PREFIX				"profile-"

/* i18n: default label for a newly created profile */
#define NA_OBJECT_PROFILE_DEFAULT_LABEL		_( "New profile" )

NAObjectProfile *na_object_profile_new( void );

gchar           *na_object_profile_get_path( const NAObjectProfile *profile );
gchar           *na_object_profile_get_parameters( const NAObjectProfile *profile );
GSList          *na_object_profile_get_basenames( const NAObjectProfile *profile );
gboolean         na_object_profile_get_matchcase( const NAObjectProfile *profile );
GSList          *na_object_profile_get_mimetypes( const NAObjectProfile *profile );
gboolean         na_object_profile_get_is_file( const NAObjectProfile *profile );
gboolean         na_object_profile_get_is_dir( const NAObjectProfile *profile );
gboolean         na_object_profile_get_multiple( const NAObjectProfile *profile );
GSList          *na_object_profile_get_schemes( const NAObjectProfile *profile );
GSList          *na_object_profile_get_folders( const NAObjectProfile *profile );

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
void             na_object_profile_set_folders( NAObjectProfile *profile, GSList *folders );

void             na_object_profile_replace_folder_uri( NAObjectProfile *profile, const gchar *old, const gchar *new );

gboolean         na_object_profile_is_candidate( const NAObjectProfile *profile, gint target, GList *files );
gboolean         na_object_profile_is_candidate_for_tracked( const NAObjectProfile *profile, GList *tracked );
gchar           *na_object_profile_parse_parameters( const NAObjectProfile *profile, gint target, GList *files );
gchar           *na_object_profile_parse_parameters_for_tracked( const NAObjectProfile *profile, GList *tracked );

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_NA_PRIVATE_OBJECT_PROFILE_FN_H__ */
