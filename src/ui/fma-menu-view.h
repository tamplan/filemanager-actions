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

#ifndef __UI_FMA_MENU_VIEW_H__
#define __UI_FMA_MENU_VIEW_H__

#include "fma-main-window-def.h"

G_BEGIN_DECLS

enum {
	MAIN_TOOLBAR_FILE_ID = 1,
	MAIN_TOOLBAR_EDIT_ID,
	MAIN_TOOLBAR_TOOLS_ID,
	MAIN_TOOLBAR_HELP_ID,
};

void fma_menu_view_init                ( FMAMainWindow *main_window );

void fma_menu_view_update_sensitivities( FMAMainWindow *main_window );

void fma_menu_view_toolbar_display     ( FMAMainWindow *main_window,
												const gchar *action_name,
												gboolean visible );

void fma_menu_view_notebook_tab_display( FMAMainWindow *main_window,
												const gchar *action_name,
												const gchar *target );

void fma_menu_view_set_notebook_label  ( FMAMainWindow *main_window );

G_END_DECLS

#endif /* __UI_FMA_MENU_VIEW_H__ */
