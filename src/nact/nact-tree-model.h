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

/**
 * SECTION: nact_tree_model
 * @short_description: #NactTreeModel class definition.
 * @include: nact/nact-tree-model.h
 *
 * NactTreeModel is derived from GtkTreeModelFilter in order to be able
 * to selectively display profiles, whether an action has one or more
 * profiles.
 *
 * NactTreeModel implements EggTreeMultiDragSource and GtkTreeDragDest
 * interfaces.
 *
 * The GtkTreeModelFilter base class embeds a GtkTreeStore.
 */
#ifndef __NACT_TREE_MODEL_H__
#define __NACT_TREE_MODEL_H__

#include <api/na-object.h>

#include "nact-tree-view.h"

G_BEGIN_DECLS

#define NACT_TREE_MODEL_TYPE                ( nact_tree_model_get_type())
#define NACT_TREE_MODEL( object )           ( G_TYPE_CHECK_INSTANCE_CAST(( object ), NACT_TREE_MODEL_TYPE, NactTreeModel ))
#define NACT_TREE_MODEL_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST(( klass ), NACT_TREE_MODEL_TYPE, NactTreeModelClass ))
#define NACT_IS_TREE_MODEL( object )        ( G_TYPE_CHECK_INSTANCE_TYPE(( object ), NACT_TREE_MODEL_TYPE ))
#define NACT_IS_TREE_MODEL_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), NACT_TREE_MODEL_TYPE ))
#define NACT_TREE_MODEL_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), NACT_TREE_MODEL_TYPE, NactTreeModelClass ))

typedef struct _NactTreeModelPrivate        NactTreeModelPrivate;

typedef struct {
	/*< private >*/
	GtkTreeModelFilter    parent;
	NactTreeModelPrivate *private;
}
	NactTreeModel;

typedef struct _NactTreeModelClassPrivate   NactTreeModelClassPrivate;

typedef struct {
	/*< private >*/
	GtkTreeModelFilterClass    parent;
	NactTreeModelClassPrivate *private;
}
	NactTreeModelClass;

/**
 * Column ordering in the tree view
 */
enum {
	TREE_COLUMN_ICON = 0,
	TREE_COLUMN_LABEL,
	TREE_COLUMN_NAOBJECT,
	TREE_N_COLUMN
};

/**
 * When getting a list of items; these indcators may be OR-ed.
 */
enum {
	TREE_LIST_SELECTED = 1<<0,
	TREE_LIST_MODIFIED = 1<<1,
	TREE_LIST_ALL = 0xff,
};

GType          nact_tree_model_get_type( void );

NactTreeModel *nact_tree_model_new( BaseWindow *window, GtkTreeView *view, NactTreeMode mode );

GtkTreePath   *nact_tree_model_delete       ( NactTreeModel *model, NAObject *object );
void           nact_tree_model_fill         ( NactTreeModel *model, GList *items );
GtkTreePath   *nact_tree_model_insert_before( NactTreeModel *model, const NAObject *object, GtkTreePath *path );
GtkTreePath   *nact_tree_model_insert_into  ( NactTreeModel *model, const NAObject *object, GtkTreePath *path );

NAObjectItem  *nact_tree_model_get_item_by_id( const NactTreeModel *model, const gchar *id );
GList         *nact_tree_model_get_items     ( const NactTreeModel *model, guint mode );
NAObject      *nact_tree_model_object_at_path( const NactTreeModel *model, GtkTreePath *path );
GtkTreePath   *nact_tree_model_object_to_path( const NactTreeModel *model, const NAObject *object );

/* find an item
 * get items (all, selected, modified)
 * remove a list of items
 * insert a list of items (here, inside)
 */

/* *** */
/*void         nact_tree_model_display( NactTreeModel *model, NAObject *object );*/
void         nact_tree_model_display_order_change( NactTreeModel *model, gint order_mode );
/*void         nact_tree_model_iter( NactTreeModel *model, FnIterOnStore fn, gpointer user_data );*/
/* *** */

G_END_DECLS

#endif /* __NACT_TREE_MODEL_H__ */
