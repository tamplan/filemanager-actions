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

#ifndef __CORE_NA_DATA_FACTORY_H__
#define __CORE_NA_DATA_FACTORY_H__

/**
 * SECTION: na_idata_factory
 * @short_description: #NAIDataFactory internal functions.
 * @include: core/na-data-factory.h
 */

#include <api/na-iio-factory.h>

G_BEGIN_DECLS

void            na_data_factory_properties( GObjectClass *class );
NAIDataFactory *na_data_factory_new       ( GType type );

void            na_data_factory_init      ( NAIDataFactory *object );
void            na_data_factory_copy      ( NAIDataFactory *target, const NAIDataFactory *source );
gboolean        na_data_factory_are_equal ( const NAIDataFactory *a, const NAIDataFactory *b );
gboolean        na_data_factory_is_valid  ( const NAIDataFactory *object );
void            na_data_factory_dump      ( const NAIDataFactory *object );
void            na_data_factory_finalize  ( NAIDataFactory *object );

void            na_data_factory_read      ( NAIDataFactory *object, const NAIIOFactory *reader, void *reader_data, GSList **messages );
void            na_data_factory_write     ( NAIDataFactory *object, const NAIIOFactory *writer, void *writer_data, GSList **messages );

void            na_data_factory_set_from_string( NAIDataFactory *object, guint data_id, const gchar *data );
void            na_data_factory_set_from_value ( NAIDataFactory *object, guint data_id, const GValue *value );
void            na_data_factory_set_from_void  ( NAIDataFactory *object, guint data_id, const void *data );

void           *na_data_factory_get       ( const NAIDataFactory *object, guint data_id );
void            na_data_factory_set_value ( const NAIDataFactory *object, guint property_id, GValue *value, GParamSpec *spec );

G_END_DECLS

#endif /* __CORE_NA_DATA_FACTORY_H__ */
