/*
 * Nautilus Pivots
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

#include "nact-action.h"
#include "nact-gconf.h"
#include "nact-pivot.h"
#include "nact-iio-provider.h"
#include "uti-lists.h"

/* action_changed_cb send events which are stacked in a static GSList
 * we so hope to optimize updating the global list of actions
 */
typedef struct {
	gchar          *uuid;
	gchar          *parm;
	NactPivotValue *value;
}
	stackItem;

struct NactPivotPrivate {
	gboolean  dispose_has_run;

	/* list of interface providers
	 * needs to be in the instance rather than in the class to be able
	 * to pass NactPivot object to the IO provider, so that the later
	 * is able to have access to the former (and its list of actions)
	 */
	GSList   *providers;

	/* list of actions
	 */
	GSList   *actions;
};

struct NactPivotClassPrivate {
};

static GObjectClass *st_parent_class = NULL;
static GSList       *st_stack_events = NULL;
static GTimeVal      st_last_event;
static guint         st_event_source_id = 0;

static GType       register_type( void );
static void        class_init( NactPivotClass *klass );
static void        instance_init( GTypeInstance *instance, gpointer klass );
static GSList     *register_interface_providers( const NactPivot *pivot );
static void        instance_dispose( GObject *object );
static void        instance_finalize( GObject *object );

static void        check_for_remove_action( NactPivot *pivot, NactAction *action );
static gint        cmp_events( gconstpointer a, gconstpointer b );
static void        free_stack_events( GSList *stack );
static gboolean    on_action_changed_timeout( gpointer user_data );
static stackItem  *stack_item_new( const gchar *uuid, const gchar *parm, const NactPivotValue *value );
static void        stack_item_free( stackItem *item );
static gulong      time_val_diff( const GTimeVal *recent, const GTimeVal *old );
static void        update_actions( NactPivot *pivot, GSList *stack );
static NactAction *get_action( GSList *list, const gchar *uuid );

NactPivot *
nact_pivot_new( void )
{
	return( g_object_new( NACT_PIVOT_TYPE, NULL ));
}

GType
nact_pivot_get_type( void )
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
		sizeof( NactPivotClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactPivot ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	return( g_type_register_static( G_TYPE_OBJECT, "NactPivot", &info, 0 ));
}

static void
class_init( NactPivotClass *klass )
{
	static const gchar *thisfn = "nact_pivot_class_init";
	g_debug( "%s: klass=%p", thisfn, klass );

	st_parent_class = g_type_class_peek_parent( klass );

	GObjectClass *object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactPivotClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_pivot_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );

	g_assert( NACT_IS_PIVOT( instance ));
	NactPivot* self = NACT_PIVOT( instance );

	self->private = g_new0( NactPivotPrivate, 1 );
	self->private->dispose_has_run = FALSE;
	self->private->providers = register_interface_providers( self );
	self->private->actions = nact_iio_provider_load_actions( G_OBJECT( self ));
}

static GSList *
register_interface_providers( const NactPivot *pivot )
{
	static const gchar *thisfn = "nact_pivot_register_interface_providers";
	g_debug( "%s", thisfn );

	GSList *list = NULL;

	list = g_slist_prepend( list, nact_gconf_new( G_OBJECT( pivot )));

	return( list );
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "nact_pivot_instance_dispose";
	g_debug( "%s: object=%p", thisfn, object );

	g_assert( NACT_IS_PIVOT( object ));
	NactPivot *self = NACT_PIVOT( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* release list of actions */
		GSList *ia;
		for( ia = self->private->actions ; ia ; ia = ia->next ){
			g_object_unref( G_OBJECT( ia->data ));
		}
		g_slist_free( self->private->actions );
		self->private->actions = NULL;

		/* chain up to the parent class */
		G_OBJECT_CLASS( st_parent_class )->dispose( object );
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "nact_pivot_instance_finalize";
	g_debug( "%s: object=%p", thisfn, object );

	g_assert( NACT_IS_PIVOT( object ));
	NactPivot *self = ( NactPivot * ) object;

	/* release the interface providers */
	GSList *ip;
	for( ip = self->private->providers ; ip ; ip = ip->next ){
		g_object_unref( G_OBJECT( ip->data ));
	}
	g_slist_free( self->private->providers );
	self->private->providers = NULL;

	/* chain call to parent class */
	if((( GObjectClass * ) st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * Returns the list of providers of the required interface.
 */
GSList *
nact_pivot_get_providers( const NactPivot *pivot, GType type )
{
	static const gchar *thisfn = "nact_pivot_get_providers";
	g_debug( "%s", thisfn );

	g_assert( NACT_IS_PIVOT( pivot ));

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
 * This function should be called when a storage subsystem detects that
 * a stored action has changed.
 * As a Nautilus extension, NactPivot will take care of updating menu
 * characteristics accordingly.
 *
 * @pivot: the NactPivot object.
 *
 * @uuid: identifiant of the action.
 *
 * @parm: the parameter path (e.g. "profile-main/path")
 *
 * @value: the new value as a NactPivotValue structure ; do not free it
 * here as it is the responsability of the allocater subsystem.
 *
 * Depending of the sort of update which occurs, we may receive many
 * notifications for the same action. We so stack the notifications and
 * start a one sec. timeout before updating the whole stack.
 */
void
nact_pivot_on_action_changed( NactPivot *pivot, const gchar *uuid, const gchar *parm, NactPivotValue *value )
{
	static const gchar *thisfn = "nact_pivot_on_action_changed";
	g_debug( "%s: pivot=%p, uuid='%s', parm='%s', value=%p", thisfn, pivot, uuid, parm, value );

	stackItem *item = ( stackItem * ) stack_item_new( uuid, parm, value );
	st_stack_events = g_slist_prepend( st_stack_events, item );

	g_get_current_time( &st_last_event );

	if( !st_event_source_id ){
		st_event_source_id = g_timeout_add_seconds( 1, ( GSourceFunc ) on_action_changed_timeout, pivot );
	}
}

/**
 * Duplicate a NactPivotValue structure and its content.
 */
NactPivotValue *
nact_pivot_duplicate_pivot_value( const NactPivotValue *value )
{
	if( !value ){
		return(( NactPivotValue * ) NULL );
	}

	NactPivotValue *newvalue = g_new0( NactPivotValue, 1 );

	switch( value->type ){

		case NACT_PIVOT_STR:
			newvalue->data = g_strdup(( gchar * ) value->data );
			break;

		case NACT_PIVOT_BOOL:
			newvalue->data = value->data;
			break;

		case NACT_PIVOT_STRLIST:
			newvalue->data = nactuti_duplicate_string_list(( GSList * ) value->data );
			break;

		default:
			g_assert_not_reached();
			break;
	}

	return( newvalue );
}

/**
 * Free a NactPivotValue structure and its content.
 */
void
nact_pivot_free_pivot_value( NactPivotValue *value )
{
	if( value ){
		switch( value->type ){

			case NACT_PIVOT_STR:
				g_free(( gchar * ) value->data );
				break;

			case NACT_PIVOT_BOOL:
				break;

			case NACT_PIVOT_STRLIST:
				nactuti_free_string_list(( GSList * ) value->data );
				break;

			default:
				g_assert_not_reached();
				break;
		}
		g_free( value );
	}
}

static void
check_for_remove_action( NactPivot *pivot, NactAction *action )
{
	static const gchar *thisfn ="check_for_remove_action";

	g_assert( NACT_IS_PIVOT( pivot ));
	g_assert( NACT_IS_ACTION( action ));

	if( nact_action_is_empty( action )){
		g_debug( "%s: removing action %p", thisfn, action );
		pivot->private->actions = g_slist_remove( pivot->private->actions, action );
	}
}

/*
 * comparaison function between two stack items
 */
static gint
cmp_events( gconstpointer a, gconstpointer b )
{
	stackItem *sa = ( stackItem * ) a;
	stackItem *sb = ( stackItem * ) b;
	return( g_strcmp0( sa->uuid, sb->uuid ));
}

static void
free_stack_events( GSList *stack )
{
	GSList *is;
	for( is = stack ; is ; is = is->next ){
		stack_item_free(( stackItem * ) is->data );
	}
	g_slist_free( stack );
}

/*
 * this timer is set when we receive the first event of a serie
 * we continue to loop until last event is at least one half of a
 * second old
 *
 * there is no race condition here as we are not multithreaded
 */
static gboolean
on_action_changed_timeout( gpointer user_data )
{
	static const gchar *thisfn = "on_action_changed_timeout";
	GTimeVal now;

	g_assert( NACT_IS_PIVOT( user_data ));

	g_get_current_time( &now );
	gulong diff = time_val_diff( &now, &st_last_event );
	if( diff < 500000 ){
		return( TRUE );
	}

	g_debug( "%s: treating stack with %d events", thisfn, g_slist_length( st_stack_events ));
	update_actions( NACT_PIVOT( user_data ), st_stack_events );

	st_event_source_id = 0;
	free_stack_events( st_stack_events );
	st_stack_events = NULL;
	return( FALSE );
}

static stackItem *
stack_item_new( const gchar *uuid, const gchar *parm, const NactPivotValue *value )
{
	stackItem *item = g_new0( stackItem, 1 );
	item->uuid = g_strdup( uuid );
	item->parm = g_strdup( parm );
	item->value = nact_pivot_duplicate_pivot_value( value );
	return( item );
}

static void
stack_item_free( stackItem *item )
{
	g_free( item->uuid );
	g_free( item->parm );
	nact_pivot_free_pivot_value( item->value );
	g_free( item );
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

/*
 * iterate through the list of events, sorted by action id
 * on new action, add it to the list, creating the object
 * when all events have been treated, check to see if the action was
 * actually removed (all fields, including key, are blank or null)
 *
 * remove = key + parm=null and value=null
 */
static void
update_actions( NactPivot *pivot, GSList *stack )
{
	GSList *it;
	NactAction *action = NULL;
	gchar *previd = NULL;

	GSList *sorted = g_slist_sort( stack, cmp_events );

	for( it = sorted ; it ; it = it->next ){
		stackItem *item = ( stackItem * ) it->data;

		if( action && g_strcmp0( previd, item->uuid )){
			g_assert( action && NACT_IS_ACTION( action ));
			check_for_remove_action( pivot, action );
			g_free( previd );
		}
		previd = g_strdup( item->uuid );
		action = get_action( pivot->private->actions, item->uuid );

		if( action ){
			nact_action_update( action, item->parm, item->value );
		} else {
			action = nact_action_create( item->uuid, item->parm, item->value );
			pivot->private->actions = g_slist_prepend( pivot->private->actions, action );
		}
	}
	if( action ){
		g_assert( action && NACT_IS_ACTION( action ));
		check_for_remove_action( pivot, action );
		g_free( previd );
	}
}

static NactAction *
get_action( GSList *list, const gchar *uuid )
{
	NactAction *found = NULL;
	GSList *ia;
	for( ia = list ; ia && !found ; ia = ia->next ){
		NactAction *action = ( NactAction * ) ia->data;
		gchar *id = nact_action_get_uuid( action );
		if( !g_strcmp0( id, uuid )){
			found = action;
		}
		g_free( id );
	}
	return( found );
}

/**
 * Returns the searched NactAction, or NULL.
 */
GObject *
nact_pivot_get_action( NactPivot *pivot, const gchar *uuid )
{
	g_assert( NACT_IS_PIVOT( pivot ));
	return( G_OBJECT( get_action( pivot->private->actions, uuid )));
}
