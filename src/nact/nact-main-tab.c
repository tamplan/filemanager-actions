/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011, 2012 Pierre Wieser and others (see AUTHORS)
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

#include <api/na-object-profile.h>

#include "nact-main-tab.h"

static gboolean on_button_press_event( GtkWidget *widget, GdkEventButton *event, NactMainWindow *window );
static void     open_popup( NactMainWindow *window, GdkEventButton *event );

/**
 * nact_main_tab_init:
 * @window: the #NactMainWindow.
 * @num_page: the page number, starting from zero.
 *
 * Common initialization of each page of the notebook.
 */
void
nact_main_tab_init( NactMainWindow *window, gint num_page )
{
	GtkNotebook *notebook;
	GtkWidget *page, *label;

	notebook = GTK_NOTEBOOK( base_window_get_widget( BASE_WINDOW( window ), "MainNotebook" ));
	page = gtk_notebook_get_nth_page( notebook, num_page );
	label = gtk_notebook_get_tab_label( notebook, page );

	base_window_signal_connect(
			BASE_WINDOW( window ),
			G_OBJECT( label ),
			"button-press-event",
			G_CALLBACK( on_button_press_event ));
}

/**
 * nact_main_tab_enable_page:
 * @window: the #NactMainWindow.
 * @num_page: the page number, starting from zero.
 * @enabled: whether the tab should be set sensitive or not.
 *
 * Set the sensitivity of the tab.
 */
void
nact_main_tab_enable_page( NactMainWindow *window, gint num_page, gboolean enabled )
{
	GtkNotebook *notebook;
	GtkWidget *page, *label;

	notebook = GTK_NOTEBOOK( base_window_get_widget( BASE_WINDOW( window ), "MainNotebook" ));
	page = gtk_notebook_get_nth_page( notebook, num_page );
	gtk_widget_set_sensitive( page, enabled );

	label = gtk_notebook_get_tab_label( notebook, page );
	gtk_widget_set_sensitive( label, enabled );
}

/**
 * nact_main_tab_is_page_enabled:
 * @window: the #NactMainWindow.
 * @num_page: the page number, starting from zero.
 *
 * Returns: %TRUE if the tab is sensitive, %FALSE else.
 */
gboolean
nact_main_tab_is_page_enabled( NactMainWindow *window, gint num_page )
{
	gboolean is_sensitive;
	GtkNotebook *notebook;
	GtkWidget *page;

	notebook = GTK_NOTEBOOK( base_window_get_widget( BASE_WINDOW( window ), "MainNotebook" ));
	page = gtk_notebook_get_nth_page( notebook, num_page );

	is_sensitive = gtk_widget_is_sensitive( page );

	g_debug( "nact_main_tab_is_page_enabled: num_page=%d, is_sensitive=%s", num_page, is_sensitive ? "True":"False" );

	return( is_sensitive );
}

static gboolean
on_button_press_event( GtkWidget *widget, GdkEventButton *event, NactMainWindow *window )
{
	gboolean stop = FALSE;

	/* single click on right button */
	if( event->type == GDK_BUTTON_PRESS && event->button == 3 ){
		open_popup( window, event );
		stop = TRUE;
	}

	return( stop );
}

static void
open_popup( NactMainWindow *window, GdkEventButton *event )
{
#if 0
	NactTreeView *items_view;
	GtkTreePath *path;

	items_view = NACT_TREE_VIEW( g_object_get_data( G_OBJECT( window ), WINDOW_DATA_TREE_VIEW ));

	if( gtk_tree_view_get_path_at_pos( items_view->private->tree_view, event->x, event->y, &path, NULL, NULL, NULL )){
		nact_tree_view_select_row_at_path( items_view, path );
		gtk_tree_path_free( path );
	}

	g_signal_emit_by_name( window, TREE_SIGNAL_CONTEXT_MENU, event );
#endif
}
