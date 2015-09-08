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
 * @short_description: #NactTreeModel private data definition.
 * @include: nact/nact-tree-model-priv.h
 */

#ifndef __UI_NACT_TREE_MODEL_PRIV_H__
#define __UI_NACT_TREE_MODEL_PRIV_H__

#include "egg-tree-multi-dnd.h"
#include "nact-clipboard.h"

G_BEGIN_DECLS

/* private instance data
 */
struct _NactTreeModelPrivate {
	gboolean        dispose_has_run;

	/* data set at instanciation time
	 */
	GtkTreeView    *treeview;
	gboolean        item_updated_connected;
	gboolean        dnd_setup;

	/* must be initialized right after the instanciation
	 */
	NactMainWindow *window;
	NactClipboard  *clipboard;
	guint           mode;

	/* runtime data
	 */
	gboolean        drag_has_profiles;
	gboolean        drag_highlight;		/* defined for on_drag_motion handler */
	gboolean        drag_drop;			/* defined for on_drag_motion handler */
};

#define TREE_MODEL_STATUSBAR_CONTEXT	"nact-tree-model-statusbar-context"

gboolean       nact_tree_model_dnd_idrag_dest_drag_data_received( GtkTreeDragDest *drag_dest, GtkTreePath *dest, GtkSelectionData  *selection_data );
gboolean       nact_tree_model_dnd_idrag_dest_row_drop_possible( GtkTreeDragDest *drag_dest, GtkTreePath *dest_path, GtkSelectionData *selection_data );

gboolean       nact_tree_model_dnd_imulti_drag_source_drag_data_get( EggTreeMultiDragSource *drag_source, GdkDragContext *context, GtkSelectionData *selection_data, GList *path_list, guint info );
gboolean       nact_tree_model_dnd_imulti_drag_source_drag_data_delete( EggTreeMultiDragSource *drag_source, GList *path_list );
GdkDragAction  nact_tree_model_dnd_imulti_drag_source_get_drag_actions( EggTreeMultiDragSource *drag_source );
GtkTargetList *nact_tree_model_dnd_imulti_drag_source_get_format_list( EggTreeMultiDragSource *drag_source );
gboolean       nact_tree_model_dnd_imulti_drag_source_row_draggable( EggTreeMultiDragSource *drag_source, GList *path_list );

void           nact_tree_model_dnd_on_drag_begin( GtkWidget *widget, GdkDragContext *context, BaseWindow *window );
/*gboolean       on_drag_motion( GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, BaseWindow *window );*/
/*gboolean       on_drag_drop( GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, BaseWindow *window );*/
void           nact_tree_model_dnd_on_drag_end( GtkWidget *widget, GdkDragContext *context, BaseWindow *window );

G_END_DECLS

#endif /* __UI_NACT_TREE_MODEL_PRIV_H__ */
