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

#ifndef __NA_IIO_PROVIDER_H__
#define __NA_IIO_PROVIDER_H__

/**
 * SECTION: na_iio_provider
 * @short_description: #NAIIOProvider interface definition.
 * @include: common/na-iio-provider.h
 *
 * This is the API all storage subsystems should implement in order to
 * provide I/O resources to NautilusActions.
 *
 * In a near or far future, provider subsystems may be extended by
 * creating extension libraries, this class loading the modules at
 * startup time (e.g. on the model of provider interfaces in Nautilus).
 */

#include "na-pivot.h"

G_BEGIN_DECLS

#define NA_IIO_PROVIDER_TYPE						( na_iio_provider_get_type())
#define NA_IIO_PROVIDER( object )					( G_TYPE_CHECK_INSTANCE_CAST( object, NA_IIO_PROVIDER_TYPE, NAIIOProvider ))
#define NA_IS_IIO_PROVIDER( object )				( G_TYPE_CHECK_INSTANCE_TYPE( object, NA_IIO_PROVIDER_TYPE ))
#define NA_IIO_PROVIDER_GET_INTERFACE( instance )	( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NA_IIO_PROVIDER_TYPE, NAIIOProviderInterface ))

typedef struct NAIIOProvider                 NAIIOProvider;

typedef struct NAIIOProviderInterfacePrivate NAIIOProviderInterfacePrivate;

typedef struct {
	GTypeInterface                 parent;
	NAIIOProviderInterfacePrivate *private;

	/**
	 * read_tree:
	 * @instance: the #NAIIOProvider provider.
	 *
	 * Reads the while items tree from the specified I/O provider.
	 *
	 * Returns: a hierarchical #GSList of menus, actions and
	 * profiles as #NAObject-derived objects.
	 */
	GList *  ( *read_items_list )    ( const NAIIOProvider *instance );

	/**
	 * is_willing_to_write:
	 * @instance: the #NAIIOProvider provider.
	 *
	 * Checks for global writability of the I/O provider.
	 *
	 * Returns: %TRUE if we are able to update/write/delete a #NAAction
	 * into this I/O provider, %FALSE else.
	 *
	 * Note that the I/O provider may return a positive writability
	 * flag when considering the whole I/O storage subsystem, while not
	 * being able to update/write/delete a particular item.
	 */
	gboolean ( *is_willing_to_write )( const NAIIOProvider *instance );

	/**
	 * is_writable:
	 * @instance: the #NAIIOProvider provider.
	 * @item: a #NAObject action or menu.
	 *
	 * Checks for writability of this particular @item.
	 *
	 * Returns: %TRUE if we are able to update/write/delete the
	 * @item, %FALSE else.
	 */
	gboolean ( *is_writable )        ( const NAIIOProvider *instance, const NAObject *item );

	/**
	 * write_tree_item:
	 * @instance: the #NAIIOProvider provider.
	 * @item: a #NAObject to be written.
	 * @message: warning/error messages detected in the operation.
	 *
	 * Updates an existing @item or writes a new one.
	 *
	 * Returns: %NA_IIO_PROVIDER_WRITE_OK if the update/write operation
	 * was successfull, or another code depending of the detected error.
	 */
	guint    ( *write_item )         ( const NAIIOProvider *instance, NAObject *item, gchar **message );

	/**
	 * delete_tree_item:
	 * @instance: the #NAIIOProvider provider.
	 * @item: a #NAObject to be deleted.
	 * @message: warning/error messages detected in the operation.
	 *
	 * Deletes an existing @item from the I/O subsystem.
	 *
	 * Returns: %NA_IIO_PROVIDER_WRITE_OK if the delete operation was
	 * successfull, or another code depending of the detected error.
	 */
	guint    ( *delete_item )        ( const NAIIOProvider *instance, const NAObject *item, gchar **message );
}
	NAIIOProviderInterface;

GType  na_iio_provider_get_type( void );

GList *na_iio_provider_get_items_tree( const NAPivot *pivot );
/*GSList *na_iio_provider_sort_tree( const NAPivot *pivot, GSList *tree );*/
guint  na_iio_provider_write_item( const NAPivot *pivot, NAObject *item, gchar **message );
guint  na_iio_provider_delete_item( const NAPivot *pivot, const NAObject *item, gchar **message );

/* modification notification message to NAPivot
 */
#define NA_IIO_PROVIDER_SIGNAL_ACTION_CHANGED			"notify-consumer-of-action-change"
#define NA_IIO_PROVIDER_SIGNAL_DISPLAY_ORDER_CHANGED	"notify-consumer-of-display-order-change"
#define NA_IIO_PROVIDER_SIGNAL_DISPLAY_ABOUT_CHANGED	"notify-consumer-of-display-about-change"

/* return code of update/write/delete operations
 */
enum {
	NA_IIO_PROVIDER_WRITE_OK = 0,
	NA_IIO_PROVIDER_NOT_WRITABLE,
	NA_IIO_PROVIDER_NOT_WILLING_TO_WRITE,
	NA_IIO_PROVIDER_WRITE_ERROR,
	NA_IIO_PROVIDER_NO_PROVIDER,
	NA_IIO_PROVIDER_PROGRAM_ERROR
};

G_END_DECLS

#endif /* __NA_IIO_PROVIDER_H__ */
