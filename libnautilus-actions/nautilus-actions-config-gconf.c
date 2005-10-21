/* Nautilus Actions configuration tool
 * Copyright (C) 2005 The GNOME Foundation
 *
 * Authors:
 *  Frederic Ruaudel (grumz@grumz.net)
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
#include "nautilus-actions-config-gconf.h"
#include "nautilus-actions-config-gconf-private.h"

enum {
	ACTION_ADDED,
	ACTION_CHANGED,
	ACTION_REMOVED,
	LAST_SIGNAL
};

static GObjectClass *parent_class = NULL;
static gint signals[LAST_SIGNAL] = { 0 };

static gboolean
save_action (NautilusActionsConfig *self, NautilusActionsConfigAction *action)
{
	g_return_val_if_fail (NAUTILUS_ACTIONS_IS_CONFIG_GCONF (self), FALSE);

	NautilusActionsConfigGconf* config = NAUTILUS_ACTIONS_CONFIG_GCONF (self);
	gchar *key;

	g_free (action->conf_section);
	action->conf_section = g_strdup_printf ("%s/%s", ACTIONS_CONFIG_DIR, action->uuid);

	/* set the version on the action */
	if (action->version)
		g_free (action->version);
	action->version = g_strdup (NAUTILUS_ACTIONS_CONFIG_VERSION);

	/* set the values in the config database */
	key = g_strdup_printf ("%s/%s", action->conf_section, ACTION_LABEL_ENTRY);
	gconf_client_set_string (config->conf_client, key, action->label, NULL);
	g_free (key);

	key = g_strdup_printf ("%s/%s", action->conf_section, ACTION_TOOLTIP_ENTRY);
	gconf_client_set_string (config->conf_client, key, action->tooltip, NULL);
	g_free (key);

	key = g_strdup_printf ("%s/%s", action->conf_section, ACTION_ICON_ENTRY);
	gconf_client_set_string (config->conf_client, key, action->icon, NULL);
	g_free (key);

	key = g_strdup_printf ("%s/%s", action->conf_section, ACTION_PATH_ENTRY);
	gconf_client_set_string (config->conf_client, key, action->path, NULL);
	g_free (key);

	key = g_strdup_printf ("%s/%s", action->conf_section, ACTION_PARAMS_ENTRY);
	gconf_client_set_string (config->conf_client, key, action->parameters, NULL);
	g_free (key);

	key = g_strdup_printf ("%s/%s", action->conf_section, ACTION_BASENAMES_ENTRY);
	gconf_client_set_list (config->conf_client, key, GCONF_VALUE_STRING, action->basenames, NULL);
	g_free (key);

	key = g_strdup_printf ("%s/%s", action->conf_section, ACTION_ISFILE_ENTRY);
	gconf_client_set_bool (config->conf_client, key, action->is_file, NULL);
	g_free (key);

	key = g_strdup_printf ("%s/%s", action->conf_section, ACTION_ISDIR_ENTRY);
	gconf_client_set_bool (config->conf_client, key, action->is_dir, NULL);
	g_free (key);

	key = g_strdup_printf ("%s/%s", action->conf_section, ACTION_MULTIPLE_ENTRY);
	gconf_client_set_bool (config->conf_client, key, action->accept_multiple_files, NULL);
	g_free (key);

	key = g_strdup_printf ("%s/%s", action->conf_section, ACTION_SCHEMES_ENTRY);
	gconf_client_set_list (config->conf_client, key, GCONF_VALUE_STRING, action->schemes, NULL);
	g_free (key);

	key = g_strdup_printf ("%s/%s", action->conf_section, ACTION_VERSION_ENTRY);
	gconf_client_set_string (config->conf_client, key, action->version, NULL);
	g_free (key);
}

gboolean
remove_action (NautilusActionsConfig *self, NautilusActionsConfigAction* action)
{
	g_return_val_if_fail (NAUTILUS_ACTIONS_IS_CONFIG_GCONF (self), FALSE);

	NautilusActionsConfigGconf* config = NAUTILUS_ACTIONS_CONFIG_GCONF (self);

	gconf_client_recursive_unset (config->conf_client, action->conf_section, 0, NULL);
}

static void
nautilus_actions_config_gconf_finalize (GObject *object)
{
	NautilusActionsConfigGconf *config = NAUTILUS_ACTIONS_CONFIG_GCONF (object);

	g_return_if_fail (NAUTILUS_ACTIONS_IS_CONFIG_GCONF (config));

	/* free all memory */
	if (config->conf_client) {
		gconf_client_remove_dir (config->conf_client, ACTIONS_CONFIG_DIR, NULL);
		gconf_client_notify_remove (config->conf_client, config->actions_notify_id);

		g_object_unref (config->conf_client);
		config->conf_client = NULL;
	}

	/* chain call to parent class */
	if (parent_class->finalize)
	{
		parent_class->finalize (object);
	}
}

static void
nautilus_actions_config_gconf_class_init (NautilusActionsConfigGconfClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	NautilusActionsConfigClass* config_class;

	parent_class = g_type_class_peek_parent (klass);
	config_class = NAUTILUS_ACTIONS_CONFIG_CLASS (klass);
	

	object_class->finalize = nautilus_actions_config_gconf_finalize;
	
	config_class->save_action = save_action;
	config_class->remove_action = remove_action;

	klass->action_added = NULL;
	klass->action_changed = NULL;
	klass->action_removed = NULL;

	/* create class signals */
	signals[ACTION_CHANGED] = g_signal_new ("action_added",
						G_TYPE_FROM_CLASS (object_class),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET (NautilusActionsConfigGconfClass, action_added),
						NULL, NULL,
						g_cclosure_marshal_VOID__POINTER,
						G_TYPE_NONE, 1, G_TYPE_POINTER);
	signals[ACTION_CHANGED] = g_signal_new ("action_changed",
						G_TYPE_FROM_CLASS (object_class),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET (NautilusActionsConfigGconfClass, action_changed),
						NULL, NULL,
						g_cclosure_marshal_VOID__POINTER,
						G_TYPE_NONE, 1, G_TYPE_POINTER);
	signals[ACTION_CHANGED] = g_signal_new ("action_removed",
						G_TYPE_FROM_CLASS (object_class),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET (NautilusActionsConfigGconfClass, action_removed),
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
nautilus_actions_config_gconf_init (NautilusActionsConfigGconf *config, NautilusActionsConfigGconfClass *klass)
{
	GSList* node;
	config->conf_client = gconf_client_get_default ();

	/* load all defined actions */
	GSList* list = gconf_client_all_dirs (config->conf_client, ACTIONS_CONFIG_DIR, NULL);

	for (node = list; node != NULL; node = node->next) {
		NautilusActionsConfigAction *action = nautilus_actions_config_action_new ();

		action->conf_section = node->data;
		action->label = get_action_string_value (config->conf_client, node->data, ACTION_LABEL_ENTRY);
		if (!action->label) {
			nautilus_actions_config_action_free (action);
			continue;
		}

		action->uuid = g_path_get_basename (action->conf_section); // Get the last part of the config section dir
		action->tooltip = get_action_string_value (config->conf_client, node->data, ACTION_TOOLTIP_ENTRY);
		action->icon = get_action_string_value (config->conf_client, node->data, ACTION_ICON_ENTRY);
		action->path = get_action_string_value (config->conf_client, node->data, ACTION_PATH_ENTRY);
		action->parameters = get_action_string_value (config->conf_client, node->data, ACTION_PARAMS_ENTRY);
		action->basenames = get_action_list_value (config->conf_client, node->data, ACTION_BASENAMES_ENTRY);
		action->is_file = get_action_bool_value (config->conf_client, node->data, ACTION_ISFILE_ENTRY);
		action->is_dir = get_action_bool_value (config->conf_client, node->data, ACTION_ISDIR_ENTRY);
		action->accept_multiple_files = get_action_bool_value (config->conf_client, node->data, ACTION_MULTIPLE_ENTRY);
		action->schemes = get_action_list_value (config->conf_client, node->data, ACTION_SCHEMES_ENTRY);
		action->version = get_action_string_value (config->conf_client, node->data, ACTION_VERSION_ENTRY);

		/* add the new action to the hash table */
		g_hash_table_insert (NAUTILUS_ACTIONS_CONFIG (config)->actions, g_strdup (action->uuid), action);
	}

	g_slist_free (list);

	/* install notification callbacks */
	gconf_client_add_dir (config->conf_client, ACTIONS_CONFIG_DIR, GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
	config->actions_notify_id = gconf_client_notify_add (config->conf_client, ACTIONS_CONFIG_DIR,
							     (GConfClientNotifyFunc) actions_changed_cb, config,
							     NULL, NULL);
}

GType
nautilus_actions_config_gconf_get_type (void)
{
   static GType type = 0;

	if (type == 0) {
		static GTypeInfo info = {
					sizeof (NautilusActionsConfigGconfClass),
					(GBaseInitFunc) NULL,
					(GBaseFinalizeFunc) NULL,
					(GClassInitFunc) nautilus_actions_config_gconf_class_init,
					NULL, NULL,
					sizeof (NautilusActionsConfigGconf),
					0,
					(GInstanceInitFunc) nautilus_actions_config_gconf_init
		};
		type = g_type_register_static (NAUTILUS_ACTIONS_TYPE_CONFIG, "NautilusActionsConfigGconf", &info, 0);
	}
	return type;
}

NautilusActionsConfigGconf *
nautilus_actions_config_gconf_get (void)
{
	static NautilusActionsConfigGconf *config = NULL;

	/* we share one NautilusActionsConfigGconf object for all */
	if (!config) {
		config = g_object_new (NAUTILUS_ACTIONS_TYPE_CONFIG_GCONF, NULL);
		return config;
	}

	return NAUTILUS_ACTIONS_CONFIG_GCONF (g_object_ref (G_OBJECT (config)));
}


