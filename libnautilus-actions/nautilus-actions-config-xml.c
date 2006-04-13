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
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <uuid/uuid.h>
#include "nautilus-actions-config-xml.h"
#include "nautilus-actions-config.h"

#define ACTIONS_CONFIG_DIR					DEFAULT_CONFIG_PATH
#define ACTIONS_PER_USER_CONFIG_DIR		".config/nautilus-actions"
#define ACTION_ROOT							"nautilus-actions-config"
#define ACTION_VERSION						"version"
#define ACTION_ACTION						"action"
#define ACTION_ACTION_NAME					"name"
#define ACTION_MENU_ITEM					"menu-item"
#define ACTION_LANG							"lang"
#define ACTION_LABEL							"label"
#define ACTION_TOOLTIP						"tooltip"
#define ACTION_COMMAND						"command"
#define ACTION_PATH							"path"
#define ACTION_PARAMS						"parameters"
#define ACTION_TEST							"test"
#define ACTION_BASENAMES					"basename"
#define ACTION_BASENAMES_MATCH			"match"
#define ACTION_ISFILE						"isfile"
#define ACTION_ISDIR							"isdir"
#define ACTION_MULTIPLE						"accept-multiple-files"
#define ACTION_SCHEMES						"scheme"
#define ACTION_SCHEMES_TYPE				"type"

static GObjectClass *parent_class = NULL;

static gboolean
save_action (NautilusActionsConfig *self, NautilusActionsConfigAction *action)
{
	return TRUE;
/*
	g_return_val_if_fail (NAUTILUS_ACTIONS_IS_CONFIG_XML (self), NULL);

	NautilusActionsConfigXml* config = NAUTILUS_ACTIONS_CONFIG_XML (self);
	gchar *key;

	// set the version on the action 
	if (action->version)
		g_free (action->version);
	action->version = g_strdup (NAUTILUS_ACTIONS_CONFIG_VERSION);
*/
}

static gboolean
remove_action (NautilusActionsConfig *self, NautilusActionsConfigAction* action)
{
	g_return_val_if_fail (NAUTILUS_ACTIONS_IS_CONFIG_XML (self), FALSE);

	//NautilusActionsConfigXml* config = NAUTILUS_ACTIONS_CONFIG_XML (self);

	return TRUE;
}

static GList *nautilus_actions_config_xml_get_config_files (void)
{
	GList* config_files = NULL;
	GDir* config_dir = NULL;
	const gchar* filename;
	gchar* path;
	gchar* per_user_dir = g_build_path ("/", g_get_home_dir (), ACTIONS_PER_USER_CONFIG_DIR, NULL);

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
	if (g_file_test (ACTIONS_CONFIG_DIR, G_FILE_TEST_IS_DIR))
	{
		config_dir = g_dir_open (ACTIONS_CONFIG_DIR, 0, NULL);
		if (config_dir != NULL)
		{
			filename = g_dir_read_name (config_dir);
			while (filename != NULL)
			{
				path = g_build_path ("/", ACTIONS_CONFIG_DIR, filename, NULL);
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

static void
nautilus_actions_config_xml_free_config_files (GList* config_files)
{
	g_list_foreach (config_files, (GFunc) g_free, NULL);
	g_list_free (config_files);
	config_files = NULL;
}

static gboolean
nautilus_actions_config_xml_action_fill_test_basenames (GSList** test_basenames, xmlNode* config_test_basename_node, const gchar* config_version)
{
	xmlNode *iter;
	gboolean retv = FALSE;

	if (g_ascii_strncasecmp (config_version, "0.1", strlen (config_version)) == 0)
	{
		//--> manage backward compatibility
		xmlChar* text = xmlNodeGetContent (config_test_basename_node);
		(*test_basenames) = g_slist_append ((*test_basenames), xmlStrdup (text));
		xmlFree (text);
		retv = TRUE;
	}
	else
	{
		for (iter = config_test_basename_node->children; iter; iter = iter->next)
		{
			xmlChar* text;
			
			if (iter->type == XML_ELEMENT_NODE &&
					g_ascii_strncasecmp ((const gchar *) iter->name, 
							     ACTION_BASENAMES_MATCH,
							     strlen (ACTION_BASENAMES_MATCH)) == 0)
			{
				text = xmlNodeGetContent (iter);
				(*test_basenames) = g_slist_append ((*test_basenames), xmlStrdup (text));
				xmlFree (text);
				retv = TRUE;
			}
		}
	}

	return retv;
}

static gboolean
nautilus_actions_config_xml_action_fill_test_scheme (GSList** test_scheme, xmlNode* config_test_scheme_node)
{
	xmlNode *iter;
	gboolean retv = FALSE;

	for (iter = config_test_scheme_node->children; iter; iter = iter->next)
	{
		xmlChar* text;
		
		if (iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp ((const gchar *) iter->name, 
						     ACTION_SCHEMES_TYPE,
						     strlen (ACTION_SCHEMES_TYPE)) == 0)
		{
			text = xmlNodeGetContent (iter);
			(*test_scheme) = g_slist_append ((*test_scheme), xmlStrdup (text));
			xmlFree (text);
			retv = TRUE;
		}
	}

	return retv;
}

static gboolean nautilus_actions_config_xml_parse_boolean (const gchar* value2parse, gboolean *value2set)
{
	gboolean retv = FALSE;

	if (g_ascii_strncasecmp (value2parse, "true", strlen ("true")) == 0)
	{
		(*value2set) = TRUE;
		retv = TRUE;
	}
	else if (g_ascii_strncasecmp (value2parse, "false", strlen ("false")) == 0)
	{
		(*value2set) = FALSE;
		retv = TRUE;
	}

	return retv;
}

static gboolean nautilus_actions_config_xml_action_fill_test (NautilusActionsConfigAction *action, xmlNode* config_test_node)
{
	xmlNode *iter;
	gboolean retv = FALSE;
	gboolean basename_ok = FALSE;
	gboolean isfile_ok = FALSE;
	gboolean isdir_ok = FALSE;
	gboolean scheme_ok = FALSE;
	gboolean accept_multiple_file_ok = FALSE;

	for (iter = config_test_node->children; iter; iter = iter->next)
	{
		xmlChar* text;
		
		if (!basename_ok && iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp ((gchar*)iter->name, 
								ACTION_BASENAMES,
								strlen (ACTION_BASENAMES)) == 0)
		{
			basename_ok = nautilus_actions_config_xml_action_fill_test_basenames (&(action->basenames), iter, action->version);
		}
		else if (!isfile_ok && iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp ((gchar*)iter->name, 
								ACTION_ISFILE,
								strlen (ACTION_ISFILE)) == 0)
		{
			text = xmlNodeGetContent (iter);
			isfile_ok = nautilus_actions_config_xml_parse_boolean ((char*)text, &(action->is_file));
			xmlFree (text);
		}
		else if (!isdir_ok && iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp ((gchar*)iter->name, 
								ACTION_ISDIR,
								strlen (ACTION_ISDIR)) == 0)
		{
			text = xmlNodeGetContent (iter);
			isdir_ok = nautilus_actions_config_xml_parse_boolean ((char*)text, &(action->is_dir));
			xmlFree (text);
		}
		else if (!accept_multiple_file_ok && iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp ((gchar*)iter->name, 
								ACTION_MULTIPLE,
								strlen (ACTION_MULTIPLE)) == 0)
		{
			text = xmlNodeGetContent (iter);
			accept_multiple_file_ok = nautilus_actions_config_xml_parse_boolean ((char*)text, &(action->accept_multiple_files));
			xmlFree (text);
		}
		else if (!scheme_ok && iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp ((gchar*)iter->name, 
								ACTION_SCHEMES,
								strlen (ACTION_SCHEMES)) == 0)
		{
			scheme_ok = nautilus_actions_config_xml_action_fill_test_scheme (&(action->schemes), iter);
		}
	}
	
	
	if (basename_ok && isfile_ok && isdir_ok && accept_multiple_file_ok && scheme_ok)
	{
		//--> manage backward compatibility
		action->match_case = TRUE;
		action->mimetypes = g_slist_append (action->mimetypes, g_strdup ("*/*"));
		retv = TRUE;
	}

	return retv;
}

static gboolean nautilus_actions_config_xml_action_fill_command (NautilusActionsConfigAction *action, xmlNode* config_command_node)
{
	xmlNode *iter;
	gboolean retv = FALSE;
	gboolean path_ok = FALSE;
	gboolean parameters_ok = FALSE;

	for (iter = config_command_node->children; iter; iter = iter->next)
	{
		xmlChar* text;
		
		if (!path_ok && iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp ((gchar*)iter->name, 
								ACTION_PATH,
								strlen (ACTION_PATH)) == 0)
		{
			text = xmlNodeGetContent (iter);
			action->path = (char*)xmlStrdup (text);
			xmlFree (text);
			path_ok = TRUE;
		}
		else if (!parameters_ok && iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp ((gchar*)iter->name, 
								ACTION_PARAMS,
								strlen (ACTION_PARAMS)) == 0)
		{
			text = xmlNodeGetContent (iter);
			action->parameters = (char*)xmlStrdup (text);
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

static gboolean nautilus_actions_config_xml_action_fill_menu_item (NautilusActionsConfigAction *action, xmlNode* config_menu_item_node)
{
	xmlNode *iter;
	gboolean retv = FALSE;
	gboolean label_ok = FALSE;
	gboolean label_lang_ok = FALSE;
	gboolean tooltip_ok = FALSE;
	gboolean tooltip_lang_ok = FALSE;
	gchar* lang = g_strdup (g_getenv ("LANG"));
	xmlChar* xmlLang;

	for (iter = config_menu_item_node->children; iter; iter = iter->next)
	{
		xmlChar* text;
		
		if (iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp ((gchar*)iter->name, 
								ACTION_LABEL,
								strlen (ACTION_LABEL)) == 0)
		{
			xmlLang = xmlGetProp (iter, BAD_CAST ACTION_LANG);
			text = xmlNodeGetContent (iter);
			if (lang == NULL && xmlLang == NULL)
			{
				//--> No $LANG set, get the default one (no xml:lang)
				action->label = (char*)xmlStrdup (text);
				label_ok = TRUE;
				label_lang_ok = TRUE;
			}
			else if (lang != NULL && xmlLang == NULL)
			{
				if (!label_lang_ok)
				{
					//--> $LANG set, not found the good xml:lang yet, get the default one (no xml:lang)
					action->label = (char*)xmlStrdup (text);
					label_ok = TRUE;
				}
			}
			else if (lang != NULL && (xmlLang != NULL && g_ascii_strncasecmp ((gchar*)xmlLang, lang, xmlStrlen (xmlLang)) == 0))
			{ 
				//--> $LANG set, found the good xml:lang, free the default one if any and set the good one instead
				if (action->label != NULL)
				{
					g_free (action->label);
				}
				action->label = (char*)xmlStrdup (text);
				label_ok = TRUE;
				label_lang_ok = TRUE;
			}
			xmlFree (text);
			xmlFree (xmlLang);
		}
		else if (iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp ((gchar*)iter->name, 
								ACTION_TOOLTIP,
								strlen (ACTION_TOOLTIP)) == 0)
		{
			xmlLang = xmlGetProp (iter, BAD_CAST ACTION_LANG);
			text = xmlNodeGetContent (iter);
			if (lang == NULL && xmlLang == NULL)
			{
				//--> No $LANG set, get the default one (no xml:lang)
				action->tooltip = (char*)xmlStrdup (text);
				tooltip_ok = TRUE;
				tooltip_lang_ok = TRUE;
			}
			else if (lang != NULL && xmlLang == NULL)
			{
				if (!tooltip_lang_ok)
				{
					//--> $LANG set, not found the good xml:lang yet, get the default one (no xml:lang)
					action->tooltip = (char*)xmlStrdup (text);
					tooltip_ok = TRUE;
				}
			}
			else if (lang != NULL && (xmlLang != NULL && g_ascii_strncasecmp ((gchar*)xmlLang, lang, xmlStrlen (xmlLang)) == 0))
			{ 
				//--> $LANG set, found the good xml:lang, free the default one if any and set the good one instead
				if (action->tooltip != NULL)
				{
					g_free (action->tooltip);
				}
				action->tooltip = (char*)xmlStrdup (text);
				tooltip_ok = TRUE;
				tooltip_lang_ok = TRUE;
			}
			xmlFree (text);
			xmlFree (xmlLang);
		}
	}

	if (label_ok && tooltip_ok)
	{
		action->icon = g_strdup ("");
		retv = TRUE;
	}

	g_free (lang);

	return retv;
}

static gboolean nautilus_actions_config_xml_action_fill (NautilusActionsConfigAction* action, xmlNode* config_node)
{
	xmlNode *iter;
	gboolean retv = FALSE;
	gboolean test_ok = FALSE;
	gboolean command_ok = FALSE;
	gboolean menu_item_ok = FALSE;
	
	for (iter = config_node->children; iter; iter = iter->next)
	{
		if (!test_ok && iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp ((gchar*)iter->name, 
											ACTION_TEST,
											strlen (ACTION_TEST)) == 0)
		{
			test_ok = nautilus_actions_config_xml_action_fill_test (action, iter);
		}
		else if (!command_ok && iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp ((gchar*)iter->name, 
											ACTION_COMMAND,
											strlen (ACTION_COMMAND)) == 0)
		{
			command_ok = nautilus_actions_config_xml_action_fill_command (action, iter);
		}
		else if (!menu_item_ok && iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp ((gchar*)iter->name, 
											ACTION_MENU_ITEM,
											strlen (ACTION_MENU_ITEM)) == 0)
		{
			menu_item_ok = nautilus_actions_config_xml_action_fill_menu_item (action, iter);
		}
	}

	//--> not used in old config files, so init to an empty string :
	action->conf_section = g_strdup ("");

	if (test_ok && command_ok && menu_item_ok)
	{
		retv = TRUE;
	}

	return retv;
}

gboolean
nautilus_actions_config_xml_parse_file (NautilusActionsConfigXml* config, const gchar* filename)
{
	xmlDoc *doc = NULL;
	xmlNode *root_node;
	xmlNode *iter;
	xmlChar* version;
	NautilusActionsConfigAction *action;
	uuid_t uuid;
	gchar uuid_str[64];
	gboolean retv = FALSE;

	doc = xmlParseFile (filename);
	if (doc != NULL)
	{
		root_node = xmlDocGetRootElement (doc);
		if (g_ascii_strncasecmp ((gchar*)root_node->name, 
										ACTION_ROOT, 
										strlen (ACTION_ROOT)) == 0)
		{
			version = xmlGetProp (root_node, BAD_CAST ACTION_VERSION);
			
			for (iter = root_node->children; iter; iter = iter->next)
			{
				xmlChar *config_name;

				if (iter->type == XML_ELEMENT_NODE &&
					g_ascii_strncasecmp ((gchar*)iter->name, 
												ACTION_ACTION, 
												strlen (ACTION_ACTION)) == 0)
				{
					config_name = xmlGetProp (iter, BAD_CAST ACTION_ACTION_NAME);
					if (config_name != NULL)
					{
						action = nautilus_actions_config_action_new ();
						action->version = (char*)xmlStrdup (version);
						uuid_generate (uuid);
						uuid_unparse (uuid, uuid_str);
						action->uuid = g_strdup (uuid_str);
						if (nautilus_actions_config_xml_action_fill (action, iter))
						{
							g_hash_table_insert (NAUTILUS_ACTIONS_CONFIG (config)->actions, g_strdup (action->uuid), action);
							retv = TRUE;
						}
						else
						{
							nautilus_actions_config_action_free (action);
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

	return retv;
}

void nautilus_actions_config_xml_load_list (NautilusActionsConfigXml* config)
{
	GList* config_files = NULL;
	GList* iter;

	config_files = nautilus_actions_config_xml_get_config_files ();

	for (iter = config_files; iter; iter = iter->next)
	{
		gchar* filename = (gchar*)iter->data;
		nautilus_actions_config_xml_parse_file (config, filename);
	}

	nautilus_actions_config_xml_free_config_files (config_files);
}

static void
nautilus_actions_config_xml_finalize (GObject *object)
{
	NautilusActionsConfig *config = (NautilusActionsConfig *) object;

	g_return_if_fail (NAUTILUS_ACTIONS_IS_CONFIG (config));

	/* chain call to parent class */
	if (parent_class->finalize)
	{
		parent_class->finalize (object);
	}
}

static void
nautilus_actions_config_xml_class_init (NautilusActionsConfigXmlClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = nautilus_actions_config_xml_finalize;
	
	NAUTILUS_ACTIONS_CONFIG_CLASS (klass)->save_action = save_action;
	NAUTILUS_ACTIONS_CONFIG_CLASS (klass)->remove_action = remove_action;
}

static void
nautilus_actions_config_xml_init (NautilusActionsConfig *config, NautilusActionsConfigClass *klass)
{
}

GType
nautilus_actions_config_xml_get_type (void)
{
   static GType type = 0;

	if (type == 0) {
		static GTypeInfo info = {
					sizeof (NautilusActionsConfigXmlClass),
					(GBaseInitFunc) NULL,
					(GBaseFinalizeFunc) NULL,
					(GClassInitFunc) nautilus_actions_config_xml_class_init,
					NULL, NULL,
					sizeof (NautilusActionsConfigXml),
					0,
					(GInstanceInitFunc) nautilus_actions_config_xml_init
		};
		type = g_type_register_static (NAUTILUS_ACTIONS_TYPE_CONFIG, "NautilusActionsConfigXml", &info, 0);
	}
	return type;
}

NautilusActionsConfigXml *
nautilus_actions_config_xml_get (void)
{
	static NautilusActionsConfigXml *config = NULL;

	/* we share one NautilusActionsConfigXml object for all */
	if (!config) {
		config = g_object_new (NAUTILUS_ACTIONS_TYPE_CONFIG_XML, NULL);
		return config;
	}

	return NAUTILUS_ACTIONS_CONFIG_XML (g_object_ref (G_OBJECT (config)));
}

// vim:ts=3:sw=3:tw=1024:cin
