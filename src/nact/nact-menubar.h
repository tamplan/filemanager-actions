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

#ifndef __NACT_MENUBAR_H__
#define __NACT_MENUBAR_H__

/*
 * SECTION: nact-menubar
 * @title: NactMenubar
 * @short_description: The Menubar class definition
 * @include: nact-menubar.h
 *
 * This is a convenience which embeds the menubar of the application.
 *
 * There is one object (because there is one menubar). It is created by
 * the main window at initialization time. It attachs itself to the window,
 * and destroys itself - via a weak ref - when the window is finalizing.
 *
 * Attaching the object to the window let us connect easily to all 'window'-
 * class messages, thus reducing the count of needed public functions.
 *
 * The #NactMenubar object connects to BASE_SIGNAL_INITIALIZE_WINDOW signal
 * at instanciation time. The caller (usually a #NactMainWindow) should take
 * care to connect itself to this signal before creating the #NactMenubar
 * object if it wants its callback be triggered first.
 */

#include "base-window.h"

/* *** */
#include <api/na-object.h>
#include <core/na-updater.h>
#include "nact-main-window.h"
/* *** */

G_BEGIN_DECLS

#define NACT_MENUBAR_TYPE                ( nact_menubar_get_type())
#define NACT_MENUBAR( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_MENUBAR_TYPE, NactMenubar ))
#define NACT_MENUBAR_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, NACT_MENUBAR_TYPE, NactMenubarClass ))
#define NACT_IS_MENUBAR( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_MENUBAR_TYPE ))
#define NACT_IS_MENUBAR_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), NACT_MENUBAR_TYPE ))
#define NACT_MENUBAR_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), NACT_MENUBAR_TYPE, NactMenubarClass ))

typedef struct _NactMenubarPrivate       NactMenubarPrivate;

typedef struct {
	/*< private >*/
	GObject             parent;
	NactMenubarPrivate *private;
}
	NactMenubar;

typedef struct _NactMenubarClassPrivate  NactMenubarClassPrivate;

typedef struct {
	/*< private >*/
	GObjectClass             parent;
	NactMenubarClassPrivate *private;
}
	NactMenubarClass;

/* Convenience macros to get a NactMenubar from a BaseWindow
 */
#define WINDOW_DATA_MENUBAR						"window-data-menubar"
#define BAR_WINDOW_VOID( window ) \
		g_return_if_fail( BASE_IS_WINDOW( window )); \
		NactMenubar *bar = ( NactMenubar * ) g_object_get_data( G_OBJECT( window ), WINDOW_DATA_MENUBAR ); \
		g_return_if_fail( NACT_IS_MENUBAR( bar ));

GType         nact_menubar_get_type( void );

NactMenubar  *nact_menubar_new     ( BaseWindow *window );

GtkUIManager *nact_menubar_get_ui_manager( const NactMenubar *bar );

/* *** */
/* this structure is updated each time the user interacts in the
 * interface ; it is then used to update action sensitivities
 */
typedef struct {
	gint       selected_menus;
	gint       selected_actions;
	gint       selected_profiles;
	gint       clipboard_menus;
	gint       clipboard_actions;
	gint       clipboard_profiles;
	gint       list_menus;
	gint       list_actions;
	gint       list_profiles;
	gboolean   is_modified;
	gboolean   have_exportables;
	gboolean   treeview_has_focus;
	gboolean   level_zero_order_changed;
	gulong     popup_handler;

	/* set by the nact_main_menubar_on_update_sensitivities() function itself
	 */
	gboolean   is_level_zero_writable;
	gboolean   has_writable_providers;
	guint      count_selected;
	GList     *selected_items;
	NAUpdater *updater;
}
	MenubarIndicatorsStruct;

#define MENUBAR_PROP_INDICATORS				"nact-menubar-indicators"

gboolean nact_main_menubar_is_level_zero_order_changed( const NactMainWindow *window );
void     nact_main_menubar_open_popup( NactMainWindow *window, GdkEventButton *event );
void     nact_main_menubar_enable_item( NactMainWindow *window, const gchar *name, gboolean enabled );
/* *** */

G_END_DECLS

#endif /* __NACT_MENUBAR_H__ */
