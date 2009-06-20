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

/*
 * NactActionProfile class definition.
 *
 * This is a companion class of NactAction. It embeds the profile
 * definition of an action.
 *
 * As NactAction itself, NactActionProfile class is derived from
 * NAObject which takes care of i/o.
 */

#include "na-object.h"

G_BEGIN_DECLS

#define NA_ACTION_PROFILE_TYPE					( na_action_profile_get_type())
#define NA_ACTION_PROFILE( object )				( G_TYPE_CHECK_INSTANCE_CAST( object, NA_ACTION_PROFILE_TYPE, NAActionProfile ))
#define NA_ACTION_PROFILE_CLASS( klass )		( G_TYPE_CHECK_CLASS_CAST( klass, NA_ACTION_PROFILE_TYPE, NAActionProfileClass ))
#define NA_IS_ACTION_PROFILE( object )			( G_TYPE_CHECK_INSTANCE_TYPE( object, NA_ACTION_PROFILE_TYPE ))
#define NA_IS_ACTION_PROFILE_CLASS( klass )		( G_TYPE_CHECK_CLASS_TYPE(( klass ), NA_ACTION_PROFILE_TYPE ))
#define NA_ACTION_PROFILE_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), NA_ACTION_PROFILE_TYPE, NAActionProfileClass ))

typedef struct NAActionProfilePrivate NAActionProfilePrivate;

typedef struct {
	NAObject                parent;
	NAActionProfilePrivate *private;
}
	NAActionProfile;

typedef struct NAActionProfileClassPrivate NAActionProfileClassPrivate;

typedef struct {
	NAObjectClass                parent;
	NAActionProfileClassPrivate *private;
}
	NAActionProfileClass;

/* instance properties
 * please note that property names must have the same spelling as the
 * NactIIOProvider parameters
 */
#define PROP_PROFILE_ACTION_STR					"action"
#define PROP_PROFILE_NAME_STR					"name"
#define PROP_PROFILE_LABEL_STR					"desc-name"
#define PROP_PROFILE_PATH_STR					"path"
#define PROP_PROFILE_PARAMETERS_STR				"parameters"
#define PROP_PROFILE_ACCEPT_MULTIPLE_STR		"accept-multiple-files"
#define PROP_PROFILE_BASENAMES_STR				"basenames"
#define PROP_PROFILE_ISDIR_STR					"isdir"
#define PROP_PROFILE_ISFILE_STR					"isfile"
#define PROP_PROFILE_MATCHCASE_STR				"matchcase"
#define PROP_PROFILE_MIMETYPES_STR				"mimetypes"
#define PROP_PROFILE_SCHEMES_STR				"schemes"

GType            na_action_profile_get_type( void );

NAActionProfile *na_action_profile_new( const NAObject *action, const gchar *name );
NAActionProfile *na_action_profile_copy( const NAActionProfile *profile );
void             na_action_profile_free( NAActionProfile *profile );

NAObject        *na_action_profile_get_action( const NAActionProfile *profile );
gchar           *na_action_profile_get_name( const NAActionProfile *profile );
gchar           *na_action_profile_get_label( const NAActionProfile *profile );
gchar           *na_action_profile_get_path( const NAActionProfile *profile );
gchar           *na_action_profile_get_parameters( const NAActionProfile *profile );
GSList          *na_action_profile_get_basenames( const NAActionProfile *profile );
gboolean         na_action_profile_get_matchcase( const NAActionProfile *profile );
GSList          *na_action_profile_get_mimetypes( const NAActionProfile *profile );
gboolean         na_action_profile_get_is_dir( const NAActionProfile *profile );
gboolean         na_action_profile_get_is_file( const NAActionProfile *profile );
gboolean         na_action_profile_get_multiple( const NAActionProfile *profile );
GSList          *na_action_profile_get_schemes( const NAActionProfile *profile );

gboolean         na_action_profile_is_candidate( const NAActionProfile *profile, GList *files );
gchar           *na_action_profile_parse_parameters( const NAActionProfile *profile, GList *files );

G_END_DECLS

#endif /* __NA_ACTION_PROFILE_H__ */
