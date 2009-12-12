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

#include "na-iio-provider.h"

/* private interface data
 */
struct NAIIOProviderInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

static gboolean st_initialized = FALSE;
static gboolean st_finalized = FALSE;

static GType    register_type( void );
static void     interface_base_init( NAIIOProviderInterface *klass );
static void     interface_base_finalize( NAIIOProviderInterface *klass );

static gboolean do_is_willing_to_write( const NAIIOProvider *instance );
static gboolean do_is_writable( const NAIIOProvider *instance, const NAObjectItem *item );

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

	if( !st_initialized ){
		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NAIIOProviderInterfacePrivate, 1 );

		klass->get_id = NULL;
		klass->get_version = NULL;
		klass->read_items = NULL;
		klass->is_willing_to_write = do_is_willing_to_write;
		klass->is_writable = do_is_writable;
		klass->write_item = NULL;
		klass->delete_item = NULL;

		st_initialized = TRUE;
	}
}

static void
interface_base_finalize( NAIIOProviderInterface *klass )
{
	static const gchar *thisfn = "na_iio_provider_interface_base_finalize";

	if( !st_finalized ){

		st_finalized = TRUE;

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );
	}
}

static gboolean
do_is_willing_to_write( const NAIIOProvider *instance )
{
	return( FALSE );
}

static gboolean
do_is_writable( const NAIIOProvider *instance, const NAObjectItem *item )
{
	return( FALSE );
}

/**
 * na_iio_provider_config_changed:
 * @instance: the calling NAIIOProvider.
 * @id: the id of the modified #NAObjectItem-derived object.
 *
 * Advertises Nautilus-Actions that this #NAIIOProvider @instance has
 * detected a modification in one of its configurations (menu or action).
 *
 * This function should be triggered for each and every #NAObjectItem-
 * derived modified objects, but only once for each one.
 */
void
na_iio_provider_config_changed( const NAIIOProvider *instance, const gchar *id )
{
	static const gchar *thisfn = "na_iio_provider_config_changed";

	g_debug( "%s: instance=%p, id=%s", thisfn, ( void * ) instance, id );

	g_signal_emit_by_name(( gpointer ) instance, "notify-consumer-of-action-change", id );
}
