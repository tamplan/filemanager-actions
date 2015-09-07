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

#ifndef __FILE_MANAGER_ACTIONS_API_OBJECT_ITEM_H__
#define __FILE_MANAGER_ACTIONS_API_OBJECT_ITEM_H__

/**
 * SECTION: object-item
 * @title: FMAObjectItem
 * @short_description: The Object Item Base Class Definition
 * @include: file-manager-actions/fma-object-item.h
 *
 * This is a pure virtual class, i.e. not an instantiatable one, but
 * serves as the base class for #FMAObjectAction and #FMAObjectMenu.
 */

#include "fma-object-id.h"

G_BEGIN_DECLS

#define FMA_TYPE_OBJECT_ITEM                ( fma_object_item_get_type())
#define FMA_OBJECT_ITEM( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_OBJECT_ITEM, FMAObjectItem ))
#define FMA_OBJECT_ITEM_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_OBJECT_ITEM, FMAObjectItemClass ))
#define FMA_IS_OBJECT_ITEM( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_OBJECT_ITEM ))
#define FMA_IS_OBJECT_ITEM_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_OBJECT_ITEM ))
#define FMA_OBJECT_ITEM_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_OBJECT_ITEM, FMAObjectItemClass ))

typedef struct _FMAObjectItemPrivate        FMAObjectItemPrivate;

typedef struct {
	/*< private >*/
	FMAObjectId           parent;
	FMAObjectItemPrivate *private;
}
	FMAObjectItem;

typedef struct _FMAObjectItemClassPrivate   FMAObjectItemClassPrivate;

typedef struct {
	/*< private >*/
	FMAObjectIdClass           parent;
	FMAObjectItemClassPrivate *private;
}
	FMAObjectItemClass;

/**
 * FMAItemTarget:
 * @ITEM_TARGET_SELECTION: when targeting the selection context menu.
 * @ITEM_TARGET_LOCATION:  when targeting the background context menu.
 * @ITEM_TARGET_TOOLBAR:   when targeting the toolbar.
 * @ITEM_TARGET_ANY:       a wilcard target defined in order to be able
 *                         to activate an action from a keyboard shortcut,
 *                         while keeping this same action hidden from the UI.
 *
 * The #FMAItemTarget mode is Nautilus-driven. It determines in which part
 * of the Nautilus UI our actions will be displayed.
 */
typedef enum {
	ITEM_TARGET_SELECTION = 1,
	ITEM_TARGET_LOCATION,
	ITEM_TARGET_TOOLBAR,
	ITEM_TARGET_ANY
}
	FMAItemTarget;

GType        fma_object_item_get_type              ( void );

FMAObjectId *fma_object_item_get_item              ( const FMAObjectItem *item, const gchar *id );
gint         fma_object_item_get_position          ( const FMAObjectItem *item, const FMAObjectId *child );
void         fma_object_item_append_item           ( FMAObjectItem *item, const FMAObjectId *child );
void         fma_object_item_insert_at             ( FMAObjectItem *item, const FMAObjectId *child, gint pos );
void         fma_object_item_insert_item           ( FMAObjectItem *item, const FMAObjectId *child, const FMAObjectId *before );
void         fma_object_item_remove_item           ( FMAObjectItem *item, const FMAObjectId *child );

guint        fma_object_item_get_items_count       ( const FMAObjectItem *item );

void         fma_object_item_count_items           ( GList *items, gint *menus, gint *actions, gint *profiles, gboolean recurse );
GList       *fma_object_item_copyref_items         ( GList *items );
GList       *fma_object_item_free_items            ( GList *items );

void         fma_object_item_deals_with_version    ( FMAObjectItem *item );
void         fma_object_item_rebuild_children_slist( FMAObjectItem *item );

gboolean     fma_object_item_is_finally_writable   ( const FMAObjectItem *item, guint *reason );
void         fma_object_item_set_writability_status( FMAObjectItem *item, gboolean writable, guint reason );

G_END_DECLS

#endif /* __FILE_MANAGER_ACTIONS_API_OBJECT_ITEM_H__ */
