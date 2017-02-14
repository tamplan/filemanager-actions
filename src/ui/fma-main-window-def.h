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

#ifndef __UI_FMA_MAIN_WINDOW_DEF_H__
#define __UI_FMA_MAIN_WINDOW_DEF_H__

/**
 * SECTION: main-window
 * @title: FMAMainWindow
 * @short_description: The Main Window class definition
 * @include: ui/fma-main-window.h
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
 *  fma_application_new()
 *  fma_application_run_with_args()
 *  |
 *  +-> [FMAApplication]
 *  |    on_startup()
 *  |      |
 *  |      +-> fma_updater_new()
 *  |          fma_menu_app()
 *  |
 *  |    on_activate()
 *  |      |
 *  |      +-> fma_main_window_new()
 *  |           |
 *  |           +-> setup_main_ui()
 *  |               setup_treeview()
 *  |               fma_clipboard_new()
 *  |               fma_sort_buttons_new()
 *  |
 *  |               fma_iaction_tab_init()
 *  |               fma_icommand_tab_init()
 *  |               fma_ibasenames_tab_init()
 *  |               fma_imimetypes_tab_init()
 *  |               fma_ifolders_tab_init()
 *  |               fma_ischemes_tab_init()
 *  |               fma_icapabilities_tab_init()
 *  |               fma_ienvironment_tab_init()
 *  |               fma_iexecution_tab_init()
 *  |               fma_iproperties_tab_init()
 *  |
 *  |               setup_monitor_pivot()
 *  |               setup_delete_event()
 *  |               fma_menu_win()
 *  |               load_items()
 *  v
 * [X] End of initialization process
 */

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define FMA_TYPE_MAIN_WINDOW                ( fma_main_window_get_type())
#define FMA_MAIN_WINDOW( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_MAIN_WINDOW, FMAMainWindow ))
#define FMA_MAIN_WINDOW_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_MAIN_WINDOW, FMAMainWindowClass ))
#define FMA_IS_MAIN_WINDOW( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_MAIN_WINDOW ))
#define FMA_IS_MAIN_WINDOW_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_MAIN_WINDOW ))
#define FMA_MAIN_WINDOW_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_MAIN_WINDOW, FMAMainWindowClass ))

typedef struct _FMAMainWindowPrivate        FMAMainWindowPrivate;

 typedef struct {
 	/*< private >*/
 	GtkApplicationWindowClass parent;
 }
 	FMAMainWindowClass;

typedef struct {
	/*< private >*/
	GtkApplicationWindow      parent;
	FMAMainWindowPrivate     *private;
}
	FMAMainWindow;

GType fma_main_window_get_type( void );

G_END_DECLS

#endif /* __UI_FMA_MAIN_WINDOW_DEF_H__ */
