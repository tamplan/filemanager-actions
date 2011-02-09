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

#ifndef __NACT_MAIN_WINDOW_H__
#define __NACT_MAIN_WINDOW_H__

/**
 * SECTION: main-window
 * @title: NactMainWindow
 * @short_description: The Main Window class definition
 * @include: nact-main-window.h
 *
 * This class is derived from NactWindow and manages the main window.
 *
 * It embeds:
 * - the menubar,
 * - the toolbar,
 * - a button bar with sort buttons,
 * - the hierarchical list of items,
 * - a notebook which displays the content of the current item,
 * - a status bar.
 *
 * NactApplication    NactMainWindow    NactTreeView    NactTreeModel   NactMenubar
 *  |
 *  +- nact_main_window_new()
 *  |   |
 *  |   +----- connect to base-init-gtk-toplevel
 *  |   |                 base-init-window
 *  |   |                 base-all-widgets-showed
 *  |   |                 pivot-items-changed
 *  |   |                 tab-item-updated
 *  |   |
 *  |   +----- nact_tree_view_new()
 *  |   |       |
 *  |   |       +-------------- connect to base-init-gtk-toplevel
 *  |   |                                  base-init-window
 *  |   |                                  base-all-widgets-showed
 *  |   |
 *  |   +----- connect to tree-selection-changed
 *  |   |                 tree-modified-count-changed
 *  |   |
 *  |   +----- nact_menubar_new()
 *  |           |
 *  |           +---------------------------------------------------- connect to base-init-window
 *  |
 * emit 'base-init-gtk-toplevel'
 *  |
 *  +--------- init gtk toplevel in each tab
 *  |          init gtk toplevel in statusbar
 *  |
 *  +-------------------------- nact_tree_model_new()
 *  |                            |
 *  |                            +------------------- connect to base-init-window
 *  |                                                 attach the model to the view
 * emit 'base-init-window'
 *  |
 *  +--------- init runtime window in each tab
 *  |          init sort buttons
 *  |          connect to delete-event
 *  |
 *  +-------------------------- connect to treeview events
 *  |                           nact_tree_ieditable_initialize()
 *  |                            |
 *  |                            +-- register as iduplicable consumer
 *  |                                connect to iduplicable signals
 *  |
 *  +------------------------------------------------ connect to dnd events
 *  |
 *  +---------------------------------------------------------------- instanciate UI manager
 *  |                                                                 connect to tree signals
 * emit 'base-all-widgets-showed'
 *  |
 *  +--------- na_updater_load_items()
 *  |           |
 *  |           +-- check and set items modification/validity status
 *  |           |    +-> which generates lot of iduplicable events
 *  |           |
 *  |           +-- check and set items writability status
 *  |
 *  +--------- na_tree_view_fill()
 *  |           +-> which generates lot of selection-changed events
 *  |
 *  |          all widgets showed on all tab
 *  |          all widgets showed on sort buttons
 *  |
 *  +-------------------------- grab focus
 *  |                           allow notifications
 *  |                           select first row (if any)
 *  |
 * [X] End of initialization process
 */

#include "nact-application.h"
#include "nact-clipboard.h"
#include "nact-window.h"
#include "nact-tree-view.h"

G_BEGIN_DECLS

#define NACT_MAIN_WINDOW_TYPE                ( nact_main_window_get_type())
#define NACT_MAIN_WINDOW( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_MAIN_WINDOW_TYPE, NactMainWindow ))
#define NACT_MAIN_WINDOW_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, NACT_MAIN_WINDOW_TYPE, NactMainWindowClass ))
#define NACT_IS_MAIN_WINDOW( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_MAIN_WINDOW_TYPE ))
#define NACT_IS_MAIN_WINDOW_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), NACT_MAIN_WINDOW_TYPE ))
#define NACT_MAIN_WINDOW_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), NACT_MAIN_WINDOW_TYPE, NactMainWindowClass ))

typedef struct _NactMainWindowPrivate        NactMainWindowPrivate;

typedef struct {
	/*< private >*/
	NactWindow             parent;
	NactMainWindowPrivate *private;
}
	NactMainWindow;

typedef struct _NactMainWindowClassPrivate   NactMainWindowClassPrivate;

typedef struct {
	/*< private >*/
	NactWindowClass             parent;
	NactMainWindowClassPrivate *private;
}
	NactMainWindowClass;

/**
 * Signals emitted by the main window
 */
#define MAIN_SIGNAL_SELECTION_CHANGED		"main-selection-changed"

/**
 * Properties set against the main window
 */
#define MAIN_PROP_ITEM						"main-current-item"
#define MAIN_PROP_PROFILE					"main-current-profile"
#define MAIN_PROP_CONTEXT					"main-current-context"
#define MAIN_PROP_EDITABLE					"main-editable"
#define MAIN_PROP_REASON					"main-reason"

GType           nact_main_window_get_type( void );

NactMainWindow *nact_main_window_new           ( const NactApplication *application );

NactClipboard  *nact_main_window_get_clipboard ( const NactMainWindow *window );
NactTreeView   *nact_main_window_get_items_view( const NactMainWindow *window );

void            nact_main_window_reload( NactMainWindow *window );

gboolean        nact_main_window_quit  ( NactMainWindow *window );

G_END_DECLS

#endif /* __NACT_MAIN_WINDOW_H__ */
