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

#ifndef __NAUTILUS_ACTIONS_API_NA_IIO_PROVIDER_H__
#define __NAUTILUS_ACTIONS_API_NA_IIO_PROVIDER_H__

#include "na-object-item.h"

G_BEGIN_DECLS

#define NA_IIO_PROVIDER_TYPE						( na_iio_provider_get_type())
#define NA_IIO_PROVIDER( instance )					( G_TYPE_CHECK_INSTANCE_CAST( instance, NA_IIO_PROVIDER_TYPE, NAIIOProvider ))
#define NA_IS_IIO_PROVIDER( instance )				( G_TYPE_CHECK_INSTANCE_TYPE( instance, NA_IIO_PROVIDER_TYPE ))
#define NA_IIO_PROVIDER_GET_INTERFACE( instance )	( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NA_IIO_PROVIDER_TYPE, NAIIOProviderInterface ))

typedef struct _NAIIOProvider                 NAIIOProvider;

typedef struct _NAIIOProviderInterfacePrivate NAIIOProviderInterfacePrivate;

typedef struct {
	GTypeInterface                 parent;
	NAIIOProviderInterfacePrivate *private;

	/**
	 * get_version:
	 * @instance: the #NAIIOProvider provider.
	 *
	 * Returns: the version of this interface supported by the I/O provider.
	 *
	 * Defaults to 1.
	 */
	guint    ( *get_version )        ( const NAIIOProvider *instance );

	/**
	 * get_id:
	 * @instance: the #NAIIOProvider provider.
	 *
	 * Returns: the id of the I/O provider, as a newly allocated string
	 * which should be g_free() by the caller.
	 *
	 * To avoid any collision, the I/O provider id is allocated by the
	 * Nautilus-Actions maintainer team. If you wish develop a new I/O
	 * provider, and so need a new provider id, please contact the
	 * maintainers (see nautilus-actions.doap).
	 *
	 * The I/O provider must implement this function.
	 */
	gchar *  ( *get_id )             ( const NAIIOProvider *instance );

	/**
	 * get_name:
	 * @instance: the #NAIIOProvider provider.
	 *
	 * Returns: the name to be displayed for this I/O provider, as a
	 * newly allocated string which should be g_free() by the caller.
	 *
	 * Defaults to an empty string.
	 */
	gchar *  ( *get_name )           ( const NAIIOProvider *instance );

	/**
	 * read_items:
	 * @instance: the #NAIIOProvider provider.
	 * @messages: a pointer to a #GSList list of strings; the provider
	 *  may append messages to this list, but shouldn't reinitialize it.
	 *
	 * Reads the whole items list from the specified I/O provider.
	 *
	 * Returns: a unordered flat #GList of #NAObject-derived objects
	 * (menus or actions); the actions embed their own profiles.
	 */
	GList *  ( *read_items )         ( const NAIIOProvider *instance, GSList **messages );

	/**
	 * is_willing_to_write:
	 * @instance: the #NAIIOProvider provider.
	 *
	 * Returns: %TRUE if this I/O provider is willing to write,
	 *  %FALSE else.
	 *
	 * The 'willing_to_write' property is intrinsic to the I/O provider.
	 * It is not supposed to make any assumption on the environment it is
	 * currently running on.
	 * This property just says that the developer/maintainer has released
	 * the needed code in order to update/create/delete #NAObject-
	 * derived objects.
	 *
	 * Note that even if this property is %TRUE, there is yet many
	 * reasons for not being able to update/delete existing items or
	 * create new ones (see e.g. #is_able_to_write() below).
	 */
	gboolean ( *is_willing_to_write )( const NAIIOProvider *instance );

	/**
	 * is_able_to_write:
	 * @instance: the #NAIIOProvider provider.
	 *
	 * Returns: %TRUE if this I/O provider is able to do write
	 * operations at runtime, %FALSE else.
	 *
	 * The 'able_to_write' property is a runtime one.
	 * When returning %TRUE, the I/O provider insures that it has
	 * sucessfully checked that it was able to write some things
	 * down to its storage subsystems.
	 *
	 * The 'able_to_write' property is independant of the
	 * 'willing_to_write' above, though it is only checked if the
	 * I/O provider is actually willing to write.
	 *
	 * This condition is only relevant when trying to define new items,
	 * to see if a willing_to provider is actually able to do write
	 * operations. It it not relevant for updating/deleting already
	 * existings items as they have already checked their own runtime
	 * writability status when readen from the storage subsystems.
	 *
	 * Note that even if this property is %TRUE, there is yet many
	 * reasons for not being able to update/delete existing items or
	 * create new ones (see e.g. 'locked' preference key).
	 */
	gboolean ( *is_able_to_write )   ( const NAIIOProvider *instance );

	/**
	 * write_item:
	 * @instance: the #NAIIOProvider provider.
	 * @item: a #NAObjectItem-derived item, menu or action.
	 * @messages: a pointer to a #GSList list of strings; the provider
	 *  may append messages to this list, but shouldn't reinitialize it.
	 *
	 * Writes a new @item.
	 *
	 * Returns: %NA_IIO_PROVIDER_CODE_OK if the write operation
	 * was successfull, or another code depending of the detected error.
	 *
	 * Note: there is no update_item function ; it is the responsability
	 * of the provider to delete the previous version of an item before
	 * actually writing the new one.
	 */
	guint    ( *write_item )         ( const NAIIOProvider *instance, const NAObjectItem *item, GSList **messages );

	/**
	 * delete_item:
	 * @instance: the #NAIIOProvider provider.
	 * @item: a #NAObjectItem-derived item, menu or action.
	 * @messages: a pointer to a #GSList list of strings; the provider
	 *  may append messages to this list, but shouldn't reinitialize it.
	 *
	 * Deletes an existing @item from the I/O subsystem.
	 *
	 * Returns: %NA_IIO_PROVIDER_CODE_OK if the delete operation was
	 * successfull, or another code depending of the detected error.
	 */
	guint    ( *delete_item )        ( const NAIIOProvider *instance, const NAObjectItem *item, GSList **messages );

	/**
	 * duplicate_data:
	 * @instance: the #NAIIOProvider provider.
	 * @dest: a #NAObjectItem-derived item, menu or action.
	 * @source: a #NAObjectItem-derived item, menu or action.
	 * @messages: a pointer to a #GSList list of strings; the provider
	 *  may append messages to this list, but shouldn't reinitialize it.
	 *
	 * Duplicates provider-specific data (if any) from @source to @dest.
	 *
	 * Note that this does not duplicate in any way any #NAObject-derived
	 * object. We are just dealing here with the provider-specific data
	 * which may have been attached to a #NAObject-derived object.
	 *
	 * Returns: %NA_IIO_PROVIDER_CODE_OK if the duplicate operation was
	 * successfull, or another code depending of the detected error.
	 */
	guint    ( *duplicate_data )     ( const NAIIOProvider *instance, NAObjectItem *dest, const NAObjectItem *source, GSList **messages );
}
	NAIIOProviderInterface;

GType na_iio_provider_get_type( void );

/* This function is to be called by the I/O provider when it detects
 * that an object has been modified in its underlying storage
 * subsystem. It eventually ends up by sending a messages to the consumers.
 */
void  na_iio_provider_item_changed ( const NAIIOProvider *instance );

#define IIO_PROVIDER_SIGNAL_ITEM_CHANGED	"na-iio-provider-notify-pivot"

/* Adding a new status here should imply also adding a new tooltip
 * in #na_io_provider_get_readonly_tooltip().
 */
/**
 * NAIIOProviderWritabilityStatus:
 *
 * @NA_IIO_PROVIDER_STATUS_UNDETERMINED: undertermined.
 * @NA_IIO_PROVIDER_STATUS_WRITABLE: the item is writable.
 * @NA_IIO_PROVIDER_STATUS_ITEM_READONLY: the item is read-only.
 * @NA_IIO_PROVIDER_STATUS_PROVIDER_NOT_WILLING_TO: the provider is not
 *  willing to write this item, or doest not implement the required
 *  interface.
 * @NA_IIO_PROVIDER_STATUS_NO_PROVIDER_FOUND: the provider has not been
 *  found.
 * @NA_IIO_PROVIDER_STATUS_PROVIDER_LOCKED_BY_ADMIN: the provider has been
 *  locked by the administrator.
 * @NA_IIO_PROVIDER_STATUS_PROVIDER_LOCKED_BY_USER: the provider has been
 *  locked by the user.
 * @NA_IIO_PROVIDER_STATUS_CONFIGURATION_LOCKED_BY_ADMIN: the whole
 *  configuration has been locked by the administrator.
 * @NA_IIO_PROVIDER_STATUS_NO_API: no API has been found.
 *
 * The reasons for which an item may not be writable.
 */
enum {
	NA_IIO_PROVIDER_STATUS_UNDETERMINED = 0,
	NA_IIO_PROVIDER_STATUS_WRITABLE,
	NA_IIO_PROVIDER_STATUS_ITEM_READONLY,
	NA_IIO_PROVIDER_STATUS_PROVIDER_NOT_WILLING_TO,
	NA_IIO_PROVIDER_STATUS_NO_PROVIDER_FOUND,
	NA_IIO_PROVIDER_STATUS_PROVIDER_LOCKED_BY_ADMIN,
	NA_IIO_PROVIDER_STATUS_PROVIDER_LOCKED_BY_USER,
	NA_IIO_PROVIDER_STATUS_CONFIGURATION_LOCKED_BY_ADMIN,
	NA_IIO_PROVIDER_STATUS_NO_API,
	/*< private >*/
	NA_IIO_PROVIDER_STATUS_LAST,
}
	NAIIOProviderWritabilityStatus;

/* adding a new code here should imply also adding a new label
 * in #na_io_provider_get_return_code_label().
 */
/**
 * NAIIOProviderOperationStatus:
 *
 * @NA_IIO_PROVIDER_CODE_OK: the requested operation has been successful.
 *
 * @NA_IIO_PROVIDER_CODE_PROGRAM_ERROR: a program error has been detected.
 *  You should open a bug in <ulink url="">Bugzilla</ulink>.
 *
 * @NA_IIO_PROVIDER_CODE_NOT_WILLING_TO_RUN:
 *
 * @NA_IIO_PROVIDER_CODE_WRITE_ERROR:
 *
 * @NA_IIO_PROVIDER_CODE_DELETE_SCHEMAS_ERROR:
 *
 * @NA_IIO_PROVIDER_CODE_DELETE_CONFIG_ERROR:
 *
 * The return code of operations.
 */
enum {
	NA_IIO_PROVIDER_CODE_OK = 0,
	NA_IIO_PROVIDER_CODE_PROGRAM_ERROR = 1 + NA_IIO_PROVIDER_STATUS_LAST,
	NA_IIO_PROVIDER_CODE_NOT_WILLING_TO_RUN,
	NA_IIO_PROVIDER_CODE_WRITE_ERROR,
	NA_IIO_PROVIDER_CODE_DELETE_SCHEMAS_ERROR,
	NA_IIO_PROVIDER_CODE_DELETE_CONFIG_ERROR,
}
	NAIIOProviderOperationStatus;

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_API_NA_IIO_PROVIDER_H__ */
