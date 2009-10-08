/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009 Pierre Wieser and others (see AUTHORS)
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

#ifndef __NA_RUNTIME_OBJECT_CLASS_H__
#define __NA_RUNTIME_OBJECT_CLASS_H__

/**
 * SECTION: na_object
 * @short_description: #NAObject class definition.
 * @include: runtime/na-object-class.h
 *
 * This is the base class for managed objects.
 *
 * It implements the #NAIDuplicable interface in order to have easily
 * duplicable derived objects. All the public API of the interface is
 * converted to #NAObject virtual functions.
 *
 * The #NAObject class is a pure virtual class.
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define NA_OBJECT_TYPE					( na_object_get_type())
#define NA_OBJECT( object )				( G_TYPE_CHECK_INSTANCE_CAST( object, NA_OBJECT_TYPE, NAObject ))
#define NA_OBJECT_CLASS( klass )		( G_TYPE_CHECK_CLASS_CAST( klass, NA_OBJECT_TYPE, NAObjectClass ))
#define NA_IS_OBJECT( object )			( G_TYPE_CHECK_INSTANCE_TYPE( object, NA_OBJECT_TYPE ))
#define NA_IS_OBJECT_CLASS( klass )		( G_TYPE_CHECK_CLASS_TYPE(( klass ), NA_OBJECT_TYPE ))
#define NA_OBJECT_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), NA_OBJECT_TYPE, NAObjectClass ))

typedef struct NAObjectPrivate NAObjectPrivate;

typedef struct {
	GObject          parent;
	NAObjectPrivate *private;
}
	NAObject;

typedef struct NAObjectClassPrivate NAObjectClassPrivate;

typedef struct {
	GObjectClass          parent;
	NAObjectClassPrivate *private;

	/**
	 * dump:
	 * @object: the #NAObject-derived object to be dumped.
	 *
	 * Dumps via g_debug the content of the object.
	 *
	 * #NAObject class takes care of calling this function for each
	 * derived class, starting from topmost base class up to most-
	 * derived one. Each derived class has so only to take care of
	 * dumping its own data.
	 */
	void       ( *dump )            ( const NAObject *object );

	/**
	 * new:
	 * @object: a #NAObject-derived object of the class that we want
	 * be returned.
	 *
	 * Returns: a newly allocated #NAObject of the same class that
	 * @object.
	 *
	 * This is a pure virtual function: the most derived class should
	 * implement it. #NAObject class defaults to return the object
	 * allocated by the most derived class which implement this
	 * function.
	 */
	NAObject * ( *new )             ( const NAObject *object );

	/**
	 * copy:
	 * @target: the #NAObject-derived object which will receive data.
	 * @source: the #NAObject-derived object which will provide data.
	 *
	 * Copies data and properties from @source to @target.
	 *
	 * Each derived class should take care of implementing this function
	 * when relevant. #NAObject class will take care of calling this
	 * function for each class of the hierarchy, starting from topmost
	 * base class up to the most-derived one. Each class has so only to
	 * take care of dumping its own data.
	 */
	void       ( *copy )            ( NAObject *target, const NAObject *source );

	/**
	 * are_equal:
	 * @a: a first #NAObject object.
	 * @b: a second #NAObject object to be compared to the first one.
	 *
	 * Compares the two objects.
	 *
	 * Returns: %TRUE if @a and @b are identical, %FALSE else.
	 *
	 * Each derived class should take care of implementing this function
	 * when relevant. #NAObject class will take care of calling this
	 * function for each class of the hierarchy, starting from topmost
	 * base class up to the most-derived one, at least while result
	 * stays at %TRUE.
	 * As soon as a difference is detected, the calling sequence will
	 * be stopped, and the result returned.
	 */
	gboolean   ( *are_equal )       ( const NAObject *a, const NAObject *b );

	/**
	 * is_valid:
	 * @object: the #NAObject object to be checked.
	 *
	 * Checks @object for validity.
	 *
	 * Returns: %TRUE if @object is valid, %FALSE else.
	 *
	 * A #NAObject is valid if its internal identifiant is set.
	 *
	 * Each derived class should take care of implementing this function
	 * when relevant. #NAObject class will take care of calling this
	 * function for each class of the hierarchy, starting from topmost
	 * base class up to the most-derived one, at least while result
	 * stays at %TRUE.
	 * As soon as a difference is detected, the calling sequence will
	 * be stopped, and the result returned.
	 */
	gboolean   ( *is_valid )        ( const NAObject *object );

	/**
	 * get_childs:
	 * @object: the #NAObject object whose childs are to be retrieved.
	 *
	 * Returns: a list of childs of @object, or NULL.
	 *
	 * As the returned list will not be freed by the caller, the
	 * implementation should really returns its own list of childs,
	 * if any.
	 */
	GList *    ( *get_childs )      ( const NAObject *object );

	/**
	 * ref:
	 * @object: the #NAObject object.
	 *
	 * Recursively ref the @object and all its childs.
	 */
	void       ( *ref )             ( NAObject *object );

	/**
	 * unref:
	 * @object: the #NAObject object.
	 *
	 * Recursively unref the @object and all its childs.
	 */
	void       ( *unref )           ( NAObject *object );
}
	NAObjectClass;

GType     na_object_get_type( void );

G_END_DECLS

#endif /* __NA_RUNTIME_OBJECT_CLASS_H__ */
