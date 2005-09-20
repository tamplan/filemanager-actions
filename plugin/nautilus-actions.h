#ifndef NAUTILUS_ACTIONS_H
#define NAUTILUS_ACTIONS_H

#include <glib-object.h>
#include <gconf/gconf-client.h>

G_BEGIN_DECLS

#define NAUTILUS_ACTIONS_TYPE  				(nautilus_actions_get_type ())
#define NAUTILUS_ACTIONS(o)	 				(G_TYPE_CHECK_INSTANCE_CAST ((o), NAUTILUS_ACTIONS_TYPE, NautilusActions))
#define NAUTILUS_ACTIONS_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), NAUTILUS_ACTIONS_TYPE, NautilusActionsClass))
#define NAUTILUS_IS_ACTIONS(o) 				(G_TYPE_CHECK_INSTANCE_TYPE ((o), NAUTILUS_ACTIONS_TYPE))
#define NAUTILUS_IS_ACTIONS_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), NAUTILUS_ACTIONS_TYPE))
#define NAUTILUS_ACTIONS_GET_CLASS(o)		(G_TYPE_INSTANCE_GET_CLASS ((obj), NAUTILUS_ACTIONS_TYPE, NautilusActionsClass))

#ifndef NAUTILUS_ACTIONS_GCONF_PATH
#define NAUTILUS_ACTIONS_GCONF_PATH "/apps/nautilus-actions"
#endif

#ifndef NAUTILUS_ACTIONS_GCONF_CONFIG_DIR
#define NAUTILUS_ACTIONS_GCONF_CONFIG_DIR "configurations"
#endif

typedef struct _NautilusActions	NautilusActions;
typedef struct _NautilusActionsClass NautilusActionsClass;

struct _NautilusActions 
{
	GObject __parent;
	gchar* config_root_dir;
	GConfClient* gconf_client;
	GList* configs;
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
