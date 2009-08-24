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

#include "na-action.h"
#include "na-iio-provider.h"

/* private interface data
 */
struct NAIIOProviderInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

static GType    register_type( void );
static void     interface_base_init( NAIIOProviderInterface *klass );
static void     interface_base_finalize( NAIIOProviderInterface *klass );

static gboolean do_is_willing_to_write( const NAIIOProvider *instance );
static gboolean do_is_writable( const NAIIOProvider *instance, const NAAction *action );
static guint    write_action( const NAIIOProvider *instance, NAAction *action, gchar **message );

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
	GType type;

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

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_INTERFACE, "NAIIOProvider", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( NAIIOProviderInterface *klass )
{
	static const gchar *thisfn = "na_iio_provider_interface_base_init";
	static gboolean initialized = FALSE;

	if( !initialized ){
		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NAIIOProviderInterfacePrivate, 1 );

		klass->read_actions = NULL;
		klass->is_willing_to_write = do_is_willing_to_write;
		klass->is_writable = do_is_writable;
		klass->write_action = NULL;
		klass->delete_action = NULL;

		initialized = TRUE;
	}
}

static void
interface_base_finalize( NAIIOProviderInterface *klass )
{
	static const gchar *thisfn = "na_iio_provider_interface_base_finalize";
	static gboolean finalized = FALSE ;

	if( !finalized ){
		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );

		finalized = TRUE;
	}
}

/**
 * na_iio_provider_read_actions:
 * @pivot: the #NAPivot object which owns the list of registered I/O
 * storage providers.
 *
 * Loads the actions from storage subsystems.
 *
 * Returns: a #GSList of newly allocated #NAAction objects.
 *
 * na_iio_provider_read_actions() loads the list of #NAAction from each
 * registered I/O storage provider, and takes care of concatenating
 * them into the returned global list.
 */
GSList *
na_iio_provider_read_actions( const NAPivot *pivot )
{
	static const gchar *thisfn = "na_iio_provider_read_actions";
	GSList *actions = NULL;
	GSList *providers, *ip, *list, *ia;
	NAIIOProvider *instance;

	g_debug( "%s: pivot=%p", thisfn, ( void * ) pivot );
	g_assert( NA_IS_PIVOT( pivot ));

	providers = na_pivot_get_providers( pivot, NA_IIO_PROVIDER_TYPE );

	for( ip = providers ; ip ; ip = ip->next ){

		instance = NA_IIO_PROVIDER( ip->data );
		if( NA_IIO_PROVIDER_GET_INTERFACE( instance )->read_actions ){

			list = NA_IIO_PROVIDER_GET_INTERFACE( instance )->read_actions( instance );

			for( ia = list ; ia ; ia = ia->next ){

				na_action_set_provider( NA_ACTION( ia->data ), instance );

				na_object_dump( NA_OBJECT( ia->data ));
			}

			actions = g_slist_concat( actions, list );
		}
	}

	return( actions );
}

/**
 * na_iio_provider_write_action:
 * @pivot: the #NAPivot object which owns the list of registered I/O
 * storage providers. if NULL, @action must already have registered
 * its own provider.
 * @action: the #NAAction action to be written.
 * @message: the I/O provider can allocate and store here an error
 * message.
 *
 * Writes an action to a willing-to storage subsystem.
 *
 * Returns: the NAIIOProvider return code.
 */
guint
na_iio_provider_write_action( const NAPivot *pivot, NAAction *action, gchar **message )
{
	static const gchar *thisfn = "na_iio_provider_write_action";
	guint ret;
	NAIIOProvider *instance;
	GSList *providers, *ip;

	g_debug( "%s: pivot=%p, action=%p, message=%p",
			thisfn, ( void * ) pivot, ( void * ) action, ( void * ) message );
	g_assert( NA_IS_PIVOT( pivot ) || !pivot );
	g_assert( NA_IS_ACTION( action ));

	ret = NA_IIO_PROVIDER_NOT_WRITABLE;

	/* try to write to the original provider of the action
	 */
	instance = NA_IIO_PROVIDER( na_action_get_provider( action ));

	if( instance ){
		ret = write_action( instance, action, message );
	}

	if( ret == NA_IIO_PROVIDER_NOT_WILLING_TO_WRITE || ret == NA_IIO_PROVIDER_NOT_WRITABLE ){
		instance = NULL;
	}

	/* else, search for a provider which is willing to write the action
	 */
	if( !instance && pivot ){
		providers = na_pivot_get_providers( pivot, NA_IIO_PROVIDER_TYPE );
		for( ip = providers ; ip ; ip = ip->next ){

			instance = NA_IIO_PROVIDER( ip->data );
			ret = write_action( instance, action, message );
			if( ret == NA_IIO_PROVIDER_WRITE_OK || ret == NA_IIO_PROVIDER_WRITE_ERROR ){
				break;
			}
		}
	}

	return( ret );
}

/**
 * na_iio_provider_delete_action:
 * @pivot: the #NAPivot object which owns the list of registered I/O
 * storage providers.
 * @action: the #NAAction action to be written.
 * @message: the I/O provider can allocate and store here an error
 * message.
 *
 * Deletes an action from the storage subsystem.
 *
 * Returns: the NAIIOProvider return code.
 *
 * Note that a new action, not already written to an I/O subsystem,
 * doesn't have any attached provider. We so do nothing...
 */
guint
na_iio_provider_delete_action( const NAPivot *pivot, const NAAction *action, gchar **message )
{
	static const gchar *thisfn = "na_iio_provider_delete_action";
	guint ret;
	NAIIOProvider *instance;

	g_debug( "%s: pivot=%p, action=%p, message=%p",
			thisfn, ( void * ) pivot, ( void * ) action, ( void * ) message );
	g_assert( NA_IS_PIVOT( pivot ));
	g_assert( NA_IS_ACTION( action ));

	ret = NA_IIO_PROVIDER_NOT_WRITABLE;
	instance = NA_IIO_PROVIDER( na_action_get_provider( action ));

	if( instance ){
		g_assert( NA_IS_IIO_PROVIDER( instance ));

		if( NA_IIO_PROVIDER_GET_INTERFACE( instance )->delete_action ){
			ret = NA_IIO_PROVIDER_GET_INTERFACE( instance )->delete_action( instance, action, message );
		}
	/*} else {
		*message = g_strdup( _( "Unable to delete the action: no I/O provider." ));
		ret = NA_IIO_PROVIDER_NO_PROVIDER;*/
	}

	return( ret );
}

static gboolean
do_is_willing_to_write( const NAIIOProvider *instance )
{
	return( FALSE );
}

static gboolean
do_is_writable( const NAIIOProvider *instance, const NAAction *action )
{
	return( FALSE );
}

static guint
write_action( const NAIIOProvider *provider, NAAction *action, gchar **message )
{
	static const gchar *thisfn = "na_iio_provider_write_action";
	guint ret;

	g_debug( "%s: provider=%p, action=%p, message=%p",
			thisfn, ( void * ) provider, ( void * ) action, ( void * ) message );

	if( !NA_IIO_PROVIDER_GET_INTERFACE( provider )->is_willing_to_write( provider )){
		return( NA_IIO_PROVIDER_NOT_WILLING_TO_WRITE );
	}

	if( !NA_IIO_PROVIDER_GET_INTERFACE( provider )->is_writable( provider, action )){
		return( NA_IIO_PROVIDER_NOT_WRITABLE );
	}

	if( !NA_IIO_PROVIDER_GET_INTERFACE( provider )->delete_action ||
		!NA_IIO_PROVIDER_GET_INTERFACE( provider )->write_action ){
		return( NA_IIO_PROVIDER_NOT_WILLING_TO_WRITE );
	}

	ret = NA_IIO_PROVIDER_GET_INTERFACE( provider )->delete_action( provider, action, message );
	if( ret != NA_IIO_PROVIDER_WRITE_OK ){
		return( ret );
	}

	return( NA_IIO_PROVIDER_GET_INTERFACE( provider )->write_action( provider, action, message ));
}
