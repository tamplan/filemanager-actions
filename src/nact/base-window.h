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

#ifndef __BASE_WINDOW_H__
#define __BASE_WINDOW_H__

/**
 * SECTION: base-window
 * @title: BaseWindow
 * @short_description: the BaseWindow base window class definition
 * @include: base-window.h
 *
 * This is a base class which encapsulates a Gtk+ windows.
 * It works together with the BaseApplication class to run a Gtk+
 * application.
 *
 * Note that two properties of #BaseApplication may be overriden on a
 * per-#BaseWindow basis. These are :
 *
 * - the #GtkBuilder UI manager
 *   the application has one global UI manager, but each window may
 *   have its own, provided that it is willing to reallocate a new
 *   one each time the window is opened.
 *
 *   Cf. http://bugzilla.gnome.org/show_bug.cgi?id=589746 against
 *   Gtk+ 2.16 : a GtkFileChooserWidget embedded in a GtkAssistant is
 *   not displayed when run more than once. As a work-around, reload
 *   the XML ui each time we run an assistant !
 *
 * - the filename which handled the window XML definition
 *   the application provides with one global default file, but each
 *   window may decide to provide its own.
 *
 *   Cf. http://bugzilla.gnome.org/show_bug.cgi?id=579345 against
 *   GtkBuilder : duplicate ids are no more allowed in a file. But we
 *   require this ability to have the same widget definition
 *   (ActionsList) in main window and export assistant.
 *   As a work-around, we have XML definition of export assistant in
 *   its own file.
 *   Another work-around could have be to let the IActionsList
 *   interface asks from the actual widget name to its implementor...
 *
 *   Sharing a same XML UI definition file implies sharing the same builder.
 */

#include "base-application.h"

G_BEGIN_DECLS

#define BASE_WINDOW_TYPE                ( base_window_get_type())
#define BASE_WINDOW( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, BASE_WINDOW_TYPE, BaseWindow ))
#define BASE_WINDOW_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, BASE_WINDOW_TYPE, BaseWindowClass ))
#define BASE_IS_WINDOW( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, BASE_WINDOW_TYPE ))
#define BASE_IS_WINDOW_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), BASE_WINDOW_TYPE ))
#define BASE_WINDOW_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), BASE_WINDOW_TYPE, BaseWindowClass ))

typedef struct _BaseWindowPrivate       BaseWindowPrivate;

typedef struct {
	/*< private >*/
	GObject            parent;
	BaseWindowPrivate *private;
}
	BaseWindow;

typedef struct _BaseWindowClassPrivate  BaseWindowClassPrivate;

/**
 * BaseWindowClass:
 * @initial_load_toplevel:
 * @runtime_init_toplevel:
 * @all_widgets_showed:
 * @dialog_response:
 * @delete_event:
 * @get_toplevel_name:
 * @get_iprefs_window_id:
 * @get_ui_filename:
 * @is_willing_to_quit:
 *
 * This defines the virtual method a derived class may, should or must implement.
 */
typedef struct {
	/*< private >*/
	GObjectClass            parent;
	BaseWindowClassPrivate *private;

	/*< public >*/
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
	 * window_get_toplevel_name:
	 * @window: this #BaseWindow instance.
	 *
	 * Pure virtual function.
	 */
	gchar *           ( *get_toplevel_name )    ( const BaseWindow *window );

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
	gchar *           ( *get_iprefs_window_id ) ( const BaseWindow *window );

	/**
	 * get_ui_filename:
	 * @window: this #BaseWindow instance.
	 *
	 * Asks the derived class for the filename of the XML definition of
	 * the user interface for this window. This XML definition must be
	 * suitable in order to be loaded via GtkBuilder.
	 *
	 * Defaults to application UI filename.
	 *
	 * Returns: the filename of the XML definition, to be g_free() by
	 * the caller.
	 */
	gchar *           ( *get_ui_filename )      ( const BaseWindow *window );

	/**
	 * is_willing_to_quit:
	 * @window: this #BaseWindow instance.
	 *
	 * Asks the derived class for the filename of the XML definition of
	 * the user interface for this window. This XML definition must be
	 * suitable in order to be loaded via GtkBuilder.
	 *
	 * Defaults to application UI filename.
	 *
	 * Returns: the filename of the XML definition, to be g_free() by
	 * the caller.
	 */
	gboolean          ( *is_willing_to_quit )   ( const BaseWindow *window );
}
	BaseWindowClass;

/**
 * Properties defined by the BaseWindow class.
 * They should be provided at object instanciation time.
 */
#define BASE_PROP_PARENT						"base-window-parent"
#define BASE_PROP_APPLICATION					"base-window-application"
#define BASE_PROP_XMLUI_FILENAME				"base-window-xmlui-filename"
#define BASE_PROP_HAS_OWN_BUILDER				"base-window-has-own-builder"
#define BASE_PROP_TOPLEVEL_NAME					"base-window-toplevel-name"

/**
 * Signals defined by the BaseWindow class.
 *
 * All signals of this class have the same behavior:
 * - the message is sent to all derived classes, which are free to
 *   connect to the signal in order to implement their own code;
 * - finally, the default class handler points to a virtual function
 * - the virtual function actually tries to call the actual function
 *   as possibly implemented by the derived class;
 * - each virtual method in a derived class should call the virtual
 *   method of its parent class if it exists;
 * - if no derived class has implemented the virtual function, the call
 *   fall back to doing nothing at all.
 */
#define BASE_SIGNAL_INITIALIZE_GTK				"base-window-initialize-gtk"
#define BASE_SIGNAL_INITIALIZE_WINDOW			"base-window-initialize-window"
#define BASE_SIGNAL_ALL_WIDGETS_SHOWED			"base-window-all-widgets-showed"

GType            base_window_get_type( void );

gboolean         base_window_init( BaseWindow *window );
int              base_window_run ( BaseWindow *window );

BaseApplication *base_window_get_application         ( const BaseWindow *window );
BaseWindow      *base_window_get_parent              ( const BaseWindow *window );
GtkWindow       *base_window_get_gtk_toplevel        ( const BaseWindow *window );
GtkWindow       *base_window_get_gtk_toplevel_by_name( const BaseWindow *window, const gchar *name );
GtkWidget       *base_window_get_widget              ( const BaseWindow *window, const gchar *name );

gboolean         base_window_is_willing_to_quit      ( const BaseWindow *window );

void             base_window_display_error_dlg       ( const BaseWindow *parent, const gchar *primary, const gchar *secondary );
gboolean         base_window_display_yesno_dlg       ( const BaseWindow *parent, const gchar *primary, const gchar *secondary );
void             base_window_display_message_dlg     ( const BaseWindow *parent, GSList *message );

gulong           base_window_signal_connect          ( BaseWindow *window, GObject *instance, const gchar *signal, GCallback fn );
gulong           base_window_signal_connect_after    ( BaseWindow *window, GObject *instance, const gchar *signal, GCallback fn );
gulong           base_window_signal_connect_by_name  ( BaseWindow *window, const gchar *name, const gchar *signal, GCallback fn );
gulong           base_window_signal_connect_with_data( BaseWindow *window, GObject *instance, const gchar *signal, GCallback fn, void *user_data );

G_END_DECLS

#endif /* __BASE_WINDOW_H__ */
