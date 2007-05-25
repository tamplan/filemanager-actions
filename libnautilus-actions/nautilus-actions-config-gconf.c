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

static GObjectClass *parent_class = NULL;

static void
nautilus_actions_config_gconf_finalize (GObject *object)
{
	NautilusActionsConfigGconf *config = NAUTILUS_ACTIONS_CONFIG_GCONF (object);

	g_return_if_fail (NAUTILUS_ACTIONS_IS_CONFIG_GCONF (config));

	/* free all memory */
	if (config->conf_client) {
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

	config_class->save_action = NULL;
	config_class->remove_action = NULL;

}

static gchar *
get_action_profile_name (const gchar *parent_dir, const gchar *profile_dir)
{
	gchar* prefix = g_strdup_printf ("%s/%s", parent_dir, ACTIONS_PROFILE_PREFIX);
	gchar* profile_name = NULL;

	if (g_str_has_prefix (profile_dir, prefix))
	{
		profile_name = g_strdup (profile_dir + strlen (prefix));
		gchar* pos = g_strrstr (profile_name, "/");
		if (pos != NULL)
		{
			*pos = '\0';
		}
	}

	g_free (prefix);

	return profile_name;
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

/*static void
actions_changed_cb (GConfClient *client,
		    guint cnxn_id,
		    GConfEntry *entry,
		    gpointer user_data)
{
	NautilusActionsConfig *config = NAUTILUS_ACTIONS_CONFIG (user_data);
	const char* key = gconf_entry_get_key (entry);
	GConfValue* value = gconf_entry_get_value (entry);

}*/

static void
nautilus_actions_config_gconf_init (NautilusActionsConfigGconf *config, NautilusActionsConfigGconfClass *klass)
{
	GSList* node;
	GSList* iter;
	GSList* profile_list = NULL;
	gchar* profile_dir;
	gchar* profile_name;
	NautilusActionsConfigActionProfile* action_profile;
	config->conf_client = gconf_client_get_default ();

	/* load all defined actions */
	GSList* list = gconf_client_all_dirs (config->conf_client, ACTIONS_CONFIG_DIR, NULL);

	for (node = list; node != NULL; node = node->next) {
		NautilusActionsConfigAction *action = nautilus_actions_config_action_new ();

		action->conf_section = (gchar*)node->data;
		action->label = get_action_string_value (config->conf_client, (gchar*)node->data, ACTION_LABEL_ENTRY);
		if (!action->label) {
			nautilus_actions_config_action_free (action);
			continue;
		}

		action->uuid = g_path_get_basename (action->conf_section); // Get the last part of the config section dir
		action->tooltip = get_action_string_value (config->conf_client, (gchar*)node->data, ACTION_TOOLTIP_ENTRY);
		action->icon = get_action_string_value (config->conf_client, (gchar*)node->data, ACTION_ICON_ENTRY);
		action->version = get_action_string_value (config->conf_client, (gchar*)node->data, ACTION_VERSION_ENTRY);

		if (g_ascii_strcasecmp (action->version, "2.0") < 0)
		{
			action_profile = nautilus_actions_config_action_profile_new ();

			//--> manage backward compatibility
			action_profile->path = get_action_string_value (config->conf_client, (gchar*)node->data, ACTION_PATH_ENTRY);
			action_profile->parameters = get_action_string_value (config->conf_client, (gchar*)node->data, ACTION_PARAMS_ENTRY);
			action_profile->basenames = get_action_list_value (config->conf_client, (gchar*)node->data, ACTION_BASENAMES_ENTRY);
			action_profile->match_case = get_action_bool_value (config->conf_client, (gchar*)node->data, ACTION_MATCHCASE_ENTRY);
			action_profile->mimetypes = get_action_list_value (config->conf_client, (gchar*)node->data, ACTION_MIMETYPES_ENTRY);
			action_profile->is_file = get_action_bool_value (config->conf_client, (gchar*)node->data, ACTION_ISFILE_ENTRY);
			action_profile->is_dir = get_action_bool_value (config->conf_client, (gchar*)node->data, ACTION_ISDIR_ENTRY);
			action_profile->accept_multiple_files = get_action_bool_value (config->conf_client, (gchar*)node->data, ACTION_MULTIPLE_ENTRY);
			action_profile->schemes = get_action_list_value (config->conf_client, (gchar*)node->data, ACTION_SCHEMES_ENTRY);

			if (g_ascii_strcasecmp (action->version, "1.0") == 0)
			{
				action_profile->match_case = TRUE;
				action_profile->mimetypes = g_slist_append (action_profile->mimetypes, g_strdup ("*/*"));
			}
			else
			{
				action_profile->mimetypes = get_action_list_value (config->conf_client, (gchar*)node->data, ACTION_MIMETYPES_ENTRY);
				action_profile->match_case = get_action_bool_value (config->conf_client, (gchar*)node->data, ACTION_MATCHCASE_ENTRY);
			}

			nautilus_actions_config_action_add_profile (action, NAUTILUS_ACTIONS_DEFAULT_PROFILE_NAME, action_profile, NULL);
		}
		else
		{
			/* load all defined profiles */
			profile_list = gconf_client_all_dirs (config->conf_client, (gchar*)node->data, NULL);

			for (iter = profile_list; iter; iter = iter->next)
			{
				profile_dir = (gchar*)iter->data;
				profile_name = get_action_profile_name ((gchar*)node->data, (gchar*)iter->data);
				action_profile = nautilus_actions_config_action_profile_new ();

				action_profile->path = get_action_string_value (config->conf_client, profile_dir, ACTION_PATH_ENTRY);
				action_profile->parameters = get_action_string_value (config->conf_client, profile_dir, ACTION_PARAMS_ENTRY);
				action_profile->basenames = get_action_list_value (config->conf_client, profile_dir, ACTION_BASENAMES_ENTRY);
				action_profile->match_case = get_action_bool_value (config->conf_client, profile_dir, ACTION_MATCHCASE_ENTRY);
				action_profile->mimetypes = get_action_list_value (config->conf_client, profile_dir, ACTION_MIMETYPES_ENTRY);
				action_profile->is_file = get_action_bool_value (config->conf_client, profile_dir, ACTION_ISFILE_ENTRY);
				action_profile->is_dir = get_action_bool_value (config->conf_client, profile_dir, ACTION_ISDIR_ENTRY);
				action_profile->accept_multiple_files = get_action_bool_value (config->conf_client, profile_dir, ACTION_MULTIPLE_ENTRY);
				action_profile->schemes = get_action_list_value (config->conf_client, profile_dir, ACTION_SCHEMES_ENTRY);

				nautilus_actions_config_action_add_profile (action, profile_name, action_profile, NULL);

				g_free (profile_name);
				g_free (profile_dir);
			}

			g_slist_free (profile_list);
		}

		/* add the new action to the hash table */
		g_hash_table_insert (NAUTILUS_ACTIONS_CONFIG (config)->actions, g_strdup (action->uuid), action);

		// Free the gconf dir string once used
		g_free (node->data);
	}

	g_slist_free (list);

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

// vim:ts=3:sw=3:tw=1024:cin
