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

#ifndef __NAUTILUS_ACTIONS_API_NA_OBJECT_ITEM_H__
#define __NAUTILUS_ACTIONS_API_NA_OBJECT_ITEM_H__

/**
 * SECTION: na_object_item
 * @short_description: #NAObjectItem class definition.
 * @include: nautilus-actions/na-object-item.h
 *
 * This is a pure virtual class, i.e. not an instantiatable one, but
 * serves as the base class for #NAObjectAction and #NAObjectMenu.
 */

#include "na-object-id.h"

G_BEGIN_DECLS

#define NA_OBJECT_ITEM_TYPE					( na_object_item_get_type())
#define NA_OBJECT_ITEM( object )			( G_TYPE_CHECK_INSTANCE_CAST( object, NA_OBJECT_ITEM_TYPE, NAObjectItem ))
#define NA_OBJECT_ITEM_CLASS( klass )		( G_TYPE_CHECK_CLASS_CAST( klass, NA_OBJECT_ITEM_TYPE, NAObjectItemClass ))
#define NA_IS_OBJECT_ITEM( object )			( G_TYPE_CHECK_INSTANCE_TYPE( object, NA_OBJECT_ITEM_TYPE ))
#define NA_IS_OBJECT_ITEM_CLASS( klass )	( G_TYPE_CHECK_CLASS_TYPE(( klass ), NA_OBJECT_ITEM_TYPE ))
#define NA_OBJECT_ITEM_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), NA_OBJECT_ITEM_TYPE, NAObjectItemClass ))

typedef struct NAObjectItemPrivate      NAObjectItemPrivate;

typedef struct {
	NAObjectId           parent;
	NAObjectItemPrivate *private;
}
	NAObjectItem;

typedef struct NAObjectItemClassPrivate NAObjectItemClassPrivate;

typedef struct {
	NAObjectIdClass           parent;
	NAObjectItemClassPrivate *private;
}
	NAObjectItemClass;

/* targets
 *
 * ITEM_TARGET_ANY is just a wilcard target defined in order to be able
 * to activate an action from a keyboard shortcut, while keeping this
 * same action hidden from the UI.
 */
enum {
	ITEM_TARGET_SELECTION = 1,
	ITEM_TARGET_LOCATION,
	ITEM_TARGET_TOOLBAR,
	ITEM_TARGET_ANY
};

GType       na_object_item_get_type( void );

gboolean    na_object_item_are_equal( const NAObjectItem *a, const NAObjectItem *b );

NAObjectId *na_object_item_get_item    ( const NAObjectItem *item, const gchar *id );
gint        na_object_item_get_position( const NAObjectItem *item, const NAObjectId *child );
void        na_object_item_append_item ( NAObjectItem *item, const NAObjectId *child );
void        na_object_item_insert_at   ( NAObjectItem *item, const NAObjectId *child, gint pos );
void        na_object_item_insert_item ( NAObjectItem *item, const NAObjectId *child, const NAObjectId *before );
void        na_object_item_remove_item ( NAObjectItem *item, const NAObjectId *child );

guint       na_object_item_get_items_count( const NAObjectItem *item );

void        na_object_item_count_items( GList *items, gint *menus, gint *actions, gint *profiles, gboolean recurse );
void        na_object_item_unref_items( GList *items );
void        na_object_item_unref_items_rec( GList *items );

void        na_object_item_deals_with_version    ( NAObjectItem *item );
void        na_object_item_rebuild_children_slist( NAObjectItem *item );

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_API_NA_OBJECT_ITEM_H__ */
