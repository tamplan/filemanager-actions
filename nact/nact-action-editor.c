/*
 * Nautilus Actions
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

#include <string.h>
#include <glib/gi18n.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkentry.h>
#include <gtk/gtktogglebutton.h>
#include <glade/glade-xml.h>
#include <libnautilus-actions/nautilus-actions-config-gconf-writer.h>
#include "nact-editor.h"
#include "nact-action-editor.h"
#include "nact-profile-editor.h"
#include "nact-utils.h"
#include "nact-prefs.h"
#include "nact.h"

enum {
	ICON_STOCK_COLUMN = 0,
	ICON_LABEL_COLUMN,
	ICON_N_COLUMN
};

/* gui callback functions */
void action_legend_button_toggled_cb (GtkToggleButton *button, gpointer user_data);
void action_field_changed_cb (GObject *object, gpointer user_data);
void action_icon_browse_button_clicked_cb (GtkButton *button, gpointer user_data);
void action_path_browse_button_clicked_cb (GtkButton *button, gpointer user_data);
void action_legend_button_toggled_cb (GtkToggleButton *button, gpointer user_data);
void action_add_scheme_clicked (GtkWidget* widget, gpointer user_data);
void action_remove_scheme_clicked (GtkWidget* widget, gpointer user_data);

static void     preview_icon_changed_cb (GtkEntry* icon_entry, gpointer user_data);
static gboolean open_action_editor( NautilusActionsConfigAction *action, const gchar* profile_name, gboolean is_new );
static void     update_example_label (void);

void
action_legend_button_toggled_cb (GtkToggleButton *button, gpointer user_data)
{
	if (gtk_toggle_button_get_active (button))
	{
		nact_show_legend_dialog( GLADE_EDIT_ACTION_DIALOG_WIDGET );
	}
	else
	{
		nact_hide_legend_dialog( GLADE_EDIT_ACTION_DIALOG_WIDGET );
	}
}

static void
preview_icon_changed_cb (GtkEntry* icon_entry, gpointer user_data)
{
	nact_preview_icon_changed_cb( icon_entry, user_data, GLADE_EDIT_ACTION_DIALOG_WIDGET );
}

static void
update_example_label (void)
{
	nact_update_example_label( GLADE_EDIT_ACTION_DIALOG_WIDGET );
}

void
action_icon_browse_button_clicked_cb (GtkButton *button, gpointer user_data)
{
	nact_icon_browse_button_clicked_cb( button, user_data, GLADE_EDIT_ACTION_DIALOG_WIDGET );
}

void
action_field_changed_cb (GObject *object, gpointer user_data)
{
	GtkWidget* editor = nact_get_glade_widget_from( GLADE_EDIT_ACTION_DIALOG_WIDGET, GLADE_EDIT_ACTION_DIALOG_WIDGET );
	GtkWidget* command_path = nact_get_glade_widget_from ("CommandPathEntry", GLADE_EDIT_ACTION_DIALOG_WIDGET);
	const gchar *path = gtk_entry_get_text (GTK_ENTRY (command_path));
	const gchar *label = gtk_entry_get_text( GTK_ENTRY( nact_get_glade_widget_from ("MenuLabelEntry", GLADE_EDIT_ACTION_DIALOG_WIDGET )));

	update_example_label ();

	if (path && strlen (path) > 0 && label && strlen( label ))
		gtk_dialog_set_response_sensitive (GTK_DIALOG (editor), GTK_RESPONSE_OK, TRUE);
	else
		gtk_dialog_set_response_sensitive (GTK_DIALOG (editor), GTK_RESPONSE_OK, FALSE);
}

void
action_path_browse_button_clicked_cb (GtkButton *button, gpointer user_data)
{
	nact_path_browse_button_clicked_cb( button, user_data, GLADE_EDIT_ACTION_DIALOG_WIDGET );
}

void
action_add_scheme_clicked (GtkWidget* widget, gpointer user_data)
{
	nact_add_scheme_clicked( widget, user_data, GLADE_EDIT_ACTION_DIALOG_WIDGET );
}

void
action_remove_scheme_clicked (GtkWidget* widget, gpointer user_data)
{
	nact_remove_scheme_clicked( widget, user_data, GLADE_EDIT_ACTION_DIALOG_WIDGET );
}

static gboolean
open_action_editor( NautilusActionsConfigAction *action, const gchar* profile_name, gboolean is_new )
{
	static const char *thisfn = "open_action_editor";
	gboolean ret = FALSE;
	static gboolean init = FALSE;
	GtkWidget* editor;
	GladeXML *gui;
	NautilusActionsConfigGconfWriter *config;
	GList* aligned_widgets = NULL;
	GList* iter;
	GSList* list;
	GtkSizeGroup* label_size_group;
	GtkSizeGroup* button_size_group;
	GtkWidget *scheme_listview;
	GtkWidget *menu_label, *menu_tooltip, *menu_icon;
	GtkWidget *command_path, *command_params, *test_patterns, *match_case, *test_mimetypes;
	GtkWidget *only_files, *only_folders, *both, *accept_multiple;
	gint width, height /*, x, y*/;
	GtkTreeModel* scheme_model;
	static NactCallbackData *scheme_data = NULL;

	g_debug( "%s: action=%p (%s), profile=%s, is_new=%s, init=%s",
			thisfn, action, action->label, profile_name, is_new ? "True":"False", init ? "True":"False" );
	if (!init)
	{
		/* load the GUI */
		gui = nact_get_glade_xml_object (GLADE_EDIT_ACTION_DIALOG_WIDGET);
		if (!gui) {
			g_error (_("Could not load interface for Nautilus Actions Config Tool"));
			return FALSE;
		}
		g_debug( "%s: dialog successfully loaded", thisfn );

		glade_xml_signal_autoconnect(gui);

		scheme_data = g_new0( NactCallbackData, 1 );
		scheme_data->dialog = GLADE_EDIT_ACTION_DIALOG_WIDGET;
		scheme_data->field_changed_cb = action_field_changed_cb;
		scheme_data->update_example_label = update_example_label;
		nact_create_schemes_selection_list ( scheme_data );

		menu_icon = nact_get_glade_widget_from ("MenuIconComboBoxEntry", GLADE_EDIT_ACTION_DIALOG_WIDGET);

		g_assert (menu_icon != NULL);

		g_signal_connect (G_OBJECT (GTK_BIN (menu_icon)->child), "changed",
							   G_CALLBACK (preview_icon_changed_cb), NULL);

		nact_fill_menu_icon_combo_list_of (GTK_COMBO_BOX_ENTRY (menu_icon));

		/* TODO: replace deprecated gtk_tooltips_set_tip by its equivalent */
		/*gtk_tooltips_set_tip (gtk_tooltips_new (), GTK_WIDGET (GTK_BIN (menu_icon)->child),
									 _("Icon of the menu item in the Nautilus popup menu"), "");*/

		aligned_widgets = nact_get_glade_widget_prefix_from ("LabelAlign", GLADE_EDIT_ACTION_DIALOG_WIDGET);
		label_size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
		for (iter = aligned_widgets; iter; iter = iter->next)
		{
			gtk_size_group_add_widget (label_size_group, GTK_WIDGET (iter->data));
		}

		aligned_widgets = nact_get_glade_widget_prefix_from ("CLabelAlign", GLADE_EDIT_ACTION_DIALOG_WIDGET);
		label_size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
		for (iter = aligned_widgets; iter; iter = iter->next)
		{
			gtk_size_group_add_widget (label_size_group, GTK_WIDGET (iter->data));
		}
		button_size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
		gtk_size_group_add_widget (button_size_group,
											nact_get_glade_widget_from ("IconBrowseButton",
																		GLADE_EDIT_ACTION_DIALOG_WIDGET));
		gtk_size_group_add_widget (button_size_group,
											nact_get_glade_widget_from ("PathBrowseButton",
																		GLADE_EDIT_ACTION_DIALOG_WIDGET));
		gtk_size_group_add_widget (button_size_group,
											nact_get_glade_widget_from ("LegendButton",
																		GLADE_EDIT_ACTION_DIALOG_WIDGET));
		/* free memory */
		g_object_unref (gui);
		init = TRUE;
	}

	editor = nact_get_glade_widget_from( GLADE_EDIT_ACTION_DIALOG_WIDGET, GLADE_EDIT_ACTION_DIALOG_WIDGET );

	nautilus_actions_config_action_dump( action );

	NautilusActionsConfigActionProfile *action_profile =
		nautilus_actions_config_action_get_profile( action, profile_name );
	g_debug( "%s: action_profile=%p", thisfn, action_profile );

	if (is_new)
	{
		gtk_window_set_title (GTK_WINDOW (editor), _("Add a new action"));
	}
	else
	{
		gchar* title = g_strdup_printf (_("Edit action \"%s\""), action->label );
		gtk_window_set_title (GTK_WINDOW (editor), title);
		g_free (title);
	}

	/* Get the default dialog size */
	gtk_window_get_default_size (GTK_WINDOW (editor), &width, &height);

	/* FIXME: update preference data for profile editor dialog

	// Override with preferred one, if any
	nact_prefs_get_edit_dialog_size (&width, &height);

	gtk_window_resize (GTK_WINDOW (editor), width, height);

	if (nact_prefs_get_edit_dialog_position (&x, &y))
	{
		gtk_window_move (GTK_WINDOW (editor), x, y);
	}
	*/

	menu_label = nact_get_glade_widget_from ("MenuLabelEntry", GLADE_EDIT_ACTION_DIALOG_WIDGET);
	gtk_entry_set_text (GTK_ENTRY (menu_label), action->label);

	menu_tooltip = nact_get_glade_widget_from ("MenuTooltipEntry", GLADE_EDIT_ACTION_DIALOG_WIDGET);
	gtk_entry_set_text (GTK_ENTRY (menu_tooltip), action->tooltip);

	menu_icon = nact_get_glade_widget_from ("MenuIconComboBoxEntry", GLADE_EDIT_ACTION_DIALOG_WIDGET);
	gtk_entry_set_text (GTK_ENTRY (GTK_BIN (menu_icon)->child), action->icon);

	command_path = nact_get_glade_widget_from ("CommandPathEntry", GLADE_EDIT_ACTION_DIALOG_WIDGET);
	gtk_entry_set_text (GTK_ENTRY (command_path), action_profile->path);

	command_params = nact_get_glade_widget_from ("CommandParamsEntry", GLADE_EDIT_ACTION_DIALOG_WIDGET);
	gtk_entry_set_text (GTK_ENTRY (command_params), action_profile->parameters);

	test_patterns = nact_get_glade_widget_from ("PatternEntry", GLADE_EDIT_ACTION_DIALOG_WIDGET);
	nact_set_action_match_string_list (GTK_ENTRY (test_patterns), action_profile->basenames, "*");

	match_case = nact_get_glade_widget_from ("MatchCaseButton", GLADE_EDIT_ACTION_DIALOG_WIDGET);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (match_case), action_profile->match_case);

	test_mimetypes = nact_get_glade_widget_from ("MimeTypeEntry", GLADE_EDIT_ACTION_DIALOG_WIDGET);

	nact_set_action_match_string_list (GTK_ENTRY (test_mimetypes), action_profile->mimetypes, "*/*");

	only_folders = nact_get_glade_widget_from ("OnlyFoldersButton", GLADE_EDIT_ACTION_DIALOG_WIDGET);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (only_folders), action_profile->is_dir);

	only_files = nact_get_glade_widget_from ("OnlyFilesButton", GLADE_EDIT_ACTION_DIALOG_WIDGET);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (only_files), action_profile->is_file);

	both = nact_get_glade_widget_from ("BothButton", GLADE_EDIT_ACTION_DIALOG_WIDGET);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (both), action_profile->is_file && action_profile->is_dir);

	accept_multiple = nact_get_glade_widget_from ("AcceptMultipleButton", GLADE_EDIT_ACTION_DIALOG_WIDGET);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (accept_multiple), action_profile->accept_multiple_files);

	scheme_listview = nact_get_glade_widget_from ("SchemesTreeView", GLADE_EDIT_ACTION_DIALOG_WIDGET);
	scheme_model = gtk_tree_view_get_model (GTK_TREE_VIEW (scheme_listview));
	/*g_assert( GTK_IS_TREE_MODEL( scheme_model ));*/
	gtk_tree_model_foreach( scheme_model, (GtkTreeModelForeachFunc) nact_reset_schemes_list, NULL);
	/*g_debug( "after tree model foreach" );*/
	g_slist_foreach (action_profile->schemes, (GFunc) nact_set_action_schemes, scheme_model);

	/* default is to not enable the OK button */
	gtk_dialog_set_response_sensitive (GTK_DIALOG (editor), GTK_RESPONSE_OK, FALSE);

	/*update_example_label ();*/
	action_field_changed_cb( NULL, NULL );

	/* run the dialog */
	switch (gtk_dialog_run (GTK_DIALOG (editor))) {
	case GTK_RESPONSE_OK :
		nautilus_actions_config_action_set_label (action, (gchar*)gtk_entry_get_text (GTK_ENTRY (menu_label)));
		nautilus_actions_config_action_set_tooltip (action, gtk_entry_get_text (GTK_ENTRY (menu_tooltip)));
		nautilus_actions_config_action_set_icon (action, gtk_entry_get_text (GTK_ENTRY (GTK_BIN (menu_icon)->child)));

		nautilus_actions_config_action_profile_set_path (action_profile, gtk_entry_get_text (GTK_ENTRY (command_path)));
		nautilus_actions_config_action_profile_set_parameters (action_profile, gtk_entry_get_text (GTK_ENTRY (command_params)));
		g_debug( "%s: parameters='%s'", thisfn, action_profile->parameters );

		list = nact_get_action_match_string_list (gtk_entry_get_text (GTK_ENTRY (test_patterns)), "*");
		nautilus_actions_config_action_profile_set_basenames (action_profile, list);
		g_slist_foreach (list, (GFunc) g_free, NULL);
		g_slist_free (list);

		nautilus_actions_config_action_profile_set_match_case (
			action_profile, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (match_case)));

		list = nact_get_action_match_string_list (gtk_entry_get_text (GTK_ENTRY (test_mimetypes)), "*/*");
		nautilus_actions_config_action_profile_set_mimetypes (action_profile, list);
		g_slist_foreach (list, (GFunc) g_free, NULL);
		g_slist_free (list);

		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (only_files))) {
			nautilus_actions_config_action_profile_set_is_file (action_profile, TRUE);
			nautilus_actions_config_action_profile_set_is_dir (action_profile, FALSE);
		} else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (only_folders))) {
			nautilus_actions_config_action_profile_set_is_file (action_profile, FALSE);
			nautilus_actions_config_action_profile_set_is_dir (action_profile, TRUE);
		} else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (both))) {
			nautilus_actions_config_action_profile_set_is_file (action_profile, TRUE);
			nautilus_actions_config_action_profile_set_is_dir (action_profile, TRUE);
		}

		nautilus_actions_config_action_profile_set_accept_multiple (
			action_profile, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (accept_multiple)));

		list = NULL;
		gtk_tree_model_foreach (scheme_model, (GtkTreeModelForeachFunc)nact_utils_get_action_schemes_list, &list);
		nautilus_actions_config_action_profile_set_schemes (action_profile, list);
		g_slist_foreach (list, (GFunc) g_free, NULL);
		g_slist_free (list);

		config = nautilus_actions_config_gconf_writer_get ();
		if (is_new)
		{
			/*if( nautilus_actions_config_action_add_profile (action, profile_name, action_profile, NULL)){*/
				/* TODO: If necessary deal with the GError returned */
				ret = nautilus_actions_config_add_action (NAUTILUS_ACTIONS_CONFIG (config), action, NULL);
			/*}*/
		}
		else
		{
			/*nautilus_actions_config_action_replace_profile (action, profile_name, action_profile);*/
			ret = nautilus_actions_config_update_action (NAUTILUS_ACTIONS_CONFIG (config), action);
		}
		g_object_unref (config);
		break;
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL :
		ret = FALSE;
		break;
	}

	/*
	// Save preferences
	list = NULL;
	gtk_tree_model_foreach (scheme_model, (GtkTreeModelForeachFunc)get_all_schemes_list, &list);
	nact_prefs_set_schemes_list (list);
	g_slist_foreach (list, (GFunc) g_free, NULL);
	g_slist_free (list);

	nact_prefs_set_edit_dialog_size (GTK_WINDOW (editor));
	nact_prefs_set_edit_dialog_position (GTK_WINDOW (editor));
	*/

	nact_hide_legend_dialog( GLADE_EDIT_ACTION_DIALOG_WIDGET );
	gtk_widget_hide (editor);

	return ret;
}

gboolean
nact_action_editor_new( void )
{
	static const gchar* new_profile_name = NAUTILUS_ACTIONS_DEFAULT_PROFILE_NAME;

	NautilusActionsConfigAction *action = nautilus_actions_config_action_new_default ();

	gboolean ret = open_action_editor( action, new_profile_name, TRUE );

	nautilus_actions_config_action_free (action);

	return( ret );
}

gboolean
nact_action_editor_edit( NautilusActionsConfigAction *action )
{
	gchar* profile_name = nautilus_actions_config_action_get_first_profile_name( action );

	NautilusActionsConfigAction *action_dup = nautilus_actions_config_action_dup( action );

	gboolean ret = open_action_editor( action_dup, profile_name, FALSE );

	nautilus_actions_config_action_free( action_dup );
	g_free( profile_name );

	return( ret );
}
