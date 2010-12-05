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

#ifndef __NAUTILUS_ACTIONS_API_NA_DBUS_H__
#define __NAUTILUS_ACTIONS_API_NA_DBUS_H__

/**
 * SECTION: dbus
 * @section_id: dbus
 * @title: DBus interface (na-dbus.h title)
 * @short_description: DBus interface (na-dbus.h short description)
 * @include: nautilus-actions/na-dbus.h
 *
 * Nautilus-Actions, through its Tracker plugin, exposes several DBus interfaces.
 * These interfaces may be queried in order to get informations about current
 * selection, Nautilus-Actions status, and so on.
 */

#include <glib.h>

G_BEGIN_DECLS

#define NAUTILUS_ACTIONS_DBUS_SERVICE	"org.nautilus-actions.DBus"

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_API_NA_DBUS_H__ */
