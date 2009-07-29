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

#ifndef __BASE_WINDOW_H__
#define __BASE_WINDOW_H__

/*
 * BaseWindow class definition.
 *
 * This is a base class which encapsulates a Gtk+ windows.
 * It works together with the BaseApplication class to run a Gtk+
 * application.
 */

#include <gtk/gtk.h>

#include "base-application-class.h"
#include "base-window-class.h"

G_BEGIN_DECLS

/* instance properties
 */
#define PROP_WINDOW_PARENT_STR				"base-window-parent"
#define PROP_WINDOW_APPLICATION_STR			"base-window-application"
#define PROP_WINDOW_TOPLEVEL_NAME_STR		"base-window-toplevel-name"
#define PROP_WINDOW_TOPLEVEL_DIALOG_STR		"base-window-toplevel-dialog"
#define PROP_WINDOW_INITIALIZED_STR			"base-window-is-initialized"

void             base_window_init( BaseWindow *window );
void             base_window_run( BaseWindow *window );

GtkWindow       *base_window_get_toplevel_dialog( BaseWindow *window );
BaseApplication *base_window_get_application( BaseWindow *window );
GtkWindow       *base_window_get_dialog( BaseWindow *window, const gchar *name );
GtkWidget       *base_window_get_widget( BaseWindow *window, const gchar *name );

void             base_window_error_dlg( BaseWindow *window, GtkMessageType type, const gchar *primary, const gchar *secondary );
gboolean         base_window_yesno_dlg( BaseWindow *window, GtkMessageType type, const gchar *first, const gchar *second );

G_END_DECLS

#endif /* __BASE_WINDOW_H__ */
