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

#include <api/na-object-api.h>
#include <api/na-gconf-monitor.h>

#include "na-io-provider.h"
#include "na-gconf-utils.h"
#include "na-iprefs.h"
#include "na-module.h"
#include "na-pivot.h"
#include "na-utils.h"

/* private class data
 */
struct NAPivotClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NAPivotPrivate {
	gboolean           dispose_has_run;

	NAPivotLoadableSet loadable_set;

	/* dynamically loaded modules (extension plugins)
	 */
	GList             *modules;

	/* list of instances to be notified of repository updates
	 * these are called 'consumers' of NAPivot
	 */
	GList             *consumers;

	/* configuration tree
	 */
	GList             *tree;

	/* whether to automatically reload the whole configuration tree
	 * when a modification is detected in one of the underlying I/O
	 * storage subsystems
	 * defaults to FALSE
	 */
	gboolean           automatic_reload;
	GTimeVal           last_event;
	guint              event_source_id;

	/* list of monitoring objects on runtime preferences
	 */
	GList             *monitors;
};

/* signals
 */
enum {
	ACTION_CHANGED,
	LAST_SIGNAL
};

/* NAPivot properties
 */
enum {
	NAPIVOT_PROP_LOADABLE_SET_ID = 1,
};

static GObjectClass *st_parent_class = NULL;
static gint          st_signals[ LAST_SIGNAL ] = { 0 };
static gint          st_timeout_msec = 100;
static gint          st_timeout_usec = 100000;

static GType         register_type( void );
static void          class_init( NAPivotClass *klass );
static void          iprefs_iface_init( NAIPrefsInterface *iface );
static void          instance_init( GTypeInstance *instance, gpointer klass );
static void          instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void          instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void          instance_dispose( GObject *object );
static void          instance_finalize( GObject *object );

/* NAIIOProvider management */
static gboolean      on_item_changed_timeout( NAPivot *pivot );
static gulong        time_val_diff( const GTimeVal *recent, const GTimeVal *old );

static NAObjectItem *get_item_from_tree( const NAPivot *pivot, GList *tree, const gchar *id );

/* NAIPivotConsumer management */
static void          free_consumers( GList *list );

/* NAGConf runtime preferences management */
static void          monitor_runtime_preferences( NAPivot *pivot );
static void          on_preferences_change( GConfClient *client, guint cnxn_id, GConfEntry *entry, NAPivot *pivot );
static void          display_order_changed( NAPivot *pivot );
static void          create_root_menu_changed( NAPivot *pivot );
static void          display_about_changed( NAPivot *pivot );

GType
na_pivot_get_type( void )
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
	static const gchar *thisfn = "na_pivot_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NAPivotClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NAPivot ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	static const GInterfaceInfo iprefs_iface_info = {
		( GInterfaceInitFunc ) iprefs_iface_init,
		NULL,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "NAPivot", &info, 0 );

	g_type_add_interface_static( type, NA_IPREFS_TYPE, &iprefs_iface_info );

	return( type );
}

static void
class_init( NAPivotClass *klass )
{
	static const gchar *thisfn = "na_pivot_class_init";
	GObjectClass *object_class;
	GParamSpec *spec;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->set_property = instance_set_property;
	object_class->get_property = instance_get_property;
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	spec = g_param_spec_int(
			NAPIVOT_PROP_LOADABLE_SET,
			"Loadable population set",
			"Nature of population to be loaded", 0, INT_MAX, 0,
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, NAPIVOT_PROP_LOADABLE_SET_ID, spec );

	klass->private = g_new0( NAPivotClassPrivate, 1 );

	/* register the signal and its default handler
	 * this signal should be sent by the IOProvider when an object
	 * has changed in the underlying storage subsystem
	 */
	st_signals[ ACTION_CHANGED ] = g_signal_new(
				NA_PIVOT_SIGNAL_ACTION_CHANGED,
				NA_IIO_PROVIDER_TYPE,
				G_SIGNAL_RUN_LAST,
				0,
				NULL,
				NULL,
				g_cclosure_marshal_VOID__POINTER,
				G_TYPE_NONE,
				1,
				G_TYPE_POINTER );
}

static void
iprefs_iface_init( NAIPrefsInterface *iface )
{
	static const gchar *thisfn = "na_pivot_iprefs_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "na_pivot_instance_init";
	NAPivot *self;

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );
	g_return_if_fail( NA_IS_PIVOT( instance ));
	self = NA_PIVOT( instance );

	self->private = g_new0( NAPivotPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->loadable_set = 0;
	self->private->modules = NULL;
	self->private->consumers = NULL;
	self->private->tree = NULL;
	self->private->automatic_reload = FALSE;
	self->private->event_source_id = 0;
	self->private->monitors = NULL;
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	NAPivot *self;

	g_return_if_fail( NA_IS_PIVOT( object ));
	self = NA_PIVOT( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case NAPIVOT_PROP_LOADABLE_SET_ID:
				g_value_set_int( value, self->private->loadable_set );
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
	NAPivot *self;

	g_return_if_fail( NA_IS_PIVOT( object ));
	self = NA_PIVOT( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case NAPIVOT_PROP_LOADABLE_SET_ID:
				self->private->loadable_set = g_value_get_int( value );
				break;

			default:
				G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
				break;
		}
	}
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "na_pivot_instance_dispose";
	NAPivot *self;

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));
	g_return_if_fail( NA_IS_PIVOT( object ));
	self = NA_PIVOT( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* release modules */
		na_module_release_modules( self->private->modules );
		self->private->modules = NULL;

		/* release list of NAIPivotConsumers */
		free_consumers( self->private->consumers );
		self->private->consumers = NULL;

		/* release item tree */
		na_object_free_items_list( self->private->tree );
		self->private->tree = NULL;

		/* release the GConf monitoring */
		na_gconf_monitor_release_monitors( self->private->monitors );

		/* release the I/O Provider objects */
		na_io_provider_terminate();

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "na_pivot_instance_finalize";
	NAPivot *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_return_if_fail( NA_IS_PIVOT( object ));
	self = NA_PIVOT( object );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * na_pivot_new:
 *
 * Returns: a newly allocated #NAPivot object.
 *
 * The returned #NAPivot is initialized with the current list of
 * #NAObjectItem-derived object.
 */
NAPivot *
na_pivot_new( NAPivotLoadableSet loadable )
{
	static const gchar *thisfn = "na_pivot_new";
	NAPivot *pivot;

	g_debug( "%s", thisfn );

	pivot = g_object_new(
			NA_PIVOT_TYPE,
			NAPIVOT_PROP_LOADABLE_SET, loadable,
			NULL );

	pivot->private->modules = na_module_load_modules();

	monitor_runtime_preferences( pivot );

	return( pivot );
}

/**
 * na_pivot_dump:
 * @pivot: the #NAPivot object do be dumped.
 *
 * Dumps the content of a #NAPivot object.
 */
void
na_pivot_dump( const NAPivot *pivot )
{
	static const gchar *thisfn = "na_pivot_dump";
	GList *it;
	int i;

	if( !pivot->private->dispose_has_run ){

		g_debug( "%s:     loadable_set=%d", thisfn, pivot->private->loadable_set );
		g_debug( "%s:          modules=%p (%d elts)", thisfn, ( void * ) pivot->private->modules, g_list_length( pivot->private->modules ));
		g_debug( "%s:        consumers=%p (%d elts)", thisfn, ( void * ) pivot->private->consumers, g_list_length( pivot->private->consumers ));
		g_debug( "%s:             tree=%p (%d elts)", thisfn, ( void * ) pivot->private->tree, g_list_length( pivot->private->tree ));
		g_debug( "%s: automatic_reload=%s", thisfn, pivot->private->automatic_reload ? "True":"False" );
		g_debug( "%s:         monitors=%p (%d elts)", thisfn, ( void * ) pivot->private->monitors, g_list_length( pivot->private->monitors ));

		for( it = pivot->private->tree, i = 0 ; it ; it = it->next ){
			g_debug( "%s:     [%d]: %p", thisfn, i++, it->data );
		}
	}
}

/**
 * na_pivot_get_providers:
 * @pivot: this #NAPivot instance.
 * @type: the type of searched interface.
 * For now, we only have NA_IIO_PROVIDER_TYPE interfaces.
 *
 * Returns: a newly allocated list of providers of the required interface.
 *
 * This function is called by interfaces API in order to find the
 * list of providers registered for their own given interface.
 *
 * The returned list should be release by calling na_pivot_free_providers().
 */
GList *
na_pivot_get_providers( const NAPivot *pivot, GType type )
{
	static const gchar *thisfn = "na_pivot_get_providers";
	GList *list = NULL;

	g_debug( "%s: pivot=%p", thisfn, ( void * ) pivot );
	g_return_val_if_fail( NA_IS_PIVOT( pivot ), NULL );

	if( !pivot->private->dispose_has_run ){

		list = na_module_get_extensions_for_type( pivot->private->modules, type );
		g_debug( "%s: list=%p, count=%d", thisfn, ( void * ) list, list ? g_list_length( list ) : 0 );
	}

	return( list );
}

/**
 * na_pivot_free_providers:
 * @providers: a list of providers.
 *
 * Frees a list of providers as returned from na_pivot_get_providers().
 */
void
na_pivot_free_providers( GList *providers )
{
	static const gchar *thisfn = "na_pivot_free_providers";

	g_debug( "%s: providers=%p", thisfn, ( void * ) providers );

	na_module_free_extensions_list( providers );
}

/*
 * this handler is trigerred by IIOProviders when an action is changed
 * in their underlying storage subsystems
 * we don't care of updating our internal list with each and every
 * atomic modification
 * instead we wait for the end of notifications serie, and then reload
 * the whole list of actions
 */
void
na_pivot_item_changed_handler( NAIIOProvider *provider, const gchar *id, NAPivot *pivot  )
{
	static const gchar *thisfn = "na_pivot_item_changed_handler";

	g_debug( "%s: provider=%p, id=%s, pivot=%p", thisfn, ( void * ) provider, id, ( void * ) pivot );

	g_return_if_fail( NA_IS_IIO_PROVIDER( provider ));
	g_return_if_fail( NA_IS_PIVOT( pivot ));

	if( !pivot->private->dispose_has_run ){

		/* set a timeout to notify clients at the end of the serie */
		g_get_current_time( &pivot->private->last_event );

		if( !pivot->private->event_source_id ){
			pivot->private->event_source_id =
				g_timeout_add( st_timeout_msec, ( GSourceFunc ) on_item_changed_timeout, pivot );
		}
	}
}

/*
 * this timer is set when we receive the first event of a serie
 * we continue to loop until last event is at least one half of a
 * second old
 *
 * there is no race condition here as we are not multithreaded
 * or .. is there ?
 */
static gboolean
on_item_changed_timeout( NAPivot *pivot )
{
	static const gchar *thisfn = "na_pivot_on_item_changed_timeout";
	GTimeVal now;
	gulong diff;
	GList *ic;

	g_debug( "%s: pivot=%p", thisfn, pivot );
	g_return_val_if_fail( NA_IS_PIVOT( pivot ), FALSE );

	g_get_current_time( &now );
	diff = time_val_diff( &now, &pivot->private->last_event );
	if( diff < st_timeout_usec ){
		return( TRUE );
	}

	if( pivot->private->automatic_reload ){
		na_pivot_load_items( pivot );
	}

	for( ic = pivot->private->consumers ; ic ; ic = ic->next ){
		na_ipivot_consumer_notify_actions_changed( NA_IPIVOT_CONSUMER( ic->data ));
	}

	pivot->private->event_source_id = 0;
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

/**
 * na_pivot_get_items:
 * @pivot: this #NAPivot instance.
 *
 * Returns: the current configuration tree.
 *
 * The returned list is owned by this #NAPivot object, and should not
 * be g_free(), nor g_object_unref() by the caller.
 */
GList *
na_pivot_get_items( const NAPivot *pivot )
{
	GList *tree = NULL;

	g_return_val_if_fail( NA_IS_PIVOT( pivot ), NULL );

	if( !pivot->private->dispose_has_run ){
		tree = pivot->private->tree;
	}

	return( tree );
}

/**
 * na_pivot_load_items:
 * @pivot: this #NAPivot instance.
 *
 * Loads the hierarchical list of items from I/O providers.
 */
void
na_pivot_load_items( NAPivot *pivot )
{
	static const gchar *thisfn = "na_pivot_load_items";
	GSList *messages, *im;

	g_debug( "%s: pivot=%p", thisfn, ( void * ) pivot );
	g_return_if_fail( NA_IS_PIVOT( pivot ));

	if( !pivot->private->dispose_has_run ){

		na_object_free_items_list( pivot->private->tree );

		pivot->private->tree = na_io_provider_read_items( pivot, &messages );

		for( im = messages ; im ; im = im->next ){
			g_warning( "%s: %s", thisfn, ( const gchar * ) im->data );
		}

		na_utils_free_string_list( messages );
	}
}

/**
 * na_pivot_add_item:
 * @pivot: this #NAPivot instance.
 * @item: the #NAObjectItem to be added to the list.
 *
 * Adds a new item to the list.
 *
 * We take the provided pointer. The provided @item should so not
 * be g_object_unref() by the caller.
 */
void
na_pivot_add_item( NAPivot *pivot, const NAObjectItem *item )
{
	g_return_if_fail( NA_IS_PIVOT( pivot ));
	g_return_if_fail( NA_IS_OBJECT_ITEM( item ));

	if( !pivot->private->dispose_has_run ){
		pivot->private->tree = g_list_append( pivot->private->tree, ( gpointer ) item );
	}
}

/**
 * na_pivot_get_action:
 * @pivot: this #NAPivot instance.
 * @id: the required item identifier.
 *
 * Returns the specified action.
 *
 * Returns: the required #NAObjectItem-derived object, or NULL if not
 * found.
 * The returned pointer is owned by #NAPivot, and should not be
 * g_free() nor g_object_unref() by the caller.
 */
NAObjectItem *
na_pivot_get_item( const NAPivot *pivot, const gchar *id )
{
	NAObjectItem *object = NULL;

	g_return_val_if_fail( NA_IS_PIVOT( pivot ), NULL );

	if( !pivot->private->dispose_has_run ){

		if( !id || !strlen( id )){
			return( NULL );
		}

		object = get_item_from_tree( pivot, pivot->private->tree, id );
	}

	return( object );
}

/**
 * na_pivot_remove_item:
 * @pivot: this #NAPivot instance.
 * @item: the #NAObjectItem to be removed from the list.
 *
 * Removes a #NAObjectItem from the hierarchical tree.
 *
 * Note that #NAPivot also g_object_unref() the removed #NAObjectItem.
 *
 * Last, note that the @item may have been already deleted, when its
 * parents has itself been removed from @pivot.
 */
void
na_pivot_remove_item( NAPivot *pivot, NAObject *item )
{
	g_debug( "na_pivot_remove_item: pivot=%p, item=%p (%s)",
			( void * ) pivot,
			( void * ) item, G_IS_OBJECT( item ) ? G_OBJECT_TYPE_NAME( item ) : "(null)" );

	g_return_if_fail( NA_IS_PIVOT( pivot ));

	if( !pivot->private->dispose_has_run ){

		pivot->private->tree = g_list_remove( pivot->private->tree, ( gconstpointer ) item );

		if( G_IS_OBJECT( item )){
			g_object_unref( item );
		}
	}
}

/**
 * na_pivot_is_item_writable:
 * @pivot: this #NAPivot object.
 * @item: the #NAObjectItem to be written.
 * @reason: the reason for what @item may not be writable.
 *
 * Returns: %TRUE: if @item is actually writable, given the current
 * status of its provider, %FALSE else.
 *
 * For an item be actually writable:
 * - the item must not be itself in a read-only store, which has been
 *   checked when first reading it
 * - the provider must be willing (resp. able) to write
 * - the provider must not has been locked by the admin
 * - the writability of the provider must not have been removed by the user
 * - the whole configuration must not have been locked by the admin.
 */
gboolean
na_pivot_is_item_writable( const NAPivot *pivot, const NAObjectItem *item, gint *reason )
{
	gboolean writable;
	NAIOProvider *provider;

	g_return_val_if_fail( NA_IS_PIVOT( pivot ), FALSE );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), FALSE );

	writable = FALSE;
	if( reason ){
		*reason = NA_IIO_PROVIDER_STATUS_UNDETERMINED;
	}

	if( !pivot->private->dispose_has_run ){

		writable = TRUE;
		if( reason ){
			*reason = NA_IIO_PROVIDER_STATUS_WRITABLE;
		}

		if( writable ){
			if( na_object_is_readonly( item )){
				writable = FALSE;
				if( reason ){
					*reason = NA_IIO_PROVIDER_STATUS_ITEM_READONLY;
				}
			}
		}

		if( writable ){
			provider = na_object_get_provider( item );
			if( provider ){
				if( !na_io_provider_is_willing_to_write( provider )){
					writable = FALSE;
					if( reason ){
						*reason = NA_IIO_PROVIDER_STATUS_PROVIDER_NOT_WILLING_TO;
					}
				} else if( na_io_provider_is_locked_by_admin( provider, pivot )){
					writable = FALSE;
					if( reason ){
						*reason = NA_IIO_PROVIDER_STATUS_PROVIDER_LOCKED_BY_ADMIN;
					}
				} else if( !na_io_provider_is_user_writable( provider, pivot )){
					writable = FALSE;
					if( reason ){
						*reason = NA_IIO_PROVIDER_STATUS_PROVIDER_LOCKED_BY_USER;
					}
				} else if( na_pivot_is_configuration_locked_by_admin( pivot )){
					writable = FALSE;
					if( reason ){
						*reason = NA_IIO_PROVIDER_STATUS_CONFIGURATION_LOCKED_BY_ADMIN;
					}
				} else if( !na_io_provider_has_write_api( provider )){
					writable = FALSE;
					if( reason ){
						*reason = NA_IIO_PROVIDER_STATUS_NO_API;
					}
				}

			/* the get_writable_provider() api already takes above checks
			 */
			} else {
				provider = na_io_provider_get_writable_provider( pivot );
				if( !provider ){
					writable = FALSE;
					if( reason ){
						*reason = NA_IIO_PROVIDER_STATUS_NO_PROVIDER_FOUND;
					}
				}
			}
		}
	}

	return( writable );
}

/**
 * na_pivot_write_item:
 * @pivot: this #NAPivot instance.
 * @item: a #NAObjectItem to be written by the storage subsystem.
 * @messages: the I/O provider can allocate and store here its error
 * messages.
 *
 * Writes an item (an action or a menu).
 *
 * Returns: the #NAIIOProvider return code.
 */
guint
na_pivot_write_item( const NAPivot *pivot, NAObjectItem *item, GSList **messages )
{
	guint ret;
	gint reason;

	ret = NA_IIO_PROVIDER_CODE_PROGRAM_ERROR;

	g_return_val_if_fail( NA_IS_PIVOT( pivot ), ret );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), ret );
	g_return_val_if_fail( messages, ret );

	if( !pivot->private->dispose_has_run ){

		NAIOProvider *provider = na_object_item_get_provider( item );
		if( !provider ){
			provider = na_io_provider_get_writable_provider( pivot );

			if( !provider ){
				ret = NA_IIO_PROVIDER_STATUS_NO_PROVIDER_FOUND;

			} else {
				na_object_set_provider( item, provider );
			}
		}

		if( provider ){

			if( !na_pivot_is_item_writable( pivot, item, &reason )){
				ret = ( guint ) reason;

			} else {
				ret = na_io_provider_write_item( provider, item, messages );
			}
		}
	}

	return( ret );
}

/**
 * na_pivot_delete_item:
 * @pivot: this #NAPivot instance.
 * @item: the #NAObjectItem to be deleted from the storage subsystem.
 * @messages: the I/O provider can allocate and store here its error
 * messages.
 *
 * Deletes an action from the I/O storage subsystem.
 *
 * Returns: the #NAIIOProvider return code.
 *
 * Note that a new item, not already written to an I/O subsystem,
 * doesn't have any attached provider. We so do nothing and return OK...
 */
guint
na_pivot_delete_item( const NAPivot *pivot, const NAObjectItem *item, GSList **messages )
{
	guint ret;
	gint reason;

	ret = NA_IIO_PROVIDER_CODE_PROGRAM_ERROR;

	g_return_val_if_fail( NA_IS_PIVOT( pivot ), ret );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), ret );
	g_return_val_if_fail( messages, ret );

	if( !pivot->private->dispose_has_run ){

		NAIOProvider *provider = na_object_item_get_provider( item );
		if( provider ){

			if( !na_pivot_is_item_writable( pivot, item, &reason )){
				ret = ( guint ) reason;

			} else {
				ret = na_io_provider_delete_item( provider, item, messages );
			}

		} else {
			ret = NA_IIO_PROVIDER_CODE_OK;
		}
	}

	return( ret );
}

/**
 * na_pivot_register_consumer:
 * @pivot: this #NAPivot instance.
 * @consumer: a #NAIPivotConsumer which wishes be notified of any
 * modification of an action or a menu in any of the underlying I/O
 * storage subsystems.
 *
 * Registers a new consumer to be notified of configuration modification.
 */
void
na_pivot_register_consumer( NAPivot *pivot, const NAIPivotConsumer *consumer )
{
	static const gchar *thisfn = "na_pivot_register_consumer";

	g_debug( "%s: pivot=%p, consumer=%p", thisfn, ( void * ) pivot, ( void * ) consumer );
	g_return_if_fail( NA_IS_PIVOT( pivot ));
	g_return_if_fail( NA_IS_IPIVOT_CONSUMER( consumer ));

	if( !pivot->private->dispose_has_run ){
		pivot->private->consumers = g_list_prepend( pivot->private->consumers, ( gpointer ) consumer );
	}
}

/**
 * na_pivot_set_automatic_reload:
 * @pivot: this #NAPivot instance.
 * @reload: whether this #NAPivot instance should automatically reload
 * its list of actions when I/O providers advertize it of a
 * modification.
 *
 * Sets the automatic reload flag.
 *
 * Note that even if the #NAPivot instance is not authorized to
 * automatically reload its list of actions when it is advertized of
 * a modification by one of the I/O providers, it always sends an
 * ad-hoc notification to its consumers.
 */
void
na_pivot_set_automatic_reload( NAPivot *pivot, gboolean reload )
{
	g_return_if_fail( NA_IS_PIVOT( pivot ));

	if( !pivot->private->dispose_has_run ){

		pivot->private->automatic_reload = reload;
	}
}

/**
 * na_pivot_is_disable_loadable:
 * @pivot: this #NAPivot instance.
 *
 * Returns: %TRUE if disabled items should be loaded, %FALSE else.
 */
gboolean
na_pivot_is_disable_loadable( const NAPivot *pivot )
{
	gboolean is_loadable;

	is_loadable = FALSE;
	g_return_val_if_fail( NA_IS_PIVOT( pivot ), is_loadable );

	if( !pivot->private->dispose_has_run ){

		is_loadable = ( pivot->private->loadable_set & PIVOT_LOAD_DISABLED );
	}

	return( is_loadable );
}

/**
 * na_pivot_is_invalid_loadable:
 * @pivot: this #NAPivot instance.
 *
 * Returns: %TRUE if invalid items should be loaded, %FALSE else.
 */
gboolean
na_pivot_is_invalid_loadable( const NAPivot *pivot )
{
	gboolean is_loadable;

	is_loadable = FALSE;
	g_return_val_if_fail( NA_IS_PIVOT( pivot ), is_loadable );

	if( !pivot->private->dispose_has_run ){

		is_loadable = ( pivot->private->loadable_set & PIVOT_LOAD_INVALID );
	}

	return( is_loadable );
}

/**
 * na_pivot_sort_alpha_asc:
 * @a: first #NAObjectId.
 * @b: second #NAObjectId.
 *
 * Sort the objects in alphabetical ascending order of their label.
 *
 * Returns:
 * -1 if @a must be sorted before @b,
 *  0 if @a and @b are equal from the local point of view,
 *  1 if @a must be sorted after @b.
 */
gint
na_pivot_sort_alpha_asc( const NAObjectId *a, const NAObjectId *b )
{
	gchar *label_a, *label_b;
	gint compare;

	label_a = na_object_get_label( a );
	label_b = na_object_get_label( b );

	compare = g_utf8_collate( label_a, label_b );

	g_free( label_b );
	g_free( label_a );

	return( compare );
}

/**
 * na_pivot_sort_alpha_desc:
 * @a: first #NAObjectId.
 * @b: second #NAObjectId.
 *
 * Sort the objects in alphabetical descending order of their label.
 *
 * Returns:
 * -1 if @a must be sorted before @b,
 *  0 if @a and @b are equal from the local point of view,
 *  1 if @a must be sorted after @b.
 */
gint
na_pivot_sort_alpha_desc( const NAObjectId *a, const NAObjectId *b )
{
	return( -1 * na_pivot_sort_alpha_asc( a, b ));
}

/**
 * na_pivot_is_level_zero_writable:
 * @pivot: this #NAPivot instance.
 *
 * Returns: %TRUE if we are able to update the level-zero list of items,
 * %FALSE else.
 */
gboolean
na_pivot_is_level_zero_writable( const NAPivot *pivot )
{
	gboolean writable;

	writable = FALSE;
	g_return_val_if_fail( NA_IS_PIVOT( pivot ), writable );

	if( !pivot->private->dispose_has_run ){

		writable = !na_pivot_is_configuration_locked_by_admin( pivot );
	}

	return( writable );
}

/**
 * na_pivot_write_level_zero:
 * @pivot: this #NAPivot instance.
 * @items: full current tree of items in #NactIActionsList treeview.
 *
 * Writes as a GConf preference order and content of level zero items.
 */
void
na_pivot_write_level_zero( const NAPivot *pivot, GList *items )
{
	static const gchar *thisfn = "na_pivot_write_level_zero";
	GList *it;
	gchar *id;
	GSList *content;

	g_debug( "%s: pivot=%p, items=%p (%d items)", thisfn, ( void * ) pivot, ( void * ) items, g_list_length( items ));
	g_return_if_fail( NA_IS_PIVOT( pivot ));

	if( !pivot->private->dispose_has_run ){

		content = NULL;
		for( it = items ; it ; it = it->next ){
			id = na_object_get_id( it->data );
			content = g_slist_prepend( content, id );
		}
		content = g_slist_reverse( content );

		na_iprefs_set_level_zero_items( NA_IPREFS( pivot ), content );

		na_utils_free_string_list( content );
	}
}

/**
 * na_pivot_is_configuration_locked_by_admin:
 * @pivot: this #NAPivot.
 *
 * Returns: %TRUE if the whole configuration has been locked by an
 * administrator, %FALSE else.
 */
gboolean
na_pivot_is_configuration_locked_by_admin( const NAPivot *pivot )
{
	gboolean locked;
	GConfClient *gconf;
	gchar *path;

	locked = FALSE;
	g_return_val_if_fail( NA_IS_PIVOT( pivot ), FALSE );

	if( !pivot->private->dispose_has_run ){

		gconf = na_iprefs_get_gconf_client( NA_IPREFS( pivot ));
		path = gconf_concat_dir_and_key( NAUTILUS_ACTIONS_GCONF_BASEDIR, "mandatory/all/locked" );

		locked = na_gconf_utils_read_bool( gconf, path, FALSE, FALSE );

		g_free( path );
	}

	return( locked );
}

static NAObjectItem *
get_item_from_tree( const NAPivot *pivot, GList *tree, const gchar *id )
{
	GList *subitems, *ia;
	NAObjectItem *found = NULL;

	for( ia = tree ; ia && !found ; ia = ia->next ){

		gchar *i_id = na_object_get_id( NA_OBJECT( ia->data ));

		if( !g_ascii_strcasecmp( id, i_id )){
			found = NA_OBJECT_ITEM( ia->data );
		}

		if( !found && NA_IS_OBJECT_ITEM( ia->data )){
			subitems = na_object_get_items_list( ia->data );
			found = get_item_from_tree( pivot, subitems, id );
		}
	}

	return( found );
}

static void
free_consumers( GList *consumers )
{
	/*g_list_foreach( consumers, ( GFunc ) g_object_unref, NULL );*/
	g_list_free( consumers );
}

static void
monitor_runtime_preferences( NAPivot *pivot )
{
	static const gchar *thisfn = "na_pivot_monitor_runtime_preferences";
	GList *list = NULL;

	g_debug( "%s: pivot=%p", thisfn, ( void * ) pivot );
	g_return_if_fail( NA_IS_PIVOT( pivot ));
	g_return_if_fail( !pivot->private->dispose_has_run );

	list = g_list_prepend( list,
			na_gconf_monitor_new(
					NA_GCONF_PREFS_PATH,
					( GConfClientNotifyFunc ) on_preferences_change,
					pivot ));

	pivot->private->monitors = list;
}

static void
on_preferences_change( GConfClient *client, guint cnxn_id, GConfEntry *entry, NAPivot *pivot )
{
	/*static const gchar *thisfn = "na_pivot_on_preferences_change";*/
	const gchar *key;
	gchar *key_entry;

	g_return_if_fail( NA_IS_PIVOT( pivot ));

	key = gconf_entry_get_key( entry );
	key_entry = na_utils_path_extract_last_dir( key );
	/*g_debug( "%s: key=%s", thisfn, key_entry );*/

	if( !g_ascii_strcasecmp( key_entry, IPREFS_CREATE_ROOT_MENU )){
		create_root_menu_changed( pivot );
	}

	if( !g_ascii_strcasecmp( key_entry, IPREFS_ADD_ABOUT_ITEM )){
		display_about_changed( pivot );
	}

	if( !g_ascii_strcasecmp( key_entry, IPREFS_DISPLAY_ALPHABETICAL_ORDER )){
		display_order_changed( pivot );
	}

	g_free( key_entry );
}

static void
display_order_changed( NAPivot *pivot )
{
	static const gchar *thisfn = "na_pivot_display_order_changed";
	GList *ic;
	gint order_mode;

	g_debug( "%s: pivot=%p", thisfn, ( void * ) pivot );
	g_assert( NA_IS_PIVOT( pivot ));

	if( !pivot->private->dispose_has_run ){

		order_mode = na_iprefs_get_order_mode( NA_IPREFS( pivot ));

		for( ic = pivot->private->consumers ; ic ; ic = ic->next ){
			na_ipivot_consumer_notify_of_display_order_change( NA_IPIVOT_CONSUMER( ic->data ), order_mode );
		}
	}
}

static void
create_root_menu_changed( NAPivot *pivot )
{
	static const gchar *thisfn = "na_pivot_create_root_menu_changed";
	GList *ic;
	gboolean enabled;

	g_debug( "%s: pivot=%p", thisfn, ( void * ) pivot );
	g_assert( NA_IS_PIVOT( pivot ));

	if( !pivot->private->dispose_has_run ){

		enabled = na_iprefs_should_create_root_menu( NA_IPREFS( pivot ));

		for( ic = pivot->private->consumers ; ic ; ic = ic->next ){
			na_ipivot_consumer_notify_of_create_root_menu_change( NA_IPIVOT_CONSUMER( ic->data ), enabled );
		}
	}
}

static void
display_about_changed( NAPivot *pivot )
{
	static const gchar *thisfn = "na_pivot_display_about_changed";
	GList *ic;
	gboolean enabled;

	g_debug( "%s: pivot=%p", thisfn, ( void * ) pivot );
	g_assert( NA_IS_PIVOT( pivot ));

	if( !pivot->private->dispose_has_run ){

		enabled = na_iprefs_should_add_about_item( NA_IPREFS( pivot ));

		for( ic = pivot->private->consumers ; ic ; ic = ic->next ){
			na_ipivot_consumer_notify_of_display_about_change( NA_IPIVOT_CONSUMER( ic->data ), enabled );
		}
	}
}
