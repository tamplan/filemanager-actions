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

#ifndef __NACT_MAIN_TAB_H__
#define __NACT_MAIN_TAB_H__

#include <api/fma-icontext.h>

#include "nact-main-window-def.h"

/**
 * SECTION: nact_main_tab
 * @short_description: Signals and properties.
 * @include: nact/nact-main-tab.h
 *
 * Here as defined signals and properties common to all tabs.
 */

/* notebook tabs
 */
enum {
	TAB_ACTION = 0,
	TAB_COMMAND,
	TAB_EXECUTION,
	TAB_BASENAMES,
	TAB_MIMETYPES,
	TAB_FOLDERS,
	TAB_SCHEMES,
	TAB_CAPABILITIES,
	TAB_ENVIRONMENT,
	TAB_PROPERTIES
};

void     nact_main_tab_init           ( NactMainWindow *window, gint page );
void     nact_main_tab_enable_page    ( NactMainWindow *window, gint page, gboolean enabled );
gboolean nact_main_tab_is_page_enabled( NactMainWindow *window, gint page );

G_END_DECLS

#endif /* __NACT_MAIN_TAB_H__ */
