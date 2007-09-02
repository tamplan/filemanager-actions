/* Nautilus Actions tool for checking actions change
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

#include <config.h>
#include <libnautilus-actions/nautilus-actions-config.h>
#include <libnautilus-actions/nautilus-actions-config-gconf-reader.h>

static void nautilus_actions_action_added_handler (NautilusActionsConfig* config, 
																				NautilusActionsConfigAction* action,
																				gpointer user_data)
{
	printf ("Action added: <>\n");
	//nautilus_actions_config_free_actions_list (self->config_list);
	//self->config_list = nautilus_actions_config_get_actions (NAUTILUS_ACTIONS_CONFIG (self->configs));
}

static void nautilus_actions_action_changed_handler (NautilusActionsConfig* config, 
																				NautilusActionsConfigAction* action,
																				gpointer user_data)
{
	printf ("Action changed: <%s>\n", action->label);
	NautilusActionsConfigAction *cur_action = nautilus_actions_config_get_action (config, action->uuid);
	printf ("Action changed: cur <%s>\n", cur_action->label);
	//nautilus_actions_config_free_actions_list (self->config_list);
	//self->config_list = nautilus_actions_config_get_actions (NAUTILUS_ACTIONS_CONFIG (self->configs));
}

static void nautilus_actions_action_removed_handler (NautilusActionsConfig* config, 
																				NautilusActionsConfigAction* action,
																				gpointer user_data)
{
	printf ("Action removed: <>\n");
	//nautilus_actions_config_free_actions_list (self->config_list);
	//self->config_list = nautilus_actions_config_get_actions (NAUTILUS_ACTIONS_CONFIG (self->configs));
}


int main (int argc, char **argv)
{
	NautilusActionsConfigGconfReader* config;

	/* Initialize the widget set */
	gtk_init (&argc, &argv);

	printf ("Tracking for Action changes started... (Type Ctrl+C to stop)\n");

	config = nautilus_actions_config_gconf_reader_get ();

	g_signal_connect_after (G_OBJECT (config), "action_added",
									(GCallback)nautilus_actions_action_added_handler,
									NULL);
	g_signal_connect_after (G_OBJECT (config), "action_changed",
									(GCallback)nautilus_actions_action_changed_handler,
									NULL);
	g_signal_connect_after (G_OBJECT (config), "action_removed",
									(GCallback)nautilus_actions_action_removed_handler,
									NULL);

	/* Enter the main event loop, and wait for user interaction */
	gtk_main ();

	/* The user lost interest */
	return 0;
}

// vim:ts=3:sw=3:tw=1024:cin
