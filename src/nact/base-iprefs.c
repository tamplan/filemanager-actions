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

#include <core/na-iprefs.h>
#include <core/na-settings.h>

#include "base-iprefs.h"
#include "nact-application.h"

/* private interface data
 */
struct _BaseIPrefsInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

static gboolean st_initialized = FALSE;
static gboolean st_finalized = FALSE;

static GType       register_type( void );
static void        interface_base_init( BaseIPrefsInterface *klass );
static void        interface_base_finalize( BaseIPrefsInterface *klass );

static NASettings *get_settings( const BaseWindow *window );
static GList      *read_int_list( const BaseWindow *window, const gchar *key );
static void        write_int_list( const BaseWindow *window, const gchar *key, GList *list );
static void        int_list_to_position( const BaseWindow *window, GList *list, gint *x, gint *y, gint *width, gint *height );
static GList      *position_to_int_list( const BaseWindow *window, gint x, gint y, gint width, gint height );
static void        free_int_list( GList *list );

GType
base_iprefs_get_type( void )
{
	static GType iface_type = 0;

	if( !iface_type ){
		iface_type = register_type();
	}

	return( iface_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "base_iprefs_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( BaseIPrefsInterface ),
		( GBaseInitFunc ) interface_base_init,
		( GBaseFinalizeFunc ) interface_base_finalize,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_INTERFACE, "BaseIPrefs", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( BaseIPrefsInterface *klass )
{
	static const gchar *thisfn = "base_iprefs_interface_base_init";

	if( !st_initialized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( BaseIPrefsInterfacePrivate, 1 );

		st_initialized = TRUE;
	}
}

static void
interface_base_finalize( BaseIPrefsInterface *klass )
{
	static const gchar *thisfn = "base_iprefs_interface_base_finalize";

	if( st_initialized && !st_finalized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		st_finalized = TRUE;

		g_free( klass->private );
	}
}

/**
 * base_iprefs_position_window:
 * @window: this #BaseWindow-derived window.
 * @wsp_name: the string which handles the window size and position in user preferences.
 *
 * Position the specified window on the screen.
 *
 * A window position is stored as a list of integers "x,y,width,height".
 */
void
base_iprefs_restore_window_position( const BaseWindow *window, const gchar *wsp_name )
{
	static const gchar *thisfn = "base_iprefs_restore_window_position";
	GtkWindow *toplevel;
	GList *list;
	gint x=0, y=0, width=0, height=0;
	GdkDisplay *display;
	GdkScreen *screen;
	gint screen_width, screen_height;

	g_return_if_fail( BASE_IS_WINDOW( window ));
	g_return_if_fail( wsp_name && strlen( wsp_name ));

	toplevel = base_window_get_gtk_toplevel( window );

	g_debug( "%s: window=%p (%s), toplevel=%p (%s), wsp_name=%s",
			thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ),
			( void * ) toplevel, G_OBJECT_TYPE_NAME( toplevel ), wsp_name );

	list = read_int_list( window, wsp_name );

	if( list ){
		int_list_to_position( window, list, &x, &y, &width, &height );
		g_debug( "%s: wsp_name=%s, x=%d, y=%d, width=%d, height=%d", thisfn, wsp_name, x, y, width, height );
		free_int_list( list );
	}

	if( width > 0 && height > 0 ){
		display = gdk_display_get_default();
		screen = gdk_display_get_screen( display, 0 );
		screen_width = gdk_screen_get_width( screen );
		screen_height = gdk_screen_get_height( screen );

		if(( x+width < screen_width ) && ( y+height < screen_height )){
			gtk_window_move( toplevel, x, y );
			gtk_window_resize( toplevel, width, height );
		}
	}
}

/**
 * base_iprefs_save_window_position:
 * @window: this #BaseWindow-derived window.
 * @wsp_name: the string which handles the window size and position in user preferences.
 *
 * Save the size and position of the specified window.
 */
void
base_iprefs_save_window_position( const BaseWindow *window, const gchar *wsp_name )
{
	static const gchar *thisfn = "base_iprefs_save_window_position";
	GtkWindow *toplevel;
	gint x, y, width, height;
	GList *list;

	g_return_if_fail( BASE_IS_WINDOW( window ));
	g_return_if_fail( wsp_name && strlen( wsp_name ));

	toplevel = base_window_get_gtk_toplevel( window );
	g_return_if_fail( GTK_IS_WINDOW( toplevel ));

	gtk_window_get_position( toplevel, &x, &y );
	gtk_window_get_size( toplevel, &width, &height );
	g_debug( "%s: wsp_name=%s, x=%d, y=%d, width=%d, height=%d", thisfn, wsp_name, x, y, width, height );

	list = position_to_int_list( window, x, y, width, height );
	write_int_list( window, wsp_name, list );
	free_int_list( list );
}

/* It seems inevitable that preferences are attached to the application.
 * Unfortunately, it does not seem possible to have a base window size and
 * position itself. So, this BaseIPrefs interface is not really a base
 * interface, but rather a common one, attached to the application
 */
static NASettings *
get_settings( const BaseWindow *window )
{
	NactApplication *appli = NACT_APPLICATION( base_window_get_application( window ));
	NAUpdater *updater = nact_application_get_updater( appli );
	return( na_pivot_get_settings( NA_PIVOT( updater )));
}

/*
 * returns a list of int
 */
static GList *
read_int_list( const BaseWindow *window, const gchar *key )
{
	NASettings *settings = get_settings( window );
	return( na_settings_get_uint_list( settings, key, NULL, NULL ));
}

static void
write_int_list( const BaseWindow *window, const gchar *key, GList *list )
{
	NASettings *settings = get_settings( window );
	na_settings_set_uint_list( settings, key, list );
}

/*
 * extract the position of the window from the list of unsigned integers
 */
static void
int_list_to_position( const BaseWindow *window, GList *list, gint *x, gint *y, gint *width, gint *height )
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
position_to_int_list( const BaseWindow *window, gint x, gint y, gint width, gint height )
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
