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

#ifndef __CORE_NA_IPREFS_H__
#define __CORE_NA_IPREFS_H__

/**
 * SECTION: na_iprefs
 * @short_description: #NAIPrefs interface definition.
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
 * - adding a 'About Nautilus Actions' item at end of actions: yes/no
 *
 *   When checked, an 'About Nautilus Actions' is displayed as the last
 *   item of the root submenu of Nautilus Actions actions.
 *
 *   It the user didn't have defined a root submenu, whether in the NACT
 *   user interface or by choosing the ad-hoc preference, the plugin
 *   doesn't provides one, and the 'About' item will not be displayed.
 */

#include <glib-object.h>
#include <gconf/gconf-client.h>

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

/* GConf Preference keys managed by IPrefs interface
 */
#define IPREFS_GCONF_BASEDIR				"/apps/nautilus-actions"
#define IPREFS_GCONF_PREFERENCES			"preferences"
#define IPREFS_GCONF_PREFS_PATH				IPREFS_GCONF_BASEDIR "/" IPREFS_GCONF_PREFERENCES

#define IPREFS_LEVEL_ZERO_ITEMS				"iprefs-level-zero"
#define IPREFS_DISPLAY_ALPHABETICAL_ORDER	"iprefs-alphabetical-order"
#define IPREFS_CREATE_ROOT_MENU				"iprefs-create-root-menu"
#define IPREFS_ADD_ABOUT_ITEM				"iprefs-add-about-item"

#define IPREFS_RELABEL_MENUS				"iprefs-relabel-menus"
#define IPREFS_RELABEL_ACTIONS				"iprefs-relabel-actions"
#define IPREFS_RELABEL_PROFILES				"iprefs-relabel-profiles"

/* alphabetical order values
 */
enum {
	IPREFS_ORDER_ALPHA_ASCENDING = 1,
	IPREFS_ORDER_ALPHA_DESCENDING,
	IPREFS_ORDER_MANUAL
};

/* import mode
 */
enum {
	IPREFS_IMPORT_NO_IMPORT = 1,
	IPREFS_IMPORT_RENUMBER,
	IPREFS_IMPORT_OVERRIDE,
	IPREFS_IMPORT_ASK
};

GType        na_iprefs_get_type( void );

gint         na_iprefs_get_order_mode( NAIPrefs *instance );
/*void         na_iprefs_set_order_mode( NAIPrefs *instance, gint mode );*/

/*gboolean     na_iprefs_should_add_about_item( NAIPrefs *instance );*/
/*void         na_iprefs_set_add_about_item( NAIPrefs *instance, gboolean enabled );*/

/*gboolean     na_iprefs_should_create_root_menu( NAIPrefs *instance );*/
/*void         na_iprefs_set_create_root_menu( NAIPrefs *instance, gboolean enabled );*/

GConfClient *na_iprefs_get_gconf_client ( const NAIPrefs *instance );

gboolean     na_iprefs_read_bool        ( const NAIPrefs *instance, const gchar *key, gboolean default_value );
gchar       *na_iprefs_read_string      ( const NAIPrefs *instance, const gchar *key, const gchar *default_value );
GSList      *na_iprefs_read_string_list ( const NAIPrefs *instance, const gchar *key, const gchar *default_value );

/*void         na_iprefs_write_bool( NAIPrefs *instance, const gchar *key, gboolean value );*/
void         na_iprefs_write_string_list( const NAIPrefs *instance, const gchar *key, GSList *value );

/*void         na_iprefs_migrate_key( NAIPrefs *instance, const gchar *old_key, const gchar *new_key );*/

G_END_DECLS

#endif /* __CORE_NA_IPREFS_H__ */
