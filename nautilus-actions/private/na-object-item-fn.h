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

#ifndef __NAUTILUS_ACTIONS_NA_PRIVATE_OBJECT_ITEM_FN_H__
#define __NAUTILUS_ACTIONS_NA_PRIVATE_OBJECT_ITEM_FN_H__

/**
 * SECTION: na_object_item
 * @short_description: #NAObjectItem public function declarations.
 * @include: nautilus-actions/private/na-object-fn.h
 *
 * Define here the public functions of the #NAObjectItem class.
 *
 * Note that most users of the class should rather use macros defined
 * in na-object-api.h
 */

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include <nautilus-actions/api/na-iio-provider.h>

#include "na-object-item-class.h"

G_BEGIN_DECLS

void           na_object_item_free_items_list( GList *items );

gchar         *na_object_item_get_tooltip( const NAObjectItem *item );
gchar         *na_object_item_get_icon( const NAObjectItem *item );
GdkPixbuf     *na_object_item_get_pixbuf( const NAObjectItem *object, GtkWidget *widget );
gint           na_object_item_get_position( const NAObjectItem *object, const NAObject *child );
NAIIOProvider *na_object_item_get_provider( const NAObjectItem *item );
gboolean       na_object_item_is_enabled( const NAObjectItem *item );
gboolean       na_object_item_is_readonly( const NAObjectItem *action );
NAObject      *na_object_item_get_item( const NAObjectItem *item, const gchar *id );
GList         *na_object_item_get_items_list( const NAObjectItem *item );
guint          na_object_item_get_items_count( const NAObjectItem *item );

void           na_object_item_count_items( GList *items, gint *menus, gint *actions, gint *profiles, gboolean recurse );

void           na_object_item_set_tooltip( NAObjectItem *item, const gchar *tooltip );
void           na_object_item_set_icon( NAObjectItem *item, const gchar *icon_name );
void           na_object_item_set_provider( NAObjectItem *item, const NAIIOProvider *provider );
void           na_object_item_set_enabled( NAObjectItem *item, gboolean enabled );
void           na_object_item_set_readonly( NAObjectItem *action, gboolean readonly );
void           na_object_item_set_items_list( NAObjectItem *item, GList *items );

void           na_object_item_append_item( NAObjectItem *object, const NAObject *item );
void           na_object_item_insert_at( NAObjectItem *object, const NAObject *item, gint pos );
void           na_object_item_insert_item( NAObjectItem *object, const NAObject *item, const NAObject *before );
void           na_object_item_remove_item( NAObjectItem *object, const NAObject *item );

GSList        *na_object_item_get_items_string_list( const NAObjectItem *item );
GSList        *na_object_item_rebuild_items_list( const NAObjectItem *item );
void           na_object_item_set_items_string_list( NAObjectItem *item, GSList *items );

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_NA_PRIVATE_OBJECT_ITEM_FN_H__ */
