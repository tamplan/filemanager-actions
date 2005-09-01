#ifndef NAUTILUS_ACTIONS_H
#define NAUTILUS_ACTIONS_H

#include <glib-object.h>

G_BEGIN_DECLS

#define NAUTILUS_TYPE_ACTIONS  (nautilus_actions_get_type ())
#define NAUTILUS_ACTIONS(o)	 (G_TYPE_CHECK_INSTANCE_CAST ((o), NAUTILUS_TYPE_ACTIONS, NautilusActions))
#define NAUTILUS_IS_ACTIONS(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), NAUTILUS_TYPE_ACTIONS))

typedef struct _NautilusActions	NautilusActions;
typedef struct _NautilusActionsClass NautilusActionsClass;

struct _NautilusActions 
{
	GObject __parent;
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
