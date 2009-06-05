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
#include "nact-iio-client.h"
#include "nact-io-client.h"

struct NactIIOClientInterfacePrivate {
};

static GType register_type( void );
static void  interface_base_init( NactIIOClientInterface *klass );
static void  interface_base_finalize( NactIIOClientInterface *klass );

GType
nact_iio_client_get_type( void )
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
	static const GTypeInfo info = {
		sizeof( NactIIOClientInterface ),
		( GBaseInitFunc ) interface_base_init,
		( GBaseFinalizeFunc ) interface_base_finalize,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL
	};

	GType type = g_type_register_static( G_TYPE_INTERFACE, "NactIIOClient", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( NactIIOClientInterface *klass )
{
	static const gchar *thisfn = "nact_iio_client_interface_base_init";
	static gboolean initialized = FALSE;

	if( !initialized ){
		g_debug( "%s: klass=%p", thisfn, klass );

		klass->private = g_new0( NactIIOClientInterfacePrivate, 1 );

		klass->get_io_client = NULL;

		initialized = TRUE;
	}
}

static void
interface_base_finalize( NactIIOClientInterface *klass )
{
	static const gchar *thisfn = "nact_iio_client_interface_base_finalize";
	static gboolean finalized = FALSE ;

	if( !finalized ){
		g_debug( "%s: klass=%p", thisfn, klass );

		g_free( klass->private );

		finalized = TRUE;
	}
}

/**
 * Returns the provider id, usually a pointer to an object which
 * implements the NactIIOProvider interface.
 *
 * @client: a pointer to an object which implements the NactIIOClient
 * interface.
 */
gpointer
nact_iio_client_get_provider_id( const NactIIOClient *client )
{
	g_return_val_if_fail( NACT_IS_IIO_CLIENT( client ), NULL );

	if( NACT_IIO_CLIENT_GET_INTERFACE( client )->get_io_client ){

		NactIOClient *io_client =
			NACT_IO_CLIENT( NACT_IIO_CLIENT_GET_INTERFACE( client )->get_io_client( client ));

		return( nact_io_client_get_provider_id( io_client ));
	}

	return( NULL );
}

/**
 * Returns the provider data, usually a pointer to some data structure
 * allocated by the provider to handle object treatments.
 *
 * @client: a pointer to an object which implements the NactIIOClient
 * interface.
 */
gpointer
nact_iio_client_get_provider_data( const NactIIOClient *client )
{
	g_return_val_if_fail( NACT_IS_IIO_CLIENT( client ), NULL );

	if( NACT_IIO_CLIENT_GET_INTERFACE( client )->get_io_client ){

		NactIOClient *io_client =
			NACT_IO_CLIENT( NACT_IIO_CLIENT_GET_INTERFACE( client )->get_io_client( client ));

		return( nact_io_client_get_provider_data( io_client ));
	}

	return( NULL );
}
