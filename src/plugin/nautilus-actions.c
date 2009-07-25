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

#include <libnautilus-extension/nautilus-extension-types.h>
#include <libnautilus-extension/nautilus-file-info.h>
#include <libnautilus-extension/nautilus-menu-provider.h>

#include <common/na-action.h>
#include <common/na-action-profile.h>
#include <common/na-pivot.h>
#include <common/na-ipivot-consumer.h>

#include "nautilus-actions.h"

/* private class data
 */
struct NautilusActionsClassPrivate {
};

/* private instance data
 */
struct NautilusActionsPrivate {
	gboolean dispose_has_run;
	NAPivot *pivot;
};

static GObjectClass *st_parent_class = NULL;
static GType         st_actions_type = 0;

static void              class_init( NautilusActionsClass *klass );
static void              menu_provider_iface_init( NautilusMenuProviderIface *iface );
static void              pivot_consumer_iface_init( NAIPivotConsumerInterface *iface );
static void              instance_init( GTypeInstance *instance, gpointer klass );
static void              instance_dispose( GObject *object );
static void              instance_finalize( GObject *object );

static GList            *get_background_items( NautilusMenuProvider *provider, GtkWidget *window, NautilusFileInfo *current_folder );
static GList            *get_file_items( NautilusMenuProvider *provider, GtkWidget *window, GList *files );
static NautilusMenuItem *create_menu_item( NAAction *action, NAActionProfile *profile, GList *files );
static void              execute_action( NautilusMenuItem *item, NAActionProfile *profile );
static void              actions_changed_handler( NAIPivotConsumer *instance, gpointer user_data );

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

	/* implements NautilusMenuItem interface
	 */
	st_actions_type = g_type_module_register_type( module, G_TYPE_OBJECT, "NautilusActions", &info, 0 );

	static const GInterfaceInfo menu_provider_iface_info = {
		( GInterfaceInitFunc ) menu_provider_iface_init,
		NULL,
		NULL
	};

	g_type_module_add_interface( module, st_actions_type, NAUTILUS_TYPE_MENU_PROVIDER, &menu_provider_iface_info );

	/* implement IPivotConsumer interface
	 */
	static const GInterfaceInfo pivot_consumer_iface_info = {
		( GInterfaceInitFunc ) pivot_consumer_iface_init,
		NULL,
		NULL
	};

	g_type_module_add_interface( module, st_actions_type, NA_IPIVOT_CONSUMER_TYPE, &pivot_consumer_iface_info );
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
pivot_consumer_iface_init( NAIPivotConsumerInterface *iface )
{
	static const gchar *thisfn = "nautilus_actions_pivot_consumer_iface_init";
	g_debug( "%s: iface=%p", thisfn, iface );

	iface->on_actions_changed = actions_changed_handler;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nautilus_actions_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );

	g_assert( NAUTILUS_IS_ACTIONS( instance ));
	NautilusActions *self = NAUTILUS_ACTIONS( instance );

	self->private = g_new0( NautilusActionsPrivate, 1 );
	self->private->dispose_has_run = FALSE;

	/* from na-pivot */
	self->private->pivot = na_pivot_new( G_OBJECT( self ));
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
	NautilusActions* self = NAUTILUS_ACTIONS( object );

	g_free( self->private );

	/* chain up to the parent class */
	G_OBJECT_CLASS( st_parent_class )->finalize( object );
}

/*
 * This function notifies Nautilus file manager that the context menu
 * items may have changed, and that it should reload them.
 *
 * Patch has been provided by Frederic Ruaudel, the initial author of
 * Nautilus-Actions, and applied on Nautilus 2.15.4 development branch
 * on 2006-06-16. It was released with Nautilus 2.16.0
 */
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
	g_debug( "%s: provider=%p, window=%p, files=%p, count=%d", thisfn, provider, window, files, g_list_length( files ));

	GList *items = NULL;
	GSList* profiles;
	GSList *ia, *ip;
	NautilusMenuItem *item;
	GSList *actions = NULL;
#ifdef NACT_MAINTAINER_MODE
	gchar *debug_label;
#endif

	g_return_val_if_fail( NAUTILUS_IS_ACTIONS( provider ), NULL );
	NautilusActions *self = NAUTILUS_ACTIONS( provider );

	/* no need to go further if there is no files in the list */
	if( !g_list_length( files )){
		return(( GList * ) NULL );
	}

	if( !self->private->dispose_has_run ){
		actions = na_pivot_get_actions( self->private->pivot );

		for( ia = actions ; ia ; ia = ia->next ){

			NAAction *action = NA_ACTION( ia->data );

#ifdef NACT_MAINTAINER_MODE
			debug_label = na_action_get_label( action );
			g_debug( "%s: examining '%s' action", thisfn, debug_label );
			g_free( debug_label );
#endif

			profiles = na_action_get_profiles( action );

			for( ip = profiles ; ip ; ip = ip->next ){

				NAActionProfile *profile = NA_ACTION_PROFILE( ip->data );

#ifdef NACT_MAINTAINER_MODE
				debug_label = na_action_profile_get_label( profile );
				g_debug( "%s: examining '%s' profile", thisfn, debug_label );
				g_free( debug_label );
#endif

				if( na_action_profile_is_candidate( profile, files )){
					item = create_menu_item( action, profile, files );
					items = g_list_append( items, item );
					break;
				}
			}
		}
	}

	return( items );
}

static NautilusMenuItem *
create_menu_item( NAAction *action, NAActionProfile *profile, GList *files )
{
	static const gchar *thisfn = "nautilus_actions_create_menu_item";
	g_debug( "%s", thisfn );

	NautilusMenuItem *item;

	gchar *uuid = na_action_get_uuid( action );
	gchar *name = g_strdup_printf( "NautilusActions::%s", uuid );
	gchar *label = na_action_get_label( action );
	gchar *tooltip = na_action_get_tooltip( action );
	gchar* icon_name = na_action_get_verified_icon_name( action );

	NAActionProfile *dup4menu = NA_ACTION_PROFILE( na_object_duplicate( NA_OBJECT( profile )));

	item = nautilus_menu_item_new( name, label, tooltip, icon_name );

	g_signal_connect_data( item,
				"activate",
				G_CALLBACK( execute_action ),
				dup4menu,
				( GClosureNotify ) g_object_unref,
				0
	);

	g_object_set_data_full( G_OBJECT( item ),
			"files",
			nautilus_file_info_list_copy( files ),
			( GDestroyNotify ) nautilus_file_info_list_free
	);

	g_free( icon_name );
	g_free( tooltip );
	g_free( label );
	g_free( name );
	g_free( uuid );

	return( item );
}

static void
execute_action( NautilusMenuItem *item, NAActionProfile *profile )
{
	static const gchar *thisfn = "nautilus_actions_execute_action";
	g_debug( "%s: item=%p, profile=%p", thisfn, item, profile );

	GList *files;
	GString *cmd;
	gchar *param, *path;

	files = ( GList* ) g_object_get_data( G_OBJECT( item ), "files" );

	path = na_action_profile_get_path( profile );
	cmd = g_string_new( path );

	param = na_action_profile_parse_parameters( profile, files );

	if( param != NULL ){
		g_string_append_printf( cmd, " %s", param );
		g_free( param );
	}

	g_debug( "%s: executing '%s'", thisfn, cmd->str );
	g_spawn_command_line_async( cmd->str, NULL );

	g_string_free (cmd, TRUE);
	g_free( path );

}

static void
actions_changed_handler( NAIPivotConsumer *instance, gpointer user_data )
{
	static const gchar *thisfn = "nautilus_actions_actions_changed_handler";
	g_debug( "%s: instance=%p, user_data=%p", thisfn, instance, user_data );

	g_assert( NAUTILUS_IS_ACTIONS( instance ));
	NautilusActions *self = NAUTILUS_ACTIONS( instance );

	if( !self->private->dispose_has_run ){

		nautilus_menu_provider_emit_items_updated_signal( NAUTILUS_MENU_PROVIDER( self ));
	}
}
