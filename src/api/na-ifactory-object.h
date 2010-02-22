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

#ifndef __NAUTILUS_ACTIONS_API_NA_IFACTORY_OBJECT_H__
#define __NAUTILUS_ACTIONS_API_NA_IFACTORY_OBJECT_H__

/**
 * SECTION: na_ifactory_object
 * @short_description: #NAIFactoryObject interface definition.
 * @include: nautilus-actions/na-ifactory_object.h
 *
 * This interface must be implemented by #NAObject-derived objects which
 * should take advantage of data factory management system.
 *
 * A #NAObject which would implement this #NAIFactoryObject interface
 * must meet following conditions:
 * - must accept an empty constructor
 *
 * Nautilus-Actions v 2.30 - API version:  1
 */

#include "na-ifactory-object-enum.h"
#include "na-ifactory-object-str.h"
#include "na-ifactory-provider-provider.h"

G_BEGIN_DECLS

#define NA_IFACTORY_OBJECT_TYPE						( na_ifactory_object_get_type())
#define NA_IFACTORY_OBJECT( instance )				( G_TYPE_CHECK_INSTANCE_CAST( instance, NA_IFACTORY_OBJECT_TYPE, NAIFactoryObject ))
#define NA_IS_IFACTORY_OBJECT( instance )				( G_TYPE_CHECK_INSTANCE_TYPE( instance, NA_IFACTORY_OBJECT_TYPE ))
#define NA_IFACTORY_OBJECT_GET_INTERFACE( instance )	( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NA_IFACTORY_OBJECT_TYPE, NAIFactoryObjectInterface ))

typedef struct NAIFactoryObject                 NAIFactoryObject;

typedef struct NAIFactoryObjectInterfacePrivate NAIFactoryObjectInterfacePrivate;

typedef struct {
	GTypeInterface                  parent;
	NAIFactoryObjectInterfacePrivate *private;

	/**
	 * get_version:
	 * @instance: this #NAIFactoryObject instance.
	 *
	 * Returns: the version of this interface supported by @instance implementation.
	 *
	 * Defaults to 1.
	 */
	guint    ( *get_version )( const NAIFactoryObject *instance );

	/**
	 * get_default:
	 * @instance: this #NAIFactoryObject instance.
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
	gchar *  ( *get_default )( const NAIFactoryObject *instance, const NadfIdType *iddef );

	/**
	 * copy:
	 * @instance: the target #NAIFactoryObject instance.
	 * @source: the source #NAIFactoryObject instance.
	 *
	 * This function is triggered when copying one instance to another,
	 * after all copyable elementary dats have been copied themselves.
	 * The target @instance may take advantage of this call to do some
	 * particular copy tasks.
	 */
	void     ( *copy )       ( NAIFactoryObject *instance, const NAIFactoryObject *source );

	/**
	 * are_equal:
	 * @a: the first #NAIFactoryObject instance.
	 * @b: the second #NAIFactoryObject instance.
	 *
	 * Returns: %TRUE if @a is equal to @b.
	 *
	 * This function is triggered after all elementary data comparisons
	 * have been sucessfully made.
	 */
	gboolean ( *are_equal )  ( const NAIFactoryObject *a, const NAIFactoryObject *b );

	/**
	 * is_valid:
	 * @object: the #NAIFactoryObject instance whose validity is to be checked.
	 *
	 * Returns: %TRUE if @object is valid.
	 *
	 * This function is triggered after all elementary data comparisons
	 * have been sucessfully made.
	 */
	gboolean ( *is_valid )   ( const NAIFactoryObject *object );

	/**
	 * read_start:
	 * @instance: this #NAIFactoryObject instance.
	 * @reader: the instance which has provided read services.
	 * @reader_data: the data associated to @reader.
	 * @messages: a pointer to a #GSList list of strings; the instance
	 *  may append messages to this list, but shouldn't reinitialize it.
	 *
	 * Called just before the object is unserialized.
	 */
	void     ( *read_start ) ( NAIFactoryObject *instance, const NAIFactoryProvider *reader, void *reader_data, GSList **messages );

	/**
	 * read_done:
	 * @instance: this #NAIFactoryObject instance.
	 * @reader: the instance which has provided read services.
	 * @reader_data: the data associated to @reader.
	 * @messages: a pointer to a #GSList list of strings; the instance
	 *  may append messages to this list, but shouldn't reinitialize it.
	 *
	 * Called when the object has been unserialized.
	 */
	void     ( *read_done )  ( NAIFactoryObject *instance, const NAIFactoryProvider *reader, void *reader_data, GSList **messages );

	/**
	 * write_start:
	 * @instance: this #NAIFactoryObject instance.
	 * @writer: the instance which has provided writing services.
	 * @writer_data: the data associated to @writer.
	 * @messages: a pointer to a #GSList list of strings; the instance
	 *  may append messages to this list, but shouldn't reinitialize it.
	 *
	 * Called just before the object is serialized.
	 */
	void     ( *write_start )( NAIFactoryObject *instance, const NAIFactoryProvider *writer, void *writer_data, GSList **messages );

	/**
	 * write_done:
	 * @instance: this #NAIFactoryObject instance.
	 * @writer: the instance which has provided writing services.
	 * @writer_data: the data associated to @writer.
	 * @messages: a pointer to a #GSList list of strings; the instance
	 *  may append messages to this list, but shouldn't reinitialize it.
	 *
	 * Called when the object has been serialized.
	 */
	void     ( *write_done ) ( NAIFactoryObject *instance, const NAIFactoryProvider *writer, void *writer_data, GSList **messages );
}
	NAIFactoryObjectInterface;

GType       na_ifactory_object_get_type( void );

void       *na_ifactory_object_get( const NAIFactoryObject *object, guint data_id );

void        na_ifactory_object_set_from_string( NAIFactoryObject *object, guint data_id, const gchar *data );
void        na_ifactory_object_set_from_void  ( NAIFactoryObject *object, guint data_id, const void *data );

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_API_NA_IFACTORY_OBJECT_H__ */
