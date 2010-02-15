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

#ifndef __NAUTILUS_ACTIONS_API_NA_IDATA_FACTORY_H__
#define __NAUTILUS_ACTIONS_API_NA_IDATA_FACTORY_H__

/**
 * SECTION: na_idata_factory
 * @short_description: #NAIDataFactory interface definition.
 * @include: nautilus-actions/na-idata_factory.h
 *
 * This interface must be implemented by #NAObject-derived objects which
 * should take advantage of data factory management system.
 *
 * A #NAObject which would implement this #NAIDataFactory interface
 * must meet following conditions:
 * - must accept an empty constructor
 *
 * Nautilus-Actions v 2.30 - API version:  1
 */

#include "na-idata-factory-enum.h"
#include "na-idata-factory-str.h"
#include "na-iio-factory-factory.h"

G_BEGIN_DECLS

#define NA_IDATA_FACTORY_TYPE						( na_idata_factory_get_type())
#define NA_IDATA_FACTORY( instance )				( G_TYPE_CHECK_INSTANCE_CAST( instance, NA_IDATA_FACTORY_TYPE, NAIDataFactory ))
#define NA_IS_IDATA_FACTORY( instance )				( G_TYPE_CHECK_INSTANCE_TYPE( instance, NA_IDATA_FACTORY_TYPE ))
#define NA_IDATA_FACTORY_GET_INTERFACE( instance )	( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NA_IDATA_FACTORY_TYPE, NAIDataFactoryInterface ))

typedef struct NAIDataFactory                 NAIDataFactory;

typedef struct NAIDataFactoryInterfacePrivate NAIDataFactoryInterfacePrivate;

typedef struct {
	GTypeInterface                  parent;
	NAIDataFactoryInterfacePrivate *private;

	/**
	 * get_version:
	 * @instance: this #NAIDataFactory instance.
	 *
	 * Returns: the version of this interface supported by @instance implementation.
	 *
	 * Defaults to 1.
	 */
	guint    ( *get_version )( const NAIDataFactory *instance );

	/**
	 * get_default:
	 * @instance: this #NAIDataFactory instance.
	 * @iddef: the #NadfIdType structure which defines the data whose
	 * default value is searched for.
	 *
	 * The @instance may take advantage of this method to setup a default
	 * value for a specific instance, or even for instances of a class when
	 * several classes share some elementary data via common #NadfIdGroup.
	 *
	 * Returns: a newly allocated string which defines the suitable
	 * default value, or %NULL.
	 */
	gchar *  ( *get_default )( const NAIDataFactory *instance, const NadfIdType *iddef );

	/**
	 * copy:
	 * @instance: the target #NAIDataFactory instance.
	 * @source: the source #NAIDataFactory instance.
	 *
	 * This function is triggered when copying one instance to another,
	 * after all copyable elementary dats have been copied themselves.
	 * The target @instance may take advantage of this call to do some
	 * particular copy tasks.
	 */
	void     ( *copy )       ( NAIDataFactory *instance, const NAIDataFactory *source );

	/**
	 * are_equal:
	 * @a: the first #NAIDataFactory instance.
	 * @b: the second #NAIDataFactory instance.
	 *
	 * Returns: %TRUE if @a is equal to @b.
	 *
	 * This function is triggered after all elementary data comparisons
	 * have been sucessfully made.
	 */
	gboolean ( *are_equal )  ( const NAIDataFactory *a, const NAIDataFactory *b );

	/**
	 * read_done:
	 * @instance: this #NAIDataFactory instance.
	 * @reader: the instance which has provided read services.
	 * @reader_data: the data associated to @reader.
	 * @messages: a pointer to a #GSList list of strings; the instance
	 *  may append messages to this list, but shouldn't reinitialize it.
	 *
	 * Called when the object has been unserialized.
	 */
	void     ( *read_done )  ( NAIDataFactory *instance, const NAIIOFactory *reader, void *reader_data, GSList **messages );

	/**
	 * write_done:
	 * @instance: this #NAIDataFactory instance.
	 * @writer: the instance which has provided writing services.
	 * @writer_data: the data associated to @writer.
	 * @messages: a pointer to a #GSList list of strings; the instance
	 *  may append messages to this list, but shouldn't reinitialize it.
	 *
	 * Called when the object has been serialized.
	 */
	void     ( *write_done ) ( NAIDataFactory *instance, const NAIIOFactory *writer, void *writer_data, GSList **messages );
}
	NAIDataFactoryInterface;

GType  na_idata_factory_get_type( void );

void  *na_idata_factory_get( const NAIDataFactory *object, guint data_id );

void   na_idata_factory_set( NAIDataFactory *object, guint data_id, const void *data );

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_API_NA_IDATA_FACTORY_H__ */
