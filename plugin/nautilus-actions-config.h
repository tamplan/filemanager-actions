#ifndef NAUTILUS_ACTIONS_CONFIG_H
#define NAUTILUS_ACTIONS_CONFIG_H

#include <glib-object.h>
#include <string.h>
#include <gconf/gconf-client.h>

G_BEGIN_DECLS

#ifndef DEFAULT_PER_USER_PATH 
#define DEFAULT_PER_USER_PATH ".config/nautilus-actions"
#endif

typedef struct _ConfigAction ConfigAction;
typedef struct _ConfigActionTest ConfigActionTest;
typedef struct _ConfigActionCommand ConfigActionCommand;
typedef struct _ConfigActionMenuItem ConfigActionMenuItem;

struct _ConfigAction
{
	gchar* name; /* name must be uniq */
	gchar* version;
	ConfigActionTest *test;
	ConfigActionCommand *command;
	ConfigActionMenuItem *menu_item;
};

struct _ConfigActionTest
{
	GSList* basenames;
	gboolean isfile;
	gboolean isdir;
	gboolean accept_multiple_file;
	GSList* schemes;
};

struct _ConfigActionCommand
{
	gchar* path;
	gchar* parameters;
};

struct _ConfigActionMenuItem
{
	gchar* label;
	gchar* tooltip;
};

GList *nautilus_actions_config_get_list (GConfClient* gconf_client, const gchar* config_root_dir);
ConfigAction *nautilus_actions_config_action_dup (ConfigAction* action);
void nautilus_actions_config_free_list (GList* config_actions);
void nautilus_actions_config_free_action (ConfigAction* action);

G_END_DECLS

#endif /* NAUTILUS_ACTIONS_CONFIG_H */

// vim:ts=3:sw=3:tw=1024
