/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2014 Pierre Wieser and others (see AUTHORS)
 *
 * Nautilus-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Nautilus-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nautilus-Actions; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Frederic Ruaudel <grumz@grumz.net>
 *   Rodrigo Moya <rodrigo@gnome-db.org>
 *   Pierre Wieser <pwieser@trychlos.org>
 *   ... and many others (see AUTHORS)
 */

#ifndef __BASE_KEYSYMS_H__
#define __BASE_KEYSYMS_H__

/**
 * SECTION: base_window
 * @short_description: #BaseWindow public function declaration.
 * @include: nact/base-window.h
 *
 * This is a base class which encapsulates a Gtk+ windows.
 * It works together with the BaseApplication class to run a Gtk+
 * application.
 */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

G_BEGIN_DECLS

/* GDK_KEY_ defines have been defined since Gtk+ 2.21.8 released on 2010-09-14
 * see http://git.gnome.org/browse/gtk+/commit/?id=750c81f43dda6c783372b983e630ecd30b776d7e
 */
#if GTK_CHECK_VERSION( 2, 21, 8 )

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

#else

#define NACT_KEY_Escape    (GDK_Escape)
#define NACT_KEY_Insert    (GDK_Insert)
#define NACT_KEY_Delete    (GDK_Delete)
#define NACT_KEY_Return    (GDK_Return)
#define NACT_KEY_KP_Delete (GDK_KP_Delete)
#define NACT_KEY_KP_Enter  (GDK_KP_Enter)
#define NACT_KEY_KP_Insert (GDK_KP_Insert)
#define NACT_KEY_Left      (GDK_Left)
#define NACT_KEY_Right     (GDK_Right)
#define NACT_KEY_F2        (GDK_F2)

#endif

G_END_DECLS

#endif /* __BASE_KEYSYMS_H__ */
