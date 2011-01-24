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

#ifndef __PLUGIN_MENU_NAUTILUS_ACTIONS_H__
#define __PLUGIN_MENU_NAUTILUS_ACTIONS_H__

/**
 * SECTION: nautilus-actions
 * @title: NautilusActions
 * @short_description: The NautilusActions plugin class definition
 * @include: plugin-menu/nautilus-actions.h
 *
 * This is the class which handles the file manager menu plugin.
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define NAUTILUS_ACTIONS_TYPE                ( nautilus_actions_get_type())
#define NAUTILUS_ACTIONS( object )           ( G_TYPE_CHECK_INSTANCE_CAST(( object ), NAUTILUS_ACTIONS_TYPE, NautilusActions ))
#define NAUTILUS_ACTIONS_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST(( klass ), NAUTILUS_ACTIONS_TYPE, NautilusActionsClass ))
#define NAUTILUS_IS_ACTIONS( object )        ( G_TYPE_CHECK_INSTANCE_TYPE(( object ), NAUTILUS_ACTIONS_TYPE ))
#define NAUTILUS_IS_ACTIONS_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), NAUTILUS_ACTIONS_TYPE ))
#define NAUTILUS_ACTIONS_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), NAUTILUS_ACTIONS_TYPE, NautilusActionsClass ))

typedef struct _NautilusActionsPrivate       NautilusActionsPrivate;

typedef struct {
	/*< private >*/
	GObject                 parent;
	NautilusActionsPrivate *private;
}
	NautilusActions;

typedef struct _NautilusActionsClassPrivate  NautilusActionsClassPrivate;

typedef struct {
	/*< private >*/
	GObjectClass                 parent;
	NautilusActionsClassPrivate *private;
}
	NautilusActionsClass;

GType nautilus_actions_get_type     ( void );
void  nautilus_actions_register_type( GTypeModule *module );

G_END_DECLS

#endif /* __PLUGIN_MENU_NAUTILUS_ACTIONS_H__ */
