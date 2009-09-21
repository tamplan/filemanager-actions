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
	void *empty;						/* so that gcc -pedantic is happy */
};

/* data set against NAIDuplicable-implementated instance
 */
#define NA_IDUPLICABLE_PROP_ORIGIN			"na-iduplicable-origin"
#define NA_IDUPLICABLE_PROP_IS_MODIFIED		"na-iduplicable-is-modified"
#define NA_IDUPLICABLE_PROP_IS_VALID		"na-iduplicable-is-valid"

/* signals emitted on NAIDuplicable when a status changes
 */
enum {
	MODIFIED_CHANGED,
	VALID_CHANGED,
	LAST_SIGNAL
};

static gboolean st_initialized = FALSE;
static gboolean st_finalized = FALSE ;
static gint     st_signals[ LAST_SIGNAL ] = { 0 };

static GType          register_type( void );
static void           interface_base_init( NAIDuplicableInterface *klass );
static void           interface_base_finalize( NAIDuplicableInterface *klass );

static NAIDuplicable *v_new( const NAIDuplicable *object );
static void           v_copy( NAIDuplicable *target, const NAIDuplicable *source );
static gboolean       v_are_equal( const NAIDuplicable *a, const NAIDuplicable *b );
static gboolean       v_is_valid( const NAIDuplicable *object );

static gboolean       get_modified( const NAIDuplicable *object );
static NAIDuplicable *get_origin( const NAIDuplicable *object );
static gboolean       get_valid( const NAIDuplicable *object );
static void           set_modified( const NAIDuplicable *object, gboolean is_modified );
static void           set_origin( const NAIDuplicable *object, const NAIDuplicable *origin );
static void           set_valid( const NAIDuplicable *object, gboolean is_valid );

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
	GType type;

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

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_INTERFACE, "NAIDuplicable", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( NAIDuplicableInterface *klass )
{
	static const gchar *thisfn = "na_iduplicable_interface_base_init";

	if( !st_initialized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NAIDuplicableInterfacePrivate, 1 );

		/**
		 * na-iduplicable-modified-changed:
		 *
		 * This signal is emitted byIDuplicable when the modification
		 * status of an object has been modified.
		 */
		st_signals[ MODIFIED_CHANGED ] = g_signal_new(
				NA_IDUPLICABLE_SIGNAL_MODIFIED_CHANGED,
				G_TYPE_OBJECT,
				G_SIGNAL_RUN_LAST,
				0,
				NULL,
				NULL,
				g_cclosure_marshal_VOID__POINTER,
				G_TYPE_NONE,
				1,
				G_TYPE_POINTER );

		/**
		 * na-iduplicable-valid-changed:
		 *
		 * This signal is emitted byIDuplicable when the validity
		 * status of an object has been modified.
		 */
		st_signals[ VALID_CHANGED ] = g_signal_new(
				NA_IDUPLICABLE_SIGNAL_VALID_CHANGED,
				G_TYPE_OBJECT,
				G_SIGNAL_RUN_LAST,
				0,
				NULL,
				NULL,
				g_cclosure_marshal_VOID__POINTER,
				G_TYPE_NONE,
				1,
				G_TYPE_POINTER );

		st_initialized = TRUE;
	}
}

static void
interface_base_finalize( NAIDuplicableInterface *klass )
{
	static const gchar *thisfn = "na_iduplicable_interface_base_finalize";

	if( !st_finalized ){

		st_finalized = TRUE;

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );
	}
}

/**
 * na_iduplicable_init:
 * @object: the #NAIDuplicable object to be initialized.
 *
 * Initializes the properties of a IDuplicable object.
 *
 * This function should be called by the implementation when creating
 * the object, e.g. from instance_init().
 */
void
na_iduplicable_init( NAIDuplicable *object )
{
	g_return_if_fail( st_initialized && !st_finalized );
	g_return_if_fail( NA_IS_IDUPLICABLE( object ));

	set_origin( object, NULL );
	set_modified( object, FALSE );
	set_valid( object, TRUE );
}

/**
 * na_iduplicable_dump:
 * @object: the #NAIDuplicable object to be dumped.
 *
 * Dumps via g_debug the properties of the object.
 *
 * We ouput here only the data we set ourselves againt the
 * #NAIDuplicable-implemented object.
 *
 * This function should be called by the implementation when it dumps
 * itself its own content.
 */
void
na_iduplicable_dump( const NAIDuplicable *object )
{
	static const gchar *thisfn = "na_iduplicable_dump";
	NAIDuplicable *origin;
	gboolean modified;
	gboolean valid;

	g_return_if_fail( st_initialized && !st_finalized );
	g_return_if_fail( NA_IS_IDUPLICABLE( object ));

	origin = get_origin( object );
	modified = get_modified( object );
	valid = get_valid( object );

	g_debug( "%s:   origin=%p", thisfn, ( void * ) origin );
	g_debug( "%s: modified=%s", thisfn, modified ? "True" : "False" );
	g_debug( "%s:    valid=%s", thisfn, valid ? "True" : "False" );
}

/**
 * na_iduplicable_duplicate:
 * @object: the #NAIDuplicable object to be duplicated.
 *
 * Exactly duplicates a #NAIDuplicable-implemented object.
 * Properties %NA_IDUPLICABLE_PROP_ORIGIN, %PROP_IDUPLICABLE_ISMODIFIED
 * and %PROP_IDUPLICABLE_ISVALID are initialized to their default
 * values.
 *
 * As %PROP_IDUPLICABLE_ISVALID property is set to %TRUE without any
 * further check, this suppose that only valid objects are duplicated.
 *
 * Returns: a new #NAIDuplicable.
 */
NAIDuplicable *
na_iduplicable_duplicate( const NAIDuplicable *object )
{
	/*static const gchar *thisfn = "na_iduplicable_duplicate";*/
	NAIDuplicable *dup = NULL;

	/*g_debug( "%s: object=%p", thisfn, ( void * ) object );*/

	g_return_val_if_fail( st_initialized && !st_finalized, NULL );
	g_return_val_if_fail( NA_IS_IDUPLICABLE( object ), NULL );

	dup = v_new( object );

	if( dup ){
		v_copy( dup, object );
		set_origin( dup, object );
		set_modified( dup, FALSE );
		set_valid( dup, TRUE );
	}

	return( dup );
}

/**
 * na_iduplicable_check_edition_status:
 * @object: the #NAIDuplicable object to be checked.
 *
 * Checks the edition status of the #NAIDuplicable object, and set up
 * the corresponding %PROP_IDUPLICABLE_ISMODIFIED and
 * %PROP_IDUPLICABLE_ISVALID properties.
 *
 * This function is supposed to be called each time the object may have
 * been modified in order to set these properties. Helper functions
 * na_iduplicable_is_modified() and na_iduplicable_is_valid() will
 * then only return the current value of the properties.
 */
void
na_iduplicable_check_edition_status( const NAIDuplicable *object )
{
	static const gchar *thisfn = "na_iduplicable_check_edition_status";
	gboolean modified = TRUE;
	NAIDuplicable *origin;
	gboolean valid;

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));
	g_return_if_fail( st_initialized && !st_finalized );
	g_return_if_fail( NA_IS_IDUPLICABLE( object ));

	origin = get_origin( object );
	if( origin ){
		modified = !v_are_equal( origin, object );
	}
	set_modified( object, modified );

	valid = v_is_valid( object );
	set_valid( object, valid );
	g_debug( "%s: object=%p, modified=%s, valid=%s",
			thisfn, ( void * ) object, modified ? "True":"False", valid ? "True":"False" );
}

/**
 * na_iduplicable_is_modified:
 * @object: the #NAIDuplicable object whose status is to be returned.
 *
 * Returns the current value of the %PROP_IDUPLICABLE_ISMODIFIED
 * property without rechecking the edition status itself.
 *
 * Returns: %TRUE is the provided object has been modified regarding of
 * the original one.
 */
gboolean
na_iduplicable_is_modified( const NAIDuplicable *object )
{
	/*static const gchar *thisfn = "na_iduplicable_is_modified";
	g_debug( "%s: object=%p", thisfn, object );*/
	gboolean is_modified = FALSE;

	g_return_val_if_fail( st_initialized && !st_finalized, FALSE );
	g_return_val_if_fail( NA_IS_IDUPLICABLE( object ), FALSE );

	is_modified = get_modified( object );

	return( is_modified );
}

/**
 * na_iduplicable_is_valid:
 * @object: the #NAIDuplicable object whose status is to be returned.
 *
 * Returns the current value of the %PROP_IDUPLICABLE_ISVALID property
 * without rechecking the edition status itself.
 *
 * Returns: %TRUE is the provided object is valid.
 */
gboolean
na_iduplicable_is_valid( const NAIDuplicable *object )
{
	/*static const gchar *thisfn = "na_iduplicable_is_valid";
	g_debug( "%s: object=%p", thisfn, object );*/
	gboolean is_valid = FALSE;

	g_return_val_if_fail( st_initialized && !st_finalized, FALSE );
	g_return_val_if_fail( NA_IS_IDUPLICABLE( object ), FALSE );

	is_valid = get_valid( object );

	return( is_valid );
}

/**
 * na_iduplicable_get_origin:
 * @object: the #NAIDuplicable object whose origin is to be returned.
 *
 * Returns the origin of a duplicated #NAIDuplicable.
 *
 * Returns: the original #NAIDuplicable, or NULL.
 */
NAIDuplicable *
na_iduplicable_get_origin( const NAIDuplicable *object )
{
	/*static const gchar *thisfn = "na_iduplicable_is_valid";
	g_debug( "%s: object=%p", thisfn, object );*/
	NAIDuplicable *origin = NULL;

	g_return_val_if_fail( st_initialized && !st_finalized, NULL );
	g_return_val_if_fail( NA_IS_IDUPLICABLE( object ), NULL );

	origin = get_origin( object );

	return( origin );
}

/**
 * na_iduplicable_set_origin:
 * @object: the #NAIDuplicable object whose origin is to be set.
 * @origin: the new original #NAIDuplicable.
 *
 * Sets the new origin of a duplicated #NAIDuplicable.
 */
void
na_iduplicable_set_origin( NAIDuplicable *object, const NAIDuplicable *origin )
{
	/*static const gchar *thisfn = "na_iduplicable_is_valid";
	g_debug( "%s: object=%p", thisfn, object );*/

	g_return_if_fail( st_initialized && !st_finalized );
	g_return_if_fail( NA_IS_IDUPLICABLE( object ));
	g_return_if_fail( NA_IS_IDUPLICABLE( origin ) || !origin );

	set_origin( object, origin );
}

static NAIDuplicable *
v_new( const NAIDuplicable *object )
{
	if( NA_IDUPLICABLE_GET_INTERFACE( object )->new ){
		return( NA_IDUPLICABLE_GET_INTERFACE( object )->new( object ));
	}

	return( NULL );
}

static void
v_copy( NAIDuplicable *target, const NAIDuplicable *source )
{
	if( NA_IDUPLICABLE_GET_INTERFACE( target )->copy ){
		NA_IDUPLICABLE_GET_INTERFACE( target )->copy( target, source );
	}
}

static gboolean
v_are_equal( const NAIDuplicable *a, const NAIDuplicable *b )
{
	if( NA_IDUPLICABLE_GET_INTERFACE( a )->are_equal ){
		return( NA_IDUPLICABLE_GET_INTERFACE( a )->are_equal( a, b ));
	}

	return( FALSE );
}

static gboolean
v_is_valid( const NAIDuplicable *object )
{
	if( NA_IDUPLICABLE_GET_INTERFACE( object )->is_valid ){
		return( NA_IDUPLICABLE_GET_INTERFACE( object )->is_valid( object ));
	}

	return( FALSE );
}

static gboolean
get_modified( const NAIDuplicable *object )
{
	return(( gboolean ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( object ), NA_IDUPLICABLE_PROP_IS_MODIFIED )));
}

/*
 * do not use here NA_IDUPLICABLE macro as it may return a (valid) NULL
 * value
 */
static NAIDuplicable *
get_origin( const NAIDuplicable *object )
{
	/*return( NA_IDUPLICABLE( g_object_get_data( G_OBJECT( object ), NA_IDUPLICABLE_PROP_ORIGIN )));*/
	return(( NAIDuplicable * ) g_object_get_data( G_OBJECT( object ), NA_IDUPLICABLE_PROP_ORIGIN ));
}

static gboolean
get_valid( const NAIDuplicable *object )
{
	return(( gboolean ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( object ), NA_IDUPLICABLE_PROP_IS_VALID )));
}

/*
 * I'd wish simulate the behavior of GObject property which is able to
 * send a message each time a property is changed ; but these data are
 * not properties of NAObject
 */
static void
set_modified( const NAIDuplicable *object, gboolean is_modified )
{
	gboolean was_modified = get_modified( object );

	if( was_modified != is_modified ){
		g_object_set_data( G_OBJECT( object ), NA_IDUPLICABLE_PROP_IS_MODIFIED, GUINT_TO_POINTER( is_modified ));
		g_signal_emit_by_name( G_OBJECT( object ), NA_IDUPLICABLE_SIGNAL_MODIFIED_CHANGED, object );
		g_debug( "na_iduplicable_set_modified: object=%p (%s) modified=%s",
				( void * ) object, G_OBJECT_TYPE_NAME( object ), is_modified ? "True":"False" );
	}
}

static void
set_origin( const NAIDuplicable *object, const NAIDuplicable *origin )
{
	g_object_set_data( G_OBJECT( object ), NA_IDUPLICABLE_PROP_ORIGIN, ( gpointer ) origin );
}

static void
set_valid( const NAIDuplicable *object, gboolean is_valid )
{
	gboolean was_valid = get_valid( object );

	if( was_valid != is_valid ){
		g_object_set_data( G_OBJECT( object ), NA_IDUPLICABLE_PROP_IS_VALID, GUINT_TO_POINTER( is_valid ));
		g_signal_emit_by_name( G_OBJECT( object ), NA_IDUPLICABLE_SIGNAL_VALID_CHANGED, object );
	}
}
