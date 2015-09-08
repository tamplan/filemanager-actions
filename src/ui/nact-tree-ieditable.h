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

#ifndef __UI_NACT_TREE_TREE_IEDITABLE_H__
#define __UI_NACT_TREE_TREE_IEDITABLE_H__

/**
 * SECTION: nact-tree_ieditable
 * @title: NactTreeIEditable
 * @short_description: The NactTreeIEditable interface definition
 * @include: nact-tree_ieditable.h
 *
 * This interface is to be implemented by a NactTreeView which would
 * want get edition features, such as inline edition, insert, delete,
 * and so on.
 *
 * NactTreeIEditable maintains the count of modified items.
 * Starting with zero when the tree view is filled up, it is incremented
 * each time an item is modified, inserted or deleted.
 * The modified count is fully recomputed after a save.
 */

#include "api/fma-object.h"

#include "nact-main-window-def.h"

G_BEGIN_DECLS

#define NACT_TREE_IEDITABLE_TYPE                      ( nact_tree_ieditable_get_type())
#define NACT_TREE_IEDITABLE( object )                 ( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_TREE_IEDITABLE_TYPE, NactTreeIEditable ))
#define NACT_IS_TREE_IEDITABLE( object )              ( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_TREE_IEDITABLE_TYPE ))
#define NACT_TREE_IEDITABLE_GET_INTERFACE( instance ) ( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NACT_TREE_IEDITABLE_TYPE, NactTreeIEditableInterface ))

typedef struct _NactTreeIEditable                     NactTreeIEditable;
typedef struct _NactTreeIEditableInterfacePrivate     NactTreeIEditableInterfacePrivate;

typedef struct {
	/*< private >*/
	GTypeInterface                     parent;
	NactTreeIEditableInterfacePrivate *private;
}
	NactTreeIEditableInterface;

/**
 * Delete operations
 */
typedef enum {
	TREE_OPE_DELETE = 0,
	TREE_OPE_MOVE
}
	TreeIEditableDeleteOpe;

GType    nact_tree_ieditable_get_type              ( void );

void     nact_tree_ieditable_initialize            ( NactTreeIEditable *instance,
															GtkTreeView *treeview,
															NactMainWindow *main_window );

void     nact_tree_ieditable_terminate             ( NactTreeIEditable *instance );

void     nact_tree_ieditable_delete                ( NactTreeIEditable *instance,
															GList *items,
															TreeIEditableDeleteOpe ope );

gboolean nact_tree_ieditable_remove_deleted        ( NactTreeIEditable *instance,
															GSList **messages );

GList   *nact_tree_ieditable_get_deleted           ( NactTreeIEditable *instance );

void     nact_tree_ieditable_insert_items          ( NactTreeIEditable *instance,
															GList *items,
															FMAObject *sibling );

void     nact_tree_ieditable_insert_at_path        ( NactTreeIEditable *instance,
															GList *items,
															GtkTreePath *path );

void     nact_tree_ieditable_insert_into           ( NactTreeIEditable *instance,
															GList *items );

void     nact_tree_ieditable_set_items             ( NactTreeIEditable *instance,
															GList *items );

void     nact_tree_ieditable_dump_modified         ( const NactTreeIEditable *instance );

gboolean nact_tree_ieditable_is_level_zero_modified( const NactTreeIEditable *instance );

G_END_DECLS

#endif /* __UI_NACT_TREE_TREE_IEDITABLE_H__ */
