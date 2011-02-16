/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011 Pierre Wieser and others (see AUTHORS)
 *
 * This Program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This Program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this Library; see the file COPYING.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330, Boston, MA 02111-1307, USA.
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

#include <core/na-iabout.h>

#include "nact-menubar-priv.h"

/**
 * nact_menubar_help_on_update_sensitivities:
 * @bar: this #NactMenubar object.
 *
 * Update sensitivities on the Help menu.
 */
void
nact_menubar_help_on_update_sensitivities( const NactMenubar *bar )
{
	nact_menubar_enable_item( bar, "HelpItem", TRUE );
	/* about always enabled */
}

/**
 * nact_menubar_help_on_help:
 * @action: the #GtkAction of the item.
 * @window: the #BaseWindow main application window.
 *
 * Triggers the "Help/Help" item.
 */
void
nact_menubar_help_on_help( GtkAction *action, BaseWindow *window )
{
	static const gchar *thisfn = "nact_menubar_help_on_help";
	GError *error;

	error = NULL;
	gtk_show_uri( NULL, "ghelp:nautilus-actions-config-tool", GDK_CURRENT_TIME, &error );
	if( error ){
		g_warning( "%s: %s", thisfn, error->message );
		g_error_free( error );
	}
}

/**
 * nact_menubar_help_on_about:
 * @action: the #GtkAction of the item.
 * @window: the #BaseWindow main application window.
 *
 * Triggers the "Help/About" item.
 */
void
nact_menubar_help_on_about( GtkAction *action, BaseWindow *window )
{
	na_iabout_display( NA_IABOUT( window ));
}