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

#ifndef __UI_BASE_KEYSYMS_H__
#define __UI_BASE_KEYSYMS_H__

/**
 * SECTION: base_keysyms
 * @short_description: Keys declarations.
 * @include: ui/base-keysyms.h
 */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

G_BEGIN_DECLS

#define NACT_KEY_Escape    (GDK_KEY_Escape)
#define NACT_KEY_Insert    (GDK_KEY_Insert)
#define NACT_KEY_Delete    (GDK_KEY_Delete)
#define NACT_KEY_Return    (GDK_KEY_Return)
#define NACT_KEY_KP_Delete (GDK_KEY_KP_Delete)
#define NACT_KEY_KP_Enter  (GDK_KEY_KP_Enter)
#define NACT_KEY_KP_Insert (GDK_KEY_KP_Insert)
#define NACT_KEY_Left      (GDK_KEY_Left)
#define NACT_KEY_Right     (GDK_KEY_Right)
#define NACT_KEY_F2        (GDK_KEY_F2)

G_END_DECLS

#endif /* __UI_BASE_KEYSYMS_H__ */
