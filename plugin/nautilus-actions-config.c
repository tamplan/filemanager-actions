#include <config.h>

#include "nautilus-actions-utils.h"
#include "nautilus-actions-config.h"

static void nautilus_actions_config_free_config_entries (GSList* config_entries)
{
	GSList* iter;
	
	for (iter = config_entries; iter; iter = iter->next)
	{
		g_free ((gchar*)iter->data);
	}

	g_slist_free (config_entries);
	config_entries = NULL;
}

static ConfigAction *nautilus_actions_config_action_new (gchar* name, gchar* version)
{
	ConfigAction* action = NULL;

	action = g_new (ConfigAction, 1);
	
	action->name = g_strdup (name); 
	action->version = g_strdup (version);
	
	action->test = g_new (ConfigActionTest, 1);
	action->test->basenames = NULL;
	action->test->isfile = FALSE;
	action->test->isdir = FALSE;
	action->test->accept_multiple_file = FALSE;
	action->test->schemes = NULL;
	
	action->command = g_new (ConfigActionCommand, 1);
	action->command->path = NULL;
	action->command->parameters = NULL;
	
	action->menu_item = g_new (ConfigActionMenuItem, 1);
	action->menu_item->label = NULL;
	action->menu_item->tooltip = NULL;
	
	return action;
}

static gboolean nautilus_actions_config_action_fill_test (GConfClient* gconf_client, const gchar* config_dir, ConfigAction* action)
{
	gboolean retv = FALSE;
	gboolean basename_ok = FALSE;
	gboolean isfile_ok = FALSE;
	gboolean isdir_ok = FALSE;
	gboolean scheme_ok = FALSE;
	gboolean accept_multiple_file_ok = FALSE;
	ConfigActionTest* test_action = action->test;
	gchar* key;
	GError* error = NULL;

	key = g_strdup_printf ("%s/basename", config_dir);
	test_action->basenames = gconf_client_get_list (gconf_client, key, GCONF_VALUE_STRING, &error);
	if (error == NULL)
	{
		basename_ok = TRUE;
	}
	g_clear_error (&error);
	g_free (key);
	
	key = g_strdup_printf ("%s/isfile", config_dir);
	test_action->isfile = gconf_client_get_bool (gconf_client, key, &error);
	if (error == NULL)
	{
		isfile_ok = TRUE;
	}
	g_clear_error (&error);
	g_free (key);
	
	key = g_strdup_printf ("%s/isdir", config_dir);
	test_action->isdir = gconf_client_get_bool (gconf_client, key, &error);
	if (error == NULL)
	{
		isdir_ok = TRUE;
	}
	g_clear_error (&error);
	g_free (key);

	/* Check data integrity */
	if (test_action->isfile == FALSE && test_action->isdir == FALSE)
	{
		/* Can't be both False */
		isfile_ok = FALSE;
		isdir_ok = FALSE;
	}

	key = g_strdup_printf ("%s/accept-multiple-files", config_dir);
	test_action->accept_multiple_file = gconf_client_get_bool (gconf_client, key, &error);
	if (error == NULL)
	{
		accept_multiple_file_ok = TRUE;
	}
	g_clear_error (&error);
	g_free (key);

	key = g_strdup_printf ("%s/scheme", config_dir);
	test_action->schemes = gconf_client_get_list (gconf_client, key, GCONF_VALUE_STRING, &error);
	if (error == NULL)
	{
		scheme_ok = TRUE;
	}
	g_clear_error (&error);
	g_free (key);	
	
	if (basename_ok && isfile_ok && isdir_ok && accept_multiple_file_ok && scheme_ok)
	{
		retv = TRUE;
	}

	return retv;
}

static gboolean nautilus_actions_config_action_fill_command (GConfClient* gconf_client, const gchar* config_dir, ConfigAction* action)
{
	gboolean retv = FALSE;
	gboolean path_ok = FALSE;
	gboolean parameters_ok = FALSE;
	ConfigActionCommand* command_action = action->command;
	gchar* key;
	GError* error = NULL;

	key = g_strdup_printf ("%s/path", config_dir);
	command_action->path = gconf_client_get_string (gconf_client, key, &error);
	if (error == NULL)
	{
		path_ok = TRUE;
	}
	g_clear_error (&error);
	g_free (key);

	key = g_strdup_printf ("%s/parameters", config_dir);
	command_action->parameters = gconf_client_get_string (gconf_client, key, &error);
	if (error == NULL)
	{
		parameters_ok = TRUE;
	}
	g_clear_error (&error);
	g_free (key);

	if (path_ok && parameters_ok)
	{
		retv = TRUE;
	}

	return retv;
}

static gboolean nautilus_actions_config_action_fill_menu_item (GConfClient* gconf_client, const gchar* config_dir, ConfigAction* action)
{
	gboolean retv = FALSE;
	gboolean label_ok = FALSE;
	gboolean tooltip_ok = FALSE;
	ConfigActionMenuItem* menu_item_action = action->menu_item;
	gchar* key;
	GError* error = NULL;

	key = g_strdup_printf ("%s/label", config_dir);
	menu_item_action->label = gconf_client_get_string (gconf_client, key, &error);
	if (error == NULL)
	{
		label_ok = TRUE;
	}
	g_clear_error (&error);
	g_free (key);

	key = g_strdup_printf ("%s/tooltip", config_dir);
	menu_item_action->tooltip = gconf_client_get_string (gconf_client, key, &error);
	if (error == NULL)
	{
		tooltip_ok = TRUE;
	}
	g_clear_error (&error);
	g_free (key);

	if (label_ok && tooltip_ok)
	{
		retv = TRUE;
	}

	return retv;
}

static gboolean nautilus_actions_config_action_fill (GConfClient* gconf_client, const gchar* config_dir, ConfigAction* action)
{
	gboolean retv = FALSE;
	gboolean test_ok = FALSE;
	gboolean command_ok = FALSE;
	gboolean menu_item_ok = FALSE;
	gchar* path;
	
	path = g_strdup_printf ("%s/test", config_dir);
	test_ok = nautilus_actions_config_action_fill_test (gconf_client, path, action);
	g_free (path);

	path = g_strdup_printf ("%s/command", config_dir);
	command_ok = nautilus_actions_config_action_fill_command (gconf_client, path, action);
	g_free (path);

	path = g_strdup_printf ("%s/menu-item", config_dir);
	menu_item_ok = nautilus_actions_config_action_fill_menu_item (gconf_client, path, action);
	g_free (path);
	
	if (test_ok && command_ok && menu_item_ok)
	{
		retv = TRUE;
	}

	return retv;
}

static GList *nautilus_actions_config_get_config (GConfClient* gconf_client, const gchar* config_dir, GList* config_actions)
{
	ConfigAction *action;
	gchar* config_name;
	gchar* config_version;
	gchar* key;

	config_name = g_path_get_basename (config_dir);
	key = g_strdup_printf ("%s/version", config_dir);
	config_version = gconf_client_get_string (gconf_client, key, NULL);
	g_free (key);
			
	if (config_version != NULL)
	{
		if (g_list_find_custom (config_actions, config_name, (GCompareFunc)nautilus_actions_utils_compare_actions) == NULL)
		{
			action = nautilus_actions_config_action_new (config_name, config_version);
			if (nautilus_actions_config_action_fill (gconf_client, config_dir, action))
			{
				config_actions = g_list_append (config_actions, action);
			}
			else
			{
				nautilus_actions_config_free_action (action);
			}
		}
	}

	g_free (config_name);
	g_free (config_version);

	return config_actions;
}

GList *nautilus_actions_config_get_list (GConfClient* gconf_client, const gchar* config_root_dir)
{
	GList* config_actions = NULL;
	GSList* config_entries = NULL;
	GSList* iter;

	config_entries = gconf_client_all_dirs (gconf_client, config_root_dir, NULL);

	for (iter = config_entries; iter; iter = iter->next)
	{
		gchar* config_dir = (gchar*)iter->data;
		config_actions = nautilus_actions_config_get_config (gconf_client, config_dir, config_actions);
	}

	nautilus_actions_config_free_config_entries (config_entries);

	return config_actions;
}

void nautilus_actions_config_action_update_test_basenames (ConfigAction* action, GSList* new_basenames)
{
	GSList* iter;

	for (iter = action->test->basenames; iter; iter = iter->next)
	{
		g_free ((gchar*)iter->data);
	}

	g_slist_free (action->test->basenames);
	action->test->basenames = NULL;

	for (iter = new_basenames; iter; iter = iter->next)
	{
		action->test->basenames = g_slist_append (action->test->basenames, g_strdup ((gchar*)iter->data));
	}
}

void nautilus_actions_config_action_update_test_isfile (ConfigAction* action, gboolean new_isfile)
{
	action->test->isfile = new_isfile;
}

void nautilus_actions_config_action_update_test_accept_multiple_files (ConfigAction* action, gboolean new_accept_multiple_files)
{
	action->test->accept_multiple_file = new_accept_multiple_files;
}

void nautilus_actions_config_action_update_test_isdir (ConfigAction* action, gboolean new_isdir)
{
	action->test->isdir = new_isdir;
}

void nautilus_actions_config_action_update_test_schemes (ConfigAction* action, GSList* new_schemes)
{
	GSList* iter;

	for (iter = action->test->schemes; iter; iter = iter->next)
	{
		g_free ((gchar*)iter->data);
	}

	g_slist_free (action->test->schemes);
	action->test->schemes = NULL;

	for (iter = new_schemes; iter; iter = iter->next)
	{
		action->test->schemes = g_slist_append (action->test->schemes, g_strdup ((gchar*)iter->data));
	}
}

void nautilus_actions_config_action_update_command_parameters (ConfigAction* action, const gchar* new_parameters)
{
	g_free (action->command->parameters);
	action->command->parameters = g_strdup (new_parameters);
}

void nautilus_actions_config_action_update_command_path (ConfigAction* action, const gchar* new_path)
{
	g_free (action->command->path);
	action->command->path = g_strdup (new_path);
}

void nautilus_actions_config_action_update_menu_item_label (ConfigAction* action, const gchar* new_label)
{
	g_free (action->menu_item->label);
	action->menu_item->label = g_strdup (new_label);
}

void nautilus_actions_config_action_update_menu_item_tooltip (ConfigAction* action, const gchar* new_tooltip)
{
	g_free (action->menu_item->tooltip);
	action->menu_item->tooltip = g_strdup (new_tooltip);
}

ConfigAction *nautilus_actions_config_action_dup (ConfigAction* action)
{
	ConfigAction* new_action = NULL;
	GSList* iter;

	if (action != NULL)
	{
		new_action = nautilus_actions_config_action_new (action->name, action->version);
	

		if (action->test != NULL)
		{
			if (action->test->basenames != NULL)
			{
				for (iter = action->test->basenames; iter; iter = iter->next)
				{
					new_action->test->basenames = g_slist_append (new_action->test->basenames, g_strdup ((gchar*)iter->data));
				}
			}
			new_action->test->isfile = action->test->isfile;
			new_action->test->isdir = action->test->isdir;
			new_action->test->accept_multiple_file = action->test->accept_multiple_file;
			if (action->test->schemes != NULL)
			{
				for (iter = action->test->schemes; iter; iter = iter->next)
				{
					new_action->test->schemes = g_slist_append (new_action->test->schemes, g_strdup ((gchar*)iter->data));
				}
			}
		}
		
		if (action->command != NULL)
		{
			new_action->command->path = g_strdup (action->command->path);
			new_action->command->parameters = g_strdup (action->command->parameters);
		}
	
		if (action->menu_item != NULL)
		{	
			new_action->menu_item->label = g_strdup (action->menu_item->label);
			new_action->menu_item->tooltip = g_strdup (action->menu_item->tooltip);
		}
	}

	return new_action;
}

void nautilus_actions_config_free_list (GList* config_actions)
{
	GList* iter;
	if (config_actions != NULL)
	{
		for (iter = config_actions; iter; iter = iter->next)
		{
			nautilus_actions_config_free_action ((ConfigAction*)iter->data);
		}
		g_list_free (config_actions);
		config_actions = NULL;
	}
}

void nautilus_actions_config_free_action (ConfigAction* action)
{
	GSList* iter;
	
	if (action != NULL)
	{
		if (action->menu_item != NULL)
		{
			if (action->menu_item->tooltip != NULL)
			{
				g_free (action->menu_item->tooltip);
				action->menu_item->tooltip = NULL;
			}
			if (action->menu_item->label != NULL)
			{
				g_free (action->menu_item->label);
				action->menu_item->label = NULL;
			}
			g_free (action->menu_item);
			action->menu_item = NULL;
		}

		if (action->command != NULL)
		{
			if (action->command->parameters != NULL)
			{
				g_free (action->command->parameters);
				action->command->parameters = NULL;
			}
			if (action->command->path != NULL)
			{
				g_free (action->command->path);
				action->command->path = NULL;
			}
			g_free (action->command);
			action->command = NULL;
		}

		if (action->test != NULL)
		{
			if (action->test->schemes != NULL)
			{
				for (iter = action->test->schemes; iter; iter = iter->next)
				{
					g_free ((gchar*)iter->data);
				}
				g_slist_free (action->test->schemes);
				action->test->schemes = NULL;
			}
			if (action->test->basenames != NULL)
			{
				for (iter = action->test->basenames; iter; iter = iter->next)
				{
					g_free ((gchar*)iter->data);
				}
				g_slist_free (action->test->basenames);
				action->test->basenames = NULL;
			}
			g_free (action->test);
			action->test = NULL;
		}

		if (action->name != NULL)
		{
			g_free (action->name);
			action->name = NULL;
		}
		if (action->version != NULL)
		{
			g_free (action->version);
			action->version = NULL;
		}
		g_free (action);
		action = NULL;
	}
}

// vim:ts=3:sw=3:tw=1024
