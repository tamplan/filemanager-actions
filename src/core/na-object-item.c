/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010 Pierre Wieser and others (see AUTHORS)
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

#include <api/na-object-api.h>

/* private class data
 */
struct NAObjectItemClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NAObjectItemPrivate {
	gboolean   dispose_has_run;

	void      *provider_data;

	/* dynamically set when reading the item from the I/O storage
	 * subsystem; may be reset from FALSE to TRUE if a write operation
	 * has returned an error.
	 * defaults to FALSE for snew, not yet written to a provider, item
	 */
	gboolean   readonly;
};

static NAObjectIdClass *st_parent_class = NULL;

static GType register_type( void );
static void  class_init( NAObjectItemClass *klass );
static void  instance_init( GTypeInstance *instance, gpointer klass );
static void  instance_dispose( GObject *object );
static void  instance_finalize( GObject *object );

GType
na_object_item_get_type( void )
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
	static const gchar *thisfn = "na_object_item_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NAObjectItemClass ),
		NULL,
		NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NAObjectItem ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( NA_OBJECT_ID_TYPE, "NAObjectItem", &info, 0 );

	return( type );
}

static void
class_init( NAObjectItemClass *klass )
{
	static const gchar *thisfn = "na_object_item_class_init";
	GObjectClass *object_class;
	NAObjectClass *naobject_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	naobject_class = NA_OBJECT_CLASS( klass );
	naobject_class->dump = NULL;
	naobject_class->copy = NULL;
	naobject_class->are_equal = NULL;
	naobject_class->is_valid = NULL;

	klass->private = g_new0( NAObjectItemClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "na_object_item_instance_init";
	NAObjectItem *self;

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	g_return_if_fail( NA_IS_OBJECT_ITEM( instance ));

	self = NA_OBJECT_ITEM( instance );

	self->private = g_new0( NAObjectItemPrivate, 1 );
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "na_object_item_instance_dispose";
	NAObjectItem *self;

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	g_return_if_fail( NA_IS_OBJECT_ITEM( object ));

	self = NA_OBJECT_ITEM( object );

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
	NAObjectItem *self;

	g_return_if_fail( NA_IS_OBJECT_ITEM( object ));

	self = NA_OBJECT_ITEM( object );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * na_object_item_copy:
 * @item: the target #NAObjectItem instance.
 * @source: the source #NAObjectItem instance.
 *
 * Copies data from @source to @item.
 *
 * This function participates to the #na_iduplicable_duplicate() stack,
 * and is triggered after all copyable elementary data (in #NAIDataFactory
 * sense) have already been copied themselves.
 *
 * We have to deal here with the subitems: duplicating childs from @source
 * to @item.
 */
void
na_object_item_copy( NAObjectItem *item, const NAObjectItem *source )
{
	static const gchar *thisfn = "na_object_item_copy";
	GList *tgt_childs, *src_childs, *ic;
	NAObject *dup;

	tgt_childs = na_object_get_items( item );
	if( tgt_childs ){

		g_warning( "%s: target %s has already %d childs",
				thisfn, G_OBJECT_TYPE_NAME( item ), g_list_length( tgt_childs ));
		na_object_unref_items( tgt_childs );
		tgt_childs = NULL;
	}

	src_childs = na_object_get_items( source );
	for( ic = src_childs ; ic ; ic = ic->next ){

		dup = ( NAObject * ) na_object_duplicate( ic->data );
		na_object_set_parent( dup, item );
		tgt_childs = g_list_prepend( tgt_childs, dup );
	}

	tgt_childs = g_list_reverse( tgt_childs );
	na_object_set_items( item, tgt_childs );
}

/**
 * na_object_item_are_equal:
 * @a: the first #NAObjectItem instance.
 * @b: the second #NAObjectItem instance.
 *
 * Returns: %TRUE if @a is equal to @b.
 *
 * This function participates to the #na_iduplicable_check_status() stack,
 * and is triggered after all comparable elementary data (in #NAIDataFactory
 * sense) have already been successfully compared.
 *
 * We have to deal here with the subitems: comparing childs by their ids
 * between @a and @b.
 */
gboolean
na_object_item_are_equal( const NAObjectItem *a, const NAObjectItem *b )
{
	gboolean equal;
	GList *a_childs, *b_childs, *it;
	gchar *first_id, *second_id;
	NAObjectId *first_obj, *second_obj;
	gint first_pos, second_pos;
	GList *second_list;

	g_return_val_if_fail( NA_IS_OBJECT_ITEM( a ), FALSE );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( b ), FALSE );

	equal = FALSE;

	if( !NA_OBJECT_ITEM( a )->private->dispose_has_run &&
		!NA_OBJECT_ITEM( b )->private->dispose_has_run ){

		equal = TRUE;

		if( equal ){
			a_childs = na_object_get_items( a );
			b_childs = na_object_get_items( b );
			equal = ( g_list_length( a_childs ) == g_list_length( b_childs ));
		}

		if( equal ){
			for( it = a_childs ; it && equal ; it = it->next ){
				first_id = na_object_get_id( it->data );
				second_obj = na_object_get_item( b, first_id );
				first_pos = -1;
				second_pos = -1;
				if( second_obj ){
					first_pos = g_list_position( a_childs, it );
					second_list = g_list_find( b_childs, second_obj );
					second_pos = g_list_position( b_childs, second_list );
#if NA_IDUPLICABLE_EDITION_STATUS_DEBUG
					g_debug( "na_object_item_are_equal: first_pos=%u, second_pos=%u", first_pos, second_pos );
#endif
					if( first_pos != second_pos ){
						equal = FALSE;
						/*g_debug( "first_id=%s, first_pos=%d, second_pos=%d", first_id, first_pos, second_pos );*/
					}
				} else {
#if NA_IDUPLICABLE_EDITION_STATUS_DEBUG
					g_debug( "na_object_item_are_equal: id=%s not found in b", first_id );
#endif
					equal = FALSE;
					/*g_debug( "first_id=%s, second not found", first_id );*/
				}
				/*g_debug( "first_id=%s first_pos=%d second_pos=%d", first_id, first_pos, second_pos );*/
				g_free( first_id );
			}
		}

		if( equal ){
			for( it = b_childs ; it && equal ; it = it->next ){
				second_id = na_object_get_id( it->data );
				first_obj = na_object_get_item( a, second_id );
				if( !first_obj ){
#if NA_IDUPLICABLE_EDITION_STATUS_DEBUG
					g_debug( "na_object_item_are_equal: id=%s not found in a", second_id );
#endif
					equal = FALSE;
					/*g_debug( "second_id=%s, first not found", second_id );*/
				}
				g_free( second_id );
			}
		}

		/*g_debug( "na_object_item_object_are_equal: a=%p (%s), b=%p (%s), are_equal=%s",
				( void * ) a, G_OBJECT_TYPE_NAME( a ),
				( void * ) b, G_OBJECT_TYPE_NAME( b ),
				equal ? "True":"False" );*/
	}

	return( equal );
}

/**
 * na_object_item_get_item:
 * @item: the #NAObjectItem from which we want retrieve a subitem.
 * @id: the id of the searched subitem.
 *
 * Returns: a pointer to the #NAObjectId-derived child with the required id.
 *
 * The returned #NAObjectId is owned by the @item object ; the
 * caller should not try to g_free() nor g_object_unref() it.
 */
NAObjectId *
na_object_item_get_item( const NAObjectItem *item, const gchar *id )
{
	GList *childs, *it;
	NAObjectId *found = NULL;
	NAObjectId *isub;
	gchar *isubid;

	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), NULL );

	if( !item->private->dispose_has_run ){

		childs = na_object_get_items( item );
		for( it = childs ; it && !found ; it = it->next ){
			isub = NA_OBJECT_ID( it->data );
			isubid = na_object_get_id( isub );
			if( !strcmp( id, isubid )){
				found = isub;
			}
			g_free( isubid );
		}
	}

	return( found );
}

/**
 * na_object_item_append_item:
 * @item: the #NAObjectItem to which add the subitem.
 * @child: a #NAObjectId to be added to list of subitems.
 *
 * Appends a new @child to the list of subitems of @item,
 * and setup the parent pointer of the child to its new parent.
 *
 * Doesn't modify the reference count on @object.
 */
void
na_object_item_append_item( NAObjectItem *item, const NAObjectId *child )
{
	GList *childs_list;

	g_return_if_fail( NA_IS_OBJECT_ITEM( item ));
	g_return_if_fail( NA_IS_OBJECT_ID( child ));

	if( !item->private->dispose_has_run ){

		childs_list = na_object_get_items( item );

		if( !g_list_find( childs_list, ( gpointer ) child )){

			childs_list = g_list_append( childs_list, ( gpointer ) child );
			na_object_set_parent( child, item );
			na_object_set_items( item, childs_list );
		}
	}
}

/**
 * na_object_item_build_items_slist:
 * @item: this #NAObjectItem object.
 *
 * Returns: a string list which contains the ordered list of ids of
 * subitems.
 *
 * Note that the returned list is built on each call to this function,
 * and is so an exact image of the current situation.
 *
 * The returned list should be na_core_utils_slist_free() by the caller.
 */
GSList *
na_object_item_build_items_slist( const NAObjectItem *item )
{
	GSList *slist;
	GList *subitems, *it;
	gchar *id;

	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), NULL );

	slist = NULL;

	if( !item->private->dispose_has_run ){

		subitems = na_object_get_items( item );

		for( it = subitems ; it ; it = it->next ){
			NAObjectId *item = NA_OBJECT_ID( it->data );
			id = na_object_get_id( item );
			slist = g_slist_prepend( slist, id );
		}

		slist = g_slist_reverse( slist );
	}

	return( slist );
}

/**
 * na_object_item_unref_items:
 * @list: a list of #NAObject-derived items.
 *
 * Recursively unref the #NAObject of the list, freeing the list at last.
 */
void
na_object_item_unref_items( GList *items )
{
	GList *it;

	for( it = items ; it ; it = it->next ){

		na_object_unref( it->data );
	}

	g_list_free( items );
}
