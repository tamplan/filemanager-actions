/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011 Pierre Wieser and others (see AUTHORS)
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

#ifndef __BASE_GTK_UTILS_H__
#define __BASE_GTK_UTILS_H__

/**
 * SECTION: base-gtk-utils
 * @title: BaseGtkUtils
 * @short_description: Gtk helper functions
 * @include: base-gtk-utils.h
 */

#include "base-window.h"

G_BEGIN_DECLS

/* window size and position
 */
void       base_gtk_utils_restore_window_position( const BaseWindow *window, const gchar *wsp_name );
void       base_gtk_utils_save_window_position   ( const BaseWindow *window, const gchar *wsp_name );

#define NACT_PROP_TOGGLE_EDITABLE			"nact-prop-toggle-editable"

void       base_gtk_utils_set_editable( GObject *widget, gboolean editable );

void       base_gtk_utils_radio_set_initial_state  ( GtkRadioButton *button,
				GCallback toggled_handler, void *user_data,
				gboolean editable, gboolean sensitive );

void       base_gtk_utils_radio_reset_initial_state( GtkRadioButton *button, GCallback toggled_handler );

void       base_gtk_utils_toggle_set_initial_state ( BaseWindow *window,
				const gchar *button_name, GCallback toggled_handler,
				gboolean active, gboolean editable, gboolean sensitive );

void       base_gtk_utils_toggle_reset_initial_state( GtkToggleButton *button );

/* image utilities
 */
GdkPixbuf *base_gtk_utils_get_pixbuf( const gchar *name, GtkWidget *widget, GtkIconSize size );
void       base_gtk_utils_render( const gchar *name, GtkImage *widget, GtkIconSize size );

/* standard dialog boxes
 */
void       base_gtk_utils_select_file( BaseWindow *window,
				const gchar *title, const gchar *wsp_name,
				GtkWidget *entry, const gchar *entry_name );

void       base_gtk_utils_select_file_with_preview( BaseWindow *window,
				const gchar *title, const gchar *wsp_name,
				GtkWidget *entry, const gchar *entry_name,
				GCallback update_preview_cb );

void       base_gtk_utils_select_dir( BaseWindow *window,
				const gchar *title, const gchar *wsp_name,
				GtkWidget *entry, const gchar *entry_name );

/* GtkWidget
 */
GtkWidget *base_gtk_utils_get_widget_by_name( GtkWindow *toplevel, const gchar *name );

G_END_DECLS

#endif /* __BASE_GTK_UTILS_H__ */
