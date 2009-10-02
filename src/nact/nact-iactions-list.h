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

/**
 * SECTION: nact_iactions_list
 * @short_description: #NactIActionsList interface definition.
 * @include: nact/nact-iactions-list.h
 *
 * This same interface is used in the main window (edition mode, default),
 * and in the export assistant (export mode).
 */

#include <gtk/gtk.h>

#include <runtime/na-object-class.h>

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
	 * @instance: this #NactIActionsList implementor.
	 * @selected_items: currently selected items.
	 *
	 * This function is an exact duplicate of the
	 * "nact-iactions-list-selection-changed" signal. Implementor may
	 * choose to to connect to the signal, or to implement this api, in
	 * order to be triggered when the selection is modified in the
	 * treeview.
	 *
	 * Be warned that both implementing the api and connecting to the
	 * signal must be considered at least as useless, and just a way to
	 * be triggered twice for the same event.
	 */
	void    ( *selection_changed )( NactIActionsList *instance, GSList *selected_items );

	/**
	 * get_treeview_name:
	 * @instance: this #NactIActionsList implementor.
	 *
	 * Returns: a newly allocated string, which contains the treeview
	 * widget name in its XML UI definition.
	 *
	 * The returned string will be g_free() by IActionsList interface.
	 *
	 * This is a pure virtual function which must be implemented.
	 */
	gchar * ( *get_treeview_name )( NactIActionsList *instance );
}
	NactIActionsListInterface;

/* signals
 */
#define IACTIONS_LIST_SIGNAL_SELECTION_CHANGED			"nact-iactions-list-selection-changed"

/* management modes
 * - edition: dnd, filter, multiple selection, item updated signal
 * - export: multiple selection
 */
enum {
	IACTIONS_LIST_MANAGEMENT_MODE_EDITION = 1,
	IACTIONS_LIST_MANAGEMENT_MODE_EXPORT
};

GType     nact_iactions_list_get_type( void );

void      nact_iactions_list_initial_load_toplevel( NactIActionsList *instance );
void      nact_iactions_list_runtime_init_toplevel( NactIActionsList *instance, GList *actions );
void      nact_iactions_list_all_widgets_showed( NactIActionsList *instance );
void      nact_iactions_list_dispose( NactIActionsList *instance );

void      nact_iactions_list_collapse_all( NactIActionsList *instance );
void      nact_iactions_list_delete( NactIActionsList *instance, GList *items );
void      nact_iactions_list_display_order_change( NactIActionsList *instance, gint order_mode );
void      nact_iactions_list_expand_all( NactIActionsList *instance );
void      nact_iactions_list_fill( NactIActionsList *instance, GList *items );
NAObject *nact_iactions_list_get_item( NactIActionsList *instance, const gchar *uuid );
GList    *nact_iactions_list_get_items( NactIActionsList *instance );
gint      nact_iactions_list_get_management_mode( NactIActionsList *instance );
GList    *nact_iactions_list_get_selected_items( NactIActionsList *instance );
gboolean  nact_iactions_list_has_exportable( NactIActionsList *instance );
gboolean  nact_iactions_list_has_modified_items( NactIActionsList *instance );
void      nact_iactions_list_insert_items( NactIActionsList *instance, GList *items, NAObject *sibling );
void      nact_iactions_list_insert_at_path( NactIActionsList *instance, GList *items, GtkTreePath *path, gboolean inside );
gboolean  nact_iactions_list_is_expanded( NactIActionsList *instance, const NAObject *item );
void      nact_iactions_list_set_management_mode( NactIActionsList *instance, gint mode );
void      nact_iactions_list_toggle_collapse( NactIActionsList *instance, const NAObject *item );

G_END_DECLS

#endif /* __NACT_IACTIONS_LIST_H__ */
