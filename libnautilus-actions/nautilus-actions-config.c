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

static GObjectClass *parent_class = NULL;

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
}

static void
nautilus_actions_config_init (NautilusActionsConfig *config, NautilusActionsConfigClass *klass)
{
	GSList *list, *node;

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

	// FIXME:
	*list = g_slist_append (*list, nautilus_actions_config_action_dup ((NautilusActionsConfigAction*)value));
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

gboolean
nautilus_actions_config_add_action (NautilusActionsConfig *config, NautilusActionsConfigAction *action)
{
	g_return_val_if_fail (NAUTILUS_ACTIONS_IS_CONFIG (config), FALSE);
	g_return_val_if_fail (action != NULL, FALSE);

	if (action->uuid != NULL) {
		if (g_hash_table_lookup (config->actions, action->uuid))
			return FALSE;
	} else {
		uuid_t uuid;
		gchar uuid_str[64];

		uuid_generate (uuid);
		uuid_unparse (uuid, uuid_str);
		action->uuid = g_strdup (uuid_str);
	}

	return NAUTILUS_ACTIONS_CONFIG_GET_CLASS (config)->save_action (config, action);
}

gboolean
nautilus_actions_config_update_action (NautilusActionsConfig *config, NautilusActionsConfigAction *action)
{
	g_return_val_if_fail (NAUTILUS_ACTIONS_IS_CONFIG (config), FALSE);
	g_return_val_if_fail (action != NULL, FALSE);

	if (!g_hash_table_lookup (config->actions, action->uuid))
		return FALSE;

	return NAUTILUS_ACTIONS_CONFIG_GET_CLASS(config)->save_action (config, action);
}

gboolean
nautilus_actions_config_remove_action (NautilusActionsConfig *config, const gchar *uuid)
{
	NautilusActionsConfigAction *action;

	g_return_val_if_fail (NAUTILUS_ACTIONS_IS_CONFIG (config), FALSE);
	g_return_val_if_fail (uuid != NULL, FALSE);

	if (!(action = g_hash_table_lookup (config->actions, uuid)))
		return FALSE;

	return NAUTILUS_ACTIONS_CONFIG_GET_CLASS(config)->remove_action (config, action);
}

NautilusActionsConfigAction *nautilus_actions_config_action_new (void)
{
	return g_new0 (NautilusActionsConfigAction, 1);
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
nautilus_actions_config_action_set_path (NautilusActionsConfigAction *action, const gchar *path)
{
	g_return_if_fail (action != NULL);

	if (action->path)
		g_free (action->path);
	action->path = g_strdup (path);
}

void
nautilus_actions_config_action_set_parameters (NautilusActionsConfigAction *action, const gchar *parameters)
{
	g_return_if_fail (action != NULL);

	if (action->parameters)
		g_free (action->parameters);
	action->parameters = g_strdup (parameters);
}

static void 
copy_list (gchar* data, GSList** list)
{
	(*list) = g_slist_append ((*list), g_strdup (data));
}

void
nautilus_actions_config_action_set_basenames (NautilusActionsConfigAction *action, GSList *basenames)
{
	g_return_if_fail (action != NULL);

	g_slist_foreach (action->basenames, (GFunc) g_free, NULL);
	g_slist_free (action->basenames);
	action->basenames = NULL;
	g_slist_foreach (basenames, (GFunc) copy_list, &(action->basenames));
}

void
nautilus_actions_config_action_set_schemes (NautilusActionsConfigAction *action, GSList *schemes)
{
	g_return_if_fail (action != NULL);

	g_slist_foreach (action->schemes, (GFunc) g_free, NULL);
	g_slist_free (action->schemes);
	action->schemes = NULL;
	g_slist_foreach (schemes, (GFunc) copy_list, &(action->schemes));
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

		if (action->path && success) {
			new_action->path = g_strdup (action->path);
		}
		else
		{
			success = FALSE;
		}

		if (action->parameters && success) {
			new_action->parameters = g_strdup (action->parameters);
		}
		else
		{
			success = FALSE;
		}

		if (action->basenames && success) {
			for (iter = action->basenames; iter; iter = iter->next)
			{
				new_action->basenames = g_slist_append (new_action->basenames, g_strdup ((gchar*)iter->data));
			}
		}
		else
		{
			success = FALSE;
		}

		new_action->is_file = action->is_file;
		new_action->is_dir = action->is_dir;
		new_action->accept_multiple_files = action->accept_multiple_files;

		if (action->schemes && success) {
			for (iter = action->schemes; iter; iter = iter->next)
			{
				new_action->schemes = g_slist_append (new_action->schemes, g_strdup ((gchar*)iter->data));
			}
		}
		else
		{
			success = FALSE;
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

void
nautilus_actions_config_action_free (NautilusActionsConfigAction *action)
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

	if (action->path) {
		g_free (action->path);
		action->path = NULL;
	}

	if (action->parameters) {
		g_free (action->parameters);
		action->parameters = NULL;
	}

	if (action->basenames) {
		g_slist_foreach (action->basenames, (GFunc) g_free, NULL);
		g_slist_free (action->basenames);
		action->basenames = NULL;
	}

	if (action->schemes) {
		g_slist_foreach (action->schemes, (GFunc) g_free, NULL);
		g_slist_free (action->schemes);
		action->schemes = NULL;
	}

	if (action->version) {
		g_free (action->version);
		action->version = NULL;
	}

	g_free (action);
}

// vim:ts=3:sw=3:tw=1024:cin
