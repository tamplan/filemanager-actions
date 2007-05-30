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

#include <string.h>
#include <glib/gi18n.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkentry.h>
#include <gtk/gtktogglebutton.h>
#include <glade/glade-xml.h>
#include "nact-editor.h"
#include "nact-profile-editor.h"
#include "nact-utils.h"
#include "nact-prefs.h"
#include "nact.h"

enum {
	ICON_STOCK_COLUMN = 0,
	ICON_LABEL_COLUMN,
	ICON_N_COLUMN
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
field_changed_cb (GObject *object, gpointer user_data)
{
	GtkWidget* editor = nact_get_glade_widget_from ("EditActionDialog", GLADE_EDIT_DIALOG_WIDGET);
	GtkWidget* menu_label = nact_get_glade_widget_from ("MenuLabelEntry", GLADE_EDIT_DIALOG_WIDGET);
	const gchar *label = gtk_entry_get_text (GTK_ENTRY (menu_label));

	if (label && strlen (label) > 0)
		gtk_dialog_set_response_sensitive (GTK_DIALOG (editor), GTK_RESPONSE_OK, TRUE);
	else
		gtk_dialog_set_response_sensitive (GTK_DIALOG (editor), GTK_RESPONSE_OK, FALSE);
}

void
icon_browse_button_clicked_cb (GtkButton *button, gpointer user_data)
{
	gchar* last_dir;
	gchar* filename;
	GtkWidget* filechooser = nact_get_glade_widget_from ("FileChooserDialog", GLADE_FILECHOOSER_DIALOG_WIDGET);
	GtkWidget* combo = nact_get_glade_widget_from ("MenuIconComboBoxEntry", GLADE_EDIT_DIALOG_WIDGET);
	gboolean set_current_location = FALSE;

	filename = (gchar*)gtk_entry_get_text (GTK_ENTRY (GTK_BIN (combo)->child));
	if (filename != NULL && strlen (filename) > 0)
	{
		set_current_location = gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (filechooser), filename);
	}
	
	if (!set_current_location)
	{
		last_dir = nact_prefs_get_icon_last_browsed_dir ();
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (filechooser), last_dir);
		g_free (last_dir);
	}

	switch (gtk_dialog_run (GTK_DIALOG (filechooser))) 
	{
		case GTK_RESPONSE_OK :
			last_dir = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (filechooser));
			nact_prefs_set_icon_last_browsed_dir (last_dir);
			g_free (last_dir);

			filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (filechooser));
			gtk_entry_set_text (GTK_ENTRY (GTK_BIN (combo)->child), filename);
			g_free (filename);
		case GTK_RESPONSE_CANCEL:
		case GTK_RESPONSE_DELETE_EVENT:
			gtk_widget_hide (filechooser);
	}		
}

static gint sort_stock_ids (gconstpointer a, gconstpointer b)
{
	GtkStockItem stock_item_a;
	GtkStockItem stock_item_b;
	gchar* label_a;
	gchar* label_b;
	gboolean is_a, is_b;
	int retv = 0;

	is_a = gtk_stock_lookup ((gchar*)a, &stock_item_a);
	is_b = gtk_stock_lookup ((gchar*)b, &stock_item_b);

	if (is_a && !is_b)
	{
		retv = 1;
	}
	else if (!is_a && is_b)
	{
		retv = -1;
	}
	else if (!is_a && !is_b)
	{
		retv = 0;
	}
	else
	{
		label_a = strip_underscore (stock_item_a.label);
		label_b = strip_underscore (stock_item_b.label);
		//retv = g_ascii_strcasecmp (label_a, label_b);
		retv = g_utf8_collate (label_a, label_b);
		g_free (label_a);
		g_free (label_b);
	}

	return retv;
}

static GtkTreeModel* create_stock_icon_model (void)
{
	GSList* stock_list = NULL;
	GSList* iter;
	GtkListStore* model;
	GtkTreeIter row;
	//GtkWidget* window = nact_get_glade_widget_from ("EditActionDialog", GLADE_EDIT_DIALOG_WIDGET);
	GtkStockItem stock_item;
	gchar* label;

	model = gtk_list_store_new (ICON_N_COLUMN, G_TYPE_STRING, G_TYPE_STRING);
	
	gtk_list_store_append (model, &row);
	
	/* i18n notes: when no icon is selected in the drop-down list */
	gtk_list_store_set (model, &row, ICON_STOCK_COLUMN, "", ICON_LABEL_COLUMN, _("None"), -1);
	stock_list = gtk_stock_list_ids ();
	GtkIconTheme* icon_theme = gtk_icon_theme_get_default ();
	stock_list = g_slist_sort (stock_list, (GCompareFunc)sort_stock_ids);

	for (iter = stock_list; iter; iter = iter->next)
	{
		GtkIconInfo *icon_info = gtk_icon_theme_lookup_icon (icon_theme, (gchar*)iter->data, GTK_ICON_SIZE_MENU, GTK_ICON_LOOKUP_FORCE_SVG);
		if (icon_info)
		{
			if (gtk_stock_lookup ((gchar*)iter->data, &stock_item))
			{
				gtk_list_store_append (model, &row);
				label = strip_underscore (stock_item.label);
				gtk_list_store_set (model, &row, ICON_STOCK_COLUMN, (gchar*)iter->data, ICON_LABEL_COLUMN, label, -1);
				g_free (label);
			}
			gtk_icon_info_free (icon_info);
		}
	}

	g_slist_foreach (stock_list, (GFunc)g_free, NULL);
	g_slist_free (stock_list);

	return GTK_TREE_MODEL (model);
}

static void preview_icon_changed_cb (GtkEntry* icon_entry, gpointer user_data)
{
	GtkWidget* image = nact_get_glade_widget_from ("IconImage", GLADE_EDIT_DIALOG_WIDGET);
	const gchar* icon_name = gtk_entry_get_text (icon_entry);
	GtkStockItem stock_item;
	GdkPixbuf* icon = NULL;
	gchar* error_msg;

	if (icon_name && strlen (icon_name) > 0)
	{
		if (gtk_stock_lookup (icon_name, &stock_item))
		{
			gtk_image_set_from_stock (GTK_IMAGE (image), icon_name, GTK_ICON_SIZE_MENU);

			gtk_widget_show (image);
		}
		else if (g_file_test (icon_name, G_FILE_TEST_EXISTS) && 
					g_file_test (icon_name, G_FILE_TEST_IS_REGULAR))
		{
			gint width;
			gint height;
			GError* error = NULL;
			
			gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &width, &height);
			icon = gdk_pixbuf_new_from_file_at_size (icon_name, width, height, &error);
			if (error)
			{
				icon = NULL;
				
				error_msg = g_strdup_printf ("Can't load icon from file %s !", icon_name);
				nautilus_actions_display_error (error_msg,  error->message);
				g_free (error_msg);
				g_error_free (error);
			}
			gtk_image_set_from_pixbuf (GTK_IMAGE (image), icon);
			
			gtk_widget_show (image);
		}
		else
		{
			gtk_widget_hide (image);
		}
	}
	else
	{
		gtk_widget_hide (image);
	}
}

static void fill_menu_icon_combo_list_of (GtkComboBoxEntry* combo)
{
	GtkCellRenderer *cell_renderer_pix;
	GtkCellRenderer *cell_renderer_text;
	
	gtk_combo_box_set_model (GTK_COMBO_BOX (combo), create_stock_icon_model ());
	if (gtk_combo_box_entry_get_text_column (combo) == -1)
	{
		gtk_combo_box_entry_set_text_column (combo, ICON_STOCK_COLUMN);
	}
	gtk_cell_layout_clear (GTK_CELL_LAYOUT (combo));

	cell_renderer_pix = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo),
		 cell_renderer_pix,
		 FALSE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo), cell_renderer_pix,
		"stock-id", ICON_STOCK_COLUMN);

	cell_renderer_text = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo),
		 cell_renderer_text,
		 TRUE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo), cell_renderer_text,
		"text", ICON_LABEL_COLUMN);

	gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
}

static gboolean
cell_edited (GtkTreeModel		   *model,
             const gchar         *path_string,
             const gchar         *new_text,
				 gint 					column)
{
	GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
	GtkTreeIter iter;
	gchar* old_text;
	gboolean toggle_state;

	gtk_tree_model_get_iter (model, &iter, path);

	gtk_tree_model_get (model, &iter, SCHEMES_CHECKBOX_COLUMN, &toggle_state, 
												 column, &old_text, -1);
	g_free (old_text);

	gtk_list_store_set (GTK_LIST_STORE (model), &iter, column,
							  g_strdup (new_text), -1);

	gtk_tree_path_free (path);

	return toggle_state;
}

static void
profile_name_edited_cb (GtkCellRendererText *cell,
             const gchar         *path_string,
             const gchar         *new_profile_desc_name,
             gpointer             data)
{
	GtkWidget* profile_list = nact_get_glade_widget_from ("ProfilesList", GLADE_EDIT_DIALOG_WIDGET);
	NautilusActionsConfigAction* action = (NautilusActionsConfigAction*)g_object_get_data (G_OBJECT (profile_list), "action");
	GtkTreeModel* model = GTK_TREE_MODEL (data);
	GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
	GtkTreeIter iter;
	GError* error = NULL;
	gchar* tmp;
	gchar* profile_name;
	NautilusActionsConfigActionProfile* action_profile;

	gtk_tree_model_get_iter (model, &iter, path);

	gtk_tree_model_get (model, &iter, PROFILE_LABEL_COLUMN, &profile_name, -1);
	action_profile = nautilus_actions_config_action_get_profile (action, profile_name);

	nautilus_actions_config_action_profile_set_desc_name (action_profile, new_profile_desc_name);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, PROFILE_DESC_LABEL_COLUMN,
								  g_strdup (new_profile_desc_name), -1);
	
	field_changed_cb (G_OBJECT (cell), NULL);
	g_free (profile_name);
	gtk_tree_path_free (path);
}

void nact_editor_fill_profiles_list (GtkWidget *list, NautilusActionsConfigAction* action)
{
	GSList *profile_names = NULL, *l;
	GtkListStore *model = GTK_LIST_STORE(gtk_tree_view_get_model (GTK_TREE_VIEW (list)));

	gtk_list_store_clear (model);

	profile_names = nautilus_actions_config_action_get_all_profile_names (action);
	profile_names = g_slist_sort (profile_names, (GCompareFunc)g_utf8_collate);
	for (l = profile_names; l != NULL; l = l->next) 
	{
		GtkTreeIter iter;
		gchar* profile_name = (gchar*)l->data;
		NautilusActionsConfigActionProfile* profile = nautilus_actions_config_action_get_profile (action, profile_name);
		gchar* profile_desc_name = profile_name;

		if (profile->desc_name != NULL && strlen (profile->desc_name) > 0)
		{
			profile_desc_name = profile->desc_name;
		}

		gtk_list_store_append (model, &iter);
		gtk_list_store_set (model, &iter, 
				    PROFILE_LABEL_COLUMN, profile_name,
					 PROFILE_DESC_LABEL_COLUMN, profile_desc_name,
				    -1);
	}

	g_slist_free (profile_names);
}

void
add_prof_button_clicked_cb (GtkButton *button, gpointer user_data)
{
	GtkWidget* profile_list = nact_get_glade_widget_from ("ProfilesList", GLADE_EDIT_DIALOG_WIDGET);
	NautilusActionsConfigAction* action = (NautilusActionsConfigAction*)g_object_get_data (G_OBJECT (profile_list), "action");

	printf ("Action label : %s\n", action->label);

	if (nact_profile_editor_new_profile (action))
	{
		nact_editor_fill_profiles_list (nact_get_glade_widget_from ("ProfilesList", GLADE_EDIT_DIALOG_WIDGET), action);
		field_changed_cb (G_OBJECT (profile_list), NULL);
	}
}

void
edit_prof_button_clicked_cb (GtkButton *button, gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeModel* model;
	GtkWidget *nact_profiles_list = nact_get_glade_widget_from ("ProfilesList", GLADE_EDIT_DIALOG_WIDGET);
	NautilusActionsConfigAction* action = (NautilusActionsConfigAction*)g_object_get_data (G_OBJECT (nact_profiles_list), "action");

	printf ("Action label : %s\n", action->label);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (nact_profiles_list));

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gchar *profile_name;
		NautilusActionsConfigActionProfile *action_profile;

		gtk_tree_model_get (model, &iter, PROFILE_LABEL_COLUMN, &profile_name, -1);

		printf ("profile_name : %s\n", profile_name);

		action_profile = nautilus_actions_config_action_profile_dup (nautilus_actions_config_action_get_profile (action, profile_name));
		if (action && action_profile) 
		{
			if (nact_profile_editor_edit_profile (action, profile_name, action_profile))
			{
				nact_editor_fill_profiles_list (nact_profiles_list, action);
				field_changed_cb (G_OBJECT (nact_profiles_list), NULL);
			}
		}

		g_free (profile_name);
	}
}

void
copy_prof_button_clicked_cb (GtkButton *button, gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeModel* model;
	GError* error = NULL;
	gchar* tmp;
	GtkWidget *nact_prof_paste_button;
	GtkWidget *nact_profiles_list = nact_get_glade_widget_from ("ProfilesList", GLADE_EDIT_DIALOG_WIDGET);
	NautilusActionsConfigAction* action = (NautilusActionsConfigAction*)g_object_get_data (G_OBJECT (nact_profiles_list), "action");
	nact_prof_paste_button = nact_get_glade_widget_from ("PasteProfileButton", GLADE_EDIT_DIALOG_WIDGET);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (nact_profiles_list));

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) 
	{
		gchar* profile_name;
		NautilusActionsConfigActionProfile* action_profile;
		NautilusActionsConfigActionProfile* new_action_profile;

		gtk_tree_model_get (model, &iter, PROFILE_LABEL_COLUMN, &profile_name, -1);

		printf ("profile_name : %s\n", profile_name);

		action_profile = nautilus_actions_config_action_profile_dup (nautilus_actions_config_action_get_profile (action, profile_name));
		new_action_profile = nautilus_actions_config_action_profile_dup (action_profile);

		if (action && new_action_profile) 
		{
			// Remove and free any existing data
			nautilus_actions_config_action_profile_free (g_object_steal_data (G_OBJECT (nact_prof_paste_button), "profile"));

			g_object_set_data (G_OBJECT (nact_prof_paste_button), "profile", new_action_profile);
			gtk_widget_set_sensitive (nact_prof_paste_button, TRUE);

		}
		else
		{
			// i18n notes: will be displayed in a dialog
			tmp = g_strdup_printf (_("Can't copy action's profile '%s' !"), profile_name);
			nautilus_actions_display_error (tmp, "");
			g_free (tmp);
		}

		g_free (profile_name);
	}

}

void
paste_prof_button_clicked_cb (GtkButton *button, gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeModel* model;
	GError* error = NULL;
	gchar* tmp;
	gchar* new_profile_name;
	GtkWidget *nact_profiles_list = nact_get_glade_widget_from ("ProfilesList", GLADE_EDIT_DIALOG_WIDGET);
	NautilusActionsConfigAction* action = (NautilusActionsConfigAction*)g_object_get_data (G_OBJECT (nact_profiles_list), "action");
	GtkWidget *nact_prof_paste_button = nact_get_glade_widget_from ("PasteProfileButton", GLADE_EDIT_DIALOG_WIDGET);
	NautilusActionsConfigActionProfile* action_profile = (NautilusActionsConfigActionProfile*)g_object_get_data (G_OBJECT (nact_prof_paste_button), "profile");

	printf ("profile_name : %s\n", action_profile->desc_name);

	// i18n notes: this is the default name of a copied profile 
	gchar* new_profile_desc_name = g_strdup_printf (_("%s Copy"), action_profile->desc_name);

	// Get a new uniq profile key name but not a new desc name because we already have it.
	nautilus_actions_config_action_get_new_default_profile_name (action, &new_profile_name, NULL);

	NautilusActionsConfigActionProfile* new_action_profile = nautilus_actions_config_action_profile_dup (action_profile);
	nautilus_actions_config_action_profile_set_desc_name (new_action_profile, new_profile_desc_name);

	if (action && new_action_profile) 
	{
		if (nautilus_actions_config_action_add_profile (action, new_profile_name, new_action_profile, &error))
		{
			nact_editor_fill_profiles_list (nact_profiles_list, action);
			field_changed_cb (G_OBJECT (nact_profiles_list), NULL);
		}
		else
		{
			// i18n notes: will be displayed in a dialog
			tmp = g_strdup_printf (_("Can't paste action's profile '%s' !"), new_profile_desc_name);
			nautilus_actions_display_error (tmp, error->message);
			g_error_free (error);
			g_free (tmp);
		}
	}
	else
	{
		// i18n notes: will be displayed in a dialog
		tmp = g_strdup_printf (_("Can't paste action's profile '%s' !"), new_profile_desc_name);
		nautilus_actions_display_error (tmp, "");
		g_free (tmp);
	}

}

void
delete_prof_button_clicked_cb (GtkButton *button, gpointer user_data)
{
	GtkTreeSelection *selection;
	gchar* tmp;
	GtkTreeIter iter;
	GtkTreeModel* model;
	GtkWidget *nact_profiles_list = nact_get_glade_widget_from ("ProfilesList", GLADE_EDIT_DIALOG_WIDGET);
	NautilusActionsConfigAction* action = (NautilusActionsConfigAction*)g_object_get_data (G_OBJECT (nact_profiles_list), "action");

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (nact_profiles_list));

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) 
	{
		gchar *profile_name;

		gtk_tree_model_get (model, &iter, PROFILE_LABEL_COLUMN, &profile_name, -1);

		printf ("profile_name : %s\n", profile_name);

		if (nautilus_actions_config_action_remove_profile (action, profile_name))
		{
			nact_editor_fill_profiles_list (nact_profiles_list, action);
			field_changed_cb (G_OBJECT (nact_profiles_list), NULL);
		}
		else
		{
			// i18n notes: will be displayed in a dialog
			tmp = g_strdup_printf (_("Can't delete action's profile '%s' !"), profile_name);
			nautilus_actions_display_error (tmp, "");
			g_free (tmp);
		}

		g_free (profile_name);
	}
}

static void
profile_list_selection_changed_cb (GtkTreeSelection *selection, gpointer user_data)
{
	GtkWidget *nact_prof_edit_button;
	GtkWidget *nact_prof_delete_button;
	GtkWidget *nact_prof_copy_button;

	nact_prof_edit_button = nact_get_glade_widget_from ("EditProfileButton", GLADE_EDIT_DIALOG_WIDGET);
	nact_prof_delete_button = nact_get_glade_widget_from ("DeleteProfileButton", GLADE_EDIT_DIALOG_WIDGET);
	nact_prof_copy_button = nact_get_glade_widget_from ("CopyProfileButton", GLADE_EDIT_DIALOG_WIDGET);

	if (gtk_tree_selection_count_selected_rows (selection) > 0) {
		gtk_widget_set_sensitive (nact_prof_edit_button, TRUE);
		gtk_widget_set_sensitive (nact_prof_delete_button, TRUE);
		gtk_widget_set_sensitive (nact_prof_copy_button, TRUE);
	} else {
		gtk_widget_set_sensitive (nact_prof_edit_button, FALSE);
		gtk_widget_set_sensitive (nact_prof_delete_button, FALSE);
		gtk_widget_set_sensitive (nact_prof_copy_button, FALSE);
	}
}

static void nact_editor_setup_profiles_list (GtkWidget *list, NautilusActionsConfigAction* action)
{
	GtkListStore *model;
	GtkTreeViewColumn *column;
	GtkCellRenderer* text_cell;

	/* create the model */
	model = gtk_list_store_new (N_PROF_COLUMN, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (list), GTK_TREE_MODEL (model));
	nact_editor_fill_profiles_list (list, action);
	g_object_unref (model);

	/* create columns on the tree view */
	text_cell = gtk_cell_renderer_text_new ();

	g_object_set (G_OBJECT (text_cell), "editable", TRUE, NULL);

	g_signal_connect (G_OBJECT (text_cell), "edited",
							G_CALLBACK (profile_name_edited_cb), 
							gtk_tree_view_get_model (GTK_TREE_VIEW (list)));

	column = gtk_tree_view_column_new_with_attributes (_("Profile Name"),
							   text_cell,
							   "text", PROFILE_DESC_LABEL_COLUMN, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

	/* set up selection */
	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (list))), "changed",
			  G_CALLBACK (profile_list_selection_changed_cb), NULL);

}

static gboolean
open_editor (NautilusActionsConfigAction *action, gboolean is_new)
{
	static gboolean init = FALSE;
	GtkWidget* editor;
	GladeXML *gui;
	gboolean ret = FALSE;
	gchar *label;
	NautilusActionsConfigGconfWriter *config;
	GList* aligned_widgets = NULL;
	GList* iter;
	GSList* list;
	GtkSizeGroup* label_size_group;
	GtkSizeGroup* button_size_group;
	GtkWidget *menu_icon, *scheme_listview;
	GtkWidget *menu_label, *menu_tooltip, *menu_profiles_list;
	GtkWidget *command_path, *command_params, *test_patterns, *match_case, *test_mimetypes;
	GtkWidget *only_files, *only_folders, *both, *accept_multiple;
	gint width, height, x, y;
	GtkTreeModel* scheme_model;

	if (!init)
	{
		/* load the GUI */
		gui = nact_get_glade_xml_object (GLADE_EDIT_DIALOG_WIDGET);
		if (!gui) {
			g_error (_("Could not load interface for Nautilus Actions Config Tool"));
			return FALSE;
		}

		glade_xml_signal_autoconnect(gui);

		menu_icon = nact_get_glade_widget_from ("MenuIconComboBoxEntry", GLADE_EDIT_DIALOG_WIDGET);

		g_assert (menu_icon != NULL);

		g_signal_connect (G_OBJECT (GTK_BIN (menu_icon)->child), "changed",
							   G_CALLBACK (preview_icon_changed_cb), NULL);
		
		fill_menu_icon_combo_list_of (GTK_COMBO_BOX_ENTRY (menu_icon));

		gtk_tooltips_set_tip (gtk_tooltips_new (), GTK_WIDGET (GTK_BIN (menu_icon)->child),
									 _("Icon of the menu item in the Nautilus popup menu"), "");
	
		
		aligned_widgets = nact_get_glade_widget_prefix_from ("LabelAlign", GLADE_EDIT_DIALOG_WIDGET);
		label_size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
		for (iter = aligned_widgets; iter; iter = iter->next)
		{
			gtk_size_group_add_widget (label_size_group, GTK_WIDGET (iter->data));
		}

		menu_profiles_list = nact_get_glade_widget_from ("ProfilesList", GLADE_EDIT_DIALOG_WIDGET);
		nact_editor_setup_profiles_list (menu_profiles_list, action);

		/* free memory */
		g_object_unref (gui);
		init = TRUE;
	}

	editor = nact_get_glade_widget_from ("EditActionDialog", GLADE_EDIT_DIALOG_WIDGET);

	if (is_new)
	{
		gtk_window_set_title (GTK_WINDOW (editor), _("Add a New Action"));
	}
	else
	{
		gchar* title = g_strdup_printf (_("Edit Action \"%s\""), action->label);
		gtk_window_set_title (GTK_WINDOW (editor), title);
		g_free (title);
	}

	/* Get the default dialog size */
	gtk_window_get_default_size (GTK_WINDOW (editor), &width, &height);
	/* Override with preferred one, if any */
	nact_prefs_get_edit_dialog_size (&width, &height);
	
	gtk_window_resize (GTK_WINDOW (editor), width, height);

	if (nact_prefs_get_edit_dialog_position (&x, &y))
	{
		gtk_window_move (GTK_WINDOW (editor), x, y);
	}
	
	menu_label = nact_get_glade_widget_from ("MenuLabelEntry", GLADE_EDIT_DIALOG_WIDGET);
	gtk_entry_set_text (GTK_ENTRY (menu_label), action->label);

	menu_tooltip = nact_get_glade_widget_from ("MenuTooltipEntry", GLADE_EDIT_DIALOG_WIDGET);
	gtk_entry_set_text (GTK_ENTRY (menu_tooltip), action->tooltip);

	menu_icon = nact_get_glade_widget_from ("MenuIconComboBoxEntry", GLADE_EDIT_DIALOG_WIDGET);
	gtk_entry_set_text (GTK_ENTRY (GTK_BIN (menu_icon)->child), action->icon);
	
	menu_profiles_list = nact_get_glade_widget_from ("ProfilesList", GLADE_EDIT_DIALOG_WIDGET);
	nact_editor_fill_profiles_list (menu_profiles_list, action);

	/* remove any old reference and reference the new action in the list */
	g_object_steal_data (G_OBJECT (menu_profiles_list), "action");
	g_object_set_data (G_OBJECT (menu_profiles_list), "action", action);

	/* run the dialog */
	gtk_dialog_set_response_sensitive (GTK_DIALOG (editor), GTK_RESPONSE_OK, FALSE);
	switch (gtk_dialog_run (GTK_DIALOG (editor))) {
	case GTK_RESPONSE_OK :
		config = nautilus_actions_config_gconf_writer_get ();

		label = (gchar*)gtk_entry_get_text (GTK_ENTRY (menu_label));
		nautilus_actions_config_action_set_label (action, label);
		nautilus_actions_config_action_set_tooltip (action, gtk_entry_get_text (GTK_ENTRY (menu_tooltip)));
		nautilus_actions_config_action_set_icon (action, gtk_entry_get_text (GTK_ENTRY (GTK_BIN (menu_icon)->child)));
		if (is_new)
		{
			// TODO: If necessary deal with the GError returned
			ret = nautilus_actions_config_add_action (NAUTILUS_ACTIONS_CONFIG (config), action, NULL);
		}
		else
		{
			ret = nautilus_actions_config_update_action (NAUTILUS_ACTIONS_CONFIG (config), action);
		}
		g_object_unref (config);
		break;
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL :
		ret = FALSE;
		break;
	}
	
	/* FIXME: update save preference code 

	// Save preferences
	list = NULL;
	gtk_tree_model_foreach (scheme_model, (GtkTreeModelForeachFunc)get_all_schemes_list, &list);
	nact_prefs_set_schemes_list (list);
	g_slist_foreach (list, (GFunc) g_free, NULL);
	g_slist_free (list);

	nact_prefs_set_edit_dialog_size (GTK_WINDOW (editor));
	nact_prefs_set_edit_dialog_position (GTK_WINDOW (editor));

	*/
	gtk_widget_hide (editor);

	return ret;
}

gboolean
nact_editor_new_action (void)
{
	gboolean val;
	NautilusActionsConfigAction *action = nautilus_actions_config_action_new_default ();

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
