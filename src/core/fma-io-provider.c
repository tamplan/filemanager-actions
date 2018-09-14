/*
 * FileManager-Actions
 * A file-manager extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
 *
 * FileManager-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * FileManager-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FileManager-Actions; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
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

#include <glib/gi18n.h>
#include <string.h>

#include <api/fma-iio-provider.h>
#include <api/fma-object-api.h>
#include <api/fma-core-utils.h>

#include "fma-iprefs.h"
#include "fma-io-provider.h"

/* private class data
 */
struct _FMAIOProviderClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _FMAIOProviderPrivate {
	gboolean        dispose_has_run;
	gchar          *id;
	FMAIIOProvider *provider;
	gulong          item_changed_handler;
	gboolean        writable;
	guint           reason;
};

/* FMAIOProvider properties
 */
enum {
	IO_PROVIDER_PROP_ID_ID = 1,
};

#define IO_PROVIDER_PROP_ID				"fma-io-provider-prop-id"

static const gchar   *st_enter_bug    = N_( "Please, be kind enough to fill out a bug report on "
											"https://gitlab.gnome.org/GNOME/filemanager-actions/issues." );

static GObjectClass  *st_parent_class = NULL;
static GList         *st_io_providers = NULL;

static GType          register_type( void );
static void           class_init( FMAIOProviderClass *klass );
static void           instance_init( GTypeInstance *instance, gpointer klass );
static void           instance_constructed( GObject *object );
static void           instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void           instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void           instance_dispose( GObject *object );
static void           instance_finalize( GObject *object );

#if 0
static void           dump( const FMAIOProvider *provider );
static void           dump_providers_list( GList *providers );
#endif
static FMAIOProvider *io_provider_new( const FMAPivot *pivot, FMAIIOProvider *module, const gchar *id );
static GList         *io_providers_list_add_from_plugins( const FMAPivot *pivot, GList *list );
static GList         *io_providers_list_add_from_prefs( const FMAPivot *pivot, GList *objects_list );
static GSList        *io_providers_get_from_prefs( void );
static GList         *io_providers_list_add_from_write_order( const FMAPivot *pivot, GList *objects_list );
static GList         *io_providers_list_append_object( const FMAPivot *pivot, GList *list, FMAIIOProvider *module, const gchar *id );
static void           io_providers_list_set_module( const FMAPivot *pivot, FMAIOProvider *provider_object, FMAIIOProvider *provider_module );
static gboolean       is_conf_writable( const FMAIOProvider *provider, const FMAPivot *pivot, gboolean *mandatory );
static gboolean       is_finally_writable( const FMAIOProvider *provider, const FMAPivot *pivot, guint *reason );
static GList         *load_items_filter_unwanted_items( const FMAPivot *pivot, GList *merged, guint loadable_set );
static GList         *load_items_filter_unwanted_items_rec( GList *merged, guint loadable_set );
static GList         *load_items_get_merged_list( const FMAPivot *pivot, guint loadable_set, GSList **messages );
static GList         *load_items_hierarchy_build( GList **tree, GSList *level_zero, gboolean list_if_empty, FMAObjectItem *parent );
static GList         *load_items_hierarchy_sort( const FMAPivot *pivot, GList *tree, GCompareFunc fn );
static gint           peek_item_by_id_compare( const FMAObject *obj, const gchar *id );
static FMAIOProvider *peek_provider_by_id( const GList *providers, const gchar *id );

GType
fma_io_provider_get_type( void )
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
	static const gchar *thisfn = "fma_io_provider_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( FMAIOProviderClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( FMAIOProvider ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "FMAIOProvider", &info, 0 );

	return( type );
}

static void
class_init( FMAIOProviderClass *klass )
{
	static const gchar *thisfn = "fma_io_provider_class_init";
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
			"Internal identifier of the I/O provider (e.g. 'fma-gconf')", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, IO_PROVIDER_PROP_ID_ID, spec );

	klass->private = g_new0( FMAIOProviderClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "fma_io_provider_instance_init";
	FMAIOProvider *self;

	g_return_if_fail( FMA_IS_IO_PROVIDER( instance ));

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self = FMA_IO_PROVIDER( instance );

	self->private = g_new0( FMAIOProviderPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->id = NULL;
	self->private->provider = NULL;
	self->private->item_changed_handler = 0;
	self->private->writable = FALSE;
	self->private->reason = IIO_PROVIDER_STATUS_UNAVAILABLE;
}

static void
instance_constructed( GObject *object )
{
	static const gchar *thisfn = "fma_io_provider_instance_constructed";
	FMAIOProviderPrivate *priv;

	g_return_if_fail( FMA_IS_IO_PROVIDER( object ));

	priv = FMA_IO_PROVIDER( object )->private;

	if( !priv->dispose_has_run ){

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->constructed ){
			G_OBJECT_CLASS( st_parent_class )->constructed( object );
		}

		g_debug( "%s: object=%p (%s), id=%s",
				thisfn,
				( void * ) object, G_OBJECT_TYPE_NAME( object ),
				priv->id );
	}
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	FMAIOProvider *self;

	g_return_if_fail( FMA_IS_IO_PROVIDER( object ));
	self = FMA_IO_PROVIDER( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case IO_PROVIDER_PROP_ID_ID:
				g_value_set_string( value, self->private->id );
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
	FMAIOProvider *self;

	g_return_if_fail( FMA_IS_IO_PROVIDER( object ));
	self = FMA_IO_PROVIDER( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case IO_PROVIDER_PROP_ID_ID:
				g_free( self->private->id );
				self->private->id = g_value_dup_string( value );
				break;
		}
	}
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "fma_io_provider_instance_dispose";
	FMAIOProvider *self;

	g_return_if_fail( FMA_IS_IO_PROVIDER( object ));

	self = FMA_IO_PROVIDER( object );

	if( !self->private->dispose_has_run ){

		g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

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
	static const gchar *thisfn = "fma_io_provider_instance_finalize";
	FMAIOProvider *self;

	g_return_if_fail( FMA_IS_IO_PROVIDER( object ));

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	self = FMA_IO_PROVIDER( object );

	g_free( self->private->id );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/*
 * fma_io_provider_find_writable_io_provider:
 * @pivot: the #FMAPivot instance.
 *
 * Returns: the first willing and able to write I/O provider, or NULL.
 *
 * The returned provider is owned by FMAIOProvider class, and should not
 * be released by the caller.
 */
FMAIOProvider *
fma_io_provider_find_writable_io_provider( const FMAPivot *pivot )
{
	const GList *providers;
	const GList *ip;
	FMAIOProvider *provider;

	providers = fma_io_provider_get_io_providers_list( pivot );

	for( ip = providers ; ip ; ip = ip->next ){
		provider = ( FMAIOProvider * ) ip->data;
		if( provider->private->writable ){
			return( provider );
		}
	}

	return( NULL );
}

/*
 * fma_io_provider_find_io_provider_by_id:
 * @pivot: the #FMAPivot instance.
 * @id: the identifier of the searched I/O provider.
 *
 * Returns: the I/O provider, or NULL.
 *
 * The returned provider is owned by FMAIOProvider class, and should not
 * be released by the caller.
 */
FMAIOProvider *
fma_io_provider_find_io_provider_by_id( const FMAPivot *pivot, const gchar *id )
{
	const GList *providers;
	const GList *ip;
	FMAIOProvider *provider;
	FMAIOProvider *found;

	providers = fma_io_provider_get_io_providers_list( pivot );
	found = NULL;

	for( ip = providers ; ip && !found ; ip = ip->next ){
		provider = FMA_IO_PROVIDER( ip->data );
		if( !strcmp( provider->private->id, id )){
			found = provider;
		}
	}

	return( found );
}

/*
 * fma_io_provider_get_io_providers_list:
 * @pivot: the current #FMAPivot instance.
 *
 * Build (if not already done) the write-ordered list of currently
 * available FMAIOProvider objects.
 *
 * A FMAIOProvider object may be created:
 * - either because we have loaded a plugin which claims to implement the
 *   FMAIIOProvider interface;
 * - or because an i/o provider identifier has been found in preferences.
 *
 * The objects in this list must be in write order:
 * - when loading items, items are ordered depending of menus items list
 *   and of level zero defined order;
 * - when writing a new item, there is a 'writable-order' preference.
 *
 * Returns: the list of I/O providers.
 *
 * The returned list is owned by #FMAIOProvider class, and should not be
 * released by the caller.
 */
const GList *
fma_io_provider_get_io_providers_list( const FMAPivot *pivot )
{
	g_return_val_if_fail( FMA_IS_PIVOT( pivot ), NULL );

	if( !st_io_providers ){
		st_io_providers = io_providers_list_add_from_write_order( pivot, NULL );
		st_io_providers = io_providers_list_add_from_plugins( pivot, st_io_providers );
		st_io_providers = io_providers_list_add_from_prefs( pivot, st_io_providers );
	}

	return( st_io_providers );
}

/*
 * adding from write-order means we only create FMAIOProvider objects
 * without having any pointer to the underlying FMAIIOProvider (if it exists)
 */
static GList *
io_providers_list_add_from_write_order( const FMAPivot *pivot, GList *objects_list )
{
	GList *merged;
	GSList *io_providers, *it;
	const gchar *id;

	merged = objects_list;
	io_providers = fma_settings_get_string_list( IPREFS_IO_PROVIDERS_WRITE_ORDER, NULL, NULL );

	for( it = io_providers ; it ; it = it->next ){
		id = ( const gchar * ) it->data;
		merged = io_providers_list_append_object( pivot, merged, NULL, id );
	}

	fma_core_utils_slist_free( io_providers );

	return( merged );
}

/*
 * add to the list a FMAIOProvider object for each loaded plugin which claim
 * to implement the FMAIIOProvider interface
 */
static GList *
io_providers_list_add_from_plugins( const FMAPivot *pivot, GList *objects_list )
{
	static const gchar *thisfn = "fma_io_provider_io_providers_list_add_from_plugins";
	GList *merged;
	GList *modules_list, *im;
	gchar *id;
	FMAIIOProvider *provider_module;

	merged = objects_list;
	modules_list = fma_pivot_get_providers( pivot, FMA_TYPE_IIO_PROVIDER );

	for( im = modules_list ; im ; im = im->next ){

		id = NULL;
		provider_module = FMA_IIO_PROVIDER( im->data );

		if( FMA_IIO_PROVIDER_GET_INTERFACE( provider_module )->get_id ){
			id = FMA_IIO_PROVIDER_GET_INTERFACE( provider_module )->get_id( provider_module );
			if( !id || !strlen( id )){
				g_warning( "%s: FMAIIOProvider %p get_id() interface returns null or empty id", thisfn, ( void * ) im->data );
				g_free( id );
				id = NULL;
			}
		} else {
			g_warning( "%s: FMAIIOProvider %p doesn't support get_id() interface", thisfn, ( void * ) im->data );
		}

		if( id ){
			merged = io_providers_list_append_object( pivot, merged, provider_module, id );
			g_free( id );
		}
	}

	fma_pivot_free_providers( modules_list );

	return( merged );
}

/*
 * add to the list FMAIOProvider objects for the identifiers we may find
 * in preferences without having found the plugin itself
 *
 * preferences come from the io-providers status.
 */
static GList *
io_providers_list_add_from_prefs( const FMAPivot *pivot, GList *objects_list )
{
	GList *merged;
	const gchar *id;
	GSList *io_providers, *it;

	merged = objects_list;
	io_providers = io_providers_get_from_prefs();

	for( it = io_providers ; it ; it = it->next ){
		id = ( const gchar * ) it->data;
		merged = io_providers_list_append_object( pivot, merged, NULL, id );
	}

	fma_core_utils_slist_free( io_providers );

	return( merged );
}

/*
 * io_providers_get_from_prefs:
 *
 * Searches in preferences system for all mentions of an i/o provider.
 * This does not mean in any way that the i/o provider is active,
 * available or so, but just that is mentioned here.
 *
 * I/o provider identifiers returned in the list are not supposed
 * to be unique, nor sorted.
 *
 * Returns: a list of i/o provider identifiers found in preferences
 * system; this list should be fma_core_utils_slist_free() by the caller.
 *
 * Since: 3.1
 */
static GSList *
io_providers_get_from_prefs( void )
{
	GSList *providers;
	GSList *groups;
	GSList *it;
	const gchar *name;
	gchar *group_prefix;
	guint prefix_len;

	providers = NULL;
	groups = fma_settings_get_groups();
	group_prefix = g_strdup_printf( "%s ", IPREFS_IO_PROVIDER_GROUP );
	prefix_len = strlen( group_prefix );

	for( it = groups ; it ; it = it->next ){
		name = ( const gchar * ) it->data;
		if( g_str_has_prefix( name, group_prefix )){
			providers = g_slist_prepend( providers, g_strdup( name+prefix_len ));
		}
	}

	g_free( group_prefix );
	fma_core_utils_slist_free( groups );

	return( providers );
}

/*
 * add to the list a FMAIOProvider object for the specified module and id
 * if it does not have been already registered
 */
static GList *
io_providers_list_append_object( const FMAPivot *pivot, GList *list, FMAIIOProvider *module, const gchar *id )
{
	GList *merged;
	FMAIOProvider *object;

	merged = list;
	object = peek_provider_by_id( list, id );

	if( !object ){
		object = io_provider_new( pivot, module, id );
		merged = g_list_append( merged, object );

	} else if( module && !object->private->provider ){
		io_providers_list_set_module( pivot, object, module );
	}

	return( merged );
}

static FMAIOProvider *
peek_provider_by_id( const GList *providers, const gchar *id )
{
	FMAIOProvider *provider = NULL;
	const GList *ip;

	for( ip = providers ; ip && !provider ; ip = ip->next ){
		if( !strcmp( FMA_IO_PROVIDER( ip->data )->private->id, id )){
			provider = FMA_IO_PROVIDER( ip->data );
		}
	}

	return( provider );
}

/*
 * allocate a new FMAIOProvider object for the specified module and id
 *
 * id is mandatory here
 * module may be NULL
 */
static FMAIOProvider *
io_provider_new( const FMAPivot *pivot, FMAIIOProvider *module, const gchar *id )
{
	FMAIOProvider *object;

	g_return_val_if_fail( id && strlen( id ), NULL );

	object = g_object_new( FMA_IO_PROVIDER_TYPE, IO_PROVIDER_PROP_ID, id, NULL );

	if( module ){
		io_providers_list_set_module( pivot, object, module );
	}

	return( object );
}

/*
 * when a IIOProvider plugin is associated with the FMAIOProvider object,
 * we connect the FMAPivot callback to the 'item-changed' signal
 */
static void
io_providers_list_set_module( const FMAPivot *pivot, FMAIOProvider *provider_object, FMAIIOProvider *provider_module )
{
	provider_object->private->provider = g_object_ref( provider_module );

	provider_object->private->item_changed_handler =
			g_signal_connect(
					provider_module, IO_PROVIDER_SIGNAL_ITEM_CHANGED,
					( GCallback ) fma_pivot_on_item_changed_handler, ( gpointer ) pivot );

	provider_object->private->writable =
			is_finally_writable( provider_object, pivot, &provider_object->private->reason );

	g_debug( "fma_io_provider_list_set_module: provider_module=%p (%s), writable=%s, reason=%d",
			( void * ) provider_module,
			provider_object->private->id,
			provider_object->private->writable ? "True":"False",
			provider_object->private->reason );
}

/*
 * fma_io_provider_unref_io_providers_list:
 *
 * Called by on #FMAPivot dispose(), free here resources allocated to
 * the I/O providers.
 */
void
fma_io_provider_unref_io_providers_list( void )
{
	g_list_foreach( st_io_providers, ( GFunc ) g_object_unref, NULL );
	g_list_free( st_io_providers );
	st_io_providers = NULL;
}

/*
 * fma_io_provider_get_id:
 * @provider: this #FMAIOProvider.
 *
 * Returns: the internal id of this #FMAIIOProvider, as a newly
 * allocated string which should be g_free() by the caller.
 */
gchar *
fma_io_provider_get_id( const FMAIOProvider *provider )
{
	gchar *id;

	g_return_val_if_fail( FMA_IS_IO_PROVIDER( provider ), NULL );

	id = NULL;

	if( !provider->private->dispose_has_run ){

		id = g_strdup( provider->private->id );
	}

	return( id );
}

/*
 * fma_io_provider_get_name:
 * @provider: this #FMAIOProvider.
 *
 * Returns: the displayable name of this #FMAIIOProvider, as a newly
 * allocated string which should be g_free() by the caller.
 *
 * This function makes sure to never return %NULL. An empty string
 * may be returned if the FMAIIOProvider is not present at runtime,
 * or does not implement the needed interface, or returns itself %NULL
 * or an empty string.
 */
gchar *
fma_io_provider_get_name( const FMAIOProvider *provider )
{
	static const gchar *thisfn = "fma_io_provider_get_name";
	gchar *name;

	name = g_strdup( "" );

	g_return_val_if_fail( FMA_IS_IO_PROVIDER( provider ), name );

	if( !provider->private->dispose_has_run ){
		if( fma_io_provider_is_available( provider ) &&
			FMA_IIO_PROVIDER_GET_INTERFACE( provider->private->provider )->get_name ){

				g_free( name );
				name = NULL;
				name = FMA_IIO_PROVIDER_GET_INTERFACE( provider->private->provider )->get_name( provider->private->provider );
				if( !name ){
					g_message( "%s: FMAIIOProvider %s get_name() interface returns NULL", thisfn, provider->private->id );
					name = g_strdup( "" );
			}

		} else {
			g_message( "%s: FMAIIOProvider %s is not available or doesn't support get_name() interface", thisfn, provider->private->id );
		}
	}

	return( name );
}

/*
 * fma_io_provider_is_available:
 * @provider: the #FMAIOProvider object.
 *
 * Returns: %TRUE if the corresponding #FMAIIOProvider module is available
 * at runtime, %FALSE else.
 */
gboolean
fma_io_provider_is_available( const FMAIOProvider *provider )
{
	gboolean is_available;

	g_return_val_if_fail( FMA_IS_IO_PROVIDER( provider ), FALSE );

	is_available = FALSE;

	if( !provider->private->dispose_has_run ){

		is_available = ( provider->private->provider && FMA_IS_IIO_PROVIDER( provider->private->provider ));
	}

	return( is_available );
}

/*
 * fma_io_provider_is_conf_readable:
 * @provider: this #FMAIOProvider.
 * @pivot: the #FMAPivot application object.
 * @mandatory: a pointer to a boolean which will be set to %TRUE if the
 *  preference is mandatory; may be %NULL.
 *
 * Returns: %TRUE is this I/O provider should be read at startup, and so
 * may participate to the global list of menus and actions, %FALSE else.
 *
 * This is a configuration property, which defaults to %TRUE.
 *
 * Whether it is editable by the user or not depends on:
 * - whether the whole configuration has been locked down by an admin;
 * - whether this flag has been set as mandatory by an admin.
 */
gboolean
fma_io_provider_is_conf_readable( const FMAIOProvider *provider, const FMAPivot *pivot, gboolean *mandatory )
{
	gboolean readable;
	gchar *group;

	g_return_val_if_fail( FMA_IS_IO_PROVIDER( provider ), FALSE );
	g_return_val_if_fail( FMA_IS_PIVOT( pivot ), FALSE );

	readable = FALSE;

	if( !provider->private->dispose_has_run ){

		group = g_strdup_printf( "%s %s", IPREFS_IO_PROVIDER_GROUP, provider->private->id );
		readable = fma_settings_get_boolean_ex( group, IPREFS_IO_PROVIDER_READABLE, NULL, mandatory );
		g_free( group );
	}

	return( readable );
}

/*
 * fma_io_provider_is_conf_writable:
 * @provider: this #FMAIOProvider.
 * @pivot: the #FMAPivot application object.
 * @mandatory: a pointer to a boolean which will be set to %TRUE if the
 *  preference is mandatory; may be %NULL.
 *
 * Returns: %TRUE is this I/O provider is candidate to be edited.
 *
 * This is a configuration property, which defaults to %TRUE.
 *
 * Whether it is editable by the user or not depends on:
 * - whether the whole configuration has been locked down by an admin;
 * - whether this flag has been set as mandatory by an admin.
 *
 * This property does not say that an item can actually be written by this
 * FMAIIOProvider module. See also is_willing_to() and is_able_to().
 */
gboolean
fma_io_provider_is_conf_writable( const FMAIOProvider *provider, const FMAPivot *pivot, gboolean *mandatory )
{
	gboolean is_writable;

	g_return_val_if_fail( FMA_IS_IO_PROVIDER( provider ), FALSE );
	g_return_val_if_fail( FMA_IS_PIVOT( pivot ), FALSE );

	is_writable = FALSE;

	if( !provider->private->dispose_has_run ){

		is_writable = is_conf_writable( provider, pivot, mandatory );
	}

	return( is_writable );
}

/**
 * fma_io_provider_is_finally_writable:
 * @provider: this #FMAIOProvider.
 * @reason: if not %NULL, a pointer to a guint which will hold the reason.
 *
 * Returns: the current writability status of this I/O provider.
 */
gboolean
fma_io_provider_is_finally_writable( const FMAIOProvider *provider, guint *reason )
{
	gboolean is_writable;

	if( reason ){
		*reason = IIO_PROVIDER_STATUS_UNDETERMINED;
	}
	g_return_val_if_fail( FMA_IS_IO_PROVIDER( provider ), FALSE );

	is_writable = FALSE;

	if( !provider->private->dispose_has_run ){

		is_writable = provider->private->writable;
		if( reason ){
			*reason = provider->private->reason;
		}
	}

	return( is_writable );
}

/*
 * fma_io_provider_load_items:
 * @pivot: the #FMAPivot object which owns the list of registered I/O
 *  storage providers.
 * @loadable_set: the set of loadable items
 *  (cf. FMAPivotLoadableSet enumeration defined in core/fma-pivot.h).
 * @messages: error messages.
 *
 * Loads the tree from I/O storage subsystems.
 *
 * Returns: a #GList of newly allocated objects as a hierarchical tree
 * in display order. This tree contains #FMAObjectMenu menus, along with
 * #FMAObjectAction actions and their #FMAObjectProfile profiles.
 *
 * The returned list should be fma_object_free_items().
 */
GList *
fma_io_provider_load_items( const FMAPivot *pivot, guint loadable_set, GSList **messages )
{
	static const gchar *thisfn = "fma_io_provider_load_items";
	GList *flat, *hierarchy, *filtered;
	GSList *level_zero;
	guint order_mode;

	g_return_val_if_fail( FMA_IS_PIVOT( pivot ), NULL );

	g_debug( "%s: pivot=%p, loadable_set=%d, messages=%p",
			thisfn, ( void * ) pivot, loadable_set, ( void * ) messages );

	/* get the global flat items list, as a merge of the list provided
	 * by each available and readable i/o provider
	 */
	flat = load_items_get_merged_list( pivot, loadable_set, messages );

	/* build the items hierarchy
	 */
	level_zero = fma_settings_get_string_list( IPREFS_ITEMS_LEVEL_ZERO_ORDER, NULL, NULL );

	hierarchy = load_items_hierarchy_build( &flat, level_zero, TRUE, NULL );

	/* items that stay left in the global flat list are simply appended
	 * to the built hierarchy, and level zero is updated accordingly
	 */
	if( flat ){
		g_debug( "%s: %d items left appended to the hierarchy", thisfn, g_list_length( flat ));
		hierarchy = g_list_concat( hierarchy, flat );
	}

	if( flat || !level_zero || !g_slist_length( level_zero )){
		g_debug( "%s: rewriting level-zero", thisfn );
		if( !fma_iprefs_write_level_zero( hierarchy, messages )){
			g_warning( "%s: unable to update level-zero", thisfn );
		}
	}

	fma_core_utils_slist_free( level_zero );

	/* sort the hierarchy according to preferences
	 */
	order_mode = fma_iprefs_get_order_mode( NULL );
	switch( order_mode ){
		case IPREFS_ORDER_ALPHA_ASCENDING:
			hierarchy = load_items_hierarchy_sort( pivot, hierarchy, ( GCompareFunc ) fma_object_id_sort_alpha_asc );
			break;

		case IPREFS_ORDER_ALPHA_DESCENDING:
			hierarchy = load_items_hierarchy_sort( pivot, hierarchy, ( GCompareFunc ) fma_object_id_sort_alpha_desc );
			break;

		case IPREFS_ORDER_MANUAL:
		default:
			break;
	}

	/* check status here...
	 */
	filtered = load_items_filter_unwanted_items( pivot, hierarchy, loadable_set );
	g_list_free( hierarchy );

	g_debug( "%s: tree after filtering and reordering (if any)", thisfn );
	fma_object_dump_tree( filtered );
	g_debug( "%s: end of tree", thisfn );

	return( filtered );
}

#if 0
static void
dump( const FMAIOProvider *provider )
{
	static const gchar *thisfn = "fma_io_provider_dump";

	g_debug( "%s:                   id=%s", thisfn, provider->private->id );
	g_debug( "%s:             provider=%p", thisfn, ( void * ) provider->private->provider );
	g_debug( "%s: item_changed_handler=%lu", thisfn, provider->private->item_changed_handler );
}

static void
dump_providers_list( GList *providers )
{
	static const gchar *thisfn = "fma_io_provider_dump_providers_list";
	GList *ip;

	g_debug( "%s: providers=%p (count=%d)", thisfn, ( void * ) providers, g_list_length( providers ));

	for( ip = providers ; ip ; ip = ip->next ){
		dump( FMA_IO_PROVIDER( ip->data ));
	}
}
#endif

static gboolean
is_conf_writable( const FMAIOProvider *provider, const FMAPivot *pivot, gboolean *mandatory )
{
	gchar *group;
	gboolean is_writable;

	group = g_strdup_printf( "%s %s", IPREFS_IO_PROVIDER_GROUP, provider->private->id );
	is_writable = fma_settings_get_boolean_ex( group, IPREFS_IO_PROVIDER_WRITABLE, NULL, mandatory );
	g_free( group );

	return( is_writable );
}

/*
 * Evaluate the writability status for this I/O provider at load time
 * This same status may later be reevaluated on demand.
 */
static gboolean
is_finally_writable( const FMAIOProvider *provider, const FMAPivot *pivot, guint *reason )
{
	static const gchar *thisfn = "fma_io_provider_is_finally_writable";
	gboolean writable;
	gboolean is_writable, mandatory;

	g_return_val_if_fail( reason, FALSE );

	writable = FALSE;
	*reason = IIO_PROVIDER_STATUS_UNAVAILABLE;

	if( provider->private->provider && FMA_IS_IIO_PROVIDER( provider->private->provider )){

		writable = TRUE;
		*reason = IIO_PROVIDER_STATUS_WRITABLE;

		if( !FMA_IIO_PROVIDER_GET_INTERFACE( provider->private->provider )->is_willing_to_write ||
			!FMA_IIO_PROVIDER_GET_INTERFACE( provider->private->provider )->is_able_to_write ||
			!FMA_IIO_PROVIDER_GET_INTERFACE( provider->private->provider )->write_item ||
			!FMA_IIO_PROVIDER_GET_INTERFACE( provider->private->provider )->delete_item ){

				writable = FALSE;
				*reason = IIO_PROVIDER_STATUS_INCOMPLETE_API;
				g_debug( "%s: provider_module=%p (%s), writable=False, reason=IIO_PROVIDER_STATUS_INCOMPLETE_API",
						thisfn, ( void * ) provider->private->provider, provider->private->id );

		} else if( !FMA_IIO_PROVIDER_GET_INTERFACE( provider->private->provider )->is_willing_to_write( provider->private->provider )){

				writable = FALSE;
				*reason = IIO_PROVIDER_STATUS_NOT_WILLING_TO;
				g_debug( "%s: provider_module=%p (%s), writable=False, reason=IIO_PROVIDER_STATUS_NOT_WILLING_TO",
						thisfn, ( void * ) provider->private->provider, provider->private->id );

		} else if( !FMA_IIO_PROVIDER_GET_INTERFACE( provider->private->provider )->is_able_to_write( provider->private->provider )){

				writable = FALSE;
				*reason = IIO_PROVIDER_STATUS_NOT_ABLE_TO;
				g_debug( "%s: provider_module=%p (%s), writable=False, reason=IIO_PROVIDER_STATUS_NOT_ABLE_TO",
						thisfn, ( void * ) provider->private->provider, provider->private->id );

		} else {
			is_writable = is_conf_writable( provider, pivot, &mandatory );
			if( !is_writable ){
				writable = FALSE;
				if( mandatory ){
					*reason = IIO_PROVIDER_STATUS_LOCKED_BY_ADMIN;
				} else {
					*reason = IIO_PROVIDER_STATUS_LOCKED_BY_USER;
				}
				g_debug( "%s: provider_module=%p (%s), writable=False, reason=IIO_PROVIDER_STATUS_LOCKED_BY_someone, mandatory=%s",
						thisfn, ( void * ) provider->private->provider, provider->private->id,
						mandatory ? "True":"False" );
			}
		}
	}

	return( writable );
}

static GList *
load_items_filter_unwanted_items( const FMAPivot *pivot, GList *hierarchy, guint loadable_set )
{
	GList *it;
	GList *filtered;

	for( it = hierarchy ; it ; it = it->next ){
		fma_object_check_status( it->data );
	}

	filtered = load_items_filter_unwanted_items_rec( hierarchy, loadable_set );

	return( filtered );
}

/*
 * build a dest tree from a source tree, removing filtered items
 * an item is filtered if it is invalid (and not loading invalid ones)
 * or disabled (and not loading disabled ones)
 */
static GList *
load_items_filter_unwanted_items_rec( GList *hierarchy, guint loadable_set )
{
	static const gchar *thisfn = "fma_io_provider_load_items_filter_unwanted_items_rec";
	GList *subitems, *subitems_f;
	GList *it, *itnext;
	GList *filtered;
	gboolean selected;
	gchar *label;
	gboolean load_invalid, load_disabled;
	gboolean is_valid, is_enabled;

	filtered = NULL;
	load_invalid = loadable_set & PIVOT_LOAD_INVALID;
	load_disabled = loadable_set & PIVOT_LOAD_DISABLED;

	for( it = hierarchy ; it ; it = itnext ){

		itnext = it->next;
		selected = FALSE;
		is_enabled = FALSE;
		is_valid = fma_object_is_valid( it->data );

		if( FMA_IS_OBJECT_PROFILE( it->data )){
			if( is_valid || load_invalid ){
				filtered = g_list_append( filtered, it->data );
				selected = TRUE;
			}
		}

		if( FMA_IS_OBJECT_ITEM( it->data )){
			is_enabled = fma_object_is_enabled( it->data );

			if(( is_enabled || load_disabled ) && ( is_valid || load_invalid )){
				subitems = fma_object_get_items( it->data );
				subitems_f = load_items_filter_unwanted_items_rec( subitems, loadable_set );
				fma_object_set_items( it->data, subitems_f );
				filtered = g_list_append( filtered, it->data );
				selected = TRUE;
			}
		}

		if( !selected ){
			label = fma_object_get_label( it->data );
			g_debug( "%s: filtering %p (%s) '%s': valid=%s, enabled=%s",
					thisfn, ( void * ) it->data, G_OBJECT_TYPE_NAME( it->data ), label,
					is_valid ? "true":"false", is_enabled ? "true":"false" );
			g_free( label );
			fma_object_unref( it->data );
		}
	}

	return( filtered );
}

/*
 * returns a concatened flat list of read actions / menus
 * we take care here of:
 * - i/o providers which appear unavailable at runtime
 * - i/o providers marked as unreadable
 * - items (actions or menus) which do not satisfy the defined loadable set
 */
static GList *
load_items_get_merged_list( const FMAPivot *pivot, guint loadable_set, GSList **messages )
{
	const GList *providers;
	const GList *ip;
	GList *merged, *items, *it;
	const FMAIOProvider *provider_object;
	const FMAIIOProvider *provider_module;

	merged = NULL;
	providers = fma_io_provider_get_io_providers_list( pivot );

	for( ip = providers ; ip ; ip = ip->next ){
		provider_object = FMA_IO_PROVIDER( ip->data );
		provider_module = provider_object->private->provider;

		if( provider_module &&
			FMA_IIO_PROVIDER_GET_INTERFACE( provider_module )->read_items &&
			fma_io_provider_is_conf_readable( provider_object, pivot, NULL )){

			items = FMA_IIO_PROVIDER_GET_INTERFACE( provider_module )->read_items( provider_module, messages );

			for( it = items ; it ; it = it->next ){
				fma_object_set_provider( it->data, provider_object );
				fma_object_dump( it->data );
			}

			merged = g_list_concat( merged, items );
		}
	}

	return( merged );
}

/*
 * builds the hierarchy
 *
 * this is a recursive function which _moves_ items from input 'tree' to
 * output list.
 */
static GList *
load_items_hierarchy_build( GList **tree, GSList *level_zero, gboolean list_if_empty, FMAObjectItem *parent )
{
	static const gchar *thisfn = "fma_io_provider_load_items_hierarchy_build";
	GList *hierarchy, *it;
	GSList *ilevel;
	GSList *subitems_ids;
	GList *subitems;

	hierarchy = NULL;

	if( g_slist_length( level_zero )){
		for( ilevel = level_zero ; ilevel ; ilevel = ilevel->next ){
			/*g_debug( "%s: id=%s", thisfn, ( gchar * ) ilevel->data );*/
			it = g_list_find_custom( *tree, ilevel->data, ( GCompareFunc ) peek_item_by_id_compare );
			if( it ){
				hierarchy = g_list_append( hierarchy, it->data );
				fma_object_set_parent( it->data, parent );

				g_debug( "%s: id=%s: %s (%p) appended to hierarchy %p",
						thisfn, ( gchar * ) ilevel->data, G_OBJECT_TYPE_NAME( it->data ), ( void * ) it->data, ( void * ) hierarchy );

				*tree = g_list_remove_link( *tree, it );

				if( FMA_IS_OBJECT_MENU( it->data )){
					subitems_ids = fma_object_get_items_slist( it->data );
					subitems = load_items_hierarchy_build( tree, subitems_ids, FALSE, FMA_OBJECT_ITEM( it->data ));
					fma_object_set_items( it->data, subitems );
					fma_core_utils_slist_free( subitems_ids );
				}
			}
		}
	}

	/* if level-zero list is empty,
	 * we consider that all items are at the same level
	 */
	else if( list_if_empty ){
		for( it = *tree ; it ; it = it->next ){
			hierarchy = g_list_append( hierarchy, it->data );
			fma_object_set_parent( it->data, parent );
		}
		g_list_free( *tree );
		*tree = NULL;
	}

	return( hierarchy );
}

static GList *
load_items_hierarchy_sort( const FMAPivot *pivot, GList *tree, GCompareFunc fn )
{
	GList *sorted;
	GList *items, *it;

	sorted = g_list_sort( tree, fn );

	/* recursively sort each level of the tree
	 */
	for( it = sorted ; it ; it = it->next ){
		if( FMA_IS_OBJECT_MENU( it->data )){
			items = fma_object_get_items( it->data );
			items = load_items_hierarchy_sort( pivot, items, fn );
			fma_object_set_items( it->data, items );
		}
	}

	return( sorted );
}

/*
 * returns zero when @obj has the required @id
 */
static gint
peek_item_by_id_compare( const FMAObject *obj, const gchar *id )
{
	gchar *obj_id;
	gint ret = 1;

	if( FMA_IS_OBJECT_ITEM( obj )){
		obj_id = fma_object_get_id( obj );
		ret = strcmp( obj_id, id );
		g_free( obj_id );
	}

	return( ret );
}

/*
 * fma_io_provider_write_item:
 * @provider: this #FMAIOProvider object.
 * @item: a #FMAObjectItem to be written to the storage subsystem.
 * @messages: error messages.
 *
 * Writes an @item to the storage subsystem.
 *
 * Returns: the FMAIIOProvider return code.
 *
 * #FMAPivot class, which should be the only caller of this function,
 * has already check that this item is writable, i.e. that all conditions
 * are met to be able to successfully write the item down to the
 * storage subsystem.
 */
guint
fma_io_provider_write_item( const FMAIOProvider *provider, const FMAObjectItem *item, GSList **messages )
{
	static const gchar *thisfn = "fma_io_provider_write_item";
	guint ret;

	g_debug( "%s: provider=%p (%s), item=%p (%s), messages=%p", thisfn,
			( void * ) provider, G_OBJECT_TYPE_NAME( provider ),
			( void * ) item, G_OBJECT_TYPE_NAME( item ),
			( void * ) messages );

	ret = IIO_PROVIDER_CODE_PROGRAM_ERROR;

	g_return_val_if_fail( FMA_IS_IO_PROVIDER( provider ), ret );
	g_return_val_if_fail( FMA_IS_OBJECT_ITEM( item ), ret );
	g_return_val_if_fail( FMA_IS_IIO_PROVIDER( provider->private->provider ), ret );
	g_return_val_if_fail( FMA_IIO_PROVIDER_GET_INTERFACE( provider->private->provider )->write_item, ret );

	ret = FMA_IIO_PROVIDER_GET_INTERFACE( provider->private->provider )->write_item( provider->private->provider, item, messages );

	if( ret == IIO_PROVIDER_CODE_OK ){
		fma_object_set_provider( item, provider );
	}

	return( ret );
}

/*
 * fma_io_provider_delete_item:
 * @provider: this #FMAIOProvider object.
 * @item: the #FMAObjectItem item to be deleted.
 * @messages: error messages.
 *
 * Deletes an item (action or menu) from the storage subsystem.
 *
 * Returns: the FMAIIOProvider return code.
 */
guint
fma_io_provider_delete_item( const FMAIOProvider *provider, const FMAObjectItem *item, GSList **messages )
{
	static const gchar *thisfn = "fma_io_provider_delete_item";
	guint ret;

	g_debug( "%s: provider=%p (%s), item=%p (%s), messages=%p", thisfn,
			( void * ) provider, G_OBJECT_TYPE_NAME( provider ),
			( void * ) item, G_OBJECT_TYPE_NAME( item ),
			( void * ) messages );

	ret = IIO_PROVIDER_CODE_PROGRAM_ERROR;

	g_return_val_if_fail( FMA_IS_IO_PROVIDER( provider ), ret );
	g_return_val_if_fail( FMA_IS_OBJECT_ITEM( item ), ret );
	g_return_val_if_fail( FMA_IS_IIO_PROVIDER( provider->private->provider ), ret );
	g_return_val_if_fail( FMA_IIO_PROVIDER_GET_INTERFACE( provider->private->provider )->delete_item, ret );

	ret = FMA_IIO_PROVIDER_GET_INTERFACE( provider->private->provider )->delete_item( provider->private->provider, item, messages );

	return( ret );
}

/*
 * fma_io_provider_duplicate_data:
 * @provider: this #FMAIOProvider object.
 * @dest: the target #FMAObjectItem item.
 * @source: the source #FMAObjectItem item.
 * @messages: error messages.
 *
 * Duplicates provider data (if any) from @source to @dest.
 *
 * Returns: the FMAIIOProvider return code.
 */
guint
fma_io_provider_duplicate_data( const FMAIOProvider *provider, FMAObjectItem *dest, const FMAObjectItem *source, GSList **messages )
{
	static const gchar *thisfn = "fma_io_provider_duplicate_data";
	guint ret;
	void *provider_data;

	g_debug( "%s: provider=%p (%s), dest=%p (%s), source=%p (%s), messages=%p", thisfn,
			( void * ) provider, G_OBJECT_TYPE_NAME( provider ),
			( void * ) dest, G_OBJECT_TYPE_NAME( dest ),
			( void * ) source, G_OBJECT_TYPE_NAME( source ),
			( void * ) messages );

	ret = IIO_PROVIDER_CODE_PROGRAM_ERROR;

	g_return_val_if_fail( FMA_IS_IO_PROVIDER( provider ), ret );
	g_return_val_if_fail( FMA_IS_OBJECT_ITEM( dest ), ret );
	g_return_val_if_fail( FMA_IS_OBJECT_ITEM( source ), ret );
	g_return_val_if_fail( FMA_IS_IIO_PROVIDER( provider->private->provider ), ret );

	fma_object_set_provider_data( dest, NULL );
	provider_data = fma_object_get_provider_data( source );

	if( provider_data &&
		FMA_IIO_PROVIDER_GET_INTERFACE( provider->private->provider )->duplicate_data ){
			ret = FMA_IIO_PROVIDER_GET_INTERFACE( provider->private->provider )->duplicate_data( provider->private->provider, dest, source, messages );
	}

	return( ret );
}

/*
 * fma_io_provider_get_readonly_tooltip:
 * @reason: the reason for why an item is not writable.
 *
 * Returns: the associated tooltip, as a newly allocated string which
 * should be g_free() by the caller.
 */
gchar *
fma_io_provider_get_readonly_tooltip( guint reason )
{
	gchar *tooltip;

	tooltip = NULL;

	switch( reason ){
		/* item is writable, so tooltip is empty */
		case IIO_PROVIDER_STATUS_WRITABLE:
			tooltip = g_strdup( "" );
			break;

		case IIO_PROVIDER_STATUS_UNAVAILABLE:
			tooltip = g_strdup( _( "Unavailable I/O provider." ));
			break;

		case IIO_PROVIDER_STATUS_INCOMPLETE_API:
			tooltip = g_strdup( _( "I/O provider implementation lacks of required API." ));
			break;

		case IIO_PROVIDER_STATUS_NOT_WILLING_TO:
			tooltip = g_strdup( _( "I/O provider is not willing to write." ));
			break;

		case IIO_PROVIDER_STATUS_NOT_ABLE_TO:
			tooltip = g_strdup( _( "I/O provider announces itself as unable to write." ));
			break;

		case IIO_PROVIDER_STATUS_LOCKED_BY_ADMIN:
			tooltip = g_strdup( _( "I/O provider has been locked down by an administrator." ));
			break;

		case IIO_PROVIDER_STATUS_LOCKED_BY_USER:
			tooltip = g_strdup( _( "I/O provider has been locked down by the user." ));
			break;

		case IIO_PROVIDER_STATUS_ITEM_READONLY:
			tooltip = g_strdup( _( "Item is read-only." ));
			break;

		case IIO_PROVIDER_STATUS_NO_PROVIDER_FOUND:
			tooltip = g_strdup( _( "No writable I/O provider found." ));
			break;

		default:
			tooltip = g_strdup_printf(
					_( "Item is not writable for an unknown reason (%d).\n%s" ), reason, st_enter_bug );
			break;
	}

	return( tooltip );
}

/*
 * fma_io_provider_get_return_code_label:
 * @code: the return code of an operation.
 *
 * Returns: the associated label, as a newly allocated string which
 * should be g_free() by the caller.
 */
gchar *
fma_io_provider_get_return_code_label( guint code )
{
	gchar *label;

	label = NULL;

	switch( code ){
		case IIO_PROVIDER_CODE_OK:
			label = g_strdup( _( "OK." ));
			break;

		case IIO_PROVIDER_CODE_PROGRAM_ERROR:
			label = g_strdup_printf( _( "Program flow error.\n%s" ), st_enter_bug );
			break;

		case IIO_PROVIDER_CODE_NOT_WILLING_TO_RUN:
			label = g_strdup( _( "The I/O provider is not willing to do that." ));
			break;

		case IIO_PROVIDER_CODE_WRITE_ERROR:
			label = g_strdup( _( "Write error in I/O provider." ));
			break;

		case IIO_PROVIDER_CODE_DELETE_SCHEMAS_ERROR:
			label = g_strdup( _( "Unable to delete GConf schemas." ));
			break;

		case IIO_PROVIDER_CODE_DELETE_CONFIG_ERROR:
			label = g_strdup( _( "Unable to delete configuration." ));
			break;

		default:
			label = g_strdup_printf( _( "Unknown return code (%d).\n%s" ), code, st_enter_bug );
			break;
	}

	return( label );
}
