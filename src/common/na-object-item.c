/*
 * Nautilus ObjectItems
 * A Nautilus extension which offers configurable context menu object_items.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <uuid/uuid.h>

#include <runtime/na-object-api.h>
#include <runtime/na-object-item-priv.h>

#include "na-object-api.h"

/**
 * na_object_item_get_pixbuf:
 * @item: this #NAObjectItem.
 * @widget: the widget for which the icon must be rendered.
 *
 * Returns the #GdkPixbuf image corresponding to the icon.
 * The image has a size of %GTK_ICON_SIZE_MENU.
 */
GdkPixbuf *na_object_item_get_pixbuf( const NAObjectItem *item, GtkWidget *widget )
{
	static const gchar *thisfn = "na_object_item_get_pixbuf";
	gchar *iconname;
	GtkStockItem stock_item;
	GdkPixbuf* icon = NULL;
	gint width, height;
	GError* error = NULL;

	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), NULL );

	if( !item->private->dispose_has_run ){

		iconname = na_object_item_get_icon( item );

		/* TODO: use the same algorythm than Nautilus to find and
		 * display an icon + move the code to NAAction class +
		 * remove na_action_get_verified_icon_name
		 */
		if( iconname ){
			if( gtk_stock_lookup( iconname, &stock_item )){
				icon = gtk_widget_render_icon( widget, iconname, GTK_ICON_SIZE_MENU, NULL );

			} else if( g_file_test( iconname, G_FILE_TEST_EXISTS )
				   && g_file_test( iconname, G_FILE_TEST_IS_REGULAR )){

				gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &width, &height);
				icon = gdk_pixbuf_new_from_file_at_size( iconname, width, height, &error );
				if( error ){
					g_warning( "%s: iconname=%s, error=%s", thisfn, iconname, error->message );
					g_error_free( error );
					error = NULL;
					icon = NULL;
				}
			}
		}

		g_free( iconname );
	}

	return( icon );
}

/**
 * na_object_item_insert_item:
 * @item: the #NAObjectItem to which add the subitem.
 * @object: a #NAObject to be inserted in the list of subitems.
 * @before: the #NAObject before which the @object should be inserted.
 *
 * Inserts a new @object in the list of subitems of @item.
 *
 * Doesn't modify the reference count on @object.
 */
void
na_object_item_insert_item( NAObjectItem *item, const NAObject *object, const NAObject *before )
{
	GList *before_list;

	g_return_if_fail( NA_IS_OBJECT_ITEM( item ));
	g_return_if_fail( NA_IS_OBJECT( object ));
	g_return_if_fail( !before || NA_IS_OBJECT( before ));

	if( !item->private->dispose_has_run ){

		if( !g_list_find( item->private->items, ( gpointer ) object )){

			before_list = NULL;

			if( before ){
				before_list = g_list_find( item->private->items, ( gconstpointer ) before );
			}

			if( before_list ){
				item->private->items = g_list_insert_before( item->private->items, before_list, ( gpointer ) object );
			} else {
				item->private->items = g_list_prepend( item->private->items, ( gpointer ) object );
			}
		}
	}
}

/**
 * na_object_item_count_items:
 * @items: a list if #NAObject to be counted.
 * @menus: will be set to the count of menus.
 * @actions: will be set to the count of actions.
 * @profiles: will be set to the count of profiles.
 * @recurse: whether to recursively count all items, or only those in
 *  level zero of the list.
 *
 * Count the numbers of items if the provided list.
 *
 * As this function is recursive, the counters should be initialized by
 * the caller before calling it.
 */
void
na_object_item_count_items( GList *items, gint *menus, gint *actions, gint *profiles, gboolean recurse )
{
	GList *it;

	for( it = items ; it ; it = it->next ){

		/*g_debug( "na_object_item_count_items: item is %s", G_OBJECT_TYPE_NAME( it->data ));*/
		if( NA_IS_OBJECT_MENU( it->data )){
			*menus += 1;
		} else if( NA_IS_OBJECT_ACTION( it->data )){
			*actions += 1;
		} else if( NA_IS_OBJECT_PROFILE( it->data )){
			*profiles += 1;
		}

		if( recurse ){
			if( NA_IS_OBJECT_ITEM( it->data )){
				na_object_item_count_items(
						NA_OBJECT_ITEM( it->data )->private->items, menus, actions, profiles, recurse );
			}
		}
	}
}
