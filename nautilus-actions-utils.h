#ifndef NAUTILUS_ACTIONS_UTILS_H
#define NAUTILUS_ACTIONS_UTILS_H

#include <glib.h>
#include "nautilus-actions-config.h"

G_BEGIN_DECLS

gchar* nautilus_actions_utils_parse_parameter (const gchar* param_template, GList* files);

gboolean nautilus_actions_utils_parse_isfile (const gchar* value2parse, IsFileType *value2set); /* not used */

gboolean nautilus_actions_utils_parse_boolean (const gchar* value2parse, gboolean *value2set);

void nautilus_actions_utils_debug (gchar* message);

G_END_DECLS

#endif /* NAUTILUS_ACTIONS_UTILS_H */

// vim:ts=3:sw=3:tw=1024
