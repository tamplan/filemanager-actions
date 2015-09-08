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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "api/fma-iduplicable.h"

/* private interface data
 */
struct _FMAIDuplicableInterfacePrivate {
	GList *consumers;
};

/* the data sructure set on each FMAIDuplicable object
 */
typedef struct {
	FMAIDuplicable *origin;
	gboolean        modified;
	gboolean        valid;
}
	DuplicableStr;

#define FMA_IDUPLICABLE_DATA_DUPLICABLE			"fma-iduplicable-data-duplicable"

/* signals emitted on FMAIDuplicable when a status changes
 */
enum {
	MODIFIED_CHANGED,
	VALID_CHANGED,
	LAST_SIGNAL
};

static FMAIDuplicableInterface *st_interface = NULL;
static guint                    st_initializations = 0;
static gint                     st_signals[ LAST_SIGNAL ] = { 0 };

static GType          register_type( void );
static void           interface_base_init( FMAIDuplicableInterface *klass );
static void           interface_base_finalize( FMAIDuplicableInterface *klass );

static void           v_copy( FMAIDuplicable *target, const FMAIDuplicable *source, guint mode );
static gboolean       v_are_equal( const FMAIDuplicable *a, const FMAIDuplicable *b );
static gboolean       v_is_valid( const FMAIDuplicable *object );

static DuplicableStr *get_duplicable_str( const FMAIDuplicable *object );

static void           on_modified_changed_class_handler( FMAIDuplicable *instance, GObject *object, gboolean is_modified );
static void           on_valid_changed_class_handler( FMAIDuplicable *instance, GObject *object, gboolean is_valid );
static void           propagate_signal_to_consumers( FMAIDuplicable *instance, const gchar *signal, GObject *object, gboolean new_status );
static void           release_signal_consumers( GList *consumers );

GType
fma_iduplicable_get_type( void )
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
	static const gchar *thisfn = "fma_iduplicable_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( FMAIDuplicableInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "FMAIDuplicable", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( FMAIDuplicableInterface *klass )
{
	static const gchar *thisfn = "fma_iduplicable_interface_base_init";

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( FMAIDuplicableInterfacePrivate, 1 );

		klass->private->consumers = NULL;

		klass->copy = NULL;
		klass->are_equal = NULL;
		klass->is_valid = NULL;

		/**
		 * FMAIDuplicable::modified-changed:
		 *
		 * This signal is emitted by #FMAIDuplicable when the modification
		 * status of an object has been modified.
		 *
		 * The default class handler propagates the signal to registered
		 * consumers.
		 *
		 * Signal args: New modification status
		 *
		 * Handler prototype:
		 * void ( *handler )( FMAIDuplicable *duplicable, FMAObject *object, gboolean is_modified, gpointer user_data );
		 *
		 * When the signal is first emitted, thus on FMAIDuplicable, @duplicable
		 * and @object are pointers to the same address. This duplication is
		 * relevant when propagating the signal to customer, as the signal is
		 * emitted on the customer itself, while we still need the @object
		 * pointer.
		 */
		st_signals[ MODIFIED_CHANGED ] = g_signal_new_class_handler(
				IDUPLICABLE_SIGNAL_MODIFIED_CHANGED,
				G_TYPE_OBJECT,
				G_SIGNAL_RUN_CLEANUP,
				G_CALLBACK( on_modified_changed_class_handler ),
				NULL,
				NULL,
				NULL,
				G_TYPE_NONE,
				2,
				G_TYPE_POINTER, G_TYPE_BOOLEAN );

		/**
		 * FMAIDuplicable::valid-changed:
		 *
		 * This signal is emitted by #FMAIDuplicable when the validity
		 * status of an object has been modified.
		 *
		 * The default class handler propagates the signal to registered
		 * consumers.
		 *
		 * Signal args: New validity status
		 *
		 * Handler prototype:
		 * void ( *handler )( FMAIDuplicable *duplicable, FMAObject *object, gboolean is_valid, gpointer user_data );
		 *
		 * When the signal is first emitted, thus on FMAIDuplicable, @duplicable
		 * and @object are pointers to the same address. This duplication is
		 * relevant when propagating the signal to customer, as the signal is
		 * emitted on the customer itself, while we still need the @object
		 * pointer.
		 */
		st_signals[ VALID_CHANGED ] = g_signal_new_class_handler(
				IDUPLICABLE_SIGNAL_VALID_CHANGED,
				G_TYPE_OBJECT,
				G_SIGNAL_RUN_CLEANUP,
				G_CALLBACK( on_valid_changed_class_handler ),
				NULL,
				NULL,
				NULL,
				G_TYPE_NONE,
				2,
				G_TYPE_POINTER, G_TYPE_BOOLEAN );

		st_interface = klass;
	}

	st_initializations += 1;
}

static void
interface_base_finalize( FMAIDuplicableInterface *klass )
{
	static const gchar *thisfn = "fma_iduplicable_interface_base_finalize";

	st_initializations -= 1;

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		release_signal_consumers( klass->private->consumers );

		g_free( klass->private );
	}
}

/**
 * fma_iduplicable_dispose:
 * @object: the #FMAIDuplicable object to be initialized.
 *
 * Releases resources.
 *
 * Since: 2.30
 */
void
fma_iduplicable_dispose( const FMAIDuplicable *object )
{
	DuplicableStr *str;

	g_return_if_fail( FMA_IS_IDUPLICABLE( object ));

	str = get_duplicable_str( object );
	g_free( str );
	g_object_set_data( G_OBJECT( object ), FMA_IDUPLICABLE_DATA_DUPLICABLE, NULL );
}

/**
 * fma_iduplicable_dump:
 * @object: the #FMAIDuplicable object to be dumped.
 *
 * Dumps via g_debug the properties of the object.
 *
 * We ouput here only the data we set ourselves againt the
 * #FMAIDuplicable -implemented object.
 *
 * This function should be called by the implementation when it dumps
 * itself its own content.
 *
 * Since: 2.30
 */
void
fma_iduplicable_dump( const FMAIDuplicable *object )
{
	static const gchar *thisfn = "fma_iduplicable_dump";
	DuplicableStr *str;

	g_return_if_fail( FMA_IS_IDUPLICABLE( object ));

	str = get_duplicable_str( object );

	g_debug( "| %s:   origin=%p", thisfn, ( void * ) str->origin );
	g_debug( "| %s: modified=%s", thisfn, str->modified ? "True" : "False" );
	g_debug( "| %s:    valid=%s", thisfn, str->valid ? "True" : "False" );
}

/**
 * fma_iduplicable_duplicate:
 * @object: the #FMAIDuplicable object to be duplicated.
 * @mode: the %DuplicableMode duplication mode.
 *
 * Exactly duplicates a #FMAIDuplicable -implemented object, including
 * modification and validity status which are copied from @object to
 * the duplicated one.
 *
 * Returns: a new #FMAIDuplicable.
 *
 * Since: 2.30
 */
FMAIDuplicable *
fma_iduplicable_duplicate( const FMAIDuplicable *object, guint mode )
{
	static const gchar *thisfn = "fma_iduplicable_duplicate";
	FMAIDuplicable *dup;
	DuplicableStr *dup_str, *obj_str;

	g_return_val_if_fail( FMA_IS_IDUPLICABLE( object ), NULL );

	g_debug( "%s: object=%p (%s)",
			thisfn,
			( void * ) object, G_OBJECT_TYPE_NAME( object ));

	dup = g_object_new( G_OBJECT_TYPE( object ), NULL );

	v_copy( dup, object, mode );

	dup_str = get_duplicable_str( dup );
	obj_str = get_duplicable_str( object );

	dup_str->origin = ( FMAIDuplicable * ) object;
	dup_str->modified = obj_str->modified;
	dup_str->valid = obj_str->valid;

	return( dup );
}

/**
 * fma_iduplicable_check_status:
 * @object: the #FMAIDuplicable object to be checked.
 *
 * Checks the edition status of the #FMAIDuplicable object, and set up
 * the corresponding properties.
 *
 * This function is supposed to be called each time the object may have
 * been modified in order to set the corresponding properties. Helper
 * functions fma_iduplicable_is_modified() and fma_iduplicable_is_valid()
 * will then only return the current value of the properties.
 *
 * fma_iduplicable_check_status() is not, as itself, recursive.
 * That is, the modification and validity status are only set on the
 * specified object.
 * #FMAObject implementation has chosen to handle itself the recursivity:
 * fma_object_check_status() so first check status for children, before
 * calling this function.
 *
 * Since: 2.30
 */
void
fma_iduplicable_check_status( const FMAIDuplicable *object )
{
	static const gchar *thisfn = "fma_iduplicable_check_status";
	DuplicableStr *str;
	gboolean was_modified, was_valid;

	g_return_if_fail( FMA_IS_IDUPLICABLE( object ));

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	str = get_duplicable_str( object );

	was_modified = str->modified;
	was_valid = str->valid;

	if( str->origin ){
		g_debug( "%s: vs. origin=%p (%s)", thisfn, ( void * ) str->origin, G_OBJECT_TYPE_NAME( str->origin ));
		g_return_if_fail( FMA_IS_IDUPLICABLE( str->origin ));
		str->modified = !v_are_equal( str->origin, object );

	} else {
		str->modified = TRUE;
	}

	if( was_modified != str->modified ){
		g_debug( "%s: %p (%s) status changed to modified=%s",
				thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ), str->modified ? "True":"False" );
		g_signal_emit_by_name( G_OBJECT( object ), IDUPLICABLE_SIGNAL_MODIFIED_CHANGED, object, str->modified );
	}

	str->valid = v_is_valid( object );

	if( was_valid != str->valid ){
		g_debug( "%s: %p (%s) status changed to valid=%s",
				thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ), str->valid ? "True":"False" );
		g_signal_emit_by_name( G_OBJECT( object ), IDUPLICABLE_SIGNAL_VALID_CHANGED, object, str->valid );
	}
}

/**
 * fma_iduplicable_get_origin:
 * @object: the #FMAIDuplicable object whose origin is to be returned.
 *
 * Returns the origin of a duplicated #FMAIDuplicable.
 *
 * Returns: the original #FMAIDuplicable, or NULL.
 *
 * Since: 2.30
 */
FMAIDuplicable *
fma_iduplicable_get_origin( const FMAIDuplicable *object )
{
	FMAIDuplicable *origin;
	DuplicableStr *str;

	g_return_val_if_fail( FMA_IS_IDUPLICABLE( object ), NULL );

	str = get_duplicable_str( object );
	origin = str->origin;

	return( origin );
}

/**
 * fma_iduplicable_is_valid:
 * @object: the #FMAIDuplicable object whose status is to be returned.
 *
 * Returns the current value of the relevant property
 * without rechecking the edition status itself.
 *
 * Returns: %TRUE is the provided object is valid.
 *
 * Since: 2.30
 */
gboolean
fma_iduplicable_is_valid( const FMAIDuplicable *object )
{
	gboolean is_valid;
	DuplicableStr *str;

	g_return_val_if_fail( FMA_IS_IDUPLICABLE( object ), FALSE );

	str = get_duplicable_str( object );
	is_valid = str->valid;

	return( is_valid );
}

/**
 * fma_iduplicable_is_modified:
 * @object: the #FMAIDuplicable object whose status is to be returned.
 *
 * Returns the current value of the 'is_modified'
 * property without rechecking the edition status itself.
 *
 * Returns: %TRUE is the provided object has been modified regarding of
 * the original one.
 *
 * Since: 2.30
 */
gboolean
fma_iduplicable_is_modified( const FMAIDuplicable *object )
{
	gboolean is_modified;
	DuplicableStr *str;

	g_return_val_if_fail( FMA_IS_IDUPLICABLE( object ), FALSE );

	str = get_duplicable_str( object );
	is_modified = str->modified;

	return( is_modified );
}

/**
 * fma_iduplicable_set_origin:
 * @object: the #FMAIDuplicable object whose origin is to be set.
 * @origin: the new original #FMAIDuplicable.
 *
 * Sets the new origin of a duplicated #FMAIDuplicable.
 *
 * Since: 2.30
 */
void
fma_iduplicable_set_origin( FMAIDuplicable *object, const FMAIDuplicable *origin )
{
	DuplicableStr *str;

	g_return_if_fail( FMA_IS_IDUPLICABLE( object ));
	g_return_if_fail( FMA_IS_IDUPLICABLE( origin ) || !origin );

	str = get_duplicable_str( object );
	str->origin = ( FMAIDuplicable * ) origin;
}

#ifdef NA_ENABLE_DEPRECATED
/**
 * fma_iduplicable_set_modified:
 * @object: the #FMAIDuplicable object whose modification status is to be set.
 * @modified: the new modification status #FMAIDuplicable.
 *
 * Sets the new modification status of a duplicated #FMAIDuplicable.
 *
 * Since: 2.30
 * Deprecated: 3.1
 */
void
fma_iduplicable_set_modified( FMAIDuplicable *object, gboolean modified )
{
	DuplicableStr *str;

	g_return_if_fail( FMA_IS_IDUPLICABLE( object ));

	str = get_duplicable_str( object );
	str->modified = modified;
}
#endif /* NA_ENABLE_DEPRECATED */

static void
v_copy( FMAIDuplicable *target, const FMAIDuplicable *source, guint mode )
{
	if( FMA_IDUPLICABLE_GET_INTERFACE( target )->copy ){
		FMA_IDUPLICABLE_GET_INTERFACE( target )->copy( target, source, mode );
	}
}

static gboolean
v_are_equal( const FMAIDuplicable *a, const FMAIDuplicable *b )
{
	if( FMA_IDUPLICABLE_GET_INTERFACE( a )->are_equal ){
		return( FMA_IDUPLICABLE_GET_INTERFACE( a )->are_equal( a, b ));
	}

	return( TRUE );
}

static gboolean
v_is_valid( const FMAIDuplicable *object )
{
	if( FMA_IDUPLICABLE_GET_INTERFACE( object )->is_valid ){
		return( FMA_IDUPLICABLE_GET_INTERFACE( object )->is_valid( object ));
	}

	return( TRUE );
}

/**
 * fma_iduplicable_register_consumer:
 * @consumer: the target instance.
 *
 * This function registers a consumer, i.e. an instance to which edition
 * status signals will be propagated.
 *
 * Since: 2.30
 */
void
fma_iduplicable_register_consumer( GObject *consumer )
{
	g_return_if_fail( st_interface );

	g_debug( "fma_iduplicable_register_consumer: consumer=%p", ( void * ) consumer );

	st_interface->private->consumers = g_list_prepend( st_interface->private->consumers, consumer );
}

static void
on_modified_changed_class_handler( FMAIDuplicable *instance, GObject *object, gboolean is_modified )
{
	if( FMA_IS_IDUPLICABLE( instance )){
		propagate_signal_to_consumers( instance, IDUPLICABLE_SIGNAL_MODIFIED_CHANGED, object, is_modified );
	}
}

static void
on_valid_changed_class_handler( FMAIDuplicable *instance, GObject *object, gboolean is_valid )
{
	if( FMA_IS_IDUPLICABLE( instance )){
		propagate_signal_to_consumers( instance, IDUPLICABLE_SIGNAL_VALID_CHANGED, object, is_valid );
	}
}

static void
propagate_signal_to_consumers( FMAIDuplicable *instance, const gchar *signal, GObject *object, gboolean new_status )
{
	static const gchar *thisfn = "fma_iduplicable_propagate_signals_to_consumers";
	GList *ic;

	g_return_if_fail( FMA_IS_IDUPLICABLE( instance ));

	g_debug( "%s: instance=%p, signal=%s", thisfn, ( void * ) instance, signal );

	for( ic = st_interface->private->consumers ; ic ; ic = ic->next ){
		g_signal_emit_by_name( ic->data, signal, object, new_status );
	}
}

static void
release_signal_consumers( GList *consumers )
{
	g_list_free( consumers );
}

static DuplicableStr *
get_duplicable_str( const FMAIDuplicable *object )
{
	DuplicableStr *str;

	str = ( DuplicableStr * ) g_object_get_data( G_OBJECT( object ), FMA_IDUPLICABLE_DATA_DUPLICABLE );

	if( !str ){
		str = g_new0( DuplicableStr, 1 );

		str->origin = NULL;
		str->modified = FALSE;
		str->valid = TRUE;

		g_object_set_data( G_OBJECT( object ), FMA_IDUPLICABLE_DATA_DUPLICABLE, str );
	}

	return( str );
}
