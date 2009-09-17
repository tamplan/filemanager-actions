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

#include <common/na-about.h>
#include <common/na-object-api.h>
#include <common/na-obj-action.h>
#include <common/na-obj-profile.h>
#include <common/na-pivot.h>
#include <common/na-iprefs.h>
#include <common/na-ipivot-consumer.h>

#include "nautilus-actions.h"

/* private class data
 */
struct NautilusActionsClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
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
static void              ipivot_consumer_iface_init( NAIPivotConsumerInterface *iface );
static void              iprefs_iface_init( NAIPrefsInterface *iface );
static void              instance_init( GTypeInstance *instance, gpointer klass );
static void              instance_dispose( GObject *object );
static void              instance_finalize( GObject *object );

static GList            *get_background_items( NautilusMenuProvider *provider, GtkWidget *window, NautilusFileInfo *current_folder );
static GList            *get_file_items( NautilusMenuProvider *provider, GtkWidget *window, GList *files );
static NautilusMenuItem *create_menu_item( NAObjectAction *action, NAObjectProfile *profile, GList *files );
/*static NautilusMenuItem *create_sub_menu( NautilusMenu **menu );*/
static void              add_about_item( NautilusMenu *menu );
static void              execute_action( NautilusMenuItem *item, NAObjectProfile *profile );
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

	static const GInterfaceInfo menu_provider_iface_info = {
		( GInterfaceInitFunc ) menu_provider_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo ipivot_consumer_iface_info = {
		( GInterfaceInitFunc ) ipivot_consumer_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo iprefs_iface_info = {
		( GInterfaceInitFunc ) iprefs_iface_init,
		NULL,
		NULL
	};

	g_debug( "%s: module=%p", thisfn, ( void * ) module );
	g_assert( st_actions_type == 0 );

	st_actions_type = g_type_module_register_type( module, G_TYPE_OBJECT, "NautilusActions", &info, 0 );

	g_type_module_add_interface( module, st_actions_type, NAUTILUS_TYPE_MENU_PROVIDER, &menu_provider_iface_info );

	g_type_module_add_interface( module, st_actions_type, NA_IPIVOT_CONSUMER_TYPE, &ipivot_consumer_iface_info );

	g_type_module_add_interface( module, st_actions_type, NA_IPREFS_TYPE, &iprefs_iface_info );
}

static void
class_init( NautilusActionsClass *klass )
{
	static const gchar *thisfn = "nautilus_actions_class_init";
	GObjectClass *gobject_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	gobject_class = G_OBJECT_CLASS( klass );
	gobject_class->dispose = instance_dispose;
	gobject_class->finalize = instance_finalize;

	klass->private = g_new0( NautilusActionsClassPrivate, 1 );
}

static void
menu_provider_iface_init( NautilusMenuProviderIface *iface )
{
	static const gchar *thisfn = "nautilus_actions_menu_provider_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->get_file_items = get_file_items;
	iface->get_background_items = get_background_items;
}

static void
ipivot_consumer_iface_init( NAIPivotConsumerInterface *iface )
{
	static const gchar *thisfn = "nautilus_actions_ipivot_consumer_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->on_actions_changed = actions_changed_handler;
}

static void
iprefs_iface_init( NAIPrefsInterface *iface )
{
	static const gchar *thisfn = "nautilus_actions_iprefs_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nautilus_actions_instance_init";
	NautilusActions *self;

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );

	g_return_if_fail( NAUTILUS_IS_ACTIONS( instance ));
	g_return_if_fail( NA_IS_IPIVOT_CONSUMER( instance ));

	self = NAUTILUS_ACTIONS( instance );

	self->private = g_new0( NautilusActionsPrivate, 1 );
	self->private->dispose_has_run = FALSE;

	/* from na-pivot */
	self->private->pivot = na_pivot_new( NA_IPIVOT_CONSUMER( self ));
	na_pivot_set_automatic_reload( self->private->pivot, TRUE );
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "nautilus_actions_instance_dispose";
	NautilusActions *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_assert( NAUTILUS_IS_ACTIONS( object ));
	self = NAUTILUS_ACTIONS( object );

	if( !self->private->dispose_has_run ){

		g_object_unref( self->private->pivot );

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}

		self->private->dispose_has_run = TRUE;
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "nautilus_actions_instance_finalize";
	NautilusActions *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_assert( NAUTILUS_IS_ACTIONS( object ));
	self = NAUTILUS_ACTIONS( object );

	g_free( self->private );

	/* chain up to the parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
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
#ifdef NA_MAINTAINER_MODE
	static const gchar *thisfn = "nautilus_actions_get_background_items";
	gchar *uri = nautilus_file_info_get_uri( current_folder );
	g_debug( "%s: provider=%p, window=%p, current_folder=%p (%s)",
			thisfn, ( void * ) provider, ( void * ) window, ( void * ) current_folder, uri );
	g_free( uri );
#endif
	return(( GList * ) NULL );
}

static GList *
get_file_items( NautilusMenuProvider *provider, GtkWidget *window, GList *files )
{
	static const gchar *thisfn = "nautilus_actions_get_file_items";
	NautilusActions *self;
	GList *items = NULL;
	GList *profiles, *ia, *ip;
	NautilusMenu *menu = NULL;
	NautilusMenuItem *item;
	GList *tree = NULL;
	gchar *label, *uuid;
	gint submenus = 0;
	gboolean add_about;

	g_debug( "%s: provider=%p, window=%p, files=%p, count=%d",
			thisfn, ( void * ) provider, ( void * ) window, ( void * ) files, g_list_length( files ));

	g_return_val_if_fail( NAUTILUS_IS_ACTIONS( provider ), NULL );
	self = NAUTILUS_ACTIONS( provider );

	/* no need to go further if there is no files in the list */
	if( !g_list_length( files )){
		return(( GList * ) NULL );
	}

	if( !self->private->dispose_has_run ){
		tree = na_pivot_get_items( self->private->pivot );

		for( ia = tree ; ia ; ia = ia->next ){

			NAObjectAction *action = NA_OBJECT_ACTION( ia->data );

			if( !na_object_is_enabled( action )){
				continue;
			}

			label = na_object_get_label( action );

			if( !label || !g_utf8_strlen( label, -1 )){
				uuid = na_object_get_id( action );
				g_warning( "%s: label null or empty for uuid=%s", thisfn, uuid );
				g_free( uuid );
				continue;
			}

			g_debug( "%s: examining '%s' action", thisfn, label );
			g_free( label );

			profiles = na_object_get_items( action );

			for( ip = profiles ; ip ; ip = ip->next ){

				NAObjectProfile *profile = NA_OBJECT_PROFILE( ip->data );

#ifdef NA_MAINTAINER_MODE
				label = na_object_get_label( profile );
				g_debug( "%s: examining '%s' profile", thisfn, label );
				g_free( label );
#endif

				if( na_object_profile_is_candidate( profile, files )){
					item = create_menu_item( action, profile, files );
					items = g_list_append( items, item );

					/*if( have_submenu ){
						if( !menu ){
							items = g_list_append( items, create_sub_menu( &menu ));
						}
						nautilus_menu_append_item( menu, item );

					} else {
					}*/
					break;
				}
			}

			na_object_free_items( profiles );
		}

		add_about = FALSE; /*na_iprefs_get_add_about_item( NA_IPREFS( self ));*/
		if( submenus == 1 && add_about ){
			add_about_item( menu );
		}
	}

	return( items );
}

static NautilusMenuItem *
create_menu_item( NAObjectAction *action, NAObjectProfile *profile, GList *files )
{
	static const gchar *thisfn = "nautilus_actions_create_menu_item";
	NautilusMenuItem *item;
	gchar *uuid, *name, *label, *tooltip, *icon_name;
	NAObjectProfile *dup4menu;

	g_debug( "%s", thisfn );

	uuid = na_object_get_id( action );
	name = g_strdup_printf( "NautilusActions::%s", uuid );
	label = na_object_get_label( action );
	tooltip = na_object_get_tooltip( action );
	icon_name = na_object_item_get_verified_icon_name( NA_OBJECT_ITEM( action ));

	dup4menu = NA_OBJECT_PROFILE( na_object_duplicate( profile ));

	item = nautilus_menu_item_new( name, label, tooltip, icon_name );

	g_signal_connect_data( item,
				"activate",
				G_CALLBACK( execute_action ),
				dup4menu,
				( GClosureNotify ) g_object_unref,
				0 );

	g_object_set_data_full( G_OBJECT( item ),
			"files",
			nautilus_file_info_list_copy( files ),
			( GDestroyNotify ) nautilus_file_info_list_free );

	g_free( icon_name );
	g_free( tooltip );
	g_free( label );
	g_free( name );
	g_free( uuid );

	return( item );
}

/*static NautilusMenuItem *
create_sub_menu( NautilusMenu **menu )
{
	NautilusMenuItem *item;

	gchar *icon_name = na_about_get_icon_name();

	item = nautilus_menu_item_new( "NautilusActionsExtensions",
			_( "Nautilus-Actions extensions" ),
			_( "A submenu which embeds the currently available Nautilus-Actions extensions" ),
			icon_name );

	if( menu ){
		*menu = nautilus_menu_new();
		nautilus_menu_item_set_submenu( item, *menu );
	}

	g_free( icon_name );

	return( item );
}*/

static void
add_about_item( NautilusMenu *menu )
{
	gchar *icon_name = na_about_get_icon_name();

	NautilusMenuItem *item = nautilus_menu_item_new(
			"AboutNautilusActions",
			_( "About Nautilus Actions" ),
			_( "Display information about Nautilus Actions" ),
			icon_name );

	g_signal_connect_data( item,
				"activate",
				G_CALLBACK( na_about_display ),
				NULL,
				NULL,
				0 );

	nautilus_menu_append_item( menu, item );

	g_free( icon_name );
}

static void
execute_action( NautilusMenuItem *item, NAObjectProfile *profile )
{
	static const gchar *thisfn = "nautilus_actions_execute_action";
	GList *files;
	GString *cmd;
	gchar *param, *path;

	g_debug( "%s: item=%p, profile=%p", thisfn, ( void * ) item, ( void * ) profile );

	files = ( GList* ) g_object_get_data( G_OBJECT( item ), "files" );

	path = na_object_profile_get_path( profile );
	cmd = g_string_new( path );

	param = na_object_profile_parse_parameters( profile, files );

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
	NautilusActions *self;

	g_debug( "%s: instance=%p, user_data=%p", thisfn, ( void * ) instance, ( void * ) user_data );
	g_return_if_fail( NAUTILUS_IS_ACTIONS( instance ));
	self = NAUTILUS_ACTIONS( instance );

	if( !self->private->dispose_has_run ){

		nautilus_menu_provider_emit_items_updated_signal( NAUTILUS_MENU_PROVIDER( self ));
	}
}
