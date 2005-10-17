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

#ifndef _NAUTILUS_ACTIONS_CONFIG_SCHEMA_H_
#define _NAUTILUS_ACTIONS_CONFIG_SCHEMA_H_

#include <glib/glist.h>
#include <glib-object.h>
#include <gconf/gconf-client.h>
#include "nautilus-actions-config.h"

G_BEGIN_DECLS

#define NAUTILUS_ACTIONS_TYPE_CONFIG_SCHEMA            (nautilus_actions_config_schema_get_type())
#define NAUTILUS_ACTIONS_CONFIG_SCHEMA(obj)            (G_TYPE_CHECK_INSTANCE_CAST (obj, NAUTILUS_ACTIONS_TYPE_CONFIG_SCHEMA, NautilusActionsConfigSchema))
#define NAUTILUS_ACTIONS_CONFIG_SCHEMA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST (klass, NAUTILUS_ACTIONS_TYPE_CONFIG_SCHEMA, NautilusActionsConfigSchemaClass))
#define NAUTILUS_ACTIONS_IS_CONFIG_SCHEMA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE(obj, NAUTILUS_ACTIONS_TYPE_CONFIG_SCHEMA))
#define NAUTILUS_ACTIONS_IS_CONFIG_SCHEMA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), NAUTILUS_ACTIONS_TYPE_CONFIG_SCHEMA))

typedef struct _NautilusActionsConfigSchema NautilusActionsConfigSchema;
typedef struct _NautilusActionsConfigSchemaClass NautilusActionsConfigSchemaClass;

struct _NautilusActionsConfigSchema {
	NautilusActionsConfig parent;

	/* Private data, don't access */
	gchar* save_path;
};

struct _NautilusActionsConfigSchemaClass {
	NautilusActionsConfigClass parent_class;
};

GType                        nautilus_actions_config_schema_get_type (void);
NautilusActionsConfigSchema  *nautilus_actions_config_schema_get (void);
gchar*							  nautilus_actions_config_schema_get_saved_filename (NautilusActionsConfigSchema* self, gchar* uuid);

G_END_DECLS

#endif

// vim:ts=3:sw=3:tw=1024:cin
