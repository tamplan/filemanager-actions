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

#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>

#include <api/fma-core-utils.h>
#include <api/fma-object-api.h>

#include "fma-io-provider.h"

/* private class data
 */
struct _FMAObjectItemClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _FMAObjectItemPrivate {
	gboolean   dispose_has_run;

	/* set at load time
	 * takes into account the runtime 'readonly' status as well as the i/o
	 * provider writability status - does not consider the level-zero case
	 */
	gboolean   writable;
	guint      reason;
};

static FMAObjectIdClass *st_parent_class = NULL;

static GType    register_type( void );
static void     class_init( FMAObjectItemClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_dispose( GObject *object );
static void     instance_finalize( GObject *object );

static void     object_dump( const FMAObject *object );
static void     object_copy( FMAObject*target, const FMAObject *source, guint mode );
static gboolean object_are_equal( const FMAObject *a, const FMAObject *b );
static gboolean object_is_valid( const FMAObject *object );

static gchar   *object_id_new_id( const FMAObjectId *item, const FMAObjectId *new_parent );

static void     count_items_rec( GList *items, gint *menus, gint *actions, gint *profiles, gboolean recurse );
static GSList  *get_children_slist( const FMAObjectItem *item );
static void     copy_children( FMAObjectItem *target, const FMAObjectItem *source, guint mode );

GType
fma_object_item_get_type( void )
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
	static const gchar *thisfn = "fma_object_item_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( FMAObjectItemClass ),
		NULL,
		NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( FMAObjectItem ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( FMA_TYPE_OBJECT_ID, "FMAObjectItem", &info, 0 );

	return( type );
}

static void
class_init( FMAObjectItemClass *klass )
{
	static const gchar *thisfn = "fma_object_item_class_init";
	GObjectClass *object_class;
	FMAObjectClass *naobject_class;
	FMAObjectIdClass *naobjectid_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	naobject_class = FMA_OBJECT_CLASS( klass );
	naobject_class->dump = object_dump;
	naobject_class->copy = object_copy;
	naobject_class->are_equal = object_are_equal;
	naobject_class->is_valid = object_is_valid;

	naobjectid_class = FMA_OBJECT_ID_CLASS( klass );
	naobjectid_class->new_id = object_id_new_id;

	klass->private = g_new0( FMAObjectItemClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	FMAObjectItem *self;

	g_return_if_fail( FMA_IS_OBJECT_ITEM( instance ));

	self = FMA_OBJECT_ITEM( instance );

	self->private = g_new0( FMAObjectItemPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->writable = TRUE;
	self->private->reason = 0;
}

static void
instance_dispose( GObject *object )
{
	FMAObjectItem *self;
	GList *children;

	g_return_if_fail( FMA_IS_OBJECT_ITEM( object ));

	self = FMA_OBJECT_ITEM( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		children = fma_object_get_items( self );
		fma_object_set_items( self, NULL );
		fma_object_free_items( children );

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	FMAObjectItem *self;

	g_return_if_fail( FMA_IS_OBJECT_ITEM( object ));

	self = FMA_OBJECT_ITEM( object );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

static void
object_dump( const FMAObject *object )
{
	static const gchar *thisfn = "fma_object_item_object_dump";
	FMAObjectItem *item;

	g_return_if_fail( FMA_IS_OBJECT_ITEM( object ));

	item = FMA_OBJECT_ITEM( object );

	if( !item->private->dispose_has_run ){

		g_debug( "| %s:      writable=%s", thisfn, item->private->writable ? "True":"False" );
		g_debug( "| %s:        reason=%u", thisfn, item->private->reason );

		/* chain up to the parent class */
		if( FMA_OBJECT_CLASS( st_parent_class )->dump ){
			FMA_OBJECT_CLASS( st_parent_class )->dump( object );
		}
	}
}

static void
object_copy( FMAObject *target, const FMAObject *source, guint mode )
{
	static const gchar *thisfn = "fma_object_item_object_copy";
	void *provider;
	FMAObjectItem *dest, *src;

	g_return_if_fail( FMA_IS_OBJECT_ITEM( target ));
	g_return_if_fail( FMA_IS_OBJECT_ITEM( source ));

	dest = FMA_OBJECT_ITEM( target );
	src = FMA_OBJECT_ITEM( source );

	if( !dest->private->dispose_has_run && !src->private->dispose_has_run ){

		if( mode == DUPLICATE_REC ||
			( mode == DUPLICATE_OBJECT && G_OBJECT_TYPE( source ) == FMA_TYPE_OBJECT_ACTION )){

				copy_children( dest, src, mode );
		}

		provider = fma_object_get_provider( source );

		if( provider ){
			if( !FMA_IS_IO_PROVIDER( provider )){
				g_warning( "%s: source=%p (%s), provider=%p is not a FMAIOProvider",
						thisfn,
						( void * ) source, G_OBJECT_TYPE_NAME( source ),
						( void * ) provider );

			} else {
				fma_io_provider_duplicate_data( FMA_IO_PROVIDER( provider ), FMA_OBJECT_ITEM( target ), FMA_OBJECT_ITEM( source ), NULL );
			}
		}

		dest->private->writable = src->private->writable;
		dest->private->reason = src->private->reason;

		/* chain up to the parent class */
		if( FMA_OBJECT_CLASS( st_parent_class )->copy ){
			FMA_OBJECT_CLASS( st_parent_class )->copy( target, source, mode );
		}
	}
}

/*
 * object_are_equal:
 * @a: the first (original) #FMAObjectItem instance.
 * @b: the second #FMAObjectItem instance.
 *
 * This function participates to the #fma_iduplicable_check_status() stack,
 * and is triggered after all comparable elementary data (in #FMAIFactoryObject
 * sense) have already been successfully compared.
 *
 * We have to deal here with the subitems: comparing children by their ids
 * between @a and @b.
 *
 * Returns: %TRUE if @a is equal to @b.
 */
static gboolean
object_are_equal( const FMAObject *a, const FMAObject *b )
{
	static const gchar *thisfn = "fma_object_item_object_are_equal";
	gboolean are_equal;
	FMAObjectItem *origin, *duplicate;
	GSList *a_slist, *b_slist;
	gchar *a_list, *b_list;

	g_return_val_if_fail( FMA_IS_OBJECT_ITEM( a ), FALSE );
	g_return_val_if_fail( FMA_IS_OBJECT_ITEM( b ), FALSE );

	are_equal = FALSE;
	origin = FMA_OBJECT_ITEM( a );
	duplicate = FMA_OBJECT_ITEM( b );

	if( !origin->private->dispose_has_run &&
		!duplicate->private->dispose_has_run ){

		g_debug( "%s: a=%p, b=%p", thisfn, ( void * ) a, ( void * ) b );

		a_slist = get_children_slist( origin );
		a_list = fma_core_utils_slist_join_at_end( a_slist, ";" );
		fma_core_utils_slist_free( a_slist );

		b_slist = get_children_slist( duplicate );
		b_list = fma_core_utils_slist_join_at_end( b_slist, ";" );
		fma_core_utils_slist_free( b_slist );

		are_equal = ( strcmp( a_list, b_list ) == 0 );

		g_free( a_list );
		g_free( b_list );
	}

	/* chain call to parent class */
	if( FMA_OBJECT_CLASS( st_parent_class )->are_equal ){
		are_equal &= FMA_OBJECT_CLASS( st_parent_class )->are_equal( a, b );
	}

	return( are_equal );
}

/*
 * must have at least one valid subitem
 */
static gboolean
object_is_valid( const FMAObject *object )
{
	static const gchar *thisfn = "fma_object_item_object_is_valid";
	gboolean is_valid;
	FMAObjectItem *item;
	GList *children, *ic;
	gint valid_children;

	g_return_val_if_fail( FMA_IS_OBJECT_ITEM( object ), FALSE );

	is_valid = FALSE;
	item = FMA_OBJECT_ITEM( object );

	if( !item->private->dispose_has_run ){
		g_debug( "%s: item=%p (%s)", thisfn, ( void * ) item, G_OBJECT_TYPE_NAME( item ));

		is_valid = TRUE;

		valid_children = 0;
		children = fma_object_get_items( item );
		for( ic = children ; ic && !valid_children ; ic = ic->next ){
			if( fma_object_is_valid( ic->data )){
				valid_children += 1;
			}
		}

		is_valid &= ( valid_children > 0 );

		if( !is_valid ){
			fma_object_debug_invalid( item, "no valid child" );
		}
	}

	/* chain up to the parent class */
	if( FMA_OBJECT_CLASS( st_parent_class )->is_valid ){
		is_valid &= FMA_OBJECT_CLASS( st_parent_class )->is_valid( object );
	}

	return( is_valid );
}

/*
 * new_parent is not relevant when allocating a new identifier for an
 * action or a menu ; it may safely be left as NULL though there is no
 * gain to check this
 */
static gchar *
object_id_new_id( const FMAObjectId *item, const FMAObjectId *new_parent )
{
	GList *children, *it;
	uuid_t uuid;
	gchar uuid_str[64];
	gchar *new_uuid = NULL;

	g_return_val_if_fail( FMA_IS_OBJECT_ITEM( item ), NULL );

	if( !FMA_OBJECT_ITEM( item )->private->dispose_has_run ){

		/* recurse into FMAObjectItems children
		 * i.e., if a menu, recurse into embedded actions
		 */
		children = fma_object_get_items( item );
		for( it = children ; it ; it = it->next ){
			fma_object_set_new_id( it->data, new_parent );
		}

		uuid_generate( uuid );
		uuid_unparse_lower( uuid, uuid_str );
		new_uuid = g_strdup( uuid_str );
	}

	return( new_uuid );
}

/**
 * fma_object_item_get_item:
 * @item: the #FMAObjectItem from which we want retrieve a subitem.
 * @id: the id of the searched subitem.
 *
 * Returns: a pointer to the #FMAObjectId -derived child with the required id.
 *
 * The returned #FMAObjectId is owned by the @item object ; the
 * caller should not try to g_free() nor g_object_unref() it.
 *
 * Since: 2.30
 */
FMAObjectId *
fma_object_item_get_item( const FMAObjectItem *item, const gchar *id )
{
	GList *children, *it;
	FMAObjectId *found = NULL;
	FMAObjectId *isub;
	gchar *isubid;

	g_return_val_if_fail( FMA_IS_OBJECT_ITEM( item ), NULL );

	if( !item->private->dispose_has_run ){

		children = fma_object_get_items( item );
		for( it = children ; it && !found ; it = it->next ){
			isub = FMA_OBJECT_ID( it->data );
			isubid = fma_object_get_id( isub );
			if( !strcmp( id, isubid )){
				found = isub;
			}
			g_free( isubid );
		}
	}

	return( found );
}

/**
 * fma_object_item_get_position:
 * @item: this #FMAObjectItem object.
 * @child: a #FMAObjectId -derived child.
 *
 * Returns: the position of @child in the subitems list of @item,
 * starting from zero, or -1 if not found.
 *
 * Since: 2.30
 */
gint
fma_object_item_get_position( const FMAObjectItem *item, const FMAObjectId *child )
{
	gint pos = -1;
	GList *children;

	g_return_val_if_fail( FMA_IS_OBJECT_ITEM( item ), pos );
	g_return_val_if_fail( FMA_IS_OBJECT_ID( child ), pos );

	if( !child ){
		return( pos );
	}

	if( !item->private->dispose_has_run ){

		children = fma_object_get_items( item );
		pos = g_list_index( children, ( gconstpointer ) child );
	}

	return( pos );
}

/**
 * fma_object_item_append_item:
 * @item: the #FMAObjectItem to which add the subitem.
 * @child: a #FMAObjectId to be added to list of subitems.
 *
 * Appends a new @child to the list of subitems of @item,
 * and setup the parent pointer of the child to its new parent.
 *
 * Doesn't modify the reference count on @object.
 *
 * Since: 2.30
 */
void
fma_object_item_append_item( FMAObjectItem *item, const FMAObjectId *child )
{
	GList *children;

	g_return_if_fail( FMA_IS_OBJECT_ITEM( item ));
	g_return_if_fail( FMA_IS_OBJECT_ID( child ));

	if( !item->private->dispose_has_run ){

		children = fma_object_get_items( item );

		if( !g_list_find( children, ( gpointer ) child )){

			children = g_list_append( children, ( gpointer ) child );
			fma_object_set_parent( child, item );
			fma_object_set_items( item, children );
		}
	}
}

/**
 * fma_object_item_insert_at:
 * @item: the #FMAObjectItem in which add the subitem.
 * @child: a #FMAObjectId -derived to be inserted in the list of subitems.
 * @pos: the position at which the @child should be inserted.
 *
 * Inserts a new @child in the list of subitems of @item.
 *
 * Doesn't modify the reference count on @child.
 *
 * Since: 2.30
 */
void
fma_object_item_insert_at( FMAObjectItem *item, const FMAObjectId *child, gint pos )
{
	GList *children, *it;
	gint i;

	g_return_if_fail( FMA_IS_OBJECT_ITEM( item ));
	g_return_if_fail( FMA_IS_OBJECT_ID( child ));

	if( !item->private->dispose_has_run ){

		children = fma_object_get_items( item );
		if( pos == -1 || pos >= g_list_length( children )){
			fma_object_append_item( item, child );

		} else {
			i = 0;
			for( it = children ; it && i <= pos ; it = it->next ){
				if( i == pos ){
					children = g_list_insert_before( children, it, ( gpointer ) child );
				}
				i += 1;
			}
			fma_object_set_items( item, children );
		}
	}
}

/**
 * fma_object_item_insert_item:
 * @item: the #FMAObjectItem to which add the subitem.
 * @child: a #FMAObjectId to be inserted in the list of subitems.
 * @before: the #FMAObjectId before which the @child should be inserted.
 *
 * Inserts a new @child in the list of subitems of @item.
 *
 * Doesn't modify the reference count on @child.
 *
 * Since: 2.30
 */
void
fma_object_item_insert_item( FMAObjectItem *item, const FMAObjectId *child, const FMAObjectId *before )
{
	GList *children;
	GList *before_list;

	g_return_if_fail( FMA_IS_OBJECT_ITEM( item ));
	g_return_if_fail( FMA_IS_OBJECT_ID( child ));
	g_return_if_fail( !before || FMA_IS_OBJECT_ID( before ));

	if( !item->private->dispose_has_run ){

		children = fma_object_get_items( item );
		if( !g_list_find( children, ( gpointer ) child )){

			before_list = NULL;

			if( before ){
				before_list = g_list_find( children, ( gconstpointer ) before );
			}

			if( before_list ){
				children = g_list_insert_before( children, before_list, ( gpointer ) child );
			} else {
				children = g_list_prepend( children, ( gpointer ) child );
			}

			fma_object_set_items( item, children );
		}
	}
}

/**
 * fma_object_item_remove_item:
 * @item: the #FMAObjectItem from which the subitem must be removed.
 * @child: a #FMAObjectId -derived to be removed from the list of subitems.
 *
 * Removes a @child from the list of subitems of @item.
 *
 * Doesn't modify the reference count on @child.
 *
 * Since: 2.30
 */
void
fma_object_item_remove_item( FMAObjectItem *item, const FMAObjectId *child )
{
	GList *children;

	g_return_if_fail( FMA_IS_OBJECT_ITEM( item ));
	g_return_if_fail( FMA_IS_OBJECT_ID( child ));

	if( !item->private->dispose_has_run ){

		children = fma_object_get_items( item );

		if( children ){
			g_debug( "fma_object_item_remove_item: removing %p (%s) from %p (%s)",
					( void * ) child, G_OBJECT_TYPE_NAME( child ),
					( void * ) item, G_OBJECT_TYPE_NAME( item ));

			children = g_list_remove( children, ( gconstpointer ) child );
			g_debug( "fma_object_item_remove_item: after: children=%p, count=%u", ( void * ) children, g_list_length( children ));
			fma_object_set_items( item, children );
		}
	}
}

/**
 * fma_object_item_get_items_count:
 * @item: the #FMAObjectItem from which we want a count of subitems.
 *
 * Returns: the count of subitems of @item.
 *
 * Since: 2.30
 */
guint
fma_object_item_get_items_count( const FMAObjectItem *item )
{
	guint count = 0;
	GList *children;

	/*g_debug( "fma_object_item_get_items_count: item=%p (%s)", ( void * ) item, G_OBJECT_TYPE_NAME( item ));*/
	g_return_val_if_fail( FMA_IS_OBJECT_ITEM( item ), 0 );

	if( !item->private->dispose_has_run ){

		children = fma_object_get_items( item );
		count = children ? g_list_length( children ) : 0;
	}

	return( count );
}

/**
 * fma_object_item_count_items:
 * @items: a list if #FMAObject -derived to be counted.
 * @menus: will be set to the count of menus.
 * @actions: will be set to the count of actions.
 * @profiles: will be set to the count of profiles.
 * @recurse: whether to recursively count all items, or only those in
 *  level zero of the list.
 *
 * Returns: the count the numbers of items if the provided list.
 *
 * As this function is recursive, the counters should be initialized by
 * the caller before calling it.
 *
 * Since: 2.30
 */
void
fma_object_item_count_items( GList *items, gint *menus, gint *actions, gint *profiles, gboolean recurse )
{
	*menus = 0;
	*actions = 0;
	*profiles = 0;

	count_items_rec( items, menus, actions, profiles, recurse );
}

static void
count_items_rec( GList *items, gint *menus, gint *actions, gint *profiles, gboolean recurse )
{
	GList *it;

	for( it = items ; it ; it = it->next ){

		if( recurse ){
			if( FMA_IS_OBJECT_ITEM( it->data )){
				GList *subitems = fma_object_get_items( it->data );
				count_items_rec( subitems, menus, actions, profiles, recurse );
			}
		}

		if( FMA_IS_OBJECT_MENU( it->data )){
			*menus += 1;

		} else if( FMA_IS_OBJECT_ACTION( it->data )){
			*actions += 1;

		} else if( FMA_IS_OBJECT_PROFILE( it->data )){
			*profiles += 1;
		}
	}
}

/**
 * fma_object_item_copyref_items:
 * @items: a list of #FMAObject -derived items.
 *
 * Creates a copy of the provided list, recursively incrementing the
 * reference count of FMAObjects.
 *
 * Returns: the new list, which should be fma_object_free_items() by the
 * caller.
 *
 * Since: 3.1
 */
GList *
fma_object_item_copyref_items( GList *items )
{
	GList *copy = NULL;

	if( items ){
		copy = g_list_copy( items );
		g_list_foreach( copy, ( GFunc ) fma_object_object_ref, NULL );
		g_debug( "fma_object_item_copyref_items: list at %p contains %s at %p (ref_count=%d)",
				( void * ) copy,
				G_OBJECT_TYPE_NAME( copy->data ), ( void * ) copy->data, G_OBJECT( copy->data )->ref_count );
	}

	return( copy );
}

/**
 * fma_object_item_free_items:
 * @items: a list of #FMAObject -derived items.
 *
 * Free the items list.
 *
 * Returns: a %NULL pointer.
 *
 * Since: 3.1
 */
GList *
fma_object_item_free_items( GList *items )
{
	if( items ){
		g_debug( "fma_object_item_free_items: freeing list at %p which contains %s at %p (ref_count=%d)",
				( void * ) items,
				G_OBJECT_TYPE_NAME( items->data ), ( void * ) items->data, G_OBJECT( items->data )->ref_count );
		g_list_foreach( items, ( GFunc ) fma_object_object_unref, NULL );
		g_list_free( items );
	}
	return( NULL );
}

/**
 * fma_object_item_deals_with_version:
 * @item: this #FMAObjectItem -derived object.
 *
 * Just after the @item has been read from FMAIFactoryProvider, setup
 * the version. This is needed because some conversions may occur in
 * this object.
 *
 * Note that there is only some 2.x versions where the version string
 * was not systematically written. If @item has been read from a
 * .desktop file, then iversion is already set to (at least) 3.
 *
 * Since: 2.30
 */
void
fma_object_item_deals_with_version( FMAObjectItem *item )
{
	guint version_uint;
	gchar *version_str;

	g_return_if_fail( FMA_IS_OBJECT_ITEM( item ));

	if( !item->private->dispose_has_run ){

		version_uint = fma_object_get_iversion( item );

		if( !version_uint ){
			version_str = fma_object_get_version( item );

			if( !version_str || !strlen( version_str )){
				g_free( version_str );
				version_str = g_strdup( "2.0" );
			}

			version_uint = atoi( version_str );
			fma_object_set_iversion( item, version_uint );

			g_free( version_str );
		}
	}
}

/**
 * fma_object_item_rebuild_children_slist:
 * @item: this #FMAObjectItem -derived object.
 *
 * Rebuild the string list of children.
 *
 * Since: 2.30
 */
void
fma_object_item_rebuild_children_slist( FMAObjectItem *item )
{
	GSList *slist;

	fma_object_set_items_slist( item, NULL );

	if( !item->private->dispose_has_run ){

		slist = get_children_slist( item );
		fma_object_set_items_slist( item, slist );
		fma_core_utils_slist_free( slist );
	}
}

/*
 */
static GSList *
get_children_slist( const FMAObjectItem *item )
{
	GSList *slist;
	GList *subitems, *it;
	gchar *id;

	slist = NULL;
	subitems = fma_object_get_items( item );

	for( it = subitems ; it ; it = it->next ){
		id = fma_object_get_id( it->data );
		slist = g_slist_prepend( slist, id );
	}

	return( g_slist_reverse( slist ));
}

/*
 * only copy children if mode is 'full recursive'
 * or mode is 'duplicate object' and this is an action with profiles
 */
static void
copy_children( FMAObjectItem *target, const FMAObjectItem *source, guint mode )
{
	static const gchar *thisfn = "fma_object_item_copy_children";
	GList *tgt_children, *src_children, *ic;
	FMAObject *dup;

	tgt_children = fma_object_get_items( target );
	if( tgt_children ){
		g_warning( "%s: target_children=%p (count=%d)",
				thisfn,
				( void * ) tgt_children, g_list_length( tgt_children ));
		g_return_if_fail( tgt_children == NULL );
	}

	src_children = fma_object_get_items( source );
	for( ic = src_children ; ic ; ic = ic->next ){

		dup = ( FMAObject * ) fma_object_duplicate( ic->data, mode );
		fma_object_set_parent( dup, target );
		tgt_children = g_list_prepend( tgt_children, dup );
	}

	tgt_children = g_list_reverse( tgt_children );
	fma_object_set_items( target, tgt_children );
}

/**
 * fma_object_item_is_finally_writable:
 * @item: this #FMAObjectItem -derived object.
 * @reason: if not %NULL, a pointer to a guint which will hold the reason code.
 *
 * Returns: the writability status of the @item.
 *
 * Since: 3.1
 */
gboolean
fma_object_item_is_finally_writable( const FMAObjectItem *item, guint *reason )
{
	gboolean writable;

	if( reason ){
		*reason = FMA_IIO_PROVIDER_STATUS_UNDETERMINED;
	}
	g_return_val_if_fail( FMA_IS_OBJECT_ITEM( item ), FALSE );

	writable = FALSE;

	if( !item->private->dispose_has_run ){

		writable = item->private->writable;
		if( reason ){
			*reason = item->private->reason;
		}
	}

	return( writable );
}

/**
 * fma_object_item_set_writability_status:
 * @item: this #FMAObjectItem -derived object.
 * @writable: whether the item is finally writable.
 * @reason: the reason code.
 *
 * Set the writability status of the @item.
 *
 * Since: 3.1
 */
void
fma_object_item_set_writability_status( FMAObjectItem *item, gboolean writable, guint reason )
{
	g_return_if_fail( FMA_IS_OBJECT_ITEM( item ));

	if( !item->private->dispose_has_run ){

		item->private->writable = writable;
		item->private->reason = reason;
	}
}
