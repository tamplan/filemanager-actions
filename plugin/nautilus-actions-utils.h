#ifndef NAUTILUS_ACTIONS_UTILS_H
#define NAUTILUS_ACTIONS_UTILS_H

#include <glib.h>
#include "nautilus-actions-config.h"

G_BEGIN_DECLS

gchar* nautilus_actions_utils_parse_parameter (const gchar* param_template, GList* files);

gint nautilus_actions_utils_compare_actions (const ConfigAction* action1, const gchar* action_name);

G_END_DECLS

#endif /* NAUTILUS_ACTIONS_UTILS_H */

// vim:ts=3:sw=3:tw=1024
