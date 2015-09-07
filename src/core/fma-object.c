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

#include <api/fma-object-api.h>

#include "na-factory-object.h"

/* private class data
 */
struct _FMAObjectClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _FMAObjectPrivate {
	gboolean   dispose_has_run;
};

static GObjectClass *st_parent_class   = NULL;

static GType    register_type( void );
static void     class_init( FMAObjectClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_dispose( GObject *object );
static void     instance_finalize( GObject *object );

static void     object_dump( const FMAObject *object );

static void     iduplicable_iface_init( FMAIDuplicableInterface *iface, void *user_data );
static void     iduplicable_copy( FMAIDuplicable *target, const FMAIDuplicable *source, guint mode );
static gboolean iduplicable_are_equal( const FMAIDuplicable *a, const FMAIDuplicable *b );
static gboolean iduplicable_is_valid( const FMAIDuplicable *object );

static void     check_status_down_rec( const FMAObject *object );
static void     check_status_up_rec( const FMAObject *object, gboolean was_modified, gboolean was_valid );
static void     v_copy( FMAObject *target, const FMAObject *source, guint mode );
static gboolean v_are_equal( const FMAObject *a, const FMAObject *b );
static gboolean v_is_valid( const FMAObject *a );
static void     dump_tree( GList *tree, gint level );

GType
fma_object_object_get_type( void )
{
	static GType item_type = 0;

	if( item_type == 0 ){
		item_type = register_type();
	}

	return( item_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "fma_object_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( FMAObjectClass ),
		NULL,
		NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( FMAObject ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	static const GInterfaceInfo iduplicable_iface_info = {
		( GInterfaceInitFunc ) iduplicable_iface_init,
		NULL,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "FMAObject", &info, 0 );

	g_type_add_interface_static( type, FMA_TYPE_IDUPLICABLE, &iduplicable_iface_info );

	return( type );
}

static void
class_init( FMAObjectClass *klass )
{
	static const gchar *thisfn = "fma_object_class_init";
	GObjectClass *object_class;
	FMAObjectClass *naobject_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	naobject_class = FMA_OBJECT_CLASS( klass );
	naobject_class->dump = object_dump;
	naobject_class->copy = NULL;
	naobject_class->are_equal = NULL;
	naobject_class->is_valid = NULL;

	klass->private = g_new0( FMAObjectClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	FMAObject *self;

	g_return_if_fail( FMA_IS_OBJECT( instance ));

	self = FMA_OBJECT( instance );

	self->private = g_new0( FMAObjectPrivate, 1 );
}

static void
instance_dispose( GObject *object )
{
	FMAObject *self;

	g_return_if_fail( FMA_IS_OBJECT( object ));

	self = FMA_OBJECT( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		fma_iduplicable_dispose( FMA_IDUPLICABLE( object ));

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	FMAObject *self;

	g_return_if_fail( FMA_IS_OBJECT( object ));

	self = FMA_OBJECT( object );

	g_free( self->private );

	if( FMA_IS_IFACTORY_OBJECT( object )){
		na_factory_object_finalize( FMA_IFACTORY_OBJECT( object ));
	}

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

static void
object_dump( const FMAObject *object )
{
	if( !object->private->dispose_has_run ){

		fma_iduplicable_dump( FMA_IDUPLICABLE( object ));

		if( FMA_IS_IFACTORY_OBJECT( object )){
			na_factory_object_dump( FMA_IFACTORY_OBJECT( object ));
		}
	}
}

static void
iduplicable_iface_init( FMAIDuplicableInterface *iface, void *user_data )
{
	static const gchar *thisfn = "fma_object_iduplicable_iface_init";

	g_debug( "%s: iface=%p, user_data=%p", thisfn, ( void * ) iface, ( void * ) user_data );

	iface->copy = iduplicable_copy;
	iface->are_equal = iduplicable_are_equal;
	iface->is_valid = iduplicable_is_valid;
}

/*
 * implementation of fma_iduplicable::copy interface virtual function
 * it recursively copies @source to @target
 */
static void
iduplicable_copy( FMAIDuplicable *target, const FMAIDuplicable *source, guint mode )
{
	static const gchar *thisfn = "fma_object_iduplicable_copy";
	FMAObject *dest, *src;

	g_return_if_fail( FMA_IS_OBJECT( target ));
	g_return_if_fail( FMA_IS_OBJECT( source ));

	dest = FMA_OBJECT( target );
	src = FMA_OBJECT( source );

	if( !dest->private->dispose_has_run &&
		!src->private->dispose_has_run ){

		g_debug( "%s: target=%p (%s), source=%p (%s), mode=%d",
				thisfn,
				( void * ) dest, G_OBJECT_TYPE_NAME( dest ),
				( void * ) src, G_OBJECT_TYPE_NAME( src ),
				mode );

		if( FMA_IS_IFACTORY_OBJECT( target )){
			na_factory_object_copy( FMA_IFACTORY_OBJECT( target ), FMA_IFACTORY_OBJECT( source ));
		}

		if( FMA_IS_ICONTEXT( target )){
			fma_icontext_copy( FMA_ICONTEXT( target ), FMA_ICONTEXT( source ));
		}

		v_copy( dest, src, mode );
	}
}

static gboolean
iduplicable_are_equal( const FMAIDuplicable *a, const FMAIDuplicable *b )
{
	static const gchar *thisfn = "fma_object_iduplicable_are_equal";
	gboolean are_equal;

	g_return_val_if_fail( FMA_IS_OBJECT( a ), FALSE );
	g_return_val_if_fail( FMA_IS_OBJECT( b ), FALSE );

	are_equal = FALSE;

	if( !FMA_OBJECT( a )->private->dispose_has_run &&
		!FMA_OBJECT( b )->private->dispose_has_run ){

		g_debug( "%s: a=%p (%s), b=%p", thisfn, ( void * ) a, G_OBJECT_TYPE_NAME( a ), ( void * ) b );

		are_equal = TRUE;

		if( FMA_IS_IFACTORY_OBJECT( a )){
			are_equal &= na_factory_object_are_equal( FMA_IFACTORY_OBJECT( a ), FMA_IFACTORY_OBJECT( b ));
		}

		if( FMA_IS_ICONTEXT( a )){
			are_equal &= fma_icontext_are_equal( FMA_ICONTEXT( a ), FMA_ICONTEXT( b ));
		}

		are_equal &= v_are_equal( FMA_OBJECT( a ), FMA_OBJECT( b ));
	}

	return( are_equal );
}

static gboolean
iduplicable_is_valid( const FMAIDuplicable *object )
{
	static const gchar *thisfn = "fma_object_iduplicable_is_valid";
	gboolean is_valid;

	g_return_val_if_fail( FMA_IS_OBJECT( object ), FALSE );

	is_valid = FALSE;

	if( !FMA_OBJECT( object )->private->dispose_has_run ){
		g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

		is_valid = TRUE;

		if( FMA_IS_IFACTORY_OBJECT( object )){
			is_valid &= na_factory_object_is_valid( FMA_IFACTORY_OBJECT( object ));
		}

		if( FMA_IS_ICONTEXT( object )){
			is_valid &= fma_icontext_is_valid( FMA_ICONTEXT( object ));
		}

		is_valid &= v_is_valid( FMA_OBJECT( object ));
	}

	return( is_valid );
}

/**
 * fma_object_object_check_status_rec:
 * @object: the #FMAObject -derived object to be checked.
 *
 * Recursively checks for the edition status of @object and its children
 * (if any).
 *
 * Internally set some properties which may be requested later. This
 * two-steps check-request let us optimize some work in the UI.
 *
 * <literallayout>
 * fma_object_object_check_status_rec( object )
 *  +- fma_iduplicable_check_status( object )
 *      +- get_origin( object )
 *      +- modified_status = v_are_equal( origin, object )
 *      |  +-> interface <structfield>FMAObjectClass::are_equal</structfield>
 *      |      which happens to be iduplicable_are_equal( a, b )
 *      |       +- v_are_equal( a, b )
 *      |           +- FMAObjectAction::are_equal()
 *      |               +- na_factory_object_are_equal()
 *      |               +- check FMAObjectActionPrivate data
 *      |               +- call parent class
 *      |                  +- FMAObjectItem::are_equal()
 *      |                      +- check FMAObjectItemPrivate data
 *      |                      +- call parent class
 *      |                          +- FMAObjectId::are_equal()
 *      |
 *      +- valid_status = v_is_valid( object )             -> interface <structfield>FMAObjectClass::is_valid</structfield>
 * </literallayout>
 *
 *   Note that the recursivity is managed here, so that we can be sure
 *   that edition status of children is actually checked before those of
 *   the parent.
 *
 * <formalpara>
 *  <title>
 *   As of 3.1.0:
 *  </title>
 *  <para>
 *   <itemizedlist>
 *    <listitem>
 *     <para>
 *      when the modification status of a FMAObjectProfile changes, then its
 *      FMAObjectAction parent is rechecked;
 *     </para>
 *    </listitem>
 *    <listitem>
 *     <para>
 *      when the validity status of an object is changed, then its parent is
 *      also rechecked.
 *     </para>
 *    </listitem>
 *   </itemizedlist>
 *  </para>
 * </formalpara>
 *
 * Since: 2.30
 */
void
fma_object_object_check_status_rec( const FMAObject *object )
{
	static const gchar *thisfn = "fma_object_object_check_status_rec";
	gboolean was_modified, was_valid;

	g_return_if_fail( FMA_IS_OBJECT( object ));

	if( !object->private->dispose_has_run ){
		g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

		was_modified = fma_object_is_modified( object );
		was_valid = fma_object_is_valid( object );
		check_status_down_rec( object );
		check_status_up_rec( object, was_modified, was_valid );
	}
}

/*
 * recursively checks the status downstream
 */
static void
check_status_down_rec( const FMAObject *object )
{
	if( FMA_IS_OBJECT_ITEM( object )){
		g_list_foreach( fma_object_get_items( object ), ( GFunc ) check_status_down_rec, NULL );
	}

	fma_iduplicable_check_status( FMA_IDUPLICABLE( object ));
}

/*
 * if the status appears changed, then rechecks the parent
 * recurse upstream while there is a parent, and its status changes
 */
static void
check_status_up_rec( const FMAObject *object, gboolean was_modified, gboolean was_valid )
{
	gboolean is_modified, is_valid;
	FMAObjectItem *parent;

	is_modified = fma_object_is_modified( object );
	is_valid = fma_object_is_valid( object );

	if(( FMA_IS_OBJECT_PROFILE( object ) && was_modified != is_modified ) ||
			was_valid != is_valid ){

			parent = fma_object_get_parent( object );

			if( parent ){
				was_modified = fma_object_is_modified( parent );
				was_valid = fma_object_is_valid( parent );
				fma_iduplicable_check_status( FMA_IDUPLICABLE( parent ));
				check_status_up_rec( FMA_OBJECT( parent ), was_modified, was_valid );
			}
	}
}

static void
v_copy( FMAObject *target, const FMAObject *source, guint mode )
{
	if( FMA_OBJECT_GET_CLASS( target )->copy ){
		FMA_OBJECT_GET_CLASS( target )->copy( target, source, mode );
	}
}

static gboolean
v_are_equal( const FMAObject *a, const FMAObject *b )
{
	if( FMA_OBJECT_GET_CLASS( a )->are_equal ){
		return( FMA_OBJECT_GET_CLASS( a )->are_equal( a, b ));
	}

	return( TRUE );
}

static gboolean
v_is_valid( const FMAObject *a )
{
	if( FMA_OBJECT_GET_CLASS( a )->is_valid ){
		return( FMA_OBJECT_GET_CLASS( a )->is_valid( a ));
	}

	return( TRUE );
}

/**
 * fma_object_object_dump:
 * @object: the #FMAObject -derived object to be dumped.
 *
 * Dumps via g_debug() the actual content of the object.
 *
 * The recursivity is dealt with here because, if we would let
 * #FMAObjectItem do this, the dump of #FMAObjectItem -derived object
 * would be splitted, children being inserted inside.
 *
 * fma_object_dump() doesn't modify the reference count of the dumped
 * object.
 *
 * Since: 2.30
 */
void
fma_object_object_dump( const FMAObject *object )
{
	GList *children, *ic;

	g_return_if_fail( FMA_IS_OBJECT( object ));

	if( !object->private->dispose_has_run ){

		fma_object_dump_norec( object );

		if( FMA_IS_OBJECT_ITEM( object )){
			children = fma_object_get_items( object );

			for( ic = children ; ic ; ic = ic->next ){
				fma_object_dump( ic->data );
			}
		}
	}
}

/**
 * fma_object_object_dump_norec:
 * @object: the #FMAObject -derived object to be dumped.
 *
 * Dumps via g_debug the actual content of the object.
 *
 * This function is not recursive.
 *
 * Since: 2.30
 */
void
fma_object_object_dump_norec( const FMAObject *object )
{
	g_return_if_fail( FMA_IS_OBJECT( object ));

	if( !object->private->dispose_has_run ){
		if( FMA_OBJECT_GET_CLASS( object )->dump ){
			FMA_OBJECT_GET_CLASS( object )->dump( object );
		}
	}
}

/**
 * fma_object_object_dump_tree:
 * @tree: a hierarchical list of #FMAObject -derived objects.
 *
 * Outputs a brief, hierarchical dump of the provided list.
 *
 * Since: 2.30
 */
void
fma_object_object_dump_tree( GList *tree )
{
	dump_tree( tree, 0 );
}

static void
dump_tree( GList *tree, gint level )
{
	GString *prefix;
	gint i;
	GList *it;
	const FMAObject *object;
	gchar *label;

	prefix = g_string_new( "" );
	for( i = 0 ; i < level ; ++i ){
		g_string_append_printf( prefix, "  " );
	}

	for( it = tree ; it ; it = it->next ){
		object = ( const FMAObject * ) it->data;
		label = fma_object_get_label( object );
		g_debug( "fma_object_dump_tree: %s%p (%s, ref_count=%u) '%s'", prefix->str,
				( void * ) object, G_OBJECT_TYPE_NAME( object ), G_OBJECT( object )->ref_count, label );
		g_free( label );

		if( FMA_IS_OBJECT_ITEM( object )){
			dump_tree( fma_object_get_items( object ), level+1 );
		}
	}

	g_string_free( prefix, TRUE );
}

/**
 * fma_object_object_reset_origin:
 * @object: a #FMAObject -derived object.
 * @origin: must be a duplication of @object.
 *
 * Recursively reset origin of @object and its children to @origin (and
 * its children), so that @origin appears as the actual origin of @object.
 *
 * The origin of @origin itself is set to NULL.
 *
 * This only works if @origin has just been duplicated from @object,
 * and thus we do not have to check if children lists are equal.
 *
 * Since: 2.30
 */
void
fma_object_object_reset_origin( FMAObject *object, const FMAObject *origin )
{
	GList *origin_children, *iorig;
	GList *object_children, *iobj;

	g_return_if_fail( FMA_IS_OBJECT( origin ));
	g_return_if_fail( FMA_IS_OBJECT( object ));

	if( !object->private->dispose_has_run && !origin->private->dispose_has_run ){

		origin_children = fma_object_get_items( origin );
		object_children = fma_object_get_items( object );

		for( iorig = origin_children, iobj = object_children ; iorig && iobj ; iorig = iorig->next, iobj = iobj->next ){
			fma_object_reset_origin( iobj->data, iorig->data );
		}

		fma_iduplicable_set_origin( FMA_IDUPLICABLE( object ), FMA_IDUPLICABLE( origin ));
		fma_iduplicable_set_origin( FMA_IDUPLICABLE( origin ), NULL );
	}
}

/**
 * fma_object_object_ref:
 * @object: a #FMAObject -derived object.
 *
 * Recursively ref the @object and all its children, incrementing their
 * reference_count by 1.
 *
 * Returns: a reference on the @object.
 *
 * Since: 2.30
 */
FMAObject *
fma_object_object_ref( FMAObject *object )
{
	FMAObject *ref;

	g_return_val_if_fail( FMA_IS_OBJECT( object ), NULL );

	ref = NULL;

	if( !object->private->dispose_has_run ){

		if( FMA_IS_OBJECT_ITEM( object )){
			g_list_foreach( fma_object_get_items( object ), ( GFunc ) fma_object_object_ref, NULL );
		}

		ref = g_object_ref( object );
	}

	return( ref );
}

/**
 * fma_object_object_unref:
 * @object: a #FMAObject -derived object.
 *
 * Recursively unref the @object and all its children, decrementing their
 * reference_count by 1.
 *
 * Note that we may want to free a copy+ref of a list of items whichy have
 * had already disposed (which is probably a bug somewhere). So first test
 * is the object is still alive.
 *
 * Since: 2.30
 */
void
fma_object_object_unref( FMAObject *object )
{
	g_return_if_fail( FMA_IS_OBJECT( object ));

	if( !object->private->dispose_has_run ){
		if( FMA_IS_OBJECT_ITEM( object )){
			g_list_foreach( fma_object_get_items( object ), ( GFunc ) fma_object_object_unref, NULL );
		}
		g_object_unref( object );
	}
}

#ifdef NA_ENABLE_DEPRECATED
/*
 * build the class hierarchy
 * returns a list of GObjectClass, which starts with FMAObject,
 * and to with the most derived class (e.g. FMAObjectAction or so)
 */
static GList *
build_class_hierarchy( const FMAObject *object )
{
	GObjectClass *class;
	GList *hierarchy;

	hierarchy = NULL;
	class = G_OBJECT_GET_CLASS( object );

	while( G_OBJECT_CLASS_TYPE( class ) != FMA_TYPE_OBJECT ){

		hierarchy = g_list_prepend( hierarchy, class );
		class = g_type_class_peek_parent( class );
	}

	hierarchy = g_list_prepend( hierarchy, class );

	return( hierarchy );
}

/**
 * fma_object_get_hierarchy:
 * @object: the #FMAObject -derived object.
 *
 * Returns: the class hierarchy,
 * from the topmost base class, to the most-derived one.
 *
 * Since: 2.30
 * Deprecated: 3.1
 */
GList *
fma_object_get_hierarchy( const FMAObject *object )
{
	GList *hierarchy;

	g_return_val_if_fail( FMA_IS_OBJECT( object ), NULL );

	hierarchy = NULL;

	if( !object->private->dispose_has_run ){

		hierarchy = build_class_hierarchy( object );
	}

	return( hierarchy );
}

/**
 * fma_object_free_hierarchy:
 * @hierarchy: the #GList of hierarchy, as returned from
 *  fma_object_get_hierarchy().
 *
 * Releases the #FMAObject hierarchy.
 *
 * Since: 2.30
 * Deprecated: 3.1
 */
void
fma_object_free_hierarchy( GList *hierarchy )
{
	g_list_free( hierarchy );
}
#endif /* NA_ENABLE_DEPRECATED */

/**
 * fma_object_object_debug_invalid:
 * @object: the #FMAObject -derived object which is invalid.
 * @reason: the reason.
 *
 * Dump the object with the invalidity reason.
 *
 * Since: 2.30
 */
void
fma_object_object_debug_invalid( const FMAObject *object, const gchar *reason )
{
	g_debug( "fma_object_object_debug_invalid: object %p (%s) is marked invalid for reason \"%s\"",
			( void * ) object, G_OBJECT_TYPE_NAME( object ), reason );
}
