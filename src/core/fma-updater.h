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

#ifndef __CORE_FMA_UPDATER_H__
#define __CORE_FMA_UPDATER_H__

/* @title: FMAUpdater
 * @short_description: The #FMAUpdater Class Definition
 * @include: core/fma-updater.h
 *
 * #FMAUpdater is a #FMAPivot-derived class which allows its clients
 * to update actions and menus.
 */

#include "fma-pivot.h"

G_BEGIN_DECLS

#define FMA_TYPE_UPDATER                ( fma_updater_get_type())
#define FMA_UPDATER( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_UPDATER, FMAUpdater ))
#define FMA_UPDATER_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_UPDATER, FMAUpdaterClass ))
#define FMA_IS_UPDATER( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_UPDATER ))
#define FMA_IS_UPDATER_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_UPDATER ))
#define FMA_UPDATER_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_UPDATER, FMAUpdaterClass ))

typedef struct _FMAUpdaterPrivate       FMAUpdaterPrivate;

typedef struct {
	/*< private >*/
	FMAPivot           parent;
	FMAUpdaterPrivate *private;
}
	FMAUpdater;

typedef struct _FMAUpdaterClassPrivate  FMAUpdaterClassPrivate;

typedef struct {
	/*< private >*/
	FMAPivotClass           parent;
	FMAUpdaterClassPrivate *private;
}
	FMAUpdaterClass;

GType       fma_updater_get_type( void );

FMAUpdater *fma_updater_new( void );

/* writability status
 */
void        fma_updater_check_item_writability_status( const FMAUpdater *updater, const FMAObjectItem *item );

gboolean    fma_updater_are_preferences_locked       ( const FMAUpdater *updater );
gboolean    fma_updater_is_level_zero_writable       ( const FMAUpdater *updater );

/* update the tree in memory
 */
void        fma_updater_append_item( FMAUpdater *updater, FMAObjectItem *item );
void        fma_updater_insert_item( FMAUpdater *updater, FMAObjectItem *item, const gchar *parent_id, gint pos );
void        fma_updater_remove_item( FMAUpdater *updater, FMAObject *item );

gboolean    fma_updater_should_pasted_be_relabeled( const FMAUpdater *updater, const FMAObject *item );

/* read from / write to the physical storage subsystem
 */
GList      *fma_updater_load_items ( FMAUpdater *updater );
guint       fma_updater_write_item ( const FMAUpdater *updater, FMAObjectItem *item, GSList **messages );
guint       fma_updater_delete_item( const FMAUpdater *updater, const FMAObjectItem *item, GSList **messages );

G_END_DECLS

#endif /* __CORE_FMA_UPDATER_H__ */
