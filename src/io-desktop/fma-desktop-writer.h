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

#ifndef __IO_DESKTOP_FMA_DESKTOP_WRITER_H__
#define __IO_DESKTOP_FMA_DESKTOP_WRITER_H__

#include <api/fma-iio-provider.h>
#include <api/fma-iexporter.h>
#include <api/fma-ifactory-provider.h>

G_BEGIN_DECLS

gboolean fma_desktop_writer_iio_provider_is_willing_to_write( const FMAIIOProvider *provider );
gboolean fma_desktop_writer_iio_provider_is_able_to_write   ( const FMAIIOProvider *provider );

guint    fma_desktop_writer_iio_provider_write_item         ( const FMAIIOProvider *provider,
																	const FMAObjectItem *item,
																	GSList **messages );
guint    fma_desktop_writer_iio_provider_delete_item        ( const FMAIIOProvider *provider,
																	const FMAObjectItem *item,
																	GSList **messages );
guint    fma_desktop_writer_iio_provider_duplicate_data     ( const FMAIIOProvider *provider,
																	FMAObjectItem *dest,
																	const FMAObjectItem *source,
																	GSList **messages );

guint    fma_desktop_writer_iexporter_export_to_buffer      ( const FMAIExporter *instance,
																	FMAIExporterBufferParmsv2 *parms );
guint    fma_desktop_writer_iexporter_export_to_file        ( const FMAIExporter *instance,
																	FMAIExporterFileParmsv2 *parms );

guint    fma_desktop_writer_ifactory_provider_write_start   ( const FMAIFactoryProvider *provider,
																	void *writer_data,
																	const FMAIFactoryObject *object,
																	GSList **messages  );
guint    fma_desktop_writer_ifactory_provider_write_data    ( const FMAIFactoryProvider *provider,
																	void *writer_data,
																	const FMAIFactoryObject *object,
																	const FMADataBoxed *boxed, GSList **messages );
guint    fma_desktop_writer_ifactory_provider_write_done    ( const FMAIFactoryProvider *provider,
																	void *writer_data,
																	const FMAIFactoryObject *object,
																	GSList **messages  );

G_END_DECLS

#endif /* __IO_DESKTOP_FMA_DESKTOP_WRITER_H__ */
