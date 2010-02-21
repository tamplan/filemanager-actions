/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010 Pierre Wieser and others (see AUTHORS)
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

#include <api/na-idata-factory.h>

#include "na-data-factory.h"

/* private interface data
 */
struct NAIDataFactoryInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

gboolean idata_factory_initialized = FALSE;
gboolean idata_factory_finalized   = FALSE;

static GType register_type( void );
static void  interface_base_init( NAIDataFactoryInterface *klass );
static void  interface_base_finalize( NAIDataFactoryInterface *klass );

static guint idata_factory_get_version( const NAIDataFactory *instance );

/**
 * Registers the GType of this interface.
 */
GType
na_idata_factory_get_type( void )
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
	static const gchar *thisfn = "na_idata_factory_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NAIDataFactoryInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "NAIDataFactory", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( NAIDataFactoryInterface *klass )
{
	static const gchar *thisfn = "na_idata_factory_interface_base_init";

	if( !idata_factory_initialized ){

		g_debug( "%s: klass=%p (%s)", thisfn, ( void * ) klass, G_OBJECT_CLASS_NAME( klass ));

		klass->private = g_new0( NAIDataFactoryInterfacePrivate, 1 );

		klass->get_version = idata_factory_get_version;
		klass->get_default = NULL;
		klass->copy = NULL;
		klass->are_equal = NULL;
		klass->is_valid = NULL;
		klass->read_start = NULL;
		klass->read_done = NULL;
		klass->write_start = NULL;
		klass->write_done = NULL;

		idata_factory_initialized = TRUE;
	}
}

static void
interface_base_finalize( NAIDataFactoryInterface *klass )
{
	static const gchar *thisfn = "na_idata_factory_interface_base_finalize";

	if( idata_factory_initialized && !idata_factory_finalized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		idata_factory_finalized = TRUE;

		g_free( klass->private );
	}
}

static guint
idata_factory_get_version( const NAIDataFactory *instance )
{
	return( 1 );
}

/**
 * na_idata_factory_get:
 * @object: this #NAIDataFactory instance.
 * @data_id: the elementary data whose value is to be got.
 *
 * Returns: the searched value.
 *
 * If the type of the value is NADF_TYPE_STRING, NADF_TYPE_LOCALE_STRING,
 * or NADF_TYPE_STRING_LIST, then the returned value is a newly allocated
 * one and should be g_free() (resp. na_core_utils_slist_free()) by the
 * caller.
 */
void *
na_idata_factory_get( const NAIDataFactory *object, guint data_id )
{
	g_return_val_if_fail( NA_IS_IDATA_FACTORY( object ), NULL );

	return( na_data_factory_get( object, data_id ));
}

/**
 * na_idata_factory_set_from_string:
 * @object: this #NAIDataFactory instance.
 * @data_id: the elementary data whose value is to be set.
 * @data: the value to set.
 *
 * Set the elementary data with the given value.
 */
void
na_idata_factory_set_from_string( NAIDataFactory *object, guint data_id, const gchar *data )
{
	g_return_if_fail( NA_IS_IDATA_FACTORY( object ));

	na_data_factory_set_from_string( object, data_id, data );
}

/**
 * na_idata_factory_set_from_void:
 * @object: this #NAIDataFactory instance.
 * @data_id: the elementary data whose value is to be set.
 * @data: the value to set.
 *
 * Set the elementary data with the given value.
 */
void
na_idata_factory_set_from_void( NAIDataFactory *object, guint data_id, const void *data )
{
	g_return_if_fail( NA_IS_IDATA_FACTORY( object ));

	na_data_factory_set_from_void( object, data_id, data );
}
