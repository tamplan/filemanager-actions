/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009 Pierre Wieser and others (see AUTHORS)
 *
 * This Program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This Program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this Library; see the file COPYING.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors:
 *   Frederic Ruaudel <grumz@grumz.net>
 *   Rodrigo Moya <rodrigo@gnome-db.org>
 *   Pierre Wieser <pwieser@trychlos.org>
 *   ... and many others (see AUTHORS)
 */

#include <config.h>
#include <uuid/uuid.h>
#include <string.h>
#include "nautilus-actions-config-gconf-reader.h"
#include "nautilus-actions-config-gconf-private.h"

static GObjectClass *parent_class = NULL;

static gboolean
save_action (NautilusActionsConfig *self, NautilusActionsConfigAction *action)
{
	g_return_val_if_fail (NAUTILUS_ACTIONS_IS_CONFIG_GCONF_READER (self), FALSE);

	/* Nothing to do, it has already been done by the GConf notification handler */

	/* Return TRUE to allow the signals to be emited */
	return TRUE;
}

static gboolean
remove_action (NautilusActionsConfig *self, NautilusActionsConfigAction* action)
{
	g_return_val_if_fail (NAUTILUS_ACTIONS_IS_CONFIG_GCONF_READER (self), FALSE);

	/* Nothing to do, it has already been done by the GConf notification handler */

	/* Return TRUE to allow the signals to be emited */
	return TRUE;
}

static void
nautilus_actions_config_gconf_reader_finalize (GObject *object)
{
	NautilusActionsConfigGconfReader *config = NAUTILUS_ACTIONS_CONFIG_GCONF_READER (object);

	g_return_if_fail (NAUTILUS_ACTIONS_IS_CONFIG_GCONF_READER (config));

	/* free all memory */
	if (NAUTILUS_ACTIONS_CONFIG_GCONF (config)->conf_client) {
		gconf_client_remove_dir (NAUTILUS_ACTIONS_CONFIG_GCONF (config)->conf_client, ACTIONS_CONFIG_DIR, NULL);
		gconf_client_notify_remove (NAUTILUS_ACTIONS_CONFIG_GCONF (config)->conf_client, config->actions_notify_id);
	}

	/* chain call to parent class */
	if (parent_class->finalize)
	{
		parent_class->finalize (object);
	}
}

static void
nautilus_actions_config_gconf_reader_class_init (NautilusActionsConfigGconfReaderClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	NautilusActionsConfigClass* config_class;

	parent_class = g_type_class_peek_parent (klass);
	config_class = NAUTILUS_ACTIONS_CONFIG_CLASS (klass);


	object_class->finalize = nautilus_actions_config_gconf_reader_finalize;

	config_class->save_action = save_action;
	config_class->remove_action = remove_action;
}

/*
static gchar* get_action_profile_name_from_key (const gchar* key, const gchar* uuid)
{
	gchar* prefix = g_strdup_printf ("%s/%s/%s", ACTIONS_CONFIG_DIR, uuid, ACTIONS_PROFILE_PREFIX);
	gchar* profile_name = NULL;

	if (g_str_has_prefix (key, prefix))
	{

		profile_name = g_strdup (key + strlen (prefix));
		gchar* pos = g_strrstr (profile_name, "/");
		if (pos != NULL)
		{
			*pos = '\0';
		}
	}

	g_free (prefix);

	return profile_name;
}
*/

/*
static void
copy_list (GConfValue* value, GSList** list)
{
	   (*list) = g_slist_append ((*list), g_strdup (gconf_value_get_string (value)));
}
*/

static void
actions_changed_cb (GConfClient *client,
		    guint cnxn_id,
		    GConfEntry *entry,
		    gpointer user_data)
{
	NautilusActionsConfig *config = NAUTILUS_ACTIONS_CONFIG (user_data);


	/* The Key value format is XXX:YYYY-YYYY-... where XXX is an number incremented to make key
	 * effectively changed each time and YYYY-YYYY-... is the modified uuid */
	const GConfValue* value = gconf_entry_get_value (entry);
	const gchar* notify_value = gconf_value_get_string (value);
	const gchar* uuid = notify_value + 4;

	/* Get the modified action from the internal list if any */
	NautilusActionsConfigAction *old_action = nautilus_actions_config_get_action (config, uuid);

	/* Get the new version from GConf if any */
	NautilusActionsConfigAction *new_action = nautilus_actions_config_gconf_get_action (NAUTILUS_ACTIONS_CONFIG_GCONF (config), uuid);

	/* If modified action is unknown internally */
	if (old_action == NULL)
	{
		if (new_action != NULL)
		{
			/* new action */
			nautilus_actions_config_add_action (config, new_action, NULL);
		}
		else
		{
			/* This case should not happen */
			g_assert_not_reached ();
		}
	}
	else
	{
		if (new_action != NULL)
		{
			/* action modified */
			nautilus_actions_config_update_action (config, new_action);
		}
		else
		{
			/* action removed */
			nautilus_actions_config_remove_action (config, uuid);
		}
	}

	/* Add & change handler duplicate actions before adding them,
	 * so we can free the new action
	 */
	nautilus_actions_config_action_free (new_action);
}

static void
nautilus_actions_config_gconf_reader_init (NautilusActionsConfigGconfReader *config, NautilusActionsConfigGconfReaderClass *klass)
{
	/* install notification callbacks */
	gconf_client_add_dir (NAUTILUS_ACTIONS_CONFIG_GCONF (config)->conf_client, ACTIONS_CONFIG_DIR, GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
	config->actions_notify_id = gconf_client_notify_add (NAUTILUS_ACTIONS_CONFIG_GCONF (config)->conf_client,
							     ACTIONS_CONFIG_NOTIFY_KEY,
							     (GConfClientNotifyFunc) actions_changed_cb, config,
							     NULL, NULL);
}

GType
nautilus_actions_config_gconf_reader_get_type (void)
{
   static GType type = 0;

	if (type == 0) {
		static GTypeInfo info = {
					sizeof (NautilusActionsConfigGconfReaderClass),
					(GBaseInitFunc) NULL,
					(GBaseFinalizeFunc) NULL,
					(GClassInitFunc) nautilus_actions_config_gconf_reader_class_init,
					NULL, NULL,
					sizeof (NautilusActionsConfigGconfReader),
					0,
					(GInstanceInitFunc) nautilus_actions_config_gconf_reader_init
		};
		type = g_type_register_static (NAUTILUS_ACTIONS_TYPE_CONFIG_GCONF, "NautilusActionsConfigGconfReader", &info, 0);
	}
	return type;
}

NautilusActionsConfigGconfReader *
nautilus_actions_config_gconf_reader_get (void)
{
	static NautilusActionsConfigGconfReader *config = NULL;

	/* we share one NautilusActionsConfigGconfReader object for all */
	if (!config) {
		config = g_object_new (NAUTILUS_ACTIONS_TYPE_CONFIG_GCONF_READER, NULL);
		return config;
	}

	return NAUTILUS_ACTIONS_CONFIG_GCONF_READER (g_object_ref (G_OBJECT (config)));
}
