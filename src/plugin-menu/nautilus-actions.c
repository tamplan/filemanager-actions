/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011 Pierre Wieser and others (see AUTHORS)
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

#include <glib/gi18n.h>

#include <libnautilus-extension/nautilus-extension-types.h>
#include <libnautilus-extension/nautilus-file-info.h>
#include <libnautilus-extension/nautilus-menu-provider.h>

#include <api/na-core-utils.h>
#include <api/na-object-api.h>

#include <core/na-pivot.h>
#include <core/na-iabout.h>
#include <core/na-selected-info.h>
#include <core/na-tokens.h>

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
	gulong   items_changed_handler;
};

static GObjectClass *st_parent_class    = NULL;
static GType         st_actions_type    = 0;
static GTimeVal      st_last_event;
static guint         st_event_source_id = 0;
static gint          st_burst_timeout   = 100;		/* burst timeout in msec */

static void              class_init( NautilusActionsClass *klass );
static void              instance_init( GTypeInstance *instance, gpointer klass );
static void              instance_constructed( GObject *object );
static void              instance_dispose( GObject *object );
static void              instance_finalize( GObject *object );

static void              iabout_iface_init( NAIAboutInterface *iface );
static gchar            *iabout_get_application_name( NAIAbout *instance );

static void              menu_provider_iface_init( NautilusMenuProviderIface *iface );
static GList            *menu_provider_get_background_items( NautilusMenuProvider *provider, GtkWidget *window, NautilusFileInfo *current_folder );
static GList            *menu_provider_get_file_items( NautilusMenuProvider *provider, GtkWidget *window, GList *files );
static GList            *menu_provider_get_toolbar_items( NautilusMenuProvider *provider, GtkWidget *window, NautilusFileInfo *current_folder );

static GList            *get_menus_items( NautilusActions *plugin, guint target, GList *selection );
static GList            *expand_tokens( GList *tree, NATokens *tokens );
static NAObjectItem     *expand_tokens_item( NAObjectItem *item, NATokens *tokens );
static void              expand_tokens_context( NAIContext *context, NATokens *tokens );
static GList            *build_nautilus_menus( NautilusActions *plugin, GList *tree, guint target, GList *files, NATokens *tokens );
static NAObjectProfile  *get_candidate_profile( NautilusActions *plugin, NAObjectAction *action, guint target, GList *files );
static NautilusMenuItem *create_item_from_profile( NAObjectProfile *profile, guint target, GList *files, NATokens *tokens );
static NautilusMenuItem *create_item_from_menu( NAObjectMenu *menu, GList *subitems, guint target );
static NautilusMenuItem *create_menu_item( NAObjectItem *item, guint target );
static void              attach_submenu_to_item( NautilusMenuItem *item, GList *subitems );
static void              weak_notify_profile( NAObjectProfile *profile, NautilusMenuItem *item );

static void              execute_action( NautilusMenuItem *item, NAObjectProfile *profile );

static GList            *create_root_menu( NautilusActions *plugin, GList *nautilus_menu );
static GList            *add_about_item( NautilusActions *plugin, GList *nautilus_menu );
static void              execute_about( NautilusMenuItem *item, NautilusActions *plugin );

static void              on_pivot_items_changed_handler( NAPivot *pivot, NautilusActions *plugin );
static void              on_runtime_preference_changed( const gchar *group, const gchar *key, gconstpointer newvalue, gboolean mandatory, NautilusActions *plugin );
static void              record_change_event( NautilusActions *plugin );
static gboolean          on_change_event_timeout( NautilusActions *plugin );
static gulong            time_val_diff( const GTimeVal *recent, const GTimeVal *old );

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

	g_assert( st_actions_type == 0 );

	g_debug( "%s: module=%p", thisfn, ( void * ) module );

	st_actions_type = g_type_module_register_type( module, G_TYPE_OBJECT, "NautilusActions", &info, 0 );

	g_type_module_add_interface( module, st_actions_type, NAUTILUS_TYPE_MENU_PROVIDER, &menu_provider_iface_info );

	g_type_module_add_interface( module, st_actions_type, NA_IABOUT_TYPE, &iabout_iface_info );
}

static void
class_init( NautilusActionsClass *klass )
{
	static const gchar *thisfn = "nautilus_actions_class_init";
	GObjectClass *gobject_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	gobject_class = G_OBJECT_CLASS( klass );
	gobject_class->constructed = instance_constructed;
	gobject_class->dispose = instance_dispose;
	gobject_class->finalize = instance_finalize;

	klass->private = g_new0( NautilusActionsClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nautilus_actions_instance_init";
	NautilusActions *self;

	g_return_if_fail( NAUTILUS_IS_ACTIONS( instance ));

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );

	self = NAUTILUS_ACTIONS( instance );

	self->private = g_new0( NautilusActionsPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

/*
 * Runtime modification management:
 * We have to react to some runtime environment modifications:
 *
 * - whether the items list has changed (we have to reload a new pivot)
 *   > registering for notifications against NAPivot
 *
 * - whether to add the 'About Nautilus-Actions' item
 * - whether to create a 'Nautilus-Actions actions' root menu
 *   > registering for notifications against NASettings
 */
static void
instance_constructed( GObject *object )
{
	static const gchar *thisfn = "nautilus_actions_instance_constructed";
	NautilusActions *self;
	NASettings *settings;

	g_return_if_fail( NAUTILUS_IS_ACTIONS( object ));

	self = NAUTILUS_ACTIONS( object );

	if( !self->private->dispose_has_run ){
		g_debug( "%s: object=%p", thisfn, ( void * ) object );

		self->private->pivot = na_pivot_new();

		/* setup NAPivot properties before loading items
		 */
		na_pivot_set_loadable( self->private->pivot, !PIVOT_LOAD_DISABLED & !PIVOT_LOAD_INVALID );
		na_pivot_load_items( self->private->pivot );

		/* register against NAPivot to be notified of items changes
		 */
		self->private->items_changed_handler =
				g_signal_connect(
						self->private->pivot,
						PIVOT_SIGNAL_ITEMS_CHANGED,
						G_CALLBACK( on_pivot_items_changed_handler ), self );

		/* register against NASettings to be notified of changes on
		 *  our runtime preferences
		 */
		settings = na_pivot_get_settings( self->private->pivot );

		/* monitor
		 * - the changes of the readability status of the i/o providers
		 * - the changes of the read order of the i/o providers
		 * - the modification of the level-zero order
		 * - whether we create a root menu
		 * - whether we add an 'About Nautilus-Actions' item
		 * - the preferred order mode
		 */
		na_settings_register_key_callback( settings,
				NA_SETTINGS_RUNTIME_IO_PROVIDER_READ_STATUS,
				G_CALLBACK( on_runtime_preference_changed ), self );

		na_settings_register_key_callback( settings,
				NA_SETTINGS_RUNTIME_IO_PROVIDERS_READ_ORDER,
				G_CALLBACK( on_runtime_preference_changed ), self );

		na_settings_register_key_callback( settings,
				NA_SETTINGS_RUNTIME_ITEMS_LEVEL_ZERO_ORDER,
				G_CALLBACK( on_runtime_preference_changed ), self );

		na_settings_register_key_callback( settings,
				NA_SETTINGS_RUNTIME_ITEMS_CREATE_ROOT_MENU,
				G_CALLBACK( on_runtime_preference_changed ), self );

		na_settings_register_key_callback( settings,
				NA_SETTINGS_RUNTIME_ITEMS_ADD_ABOUT_ITEM,
				G_CALLBACK( on_runtime_preference_changed ), self );

		na_settings_register_key_callback( settings,
				NA_SETTINGS_RUNTIME_ITEMS_LIST_ORDER_MODE,
				G_CALLBACK( on_runtime_preference_changed ), self );

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->constructed ){
			G_OBJECT_CLASS( st_parent_class )->constructed( object );
		}
	}
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

		self->private->dispose_has_run = TRUE;

		if( self->private->items_changed_handler ){
			g_signal_handler_disconnect( self->private->pivot, self->private->items_changed_handler );
		}
		g_object_unref( self->private->pivot );

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
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

static void
iabout_iface_init( NAIAboutInterface *iface )
{
	static const gchar *thisfn = "nautilus_actions_iabout_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->get_application_name = iabout_get_application_name;
}

static gchar *
iabout_get_application_name( NAIAbout *instance )
{
	/* i18n: title of the About dialog box, when seen from Nautilus file manager */
	return( g_strdup( _( "Nautilus-Actions" )));
}

static void
menu_provider_iface_init( NautilusMenuProviderIface *iface )
{
	static const gchar *thisfn = "nautilus_actions_menu_provider_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->get_file_items = menu_provider_get_file_items;
	iface->get_background_items = menu_provider_get_background_items;
	iface->get_toolbar_items = menu_provider_get_toolbar_items;
}

/*
 * this function is called when nautilus has to paint a folder background
 * one of the first calls is with current_folder = 'x-nautilus-desktop:///'
 * the menu items are available :
 * a) in File menu
 * b) in contextual menu of the folder if there is no current selection
 *
 * get_background_items is very similar, from the user point of view, to
 * get_file_items when:
 * - either there is zero item selected - current_folder should so be the
 *   folder currently displayed in the file manager view
 * - or when there is only one selected directory
 *
 * Note that 'x-nautilus-desktop:///' cannot be interpreted by
 * #NASelectedInfo::query_file_attributes() function. It so never participate
 * to the display of actions.
 */
static GList *
menu_provider_get_background_items( NautilusMenuProvider *provider, GtkWidget *window, NautilusFileInfo *current_folder )
{
	static const gchar *thisfn = "nautilus_actions_menu_provider_get_background_items";
	GList *nautilus_menus_list = NULL;
	gchar *uri;
	GList *selected;

	g_return_val_if_fail( NAUTILUS_IS_ACTIONS( provider ), NULL );

	if( !NAUTILUS_ACTIONS( provider )->private->dispose_has_run ){

		selected = na_selected_info_get_list_from_item( current_folder );

		if( selected ){
			uri = nautilus_file_info_get_uri( current_folder );
			g_debug( "%s: provider=%p, window=%p, current_folder=%p (%s)",
					thisfn, ( void * ) provider, ( void * ) window, ( void * ) current_folder, uri );
			g_free( uri );

			nautilus_menus_list = get_menus_items( NAUTILUS_ACTIONS( provider ), ITEM_TARGET_LOCATION, selected );
			na_selected_info_free_list( selected );
		}
	}

	return( nautilus_menus_list );
}

/*
 * this function is called each time the selection changed
 * menus items are available :
 * a) in Edit menu while the selection stays unchanged
 * b) in contextual menu while the selection stays unchanged
 */
static GList *
menu_provider_get_file_items( NautilusMenuProvider *provider, GtkWidget *window, GList *files )
{
	static const gchar *thisfn = "nautilus_actions_menu_provider_get_file_items";
	GList *nautilus_menus_list = NULL;
	GList *selected;

	g_return_val_if_fail( NAUTILUS_IS_ACTIONS( provider ), NULL );

	if( !NAUTILUS_ACTIONS( provider )->private->dispose_has_run ){

		/* no need to go further if there is no files in the list */
		if( !g_list_length( files )){
			return(( GList * ) NULL );
		}

#ifdef NA_MAINTAINER_MODE
		GList *im;
		for( im = files ; im ; im = im->next ){
			gchar *uri = nautilus_file_info_get_uri( NAUTILUS_FILE_INFO( im->data ));
			g_debug( "%s: uri=%s", thisfn, uri );
			g_free( uri );
		}
#endif

		selected = na_selected_info_get_list_from_list(( GList * ) files );

		if( selected ){
			g_debug( "%s: provider=%p, window=%p, files=%p, count=%d",
					thisfn, ( void * ) provider, ( void * ) window, ( void * ) files, g_list_length( files ));

			nautilus_menus_list = get_menus_items( NAUTILUS_ACTIONS( provider ), ITEM_TARGET_SELECTION, selected );
			na_selected_info_free_list( selected );
		}
	}

	return( nautilus_menus_list );
}

/*
 * as of 2.26, this function is only called for folders, but for the
 * desktop (x-nautilus-desktop:///) which seems to be only called by
 * get_background_items ; also, only actions (not menus) are displayed
 */
static GList *
menu_provider_get_toolbar_items( NautilusMenuProvider *provider, GtkWidget *window, NautilusFileInfo *current_folder )
{
	static const gchar *thisfn = "nautilus_actions_menu_provider_get_toolbar_items";
	GList *nautilus_menus_list = NULL;
	gchar *uri;
	GList *selected;

	g_return_val_if_fail( NAUTILUS_IS_ACTIONS( provider ), NULL );

	if( !NAUTILUS_ACTIONS( provider )->private->dispose_has_run ){

		selected = na_selected_info_get_list_from_item( current_folder );

		if( selected ){
			uri = nautilus_file_info_get_uri( current_folder );
			g_debug( "%s: provider=%p, window=%p, current_folder=%p (%s)",
					thisfn, ( void * ) provider, ( void * ) window, ( void * ) current_folder, uri );
			g_free( uri );

			nautilus_menus_list = get_menus_items( NAUTILUS_ACTIONS( provider ), ITEM_TARGET_TOOLBAR, selected );
			na_selected_info_free_list( selected );
		}
	}

	return( nautilus_menus_list );
}

static GList *
get_menus_items( NautilusActions *plugin, guint target, GList *selection )
{
	GList *menus_list;
	NATokens *tokens;
	GList *pivot_tree, *copy_tree;
	NASettings *settings;
	gboolean items_add_about_item;
	gboolean items_create_root_menu;

	g_return_val_if_fail( NA_IS_PIVOT( plugin->private->pivot ), NULL );

	tokens = na_tokens_new_from_selection( selection );
	pivot_tree = na_pivot_get_items( plugin->private->pivot );
	copy_tree = expand_tokens( pivot_tree, tokens );

	menus_list = build_nautilus_menus( plugin, copy_tree, target, selection, tokens );

	na_object_unref_items( copy_tree );
	g_object_unref( tokens );

	if( target != ITEM_TARGET_TOOLBAR ){

		settings = na_pivot_get_settings( plugin->private->pivot );

		items_create_root_menu = na_settings_get_boolean( settings, NA_SETTINGS_RUNTIME_ITEMS_CREATE_ROOT_MENU, NULL, NULL );
		if( items_create_root_menu ){
			menus_list = create_root_menu( plugin, menus_list );
		}

		items_add_about_item = na_settings_get_boolean( settings, NA_SETTINGS_RUNTIME_ITEMS_ADD_ABOUT_ITEM, NULL, NULL );
		if( items_add_about_item ){
			menus_list = add_about_item( plugin, menus_list );
		}
	}

	return( menus_list );
}

/*
 * create a copy of the tree where almost all fields which may embed
 * parameters have been expanded
 * here, 'almost' should be readen as:
 * - all displayable fields, or fields which may have an impact on the display
 *   (e.g. label, tooltip, icon name)
 * - all fields which we do not need later
 *
 * we keep until the last item activation the Exec key, whose first parameter
 * actualy determines the form (singular or plural) of the execution..
 */
static GList *
expand_tokens( GList *pivot_tree, NATokens *tokens )
{
	GList *tree, *it;

	tree = NULL;

	for( it = pivot_tree ; it ; it = it->next ){
		NAObjectItem *item = NA_OBJECT_ITEM( na_object_duplicate( it->data ));
		tree = g_list_prepend( tree, expand_tokens_item( item, tokens ));
	}

	return( g_list_reverse( tree ));
}

static NAObjectItem *
expand_tokens_item( NAObjectItem *item, NATokens *tokens )
{
	gchar *old, *new;
	GSList *subitems_slist, *its, *new_slist;
	GList *subitems, *it, *new_list;
	NAObjectItem *expanded_item;

	/* label, tooltip and icon name
	 * plus the toolbar label if this is an action
	 */
	old = na_object_get_label( item );
	new = na_tokens_parse_for_display( tokens, old, TRUE );
	na_object_set_label( item, new );
	g_free( old );
	g_free( new );

	old = na_object_get_tooltip( item );
	new = na_tokens_parse_for_display( tokens, old, TRUE );
	na_object_set_tooltip( item, new );
	g_free( old );
	g_free( new );

	old = na_object_get_icon( item );
	new = na_tokens_parse_for_display( tokens, old, TRUE );
	na_object_set_icon( item, new );
	g_free( old );
	g_free( new );

	if( NA_IS_OBJECT_ACTION( item )){
		old = na_object_get_toolbar_label( item );
		new = na_tokens_parse_for_display( tokens, old, TRUE );
		na_object_set_toolbar_label( item, new );
		g_free( old );
		g_free( new );
	}

	/* A NAObjectItem, whether it is an action or a menu, is also a NAIContext
	 */
	expand_tokens_context( NA_ICONTEXT( item ), tokens );

	/* subitems lists, whether this is the profiles list of an action
	 * or the items list of a menu, may be dynamic and embed a command;
	 * this command itself may embed parameters
	 */
	subitems_slist = na_object_get_items_slist( item );
	new_slist = NULL;
	for( its = subitems_slist ; its ; its = its->next ){
		old = ( gchar * ) its->data;
		if( old[0] == '[' && old[strlen(old)-1] == ']' ){
			new = na_tokens_parse_for_display( tokens, old, FALSE );
		} else {
			new = g_strdup( old );
		}
		new_slist = g_slist_prepend( new_slist, new );
	}
	na_object_set_items_slist( item, new_slist );
	na_core_utils_slist_free( subitems_slist );
	na_core_utils_slist_free( new_slist );

	/* last, deal with subitems
	 */
	subitems = na_object_get_items( item );

	if( NA_IS_OBJECT_MENU( item )){
		new_list = NULL;

		for( it = subitems ; it ; it = it->next ){
			expanded_item = expand_tokens_item( NA_OBJECT_ITEM( it->data ), tokens );
			new_list = g_list_prepend( new_list, expanded_item );
		}

		na_object_set_items( item, g_list_reverse( new_list ));

	} else {
		g_return_val_if_fail( NA_IS_OBJECT_ACTION( item ), NULL );

		for( it = subitems ; it ; it = it->next ){

			/* desktop Exec key = GConf path+parameters
			 * do not touch them here
			 */
			old = na_object_get_working_dir( it->data );
			new = na_tokens_parse_for_display( tokens, old, FALSE );
			na_object_set_working_dir( it->data, new );
			g_free( old );
			g_free( new );

			/* a NAObjectProfile is also a NAIContext
			 */
			expand_tokens_context( NA_ICONTEXT( it->data ), tokens );
		}
	}

	return( item );
}

static void
expand_tokens_context( NAIContext *context, NATokens *tokens )
{
	gchar *old, *new;

	old = na_object_get_try_exec( context );
	new = na_tokens_parse_for_display( tokens, old, FALSE );
	na_object_set_try_exec( context, new );
	g_free( old );
	g_free( new );

	old = na_object_get_show_if_registered( context );
	new = na_tokens_parse_for_display( tokens, old, FALSE );
	na_object_set_show_if_registered( context, new );
	g_free( old );
	g_free( new );

	old = na_object_get_show_if_true( context );
	new = na_tokens_parse_for_display( tokens, old, FALSE );
	na_object_set_show_if_true( context, new );
	g_free( old );
	g_free( new );

	old = na_object_get_show_if_running( context );
	new = na_tokens_parse_for_display( tokens, old, FALSE );
	na_object_set_show_if_running( context, new );
	g_free( old );
	g_free( new );
}

/*
 * @plugin: this #NautilusActions module instance.
 * @tree: a copy of the #NAPivot tree, where all fields - but
 *  displayable parameters expanded
 * @target: whether we target location or context menu, or toolbar.
 * @files: the current selection in the file-manager, as a #GList of #NASelectedInfo items
 * @tokens: a #NATokens object which embeds all possible values, regarding the
 *  current selection, for all parameters
 *
 * When building a menu for the toolbar, do not use menus hierarchy
 *
 * As menus, actions and profiles may embed parameters in their data,
 * all the hierarchy must be recursively re-parsed, and should be
 * re-checked for validity !
 */
static GList *
build_nautilus_menus( NautilusActions *plugin, GList *tree, guint target, GList *files, NATokens *tokens )
{
	static const gchar *thisfn = "nautilus_actions_build_nautilus_menus";
	GList *menus_list = NULL;
	GList *subitems, *submenu;
	GList *it;
	NAObjectProfile *profile;
	NautilusMenuItem *item;

	g_debug( "%s: plugin=%p, tree=%p, target=%d, files=%p (count=%d)",
			thisfn, ( void * ) plugin, ( void * ) tree, target,
			( void * ) files, g_list_length( files ));

	for( it = tree ; it ; it = it->next ){

		g_return_val_if_fail( NA_IS_OBJECT_ITEM( it->data ), NULL );

#ifdef NA_MAINTAINER_MODE
		/* check this here as a security though NAPivot should only have
		 * loaded valid and enabled items
		 */
		if( !na_object_is_enabled( it->data )){
			gchar *label = na_object_get_label( it->data );
			g_warning( "%s: '%s' item: enabled=%s, valid=%s", thisfn, label,
					na_object_is_enabled( it->data ) ? "True":"False",
					na_object_is_valid( it->data ) ? "True":"False" );
			g_free( label );
			continue;
		}
#endif

		/* but we have to re-check for validity as a label may become
		 * dynamically empty - thus the NAObjectItem invalid :(
		 */
		if( !na_object_is_valid( it->data )){
			continue;
		}

		if( !na_icontext_is_candidate( NA_ICONTEXT( it->data ), target, files )){
			continue;
		}

		/* recursively build sub-menus
		 */
		if( NA_IS_OBJECT_MENU( it->data )){
			subitems = na_object_get_items( it->data );
			g_debug( "%s: menu has %d items", thisfn, g_list_length( subitems ));
			submenu = build_nautilus_menus( plugin, subitems, target, files, tokens );
			g_debug( "%s: submenu has %d items", thisfn, g_list_length( submenu ));

			if( submenu ){
				if( target == ITEM_TARGET_TOOLBAR ){
					menus_list = g_list_concat( menus_list, submenu );

				} else {
					item = create_item_from_menu( NA_OBJECT_MENU( it->data ), submenu, target );
					menus_list = g_list_append( menus_list, item );
				}
			}
			continue;
		}

		g_return_val_if_fail( NA_IS_OBJECT_ACTION( it->data ), NULL );

		profile = get_candidate_profile( plugin, NA_OBJECT_ACTION( it->data ), target, files );
		if( profile ){
			item = create_item_from_profile( profile, target, files, tokens );
			menus_list = g_list_append( menus_list, item );
		}
	}

	return( menus_list );
}

/*
 * could also be a NAObjectAction method - but this is not used elsewhere
 */
static NAObjectProfile *
get_candidate_profile( NautilusActions *plugin, NAObjectAction *action, guint target, GList *files )
{
	static const gchar *thisfn = "nautilus_actions_get_candidate_profile";
	NAObjectProfile *candidate = NULL;
	gchar *action_label;
	gchar *profile_label;
	GList *profiles, *ip;

	action_label = na_object_get_label( action );
	profiles = na_object_get_items( action );

	for( ip = profiles ; ip && !candidate ; ip = ip->next ){
		NAObjectProfile *profile = NA_OBJECT_PROFILE( ip->data );

		if( na_icontext_is_candidate( NA_ICONTEXT( profile ), target, files )){
			profile_label = na_object_get_label( profile );
			g_debug( "%s: selecting %s (profile=%p '%s')", thisfn, action_label, ( void * ) profile, profile_label );
			g_free( profile_label );

			candidate = profile;
		}
	}

	g_free( action_label );

	return( candidate );
}

static NautilusMenuItem *
create_item_from_profile( NAObjectProfile *profile, guint target, GList *files, NATokens *tokens )
{
	NautilusMenuItem *item;
	NAObjectAction *action;
	NAObjectProfile *duplicate;

	action = NA_OBJECT_ACTION( na_object_get_parent( profile ));
	duplicate = NA_OBJECT_PROFILE( na_object_duplicate( profile ));
	na_object_set_parent( duplicate, NULL );

	item = create_menu_item( NA_OBJECT_ITEM( action ), target );

	/* attach a weak ref on the Nautilus menu item: our profile will be
	 * unreffed in weak notify function
	 */
	g_signal_connect( item,
				"activate",
				G_CALLBACK( execute_action ),
				duplicate );

	g_object_weak_ref( G_OBJECT( item ), ( GWeakNotify ) weak_notify_profile, duplicate );

	g_object_set_data_full( G_OBJECT( item ),
			"nautilus-actions-tokens",
			g_object_ref( tokens ),
			( GDestroyNotify ) g_object_unref );

	return( item );
}

/*
 * called _after_ the NautilusMenuItem has been finalized
 */
static void
weak_notify_profile( NAObjectProfile *profile, NautilusMenuItem *item )
{
	g_debug( "nautilus_actions_weak_notify_profile: profile=%p (ref_count=%d)",
			( void * ) profile, G_OBJECT( profile )->ref_count );

	g_object_unref( profile );
}

/*
 * note that each appended NautilusMenuItem is ref-ed by the NautilusMenu
 * we can so safely release our own ref on subitems after this function
 */
static NautilusMenuItem *
create_item_from_menu( NAObjectMenu *menu, GList *subitems, guint target )
{
	/*static const gchar *thisfn = "nautilus_actions_create_item_from_menu";*/
	NautilusMenuItem *item;

	item = create_menu_item( NA_OBJECT_ITEM( menu ), target );

	attach_submenu_to_item( item, subitems );
	nautilus_menu_item_list_free( subitems );

	/*g_debug( "%s: returning item=%p", thisfn, ( void * ) item );*/
	return( item );
}

static NautilusMenuItem *
create_menu_item( NAObjectItem *item, guint target )
{
	NautilusMenuItem *menu_item;
	gchar *id, *name, *label, *tooltip, *icon;

	id = na_object_get_id( item );
	name = g_strdup_printf( "%s-%s-%s-%d", PACKAGE, G_OBJECT_TYPE_NAME( item ), id, target );
	label = na_object_get_label( item );
	tooltip = na_object_get_tooltip( item );
	icon = na_object_get_icon( item );

	menu_item = nautilus_menu_item_new( name, label, tooltip, icon );

	g_free( icon );
 	g_free( tooltip );
 	g_free( label );
 	g_free( name );
 	g_free( id );

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

/*
 * callback triggered when an item is activated
 * path and parameters must yet been parsed against tokens
 *
 * note that if first parameter if of singular form, then we have to loop
 * againt the selected, each time replacing the singular parameters with
 * the current item of the selection
 */
static void
execute_action( NautilusMenuItem *item, NAObjectProfile *profile )
{
	static const gchar *thisfn = "nautilus_actions_execute_action";
	NATokens *tokens;

	g_debug( "%s: item=%p, profile=%p", thisfn, ( void * ) item, ( void * ) profile );

	tokens = NA_TOKENS( g_object_get_data( G_OBJECT( item ), "nautilus-actions-tokens" ));
	na_tokens_execute_action( tokens, profile );
}

/*
 * create a root submenu
 */
static GList *
create_root_menu( NautilusActions *plugin, GList *menu )
{
	static const gchar *thisfn = "nautilus_actions_create_root_menu";
	GList *nautilus_menu;
	NautilusMenuItem *root_item;
	gchar *icon;

	g_debug( "%s: plugin=%p, menu=%p (%d items)",
			thisfn, ( void * ) plugin, ( void * ) menu, g_list_length( menu ));

	if( !menu || !g_list_length( menu )){
		return( NULL );
	}

	icon = na_iabout_get_icon_name();
	root_item = nautilus_menu_item_new( "NautilusActionsExtensions",
				/* i18n: label of an automagic root submenu */
				_( "Nautilus-Actions actions" ),
				/* i18n: tooltip of an automagic root submenu */
				_( "A submenu which embeds the currently available Nautilus-Actions actions and menus" ),
				icon );
	attach_submenu_to_item( root_item, menu );
	nautilus_menu = g_list_append( NULL, root_item );
	g_free( icon );

	return( nautilus_menu );
}

/*
 * if there is a root submenu,
 * then add the about item to the end of the first level of this menu
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

	have_root_menu = FALSE;
	nautilus_menu = menu;

	if( g_list_length( menu ) == 1 ){
		root_item = NAUTILUS_MENU_ITEM( menu->data );
		g_object_get( G_OBJECT( root_item ), "menu", &first, NULL );
		if( first ){
			g_return_val_if_fail( NAUTILUS_IS_MENU( first ), NULL );
			have_root_menu = TRUE;
		}
	}

	if( have_root_menu ){
		icon = na_iabout_get_icon_name();

		about_item = nautilus_menu_item_new( "AboutNautilusActions",
					_( "About Nautilus-Actions" ),
					_( "Display some informations about Nautilus-Actions" ),
					icon );

		g_signal_connect_data( about_item,
					"activate",
					G_CALLBACK( execute_about ),
					plugin,
					NULL,
					0 );

		nautilus_menu_append_item( first, about_item );

		g_free( icon );
	}

	return( nautilus_menu );
}

static void
execute_about( NautilusMenuItem *item, NautilusActions *plugin )
{
	g_return_if_fail( NAUTILUS_IS_ACTIONS( plugin ));
	g_return_if_fail( NA_IS_IABOUT( plugin ));

	na_iabout_display( NA_IABOUT( plugin ));
}

/*
 * Not only the items list itself, but also several runtime preferences have
 * an effect on the display of items in file manager context menu.
 *
 * We of course monitor here all these informations; only asking NAPivot
 * for reloading its items when we detect the end of a burst of changes.
 *
 * Only when NAPivot has finished with reloading its items list, then we
 * inform the file manager that its items list has changed.
 */

/* signal emitted by NAPivot at the end of a burst of 'item-changed' signals
 * from i/o providers
 */
static void
on_pivot_items_changed_handler( NAPivot *pivot, NautilusActions *plugin )
{
	g_return_if_fail( NA_IS_PIVOT( pivot ));
	g_return_if_fail( NAUTILUS_IS_ACTIONS( plugin ));

	if( !plugin->private->dispose_has_run ){
		record_change_event( plugin );
	}
}

static void
on_runtime_preference_changed( const gchar *group, const gchar *key, gconstpointer newvalue, gboolean mandatory, NautilusActions *plugin )
{
	g_return_if_fail( NAUTILUS_IS_ACTIONS( plugin ));

	if( !plugin->private->dispose_has_run ){
		record_change_event( plugin );
	}
}

/* each signal handler or settings callback calls this function
 * so that we are sure that all change events are taken into account
 * in our timeout management
 *
 * create a new event source if it does not exists yet
 */
static void
record_change_event( NautilusActions *plugin )
{
	g_get_current_time( &st_last_event );

	if( !st_event_source_id ){
		st_event_source_id =
			g_timeout_add( st_burst_timeout, ( GSourceFunc ) on_change_event_timeout, plugin );
	}
}

/* this is called periodically at each 'st_burst_timeout' (100ms) interval
 * as soon as the event source is created
 *
 * if the last recorded change event, whose timestamp has been set in 'st_last_event'
 * is outside of our timeout, then we assume that the burst has finished
 *
 * we so reload the items, signal the file manager, and reset the event source.
 */
static gboolean
on_change_event_timeout( NautilusActions *plugin )
{
	static const gchar *thisfn = "nautilus_actions_on_change_event_timeout";
	GTimeVal now;
	gulong diff;
	gulong timeout_usec = 1000*st_burst_timeout;

	g_get_current_time( &now );
	diff = time_val_diff( &now, &st_last_event );
	if( diff < timeout_usec ){
		/* continue periodic calls */
		return( TRUE );
	}

	/* do what we have to and close the event source
	 */
	g_debug( "%s: timeout expired", thisfn );

	na_pivot_load_items( plugin->private->pivot );

	nautilus_menu_provider_emit_items_updated_signal( NAUTILUS_MENU_PROVIDER( plugin ));

	st_event_source_id = 0;

	return( FALSE );
}

/*
 * returns the difference in microseconds.
 */
static gulong
time_val_diff( const GTimeVal *recent, const GTimeVal *old )
{
	gulong microsec = 1000000 * ( recent->tv_sec - old->tv_sec );
	microsec += recent->tv_usec  - old->tv_usec;
	return( microsec );
}
