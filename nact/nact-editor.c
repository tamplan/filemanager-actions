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
#include "nact-utils.h"

enum {
	STOCK_COLUMN = 0,
	LABEL_COLUMN,
	N_COLUMN
};

static gchar *
strip_underscore (const gchar *text)
{
	/* Code from gtk-demo */
	gchar *p, *q;
	gchar *result;

	result = g_strdup (text);
	p = q = result;
	while (*p) 
	{
		if (*p != '_')
		{
			*q = *p;
			q++;
		}
		p++;
	}
	*q = '\0';

	return result;
}

void
field_changed_cb (GObject *editable, gpointer user_data)
{
	GtkWidget* editor = nact_get_glade_widget_from ("EditActionDialog", GLADE_EDIT_DIALOG_WIDGET);
	GtkWidget* menu_label = nact_get_glade_widget_from ("MenuLabelEntry", GLADE_EDIT_DIALOG_WIDGET);
	gchar *label = gtk_entry_get_text (GTK_ENTRY (menu_label));

	if (label && strlen (label) > 0)
		gtk_dialog_set_response_sensitive (GTK_DIALOG (editor), GTK_RESPONSE_OK, TRUE);
	else
		gtk_dialog_set_response_sensitive (GTK_DIALOG (editor), GTK_RESPONSE_OK, FALSE);
}

void
legend_button_clicked_cb (GtkButton *button, gpointer user_data)
{
	GtkWidget* editor = nact_get_glade_widget_from ("EditActionDialog", GLADE_EDIT_DIALOG_WIDGET);
	GladeXML *gui;
	GtkWidget *legend_dialog;

	gui = nact_get_glade_xml_object ("LegendDialog");
	if (!gui)
		return;

	legend_dialog = nact_get_glade_widget_from ("LegendDialog", GLADE_LEGEND_DIALOG_WIDGET);
	gtk_window_set_transient_for (GTK_WINDOW (legend_dialog), GTK_WINDOW (editor));

	gtk_dialog_run (GTK_DIALOG (legend_dialog));

	gtk_widget_hide (legend_dialog);
	g_object_unref (gui);
}

static GtkTreeModel* create_stock_icon_model (void)
{
	GSList* stock_list = NULL;
	GSList* iter;
	GtkListStore* model;
	GtkTreeIter row;
	GtkWidget* window = nact_get_glade_widget_from ("EditActionDialog", GLADE_EDIT_DIALOG_WIDGET);
	GdkPixbuf* icon = NULL;
	GtkStockItem stock_item;
	gchar* label;

	model = gtk_list_store_new (N_COLUMN, G_TYPE_STRING, G_TYPE_STRING);
	
	gtk_list_store_append (model, &row);
	
	gtk_list_store_set (model, &row, STOCK_COLUMN, "", LABEL_COLUMN, _("None"), -1);
	stock_list = gtk_stock_list_ids ();

	for (iter = stock_list; iter; iter = iter->next)
	{
		if (gtk_stock_lookup ((gchar*)iter->data, &stock_item))
		{
			icon = gtk_widget_render_icon (window, (gchar*)iter->data, GTK_ICON_SIZE_MENU, NULL);
			gtk_list_store_append (model, &row);
			label = strip_underscore (stock_item.label);
			gtk_list_store_set (model, &row, STOCK_COLUMN, (gchar*)iter->data, LABEL_COLUMN, label, -1);
			g_free (label);
		}
	}

	g_slist_foreach (stock_list, (GFunc)g_free, NULL);
	g_slist_free (stock_list);

	return GTK_TREE_MODEL (model);
}

static void fill_menu_icon_combo_list_of (GtkComboBoxEntry* combo)
{
	GtkCellRendererPixbuf *cell_renderer_pix;
	GtkCellRendererText *cell_renderer_text;
	
	gtk_combo_box_set_model (GTK_COMBO_BOX (combo), create_stock_icon_model ());
	gtk_combo_box_entry_set_text_column (combo, STOCK_COLUMN);
	gtk_cell_layout_clear (GTK_CELL_LAYOUT (combo));

	cell_renderer_pix = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo),
		 cell_renderer_pix,
		 FALSE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo), cell_renderer_pix,
		"stock-id", STOCK_COLUMN);

	cell_renderer_text = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo),
		 cell_renderer_text,
		 TRUE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo), cell_renderer_text,
		"text", LABEL_COLUMN);

	gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
}

static gboolean
open_editor (NautilusActionsConfigAction *action, gboolean is_new)
{
	static gboolean init = FALSE;
	GtkWidget* editor;
	GladeXML *gui;
	gboolean ret = FALSE;
	gchar *label;
	NautilusActionsConfigGconf *config;
	GList* aligned_widgets = NULL;
	GList* iter;
	GtkSizeGroup* label_size_group;
	GtkSizeGroup* button_size_group;
	GtkWidget* menu_icon;
	GtkWidget *menu_label, *menu_tooltip;
	GtkWidget *command_path, *command_params;
	GtkWidget *only_files, *only_folders, *both, *accept_multiple;

	if (!init)
	{
		/* load the GUI */
		gui = nact_get_glade_xml_object (GLADE_EDIT_DIALOG_WIDGET);
		if (!gui) {
			nautilus_actions_display_error (_("Could not load interface for Nautilus Actions Config Tool"));
			return FALSE;
		}

		glade_xml_signal_autoconnect(gui);

		menu_icon = nact_get_glade_widget_from ("MenuIconComboBoxEntry", GLADE_EDIT_DIALOG_WIDGET);
		fill_menu_icon_combo_list_of (GTK_COMBO_BOX_ENTRY (menu_icon));
	
		aligned_widgets = nact_get_glade_widget_prefix_from ("LabelAlign", GLADE_EDIT_DIALOG_WIDGET);
		label_size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
		for (iter = aligned_widgets; iter; iter = iter->next)
		{
			gtk_size_group_add_widget (label_size_group, GTK_WIDGET (iter->data));
		}
		button_size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
		gtk_size_group_add_widget (button_size_group, 
											nact_get_glade_widget_from ("IconBrowseButton", 
																		GLADE_EDIT_DIALOG_WIDGET));
		gtk_size_group_add_widget (button_size_group, 
											nact_get_glade_widget_from ("BrowseButton", 
																		GLADE_EDIT_DIALOG_WIDGET));
		gtk_size_group_add_widget (button_size_group, 
											nact_get_glade_widget_from ("LegendButton", 
																		GLADE_EDIT_DIALOG_WIDGET));
		/* free memory */
		g_object_unref (gui);
		init = TRUE;
	}

	editor = nact_get_glade_widget_from ("EditActionDialog", GLADE_EDIT_DIALOG_WIDGET);

	menu_label = nact_get_glade_widget_from ("MenuLabelEntry", GLADE_EDIT_DIALOG_WIDGET);
	gtk_entry_set_text (GTK_ENTRY (menu_label), action->label);

	menu_tooltip = nact_get_glade_widget_from ("MenuTooltipEntry", GLADE_EDIT_DIALOG_WIDGET);
	gtk_entry_set_text (GTK_ENTRY (menu_tooltip), action->tooltip);

	menu_icon = nact_get_glade_widget_from ("MenuIconComboBoxEntry", GLADE_EDIT_DIALOG_WIDGET);
	gtk_entry_set_text (GTK_ENTRY (GTK_BIN (menu_icon)->child), action->icon);
	
	command_path = nact_get_glade_widget_from ("CommandPathEntry", GLADE_EDIT_DIALOG_WIDGET);
	gtk_entry_set_text (GTK_ENTRY (command_path), action->path);

	command_params = nact_get_glade_widget_from ("CommandParamsEntry", GLADE_EDIT_DIALOG_WIDGET);
	gtk_entry_set_text (GTK_ENTRY (command_params), action->parameters);

	only_folders = nact_get_glade_widget_from ("OnlyFoldersButton", GLADE_EDIT_DIALOG_WIDGET);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (only_folders), action->is_dir);

	only_files = nact_get_glade_widget_from ("OnlyFilesButton", GLADE_EDIT_DIALOG_WIDGET);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (only_files), action->is_file);

	both = nact_get_glade_widget_from ("BothButton", GLADE_EDIT_DIALOG_WIDGET);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (both), action->is_file && action->is_dir);

	accept_multiple = nact_get_glade_widget_from ("AcceptMultipleButton", GLADE_EDIT_DIALOG_WIDGET);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (accept_multiple), action->accept_multiple_files);

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
			nautilus_actions_config_action_set_icon (action, gtk_entry_get_text (GTK_ENTRY (menu_icon)));
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
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL :
		ret = FALSE;
		break;
	}

	gtk_widget_hide (editor);

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

// vim:ts=3:sw=3:tw=1024:cin
