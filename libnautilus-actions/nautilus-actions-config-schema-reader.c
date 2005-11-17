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
#include "nautilus-actions-config-schema-reader.h"
#include "nautilus-actions-config-gconf-private.h"

static GObjectClass *parent_class = NULL;

typedef enum {
	ACTION_NONE_TYPE = 0,
	ACTION_LABEL_TYPE,
	ACTION_TOOLTIP_TYPE,
	ACTION_ICON_TYPE,
	ACTION_PATH_TYPE,
	ACTION_PARAMS_TYPE,
	ACTION_BASENAMES_TYPE,
	ACTION_ISFILE_TYPE,
	ACTION_ISDIR_TYPE,
	ACTION_MULTIPLE_TYPE,
	ACTION_SCHEMES_TYPE,
	ACTION_VERSION_TYPE
} ActionFieldType;

static gchar* get_action_uuid_from_key (const gchar* key)
{
	g_return_val_if_fail (g_str_has_prefix (key, ACTIONS_CONFIG_DIR), NULL);
	
	gchar* uuid = g_strdup (key + strlen (ACTIONS_CONFIG_DIR "/"));
	gchar* pos = g_strrstr (uuid, "/");
	if (pos != NULL)
	{
		*pos = '\0';
	}

	return uuid;
}
	
static gboolean nautilus_actions_config_schema_reader_action_parse_schema_key_locale (xmlNode *config_node, gchar** value)
{
	gboolean retv = FALSE;
	xmlNode* iter;
	
	for (iter = config_node->children; iter; iter = iter->next)
	{
		xmlChar *text;

		if (!retv && iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp ((gchar*)iter->name, 
								NA_GCONF_XML_SCHEMA_DFT,
								strlen (NA_GCONF_XML_SCHEMA_DFT)) == 0)
		{
			*value = xmlNodeGetContent (iter);
			retv = TRUE;
		}
	}

	return retv;
}

static gboolean nautilus_actions_config_schema_reader_action_parse_schema_key (xmlNode *config_node, ActionFieldType* type, gchar** value, gchar** uuid, gboolean get_uuid)
{
	gboolean retv = FALSE;
	xmlNode* iter;
	gboolean is_key_ok = FALSE;
//	gboolean is_type_ok = FALSE;
	gboolean is_default_value_ok = FALSE;
	
	*type = ACTION_NONE_TYPE;
	for (iter = config_node->children; iter; iter = iter->next)
	{
		xmlChar *text;

		if (!is_key_ok && iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp ((gchar*)iter->name, 
								NA_GCONF_XML_SCHEMA_APPLYTO,
								strlen (NA_GCONF_XML_SCHEMA_APPLYTO)) == 0)
		{
			text = xmlNodeGetContent (iter);

			if (get_uuid)
			{
				*uuid = get_action_uuid_from_key (text);
			}

			if (g_str_has_suffix (text, ACTION_LABEL_ENTRY))
			{
				*type = ACTION_LABEL_TYPE;
			}
			else if (g_str_has_suffix (text, ACTION_TOOLTIP_ENTRY))
			{
				*type = ACTION_TOOLTIP_TYPE;
			}
			else if (g_str_has_suffix (text, ACTION_ICON_ENTRY))
			{
				*type = ACTION_ICON_TYPE;
			}
			else if (g_str_has_suffix (text, ACTION_PATH_ENTRY))
			{
				*type = ACTION_PATH_TYPE;
			}
			else if (g_str_has_suffix (text, ACTION_PARAMS_ENTRY))
			{
				*type = ACTION_PARAMS_TYPE;
			}
			else if (g_str_has_suffix (text, ACTION_BASENAMES_ENTRY))
			{
				*type = ACTION_BASENAMES_TYPE;
			}
			else if (g_str_has_suffix (text, ACTION_ISFILE_ENTRY))
			{
				*type = ACTION_ISFILE_TYPE;
			}
			else if (g_str_has_suffix (text, ACTION_ISDIR_ENTRY))
			{
				*type = ACTION_ISDIR_TYPE;
			}
			else if (g_str_has_suffix (text, ACTION_MULTIPLE_ENTRY))
			{
				*type = ACTION_MULTIPLE_TYPE;
			}
			else if (g_str_has_suffix (text, ACTION_SCHEMES_ENTRY))
			{
				*type = ACTION_SCHEMES_TYPE;
			}

			if (*type != ACTION_NONE_TYPE)
			{
				is_key_ok = TRUE;
			}
			xmlFree (text);
		}
		else if (!is_default_value_ok && iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp ((gchar*)iter->name, 
								NA_GCONF_XML_SCHEMA_DFT,
								strlen (NA_GCONF_XML_SCHEMA_DFT)) == 0)
		{
			*value = xmlNodeGetContent (iter);
			is_default_value_ok = TRUE;
		}
		else if (!is_default_value_ok && iter->type == XML_ELEMENT_NODE &&
				g_ascii_strncasecmp ((gchar*)iter->name, 
								NA_GCONF_XML_SCHEMA_LOCALE,
								strlen (NA_GCONF_XML_SCHEMA_LOCALE)) == 0)
		{
			//--> FIXME: Manage $lang attribute (at the moment it only take the first found)
			is_default_value_ok = nautilus_actions_config_schema_reader_action_parse_schema_key_locale (iter, value);
		}
	}

	if (is_default_value_ok && is_key_ok)
	{
		retv = TRUE;
	}
	
	return retv;
}

static GSList* schema_string_to_gslist (const gchar* str_list_value)
{
	GSList* list = NULL;
	gchar* ptr = str_list_value;
	gchar* start = NULL;
	gchar* str_list = NULL;
	gchar** str_list_splited = NULL;
	gboolean found = FALSE;
	int i;
	
	//--> First remove the brackets []
	while (*ptr != '[')
	{
		ptr++;
	}

	if (*ptr == '[')
	{
		ptr++;
		start = ptr;
		i = 0;
		while (*ptr != ']')
		{
			i++;
			ptr++;
		}
		if (*ptr == ']')
		{
			str_list = g_strndup (start, i);
		}
	}

	//--> split the result and fill the list
	if (str_list != NULL)
	{
		str_list_splited = g_strsplit (str_list, ",", -1);
		i = 0;
		while (str_list_splited[i] != NULL)
		{
			list = g_slist_append (list, g_strdup (str_list_splited[i]));
			i++;
		}
		g_strfreev (str_list_splited);
	}
	
	return list;	
}

static gboolean schema_string_to_bool (const gchar* str_bool_value)
{
	gboolean bool_value = FALSE;
	if (g_ascii_strcasecmp (str_bool_value, "true") == 0)
	{
		bool_value = TRUE;
	}

	return bool_value;
}

static gboolean nautilus_actions_config_schema_reader_action_fill (NautilusActionsConfigAction* action, xmlNode* config_node)
{
	xmlNode* iter;
	gboolean retv = FALSE;
	gboolean is_label_ok = FALSE;
	gboolean is_tooltip_ok = FALSE;
	gboolean is_icon_ok = FALSE;
	gboolean is_path_ok = FALSE;
	gboolean is_params_ok = FALSE;
	gboolean is_basenames_ok = FALSE;
	gboolean is_isfile_ok = FALSE;
	gboolean is_isdir_ok = FALSE;
	gboolean is_multiple_ok = FALSE;
	gboolean is_schemes_ok = FALSE;
	gboolean is_version_ok = FALSE;
	ActionFieldType type;
	GSList* list = NULL;

	for (iter = config_node->children; iter; iter = iter->next)
	{
		type = ACTION_NONE_TYPE;
		if (iter->type == XML_ELEMENT_NODE &&
			g_ascii_strncasecmp ((gchar*)iter->name, 
											NA_GCONF_XML_SCHEMA_ENTRY,
											strlen (NA_GCONF_XML_SCHEMA_ENTRY)) == 0)
		{
			gchar* value;

			if (nautilus_actions_config_schema_reader_action_parse_schema_key (iter, &type, &value, &(action->uuid), (action->uuid != NULL)))
			{

				switch (type)
				{
					case ACTION_LABEL_TYPE:
						is_label_ok = TRUE;
						nautilus_actions_config_action_set_label (action, value);
						break;
					case ACTION_TOOLTIP_TYPE:
						is_tooltip_ok = TRUE;
						nautilus_actions_config_action_set_tooltip (action, value);
						break;
					case ACTION_ICON_TYPE:
						is_icon_ok = TRUE;
						nautilus_actions_config_action_set_icon (action, value);
						break;
					case ACTION_PATH_TYPE:
						is_path_ok = TRUE;
						nautilus_actions_config_action_set_path (action, value);
						break;
					case ACTION_PARAMS_TYPE:
						is_params_ok = TRUE;
						nautilus_actions_config_action_set_parameters (action, value);
						break;
					case ACTION_BASENAMES_TYPE:
						is_basenames_ok = TRUE;
						list = schema_string_to_gslist (value);
						nautilus_actions_config_action_set_basenames (action, list);
						g_slist_foreach (list, (GFunc)g_free, NULL);
						g_slist_free (list);
						break;
					case ACTION_ISFILE_TYPE:
						is_isfile_ok = TRUE;
						nautilus_actions_config_action_set_is_file (action, schema_string_to_bool (value));
						break;
					case ACTION_ISDIR_TYPE:
						is_isdir_ok = TRUE;
						nautilus_actions_config_action_set_is_dir (action, schema_string_to_bool (value));
						break;
					case ACTION_MULTIPLE_TYPE:
						is_multiple_ok = TRUE;
						nautilus_actions_config_action_set_accept_multiple (action, schema_string_to_bool (value));
						break;
					case ACTION_SCHEMES_TYPE:
						is_schemes_ok = TRUE;
						list = schema_string_to_gslist (value);
						nautilus_actions_config_action_set_schemes (action, list);
						g_slist_foreach (list, (GFunc)g_free, NULL);
						g_slist_free (list);
						break;
					case ACTION_VERSION_TYPE:
						is_version_ok = TRUE;
						action->version = g_strdup (NAUTILUS_ACTIONS_CONFIG_VERSION);
						break;
					default:
						retv = FALSE;
						break;
				}

				g_free (value);
			}	
		}
	}
		
	if (retv && (is_version_ok && is_schemes_ok && is_multiple_ok && 
				is_isdir_ok && is_isfile_ok && is_basenames_ok && 
				is_params_ok && is_path_ok && is_icon_ok && 
				is_tooltip_ok && is_label_ok))
	{
		retv = TRUE;
	}	

	return retv;
}

gboolean nautilus_actions_config_schema_reader_parse_file (NautilusActionsConfigSchemaReader* config, const gchar* filename)
{
	xmlDoc* doc = NULL;
	xmlNode* root_node;
	xmlNode* iter;
	NautilusActionsConfigAction *action;
	gboolean retv = FALSE;

	doc = xmlParseFile (filename);
	if (doc != NULL)
	{
		root_node = xmlDocGetRootElement (doc);
		if (g_ascii_strncasecmp ((gchar*)root_node->name, 
										NA_GCONF_XML_ROOT, 
										strlen (NA_GCONF_XML_ROOT)) == 0)
		{
			for (iter = root_node->children; iter; iter = iter->next)
			{
				if (iter->type == XML_ELEMENT_NODE &&
					g_ascii_strncasecmp ((gchar*)iter->name, 
												NA_GCONF_XML_SCHEMA_LIST, 
												strlen (NA_GCONF_XML_SCHEMA_LIST)) == 0)
				{
					action = nautilus_actions_config_action_new ();
					if (action->uuid != NULL)
					{
						g_free (action->uuid);
						action->uuid = NULL;
					}
					if (nautilus_actions_config_schema_reader_action_fill (action, iter))
					{
						g_hash_table_insert (NAUTILUS_ACTIONS_CONFIG (config)->actions, g_strdup (action->uuid), action);
						retv = TRUE;
					}
					else
					{
						nautilus_actions_config_action_free (action);
					}
				}
			}
		}
		xmlFreeDoc (doc);
	}

	xmlCleanupParser ();
	
	return retv;
}

static gboolean
save_action (NautilusActionsConfig *self, NautilusActionsConfigAction *action)
{
	gboolean retv = TRUE;
	g_return_val_if_fail (NAUTILUS_ACTIONS_IS_CONFIG_SCHEMA_READER (self), FALSE);

//	NautilusActionsConfigSchemaReader* config = NAUTILUS_ACTIONS_CONFIG_SCHEMA_READER (self);
	
	return retv;
}

static gboolean
remove_action (NautilusActionsConfig *self, NautilusActionsConfigAction* action)
{
	g_return_val_if_fail (NAUTILUS_ACTIONS_IS_CONFIG_SCHEMA_READER (self), FALSE);

//	NautilusActionsConfigSchemaReader* config = NAUTILUS_ACTIONS_CONFIG_SCHEMA_READER (self);

	return TRUE;
}

static void
nautilus_actions_config_schema_reader_finalize (GObject *object)
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
nautilus_actions_config_schema_reader_class_init (NautilusActionsConfigSchemaReaderClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GParamSpec* pspec;

	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = nautilus_actions_config_schema_reader_finalize;

	NAUTILUS_ACTIONS_CONFIG_CLASS (klass)->save_action = save_action;
	NAUTILUS_ACTIONS_CONFIG_CLASS (klass)->remove_action = remove_action;
}

static void
nautilus_actions_config_schema_reader_init (NautilusActionsConfig *config, NautilusActionsConfigClass *klass)
{
}

GType
nautilus_actions_config_schema_reader_get_type (void)
{
   static GType type = 0;

	if (type == 0) {
		static GTypeInfo info = {
					sizeof (NautilusActionsConfigSchemaReaderClass),
					(GBaseInitFunc) NULL,
					(GBaseFinalizeFunc) NULL,
					(GClassInitFunc) nautilus_actions_config_schema_reader_class_init,
					NULL, NULL,
					sizeof (NautilusActionsConfigSchemaReader),
					0,
					(GInstanceInitFunc) nautilus_actions_config_schema_reader_init
		};
		type = g_type_register_static (NAUTILUS_ACTIONS_TYPE_CONFIG, "NautilusActionsConfigSchemaReader", &info, 0);
	}
	return type;
}

NautilusActionsConfigSchemaReader *
nautilus_actions_config_schema_reader_get (void)
{
	static NautilusActionsConfigSchemaReader *config = NULL;

	/* we share one NautilusActionsConfigSchemaReader object for all */
	if (!config) {
		config = g_object_new (NAUTILUS_ACTIONS_TYPE_CONFIG_SCHEMA_READER, NULL);
		return config;
	}

	return NAUTILUS_ACTIONS_CONFIG_SCHEMA_READER (g_object_ref (G_OBJECT (config)));
}

// vim:ts=3:sw=3:tw=1024:cin
