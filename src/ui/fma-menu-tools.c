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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "fma-assistant-export.h"
#include "fma-assistant-import.h"
#include "fma-menu.h"
#include "fma-menu-tools.h"

/**
 * fma_menu_tools_update_sensitivities:
 * @main_window: the #FMAMainWindow main window.
 *
 * Update sensitivities on the Tools menu.
 */
void
fma_menu_tools_update_sensitivities( FMAMainWindow *main_window )
{
	sMenuData *sdata;

	sdata = fma_menu_get_data( main_window );

	/* import item enabled if at least one writable provider */
	fma_menu_enable_item( main_window, "import", sdata->has_writable_providers );

	/* export item enabled if IActionsList store contains actions */
	fma_menu_enable_item( main_window, "export", sdata->have_exportables );
}

/**
 * fma_menu_tools_import:
 * @main_window: the #FMAMainWindow main window.
 *
 * Triggers the "Tools/Import assitant" item.
 */
void
fma_menu_tools_import( FMAMainWindow *main_window )
{
	fma_assistant_import_run( main_window );
}

/**
 * fma_menu_tools_export:
 * @main_window: the #FMAMainWindow main window.
 *
 * Triggers the "Tools/Export assistant" item.
 */
void
fma_menu_tools_export( FMAMainWindow *main_window )
{
	fma_assistant_export_run( main_window );
}

