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

#include <api/na-ifactory-provider.h>

#include "na-data-factory.h"
#include "na-ifactory-provider-priv.h"
#include "na-factory-provider.h"

gboolean               ifactory_provider_initialized = FALSE;
gboolean               ifactory_provider_finalized   = FALSE;
NAIFactoryProviderInterface *ifactory_provider_klass       = NULL;

static GType register_type( void );
static void  interface_base_init( NAIFactoryProviderInterface *klass );
static void  interface_base_finalize( NAIFactoryProviderInterface *klass );

static guint ifactory_provider_get_version( const NAIFactoryProvider *instance );

static void  v_factory_provider_read_start( const NAIFactoryProvider *reader, void *reader_data, NAIDataFactory *serializable, GSList **messages );
static void  v_factory_provider_read_done( const NAIFactoryProvider *reader, void *reader_data, NAIDataFactory *serializable, GSList **messages );
static void  v_factory_provider_write_start( const NAIFactoryProvider *writer, void *writer_data, NAIDataFactory *serializable, GSList **messages );
static void  v_factory_provider_write_done( const NAIFactoryProvider *writer, void *writer_data, NAIDataFactory *serializable, GSList **messages );

/**
 * Registers the GType of this interface.
 */
GType
na_ifactory_provider_get_type( void )
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
	static const gchar *thisfn = "na_ifactory_provider_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NAIFactoryProviderInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "NAIFactoryProvider", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( NAIFactoryProviderInterface *klass )
{
	static const gchar *thisfn = "na_ifactory_provider_interface_base_init";

	if( !ifactory_provider_initialized ){

		g_debug( "%s: klass=%p (%s)", thisfn, ( void * ) klass, G_OBJECT_CLASS_NAME( klass ));

		klass->private = g_new0( NAIFactoryProviderInterfacePrivate, 1 );

		klass->get_version = ifactory_provider_get_version;
		klass->read_value = NULL;
		klass->read_done = NULL;
		klass->write_value = NULL;
		klass->write_done = NULL;

		ifactory_provider_klass = klass;
		ifactory_provider_initialized = TRUE;
	}
}

static void
interface_base_finalize( NAIFactoryProviderInterface *klass )
{
	static const gchar *thisfn = "na_ifactory_provider_interface_base_finalize";
	GList *ip;
	NadfImplement *known;

	if( ifactory_provider_initialized && !ifactory_provider_finalized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		ifactory_provider_finalized = TRUE;

		for( ip = klass->private->registered ; ip ; ip = ip->next ){
			known = ( NadfImplement * ) ip->data;
			g_free( known );
		}
		g_list_free( klass->private->registered );

		g_free( klass->private );
	}
}

static guint
ifactory_provider_get_version( const NAIFactoryProvider *instance )
{
	return( 1 );
}

/**
 * na_ifactory_provider_read_item:
 * @reader: the instance which implements this #NAIFactoryProvider interface.
 * @reader_data: instance data.
 * @type: the #GType which identifies the #NAIDataFactory type.
 * @messages: a pointer to a #GSList list of strings; the implementation
 *  may append messages to this list, but shouldn't reinitialize it.
 *
 * Returns: a newly instantiated #NAIDataFactory object just readen from @reader.
 */
NAIDataFactory *
na_ifactory_provider_read_item( const NAIFactoryProvider *reader, void *reader_data, GType type, GSList **messages )
{
	static const gchar *thisfn = "na_ifactory_provider_read_item";
	NAIDataFactory *serializable;
	gchar *msg;

	serializable = NULL;

	if( ifactory_provider_initialized && !ifactory_provider_finalized ){

		g_return_val_if_fail( NA_IS_IFACTORY_PROVIDER( reader ), NULL );

		serializable = na_data_factory_new( type );

		if( serializable ){
			v_factory_provider_read_start( reader, reader_data, serializable, messages );
			na_data_factory_read( serializable, reader, reader_data, messages );
			v_factory_provider_read_done( reader, reader_data, serializable, messages );

		} else {
			msg = g_strdup_printf( "%s: %ld: unknown type", thisfn, ( long ) type );
			g_warning( "%s", msg );
			*messages = g_slist_append( *messages, msg );
		}
	}

	return( serializable );
}

/**
 * na_ifactory_provider_write_item:
 * @writer: the instance which implements this #NAIFactoryProvider interface.
 * @writer_data: instance data.
 * @serializable: the #NAIDataFactory-derived object to be serialized.
 * @messages: a pointer to a #GSList list of strings; the implementation
 *  may append messages to this list, but shouldn't reinitialize it.
 *
 * Writes the data down to the FactoryProvider.
 */
void
na_ifactory_provider_write_item( const NAIFactoryProvider *writer, void *writer_data, NAIDataFactory *serializable, GSList **messages )
{
	g_return_if_fail( NA_IS_IFACTORY_PROVIDER( writer ));
	g_return_if_fail( NA_IS_IDATA_FACTORY( serializable ));

	if( ifactory_provider_initialized && !ifactory_provider_finalized ){

		v_factory_provider_write_start( writer, writer_data, serializable, messages );
		na_data_factory_write( serializable, writer, writer_data, messages );
		v_factory_provider_write_done( writer, writer_data, serializable, messages );
	}
}

/**
 * na_ifactory_provider_get_idtype_from_gconf_key:
 * @entry: the name of the node we are searching for.
 *
 * Returns: the definition of the data which is exported as @entry in GConf,
 * or %NULL if not found.
 */
NadfIdType *
na_ifactory_provider_get_idtype_from_gconf_key( const gchar *entry )
{
	return( na_factory_provider_get_idtype_from_gconf_key( entry ));
}

static void
v_factory_provider_read_start( const NAIFactoryProvider *reader, void *reader_data, NAIDataFactory *serializable, GSList **messages )
{
	if( NA_IFACTORY_PROVIDER_GET_INTERFACE( reader )->read_start ){
		NA_IFACTORY_PROVIDER_GET_INTERFACE( reader )->read_start( reader, reader_data, serializable, messages );
	}
}

static void
v_factory_provider_read_done( const NAIFactoryProvider *reader, void *reader_data, NAIDataFactory *serializable, GSList **messages )
{
	if( NA_IFACTORY_PROVIDER_GET_INTERFACE( reader )->read_done ){
		NA_IFACTORY_PROVIDER_GET_INTERFACE( reader )->read_done( reader, reader_data, serializable, messages );
	}
}

static void
v_factory_provider_write_start( const NAIFactoryProvider *writer, void *writer_data, NAIDataFactory *serializable, GSList **messages )
{
	if( NA_IFACTORY_PROVIDER_GET_INTERFACE( writer )->write_start ){
		NA_IFACTORY_PROVIDER_GET_INTERFACE( writer )->write_start( writer, writer_data, serializable, messages );
	}
}

static void
v_factory_provider_write_done( const NAIFactoryProvider *writer, void *writer_data, NAIDataFactory *serializable, GSList **messages )
{
	if( NA_IFACTORY_PROVIDER_GET_INTERFACE( writer )->write_done ){
		NA_IFACTORY_PROVIDER_GET_INTERFACE( writer )->write_done( writer, writer_data, serializable, messages );
	}
}
