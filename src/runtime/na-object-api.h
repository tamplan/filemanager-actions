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

#ifndef __NA_OBJECT_API_H__
#define __NA_OBJECT_API_H__

/**
 * SECTION: na_object
 * @short_description: #NAObject public API.
 * @include: runtime/na-object-api.h
 *
 * We define here a common API which makes easier to write (and read)
 * the code ; all object functions are named na_object ; all arguments
 * are casted directly in the macro.
 *
 * Only define here the API which is relevant in libna-runtime library.
 */

#include "na-object-fn.h"
#include "na-object-id-fn.h"
#include "na-object-item-fn.h"
#include "na-object-menu-fn.h"
#include "na-object-action-fn.h"
#include "na-object-profile-fn.h"

G_BEGIN_DECLS

/* NAObject
 */
#define na_object_dump( object )					na_object_object_dump( NA_OBJECT( object ))
#define na_object_dump_norec( object )				na_object_object_dump_norec( NA_OBJECT( object ))
#define na_object_dump_tree( tree )					na_object_object_dump_tree( tree )

/* NAIDuplicable
 */
#define na_object_duplicate( object )				na_object_iduplicable_duplicate( NA_OBJECT( object ))
#define na_object_is_modified( object )				na_object_iduplicable_is_modified( NA_OBJECT( object ))

/* NAObjectId
 */
#define na_object_get_id( object )					na_object_id_get_id( NA_OBJECT_ID( object ))
#define na_object_get_label( object )				na_object_id_get_label( NA_OBJECT_ID( object ))

#define na_object_set_id( object, id )				na_object_id_set_id( NA_OBJECT_ID( object ), id )
#define na_object_set_new_id( object, parent )		na_object_id_set_new_id( NA_OBJECT_ID( object ), ( NAObjectId * ) parent )
#define na_object_set_label( object, label )		na_object_id_set_label( NA_OBJECT_ID( object ), label )

/* NAObjectItem
 */
#define na_object_get_tooltip( object )				na_object_item_get_tooltip( NA_OBJECT_ITEM( object ))
#define na_object_get_icon( object )				na_object_item_get_icon( NA_OBJECT_ITEM( object ))
#define na_object_get_provider( object )			na_object_item_get_provider( NA_OBJECT_ITEM( object ))
#define na_object_is_enabled( object )				na_object_item_is_enabled( NA_OBJECT_ITEM( object ))
#define na_object_get_item( object, id )			na_object_item_get_item( NA_OBJECT_ITEM( object ), id )
#define na_object_get_items( object )				na_object_item_get_items( NA_OBJECT_ITEM( object ))
#define na_object_get_items_count( object )			na_object_item_get_items_count( NA_OBJECT_ITEM( object ))
#define na_object_free_items( list )				na_object_item_free_items( list )

#define na_object_set_tooltip( object, tooltip )	na_object_item_set_tooltip( NA_OBJECT_ITEM( object ), tooltip )
#define na_object_set_icon( object, icon )			na_object_item_set_icon( NA_OBJECT_ITEM( object ), icon )
#define na_object_set_provider( object, provider )	na_object_item_set_provider( NA_OBJECT_ITEM( object ), provider )
#define na_object_set_enabled( object, enabled )	na_object_item_set_enabled( NA_OBJECT_ITEM( object ), enabled )
#define na_object_set_items( object, list )			na_object_item_set_items( NA_OBJECT_ITEM( object ), list )

#define na_object_append_item( object, item )		na_object_item_append_item( NA_OBJECT_ITEM( object ), NA_OBJECT( item ))

G_END_DECLS

#endif /* __NA_OBJECT_API_H__ */
