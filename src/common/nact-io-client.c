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
#include "nact-io-client.h"

struct NactIOClientPrivate {
	gboolean dispose_has_run;
	gpointer provider_id;
	gpointer provider_data;
};

struct NactIOClientClassPrivate {
};

enum {
	PROP_PROVIDER_ID = 1,
	PROP_PROVIDER_DATA
};

static GObjectClass *st_parent_class = NULL;

static GType   register_type( void );
static void    class_init( NactIOClientClass *klass );
static void    instance_init( GTypeInstance *instance, gpointer klass );
static void    instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void    instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void    instance_dispose( GObject *object );
static void    instance_finalize( GObject *object );

NactIOClient *
nact_io_client_new( gpointer provider, gpointer data )
{
	return( g_object_new( NACT_IO_CLIENT_TYPE, "provider-id", provider, "provider-data", data, NULL ));
}

GType
nact_io_client_get_type( void )
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
		sizeof( NactIOClientClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactIOClient ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	return( g_type_register_static( G_TYPE_OBJECT, "NactIOClient", &info, 0 ));
}

static void
class_init( NactIOClientClass *klass )
{
	static const gchar *thisfn = "nact_io_client_class_init";
	g_debug( "%s: klass=%p", thisfn, klass );

	st_parent_class = g_type_class_peek_parent( klass );

	GObjectClass *object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
	object_class->set_property = instance_set_property;
	object_class->get_property = instance_get_property;

	GParamSpec *spec;
	spec = g_param_spec_pointer(
			"provider-id",
			"provider-id",
			"IIO Provider internal id.",
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_PROVIDER_ID, spec );

	spec = g_param_spec_pointer(
			"provider-data",
			"provider-data",
			"IIO Provider internal data area pointer",
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_PROVIDER_DATA, spec );

	klass->private = g_new0( NactIOClientClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	/*static const gchar *thisfn = "nact_io_client_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );*/

	g_assert( NACT_IS_IO_CLIENT( instance ));
	NactIOClient *self = NACT_IO_CLIENT( instance );

	self->private = g_new0( NactIOClientPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->provider_id = NULL;
	self->private->provider_data = NULL;
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	g_assert( NACT_IS_IO_CLIENT( object ));
	NactIOClient *self = NACT_IO_CLIENT( object );

	switch( property_id ){
		case PROP_PROVIDER_ID:
			g_value_set_pointer( value, self->private->provider_id );
			break;

		case PROP_PROVIDER_DATA:
			g_value_set_pointer( value, self->private->provider_data );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	/*static const gchar *thisfn = "nact_object_instance_set_property";*/

	g_assert( NACT_IS_IO_CLIENT( object ));
	NactIOClient *self = NACT_IO_CLIENT( object );

	switch( property_id ){
		case PROP_PROVIDER_ID:
			self->private->provider_id = g_value_get_pointer( value );
			/*g_debug( "%s: io_origin=%d", thisfn, self->private->io_origin );*/
			break;

		case PROP_PROVIDER_DATA:
			self->private->provider_data = g_value_get_pointer( value );
			/*g_debug( "%s: io_subsystem=%p", thisfn, self->private->io_subsystem );*/
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_dispose( GObject *object )
{
	g_assert( NACT_IS_IO_CLIENT( object ));
	NactIOClient *self = NACT_IO_CLIENT( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		G_OBJECT_CLASS( st_parent_class )->dispose( object );
	}
}

static void
instance_finalize( GObject *object )
{
	g_assert( NACT_IS_IO_CLIENT( object ));
	/*NactIOClient *self = ( NactIOClient * ) object;*/

	/* chain call to parent class */
	if( st_parent_class->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * Returns the provider id, usually a pointer to an object which
 * implements the NactIIOProvider interface.
 *
 * @client: a pointer to a NactIOClient object.
 */
gpointer
nact_io_client_get_provider_id( const NactIOClient *client )
{
	g_assert( NACT_IS_IO_CLIENT( client ));
	gpointer id;
	g_object_get( G_OBJECT( client ), "provider-id", &id, NULL );
	return( id );
}

/**
 * Returns the provider data, usually a pointer to a structure which
 * holds some NactIIOProvider-relative data.
 *
 * @client: a pointer to a NactIOClient object.
 */
gpointer
nact_io_client_get_provider_data( const NactIOClient *client )
{
	g_assert( NACT_IS_IO_CLIENT( client ));
	gpointer data;
	g_object_get( G_OBJECT( client ), "provider-data", &data, NULL );
	return( data );
}
