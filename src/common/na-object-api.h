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

#ifndef __NA_COMMON_OBJECT_API_H__
#define __NA_COMMON_OBJECT_API_H__

/**
 * SECTION: na_object
 * @short_description: #NAObject public API extension.
 * @include: common/na-object-api.h
 *
 * #NAObject main public API is defined as part of in libna-runtime
 * convenience library. Found here is public API not shared by the
 * Nautilus Actions plugin.
 */

#include <runtime/na-object-api.h>

#include "na-object-fn.h"
#include "na-object-id-fn.h"
#include "na-object-item-fn.h"
#include "na-object-menu-fn.h"
#include "na-object-action-fn.h"
#include "na-object-profile-fn.h"

G_BEGIN_DECLS

/* NAObject
 */
#define na_object_reset_origin( object, origin )	na_object_object_reset_origin( NA_OBJECT( object ), ( NAObject * ) origin )

/* NAIDuplicable
 */
#define na_object_get_origin( object )				na_object_iduplicable_get_origin( NA_OBJECT( object ))
#define na_object_set_origin( object, origin )		na_object_iduplicable_set_origin( NA_OBJECT( object ), NA_OBJECT( origin ))

/* NAObjectId
 */
#define na_object_prepare_for_paste( object, pivot, renumber, action ) \
													na_object_id_prepare_for_paste( NA_OBJECT_ID( object ), pivot, renumber, ( NAObjectAction * ) action )
#define na_object_set_copy_of_label( object )		na_object_id_set_copy_of_label( NA_OBJECT_ID( object ))

/* NAObjectItem
 */
#define na_object_insert_item( object, item, before ) \
													na_object_item_insert_item( NA_OBJECT_ITEM( object ), NA_OBJECT( item ), ( NAObject * ) before )

G_END_DECLS

#endif /* __NA_COMMON_OBJECT_API_H__ */
