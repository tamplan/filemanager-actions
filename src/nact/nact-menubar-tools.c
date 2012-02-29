/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2012 Pierre Wieser and others (see AUTHORS)
 *
 * Nautilus-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General  Public  License  as
 * published by the Free Software Foundation; either  version  2  of
 * the License, or (at your option) any later version.
 *
 * Nautilus-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even  the  implied  warranty  of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See  the  GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public  License
 * along with Nautilus-Actions; see the file  COPYING.  If  not,  see
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

#include "nact-assistant-export.h"
#include "nact-assistant-import.h"
#include "nact-menubar-priv.h"

/**
 * nact_menubar_tools_on_update_sensitivities:
 * @bar: this #NactMenubar object.
 *
 * Update sensitivities on the Tools menu.
 */
void
nact_menubar_tools_on_update_sensitivities( const NactMenubar *bar )
{
	/* import item enabled if at least one writable provider */
	nact_menubar_enable_item( bar, "ImportItem", bar->private->has_writable_providers );

	/* export item enabled if IActionsList store contains actions */
	nact_menubar_enable_item( bar, "ExportItem", bar->private->have_exportables );
}

/**
 * nact_menubar_tools_on_import:
 * @action: the #GtkAction of the item.
 * @window: the #BaseWindow main application window.
 *
 * Triggers the "Tools/Import assitant" item.
 */
void
nact_menubar_tools_on_import( GtkAction *action, BaseWindow *window )
{
	nact_assistant_import_run( BASE_WINDOW( window ));
}

/**
 * nact_menubar_tools_on_export:
 * @action: the #GtkAction of the item.
 * @window: the #BaseWindow main application window.
 *
 * Triggers the "Tools/Export assistant" item.
 */
void
nact_menubar_tools_on_export( GtkAction *action, BaseWindow *window )
{
	nact_assistant_export_run( BASE_WINDOW( window ));
}
