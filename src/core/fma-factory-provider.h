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

#ifndef __CORE_FMA_FACTORY_PROVIDER_H__
#define __CORE_FMA_FACTORY_PROVIDER_H__

/* @title: FMAIFactoryProvider
 * @short_description: The #FMAIFactoryProvider Internal Functions
 * @include: core/fma-factory-provider.h
 *
 * Declare the function only accessed from core library (not published as API).
 */

#include <api/fma-data-boxed.h>
#include <api/fma-ifactory-provider.h>

G_BEGIN_DECLS

FMADataBoxed *fma_factory_provider_read_data ( const FMAIFactoryProvider *reader, void *reader_data,
									const FMAIFactoryObject *object, const FMADataDef *def,
									GSList **messages );

guint         fma_factory_provider_write_data( const FMAIFactoryProvider *writer, void *writer_data,
									const FMAIFactoryObject *object, const FMADataBoxed *boxed,
									GSList **messages );

G_END_DECLS

#endif /* __CORE_FMA_FACTORY_PROVIDER_H__ */
