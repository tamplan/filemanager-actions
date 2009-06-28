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

#include <glib.h>

#include <common/na-pivot.h>

#include "nact-application.h"
#include "nact-window.h"

/* private class data
 */
struct NactWindowClassPrivate {
};

/* private instance data
 */
struct NactWindowPrivate {
	gboolean dispose_has_run;
	GSList  *signals;
};

/* connected signal, to be disconnected at NactWindow dispose
 */
typedef struct {
	gpointer instance;
	gulong   handler_id;
}
	NactWindowRecordedSignal;

static GObjectClass *st_parent_class = NULL;

static GType register_type( void );
static void  class_init( NactWindowClass *klass );
static void  instance_init( GTypeInstance *instance, gpointer klass );
static void  instance_dispose( GObject *application );
static void  instance_finalize( GObject *application );

GType
nact_window_get_type( void )
{
	static GType window_type = 0;

	if( !window_type ){
		window_type = register_type();
	}

	return( window_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "nact_window_register_type";
	g_debug( "%s", thisfn );

	g_type_init();

	static GTypeInfo info = {
		sizeof( NactWindowClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactWindow ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	return( g_type_register_static( BASE_WINDOW_TYPE, "NactWindow", &info, 0 ));
}

static void
class_init( NactWindowClass *klass )
{
	static const gchar *thisfn = "nact_window_class_init";
	g_debug( "%s: klass=%p", thisfn, klass );

	st_parent_class = g_type_class_peek_parent( klass );

	GObjectClass *object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactWindowClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_window_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );

	g_assert( NACT_IS_WINDOW( instance ));
	NactWindow *self = NACT_WINDOW( instance );

	self->private = g_new0( NactWindowPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->signals = NULL;
}

static void
instance_dispose( GObject *window )
{
	static const gchar *thisfn = "nact_window_instance_dispose";
	g_debug( "%s: window=%p", thisfn, window );

	g_assert( NACT_IS_WINDOW( window ));
	NactWindow *self = NACT_WINDOW( window );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		GSList *is;
		for( is = self->private->signals ; is ; is = is->next ){
			NactWindowRecordedSignal *str = ( NactWindowRecordedSignal * ) is->data;
			g_signal_handler_disconnect( str->instance, str->handler_id );
			g_debug( "%s: disconnecting signal handler %p:%lu", thisfn, str->instance, str->handler_id );
			g_free( str );
		}
		g_slist_free( self->private->signals );

		/* chain up to the parent class */
		G_OBJECT_CLASS( st_parent_class )->dispose( window );
	}
}

static void
instance_finalize( GObject *window )
{
	static const gchar *thisfn = "nact_window_instance_finalize";
	g_debug( "%s: window=%p", thisfn, window );

	g_assert( NACT_IS_WINDOW( window ));
	NactWindow *self = ( NactWindow * ) window;

	g_free( self->private );

	/* chain call to parent class */
	if( st_parent_class->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( window );
	}
}

/**
 * Returns a pointer to the list of actions.
 */
GObject *
nact_window_get_pivot( NactWindow *window )
{
	NactApplication *application;
	g_object_get( G_OBJECT( window ), PROP_WINDOW_APPLICATION_STR, &application, NULL );
	g_return_val_if_fail( NACT_IS_APPLICATION( application ), NULL );

	GObject *pivot = nact_application_get_pivot( application );
	g_return_val_if_fail( NA_IS_PIVOT( pivot ), NULL );

	return( pivot );
}

/**
 * Returns a pointer to the specified action.
 */
GObject *
nact_window_get_action( NactWindow *window, const gchar *uuid )
{
	NAPivot *pivot = NA_PIVOT( nact_window_get_pivot( window ));
	g_return_val_if_fail( NA_IS_PIVOT( pivot ), NULL );

	GObject *action = na_pivot_get_action( pivot, uuid );
	return( action );
}

/**
 * Returns a pointer to the list of actions.
 */
GSList *
nact_window_get_actions( NactWindow *window )
{
	NAPivot *pivot = NA_PIVOT( nact_window_get_pivot( window ));
	g_return_val_if_fail( NA_IS_PIVOT( pivot ), NULL );

	GSList *actions = na_pivot_get_actions( pivot );
	return( actions );
}

/**
 * Records a connected signal, to be disconnected at NactWindow dispose.
 */
void
nact_window_on_signal_connected( NactWindow *window, gpointer instance, gulong handler_id )
{
	static const gchar *thisfn = "nact_window_on_signal_connected";
	g_debug( "%s: window=%p, instance=%p, handler_id=%lu", thisfn, window, instance, handler_id );

	NactWindowRecordedSignal *str = g_new0( NactWindowRecordedSignal, 1 );
	str->instance = instance;
	str->handler_id = handler_id;
	window->private->signals = g_slist_prepend( window->private->signals, str );
}
