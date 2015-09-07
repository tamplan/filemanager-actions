/*
 * FileManager-Actions
 * A file-manager extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
 *
 * FileManager-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * FileManager-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FileManager-Actions; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Frederic Ruaudel <grumz@grumz.net>
 *   Rodrigo Moya <rodrigo@gnome-db.org>
 *   Pierre Wieser <pwieser@trychlos.org>
 *   ... and many others (see AUTHORS)
 */

#ifndef __CORE_FMA_GTK_UTILS_H__
#define __CORE_FMA_GTK_UTILS_H__

/* @title: GTK+
 * @short_description: The Gtk+ Library Utilities.
 * @include: core/fma-gtk-utils.h
 */

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* widget hierarchy
 */
GtkWidget *fma_gtk_utils_find_widget_by_name   ( GtkContainer *container,
													const gchar *name );
void       fma_gtk_utils_connect_widget_by_name( GtkContainer *container,
													const gchar *name,
													const gchar *signal,
													GCallback cb,
													void *user_data );

#ifdef NA_MAINTAINER_MODE
void       fma_gtk_utils_dump_children          ( GtkContainer *container );
#endif

/* window size and position
 */
void       fma_gtk_utils_restore_window_position( GtkWindow *toplevel, const gchar *wsp_name );
void       fma_gtk_utils_save_window_position   ( GtkWindow *toplevel, const gchar *wsp_name );

/* widget status
 */
void       fma_gtk_utils_set_editable( GObject *widget, gboolean editable );

void       fma_gtk_utils_radio_set_initial_state  ( GtkRadioButton *button,
				GCallback toggled_handler, void *user_data,
				gboolean editable, gboolean sensitive );

void       fma_gtk_utils_radio_reset_initial_state( GtkRadioButton *button, GCallback toggled_handler );

/* default height of a panel bar (dirty hack!)
 */
#define DEFAULT_HEIGHT		22

#define FMA_TOGGLE_DATA_EDITABLE		"fma-toggle-data-editable"
#define FMA_TOGGLE_DATA_BUTTON			"fma-toggle-data-button"
#define FMA_TOGGLE_DATA_HANDLER			"fma-toggle-data-handler"
#define FMA_TOGGLE_DATA_USER_DATA		"fma-toggle-data-user-data"

G_END_DECLS

#endif /* __CORE_FMA_GTK_UTILS_H__ */
