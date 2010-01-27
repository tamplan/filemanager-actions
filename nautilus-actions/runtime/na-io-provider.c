/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010 Pierre Wieser and others (see AUTHORS)
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

/* private class data
 */
struct NAIOProviderClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NAIOProviderPrivate {
	gboolean       dispose_has_run;
	gchar         *id;
	gchar         *name;
	gboolean       read_at_startup;
	gboolean       writable;
	NAIIOProvider *provider;
	gulong         item_changed_handler;
};

/* NAIOProvider properties
 */
enum {
	IO_PROVIDER_PROP_ID_ID = 1,
	IO_PROVIDER_PROP_PROVIDER_ID,
};

#define IO_PROVIDER_PROP_ID				"na-io-provider-prop-id"
#define IO_PROVIDER_PROP_PROVIDER		"na-io-provider-prop-provider"

static GObjectClass *st_parent_class = NULL;
static GList        *st_io_providers = NULL;

static GType  register_type( void );
static void   class_init( NAIOProviderClass *klass );
static void   instance_init( GTypeInstance *instance, gpointer klass );
static void   instance_constructed( GObject *object );
static void   instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void   instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void   instance_dispose( GObject *object );
static void   instance_finalize( GObject *object );

static void   setup_io_providers( const NAPivot *pivot, GSList *priority );
static GList *allocate_ordered_providers( GSList *priority );
static GList *merge_available_io_providers( const NAPivot *pivot, GList *ordered_providers );
static void   io_provider_set_provider( const NAPivot *pivot, NAIOProvider *provider, NAIIOProvider *instance );
static GList *add_io_providers_from_prefs( const NAPivot *pivot, GList *runtime_providers );

static void   update_io_providers( const NAPivot *pivot, GList *runtime_providers );
static GList *filter_io_providers( GList *runtime_providers, NAPivotIOProviderSet set );

static void   dump( const NAIOProvider *provider );

static GList *get_merged_items_list( const NAPivot *pivot, GList *providers, GSList **messages );
static GList *build_hierarchy( GList **tree, GSList *level_zero, gboolean list_if_empty );
static gint   search_item( const NAObject *obj, const gchar *uuid );
static GList *sort_tree( const NAPivot *pivot, GList *tree, GCompareFunc fn );
static GList *filter_unwanted_items( const NAPivot *pivot, GList *merged );
static GList *filter_unwanted_items_rec( GList *merged, gboolean load_disabled, gboolean load_invalid );

static guint  try_write_item( const NAIIOProvider *instance, NAObjectItem *item, GSList **messages );

GType
na_io_provider_get_type( void )
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
	static const gchar *thisfn = "na_io_provider_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NAIOProviderClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NAIOProvider ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "NAIOProvider", &info, 0 );

	return( type );
}

static void
class_init( NAIOProviderClass *klass )
{
	static const gchar *thisfn = "na_io_provider_class_init";
	GObjectClass *object_class;
	GParamSpec *spec;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->constructed = instance_constructed;
	object_class->set_property = instance_set_property;
	object_class->get_property = instance_get_property;
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	spec = g_param_spec_string(
			IO_PROVIDER_PROP_ID,
			"I/O Provider Id",
			"Internal identifiant of the I/O provider", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, IO_PROVIDER_PROP_ID_ID, spec );

	spec = g_param_spec_pointer(
			IO_PROVIDER_PROP_PROVIDER,
			"NAIIOProvider",
			"A reference on the NAIIOProvider plugin module",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, IO_PROVIDER_PROP_PROVIDER_ID, spec );

	klass->private = g_new0( NAIOProviderClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "na_io_provider_instance_init";
	NAIOProvider *self;

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );
	g_return_if_fail( NA_IS_IO_PROVIDER( instance ));
	self = NA_IO_PROVIDER( instance );

	self->private = g_new0( NAIOProviderPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->id = NULL;
	self->private->name = NULL;
	self->private->read_at_startup = TRUE;
	self->private->writable = TRUE;
	self->private->provider = NULL;
	self->private->item_changed_handler = 0;
}

static void
instance_constructed( GObject *object )
{
	static const gchar *thisfn = "na_io_provider_instance_constructed";
	NAIOProvider *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_return_if_fail( NA_IS_IO_PROVIDER( object ));
	self = NA_IO_PROVIDER( object );

	if( !self->private->dispose_has_run ){

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->constructed ){
			G_OBJECT_CLASS( st_parent_class )->constructed( object );
		}
	}
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	NAIOProvider *self;

	g_return_if_fail( NA_IS_IO_PROVIDER( object ));
	self = NA_IO_PROVIDER( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case IO_PROVIDER_PROP_ID_ID:
				g_value_set_string( value, self->private->id );
				break;

			case IO_PROVIDER_PROP_PROVIDER_ID:
				g_value_set_pointer( value, self->private->provider );
				break;

			default:
				G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
				break;
		}
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	NAIOProvider *self;

	g_return_if_fail( NA_IS_IO_PROVIDER( object ));
	self = NA_IO_PROVIDER( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case IO_PROVIDER_PROP_ID_ID:
				g_free( self->private->id );
				self->private->id = g_value_dup_string( value );
				break;

			case IO_PROVIDER_PROP_PROVIDER_ID:
				self->private->provider = g_value_get_pointer( value );
				break;
		}
	}
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "na_io_provider_instance_dispose";
	NAIOProvider *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_return_if_fail( NA_IS_IO_PROVIDER( object ));
	self = NA_IO_PROVIDER( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		if( self->private->provider ){

			if( g_signal_handler_is_connected( self->private->provider, self->private->item_changed_handler )){
				g_signal_handler_disconnect( self->private->provider, self->private->item_changed_handler );
			}

			g_object_unref( self->private->provider );
		}

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "na_io_provider_instance_finalize";
	NAIOProvider *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_return_if_fail( NA_IS_IO_PROVIDER( object ));
	self = NA_IO_PROVIDER( object );

	g_free( self->private->id );
	g_free( self->private->name );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * na_io_provider_terminate:
 *
 * Called by on #NAPivot dispose(), free here resources allocated to
 * the I/O providers.
 */
void
na_io_provider_terminate( void )
{
	g_list_foreach( st_io_providers, ( GFunc ) g_object_unref, NULL );
	g_list_free( st_io_providers );
	st_io_providers = NULL;
}

/**
 * na_io_provider_get_providers_list:
 * @pivot: the current #NAPivot instance.
 *
 * Returns: the list of I/O providers for @pivot.
 *
 * The returned list is owned by #NAIOProvider class, and should not be
 * released by the caller.
 */
GList *
na_io_provider_get_providers_list( const NAPivot *pivot )
{
	GSList *order;

	g_return_val_if_fail( NA_IS_PIVOT( pivot ), NULL );

	if( !st_io_providers ){

		order = na_iprefs_read_string_list( NA_IPREFS( pivot ), IO_PROVIDER_KEY_ORDER, NULL );
		setup_io_providers( pivot, order );
		na_utils_free_string_list( order );
	}

	return( st_io_providers );
}

/*
 * build the static list of I/O providers, depending of setup of NAPivot
 * doing required initializations
 */
static void
setup_io_providers( const NAPivot *pivot, GSList *priority )
{
	GList *ordered_providers;
	GList *merged_providers;
	NAPivotIOProviderSet set;
	GList *all_providers;
	GList *filtered_providers;

	g_return_if_fail( st_io_providers == NULL );

	/* allocate the ordered list */
	ordered_providers = allocate_ordered_providers( priority );

	/* merge with available I/O providers */
	merged_providers = merge_available_io_providers( pivot, ordered_providers );

	/* add providers found in prefs, in not stuck to those available at runtime */
	set = na_pivot_get_io_provider_set( pivot );

	if( set & PIVOT_IO_PROVIDER_AVAILABLE ){
		all_providers = merged_providers;

	} else {
		all_providers = add_io_providers_from_prefs( pivot, merged_providers );
	}

	/* update all those providers from prefs */
	update_io_providers( pivot, all_providers );

	/* filter if we don't want not readable or not writable */
	filtered_providers = filter_io_providers( all_providers, set );

	st_io_providers = filtered_providers;
}

static GList *
allocate_ordered_providers( GSList *priority )
{
	GSList *ip;
	NAIOProvider *provider;
	GList *providers;

	providers = NULL;

	for( ip = priority ; ip ; ip = ip->next ){
		provider = g_object_new( NA_IO_PROVIDER_TYPE, IO_PROVIDER_PROP_ID, ( const gchar * ) ip->data, NULL );
		providers = g_list_append( providers, provider );
	}

	return( providers );
}

/*
 * merge the ordered list of I/O providers (which have only Id)
 * with those found available at runtime
 */
static GList *
merge_available_io_providers( const NAPivot *pivot, GList *ordered )
{
	static const gchar *thisfn = "na_io_provider_merge_available_io_providers";
	GList *merged;
	GList *module_objects, *im;
	gchar *id;
	NAIOProvider *provider;

	merged = ordered;

	module_objects = na_pivot_get_providers( pivot, NA_IIO_PROVIDER_TYPE );
	for( im = module_objects ; im ; im = im->next ){

		id = NULL;
		if( NA_IIO_PROVIDER_GET_INTERFACE( NA_IIO_PROVIDER( im->data ))->get_id ){
			id = NA_IIO_PROVIDER_GET_INTERFACE( NA_IIO_PROVIDER( im->data ))->get_id( NA_IIO_PROVIDER( im->data ) );
		} else {
			g_warning( "%s: NAIIOProvider %p doesn't support get_id() interface", thisfn, ( void * ) im->data );
		}

		provider = NULL;
		if( id ){

			provider = na_io_provider_find_provider_by_id( merged, id );
			if( !provider ){

				provider = g_object_new( NA_IO_PROVIDER_TYPE, IO_PROVIDER_PROP_ID, id, NULL );
				merged = g_list_append( merged, provider );
			}

			io_provider_set_provider( pivot, provider, NA_IIO_PROVIDER( im->data ));

			g_free( id );
		}
	}

	na_pivot_free_providers( module_objects );

	return( merged );
}

static void
io_provider_set_provider( const NAPivot *pivot, NAIOProvider *provider, NAIIOProvider *instance )
{
	static const gchar *thisfn = "na_io_provider_set_provider";

	g_return_if_fail( NA_IS_IO_PROVIDER( provider ));
	g_return_if_fail( NA_IS_IIO_PROVIDER( instance ));

	provider->private->provider = g_object_ref( instance );

	if( NA_IIO_PROVIDER_GET_INTERFACE( instance )->get_name ){
		provider->private->name = NA_IIO_PROVIDER_GET_INTERFACE( instance )->get_name( instance );
	} else {
		g_warning( "%s: NAIIOProvider %p doesn't support get_name() interface", thisfn, ( void * ) instance );
	}

	provider->private->item_changed_handler =
			g_signal_connect(
					instance,
					NA_PIVOT_SIGNAL_ACTION_CHANGED,
					( GCallback ) na_pivot_item_changed_handler,
					( gpointer ) pivot );
}

static GList *
add_io_providers_from_prefs( const NAPivot *pivot, GList *runtime_providers )
{
	GConfClient *gconf;
	gchar *path, *id;
	GSList *ids, *iid;
	GList *providers;
	NAIOProvider *provider;

	providers = runtime_providers;
	path = gconf_concat_dir_and_key( NAUTILUS_ACTIONS_GCONF_BASEDIR, IO_PROVIDER_KEY_ROOT );
	gconf = na_iprefs_get_gconf_client( NA_IPREFS( pivot ));

	ids = na_gconf_utils_get_subdirs( gconf, path );

	for( iid = ids ; iid ; iid = iid->next ){
		id = na_utils_path_extract_last_dir(( const gchar * ) iid->data );
		provider = na_io_provider_find_provider_by_id( providers, id );

		if( !provider ){
			provider = g_object_new( NA_IO_PROVIDER_TYPE, IO_PROVIDER_PROP_ID, id, NULL );
			providers = g_list_append( providers, provider );
		}

		g_free( id );
	}

	na_gconf_utils_free_subdirs( ids );

	g_free( path );

	return( providers );
}

static void
update_io_providers( const NAPivot *pivot, GList *providers )
{
	GConfClient *gconf;
	gchar *path, *key, *entry;
	GList *ip;

	path = gconf_concat_dir_and_key( NAUTILUS_ACTIONS_GCONF_BASEDIR, IO_PROVIDER_KEY_ROOT );
	gconf = na_iprefs_get_gconf_client( NA_IPREFS( pivot ));

	for( ip = providers ; ip ; ip = ip->next ){

		key = gconf_concat_dir_and_key( path, ( const gchar * ) NA_IO_PROVIDER( ip->data )->private->id );

		entry = gconf_concat_dir_and_key( key, IO_PROVIDER_KEY_READABLE );
		NA_IO_PROVIDER( ip->data )->private->read_at_startup = na_gconf_utils_read_bool( gconf, entry, FALSE, TRUE );
		g_free( entry );

		entry = gconf_concat_dir_and_key( key, IO_PROVIDER_KEY_WRITABLE );
		NA_IO_PROVIDER( ip->data )->private->writable = na_gconf_utils_read_bool( gconf, entry, FALSE, TRUE );
		g_free( entry );

		g_free( key );
	}

	g_free( path );
}

static GList *
filter_io_providers( GList *providers, NAPivotIOProviderSet set )
{
	GList *to_remove, *ip;

	to_remove = NULL;
	for( ip = providers ; ip ; ip = ip->next ){

		if( set & PIVOT_IO_PROVIDER_READABLE_AT_STARTUP ){
			if( !NA_IO_PROVIDER( ip->data )->private->read_at_startup ){
				to_remove = g_list_prepend( to_remove, ip->data );
			}
		}

		if( set & PIVOT_IO_PROVIDER_WRITABLE ){
			if( !NA_IO_PROVIDER( ip->data )->private->writable ){
				to_remove = g_list_prepend( to_remove, ip->data );
			}
		}
	}

	for( ip = to_remove ; ip ; ip = ip->next ){
		providers = g_list_remove( providers, ip->data );
		g_object_unref( ip->data );
	}

	return( providers );
}

void
na_io_provider_dump_providers_list( GList *providers )
{
	static const gchar *thisfn = "na_io_provider_dump_providers_list";
	GList *ip;

	g_debug( "%s: providers=%p (count=%d)", thisfn, ( void * ) providers, g_list_length( providers ));

	for( ip = providers ; ip ; ip = ip->next ){
		dump( NA_IO_PROVIDER( ip->data ));
	}
}

static void
dump( const NAIOProvider *provider )
{
	static const gchar *thisfn = "na_io_provider_dump";

	g_debug( "%s:                   id=%s", thisfn, provider->private->id );
	g_debug( "%s:                 name=%s", thisfn, provider->private->name );
	g_debug( "%s:      read_at_startup=%s", thisfn, provider->private->read_at_startup ? "True":"False" );
	g_debug( "%s:             writable=%s", thisfn, provider->private->writable ? "True":"False" );
	g_debug( "%s:             provider=%p", thisfn, ( void * ) provider->private->provider );
	g_debug( "%s: item_changed_handler=%ld", thisfn, provider->private->item_changed_handler );
}

/**
 * na_io_provider_find_provider_by_id:
 * @providers: the current list of #NAIOProvider.
 * @id: the searched internal id.
 *
 * Returns: the searched #NAIOProvider, or %NULL if not found.
 *
 * The returned object is owned by #NAIOProvider class, and should not
 * be g_object_unref() by the user.
 */
NAIOProvider *
na_io_provider_find_provider_by_id( GList *providers, const gchar *id )
{
	NAIOProvider *provider;
	GList *ip;

	provider = NULL;

	for( ip = providers ; ip && !provider ; ip = ip->next ){
		if( !strcmp( NA_IO_PROVIDER( ip->data )->private->id, id )){
			provider = NA_IO_PROVIDER( ip->data );
		}
	}

	return( provider );
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

	providers = na_io_provider_get_providers_list( pivot );

	merged = get_merged_items_list( pivot, providers, messages );
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

		instance = NA_IO_PROVIDER( ip->data )->private->provider;
		if( instance ){

			if( NA_IIO_PROVIDER_GET_INTERFACE( instance )->read_items ){

				list = NA_IIO_PROVIDER_GET_INTERFACE( instance )->read_items( instance, messages );

				for( item = list ; item ; item = item->next ){

					na_object_set_provider( item->data, instance );
					na_object_dump( item->data );
				}

				merged = g_list_concat( merged, list );
			}
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
 * na_io_provider_get_id:
 * @provider: this #NAIOProvider.
 *
 * Returns: the internal id of this #NAIIOProvider, as a newly
 * allocated string which should be g_free() by the caller.
 */
gchar *
na_io_provider_get_id( const NAIOProvider *provider )
{
	gchar *id;

	id = NULL;
	g_return_val_if_fail( NA_IS_IO_PROVIDER( provider ), id );

	if( !provider->private->dispose_has_run ){

		id = g_strdup( provider->private->id );
	}

	return( id );
}

/**
 * na_io_provider_get_name:
 * @provider: this #NAIOProvider.
 *
 * Returns: the displayable name of this #NAIIOProvider, as a newly
 * allocated string which should be g_free() by the caller.
 *
 * May return %NULL is the NAIIOProvider is not present at runtime.
 */
gchar *
na_io_provider_get_name( const NAIOProvider *provider )
{
	gchar *name;

	name = NULL;
	g_return_val_if_fail( NA_IS_IO_PROVIDER( provider ), name );

	if( !provider->private->dispose_has_run ){

		if( provider->private->name ){
			name = g_strdup( provider->private->name );
		}
	}

	return( name );
}

/**
 * na_io_provider_is_readable_at_startup:
 * @provider: this #NAIOProvider.
 *
 * Returns: %TRUE is this I/O provider should be read at startup, and so
 * may participate to the global list of menus and actions.
 *
 * This is a user preference.
 * If the preference is not recorded, then it defaults to %TRUE.
 * This means that the user may adjust its personal configuration to
 * fully ignore menu/action items from a NAIIOProvider, just by setting
 * the corresponding flag to %FALSE.
 */
gboolean
na_io_provider_is_readable_at_startup( const NAIOProvider *provider )
{
	gboolean to_be_read;

	to_be_read = FALSE;
	g_return_val_if_fail( NA_IS_IO_PROVIDER( provider ), to_be_read );

	if( !provider->private->dispose_has_run ){

		to_be_read = provider->private->read_at_startup;
	}

	return( to_be_read );
}

/**
 * na_io_provider_is_writable:
 * @provider: this #NAIOProvider.
 *
 * Returns: %TRUE is this I/O provider is writable.
 *
 * This is a user preference, and doesn't suppose that the NAIIOProvider
 * will actually be writable or not.
 */
gboolean
na_io_provider_is_writable( const NAIOProvider *provider )
{
	gboolean writable;

	writable = FALSE;
	g_return_val_if_fail( NA_IS_IO_PROVIDER( provider ), writable );

	if( !provider->private->dispose_has_run ){

		writable = provider->private->writable;
	}

	return( writable );
}

/**
 * na_io_provider_get_provider:
 * @provider: the #NAIOProvider object.
 *
 * Returns: the I/O interface instance, or NULL.
 *
 * The returned #NAIIOProvider instance is owned by the #NAIOProvider
 * object, and should not be g_object_unref() by the caller.
 */
NAIIOProvider *
na_io_provider_get_provider( const NAIOProvider *provider )
{
	NAIIOProvider *instance;

	instance = NULL;
	g_return_val_if_fail( NA_IS_IO_PROVIDER( provider ), instance );

	if( !provider->private->dispose_has_run ){

		instance = provider->private->provider;
	}

	return( instance );
}

/**
 * na_io_provider_set_readable_at_startup:
 * @provider: the #NAIOProvider object.
 * @readable: whether this I/O provider may be read at startup.
 *
 * Set the 'read_a_startup' property.
 */
void
na_io_provider_set_readable_at_startup( NAIOProvider *provider, gboolean readable )
{
	g_return_if_fail( NA_IS_IO_PROVIDER( provider ));

	if( !provider->private->dispose_has_run ){

		provider->private->read_at_startup = readable;
	}
}

/**
 * na_io_provider_set_writable:
 * @provider: the #NAIOProvider object.
 * @writable: whether this I/O provider may be written.
 *
 * Set the 'writable' property.
 *
 * Note that this is only a user preference, and would not make writable
 * a NAIIOProvider which would either be read-only.
 */
void
na_io_provider_set_writable( NAIOProvider *provider, gboolean writable )
{
	g_return_if_fail( NA_IS_IO_PROVIDER( provider ));

	if( !provider->private->dispose_has_run ){

		provider->private->writable = writable;
	}
}

/**
 * na_io_provider_get_provider_name:
 * @provider: the #NAIIOProvider whose name is to be returned.
 *
 * Returns: a displayble name for the provider, as a newly allocated
 * string which should be g_free() by the caller.
 */
gchar *
na_io_provider_get_provider_name( const NAIIOProvider *provider )
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
#if 0
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
#endif

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
	/*GConfClient *gconf;
	gchar *id;
	gchar *key;*/

	writable = FALSE;
	locked = FALSE;

	if( NA_IIO_PROVIDER_GET_INTERFACE( provider )->is_willing_to_write ){

		writable = NA_IIO_PROVIDER_GET_INTERFACE( provider )->is_willing_to_write( provider );

#if 0
		if( writable ){
			id = na_io_provider_get_id( pivot, provider );
			key = g_strdup_printf( "%s/mandatory/%s/locked", NAUTILUS_ACTIONS_GCONF_BASEDIR, id );
			gconf = na_iprefs_get_gconf_client( NA_IPREFS( pivot ));
			locked = na_gconf_utils_read_bool( gconf, key, TRUE, locked );
			/*g_debug( "%s: id=%s, locked=%s", thisfn, id, locked ? "True":"False" );*/
			g_free( key );
			g_free( id );
		}
#endif
	}

	return( writable && !locked );
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
 * doesn't have any attached provider. We so do nothing and return OK...
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

	} else {
		ret = NA_IIO_PROVIDER_WRITE_OK;
	}

	return( ret );
}
