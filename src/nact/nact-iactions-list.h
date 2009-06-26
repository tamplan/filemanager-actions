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

/*
 * NactIActionsList interface definition.
 *
 * This interface defines some API against the ActionsList listbox.
 * Our NactWindow may implement it in order to personalize the
 * behaviour of the listbox.
 */

#include <gtk/gtk.h>

#include "nact-window.h"

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

	/* api */
	void     ( *init_widget )          ( NactWindow *window );
	void     ( *fill_actions_list )    ( NactWindow *window );
	void     ( *on_selection_changed ) ( GtkTreeSelection *selection, gpointer user_data );
	gboolean ( *on_button_press_event )( GtkWidget *widget, GdkEventButton *event, gpointer data );
	gboolean ( *on_double_click )      ( GtkWidget *widget, GdkEventButton *event, gpointer data );
}
	NactIActionsListInterface;

GType nact_iactions_list_get_type( void );

void  nact_iactions_list_init( NactWindow *window );
void  nact_iactions_list_fill( NactWindow *window );

G_END_DECLS

#endif /* __NACT_IACTIONS_LIST_H__ */
