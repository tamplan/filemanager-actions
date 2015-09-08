/*
 * FileManager-Actions
 * A file-manager extension which offers configurable context menu modules.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
 *
 * FileManager-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * FileManager-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FileManager-Actions; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Frederic Ruaudel <grumz@grumz.net>
 *   Rodrigo Moya <rodrigo@gnome-db.org>
 *   Pierre Wieser <pwieser@trychlos.org>
 *   ... and many others (see AUTHORS)
 */

#ifndef __CORE_FMA_SETTINGS_H__
#define __CORE_FMA_SETTINGS_H__

/* @title: FMASettings
 * @short_description: The Settings Class Definition
 * @include: core/fma-settings.h
 *
 * The #FMASettings class manages users preferences.
 *
 * Actual configuration may come from two sources:
 * - a global configuration, which apply to all users, as read-only
 *   parameters;
 * - a per-user configuration.
 *
 * The configuration is implemented as keyed files:
 * - global configuration is sysconfdir/xdg/filemanager-actions/nautilus-actions.conf
 * - per-user configuration is HOME/.config/filemanager-actions/nautilus-actions.conf
 *
 * Each setting may so have its own read-only attribute, whether it
 * has been read from the global configuration or from the
 * per-user one.
 *
 * FMASettings class monitors the whole configuration.
 * A client may be informed of a modification of the value of a key either by
 * pre-registering a callback on this key (see fma_settings_register_key_callback()
 * function), or by connecting to and filtering the notification signal.
 *
 * #FMASettings class defines a singleton object, which allocates itself
 * when needed
 */

#include <glib-object.h>

G_BEGIN_DECLS

/* This is a composite key;
 * by registering a callback on this key, a client may be informed of any
 * modification regarding the readability status of the i/o providers.
 */
#define IPREFS_IO_PROVIDERS_READ_STATUS			"io-providers-read-status-composite-key"

/* other keys, mainly user preferences
 */
#define IPREFS_ADMIN_PREFERENCES_LOCKED			"preferences-locked"
#define IPREFS_ADMIN_IO_PROVIDERS_LOCKED		"io-providers-locked"
#define IPREFS_ASSISTANT_ESC_CONFIRM			"assistant-esc-confirm"
#define IPREFS_ASSISTANT_ESC_QUIT				"assistant-esc-quit"
#define IPREFS_CAPABILITY_ADD_CAPABILITY_WSP	"capability-add-capability-wsp"
#define IPREFS_COMMAND_CHOOSER_WSP				"command-command-chooser-wsp"
#define IPREFS_COMMAND_CHOOSER_URI				"command-command-chooser-lfu"
#define IPREFS_COMMAND_LEGEND_WSP				"command-legend-wsp"
#define IPREFS_DESKTOP_ENVIRONMENT				"desktop-environment"
#define IPREFS_CONFIRM_LOGOUT_WSP				"confirm-logout-wsp"
#define IPREFS_WORKING_DIR_WSP					"command-working-dir-chooser-wsp"
#define IPREFS_WORKING_DIR_URI					"command-working-dir-chooser-lfu"
#define IPREFS_SHOW_IF_RUNNING_WSP				"environment-show-if-running-wsp"
#define IPREFS_SHOW_IF_RUNNING_URI				"environment-show-if-running-lfu"
#define IPREFS_TRY_EXEC_WSP						"environment-try-exec-wsp"
#define IPREFS_TRY_EXEC_URI						"environment-try-exec-lfu"
#define IPREFS_EXPORT_ASK_USER_WSP				"export-ask-user-wsp"
#define IPREFS_EXPORT_ASK_USER_LAST_FORMAT		"export-ask-user-last-format"
#define IPREFS_EXPORT_ASK_USER_KEEP_LAST_CHOICE	"export-ask-user-keep-last-choice"
#define IPREFS_EXPORT_ASSISTANT_WSP				"export-assistant-wsp"
#define IPREFS_EXPORT_ASSISTANT_URI				"export-assistant-lfu"
#define IPREFS_EXPORT_ASSISTANT_PANED			"export-assistant-paned-width"
#define IPREFS_EXPORT_PREFERRED_FORMAT			"export-preferred-format"
#define IPREFS_FOLDER_CHOOSER_WSP				"folder-chooser-wsp"
#define IPREFS_FOLDER_CHOOSER_URI				"folder-chooser-lfu"
#define IPREFS_IMPORT_ASK_USER_WSP				"import-ask-user-wsp"
#define IPREFS_IMPORT_ASK_USER_LAST_MODE		"import-ask-user-last-mode"
#define IPREFS_IMPORT_ASSISTANT_WSP				"import-assistant-wsp"
#define IPREFS_IMPORT_ASSISTANT_URI				"import-assistant-lfu"
#define IPREFS_IMPORT_ASK_USER_KEEP_LAST_CHOICE	"import-ask-user-keep-last-choice"
#define IPREFS_IMPORT_PREFERRED_MODE			"import-preferred-mode"
#define IPREFS_ICON_CHOOSER_URI					"item-icon-chooser-lfu"
#define IPREFS_ICON_CHOOSER_PANED				"item-icon-chooser-paned-width"
#define IPREFS_ICON_CHOOSER_WSP					"item-icon-chooser-wsp"
#define IPREFS_IO_PROVIDERS_WRITE_ORDER			"io-providers-write-order"
#define IPREFS_ITEMS_ADD_ABOUT_ITEM				"items-add-about-item"
#define IPREFS_ITEMS_CREATE_ROOT_MENU			"items-create-root-menu"
#define IPREFS_ITEMS_LEVEL_ZERO_ORDER			"items-level-zero-order"
#define IPREFS_ITEMS_LIST_ORDER_MODE			"items-list-order-mode"
#define IPREFS_MAIN_PANED						"main-paned-width"
#define IPREFS_MAIN_SAVE_AUTO					"main-save-auto"
#define IPREFS_MAIN_SAVE_PERIOD					"main-save-period"
#define IPREFS_MAIN_TABS_POS					"main-tabs-pos"
#define IPREFS_MAIN_TOOLBAR_EDIT_DISPLAY		"main-toolbar-edit-display"
#define IPREFS_MAIN_TOOLBAR_FILE_DISPLAY		"main-toolbar-file-display"
#define IPREFS_MAIN_TOOLBAR_HELP_DISPLAY		"main-toolbar-help-display"
#define IPREFS_MAIN_TOOLBAR_TOOLS_DISPLAY		"main-toolbar-tools-display"
#define IPREFS_MAIN_WINDOW_WSP					"main-window-wsp"
#define IPREFS_PREFERENCES_WSP					"preferences-wsp"
#define IPREFS_PLUGIN_MENU_LOG					"plugin-menu-log-enabled"
#define IPREFS_RELABEL_DUPLICATE_ACTION			"relabel-when-duplicate-action"
#define IPREFS_RELABEL_DUPLICATE_MENU			"relabel-when-duplicate-menu"
#define IPREFS_RELABEL_DUPLICATE_PROFILE		"relabel-when-duplicate-profile"
#define IPREFS_SCHEME_ADD_SCHEME_WSP			"scheme-add-scheme-wsp"
#define IPREFS_SCHEME_DEFAULT_LIST				"scheme-default-list"
#define IPREFS_TERMINAL_PATTERN					"terminal-pattern"

#define IPREFS_IO_PROVIDER_GROUP				"io-provider"
#define IPREFS_IO_PROVIDER_READABLE				"readable"
#define IPREFS_IO_PROVIDER_WRITABLE				"writable"

/* pre-registration of a callback
 */
typedef void ( *FMASettingsKeyCallback )( const gchar *group, const gchar *key, gconstpointer new_value, gboolean mandatory, void *user_data );

void      fma_settings_register_key_callback( const gchar *key, GCallback callback, gpointer user_data );

/* signal sent when the value of a key changes
 */
#define SETTINGS_SIGNAL_KEY_CHANGED				"settings-key-changed"

void      fma_settings_free                 ( void );

gboolean  fma_settings_get_boolean          ( const gchar *key, gboolean *found, gboolean *mandatory );
gboolean  fma_settings_get_boolean_ex       ( const gchar *group, const gchar *key, gboolean *found, gboolean *mandatory );
gchar    *fma_settings_get_string           ( const gchar *key, gboolean *found, gboolean *mandatory );
GSList   *fma_settings_get_string_list      ( const gchar *key, gboolean *found, gboolean *mandatory );
guint     fma_settings_get_uint             ( const gchar *key, gboolean *found, gboolean *mandatory );
GList    *fma_settings_get_uint_list        ( const gchar *key, gboolean *found, gboolean *mandatory );

gboolean  fma_settings_set_boolean          ( const gchar *key, gboolean value );
gboolean  fma_settings_set_boolean_ex       ( const gchar *group, const gchar *key, gboolean value );
gboolean  fma_settings_set_string           ( const gchar *key, const gchar *value );
gboolean  fma_settings_set_string_ex        ( const gchar *group, const gchar *key, const gchar *value );
gboolean  fma_settings_set_string_list      ( const gchar *key, const GSList *value );
gboolean  fma_settings_set_int_ex           ( const gchar *group, const gchar *key, int value );
gboolean  fma_settings_set_uint             ( const gchar *key, guint value );
gboolean  fma_settings_set_uint_list        ( const gchar *key, const GList *value );

GSList   *fma_settings_get_groups           ( void );

G_END_DECLS

#endif /* __CORE_FMA_SETTINGS_H__ */
