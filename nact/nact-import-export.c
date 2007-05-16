/* Nautilus Actions configuration tool
 * Copyright (C) 2005 The GNOME Foundation
 *
 * Authors:
 *  Frederic Ruaudel (grumz@grumz.net)
 *	 Rodrigo Moya (rodrigo@gnome-db.org)
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
#include <string.h>
#include <glib/gi18n.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <glade/glade-xml.h>
#include <libnautilus-actions/nautilus-actions-config.h>
#include <libnautilus-actions/nautilus-actions-config-schema-reader.h>
#include <libnautilus-actions/nautilus-actions-config-schema-writer.h>
#include <libnautilus-actions/nautilus-actions-config-gconf-writer.h>
#include "nact-utils.h"
#include "nact-import-export.h"
#include "nact-prefs.h"
#include "nact.h"

void mode_toggled_cb (GtkWidget* widget, gpointer user_data)
{
	GtkWidget* import_radio = nact_get_glade_widget_from ("ImportRadioButton", GLADE_IM_EX_PORT_DIALOG_WIDGET);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (import_radio)))
	{
		gtk_widget_set_sensitive (nact_get_glade_widget_from ("ImportVBox", GLADE_IM_EX_PORT_DIALOG_WIDGET), TRUE);
		gtk_widget_set_sensitive (nact_get_glade_widget_from ("ExportVBox", GLADE_IM_EX_PORT_DIALOG_WIDGET), FALSE);
	}
	else
	{
		gtk_widget_set_sensitive (nact_get_glade_widget_from ("ExportVBox", GLADE_IM_EX_PORT_DIALOG_WIDGET), TRUE);
		gtk_widget_set_sensitive (nact_get_glade_widget_from ("ImportVBox", GLADE_IM_EX_PORT_DIALOG_WIDGET), FALSE);
	}
}

void import_all_config_toggled_cb (GtkWidget* check_button, gpointer data) 
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_button)))
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (nact_get_glade_widget_from ("XMLRadioButton", 
																			GLADE_IM_EX_PORT_DIALOG_WIDGET)), TRUE);
		gtk_widget_set_sensitive (nact_get_glade_widget_from ("FileHBox", GLADE_IM_EX_PORT_DIALOG_WIDGET), FALSE);
		gtk_widget_set_sensitive (nact_get_glade_widget_from ("TypeConfigVBox", GLADE_IM_EX_PORT_DIALOG_WIDGET), FALSE);
	}
	else
	{
		gtk_widget_set_sensitive (nact_get_glade_widget_from ("FileHBox", GLADE_IM_EX_PORT_DIALOG_WIDGET), TRUE);
		gtk_widget_set_sensitive (nact_get_glade_widget_from ("TypeConfigVBox", GLADE_IM_EX_PORT_DIALOG_WIDGET), TRUE);
	}
}

void import_browse_button_clicked_cb (GtkWidget* widget, gpointer data)
{
	gchar* last_dir;
	gchar* filename;
	GtkWidget* filechooser = nact_get_glade_widget_from ("FileChooserDialog", GLADE_FILECHOOSER_DIALOG_WIDGET);
	GtkWidget* entry = nact_get_glade_widget_from ("ImportEntry", GLADE_IM_EX_PORT_DIALOG_WIDGET);
	gboolean set_current_location = FALSE;

	filename = (gchar*)gtk_entry_get_text (GTK_ENTRY (entry));
	if (filename != NULL && strlen (filename) > 0)
	{
		set_current_location = gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (filechooser), filename);
	}
	
	if (!set_current_location)
	{
		last_dir = nact_prefs_get_import_last_browsed_dir ();
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (filechooser), last_dir);
		g_free (last_dir);
	}

	switch (gtk_dialog_run (GTK_DIALOG (filechooser))) 
	{
		case GTK_RESPONSE_OK :
			last_dir = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (filechooser));
			nact_prefs_set_import_last_browsed_dir (last_dir);
			g_free (last_dir);

			filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (filechooser));
			gtk_entry_set_text (GTK_ENTRY (entry), filename);
			g_free (filename);
		case GTK_RESPONSE_CANCEL:
		case GTK_RESPONSE_DELETE_EVENT:
			gtk_widget_hide (filechooser);
	}	
}

void export_browse_button_clicked_cb (GtkWidget* widget, gpointer data) 
{
	gchar* last_dir;
	gchar* foldername;
	GtkWidget* folderchooser = nact_get_glade_widget_from ("FolderChooserDialog", GLADE_FOLDERCHOOSER_DIALOG_WIDGET);
	GtkWidget* entry = nact_get_glade_widget_from ("ExportEntry", GLADE_IM_EX_PORT_DIALOG_WIDGET);
	gboolean set_current_location = FALSE;

	foldername = (gchar*)gtk_entry_get_text (GTK_ENTRY (entry));
	if (foldername != NULL && strlen (foldername) > 0)
	{
		set_current_location = gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (folderchooser), foldername);
	}
	
	if (!set_current_location)
	{
		last_dir = nact_prefs_get_export_last_browsed_dir ();
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (folderchooser), last_dir);
		g_free (last_dir);
	}

	switch (gtk_dialog_run (GTK_DIALOG (folderchooser))) 
	{
		case GTK_RESPONSE_OK :
			last_dir = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (folderchooser));
			nact_prefs_set_export_last_browsed_dir (last_dir);
			g_free (last_dir);

			foldername = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (folderchooser));
			gtk_entry_set_text (GTK_ENTRY (entry), foldername);
			g_free (foldername);
		case GTK_RESPONSE_CANCEL:
		case GTK_RESPONSE_DELETE_EVENT:
			gtk_widget_hide (folderchooser);
	}	
}


static void
list_selection_changed_cb (GtkTreeSelection *selection, gpointer user_data)
{
/*
	GtkWidget *nact_edit_button;
	GtkWidget *nact_delete_button;
	
	nact_edit_button = nact_get_glade_widget ("EditActionButton");
	nact_delete_button = nact_get_glade_widget ("DeleteActionButton");

	if (gtk_tree_selection_count_selected_rows (selection) > 0) {
		gtk_widget_set_sensitive (nact_edit_button, TRUE);
		gtk_widget_set_sensitive (nact_delete_button, TRUE);
	} else {
		gtk_widget_set_sensitive (nact_edit_button, FALSE);
		gtk_widget_set_sensitive (nact_delete_button, FALSE);
	}
*/
}

static void nact_setup_actions_list (GtkWidget *list)
{
	GtkTreeViewColumn *column;
	GtkTreeSelection* selection;
	GtkTreeModel* nact_action_list_model;

	/* Get the model from the main list */
	nact_action_list_model = gtk_tree_view_get_model (GTK_TREE_VIEW (nact_get_glade_widget ("ActionsList")));

	gtk_tree_view_set_model (GTK_TREE_VIEW (list), nact_action_list_model);

	/* create columns on the tree view */
	column = gtk_tree_view_column_new_with_attributes (_("Icon"),
							   gtk_cell_renderer_pixbuf_new (),
							   "pixbuf", MENU_ICON_COLUMN, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

	column = gtk_tree_view_column_new_with_attributes (_("Label"),
							   gtk_cell_renderer_text_new (),
							   "text", MENU_LABEL_COLUMN, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
	
	/* set up selection */
	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (list))), "changed",
			  G_CALLBACK (list_selection_changed_cb), NULL);

}

gboolean nact_import_actions (void)
{
	gboolean retv = FALSE;
/* FIXME: Remove backward compat with XML config format file

	GtkWidget* check_button;
	GSList* iter;
	NautilusActionsConfigGconfWriter *config;
	NautilusActionsConfigSchemaReader *schema_reader;
	NautilusActionsConfigXml* xml_reader;
	NautilusActionsConfig* generic_reader = NULL;
	gchar* error_message;
	GError* error = NULL;
	const gchar* file_path = gtk_entry_get_text (GTK_ENTRY (nact_get_glade_widget_from ("ImportEntry",
																			GLADE_IM_EX_PORT_DIALOG_WIDGET)));

	config = nautilus_actions_config_gconf_writer_get ();
	schema_reader = nautilus_actions_config_schema_reader_get ();
	nautilus_actions_config_clear (schema_reader);
	xml_reader = nautilus_actions_config_xml_get ();
	nautilus_actions_config_clear (xml_reader);

	check_button = nact_get_glade_widget_from ("ImportAllCheckButton", GLADE_IM_EX_PORT_DIALOG_WIDGET);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_button)))
	{
		nautilus_actions_config_xml_load_list (xml_reader);
		generic_reader = NAUTILUS_ACTIONS_CONFIG (xml_reader);
	}
	else if (file_path != NULL && strlen (file_path) > 0)
	{
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (nact_get_glade_widget_from ("XMLRadioButton", 
																			GLADE_IM_EX_PORT_DIALOG_WIDGET))))
		{
			if (nautilus_actions_config_xml_parse_file (xml_reader, file_path, &error))
			{
				generic_reader = NAUTILUS_ACTIONS_CONFIG (xml_reader);
			}
			else
			{
				error_message = g_strdup_printf (_("Can't parse file '%s' as old XML config file !"), file_path);
				nautilus_actions_display_error (error_message, error->message);
				g_error_free (error);
				g_free (error_message);
			}
		}
		else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (nact_get_glade_widget_from ("GConfRadioButton", 
																			GLADE_IM_EX_PORT_DIALOG_WIDGET))))
		{
			if (nautilus_actions_config_schema_reader_parse_file (schema_reader, file_path, &error))
			{
				generic_reader = NAUTILUS_ACTIONS_CONFIG (schema_reader);
			}
			else
			{
				error_message = g_strdup_printf (_("Can't parse file '%s' as GConf schema description file !"), file_path);
				nautilus_actions_display_error (error_message, error->message);
				g_error_free (error);
				g_free (error_message);
			}
		}
		else // Automatic detection asked 
		{
			//--> we are ignoring the first error because if it fails here it will not in the next 
			// or if both fails, we kept the current GConf config format as the most important
			if (nautilus_actions_config_xml_parse_file (xml_reader, file_path, NULL))
			{
				generic_reader = NAUTILUS_ACTIONS_CONFIG (xml_reader);
			}
			else if (nautilus_actions_config_schema_reader_parse_file (schema_reader, file_path, &error))
			{
				generic_reader = NAUTILUS_ACTIONS_CONFIG (schema_reader);
			}
			else
			{
				error_message = g_strdup_printf (_("Can't parse file '%s' !"), file_path);
				nautilus_actions_display_error (error_message, error->message);
				g_error_free (error);
				g_free (error_message);
			}
		}
	}

	if (generic_reader != NULL)
	{
		GSList* actions = nautilus_actions_config_get_actions (generic_reader);

		for (iter = actions; iter; iter = iter->next)
		{
			NautilusActionsConfigAction* action = (NautilusActionsConfigAction*)(iter->data);
			if (nautilus_actions_config_add_action (NAUTILUS_ACTIONS_CONFIG (config), action, &error))
			{
				retv = TRUE;
			}
			else
			{
				// i18n notes: %s is the label of the action (eg, 'Mount ISO')
				error_message = g_strdup_printf (_("Action '%s' importation failed !"), action->label);
				nautilus_actions_display_error (error_message, error->message);
				g_error_free (error);
				g_free (error_message);
			}
		}
	}
*/
	return retv;
}

gboolean nact_export_actions (void)
{
	gboolean retv = FALSE;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GList* list_iter;
	GtkWidget *nact_actions_list;
	GtkTreeModel* model;
	GList* selection_list = NULL;
	NautilusActionsConfigGconfWriter *config;
	NautilusActionsConfigSchemaWriter *schema_writer;
	const gchar* save_path = gtk_entry_get_text (GTK_ENTRY (nact_get_glade_widget_from ("ExportEntry",
																			GLADE_IM_EX_PORT_DIALOG_WIDGET)));

	config = nautilus_actions_config_gconf_writer_get ();
	schema_writer = nautilus_actions_config_schema_writer_get ();
	g_object_set (G_OBJECT (schema_writer), "save-path", save_path, NULL);
	nact_actions_list = nact_get_glade_widget_from ("ExportTreeView", GLADE_IM_EX_PORT_DIALOG_WIDGET);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (nact_actions_list));

	selection_list = gtk_tree_selection_get_selected_rows (selection, &model);
	if (selection_list)
	{
		for (list_iter = selection_list; list_iter; list_iter = list_iter->next)
		{
			gchar *uuid;
			NautilusActionsConfigAction *action;

			gtk_tree_model_get_iter (model, &iter, (GtkTreePath*)(list_iter->data));
			gtk_tree_model_get (model, &iter, UUID_COLUMN, &uuid, -1);

			action = nautilus_actions_config_get_action (NAUTILUS_ACTIONS_CONFIG (config), uuid);
			// TODO: Better error handling: deal with the GError param
			if (nautilus_actions_config_add_action (NAUTILUS_ACTIONS_CONFIG (schema_writer), action, NULL))
			{
				nautilus_actions_config_schema_writer_get_saved_filename (schema_writer, action->uuid);
			}

			g_free (uuid);
		}

		g_list_foreach (selection_list, (GFunc)gtk_tree_path_free, NULL);
		g_list_free (selection_list);
		retv = TRUE;
	}
	
	if (retv)
	{
		gchar* command = g_strdup_printf ("nautilus %s", save_path);
		g_spawn_command_line_async (command, NULL);
	}

	return retv;
}

gboolean nact_import_export_actions (void)
{
	static gboolean init = FALSE;
	gboolean retv = FALSE;
	GtkWidget* nact_action_list_tree;
	GtkWidget* import_export_dialog;
	GtkWidget* import_radio;
	GtkTreeSelection *selection;
	GList* aligned_widgets = NULL;
	GList* iter;
	GtkSizeGroup* label_size_group;
	GtkSizeGroup* button_size_group;
	gint width, height, x, y;
	gchar* last_dir;

	if (!init)
	{
		/* load the GUI */
		GladeXML* gui = nact_get_glade_xml_object (GLADE_IM_EX_PORT_DIALOG_WIDGET);
		if (!gui) {
			g_error (_("Could not load interface for Nautilus Actions Config Tool"));
			return FALSE;
		}
		
		glade_xml_signal_autoconnect (gui);

		nact_action_list_tree = nact_get_glade_widget_from ("ExportTreeView", GLADE_IM_EX_PORT_DIALOG_WIDGET);
		nact_setup_actions_list (nact_action_list_tree);
		
		aligned_widgets = nact_get_glade_widget_prefix_from ("IELabelAlign", GLADE_IM_EX_PORT_DIALOG_WIDGET);
		label_size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
		for (iter = aligned_widgets; iter; iter = iter->next)
		{
			gtk_size_group_add_widget (label_size_group, GTK_WIDGET (iter->data));
		}
		button_size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
		gtk_size_group_add_widget (button_size_group, 
											nact_get_glade_widget_from ("ImportBrowseButton", 
																		GLADE_IM_EX_PORT_DIALOG_WIDGET));
		gtk_size_group_add_widget (button_size_group, 
											nact_get_glade_widget_from ("ExportBrowseButton", 
																		GLADE_IM_EX_PORT_DIALOG_WIDGET));
		/* free memory */
		g_object_unref (gui);
		init = TRUE;
	}
	nact_action_list_tree = nact_get_glade_widget_from ("ExportTreeView", GLADE_IM_EX_PORT_DIALOG_WIDGET);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (nact_action_list_tree));
	gtk_tree_selection_unselect_all (selection);

	import_export_dialog = nact_get_glade_widget_from (GLADE_IM_EX_PORT_DIALOG_WIDGET, GLADE_IM_EX_PORT_DIALOG_WIDGET);

	import_radio = nact_get_glade_widget_from ("ImportRadioButton", GLADE_IM_EX_PORT_DIALOG_WIDGET);

	/* Get the default dialog size */
	gtk_window_get_default_size (GTK_WINDOW (import_export_dialog), &width, &height);
	
	/* Override with preferred one, if any */
	nact_prefs_get_im_ex_dialog_size (&width, &height);
	
	gtk_window_resize (GTK_WINDOW (import_export_dialog), width, height);

	if (nact_prefs_get_im_ex_dialog_position (&x, &y))
	{
		gtk_window_move (GTK_WINDOW (import_export_dialog), x, y);
	}

	last_dir = nact_prefs_get_export_last_browsed_dir ();
	gtk_entry_set_text (GTK_ENTRY (nact_get_glade_widget_from ("ExportEntry", GLADE_IM_EX_PORT_DIALOG_WIDGET)), last_dir);
	gtk_entry_select_region (GTK_ENTRY (nact_get_glade_widget_from ("ExportEntry", GLADE_IM_EX_PORT_DIALOG_WIDGET)), 0, -1);
	
	/* run the dialog */
	switch (gtk_dialog_run (GTK_DIALOG (import_export_dialog))) 
	{
		case GTK_RESPONSE_OK :
			if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (import_radio)))
			{
				/* Import mode */
				retv = nact_import_actions ();
			}
			else
			{
				/* Export Mode */
				retv = nact_export_actions ();
			}
			break;
		case GTK_RESPONSE_DELETE_EVENT:
		case GTK_RESPONSE_CANCEL :
			retv = FALSE;
			break;
	}

	nact_prefs_set_im_ex_dialog_size (GTK_WINDOW (import_export_dialog));
	nact_prefs_set_im_ex_dialog_position (GTK_WINDOW (import_export_dialog));

	gtk_widget_hide (import_export_dialog);

	return retv;
}

// vim:ts=3:sw=3:tw=1024:cin
