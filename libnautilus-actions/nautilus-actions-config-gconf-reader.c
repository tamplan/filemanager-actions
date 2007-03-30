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
#include <string.h>
#include "nautilus-actions-config-gconf-reader.h"
#include "nautilus-actions-config-gconf-private.h"

static GObjectClass *parent_class = NULL;

static gboolean
save_action (NautilusActionsConfig *self, NautilusActionsConfigAction *action)
{
	g_return_val_if_fail (NAUTILUS_ACTIONS_IS_CONFIG_GCONF_READER (self), FALSE);

	/* Nothing to do, it has already been done by the GConf notification handler */

	/* Return TRUE to allow the signals to be emited */
	return TRUE;
}

static gboolean
remove_action (NautilusActionsConfig *self, NautilusActionsConfigAction* action)
{
	g_return_val_if_fail (NAUTILUS_ACTIONS_IS_CONFIG_GCONF_READER (self), FALSE);

	/* Nothing to do, it has already been done by the GConf notification handler */

	/* Return TRUE to allow the signals to be emited */
	return TRUE;
}

static void
nautilus_actions_config_gconf_reader_finalize (GObject *object)
{
	NautilusActionsConfigGconfReader *config = NAUTILUS_ACTIONS_CONFIG_GCONF_READER (object);

	g_return_if_fail (NAUTILUS_ACTIONS_IS_CONFIG_GCONF_READER (config));

	/* free all memory */
	if (NAUTILUS_ACTIONS_CONFIG_GCONF (config)->conf_client) {
		gconf_client_remove_dir (NAUTILUS_ACTIONS_CONFIG_GCONF (config)->conf_client, ACTIONS_CONFIG_DIR, NULL);
		gconf_client_notify_remove (NAUTILUS_ACTIONS_CONFIG_GCONF (config)->conf_client, config->actions_notify_id);
	}

	/* chain call to parent class */
	if (parent_class->finalize)
	{
		parent_class->finalize (object);
	}
}

static void
nautilus_actions_config_gconf_reader_class_init (NautilusActionsConfigGconfReaderClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	NautilusActionsConfigClass* config_class;

	parent_class = g_type_class_peek_parent (klass);
	config_class = NAUTILUS_ACTIONS_CONFIG_CLASS (klass);


	object_class->finalize = nautilus_actions_config_gconf_reader_finalize;

	config_class->save_action = save_action;
	config_class->remove_action = remove_action;
}

static gchar* get_action_profile_name_from_key (const gchar* key, const gchar* uuid)
{
	gchar* prefix = g_strdup_printf ("%s/%s/%s", ACTIONS_CONFIG_DIR, uuid, ACTIONS_PROFILE_PREFIX);
	gchar* profile_name = NULL;

	if (g_str_has_prefix (key, prefix))
	{
	
		profile_name = g_strdup (key + strlen (prefix));
		gchar* pos = g_strrstr (profile_name, "/");
		if (pos != NULL)
		{
			*pos = '\0';
		}
	}

	g_free (prefix);

	return profile_name;
}

static gchar* get_action_uuid_from_key (const gchar* key)
{
	g_return_val_if_fail (g_str_has_prefix (key, ACTIONS_CONFIG_DIR), NULL);
	
	gchar* uuid = g_strdup (key + strlen (ACTIONS_CONFIG_DIR "/"));
	gchar* pos = g_strstr_len (uuid, strlen (uuid), "/");
	if (pos != NULL)
	{
		*pos = '\0';
	}

	return uuid;
}

static void
copy_list (GConfValue* value, GSList** list)
{
	   (*list) = g_slist_append ((*list), g_strdup (gconf_value_get_string (value)));
}

static void
actions_changed_cb (GConfClient *client,
		    guint cnxn_id,
		    GConfEntry *entry,
		    gpointer user_data)
{
	NautilusActionsConfig *config = NAUTILUS_ACTIONS_CONFIG (user_data);
	const char* key = gconf_entry_get_key (entry);
	GConfValue* value = gconf_entry_get_value (entry);
	gchar* uuid = get_action_uuid_from_key (key);
	GSList* list = NULL;
	gboolean is_new = FALSE;
	gchar* profile_name = get_action_profile_name_from_key (key, uuid);
	if (!profile_name)
	{
		profile_name = g_strdup (NAUTILUS_ACTIONS_DEFAULT_PROFILE_NAME);
	}

	NautilusActionsConfigAction *action = nautilus_actions_config_get_action (config, uuid);
	NautilusActionsConfigActionProfile* action_profile = nautilus_actions_config_action_get_profile (action, profile_name);

	if (action == NULL && value != NULL)
	{
		/* new action */
		action = nautilus_actions_config_action_new_default ();
		nautilus_actions_config_action_set_uuid (action, uuid);
		is_new = TRUE;
	}

	if (value == NULL)
	{
		if (action != NULL)
		{
			/* delete action if not already done */
			nautilus_actions_config_remove_action (config, uuid);
		}
	}
	else
	{
		if (g_str_has_suffix (key, ACTION_LABEL_ENTRY))
		{
			nautilus_actions_config_action_set_label (action, gconf_value_get_string (value));
		}
		else if (g_str_has_suffix (key, ACTION_TOOLTIP_ENTRY))
		{
			nautilus_actions_config_action_set_tooltip (action, gconf_value_get_string (value));
		}
		else if (g_str_has_suffix (key, ACTION_ICON_ENTRY))
		{
			nautilus_actions_config_action_set_icon (action, gconf_value_get_string (value));
		}
		else if (g_str_has_suffix (key, ACTION_PATH_ENTRY))
		{
			nautilus_actions_config_action_profile_set_path (action_profile, gconf_value_get_string (value));
		}
		else if (g_str_has_suffix (key, ACTION_PARAMS_ENTRY))
		{
			nautilus_actions_config_action_profile_set_parameters (action_profile, gconf_value_get_string (value));
		}
		else if (g_str_has_suffix (key, ACTION_BASENAMES_ENTRY))
		{
			list = NULL;
			g_slist_foreach (gconf_value_get_list (value), (GFunc) copy_list, &list);
			nautilus_actions_config_action_profile_set_basenames (action_profile, list);
			g_slist_foreach (list, (GFunc)g_free, NULL);
			g_slist_free (list);
		}
		else if (g_str_has_suffix (key, ACTION_MATCHCASE_ENTRY))
		{
			nautilus_actions_config_action_profile_set_match_case (action_profile, gconf_value_get_bool (value));
		}
		else if (g_str_has_suffix (key, ACTION_MIMETYPES_ENTRY))
		{
			list = NULL;
			g_slist_foreach (gconf_value_get_list (value), (GFunc) copy_list, &list);
			nautilus_actions_config_action_profile_set_mimetypes (action_profile, list);
			g_slist_foreach (list, (GFunc)g_free, NULL);
			g_slist_free (list);
		}
		else if (g_str_has_suffix (key, ACTION_ISFILE_ENTRY))
		{
			nautilus_actions_config_action_profile_set_is_file (action_profile, gconf_value_get_bool (value));
		}
		else if (g_str_has_suffix (key, ACTION_ISDIR_ENTRY))
		{
			nautilus_actions_config_action_profile_set_is_dir (action_profile, gconf_value_get_bool (value));
		}
		else if (g_str_has_suffix (key, ACTION_MULTIPLE_ENTRY))
		{
			nautilus_actions_config_action_profile_set_accept_multiple (action_profile, gconf_value_get_bool (value));
		}
		else if (g_str_has_suffix (key, ACTION_SCHEMES_ENTRY))
		{
			list = NULL;
			g_slist_foreach (gconf_value_get_list (value), (GFunc) copy_list, &list);
			nautilus_actions_config_action_profile_set_schemes (action_profile, list);
			g_slist_foreach (list, (GFunc)g_free, NULL);
			g_slist_free (list);
		}

		if (is_new)
		{
			nautilus_actions_config_add_action (config, action, NULL);
			nautilus_actions_config_action_free (action);
		}
		else
		{
			nautilus_actions_config_update_action (config, action);
		}
	}

	g_free (profile_name);
	g_free (uuid);
}

static void
nautilus_actions_config_gconf_reader_init (NautilusActionsConfigGconfReader *config, NautilusActionsConfigGconfReaderClass *klass)
{
	/* install notification callbacks */
	gconf_client_add_dir (NAUTILUS_ACTIONS_CONFIG_GCONF (config)->conf_client, ACTIONS_CONFIG_DIR, GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
	config->actions_notify_id = gconf_client_notify_add (NAUTILUS_ACTIONS_CONFIG_GCONF (config)->conf_client, 
							     ACTIONS_CONFIG_DIR,
							     (GConfClientNotifyFunc) actions_changed_cb, config,
							     NULL, NULL);
}

GType
nautilus_actions_config_gconf_reader_get_type (void)
{
   static GType type = 0;

	if (type == 0) {
		static GTypeInfo info = {
					sizeof (NautilusActionsConfigGconfReaderClass),
					(GBaseInitFunc) NULL,
					(GBaseFinalizeFunc) NULL,
					(GClassInitFunc) nautilus_actions_config_gconf_reader_class_init,
					NULL, NULL,
					sizeof (NautilusActionsConfigGconfReader),
					0,
					(GInstanceInitFunc) nautilus_actions_config_gconf_reader_init
		};
		type = g_type_register_static (NAUTILUS_ACTIONS_TYPE_CONFIG_GCONF, "NautilusActionsConfigGconfReader", &info, 0);
	}
	return type;
}

NautilusActionsConfigGconfReader *
nautilus_actions_config_gconf_reader_get (void)
{
	static NautilusActionsConfigGconfReader *config = NULL;

	/* we share one NautilusActionsConfigGconfReader object for all */
	if (!config) {
		config = g_object_new (NAUTILUS_ACTIONS_TYPE_CONFIG_GCONF_READER, NULL);
		return config;
	}

	return NAUTILUS_ACTIONS_CONFIG_GCONF_READER (g_object_ref (G_OBJECT (config)));
}
// vim:ts=3:sw=3:tw=1024:cin
