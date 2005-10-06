/* Nautilus Actions configuration tool
 * Copyright (C) 2005 The GNOME Foundation
 *
 * Authors:
 *  Frederic Ruaudel (grumz@grumz.net)
 *	 Rodrigo Moya (rodrigo@gnome-db.org)
 *
 * This Program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this Library; see the file COPYING.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef NAUTILUS_ACTIONS_H
#define NAUTILUS_ACTIONS_H

#include <glib-object.h>
#include <libnautilus-actions/nautilus-actions-config-gconf.h>

G_BEGIN_DECLS

#define NAUTILUS_ACTIONS_TYPE  				(nautilus_actions_get_type ())
#define NAUTILUS_ACTIONS(o)	 				(G_TYPE_CHECK_INSTANCE_CAST ((o), NAUTILUS_ACTIONS_TYPE, NautilusActions))
#define NAUTILUS_ACTIONS_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), NAUTILUS_ACTIONS_TYPE, NautilusActionsClass))
#define NAUTILUS_IS_ACTIONS(o) 				(G_TYPE_CHECK_INSTANCE_TYPE ((o), NAUTILUS_ACTIONS_TYPE))
#define NAUTILUS_IS_ACTIONS_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), NAUTILUS_ACTIONS_TYPE))
#define NAUTILUS_ACTIONS_GET_CLASS(o)		(G_TYPE_INSTANCE_GET_CLASS ((obj), NAUTILUS_ACTIONS_TYPE, NautilusActionsClass))

typedef struct _NautilusActions	NautilusActions;
typedef struct _NautilusActionsClass NautilusActionsClass;

struct _NautilusActions 
{
	GObject __parent;
	NautilusActionsConfigGconf* configs;
	gboolean dispose_has_run;
};

struct _NautilusActionsClass
{
	GObjectClass __parent;
};

GType nautilus_actions_get_type (void);
void nautilus_actions_register_type (GTypeModule *module);

G_END_DECLS

#endif /* NAUTILUS_ACTIONS_H */

// vim:ts=3:sw=3:tw=1024
