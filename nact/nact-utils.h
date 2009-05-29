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

#ifndef __NACT_UTILS_H__
#define __NACT_UTILS_H__

#include <glade/glade-xml.h>
#include <gtk/gtk.h>

#define GLADE_MAIN_WIDGET "ActionsDialog"
#define GLADE_EDIT_DIALOG_WIDGET "EditActionDialog"
#define GLADE_EDIT_ACTION_DIALOG_WIDGET "EditActionDialogExt"
#define GLADE_EDIT_PROFILE_DIALOG_WIDGET "EditProfileDialog"
#define GLADE_LEGEND_DIALOG_WIDGET "LegendDialog"
#define GLADE_FILECHOOSER_DIALOG_WIDGET "FileChooserDialog"
#define GLADE_FOLDERCHOOSER_DIALOG_WIDGET "FolderChooserDialog"
#define GLADE_IM_EX_PORT_DIALOG_WIDGET "ImportExportDialog"
#define GLADE_ERROR_DIALOG_WIDGET "ErrorDialog"
#define GLADE_ABOUT_DIALOG_WIDGET "AboutDialog"

enum {
	SCHEMES_CHECKBOX_COLUMN = 0,
	SCHEMES_KEYWORD_COLUMN,
	SCHEMES_DESC_COLUMN,
	SCHEMES_N_COLUMN
};

GladeXML   *nact_get_glade_xml_object (const gchar* root_widget);
GtkWidget  *nact_get_glade_widget_from (const gchar* widget_name, const gchar* root_widget);
GtkWidget  *nact_get_glade_widget (const gchar* widget_name);
GList      *nact_get_glade_widget_prefix_from (const gchar* widget_name, const gchar* root_widget);
GList      *nact_get_glade_widget_prefix (const gchar* widget_name);
void        nact_destroy_glade_objects (void);
void        nautilus_actions_display_error (const gchar *primary_msg, const gchar *secondary_msg);
gboolean    nact_utils_get_action_schemes_list (GtkTreeModel* scheme_model, GtkTreePath *path,
													  GtkTreeIter* iter, gpointer data);
gchar      *nact_utils_parse_parameter( const gchar *dialog );

#endif /* __NACT_UTILS_H__ */
