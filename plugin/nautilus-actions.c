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

#include <string.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-file-info.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libnautilus-extension/nautilus-extension-types.h>
#include <libnautilus-extension/nautilus-file-info.h>
#include <libnautilus-extension/nautilus-menu-provider.h>
#include "nautilus-actions.h"
#include <libnautilus-actions/nautilus-actions-config.h>
#include <libnautilus-actions/nautilus-actions-config-gconf.h>
#include "nautilus-actions-test.h"
#include "nautilus-actions-utils.h"

static GObjectClass *parent_class = NULL;
static GType actions_type = 0;

GType nautilus_actions_get_type (void) 
{
	return actions_type;
}

static void nautilus_actions_execute (NautilusMenuItem *item, NautilusActionsConfigAction *action)
{
	GList *files;
	GString *cmd;
	gchar* param = NULL;

	files = (GList*)g_object_get_data (G_OBJECT (item), "files");

	cmd = g_string_new (action->path);

	param = nautilus_actions_utils_parse_parameter (action->parameters, files);
	
	if (param != NULL)
	{
		g_string_append_printf (cmd, " %s", param);
		g_free (param);
	}

	g_spawn_command_line_async (cmd->str, NULL);
	
	g_string_free (cmd, TRUE);

}

static NautilusMenuItem *nautilus_actions_create_menu_item (NautilusActionsConfigAction *action, GList *files)
{
	NautilusMenuItem *item;
	gchar* name;

	name = g_strdup_printf ("NautilusActions::%s", action->uuid);
	
	item = nautilus_menu_item_new (name, 
				action->label, 
				action->tooltip, 
				action->icon);

	g_signal_connect_data (item, 
				"activate", 
				G_CALLBACK (nautilus_actions_execute),
				action, 
				(GClosureNotify)nautilus_actions_config_action_free, 
				0);

	g_object_set_data_full (G_OBJECT (item),
			"files",
			nautilus_file_info_list_copy (files),
			(GDestroyNotify) nautilus_file_info_list_free);
	
	
	g_free (name);
	
	return item;
}

static GList *nautilus_actions_get_file_items (NautilusMenuProvider *provider, GtkWidget *window, GList *files)
{
	GList *items = NULL;
	GSList* config_list;
	GSList *iter;
	NautilusMenuItem *item;
	NautilusActions* self = NAUTILUS_ACTIONS (provider);

	if (!self->dispose_has_run)
	{
		config_list = nautilus_actions_config_get_actions (NAUTILUS_ACTIONS_CONFIG (self->configs));
		for (iter = config_list; iter; iter = iter->next)
		{
			/* Foreach configured action, check if we add a menu item */
			NautilusActionsConfigAction *action = nautilus_actions_config_action_dup ((NautilusActionsConfigAction*)iter->data);

			if (nautilus_actions_test_validate (action, files))
			{
				item = nautilus_actions_create_menu_item (action, files);
				items = g_list_append (items, item);
			}
			else
			{
				nautilus_actions_config_action_free (action);
			}
		}

		nautilus_actions_config_free_actions_list (config_list);
	}
	
	return items;
}
/*
static void nautilus_actions_notify_config_changes (GConfClient* client, 
																	 guint cnxid,
																	 GConfEntry* entry,
																	 gpointer user_data)
{
	NautilusActions* self = NAUTILUS_ACTIONS (user_data);
	gchar* key = g_strdup (gconf_entry_get_key (entry));
	gchar* tmp;
	gchar* config_name = NULL;
	gchar** strlist;
	GList* iter = NULL;

	if (!self->dispose_has_run)
	{
		// Fisrt, get the config name :
		tmp = key + strlen (self->config_root_dir) + 1; // remove the root path from the key + a slash
		strlist = g_strsplit_set (tmp, "/", 2); // Separate the first token of the relative path from the end
		config_name = g_strdup (strlist[0]);
		if ((iter = g_list_find_custom (self->configs, config_name, 
										(GCompareFunc)nautilus_actions_utils_compare_actions)) != NULL)
		{
			// config already exist => update value
			ConfigAction* action = (ConfigAction*)iter->data;

			if (g_ascii_strcasecmp (strlist[1], "test/basename") == 0)
			{
				nautilus_actions_config_action_update_test_basenames (action, gconf_value_get_list (gconf_entry_get_value (entry)));
			}
			else if (g_ascii_strcasecmp (strlist[1], "test/isfile") == 0)
			{
				nautilus_actions_config_action_update_test_isfile (action, gconf_value_get_bool (gconf_entry_get_value (entry)));
			}
			else if (g_ascii_strcasecmp (strlist[1], "test/isdir") == 0)
			{
				nautilus_actions_config_action_update_test_isdir (action, gconf_value_get_bool (gconf_entry_get_value (entry)));
			}
			else if (g_ascii_strcasecmp (strlist[1], "test/accept-multiple-files") == 0)
			{
				nautilus_actions_config_action_update_test_accept_multiple_files (action, gconf_value_get_bool (gconf_entry_get_value (entry)));
			}
			else if (g_ascii_strcasecmp (strlist[1], "test/scheme") == 0)
			{
				nautilus_actions_config_action_update_test_schemes (action, gconf_value_get_list (gconf_entry_get_value (entry)));
			}
			else if (g_ascii_strcasecmp (strlist[1], "command/parameters") == 0)
			{
				nautilus_actions_config_action_update_command_parameters (action, gconf_value_get_string (gconf_entry_get_value (entry)));
			}
			else if (g_ascii_strcasecmp (strlist[1], "command/path") == 0)
			{
				nautilus_actions_config_action_update_command_path (action, gconf_value_get_string (gconf_entry_get_value (entry)));
			}
			else if (g_ascii_strcasecmp (strlist[1], "menu-item/label") == 0)
			{
				nautilus_actions_config_action_update_menu_item_label (action, gconf_value_get_string (gconf_entry_get_value (entry)));
			}
			else if (g_ascii_strcasecmp (strlist[1], "menu-item/tooltip") == 0)
			{
				nautilus_actions_config_action_update_menu_item_tooltip (action, gconf_value_get_string (gconf_entry_get_value (entry)));
			}
			else if (g_ascii_strcasecmp (strlist[1], "version") == 0)
			{
				nautilus_actions_config_action_update_version (action, gconf_value_get_string (gconf_entry_get_value (entry)));
			}
		}
		else
		{
			// New config
		}
		g_strfreev (strlist);
	}

	g_free (key);
}
*/
static void nautilus_actions_instance_dispose (GObject *obj)
{
	NautilusActions* self = NAUTILUS_ACTIONS (obj);
	
	if (!self->dispose_has_run)
	{
		self->dispose_has_run = TRUE;

		g_object_unref (self->configs);

		/* Chain up to the parent class */
		G_OBJECT_CLASS (parent_class)->dispose (obj);
	}
}

static void nautilus_actions_instance_finalize (GObject* obj)
{
	NautilusActions* self = NAUTILUS_ACTIONS (obj);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void nautilus_actions_class_init (NautilusActionsClass *actions_class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (actions_class);

	gobject_class->dispose = nautilus_actions_instance_dispose;
	gobject_class->finalize = nautilus_actions_instance_finalize;
}

static void nautilus_actions_instance_init (GTypeInstance *instance, gpointer klass)
{
	NautilusActions* self = NAUTILUS_ACTIONS (instance);
	
	self->configs = NULL;
	self->configs = nautilus_actions_config_gconf_get ();
	self->dispose_has_run = FALSE;

	parent_class = g_type_class_peek_parent (klass);
}

static void nautilus_actions_menu_provider_iface_init (NautilusMenuProviderIface *iface)
{
	iface->get_file_items = nautilus_actions_get_file_items;
}

void nautilus_actions_register_type (GTypeModule *module)
{
	static const GTypeInfo info = {
		sizeof (NautilusActionsClass),
		(GBaseInitFunc) NULL,
		(GBaseFinalizeFunc) NULL,
		(GClassInitFunc) nautilus_actions_class_init,
		NULL,
		NULL,
		sizeof (NautilusActions),
		0,
		(GInstanceInitFunc)nautilus_actions_instance_init,
	};

	static const GInterfaceInfo menu_provider_iface_info = {
		(GInterfaceInitFunc) nautilus_actions_menu_provider_iface_init,
		NULL,
		NULL
	};

	actions_type = g_type_module_register_type (module,
								G_TYPE_OBJECT,
								"NautilusActions",
								&info, 0);

	g_type_module_add_interface (module,
								actions_type,
								NAUTILUS_TYPE_MENU_PROVIDER,
								&menu_provider_iface_info);
}

// vim:ts=3:sw=3:tw=1024
