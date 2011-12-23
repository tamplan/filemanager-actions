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

#include <string.h>

#include "na-gtk-utils.h"
#include "na-settings.h"

static void   int_list_to_position( GList *list, gint *x, gint *y, gint *width, gint *height );
static GList *position_to_int_list( gint x, gint y, gint width, gint height );
static void   free_int_list( GList *list );

/*
 * na_gtk_utils_find_widget_by_type:
 * @container: a #GtkContainer, usually the #GtkWindow toplevel.
 * @type: the searched #GType.
 *
 * Returns: the first child widget which is of @type type.
 */
GtkWidget *
na_gtk_utils_find_widget_by_type( GtkContainer *container, GType type )
{
	GList *children = gtk_container_get_children( container );
	GList *ic;
	GtkWidget *found = NULL;
	GtkWidget *child;
	const gchar *child_name;

	for( ic = children ; ic && !found ; ic = ic->next ){

		if( GTK_IS_WIDGET( ic->data )){
			if( G_OBJECT_TYPE( ic->data ) == type ){
				found = GTK_WIDGET( ic->data );
			} else if( GTK_IS_CONTAINER( child )){
				found = na_gtk_utils_find_widget_by_type( GTK_CONTAINER( child ), type );
			}
		}
	}

	g_list_free( children );
	return( found );
}

/*
 * na_gtk_utils_search_for_child_widget:
 * @container: a #GtkContainer, usually the #GtkWindow toplevel.
 * @name: the name of the searched widget.
 *
 * Returns: the searched widget.
 */
GtkWidget *
na_gtk_utils_search_for_child_widget( GtkContainer *container, const gchar *name )
{
	GList *children = gtk_container_get_children( container );
	GList *ic;
	GtkWidget *found = NULL;
	GtkWidget *child;
	const gchar *child_name;

	for( ic = children ; ic && !found ; ic = ic->next ){

		if( GTK_IS_WIDGET( ic->data )){
			child = GTK_WIDGET( ic->data );
			child_name = gtk_buildable_get_name( GTK_BUILDABLE( child ));
			if( child_name && strlen( child_name )){
				/*g_debug( "%s: child=%s", thisfn, child_name );*/

				if( !g_ascii_strcasecmp( name, child_name )){
					found = child;
					break;

				} else if( GTK_IS_CONTAINER( child )){
					found = na_gtk_utils_search_for_child_widget( GTK_CONTAINER( child ), name );
				}
			}
		}
	}

	g_list_free( children );
	return( found );
}

#ifdef NA_MAINTAINER_MODE
static void
dump_children( const gchar *thisfn, GtkContainer *container, int level )
{
	GList *children = gtk_container_get_children( container );
	GList *ic;
	GtkWidget *child;
	const gchar *child_name;
	GString *prefix;
	int i;

	prefix = g_string_new( "" );
	for( i = 0 ; i <= level ; ++i ){
		g_string_append_printf( prefix, "%s", "|  " );
	}

	for( ic = children ; ic ; ic = ic->next ){

		if( GTK_IS_WIDGET( ic->data )){
			child = GTK_WIDGET( ic->data );
			child_name = gtk_buildable_get_name( GTK_BUILDABLE( child ));
			g_debug( "%s: %s%s\t%p %s",
					thisfn, prefix->str, G_OBJECT_TYPE_NAME( child ), ( void * ) child, child_name );

			if( GTK_IS_CONTAINER( child )){
				dump_children( thisfn, GTK_CONTAINER( child ), level+1 );
			}
		}
	}

	g_list_free( children );
	g_string_free( prefix, TRUE );
}

void
na_gtk_utils_dump_children( GtkContainer *container )
{
	static const gchar *thisfn = "na_gtk_utils_dump_children";

	g_debug( "%s: container=%p", thisfn, container );

	dump_children( thisfn, container, 0 );
}
#endif

/**
 * na_gtk_utils_restore_position_window:
 * @toplevel: the #GtkWindow window.
 * @wsp_name: the string which handles the window size and position in user preferences.
 *
 * Position the specified window on the screen.
 *
 * A window position is stored as a list of integers "x,y,width,height".
 */
void
na_gtk_utils_restore_window_position( GtkWindow *toplevel, const gchar *wsp_name )
{
	static const gchar *thisfn = "na_gtk_utils_restore_window_position";
	GList *list;
	gint x=0, y=0, width=0, height=0;
	GdkDisplay *display;
	GdkScreen *screen;
	gint screen_width, screen_height;

	g_return_if_fail( GTK_IS_WINDOW( toplevel ));
	g_return_if_fail( wsp_name && strlen( wsp_name ));

	g_debug( "%s: toplevel=%p (%s), wsp_name=%s",
			thisfn, ( void * ) toplevel, G_OBJECT_TYPE_NAME( toplevel ), wsp_name );

	list = na_settings_get_uint_list( wsp_name, NULL, NULL );

	if( list ){
		int_list_to_position( list, &x, &y, &width, &height );
		g_debug( "%s: wsp_name=%s, x=%d, y=%d, width=%d, height=%d", thisfn, wsp_name, x, y, width, height );
		free_int_list( list );
	}

	x = MAX( 1, x );
	y = MAX( 1, y );
	width = MAX( 1, width );
	height = MAX( 1, height );

	display = gdk_display_get_default();
	screen = gdk_display_get_screen( display, 0 );
	screen_width = gdk_screen_get_width( screen );
	screen_height = gdk_screen_get_height( screen );

	/* very dirty hack based on the assumption that Gnome 2.x has a bottom
	 * and a top panel bars, while Gnome 3.x only has one.
	 * Don't know how to get usable height of screen, and don't bother today.
	 */
	screen_height -= DEFAULT_HEIGHT;
#if ! GTK_CHECK_VERSION( 3, 0, 0 )
	screen_height -= DEFAULT_HEIGHT;
#endif

	width = MIN( width, screen_width-x );
	height = MIN( height, screen_height-y );

	g_debug( "%s: wsp_name=%s, screen=(%d,%d), x=%d, y=%d, width=%d, height=%d",
			thisfn, wsp_name, screen_width, screen_height, x, y, width, height );

	gtk_window_move( toplevel, x, y );
	gtk_window_resize( toplevel, width, height );
}

/**
 * na_gtk_utils_save_window_position:
 * @toplevel: the #GtkWindow window.
 * @wsp_name: the string which handles the window size and position in user preferences.
 *
 * Save the size and position of the specified window.
 */
void
na_gtk_utils_save_window_position( GtkWindow *toplevel, const gchar *wsp_name )
{
	static const gchar *thisfn = "na_gtk_utils_save_window_position";
	gint x, y, width, height;
	GList *list;

	g_return_if_fail( GTK_IS_WINDOW( toplevel ));
	g_return_if_fail( wsp_name && strlen( wsp_name ));

	gtk_window_get_position( toplevel, &x, &y );
	gtk_window_get_size( toplevel, &width, &height );
	g_debug( "%s: wsp_name=%s, x=%d, y=%d, width=%d, height=%d", thisfn, wsp_name, x, y, width, height );

	list = position_to_int_list( x, y, width, height );
	na_settings_set_uint_list( wsp_name, list );
	free_int_list( list );
}

/*
 * extract the position of the window from the list of unsigned integers
 */
static void
int_list_to_position( GList *list, gint *x, gint *y, gint *width, gint *height )
{
	GList *it;
	int i;

	g_assert( x );
	g_assert( y );
	g_assert( width );
	g_assert( height );

	for( it=list, i=0 ; it ; it=it->next, i+=1 ){
		switch( i ){
			case 0:
				*x = GPOINTER_TO_UINT( it->data );
				break;
			case 1:
				*y = GPOINTER_TO_UINT( it->data );
				break;
			case 2:
				*width = GPOINTER_TO_UINT( it->data );
				break;
			case 3:
				*height = GPOINTER_TO_UINT( it->data );
				break;
		}
	}
}

static GList *
position_to_int_list( gint x, gint y, gint width, gint height )
{
	GList *list = NULL;

	list = g_list_append( list, GUINT_TO_POINTER( x ));
	list = g_list_append( list, GUINT_TO_POINTER( y ));
	list = g_list_append( list, GUINT_TO_POINTER( width ));
	list = g_list_append( list, GUINT_TO_POINTER( height ));

	return( list );
}

/*
 * free the list of int
 */
static void
free_int_list( GList *list )
{
	g_list_free( list );
}
