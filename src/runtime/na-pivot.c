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
#include <uuid/uuid.h>

#include "na-object-api.h"
#include "na-iio-provider.h"
#include "na-gconf-monitor.h"
#include "na-gconf-provider.h"
#include "na-iprefs.h"
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
	gboolean dispose_has_run;

	/* list of instances to be notified of repository updates
	 * these are called 'consumers' of NAPivot
	 */
	GList   *consumers;

	/* list of NAIIOProvider interface providers
	 * needs to be in the instance rather than in the class to be able
	 * to pass NAPivot object to the IO provider, so that the later
	 * is able to have access to the former (and its list of actions)
	 */
	GList   *providers;

	/* configuration tree
	 */
	GList   *tree;

	/* whether to automatically reload the whole configuration tree
	 * when a modification has been detected in one of the underlying
	 * I/O storage subsystems
	 * defaults to FALSE
	 */
	gboolean automatic_reload;

	/* list of monitoring objects on runtime preferences
	 */
	GList   *monitors;
};

enum {
	ACTION_CHANGED,
	LAST_SIGNAL
};

static GObjectClass *st_parent_class = NULL;
static gint          st_signals[ LAST_SIGNAL ] = { 0 };
static GTimeVal      st_last_event;
static guint         st_event_source_id = 0;
static gint          st_timeout_msec = 100;
static gint          st_timeout_usec = 100000;

static GType     register_type( void );
static void      class_init( NAPivotClass *klass );
static void      iprefs_iface_init( NAIPrefsInterface *iface );
static void      instance_init( GTypeInstance *instance, gpointer klass );
static void      instance_dispose( GObject *object );
static void      instance_finalize( GObject *object );

static NAObject *get_item_from_tree( const NAPivot *pivot, GList *tree, uuid_t uuid );

/* NAIPivotConsumer management */
static void      free_consumers( GList *list );

/* NAIIOProvider management */
static void      register_io_providers( NAPivot *pivot );
static void      action_changed_handler( NAPivot *pivot, gpointer user_data );
static gboolean  on_actions_changed_timeout( gpointer user_data );
static gulong    time_val_diff( const GTimeVal *recent, const GTimeVal *old );

/* NAGConf runtime preferences management */
static void      monitor_runtime_preferences( NAPivot *pivot );

static void      on_preferences_change( GConfClient *client, guint cnxn_id, GConfEntry *entry, NAPivot *pivot );
static void      display_order_changed( NAPivot *pivot );
static void      display_about_changed( NAPivot *pivot );

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

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NAPivotClassPrivate, 1 );

	/* register the signal and its default handler
	 * this signal should be sent by the IIOProvider when an action
	 * has changed in the underlying storage subsystem
	 */
	st_signals[ ACTION_CHANGED ] = g_signal_new_class_handler(
				NA_IIO_PROVIDER_SIGNAL_ACTION_CHANGED,
				G_TYPE_FROM_CLASS( klass ),
				G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
				( GCallback ) action_changed_handler,
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

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );
	g_return_if_fail( NA_IS_PIVOT( instance ));
	self = NA_PIVOT( instance );

	self->private = g_new0( NAPivotPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->consumers = NULL;
	self->private->providers = NULL;
	self->private->tree = NULL;
	self->private->automatic_reload = FALSE;
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "na_pivot_instance_dispose";
	NAPivot *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_return_if_fail( NA_IS_PIVOT( object ));
	self = NA_PIVOT( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* release list of NAIPivotConsumers */
		free_consumers( self->private->consumers );
		self->private->consumers = NULL;

		/* release list of NAIIOProviders */
		na_pivot_free_providers( self->private->providers );
		self->private->providers = NULL;

		/* release item tree */
		na_object_free_items( self->private->tree );
		self->private->tree = NULL;

		/* release the GConf monitoring */
		na_gconf_monitor_release_monitors( self->private->monitors );

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
 * @target: a GObject which wishes be notified of any modification of
 * an action in any of the underlying I/O storage subsystems.
 *
 * Allocates a new #NAPivot object.
 *
 * The target object should implement NAIPivotConsumer interface.
 * It will be triggered on changes on IIOProviders and runtime
 * preferences.
 */
NAPivot *
na_pivot_new( const NAIPivotConsumer *target )
{
	static const gchar *thisfn = "na_pivot_new";
	NAPivot *pivot;

	g_debug( "%s: target=%p", thisfn, ( void * ) target );
	g_return_val_if_fail( NA_IS_IPIVOT_CONSUMER( target ) || !target, NULL );

	pivot = g_object_new( NA_PIVOT_TYPE, NULL );

	register_io_providers( pivot );

	if( target ){
		na_pivot_register_consumer( pivot, target );
	}

	monitor_runtime_preferences( pivot );

	pivot->private->tree = na_iio_provider_get_items_tree( pivot );

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

		g_debug( "%s: consumers=%p (%d elts)", thisfn, ( void * ) pivot->private->consumers, g_list_length( pivot->private->consumers ));
		g_debug( "%s: providers=%p (%d elts)", thisfn, ( void * ) pivot->private->providers, g_list_length( pivot->private->providers ));
		g_debug( "%s:      tree=%p (%d elts)", thisfn, ( void * ) pivot->private->tree, g_list_length( pivot->private->tree ));

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
	GList *ip;

	g_debug( "%s: pivot=%p", thisfn, ( void * ) pivot );
	g_return_val_if_fail( NA_IS_PIVOT( pivot ), NULL );

	if( !pivot->private->dispose_has_run ){

		for( ip = pivot->private->providers ; ip ; ip = ip->next ){
			if( G_TYPE_CHECK_INSTANCE_TYPE( G_OBJECT( ip->data ), type )){
				list = g_list_prepend( list, g_object_ref( ip->data ));
			}
		}
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

	g_list_foreach( providers, ( GFunc ) g_object_unref, NULL );
	g_list_free( providers );
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
 * na_pivot_reload_items:
 * @pivot: this #NAPivot instance.
 *
 * Reloads the hierarchical list of items from I/O providers.
 */
void
na_pivot_reload_items( NAPivot *pivot )
{
	g_return_if_fail( NA_IS_PIVOT( pivot ));

	if( !pivot->private->dispose_has_run ){

		na_object_free_items( pivot->private->tree );

		pivot->private->tree = na_iio_provider_get_items_tree( pivot );
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
na_pivot_add_item( NAPivot *pivot, const NAObject *item )
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
 * @uuid: the required globally unique identifier (uuid).
 *
 * Returns the specified action.
 *
 * Returns: the required #NAAction object, or NULL if not found.
 * The returned pointer is owned by #NAPivot, and should not be
 * g_free() nor g_object_unref() by the caller.
 */
NAObject *
na_pivot_get_item( const NAPivot *pivot, const gchar *uuid )
{
	uuid_t uuid_bin;
	NAObject *object = NULL;

	g_return_val_if_fail( NA_IS_PIVOT( pivot ), NULL );

	if( !pivot->private->dispose_has_run ){

		if( !uuid || !strlen( uuid )){
			return( NULL );
		}

		uuid_parse( uuid, uuid_bin );

		object = get_item_from_tree( pivot, pivot->private->tree, uuid_bin );
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
			( void * ) item, G_OBJECT_TYPE_NAME( item ));

	g_return_if_fail( NA_IS_PIVOT( pivot ));

	if( !pivot->private->dispose_has_run ){

		pivot->private->tree = g_list_remove( pivot->private->tree, ( gconstpointer ) item );

		if( NA_IS_OBJECT( item )){
			g_object_unref( item );
		}
	}
}

/**
 * na_pivot_delete_item:
 * @pivot: this #NAPivot instance.
 * @item: the #NAObjectItem to be deleted from the storage subsystem.
 * @message: the I/O provider can allocate and store here an error
 * message.
 *
 * Deletes an action from the I/O storage subsystem.
 *
 * Returns: the #NAIIOProvider return code.
 */
guint
na_pivot_delete_item( const NAPivot *pivot, const NAObject *item, gchar **message )
{
	guint ret = NA_IIO_PROVIDER_NOT_WILLING_TO_WRITE;

	g_return_val_if_fail( NA_IS_PIVOT( pivot ), NA_IIO_PROVIDER_PROGRAM_ERROR );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), NA_IIO_PROVIDER_PROGRAM_ERROR );
	g_return_val_if_fail( message, NA_IIO_PROVIDER_PROGRAM_ERROR );

	if( !pivot->private->dispose_has_run ){
		ret = na_iio_provider_delete_item( pivot, item, message );
	}

	return( ret );
}

/**
 * na_pivot_write_item:
 * @pivot: this #NAPivot instance.
 * @item: a #NAObjectItem to be written by the storage subsystem.
 * @message: the I/O provider can allocate and store here an error
 * message.
 *
 * Writes an item (an action or a menu).
 *
 * Returns: the #NAIIOProvider return code.
 */
guint
na_pivot_write_item( const NAPivot *pivot, NAObject *item, gchar **message )
{
	guint ret = NA_IIO_PROVIDER_NOT_WILLING_TO_WRITE;

	g_return_val_if_fail( NA_IS_PIVOT( pivot ), NA_IIO_PROVIDER_PROGRAM_ERROR );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), NA_IIO_PROVIDER_PROGRAM_ERROR );
	g_return_val_if_fail( message, NA_IIO_PROVIDER_PROGRAM_ERROR );

	if( !pivot->private->dispose_has_run ){
		ret = na_iio_provider_write_item( pivot, item, message );
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
 * na_pivot_get_automatic_reload:
 * @pivot: this #NAPivot instance.
 *
 * Returns: the automatic reload flag.
 */
gboolean
na_pivot_get_automatic_reload( const NAPivot *pivot )
{
	gboolean auto_reload = FALSE;

	g_return_val_if_fail( NA_IS_PIVOT( pivot ), FALSE );

	if( !pivot->private->dispose_has_run ){
		auto_reload = pivot->private->automatic_reload;
	}

	return( auto_reload );
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

static NAObject *
get_item_from_tree( const NAPivot *pivot, GList *tree, uuid_t uuid )
{
	uuid_t i_uuid_bin;
	GList *subitems, *ia;
	NAObject *found = NULL;

	for( ia = tree ; ia && !found ; ia = ia->next ){

		gchar *i_uuid = na_object_get_id( NA_OBJECT( ia->data ));
		uuid_parse( i_uuid, i_uuid_bin );
		g_free( i_uuid );

		if( !uuid_compare( uuid, i_uuid_bin )){
			found = NA_OBJECT( ia->data );
		}

		if( !found && NA_IS_OBJECT_ITEM( ia->data )){
			subitems = na_object_get_items( ia->data );
			found = get_item_from_tree( pivot, subitems, uuid );
			na_object_free_items( subitems );
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

/*
 * Note that each implementation of NAIIOProvider interface must have
 * this same type of constructor, which accepts as parameter a pointer
 * to this NAPivot object.
 * This is required because NAIIOProviders will send all their
 * notification messages to this NAPivot, letting this later redirect
 * them to appropriate NAIPivotConsumers.
 */
static void
register_io_providers( NAPivot *pivot )
{
	static const gchar *thisfn = "na_pivot_register_io_providers";
	GList *list = NULL;

	g_debug( "%s: pivot=%p", thisfn, ( void * ) pivot );
	g_return_if_fail( NA_IS_PIVOT( pivot ));
	g_return_if_fail( !pivot->private->dispose_has_run );

	list = g_list_prepend( list, na_gconf_provider_new( pivot ));

	pivot->private->providers = list;
}

/*
 * this handler is trigerred by IIOProviders when an action is changed
 * in the underlying storage subsystems
 * we don't care of updating our internal list with each and every
 * atomic modification
 * instead we wait for the end of notifications serie, and then reload
 * the whole list of actions
 */
static void
action_changed_handler( NAPivot *self, gpointer user_data  )
{
	/*static const gchar *thisfn = "na_pivot_action_changed_handler";
	g_debug( "%s: self=%p, data=%p", thisfn, self, user_data );*/

	g_return_if_fail( NA_IS_PIVOT( self ));
	g_return_if_fail( !self->private->dispose_has_run );
	g_return_if_fail( user_data );

	if( self->private->dispose_has_run ){
		return;
	}

	/* set a timeout to notify clients at the end of the serie */
	g_get_current_time( &st_last_event );
	if( !st_event_source_id ){
		st_event_source_id = g_timeout_add( st_timeout_msec, ( GSourceFunc ) on_actions_changed_timeout, self );
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
on_actions_changed_timeout( gpointer user_data )
{
	/*static const gchar *thisfn = "na_pivot_on_actions_changed_timeout";
	g_debug( "%s: pivot=%p", thisfn, user_data );*/
	GTimeVal now;
	NAPivot *pivot;
	gulong diff;
	GList *ic;

	g_return_val_if_fail( NA_IS_PIVOT( user_data ), FALSE );
	pivot = NA_PIVOT( user_data );

	g_get_current_time( &now );
	diff = time_val_diff( &now, &st_last_event );
	if( diff < st_timeout_usec ){
		return( TRUE );
	}

	if( pivot->private->automatic_reload ){
		na_pivot_reload_items( pivot );
	}

	for( ic = pivot->private->consumers ; ic ; ic = ic->next ){
		na_ipivot_consumer_notify_actions_changed( NA_IPIVOT_CONSUMER( ic->data ));
	}

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

/**
 * na_pivot_free_notify:
 * @npn: a #NAPivotNotify structure.
 *
 * Frees a #NAPivotNotify structure and its content.
 */
void
na_pivot_free_notify( NAPivotNotify *npn )
{
	if( npn ){
		if( npn->type ){
			switch( npn->type ){

				case NA_PIVOT_STR:
					g_free(( gchar * ) npn->data );
					break;

				case NA_PIVOT_BOOL:
					break;

				case NA_PIVOT_STRLIST:
					na_utils_free_string_list(( GSList * ) npn->data );
					break;

				default:
					g_debug( "na_pivot_free_notify: uuid=%s, profile=%s, parm=%s, type=%d",
							npn->uuid, npn->profile, npn->parm, npn->type );
					g_assert_not_reached();
					break;
			}
		}
		g_free( npn->uuid );
		g_free( npn->profile );
		g_free( npn->parm );
		g_free( npn );
	}
}

static void
monitor_runtime_preferences( NAPivot *pivot )
{
	GList *list = NULL;

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
