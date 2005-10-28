/* Nautilus Actions configuration tool
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
#include <string.h>
#include <glib/gi18n.h>
#include "nact-utils.h"

#define GLADE_FILE GLADEDIR "/nautilus-actions-config.glade"

static GHashTable* get_glade_object_hashtable ()
{
	static GHashTable* glade_object_hash = NULL;

	if (glade_object_hash == NULL)
	{
		glade_object_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
	}

	return glade_object_hash;
}

GladeXML* nact_get_glade_xml_object (const gchar* root_widget)
{
	GHashTable* glade_object_hash = get_glade_object_hashtable ();
	GladeXML* retv = NULL;

	retv = (GladeXML*)g_hash_table_lookup (glade_object_hash, root_widget);
	if (retv == NULL)
	{
		retv = glade_xml_new (GLADE_FILE, root_widget, NULL);

		g_hash_table_insert (glade_object_hash, g_strdup (root_widget), retv);
	}

	return GLADE_XML (g_object_ref (retv));
}

GtkWidget* nact_get_glade_widget_from (const gchar* widget_name, const gchar* root_widget)
{
	GladeXML* xml = nact_get_glade_xml_object (root_widget);
	return glade_xml_get_widget (xml, widget_name);
}

GtkWidget* nact_get_glade_widget (const gchar* widget_name)
{
	return nact_get_glade_widget_from (widget_name, GLADE_MAIN_WIDGET);
}

GList* nact_get_glade_widget_prefix_from (const gchar* widget_name, const gchar* root_widget)
{
	GladeXML* xml = nact_get_glade_xml_object (root_widget);
	return glade_xml_get_widget_prefix (xml, widget_name);
}

GList* nact_get_glade_widget_prefix (const gchar* widget_name)
{
	return nact_get_glade_widget_prefix_from (widget_name, GLADE_MAIN_WIDGET);
}

void nact_destroy_glade_objects ()
{
	GHashTable* glade_object_hash = get_glade_object_hashtable ();
	g_hash_table_destroy (glade_object_hash);
}

gboolean nact_utils_get_action_schemes_list (GtkTreeModel* scheme_model, GtkTreePath *path, 
													  GtkTreeIter* iter, gpointer data)
{
	GSList** list = data;
	gboolean toggle_state;
	gchar* scheme;

	gtk_tree_model_get (scheme_model, iter, SCHEMES_CHECKBOX_COLUMN, &toggle_state, 
														 SCHEMES_KEYWORD_COLUMN, &scheme, -1);

	if (toggle_state)
	{
		(*list) = g_slist_append ((*list), scheme);
	}
	else
	{
		g_free (scheme);
	}

	return FALSE; // Don't stop looping
}

static gchar* nact_utils_joinv (const gchar* start, const gchar* separator, gchar** list)
{
	GString* tmp_string = g_string_new ("");
	int i;

	g_return_val_if_fail (list != NULL, NULL);

	if (start != NULL)
	{
		tmp_string = g_string_append (tmp_string, start);
	}

	if (list[0] != NULL)
	{
		tmp_string = g_string_append (tmp_string, _(list[0]));
	}
	
	for (i = 1; list[i] != NULL; i++)
	{
		if (separator)
		{
			tmp_string = g_string_append (tmp_string, separator);
		}
		tmp_string = g_string_append (tmp_string, _(list[i]));
	}

	return g_string_free (tmp_string, FALSE);
}

gchar* nact_utils_parse_parameter (void)
{
	/*
	 * Valid parameters :
	 * 
	 * %u : gnome-vfs URI
	 * %d : base dir of the selected file(s)/folder(s)
	 * %f : the name of the selected file/folder or the 1st one if many are selected
	 * %m : list of the basename of the selected files/directories separated by space.
	 * %M : list of the selected files/directories with their complete path separated by space.
	 * %s : scheme of the gnome-vfs URI
	 * %h : hostname of the gnome-vfs URI
	 * %U : username of the gnome-vfs URI
	 * %% : a percent sign
	 *
	 */
	gchar* retv = NULL;
	GString* tmp_string = g_string_new ("");
	
	/* i18n notes: example strings for the command preview */
	gchar* ex_path = _("/path/to");
	gchar* ex_files[] = { N_("file1.txt"), N_("file2.txt"), NULL };
	gchar* ex_dirs[] = { N_("folder1"), N_("folder2"), NULL };
	gchar* ex_mixed[] = { N_("file1.txt"), N_("folder1"), NULL };
	gchar* ex_scheme_default = "file";
	gchar* ex_host_default = _("test.example.net");
	gchar* ex_one_file = _("file.txt");
	gchar* ex_one_dir = _("folder");
	gchar* ex_one;
	gchar* ex_list;
	gchar* ex_path_list;
	gchar* ex_scheme;
	gchar* ex_host;

	const gchar* param_template = gtk_entry_get_text (GTK_ENTRY (nact_get_glade_widget_from ("CommandParamsEntry", 
																										GLADE_EDIT_DIALOG_WIDGET)));
	gchar* iter = g_strdup (param_template);
	gchar* old_iter = iter;
	gchar* tmp;
	gchar* separator;
	gchar* start;
	GSList* scheme_list = NULL;

	const gchar* command = gtk_entry_get_text (GTK_ENTRY (nact_get_glade_widget_from ("CommandPathEntry", 
																								GLADE_EDIT_DIALOG_WIDGET)));

	g_string_append_printf (tmp_string, "%s ", command);

	gboolean is_file = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (nact_get_glade_widget_from ("OnlyFilesButton", 
																												GLADE_EDIT_DIALOG_WIDGET)));
	gboolean is_dir = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (nact_get_glade_widget_from ("OnlyFoldersButton", 
																												GLADE_EDIT_DIALOG_WIDGET)));
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (nact_get_glade_widget_from ("BothButton", 
																							GLADE_EDIT_DIALOG_WIDGET))))
	{
		is_file = TRUE;
		is_dir = TRUE;
	}
	gboolean accept_multiple = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (nact_get_glade_widget_from ("AcceptMultipleButton", 
																															GLADE_EDIT_DIALOG_WIDGET)));
	
	GtkTreeModel* scheme_model = gtk_tree_view_get_model (GTK_TREE_VIEW (nact_get_glade_widget_from ("SchemesTreeView", 
																													GLADE_EDIT_DIALOG_WIDGET)));
	gtk_tree_model_foreach (scheme_model, (GtkTreeModelForeachFunc)nact_utils_get_action_schemes_list, &scheme_list);

	separator = g_strdup_printf (" %s/", ex_path);
	start = g_strdup_printf ("%s/", ex_path);
	if (accept_multiple)
	{
		if (is_file && is_dir)
		{
			ex_one = ex_files[0];
			ex_list = nact_utils_joinv (NULL, " ", ex_mixed);
			ex_path_list = nact_utils_joinv (start, separator, ex_mixed);
		}
		else if (is_dir)
		{
			ex_one = ex_dirs[0];
			ex_list = nact_utils_joinv (NULL, " ", ex_dirs);
			ex_path_list = nact_utils_joinv (start, separator, ex_dirs);
		}
		else if (is_file)
		{
			ex_one = ex_files[0];
			ex_list = nact_utils_joinv (NULL, " ", ex_files);
			ex_path_list = nact_utils_joinv (start, separator, ex_files);
		}
	}
	else
	{
		if (is_dir && !is_file)
		{
			ex_one = ex_one_dir;
		}
		else
		{
			ex_one = ex_one_file;
		}
		ex_list = g_strdup (ex_one);
		ex_path_list = g_strjoin ("/", ex_path, ex_one, NULL);
	}
	g_free (start);
	g_free (separator);
	
	if (scheme_list != NULL)
	{
		ex_scheme = (gchar*)scheme_list->data;
		if (g_ascii_strcasecmp (ex_scheme, "file") == 0)
		{
			if (g_slist_length (scheme_list) > 1)
			{
				ex_scheme = (gchar*)scheme_list->next->data;
				ex_host = ex_host_default;
			}
			else
			{
				ex_host = "";
			}
		}
		else
		{
			ex_host = ex_host_default;
		}
	}
	else
	{
		ex_scheme = ex_scheme_default;
		ex_host = "";
	}
	
	while (iter = g_strstr_len (iter, strlen (iter), "%"))
	{
		tmp_string = g_string_append_len (tmp_string, old_iter, strlen (old_iter) - strlen (iter));
		switch (iter[1])
		{
			case 'u': // gnome-vfs URI
				tmp = g_strjoin (NULL, ex_scheme, "://", ex_path, "/", ex_one, NULL);
				tmp_string = g_string_append (tmp_string, tmp);
				g_free (tmp);
				break;
			case 'd': // base dir of the selected file(s)/folder(s)
				tmp_string = g_string_append (tmp_string, ex_path);
				break;
			case 'f': // the basename of the selected file/folder or the 1st one if many are selected
				tmp_string = g_string_append (tmp_string, ex_one);
				break;
			case 'm': // list of the basename of the selected files/directories separated by space
				tmp_string = g_string_append (tmp_string, ex_list);
				break;
			case 'M': // list of the selected files/directories with their complete path separated by space.
				tmp_string = g_string_append (tmp_string, ex_path_list);
				break;
			case 's': // scheme of the gnome-vfs URI
				tmp_string = g_string_append (tmp_string, ex_scheme);
				break;
			case 'h': // hostname of the gnome-vfs URI
				tmp_string = g_string_append (tmp_string, ex_host);
				break;
			case 'U': // username of the gnome-vfs URI
				tmp_string = g_string_append (tmp_string, "root");
				break;
			case '%': // a percent sign
				tmp_string = g_string_append_c (tmp_string, '%');
				break;
		}
		iter+=2; // skip the % sign and the character after.
		old_iter = iter; // store the new start of the string
	}
	tmp_string = g_string_append_len (tmp_string, old_iter, strlen (old_iter));
	
	g_free (ex_list);
	g_free (ex_path_list);
	g_free (iter);

	retv = g_string_free (tmp_string, FALSE); // return the content of the GString

	return retv;
}

// vim:ts=3:sw=3:tw=1024:cin
