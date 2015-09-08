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

#ifndef __UI_NACT_MENU_EDIT_H__
#define __UI_NACT_MENU_EDIT_H__

#include "nact-main-window-def.h"

G_BEGIN_DECLS

void nact_menu_edit_update_sensitivities( NactMainWindow *main_window );

void nact_menu_edit_cut                 ( NactMainWindow *main_window );

void nact_menu_edit_copy                ( NactMainWindow *main_window );

void nact_menu_edit_paste               ( NactMainWindow *main_window );

void nact_menu_edit_paste_into          ( NactMainWindow *main_window );

void nact_menu_edit_duplicate           ( NactMainWindow *main_window );

void nact_menu_edit_delete              ( NactMainWindow *main_window );

G_END_DECLS

#endif /* __UI_NACT_MENU_EDIT_H__ */
