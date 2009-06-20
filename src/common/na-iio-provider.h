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

/*
 * NAIIOProvider interface definition.
 *
 * This is the API all storage subsystems should implement in order to
 * provide i/o resources to NautilusActions.
 *
 * In a near or far future, provider subsystems may be extended by
 * creating extension libraries, this class loading the modules at
 * startup time (e.g. on the model of provider interfaces in Nautilus).
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define NA_IIO_PROVIDER_TYPE						( na_iio_provider_get_type())
#define NA_IIO_PROVIDER( object )					( G_TYPE_CHECK_INSTANCE_CAST( object, NA_IIO_PROVIDER_TYPE, NAIIOProvider ))
#define NA_IS_IIO_PROVIDER( object )				( G_TYPE_CHECK_INSTANCE_TYPE( object, NA_IIO_PROVIDER_TYPE ))
#define NA_IIO_PROVIDER_GET_INTERFACE( instance )	( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NA_IIO_PROVIDER_TYPE, NAIIOProviderInterface ))

typedef struct NAIIOProvider NAIIOProvider;

typedef struct NAIIOProviderInterfacePrivate NAIIOProviderInterfacePrivate;

typedef struct {
	GTypeInterface                 parent;
	NAIIOProviderInterfacePrivate *private;

	/* i/o api */
	GSList * ( *read_actions )       ( NAIIOProvider *instance );
	gboolean ( *is_writable )        ( NAIIOProvider *instance );
	gboolean ( *is_willing_to_write )( NAIIOProvider *instance, const GObject *action );
	guint    ( *write_action )       ( NAIIOProvider *instance, const GObject *action, gchar **message );
}
	NAIIOProviderInterface;

GType    na_iio_provider_get_type( void );

GSList  *na_iio_provider_read_actions( const GObject *pivot );

guint    na_iio_provider_write_action( const GObject *pivot, const GObject *action, gchar **message );

/* return code of write_action function
 */
enum {
	NA_IIO_PROVIDER_WRITE_OK = 0,
	NA_IIO_PROVIDER_NOT_WRITABLE,
	NA_IIO_PROVIDER_NOT_WILLING_TO_WRITE,
	NA_IIO_PROVIDER_WRITE_ERROR
};

G_END_DECLS

#endif /* __NA_IIO_PROVIDER_H__ */
