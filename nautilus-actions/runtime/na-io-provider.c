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

#include <api/na-iio-provider.h>
#include <api/na-object-api.h>

#include "na-gconf-utils.h"
#include "na-io-provider.h"
#include "na-iprefs.h"
#include "na-utils.h"

static GList *get_merged_items_list( const NAPivot *pivot, GList *providers, GSList **messages );
static GList *build_hierarchy( GList **tree, GSList *level_zero, gboolean list_if_empty );
static gint   search_item( const NAObject *obj, const gchar *uuid );
static GList *sort_tree( const NAPivot *pivot, GList *tree, GCompareFunc fn );
static GList *filter_unwanted_items( const NAPivot *pivot, GList *merged );
static GList *filter_unwanted_items_rec( GList *merged, gboolean load_disabled, gboolean load_invalid );

static guint  try_write_item( const NAIIOProvider *instance, NAObjectItem *item, GSList **messages );

/**
 * na_io_provider_register_callbacks:
 * @pivot: the current #NAPivot instance.
 *
 * Registers an handler for the item-changed signal for each loaded
 * NAIIOProvider plugin.
 */
void
na_io_provider_register_callbacks( const NAPivot *pivot )
{
	GList *providers, *ip;

	providers = na_pivot_get_providers( pivot, NA_IIO_PROVIDER_TYPE );

	for( ip = providers ; ip ; ip = ip->next ){
		g_signal_connect(
				( gpointer ) ip->data,
				NA_PIVOT_SIGNAL_ACTION_CHANGED,
				( GCallback ) na_pivot_item_changed_handler,
				( gpointer ) pivot );
	}

	na_pivot_free_providers( providers );
}

/**
 * na_io_provider_get_provider:
 * @pivot: the #NAPivot object.
 * @id: the id of the searched I/O provider.
 *
 * Returns: the found I/O provider, or NULL.
 *
 * The returned provider should be g_object_unref() by the caller.
 */
NAIIOProvider *
na_io_provider_get_provider( const NAPivot *pivot, const gchar *id )
{
	NAIIOProvider *provider;
	GList *providers, *ip;
	gchar *ip_id;

	provider = NULL;
	providers = na_pivot_get_providers( pivot, NA_IIO_PROVIDER_TYPE );

	for( ip = providers ; ip && !provider ; ip = ip->next ){
		ip_id = na_io_provider_get_id( pivot, NA_IIO_PROVIDER( ip->data ));
		if( ip_id ){
			if( !strcmp( ip_id, id )){
				provider = NA_IIO_PROVIDER( ip->data );
				g_object_ref( provider );
			}
			g_free( ip_id );
		}
	}

	na_pivot_free_providers( providers );

	return( provider );
}

/**
 * na_io_provider_get_writable_provider:
 * @pivot: the #NAPivot object.
 *
 * Returns: the first willing to write I/O provider, or NULL.
 *
 * The returned provider should be g_object_unref() by the caller.
 */
NAIIOProvider *
na_io_provider_get_writable_provider( const NAPivot *pivot )
{
	NAIIOProvider *provider;
	GList *providers, *ip;

	provider = NULL;
	providers = na_pivot_get_providers( pivot, NA_IIO_PROVIDER_TYPE );

	for( ip = providers ; ip && !provider ; ip = ip->next ){
		if( na_io_provider_is_willing_to_write( pivot, NA_IIO_PROVIDER( ip->data ))){
			provider = NA_IIO_PROVIDER( ip->data );
			g_object_ref( provider );
		}
	}

	na_pivot_free_providers( providers );

	return( provider );
}

/**
 * na_io_provider_get_id:
 * @pivot: the current #NAPivot instance.
 * @provider: the #NAIIOProvider whose id is to be returned.
 *
 * Returns: the provider's id as a newly allocated string which should
 * be g_free() by the caller, or NULL.
 */
gchar *
na_io_provider_get_id( const NAPivot *pivot, const NAIIOProvider *provider )
{
	gchar *id;

	id = NULL;
	if( NA_IIO_PROVIDER_GET_INTERFACE( provider )->get_id ){
		id = NA_IIO_PROVIDER_GET_INTERFACE( provider )->get_id( provider );
	}

	return( id );
}

/**
 * na_io_provider_get_name:
 * @pivot: the current #NAPivot instance.
 * @provider: the #NAIIOProvider whose name is to be returned.
 *
 * Returns: a displayble name for the provider, as a newly allocated
 * string which should be g_free() by the caller.
 */
gchar *
na_io_provider_get_name( const NAPivot *pivot, const NAIIOProvider *provider )
{
	gchar *name;

	name = NULL;
	if( NA_IIO_PROVIDER_GET_INTERFACE( provider )->get_name ){
		name = NA_IIO_PROVIDER_GET_INTERFACE( provider )->get_name( provider );
	} else {
		name = g_strdup( "" );
	}

	return( name );
}

/**
 * na_io_provider_get_version:
 * @pivot: the current #NAPivot instance.
 * @provider: the #NAIIOProvider whose id is to be returned.
 *
 * Returns: the API's version the provider supports.
 */
guint
na_io_provider_get_version( const NAPivot *pivot, const NAIIOProvider *provider )
{
	guint version;

	version = 1;
	if( NA_IIO_PROVIDER_GET_INTERFACE( provider )->get_version ){
		version = NA_IIO_PROVIDER_GET_INTERFACE( provider )->get_version( provider );
	}

	return( version );
}

/**
 * na_io_provider_is_willing_to_write:
 * @pivot: the current #NAPivot instance.
 * @provider: the #NAIIOProvider whose name is to be returned.
 *
 * Returns: %TRUE if the I/O provider is willing to write _and_ it didn't
 * has been locked down by a sysadmin.
 */
gboolean
na_io_provider_is_willing_to_write( const NAPivot *pivot, const NAIIOProvider *provider )
{
	/*static const gchar *thisfn = "na_io_provider_is_willing_to_write";*/
	gboolean writable;
	gboolean locked;
	GConfClient *gconf;
	gchar *id;
	gchar *key;

	writable = FALSE;
	locked = FALSE;

	if( NA_IIO_PROVIDER_GET_INTERFACE( provider )->is_willing_to_write ){

		writable = NA_IIO_PROVIDER_GET_INTERFACE( provider )->is_willing_to_write( provider );

		if( writable ){
			id = na_io_provider_get_id( pivot, provider );
			key = g_strdup_printf( "%s/mandatory/%s/locked", NAUTILUS_ACTIONS_GCONF_BASEDIR, id );
			gconf = na_iprefs_get_gconf_client( NA_IPREFS( pivot ));
			locked = na_gconf_utils_read_bool( gconf, key, TRUE, locked );
			/*g_debug( "%s: id=%s, locked=%s", thisfn, id, locked ? "True":"False" );*/
			g_free( key );
			g_free( id );
		}
	}

	return( writable && !locked );
}

/**
 * na_io_provider_read_items:
 * @pivot: the #NAPivot object which owns the list of registered I/O
 * storage providers.
 * @messages: error messages.
 *
 * Loads the tree from I/O storage subsystems.
 *
 * Returns: a #GList of newly allocated objects as a hierarchical tree
 * in display order. This tree may contain #NAActionMenu menus and
 * #NAAction actions and their #NAActionProfile profiles.
 */
GList *
na_io_provider_read_items( const NAPivot *pivot, GSList **messages )
{
	static const gchar *thisfn = "na_io_provider_read_items";
	GList *providers;
	GList *merged, *hierarchy, *filtered;
	GSList *level_zero;
	gint order_mode;

	g_debug( "%s: pivot=%p, messages=%p", thisfn, ( void * ) pivot, ( void * ) messages );
	g_return_val_if_fail( NA_IS_PIVOT( pivot ), NULL );
	g_return_val_if_fail( NA_IS_IPREFS( pivot ), NULL );

	hierarchy = NULL;
	*messages = NULL;

	providers = na_pivot_get_providers( pivot, NA_IIO_PROVIDER_TYPE );
	merged = get_merged_items_list( pivot, providers, messages );
	na_pivot_free_providers( providers );

	level_zero = na_iprefs_get_level_zero_items( NA_IPREFS( pivot ));
	hierarchy = build_hierarchy( &merged, level_zero, TRUE );

	/* items that stay left in the merged list are simply appended
	 * to the built hierarchy, and level zero is updated accordingly
	 */
	if( merged ){
		g_debug( "%s: %d items left appended to the hierarchy", thisfn, g_list_length( merged ));
		hierarchy = g_list_concat( hierarchy, merged );
	}

	if( merged || !level_zero || !g_slist_length( level_zero )){
		g_debug( "%s: rewriting level-zero", thisfn );
		na_pivot_write_level_zero( pivot, hierarchy );
	}

	na_utils_free_string_list( level_zero );

	order_mode = na_iprefs_get_order_mode( NA_IPREFS( pivot ));
	switch( order_mode ){
		case IPREFS_ORDER_ALPHA_ASCENDING:
			hierarchy = sort_tree( pivot, hierarchy, ( GCompareFunc ) na_pivot_sort_alpha_asc );
			break;

		case IPREFS_ORDER_ALPHA_DESCENDING:
			hierarchy = sort_tree( pivot, hierarchy, ( GCompareFunc ) na_pivot_sort_alpha_desc );
			break;

		case IPREFS_ORDER_MANUAL:
		default:
			break;
	}

	filtered = filter_unwanted_items( pivot, hierarchy );
	g_list_free( hierarchy );

	g_debug( "%s: tree after filtering and reordering (if any)", thisfn );
	na_object_dump_tree( filtered );
	g_debug( "%s: end of tree", thisfn );

	return( filtered );
}

/*
 * returns a concatened list of readen actions / menus
 */
static GList *
get_merged_items_list( const NAPivot *pivot, GList *providers, GSList **messages )
{
	GList *ip;
	GList *merged = NULL;
	GList *list, *item;
	NAIIOProvider *instance;

	for( ip = providers ; ip ; ip = ip->next ){

		instance = NA_IIO_PROVIDER( ip->data );
		if( NA_IIO_PROVIDER_GET_INTERFACE( instance )->read_items ){

			list = NA_IIO_PROVIDER_GET_INTERFACE( instance )->read_items( instance, messages );

			for( item = list ; item ; item = item->next ){

				na_object_set_provider( item->data, instance );
				na_object_dump( item->data );
			}

			merged = g_list_concat( merged, list );
		}
	}

	return( merged );
}

/*
 * recursively builds the hierarchy
 * note that we add a ref count to object installed in new hierarchy
 * so that eventual non-referenced objects will not cause memory leak
 * when releasing initial merged tree
 */
static GList *
build_hierarchy( GList **tree, GSList *level_zero, gboolean list_if_empty )
{
	static const gchar *thisfn = "na_iio_provider_build_hierarchy";
	GList *hierarchy, *it;
	GSList *ilevel;
	GSList *subitems_ids;
	GList *subitems;

	hierarchy = NULL;

	if( g_slist_length( level_zero )){
		for( ilevel = level_zero ; ilevel ; ilevel = ilevel->next ){
			g_debug( "%s: uuid=%s", thisfn, ( gchar * ) ilevel->data );
			it = g_list_find_custom( *tree, ilevel->data, ( GCompareFunc ) search_item );
			if( it ){
				hierarchy = g_list_append( hierarchy, it->data );

				g_debug( "%s: uuid=%s: %s (%p) appended to hierarchy %p",
						thisfn, ( gchar * ) ilevel->data, G_OBJECT_TYPE_NAME( it->data ), ( void * ) it->data, ( void * ) hierarchy );

				*tree = g_list_remove_link( *tree, it );

				if( NA_IS_OBJECT_MENU( it->data )){
					subitems_ids = na_object_item_get_items_string_list( NA_OBJECT_ITEM( it->data ));
					subitems = build_hierarchy( tree, subitems_ids, FALSE );
					na_object_set_items_list( it->data, subitems );
					na_utils_free_string_list( subitems_ids );
				}
			}
		}
	}

	/* if level-zero list is empty, we consider that all actions go to it
	 */
	else if( list_if_empty ){
		for( it = *tree ; it ; it = it->next ){
			hierarchy = g_list_append( hierarchy, it->data );
		}
		g_list_free( *tree );
		*tree = NULL;
	}

	return( hierarchy );
}

/*
 * returns zero when obj has the required uuid
 */
static gint
search_item( const NAObject *obj, const gchar *uuid )
{
	gchar *obj_id;
	gint ret = 1;

	if( NA_IS_OBJECT_ITEM( obj )){
		obj_id = na_object_get_id( obj );
		ret = strcmp( obj_id, uuid );
		g_free( obj_id );
	}

	return( ret );
}

static GList *
sort_tree( const NAPivot *pivot, GList *tree, GCompareFunc fn )
{
	GList *sorted;
	GList *items, *it;

	sorted = g_list_sort( tree, fn );

	/* recursively sort each level of the tree
	 */
	for( it = sorted ; it ; it = it->next ){
		if( NA_IS_OBJECT_MENU( it->data )){
			items = na_object_get_items_list( it->data );
			items = sort_tree( pivot, items, fn );
			na_object_set_items_list( it->data, items );
		}
	}

	return( sorted );
}

static GList *
filter_unwanted_items( const NAPivot *pivot, GList *hierarchy )
{
	gboolean load_disabled;
	gboolean load_invalid;
	GList *it;
	GList *filtered;

	load_disabled = na_pivot_is_disable_loadable( pivot );
	load_invalid = na_pivot_is_invalid_loadable( pivot );

	for( it = hierarchy ; it ; it = it->next ){
		na_object_check_status( it->data );
	}

	filtered = filter_unwanted_items_rec( hierarchy, load_disabled, load_invalid );

	return( filtered );
}

/*
 * build a dest tree from a source tree, removing filtered items
 * an item is filtered if it is invalid (and not loading invalid ones)
 * or disabled (and not loading disabled ones)
 */
static GList *
filter_unwanted_items_rec( GList *hierarchy, gboolean load_disabled, gboolean load_invalid )
{
	static const gchar *thisfn = "na_io_provider_filter_unwanted_items_rec";
	GList *subitems, *subitems_f;
	GList *it, *itnext;
	GList *filtered;
	gboolean selected;
	gchar *label;

	filtered = NULL;
	for( it = hierarchy ; it ; it = itnext ){

		itnext = it->next;
		selected = FALSE;

		if( NA_IS_OBJECT_PROFILE( it->data )){
			if( na_object_is_valid( it->data ) || load_invalid ){
				filtered = g_list_append( filtered, it->data );
				selected = TRUE;
			}
		}

		if( NA_IS_OBJECT_ITEM( it->data )){
			if(( na_object_is_enabled( it->data ) || load_disabled ) &&
				( na_object_is_valid( it->data ) || load_invalid )){

				subitems = na_object_get_items_list( it->data );
				subitems_f = filter_unwanted_items_rec( subitems, load_disabled, load_invalid );
				na_object_set_items_list( it->data, subitems_f );
				filtered = g_list_append( filtered, it->data );
				selected = TRUE;
			}
		}

		if( !selected ){
			label = na_object_get_label( it->data );
			g_debug( "%s: filtering %p (%s) '%s'", thisfn, ( void * ) it->data, G_OBJECT_TYPE_NAME( it->data ), label );
			g_free( label );
			na_object_unref( it->data );
		}
	}

	return( filtered );
}

/**
 * na_io_provider_write_item:
 * @pivot: the #NAPivot object which owns the list of registered I/O
 * storage providers. if NULL, @action must already have registered
 * its own provider.
 * @item: a #NAObjectItem to be written by the storage subsystem.
 * @messages: error messages.
 *
 * Writes an @item to a willing-to storage subsystem.
 *
 * Returns: the NAIIOProvider return code.
 */
guint
na_io_provider_write_item( const NAPivot *pivot, NAObjectItem *item, GSList **messages )
{
	static const gchar *thisfn = "na_io_provider_write_item";
	guint ret;
	NAIIOProvider *instance;
	NAIIOProvider *bad_instance;
	GList *providers, *ip;

	g_debug( "%s: pivot=%p (%s), item=%p (%s), messages=%p", thisfn,
			( void * ) pivot, G_OBJECT_TYPE_NAME( pivot ),
			( void * ) item, G_OBJECT_TYPE_NAME( item ),
			( void * ) messages );

	g_return_val_if_fail(( NA_IS_PIVOT( pivot ) || !pivot ), NA_IIO_PROVIDER_PROGRAM_ERROR );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), NA_IIO_PROVIDER_PROGRAM_ERROR );

	ret = NA_IIO_PROVIDER_NOT_WRITABLE;
	bad_instance = NULL;
	*messages = NULL;

	/* try to write to the original provider of the item
	 */
	instance = NA_IIO_PROVIDER( na_object_get_provider( item ));
	if( instance ){
		ret = try_write_item( instance, item, messages );
		if( ret == NA_IIO_PROVIDER_NOT_WILLING_TO_WRITE || ret == NA_IIO_PROVIDER_NOT_WRITABLE ){
			bad_instance = instance;
			instance = NULL;
		}
	}

	/* else, search for a provider which is willing to write the item
	 */
	if( !instance && pivot ){
		providers = na_pivot_get_providers( pivot, NA_IIO_PROVIDER_TYPE );
		for( ip = providers ; ip ; ip = ip->next ){

			instance = NA_IIO_PROVIDER( ip->data );
			if( !bad_instance || instance != bad_instance ){
				ret = try_write_item( instance, item, messages );
				if( ret == NA_IIO_PROVIDER_WRITE_OK ){
					na_object_set_provider( item, instance );
					break;
				}
			}
		}
		na_pivot_free_providers( providers );
	}

	return( ret );
}

static guint
try_write_item( const NAIIOProvider *provider, NAObjectItem *item, GSList **messages )
{
	static const gchar *thisfn = "na_io_provider_try_write_item";
	guint ret;

	g_debug( "%s: provider=%p, item=%p, messages=%p",
			thisfn, ( void * ) provider, ( void * ) item, ( void * ) messages );

	if( !NA_IIO_PROVIDER_GET_INTERFACE( provider )->is_willing_to_write( provider )){
		return( NA_IIO_PROVIDER_NOT_WILLING_TO_WRITE );
	}

	if( !NA_IIO_PROVIDER_GET_INTERFACE( provider )->is_writable( provider, item )){
		return( NA_IIO_PROVIDER_NOT_WRITABLE );
	}

	if( !NA_IIO_PROVIDER_GET_INTERFACE( provider )->delete_item ||
		!NA_IIO_PROVIDER_GET_INTERFACE( provider )->write_item ){
		return( NA_IIO_PROVIDER_NOT_WILLING_TO_WRITE );
	}

	ret = NA_IIO_PROVIDER_GET_INTERFACE( provider )->delete_item( provider, item, messages );
	if( ret != NA_IIO_PROVIDER_WRITE_OK ){
		return( ret );
	}

	return( NA_IIO_PROVIDER_GET_INTERFACE( provider )->write_item( provider, item, messages ));
}

/**
 * na_io_provider_delete_item:
 * @pivot: the #NAPivot object which owns the list of registered I/O
 * storage providers.
 * @item: the #NAObjectItem item to be deleted.
 * @messages: error messages.
 *
 * Deletes an item (action or menu) from the storage subsystem.
 *
 * Returns: the NAIIOProvider return code.
 *
 * Note that a new item, not already written to an I/O subsystem,
 * doesn't have any attached provider. We so do nothing...
 */
guint
na_io_provider_delete_item( const NAPivot *pivot, const NAObjectItem *item, GSList **messages )
{
	static const gchar *thisfn = "na_io_provider_delete_item";
	guint ret;
	NAIIOProvider *instance;

	g_debug( "%s: pivot=%p (%s), item=%p (%s), messages=%p", thisfn,
			( void * ) pivot, G_OBJECT_TYPE_NAME( pivot ),
			( void * ) item, G_OBJECT_TYPE_NAME( item ),
			( void * ) messages );

	g_return_val_if_fail( NA_IS_PIVOT( pivot ), NA_IIO_PROVIDER_PROGRAM_ERROR );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), NA_IIO_PROVIDER_PROGRAM_ERROR );

	*messages = NULL;
	ret = NA_IIO_PROVIDER_NOT_WRITABLE;
	instance = NA_IIO_PROVIDER( na_object_get_provider( item ));

	if( instance ){
		g_return_val_if_fail( NA_IS_IIO_PROVIDER( instance ), NA_IIO_PROVIDER_PROGRAM_ERROR );

		if( NA_IIO_PROVIDER_GET_INTERFACE( instance )->delete_item ){
			ret = NA_IIO_PROVIDER_GET_INTERFACE( instance )->delete_item( instance, item, messages );
		}
	}

	return( ret );
}
