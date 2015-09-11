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

#ifndef __FILEMANAGER_ACTIONS_API_OBJECT_H__
#define __FILEMANAGER_ACTIONS_API_OBJECT_H__

/**
 * SECTION: object
 * @title: FMAObject
 * @short_description: The Deepest Base Class Definition
 * @include: filemanager-actions/fma-object.h
 *
 * This is the base class of all our data object hierarchy. #FMAObject is
 * supposed to be used as a pure virtual base class, i.e. should only be
 * derived.
 *
 * All the API described here is rather private. External code should
 * use the API described in <filename>filemanager-actions/fma-object-api.h</filename>.
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define FMA_TYPE_OBJECT                ( fma_object_object_get_type())
#define FMA_OBJECT( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_OBJECT, FMAObject ))
#define FMA_OBJECT_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_OBJECT, FMAObjectClass ))
#define FMA_IS_OBJECT( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_OBJECT ))
#define FMA_IS_OBJECT_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_OBJECT ))
#define FMA_OBJECT_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_OBJECT, FMAObjectClass ))

typedef struct _FMAObjectPrivate       FMAObjectPrivate;

typedef struct {
	/*< private >*/
	GObject           parent;
	FMAObjectPrivate *private;
}
	FMAObject;

typedef struct _FMAObjectClassPrivate  FMAObjectClassPrivate;

/**
 * FMAObjectClass:
 * @dump:      Dumps the #FMAObject -part of the #FMAObject -derived object.
 * @copy:      Copies a #FMAObject to another.
 * @are_equal: Tests if two #FMAObject are equal.
 * @is_valid:  Tests if a #FMAObject is valid.
 *
 * The #FMAObjectClass defines some methods available to derived classes.
 */
typedef struct {
	/*< private >*/
	GObjectClass           parent;
	FMAObjectClassPrivate *private;

	/*< public >*/
	/**
	 * dump:
	 * @object: the FMAObject-derived object to be dumped.
	 *
	 * Dumps via g_debug the content of the object.
	 *
	 * The derived class should call its parent class at the end of the
	 * dump of its own datas.
	 *
	 * Since: 2.30
	 */
	void     ( *dump )     ( const FMAObject *object );

	/**
	 * copy:
	 * @target: the FMAObject-derived object which will receive data.
	 * @source: the FMAObject-derived object which provides data.
	 * @mode: the copy mode.
	 *
	 * Copies data and properties from @source to @target.
	 *
	 * The derived class should call its parent class at the end of the
	 * copy of its own datas.
	 *
	 * Since: 2.30
	 */
	void     ( *copy )     ( FMAObject *target, const FMAObject *source, guint mode );

	/**
	 * are_equal:
	 * @a: a first FMAObject object.
	 * @b: a second FMAObject object to be compared to the first one.
	 *
	 * Compares the two objects.
	 *
	 * When testing for the modification status of an object, @a stands for
	 * the original object, while @b stands for the duplicated one.
	 *
	 * As long as no difference is detected, the derived class should call
	 * its parent class at the end of its comparison.
	 * As soon as a difference is detected, the calling sequence should
	 * be stopped, and the result returned.
	 *
	 * Returns: TRUE if @a and @b are identical, FALSE else.
	 *
	 * Since: 2.30
	 */
	gboolean ( *are_equal )( const FMAObject *a, const FMAObject *b );

	/**
	 * is_valid:
	 * @object: the FMAObject object to be checked.
	 *
	 * Checks @object for validity.
	 *
	 * A FMAObject is valid if its internal identifier is set.
	 *
	 * As long as the item is valid, the derived class should call its parent
	 * at the end of its checks.
	 * As soon as an error is detected, the calling sequence should be stopped,
	 * and the result returned.
	 *
	 * Returns: TRUE if @object is valid, FALSE else.
	 *
	 * Since: 2.30
	 */
	gboolean ( *is_valid ) ( const FMAObject *object );
}
	FMAObjectClass;

GType      fma_object_object_get_type        ( void );

void       fma_object_object_check_status_rec( const FMAObject *object );

void       fma_object_object_reset_origin    ( FMAObject *object, const FMAObject *origin );

FMAObject *fma_object_object_ref             ( FMAObject *object );
void       fma_object_object_unref           ( FMAObject *object );

void       fma_object_object_dump            ( const FMAObject *object );
void       fma_object_object_dump_norec      ( const FMAObject *object );
void       fma_object_object_dump_tree       ( GList *tree );

#ifdef FMA_ENABLE_DEPRECATED
GList     *fma_object_get_hierarchy          ( const FMAObject *object );
void       fma_object_free_hierarchy         ( GList *hierarchy );
#endif

void       fma_object_object_debug_invalid   ( const FMAObject *object, const gchar *reason );

G_END_DECLS

#endif /* __FILEMANAGER_ACTIONS_API_OBJECT_H__ */
