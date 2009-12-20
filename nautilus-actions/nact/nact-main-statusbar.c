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

#include <glib/gi18n.h>

#include "nact-main-statusbar.h"

typedef struct {
	guint         event_source_id;
	guint         context_id;
	GtkStatusbar *bar;
}
	StatusbarTimeoutDisplayStruct;

#define LOCKED_IMAGE					PKGDATADIR "/locked.png"

static GtkStatusbar *get_statusbar( const NactMainWindow *window );
static gboolean      display_timeout( StatusbarTimeoutDisplayStruct *stds );
static void          display_timeout_free( StatusbarTimeoutDisplayStruct *stds );

/**
 * nact_main_statusbar_display_status:
 * @window: the #NactMainWindow instance.
 * @context: the context to be displayed.
 * @status: the message.
 *
 * Push a message.
 */
void
nact_main_statusbar_display_status( NactMainWindow *window, const gchar *context, const gchar *status )
{
	static const gchar *thisfn = "nact_main_statusbar_display_status";
	GtkStatusbar *bar;

	g_debug( "%s: window=%p, context=%s, status=%s", thisfn, ( void * ) window, context, status );

	if( !status || !g_utf8_strlen( status, -1 )){
		return;
	}

	bar = get_statusbar( window );

	if( bar ){
		guint context_id = gtk_statusbar_get_context_id( bar, context );
		gtk_statusbar_push( bar, context_id, status );
	}
}

/**
 * nact_main_statusbar_display_with_timeout:
 * @window: the #NactMainWindow instance.
 * @context: the context to be displayed.
 * @status: the message.
 *
 * Push a message.
 * Automatically pop it after a timeout.
 * The timeout is not suspended when another message is pushed onto the
 * previous one.
 */
void
nact_main_statusbar_display_with_timeout( NactMainWindow *window, const gchar *context, const gchar *status )
{
	static const gchar *thisfn = "nact_main_statusbar_display_with_timeout";
	GtkStatusbar *bar;
	StatusbarTimeoutDisplayStruct *stds;

	g_debug( "%s: window=%p, context=%s, status=%s", thisfn, ( void * ) window, context, status );

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

/**
 * nact_main_statusbar_hide_status:
 * @window: the #NactMainWindow instance.
 * @context: the context to be hidden.
 *
 * Hide the specified context.
 */
void
nact_main_statusbar_hide_status( NactMainWindow *window, const gchar *context )
{
	static const gchar *thisfn = "nact_main_statusbar_hide_status";
	GtkStatusbar *bar;

	g_debug( "%s: window=%p, context=%s", thisfn, ( void * ) window, context );

	bar = get_statusbar( window );

	if( bar ){
		guint context_id = gtk_statusbar_get_context_id( bar, context );
		gtk_statusbar_pop( bar, context_id );
	}
}

/**
 * nact_main_statusbar_set_locked:
 * @window: the #NactMainWindow instance.
 * @provider: whether the current provider is locked (read-only).
 * @item: whether the current item is locked (read-only).
 *
 * Displays the writability status of the current item as an image.
 * Installs the corresponding tooltip.
 */
void
nact_main_statusbar_set_locked( NactMainWindow *window, gboolean provider, gboolean item )
{
	static const gchar *thisfn = "nact_main_statusbar_set_locked";
	static const gchar *tooltip_provider = N_( "I/O Provider is locked down." );
	static const gchar *tooltip_item = N_( "Item is read-only." );
	GtkStatusbar *bar;
	GtkFrame *frame;
	GtkImage *image;
	gchar *tooltip;
	gchar *tmp;

	g_debug( "%s: window=%p, provider=%s, item=%s", thisfn, ( void * ) window, provider ? "True":"False", item ? "True":"False" );

	bar = get_statusbar( window );
	frame = GTK_FRAME( base_window_get_widget( BASE_WINDOW( window ), "ActionLockedFrame" ));
	image = GTK_IMAGE( base_window_get_widget( BASE_WINDOW( window ), "ActionLockedImage" ));

	if( bar && frame && image ){

		tooltip = g_strdup( "" );

		if( provider || item ){
			gtk_image_set_from_file( image, LOCKED_IMAGE );
			gtk_widget_show( GTK_WIDGET( image ));
			gtk_frame_set_shadow_type( frame, GTK_SHADOW_NONE );

			if( provider ){
				g_free( tooltip );
				tooltip = g_strdup( tooltip_provider );
			}
			if( item ){
				if( provider ){
					tmp = g_strdup_printf( "%s\n%s", tooltip, tooltip_item );
					g_free( tooltip );
					tooltip = tmp;
				} else {
					g_free( tooltip );
					tooltip = g_strdup( tooltip_item );
				}
			}

		} else {
			gtk_image_set_from_icon_name( image, "gnome-stock-blank", GTK_ICON_SIZE_MENU );
			gtk_widget_hide( GTK_WIDGET( image ));
			gtk_frame_set_shadow_type( frame, GTK_SHADOW_IN );
		}

		gtk_widget_set_tooltip_text( GTK_WIDGET( image ), tooltip );
		g_free( tooltip );
	}
}

/*
 * Returns the status bar widget
 */
static GtkStatusbar *
get_statusbar( const NactMainWindow *window )
{
	GtkWidget *statusbar;

	statusbar = base_window_get_widget( BASE_WINDOW( window ), "MainStatusbar" );

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
	g_debug( "nact_main_statusbar_display_timeout_free: stds=%p", ( void * ) stds );

	g_free( stds );
}
