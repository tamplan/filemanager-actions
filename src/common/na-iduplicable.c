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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "na-iduplicable.h"

/* private interface data
 */
struct NAIDuplicableInterfacePrivate {
};

/* data set against GObject
 */
#define PROP_IDUPLICABLE_ORIGIN			"na-iduplicable-origin"
#define PROP_IDUPLICABLE_ISMODIFIED		"na-iduplicable-is-modified"
#define PROP_IDUPLICABLE_ISVALID		"na-iduplicable-is-valid"

static GType     register_type( void );
static void      interface_base_init( NAIDuplicableInterface *klass );
static void      interface_base_finalize( NAIDuplicableInterface *klass );

static NAObject *v_duplicate( const NAObject *object );
static gboolean  v_are_equal( const NAObject *a, const NAObject *b );
static gboolean  v_is_valid( const NAObject *object );

static NAObject *get_origin( const NAObject *object );
static void      set_origin( const NAObject *object, const NAObject *origin );
static gboolean  get_modified( const NAObject *object );
static void      set_modified( const NAObject *object, gboolean is_modified );
static gboolean  get_valid( const NAObject *object );
static void      set_valid( const NAObject *object, gboolean is_valid );

GType
na_iduplicable_get_type( void )
{
	static GType iface_type = 0;

	if( !iface_type ){
		iface_type = register_type();
	}

	return( iface_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "na_iduplicable_register_type";
	g_debug( "%s", thisfn );

	static const GTypeInfo info = {
		sizeof( NAIDuplicableInterface ),
		( GBaseInitFunc ) interface_base_init,
		( GBaseFinalizeFunc ) interface_base_finalize,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL
	};

	GType type = g_type_register_static( G_TYPE_INTERFACE, "NAIDuplicable", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( NAIDuplicableInterface *klass )
{
	static const gchar *thisfn = "na_iduplicable_interface_base_init";
	static gboolean initialized = FALSE;

	if( !initialized ){
		g_debug( "%s: klass=%p", thisfn, klass );

		klass->private = g_new0( NAIDuplicableInterfacePrivate, 1 );

		initialized = TRUE;
	}
}

static void
interface_base_finalize( NAIDuplicableInterface *klass )
{
	static const gchar *thisfn = "na_iduplicable_interface_base_finalize";
	static gboolean finalized = FALSE ;

	if( !finalized ){
		g_debug( "%s: klass=%p", thisfn, klass );

		g_free( klass->private );

		finalized = TRUE;
	}
}

/**
 * na_iduplicable_init:
 * @object: the #NAObject object to be initialized.
 *
 * Initializes the properties of a IDuplicable object.
 *
 * This function should be called when creating the object, e.g. from
 * instance_init().
 */
void
na_iduplicable_init( NAObject *object )
{
	g_assert( NA_IS_OBJECT( object ));
	g_assert( NA_IS_IDUPLICABLE( object ));

	set_origin( object, NULL );
	set_modified( object, FALSE );
	set_valid( object, TRUE );
}

/**
 * na_iduplicable_dump:
 * @object: the #NAObject object to be duplicated.
 *
 * Dumps via g_debug the properties of the object.
 */
void
na_iduplicable_dump( const NAObject *object )
{
	static const gchar *thisfn = "na_iduplicable_dump";

	NAObject *origin = NULL;
	gboolean modified = FALSE;
	gboolean valid = TRUE;

	if( object ){
		g_assert( NA_IS_OBJECT( object ));
		g_assert( NA_IS_IDUPLICABLE( object ));

		origin = get_origin( object );
		modified = get_modified( object );
		valid = get_valid( object );
	}

	g_debug( "%s:   origin=%p", thisfn, origin );
	g_debug( "%s: modified=%s", thisfn, modified ? "True" : "False" );
	g_debug( "%s:    valid=%s", thisfn, valid ? "True" : "False" );
}

/**
 * na_iduplicable_duplicate:
 * @object: the #NAObject object to be duplicated.
 *
 * Exactly duplicates a #NAObject-derived object.
 * Properties %PROP_IDUPLICABLE_ORIGIN, %PROP_IDUPLICABLE_ISMODIFIED
 * and %PROP_IDUPLICABLE_ISVALID are initialized to their default
 * values.
 *
 * As %PROP_IDUPLICABLE_ISVALID property is set to %TRUE without any
 * further check, this suppose that only valid objects are duplicated.
 *
 * Returns: a new #NAObject.
 */
NAObject *
na_iduplicable_duplicate( const NAObject *object )
{
	static const gchar *thisfn = "na_iduplicable_duplicate";
	g_debug( "%s: object=%p", thisfn, object );

	NAObject *dup = NULL;

	if( object ){
		g_assert( NA_IS_OBJECT( object ));
		g_assert( NA_IS_IDUPLICABLE( object ));

		dup = v_duplicate( object );

		set_origin( dup, object );
		set_modified( dup, FALSE );
		set_valid( dup, TRUE );
	}

	return( dup );
}

/**
 * na_iduplicable_check_edited_status:
 * @object: the #NAObject object to be checked.
 *
 * Checks the edition status of the #NAObject object, and set up the
 * corresponding %PROP_IDUPLICABLE_ISMODIFIED and
 * %PROP_IDUPLICABLE_ISVALID properties.
 *
 * This function is supposed to be called each time the object may have
 * been modified in order to set these properties. Helper functions
 * na_iduplicable_is_modified() and na_iduplicable_is_valid() will
 * then only return the current value of the properties.
 */
void
na_iduplicable_check_edited_status( const NAObject *object )
{
	/*static const gchar *thisfn = "na_iduplicable_check_edited_status";
	g_debug( "%s: object=%p", thisfn, object );*/

	if( object ){
		g_assert( NA_IS_OBJECT( object ));
		g_assert( NA_IS_IDUPLICABLE( object ));

		gboolean modified = TRUE;
		NAObject *origin = get_origin( object );
		if( origin ){
			modified = !v_are_equal( object, origin );
		}
		set_modified( object, modified );

		gboolean valid = v_is_valid( object );
		set_valid( object, valid );
	}
}

/**
 * na_iduplicable_is_modified:
 * @object: the #NAObject object whose status is to be returned.
 *
 * Returns the current value of the %PROP_IDUPLICABLE_ISMODIFIED
 * property without rechecking the edition status itself.
 *
 * Returns: %TRUE is the provided object has been modified regarding of
 * the original one.
 */
gboolean
na_iduplicable_is_modified( const NAObject *object )
{
	/*static const gchar *thisfn = "na_iduplicable_is_modified";
	g_debug( "%s: object=%p", thisfn, object );*/

	gboolean is_modified = FALSE;

	if( object ){
		g_assert( NA_IS_OBJECT( object ));
		g_assert( NA_IS_IDUPLICABLE( object ));

		is_modified = get_modified( object );
	}

	return( is_modified );
}

/**
 * na_iduplicable_is_valid:
 * @object: the #NAObject object whose status is to be returned.
 *
 * Returns the current value of the %PROP_IDUPLICABLE_ISVALID property
 * without rechecking the edition status itself.
 *
 * Returns: %TRUE is the provided object is valid.
 */
gboolean
na_iduplicable_is_valid( const NAObject *object )
{
	/*static const gchar *thisfn = "na_iduplicable_is_valid";
	g_debug( "%s: object=%p", thisfn, object );*/

	gboolean is_valid = FALSE;

	if( object ){
		g_assert( NA_IS_OBJECT( object ));
		g_assert( NA_IS_IDUPLICABLE( object ));

		is_valid = get_valid( object );
	}

	return( is_valid );
}

/**
 * na_iduplicable_get_origin:
 * @object: the #NAObject object whose origin is to be returned.
 *
 * Returns the origin of a duplicated #NAObject.
 *
 * Returns: the original #NAObject, or NULL.
 */
NAObject *
na_iduplicable_get_origin( const NAObject *object )
{
	/*static const gchar *thisfn = "na_iduplicable_is_valid";
	g_debug( "%s: object=%p", thisfn, object );*/

	NAObject *origin = NULL;

	if( object ){
		g_assert( NA_IS_OBJECT( object ));
		g_assert( NA_IS_IDUPLICABLE( object ));

		origin = get_origin( object );
	}

	return( origin );
}

/**
 * na_iduplicable_set_origin:
 * @object: the #NAObject object whose origin is to be returned.
 * @origin: the new original #NAObject.
 *
 * Sets the new origin of a duplicated #NAObject.
 */
void
na_iduplicable_set_origin( NAObject *object, const NAObject *origin )
{
	/*static const gchar *thisfn = "na_iduplicable_is_valid";
	g_debug( "%s: object=%p", thisfn, object );*/

	if( object ){
		g_assert( NA_IS_OBJECT( object ));
		g_assert( NA_IS_IDUPLICABLE( object ));

		set_origin( object, origin );
	}
}

static NAObject *
v_duplicate( const NAObject *object )
{
	NAIDuplicable *instance = NA_IDUPLICABLE( object );

	if( NA_IDUPLICABLE_GET_INTERFACE( instance )->duplicate ){
		return( NA_IDUPLICABLE_GET_INTERFACE( instance )->duplicate( object ));
	}

	return( NULL );
}

static gboolean
v_are_equal( const NAObject *a, const NAObject *b )
{
	NAIDuplicable *instance = NA_IDUPLICABLE( a );

	if( NA_IDUPLICABLE_GET_INTERFACE( instance )->are_equal ){
		return( NA_IDUPLICABLE_GET_INTERFACE( instance )->are_equal( a, b ));
	}

	return( TRUE );
}

static gboolean
v_is_valid( const NAObject *object )
{
	NAIDuplicable *instance = NA_IDUPLICABLE( object );

	if( NA_IDUPLICABLE_GET_INTERFACE( instance )->is_valid ){
		return( NA_IDUPLICABLE_GET_INTERFACE( instance )->is_valid( object ));
	}

	return( TRUE );
}

static NAObject *
get_origin( const NAObject *object )
{
	return( NA_OBJECT( g_object_get_data( G_OBJECT( object ), PROP_IDUPLICABLE_ORIGIN )));
}

static void
set_origin( const NAObject *object, const NAObject *origin )
{
	g_object_set_data( G_OBJECT( object ), PROP_IDUPLICABLE_ORIGIN, ( gpointer ) origin );
}

static gboolean
get_modified( const NAObject *object )
{
	return(( gboolean ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( object ), PROP_IDUPLICABLE_ISMODIFIED )));
}

static void
set_modified( const NAObject *object, gboolean is_modified )
{
	g_object_set_data( G_OBJECT( object ), PROP_IDUPLICABLE_ISMODIFIED, GUINT_TO_POINTER( is_modified ));
}

static gboolean
get_valid( const NAObject *object )
{
	return(( gboolean ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( object ), PROP_IDUPLICABLE_ISVALID )));
}

static void
set_valid( const NAObject *object, gboolean is_valid )
{
	g_object_set_data( G_OBJECT( object ), PROP_IDUPLICABLE_ISVALID, GUINT_TO_POINTER( is_valid ));
}
