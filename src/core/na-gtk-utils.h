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

#ifndef __CORE_API_NA_GTK_UTILS_H__
#define __CORE_API_NA_GTK_UTILS_H__

/* @title: GTK+
 * @short_description: The Gtk+ Library Utilities.
 * @include: core/na-gtk-utils.h
 */

#include <gtk/gtk.h>

G_BEGIN_DECLS

GtkWidget *na_gtk_utils_find_widget_by_type    ( GtkContainer *container, GType type );
GtkWidget *na_gtk_utils_search_for_child_widget( GtkContainer *container, const gchar *name );

#ifdef NA_MAINTAINER_MODE
void       na_gtk_utils_dump_children          ( GtkContainer *container );
#endif

/* window size and position
 */
void       na_gtk_utils_restore_window_position( GtkWindow *toplevel, const gchar *wsp_name );
void       na_gtk_utils_save_window_position   ( GtkWindow *toplevel, const gchar *wsp_name );

/* default height of a panel bar (dirty hack!)
 */
#define DEFAULT_HEIGHT		22

G_END_DECLS

#endif /* __CORE_API_NA_GTK_UTILS_H__ */
