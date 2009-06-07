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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-file-info.h>
#include <libgnomevfs/gnome-vfs-ops.h>

#include <libnautilus-extension/nautilus-extension-types.h>
#include <libnautilus-extension/nautilus-file-info.h>
#include <libnautilus-extension/nautilus-menu-provider.h>

#include <common/nact-action.h>
#include <common/nact-pivot.h>
#include <common/nautilus-actions-config.h>
#include <common/nautilus-actions-config-gconf-reader.h>
#include "nautilus-actions.h"
#include "nautilus-actions-test.h"
#include "nautilus-actions-utils.h"

/* private class data
 */
struct NautilusActionsClassPrivate {
};

/* private instance data
 */
struct NautilusActionsPrivate {
	gboolean   dispose_has_run;

	/* from nact-pivot */
	NactPivot *pivot;

	/* original */
	NautilusActionsConfigGconfReader* configs;
	GSList* config_list;
};

/* We have a double stage notification system :
 *
 * 1. when the storage subsystems detects a change on an action, it must
 *    emit a signal to notify us of this change ; we so have to update
 *    accordingly the list of actions we maintain
 *
 * 2. when we have successfully updated the list of actions, we have to
 *    notify nautilus to update its contextual menu ; this is left to
 *    NautilusActions class
 *
 * This same signal is then first emitted by the IIOProvider to the
 * NactPivot object which handles it. When all modifications have been
 * treated, NactPivot notifies NautilusActions which itself asks
 * Nautilus for updating its menu
 */
enum {
	ACTION_CHANGED,
	LAST_SIGNAL
};

#define SIGNAL_ACTION_CHANGED_NAME		"notify_nautilus_of_action_changed"

static GObjectClass *st_parent_class = NULL;
static GType         st_actions_type = 0;
static gint          st_signals[ LAST_SIGNAL ] = { 0 };

static void class_init( NautilusActionsClass *klass );
static void menu_provider_iface_init( NautilusMenuProviderIface *iface );
static void instance_init( GTypeInstance *instance, gpointer klass );
static void instance_dispose( GObject *object );
static void instance_finalize( GObject *object );

static GList            *get_background_items( NautilusMenuProvider *provider, GtkWidget *window, NautilusFileInfo *current_folder );
static GList            *get_file_items( NautilusMenuProvider *provider, GtkWidget *window, GList *files );
static const gchar      *get_verified_icon_name( const gchar* icon_name );
static NautilusMenuItem *create_menu_item( NautilusActionsConfigAction *action, GList *files, NautilusActionsConfigActionProfile* action_profile );
static void              execute_action( NautilusMenuItem *item, NautilusActionsConfigActionProfile *action_profile );
static void              action_changed_handler( NautilusActions *instance, gpointer user_data );
static void              action_changed_handler_old( NautilusActionsConfig* config, NautilusActionsConfigAction* action, gpointer user_data );

GType
nautilus_actions_get_type( void )
{
	g_assert( st_actions_type );
	return( st_actions_type );
}

void
nautilus_actions_register_type( GTypeModule *module )
{
	static const gchar *thisfn = "nautilus_actions_register_type";
	g_debug( "%s: module=%p", thisfn, module );

	g_assert( st_actions_type == 0 );

	static const GTypeInfo info = {
		sizeof( NautilusActionsClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NautilusActions ),
		0,
		( GInstanceInitFunc ) instance_init,
	};

	st_actions_type = g_type_module_register_type( module, G_TYPE_OBJECT, "NautilusActions", &info, 0 );

	static const GInterfaceInfo menu_provider_iface_info = {
		( GInterfaceInitFunc ) menu_provider_iface_init,
		NULL,
		NULL
	};

	g_type_module_add_interface( module, st_actions_type, NAUTILUS_TYPE_MENU_PROVIDER, &menu_provider_iface_info );
}

static void
class_init( NautilusActionsClass *klass )
{
	static const gchar *thisfn = "nautilus_actions_class_init";
	g_debug( "%s: klass=%p", thisfn, klass );

	st_parent_class = g_type_class_peek_parent( klass );

	GObjectClass *gobject_class = G_OBJECT_CLASS( klass );
	gobject_class->dispose = instance_dispose;
	gobject_class->finalize = instance_finalize;

	klass->private = g_new0( NautilusActionsClassPrivate, 1 );

	/* we could have set a default handler here, which have been
	 * avoided us to connect to the signal ; but a default handler is
	 * addressed via a class structure offset, and thus cannot work
	 * when defined in a private structure
	 *
	 * the previous point applies to g_signal_new
	 * g_signal_new_class_handler let us specify a standard C callback
	 */
	st_signals[ ACTION_CHANGED ] = g_signal_new_class_handler(
				SIGNAL_ACTION_CHANGED_NAME,
				G_TYPE_FROM_CLASS( klass ),
				G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
				( GCallback ) action_changed_handler,
				NULL,
				NULL,
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE,
				0
	);
}

static void
menu_provider_iface_init( NautilusMenuProviderIface *iface )
{
	static const gchar *thisfn = "nautilus_actions_menu_provider_iface_init";
	g_debug( "%s: iface=%p", thisfn, iface );

	iface->get_file_items = get_file_items;
	iface->get_background_items = get_background_items;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nautilus_actions_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );

	g_assert( NAUTILUS_IS_ACTIONS( instance ));
	NautilusActions *self = NAUTILUS_ACTIONS( instance );

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

	self->private = g_new0( NautilusActionsPrivate, 1 );

	/* from nact-pivot */
	self->private->pivot = nact_pivot_new( G_OBJECT( self ));

	/* see nautilus_actions_class_init for why we have to connect an
	 * handler to our signal instead of relying on default handler
	 * see also nautilus_actions_class_init for why g_signal_connect is
	 * no more needed
	 */
	/*g_signal_connect(
			G_OBJECT( self ),
			SIGNAL_ACTION_CHANGED_NAME,
			( GCallback ) action_changed_handler,
			NULL
	);*/

	self->private->configs = NULL;
	self->private->configs = nautilus_actions_config_gconf_reader_get ();
	self->private->config_list = NULL;
	self->private->config_list = nautilus_actions_config_get_actions (NAUTILUS_ACTIONS_CONFIG (self->private->configs));
	self->private->dispose_has_run = FALSE;

	g_signal_connect_after(
			G_OBJECT( self->private->configs ),
			"action_added",
			( GCallback ) action_changed_handler_old,
			self
	);
	g_signal_connect_after(
			G_OBJECT( self->private->configs ),
			"action_changed_old",
			( GCallback ) action_changed_handler_old,
			self
	);
	g_signal_connect_after(
			G_OBJECT( self->private->configs ),
			"action_removed",
			( GCallback ) action_changed_handler_old,
			self
	);
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "nautilus_actions_instance_dispose";
	g_debug( "%s: object=%p", thisfn, object );

	g_assert( NAUTILUS_IS_ACTIONS( object ));
	NautilusActions *self = NAUTILUS_ACTIONS( object );

	if( !self->private->dispose_has_run ){
		self->private->dispose_has_run = TRUE;

		g_object_unref( self->private->pivot );
		g_object_unref( self->private->configs );

		/* chain up to the parent class */
		G_OBJECT_CLASS( st_parent_class )->dispose( object );
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "nautilus_actions_instance_finalize";
	g_debug( "%s: object=%p", thisfn, object );

	g_assert( NAUTILUS_IS_ACTIONS( object ));
	/*NautilusActions* self = NAUTILUS_ACTIONS (obj);*/

	/* chain up to the parent class */
	G_OBJECT_CLASS( st_parent_class )->finalize( object );
}

#ifndef HAVE_NAUTILUS_MENU_PROVIDER_EMIT_ITEMS_UPDATED_SIGNAL
static void nautilus_menu_provider_emit_items_updated_signal (NautilusMenuProvider *provider)
{
	/* -> fake function for backward compatibility
	 * -> do nothing
	 */
}
#endif

/*
 * this function is called when nautilus has to paint a folder background
 * one of the first calls is with current_folder = 'x-nautilus-desktop:///'
 * we have nothing to do here ; the function is left as a placeholder
 * (and as an historic remainder)
 */
static GList *
get_background_items( NautilusMenuProvider *provider, GtkWidget *window, NautilusFileInfo *current_folder )
{
#ifdef NACT_MAINTAINER_MODE
	static const gchar *thisfn = "nautilus_actions_get_background_items";
	gchar *uri = nautilus_file_info_get_uri( current_folder );
	g_debug( "%s: provider=%p, window=%p, current_folder=%p (%s)", thisfn, provider, window, current_folder, uri );
	g_free( uri );
#endif
	return(( GList * ) NULL );
}

static GList *
get_file_items( NautilusMenuProvider *provider, GtkWidget *window, GList *files )
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

	if (!self->private->dispose_has_run)
	{
		for (iter = self->private->config_list; iter; iter = iter->next)
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
					item = create_menu_item (action, files, action_profile);
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

static const gchar *
get_verified_icon_name( const gchar* icon_name )
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

static NautilusMenuItem *
create_menu_item( NautilusActionsConfigAction *action, GList *files, NautilusActionsConfigActionProfile* action_profile )
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
				G_CALLBACK (execute_action),
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

static void
execute_action( NautilusMenuItem *item, NautilusActionsConfigActionProfile *action_profile )
{
	static const gchar *thisfn = "nautilus_actions_execute_action";
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

static void
action_changed_handler( NautilusActions *self, gpointer user_data )
{
	static const gchar *thisfn = "nautilus_actions_action_changed_handler";
	g_debug( "%s: self=%p, user_data=%p", thisfn, self, user_data );

	g_return_if_fail( NAUTILUS_IS_ACTIONS( self ));

	if( !self->private->dispose_has_run ){

		nautilus_menu_provider_emit_items_updated_signal( NAUTILUS_MENU_PROVIDER( self ));

		/*nautilus_actions_config_free_actions_list (self->private->config_list);
		self->private->config_list = nautilus_actions_config_get_actions (NAUTILUS_ACTIONS_CONFIG (self->private->configs));*/
	}
}

static void
action_changed_handler_old( NautilusActionsConfig* config,
						NautilusActionsConfigAction* action,
						gpointer user_data )
{
	static const gchar *thisfn = "nautilus_actions_action_changed_handler_old";
	g_debug( "%s", thisfn );

	NautilusActions* self = NAUTILUS_ACTIONS (user_data);

	g_return_if_fail (NAUTILUS_IS_ACTIONS (self));

	if (!self->private->dispose_has_run)
	{
		nautilus_menu_provider_emit_items_updated_signal(( NautilusMenuProvider * ) self );

		nautilus_actions_config_free_actions_list (self->private->config_list);
		self->private->config_list = nautilus_actions_config_get_actions (NAUTILUS_ACTIONS_CONFIG (self->private->configs));
	}
}
