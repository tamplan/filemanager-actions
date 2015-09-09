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
 * SECTION: fma_tree_model
 * @short_description: #FMATreeModel class definition.
 * @include: ui/fma-tree-model.h
 *
 * FMATreeModel is derived from GtkTreeModelFilter in order to be able
 * to selectively display profiles, whether an action has one or more
 * profiles.
 *
 * FMATreeModel implements EggTreeMultiDragSource and GtkTreeDragDest
 * interfaces.
 *
 * The GtkTreeModelFilter base class embeds a GtkTreeStore.
 */
#ifndef __UI_FMA_TREE_MODEL_H__
#define __UI_FMA_TREE_MODEL_H__

#include "api/fma-object-item.h"

#include "fma-main-window-def.h"

G_BEGIN_DECLS

#define FMA_TYPE_TREE_MODEL                ( fma_tree_model_get_type())
#define FMA_TREE_MODEL( object )           ( G_TYPE_CHECK_INSTANCE_CAST(( object ), FMA_TYPE_TREE_MODEL, FMATreeModel ))
#define FMA_TREE_MODEL_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST(( klass ), FMA_TYPE_TREE_MODEL, FMATreeModelClass ))
#define FMA_IS_TREE_MODEL( object )        ( G_TYPE_CHECK_INSTANCE_TYPE(( object ), FMA_TYPE_TREE_MODEL ))
#define FMA_IS_TREE_MODEL_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_TREE_MODEL ))
#define FMA_TREE_MODEL_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_TREE_MODEL, FMATreeModelClass ))

typedef struct _FMATreeModelPrivate        FMATreeModelPrivate;

typedef struct {
	/*< private >*/
	GtkTreeModelFilter      parent;
	FMATreeModelPrivate    *private;
}
	FMATreeModel;

typedef struct {
	/*< private >*/
	GtkTreeModelFilterClass parent;
}
	FMATreeModelClass;

/**
 * Column ordering in the tree view
 */
enum {
	TREE_COLUMN_ICON = 0,
	TREE_COLUMN_LABEL,
	TREE_COLUMN_NAOBJECT,
	TREE_N_COLUMN
};

GType          fma_tree_model_get_type        ( void );

FMATreeModel  *fma_tree_model_new             ( GtkTreeView *treeview );

void           fma_tree_model_set_main_window ( FMATreeModel *tmodel,
														FMAMainWindow *main_window );

void           fma_tree_model_set_edition_mode( FMATreeModel *tmodel,
														guint mode );

GtkTreePath   *fma_tree_model_delete          ( FMATreeModel *model,
														FMAObject *object );

void           fma_tree_model_fill            ( FMATreeModel *model,
														GList *items );

GtkTreePath   *fma_tree_model_insert_before   ( FMATreeModel *model,
														const FMAObject *object,
														GtkTreePath *path );

GtkTreePath   *fma_tree_model_insert_into     ( FMATreeModel *model,
														const FMAObject *object,
														GtkTreePath *path );

FMAObjectItem *fma_tree_model_get_item_by_id  ( const FMATreeModel *model,
														const gchar *id );

GList         *fma_tree_model_get_items       ( const FMATreeModel *model,
														guint mode );

FMAObject     *fma_tree_model_object_at_path  ( const FMATreeModel *model,
														GtkTreePath *path );

GtkTreePath   *fma_tree_model_object_to_path  ( const FMATreeModel *model,
														const FMAObject *object );

G_END_DECLS

#endif /* __UI_FMA_TREE_MODEL_H__ */
