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

#include <common/na-object-api.h>
#include <common/na-object-menu.h>
#include <common/na-object-action.h>
#include <common/na-object-profile.h>
#include <common/na-pivot.h>
#include <common/na-iabout.h>
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
static void              iabout_iface_init( NAIAboutInterface *iface );
static void              ipivot_consumer_iface_init( NAIPivotConsumerInterface *iface );
static void              instance_init( GTypeInstance *instance, gpointer klass );
static void              instance_dispose( GObject *object );
static void              instance_finalize( GObject *object );

static GList            *get_background_items( NautilusMenuProvider *provider, GtkWidget *window, NautilusFileInfo *current_folder );
static GList            *get_file_items( NautilusMenuProvider *provider, GtkWidget *window, GList *files );
static GList            *build_nautilus_menus( NautilusActions *plugin, GList *tree, GList *files );
static NAObjectProfile  *is_action_candidate( NautilusActions *plugin, NAObjectAction *action, GList *files );
static NautilusMenuItem *create_item_from_profile( NAObjectProfile *profile, GList *files );
static NautilusMenuItem *create_item_from_menu( NAObjectMenu *menu, GList *subitems );
static NautilusMenuItem *create_menu_item( NAObjectItem *item );
static void              attach_submenu_to_item( NautilusMenuItem *item, GList *subitems );

static void              execute_action( NautilusMenuItem *item, NAObjectProfile *profile );

static GList            *add_about_item( NautilusActions *plugin, GList *nautilus_menu );
static gchar            *iabout_get_application_name( NAIAbout *instance );
static void              execute_about( NautilusMenuItem *item, NautilusActions *plugin );

static void              actions_changed_handler( NAIPivotConsumer *instance, gpointer user_data );
static void              display_about_changed_handler( NAIPivotConsumer *instance, gboolean enabled );
static void              display_order_changed_handler( NAIPivotConsumer *instance, gint order_mode );

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

	static const GInterfaceInfo iabout_iface_info = {
		( GInterfaceInitFunc ) iabout_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo ipivot_consumer_iface_info = {
		( GInterfaceInitFunc ) ipivot_consumer_iface_init,
		NULL,
		NULL
	};

	g_debug( "%s: module=%p", thisfn, ( void * ) module );
	g_assert( st_actions_type == 0 );

	st_actions_type = g_type_module_register_type( module, G_TYPE_OBJECT, "NautilusActions", &info, 0 );

	g_type_module_add_interface( module, st_actions_type, NAUTILUS_TYPE_MENU_PROVIDER, &menu_provider_iface_info );

	g_type_module_add_interface( module, st_actions_type, NA_IABOUT_TYPE, &iabout_iface_info );

	g_type_module_add_interface( module, st_actions_type, NA_IPIVOT_CONSUMER_TYPE, &ipivot_consumer_iface_info );
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
iabout_iface_init( NAIAboutInterface *iface )
{
	static const gchar *thisfn = "nautilus_actions_iabout_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->get_application_name = iabout_get_application_name;
}

static void
ipivot_consumer_iface_init( NAIPivotConsumerInterface *iface )
{
	static const gchar *thisfn = "nautilus_actions_ipivot_consumer_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->on_actions_changed = actions_changed_handler;
	iface->on_display_about_changed = display_about_changed_handler;
	iface->on_display_order_changed = display_order_changed_handler;
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
	g_return_if_fail( NAUTILUS_IS_ACTIONS( object ));
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
	g_return_if_fail( NAUTILUS_IS_ACTIONS( object ));
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
 * .../...
 * until we have some actions defined as specific to backgrounds !
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
	GList *nautilus_menus_list = NULL;
	NautilusActions *self;
	GList *pivot_tree;
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

		pivot_tree = na_pivot_get_items( self->private->pivot );

		nautilus_menus_list = build_nautilus_menus( self, pivot_tree, files );
		g_debug( "%s: menus has %d level zero items", thisfn, g_list_length( nautilus_menus_list ));

		add_about = na_iprefs_should_add_about_item( NA_IPREFS( self->private->pivot ));
		g_debug( "%s: add_about=%s", thisfn, add_about ? "True":"False" );
		if( add_about ){
			nautilus_menus_list = add_about_item( self, nautilus_menus_list );
		}
	}

	return( nautilus_menus_list );
}

static GList *
build_nautilus_menus( NautilusActions *plugin, GList *tree, GList *files )
{
	static const gchar *thisfn = "nautilus_actions_build_nautilus_menus";
	GList *menus_list = NULL;
	GList *subitems, *submenu;
	GList *it;
	NAObjectProfile *profile;
	NautilusMenuItem *item;
	gchar *label;

	g_debug( "%s: plugin=%p, tree=%p, files=%p",
			thisfn, ( void * ) plugin, ( void * ) tree, ( void * ) files );

	for( it = tree ; it ; it = it->next ){

		g_return_val_if_fail( NA_IS_OBJECT_ITEM( it->data ), NULL );

		if( !na_object_is_enabled( it->data )){
			continue;
		}

		label = na_object_get_label( it->data );
		g_debug( "%s: %s - %s", thisfn, G_OBJECT_TYPE_NAME( it->data ), label );
		g_free( label );

		if( NA_IS_OBJECT_MENU( it->data )){
			subitems = na_object_get_items( it->data );
			submenu = build_nautilus_menus( plugin, subitems, files );
			/*g_debug( "%s: submenu has %d items", thisfn, g_list_length( submenu ));*/
			na_object_free_items( subitems );
			if( submenu ){
				item = create_item_from_menu( NA_OBJECT_MENU( it->data ), submenu );
				menus_list = g_list_append( menus_list, item );
			}
			continue;
		}

		g_return_val_if_fail( NA_IS_OBJECT_ACTION( it->data ), NULL );

		profile = is_action_candidate( plugin, NA_OBJECT_ACTION( it->data ), files );
		if( profile ){
			item = create_item_from_profile( profile, files );
			menus_list = g_list_append( menus_list, item );
		}
	}

	return( menus_list );
}

/*
 * could also be a NAObjectAction method - but this is not used elsewhere
 */
static NAObjectProfile *
is_action_candidate( NautilusActions *plugin, NAObjectAction *action, GList *files )
{
	static const gchar *thisfn = "nautilus_actions_is_action_candidate";
	NAObjectProfile *candidate = NULL;
	gchar *action_label, *uuid;
	gchar *profile_label;
	GList *profiles, *ip;

	action_label = na_object_get_label( action );

	if( !action_label || !g_utf8_strlen( action_label, -1 )){
		uuid = na_object_get_id( action );
		g_warning( "%s: label null or empty for uuid=%s", thisfn, uuid );
		g_free( uuid );
		return( NULL );
	}

	profiles = na_object_get_items( action );
	for( ip = profiles ; ip && !candidate ; ip = ip->next ){

		NAObjectProfile *profile = NA_OBJECT_PROFILE( ip->data );
		if( na_object_profile_is_candidate( profile, files )){

			profile_label = na_object_get_label( profile );
			g_debug( "%s: selecting %s - %s", thisfn, action_label, profile_label );
			g_free( profile_label );

			candidate = profile;
 		}
 	}

	g_free( action_label );

	return( candidate );
}

static NautilusMenuItem *
create_item_from_profile( NAObjectProfile *profile, GList *files )
{
	NautilusMenuItem *item;
	NAObjectAction *action;
	NAObjectProfile *dup4menu;

	action = na_object_profile_get_action( profile );

	item = create_menu_item( NA_OBJECT_ITEM( action ));

	dup4menu = NA_OBJECT_PROFILE( na_object_duplicate( profile ));

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

	return( item );
}

/*
 * note that each appended NautilusMenuItem is ref-ed by the NautilusMenu
 * we can so safely release our own ref on subitems after this function
 */
static NautilusMenuItem *
create_item_from_menu( NAObjectMenu *menu, GList *subitems )
{
	/*static const gchar *thisfn = "nautilus_actions_create_item_from_menu";*/
	NautilusMenuItem *item;

	item = create_menu_item( NA_OBJECT_ITEM( menu ));
	attach_submenu_to_item( item, subitems );
	nautilus_menu_item_list_free( subitems );

	/*g_debug( "%s: returning item=%p", thisfn, ( void * ) item );*/
	return( item );
}

static NautilusMenuItem *
create_menu_item( NAObjectItem *item )
{
	NautilusMenuItem *menu_item;
	gchar *uuid, *name, *label, *tooltip, *icon;

	uuid = na_object_get_id( item );
	name = g_strdup_printf( "%s-%s-%s", PACKAGE, G_OBJECT_TYPE_NAME( item ), uuid );
	label = na_object_get_label( item );
	/*g_debug( "nautilus_actions_create_menu_item: %s - %s", name, label );*/
	tooltip = na_object_get_tooltip( item );
	icon = na_object_get_icon( item );

	menu_item = nautilus_menu_item_new( name, label, tooltip, icon );

	g_free( icon );
 	g_free( tooltip );
 	g_free( label );
 	g_free( name );
 	g_free( uuid );

	return( menu_item );
}

static void
attach_submenu_to_item( NautilusMenuItem *item, GList *subitems )
{
	NautilusMenu *submenu;
	GList *it;

	submenu = nautilus_menu_new();
	nautilus_menu_item_set_submenu( item, submenu );

	for( it = subitems ; it ; it = it->next ){
		nautilus_menu_append_item( submenu, NAUTILUS_MENU_ITEM( it->data ));
	}
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

/*
 * if there is a root submenu,
 * then add the about item to the end of the first level of this menu
 * else create a root submenu
 */
static GList *
add_about_item( NautilusActions *plugin, GList *menu )
{
	static const gchar *thisfn = "nautilus_actions_add_about_item";
	GList *nautilus_menu;
	gboolean have_root_menu;
	NautilusMenuItem *root_item;
	NautilusMenuItem *about_item;
	NautilusMenu *first;
	gchar *icon;

	g_debug( "%s: plugin=%p, menu=%p (%d items)",
			thisfn, ( void * ) plugin, ( void * ) menu, g_list_length( menu ));

	if( !menu || !g_list_length( menu )){
		return( NULL );
	}

	icon = na_iabout_get_icon_name();
	have_root_menu = FALSE;

	if( g_list_length( menu ) == 1 ){
		root_item = NAUTILUS_MENU_ITEM( menu->data );
		g_object_get( G_OBJECT( root_item ), "menu", &first, NULL );
		if( first ){
			g_return_val_if_fail( NAUTILUS_IS_MENU( first ), NULL );
			have_root_menu = TRUE;
		}
	}

	if( have_root_menu ){
		nautilus_menu = menu;

	} else {
		root_item = nautilus_menu_item_new( "NautilusActionsExtensions",
				/* i18n: label of an automagic root submenu */
				_( "Nautilus Actions" ),
				/* i18n: tooltip of an automagic root submenu */
				_( "A submenu which embeds the currently available Nautilus-Actions extensions" ),
				icon );
		attach_submenu_to_item( root_item, menu );
		nautilus_menu = g_list_append( NULL, root_item );
		g_object_get( G_OBJECT( root_item ), "menu", &first, NULL );
	}

	g_return_val_if_fail( g_list_length( nautilus_menu ) == 1, NULL );
	g_return_val_if_fail( NAUTILUS_IS_MENU( first ), NULL );

	about_item = nautilus_menu_item_new( "AboutNautilusActions",
				_( "About Nautilus Actions" ),
				_( "Display information about Nautilus Actions" ),
				icon );

	g_signal_connect_data( about_item,
				"activate",
				G_CALLBACK( execute_about ),
				plugin,
				NULL,
				0 );

	nautilus_menu_append_item( first, about_item );

	g_free( icon );

	return( nautilus_menu );
}

static gchar *
iabout_get_application_name( NAIAbout *instance )
{
	/* i18n: title of the About dialog box, when seen from Nautilus file manager */
	return( g_strdup( _( "Nautilus Actions" )));
}

static void
execute_about( NautilusMenuItem *item, NautilusActions *plugin )
{
	g_return_if_fail( NAUTILUS_IS_ACTIONS( plugin ));
	g_return_if_fail( NA_IS_IABOUT( plugin ));

	na_iabout_display( NA_IABOUT( plugin ));
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

static void
display_about_changed_handler( NAIPivotConsumer *instance, gboolean enabled )
{
	static const gchar *thisfn = "nautilus_actions_display_about_changed_handler";
	NautilusActions *self;

	g_debug( "%s: instance=%p, enabled=%s", thisfn, ( void * ) instance, enabled ? "True":"False" );
	g_return_if_fail( NAUTILUS_IS_ACTIONS( instance ));
	self = NAUTILUS_ACTIONS( instance );

	if( !self->private->dispose_has_run ){
		nautilus_menu_provider_emit_items_updated_signal( NAUTILUS_MENU_PROVIDER( self ));
	}
}

static void
display_order_changed_handler( NAIPivotConsumer *instance, gint order_mode )
{
	static const gchar *thisfn = "nautilus_actions_display_order_changed_handler";
	NautilusActions *self;

	g_debug( "%s: instance=%p, order_mode=%d", thisfn, ( void * ) instance, order_mode );
	g_return_if_fail( NAUTILUS_IS_ACTIONS( instance ));
	self = NAUTILUS_ACTIONS( instance );

	if( !self->private->dispose_has_run ){
		nautilus_menu_provider_emit_items_updated_signal( NAUTILUS_MENU_PROVIDER( self ));
	}
}
