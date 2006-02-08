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

#include <config.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkmessagedialog.h>
#include <gtk/gtktreeview.h>
#include <glade/glade-xml.h>
#include <libnautilus-actions/nautilus-actions-config.h>
#include <libnautilus-actions/nautilus-actions-config-gconf-writer.h>
#include "nact-utils.h"
#include "nact.h"
#include "nact-editor.h"
#include "nact-import-export.h"
#include "nact-prefs.h"

static NautilusActionsConfigGconfWriter *config = NULL;

gint actions_list_sort_by_label (gconstpointer a1, gconstpointer a2)
{
	NautilusActionsConfigAction* action1 = (NautilusActionsConfigAction*)a1;
	NautilusActionsConfigAction* action2 = (NautilusActionsConfigAction*)a2;

	return g_utf8_collate (action1->label, action2->label);
}

void nact_fill_actions_list (GtkWidget *list)
{
	GSList *actions, *l;
	GtkListStore *model = GTK_LIST_STORE(gtk_tree_view_get_model (GTK_TREE_VIEW (list)));

	gtk_list_store_clear (model);

	actions = nautilus_actions_config_get_actions (NAUTILUS_ACTIONS_CONFIG (config));
	actions = g_slist_sort (actions, (GCompareFunc)actions_list_sort_by_label);
	for (l = actions; l != NULL; l = l->next) {
		GtkTreeIter iter;
		GtkStockItem item;
		GdkPixbuf* icon = NULL;
		NautilusActionsConfigAction *action = l->data;

		if (action->icon != NULL) {
			if (gtk_stock_lookup (action->icon, &item)) {
				icon = gtk_widget_render_icon (list, action->icon, GTK_ICON_SIZE_MENU, NULL);
			} else if (g_file_test (action->icon, G_FILE_TEST_EXISTS)
				   && g_file_test (action->icon, G_FILE_TEST_IS_REGULAR)) {
				gint width;
				gint height;
				GError* error = NULL;
				
				gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &width, &height);
				icon = gdk_pixbuf_new_from_file_at_size (action->icon, width, height, &error);
				if (error)
					icon = NULL;
			}
		}
		gtk_list_store_append (model, &iter);
		gtk_list_store_set (model, &iter, 
				    MENU_ICON_COLUMN, icon, 
				    MENU_LABEL_COLUMN, action->label,
				    UUID_COLUMN, action->uuid,
				    -1);
	}

	nautilus_actions_config_free_actions_list (actions);
}

void 
nautilus_actions_display_error (const gchar *msg)
{
}

void
add_button_clicked_cb (GtkButton *button, gpointer user_data)
{
	if (nact_editor_new_action ())
		nact_fill_actions_list (nact_get_glade_widget ("ActionsList"));
}

void
edit_button_clicked_cb (GtkButton *button, gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkWidget *nact_actions_list;
	GtkTreeModel* model;

	nact_actions_list = nact_get_glade_widget ("ActionsList");

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (nact_actions_list));

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gchar *uuid;
		NautilusActionsConfigAction *action;

		gtk_tree_model_get (model, &iter, UUID_COLUMN, &uuid, -1);

		action = nautilus_actions_config_get_action (NAUTILUS_ACTIONS_CONFIG (config), uuid);
		if (action) {
			if (nact_editor_edit_action (action))
				nact_fill_actions_list (nact_actions_list);
		}

		g_free (uuid);
	}
}

void
duplicate_button_clicked_cb (GtkButton *button, gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkWidget *nact_actions_list;
	GtkTreeModel* model;

	nact_actions_list = nact_get_glade_widget ("ActionsList");

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (nact_actions_list));

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) 
	{
		gchar *uuid;
		NautilusActionsConfigAction *action;
		NautilusActionsConfigAction* new_action;

		gtk_tree_model_get (model, &iter, UUID_COLUMN, &uuid, -1);

		action = nautilus_actions_config_get_action (NAUTILUS_ACTIONS_CONFIG (config), uuid);
		new_action = nautilus_actions_config_action_dup_new (action);
		if (new_action) 
		{
			if (nautilus_actions_config_add_action (NAUTILUS_ACTIONS_CONFIG (config), new_action))
			{
				nact_fill_actions_list (nact_actions_list);
			}
		}

		g_free (uuid);
	}
}

void
delete_button_clicked_cb (GtkButton *button, gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkWidget *nact_actions_list;
	GtkTreeModel* model;

	nact_actions_list = nact_get_glade_widget ("ActionsList");

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (nact_actions_list));

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gchar *uuid;

		gtk_tree_model_get (model, &iter, UUID_COLUMN, &uuid, -1);
		nautilus_actions_config_remove_action (NAUTILUS_ACTIONS_CONFIG (config), uuid);
		nact_fill_actions_list (nact_actions_list);

		g_free (uuid);
	}
}

void im_export_button_clicked_cb (GtkButton *button, gpointer user_data)
{
	GtkWidget *nact_actions_list;

	if (nact_import_export_actions ())
	{
		nact_actions_list = nact_get_glade_widget ("ActionsList");
		nact_fill_actions_list (nact_actions_list);
	}
}

void
dialog_response_cb (GtkDialog *dialog, gint response_id, gpointer user_data)
{
	switch (response_id) {
	case GTK_RESPONSE_NONE :
	case GTK_RESPONSE_DELETE_EVENT :
	case GTK_RESPONSE_CLOSE :
		nact_prefs_set_main_dialog_size (GTK_WINDOW (dialog));
		nact_prefs_set_main_dialog_position (GTK_WINDOW (dialog));
		nact_prefs_save_preferences ();
		gtk_widget_destroy (GTK_WIDGET (dialog));
		nact_destroy_glade_objects ();
		g_object_unref (config);
		gtk_main_quit ();
		break;
	case GTK_RESPONSE_HELP :
		break;
	}
}

static void
list_selection_changed_cb (GtkTreeSelection *selection, gpointer user_data)
{
	GtkWidget *nact_edit_button;
	GtkWidget *nact_delete_button;
	GtkWidget *nact_duplicate_button;
	
	nact_edit_button = nact_get_glade_widget ("EditActionButton");
	nact_delete_button = nact_get_glade_widget ("DeleteActionButton");
	nact_duplicate_button = nact_get_glade_widget ("DuplicateActionButton");

	if (gtk_tree_selection_count_selected_rows (selection) > 0) {
		gtk_widget_set_sensitive (nact_edit_button, TRUE);
		gtk_widget_set_sensitive (nact_delete_button, TRUE);
		gtk_widget_set_sensitive (nact_duplicate_button, TRUE);
	} else {
		gtk_widget_set_sensitive (nact_edit_button, FALSE);
		gtk_widget_set_sensitive (nact_delete_button, FALSE);
		gtk_widget_set_sensitive (nact_duplicate_button, FALSE);
	}
}

static void nact_setup_actions_list (GtkWidget *list)
{
	GtkListStore *model;
	GtkTreeViewColumn *column;

	/* create the model */
	model = gtk_list_store_new (N_COLUMN, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (list), GTK_TREE_MODEL (model));
	nact_fill_actions_list (list);
	g_object_unref (model);

	/* create columns on the tree view */
	column = gtk_tree_view_column_new_with_attributes (_("Icon"),
							   gtk_cell_renderer_pixbuf_new (),
							   "pixbuf", MENU_ICON_COLUMN, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

	column = gtk_tree_view_column_new_with_attributes (_("Label"),
							   gtk_cell_renderer_text_new (),
							   "text", MENU_LABEL_COLUMN, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

	/* set up selection */
	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (list))), "changed",
			  G_CALLBACK (list_selection_changed_cb), NULL);

}

static void
init_dialog (void)
{
	gint width, height, x, y;
	GtkWidget *nact_dialog;
	GtkWidget *nact_actions_list;
	GtkWidget* nact_edit_button;
	GladeXML *gui = nact_get_glade_xml_object (GLADE_MAIN_WIDGET);
	if (!gui) {
		nautilus_actions_display_error (_("Could not load interface for Nautilus Actions Config Tool"));
		exit (1);
	}

	glade_xml_signal_autoconnect(gui);

	nact_dialog = nact_get_glade_widget ("ActionsDialog");

	nact_actions_list = nact_get_glade_widget ("ActionsList");
	nact_setup_actions_list (nact_actions_list);

	/* Get the default dialog size */
	gtk_window_get_default_size (GTK_WINDOW (nact_dialog), &width, &height);
	/* Override with preferred one, if any */
	nact_prefs_get_main_dialog_size (&width, &height);
	
	gtk_window_resize (GTK_WINDOW (nact_dialog), width, height);

	/* Get the default position */
	gtk_window_get_position (GTK_WINDOW (nact_dialog), &x, &y);
	/* Override with preferred one, if any */
	nact_prefs_get_main_dialog_position (&x, &y);

	gtk_window_move (GTK_WINDOW (nact_dialog), x, y);

#if ((GTK_MAJOR_VERSION == 2) && (GTK_MINOR_VERSION == 4))	
	/* Fix a stock icon bug with GTK+ 2.4 */
	nact_edit_button = nact_get_glade_widget ("EditActionButton");
	gtk_button_set_use_stock (GTK_BUTTON (nact_edit_button), FALSE);
	gtk_button_set_use_underline (GTK_BUTTON (nact_edit_button), TRUE);
	gtk_button_set_label (GTK_BUTTON (nact_edit_button), "_Edit");
#endif

	/* display the dialog */
	gtk_widget_show (nact_dialog);
	g_object_unref (gui);
}

int
main (int argc, char *argv[])
{
	/* initialize application */
#ifdef ENABLE_NLS
        bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
# ifdef HAVE_BIND_TEXTDOMAIN_CODESET
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
# endif
        textdomain (GETTEXT_PACKAGE);
#endif

	gtk_init (&argc, &argv);

	config = nautilus_actions_config_gconf_writer_get ();

	/* create main dialog */
	init_dialog ();

	/* run the application */
	gtk_main ();
	return 0;
}

// vim:ts=3:sw=3:tw=1024:cin
