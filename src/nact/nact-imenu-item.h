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

#ifndef __NACT_IMENU_ITEM_H__
#define __NACT_IMENU_ITEM_H__

/*
 * NactIMenuItem interface definition.
 *
 * This interface implements the "Nautilus Menu Item" box.
 */

#include <common/na-action.h>

#include "nact-window.h"

G_BEGIN_DECLS

#define NACT_IMENU_ITEM_TYPE						( nact_imenu_item_get_type())
#define NACT_IMENU_ITEM( object )					( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_IMENU_ITEM_TYPE, NactIMenuItem ))
#define NACT_IS_IMENU_ITEM( object )				( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_IMENU_ITEM_TYPE ))
#define NACT_IMENU_ITEM_GET_INTERFACE( instance )	( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NACT_IMENU_ITEM_TYPE, NactIMenuItemInterface ))

typedef struct NactIMenuItem NactIMenuItem;

typedef struct NactIMenuItemInterfacePrivate NactIMenuItemInterfacePrivate;

typedef struct {
	GTypeInterface                 parent;
	NactIMenuItemInterfacePrivate *private;

	/* api */
	GObject * ( *get_edited_action )( NactWindow *window );
	void      ( *field_modified )   ( NactWindow *window );
}
	NactIMenuItemInterface;

GType    nact_imenu_item_get_type( void );

void     nact_imenu_item_initial_load( NactWindow *dialog, NAAction *action );
void     nact_imenu_item_size_labels( NactWindow *window, GObject *size_group );
void     nact_imenu_item_size_buttons( NactWindow *window, GObject *size_group );

void     nact_imenu_item_runtime_init( NactWindow *dialog, NAAction *action );
void     nact_imenu_item_all_widgets_showed( NactWindow *dialog );

gboolean nact_imenu_item_has_label( NactWindow *window );

void     nact_imenu_item_dispose( NactWindow *dialog );

G_END_DECLS

#endif /* __NACT_IMENU_ITEM_H__ */
