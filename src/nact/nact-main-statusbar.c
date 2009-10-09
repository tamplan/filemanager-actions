/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009 Pierre Wieser and others (see AUTHORS)
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

#include "nact-main-statusbar.h"

typedef struct {
	guint         event_source_id;
	guint         context_id;
	GtkStatusbar *bar;
}
	StatusbarTimeoutDisplayStruct;

static GtkStatusbar *get_statusbar( const NactMainWindow *window );
static gboolean      display_timeout( StatusbarTimeoutDisplayStruct *stds );
static void          display_timeout_free( StatusbarTimeoutDisplayStruct *stds );

void
nact_main_statusbar_display_status( NactMainWindow *window, const gchar *context, const gchar *status )
{
	GtkStatusbar *bar;

	if( !status || !g_utf8_strlen( status, -1 )){
		return;
	}

	bar = get_statusbar( window );

	if( bar ){
		guint context_id = gtk_statusbar_get_context_id( bar, context );
		gtk_statusbar_push( bar, context_id, status );
	}
}

/*
 * push a message
 * automatically pop it after a timeout
 * the timeout is not suspended when another message is pushed onto the
 * previous one
 */
void
nact_main_statusbar_display_with_timeout( NactMainWindow *window, const gchar *context, const gchar *status )
{
	GtkStatusbar *bar;
	StatusbarTimeoutDisplayStruct *stds;

	if( !status || !g_utf8_strlen( status, -1 )){
		return;
	}

	bar = get_statusbar( window );

	if( bar ){
		guint context_id = gtk_statusbar_get_context_id( bar, context );
		gtk_statusbar_push( bar, context_id, status );

		stds = g_new0( StatusbarTimeoutDisplayStruct, 1 );
		stds->context_id = context_id;
		stds->bar = bar;
		stds->event_source_id = g_timeout_add_seconds_full(
				G_PRIORITY_DEFAULT,
				10,
				( GSourceFunc ) display_timeout,
				stds,
				( GDestroyNotify ) display_timeout_free );
	}
}

void
nact_main_statusbar_hide_status( NactMainWindow *window, const gchar *context )
{
	GtkStatusbar *bar;

	bar = get_statusbar( window );

	if( bar ){
		guint context_id = gtk_statusbar_get_context_id( bar, context );
		gtk_statusbar_pop( bar, context_id );
	}
}

/*
 * Returns the status bar widget
 */
static GtkStatusbar *
get_statusbar( const NactMainWindow *window )
{
	GtkWidget *statusbar;

	statusbar = base_window_get_widget( BASE_WINDOW( window ), "StatusBar" );

	return( GTK_STATUSBAR( statusbar ));
}

static gboolean
display_timeout( StatusbarTimeoutDisplayStruct *stds )
{
	gboolean keep_source = FALSE;

	gtk_statusbar_pop( stds->bar, stds->context_id );

	return( keep_source );
}

static void
display_timeout_free( StatusbarTimeoutDisplayStruct *stds )
{
	g_free( stds );
}
