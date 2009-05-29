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

#ifndef __NACT_EDITOR_H__
#define __NACT_EDITOR_H__

#include <gtk/gtk.h>
#include <common/nautilus-actions-config.h>

enum {
	PROFILE_LABEL_COLUMN = 0,
	PROFILE_DESC_LABEL_COLUMN,
	N_PROF_COLUMN
};

gboolean nact_editor_new_action( void );
gboolean nact_editor_edit_action( NautilusActionsConfigAction* action );

void     nact_fill_menu_icon_combo_list_of( GtkComboBoxEntry* combo );
void     nact_preview_icon_changed_cb( GtkEntry* icon_entry, gpointer user_data, const gchar *dialog );
void     nact_icon_browse_button_clicked_cb( GtkButton *button, gpointer user_data, const gchar* dialog );

#endif /* __NACT_EDITOR_H__ */
