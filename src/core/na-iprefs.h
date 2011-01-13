/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011 Pierre Wieser and others (see AUTHORS)
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

#ifndef __CORE_NA_IPREFS_H__
#define __CORE_NA_IPREFS_H__

/* @title: NAIPrefs
 * @short_description: The #NAIPrefs Interface Definition
 * @include: core/na-iprefs.h
 *
 * This interface should only be implemented by #NAPivot. This is
 * because the interface stores as an implementor structure some data
 * which are only relevant for one client (e.g. GConfClient).
 *
 * Though all modules may use the public functions na_iprefs_xxx(),
 * only #NAPivot will receive update notifications, taking itself care
 * of proxying them to its identified #NAIPivotConsumers.
 *
 * Displaying the actions.
 *
 * - actions in alphabetical order:
 *
 *   Nautilus-Actions used to display the actions in alphabetical order.
 *   Starting with 1.12.x, Nautilus-Actions lets the user rearrange
 *   himself the order of its actions.
 *
 *   This option may have three values :
 *   - ascending alphabetical order (historical behavior, and default),
 *   - descending alphabetical order,
 *   - manual ordering.
 *
 * - adding a 'About Nautilus-Actions' item at end of actions: yes/no
 *
 *   When checked, an 'About Nautilus-Actions' is displayed as the last
 *   item of the root submenu of Nautilus-Actions actions.
 *
 *   It the user didn't have defined a root submenu, whether in the NACT
 *   user interface or by choosing the ad-hoc preference, the plugin
 *   doesn't provides one, and the 'About' item will not be displayed.
 *
 * Starting with 3.1.0, NAIPrefs interface is deprecated.
 * Instead, this file implements all maps needed to transform an enum
 * used in the code to and from a string stored in preferences.
 */

#include "na-pivot.h"

G_BEGIN_DECLS

/* sort mode of the items in the file manager context menu
 */
enum {
	IPREFS_ORDER_ALPHA_ASCENDING = 1,	/* default */
	IPREFS_ORDER_ALPHA_DESCENDING,
	IPREFS_ORDER_MANUAL
};

guint na_iprefs_get_import_mode( const NAPivot *pivot, const gchar *pref );
void  na_iprefs_set_import_mode( const NAPivot *pivot, const gchar *pref, guint mode );

guint na_iprefs_get_order_mode ( const NAPivot *pivot );
void  na_iprefs_set_order_mode ( const NAPivot *pivot, gint mode );

G_END_DECLS

#endif /* __CORE_NA_IPREFS_H__ */
