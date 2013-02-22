/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2013 Pierre Wieser and others (see AUTHORS)
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

#ifndef __NACT_MAIN_STATUSBAR_H__
#define __NACT_MAIN_STATUSBAR_H__

/**
 * SECTION: nact_main_statusbar
 * @short_description: Main statusbar functions.
 * @include: nact/nact-main-statusbar.h
 */

#include "nact-main-window.h"

G_BEGIN_DECLS

void  nact_main_statusbar_initialize_gtk_toplevel( NactMainWindow *window );

void  nact_main_statusbar_display_status( NactMainWindow *window, const gchar *context, const gchar *status );
void  nact_main_statusbar_display_with_timeout( NactMainWindow *window, const gchar *context, const gchar *status );
void  nact_main_statusbar_hide_status( NactMainWindow *window, const gchar *context );
void  nact_main_statusbar_set_locked( NactMainWindow *window, gboolean readonly, gint reason );

G_END_DECLS

#endif /* __NACT_MAIN_STATUSBAR_H__ */
