/* Nautilus Actions configuration tool
 * Copyright (C) 2005 The GNOME Foundation
 *
 * Authors:
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
#include <glib/gi18n.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkmessagedialog.h>
#include <gtk/gtktreeview.h>
#include <glade/glade-xml.h>
#include <libnautilus-actions/nautilus-actions-config.h>

static GladeXML *gui;
static GtkWidget *nact_dialog;
static GtkWidget *nact_actions_list;
static GtkWidget *nact_add_button;
static GtkWidget *nact_edit_button;
static GtkWidget *nact_delete_button;
static NautilusActionsConfig *config;

void
nautilus_actions_display_error (const gchar *msg)
{
}

static void
add_button_clicked_cb (GtkButton *button, gpointer user_data)
{
	nact_editor_new_action ();
}

static void
edit_button_clicked_cb (GtkButton *button, gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (nact_actions_list));

	if (gtk_tree_selection_get_selected (selection, NULL, &iter)) {
		gchar *label;
		NautilusActionsConfigAction *action;

		gtk_tree_model_get (gtk_tree_view_get_model (GTK_TREE_VIEW (nact_actions_list)), &iter, 1, &label, -1);

		action = nautilus_actions_config_get_action (config, label);
		if (action)
			nact_editor_edit_action (action);

		g_free (label);
	}
}

static void
delete_button_clicked_cb (GtkButton *button, gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (nact_actions_list));

	if (gtk_tree_selection_get_selected (selection, NULL, &iter)) {
		gchar *label;

		gtk_tree_model_get (gtk_tree_view_get_model (GTK_TREE_VIEW (nact_actions_list)), &iter, 1, &label, -1);
		nautilus_actions_config_remove_action (config, label);

		g_free (label);
	}
}

static void
dialog_response_cb (GtkDialog *dialog, gint response_id, gpointer user_data)
{
	switch (response_id) {
	case GTK_RESPONSE_CLOSE :
		gtk_widget_destroy (GTK_WIDGET (dialog));
		gtk_main_quit ();
		break;
	case GTK_RESPONSE_HELP :
		break;
	}
}

static void
list_selection_changed_cb (GtkTreeSelection *selection, gpointer user_data)
{
	if (gtk_tree_selection_count_selected_rows (selection) > 0) {
		gtk_widget_set_sensitive (nact_edit_button, TRUE);
		gtk_widget_set_sensitive (nact_delete_button, TRUE);
	} else {
		gtk_widget_set_sensitive (nact_edit_button, FALSE);
		gtk_widget_set_sensitive (nact_delete_button, FALSE);
	}
}

static void
setup_actions_list (GtkWidget *list)
{
	GtkListStore *model;
	GSList *actions, *l;
	GtkTreeViewColumn *column;

	/* create the model */
	model = gtk_list_store_new (2, GDK_TYPE_PIXBUF, G_TYPE_STRING);

	actions = nautilus_actions_config_get_actions (config);
	for (l = actions; l != NULL; l = l->next) {
		GtkTreeIter iter;
		NautilusActionsConfigAction *action = l->data;

		gtk_list_store_append (model, &iter);
		gtk_list_store_set (model, &iter, 1, action->label, -1);
	}

	nautilus_actions_config_free_actions_list (actions);

	gtk_tree_view_set_model (GTK_TREE_VIEW (list), GTK_TREE_MODEL (model));
	g_object_unref (model);

	/* create columns on the tree view */
	column = gtk_tree_view_column_new_with_attributes (_("Icon"),
							   gtk_cell_renderer_pixbuf_new (),
							   "pixbuf", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

	column = gtk_tree_view_column_new_with_attributes (_("Label"),
							   gtk_cell_renderer_text_new (),
							   "text", 1, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

	gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

	/* set up selection */
	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (list))), "changed",
			  G_CALLBACK (list_selection_changed_cb), NULL);
}

static void
init_dialog (void)
{
	gui = glade_xml_new (GLADEDIR "/nautilus-actions-config.glade", "ActionsDialog", NULL);
	if (!gui) {
		nautilus_actions_display_error (_("Could not load interface for Nautilus Actions Config Tool"));
		exit (1);
	}

	nact_dialog = glade_xml_get_widget (gui, "ActionsDialog");
	g_signal_connect (G_OBJECT (nact_dialog), "response", G_CALLBACK (dialog_response_cb), NULL);

	nact_actions_list = glade_xml_get_widget (gui, "ActionsList");
	setup_actions_list (nact_actions_list);

	nact_add_button = glade_xml_get_widget (gui, "AddActionButton");
	g_signal_connect (G_OBJECT (nact_add_button), "clicked", G_CALLBACK (add_button_clicked_cb), NULL);

	nact_edit_button = glade_xml_get_widget (gui, "EditActionButton");
	gtk_widget_set_sensitive (nact_edit_button, FALSE);
	g_signal_connect (G_OBJECT (nact_edit_button), "clicked", G_CALLBACK (edit_button_clicked_cb), NULL);

	nact_delete_button = glade_xml_get_widget (gui, "DeleteActionButton");
	gtk_widget_set_sensitive (nact_delete_button, FALSE);
	g_signal_connect (G_OBJECT (nact_delete_button), "clicked", G_CALLBACK (delete_button_clicked_cb), NULL);

	/* display the dialog */
	gtk_widget_show (nact_dialog);
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

	config = nautilus_actions_config_get ();

	/* create main dialog */
	init_dialog ();

	/* run the application */
	gtk_main ();
}
