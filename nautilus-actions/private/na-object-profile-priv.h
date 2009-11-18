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

#ifndef __NAUTILUS_ACTIONS_NA_PRIVATE_OBJECT_PROFILE_PRIV_H__
#define __NAUTILUS_ACTIONS_NA_PRIVATE_OBJECT_PROFILE_PRIV_H__

#include "na-object-profile-class.h"

G_BEGIN_DECLS

/* private instance data
 */
struct NAObjectProfilePrivate {
	gboolean  dispose_has_run;

	/* profile properties
	 */
	gchar    *path;
	gchar    *parameters;

	/* ... for target 'FileSelection'
	 */
	GSList   *basenames;
	gboolean  match_case;
	GSList   *mimetypes;
	gboolean  is_file;
	gboolean  is_dir;
	gboolean  accept_multiple;
	GSList   *schemes;

	/* ... for targets 'Background' and 'Toolbar'
	 */
	GSList   *folders;
};

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_NA_PRIVATE_OBJECT_PROFILE_PRIV_H__ */
