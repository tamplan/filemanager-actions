/* Nautilus Actions configuration tool
 * Copyright (C) 2005 The GNOME Foundation
 *
 * Authors:
 *	 Rodrigo Moya (rodrigo@gnome-db.org)
 *  Frederic Ruaudel (grumz@grumz.net)
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
#include "nautilus-actions-config.h"
#include "nautilus-actions-config-gconf-private.h"

enum {
        ACTION_ADDED,
        ACTION_CHANGED,
        ACTION_REMOVED,
        LAST_SIGNAL
};

static GObjectClass *parent_class = NULL;
static gint signals[LAST_SIGNAL] = { 0 };

static void nautilus_actions_config_action_removed_default_handler (NautilusActionsConfig* config, 
																				NautilusActionsConfigAction* action,
																				gpointer user_data);
static void nautilus_actions_config_action_changed_default_handler (NautilusActionsConfig* config, 
																				NautilusActionsConfigAction* action,
																				gpointer user_data);
static void nautilus_actions_config_action_added_default_handler (NautilusActionsConfig* config, 
																				NautilusActionsConfigAction* action,
																				gpointer user_data);

static void
nautilus_actions_config_finalize (GObject *object)
{
	NautilusActionsConfig *config = (NautilusActionsConfig *) object;

	g_return_if_fail (NAUTILUS_ACTIONS_IS_CONFIG (config));

	/* free all memory */
	if (config->actions) {
		g_hash_table_destroy (config->actions);
		config->actions = NULL;
	}

	/* chain call to parent class */
	if (parent_class->finalize)
		parent_class->finalize (object);
}

static void
nautilus_actions_config_class_init (NautilusActionsConfigClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = nautilus_actions_config_finalize;
	klass->save_action = NULL; /* Pure virtual function */
	klass->remove_action = NULL; /* Pure virtual function */

	klass->action_added = nautilus_actions_config_action_added_default_handler;
	klass->action_changed = nautilus_actions_config_action_changed_default_handler;
	klass->action_removed = nautilus_actions_config_action_removed_default_handler;

	/* create class signals */
	signals[ACTION_ADDED] = g_signal_new ("action_added",
														G_TYPE_FROM_CLASS (object_class),
														G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
														G_STRUCT_OFFSET (NautilusActionsConfigClass, action_added),
														NULL, NULL,
														g_cclosure_marshal_VOID__POINTER,
														G_TYPE_NONE, 1, G_TYPE_POINTER);
	signals[ACTION_CHANGED] = g_signal_new ("action_changed",
														G_TYPE_FROM_CLASS (object_class),
														G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
														G_STRUCT_OFFSET (NautilusActionsConfigClass, action_changed),
														NULL, NULL,
														g_cclosure_marshal_VOID__POINTER,
														G_TYPE_NONE, 1, G_TYPE_POINTER);
	signals[ACTION_REMOVED] = g_signal_new ("action_removed",
														G_TYPE_FROM_CLASS (object_class),
														G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
														G_STRUCT_OFFSET (NautilusActionsConfigClass, action_removed),
														NULL, NULL,
														g_cclosure_marshal_VOID__POINTER,
														G_TYPE_NONE, 1, G_TYPE_POINTER);

}

static void
nautilus_actions_config_init (NautilusActionsConfig *config, NautilusActionsConfigClass *klass)
{
	config->actions = g_hash_table_new_full (g_str_hash, g_str_equal,
						 (GDestroyNotify) g_free,
						 (GDestroyNotify) nautilus_actions_config_action_free);

	/* actions must be loaded by the children class */
}

GType
nautilus_actions_config_get_type (void)
{
   static GType type = 0;

        if (type == 0) {
                static GTypeInfo info = {
                        sizeof (NautilusActionsConfigClass),
                        (GBaseInitFunc) NULL,
                        (GBaseFinalizeFunc) NULL,
                        (GClassInitFunc) nautilus_actions_config_class_init,
                        NULL, NULL,
                        sizeof (NautilusActionsConfig),
                        0,
			(GInstanceInitFunc) nautilus_actions_config_init
                };
		type = g_type_register_static (G_TYPE_OBJECT, "NautilusActionsConfig", &info, 0);
        }
        return type;
}

static void
add_hash_action_to_list (gpointer key, gpointer value, gpointer user_data)
{
	GSList **list = user_data;

	NautilusActionsConfigAction* action_copy = nautilus_actions_config_action_dup ((NautilusActionsConfigAction*)value);
	
	if (action_copy != NULL)
	{
		(*list) = g_slist_append ((*list), action_copy);
	}
}

NautilusActionsConfigAction *
nautilus_actions_config_get_action (NautilusActionsConfig *config, const gchar *uuid)
{
	g_return_val_if_fail (NAUTILUS_ACTIONS_IS_CONFIG (config), NULL);

	return g_hash_table_lookup (config->actions, uuid);
}

GSList *
nautilus_actions_config_get_actions (NautilusActionsConfig *config)
{
	GSList *list = NULL;

	g_return_val_if_fail (NAUTILUS_ACTIONS_IS_CONFIG (config), NULL);

	g_hash_table_foreach (config->actions, (GHFunc) add_hash_action_to_list, &list);

	return list;
}

void
nautilus_actions_config_free_actions_list (GSList *list)
{
	g_slist_foreach (list, (GFunc)nautilus_actions_config_action_free, NULL);
	g_slist_free (list);
}

static gchar* get_new_uuid (void)
{
		uuid_t uuid;
		gchar uuid_str[64];

		uuid_generate (uuid);
		uuid_unparse (uuid, uuid_str);
		return g_strdup (uuid_str);
}

gboolean
nautilus_actions_config_add_action (NautilusActionsConfig *config, NautilusActionsConfigAction *action, GError** error)
{
	gboolean retv = FALSE;
	NautilusActionsConfigAction* found_action;
	
	g_assert (NAUTILUS_ACTIONS_IS_CONFIG (config));
	g_assert (action != NULL);

	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	if (action->uuid != NULL) 
	{
		found_action = (NautilusActionsConfigAction*)g_hash_table_lookup (config->actions, action->uuid);
		if (found_action != NULL) 
		{
			// i18n notes: will be displayed in an error dialog
			g_set_error (error, NAUTILUS_ACTIONS_CONFIG_ERROR, NAUTILUS_ACTIONS_CONFIG_ERROR_FAILED, _("The action '%s' already exists with the name '%s', please first remove the existing one before trying to add this one"), action->label, found_action->label);
			return FALSE;
		}
	} 
	else 
	{
		action->uuid = get_new_uuid ();
	}

	if (NAUTILUS_ACTIONS_CONFIG_GET_CLASS (config)->save_action (config, action))
	{
		g_signal_emit (config, signals[ACTION_ADDED], 0, action);
		retv = TRUE;
	}
	else
	{
		// i18n notes: will be displayed in an error dialog
		g_set_error (error, NAUTILUS_ACTIONS_CONFIG_ERROR, NAUTILUS_ACTIONS_CONFIG_ERROR_FAILED, _("Can't save action '%s'"), action->label);
	}
	
	return retv;
}

static void nautilus_actions_config_action_added_default_handler (NautilusActionsConfig* config, 
																				NautilusActionsConfigAction* action,
																				gpointer user_data)
{
	NautilusActionsConfigAction* action_copy = nautilus_actions_config_action_dup (action);
	if (action_copy)
	{
		g_hash_table_insert (config->actions, g_strdup (action->uuid), action_copy);
	}
}

gboolean
nautilus_actions_config_update_action (NautilusActionsConfig *config, NautilusActionsConfigAction *action)
{
	gboolean retv = FALSE;
	g_return_val_if_fail (NAUTILUS_ACTIONS_IS_CONFIG (config), FALSE);
	g_return_val_if_fail (action != NULL, FALSE);

	if (!g_hash_table_lookup (config->actions, action->uuid))
		return FALSE;

	if (NAUTILUS_ACTIONS_CONFIG_GET_CLASS(config)->save_action (config, action))
	{
		g_signal_emit (config, signals[ACTION_CHANGED], 0, action);
		retv = TRUE;
	}
	
	return retv;
}

static void nautilus_actions_config_action_changed_default_handler (NautilusActionsConfig* config, 
																				NautilusActionsConfigAction* action,
																				gpointer user_data)
{
}

gboolean
nautilus_actions_config_remove_action (NautilusActionsConfig *config, const gchar *uuid)
{
	NautilusActionsConfigAction *action;
	gboolean retv = FALSE;

	g_return_val_if_fail (NAUTILUS_ACTIONS_IS_CONFIG (config), FALSE);
	g_return_val_if_fail (uuid != NULL, FALSE);

	if (!(action = g_hash_table_lookup (config->actions, uuid)))
		return FALSE;

	if (NAUTILUS_ACTIONS_CONFIG_GET_CLASS(config)->remove_action (config, action))
	{
		g_signal_emit (config, signals[ACTION_REMOVED], 0, action);
		retv = TRUE;
	}

	return retv;
}

static gboolean
clear_actions_hashtable (gchar *uuid, NautilusActionsConfigAction *action, NautilusActionsConfig *config)
{
	gboolean retv = FALSE;
	
	if (NAUTILUS_ACTIONS_CONFIG_GET_CLASS(config)->remove_action (config, action))
	{
		retv = TRUE;
	}

	return retv;
}

gboolean
nautilus_actions_config_clear (NautilusActionsConfig *config)
{
	gboolean retv = FALSE;

	g_return_val_if_fail (NAUTILUS_ACTIONS_IS_CONFIG (config), FALSE);
	g_return_val_if_fail (config->actions != NULL, FALSE);

	int nb_actions = g_hash_table_size (config->actions);
	int nb_actions_removed = g_hash_table_foreach_remove (config->actions, (GHRFunc)clear_actions_hashtable, config);

	if (nb_actions_removed == nb_actions)
	{
		retv = TRUE;
	}
	
	return retv;
}

static void nautilus_actions_config_action_removed_default_handler (NautilusActionsConfig* config, 
																				NautilusActionsConfigAction* action,
																				gpointer user_data)
{
	if (!g_hash_table_remove (config->actions, action->uuid))
	{
		g_signal_stop_emission (config, signals[ACTION_REMOVED], 0);
		g_print ("Error: can't remove action => stop signal emission\n");
	}
}

NautilusActionsConfigActionProfile *nautilus_actions_config_action_profile_new (void)
{
	return g_new0 (NautilusActionsConfigActionProfile, 1);
}

NautilusActionsConfigActionProfile *nautilus_actions_config_action_profile_new_default (void)
{
	NautilusActionsConfigActionProfile* new_action_profile = nautilus_actions_config_action_profile_new();
	//--> Set some good default values
	new_action_profile->path = g_strdup ("");
	new_action_profile->parameters = g_strdup ("");
	new_action_profile->basenames = NULL;
	new_action_profile->basenames = g_slist_append (new_action_profile->basenames, g_strdup ("*"));
	new_action_profile->match_case = TRUE;
	new_action_profile->mimetypes = NULL;
	new_action_profile->mimetypes = g_slist_append (new_action_profile->mimetypes, g_strdup ("*/*"));
	new_action_profile->is_file = TRUE;
	new_action_profile->is_dir = FALSE;
	new_action_profile->accept_multiple_files = FALSE;
	new_action_profile->schemes = NULL;
	new_action_profile->schemes = g_slist_append (new_action_profile->schemes, g_strdup ("file"));
	
	return new_action_profile;
}

gboolean
nautilus_actions_config_action_profile_exists (NautilusActionsConfigAction *action, 
									 							gchar* profile_name)
{
	gboolean retv = FALSE;

	if (g_hash_table_lookup (action->profiles, profile_name) != NULL)
	{
		retv = TRUE;
	}

	return retv;
}

NautilusActionsConfigActionProfile*
nautilus_actions_config_action_get_profile (NautilusActionsConfigAction *action, 
									 						gchar* profile_name)
{
	return g_hash_table_lookup (action->profiles, profile_name);
}

void
nautilus_actions_config_action_add_profile (NautilusActionsConfigAction *action, 
									 gchar* profile_name,
								 	 NautilusActionsConfigActionProfile* profile)
{
	g_hash_table_insert (action->profiles, g_strdup (profile_name), profile);
}

void
nautilus_actions_config_action_replace_profile (NautilusActionsConfigAction *action, 
									 gchar* profile_name,
								 	 NautilusActionsConfigActionProfile* profile)
{
	//--> the old value is freed by the function
	g_hash_table_replace (action->profiles, g_strdup (profile_name), profile);
}

gboolean
nautilus_actions_config_action_remove_profile (NautilusActionsConfigAction *action, 
									 gchar* profile_name)
{
	g_hash_table_remove (action->profiles, profile_name);
}

NautilusActionsConfigAction *nautilus_actions_config_action_new (void)
{
	return g_new0 (NautilusActionsConfigAction, 1);
}

NautilusActionsConfigAction *nautilus_actions_config_action_new_default (void)
{
	NautilusActionsConfigAction* new_action = nautilus_actions_config_action_new();
	//--> Set some good default values
	new_action->conf_section = g_strdup ("");
	new_action->uuid = get_new_uuid ();
	new_action->label = g_strdup ("");
	new_action->tooltip = g_strdup ("");
	new_action->icon = g_strdup ("");
	new_action->profiles = g_hash_table_new_full (g_str_hash, g_str_equal,
						 (GDestroyNotify) g_free,
						 (GDestroyNotify) nautilus_actions_config_action_profile_free);
	g_hash_table_insert (new_action->profiles, g_strdup (NAUTILUS_ACTIONS_DEFAULT_PROFILE_NAME), nautilus_actions_config_action_profile_new_default ());
	new_action->version = g_strdup (NAUTILUS_ACTIONS_CONFIG_VERSION);
	
	return new_action;
}

void
nautilus_actions_config_action_set_uuid (NautilusActionsConfigAction *action, const gchar *uuid)
{
	g_return_if_fail (action != NULL);

	if (action->uuid)
	{
		g_free (action->uuid);
	}
	action->uuid = g_strdup (uuid);

	if (action->conf_section)
	{
		g_free (action->conf_section);
	}
	action->conf_section = g_strdup_printf ("%s/%s", ACTIONS_CONFIG_DIR, uuid);
}

void
nautilus_actions_config_action_set_label (NautilusActionsConfigAction *action, const gchar *label)
{
	g_return_if_fail (action != NULL);

	if (action->label)
		g_free (action->label);
	action->label = g_strdup (label);
}

void
nautilus_actions_config_action_set_tooltip (NautilusActionsConfigAction *action, const gchar *tooltip)
{
	g_return_if_fail (action != NULL);

	if (action->tooltip)
		g_free (action->tooltip);
	action->tooltip = g_strdup (tooltip);
}

void
nautilus_actions_config_action_set_icon (NautilusActionsConfigAction *action, const gchar *icon)
{
	g_return_if_fail (action != NULL);

	if (action->icon)
		g_free (action->icon);
	action->icon = g_strdup (icon);
}

void
nautilus_actions_config_action_profile_set_path (NautilusActionsConfigActionProfile *action_profile, const gchar *path)
{
	g_return_if_fail (action_profile != NULL);

	if (action_profile->path)
		g_free (action_profile->path);
	action_profile->path = g_strdup (path);
}

void
nautilus_actions_config_action_profile_set_parameters (NautilusActionsConfigActionProfile *action_profile, const gchar *parameters)
{
	g_return_if_fail (action_profile != NULL);

	if (action_profile->parameters)
		g_free (action_profile->parameters);
	action_profile->parameters = g_strdup (parameters);
}

static void 
copy_list (gchar* data, GSList** list)
{
	(*list) = g_slist_append ((*list), g_strdup (data));
}

static void 
copy_list_strdown (gchar* data, GSList** list)
{
	// make sure that the elements are copied with their case lowered
	(*list) = g_slist_append ((*list), g_ascii_strdown (data, strlen (data)));
}

void
nautilus_actions_config_action_profile_set_basenames (NautilusActionsConfigActionProfile *action_profile, GSList *basenames)
{
	g_return_if_fail (action_profile != NULL);

	g_slist_foreach (action_profile->basenames, (GFunc) g_free, NULL);
	g_slist_free (action_profile->basenames);
	action_profile->basenames = NULL;
	g_slist_foreach (basenames, (GFunc) copy_list, &(action_profile->basenames));
}

void nautilus_actions_config_action_profile_set_mimetypes (NautilusActionsConfigActionProfile *action_profile, GSList *mimetypes)
{
	g_return_if_fail (action_profile != NULL);

	g_slist_foreach (action_profile->mimetypes, (GFunc) g_free, NULL);
	g_slist_free (action_profile->mimetypes);
	action_profile->mimetypes = NULL;
	g_slist_foreach (mimetypes, (GFunc) copy_list_strdown, &(action_profile->mimetypes));
}


void
nautilus_actions_config_action_profile_set_schemes (NautilusActionsConfigActionProfile *action_profile, GSList *schemes)
{
	g_return_if_fail (action_profile != NULL);

	g_slist_foreach (action_profile->schemes, (GFunc) g_free, NULL);
	g_slist_free (action_profile->schemes);
	action_profile->schemes = NULL;
	g_slist_foreach (schemes, (GFunc) copy_list_strdown, &(action_profile->schemes));
}

NautilusActionsConfigActionProfile* nautilus_actions_config_action_profile_dup (NautilusActionsConfigActionProfile *action_profile)
{
	NautilusActionsConfigActionProfile* new_action_profile = NULL;
	gboolean success = TRUE;
	GSList* iter;

	if (action_profile != NULL)
	{
		new_action_profile = nautilus_actions_config_action_profile_new ();

		if (action_profile->path) {
			new_action_profile->path = g_strdup (action_profile->path);
		}
		else
		{
			success = FALSE;
		}

		if (action_profile->parameters && success) {
			new_action_profile->parameters = g_strdup (action_profile->parameters);
		}
		else
		{
			success = FALSE;
		}

		if (action->basenames && success) {
			for (iter = action_profile->basenames; iter; iter = iter->next)
			{
				new_action_profile->basenames = g_slist_append (new_action_profile->basenames, g_strdup ((gchar*)iter->data));
			}
		}
		
		new_action_profile->match_case = action_profile->match_case;

		if (action_profile->mimetypes && success) {
			for (iter = action_profile->mimetypes; iter; iter = iter->next)
			{
				new_action_profile->mimetypes = g_slist_append (new_action_profile->mimetypes, g_strdup ((gchar*)iter->data));
			}
		}

		new_action_profile->is_file = action_profile->is_file;
		new_action_profile->is_dir = action_profile->is_dir;
		new_action_profile->accept_multiple_files = action_profile->accept_multiple_files;

		if (action_profile->schemes && success) {
			for (iter = action_profile->schemes; iter; iter = iter->next)
			{
				new_action_profile->schemes = g_slist_append (new_action_profile->schemes, g_strdup ((gchar*)iter->data));
			}
		}

	}
	else
	{
		success = FALSE;
	}

	if (!success)
	{
		nautilus_actions_config_action_profile_free (new_action_profile);
		new_action_profile = NULL;
	}

	return new_action_profile;
}

void
nautilus_actions_config_action_profile_free (NautilusActionsConfigActionProfile *action_profile)
{
	if (action_profile != NULL)
	{
		if (action_profile->path) {
			g_free (action_profile->path);
			action_profile->path = NULL;
		}

		if (action_profile->parameters) {
			g_free (action_profile->parameters);
			action_profile->parameters = NULL;
		}

		if (action_profile->basenames) {
			g_slist_foreach (action_profile->basenames, (GFunc) g_free, NULL);
			g_slist_free (action_profile->basenames);
			action_profile->basenames = NULL;
		}

		if (action_profile->mimetypes) {
			g_slist_foreach (action_profile->mimetypes, (GFunc) g_free, NULL);
			g_slist_free (action_profile->mimetypes);
			action_profile->mimetypes = NULL;
		}

		if (action_profile->schemes) {
			g_slist_foreach (action_profile->schemes, (GFunc) g_free, NULL);
			g_slist_free (action_profile->schemes);
			action_profile->schemes = NULL;
		}

		g_free (action_profile);
		action_profile = NULL;
	}
}

static void duplicate_profile_hash_values (gchar* profile_name, NautilusActionsConfigActionProfile* action_profile, NautilusActionsConfigAction** new_action)
{
	if (new_action != NULL && (*new_action) != NULL && action_profile != NULL)
	{
		g_hash_table_insert ((*new_action)->profiles, g_strdup (profile_name), nautilus_actions_config_action_profile_dup (action_profile));
	}
}

NautilusActionsConfigAction* nautilus_actions_config_action_dup (NautilusActionsConfigAction *action)
{
	NautilusActionsConfigAction* new_action = NULL;
	gboolean success = TRUE;
	GSList* iter;

	if (action != NULL)
	{
		new_action = nautilus_actions_config_action_new ();
		if (action->conf_section) {
			new_action->conf_section = g_strdup (action->conf_section);
		}
		else
		{
			success = FALSE;
		}

		if (action->uuid) {
			new_action->uuid = g_strdup (action->uuid);
		}
		else
		{
			success = FALSE;
		}

		if (action->label && success) {
			new_action->label = g_strdup (action->label);
		}
		else
		{
			success = FALSE;
		}

		if (action->tooltip && success) {
			new_action->tooltip = g_strdup (action->tooltip);
		}
		else
		{
			success = FALSE;
		}

		if (action->icon && success) {
			new_action->icon = g_strdup (action->icon);
		}
		else
		{
			success = FALSE;
		}

		if (action->profiles && success) {
			new_action->profiles = g_hash_table_new_full (g_str_hash, g_str_equal,
						 (GDestroyNotify) g_free,
						 (GDestroyNotify) nautilus_actions_config_action_profile_free);
			g_hash_table_foreach (action->profiles, (GHFunc) duplicate_profile_hash_values, &new_action);
		}

		if (action->version && success) {
			new_action->version = g_strdup (action->version);
		}
		else
		{
			success = FALSE;
		}
	}
	else
	{
		success = FALSE;
	}

	if (!success)
	{
		nautilus_actions_config_action_free (new_action);
		new_action = NULL;
	}

	return new_action;
}

NautilusActionsConfigAction *nautilus_actions_config_action_dup_new (NautilusActionsConfigAction *action)
{
	NautilusActionsConfigAction* new_action = NULL;

	if ((new_action = nautilus_actions_config_action_dup (action)) != NULL)
	{
		gchar* uuid = get_new_uuid ();
		nautilus_actions_config_action_set_uuid (new_action, uuid);
		g_free (uuid);
	}

	return new_action;
}

void
nautilus_actions_config_action_free (NautilusActionsConfigAction *action)
{
	if (action != NULL)
	{
		if (action->conf_section) {
			g_free (action->conf_section);
			action->conf_section = NULL;
		}

		if (action->uuid) {
			g_free (action->uuid);
			action->uuid = NULL;
		}

		if (action->label) {
			g_free (action->label);
			action->label = NULL;
		}

		if (action->tooltip) {
			g_free (action->tooltip);
			action->tooltip = NULL;
		}

		if (action->icon) {
			g_free (action->icon);
			action->icon = NULL;
		}

		if (action->profiles) {
			g_hash_table_destroy (action->profiles);
			action->profiles = NULL;
		}

		if (action->version) {
			g_free (action->version);
			action->version = NULL;
		}

		g_free (action);
		action = NULL;
	}
}

// vim:ts=3:sw=3:tw=1024:cin
