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
 * This interface should only be implemented by #NAPivot. This is
 * because the interface stores as an implementor structure some data
 * which are only relevant for one client (GConfClient, GConf monitors,
 * etc.).
 *
 * Though all modules may use the public functions na_iprefs_xxx(),
 * only #NAPivot will receive update notifications, taking itself care
 * of proxying them to identified consumers.
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
 * - adding a 'About Nautilus Actions' item at end of actions: yes/no
 *
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

#include "na-gconf-keys-base.h"

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

GSList  *na_iprefs_get_level_zero_items( NAIPrefs *instance );
void     na_iprefs_set_level_zero_items( NAIPrefs *instance, GSList *order );

gint     na_iprefs_get_order_mode( NAIPrefs *instance );
void     na_iprefs_set_order_mode( NAIPrefs *instance, gint mode );

gboolean na_iprefs_should_add_about_item( NAIPrefs *instance );
void     na_iprefs_set_add_about_item( NAIPrefs *instance, gboolean enabled );

gchar   *na_iprefs_read_string( NAIPrefs *instance, const gchar *key, const gchar *default_value );
void     na_iprefs_write_string( NAIPrefs *instance, const gchar *key, const gchar *value );

/* GConf key
 */
#define NA_GCONF_PREFERENCES				"preferences"
#define NA_GCONF_PREFS_PATH					NAUTILUS_ACTIONS_GCONF_BASEDIR "/" NA_GCONF_PREFERENCES

/* GConf Preference keys managed by IPrefs interface
 */
#define PREFS_LEVEL_ZERO_ITEMS				"iprefs-level-zero"
#define PREFS_DISPLAY_ALPHABETICAL_ORDER	"iprefs-alphabetical-order"
#define PREFS_ADD_ABOUT_ITEM				"iprefs-add-about-item"

/* alphabetical order values
 */
enum {
	PREFS_ORDER_ALPHA_ASCENDING = 1,
	PREFS_ORDER_ALPHA_DESCENDING,
	PREFS_ORDER_MANUAL
};

G_END_DECLS

#endif /* __NA_IPREFS_H__ */
