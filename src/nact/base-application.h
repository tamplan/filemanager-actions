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

#ifndef __BASE_APPLICATION_H__
#define __BASE_APPLICATION_H__

/*
 * BaseApplication class definition.
 *
 * This is a base class for Gtk+ programs.
 */

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BASE_APPLICATION_TYPE					( base_application_get_type())
#define BASE_APPLICATION( object )				( G_TYPE_CHECK_INSTANCE_CAST( object, BASE_APPLICATION_TYPE, BaseApplication ))
#define BASE_APPLICATION_CLASS( klass )			( G_TYPE_CHECK_CLASS_CAST( klass, BASE_APPLICATION_TYPE, BaseApplicationClass ))
#define BASE_IS_APPLICATION( object )			( G_TYPE_CHECK_INSTANCE_TYPE( object, BASE_APPLICATION_TYPE ))
#define BASE_IS_APPLICATION_CLASS( klass )		( G_TYPE_CHECK_CLASS_TYPE(( klass ), BASE_APPLICATION_TYPE ))
#define BASE_APPLICATION_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), BASE_APPLICATION_TYPE, BaseApplicationClass ))

typedef struct BaseApplicationPrivate BaseApplicationPrivate;

typedef struct {
	GObject                 parent;
	BaseApplicationPrivate *private;
}
	BaseApplication;

typedef struct BaseApplicationClassPrivate BaseApplicationClassPrivate;

typedef struct {
	GObjectClass                 parent;
	BaseApplicationClassPrivate *private;

	/* virtual functions */
	int       ( *run )                         ( BaseApplication *appli );
	void      ( *initialize )                  ( BaseApplication *appli );
	void      ( *initialize_i18n )             ( BaseApplication *appli );
	void      ( *initialize_gtk )              ( BaseApplication *appli );
	void      ( *initialize_application_name ) ( BaseApplication *appli );
	void      ( *initialize_icon_name )        ( BaseApplication *appli );
	void      ( *initialize_unique )           ( BaseApplication *appli );
	void      ( *initialize_ui )               ( BaseApplication *appli );
	gboolean  ( *is_willing_to_run )           ( BaseApplication *appli );
	void      ( *advertise_willing_to_run )    ( BaseApplication *appli );
	void      ( *advertise_not_willing_to_run )( BaseApplication *appli );
	void      ( *start )                       ( BaseApplication *appli );
	void      ( *finish )                      ( BaseApplication *appli );
	gchar   * ( *get_unique_name )             ( BaseApplication *appli );
	gchar   * ( *get_application_name )        ( BaseApplication *appli );
	gchar   * ( *get_icon_name )               ( BaseApplication *appli );
	gchar   * ( *get_ui_filename )             ( BaseApplication *appli );
	GObject * ( *get_main_window )             ( BaseApplication *appli );
}
	BaseApplicationClass;

/* instance properties
 */
#define PROP_APPLICATION_ARGC_STR				"argc"
#define PROP_APPLICATION_ARGV_STR				"argv"
#define PROP_APPLICATION_UNIQUE_NAME_STR		"unique-name"
#define PROP_APPLICATION_UNIQUE_APP_STR			"unique-app"
#define PROP_APPLICATION_NAME_STR				"application-name"
#define PROP_APPLICATION_ICON_NAME_STR			"icon-name"
#define PROP_APPLICATION_CODE_STR				"code"
#define PROP_APPLICATION_UI_XML_STR				"ui-xml"
#define PROP_APPLICATION_UI_FILENAME_STR		"ui-filename"
#define PROP_APPLICATION_MAIN_WINDOW_STR		"main-window"

GType      base_application_get_type( void );

int        base_application_run( BaseApplication *application );

gchar     *base_application_get_icon_name( BaseApplication *application );
GObject   *base_application_get_main_window( BaseApplication *application );

GtkWidget *base_application_get_widget( BaseApplication *application, const gchar *name );

void       base_application_error_dlg( BaseApplication *application, GtkMessageType type, const gchar *primary, const gchar *secondary );

G_END_DECLS

#endif /* __BASE_APPLICATION_H__ */
