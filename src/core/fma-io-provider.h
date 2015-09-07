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

#ifndef __CORE_FMA_IO_PROVIDER_H__
#define __CORE_FMA_IO_PROVIDER_H__

/* @title: FMAIOProvider
 * @short_description: The FMAIOProvider Class Definition
 * @include: core/fma-io-provider.h
 *
 * #FMAIOProvider is the FileManager-Actions class which is used to manage
 * external I/O Providers which implement #FMAIIOProvider interface. Each
 * #FMAIOProvider objects may (or not) encapsulates one #FMAIIOProvider
 * provider.
 *
 * Internal FileManager-Actions code should never directly call a
 * #FMAIIOProvider interface method, but rather should call the
 * corresponding FMAIOProvider class method.
 *
 * Two preferences are used for each i/o provider:
 * 'readable': means that the i/o provider should be read when building
 *  the items hierarchy
 * 'writable': means that the i/o provider is candidate when writing a
 *  new item; this also means that existing items are deletable.
 *
 * To be actually writable, a i/o provider must:
 * - be set as 'writable' from a configuration point of view
 *   this may or not be edited depending of this is a mandatory or user
 *   preference
 * - be willing to write: this is an intrisinc i/o provider attribute
 * - be able to write: this is a runtime i/o provider property
 *
 * and the whole configuration must not have been locked by an admin.
 */

#include "na-pivot.h"

G_BEGIN_DECLS

#define FMA_IO_PROVIDER_TYPE                ( fma_io_provider_get_type())
#define FMA_IO_PROVIDER( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_IO_PROVIDER_TYPE, FMAIOProvider ))
#define FMA_IO_PROVIDER_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_IO_PROVIDER_TYPE, FMAIOProviderClass ))
#define FMA_IS_IO_PROVIDER( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_IO_PROVIDER_TYPE ))
#define FMA_IS_IO_PROVIDER_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_IO_PROVIDER_TYPE ))
#define FMA_IO_PROVIDER_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_IO_PROVIDER_TYPE, FMAIOProviderClass ))

typedef struct _FMAIOProviderPrivate        FMAIOProviderPrivate;

typedef struct {
	/*< private >*/
	GObject               parent;
	FMAIOProviderPrivate *private;
}
	FMAIOProvider;

typedef struct _FMAIOProviderClassPrivate   FMAIOProviderClassPrivate;

typedef struct {
	/*< private >*/
	GObjectClass               parent;
	FMAIOProviderClassPrivate *private;
}
	FMAIOProviderClass;

/* signal sent from a FMAIIOProvider
 * via the fma_iio_provider_item_changed() function
 */
#define IO_PROVIDER_SIGNAL_ITEM_CHANGED		"io-provider-item-changed"

GType          fma_io_provider_get_type                 ( void );

FMAIOProvider *fma_io_provider_find_writable_io_provider( const NAPivot *pivot );
FMAIOProvider *fma_io_provider_find_io_provider_by_id   ( const NAPivot *pivot, const gchar *id );
const GList   *fma_io_provider_get_io_providers_list    ( const NAPivot *pivot );
void           fma_io_provider_unref_io_providers_list  ( void );

gchar         *fma_io_provider_get_id                   ( const FMAIOProvider *provider );
gchar         *fma_io_provider_get_name                 ( const FMAIOProvider *provider );
gboolean       fma_io_provider_is_available             ( const FMAIOProvider *provider );
gboolean       fma_io_provider_is_conf_readable         ( const FMAIOProvider *provider, const NAPivot *pivot, gboolean *mandatory );
gboolean       fma_io_provider_is_conf_writable         ( const FMAIOProvider *provider, const NAPivot *pivot, gboolean *mandatory );
gboolean       fma_io_provider_is_finally_writable      ( const FMAIOProvider *provider, guint *reason );

GList         *fma_io_provider_load_items               ( const NAPivot *pivot, guint loadable_set, GSList **messages );

guint          fma_io_provider_write_item               ( const FMAIOProvider *provider, const FMAObjectItem *item, GSList **messages );
guint          fma_io_provider_delete_item              ( const FMAIOProvider *provider, const FMAObjectItem *item, GSList **messages );
guint          fma_io_provider_duplicate_data           ( const FMAIOProvider *provider, FMAObjectItem *dest, const FMAObjectItem *source, GSList **messages );

gchar         *fma_io_provider_get_readonly_tooltip     ( guint reason );
gchar         *fma_io_provider_get_return_code_label    ( guint code );

G_END_DECLS

#endif /* __CORE_FMA_IO_PROVIDER_H__ */
