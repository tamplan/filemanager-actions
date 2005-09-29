/* Nautilus Actions configuration tool
 * Copyright (C) 2005 The GNOME Foundation
 *
 * Authors:
 *	 Rodrigo Moya (rodrigo@gnome-db.org)
 *
 * This Program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this Library; see the file COPYING.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <uuid/uuid.h>
#include "nautilus-actions-config.h"

#define ACTIONS_CONFIG_DIR     "/apps/nautilus-actions/configurations"
#define ACTION_LABEL_ENTRY     "label"
#define ACTION_TOOLTIP_ENTRY   "tooltip"
#define ACTION_PATH_ENTRY      "path"
#define ACTION_PARAMS_ENTRY    "parameters"
#define ACTION_BASENAMES_ENTRY "basenames"
#define ACTION_ISFILE_ENTRY    "isfile"
#define ACTION_ISDIR_ENTRY     "isdir"
#define ACTION_MULTIPLE_ENTRY  "accept-multiple-files"
#define ACTION_SCHEMES_ENTRY   "schemes"
#define ACTION_VERSION_ENTRY   "version"

enum {
	ACTION_ADDED,
	ACTION_CHANGED,
	ACTION_REMOVED,
	LAST_SIGNAL
};

static GObjectClass *parent_class = NULL;
static gint signals[LAST_SIGNAL] = { 0 };

static void
nautilus_actions_config_finalize (GObject *object)
{
	NautilusActionsConfig *config = (NautilusActionsConfig *) object;

	g_return_if_fail (NAUTILUS_ACTIONS_IS_CONFIG (config));

	/* free all memory */
	if (config->conf_client) {
		gconf_client_remove_dir (config->conf_client, ACTIONS_CONFIG_DIR, NULL);
		gconf_client_notify_remove (config->conf_client, config->actions_notify_id);

		g_object_unref (config->conf_client);
		config->conf_client = NULL;
	}

	if (config->actions) {
		g_hash_table_destroy (config->actions);
		config->actions = NULL;
	}

	/* chain call to parent class */
	if (parent_class->finalize)
		parent_class->finalize (object);
}

static void
nautilus_actions_config_class_init (NautilusActionsConfigClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = nautilus_actions_config_finalize;
	klass->action_added = NULL;
	klass->action_changed = NULL;
	klass->action_removed = NULL;

	/* create class signals */
	signals[ACTION_CHANGED] = g_signal_new ("action_added",
						G_TYPE_FROM_CLASS (object_class),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET (NautilusActionsConfigClass, action_added),
						NULL, NULL,
						g_cclosure_marshal_VOID__POINTER,
						G_TYPE_NONE, 1, G_TYPE_POINTER);
	signals[ACTION_CHANGED] = g_signal_new ("action_changed",
						G_TYPE_FROM_CLASS (object_class),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET (NautilusActionsConfigClass, action_changed),
						NULL, NULL,
						g_cclosure_marshal_VOID__POINTER,
						G_TYPE_NONE, 1, G_TYPE_POINTER);
	signals[ACTION_CHANGED] = g_signal_new ("action_removed",
						G_TYPE_FROM_CLASS (object_class),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET (NautilusActionsConfigClass, action_removed),
						NULL, NULL,
						g_cclosure_marshal_VOID__POINTER,
						G_TYPE_NONE, 1, G_TYPE_POINTER);
}

static gboolean
get_action_bool_value (GConfClient *client, const gchar *dir, const gchar *value)
{
	gchar *key;
	gboolean b;

	key = g_strdup_printf ("%s/%s", dir, value);
	b = gconf_client_get_bool (client, key, NULL);

	g_free (key);

	return b;
}

static GSList *
get_action_list_value (GConfClient *client, const gchar *dir, const gchar *value)
{
	gchar *key;
	GSList *l;

	key = g_strdup_printf ("%s/%s", dir, value);
	l = gconf_client_get_list (client, key, GCONF_VALUE_STRING, NULL);

	g_free (key);

	return l;
}

static gchar *
get_action_string_value (GConfClient *client, const gchar *dir, const gchar *value)
{
	gchar *key, *s;

	key = g_strdup_printf ("%s/%s", dir, value);
	s = gconf_client_get_string (client, key, NULL);

	g_free (key);

	return s;
}

static void
actions_changed_cb (GConfClient *client,
		    guint cnxn_id,
		    GConfEntry *entry,
		    gpointer user_data)
{
	NautilusActionsConfig *config = user_data;
}

static void
nautilus_actions_config_init (NautilusActionsConfig *config, NautilusActionsConfigClass *klass)
{
	GSList *list, *node;

	config->conf_client = gconf_client_get_default ();
	config->actions = g_hash_table_new_full (g_str_hash, g_str_equal,
						 (GDestroyNotify) g_free,
						 (GDestroyNotify) nautilus_actions_config_action_free);

	/* load all defined actions */
	list = gconf_client_all_dirs (config->conf_client, ACTIONS_CONFIG_DIR, NULL);

	for (node = list; node != NULL; node = node->next) {
		NautilusActionsConfigAction *action = nautilus_actions_config_action_new ();

		action->conf_section = node->data;
		action->label = get_action_string_value (config->conf_client, node->data, ACTION_LABEL_ENTRY);
		if (!action->label) {
			nautilus_actions_config_action_free (action);
			continue;
		}

		action->tooltip = get_action_string_value (config->conf_client, node->data, ACTION_TOOLTIP_ENTRY);
		action->path = get_action_string_value (config->conf_client, node->data, ACTION_PATH_ENTRY);
		action->parameters = get_action_string_value (config->conf_client, node->data, ACTION_PARAMS_ENTRY);
		action->basenames = get_action_list_value (config->conf_client, node->data, ACTION_BASENAMES_ENTRY);
		action->is_file = get_action_bool_value (config->conf_client, node->data, ACTION_ISFILE_ENTRY);
		action->is_dir = get_action_bool_value (config->conf_client, node->data, ACTION_ISDIR_ENTRY);
		action->accept_multiple_files = get_action_bool_value (config->conf_client, node->data, ACTION_MULTIPLE_ENTRY);
		action->schemes = get_action_list_value (config->conf_client, node->data, ACTION_SCHEMES_ENTRY);
		action->version = get_action_string_value (config->conf_client, node->data, ACTION_VERSION_ENTRY);

		/* add the new action to the hash table */
		g_hash_table_insert (config->actions, g_strdup (action->label), action);
	}

	g_slist_free (list);

	/* install notification callbacks */
	gconf_client_add_dir (config->conf_client, ACTIONS_CONFIG_DIR, GCONF_CLIENT_PRELOAD_NONE, NULL);
	config->actions_notify_id = gconf_client_notify_add (config->conf_client, ACTIONS_CONFIG_DIR,
							     (GConfClientNotifyFunc) actions_changed_cb, config,
							     NULL, NULL);
}

GType
nautilus_actions_config_get_type (void)
{
   static GType type = 0;

        if (type == 0) {
                static GTypeInfo info = {
                        sizeof (NautilusActionsConfigClass),
                        (GBaseInitFunc) NULL,
                        (GBaseFinalizeFunc) NULL,
                        (GClassInitFunc) nautilus_actions_config_class_init,
                        NULL, NULL,
                        sizeof (NautilusActionsConfig),
                        0,
			(GInstanceInitFunc) nautilus_actions_config_init
                };
		type = g_type_register_static (G_TYPE_OBJECT, "NautilusActionsConfig", &info, 0);
        }
        return type;
}

NautilusActionsConfig *
nautilus_actions_config_get (void)
{
	static NautilusActionsConfig *config = NULL;

	/* we share one NautilusActionsConfig object for all */
	if (!config) {
		config = g_object_new (NAUTILUS_ACTIONS_TYPE_CONFIG, NULL);
		return config;
	}

	return NAUTILUS_ACTIONS_CONFIG (g_object_ref (G_OBJECT (config)));
}

static void
add_hash_action_to_list (gpointer key, gpointer value, gpointer user_data)
{
	GSList **list = user_data;

	*list = g_slist_append (*list, value);
}

NautilusActionsConfigAction *
nautilus_actions_config_get_action (NautilusActionsConfig *config, const gchar *label)
{
	g_return_val_if_fail (NAUTILUS_ACTIONS_IS_CONFIG (config), NULL);

	return g_hash_table_lookup (config->actions, label);
}

GSList *
nautilus_actions_config_get_actions (NautilusActionsConfig *config)
{
	GSList *list = NULL;

	g_return_val_if_fail (NAUTILUS_ACTIONS_IS_CONFIG (config), NULL);

	g_hash_table_foreach (config->actions, (GHFunc) add_hash_action_to_list, &list);

	return list;
}

void
nautilus_actions_config_free_actions_list (GSList *list)
{
	g_slist_free (list);
}

static gboolean
save_action (NautilusActionsConfig *config, NautilusActionsConfigAction *action)
{
	gchar *key;

	/* set the version on the action */
	if (action->version)
		g_free (action->version);
	action->version = g_strdup (VERSION);

	/* set the values in the config database */
	key = g_strdup_printf ("%s/%s", action->conf_section, ACTION_LABEL_ENTRY);
	gconf_client_set_string (config->conf_client, key, action->label, NULL);
	g_free (key);

	key = g_strdup_printf ("%s/%s", action->conf_section, ACTION_TOOLTIP_ENTRY);
	gconf_client_set_string (config->conf_client, key, action->tooltip, NULL);
	g_free (key);

	key = g_strdup_printf ("%s/%s", action->conf_section, ACTION_PATH_ENTRY);
	gconf_client_set_string (config->conf_client, key, action->path, NULL);
	g_free (key);

	key = g_strdup_printf ("%s/%s", action->conf_section, ACTION_PARAMS_ENTRY);
	gconf_client_set_string (config->conf_client, key, action->parameters, NULL);
	g_free (key);

	//g_strdup_printf ("%s/%s", actions->conf_section, ACTION_BASENAMES_ENTRY);

	key = g_strdup_printf ("%s/%s", action->conf_section, ACTION_ISFILE_ENTRY);
	gconf_client_set_bool (config->conf_client, key, action->is_file, NULL);
	g_free (key);

	key = g_strdup_printf ("%s/%s", action->conf_section, ACTION_ISDIR_ENTRY);
	gconf_client_set_bool (config->conf_client, key, action->is_dir, NULL);
	g_free (key);

	key = g_strdup_printf ("%s/%s", action->conf_section, ACTION_MULTIPLE_ENTRY);
	gconf_client_set_bool (config->conf_client, key, action->accept_multiple_files, NULL);
	g_free (key);

	//g_strdup_printf ("%s/%s", action->conf_section, ACTION_SCHEMES_ENTRY);

	key = g_strdup_printf ("%s/%s", action->conf_section, ACTION_VERSION_ENTRY);
	gconf_client_set_string (config->conf_client, key, action->version, NULL);
	g_free (key);
}

gboolean
nautilus_actions_config_add_action (NautilusActionsConfig *config, NautilusActionsConfigAction *action)
{
	uuid_t uuid;
	gchar uuid_str[64];

	g_return_val_if_fail (NAUTILUS_ACTIONS_IS_CONFIG (config), FALSE);
	g_return_val_if_fail (action != NULL, FALSE);

	if (g_hash_table_lookup (config->actions, action->label))
		return FALSE;

	/* generate an ID for this new entry */
	g_free (action->conf_section);
	uuid_generate (uuid);
	uuid_unparse (uuid, uuid_str);
	action->conf_section = g_strdup_printf ("%s/%s", ACTIONS_CONFIG_DIR, uuid_str);

	/* now save the keys */
	return save_action (config, action);
}

gboolean
nautilus_actions_config_update_action (NautilusActionsConfig *config, NautilusActionsConfigAction *action)
{
	g_return_val_if_fail (NAUTILUS_ACTIONS_IS_CONFIG (config), FALSE);
	g_return_val_if_fail (action != NULL, FALSE);

	if (!g_hash_table_lookup (config->actions, action->label))
		return FALSE;

	return save_action (config, action);
}

gboolean
nautilus_actions_config_remove_action (NautilusActionsConfig *config, const gchar *label)
{
}

NautilusActionsConfigAction *nautilus_actions_config_action_new (void)
{
	return g_new0 (NautilusActionsConfigAction, 1);
}

void
nautilus_actions_config_action_set_label (NautilusActionsConfigAction *action, const gchar *label)
{
	g_return_if_fail (action != NULL);

	if (action->label)
		g_free (action->label);
	action->label = g_strdup (label);
}

void
nautilus_actions_config_action_set_tooltip (NautilusActionsConfigAction *action, const gchar *tooltip)
{
	g_return_if_fail (action != NULL);

	if (action->tooltip)
		g_free (action->tooltip);
	action->tooltip = g_strdup (tooltip);
}

void
nautilus_actions_config_action_set_path (NautilusActionsConfigAction *action, const gchar *path)
{
	g_return_if_fail (action != NULL);

	if (action->path)
		g_free (action->path);
	action->path = g_strdup (path);
}

void
nautilus_actions_config_action_set_parameters (NautilusActionsConfigAction *action, const gchar *parameters)
{
	g_return_if_fail (action != NULL);

	if (action->parameters)
		g_free (action->parameters);
	action->parameters = g_strdup (parameters);
}

void
nautilus_actions_config_action_set_basenames (NautilusActionsConfigAction *action, GSList *basenames)
{
}

void
nautilus_actions_config_action_free (NautilusActionsConfigAction *action)
{
	if (action->conf_section) {
		g_free (action->conf_section);
		action->conf_section = NULL;
	}

	if (action->label) {
		g_free (action->label);
		action->label = NULL;
	}

	if (action->tooltip) {
		g_free (action->tooltip);
		action->tooltip = NULL;
	}

	if (action->path) {
		g_free (action->path);
		action->path = NULL;
	}

	if (action->parameters) {
		g_free (action->parameters);
		action->parameters = NULL;
	}

	if (action->basenames) {
		g_slist_foreach (action->basenames, (GFunc) g_free, NULL);
		g_slist_free (action->basenames);
		action->basenames = NULL;
	}

	if (action->schemes) {
		g_slist_foreach (action->schemes, (GFunc) g_free, NULL);
		g_slist_free (action->schemes);
		action->schemes = NULL;
	}

	if (action->version) {
		g_free (action->version);
		action->version = NULL;
	}

	g_free (action);
}
