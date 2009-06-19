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

#ifndef __NACT_WND_ACTIONS_H__
#define __NACT_WND_ACTIONS_H__

/*
 * NactWndActions class definition.
 *
 * This class is derived from NactWindow.
 * It encapsulates the "ActionsDialog" window.
 */

#include "nact-window.h"

G_BEGIN_DECLS

#define NACT_WND_ACTIONS_TYPE					( nact_wnd_actions_get_type())
#define NACT_WND_ACTIONS( object )				( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_WND_ACTIONS_TYPE, NactWndActions ))
#define NACT_WND_ACTIONS_CLASS( klass )			( G_TYPE_CHECK_CLASS_CAST( klass, NACT_WND_ACTIONS_TYPE, NactWndActionsClass ))
#define NACT_IS_WND_ACTIONS( object )			( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_WND_ACTIONS_TYPE ))
#define NACT_IS_WND_ACTIONS_CLASS( klass )		( G_TYPE_CHECK_CLASS_TYPE(( klass ), NACT_WND_ACTIONS_TYPE ))
#define NACT_WND_ACTIONS_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), NACT_WND_ACTIONS_TYPE, NactWndActionsClass ))

typedef struct NactWndActionsPrivate NactWndActionsPrivate;

typedef struct {
	NactWindow             parent;
	NactWndActionsPrivate *private;
}
	NactWndActions;

typedef struct NactWndActionsClassPrivate NactWndActionsClassPrivate;

typedef struct {
	NactWindowClass             parent;
	NactWndActionsClassPrivate *private;
}
	NactWndActionsClass;

GType           nact_wnd_actions_get_type( void );

NactWndActions *nact_wnd_actions_new( GObject *application );

G_END_DECLS

#endif /* __NACT_WND_ACTIONS_H__ */
