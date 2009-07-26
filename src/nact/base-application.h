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

#ifndef __BASE_APPLICATION_H__
#define __BASE_APPLICATION_H__

/*
 * BaseApplication class definition.
 *
 * This is a base class for Gtk+ programs.
 */

#include <gtk/gtk.h>

#include "base-application-class.h"
#include "base-window-class.h"

G_BEGIN_DECLS

/* instance properties
 */
#define PROP_APPLICATION_ARGC_STR				"base-application-argc"
#define PROP_APPLICATION_ARGV_STR				"base-application-argv"
#define PROP_APPLICATION_UNIQUE_NAME_STR		"base-application-unique-name"
#define PROP_APPLICATION_UNIQUE_APP_STR			"base-application-unique-app"
#define PROP_APPLICATION_NAME_STR				"base-application-application-name"
#define PROP_APPLICATION_ICON_NAME_STR			"base-application-icon-name"
#define PROP_APPLICATION_CODE_STR				"base-application-code"
#define PROP_APPLICATION_UI_XML_STR				"base-application-ui-xml"
#define PROP_APPLICATION_UI_FILENAME_STR		"base-application-ui-filename"
#define PROP_APPLICATION_MAIN_WINDOW_STR		"base-application-main-window"

int         base_application_run( BaseApplication *application );

gchar      *base_application_get_ui_filename( BaseApplication *application );
gchar      *base_application_get_name( BaseApplication *application );
gchar      *base_application_get_icon_name( BaseApplication *application );
BaseWindow *base_application_get_main_window( BaseApplication *application );

GtkWindow  *base_application_get_dialog( BaseApplication *application, const gchar *name );
GtkWidget  *base_application_get_widget( BaseApplication *application, BaseWindow *window, const gchar *name );
GtkWidget  *base_application_search_for_widget( BaseApplication *application, GtkWindow *window, const gchar *name );

void        base_application_error_dlg( BaseApplication *application, GtkMessageType type, const gchar *primary, const gchar *secondary );
gboolean    base_application_yesno_dlg( BaseApplication *application, GtkMessageType type, const gchar *first, const gchar *second );

G_END_DECLS

#endif /* __BASE_APPLICATION_H__ */
