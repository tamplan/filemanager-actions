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

#include <libnautilus-extension/nautilus-file-info.h>

#include <runtime/na-object-api.h>
#include <runtime/na-object-profile-priv.h>

#include "na-object-api.h"
#include "na-utils.h"

/**
 * na_object_profile_replace_folder_uri:
 * @profile: the #NAObjectProfile to be updated.
 * @old: the old uri.
 * @new: the new uri.
 *
 * Replaces the @old URI by the @new one.
 */
void
na_object_profile_replace_folder_uri( NAObjectProfile *profile, const gchar *old, const gchar *new )
{
	g_return_if_fail( NA_IS_OBJECT_PROFILE( profile ));

	if( !profile->private->dispose_has_run ){

		profile->private->folders = na_utils_remove_from_string_list( profile->private->folders, old );
		profile->private->folders = g_slist_append( profile->private->folders, ( gpointer ) g_strdup( new ));
	}
}

/**
 * na_object_profile_set_scheme:
 * @profile: the #NAObjectProfile to be updated.
 * @scheme: name of the scheme.
 * @selected: whether this scheme is candidate to this profile.
 *
 * Sets the status of a scheme relative to this profile.
 */
void
na_object_profile_set_scheme( NAObjectProfile *profile, const gchar *scheme, gboolean selected )
{
	/*static const gchar *thisfn = "na_object_profile_set_scheme";*/
	gboolean exist;

	g_return_if_fail( NA_IS_OBJECT_PROFILE( profile ));

	if( !profile->private->dispose_has_run ){

		exist = na_utils_find_in_list( profile->private->schemes, scheme );
		/*g_debug( "%s: scheme=%s exist=%s", thisfn, scheme, exist ? "True":"False" );*/

		if( selected && !exist ){
			profile->private->schemes = g_slist_prepend( profile->private->schemes, g_strdup( scheme ));
		}
		if( !selected && exist ){
			profile->private->schemes = na_utils_remove_ascii_from_string_list( profile->private->schemes, scheme );
		}
	}
}
