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

#ifndef __FILE_MANAGER_ACTIONS_API_OBJECT_ID_H__
#define __FILE_MANAGER_ACTIONS_API_OBJECT_ID_H__

/**
 * SECTION: object-id
 * @title: FMAObjectId
 * @short_description: The Identified Object Base Class Definition
 * @include: file-manager-actions/fma-object-id.h
 *
 * This is a pure virtual class, i.e. not an instantiatable one.
 * It serves as the base class for #FMAObject -derived object which have
 * a unique Id, i.e. for #FMAObjectItem and #FMAObjectProfile.
 */

#include "fma-object.h"

G_BEGIN_DECLS

#define FMA_TYPE_OBJECT_ID                ( fma_object_id_get_type())
#define FMA_OBJECT_ID( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_OBJECT_ID, FMAObjectId ))
#define FMA_OBJECT_ID_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_OBJECT_ID, FMAObjectIdClass ))
#define FMA_IS_OBJECT_ID( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_OBJECT_ID ))
#define FMA_IS_OBJECT_ID_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_OBJECT_ID ))
#define FMA_OBJECT_ID_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_OBJECT_ID, FMAObjectIdClass ))

typedef struct _FMAObjectIdPrivate        FMAObjectIdPrivate;

typedef struct {
	/*< private >*/
	FMAObject           parent;
	FMAObjectIdPrivate *private;
}
	FMAObjectId;

typedef struct _FMAObjectIdClassPrivate   FMAObjectIdClassPrivate;

/**
 * FMAObjectIdClass:
 * @new_id: Allocate a new id to an existing FMAObjectId.
 *
 * The #FMAObjectIdClass defines some methods available to derived classes.
 */
typedef struct {
	/*< private >*/
	FMAObjectClass           parent;
	FMAObjectIdClassPrivate *private;

	/*< public >*/
	/**
	 * new_id:
	 * @object: a FMAObjectId object.
	 * @new_parent: possibly the new FMAObjectId parent, or NULL.
	 * If not NULL, this should actually be a FMAObjectItem.
	 *
	 * If @object is a FMAObjectProfile, then @new_parent must be a
	 * not null FMAObjectAction. This function ensures that the new
	 * profile name does not already exist in the given @new_parent.
	 *
	 * This is a pure virtual function which should be implemented by
	 * the actual class. Actually, we asks for the most-derived class
	 * which implements this function.
	 *
	 * Returns: a new id suitable for this @object.
	 *
	 * Since: 2.30
	 */
	gchar * ( *new_id )( const FMAObjectId *object, const FMAObjectId *new_parent );
}
	FMAObjectIdClass;

GType  fma_object_id_get_type         ( void );

gint   fma_object_id_sort_alpha_asc   ( const FMAObjectId *a, const FMAObjectId *b );
gint   fma_object_id_sort_alpha_desc  ( const FMAObjectId *a, const FMAObjectId *b );

void   fma_object_id_prepare_for_paste( FMAObjectId *object, gboolean relabel, gboolean renumber, FMAObjectId *parent );
void   fma_object_id_set_copy_of_label( FMAObjectId *object );
void   fma_object_id_set_new_id       ( FMAObjectId *object, const FMAObjectId *new_parent );

G_END_DECLS

#endif /* __FILE_MANAGER_ACTIONS_API_OBJECT_ID_H__ */
