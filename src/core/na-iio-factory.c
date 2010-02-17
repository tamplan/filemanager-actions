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

#include <api/na-iio-factory.h>

#include "na-data-factory.h"
#include "na-iio-factory-priv.h"
#include "na-io-factory.h"

gboolean               iio_factory_initialized = FALSE;
gboolean               iio_factory_finalized   = FALSE;
NAIIOFactoryInterface *iio_factory_klass       = NULL;

static GType register_type( void );
static void  interface_base_init( NAIIOFactoryInterface *klass );
static void  interface_base_finalize( NAIIOFactoryInterface *klass );

static guint iio_factory_get_version( const NAIIOFactory *instance );

static void  v_io_factory_read_start( const NAIIOFactory *reader, void *reader_data, NAIDataFactory *serializable, GSList **messages );
static void  v_io_factory_read_done( const NAIIOFactory *reader, void *reader_data, NAIDataFactory *serializable, GSList **messages );
static void  v_io_factory_write_start( const NAIIOFactory *writer, void *writer_data, NAIDataFactory *serializable, GSList **messages );
static void  v_io_factory_write_done( const NAIIOFactory *writer, void *writer_data, NAIDataFactory *serializable, GSList **messages );

/**
 * Registers the GType of this interface.
 */
GType
na_iio_factory_get_type( void )
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
	static const gchar *thisfn = "na_iio_factory_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NAIIOFactoryInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "NAIIOFactory", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( NAIIOFactoryInterface *klass )
{
	static const gchar *thisfn = "na_iio_factory_interface_base_init";

	if( !iio_factory_initialized ){

		g_debug( "%s: klass=%p (%s)", thisfn, ( void * ) klass, G_OBJECT_CLASS_NAME( klass ));

		klass->private = g_new0( NAIIOFactoryInterfacePrivate, 1 );

		klass->get_version = iio_factory_get_version;
		klass->read_value = NULL;
		klass->read_done = NULL;
		klass->write_value = NULL;
		klass->write_done = NULL;

		iio_factory_klass = klass;
		iio_factory_initialized = TRUE;
	}
}

static void
interface_base_finalize( NAIIOFactoryInterface *klass )
{
	static const gchar *thisfn = "na_iio_factory_interface_base_finalize";
	GList *ip;
	NadfImplement *known;

	if( iio_factory_initialized && !iio_factory_finalized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		iio_factory_finalized = TRUE;

		for( ip = klass->private->registered ; ip ; ip = ip->next ){
			known = ( NadfImplement * ) ip->data;
			g_free( known );
		}
		g_list_free( klass->private->registered );

		g_free( klass->private );
	}
}

static guint
iio_factory_get_version( const NAIIOFactory *instance )
{
	return( 1 );
}

/**
 * na_iio_factory_read_item:
 * @reader: the instance which implements this #NAIIOFactory interface.
 * @reader_data: instance data.
 * @type: the #GType which identifies the #NAIDataFactory type.
 * @messages: a pointer to a #GSList list of strings; the implementation
 *  may append messages to this list, but shouldn't reinitialize it.
 *
 * Returns: a newly instantiated #NAIDataFactory object just readen from @reader.
 */
NAIDataFactory *
na_iio_factory_read_item( const NAIIOFactory *reader, void *reader_data, GType type, GSList **messages )
{
	static const gchar *thisfn = "na_iio_factory_read_item";
	NAIDataFactory *serializable;
	gchar *msg;

	serializable = NULL;

	if( iio_factory_initialized && !iio_factory_finalized ){

		g_return_val_if_fail( NA_IS_IIO_FACTORY( reader ), NULL );

		serializable = na_data_factory_new( type );

		if( serializable ){
			v_io_factory_read_start( reader, reader_data, serializable, messages );
			na_data_factory_read( serializable, reader, reader_data, messages );
			v_io_factory_read_done( reader, reader_data, serializable, messages );

		} else {
			msg = g_strdup_printf( "%s: %ld: unknown type", thisfn, ( long ) type );
			g_warning( "%s", msg );
			*messages = g_slist_append( *messages, msg );
		}
	}

	return( serializable );
}

/**
 * na_iio_factory_write_item:
 * @writer: the instance which implements this #NAIIOFactory interface.
 * @writer_data: instance data.
 * @serializable: the #NAIDataFactory-derived object to be serialized.
 * @messages: a pointer to a #GSList list of strings; the implementation
 *  may append messages to this list, but shouldn't reinitialize it.
 *
 * Writes the data down to the IOFactory.
 */
void
na_iio_factory_write_item( const NAIIOFactory *writer, void *writer_data, NAIDataFactory *serializable, GSList **messages )
{
	g_return_if_fail( NA_IS_IIO_FACTORY( writer ));
	g_return_if_fail( NA_IS_IDATA_FACTORY( serializable ));

	if( iio_factory_initialized && !iio_factory_finalized ){

		v_io_factory_write_start( writer, writer_data, serializable, messages );
		na_data_factory_write( serializable, writer, writer_data, messages );
		v_io_factory_write_done( writer, writer_data, serializable, messages );
	}
}

static void
v_io_factory_read_start( const NAIIOFactory *reader, void *reader_data, NAIDataFactory *serializable, GSList **messages )
{
	if( NA_IIO_FACTORY_GET_INTERFACE( reader )->read_start ){
		NA_IIO_FACTORY_GET_INTERFACE( reader )->read_start( reader, reader_data, serializable, messages );
	}
}

static void
v_io_factory_read_done( const NAIIOFactory *reader, void *reader_data, NAIDataFactory *serializable, GSList **messages )
{
	if( NA_IIO_FACTORY_GET_INTERFACE( reader )->read_done ){
		NA_IIO_FACTORY_GET_INTERFACE( reader )->read_done( reader, reader_data, serializable, messages );
	}
}

static void
v_io_factory_write_start( const NAIIOFactory *writer, void *writer_data, NAIDataFactory *serializable, GSList **messages )
{
	if( NA_IIO_FACTORY_GET_INTERFACE( writer )->write_start ){
		NA_IIO_FACTORY_GET_INTERFACE( writer )->write_start( writer, writer_data, serializable, messages );
	}
}

static void
v_io_factory_write_done( const NAIIOFactory *writer, void *writer_data, NAIDataFactory *serializable, GSList **messages )
{
	if( NA_IIO_FACTORY_GET_INTERFACE( writer )->write_done ){
		NA_IIO_FACTORY_GET_INTERFACE( writer )->write_done( writer, writer_data, serializable, messages );
	}
}
