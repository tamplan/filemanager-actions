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

#ifndef _NACT_UTILS_H_
#define _NACT_UTILS_H_

#include <glade/glade-xml.h>
#include <gtk/gtk.h>

#define GLADE_MAIN_WIDGET "ActionsDialog"
#define GLADE_EDIT_DIALOG_WIDGET "EditActionDialog"
#define GLADE_LEGEND_DIALOG_WIDGET "LegendDialog"
#define GLADE_FILECHOOSER_DIALOG_WIDGET "FileChooserDialog"

enum {
	SCHEMES_CHECKBOX_COLUMN = 0,
	SCHEMES_KEYWORD_COLUMN,
	SCHEMES_DESC_COLUMN,
	SCHEMES_N_COLUMN
};

GladeXML* nact_get_glade_xml_object (const gchar* root_widget);
GtkWidget* nact_get_glade_widget_from (const gchar* widget_name, const gchar* root_widget);
GtkWidget* nact_get_glade_widget (const gchar* widget_name);
GList* nact_get_glade_widget_prefix_from (const gchar* widget_name, const gchar* root_widget);
GList* nact_get_glade_widget_prefix (const gchar* widget_name);
void nact_destroy_glade_objects (void);
gboolean nact_utils_get_action_schemes_list (GtkTreeModel* scheme_model, GtkTreePath *path, 
													  GtkTreeIter* iter, gpointer data);
gchar* nact_utils_parse_parameter (void);


#endif

// vim:ts=3:sw=3:tw=1024:cin
