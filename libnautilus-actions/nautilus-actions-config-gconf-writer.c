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
#include "nautilus-actions-config-gconf-writer.h"
#include "nautilus-actions-config-gconf-private.h"

static GObjectClass *parent_class = NULL;

static gboolean
save_action (NautilusActionsConfig *self, NautilusActionsConfigAction *action)
{
	g_return_val_if_fail (NAUTILUS_ACTIONS_IS_CONFIG_GCONF_WRITER (self), FALSE);

	NautilusActionsConfigGconf* config = NAUTILUS_ACTIONS_CONFIG_GCONF (self);
	gchar *key;

	g_free (action->conf_section);
	action->conf_section = g_strdup_printf ("%s/%s", ACTIONS_CONFIG_DIR, action->uuid);

	/* set the version on the action */
	if (action->version)
		g_free (action->version);
	action->version = g_strdup (NAUTILUS_ACTIONS_CONFIG_VERSION);

	/* set the values in the config database */
	key = g_strdup_printf ("%s/%s", action->conf_section, ACTION_LABEL_ENTRY);
	gconf_client_set_string (config->conf_client, key, action->label, NULL);
	g_free (key);

	key = g_strdup_printf ("%s/%s", action->conf_section, ACTION_TOOLTIP_ENTRY);
	gconf_client_set_string (config->conf_client, key, action->tooltip, NULL);
	g_free (key);

	key = g_strdup_printf ("%s/%s", action->conf_section, ACTION_ICON_ENTRY);
	gconf_client_set_string (config->conf_client, key, action->icon, NULL);
	g_free (key);

	key = g_strdup_printf ("%s/%s", action->conf_section, ACTION_PATH_ENTRY);
	gconf_client_set_string (config->conf_client, key, action->path, NULL);
	g_free (key);

	key = g_strdup_printf ("%s/%s", action->conf_section, ACTION_PARAMS_ENTRY);
	gconf_client_set_string (config->conf_client, key, action->parameters, NULL);
	g_free (key);

	key = g_strdup_printf ("%s/%s", action->conf_section, ACTION_BASENAMES_ENTRY);
	gconf_client_set_list (config->conf_client, key, GCONF_VALUE_STRING, action->basenames, NULL);
	g_free (key);

	key = g_strdup_printf ("%s/%s", action->conf_section, ACTION_ISFILE_ENTRY);
	gconf_client_set_bool (config->conf_client, key, action->is_file, NULL);
	g_free (key);

	key = g_strdup_printf ("%s/%s", action->conf_section, ACTION_ISDIR_ENTRY);
	gconf_client_set_bool (config->conf_client, key, action->is_dir, NULL);
	g_free (key);

	key = g_strdup_printf ("%s/%s", action->conf_section, ACTION_MULTIPLE_ENTRY);
	gconf_client_set_bool (config->conf_client, key, action->accept_multiple_files, NULL);
	g_free (key);

	key = g_strdup_printf ("%s/%s", action->conf_section, ACTION_SCHEMES_ENTRY);
	gconf_client_set_list (config->conf_client, key, GCONF_VALUE_STRING, action->schemes, NULL);
	g_free (key);

	key = g_strdup_printf ("%s/%s", action->conf_section, ACTION_VERSION_ENTRY);
	gconf_client_set_string (config->conf_client, key, action->version, NULL);
	g_free (key);

	return TRUE;
}

static gboolean
remove_action (NautilusActionsConfig *self, NautilusActionsConfigAction* action)
{
	g_return_val_if_fail (NAUTILUS_ACTIONS_IS_CONFIG_GCONF_WRITER (self), FALSE);

	NautilusActionsConfigGconf* config = NAUTILUS_ACTIONS_CONFIG_GCONF (self);

	return gconf_client_recursive_unset (config->conf_client, action->conf_section, 0, NULL);
}

static void
nautilus_actions_config_gconf_writer_finalize (GObject *object)
{
	NautilusActionsConfigGconfWriter *config = NAUTILUS_ACTIONS_CONFIG_GCONF_WRITER (object);

	g_return_if_fail (NAUTILUS_ACTIONS_IS_CONFIG_GCONF_WRITER (config));

	/* chain call to parent class */
	if (parent_class->finalize)
	{
		parent_class->finalize (object);
	}
}

static void
nautilus_actions_config_gconf_writer_class_init (NautilusActionsConfigGconfWriterClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	NautilusActionsConfigClass* config_class;

	parent_class = g_type_class_peek_parent (klass);
	config_class = NAUTILUS_ACTIONS_CONFIG_CLASS (klass);


	object_class->finalize = nautilus_actions_config_gconf_writer_finalize;

	config_class->save_action = save_action;
	config_class->remove_action = remove_action;
}

static void
nautilus_actions_config_gconf_writer_init (NautilusActionsConfigGconfWriter *config, NautilusActionsConfigGconfWriterClass *klass)
{
}

GType
nautilus_actions_config_gconf_writer_get_type (void)
{
   static GType type = 0;

	if (type == 0) {
		static GTypeInfo info = {
					sizeof (NautilusActionsConfigGconfWriterClass),
					(GBaseInitFunc) NULL,
					(GBaseFinalizeFunc) NULL,
					(GClassInitFunc) nautilus_actions_config_gconf_writer_class_init,
					NULL, NULL,
					sizeof (NautilusActionsConfigGconfWriter),
					0,
					(GInstanceInitFunc) nautilus_actions_config_gconf_writer_init
		};
		type = g_type_register_static (NAUTILUS_ACTIONS_TYPE_CONFIG_GCONF, "NautilusActionsConfigGconfWriter", &info, 0);
	}
	return type;
}

NautilusActionsConfigGconfWriter *
nautilus_actions_config_gconf_writer_get (void)
{
	static NautilusActionsConfigGconfWriter *config = NULL;

	/* we share one NautilusActionsConfigGconfWriter object for all */
	if (!config) {
		config = g_object_new (NAUTILUS_ACTIONS_TYPE_CONFIG_GCONF_WRITER, NULL);
		return config;
	}

	return NAUTILUS_ACTIONS_CONFIG_GCONF_WRITER (g_object_ref (G_OBJECT (config)));
}
// vim:ts=3:sw=3:tw=1024:cin
