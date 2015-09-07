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

#include "api/na-object.h"

#include "nact-main-window-def.h"

G_BEGIN_DECLS

#define NACT_TYPE_TREE_MODEL                ( nact_tree_model_get_type())
#define NACT_TREE_MODEL( object )           ( G_TYPE_CHECK_INSTANCE_CAST(( object ), NACT_TYPE_TREE_MODEL, NactTreeModel ))
#define NACT_TREE_MODEL_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST(( klass ), NACT_TYPE_TREE_MODEL, NactTreeModelClass ))
#define NACT_IS_TREE_MODEL( object )        ( G_TYPE_CHECK_INSTANCE_TYPE(( object ), NACT_TYPE_TREE_MODEL ))
#define NACT_IS_TREE_MODEL_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), NACT_TYPE_TREE_MODEL ))
#define NACT_TREE_MODEL_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), NACT_TYPE_TREE_MODEL, NactTreeModelClass ))

typedef struct _NactTreeModelPrivate        NactTreeModelPrivate;

typedef struct {
	/*< private >*/
	GtkTreeModelFilter      parent;
	NactTreeModelPrivate   *private;
}
	NactTreeModel;

typedef struct {
	/*< private >*/
	GtkTreeModelFilterClass parent;
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

GType          nact_tree_model_get_type        ( void );

NactTreeModel *nact_tree_model_new             ( GtkTreeView *treeview );

void           nact_tree_model_set_main_window ( NactTreeModel *tmodel,
														NactMainWindow *main_window );

void           nact_tree_model_set_edition_mode( NactTreeModel *tmodel,
														guint mode );

GtkTreePath   *nact_tree_model_delete          ( NactTreeModel *model,
														NAObject *object );

void           nact_tree_model_fill            ( NactTreeModel *model,
														GList *items );

GtkTreePath   *nact_tree_model_insert_before   ( NactTreeModel *model,
														const NAObject *object,
														GtkTreePath *path );

GtkTreePath   *nact_tree_model_insert_into     ( NactTreeModel *model,
														const NAObject *object,
														GtkTreePath *path );

NAObjectItem  *nact_tree_model_get_item_by_id  ( const NactTreeModel *model,
														const gchar *id );

GList         *nact_tree_model_get_items       ( const NactTreeModel *model,
														guint mode );

NAObject      *nact_tree_model_object_at_path  ( const NactTreeModel *model,
														GtkTreePath *path );

GtkTreePath   *nact_tree_model_object_to_path  ( const NactTreeModel *model,
														const NAObject *object );

G_END_DECLS

#endif /* __NACT_TREE_MODEL_H__ */
