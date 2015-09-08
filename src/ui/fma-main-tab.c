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

#include "api/fma-object-profile.h"

#include "core/fma-gtk-utils.h"

#include "fma-main-tab.h"
#include "nact-main-window.h"

/**
 * fma_main_tab_init:
 * @window: the #NactMainWindow.
 * @num_page: the page number, starting from zero.
 *
 * Common initialization of each page of the notebook.
 * (provided that the page has itself called fma_main_tab_init())
 */
void
fma_main_tab_init( NactMainWindow *main_window, gint num_page )
{
	GtkWidget *notebook;
	GtkWidget *page;
	const gchar *text;

	/* popup menu is enabled in NactMainWindow::setup_main_ui()
	 * but the displayed labels default to be those of the tab, i.e. embed
	 * an underscore as an accelerator - so get rid of this
	 */
	notebook = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( main_window ), "main-notebook" );
	g_return_if_fail( notebook && GTK_IS_NOTEBOOK( notebook ));
	page = gtk_notebook_get_nth_page( GTK_NOTEBOOK( notebook ), num_page );
	text = gtk_notebook_get_tab_label_text( GTK_NOTEBOOK( notebook ), page );
	gtk_notebook_set_menu_label_text( GTK_NOTEBOOK( notebook ), page, text );
}

/**
 * fma_main_tab_enable_page:
 * @window: the #NactMainWindow.
 * @num_page: the page number, starting from zero.
 * @enabled: whether the tab should be set sensitive or not.
 *
 * Set the sensitivity of the tab.
 */
void
fma_main_tab_enable_page( NactMainWindow *window, gint num_page, gboolean enabled )
{
	GtkWidget *notebook;
	GtkWidget *page, *label;

	notebook = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( window ), "main-notebook" );
	g_return_if_fail( notebook && GTK_IS_NOTEBOOK( notebook ));
	page = gtk_notebook_get_nth_page( GTK_NOTEBOOK( notebook ), num_page );
	gtk_widget_set_sensitive( page, enabled );

	label = gtk_notebook_get_tab_label( GTK_NOTEBOOK( notebook ), page );
	gtk_widget_set_sensitive( label, enabled );
}

/**
 * fma_main_tab_is_page_enabled:
 * @window: the #NactMainWindow.
 * @num_page: the page number, starting from zero.
 *
 * Returns: %TRUE if the tab is sensitive, %FALSE else.
 */
gboolean
fma_main_tab_is_page_enabled( NactMainWindow *window, gint num_page )
{
	gboolean is_sensitive;
	GtkWidget *notebook, *page;

	notebook = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( window ), "main-notebook" );
	g_return_val_if_fail( notebook && GTK_IS_NOTEBOOK( notebook ), FALSE );
	page = gtk_notebook_get_nth_page( GTK_NOTEBOOK( notebook ), num_page );
	is_sensitive = gtk_widget_is_sensitive( page );

	g_debug( "fma_main_tab_is_page_enabled: num_page=%d, is_sensitive=%s", num_page, is_sensitive ? "True":"False" );

	return( is_sensitive );
}
