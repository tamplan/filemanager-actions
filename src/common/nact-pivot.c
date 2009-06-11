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

#include "nact-action.h"
#include "nact-gconf.h"
#include "nact-pivot.h"
#include "nact-iio-provider.h"
#include "nact-uti-lists.h"

/* private class data
 */
struct NactPivotClassPrivate {
};

/* private instance data
 */
struct NactPivotPrivate {
	gboolean  dispose_has_run;

	/* instance to be notified of an action modification
	 */
	gpointer  notified;

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

/* private instance properties
 */
enum {
	PROP_NOTIFIED = 1
};

#define PROP_NOTIFIED_STR				"to-be-notified"

/* signal definition
 */
enum {
	ACTION_CHANGED,
	LAST_SIGNAL
};

#define SIGNAL_ACTION_CHANGED_NAME		"notify_pivot_of_action_changed"

static GObjectClass *st_parent_class = NULL;
static gint          st_signals[ LAST_SIGNAL ] = { 0 };
static GTimeVal      st_last_event;
static guint         st_event_source_id = 0;
static gint          st_timeout_usec = 500000;

static GType       register_type( void );
static void        class_init( NactPivotClass *klass );
static void        instance_init( GTypeInstance *instance, gpointer klass );
static GSList     *register_interface_providers( const NactPivot *pivot );
static void        instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void        instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void        instance_dispose( GObject *object );
static void        instance_finalize( GObject *object );

static void        free_actions( GSList *list );
static void        action_changed_handler( NactPivot *pivot, gpointer user_data );
static gboolean    on_action_changed_timeout( gpointer user_data );
static gulong      time_val_diff( const GTimeVal *recent, const GTimeVal *old );

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
	object_class->set_property = instance_set_property;
	object_class->get_property = instance_get_property;

	GParamSpec *spec;
	spec = g_param_spec_pointer(
			PROP_NOTIFIED_STR,
			PROP_NOTIFIED_STR,
			"A pointer to a GObject which will receive action_changed notifications",
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_NOTIFIED, spec );

	klass->private = g_new0( NactPivotClassPrivate, 1 );

	/* see nautilus_actions_class_init for why we use this function
	 */
	st_signals[ ACTION_CHANGED ] = g_signal_new_class_handler(
				SIGNAL_ACTION_CHANGED_NAME,
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
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	g_assert( NACT_IS_PIVOT( object ));
	NactPivot *self = NACT_PIVOT( object );

	switch( property_id ){
		case PROP_NOTIFIED:
			g_value_set_pointer( value, self->private->notified );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	g_assert( NACT_IS_PIVOT( object ));
	NactPivot *self = NACT_PIVOT( object );

	switch( property_id ){
		case PROP_NOTIFIED:
			self->private->notified = g_value_get_pointer( value );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
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
		free_actions( self->private->actions );
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
 * Allocates a new NactPivot object.
 *
 * @target: the GObject which will handled Nautilus notification, and
 * should be notified when an actions is added, modified or removed in
 * one of the underlying storage subsystems.
 *
 * The target object will receive a "notify_nautilus_of_action_changed"
 * message, without any parameter.
 */
NactPivot *
nact_pivot_new( const GObject *target )
{
	return( g_object_new( NACT_PIVOT_TYPE, PROP_NOTIFIED_STR, target, NULL ));
}

/**
 * Returns the list of providers of the required interface.
 *
 * This function is called by interfaces API in order to find the
 * list of providers registered for this given interface.
 *
 * @pivot: this instance.
 *
 * @type: the type of searched interface.
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
 * Return the list of actions.
 */
GSList *
nact_pivot_get_actions( const NactPivot *pivot )
{
	g_assert( NACT_IS_PIVOT( pivot ));
	return( pivot->private->actions );
}

static void
free_actions( GSList *list )
{
	GSList *ia;
	for( ia = list ; ia ; ia = ia->next ){
		g_object_unref( NACT_ACTION( ia->data ));
	}
	g_slist_free( list );
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
action_changed_handler( NactPivot *self, gpointer user_data  )
{
	/*static const gchar *thisfn = "nact_pivot_action_changed_handler";
	g_debug( "%s: self=%p, data=%p", thisfn, self, user_data );*/

	g_assert( NACT_IS_PIVOT( self ));
	g_assert( user_data );
	if( self->private->dispose_has_run ){
		return;
	}

	/* set a timeout to notify nautilus at the end of the serie */
	g_get_current_time( &st_last_event );
	if( !st_event_source_id ){
		st_event_source_id = g_timeout_add_seconds( 1, ( GSourceFunc ) on_action_changed_timeout, self );
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
on_action_changed_timeout( gpointer user_data )
{
	/*static const gchar *thisfn = "nact_pivot_on_action_changed_timeout";
	g_debug( "%s: pivot=%p", thisfn, user_data );*/

	GTimeVal now;

	g_assert( NACT_IS_PIVOT( user_data ));
	NactPivot *pivot = NACT_PIVOT( user_data );

	g_get_current_time( &now );
	gulong diff = time_val_diff( &now, &st_last_event );
	if( diff < st_timeout_usec ){
		return( TRUE );
	}

	free_actions( pivot->private->actions );
	pivot->private->actions = nact_iio_provider_load_actions( G_OBJECT( pivot ));

	g_signal_emit_by_name( G_OBJECT( pivot->private->notified ), "notify_nautilus_of_action_changed" );
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
 * Free a NactPivotValue structure and its content.
 */
void
nact_pivot_free_notify( NactPivotNotify *npn )
{
	if( npn ){
		if( npn->type ){
			switch( npn->type ){

				case NACT_PIVOT_STR:
					g_free(( gchar * ) npn->data );
					break;

				case NACT_PIVOT_BOOL:
					break;

				case NACT_PIVOT_STRLIST:
					nactuti_free_string_list(( GSList * ) npn->data );
					break;

				default:
					g_debug( "nact_pivot_free_notify: uuid=%s, profile=%s, parm=%s, type=%d",
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
