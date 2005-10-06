/* Nautilus Actions configuration tool
 * Copyright (C) 2005 The GNOME Foundation
 *
 * Authors:
 *	 Rodrigo Moya (rodrigo@gnome-db.org)
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

#include <glib/gi18n.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkentry.h>
#include <gtk/gtktogglebutton.h>
#include <glade/glade-xml.h>
#include "nact-editor.h"

static GtkWidget *editor = NULL, *menu_label, *menu_tooltip, *command_path, *command_params,
	*only_files, *only_folders, *both, *accept_multiple;

static void
field_changed_cb (GObject *editable, gpointer user_data)
{
	gchar *label = gtk_entry_get_text (GTK_ENTRY (menu_label));

	if (label && strlen (label) > 0)
		gtk_dialog_set_response_sensitive (GTK_DIALOG (editor), GTK_RESPONSE_OK, TRUE);
	else
		gtk_dialog_set_response_sensitive (GTK_DIALOG (editor), GTK_RESPONSE_OK, FALSE);
}

static void
legend_button_clicked_cb (GtkButton *button, gpointer user_data)
{
	GladeXML *gui;
	GtkWidget *legend_dialog;

	gui = glade_xml_new (GLADEDIR "/nautilus-actions-config.glade", "LegendDialog", NULL);
	if (!gui)
		return;

	legend_dialog = glade_xml_get_widget (gui, "LegendDialog");
	gtk_window_set_transient_for (GTK_WINDOW (legend_dialog), GTK_WINDOW (editor));

	gtk_dialog_run (GTK_DIALOG (legend_dialog));

	gtk_widget_destroy (legend_dialog);
	g_object_unref (gui);
}

static gboolean
open_editor (NautilusActionsConfigAction *action, gboolean is_new)
{
	GladeXML *gui;
	gboolean ret = FALSE;
	gchar *label;
	NautilusActionsConfigGconf *config;

	/* only allow one dialog at a time */
	if (editor != NULL)
		return FALSE;

	/* load the GUI */
	gui = glade_xml_new (GLADEDIR "/nautilus-actions-config.glade", "EditActionDialog", NULL);
	if (!gui) {
		nautilus_actions_display_error (_("Could not load interface for Nautilus Actions Config Tool"));
		return FALSE;
	}

	editor = glade_xml_get_widget (gui, "EditActionDialog");

	menu_label = glade_xml_get_widget (gui, "MenuLabelEntry");
	gtk_entry_set_text (GTK_ENTRY (menu_label), action->label);
	g_signal_connect (G_OBJECT (menu_label), "changed", G_CALLBACK (field_changed_cb), NULL);

	menu_tooltip = glade_xml_get_widget (gui, "MenuTooltipEntry");
	gtk_entry_set_text (GTK_ENTRY (menu_tooltip), action->tooltip);
	g_signal_connect (G_OBJECT (menu_label), "changed", G_CALLBACK (field_changed_cb), NULL);

	command_path = glade_xml_get_widget (gui, "CommandPathEntry");
	gtk_entry_set_text (GTK_ENTRY (command_path), action->path);
	g_signal_connect (G_OBJECT (command_path), "changed", G_CALLBACK (field_changed_cb), NULL);

	command_params = glade_xml_get_widget (gui, "CommandParamsEntry");
	gtk_entry_set_text (GTK_ENTRY (command_params), action->parameters);
	g_signal_connect (G_OBJECT (command_params), "changed", G_CALLBACK (field_changed_cb), NULL);

	only_folders = glade_xml_get_widget (gui, "OnlyFoldersButton");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (only_folders), action->is_dir);
	g_signal_connect (G_OBJECT (only_folders), "toggled", G_CALLBACK (field_changed_cb), NULL);

	only_files = glade_xml_get_widget (gui, "OnlyFilesButton");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (only_files), action->is_file);
	g_signal_connect (G_OBJECT (only_files), "toggled", G_CALLBACK (field_changed_cb), NULL);

	both = glade_xml_get_widget (gui, "BothButton");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (only_folders), action->is_file && action->is_dir);
	g_signal_connect (G_OBJECT (both), "toggled", G_CALLBACK (field_changed_cb), NULL);

	accept_multiple = glade_xml_get_widget (gui, "AcceptMultipleButton");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (accept_multiple), action->accept_multiple_files);
	g_signal_connect (G_OBJECT (accept_multiple), "toggled", G_CALLBACK (field_changed_cb), NULL);

	g_signal_connect (G_OBJECT (glade_xml_get_widget (gui, "LegendButton")), "clicked", G_CALLBACK (legend_button_clicked_cb), NULL);

	/* run the dialog */
	gtk_dialog_set_response_sensitive (GTK_DIALOG (editor), GTK_RESPONSE_OK, FALSE);
	switch (gtk_dialog_run (GTK_DIALOG (editor))) {
	case GTK_RESPONSE_OK :
		config = nautilus_actions_config_gconf_get ();
		label = gtk_entry_get_text (GTK_ENTRY (menu_label));
		if (is_new && nautilus_actions_config_get_action (NAUTILUS_ACTIONS_CONFIG (config), label)) {
			gchar *str;

			str = g_strdup_printf (_("There is already an action named %s. Please choose another name"), label);
			nautilus_actions_display_error (str);
			g_free (str);
			ret = FALSE;
		} else {
			nautilus_actions_config_action_set_label (action, label);
			nautilus_actions_config_action_set_tooltip (action, gtk_entry_get_text (GTK_ENTRY (menu_tooltip)));
			nautilus_actions_config_action_set_path (action, gtk_entry_get_text (GTK_ENTRY (command_path)));
			nautilus_actions_config_action_set_parameters (action, gtk_entry_get_text (GTK_ENTRY (command_params)));

			if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (only_files))) {
				nautilus_actions_config_action_set_is_file (action, TRUE);
				nautilus_actions_config_action_set_is_dir (action, FALSE);
			} else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (only_folders))) {
				nautilus_actions_config_action_set_is_file (action, FALSE);
				nautilus_actions_config_action_set_is_dir (action, TRUE);
			} else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (both))) {
				nautilus_actions_config_action_set_is_file (action, TRUE);
				nautilus_actions_config_action_set_is_dir (action, TRUE);
			}

			nautilus_actions_config_action_set_accept_multiple (
				action, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (accept_multiple)));

			if (is_new)
				ret = nautilus_actions_config_add_action (NAUTILUS_ACTIONS_CONFIG (config), action);
			else
				ret = nautilus_actions_config_update_action (NAUTILUS_ACTIONS_CONFIG (config), action);
		}
		break;
	case GTK_RESPONSE_CANCEL :
		ret = FALSE;
		break;
	}

	/* free memory */
	gtk_widget_destroy (editor);
	editor = NULL;
	g_object_unref (gui);

	return ret;
}

gboolean
nact_editor_new_action (void)
{
	gboolean val;
	NautilusActionsConfigAction *action = nautilus_actions_config_action_new ();

	val = open_editor (action, TRUE);
	nautilus_actions_config_action_free (action);

	return val;
}

gboolean
nact_editor_edit_action (NautilusActionsConfigAction *action)
{
	return open_editor (action, FALSE);
}
