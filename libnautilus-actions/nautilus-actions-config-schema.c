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
#include "nautilus-actions-config-schema.h"
#include "nautilus-actions-config-gconf-private.h"

enum {
	NAUTILUS_ACTIONS_CONFIG_SCHEMA_SAVE_PATH = 1,
};

static GObjectClass *parent_class = NULL;

gchar* nautilus_actions_config_schema_get_saved_filename (NautilusActionsConfigSchema* self, gchar* uuid)
{
	gchar* filename = NULL;
	gchar* path = NULL;

	filename = g_strdup_printf ("config_%s.schemas", uuid);
	path = g_build_filename (self->save_path, filename, NULL);
	g_free (filename);

	return path;
}

static void nautilus_actions_config_schema_set_property (GObject *object, guint property_id,
																			const GValue* value, GParamSpec *pspec)
{
	NautilusActionsConfigSchema* self = NAUTILUS_ACTIONS_CONFIG_SCHEMA (object);

	switch (property_id) 
	{
		case NAUTILUS_ACTIONS_CONFIG_SCHEMA_SAVE_PATH:
			if (self->save_path)
			{
				g_free (self->save_path);
			}	
			self->save_path = g_value_dup_string (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}		
}

static void nautilus_actions_config_schema_get_property (GObject *object, guint property_id,
																			const GValue* value, GParamSpec *pspec)
{
	
	NautilusActionsConfigSchema* self = NAUTILUS_ACTIONS_CONFIG_SCHEMA (object);

	switch (property_id) 
	{
		case NAUTILUS_ACTIONS_CONFIG_SCHEMA_SAVE_PATH:
			g_value_set_string (value, self->save_path);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}		
}

static void create_schema_entry (xmlNodePtr list_node, xmlChar* key_path, 
											xmlChar* type, xmlChar* value, 
											xmlChar* short_desc, xmlChar* long_desc, 
											gboolean is_l10n_value)
{
	xmlNodePtr schema_node = NULL;
	xmlNodePtr locale_node = NULL;
	xmlChar* content = NULL;
	xmlNodePtr* value_root_node = NULL;

	//--> Menu item entries : label
	schema_node = xmlNewChild (list_node, NULL, "schema", NULL);
	content = g_build_path ("/", ACTIONS_SCHEMA_PREFIX, key_path, NULL);
	xmlNewChild (schema_node, NULL, "key", content);
	xmlFree (content);
	xmlNewChild (schema_node, NULL, "applyto", key_path);
	xmlNewChild (schema_node, NULL, "owner", ACTIONS_SCHEMA_OWNER);
	xmlNewChild (schema_node, NULL, "type", type);
	if (g_ascii_strncasecmp (type, "list", strlen ("list")) == 0)
	{
		xmlNewChild (schema_node, NULL, "list_type", "string");
	}
	locale_node = xmlNewChild (schema_node, NULL, "locale", NULL);
	xmlNewProp (locale_node, "name", "C");
	value_root_node = schema_node;
	if (is_l10n_value)
	{
		// if the default value must be localized, put it in the <locale> element
		value_root_node = locale_node;
	}
	xmlNewChild (value_root_node, NULL, "default", value);

	xmlNewChild (locale_node, NULL, "short", short_desc);
	xmlNewChild (locale_node, NULL, "long", long_desc);
}

static gchar* gslist_to_schema_string (GSList* list)
{
	GSList* iter = NULL;
	GString* result = NULL;

	result = g_string_new ("[");
	if (list)
	{
		iter = list;
		result = g_string_append (result, (gchar*)iter->data);
		iter = iter->next;
		while (iter)
		{
			result = g_string_append_c (result, ',');
			result = g_string_append (result, (gchar*)iter->data);
			iter = iter->next;
		}
	}
	result = g_string_append_c (result, ']');

	return g_string_free (result, FALSE);
}

static const gchar* bool_to_schema_string (gboolean bool_value)
{
	if (bool_value)
	{
		return "true";
	}
	return "false";
}

static gboolean
save_action (NautilusActionsConfig *self, NautilusActionsConfigAction *action)
{
	gboolean retv = FALSE;
	g_return_val_if_fail (NAUTILUS_ACTIONS_IS_CONFIG_SCHEMA (self), FALSE);

	NautilusActionsConfigSchema* config = NAUTILUS_ACTIONS_CONFIG_SCHEMA (self);
	xmlDocPtr doc = NULL;
	xmlNodePtr root_node = NULL;
	xmlNodePtr list_node = NULL;
	xmlChar* content = NULL;
	xmlChar* str_list = NULL;
	gchar* path = NULL;

	// update the version on the action 
	if (action->version)
	{
		g_free (action->version);
	}
	action->version = g_strdup (NAUTILUS_ACTIONS_CONFIG_VERSION);

	// Create the GConf schema XML file and write it in the save_path folder
	doc = xmlNewDoc ("1.0");
	root_node = xmlNewNode (NULL, "gconfschemafile");
	xmlDocSetRootElement (doc, root_node);
	list_node = xmlNewChild (root_node, NULL, "schemalist", NULL);

	//--> Menu item entries : label
	content = g_build_path ("/", ACTIONS_CONFIG_DIR, action->uuid, ACTION_LABEL_ENTRY, NULL);
	create_schema_entry (list_node, content, "string", action->label, ACTION_LABEL_DESC_SHORT, ACTION_LABEL_DESC_LONG, TRUE);
	xmlFree (content);

	//--> Menu item entries : tooltip
	content = g_build_path ("/", ACTIONS_CONFIG_DIR, action->uuid, ACTION_TOOLTIP_ENTRY, NULL);
	create_schema_entry (list_node, content, "string", action->tooltip, ACTION_TOOLTIP_DESC_SHORT, ACTION_TOOLTIP_DESC_LONG, TRUE);
	xmlFree (content);

	//--> Menu item entries : icon
	content = g_build_path ("/", ACTIONS_CONFIG_DIR, action->uuid, ACTION_ICON_ENTRY, NULL);
	create_schema_entry (list_node, content, "string", action->icon, ACTION_ICON_DESC_SHORT, ACTION_ICON_DESC_LONG, FALSE);
	xmlFree (content);

	//--> Command entries : path
	content = g_build_path ("/", ACTIONS_CONFIG_DIR, action->uuid, ACTION_PATH_ENTRY, NULL);
	create_schema_entry (list_node, content, "string", action->path, ACTION_PATH_DESC_SHORT, ACTION_PATH_DESC_LONG, FALSE);
	xmlFree (content);

	//--> Command entries : parameters
	content = g_build_path ("/", ACTIONS_CONFIG_DIR, action->uuid, ACTION_PARAMS_ENTRY, NULL);
	create_schema_entry (list_node, content, "string", action->parameters, ACTION_PARAMS_DESC_SHORT, ACTION_PARAMS_DESC_LONG, FALSE);
	xmlFree (content);

	//--> Test entries : basenames
	content = g_build_path ("/", ACTIONS_CONFIG_DIR, action->uuid, ACTION_BASENAMES_ENTRY, NULL);
	str_list = gslist_to_schema_string (action->basenames);
	create_schema_entry (list_node, content, "list", str_list, ACTION_BASENAMES_DESC_SHORT, ACTION_BASENAMES_DESC_LONG, FALSE);
	xmlFree (str_list);
	xmlFree (content);
		
	//--> test entries : is_file
	content = g_build_path ("/", ACTIONS_CONFIG_DIR, action->uuid, ACTION_ISFILE_ENTRY, NULL);
	create_schema_entry (list_node, content, "bool", bool_to_schema_string (action->is_file), ACTION_ISFILE_DESC_SHORT, _(ACTION_ISFILE_DESC_LONG), FALSE);
	xmlFree (content);

	//--> test entries : is_dir
	content = g_build_path ("/", ACTIONS_CONFIG_DIR, action->uuid, ACTION_ISDIR_ENTRY, NULL);
	create_schema_entry (list_node, content, "bool", bool_to_schema_string (action->is_dir), ACTION_ISDIR_DESC_SHORT, _(ACTION_ISDIR_DESC_LONG), FALSE);
	xmlFree (content);

	//--> test entries : accept-multiple-files
	content = g_build_path ("/", ACTIONS_CONFIG_DIR, action->uuid, ACTION_MULTIPLE_ENTRY, NULL);
	create_schema_entry (list_node, content, "bool", bool_to_schema_string (action->accept_multiple_files), ACTION_MULTIPLE_DESC_SHORT, ACTION_MULTIPLE_DESC_LONG, FALSE);
	xmlFree (content);

	//--> test entries : schemes
	content = g_build_path ("/", ACTIONS_CONFIG_DIR, action->uuid, ACTION_SCHEMES_ENTRY, NULL);
	str_list = gslist_to_schema_string (action->schemes);
	create_schema_entry (list_node, content, "list", str_list, ACTION_SCHEMES_DESC_SHORT, ACTION_SCHEMES_DESC_LONG, FALSE);
	xmlFree (str_list);
	xmlFree (content);

	//--> general entry : version
	content = g_build_path ("/", ACTIONS_CONFIG_DIR, action->uuid, ACTION_VERSION_ENTRY, NULL);
	create_schema_entry (list_node, content, "string", action->version, ACTION_VERSION_DESC_SHORT, ACTION_VERSION_DESC_LONG, FALSE);
	xmlFree (content);

	// generate the filename name and save the schema into it
	path = nautilus_actions_config_schema_get_saved_filename (config, action->uuid);
	if (xmlSaveFormatFileEnc (path, doc, "UTF-8", 1) != -1)
	{
		retv = TRUE;
	}
	g_free (path);

	xmlFreeDoc (doc);
	xmlCleanupParser();
	
	return retv;
}

static gboolean
remove_action (NautilusActionsConfig *self, NautilusActionsConfigAction* action)
{
	g_return_val_if_fail (NAUTILUS_ACTIONS_IS_CONFIG_SCHEMA (self), FALSE);

	NautilusActionsConfigSchema* config = NAUTILUS_ACTIONS_CONFIG_SCHEMA (self);

	return TRUE;
}

static void
nautilus_actions_config_schema_finalize (GObject *object)
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
nautilus_actions_config_schema_class_init (NautilusActionsConfigSchemaClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GParamSpec* pspec;

	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = nautilus_actions_config_schema_finalize;
	object_class->set_property = nautilus_actions_config_schema_set_property;
	object_class->get_property = nautilus_actions_config_schema_get_property;

	pspec = g_param_spec_string ("save-path",
										  "Schema Save path property",
										  "Set/Get the path where the schema files will be saved",
										  g_get_home_dir (),
										  G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
	g_object_class_install_property (object_class,
												NAUTILUS_ACTIONS_CONFIG_SCHEMA_SAVE_PATH,
												pspec);

	NAUTILUS_ACTIONS_CONFIG_CLASS (klass)->save_action = save_action;
	NAUTILUS_ACTIONS_CONFIG_CLASS (klass)->remove_action = remove_action;
}

static void
nautilus_actions_config_schema_init (NautilusActionsConfig *config, NautilusActionsConfigClass *klass)
{
}

GType
nautilus_actions_config_schema_get_type (void)
{
   static GType type = 0;

	if (type == 0) {
		static GTypeInfo info = {
					sizeof (NautilusActionsConfigSchemaClass),
					(GBaseInitFunc) NULL,
					(GBaseFinalizeFunc) NULL,
					(GClassInitFunc) nautilus_actions_config_schema_class_init,
					NULL, NULL,
					sizeof (NautilusActionsConfigSchema),
					0,
					(GInstanceInitFunc) nautilus_actions_config_schema_init
		};
		type = g_type_register_static (NAUTILUS_ACTIONS_TYPE_CONFIG, "NautilusActionsConfigSchema", &info, 0);
	}
	return type;
}

NautilusActionsConfigSchema *
nautilus_actions_config_schema_get (void)
{
	static NautilusActionsConfigSchema *config = NULL;

	/* we share one NautilusActionsConfigSchema object for all */
	if (!config) {
		config = g_object_new (NAUTILUS_ACTIONS_TYPE_CONFIG_SCHEMA, NULL);
		return config;
	}

	return NAUTILUS_ACTIONS_CONFIG_SCHEMA (g_object_ref (G_OBJECT (config)));
}

// vim:ts=3:sw=3:tw=1024:cin
