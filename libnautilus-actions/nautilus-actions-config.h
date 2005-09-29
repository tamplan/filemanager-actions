/* Nautilus Actions configuration tool
 * Copyright (C) 2005 The GNOME Foundation
 *
 * Authors:
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

#ifndef _NAUTILUS_ACTIONS_CONFIG_H_
#define _NAUTILUS_ACTIONS_CONFIG_H_

#include <glib/glist.h>
#include <glib-object.h>
#include <gconf/gconf-client.h>

G_BEGIN_DECLS

#define NAUTILUS_ACTIONS_TYPE_CONFIG            (nautilus_actions_config_get_type())
#define NAUTILUS_ACTIONS_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST (obj, NAUTILUS_ACTIONS_TYPE_CONFIG, NautilusActionsConfig))
#define NAUTILUS_ACTIONS_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST (klass, NAUTILUS_ACTIONS_TYPE_CONFIG, NautilusActionsConfigClass))
#define NAUTILUS_ACTIONS_IS_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE(obj, NAUTILUS_ACTIONS_TYPE_CONFIG))
#define NAUTILUS_ACTIONS_IS_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), NAUTILUS_ACTIONS_TYPE_CONFIG))

typedef struct {
	gchar *conf_section;
	gchar *label;
	gchar *tooltip;
	gchar *path;
	gchar *parameters;
	GSList *basenames;
	gboolean is_dir;
	gboolean is_file;
	gboolean accept_multiple_files;
	GSList *schemes;
	gchar *version;
} NautilusActionsConfigAction;

typedef struct _NautilusActionsConfig NautilusActionsConfig;
typedef struct _NautilusActionsConfigClass NautilusActionsConfigClass;

struct _NautilusActionsConfig {
	GObject parent;

	/* Private data, don't access */
	GConfClient *conf_client;
	GHashTable *actions;
	guint actions_notify_id;
};

struct _NautilusActionsConfigClass {
	GObjectClass parent_class;

	void (* action_added) (NautilusActionsConfig *config, NautilusActionsConfigAction *action);
	void (* action_changed) (NautilusActionsConfig *config, NautilusActionsConfigAction *action);
	void (* action_removed) (NautilusActionsConfig *config, NautilusActionsConfigAction *action);
};

GType                        nautilus_actions_config_get_type (void);
NautilusActionsConfig       *nautilus_actions_config_get (void);

NautilusActionsConfigAction *nautilus_actions_config_get_action (NautilusActionsConfig *config, const gchar *label);
GSList                      *nautilus_actions_config_get_actions (NautilusActionsConfig *config);
void                         nautilus_actions_config_free_actions_list (GSList *list);
gboolean                     nautilus_actions_config_add_action (NautilusActionsConfig *config,
								 NautilusActionsConfigAction *action);
gboolean                     nautilus_actions_config_update_action (NautilusActionsConfig *config,
								    NautilusActionsConfigAction *action);
gboolean                     nautilus_actions_config_remove_action (NautilusActionsConfig *config,
								    const gchar *label);

NautilusActionsConfigAction *nautilus_actions_config_action_new (void);
void                         nautilus_actions_config_action_set_label (NautilusActionsConfigAction *action,
								       const gchar *label);
void                         nautilus_actions_config_action_set_tooltip (NautilusActionsConfigAction *action,
									 const gchar *tooltip);
void                         nautilus_actions_config_action_set_path (NautilusActionsConfigAction *action,
								      const gchar *path);
void                         nautilus_actions_config_action_set_parameters (NautilusActionsConfigAction *action,
									    const gchar *parameters);

#define nautilus_actions_config_action_set_is_dir(action, b) { if ((action)) (action)->is_dir = b; }
#define nautilus_actions_config_action_set_is_file(action, b) { if ((action)) (action)->is_file = b; }
#define nautilus_actions_config_action_set_accept_multiple(action, b) { if ((action)) (action)->accept_multiple_files = b; }

void                         nautilus_actions_config_action_free (NautilusActionsConfigAction *action);

G_END_DECLS

#endif
