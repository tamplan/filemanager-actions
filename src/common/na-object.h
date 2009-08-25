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

#ifndef __NA_OBJECT_H__
#define __NA_OBJECT_H__

/**
 * SECTION: na_object
 * @short_description: #NAObject class definition.
 * @include: common/na-object.h
 *
 * This is the base class for NAAction and NActionProfile.
 *
 * It implements the NAIDuplicable interface in order to have easily
 * duplicable derived objects.
 *
 * A #NAObject object is characterized by :
 * - an internal identifiant (ASCII, case insensitive)
 * - a libelle (UTF8, localizable).
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
	 * In order to get a down-to-top display, the derived class
	 * implementation should call its parent class before actually
	 * dumping its own data and properties.
	 */
	void       ( *dump )               ( const NAObject *object );

	/**
	 * check_edited_status:
	 * @object: the #NAObject-derived object to be checked.
	 *
	 * Checks a #NAObject-derived object for modification and validity
	 * status.
	 */
	void       ( *check_edited_status )( const NAObject *object );

	/**
	 * duplicate:
	 * @object: the #NAObject-derived object to be dumped.
	 *
	 * Duplicates a #NAObject-derived object.
	 *
	 * As the most-derived class will actually allocate the new object
	 * with the right class, it shouldn't call its parent class.
	 *
	 * Copying data and properties should then be done via the
	 * na_object_copy() function.
	 *
	 * Returns: a newly allocated object, which is an exact copy of
	 * @object.
	 */
	NAObject * ( *duplicate )          ( const NAObject *object );

	/**
	 * copy:
	 * @target: the #NAObject-derived object which will receive data.
	 * @source: the #NAObject-derived object which will provide data.
	 *
	 * Copies data and properties from @source to @target.
	 *
	 * Each derived class should take care of calling its parent class
	 * to complete the copy.
	 */
	void       ( *copy )               ( NAObject *target, const NAObject *source );

	/**
	 * are_equal:
	 * @a: a first #NAObject object.
	 * @b: a second #NAObject object to be compared to the first one.
	 *
	 * Compares the two objects.
	 *
	 * At least when it finds that @a and @b are equal, each derived
	 * class should call its parent class to give it an opportunity to
	 * detect a difference.
	 *
	 * Returns: %TRUE if @a and @b are identical, %FALSE else.
	 */
	gboolean   ( *are_equal )          ( const NAObject *a, const NAObject *b );

	/**
	 * is_valid:
	 * @object: the #NAObject object to be checked.
	 *
	 * Checks @object for validity.
	 *
	 * At least when it finds that @object is valid, each derived class
	 * should call its parent class to give it an opportunity to detect
	 * an error.
	 *
	 * A #NAObject is valid if its internal identifiant is set.
	 *
	 * Returns: %TRUE if @object is valid, %FALSE else.
	 */
	gboolean   ( *is_valid )           ( const NAObject *object );
}
	NAObjectClass;

/* object properties
 * used in derived classes to access to the properties
 */
enum {
	PROP_NAOBJECT_ID = 1,
	PROP_NAOBJECT_LABEL
};

GType     na_object_get_type( void );

void      na_object_dump( const NAObject *object );
NAObject *na_object_duplicate( const NAObject *object );
void      na_object_copy( NAObject *target, const NAObject *source );

void      na_object_check_edited_status( const NAObject *object );
gboolean  na_object_are_equal( const NAObject *a, const NAObject *b );
gboolean  na_object_is_valid( const NAObject *object );

NAObject *na_object_get_origin( const NAObject *object );
gboolean  na_object_get_modified_status( const NAObject *object );
gboolean  na_object_get_valid_status( const NAObject *object );

void      na_object_set_origin( NAObject *object, const NAObject *origin );

gchar    *na_object_get_id( const NAObject *object );
gchar    *na_object_get_label( const NAObject *object );

void      na_object_set_id( NAObject *object, const gchar *id );
void      na_object_set_label( NAObject *object, const gchar *label );

G_END_DECLS

#endif /* __NA_OBJECT_H__ */
