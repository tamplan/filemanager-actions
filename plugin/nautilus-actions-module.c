#include <libnautilus-extension/nautilus-extension-types.h>
#include <libnautilus-extension/nautilus-column-provider.h>
#include "nautilus-actions.h"
#include "nautilus-actions-utils.h"

void nautilus_module_initialize (GTypeModule*module)
{
	nautilus_actions_register_type (module);
}

void nautilus_module_shutdown (void)
{
}

void nautilus_module_list_types (const GType **types, int *num_types)
{
	static GType type_list[1];
	
	type_list[0] = NAUTILUS_TYPE_ACTIONS;
	*types = type_list;

	*num_types = 1;
}

// vim:ts=3:sw=3:tw=1024
