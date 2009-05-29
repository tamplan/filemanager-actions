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
#include <string.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-file-info.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libnautilus-extension/nautilus-extension-types.h>
#include <libnautilus-extension/nautilus-file-info.h>
#include <libnautilus-extension/nautilus-menu-provider.h>
#include "nautilus-actions.h"
#include <common/nautilus-actions-config.h>
#include <common/nautilus-actions-config-gconf-reader.h>
#include "nautilus-actions-test.h"
#include "nautilus-actions-utils.h"

static GObjectClass *parent_class = NULL;
static GType actions_type = 0;

GType nautilus_actions_get_type (void)
{
	return actions_type;
}

#ifndef HAVE_NAUTILUS_MENU_PROVIDER_EMIT_ITEMS_UPDATED_SIGNAL
static void nautilus_menu_provider_emit_items_updated_signal (NautilusMenuProvider *provider)
{
	/* -> fake function for backward compatibility
	 * -> do nothing
	 */
}
#endif

static void nautilus_actions_execute (NautilusMenuItem *item, NautilusActionsConfigActionProfile *action_profile)
{
	static const gchar *thisfn = "nautilus_actions_execute";
	g_debug( "%s", thisfn );

	GList *files;
	GString *cmd;
	gchar* param = NULL;

	files = (GList*)g_object_get_data (G_OBJECT (item), "files");

	cmd = g_string_new (action_profile->path);

	param = nautilus_actions_utils_parse_parameter (action_profile->parameters, files);

	if (param != NULL)
	{
		g_string_append_printf (cmd, " %s", param);
		g_free (param);
	}

	g_spawn_command_line_async (cmd->str, NULL);
	g_debug( "%s: commande='%s'", thisfn, cmd->str );

	g_string_free (cmd, TRUE);

}

static const gchar* get_verified_icon_name (const gchar* icon_name)
{
	if (icon_name[0] == '/')
	{
		if (!g_file_test (icon_name, G_FILE_TEST_IS_REGULAR))
		{
			return NULL;
		}
	}
	else if (strlen (icon_name) == 0)
	{
		return NULL;
	}

	return icon_name;
}

static NautilusMenuItem *nautilus_actions_create_menu_item (NautilusActionsConfigAction *action, GList *files, NautilusActionsConfigActionProfile* action_profile)
{
	static const gchar *thisfn = "nautilus_actions_create_menu_item";
	g_debug( "%s", thisfn );

	NautilusMenuItem *item;
	gchar* name;
	const gchar* icon_name = get_verified_icon_name (g_strstrip (action->icon));
	NautilusActionsConfigActionProfile* action_profile4menu = nautilus_actions_config_action_profile_dup (action_profile);

	name = g_strdup_printf ("NautilusActions::%s", action->uuid);

	item = nautilus_menu_item_new (name,
				action->label,
				action->tooltip,
				icon_name);

	g_signal_connect_data (item,
				"activate",
				G_CALLBACK (nautilus_actions_execute),
				action_profile4menu,
				(GClosureNotify)nautilus_actions_config_action_profile_free,
				0);

	g_object_set_data_full (G_OBJECT (item),
			"files",
			nautilus_file_info_list_copy (files),
			(GDestroyNotify) nautilus_file_info_list_free);


	g_free (name);

	return item;
}

/*static void get_hash_keys (gchar* key, gchar* value, GSList** list)
{
	*list = g_slist_append (*list, key);
}*/

static GList *nautilus_actions_get_file_items (NautilusMenuProvider *provider, GtkWidget *window, GList *files)
{
	static const gchar *thisfn = "nautilus_actions_get_file_items";
	g_debug( "%s provider=%p, window=%p, files=%p, count=%d", thisfn, provider, window, files, g_list_length( files ));

	GList *items = NULL;
	GSList *iter;
	NautilusMenuItem *item;
	NautilusActions* self = NAUTILUS_ACTIONS (provider);
	gchar* profile_name = NULL;
	GSList* profile_list = NULL;
	GSList* iter2;
	gboolean found;

	g_return_val_if_fail (NAUTILUS_IS_ACTIONS (self), NULL);

	/* no need to go further if there is no files in the list */
	if( !g_list_length( files )){
		return(( GList * ) NULL );
	}

	if (!self->dispose_has_run)
	{
		for (iter = self->config_list; iter; iter = iter->next)
		{
			/* Foreach configured action, check if we add a menu item */
			NautilusActionsConfigAction *action = (NautilusActionsConfigAction*)iter->data;

			/* Retrieve all profile name */
			/*g_hash_table_foreach (action->profiles, (GHFunc)get_hash_keys, &profile_list);*/
			profile_list = nautilus_actions_config_action_get_all_profile_names( action );

			iter2 = profile_list;
			found = FALSE;
			while (iter2 && !found)
			{
				profile_name = (gchar*)iter2->data;
				NautilusActionsConfigActionProfile* action_profile = nautilus_actions_config_action_get_profile (action, profile_name);
				g_debug( "%s: profile='%s' (%p)", thisfn, profile_name, action_profile );

				if (nautilus_actions_test_validate (action_profile, files))
				{
					item = nautilus_actions_create_menu_item (action, files, action_profile);
					items = g_list_append (items, item);
					found = TRUE;
				}

				iter2 = iter2->next;
			}
			nautilus_actions_config_action_free_all_profile_names( profile_list );
		}
	}

	return items;
}

static GList *nautilus_actions_get_background_items (NautilusMenuProvider *provider, GtkWidget *window, NautilusFileInfo *current_folder)
{
	GList *items = NULL;
	GList *files = NULL;

	files = g_list_append (files, current_folder);
	items = nautilus_actions_get_file_items (provider, window, files);
	g_list_free (files);

	return items;
}

static void nautilus_actions_instance_dispose (GObject *obj)
{
	static const gchar *thisfn = "nautilus_actions_instance_dispose";
	g_debug( "%s: obj=%p", thisfn, obj );

	NautilusActions* self = NAUTILUS_ACTIONS (obj);

	if (!self->dispose_has_run)
	{
		self->dispose_has_run = TRUE;

		g_object_unref (self->configs);

		/* Chain up to the parent class */
		G_OBJECT_CLASS (parent_class)->dispose (obj);
	}
}

static void nautilus_actions_action_changed_handler (NautilusActionsConfig* config,
																				NautilusActionsConfigAction* action,
																				gpointer user_data)
{
	static const gchar *thisfn = "nautilus_actions_action_changed_handler";
	g_debug( "%s", thisfn );

	NautilusActions* self = NAUTILUS_ACTIONS (user_data);

	g_return_if_fail (NAUTILUS_IS_ACTIONS (self));

	if (!self->dispose_has_run)
	{
		nautilus_menu_provider_emit_items_updated_signal(( NautilusMenuProvider * ) self );

		nautilus_actions_config_free_actions_list (self->config_list);
		self->config_list = nautilus_actions_config_get_actions (NAUTILUS_ACTIONS_CONFIG (self->configs));
	}
}

static void nautilus_actions_instance_finalize (GObject* obj)
{
	static const gchar *thisfn = "nautilus_actions_instance_finalize";
	g_debug( "%s: obj=%p", thisfn, obj );

	/*NautilusActions* self = NAUTILUS_ACTIONS (obj);*/

	/* Chain up to the parent class */
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void nautilus_actions_instance_init (GTypeInstance *instance, gpointer klass)
{
	static const gchar *thisfn = "nautilus_actions_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );

	/* Patch from Bruce van der Kooij <brucevdkooij@gmail.com>
	 *
	 * TODO: GnomeVFS needs to be initialized before gnome_vfs methods
	 * can be used. Since GnomeVFS has been deprecated it would be
	 * a good idea to rewrite this extension to use equivalent methods
	 * from GIO/GVFS.
	 *
	 * plugins/nautilus-actions-utils.c:nautilus_actions_utils_parse_parameter
	 * is the only function that makes use of gnome_vfs methods.
	 *
	 * See: Bug #574919
	 */
	gnome_vfs_init ();

	NautilusActions* self = NAUTILUS_ACTIONS (instance);

	self->configs = NULL;
	self->configs = nautilus_actions_config_gconf_reader_get ();
	self->config_list = NULL;
	self->config_list = nautilus_actions_config_get_actions (NAUTILUS_ACTIONS_CONFIG (self->configs));
	self->dispose_has_run = FALSE;

	g_signal_connect_after (G_OBJECT (self->configs), "action_added",
									(GCallback)nautilus_actions_action_changed_handler,
									self);
	g_signal_connect_after (G_OBJECT (self->configs), "action_changed",
									(GCallback)nautilus_actions_action_changed_handler,
									self);
	g_signal_connect_after (G_OBJECT (self->configs), "action_removed",
									(GCallback)nautilus_actions_action_changed_handler,
									self);

	parent_class = g_type_class_peek_parent (klass);
}

static void nautilus_actions_menu_provider_iface_init (NautilusMenuProviderIface *iface)
{
	static const gchar *thisfn = "nautilus_actions_menu_provider_iface_init";
	g_debug( "%s: iface=%p", thisfn, iface );

	iface->get_file_items = nautilus_actions_get_file_items;
	iface->get_background_items = nautilus_actions_get_background_items;
}

static void nautilus_actions_class_init (NautilusActionsClass *actions_class)
{
	static const gchar *thisfn = "nautilus_actions_class_init";
	g_debug( "%s: action_class=%p", thisfn, actions_class );

	GObjectClass *gobject_class = G_OBJECT_CLASS (actions_class);

	gobject_class->dispose = nautilus_actions_instance_dispose;
	gobject_class->finalize = nautilus_actions_instance_finalize;
}

void nautilus_actions_register_type (GTypeModule *module)
{
	static const gchar *thisfn = "nautilus_actions_register_type";
	g_debug( "%s: module=%p", thisfn, module );

	static const GTypeInfo info = {
		sizeof (NautilusActionsClass),
		(GBaseInitFunc) NULL,
		(GBaseFinalizeFunc) NULL,
		(GClassInitFunc) nautilus_actions_class_init,
		NULL,
		NULL,
		sizeof (NautilusActions),
		0,
		(GInstanceInitFunc)nautilus_actions_instance_init,
	};

	static const GInterfaceInfo menu_provider_iface_info = {
		(GInterfaceInitFunc) nautilus_actions_menu_provider_iface_init,
		NULL,
		NULL
	};

	actions_type = g_type_module_register_type (module,
								G_TYPE_OBJECT,
								"NautilusActions",
								&info, 0);

	g_type_module_add_interface (module,
								actions_type,
								NAUTILUS_TYPE_MENU_PROVIDER,
								&menu_provider_iface_info);
}
