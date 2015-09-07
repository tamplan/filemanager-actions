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

#ifndef __NACT_MAIN_WINDOW_DEF_H__
#define __NACT_MAIN_WINDOW_DEF_H__

/**
 * SECTION: main-window
 * @title: NactMainWindow
 * @short_description: The Main Window class definition
 * @include: nact-main-window.h
 *
 * This class is derived from GtkApplicationWindow and manages the main
 * window.
 *
 * It embeds:
 * - the menubar,
 * - the toolbar,
 * - a button bar with sort buttons,
 * - the hierarchical list of items,
 * - a notebook which displays the content of the current item in
 *   several tabs,
 * - a status bar with some indicators.
 *
 * [main]
 *  set_log_handler()
 *  nact_application_new()
 *  nact_application_run_with_args()
 *  |
 *  +-> [NactApplication]
 *  |    on_startup()
 *  |      |
 *  |      +-> na_updater_new()
 *  |          nact_menu_app()
 *  |
 *  |    on_activate()
 *  |      |
 *  |      +-> nact_main_window_new()
 *  |           |
 *  |           +-> setup_main_ui()
 *  |               setup_treeview()
 *  |               nact_clipboard_new()
 *  |               nact_sort_buttons_new()
 *  |
 *  |               nact_iaction_tab_init()
 *  |               nact_icommand_tab_init()
 *  |               nact_ibasenames_tab_init()
 *  |               nact_imimetypes_tab_init()
 *  |               nact_ifolders_tab_init()
 *  |               nact_ischemes_tab_init()
 *  |               nact_icapabilities_tab_init()
 *  |               nact_ienvironment_tab_init()
 *  |               nact_iexecution_tab_init()
 *  |               nact_iproperties_tab_init()
 *  |
 *  |               setup_monitor_pivot()
 *  |               setup_delete_event()
 *  |               nact_menu_win()
 *  |               load_items()
 *  v
 * [X] End of initialization process
 */

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define NACT_TYPE_MAIN_WINDOW                ( nact_main_window_get_type())
#define NACT_MAIN_WINDOW( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_TYPE_MAIN_WINDOW, NactMainWindow ))
#define NACT_MAIN_WINDOW_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, NACT_TYPE_MAIN_WINDOW, NactMainWindowClass ))
#define NACT_IS_MAIN_WINDOW( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_TYPE_MAIN_WINDOW ))
#define NACT_IS_MAIN_WINDOW_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), NACT_TYPE_MAIN_WINDOW ))
#define NACT_MAIN_WINDOW_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), NACT_TYPE_MAIN_WINDOW, NactMainWindowClass ))

typedef struct _NactMainWindowPrivate        NactMainWindowPrivate;

 typedef struct {
 	/*< private >*/
 	GtkApplicationWindowClass parent;
 }
 	NactMainWindowClass;

typedef struct {
	/*< private >*/
	GtkApplicationWindow      parent;
	NactMainWindowPrivate    *private;
}
	NactMainWindow;

GType nact_main_window_get_type( void );

G_END_DECLS

#endif /* __NACT_MAIN_WINDOW_DEF_H__ */
