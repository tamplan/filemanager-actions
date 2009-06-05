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

#include "nact-action.h"
#include "nact-action-profile.h"
#include "nact-iio-provider.h"
#include "nact-pivot.h"

struct NactIIOProviderInterfacePrivate {
};

static GType register_type( void );
static void  interface_base_init( NactIIOProviderInterface *klass );
static void  interface_base_finalize( NactIIOProviderInterface *klass );

/**
 * Registers the GType of this interface.
 */
GType
nact_iio_provider_get_type( void )
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
	static const gchar *thisfn = "nact_iio_provider_register_type";
	g_debug( "%s", thisfn );

	static const GTypeInfo info = {
		sizeof( NactIIOProviderInterface ),
		( GBaseInitFunc ) interface_base_init,
		( GBaseFinalizeFunc ) interface_base_finalize,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL
	};

	GType type = g_type_register_static( G_TYPE_INTERFACE, "NactIIOProvider", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( NactIIOProviderInterface *klass )
{
	static const gchar *thisfn = "nact_iio_provider_interface_base_init";
	static gboolean initialized = FALSE;

	if( !initialized ){
		g_debug( "%s: klass=%p", thisfn, klass );

		klass->private = g_new0( NactIIOProviderInterfacePrivate, 1 );

		klass->load_actions = NULL;
		klass->load_action_properties = NULL;
		klass->load_profiles = NULL;
		klass->load_profile_properties = NULL;
		klass->release_data = NULL;

		initialized = TRUE;
	}
}

static void
interface_base_finalize( NactIIOProviderInterface *klass )
{
	static const gchar *thisfn = "nact_iio_provider_interface_base_finalize";
	static gboolean finalized = FALSE ;

	if( !finalized ){
		g_debug( "%s: klass=%p", thisfn, klass );

		g_free( klass->private );

		finalized = TRUE;
	}
}

/**
 * Load the defined actions.
 *
 * Return a GSList of NactAction objects.
 */
GSList *
nact_iio_provider_load_actions( const GObject *object )
{
	static const gchar *thisfn = "nact_iio_provider_load_actions";
	g_debug( "%s", thisfn );

	g_assert( NACT_IS_PIVOT( object ));
	NactPivot *pivot = NACT_PIVOT( object );

	GSList *actions = NULL;
	GSList *ip;
	GSList *list;
	NactIIOProvider *instance;

	GSList *providers = nact_pivot_get_providers( pivot, NACT_IIO_PROVIDER_TYPE );

	for( ip = providers ; ip ; ip = ip->next ){

		instance = NACT_IIO_PROVIDER( ip->data );

		if( NACT_IIO_PROVIDER_GET_INTERFACE( instance )->load_actions ){
			list = NACT_IIO_PROVIDER_GET_INTERFACE( instance )->load_actions( instance );
			actions = g_slist_concat( actions, list );
		}
	}

	return( actions );
}

/**
 * Load and set properties of the action.
 */
void
nact_iio_provider_load_action_properties( NactIIOClient *client )
{
	static const gchar *thisfn = "nact_iio_provider_load_action_properties";
	g_debug( "%s: client=%p", thisfn, client );

	g_assert( NACT_IS_IIO_CLIENT( client ));
	g_assert( NACT_IS_ACTION( client ));

	NactIIOProvider *provider = nact_iio_client_get_provider_id( client );

	if( NACT_IIO_PROVIDER_GET_INTERFACE( provider )->load_action_properties ){
		NACT_IIO_PROVIDER_GET_INTERFACE( provider )->load_action_properties( client );
	}
}

/**
 * Load the defined profiles for the action.
 *
 * Return a GSList of NactActionProfile objects.
 */
GSList *
nact_iio_provider_load_profiles( NactIIOClient *client )
{
	static const gchar *thisfn = "nact_iio_provider_load_profiles";
	g_debug( "%s: client=%p", thisfn, client );

	g_assert( NACT_IS_IIO_CLIENT( client ));
	g_assert( NACT_IS_ACTION( client ));

	NactIIOProvider *provider = nact_iio_client_get_provider_id( client );

	GSList *profiles = NULL;

	if( NACT_IIO_PROVIDER_GET_INTERFACE( provider )->load_profiles ){
		GSList *list = NACT_IIO_PROVIDER_GET_INTERFACE( provider )->load_profiles( client );
		profiles = g_slist_concat( profiles, list );
	}

	return( profiles );
}

/**
 * Load and set properties of the profile.
 */
void
nact_iio_provider_load_profile_properties( NactObject *profile )
{
	static const gchar *thisfn = "nact_iio_provider_load_profile_properties";
	g_debug( "%s", thisfn );

	g_assert( NACT_IS_ACTION_PROFILE( profile ));

	NactIIOClient *client =
		NACT_IIO_CLIENT( nact_action_profile_get_action( NACT_ACTION_PROFILE( profile )));

	g_assert( NACT_IS_IIO_CLIENT( client ));
	g_assert( NACT_IS_ACTION( client ));

	NactIIOProvider *provider = nact_iio_client_get_provider_id( client );

	if( NACT_IIO_PROVIDER_GET_INTERFACE( provider )->load_profile_properties ){
		NACT_IIO_PROVIDER_GET_INTERFACE( provider )->load_profile_properties( profile );
	}
}

/**
 * Called by nact_io_client_instance_dispose.
 */
void
nact_iio_provider_release_data( NactIIOClient *client )
{
	g_assert( NACT_IS_IIO_CLIENT( client ));

	NactIIOProvider *provider = nact_iio_client_get_provider_id( client );

	if( NACT_IIO_PROVIDER_GET_INTERFACE( provider )->release_data ){
		NACT_IIO_PROVIDER_GET_INTERFACE( provider )->release_data( client );
	}
}
