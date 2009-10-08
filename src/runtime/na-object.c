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

#include "na-object-api.h"
#include "na-iduplicable.h"
#include "na-object-priv.h"

/* private class data
 */
struct NAObjectClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

static GObjectClass *st_parent_class = NULL;

static GType          register_type( void );
static void           class_init( NAObjectClass *klass );
static void           iduplicable_iface_init( NAIDuplicableInterface *iface );
static void           instance_init( GTypeInstance *instance, gpointer klass );
static void           instance_constructed( GObject *object );
static void           instance_dispose( GObject *object );
static void           instance_finalize( GObject *object );

static NAIDuplicable *iduplicable_new( const NAIDuplicable *object );
static void           iduplicable_copy( NAIDuplicable *target, const NAIDuplicable *source );
static gboolean       iduplicable_are_equal( const NAIDuplicable *a, const NAIDuplicable *b );
static gboolean       iduplicable_is_valid( const NAIDuplicable *object );

static GList         *v_get_childs( const NAObject *object );
static void           v_unref( NAObject *object );

static gboolean       are_equal_hierarchy( const NAObject *a, const NAObject *b );
static void           copy_hierarchy( NAObject *target, const NAObject *source );
static gboolean       do_are_equal( const NAObject *a, const NAObject *b );
static void           do_copy( NAObject *target, const NAObject *source );
static void           do_dump( const NAObject *object );
static gboolean       do_is_valid( const NAObject *object );
static void           dump_hierarchy( const NAObject *object );
static void           dump_tree( GList *tree, gint level );
static gboolean       is_valid_hierarchy( const NAObject *object );
static NAObject      *most_derived_new( const NAObject *object );

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
	klass->new = NULL;
	klass->copy = do_copy;
	klass->are_equal = do_are_equal;
	klass->is_valid = do_is_valid;
	klass->get_childs = NULL;
	klass->unref = NULL;
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

		na_iduplicable_dispose( NA_IDUPLICABLE( object ));

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
 * na_object_iduplicable_duplicate:
 * @object: the #NAObject object to be dumped.
 *
 * Exactly duplicates a #NAObject-derived object.
 *
 * Returns: the new #NAObject.
 *
 *   na_object_duplicate( origin )
 *   +- na_object_iduplicable_duplicate( origin )
 *      +- na_iduplicable_duplicate( origin )
 *         +- dup = v_new( object )
 *         |  +- interface->new( object)
 *         |     +- iduplicable_new( object )
 *         |        +- most_derived_new( object )
 *         |           +- object_new( ... ) from a derived class
 *         +- v_copy( dup, origin )
 *         |  +- interface->copy( dup, origin )
 *         |     +- iduplicable_copy( target, source )
 *         |        +- copy_hierarchy( target, source )
 *         |           +- object_copy( ... ) from each successive derived class
 *         +- set_origin( dup, origin )
 *         +- set_modified( dup, FALSE )
 *         +- set_valid( dup, FALSE )
 *
 * Though the interface api is not recursive per se, the implementation
 * is ; i.e. duplicating a #NAObjectItem also duplicates the whole tree
 * inside.
 */
NAObject *
na_object_iduplicable_duplicate( const NAObject *object )
{
	NAIDuplicable *duplicate = NULL;

	g_return_val_if_fail( NA_IS_OBJECT( object ), NULL );
	g_return_val_if_fail( NA_IS_IDUPLICABLE( object ), NULL );

	if( !object->private->dispose_has_run ){

		duplicate = na_iduplicable_duplicate( NA_IDUPLICABLE( object ));

		/*g_debug( "na_object_iduplicable_duplicate: object=%p (%s), duplicate=%p (%s)",
				( void * ) object, G_OBJECT_TYPE_NAME( object ),
				( void * ) duplicate, duplicate ? G_OBJECT_TYPE_NAME( duplicate ) : "" );*/
	}

	/* do not use NA_OBJECT macro as we may return a (valid) NULL value */
	return(( NAObject * ) duplicate );
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
	gboolean are_equal = FALSE;

	g_return_val_if_fail( NA_IS_OBJECT( a ), FALSE );
	g_return_val_if_fail( NA_IS_OBJECT( b ), FALSE );

	if( !a->private->dispose_has_run && !b->private->dispose_has_run ){
		are_equal = are_equal_hierarchy( a, b );
	}

	return( are_equal );
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
	gboolean is_modified = FALSE;

	g_return_val_if_fail( NA_IS_OBJECT( object ), FALSE );

	if( !object->private->dispose_has_run ){
		is_modified = na_iduplicable_is_modified( NA_IDUPLICABLE( object ));
	}

	return( is_modified );
}

/**
 * na_object_object_dump:
 * @object: the #NAObject-derived object to be dumped.
 *
 * Dumps via g_debug the actual content of the object.
 *
 * The recursivity is dealt with here. If we let #NAObjectItem do this,
 * the dump of #NAObjectItem-derived object will be splitted, childs
 * being inserted inside.
 *
 * na_object_dump() doesn't modify the reference count of the dumped
 * object.
 */
void
na_object_object_dump( const NAObject *object )
{
	GList *childs, *ic;

	g_return_if_fail( NA_IS_OBJECT( object ));

	if( !object->private->dispose_has_run ){

		na_object_object_dump_norec( object );

		childs = v_get_childs( object );
		for( ic = childs ; ic ; ic = ic->next ){
			na_object_object_dump( NA_OBJECT( ic->data ));
		}
	}
}

/**
 * na_object_object_dump_norec:
 * @object: the #NAObject-derived object to be dumped.
 *
 * Dumps via g_debug the actual content of the object.
 *
 * This function is not recursive.
 */
void
na_object_object_dump_norec( const NAObject *object )
{
	g_return_if_fail( NA_IS_OBJECT( object ));

	if( !object->private->dispose_has_run ){
		dump_hierarchy( object );
	}
}

/**
 * na_object_object_dump_tree:
 * @tree: a hierarchical list of #NAObject-derived objects.
 *
 * Outputs a brief, hierarchical dump of the provided list.
 */
void
na_object_object_dump_tree( GList *tree )
{
	dump_tree( tree, 0 );
}

/**
 * na_object_object_unref:
 * @object: a #NAObject-derived object.
 *
 * Recursively unref the @object and all its childs, decrementing their
 * reference_count by 1.
 */
void
na_object_object_unref( NAObject *object )
{
	g_return_if_fail( NA_IS_OBJECT( object ));

	if( !object->private->dispose_has_run ){

		g_debug( "na_object_object_unref: object=%p (%s, ref_count=%d)",
				( void * ) object, G_OBJECT_TYPE_NAME( object ), G_OBJECT( object )->ref_count );

		v_unref( object );

		g_debug( "na_object_object_unref: unreffing %p (%s, ref_count=%d)",
				( void * ) object, G_OBJECT_TYPE_NAME( object ), G_OBJECT( object )->ref_count );

		g_object_unref( object );
	}
}

/**
 * na_object_most_derived_get_childs:
 * @object: this #NAObject instance.
 *
 * Returns: the list of childs as returned by the most derived class
 * which implements the virtual function 'get_childs'.
 *
 * The returned list should be g_list_free() by the caller.
 */
GList *
na_object_most_derived_get_childs( const NAObject *object )
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

/**
 * na_object_get_hierarchy:
 *
 * Returns the class hierarchy,
 * from the topmost base class, to the most-derived one.
 */
GList *
na_object_get_hierarchy( const NAObject *object )
{
	GList *hierarchy = NULL;
	GObjectClass *class;

	g_return_val_if_fail( NA_IS_OBJECT( object ), NULL );

	if( !object->private->dispose_has_run ){

		class = G_OBJECT_GET_CLASS( object );

		while( G_OBJECT_CLASS_TYPE( class ) != NA_OBJECT_TYPE ){
			hierarchy = g_list_prepend( hierarchy, class );
			class = g_type_class_peek_parent( class );
		}

		hierarchy = g_list_prepend( hierarchy, class );
	}

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

static NAIDuplicable *
iduplicable_new( const NAIDuplicable *object )
{
	NAIDuplicable *new_object = NULL;

	g_return_val_if_fail( NA_IS_OBJECT( object ), NULL );

	if( !NA_OBJECT( object )->private->dispose_has_run ){
		/* do not use NA_IDUPLICABLE macro as we may return a (valid) NULL value */
		new_object = ( NAIDuplicable * ) most_derived_new( NA_OBJECT( object ));
	}

	return( new_object );
}

static void
iduplicable_copy( NAIDuplicable *target, const NAIDuplicable *source )
{
	g_return_if_fail( NA_IS_OBJECT( target ));
	g_return_if_fail( NA_IS_OBJECT( source ));

	if( !NA_OBJECT( source )->private->dispose_has_run &&
		!NA_OBJECT( target )->private->dispose_has_run ){

			copy_hierarchy( NA_OBJECT( target ), NA_OBJECT( source ));
	}
}

static gboolean
iduplicable_are_equal( const NAIDuplicable *a, const NAIDuplicable *b )
{
	gboolean are_equal = FALSE;

	g_return_val_if_fail( NA_IS_OBJECT( a ), FALSE );
	g_return_val_if_fail( NA_IS_OBJECT( b ), FALSE );

	if( !NA_OBJECT( a )->private->dispose_has_run &&
		!NA_OBJECT( b )->private->dispose_has_run ){

		are_equal = are_equal_hierarchy( NA_OBJECT( a ), NA_OBJECT( b ));
	}

	return( are_equal );
}

static gboolean
iduplicable_is_valid( const NAIDuplicable *object )
{
	gboolean is_valid = FALSE;

	if( !NA_OBJECT( object )->private->dispose_has_run ){
		is_valid = is_valid_hierarchy( NA_OBJECT( object ));
	}

	return( is_valid );
}

static GList *
v_get_childs( const NAObject *object ){

	return( na_object_most_derived_get_childs( object ));
}

static void
v_unref( NAObject *object )
{
	if( NA_OBJECT_GET_CLASS( object )->unref ){
		NA_OBJECT_GET_CLASS( object )->unref( object );
	}
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

static gboolean
do_are_equal( const NAObject *a, const NAObject *b )
{
	gboolean are_equal;

	/* as there is no data in NAObject, they are considered here as
	 * equal is both null or both not null
	 */
	are_equal = ( a && b ) || ( !a && !b );

#if NA_IDUPLICABLE_EDITION_STATUS_DEBUG
	g_debug( "na_object_do_are_equal: a=%p (%s), b=%p (%s), are_equal=%s",
			( void * ) a, G_OBJECT_TYPE_NAME( a ),
			( void * ) b, G_OBJECT_TYPE_NAME( b ),
			are_equal ? "True":"False" );
#endif

	return( are_equal );
}

static void
do_copy( NAObject *target, const NAObject *source )
{
	/* nothing to do here */
}

static void
do_dump( const NAObject *object )
{
	static const char *thisfn = "na_object_do_dump";

	g_debug( "%s: object=%p (%s, ref_count=%d)", thisfn,
			( void * ) object, G_OBJECT_TYPE_NAME( object ), G_OBJECT( object )->ref_count );

	na_iduplicable_dump( NA_IDUPLICABLE( object ));
}

static gboolean
do_is_valid( const NAObject *object )
{
	/* as there is no data in NAObject, it is always valid */
	return( object ? TRUE : FALSE );
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
dump_tree( GList *tree, gint level )
{
	GString *prefix;
	gint i;
	GList *subitems, *it;
	gchar *id;
	gchar *label;

	prefix = g_string_new( "" );
	for( i = 0 ; i < level ; ++i ){
		g_string_append_printf( prefix, "  " );
	}

	for( it = tree ; it ; it = it->next ){
		id = na_object_get_id( it->data );
		label = na_object_get_label( it->data );
		g_debug( "na_object_dump_tree: %s%p (%s) %s \"%s\"",
				prefix->str, ( void * ) it->data, G_OBJECT_TYPE_NAME( it->data ), id, label );
		g_free( id );
		g_free( label );

		if( NA_IS_OBJECT_ITEM( it->data )){
			subitems = na_object_get_items_list( it->data );
			dump_tree( subitems, level+1 );
		}
	}

	g_string_free( prefix, TRUE );
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
