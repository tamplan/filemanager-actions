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

#ifndef __NACT_IMENUBAR_H__
#define __NACT_IMENUBAR_H__

/*
 * NactIMenubar interface definition.
 */

#include <gtk/gtk.h>

#include "nact-main-window.h"

G_BEGIN_DECLS

#define NACT_IMENUBAR_TYPE						( nact_imenubar_get_type())
#define NACT_IMENUBAR( object )					( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_IMENUBAR_TYPE, NactIMenubar ))
#define NACT_IS_IMENUBAR( object )				( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_IMENUBAR_TYPE ))
#define NACT_IMENUBAR_GET_INTERFACE( instance )	( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NACT_IMENUBAR_TYPE, NactIMenubarInterface ))

typedef struct NactIMenubar NactIMenubar;

typedef struct NactIMenubarInterfacePrivate NactIMenubarInterfacePrivate;

typedef struct {
	GTypeInterface                parent;
	NactIMenubarInterfacePrivate *private;

	/* api */
	void        ( *add_action )            ( NactWindow *window, NAAction* action );
	void        ( *add_profile )           ( NactWindow *window, NAActionProfile *profile );
	void        ( *remove_action )         ( NactWindow *window, NAAction *action );
	GSList *    ( *get_deleted_actions )   ( NactWindow *window );
	void        ( *free_deleted_actions )  ( NactWindow *window );
	void        ( *push_removed_action )   ( NactWindow *window, NAAction *action );
	GSList *    ( *get_actions )           ( NactWindow *window );
	NAObject *  ( *get_selected )          ( NactWindow *window );
	void        ( *setup_dialog_title )    ( NactWindow *window );
	void        ( *update_actions_list )   ( NactWindow *window );
	void        ( *select_actions_list )   ( NactWindow *window, GType type, const gchar *uuid, const gchar *label );
	gint        ( *count_actions )         ( NactWindow *window );
	gint        ( *count_modified_actions )( NactWindow *window );
	void        ( *reload_actions )        ( NactWindow *window );
}
	NactIMenubarInterface;

GType nact_imenubar_get_type( void );

void  nact_imenubar_init( NactMainWindow *window );

void  nact_imenubar_on_delete_event( NactWindow *window );

G_END_DECLS

#endif /* __NACT_IMENUBAR_H__ */
