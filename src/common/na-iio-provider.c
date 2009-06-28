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

#include "na-action.h"
#include "na-action-profile.h"
#include "na-iio-provider.h"
#include "na-pivot.h"

/* private interface data
 */
struct NAIIOProviderInterfacePrivate {
};

static GType    register_type( void );
static void     interface_base_init( NAIIOProviderInterface *klass );
static void     interface_base_finalize( NAIIOProviderInterface *klass );

static gboolean do_is_writable( NAIIOProvider *instance );
static gboolean do_is_willing_to_write( NAIIOProvider *instance, const GObject *action );

static gint     sort_actions_by_label( gconstpointer a1, gconstpointer a2 );

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
	g_debug( "%s", thisfn );

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

	GType type = g_type_register_static( G_TYPE_INTERFACE, "NAIIOProvider", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( NAIIOProviderInterface *klass )
{
	static const gchar *thisfn = "na_iio_provider_interface_base_init";
	static gboolean initialized = FALSE;

	if( !initialized ){
		g_debug( "%s: klass=%p", thisfn, klass );

		klass->private = g_new0( NAIIOProviderInterfacePrivate, 1 );

		klass->read_actions = NULL;
		klass->is_writable = do_is_writable;
		klass->is_willing_to_write = do_is_willing_to_write;
		klass->write_action = NULL;

		initialized = TRUE;
	}
}

static void
interface_base_finalize( NAIIOProviderInterface *klass )
{
	static const gchar *thisfn = "na_iio_provider_interface_base_finalize";
	static gboolean finalized = FALSE ;

	if( !finalized ){
		g_debug( "%s: klass=%p", thisfn, klass );

		g_free( klass->private );

		finalized = TRUE;
	}
}

/**
 * Loads the actions defined in the system.
 *
 * @object: the pivot object which owns the list of registered
 * interface providers.
 *
 * Returns a GSList of newly allocated NAAction objects.
 */
GSList *
na_iio_provider_read_actions( const GObject *object )
{
	static const gchar *thisfn = "na_iio_provider_read_actions";
	g_debug( "%s", thisfn );

	g_assert( NA_IS_PIVOT( object ));
	NAPivot *pivot = NA_PIVOT( object );

	GSList *actions = NULL;
	GSList *ip, *il;
	GSList *list;
	NAIIOProvider *instance;

	GSList *providers = na_pivot_get_providers( pivot, NA_IIO_PROVIDER_TYPE );

	for( ip = providers ; ip ; ip = ip->next ){

		instance = NA_IIO_PROVIDER( ip->data );
		if( NA_IIO_PROVIDER_GET_INTERFACE( instance )->read_actions ){

			list = NA_IIO_PROVIDER_GET_INTERFACE( instance )->read_actions( instance );

			for( il = list ; il ; il = il->next ){
				g_object_set_data( G_OBJECT( il->data ), "provider", instance );
			}

			actions = g_slist_concat( actions, list );
		}
	}

	actions = g_slist_sort( actions, ( GCompareFunc ) sort_actions_by_label );

#ifdef NACT_MAINTAINER_MODE
	for( ip = actions ; ip ; ip = ip->next ){
		na_object_dump( NA_OBJECT( ip->data ));
	}
#endif

	return( actions );
}

/**
 * Writes an action to a willing-to storage subsystem.
 *
 * @obj_pivot: the pivot object which owns the list of registered
 * interface providers.
 *
 * @obj_action: the action to be written.
 *
 * @message: the I/O provider can allocate and store here an error
 * message.
 *
 * Returns the IIOProvider return code.
 */
guint
na_iio_provider_write_action( const GObject *obj_pivot, const GObject *obj_action, gchar **message )
{
	static const gchar *thisfn = "na_iio_provider_write_action";
	g_debug( "%s", thisfn );

	g_assert( NA_IS_PIVOT( obj_pivot ));
	NAPivot *pivot = NA_PIVOT( obj_pivot );

	g_assert( NA_IS_ACTION( obj_action ));

	guint ret = NA_IIO_PROVIDER_NOT_WRITABLE;
	GSList *ip;
	NAIIOProvider *instance;

	GSList *providers = na_pivot_get_providers( pivot, NA_IIO_PROVIDER_TYPE );

	for( ip = providers ; ip ; ip = ip->next ){

		instance = NA_IIO_PROVIDER( ip->data );
		if( NA_IIO_PROVIDER_GET_INTERFACE( instance )->write_action ){

			ret = NA_IIO_PROVIDER_GET_INTERFACE( instance )->write_action( instance, obj_action, message );
			if( ret == NA_IIO_PROVIDER_WRITE_OK || ret == NA_IIO_PROVIDER_WRITE_ERROR ){
				break;
			}
		}
	}

	return( ret );
}

/**
 * Deletes an action from the storage subsystem.
 *
 * @obj_pivot: the pivot object which owns the list of registered
 * interface providers.
 *
 * @obj_action: the action to be deleted.
 *
 * @message: the I/O provider can allocate and store here an error
 * message.
 *
 * Returns the IIOProvider return code.
 */
guint
na_iio_provider_delete_action( const GObject *obj_pivot, const GObject *obj_action, gchar **message )
{
	static const gchar *thisfn = "na_iio_provider_delete_action";
	g_debug( "%s: pivot=%p, action=%p, message=%p", thisfn, obj_pivot, obj_action, message );

	g_assert( NA_IS_ACTION( obj_action ));
	guint ret = NA_IIO_PROVIDER_NOT_WRITABLE;

	NAIIOProvider *instance = NA_IIO_PROVIDER( na_action_get_provider( NA_ACTION( obj_action )));
	if( instance ){
		g_assert( NA_IS_IIO_PROVIDER( instance ));

		if( NA_IIO_PROVIDER_GET_INTERFACE( instance )->delete_action ){
			ret = NA_IIO_PROVIDER_GET_INTERFACE( instance )->delete_action( instance, obj_action, message );
		}
	}

	return( ret );
}

static gboolean
do_is_writable( NAIIOProvider *instance )
{
	return( FALSE );
}

static gboolean
do_is_willing_to_write( NAIIOProvider *instance, const GObject *action )
{
	return( FALSE );
}

static gint
sort_actions_by_label( gconstpointer a1, gconstpointer a2 )
{
	NAAction *action1 = NA_ACTION( a1 );
	gchar *label1 = na_action_get_label( action1 );

	NAAction *action2 = NA_ACTION( a2 );
	gchar *label2 = na_action_get_label( action2 );

	gint ret = g_utf8_collate( label1, label2 );

	g_free( label1 );
	g_free( label2 );

	return( ret );
}
