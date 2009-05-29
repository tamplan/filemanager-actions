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

#ifndef __NACT_PROFILE_EDITOR_H__
#define __NACT_PROFILE_EDITOR_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <common/nautilus-actions-config.h>
#include <common/nautilus-actions-config-gconf-writer.h>

gboolean nact_profile_editor_new_profile (NautilusActionsConfigAction* action);
gboolean nact_profile_editor_edit_profile (NautilusActionsConfigAction *action, gchar* profile_name, NautilusActionsConfigActionProfile* action_profile);

/* a structure to be passed as user data to schemes callbacks */
typedef struct {
	gchar  *dialog;
	void ( *field_changed_cb )( GObject *object, gpointer user_data );
	void ( *update_example_label )( void );
	GtkWidget *listview;
}
	NactCallbackData;

void     nact_add_scheme_clicked( GtkWidget* widget, gpointer user_data, const gchar *dialog );
void     nact_create_schemes_selection_list( NactCallbackData *scheme_data );
GSList * nact_get_action_match_string_list (const gchar* patterns, const gchar* default_string);
void     nact_hide_legend_dialog( const gchar *dialog );
void     nact_path_browse_button_clicked_cb( GtkButton *button, gpointer user_data, const gchar *dialog );
void     nact_remove_scheme_clicked( GtkWidget* widget, gpointer user_data, const gchar *dialog );
gboolean nact_reset_schemes_list (GtkTreeModel* scheme_model, GtkTreePath *path, GtkTreeIter* iter, gpointer data);
void     nact_set_action_match_string_list (GtkEntry* entry, GSList* basenames, const gchar* default_string);
void     nact_set_action_schemes (gchar* action_scheme, GtkTreeModel* scheme_model);
void     nact_show_legend_dialog( const gchar *dialog );
void     nact_update_example_label( const gchar *dialog );

#endif /* __NACT_PROFILE_EDITOR_H__ */
