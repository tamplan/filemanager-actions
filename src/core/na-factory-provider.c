/*
 * FileManager-Actions
 * A file-manager extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
 *
 * FileManager-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * FileManager-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FileManager-Actions; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
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

#include <api/na-iio-provider.h>

#include "na-factory-provider.h"

/*
 * na_factory_provider_read_data:
 * @reader: the instance which implements this #FMAIFactoryProvider interface.
 * @reader_data: instance data.
 * @object: the #NAIFactoryobject being unserialized.
 * @def: a #FMADataDef structure which identifies the data to be unserialized.
 * @messages: a pointer to a #GSList list of strings; the implementation
 *  may append messages to this list, but shouldn't reinitialize it.
 *
 * Reads the specified data and set it up into the @boxed.
 *
 * Returns: a new #FMADataBoxed object which contains the data.
 */
FMADataBoxed *
na_factory_provider_read_data( const FMAIFactoryProvider *reader, void *reader_data,
								const FMAIFactoryObject *object, const FMADataDef *def,
								GSList **messages )
{
	FMADataBoxed *boxed;

	g_return_val_if_fail( FMA_IS_IFACTORY_PROVIDER( reader ), NULL );
	g_return_val_if_fail( FMA_IS_IFACTORY_OBJECT( object ), NULL );

	boxed = NULL;

	if( FMA_IFACTORY_PROVIDER_GET_INTERFACE( reader )->read_data ){
		boxed = FMA_IFACTORY_PROVIDER_GET_INTERFACE( reader )->read_data( reader, reader_data, object, def, messages );
	}

	return( boxed );
}

/*
 * na_factory_provider_write_data:
 * @writer: the instance which implements this #FMAIFactoryProvider interface.
 * @writer_data: instance data.
 * @object: the #NAIFactoryobject being serialized.
 * @boxed: the #FMADataBoxed object which is to be serialized.
 * @messages: a pointer to a #GSList list of strings; the implementation
 *  may append messages to this list, but shouldn't reinitialize it.
 *
 * Returns: a NAIIOProvider operation return code.
 */
guint
na_factory_provider_write_data( const FMAIFactoryProvider *writer, void *writer_data,
								const FMAIFactoryObject *object, const FMADataBoxed *boxed,
								GSList **messages )
{
	guint code;

	g_return_val_if_fail( FMA_IS_IFACTORY_PROVIDER( writer ), NA_IIO_PROVIDER_CODE_PROGRAM_ERROR );
	g_return_val_if_fail( FMA_IS_IFACTORY_OBJECT( object ), NA_IIO_PROVIDER_CODE_PROGRAM_ERROR );

	code = NA_IIO_PROVIDER_CODE_NOT_WILLING_TO_RUN;

	if( FMA_IFACTORY_PROVIDER_GET_INTERFACE( writer )->write_data ){
		code = FMA_IFACTORY_PROVIDER_GET_INTERFACE( writer )->write_data( writer, writer_data, object, boxed, messages );
	}

	return( code );
}
