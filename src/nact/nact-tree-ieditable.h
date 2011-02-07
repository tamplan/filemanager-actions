/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011 Pierre Wieser and others (see AUTHORS)
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

#ifndef __NACT_TREE_TREE_IEDITABLE_H__
#define __NACT_TREE_TREE_IEDITABLE_H__

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

#include <api/na-object.h>

#include "base-window.h"

G_BEGIN_DECLS

#define NACT_TREE_IEDITABLE_TYPE                      ( nact_tree_ieditable_get_type())
#define NACT_TREE_IEDITABLE( object )                 ( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_TREE_IEDITABLE_TYPE, NactTreeIEditable ))
#define NACT_IS_TREE_IEDITABLE( object )              ( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_TREE_IEDITABLE_TYPE ))
#define NACT_TREE_IEDITABLE_GET_INTERFACE( instance ) ( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NACT_TREE_IEDITABLE_TYPE, NactTreeIEditableInterface ))

typedef struct _NactTreeIEditable                     NactTreeIEditable;
typedef struct _NactTreeIEditableInterfacePrivate     NactTreeIEditableInterfacePrivate;

typedef struct {
	/*< private >*/
	GTypeInterface                 parent;
	NactTreeIEditableInterfacePrivate *private;
}
	NactTreeIEditableInterface;

GType nact_tree_ieditable_get_type( void );

void  nact_tree_ieditable_initialize( NactTreeIEditable *instance, GtkTreeView *treeview, BaseWindow *window );
void  nact_tree_ieditable_terminate ( NactTreeIEditable *instance );

void  nact_tree_ieditable_delete        ( NactTreeIEditable *instance, GList *items, gboolean select_at_end );
void  nact_tree_ieditable_insert_items  ( NactTreeIEditable *instance, GList *items, NAObject *sibling );
void  nact_tree_ieditable_insert_at_path( NactTreeIEditable *instance, GList *items, GtkTreePath *path );
void  nact_tree_ieditable_insert_into   ( NactTreeIEditable *instance, GList *items );

#if 0
void      nact_tree_ieditable_brief_tree_dump( NactTreeIEditable *instance );
void      nact_tree_ieditable_display_order_change( NactTreeIEditable *instance, gint order_mode );
gint      nact_tree_ieditable_get_management_mode( NactTreeIEditable *instance );
gboolean  nact_tree_ieditable_has_modified_items( NactTreeIEditable *instance );
GList    *nact_tree_ieditable_remove_rec( GList *list, NAObject *object );

void      nact_tree_ieditable_bis_clear_selection( NactTreeIEditable *instance, GtkTreeView *treeview );
void      nact_tree_ieditable_bis_collapse_to_parent( NactTreeIEditable *instance );
void      nact_tree_ieditable_bis_expand_to_first_child( NactTreeIEditable *instance );
NAObject *nact_tree_ieditable_bis_get_item( NactTreeIEditable *instance, const gchar *id );
GList    *nact_tree_ieditable_bis_get_items( NactTreeIEditable *instance );
GList    *nact_tree_ieditable_bis_get_selected_items( NactTreeIEditable *instance );
void      nact_tree_ieditable_bis_list_modified_items( NactTreeIEditable *instance );
void      nact_tree_ieditable_bis_remove_modified( NactTreeIEditable *instance, const NAObjectItem *item );
void      nact_tree_ieditable_bis_select_first_row( NactTreeIEditable *instance );
void      nact_tree_ieditable_bis_select_row_at_path( NactTreeIEditable *instance, GtkTreeView *treeview, GtkTreeModel *model, GtkTreePath *path );
void      nact_tree_ieditable_bis_toggle_collapse( NactTreeIEditable *instance );
void      nact_tree_ieditable_bis_toggle_collapse_object( NactTreeIEditable *instance, const NAObject *item );
#endif

G_END_DECLS

#endif /* __NACT_TREE_TREE_IEDITABLE_H__ */
