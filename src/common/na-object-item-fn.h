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

#ifndef __NA_OBJECT_ITEM_FN_H__
#define __NA_OBJECT_ITEM_FN_H__

/**
 * SECTION: na_object_item
 * @short_description: #NAObjectItem class definition.
 * @include: common/na-object-item.h
 *
 * Derived from #NAObjectId class, this class is built to be used as
 * a base class for objects which have a tooltip and an icon.
 */

#include <gtk/gtk.h>

#include "na-object-item-class.h"
#include "na-iio-provider.h"

G_BEGIN_DECLS

gchar         *na_object_item_get_tooltip( const NAObjectItem *item );
gchar         *na_object_item_get_icon( const NAObjectItem *item );
gchar         *na_object_item_get_verified_icon_name( const NAObjectItem *item );
GdkPixbuf     *na_object_item_get_pixbuf( const NAObjectItem *item, GtkWidget *widget );
gboolean       na_object_item_is_enabled( const NAObjectItem *item );
NAIIOProvider *na_object_item_get_provider( const NAObjectItem *item );
NAObject      *na_object_item_get_item( const NAObjectItem *item, const gchar *id );
GList         *na_object_item_get_items( const NAObjectItem *item );
guint          na_object_item_get_items_count( const NAObjectItem *item );
void           na_object_item_free_items( GList *items );

void           na_object_item_set_tooltip( NAObjectItem *item, const gchar *tooltip );
void           na_object_item_set_icon( NAObjectItem *item, const gchar *icon_name );
void           na_object_item_set_enabled( NAObjectItem *item, gboolean enabled );
void           na_object_item_set_provider( NAObjectItem *item, const NAIIOProvider *provider );
void           na_object_item_set_items( NAObjectItem *item, GList *items );

void           na_object_item_append_item( NAObjectItem *item, const NAObject *object );
void           na_object_item_insert_item( NAObjectItem *item, const NAObject *object );
void           na_object_item_remove_item( NAObjectItem *item, NAObject *object );

G_END_DECLS

#endif /* __NA_OBJECT_ITEM_FN_H__ */
