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

#ifndef __NA_OBJECT_ITEM_H__
#define __NA_OBJECT_ITEM_H__

/**
 * SECTION: na_object_item
 * @short_description: #NAObjectItem class definition.
 * @include: common/na-object-item.h
 *
 * Derived from #NAObject class, this class is built to be used as a
 * base class for objects which have a tooltip and an icon.
 */

#include "na-object.h"

G_BEGIN_DECLS

#define NA_OBJECT_ITEM_TYPE					( na_object_item_get_type())
#define NA_OBJECT_ITEM( object )			( G_TYPE_CHECK_INSTANCE_CAST( object, NA_OBJECT_ITEM_TYPE, NAObjectItem ))
#define NA_OBJECT_ITEM_CLASS( klass )		( G_TYPE_CHECK_CLASS_CAST( klass, NA_OBJECT_ITEM_TYPE, NAObjectItemClass ))
#define NA_IS_OBJECT_ITEM( object )			( G_TYPE_CHECK_INSTANCE_TYPE( object, NA_OBJECT_ITEM_TYPE ))
#define NA_IS_OBJECT_ITEM_CLASS( klass )	( G_TYPE_CHECK_CLASS_TYPE(( klass ), NA_OBJECT_ITEM_TYPE ))
#define NA_OBJECT_ITEM_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), NA_OBJECT_ITEM_TYPE, NAObjectItemClass ))

typedef struct NAObjectItemPrivate NAObjectItemPrivate;

typedef struct {
	NAObject             parent;
	NAObjectItemPrivate *private;
}
	NAObjectItem;

typedef struct NAObjectItemClassPrivate NAObjectItemClassPrivate;

typedef struct {
	NAObjectClass             parent;
	NAObjectItemClassPrivate *private;
}
	NAObjectItemClass;

/* object properties
 * used in derived classes to access the properties
 */
enum {
	PROP_NAOBJECT_ITEM_TOOLTIP = 1,
	PROP_NAOBJECT_ITEM_ICON
};

GType  na_object_item_get_type( void );

gchar *na_object_item_get_tooltip( const NAObjectItem *item );
gchar *na_object_item_get_icon( const NAObjectItem *item );
gchar *na_object_item_get_verified_icon_name( const NAObjectItem *item );

void   na_object_item_set_tooltip( NAObjectItem *item, const gchar *tooltip );
void   na_object_item_set_icon( NAObjectItem *item, const gchar *icon_name );

G_END_DECLS

#endif /* __NA_OBJECT_ITEM_H__ */
