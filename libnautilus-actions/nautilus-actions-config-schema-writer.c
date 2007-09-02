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
#include "nautilus-actions-config-schema-writer.h"
#include "nautilus-actions-config-gconf-private.h"

enum {
	NAUTILUS_ACTIONS_CONFIG_SCHEMA_WRITER_SAVE_PATH = 1,
};

static GObjectClass *parent_class = NULL;

gchar* nautilus_actions_config_schema_writer_get_saved_filename (NautilusActionsConfigSchemaWriter* self, gchar* uuid)
{
	gchar* filename = NULL;
	gchar* path = NULL;

	filename = g_strdup_printf ("config_%s.schemas", uuid);
	path = g_build_filename (self->save_path, filename, NULL);
	g_free (filename);

	return path;
}

static void nautilus_actions_config_schema_writer_set_property (GObject *object, guint property_id,
																			const GValue* value, GParamSpec *pspec)
{
	NautilusActionsConfigSchemaWriter* self = NAUTILUS_ACTIONS_CONFIG_SCHEMA_WRITER (object);

	switch (property_id) 
	{
		case NAUTILUS_ACTIONS_CONFIG_SCHEMA_WRITER_SAVE_PATH:
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

static void nautilus_actions_config_schema_writer_get_property (GObject *object, guint property_id,
																			GValue* value, GParamSpec *pspec)
{
	
	NautilusActionsConfigSchemaWriter* self = NAUTILUS_ACTIONS_CONFIG_SCHEMA_WRITER (object);

	switch (property_id) 
	{
		case NAUTILUS_ACTIONS_CONFIG_SCHEMA_WRITER_SAVE_PATH:
			g_value_set_string (value, self->save_path);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}		
}

static void create_schema_entry (xmlDocPtr doc, xmlNodePtr list_node, xmlChar* key_path, 
											char* type, const char* value, 
											char* short_desc, char* long_desc, 
											gboolean is_l10n_value)
{
	xmlNodePtr schema_node = NULL;
	xmlNodePtr locale_node = NULL;
	xmlChar* content = NULL;
	xmlChar* encoded_content = NULL;
	xmlNodePtr value_root_node = NULL;

	schema_node = xmlNewChild (list_node, NULL, BAD_CAST NA_GCONF_XML_SCHEMA_ENTRY, NULL);
	content = BAD_CAST g_build_path ("/", ACTIONS_SCHEMA_PREFIX, key_path, NULL);
	xmlNewChild (schema_node, NULL, BAD_CAST NA_GCONF_XML_SCHEMA_KEY, content);
	xmlFree (content);
	xmlNewChild (schema_node, NULL, BAD_CAST NA_GCONF_XML_SCHEMA_APPLYTO, key_path);
	xmlNewChild (schema_node, NULL, BAD_CAST NA_GCONF_XML_SCHEMA_OWNER, BAD_CAST ACTIONS_SCHEMA_OWNER);
	xmlNewChild (schema_node, NULL, BAD_CAST NA_GCONF_XML_SCHEMA_TYPE, BAD_CAST type);
	if (g_ascii_strncasecmp (type, "list", strlen ("list")) == 0)
	{
		xmlNewChild (schema_node, NULL, BAD_CAST NA_GCONF_XML_SCHEMA_LIST_TYPE, BAD_CAST "string");
	}
	locale_node = xmlNewChild (schema_node, NULL, BAD_CAST NA_GCONF_XML_SCHEMA_LOCALE, NULL);
	xmlNewProp (locale_node, BAD_CAST "name", BAD_CAST "C");
	value_root_node = schema_node;
	if (is_l10n_value)
	{
		// if the default value must be localized, put it in the <locale> element
		value_root_node = locale_node;
	}
	// Encode special chars <, >, &, ...
	encoded_content = xmlEncodeSpecialChars (doc, BAD_CAST value);
	xmlNewChild (value_root_node, NULL, BAD_CAST NA_GCONF_XML_SCHEMA_DFT, encoded_content);
	xmlFree (encoded_content);

	// Encode special chars <, >, &, ...
	encoded_content = xmlEncodeSpecialChars (doc, BAD_CAST short_desc);
	xmlNewChild (locale_node, NULL, BAD_CAST NA_GCONF_XML_SCHEMA_SHORT, encoded_content);
	xmlFree (encoded_content);
	
	// Encode special chars <, >, &, ...
	encoded_content = xmlEncodeSpecialChars (doc, BAD_CAST long_desc);
	xmlNewChild (locale_node, NULL, BAD_CAST NA_GCONF_XML_SCHEMA_LONG, encoded_content);
	xmlFree (encoded_content);
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

static void get_hash_keys (gchar* key, gchar* value, GSList** list)
{
	*list = g_slist_append (*list, key);
}

static gboolean
save_action (NautilusActionsConfig *self, NautilusActionsConfigAction *action)
{
	gboolean retv = FALSE;
	g_return_val_if_fail (NAUTILUS_ACTIONS_IS_CONFIG_SCHEMA_WRITER (self), FALSE);

	NautilusActionsConfigSchemaWriter* config = NAUTILUS_ACTIONS_CONFIG_SCHEMA_WRITER (self);
	xmlDocPtr doc = NULL;
	xmlNodePtr root_node = NULL;
	xmlNodePtr list_node = NULL;
	xmlChar* content = NULL;
	xmlChar* str_list = NULL;
	gchar* path = NULL;
	GSList* profile_list = NULL;
	GSList* iter;

	// update the version on the action 
	if (action->version)
	{
		g_free (action->version);
	}
	action->version = g_strdup (NAUTILUS_ACTIONS_CONFIG_VERSION);

	// Create the GConf schema XML file and write it in the save_path folder
	doc = xmlNewDoc (BAD_CAST "1.0");
	root_node = xmlNewNode (NULL, BAD_CAST NA_GCONF_XML_ROOT);
	xmlDocSetRootElement (doc, root_node);
	list_node = xmlNewChild (root_node, NULL, BAD_CAST NA_GCONF_XML_SCHEMA_LIST, NULL);

	//--> Menu item entries : label
	content = BAD_CAST g_build_path ("/", ACTIONS_CONFIG_DIR, action->uuid, ACTION_LABEL_ENTRY, NULL);
	create_schema_entry (doc, list_node, content, "string", action->label, ACTION_LABEL_DESC_SHORT, ACTION_LABEL_DESC_LONG, TRUE);
	xmlFree (content);

	//--> Menu item entries : tooltip
	content = BAD_CAST g_build_path ("/", ACTIONS_CONFIG_DIR, action->uuid, ACTION_TOOLTIP_ENTRY, NULL);
	create_schema_entry (doc, list_node, content, "string", action->tooltip, ACTION_TOOLTIP_DESC_SHORT, ACTION_TOOLTIP_DESC_LONG, TRUE);
	xmlFree (content);

	//--> Menu item entries : icon
	content = BAD_CAST g_build_path ("/", ACTIONS_CONFIG_DIR, action->uuid, ACTION_ICON_ENTRY, NULL);
	create_schema_entry (doc, list_node, content, "string", action->icon, ACTION_ICON_DESC_SHORT, ACTION_ICON_DESC_LONG, FALSE);
	xmlFree (content);

	g_hash_table_foreach (action->profiles, (GHFunc)get_hash_keys, &profile_list);

	for (iter = profile_list; iter; iter = iter->next)
	{
		gchar* profile_name = (gchar*)iter->data;
		gchar* profile_dir = g_strdup_printf ("%s%s", ACTIONS_PROFILE_PREFIX, profile_name);
		NautilusActionsConfigActionProfile* action_profile = nautilus_actions_config_action_get_profile (action, profile_name);

		//--> Profile entries : desc-name
		content = BAD_CAST g_build_path ("/", ACTIONS_CONFIG_DIR, action->uuid, profile_dir, ACTION_PROFILE_DESC_NAME_ENTRY, NULL);
		create_schema_entry (doc, list_node, content, "string", action_profile->desc_name, ACTION_PROFILE_NAME_DESC_SHORT, ACTION_PROFILE_NAME_DESC_LONG, FALSE);
		xmlFree (content);
	
		//--> Command entries : path
		content = BAD_CAST g_build_path ("/", ACTIONS_CONFIG_DIR, action->uuid, profile_dir, ACTION_PATH_ENTRY, NULL);
		create_schema_entry (doc, list_node, content, "string", action_profile->path, ACTION_PATH_DESC_SHORT, ACTION_PATH_DESC_LONG, FALSE);
		xmlFree (content);

		//--> Command entries : parameters
		content = BAD_CAST g_build_path ("/", ACTIONS_CONFIG_DIR, action->uuid, profile_dir, ACTION_PARAMS_ENTRY, NULL);
		create_schema_entry (doc, list_node, content, "string", action_profile->parameters, ACTION_PARAMS_DESC_SHORT, ACTION_PARAMS_DESC_LONG, FALSE);
		xmlFree (content);

		//--> Test entries : basenames
		content = BAD_CAST g_build_path ("/", ACTIONS_CONFIG_DIR, action->uuid, profile_dir, ACTION_BASENAMES_ENTRY, NULL);
		str_list = BAD_CAST gslist_to_schema_string (action_profile->basenames);
		create_schema_entry (doc, list_node, content, "list", (char*)str_list, ACTION_BASENAMES_DESC_SHORT, ACTION_BASENAMES_DESC_LONG, FALSE);
		xmlFree (str_list);
		xmlFree (content);

		//--> test entries : match_case
		content = BAD_CAST g_build_path ("/", ACTIONS_CONFIG_DIR, action->uuid, profile_dir, ACTION_MATCHCASE_ENTRY, NULL);
		create_schema_entry (doc, list_node, content, "bool", bool_to_schema_string (action_profile->match_case), ACTION_MATCHCASE_DESC_SHORT, ACTION_MATCHCASE_DESC_LONG, FALSE);
		xmlFree (content);

		//--> Test entries : mimetypes
		content = BAD_CAST g_build_path ("/", ACTIONS_CONFIG_DIR, action->uuid, profile_dir, ACTION_MIMETYPES_ENTRY, NULL);
		str_list = BAD_CAST gslist_to_schema_string (action_profile->mimetypes);
		create_schema_entry (doc, list_node, content, "list", (char*)str_list, ACTION_MIMETYPES_DESC_SHORT, ACTION_MIMETYPES_DESC_LONG, FALSE);
		xmlFree (str_list);
		xmlFree (content);
				
		//--> test entries : is_file
		content = BAD_CAST g_build_path ("/", ACTIONS_CONFIG_DIR, action->uuid, profile_dir, ACTION_ISFILE_ENTRY, NULL);
		create_schema_entry (doc, list_node, content, "bool", bool_to_schema_string (action_profile->is_file), ACTION_ISFILE_DESC_SHORT, _(ACTION_ISFILE_DESC_LONG), FALSE);
		xmlFree (content);

		//--> test entries : is_dir
		content = BAD_CAST g_build_path ("/", ACTIONS_CONFIG_DIR, action->uuid, profile_dir, ACTION_ISDIR_ENTRY, NULL);
		create_schema_entry (doc, list_node, content, "bool", bool_to_schema_string (action_profile->is_dir), ACTION_ISDIR_DESC_SHORT, _(ACTION_ISDIR_DESC_LONG), FALSE);
		xmlFree (content);

		//--> test entries : accept-multiple-files
		content = BAD_CAST g_build_path ("/", ACTIONS_CONFIG_DIR, action->uuid, profile_dir, ACTION_MULTIPLE_ENTRY, NULL);
		create_schema_entry (doc, list_node, content, "bool", bool_to_schema_string (action_profile->accept_multiple_files), ACTION_MULTIPLE_DESC_SHORT, ACTION_MULTIPLE_DESC_LONG, FALSE);
		xmlFree (content);

		//--> test entries : schemes
		content = BAD_CAST g_build_path ("/", ACTIONS_CONFIG_DIR, action->uuid, profile_dir, ACTION_SCHEMES_ENTRY, NULL);
		str_list = BAD_CAST gslist_to_schema_string (action_profile->schemes);
		create_schema_entry (doc, list_node, content, "list", (char*)str_list, ACTION_SCHEMES_DESC_SHORT, ACTION_SCHEMES_DESC_LONG, FALSE);
		xmlFree (str_list);
		xmlFree (content);

		g_free (profile_dir);
	}
	g_slist_free (profile_list);


	//--> general entry : version
	content = BAD_CAST g_build_path ("/", ACTIONS_CONFIG_DIR, action->uuid, ACTION_VERSION_ENTRY, NULL);
	create_schema_entry (doc, list_node, content, "string", action->version, ACTION_VERSION_DESC_SHORT, ACTION_VERSION_DESC_LONG, FALSE);
	xmlFree (content);

	// generate the filename name and save the schema into it
	path = nautilus_actions_config_schema_writer_get_saved_filename (config, action->uuid);
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
	g_return_val_if_fail (NAUTILUS_ACTIONS_IS_CONFIG_SCHEMA_WRITER (self), FALSE);

	//NautilusActionsConfigSchemaWriter* config = NAUTILUS_ACTIONS_CONFIG_SCHEMA_WRITER (self);

	return TRUE;
}

static void
nautilus_actions_config_schema_writer_finalize (GObject *object)
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
nautilus_actions_config_schema_writer_class_init (NautilusActionsConfigSchemaWriterClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GParamSpec* pspec;

	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = nautilus_actions_config_schema_writer_finalize;
	object_class->set_property = nautilus_actions_config_schema_writer_set_property;
	object_class->get_property = nautilus_actions_config_schema_writer_get_property;

	pspec = g_param_spec_string ("save-path",
										  "Schema Save path property",
										  "Set/Get the path where the schema files will be saved",
										  g_get_home_dir (),
										  G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
	g_object_class_install_property (object_class,
												NAUTILUS_ACTIONS_CONFIG_SCHEMA_WRITER_SAVE_PATH,
												pspec);

	NAUTILUS_ACTIONS_CONFIG_CLASS (klass)->save_action = save_action;
	NAUTILUS_ACTIONS_CONFIG_CLASS (klass)->remove_action = remove_action;
}

static void
nautilus_actions_config_schema_writer_init (NautilusActionsConfig *config, NautilusActionsConfigClass *klass)
{
}

GType
nautilus_actions_config_schema_writer_get_type (void)
{
   static GType type = 0;

	if (type == 0) {
		static GTypeInfo info = {
					sizeof (NautilusActionsConfigSchemaWriterClass),
					(GBaseInitFunc) NULL,
					(GBaseFinalizeFunc) NULL,
					(GClassInitFunc) nautilus_actions_config_schema_writer_class_init,
					NULL, NULL,
					sizeof (NautilusActionsConfigSchemaWriter),
					0,
					(GInstanceInitFunc) nautilus_actions_config_schema_writer_init
		};
		type = g_type_register_static (NAUTILUS_ACTIONS_TYPE_CONFIG, "NautilusActionsConfigSchemaWriter", &info, 0);
	}
	return type;
}

NautilusActionsConfigSchemaWriter *
nautilus_actions_config_schema_writer_get (void)
{
	static NautilusActionsConfigSchemaWriter *config = NULL;

	/* we share one NautilusActionsConfigSchemaWriter object for all */
	if (!config) {
		config = g_object_new (NAUTILUS_ACTIONS_TYPE_CONFIG_SCHEMA_WRITER, NULL);
		return config;
	}

	return NAUTILUS_ACTIONS_CONFIG_SCHEMA_WRITER (g_object_ref (G_OBJECT (config)));
}

// vim:ts=3:sw=3:tw=1024:cin
