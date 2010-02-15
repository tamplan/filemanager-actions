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

#ifndef __NAUTILUS_ACTIONS_API_NA_IIO_FACTORY_H__
#define __NAUTILUS_ACTIONS_API_NA_IIO_FACTORY_H__

/**
 * SECTION: na_iio_factory
 * @short_description: #NAIIOFactory interface definition.
 * @include: nautilus-actions/na-iio_factory.h
 *
 * This is the interface used by data factory management system for
 * having serialization/unserialization services. This interface should
 * be implemented by I/O providers which would take advantage of this
 * system.
 *
 * Nautilus-Actions v 2.30 - API version:  1
 */

#include "na-idata-factory.h"
#include "na-iio-factory-factory.h"

G_BEGIN_DECLS

#define NA_IIO_FACTORY_TYPE							( na_iio_factory_get_type())
#define NA_IIO_FACTORY( instance )					( G_TYPE_CHECK_INSTANCE_CAST( instance, NA_IIO_FACTORY_TYPE, NAIIOFactory ))
#define NA_IS_IIO_FACTORY( instance )				( G_TYPE_CHECK_INSTANCE_TYPE( instance, NA_IIO_FACTORY_TYPE ))
#define NA_IIO_FACTORY_GET_INTERFACE( instance )	( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NA_IIO_FACTORY_TYPE, NAIIOFactoryInterface ))

typedef struct NAIIOFactoryInterfacePrivate NAIIOFactoryInterfacePrivate;

typedef struct {
	GTypeInterface                parent;
	NAIIOFactoryInterfacePrivate *private;

	/**
	 * get_version:
	 * @instance: this #NAIIOFactory instance.
	 *
	 * Returns: the version of this interface supported by @instance implementation.
	 *
	 * Defaults to 1.
	 */
	guint    ( *get_version )( const NAIIOFactory *instance );

	/**
	 * read_value:
	 * @reader: this #NAIIOFactory instance.
	 * @reader_data: the data associated to this instance.
	 * @iddef: the description of the data to be readen.
	 * @messages: a pointer to a #GSList list of strings; the provider
	 *  may append messages to this list, but shouldn't reinitialize it.
	 *
	 * Returns: a newly allocated #GValue, or %NULL if an error has occurred.
	 *
	 * Note that a string list should be returned as a #GValue of type
	 * G_TYPE_POINTER, which itself must be a GSList pointer.
	 *
	 * The returned #GValue, and its content if apply, will be freed
	 * by the caller.
	 *
	 * This method must be implemented in order any data be read.
	 */
	GValue * ( *read_value ) ( const NAIIOFactory *reader, void *reader_data, const NadfIdType *iddef, GSList **messages );

	/**
	 * read_done:
	 * @reader: this #NAIIOFactory instance.
	 * @reader_data: the data associated to this instance.
	 * @object: the #NAIDataFactory object which comes to be readen.
	 * @messages: a pointer to a #GSList list of strings; the provider
	 *  may append messages to this list, but shouldn't reinitialize it.
	 *
	 * API called by #NAIDataFactory when all data have been readen.
	 * Implementor may take advantage of this to do some cleanup.
	 */
	void     ( *read_done )  ( const NAIIOFactory *reader, void *reader_data, NAIDataFactory *object, GSList **messages  );

	/**
	 * write_value:
	 * @writer: this #NAIIOFactory instance.
	 * @writer_data: the data associated to this instance.
	 * @iddef: the description of the data to be written.
	 * @value: the #NADataElement to be written down.
	 * @messages: a pointer to a #GSList list of strings; the provider
	 *  may append messages to this list, but shouldn't reinitialize it.
	 *
	 * Write the data embedded in @value down to @instance.
	 *
	 * This method must be implemented in order any data be written.
	 */
	void     ( *write_value )( const NAIIOFactory *writer, void *writer_data, const NadfIdType *iddef, GValue *value, GSList **messages );

	/**
	 * write_done:
	 * @writer: this #NAIIOFactory instance.
	 * @writer_data: the data associated to this instance.
	 * @object: the #NAIDataFactory object which comes to be written.
	 * @messages: a pointer to a #GSList list of strings; the provider
	 *  may append messages to this list, but shouldn't reinitialize it.
	 *
	 * API called by #NAIDataFactory when all data have been written.
	 * Implementor may take advantage of this to do some cleanup.
	 */
	void     ( *write_done ) ( const NAIIOFactory *writer, void *writer_data, NAIDataFactory *object, GSList **messages  );
}
	NAIIOFactoryInterface;

GType           na_iio_factory_get_type( void );

NAIDataFactory *na_iio_factory_read_item  ( const NAIIOFactory *reader, void *reader_data, GType type, GSList **messages );

void            na_iio_factory_write_item( const NAIIOFactory *writer, void *writer_data, NAIDataFactory *serializable, GSList **messages );

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_API_NA_IIO_FACTORY_H__ */
