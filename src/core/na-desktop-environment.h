/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011, 2012 Pierre Wieser and others (see AUTHORS)
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

#ifndef __CORE_API_NA_DESKTOP_ENVIRONMENT_H__
#define __CORE_API_NA_DESKTOP_ENVIRONMENT_H__

/* @title: Desktop Environment
 * @short_description: Desktop Environment Utilities.
 * @include: core/na-desktop-environment.h
 *
 * The desktop environment is used with OnlyShowIn and NotShowIn keys of
 * our desktop files. Unfortunately, there is no consensus between desktops
 * on how to determine the currently running desktop.
 * See http://lists.freedesktop.org/archives/xdg/2009-August/010940.html
 * and http://lists.freedesktop.org/archives/xdg/2011-February/011818.html
 *
 * Waiting for such a consensus (!), we have to determine this ourself.
 * Two methods:
 * - letting the user select himself which is the currenly running desktop
 *   as a user preference
 * - try to determine it automatically.
 *
 * Known desktop environments are defined at
 * http://standards.freedesktop.org/menu-spec/latest/apb.html.
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define DESKTOP_GNOME		"GNOME"
#define DESKTOP_KDE			"KDE"
#define DESKTOP_LXDE		"LXDE"
#define DESKTOP_ROX			"ROX"
#define DESKTOP_XFCE		"XFCE"
#define DESKTOP_OLD			"Old"

typedef struct {
	const gchar *id;
	const gchar *label;
}
	NADesktopEnv;

const NADesktopEnv *na_desktop_environment_get_known_list        ( void );

const gchar        *na_desktop_environment_detect_running_desktop( void );

G_END_DECLS

#endif /* __CORE_API_NA_DESKTOP_ENVIRONMENT_H__ */
