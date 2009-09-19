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

#include <string.h>

#include "na-object-class.h"
#include "na-object-fn.h"
#include "na-iduplicable.h"

/* private class data
 */
struct NAObjectClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NAObjectPrivate {
	gboolean dispose_has_run;
};

static GObjectClass *st_parent_class = NULL;

static GType          register_type( void );
static void           class_init( NAObjectClass *klass );
static void           iduplicable_iface_init( NAIDuplicableInterface *iface );
static void           instance_init( GTypeInstance *instance, gpointer klass );
static void           instance_constructed( GObject *object );
static void           instance_dispose( GObject *object );
static void           instance_finalize( GObject *object );

static void           dump_hierarchy( const NAObject *object );
static void           do_dump( const NAObject *object );

static gchar         *most_derived_clipboard_id( const NAObject *object );

static void           ref_hierarchy( const NAObject *object );

static NAIDuplicable *iduplicable_new( const NAIDuplicable *object );
static NAObject      *most_derived_new( const NAObject *object );

static void           iduplicable_copy( NAIDuplicable *target, const NAIDuplicable *source );
static void           copy_hierarchy( NAObject *target, const NAObject *source );

static gboolean       iduplicable_are_equal( const NAIDuplicable *a, const NAIDuplicable *b );
static gboolean       are_equal_hierarchy( const NAObject *a, const NAObject *b );
static gboolean       do_are_equal( const NAObject *a, const NAObject *b );

static gboolean       iduplicable_is_valid( const NAIDuplicable *object );
static gboolean       is_valid_hierarchy( const NAObject *object );
static gboolean       do_is_valid( const NAObject *object );

static void           do_copy( NAObject *target, const NAObject *source );

static GList         *v_get_childs( const NAObject *object );
static GList         *most_derived_get_childs( const NAObject *object );

GType
na_object_get_type( void )
{
	static GType object_type = 0;

	if( !object_type ){
		object_type = register_type();
	}

	return( object_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "na_object_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NAObjectClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NAObject ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	static const GInterfaceInfo iduplicable_iface_info = {
		( GInterfaceInitFunc ) iduplicable_iface_init,
		NULL,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "NAObject", &info, 0 );

	g_type_add_interface_static( type, NA_IDUPLICABLE_TYPE, &iduplicable_iface_info );

	return( type );
}

static void
class_init( NAObjectClass *klass )
{
	static const gchar *thisfn = "na_object_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->constructed = instance_constructed;
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NAObjectClassPrivate, 1 );

	klass->dump = do_dump;
	klass->get_clipboard_id = NULL;
	klass->ref = NULL;
	klass->new = NULL;
	klass->copy = do_copy;
	klass->are_equal = do_are_equal;
	klass->is_valid = do_is_valid;
	klass->get_childs = NULL;
}

static void
iduplicable_iface_init( NAIDuplicableInterface *iface )
{
	static const gchar *thisfn = "na_object_iduplicable_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->new = iduplicable_new;
	iface->copy = iduplicable_copy;
	iface->are_equal = iduplicable_are_equal;
	iface->is_valid = iduplicable_is_valid;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "na_object_instance_init";
	NAObject *self;

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_CLASS_NAME( klass ), ( void * ) klass );
	g_return_if_fail( NA_IS_OBJECT( instance ));
	self = NA_OBJECT( instance );

	self->private = g_new0( NAObjectPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_constructed( GObject *object )
{
	/*static const gchar *thisfn = "na_object_instance_constructed";*/

	/*g_debug( "%s: object=%p", thisfn, ( void * ) object );*/
	g_return_if_fail( NA_IS_OBJECT( object ));

	na_iduplicable_init( NA_IDUPLICABLE( object ));

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->constructed ){
		G_OBJECT_CLASS( st_parent_class )->constructed( object );
	}
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "na_object_instance_dispose";
	NAObject *self;

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));
	g_return_if_fail( NA_IS_OBJECT( object ));
	self = NA_OBJECT( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	NAObject *self;

	g_return_if_fail( NA_IS_OBJECT( object ));
	self = NA_OBJECT( object );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * na_object_object_dump:
 * @object: the #NAObject-derived object to be dumped.
 *
 * Dumps via g_debug the actual content of the object.
 */
void
na_object_object_dump( const NAObject *object )
{
	g_return_if_fail( NA_IS_OBJECT( object ));
	g_return_if_fail( !object->private->dispose_has_run );

	dump_hierarchy( object );
}

static void
dump_hierarchy( const NAObject *object )
{
	GList *hierarchy, *ih;

	hierarchy = na_object_get_hierarchy( object );

	for( ih = hierarchy ; ih ; ih = ih->next ){
		if( NA_OBJECT_CLASS( ih->data )->dump ){
			NA_OBJECT_CLASS( ih->data )->dump( object );
		}
	}

	na_object_free_hierarchy( hierarchy );
}

static void
do_dump( const NAObject *object )
{
	static const char *thisfn = "na_object_do_dump";

	g_debug( "%s: object=%p", thisfn, ( void * ) object );

	na_iduplicable_dump( NA_IDUPLICABLE( object ));
}

/**
 * na_object_object_get_clipboard_id:
 * @object: the #NAObject-derived object for which we will get a id.
 *
 * Returns: a newly allocated string which contains an id for the
 * #NAobject. This id is suitable for the internal clipboard.
 *
 * The returned string should be g_free() by the caller.
 */
gchar *
na_object_object_get_clipboard_id( const NAObject *object )
{
	g_return_val_if_fail( NA_IS_OBJECT( object ), NULL );
	g_return_val_if_fail( !object->private->dispose_has_run, NULL );

	return( most_derived_clipboard_id( object ));
}

static gchar *
most_derived_clipboard_id( const NAObject *object )
{
	gchar *clipboard_id;
	GList *hierarchy, *ih;
	gboolean found;

	found = FALSE;
	clipboard_id = NULL;
	hierarchy = g_list_reverse( na_object_get_hierarchy( object ));

	for( ih = hierarchy ; ih && !found ; ih = ih->next ){
		if( NA_OBJECT_CLASS( ih->data )->get_clipboard_id ){
			clipboard_id = NA_OBJECT_CLASS( ih->data )->get_clipboard_id( object );
			found = TRUE;
		}
	}

	na_object_free_hierarchy( hierarchy );

	return( clipboard_id );
}

/**
 * TODO: get ride of this
 * na_object_object_ref:
 * @object: the #NAObject-derived object to be reffed.
 *
 * Returns: a ref on the #NAobject.
 *
 * If the object has childs, then it should also have reffed them.
 */
NAObject *
na_object_object_ref( const NAObject *object )
{
	g_return_val_if_fail( NA_IS_OBJECT( object ), NULL );
	g_return_val_if_fail( !object->private->dispose_has_run, NULL );

	ref_hierarchy( object );

	return( g_object_ref(( gpointer ) object ));
}

static void
ref_hierarchy( const NAObject *object )
{
	GList *hierarchy, *ih;

	hierarchy = na_object_get_hierarchy( object );

	for( ih = hierarchy ; ih ; ih = ih->next ){
		if( NA_OBJECT_CLASS( ih->data )->ref ){
			NA_OBJECT_CLASS( ih->data )->ref( object );
		}
	}

	na_object_free_hierarchy( hierarchy );
}

/**
 * na_object_iduplicable_duplicate:
 * @object: the #NAObject object to be dumped.
 *
 * Exactly duplicates a #NAObject-derived object.
 *
 * Returns: the new #NAObject.
 *
 *     na_object_duplicate( origin )
 *      +- na_iduplicable_duplicate( origin )
 *      |   +- dup = duplicate( origin )
 *      |   |   +- dup = v_new( object ) -> interface new()
 *      |   |   +- v_copy( dup, origin ) -> interface copy()
 *      |   |
 *      |   +- set_origin( dup, origin )
 *      |   +- set_modified( dup, FALSE )
 *      |   +- set_valid( dup, FALSE )
 *      |
 *      +- na_object_check_edited_status
 */
NAObject *
na_object_iduplicable_duplicate( const NAObject *object )
{
	NAIDuplicable *duplicate;

	g_return_val_if_fail( NA_IS_OBJECT( object ), NULL );
	g_return_val_if_fail( NA_IS_IDUPLICABLE( object ), NULL );
	g_return_val_if_fail( !object->private->dispose_has_run, NULL );

	duplicate = na_iduplicable_duplicate( NA_IDUPLICABLE( object ));

	/*g_debug( "na_object_iduplicable_duplicate: object is %s at %p, duplicate is %s at %p",
			G_OBJECT_TYPE_NAME( object ), ( void * ) object,
			duplicate ? G_OBJECT_TYPE_NAME( duplicate ) : "", ( void * ) duplicate );*/

	/*if( duplicate ){
		na_iduplicable_check_edition_status( duplicate );
	}*/

	return( NA_OBJECT( duplicate ));
}

static NAIDuplicable *
iduplicable_new( const NAIDuplicable *object )
{
	g_return_val_if_fail( NA_IS_OBJECT( object ), NULL );
	g_return_val_if_fail( !NA_OBJECT( object )->private->dispose_has_run, NULL );

	return( NA_IDUPLICABLE( most_derived_new( NA_OBJECT( object ))));
}

static NAObject *
most_derived_new( const NAObject *object )
{
	NAObject *new_object;
	GList *hierarchy, *ih;
	gboolean found;

	found = FALSE;
	new_object = NULL;
	hierarchy = g_list_reverse( na_object_get_hierarchy( object ));

	for( ih = hierarchy ; ih && !found ; ih = ih->next ){
		if( NA_OBJECT_CLASS( ih->data )->new ){
			new_object = NA_OBJECT_CLASS( ih->data )->new( object );
			found = TRUE;
		}
	}

	na_object_free_hierarchy( hierarchy );

	return( new_object );
}

/**
 * na_object_object_copy:
 * @target: the #NAObject-derived object which will receive data.
 * @source: the #NAObject-derived object which will provide data.
 *
 * Copies data and properties from @source to @target.
 */
void
na_object_object_copy( NAObject *target, const NAObject *source )
{
	g_return_if_fail( NA_IS_OBJECT( target ));
	g_return_if_fail( !target->private->dispose_has_run );
	g_return_if_fail( NA_IS_OBJECT( source ));
	g_return_if_fail( !source->private->dispose_has_run );

	copy_hierarchy( target, source );
}

static void
iduplicable_copy( NAIDuplicable *target, const NAIDuplicable *source )
{
	g_return_if_fail( NA_IS_OBJECT( target ));
	g_return_if_fail( !NA_OBJECT( target )->private->dispose_has_run );
	g_return_if_fail( NA_IS_OBJECT( source ));
	g_return_if_fail( !NA_OBJECT( source )->private->dispose_has_run );

	copy_hierarchy( NA_OBJECT( target ), NA_OBJECT( source ));
}

static void
copy_hierarchy( NAObject *target, const NAObject *source )
{
	GList *hierarchy, *ih;

	hierarchy = na_object_get_hierarchy( source );

	for( ih = hierarchy ; ih ; ih = ih->next ){
		if( NA_OBJECT_CLASS( ih->data )->copy ){
			NA_OBJECT_CLASS( ih->data )->copy( target, source );
		}
	}

	na_object_free_hierarchy( hierarchy );
}

static void
do_copy( NAObject *target, const NAObject *source )
{
	/* nothing to do here */
}

/**
 * na_object_get_hierarchy:
 *
 * Returns the class hierarchy,
 * from the topmost base class, to the most-derived one.
 */
GList *
na_object_get_hierarchy( const NAObject *object )
{
	GList *hierarchy;
	GObjectClass *class;

	hierarchy = NULL;
	class = G_OBJECT_GET_CLASS( object );

	while( G_OBJECT_CLASS_TYPE( class ) != NA_OBJECT_TYPE ){
		hierarchy = g_list_prepend( hierarchy, class );
		class = g_type_class_peek_parent( class );
	}
	hierarchy = g_list_prepend( hierarchy, class );

	return( hierarchy );
}

/**
 * na_object_free_hierarchy:
 */
void
na_object_free_hierarchy( GList *hierarchy )
{
	g_list_free( hierarchy );
}

/**
 * na_object_iduplicable_are_equal:
 * @a: a first #NAObject object.
 * @b: a second #NAObject object to be compared to the first one.
 *
 * Compares the two #NAObject objects.
 *
 * At least when it finds that @a and @b are equal, each derived
 * class should call its parent class to give it an opportunity to
 * detect a difference.
 *
 * Returns: %TRUE if @a and @b are identical, %FALSE else.
 */
gboolean
na_object_iduplicable_are_equal( const NAObject *a, const NAObject *b )
{
	g_return_val_if_fail( NA_IS_OBJECT( a ), FALSE );
	g_return_val_if_fail( !a->private->dispose_has_run, FALSE );
	g_return_val_if_fail( NA_IS_OBJECT( b ), FALSE );
	g_return_val_if_fail( !b->private->dispose_has_run, FALSE );

	return( are_equal_hierarchy( a, b ));
}

static gboolean
iduplicable_are_equal( const NAIDuplicable *a, const NAIDuplicable *b )
{
	g_return_val_if_fail( NA_IS_OBJECT( a ), FALSE );
	g_return_val_if_fail( !NA_OBJECT( a )->private->dispose_has_run, FALSE );
	g_return_val_if_fail( NA_IS_OBJECT( b ), FALSE );
	g_return_val_if_fail( !NA_OBJECT( b )->private->dispose_has_run, FALSE );

	return( are_equal_hierarchy( NA_OBJECT( a ), NA_OBJECT( b )));
}

static gboolean
are_equal_hierarchy( const NAObject *a, const NAObject *b )
{
	gboolean are_equal;
	GList *hierarchy, *ih;

	are_equal = TRUE;
	hierarchy = na_object_get_hierarchy( b );

	for( ih = hierarchy ; ih && are_equal ; ih = ih->next ){
		if( NA_OBJECT_CLASS( ih->data )->are_equal ){
			are_equal = NA_OBJECT_CLASS( ih->data )->are_equal( a, b );
		}
	}

	na_object_free_hierarchy( hierarchy );

	return( are_equal );
}

static gboolean
do_are_equal( const NAObject *a, const NAObject *b )
{
	/*g_debug( "na_object_do_are_equal: a=%s at %p, b=%s at %p",
			G_OBJECT_TYPE_NAME( a ), ( void * ) a, G_OBJECT_TYPE_NAME( b ), ( void * ) b );*/

	/* as there is no data in NAObject, they are considered here as
	 * equal is both null or both not null
	 */
	return(( a && b ) || ( !a && !b ));
}

static gboolean
iduplicable_is_valid( const NAIDuplicable *object )
{
	return( is_valid_hierarchy( NA_OBJECT( object )));
}

static gboolean
is_valid_hierarchy( const NAObject *object )
{
	gboolean is_valid;
	GList *hierarchy, *ih;

	is_valid = TRUE;
	hierarchy = na_object_get_hierarchy( object );

	for( ih = hierarchy ; ih && is_valid ; ih = ih->next ){
		if( NA_OBJECT_CLASS( ih->data )->is_valid ){
			is_valid = NA_OBJECT_CLASS( ih->data )->is_valid( object );
		}
	}

	na_object_free_hierarchy( hierarchy );

	return( is_valid );
}

static gboolean
do_is_valid( const NAObject *object )
{
	/* as there is no data in NAObject, it is always valid */
	return( object ? TRUE : FALSE );
}

/**
 * na_object_iduplicable_check_edition_status:
 * @object: the #NAObject object to be checked.
 *
 * Recursively checks for the edition status of @object and its childs
 * (if any).
 *
 * Internally set some properties which may be requested later. This
 * two-steps check-request let us optimize some work in the UI.
 *
 * na_object_check_edition_status( object )
 *  +- na_iduplicable_check_edition_status( object )
 *      +- get_origin( object )
 *      +- modified_status = v_are_equal( origin, object ) -> interface are_equal()
 *      +- valid_status = v_is_valid( object )             -> interface is_valid()
 */
void
na_object_iduplicable_check_edition_status( const NAObject *object )
{
	GList *childs, *ic;

	g_return_if_fail( NA_IS_OBJECT( object ));
	g_return_if_fail( !object->private->dispose_has_run );

	na_iduplicable_check_edition_status( NA_IDUPLICABLE( object ));

	childs = v_get_childs( object );
	for( ic = childs ; ic ; ic = ic->next ){
		na_iduplicable_check_edition_status( NA_IDUPLICABLE( ic->data ));
	}
}

/**
 * na_object_iduplicable_is_modified:
 * @object: the #NAObject object whose status is requested.
 *
 * Returns the current modification status of @object.
 *
 * This suppose that @object has been previously duplicated in order
 * to get benefits provided by the IDuplicable interface.
 *
 * This suppose also that the edition status of @object has previously
 * been checked via na_object_check_edited_status().
 *
 * Returns: %TRUE is the provided object has been modified regarding to
 * the original one, %FALSE else.
 */
gboolean
na_object_iduplicable_is_modified( const NAObject *object )
{
	g_return_val_if_fail( NA_IS_OBJECT( object ), FALSE );
	g_return_val_if_fail( !object->private->dispose_has_run, FALSE );

	return( na_iduplicable_is_modified( NA_IDUPLICABLE( object )));
}

/**
 * na_object_iduplicable_is_valid:
 * @object: the #NAObject object whose validity is to be checked.
 *
 * Gets the validity status of @object.
 *
 * Returns: %TRUE is @object is valid, %FALSE else.
 */
gboolean
na_object_iduplicable_is_valid( const NAObject *object )
{
	g_return_val_if_fail( NA_IS_OBJECT( object ), FALSE );
	g_return_val_if_fail( !object->private->dispose_has_run, FALSE );

	return( na_iduplicable_is_valid( NA_IDUPLICABLE( object )));
}

/**
 * na_object_iduplicable_get_origin:
 * @object: the #NAObject object whose status is requested.
 *
 * Returns the original object which was at the origin of @object.
 *
 * Returns: a #NAObject, or NULL.
 */
NAObject *
na_object_iduplicable_get_origin( const NAObject *object )
{
	g_return_val_if_fail( NA_IS_OBJECT( object ), NULL );
	g_return_val_if_fail( !object->private->dispose_has_run, NULL );

	return( NA_OBJECT( na_iduplicable_get_origin( NA_IDUPLICABLE( object ))));
}

/**
 * na_object_iduplicable_set_origin:
 * @object: the #NAObject object whose origin is to be set.
 * @origin: a #NAObject which will be set as the new origin of @object.
 *
 * Sets the new origin of @object.
 */
void
na_object_iduplicable_set_origin( NAObject *object, const NAObject *origin )
{
	g_return_if_fail( NA_IS_OBJECT( object ));
	g_return_if_fail( !object->private->dispose_has_run );
	g_return_if_fail( NA_IS_OBJECT( origin ) || !origin );
	g_return_if_fail( !origin || !origin->private->dispose_has_run );

	na_iduplicable_set_origin( NA_IDUPLICABLE( object ), NA_IDUPLICABLE( origin ));
}

/**
 * na_object_iduplicable_set_origin_recurse:
 * @object: the #NAObject object whose origin is to be set.
 * @origin: a #NAObject which will be set as the new origin of @object.
 *
 * Sets the new origin of @object, and of all its childs if any.
 */
void
na_object_iduplicable_set_origin_recurse( NAObject *object, const NAObject *origin )
{
	GList *childs, *ic;

	na_object_iduplicable_set_origin( object, origin );

	childs = v_get_childs( object );

	for( ic = childs ; ic ; ic = ic->next ){
		na_object_iduplicable_set_origin_recurse( NA_OBJECT( ic->data ), origin );
	}
}

static GList *
v_get_childs( const NAObject *object ){

	return( most_derived_get_childs( object ));
}

static GList *
most_derived_get_childs( const NAObject *object )
{
	GList *childs;
	GList *hierarchy, *ih;
	gboolean found;

	found = FALSE;
	childs = NULL;
	hierarchy = g_list_reverse( na_object_get_hierarchy( object ));

	for( ih = hierarchy ; ih && !found ; ih = ih->next ){
		if( NA_OBJECT_CLASS( ih->data )->get_childs ){
			childs = NA_OBJECT_CLASS( ih->data )->get_childs( object );
			found = TRUE;
		}
	}

	return( childs );
}
