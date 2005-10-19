/* Nautilus Actions new config tool
 * Copyright (C) 2005 The GNOME Foundation
 *
 * Authors:
 *  Frederic Ruaudel (grumz@grumz.net)
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
	
	type_list[0] = NAUTILUS_ACTIONS_TYPE;
	*types = type_list;

	*num_types = 1;
}

// vim:ts=3:sw=3:tw=1024
