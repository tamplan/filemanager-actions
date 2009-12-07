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

#ifndef __NAUTILUS_ACTIONS_NA_OBJECT_API_H__
#define __NAUTILUS_ACTIONS_NA_OBJECT_API_H__

/**
 * SECTION: na_object
 * @short_description: #NAObject public API.
 * @include: nautilus-actions/api/na-object-api.h
 *
 * We define here a common API which makes easier to write (and read)
 * the code ; all object functions are named na_object ; all arguments
 * are casted directly in the macro.
 */

#include <nautilus-actions/private/na-object-fn.h>
#include <nautilus-actions/private/na-object-id-fn.h>
#include <nautilus-actions/private/na-object-item-fn.h>
#include <nautilus-actions/private/na-object-menu-fn.h>
#include <nautilus-actions/private/na-object-action-fn.h>
#include <nautilus-actions/private/na-object-profile-fn.h>

G_BEGIN_DECLS

/* NAObject
 */
#define na_object_dump( object )					na_object_object_dump( NA_OBJECT( object ))
#define na_object_dump_norec( object )				na_object_object_dump_norec( NA_OBJECT( object ))
#define na_object_dump_tree( tree )					na_object_object_dump_tree( tree )
#define na_object_ref( object )						na_object_object_ref( NA_OBJECT( object ))
#define na_object_reset_origin( object, origin )	na_object_object_reset_origin( NA_OBJECT( object ), (( NAObject * )( origin )))
#define na_object_reset_status( object )			na_object_object_reset_status( NA_OBJECT( object ))
#define na_object_unref( object )					na_object_object_unref( NA_OBJECT( object ))

/* NAIDuplicable
 */
#define na_object_check_status( object )			na_object_iduplicable_check_status( NA_OBJECT( object ))
#define na_object_duplicate( object )				na_object_iduplicable_duplicate( NA_OBJECT( object ))
#define na_object_get_origin( object )				na_object_iduplicable_get_origin( NA_OBJECT( object ))
#define na_object_set_origin( object, origin )		na_object_iduplicable_set_origin( NA_OBJECT( object ), NA_OBJECT( origin ))
#define na_object_are_equal( a, b )					na_object_iduplicable_are_equal( NA_OBJECT( a ), NA_OBJECT( b ))
#define na_object_is_modified( object )				na_object_iduplicable_is_modified( NA_OBJECT( object ))
#define na_object_is_valid( object )				na_object_iduplicable_is_valid( NA_OBJECT( object ))

/* NAObjectId
 */
#define na_object_check_status_up( object )			na_object_id_check_status_up( NA_OBJECT_ID( object ))
#define na_object_get_id( object )					na_object_id_get_id( NA_OBJECT_ID( object ))
#define na_object_get_label( object )				na_object_id_get_label( NA_OBJECT_ID( object ))
#define na_object_get_parent( object )				na_object_id_get_parent( NA_OBJECT_ID( object ))

#define na_object_set_id( object, id )				na_object_id_set_id( NA_OBJECT_ID( object ), ( id ))
#define na_object_set_new_id( object, parent )		na_object_id_set_new_id( NA_OBJECT_ID( object ), (( NAObjectId * )( parent )))
#define na_object_set_label( object, label )		na_object_id_set_label( NA_OBJECT_ID( object ), ( label ))
#define na_object_set_parent( object, parent )		na_object_id_set_parent( NA_OBJECT_ID( object ), (( NAObjectItem * )( parent )))

#define na_object_prepare_for_paste( object, pivot, renumber, action ) \
													na_object_id_prepare_for_paste( NA_OBJECT_ID( object ), ( pivot ), ( renumber ), (( NAObjectAction * )( action )))
#define na_object_set_copy_of_label( object )		na_object_id_set_copy_of_label( NA_OBJECT_ID( object ))

/* NAObjectItem
 */
#define na_object_free_items_list( list )			na_object_item_free_items_list( list )

#define na_object_get_tooltip( object )				na_object_item_get_tooltip( NA_OBJECT_ITEM( object ))
#define na_object_get_icon( object )				na_object_item_get_icon( NA_OBJECT_ITEM( object ))
#define na_object_get_provider( object )			na_object_item_get_provider( NA_OBJECT_ITEM( object ))
#define na_object_is_enabled( object )				na_object_item_is_enabled( NA_OBJECT_ITEM( object ))
#define na_object_is_readonly( object )				na_object_item_is_readonly( NA_OBJECT_ITEM( object ))
#define na_object_get_item( object, id )			na_object_item_get_item( NA_OBJECT_ITEM( object ), ( id ))
#define na_object_get_items_list( object )			na_object_item_get_items_list( NA_OBJECT_ITEM( object ))
#define na_object_get_items_count( object )			na_object_item_get_items_count( NA_OBJECT_ITEM( object ))

#define na_object_set_tooltip( object, tooltip )	na_object_item_set_tooltip( NA_OBJECT_ITEM( object ), ( tooltip ))
#define na_object_set_icon( object, icon )			na_object_item_set_icon( NA_OBJECT_ITEM( object ), ( icon ))
#define na_object_set_provider( object, provider )	na_object_item_set_provider( NA_OBJECT_ITEM( object ), ( provider ))
#define na_object_set_enabled( object, enabled )	na_object_item_set_enabled( NA_OBJECT_ITEM( object ), ( enabled ))
#define na_object_set_readonly( object, readonly )	na_object_item_set_readonly( NA_OBJECT_ITEM( object ), ( readonly ))
#define na_object_set_items_list( object, list )	na_object_item_set_items_list( NA_OBJECT_ITEM( object ), ( list ))

#define na_object_append_item( object, item )		na_object_item_append_item( NA_OBJECT_ITEM( object ), NA_OBJECT( item ))
#define na_object_get_position( object, child )		na_object_item_get_position( NA_OBJECT_ITEM( object ), NA_OBJECT( child ))
#define na_object_insert_at( object, child, pos )	na_object_item_insert_at( NA_OBJECT_ITEM( object ), NA_OBJECT( child ), ( pos ))
#define na_object_insert_item( object, item, before ) \
													na_object_item_insert_item( NA_OBJECT_ITEM( object ), NA_OBJECT( item ), (( NAObject * )( before )))
#define na_object_remove_item( object, item )		na_object_item_remove_item( NA_OBJECT_ITEM( object ), NA_OBJECT( item ))

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_NA_OBJECT_API_H__ */
