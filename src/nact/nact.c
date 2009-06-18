/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009 Pierre Wieser and others (see AUTHORS)
 *
 * This Program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This Program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this Library; see the file COPYING.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors:
 *   Frederic Ruaudel <grumz@grumz.net>
 *   Rodrigo Moya <rodrigo@gnome-db.org>
 *   Pierre Wieser <pwieser@trychlos.org>
 *   ... and many others (see AUTHORS)
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>

#include <glib/gi18n.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkmessagedialog.h>
#include <gtk/gtktreeview.h>
#include <glade/glade-xml.h>

#include <common/na-action.h>
#include <common/na-action-profile.h>
#include <common/na-pivot.h>
#include <common/nautilus-actions-config.h>
#include <common/nautilus-actions-config-gconf-writer.h>

#include "nact.h"
#include "nact-application.h"
#include "nact-editor.h"
#include "nact-import-export.h"
#include "nact-prefs.h"
#include "nact-action-editor.h"
#include "nact-utils.h"

/* gui callback functions */
void     dialog_response_cb (GtkDialog *dialog, gint response_id, gpointer user_data);
void     add_button_clicked_cb (GtkButton *button, gpointer user_data);
void     delete_button_clicked_cb (GtkButton *button, gpointer user_data);
void     duplicate_button_clicked_cb (GtkButton *button, gpointer user_data);
void     edit_button_clicked_cb (GtkButton *button, gpointer user_data);
void     im_export_button_clicked_cb (GtkButton *button, gpointer user_data);
gboolean on_ActionsList_button_press_event( GtkWidget *widget, GdkEventButton *event, gpointer data );

static NactApplication *st_application = NULL;
static NAPivot         *st_pivot = NULL;

static void  list_selection_changed_cb (GtkTreeSelection *selection, gpointer user_data);
static void  fill_actions_list( GtkWidget *list );
static void  setup_actions_list( GtkWidget *list );

gboolean
on_ActionsList_button_press_event( GtkWidget *widget, GdkEventButton *event, gpointer data )
{
	if( event->type == GDK_2BUTTON_PRESS ){
		edit_button_clicked_cb( NULL, NULL );
		return( TRUE );
	}
	/* unmanaged event: let the framework do its job */
	return( FALSE );
}

static void
fill_actions_list (GtkWidget *list)
{
	GSList *actions, *l;
	GtkListStore *model = GTK_LIST_STORE(gtk_tree_view_get_model (GTK_TREE_VIEW (list)));

	gtk_list_store_clear (model);

	actions = na_pivot_get_label_sorted_actions( st_pivot );

	for( l = actions ; l != NULL ; l = l->next ){
		GtkTreeIter iter;
		GtkStockItem item;
		GdkPixbuf* icon = NULL;

		NAAction *action = NA_ACTION( l->data );
		gchar *uuid = na_action_get_uuid( action );
		gchar *label = na_action_get_label( action );
		gchar *iconname = na_action_get_icon( action );

		/* TODO: use the same algorythm than Nautilus to find and
		 * display an icon + move the code to NAAction class +
		 * remove na_action_get_verified_icon_name
		 */
		if( icon ){
			if( gtk_stock_lookup( iconname, &item )){
				icon = gtk_widget_render_icon( list, iconname, GTK_ICON_SIZE_MENU, NULL );

			} else if( g_file_test( iconname, G_FILE_TEST_EXISTS )
				   && g_file_test( iconname, G_FILE_TEST_IS_REGULAR )){
				gint width;
				gint height;
				GError* error = NULL;

				gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &width, &height);
				icon = gdk_pixbuf_new_from_file_at_size( iconname, width, height, &error );
				if (error)
				{
					icon = NULL;
					g_error_free (error);
				}
			}
		}
		gtk_list_store_append (model, &iter);
		gtk_list_store_set (model, &iter,
				    MENU_ICON_COLUMN, icon,
				    MENU_LABEL_COLUMN, label,
				    UUID_COLUMN, uuid,
				    -1);

		g_free( iconname );
		g_free( label );
		g_free( uuid );
	}

	nautilus_actions_config_free_actions_list (actions);
}

/*
 * creating a new action
 * pwi 2009-05-19
 * I don't want the profiles feature spread wide while I'm not convinced
 * that it is useful and actually used.
 * so the new action is silently created with a default profile name
 */
void
add_button_clicked_cb (GtkButton *button, gpointer user_data)
{
	if( nact_action_editor_new())
		fill_actions_list( nact_get_glade_widget( "ActionsList" ));
}

/*
 * editing an existing action
 * pwi 2009-05-19
 * I don't want the profiles feature spread wide while I'm not convinced
 * that it is useful and actually used.
 * so :
 * - if there is only one profile, the user will be directed to a dialog
 *   box which includes all needed fields, but without any profile notion
 * - if there are more than one profile, one can assume that the user has
 *   found a use to the profiles, and let him edit them
 */
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
		NAAction *action;

		gtk_tree_model_get (model, &iter, UUID_COLUMN, &uuid, -1);

		action = NA_ACTION( na_pivot_get_action( st_pivot, uuid ));

		if( action ){
			guint count = na_action_get_profiles_count( action );
			if( count > 1 ){
				if (nact_editor_edit_action (( NautilusActionsConfigAction *) action))
					fill_actions_list (nact_actions_list);
			} else {
				if( nact_action_editor_edit( ( NautilusActionsConfigAction *) action ))
					fill_actions_list( nact_actions_list );
			}
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
	gchar *error = NULL;
	gchar *tmp, *label;

	nact_actions_list = nact_get_glade_widget ("ActionsList");

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (nact_actions_list));

	if (gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		gchar *uuid;
		NAAction *action;
		NAAction* new_action;

		gtk_tree_model_get (model, &iter, UUID_COLUMN, &uuid, -1);

		action = NA_ACTION( na_pivot_get_action( st_pivot, uuid ));
		new_action = na_action_duplicate( action );
		na_action_set_new_uuid( new_action );

		/*if( nautilus_actions_config_add_action( NAUTILUS_ACTIONS_CONFIG (config), new_action, &error )){*/
		if( na_pivot_write_action( st_pivot, G_OBJECT( new_action ), &error )){
			fill_actions_list (nact_actions_list);
		} else {
			/* i18n notes: will be displayed in a dialog */
			label = na_action_get_label( action );
			tmp = g_strdup_printf (_("Can't duplicate action '%s'!"), label);
			nautilus_actions_display_error( tmp, error );
			g_free( error );
			g_free( label );
			g_free( tmp );
		}

		g_free( uuid );
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
		/*nautilus_actions_config_remove_action (NAUTILUS_ACTIONS_CONFIG (config), uuid);*/
		fill_actions_list (nact_actions_list);

		g_free (uuid);
	}
}

void
im_export_button_clicked_cb (GtkButton *button, gpointer user_data)
{
	GtkWidget *nact_actions_list;

	if (nact_import_export_actions ())
	{
		nact_actions_list = nact_get_glade_widget ("ActionsList");
		fill_actions_list (nact_actions_list);
	}
}

void
dialog_response_cb (GtkDialog *dialog, gint response_id, gpointer user_data)
{
	GtkWidget* nact_about_dialog;
	GtkWidget *nact_prof_paste_button = nact_get_glade_widget_from ("PasteProfileButton", GLADE_EDIT_DIALOG_WIDGET);

	switch (response_id) {
	case GTK_RESPONSE_NONE :
	case GTK_RESPONSE_DELETE_EVENT :
	case GTK_RESPONSE_CLOSE :
		/* Free any profile in the clipboard */
		nautilus_actions_config_action_profile_free (g_object_steal_data (G_OBJECT (nact_prof_paste_button), "profile"));

		/* FIXME : update pref settings
		nact_prefs_set_main_dialog_size (GTK_WINDOW (dialog));
		nact_prefs_set_main_dialog_position (GTK_WINDOW (dialog));
		nact_prefs_save_preferences ();
		*/

		gtk_widget_destroy (GTK_WIDGET (dialog));
		nact_destroy_glade_objects ();
		gtk_main_quit ();
		break;
	case GTK_RESPONSE_HELP :
#if ((GTK_MAJOR_VERSION == 2) && (GTK_MINOR_VERSION >= 6))
		nact_about_dialog = nact_get_glade_widget_from ("AboutDialog", GLADE_ABOUT_DIALOG_WIDGET);
		gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (nact_about_dialog), PACKAGE_VERSION);
		gtk_about_dialog_set_logo_icon_name (GTK_ABOUT_DIALOG (nact_about_dialog), "nautilus-actions");
		gtk_dialog_run (GTK_DIALOG (nact_about_dialog));
		gtk_widget_hide (nact_about_dialog);
#endif
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

static void
setup_actions_list (GtkWidget *list)
{
	GtkListStore *model;
	GtkTreeViewColumn *column;

	/* create the model */
	model = gtk_list_store_new (N_COLUMN, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (list), GTK_TREE_MODEL (model));
	fill_actions_list( list );
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

GtkWindow *
nact_init_dialog( GObject *application )
{
	gint width, height, x, y;
	GtkWidget *nact_dialog;
	GtkWidget *nact_actions_list;
	/*GtkWidget* nact_edit_button;*/
	GtkWidget* nact_about_button;

	g_assert( NACT_IS_APPLICATION( application ));
	st_application = NACT_APPLICATION( application );

	g_object_get( G_OBJECT( st_application ), "pivot", &st_pivot, NULL );
	g_assert( NA_IS_PIVOT( st_pivot ));

	/*config = nautilus_actions_config_gconf_writer_get ();*/

	GladeXML *gui = nact_get_glade_xml_object (GLADE_MAIN_WIDGET);
	if (!gui) {
		g_error (_("Could not load interface for Nautilus Actions Config Tool"));
		exit (1);
	}

	glade_xml_signal_autoconnect(gui);

	nact_dialog = nact_get_glade_widget ("ActionsDialog");

	nact_actions_list = nact_get_glade_widget ("ActionsList");
	setup_actions_list( nact_actions_list );

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

#if ((GTK_MAJOR_VERSION == 2) && (GTK_MINOR_VERSION >= 6))
	/* Add about dialog for GTK+ >= 2.6 */
	nact_about_button = nact_get_glade_widget ("AboutButton");
	gtk_widget_show (nact_about_button);
#endif

	/* display the dialog */
	gtk_widget_show (nact_dialog);
	g_object_unref (gui);

	return( GTK_WINDOW( nact_dialog ));
}
