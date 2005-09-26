#include <config.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "nautilus-actions-utils.h"
#include "nautilus-actions-config.h"

static GList *nautilus_actions_config_get_config_files (void)
{
	GList* config_files = NULL;
	GDir* config_dir = NULL;
	gchar* filename;
	gchar* path;
	gchar* per_user_dir = g_build_path ("/", g_get_home_dir (), DEFAULT_PER_USER_PATH, NULL);

	/* First get the per user files so they have priority because there are first parsed */

	if (g_file_test (per_user_dir, G_FILE_TEST_IS_DIR))
	{
		config_dir = g_dir_open (per_user_dir, 0, NULL);
		if (config_dir != NULL)
		{
			filename = g_dir_read_name (config_dir);
			while (filename != NULL)
			{
				path = g_build_path ("/", per_user_dir, filename, NULL);
				if (g_file_test (path, G_FILE_TEST_IS_REGULAR))
				{
					// This is a regular file, we can add it 
					config_files = g_list_append (config_files, g_strdup (path));
				}
				g_free (path);
				filename = g_dir_read_name (config_dir);
			}

			g_dir_close (config_dir);
		}
	}
	
	g_free (per_user_dir);

	/* Then get system-wide config files, if there are duplicate of above, they will be skipped during parsing */
	if (g_file_test (DEFAULT_CONFIG_PATH, G_FILE_TEST_IS_DIR))
	{
		config_dir = g_dir_open (DEFAULT_CONFIG_PATH, 0, NULL);
		if (config_dir != NULL)
		{
			filename = g_dir_read_name (config_dir);
			while (filename != NULL)
			{
				path = g_build_path ("/", DEFAULT_CONFIG_PATH, filename, NULL);
				if (g_file_test (path, G_FILE_TEST_IS_REGULAR))
				{
					/* This is a regular file, we can add it */
					config_files = g_list_append (config_files, g_strdup (path));
				}
				g_free (path);
				filename = g_dir_read_name (config_dir);
			}

			g_dir_close (config_dir);
		}
	}

	return config_files;
}

static void nautilus_actions_config_free_config_files (GList* config_files)
{
	GList* iter;
	
	for (iter = config_files; iter; iter = iter->next)
	{
		g_free ((gchar*)iter->data);
	}

	g_list_free (config_files);
	config_files = NULL;
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

static gboolean nautilus_actions_config_action_fill_test_basenames (GList** test_basenames, xmlNode* config_test_basename_node, const gchar* config_version)
{
	xmlNode *iter;
	gboolean retv = FALSE;

	if (g_ascii_strncasecmp (config_version, "0.1", strlen (config_version)) == 0)
	{
		//--> manage backward compatibility
		xmlChar* text = xmlNodeGetContent (config_test_basename_node);
		(*test_basenames) = g_list_append ((*test_basenames), xmlStrdup (text));
		xmlFree (text);
		retv = TRUE;
	}
	else
	{
		for (iter = config_test_basename_node->children; iter; iter = iter->next)
		{
			xmlChar* text;
			
			if (iter->type == XML_ELEMENT_NODE &&
					g_ascii_strncasecmp (iter->name, 
									"match",
									strlen ("match")) == 0)
			{
				text = xmlNodeGetContent (iter);
				(*test_basenames) = g_list_append ((*test_basenames), xmlStrdup (text));
				xmlFree (text);
				retv = TRUE;
			}
		}
	}

	return retv;
}

static gboolean nautilus_actions_config_action_fill_test_scheme (GList** test_scheme, xmlNode* config_test_scheme_node)
{
	xmlNode *iter;
	gboolean retv = FALSE;

	for (iter = config_test_scheme_node->children; iter; iter = iter->next)
	{
		xmlChar* text;
		
		if (iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp (iter->name, 
								"type",
								strlen ("type")) == 0)
		{
			text = xmlNodeGetContent (iter);
			(*test_scheme) = g_list_append ((*test_scheme), xmlStrdup (text));
			xmlFree (text);
			retv = TRUE;
		}
	}

	return retv;
}

static gboolean nautilus_actions_config_action_fill_test (ConfigAction *action, xmlNode* config_test_node)
{
	xmlNode *iter;
	gboolean retv = FALSE;
	gboolean basename_ok = FALSE;
	gboolean isfile_ok = FALSE;
	gboolean isdir_ok = FALSE;
	gboolean scheme_ok = FALSE;
	gboolean accept_multiple_file_ok = FALSE;
	ConfigActionTest* test_action = action->test;


	for (iter = config_test_node->children; iter; iter = iter->next)
	{
		xmlChar* text;
		
		if (!basename_ok && iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp ((gchar*)iter->name, 
								"basename",
								strlen ("basename")) == 0)
		{
			basename_ok = nautilus_actions_config_action_fill_test_basenames (&(test_action->basenames), iter, action->version);
		}
		else if (!isfile_ok && iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp ((gchar*)iter->name, 
								"isfile",
								strlen ("isfile")) == 0)
		{
			text = xmlNodeGetContent (iter);
			isfile_ok = nautilus_actions_utils_parse_boolean (text, &test_action->isfile);
			xmlFree (text);
		}
		else if (!isdir_ok && iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp ((gchar*)iter->name, 
								"isdir",
								strlen ("isdir")) == 0)
		{
			text = xmlNodeGetContent (iter);
			isdir_ok = nautilus_actions_utils_parse_boolean (text, &test_action->isdir);
			xmlFree (text);
		}
		else if (!accept_multiple_file_ok && iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp (iter->name, 
								"accept-multiple-files",
								strlen ("accept-multiple-files")) == 0)
		{
			text = xmlNodeGetContent (iter);
			accept_multiple_file_ok = nautilus_actions_utils_parse_boolean (text, &test_action->accept_multiple_file);
			xmlFree (text);
		}
		else if (!accept_multiple_file_ok && iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp (iter->name, 
								"accept-multiple-files",
								strlen ("accept-multiple-files")) == 0)
		{
			text = xmlNodeGetContent (iter);
			accept_multiple_file_ok = nautilus_actions_utils_parse_boolean (text, &test_action->accept_multiple_file);
			xmlFree (text);
		}
		else if (!scheme_ok && iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp (iter->name, 
								"scheme",
								strlen ("scheme")) == 0)
		{
			scheme_ok = nautilus_actions_config_action_fill_test_scheme (&(test_action->schemes), iter);
		}
	}
	
	
	if (basename_ok && isfile_ok && isdir_ok && accept_multiple_file_ok && scheme_ok)
	{
		retv = TRUE;
	}

	return retv;
}

static gboolean nautilus_actions_config_action_fill_command (ConfigAction *action, xmlNode* config_command_node)
{
	xmlNode *iter;
	gboolean retv = FALSE;
	gboolean path_ok = FALSE;
	gboolean parameters_ok = FALSE;
	ConfigActionCommand* command_action = action->command;

	for (iter = config_command_node->children; iter; iter = iter->next)
	{
		xmlChar* text;
		
		if (!path_ok && iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp (iter->name, 
								"path",
								strlen ("path")) == 0)
		{
			text = xmlNodeGetContent (iter);
			command_action->path = xmlStrdup (text);
			xmlFree (text);
			path_ok = TRUE;
		}
		else if (!parameters_ok && iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp (iter->name, 
								"parameters",
								strlen ("parameters")) == 0)
		{
			text = xmlNodeGetContent (iter);
			command_action->parameters = xmlStrdup (text);
			xmlFree (text);
			parameters_ok = TRUE;
		}
	}

	if (path_ok && parameters_ok)
	{
		retv = TRUE;
	}

	return retv;
}

static gboolean nautilus_actions_config_action_fill_menu_item (ConfigAction *action, xmlNode* config_menu_item_node)
{
	xmlNode *iter;
	gboolean retv = FALSE;
	gboolean label_ok = FALSE;
	gboolean label_lang_ok = FALSE;
	gboolean tooltip_ok = FALSE;
	gboolean tooltip_lang_ok = FALSE;
	ConfigActionMenuItem* menu_item_action = action->menu_item;
	gchar* lang = g_strdup (g_getenv ("LANG"));
	xmlChar* xmlLang;

	for (iter = config_menu_item_node->children; iter; iter = iter->next)
	{
		xmlChar* text;
		
		if (iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp (iter->name, 
								"label",
								strlen ("label")) == 0)
		{
			xmlLang = xmlGetProp (iter, "lang");
			text = xmlNodeGetContent (iter);
			if (lang == NULL && xmlLang == NULL)
			{
				//--> No $LANG set, get the default one (no xml:lang)
				menu_item_action->label = xmlStrdup (text);
				label_ok = TRUE;
				label_lang_ok = TRUE;
			}
			else if (lang != NULL && xmlLang == NULL)
			{
				if (!label_lang_ok)
				{
					//--> $LANG set, not found the good xml:lang yet, get the default one (no xml:lang)
					menu_item_action->label = xmlStrdup (text);
					label_ok = TRUE;
				}
			}
			else if (lang != NULL && (xmlLang != NULL && g_ascii_strncasecmp (xmlLang, lang, strlen (xmlLang)) == 0))
			{ 
				//--> $LANG set, found the good xml:lang, free the default one if any and set the good one instead
				if (menu_item_action->label != NULL)
				{
					g_free (menu_item_action->label);
				}
				menu_item_action->label = xmlStrdup (text);
				label_ok = TRUE;
				label_lang_ok = TRUE;
			}
			xmlFree (text);
			xmlFree (xmlLang);
		}
		else if (iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp (iter->name, 
								"tooltip",
								strlen ("tooltip")) == 0)
		{
			xmlLang = xmlGetProp (iter, "lang");
			text = xmlNodeGetContent (iter);
			if (lang == NULL && xmlLang == NULL)
			{
				//--> No $LANG set, get the default one (no xml:lang)
				menu_item_action->tooltip = xmlStrdup (text);
				tooltip_ok = TRUE;
				tooltip_lang_ok = TRUE;
			}
			else if (lang != NULL && xmlLang == NULL)
			{
				if (!tooltip_lang_ok)
				{
					//--> $LANG set, not found the good xml:lang yet, get the default one (no xml:lang)
					menu_item_action->tooltip = xmlStrdup (text);
					tooltip_ok = TRUE;
				}
			}
			else if (lang != NULL && (xmlLang != NULL && g_ascii_strncasecmp (xmlLang, lang, strlen (xmlLang)) == 0))
			{ 
				//--> $LANG set, found the good xml:lang, free the default one if any and set the good one instead
				if (menu_item_action->tooltip != NULL)
				{
					g_free (menu_item_action->tooltip);
				}
				menu_item_action->tooltip = xmlStrdup (text);
				tooltip_ok = TRUE;
				tooltip_lang_ok = TRUE;
			}
			xmlFree (text);
			xmlFree (xmlLang);
		}
	}

	if (label_ok && tooltip_ok)
	{
		retv = TRUE;
	}

	g_free (lang);

	return retv;
}

static gboolean nautilus_actions_config_action_fill (ConfigAction* action, xmlNode* config_node)
{
	xmlNode *iter;
	gboolean retv = FALSE;
	gboolean test_ok = FALSE;
	gboolean command_ok = FALSE;
	gboolean menu_item_ok = FALSE;
	
	for (iter = config_node->children; iter; iter = iter->next)
	{
		if (!test_ok && iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp (iter->name, 
											"test",
											strlen ("test")) == 0)
		{
			test_ok = nautilus_actions_config_action_fill_test (action, iter);
		}
		else if (!command_ok && iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp (iter->name, 
											"command",
											strlen ("command")) == 0)
		{
			command_ok = nautilus_actions_config_action_fill_command (action, iter);
		}
		else if (!menu_item_ok && iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp (iter->name, 
											"menu-item",
											strlen ("menu-item")) == 0)
		{
			menu_item_ok = nautilus_actions_config_action_fill_menu_item (action, iter);
		}
	}

	if (test_ok && command_ok && menu_item_ok)
	{
		retv = TRUE;
	}

	return retv;
}

/*

GList *nautilus_actions_config_get_fake_list (char* filename, int level)
{
	GList* config_actions = NULL;
	GList* config_files = NULL;
	ConfigAction* action;
	GList* iter;
	gchar* base = g_path_get_basename (filename);

	action = nautilus_actions_config_action_new (base, 0.1);
	g_free (base);
	action->test->basename = g_strdup ("*");
	action->test->isfile = TRUE;
	action->test->isdir = TRUE;
	action->test->accept_multiple_file = TRUE;
	action->test->schemes = g_list_append (action->test->schemes, g_strdup ("sftp"));
	action->test->schemes = g_list_append (action->test->schemes, g_strdup ("file"));
	action->command->path = g_strdup ("/home/ruaudel/Projects/Glade/displayparam/displayparam.py");
	action->command->parameters = g_strdup ("-uri=%u -dir=%d -parent=%p -file=%f -scheme=%s -host=%h -user=%U -foo=100%% %m");
	action->menu_item->label = g_strdup ("Fake Action");
	action->menu_item->tooltip = g_strdup_printf ("Fake action cause %s(%d) failed to parse", filename, level);
	config_actions = g_list_append (config_actions, action);

	return config_actions;
}
*/

static gint nautilus_actions_compare_actions (const ConfigAction* action1, const gchar* action_name)
{
	return g_ascii_strcasecmp (action1->name, action_name);
}

static GList *nautilus_actions_config_parse_file (const gchar* filename, GList* config_actions)
{
	xmlDoc *doc = NULL;
	xmlNode *root_node;
	xmlNode *iter;
	xmlChar* version;
	ConfigAction *action;

	doc = xmlParseFile (filename);
	if (doc != NULL)
	{
		root_node = xmlDocGetRootElement (doc);
		if (g_ascii_strncasecmp (root_node->name, 
										"nautilus-actions-config", 
										strlen ("nautilus-actions-config")) == 0)
		{
			version = xmlGetProp (root_node, "version");
			
			for (iter = root_node->children; iter; iter = iter->next)
			{
				xmlChar *config_name;

				if (iter->type == XML_ELEMENT_NODE &&
					g_ascii_strncasecmp (iter->name, 
												"action", 
												strlen ("action")) == 0)
				{
					config_name = xmlGetProp (iter, "name");
					if (config_name != NULL)
					{
						if (g_list_find_custom (config_actions, config_name, nautilus_actions_compare_actions) == NULL)
						{
							// We skip this file because per user config have priority and there are in the beginning of the file list
							action = nautilus_actions_config_action_new ((gchar*)config_name, version);
							if (nautilus_actions_config_action_fill (action, iter))
							{
								config_actions = g_list_append (config_actions, action);
							}
							else
							{
								nautilus_actions_config_free_action (action);
							}
						}
						xmlFree (config_name);
					}
				}
			}
			xmlFree (version);
		}

		xmlFreeDoc(doc);
	}

	xmlCleanupParser();

	return config_actions;
}

/*
GList *nautilus_actions_config_get_list (void)
{
	GList* config_actions = NULL;
	GList* config_files = NULL;
	ConfigAction* action;
	GList* iter;

	action = nautilus_actions_config_action_new ("test", 0.1);
	action->test->basename = g_strdup ("**");
	action->test->isfile = TRUE;
	action->test->isdir = TRUE;
	action->test->accept_multiple_file = TRUE;
	action->test->schemes = g_list_append (action->test->schemes, g_strdup ("sftp"));
	action->test->schemes = g_list_append (action->test->schemes, g_strdup ("file"));
	action->command->path = g_strdup ("/home/ruaudel/Projects/Glade/displayparam/displayparam.py");
	action->command->parameters = g_strdup ("-uri=%u -dir=%d -parent=%p -file=%f -scheme=%s -host=%h -user=%U -foo=100%% %m");
	action->menu_item->label = g_strdup ("Test Action");
	action->menu_item->tooltip = g_strdup ("Test Action la chiotte a debugger");
	config_actions = g_list_append (config_actions, action);
	action = nautilus_actions_config_action_new ("test2");
	action->test->basename = g_strdup ("*");
	action->test->isfile = TRUE;
	action->test->isdir = TRUE;
	action->test->accept_multiple_file = TRUE;
	action->test->schemes = g_list_append (action->test->schemes, g_strdup ("sftp"));
	action->test->schemes = g_list_append (action->test->schemes, g_strdup ("file"));
	action->command->path = g_strdup ("/home/ruaudel/Projects/Glade/displayparam/displayparam.py");
	action->command->parameters = g_strdup ("-uri=%u -dir=%d -parent=%p -file=%f -scheme=%s -host=%h -user=%U -foo=100%% %m");
	action->menu_item->label = g_strdup ("Test Action2");
	action->menu_item->tooltip = g_strdup ("Test Action2 la chiotte a debugger");
	config_actions = g_list_append (config_actions, action);

	return config_actions;
}

*/
GList *nautilus_actions_config_get_list (void)
{
	GList* config_actions = NULL;
	GList* config_files = NULL;
	GList* iter;

	config_files = nautilus_actions_config_get_config_files ();

	for (iter = config_files; iter; iter = iter->next)
	{
		gchar* filename = (gchar*)iter->data;
		//config_actions = g_list_concat (config_actions, nautilus_actions_config_parse_file (filename));
		config_actions = nautilus_actions_config_parse_file (filename, config_actions);
	}

	nautilus_actions_config_free_config_files (config_files);

	return config_actions;
}

ConfigAction *nautilus_actions_config_action_dup (ConfigAction* action)
{
	ConfigAction* new_action = NULL;
	GList* iter;

	if (action != NULL)
	{
		new_action = nautilus_actions_config_action_new (action->name, action->version);
	

		if (action->test != NULL)
		{
			if (action->test->basenames != NULL)
			{
				for (iter = action->test->basenames; iter; iter = iter->next)
				{
					new_action->test->basenames = g_list_append (new_action->test->basenames, g_strdup ((gchar*)iter->data));
				}
			}
			new_action->test->isfile = action->test->isfile;
			new_action->test->isdir = action->test->isdir;
			new_action->test->accept_multiple_file = action->test->accept_multiple_file;
			if (action->test->schemes != NULL)
			{
				for (iter = action->test->schemes; iter; iter = iter->next)
				{
					new_action->test->schemes = g_list_append (new_action->test->schemes, g_strdup ((gchar*)iter->data));
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
	GList* iter;
	
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
				g_list_free (action->test->schemes);
				action->test->schemes = NULL;
			}
			if (action->test->basenames != NULL)
			{
				for (iter = action->test->basenames; iter; iter = iter->next)
				{
					g_free ((gchar*)iter->data);
				}
				g_list_free (action->test->basenames);
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
