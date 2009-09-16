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

#ifndef __BASE_WINDOW_CLASS_H__
#define __BASE_WINDOW_CLASS_H__

/*
 * BaseWindow class definition.
 *
 * This is a base class which encapsulates a Gtk+ windows.
 * It works together with the BaseApplication class to run a Gtk+
 * application.
 */

#include <gtk/gtk.h>

#include "base-application-class.h"

G_BEGIN_DECLS

#define BASE_WINDOW_TYPE				( base_window_get_type())
#define BASE_WINDOW( object )			( G_TYPE_CHECK_INSTANCE_CAST( object, BASE_WINDOW_TYPE, BaseWindow ))
#define BASE_WINDOW_CLASS( klass )		( G_TYPE_CHECK_CLASS_CAST( klass, BASE_WINDOW_TYPE, BaseWindowClass ))
#define BASE_IS_WINDOW( object )		( G_TYPE_CHECK_INSTANCE_TYPE( object, BASE_WINDOW_TYPE ))
#define BASE_IS_WINDOW_CLASS( klass )	( G_TYPE_CHECK_CLASS_TYPE(( klass ), BASE_WINDOW_TYPE ))
#define BASE_WINDOW_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), BASE_WINDOW_TYPE, BaseWindowClass ))

typedef struct BaseWindowPrivate BaseWindowPrivate;

typedef struct {
	GObject            parent;
	BaseWindowPrivate *private;
}
	BaseWindow;

typedef struct BaseWindowClassPrivate BaseWindowClassPrivate;

typedef struct {
	GObjectClass            parent;
	BaseWindowClassPrivate *private;

	/**
	 * initial_load_toplevel:
	 * @window: this #BaseWindow instance.
	 */
	void              ( *initial_load_toplevel )( BaseWindow *window, gpointer user_data );

	/**
	 * runtime_init_toplevel:
	 * @window: this #BaseWindow instance.
	 */
	void              ( *runtime_init_toplevel )( BaseWindow *window, gpointer user_data );

	/**
	 * all_widgets_showed:
	 * @window: this #BaseWindow instance.
	 */
	void              ( *all_widgets_showed )   ( BaseWindow *window, gpointer user_data );

	/**
	 * dialog_response:
	 * @window: this #BaseWindow instance.
	 */
	gboolean          ( *dialog_response )      ( GtkDialog *dialog, gint code, BaseWindow *window );

	/**
	 * delete_event:
	 * @window: this #BaseWindow instance.
	 *
	 * The #BaseWindow class connects to the "delete-event" signal,
	 * and transforms it into a virtual function. The derived class
	 * can so implement the virtual function, without having to take
	 * care of the signal itself.
	 */
	gboolean          ( *delete_event )         ( BaseWindow *window, GtkWindow *toplevel, GdkEvent *event );

	/**
	 * get_application:
	 * @window: this #BaseWindow instance.
	 */
	BaseApplication * ( *get_application )      ( BaseWindow *window );

	/**
	 * window_get_toplevel_name:
	 * @window: this #BaseWindow instance.
	 *
	 * Pure virtual function.
	 */
	gchar *           ( *get_toplevel_name )    ( BaseWindow *window );

	/**
	 * get_toplevel_window:
	 * @window: this #BaseWindow instance.
	 *
	 * Returns the toplevel #GtkWindow associated with this #BaseWindow
	 * instance.
	 */
	GtkWindow *       ( *get_toplevel_window )  ( BaseWindow *window );

	/**
	 * get_window:
	 * @window: this #BaseWindow instance.
	 *
	 * Returns the named GtkWindow.
	 */
	GtkWindow *       ( *get_window )           ( BaseWindow *window, const gchar *name );

	/**
	 * get_widget:
	 * @window: this #BaseWindow instance.
	 *
	 * Returns the named #GtkWidget searched as a descendant of the
	 * #GtkWindow toplevel associated to this #Basewindow instance.
	 */
	GtkWidget *       ( *get_widget )           ( BaseWindow *window, const gchar *name );

	/**
	 * get_iprefs_window_id:
	 * @window: this #BaseWindow instance.
	 *
	 * Asks the derived class for the string which must be used to
	 * store last size and position of the window in GConf preferences.
	 *
	 * This delegates to #BaseWindow-derived classes the NactIPrefs
	 * interface virtual function.
	 */
	gchar *           ( *get_iprefs_window_id ) ( BaseWindow *window );
}
	BaseWindowClass;

GType base_window_get_type( void );

G_END_DECLS

#endif /* __BASE_WINDOW_CLASS_H__ */
