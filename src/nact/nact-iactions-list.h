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

#ifndef __NACT_IACTIONS_LIST_H__
#define __NACT_IACTIONS_LIST_H__

#include <common/na-object-class.h>

G_BEGIN_DECLS

#define NACT_IACTIONS_LIST_TYPE							( nact_iactions_list_get_type())
#define NACT_IACTIONS_LIST( object )					( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_IACTIONS_LIST_TYPE, NactIActionsList ))
#define NACT_IS_IACTIONS_LIST( object )					( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_IACTIONS_LIST_TYPE ))
#define NACT_IACTIONS_LIST_GET_INTERFACE( instance )	( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NACT_IACTIONS_LIST_TYPE, NactIActionsListInterface ))

typedef struct NactIActionsList NactIActionsList;

typedef struct NactIActionsListInterfacePrivate NactIActionsListInterfacePrivate;

typedef struct {
	GTypeInterface                    parent;
	NactIActionsListInterfacePrivate *private;

	/**
	 * selection_changed:
	 * @instance: this #NactIActionsList instance.
	 * @selected_items: currently selected items.
	 */
	void ( *selection_changed )( NactIActionsList *instance, GSList *selected_items );

	/**
	 * item_updated:
	 * @instance: this #NactIActionsList instance.
	 * @object: the modified #NAObject.
	 */
	void ( *item_updated )     ( NactIActionsList *instance, NAObject *object );
}
	NactIActionsListInterface;

/* signals
 */
#define IACTIONS_LIST_SIGNAL_SELECTION_CHANGED			"nact-iactions-list-selection-changed"
#define IACTIONS_LIST_SIGNAL_ITEM_UPDATED				"nact-iactions-list-item-updated"

GType     nact_iactions_list_get_type( void );

void      nact_iactions_list_initial_load_toplevel( NactIActionsList *instance );
void      nact_iactions_list_runtime_init_toplevel( NactIActionsList *instance, GList *actions );
void      nact_iactions_list_all_widgets_showed( NactIActionsList *instance );
void      nact_iactions_list_dispose( NactIActionsList *instance );

void      nact_iactions_list_delete_selection( NactIActionsList *instance );
void      nact_iactions_list_fill( NactIActionsList *instance, GList *items );
GList    *nact_iactions_list_get_items( NactIActionsList *instance );
GList    *nact_iactions_list_get_selected_items( NactIActionsList *instance );
gboolean  nact_iactions_list_has_exportable( NactIActionsList *instance );
gboolean  nact_iactions_list_has_modified_items( NactIActionsList *instance );
void      nact_iactions_list_insert_items( NactIActionsList *instance, GList *items, NAObject *sibling );
gboolean  nact_iactions_list_is_expanded( NactIActionsList *instance, const NAObject *item );
gboolean  nact_iactions_list_is_only_actions_mode( NactIActionsList *instance );
void      nact_iactions_list_select_row( NactIActionsList *instance, GtkTreePath *path );
void      nact_iactions_list_set_dnd_mode( NactIActionsList *instance, gboolean have_dnd );
void      nact_iactions_list_set_filter_selection_mode( NactIActionsList *instance, gboolean filter );
void      nact_iactions_list_set_multiple_selection_mode( NactIActionsList *instance, gboolean multiple );
void      nact_iactions_list_set_only_actions_mode( NactIActionsList *instance, gboolean only_actions );
void      nact_iactions_list_toggle_collapse( NactIActionsList *instance, const NAObject *item );

G_END_DECLS

#endif /* __NACT_IACTIONS_LIST_H__ */
