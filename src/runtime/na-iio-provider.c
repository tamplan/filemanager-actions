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

#include "na-object-api.h"
#include "na-iio-provider.h"
#include "na-iprefs.h"
#include "na-utils.h"

/* private interface data
 */
struct NAIIOProviderInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

static gboolean st_initialized = FALSE;
static gboolean st_finalized = FALSE;

static GType    register_type( void );
static void     interface_base_init( NAIIOProviderInterface *klass );
static void     interface_base_finalize( NAIIOProviderInterface *klass );

static GList   *build_hierarchy( GList **tree, GSList *level_zero, gboolean list_if_empty );
static gint     search_item( const NAObject *obj, const gchar *uuid );
static GList   *get_merged_items_list( const NAPivot *pivot, GList *providers );

static guint    try_write_item( const NAIIOProvider *instance, NAObject *item, gchar **message );

static gboolean do_is_willing_to_write( const NAIIOProvider *instance );
static gboolean do_is_writable( const NAIIOProvider *instance, const NAObject *item );

static GList   *sort_tree( const NAPivot *pivot, GList *tree, GCompareFunc fn );

/**
 * Registers the GType of this interface.
 */
GType
na_iio_provider_get_type( void )
{
	static GType object_type = 0;

	if( !object_type ){
		object_type = register_type();
	}

	return( object_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "na_iio_provider_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NAIIOProviderInterface ),
		( GBaseInitFunc ) interface_base_init,
		( GBaseFinalizeFunc ) interface_base_finalize,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_INTERFACE, "NAIIOProvider", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( NAIIOProviderInterface *klass )
{
	static const gchar *thisfn = "na_iio_provider_interface_base_init";

	if( !st_initialized ){
		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NAIIOProviderInterfacePrivate, 1 );

		klass->read_items_list = NULL;
		klass->is_willing_to_write = do_is_willing_to_write;
		klass->is_writable = do_is_writable;
		klass->write_item = NULL;
		klass->delete_item = NULL;

		st_initialized = TRUE;
	}
}

static void
interface_base_finalize( NAIIOProviderInterface *klass )
{
	static const gchar *thisfn = "na_iio_provider_interface_base_finalize";

	if( !st_finalized ){

		st_finalized = TRUE;

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );
	}
}

/**
 * na_iio_provider_get_items_tree:
 * @pivot: the #NAPivot object which owns the list of registered I/O
 * storage providers.
 *
 * Loads the tree from I/O storage subsystems.
 *
 * Returns: a #GList of newly allocated objects as a hierarchical tree
 * in display order. This tree may contain #NAActionMenu menus and
 * #NAAction actions and their #NAActionProfile profiles.
 */
GList *
na_iio_provider_get_items_tree( const NAPivot *pivot )
{
	static const gchar *thisfn = "na_iio_provider_get_items_tree";
	GList *providers;
	GList *merged, *hierarchy, *it;
	GSList *level_zero;
	gint order_mode;

	g_debug( "%s: pivot=%p", thisfn, ( void * ) pivot );
	g_return_val_if_fail( NA_IS_PIVOT( pivot ), NULL );
	g_return_val_if_fail( NA_IS_IPREFS( pivot ), NULL );

	hierarchy = NULL;

	if( st_initialized && !st_finalized ){

		providers = na_pivot_get_providers( pivot, NA_IIO_PROVIDER_TYPE );
		merged = get_merged_items_list( pivot, providers );
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
		/*na_object_free_items_list( merged );*/

		g_debug( "%s: tree before alphabetical reordering (if any)", thisfn );
		na_object_dump_tree( hierarchy );
		g_debug( "%s: end of tree", thisfn );

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

		for( it = hierarchy ; it ; it = it->next ){
			na_object_check_edition_status( it->data );
		}
	}

	return( hierarchy );
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

/*
 * returns a concatened list of readen actions / menus
 */
static GList *
get_merged_items_list( const NAPivot *pivot, GList *providers )
{
	GList *ip;
	GList *merged = NULL;
	GList *list, *item;
	NAIIOProvider *instance;

	for( ip = providers ; ip ; ip = ip->next ){

		instance = NA_IIO_PROVIDER( ip->data );
		if( NA_IIO_PROVIDER_GET_INTERFACE( instance )->read_items_list ){

			list = NA_IIO_PROVIDER_GET_INTERFACE( instance )->read_items_list( instance );

			for( item = list ; item ; item = item->next ){

				na_object_set_provider( item->data, instance );
				na_object_dump( item->data );
			}

			merged = g_list_concat( merged, list );
		}
	}

	return( merged );
}

/**
 * na_iio_provider_write_item:
 * @pivot: the #NAPivot object which owns the list of registered I/O
 * storage providers. if NULL, @action must already have registered
 * its own provider.
 * @item: a #NAObject to be written by the storage subsystem.
 * @message: the I/O provider can allocate and store here an error
 * message.
 *
 * Writes an @item to a willing-to storage subsystem.
 *
 * Returns: the NAIIOProvider return code.
 */
guint
na_iio_provider_write_item( const NAPivot *pivot, NAObject *item, gchar **message )
{
	static const gchar *thisfn = "na_iio_provider_write_item";
	guint ret;
	NAIIOProvider *instance;
	NAIIOProvider *bad_instance;
	GList *providers, *ip;

	g_debug( "%s: pivot=%p (%s), item=%p (%s), message=%p", thisfn,
			( void * ) pivot, G_OBJECT_TYPE_NAME( pivot ),
			( void * ) item, G_OBJECT_TYPE_NAME( item ),
			( void * ) message );

	g_return_val_if_fail(( NA_IS_PIVOT( pivot ) || !pivot ), NA_IIO_PROVIDER_PROGRAM_ERROR );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), NA_IIO_PROVIDER_PROGRAM_ERROR );

	ret = NA_IIO_PROVIDER_NOT_WILLING_TO_WRITE;

	if( st_initialized && !st_finalized ){

		ret = NA_IIO_PROVIDER_NOT_WRITABLE;
		bad_instance = NULL;

		/* try to write to the original provider of the item
		 */
		instance = NA_IIO_PROVIDER( na_object_get_provider( item ));
		if( instance ){
			ret = try_write_item( instance, item, message );
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
				if( !bad_instance || bad_instance != instance ){
					ret = try_write_item( instance, item, message );
					if( ret == NA_IIO_PROVIDER_WRITE_OK ){
						break;
					}
				}
			}
			na_pivot_free_providers( providers );
		}
	}

	return( ret );
}

static guint
try_write_item( const NAIIOProvider *provider, NAObject *item, gchar **message )
{
	static const gchar *thisfn = "na_iio_provider_try_write_item";
	guint ret;

	g_debug( "%s: provider=%p, item=%p, message=%p",
			thisfn, ( void * ) provider, ( void * ) item, ( void * ) message );

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

	ret = NA_IIO_PROVIDER_GET_INTERFACE( provider )->delete_item( provider, item, message );
	if( ret != NA_IIO_PROVIDER_WRITE_OK ){
		return( ret );
	}

	return( NA_IIO_PROVIDER_GET_INTERFACE( provider )->write_item( provider, item, message ));
}

/**
 * na_iio_provider_delete_item:
 * @pivot: the #NAPivot object which owns the list of registered I/O
 * storage providers.
 * @item: the #NAObject item to be deleted.
 * @message: the I/O provider can allocate and store here an error
 * message.
 *
 * Deletes an item (action or menu) from the storage subsystem.
 *
 * Returns: the NAIIOProvider return code.
 *
 * Note that a new item, not already written to an I/O subsystem,
 * doesn't have any attached provider. We so do nothing...
 */
guint
na_iio_provider_delete_item( const NAPivot *pivot, const NAObject *item, gchar **message )
{
	static const gchar *thisfn = "na_iio_provider_delete_item";
	guint ret;
	NAIIOProvider *instance;

	g_debug( "%s: pivot=%p (%s), item=%p (%s), message=%p", thisfn,
			( void * ) pivot, G_OBJECT_TYPE_NAME( pivot ),
			( void * ) item, G_OBJECT_TYPE_NAME( item ),
			( void * ) message );

	g_return_val_if_fail( NA_IS_PIVOT( pivot ), NA_IIO_PROVIDER_PROGRAM_ERROR );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), NA_IIO_PROVIDER_PROGRAM_ERROR );

	ret = NA_IIO_PROVIDER_NOT_WILLING_TO_WRITE;

	if( st_initialized && !st_finalized ){

		ret = NA_IIO_PROVIDER_NOT_WRITABLE;
		instance = NA_IIO_PROVIDER( na_object_get_provider( item ));

		if( instance ){
			g_return_val_if_fail( NA_IS_IIO_PROVIDER( instance ), NA_IIO_PROVIDER_PROGRAM_ERROR );

			if( NA_IIO_PROVIDER_GET_INTERFACE( instance )->delete_item ){
				ret = NA_IIO_PROVIDER_GET_INTERFACE( instance )->delete_item( instance, item, message );
			}
		}
	}

	return( ret );
}

static gboolean
do_is_willing_to_write( const NAIIOProvider *instance )
{
	return( FALSE );
}

static gboolean
do_is_writable( const NAIIOProvider *instance, const NAObject *item )
{
	return( FALSE );
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
		if( NA_IS_OBJECT_ITEM( it->data )){
			items = na_object_get_items_list( it->data );
			items = sort_tree( pivot, items, fn );
			na_object_set_items_list( it->data, items );
		}
	}

	return( sorted );
}
