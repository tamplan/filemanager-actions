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

#ifndef __NACT_IDUPLICABLE_H__
#define __NACT_IDUPLICABLE_H__

/**
 * SECTION: na_iduplicable
 * @short_description: #NAObject IDuplicable interface.
 * @include: common/na-iduplicable.h
 *
 * This interface is implemented by #NAObject in order to let
 * #NAObject-derived instance duplication be easily tracked. This works
 * by keeping a pointer on the original object at duplication time, and
 * then only checking status when explictely required.
 *
 * As the reference count of the original object is not incremented
 * here, the caller has to garantee himself that the original object
 * will stay in life at least as long as the duplicated one.
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define NA_IDUPLICABLE_TYPE							( na_iduplicable_get_type())
#define NA_IDUPLICABLE( object )					( G_TYPE_CHECK_INSTANCE_CAST( object, NA_IDUPLICABLE_TYPE, NAIDuplicable ))
#define NA_IS_IDUPLICABLE( object )					( G_TYPE_CHECK_INSTANCE_TYPE( object, NA_IDUPLICABLE_TYPE ))
#define NA_IDUPLICABLE_GET_INTERFACE( instance )	( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NA_IDUPLICABLE_TYPE, NAIDuplicableInterface ))

typedef struct NAIDuplicable NAIDuplicable;

typedef struct NAIDuplicableInterfacePrivate NAIDuplicableInterfacePrivate;

typedef struct {
	GTypeInterface                 parent;
	NAIDuplicableInterfacePrivate *private;

	/**
	 * get_new_instance:
	 * @nstance: a #NAIDuplicable instance of the klass of which we
	 * want a new instance.
	 *
	 * Returns a new instance of the same class.
	 *
	 * Returns: a newly allocated #NAIDuplicable object.
	 */
	NAIDuplicable * ( *new )      ( const NAIDuplicable *object );

	/**
	 * copy:
	 * @target: the #NAIDuplicable target of the copy.
	 * @source: the #NAIDuplicable source of the copy
	 *
	 * Copies data from @source to @ŧarget, so that @target becomes an
	 * exact copy of @source.
	 */
	void            ( *copy )     ( NAIDuplicable *target, const NAIDuplicable *source );

	/**
	 * are_equal:
	 * @a: a first #NAIDuplicable object.
	 * @b: a second #NAIDuplicable object to be compared to the first
	 * one.
	 *
	 * Compares the two objects.
	 *
	 * The implementor should define a are_equal()-equivalent virtual
	 * function so that each #NAIDuplicable-derived class be able to
	 * check for identity.
	 *
	 * Returns: %TRUE if @a and @b are identical, %FALSE else.
	 */
	gboolean        ( *are_equal )( const NAIDuplicable *a, const NAIDuplicable *b );

	/**
	 * is_valid:
	 * @object: the #NAIDuplicable object to be checked.
	 *
	 * Checks @object for validity.
	 *
	 * The implementor should define a is_valid()-equivalent virtual
	 * function so that each #NAIDuplicable-derived class be able to
	 * check for validity.
	 *
	 * Returns: %TRUE if @object is valid, %FALSE else.
	 */
	gboolean        ( *is_valid ) ( const NAIDuplicable *object );
}
	NAIDuplicableInterface;

GType          na_iduplicable_get_type( void );

void           na_iduplicable_init( NAIDuplicable *object );

void           na_iduplicable_dump( const NAIDuplicable *object );

NAIDuplicable *na_iduplicable_duplicate( const NAIDuplicable *object );

void           na_iduplicable_check_edited_status( const NAIDuplicable *object );

gboolean       na_iduplicable_is_modified( const NAIDuplicable *object );

gboolean       na_iduplicable_is_valid( const NAIDuplicable *object );

NAIDuplicable *na_iduplicable_get_origin( const NAIDuplicable *object );

void           na_iduplicable_set_origin( NAIDuplicable *object, const NAIDuplicable *origin );

G_END_DECLS

#endif /* __NA_IDUPLICABLE_H__ */
