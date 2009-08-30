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

#ifndef __NA_IPREFS_H__
#define __NA_IPREFS_H__

/**
 * SECTION: na_iprefs
 * @short_description: #NAIPrefs interface definition.
 * @include: common/na-iprefs.h
 *
 * This interface is to be implemented by all modules which wish take
 * benefit of preferences management. It only manages preferences which
 * are used by the plugin, and used/updated in the NACT user interface.
 *
 * Displaying the actions.
 *
 * - actions in alphabetical order: yes/no
 *   Nautilus-Actions used to display the actions in alphabetical order.
 *   Starting with 1.12.x, Nautilus-Actions lets the user rearrange
 *   himself the order of its actions.
 *   Defaults to yes to stay compatible with previous versions.
 *
 *   Actions can be organized in a set of submenus. In this case, the
 *   'alphabetical order' preferences is also satisfied, on a level
 *   basis.
 *   This is not a preference: as submenus are available, user is free
 *   to define some in NACT ; plugin will take care of them.
 *
 *   Defined order is saved in the same time than actions. So
 *   considering the following operations:
 *
 *   a) set preference to 'no'
 *   b) rearrange the items in any order
 *   c) save
 *   d) set preference to 'yes'
 *      -> the items are reordered in alphabetical order
 *   e) set preference to 'no'
 *      -> the previous order is restaured (as it has been previously
 *         saved)
 *
 *   but
 *
 *   a) set preference to 'no'
 *   b) rearrange the items in any order
 *   c) set preference to 'yes'
 *      -> the items are reordered in alphabetical order
 *   d) save
 *   e) set preference to 'no'
 *      -> the items stay in alphabetical order, as the previous save
 *         has removed the previous order.
 *
 * - adding a 'About Nautilus Actions' item at end of actions: yes/no
 *   This is used only when there is a root submenu, i.e. when the
 *   Nautilus context menu will only display one item (the root
 *   submenu). Only in this case, and if preference is 'yes', the we
 *   will add the About item at the end of the first level of submenu.
 *
 *   Note that, as a convenience, the NACT user interface provides the
 *   user with a standard item (Nautilus Actions actions) which can be
 *   used as a root menu.
 *
 *   No 'About' item is added when user organize its actions so that
 *   Nautilus context menu will have several entries at the first level.
 *
 * In all cases, the plugin takes care of providing actions to Nautilus
 * if the same order than those they are displayed in NACT.
 */

#include <glib-object.h>

#include "na-gconf-keys.h"

G_BEGIN_DECLS

#define NA_IPREFS_TYPE						( na_iprefs_get_type())
#define NA_IPREFS( object )					( G_TYPE_CHECK_INSTANCE_CAST( object, NA_IPREFS_TYPE, NAIPrefs ))
#define NA_IS_IPREFS( object )				( G_TYPE_CHECK_INSTANCE_TYPE( object, NA_IPREFS_TYPE ))
#define NA_IPREFS_GET_INTERFACE( instance )	( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NA_IPREFS_TYPE, NAIPrefsInterface ))

typedef struct NAIPrefs NAIPrefs;

typedef struct NAIPrefsInterfacePrivate NAIPrefsInterfacePrivate;

typedef struct {
	GTypeInterface            parent;
	NAIPrefsInterfacePrivate *private;
}
	NAIPrefsInterface;

GType    na_iprefs_get_type( void );

gboolean na_iprefs_get_alphabetical_order( NAIPrefs *instance );
gboolean na_iprefs_get_add_about_item( NAIPrefs *instance );

gboolean na_iprefs_get_bool( NAIPrefs *instance, const gchar *key );
void     na_iprefs_set_bool( NAIPrefs *instance, const gchar *key, gboolean value );

/* GConf general information
 */
#define NA_GCONF_PREFS_PATH		NAUTILUS_ACTIONS_CONFIG_GCONF_BASEDIR "/" NA_GCONF_SCHEMA_PREFERENCES

/* GConf Preference keys managed by IPrefs interface
 */
#define PREFS_DISPLAY_ALPHABETICAL_ORDER	"preferences-alphabetical-order"
#define PREFS_ADD_ABOUT_ITEM				"preferences-add-about-item"

G_END_DECLS

#endif /* __NA_IPREFS_H__ */
