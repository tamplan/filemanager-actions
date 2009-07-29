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

#include "na-ipivot-consumer.h"

/* private interface data
 */
struct NAIPivotConsumerInterfacePrivate {
};

static GType register_type( void );
static void  interface_base_init( NAIPivotConsumerInterface *klass );
static void  interface_base_finalize( NAIPivotConsumerInterface *klass );

/*static void  do_actions_changed( NAIPivotConsumer *instance, gpointer user_data );*/

/**
 * Registers the GType of this interface.
 */
GType
na_ipivot_consumer_get_type( void )
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
	static const gchar *thisfn = "na_ipivot_consumer_register_type";
	g_debug( "%s", thisfn );

	static const GTypeInfo info = {
		sizeof( NAIPivotConsumerInterface ),
		( GBaseInitFunc ) interface_base_init,
		( GBaseFinalizeFunc ) interface_base_finalize,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL
	};

	GType type = g_type_register_static( G_TYPE_INTERFACE, "NAIPivotConsumer", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( NAIPivotConsumerInterface *klass )
{
	static const gchar *thisfn = "na_ipivot_consumer_interface_base_init";
	static gboolean initialized = FALSE;

	if( !initialized ){
		g_debug( "%s: klass=%p", thisfn, klass );

		klass->private = g_new0( NAIPivotConsumerInterfacePrivate, 1 );

		klass->on_actions_changed = NULL /*do_actions_changed*/;

		initialized = TRUE;
	}
}

static void
interface_base_finalize( NAIPivotConsumerInterface *klass )
{
	static const gchar *thisfn = "na_ipivot_consumer_interface_base_finalize";
	static gboolean finalized = FALSE ;

	if( !finalized ){
		g_debug( "%s: klass=%p", thisfn, klass );

		g_free( klass->private );

		finalized = TRUE;
	}
}

/**
 * na_ipivot_consumer_notify:
 * @instance: the #NAIPivotConsumer instance to be notified of the end
 * of the modifications.
 *
 * Notifies the consumers that the actions have been modified.
 */
void na_ipivot_consumer_notify( NAIPivotConsumer *instance )
{
	static const gchar *thisfn = "na_ipivot_consumer_notify";
	g_debug( "%s: instance=%p", thisfn, instance );

	if( NA_IPIVOT_CONSUMER_GET_INTERFACE( instance )->on_actions_changed ){
		NA_IPIVOT_CONSUMER_GET_INTERFACE( instance )->on_actions_changed( instance, NULL );
	}
}

/*static void
do_actions_changed( NAIPivotConsumer *instance, gpointer user_data )
{
	static const gchar *thisfn = "na_ipivot_consumer_do_actions_changed";
	g_debug( "%s: instance=%p, user_data=%p", thisfn, instance, user_data );
}*/
