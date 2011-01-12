/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu modules.
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

#ifndef __CORE_NA_SETTINGS_H__
#define __CORE_NA_SETTINGS_H__

/* @title: NASettings
 * @short_description: The Settings Class Definition
 * @include: core/na-settings.h
 *
 * The #NASettings class manages users preferences.
 *
 * Actual configuration may come from two sources:
 * - a global configuration, which apply to all users, as read-only
 *   parameters;
 * - a per-user configuration.
 *
 * The configuration is implemented as keyed files:
 * - global configuration is sysconfdir/nautilus-actions.conf
 * - per-user configuration is HOME/.config/nautilus-actions.conf
 *
 * Each setting has so its own read-only attribute, whether it
 * has been readen from the global configuration or from the
 * per-user one.
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define NA_SETTINGS_TYPE                  ( na_settings_get_type())
#define NA_SETTINGS( object )             ( G_TYPE_CHECK_INSTANCE_CAST( object, NA_SETTINGS_TYPE, NASettings ))
#define NA_SETTINGS_CLASS( klass )        ( G_TYPE_CHECK_CLASS_CAST( klass, NA_SETTINGS_TYPE, NASettingsClass ))
#define NA_IS_SETTINGS( object )          ( G_TYPE_CHECK_INSTANCE_TYPE( object, NA_SETTINGS_TYPE ))
#define NA_IS_SETTINGS_CLASS( klass )     ( G_TYPE_CHECK_CLASS_TYPE(( klass ), NA_SETTINGS_TYPE ))
#define NA_SETTINGS_GET_CLASS( object )   ( G_TYPE_INSTANCE_GET_CLASS(( object ), NA_SETTINGS_TYPE, NASettingsClass ))

typedef struct _NASettingsPrivate         NASettingsPrivate;
typedef struct _NASettingsClassPrivate    NASettingsClassPrivate;

typedef struct {
	/*< private >*/
	GObject            parent;
	NASettingsPrivate *private;
}
	NASettings;

typedef struct {
	/*< private >*/
	GObjectClass            parent;
	NASettingsClassPrivate *private;
}
	NASettingsClass;

GType na_settings_get_type( void );

/* monitorable keys
 */
#define NA_SETTINGS_RUNTIME_IO_PROVIDER_READ_STATUS		"io-provider-read-status-composite-key"
#define NA_SETTINGS_RUNTIME_IO_PROVIDERS_READ_ORDER		"io-providers-read-order"
#define NA_SETTINGS_RUNTIME_ITEMS_ADD_ABOUT_ITEM		"items-add-about-item"
#define NA_SETTINGS_RUNTIME_ITEMS_CREATE_ROOT_MENU		"items-create-root-menu"
#define NA_SETTINGS_RUNTIME_ITEMS_LEVEL_ZERO_ORDER		"items-level-zero-order"
#define NA_SETTINGS_RUNTIME_ITEMS_LIST_ORDER_MODE		"items-list-order-mode"

typedef void ( *NASettingsKeyCallback )( const gchar *group, const gchar *key, gconstpointer new_value, gboolean mandatory, void *user_data );

NASettings *na_settings_new                     ( void );

void        na_settings_register_key_callback   ( NASettings *settings, const gchar *key, GCallback callback, gpointer user_data );

gboolean    na_settings_get_boolean             ( NASettings *settings, const gchar *key, gboolean *found, gboolean *mandatory );
gboolean    na_settings_get_boolean_ex          ( NASettings *settings, const gchar *group, const gchar *key, gboolean *found, gboolean *mandatory );
gchar      *na_settings_get_string              ( NASettings *settings, const gchar *key, gboolean *found, gboolean *mandatory );
GSList     *na_settings_get_string_list         ( NASettings *settings, const gchar *key, gboolean *found, gboolean *mandatory );
guint       na_settings_get_uint                ( NASettings *settings, const gchar *key, gboolean *found, gboolean *mandatory );
GList      *na_settings_get_uint_list           ( NASettings *settings, const gchar *key, gboolean *found, gboolean *mandatory );

void        na_settings_set_boolean             ( NASettings *settings, const gchar *key, gboolean value );
void        na_settings_set_string              ( NASettings *settings, const gchar *key, gchar *value );
void        na_settings_set_string_list         ( NASettings *settings, const gchar *key, GSList *value );
void        na_settings_set_uint                ( NASettings *settings, const gchar *key, guint value );
void        na_settings_set_uint_list           ( NASettings *settings, const gchar *key, GList *value );

G_END_DECLS

#endif /* __CORE_NA_SETTINGS_H__ */
