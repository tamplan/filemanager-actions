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

#ifndef __FILEMANAGER_ACTIONS_API_IDUPLICABLE_H__
#define __FILEMANAGER_ACTIONS_API_IDUPLICABLE_H__

/**
 * SECTION: iduplicable
 * @title: FMAIDuplicable
 * @short_description: The Duplication Interface
 * @include: filemanager-actions/private/fma-iduplicable.h
 *
 * This interface is implemented by #FMAObject in order to let
 * #FMAObject -derived instance duplication be easily tracked. This works
 * by keeping a pointer on the original object at duplication time, and
 * then only checking edition status when explicitely required.
 *
 * As the reference count of the original object is not incremented
 * here, the caller has to garantee itself that the original object
 * will stay in life at least as long as the duplicated one.
 *
 * <refsect2>
 *  <title>
 *   Modification status in FileManager-Actions configuration tool
 *  </title>
 *  <itemizedlist>
 *   <listitem>
 *    <para>
 *     Objects whose origin is NULL are considered as modified ; this is
 *     in particular the case of new, pasted, imported and dropped
 *     objects.
 *    </para>
 *   </listitem>
 *   <listitem>
 *    <para>
 *     when a new object, whether is is really new or it has been pasted,
 *     imported or dropped, is inserted somewhere in the tree, its
 *     immediate parent is also marked as modified.
 *    </para>
 *   </listitem>
 *   <listitem>
 *    <para>
 *     Check for edition status, which positions modification and validity
 *     status, is not recursive ; it is the responsability of the
 *     implementation to check for edition status of children of object..
 *    </para>
 *   </listitem>
 *  </itemizedlist>
 * </refsect2>
 *
 * <refsect2>
 *  <title>Versions historic</title>
 *  <table>
 *    <title>Historic of the versions of the #FMAIDuplicable interface</title>
 *    <tgroup rowsep="1" colsep="1" align="center" cols="3">
 *      <colspec colname="na-version" />
 *      <colspec colname="api-version" />
 *      <colspec colname="current" />
 *      <thead>
 *        <row>
 *          <entry>&prodname; version</entry>
 *          <entry>#FMAIDuplicable interface version</entry>
 *          <entry></entry>
 *        </row>
 *      </thead>
 *      <tbody>
 *        <row>
 *          <entry>since 2.30</entry>
 *          <entry>1</entry>
 *          <entry>current version</entry>
 *        </row>
 *      </tbody>
 *    </tgroup>
 *  </table>
 * </refsect2>
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define FMA_TYPE_IDUPLICABLE                      ( fma_iduplicable_get_type())
#define FMA_IDUPLICABLE( instance )               ( G_TYPE_CHECK_INSTANCE_CAST( instance, FMA_TYPE_IDUPLICABLE, FMAIDuplicable ))
#define FMA_IS_IDUPLICABLE( instance )            ( G_TYPE_CHECK_INSTANCE_TYPE( instance, FMA_TYPE_IDUPLICABLE ))
#define FMA_IDUPLICABLE_GET_INTERFACE( instance ) ( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), FMA_TYPE_IDUPLICABLE, FMAIDuplicableInterface ))

typedef struct _FMAIDuplicable                    FMAIDuplicable;
typedef struct _FMAIDuplicableInterfacePrivate    FMAIDuplicableInterfacePrivate;

/**
 * FMAIDuplicableInterface:
 * @copy:      copies one object to another.
 * @are_equal: tests if two objects are equals.
 * @is_valid:  tests if one object is valid.
 *
 * This interface is implemented by #FMAObject objects, in order to be able
 * to keep the trace of all duplicated objects.
 */
typedef struct {
	/*< private >*/
	GTypeInterface                  parent;
	FMAIDuplicableInterfacePrivate *private;

	/*< public >*/
	/**
	 * copy:
	 * @target: the #FMAIDuplicable target of the copy.
	 * @source: the #FMAIDuplicable source of the copy.
	 * @mode: the duplication mode.
	 *
	 * Copies data from @source to @ŧarget, so that @target becomes an
	 * exact copy of @source.
	 *
	 * Each derived class of the implementation should define this
	 * function to copy its own data. The implementation should take
	 * care itself of calling each function in the class hierarchy,
	 * from topmost base class to most-derived one.
	 *
	 * Since: 2.30
	 */
	void     ( *copy )      ( FMAIDuplicable *target, const FMAIDuplicable *source, guint mode );

	/**
	 * are_equal:
	 * @a: a first #FMAIDuplicable object.
	 * @b: a second #FMAIDuplicable object to be compared to the first
	 * one.
	 *
	 * Compares the two objects.
	 *
	 * Each derived class of the implementation should define this
	 * function to compare its own data. The implementation should take
	 * care itself of calling each function in the class hierarchy,
	 * from topmost base class to most-derived one.
	 *
	 * When testing for the modification status of an object, @a stands for
	 * the original object, while @b stands for the duplicated one.
	 *
	 * Returns: TRUE if @a and @b are identical, FALSE else.
	 *
	 * Since: 2.30
	 */
	gboolean ( *are_equal ) ( const FMAIDuplicable *a, const FMAIDuplicable *b );

	/**
	 * is_valid:
	 * @object: the FMAIDuplicable object to be checked.
	 *
	 * Checks @object for validity.
	 *
	 * Each derived class of the implementation should define this
	 * function to compare its own data. The implementation should take
	 * care itself of calling each function in the class hierarchy,
	 * from topmost base class to most-derived one.
	 *
	 * Returns: TRUE if @object is valid, FALSE else.
	 *
	 * Since: 2.30
	 */
	gboolean ( *is_valid )  ( const FMAIDuplicable *object );
}
	FMAIDuplicableInterface;

#define IDUPLICABLE_SIGNAL_MODIFIED_CHANGED		"iduplicable-modified-changed"
#define IDUPLICABLE_SIGNAL_VALID_CHANGED		"iduplicable-valid-changed"

/**
 * DuplicateMode:
 * @DUPLICATE_ONLY:   only duplicates the provided object.
 * @DUPLICATE_OBJECT: only duplicate a menu
 *                    (a menu with some subitems is duplicated to an empty menu)
 * @DUPLICATE_REC:    recursively duplicates all the provided hierarchy.
 */
typedef enum {
	DUPLICATE_ONLY = 1,
	DUPLICATE_OBJECT,
	DUPLICATE_REC
}
	DuplicableMode;

GType           fma_iduplicable_get_type         ( void );

void            fma_iduplicable_dispose          ( const FMAIDuplicable *object );
void            fma_iduplicable_dump             ( const FMAIDuplicable *object );
FMAIDuplicable *fma_iduplicable_duplicate        ( const FMAIDuplicable *object, guint mode );
void            fma_iduplicable_check_status     ( const FMAIDuplicable *object );

FMAIDuplicable *fma_iduplicable_get_origin       ( const FMAIDuplicable *object );
gboolean        fma_iduplicable_is_valid         ( const FMAIDuplicable *object );
gboolean        fma_iduplicable_is_modified      ( const FMAIDuplicable *object );

void            fma_iduplicable_set_origin       ( FMAIDuplicable *object, const FMAIDuplicable *origin );

void            fma_iduplicable_register_consumer( GObject *consumer );

#ifdef NA_ENABLE_DEPRECATED
void            fma_iduplicable_set_modified( FMAIDuplicable *object, gboolean modified );
#endif

G_END_DECLS

#endif /* __FILEMANAGER_ACTIONS_API_IDUPLICABLE_H__ */
