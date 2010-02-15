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

#include "na-iio-factory-priv.h"
#include "na-io-factory.h"

extern gboolean               iio_factory_initialized;		/* defined in na-iio-factory.c */
extern gboolean               iio_factory_finalized;		/* defined in na-iio-factory.c */
extern NAIIOFactoryInterface *iio_factory_klass;			/* defined in na-iio-factory.c */

/**
 * na_io_factory_register:
 * @type: the #GType of the implementation class.
 * @groups: a table of #NadfIdGroup structures which defines the
 *  serializable properties which will be attached to each instance of
 *  this class at serialization time.
 *
 * Registers the implementation #GType @type.
 */
void
na_io_factory_register( GType type, const NadfIdGroup *groups )
{
	static const gchar *thisfn = "na_io_factory_register";
	NadfImplement *known;
	NadfIdGroup *registered;

	if( iio_factory_initialized && !iio_factory_finalized ){

		g_debug( "%s: type=%lu, groups=%p", thisfn, ( unsigned long ) type, ( void * ) groups );

		g_return_if_fail( groups != NULL );

		registered = na_io_factory_get_groups( type );
		if( registered ){
			g_warning( "%s: type=%lu: already registered", thisfn, ( unsigned long ) type );

		} else {
			/* register the implementation
			 */
			known = g_new0( NadfImplement, 1 );
			known->type = type;
			known->groups = ( NadfIdGroup * ) groups;

			iio_factory_klass->private->registered = g_list_prepend( iio_factory_klass->private->registered, known );
		}
	}
}

/**
 * na_io_factory_get_groups:
 * @type: a previously registered #GType.
 *
 * Returns the #NadfIdGroups table which has been registered for this @type,
 * or %NULL.
 */
NadfIdGroup *
na_io_factory_get_groups( GType type )
{
	GList *it;

	if( iio_factory_initialized && !iio_factory_finalized ){

		for( it = iio_factory_klass->private->registered ; it ; it = it->next ){
			if((( NadfImplement * ) it->data )->type == type ){
				return((( NadfImplement * ) it->data )->groups );
			}
		}
	}

	return( NULL );
}

/**
 * na_io_factory_read_value:
 * @reader: the instance which implements this #NAIIOFactory interface.
 * @reader_data: instance data.
 * @iddef: a NadfIdType structure which identifies the data to be unserialized.
 * @messages: a pointer to a #GSList list of strings; the implementation
 *  may append messages to this list, but shouldn't reinitialize it.
 *
 * Returns: the desired value, as a newly allocated #GValue, or NULL.
 */
GValue *
na_io_factory_read_value( const NAIIOFactory *reader, void *reader_data, const NadfIdType *iddef, GSList **messages )
{
	GValue *value;

	g_return_val_if_fail( NA_IS_IIO_FACTORY( reader ), NULL );

	value = NULL;

	if( iio_factory_initialized && !iio_factory_finalized ){

		if( NA_IIO_FACTORY_GET_INTERFACE( reader )->read_value ){

			value = NA_IIO_FACTORY_GET_INTERFACE( reader )->read_value( reader, reader_data, iddef, messages );
		}
	}

	return( value );
}
