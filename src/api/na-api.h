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

#ifndef __NAUTILUS_ACTIONS_NA_API_H__
#define __NAUTILUS_ACTIONS_NA_API_H__

/**
 * SECTION: na_api
 * @short_description: #NAAPI interface definition.
 * @include: nautilus-actions/api/na-api.h
 *
 * These are common functions a Nautilus-Actions extension should
 * implement in order to be dynamically registered and identified.
 *
 * Nautilus-Actions v 2.30 - API version:  1
 */

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

/**
 * na_api_module_init:
 * @module: the #GTypeModule of the library being loaded.
 *
 * This function is called by Nautilus-Actions each time the library
 * is first loaded in memory.
 *
 * The dynamically loaded library may benefit of being triggered by
 * initializing itself, registering its internal GTypes, etc.
 * It should at least register module GTypes it provides.
 *
 * All N-A modules must implement this function.
 *
 * Returns: %TRUE if the initialization is successfull, %FALSE else.
 * In this later case, the plugin is unloaded.
 */
gboolean     na_api_module_init       ( GTypeModule *module );

/**
 * na_api_module_get_version:
 *
 * Returns: the version of this API supported by the module.
 *
 * If this function is not implemented by the plugin, the loader
 * considers that the plugin only implements version 1 of this API.
 */
guint        na_api_module_get_version( void );

/**
 * na_api_module_list_types:
 * @types: the array of #GType this dynamically library implements.
 *
 * This function is called by Nautilus-Actions in order to get the
 * internal GTypes implemented by the dynamically loaded library.
 *
 * Returned GTypes should already have been registered in GType system
 * (e.g. at na_api_module_init() time), and may implement one or more of
 * the interfaces defined in Nautilus-Actions API.
 *
 * One GObject will be instantiated by Nautilus-Actions for each returned
 * GType.
 *
 * All N-A modules must implement this functon, possibly returning an
 * empty list.
 *
 * Returns: the number of GTypes item in the @types array.
 */
guint        na_api_module_list_types ( const GType **types );

/**
 * na_api_module_shutdown:
 *
 * This function is called by Nautilus-Actions when it is about to
 * shutdown itself.
 *
 * The dynamicaly loaded library may benefit of this call to release
 * any resource it may have previously allocated.
 *
 * All N-A modules must implement this function.
 */
void         na_api_module_shutdown   ( void );

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_NA_API_H__ */
