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

#include "na-action.h"
#include "na-gconf.h"
#include "na-pivot.h"
#include "na-iio-provider.h"
#include "na-ipivot-consumer.h"
#include "na-utils.h"

/* private class data
 */
struct NAPivotClassPrivate {
};

/* private instance data
 */
struct NAPivotPrivate {
	gboolean dispose_has_run;

	/* list of instances to be notified of an action modification
	 * these are called 'consumers' of NAPivot
	 */
	GSList  *notified;

	/* list of interface providers
	 * needs to be in the instance rather than in the class to be able
	 * to pass NAPivot object to the IO provider, so that the later
	 * is able to have access to the former (and its list of actions)
	 */
	GSList  *providers;

	/* list of actions
	 */
	GSList  *actions;
	gboolean reload;
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

static GType    register_type( void );
static void     class_init( NAPivotClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static GSList  *register_interface_providers( const NAPivot *pivot );
static void     instance_dispose( GObject *object );
static void     instance_finalize( GObject *object );

static void     free_consumers( GSList *list );
static void     action_changed_handler( NAPivot *pivot, gpointer user_data );
static gboolean on_actions_changed_timeout( gpointer user_data );
static gulong   time_val_diff( const GTimeVal *recent, const GTimeVal *old );

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

	return( g_type_register_static( G_TYPE_OBJECT, "NAPivot", &info, 0 ));
}

static void
class_init( NAPivotClass *klass )
{
	static const gchar *thisfn = "na_pivot_class_init";
	g_debug( "%s: klass=%p", thisfn, klass );

	st_parent_class = g_type_class_peek_parent( klass );

	GObjectClass *object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NAPivotClassPrivate, 1 );

	/* register the signal and its default handler
	 * this signal should be sent by the IIOProvider when an actions
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
				G_TYPE_POINTER
	);
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "na_pivot_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );

	g_assert( NA_IS_PIVOT( instance ));
	NAPivot* self = NA_PIVOT( instance );

	self->private = g_new0( NAPivotPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->notified = NULL;
	self->private->providers = register_interface_providers( self );
	self->private->actions = na_iio_provider_read_actions( self );
	self->private->reload = TRUE;
}

static GSList *
register_interface_providers( const NAPivot *pivot )
{
	static const gchar *thisfn = "na_pivot_register_interface_providers";
	g_debug( "%s", thisfn );

	GSList *list = NULL;

	list = g_slist_prepend( list, na_gconf_new( G_OBJECT( pivot )));

	return( list );
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "na_pivot_instance_dispose";
	g_debug( "%s: object=%p", thisfn, object );

	g_assert( NA_IS_PIVOT( object ));
	NAPivot *self = NA_PIVOT( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* release list of containers to be notified */
		free_consumers( self->private->notified );

		/* release list of actions */
		na_pivot_free_actions( self->private->actions );
		self->private->actions = NULL;

		/* chain up to the parent class */
		G_OBJECT_CLASS( st_parent_class )->dispose( object );
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "na_pivot_instance_finalize";
	g_debug( "%s: object=%p", thisfn, object );

	g_assert( NA_IS_PIVOT( object ));
	NAPivot *self = ( NAPivot * ) object;

	/* release the interface providers */
	GSList *ip;
	for( ip = self->private->providers ; ip ; ip = ip->next ){
		g_object_unref( G_OBJECT( ip->data ));
	}
	g_slist_free( self->private->providers );
	self->private->providers = NULL;

	g_free( self->private );

	/* chain call to parent class */
	if((( GObjectClass * ) st_parent_class )->finalize ){
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
 * The target object will receive a "notify_nautilus_of_action_changed"
 * message, without any parameter.
 */
NAPivot *
na_pivot_new( const GObject *target )
{
	NAPivot *pivot = g_object_new( NA_PIVOT_TYPE, NULL );

	if( target ){
		na_pivot_add_consumer( pivot, target );
	}

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

	GSList *it;
	int i;
	g_debug( "%s:  notified=%p (%d elts)", thisfn, pivot->private->notified, g_slist_length( pivot->private->notified ));
	g_debug( "%s: providers=%p (%d elts)", thisfn, pivot->private->providers, g_slist_length( pivot->private->providers ));
	g_debug( "%s:   actions=%p (%d elts)", thisfn, pivot->private->actions, g_slist_length( pivot->private->actions ));
	for( it = pivot->private->actions, i = 0 ; it ; it = it->next ){
		g_debug( "%s:   [%d]: %p", thisfn, i++, it->data );
	}
}

/**
 * na_pivot_get_providers:
 * @pivot: this #NAPivot instance.
 * @type: the type of searched interface.
 * For now, we only have NA_IIO_PROVIDER_TYPE interfaces.
 *
 * Returns the list of providers of the required interface.
 *
 * This function is called by interfaces API in order to find the
 * list of providers registered for this given interface.
 *
 * Returns: the list of providers of the required interface.
 * This list should be na_pivot_free_providers().
 */
GSList *
na_pivot_get_providers( const NAPivot *pivot, GType type )
{
	static const gchar *thisfn = "na_pivot_get_providers";
	g_debug( "%s: pivot=%p", thisfn, pivot );

	g_assert( NA_IS_PIVOT( pivot ));

	GSList *list = NULL;
	GSList *ip;
	for( ip = pivot->private->providers ; ip ; ip = ip->next ){
		if( G_TYPE_CHECK_INSTANCE_TYPE( G_OBJECT( ip->data ), type )){
			list = g_slist_prepend( list, ip->data );
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
na_pivot_free_providers( GSList *providers )
{
	g_slist_free( providers );
}

/**
 * na_pivot_get_actions:
 * @pivot: this #NAPivot instance.
 *
 * Returns the list of actions.
 *
 * Returns: the list of #NAAction actions.
 * The returned list is owned by this #NAPivot object, and should not
 * be g_free(), nor g_object_unref() by the caller.
 */
GSList *
na_pivot_get_actions( const NAPivot *pivot )
{
	g_assert( NA_IS_PIVOT( pivot ));

	return( pivot->private->actions );
}

/**
 * na_pivot_reload_actions:
 * @pivot: this #NAPivot instance.
 *
 * Reloads the list of actions from I/O providers.
 */
void
na_pivot_reload_actions( NAPivot *pivot )
{
	g_assert( NA_IS_PIVOT( pivot ));

	if( pivot->private->actions ){
		na_pivot_free_actions( pivot->private->actions );
	}

	pivot->private->actions = na_iio_provider_read_actions( pivot );
}

/**
 * na_pivot_get_duplicate_actions:
 * @pivot: this #NAPivot instance.
 *
 * Returns an exact copy of the current list of actions.
 *
 * Returns: a #GSList of #NAAction actions.
 * The caller should na_pivot_free_actions() after usage.
 */
GSList *
na_pivot_get_duplicate_actions( const NAPivot *pivot )
{
	g_assert( NA_IS_PIVOT( pivot ));

	GSList *list = NULL;
	GSList *ia;

	for( ia = pivot->private->actions ; ia ; ia = ia->next ){
		list = g_slist_prepend( list, na_object_duplicate( NA_OBJECT( ia->data )));
	}

	return( list );
}

/**
 * na_pivot_free_actions:
 * @list: a #GSList of #NAActions to be freed.
 *
 * Frees a list of actions.
 */
void
na_pivot_free_actions( GSList *actions )
{
	GSList *ia;
	for( ia = actions ; ia ; ia = ia->next ){
		g_object_unref( NA_ACTION( ia->data ));
	}
	g_slist_free( actions );
}

/**
 * na_pivot_add_action:
 * @pivot: this #NAPivot instance.
 * @action: the #NAAction to be added to the list.
 *
 * Adds a new #NAAction to the list of actions.
 *
 * We take the provided pointer. The provided #NAAction should so not
 * be g_object_unref() by the caller.
 */
void
na_pivot_add_action( NAPivot *pivot, const NAAction *action )
{
	g_assert( NA_IS_PIVOT( pivot ));

	pivot->private->actions = g_slist_prepend( pivot->private->actions, ( gpointer ) action );
}

/**
 * na_pivot_remove_action:
 * @pivot: this #NAPivot instance.
 * @action: the #NAAction to be removed to the list.
 *
 * Removes a #NAAction from the list of actions.
 *
 * Note that #NAPivot also g_object_unref() the removed #NAAction.
 */
void
na_pivot_remove_action( NAPivot *pivot, NAAction *action )
{
	g_assert( NA_IS_PIVOT( pivot ));

	pivot->private->actions = g_slist_remove( pivot->private->actions, ( gconstpointer ) action );
	g_object_unref( action );
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
NAAction *
na_pivot_get_action( const NAPivot *pivot, const gchar *uuid )
{
	uuid_t uua, i_uub;

	g_assert( NA_IS_PIVOT( pivot ));
	if( !uuid || !strlen( uuid )){
		return( NULL );
	}

	uuid_parse( uuid, uua );

	GSList *ia;
	for( ia = pivot->private->actions ; ia ; ia = ia->next ){

		gchar *i_uuid = na_action_get_uuid( NA_ACTION( ia->data ));
		uuid_parse( i_uuid, i_uub );
		g_free( i_uuid );

		if( !uuid_compare( uua, i_uub )){
			return( NA_ACTION( ia->data ));
		}
	}

	return( NULL );
}

/**
 * na_pivot_write_action:
 * @pivot: this #NAPivot instance.
 * @action: a #NAAction action to be written by the storage subsystem.
 * @message: the I/O provider can allocate and store here an error
 * message.
 *
 * Writes an action.
 *
 * Returns: the #NAIIOProvider return code.
 */
guint
na_pivot_write_action( const NAPivot *pivot, NAAction *action, gchar **message )
{
	g_assert( NA_IS_PIVOT( pivot ));
	g_assert( NA_IS_ACTION( action ));
	g_assert( message );
	return( na_iio_provider_write_action( pivot, action, message ));
}

/**
 * na_pivot_delete_action:
 * @pivot: this #NAPivot instance.
 * @action: a #NAAction action to be deleted from the storage
 * subsystem.
 * @message: the I/O provider can allocate and store here an error
 * message.
 *
 * Deletes an action from the I/O storage subsystem.
 *
 * Returns: the #NAIIOProvider return code.
 */
guint
na_pivot_delete_action( const NAPivot *pivot, const NAAction *action, gchar **message )
{
	g_assert( NA_IS_PIVOT( pivot ));
	g_assert( NA_IS_ACTION( action ));
	g_assert( message );
	return( na_iio_provider_delete_action( pivot, action, message ));
}

/**
 * na_pivot_add_consumer:
 * @pivot: this #NAPivot instance.
 * @consumer: a #GObject which wishes be notified of any modification
 * of an action in any of the underlying I/O storage subsystems.
 *
 * Registers a new consumer to be notified of an action modification.
 */
void
na_pivot_add_consumer( NAPivot *pivot, const GObject *consumer )
{
	static const gchar *thisfn = "na_pivot_add_consumer";
	g_debug( "%s: pivot=%p, consumer=%p", thisfn, pivot, consumer );

	g_assert( NA_IS_PIVOT( pivot ));
	g_assert( G_IS_OBJECT( consumer ));

	pivot->private->notified = g_slist_prepend( pivot->private->notified, ( gpointer ) consumer );
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
	g_assert( NA_IS_PIVOT( pivot ));

	return( pivot->private->reload );
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
	g_assert( NA_IS_PIVOT( pivot ));

	pivot->private->reload = reload;
}

static void
free_consumers( GSList *consumers )
{
	GSList *ic;
	for( ic = consumers ; ic ; ic = ic->next )
		;
	g_slist_free( consumers );
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

	g_assert( NA_IS_PIVOT( self ));
	g_assert( user_data );
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

	g_assert( NA_IS_PIVOT( user_data ));
	const NAPivot *pivot = NA_PIVOT( user_data );

	g_get_current_time( &now );
	gulong diff = time_val_diff( &now, &st_last_event );
	if( diff < st_timeout_usec ){
		return( TRUE );
	}

	if( pivot->private->reload ){
		na_pivot_free_actions( pivot->private->actions );
		pivot->private->actions = na_iio_provider_read_actions( pivot );
	}

	GSList *ic;
	for( ic = pivot->private->notified ; ic ; ic = ic->next ){
		na_ipivot_consumer_notify( NA_IPIVOT_CONSUMER( ic->data ));
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
 * Free a NAPivotValue structure and its content.
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
