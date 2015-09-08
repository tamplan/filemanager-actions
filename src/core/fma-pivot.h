/*
 * FileManager-Actions
 * A file-manager extension which offers configurable context menu pivots.
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

#ifndef __CORE_FMA_PIVOT_H__
#define __CORE_FMA_PIVOT_H__

/* @title: FMAPivot
 * @short_description: The #FMAPivot Class Definition
 * @include: core/fma-pivot.h
 *
 * A consuming program should allocate one new FMAPivot object in its
 * startup phase. The class takes care of declaring the I/O interfaces,
 * while registering the known providers.
 * 		FMAPivot *pivot = fma_pivot_new();
 *
 * With this newly allocated #FMAPivot object, the consuming program
 * is then able to ask for loading the items.
 * 		fma_pivot_set_loadable( pivot, PIVOT_LOADABLE_SET );
 * 		fma_pivot_load_items( pivot );
 *
 * Notification system.
 *
 * The FMAPivot object acts as a sort of "summarizing relay" for notification
 * messages sent by I/O storage providers:
 *
 * - When an I/O storage subsystem detects a change on an item it manages,
 *   action or menu, it is first supposed to do its best effort in order
 *   to summarize its notifications messages;
 *
 * - At the end of this first stage of summarization, the I/O provider
 *   should call the fma_iio_provider_item_changed() function, which
 *   itself will emit the "io-provider-item-changed" signal.
 *   This is done so that an external I/O provider does not have to know
 *   anything with the signal name, but has only to take care of calling
 *   a function of the FMAIIOProvider API.
 *
 * - The emitted signal is catched by fma_pivot_on_item_changed_handler(),
 *   which was connected when the I/O provider plugin was associated with
 *   the FMAIOProvider object.
 *
 * - The FMAPivot object receives these notifications originating from all
 *   loaded I/O providers, itself summarizes them, and only then notify its
 *   consumers with only one message for a whole set of modifications.
 *
 * It is eventually up to the consumer to connect to this signal, and
 * choose itself whether to reload items or not.
 */

#include <api/fma-iio-provider.h>
#include <api/fma-object-api.h>

#include "na-settings.h"

G_BEGIN_DECLS

#define FMA_TYPE_PIVOT                ( fma_pivot_get_type())
#define FMA_PIVOT( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_PIVOT, FMAPivot ))
#define FMA_PIVOT_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_PIVOT, FMAPivotClass ))
#define FMA_IS_PIVOT( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_PIVOT ))
#define FMA_IS_PIVOT_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_PIVOT ))
#define FMA_PIVOT_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_PIVOT, FMAPivotClass ))

typedef struct _FMAPivotPrivate       FMAPivotPrivate;

typedef struct {
	/*< private >*/
	GObject          parent;
	FMAPivotPrivate *private;
}
	FMAPivot;

typedef struct _FMAPivotClassPrivate  FMAPivotClassPrivate;

typedef struct {
	/*< private >*/
	GObjectClass          parent;
	FMAPivotClassPrivate *private;
}
	FMAPivotClass;

GType    fma_pivot_get_type( void );

/* properties
 */
#define PIVOT_PROP_LOADABLE						"pivot-prop-loadable"
#define PIVOT_PROP_TREE							"pivot-prop-tree"

/* signals
 *
 * FMAPivot acts as a 'summarizing' proxy for signals emitted by the
 * FMAIIOProvider providers when they detect a modification in their
 * underlying items storage subsystems.
 *
 * As several to many signals may be emitted when such a modification occurs,
 * FMAPivot summarizes all these signals in an only one 'items-changed' event.
 */
#define PIVOT_SIGNAL_ITEMS_CHANGED				"pivot-items-changed"

/* Loadable population
 * NACT management user interface defaults to PIVOT_LOAD_ALL
 * N-A plugin set the loadable population to !PIVOT_LOAD_DISABLED & !PIVOT_LOAD_INVALID
 */
typedef enum {
	PIVOT_LOAD_NONE     = 0,
	PIVOT_LOAD_DISABLED = 1 << 0,
	PIVOT_LOAD_INVALID  = 1 << 1,
	PIVOT_LOAD_ALL      = 0xff
}
	FMAPivotLoadableSet;

FMAPivot      *fma_pivot_new                    ( void );
void           fma_pivot_dump                   ( const FMAPivot *pivot );

/* Management of the plugins which claim to implement a FileManager-Actions interface.
 * As of 2.30, these may be FMAIIOProvider, FMAIImporter or FMAIExporter
 */
GList         *fma_pivot_get_providers          ( const FMAPivot *pivot, GType type );
void           fma_pivot_free_providers         ( GList *providers );

/* Items, menus and actions, management
 */
FMAObjectItem *fma_pivot_get_item               ( const FMAPivot *pivot, const gchar *id );
GList         *fma_pivot_get_items              ( const FMAPivot *pivot );
void           fma_pivot_load_items             ( FMAPivot *pivot );
void           fma_pivot_set_new_items          ( FMAPivot *pivot, GList *tree );

void           fma_pivot_on_item_changed_handler( FMAIIOProvider *provider, FMAPivot *pivot  );

/* FMAPivot properties and configuration
 */
void           fma_pivot_set_loadable           ( FMAPivot *pivot, guint loadable );

G_END_DECLS

#endif /* __CORE_FMA_PIVOT_H__ */
