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
 *
 * Signals, their rules and uses
 * =============================
 * TREE_SIGNAL_SELECTION_CHANGED
 *   The signal is sent on the BaseWindow by the tree view each time the selection
 *   changes.
 *   Args:
 *   - the list of selected items, may be NULL.
 *   Consumers:
 *   - the main window updates its 'current' properties, then send the
 *     MAIN_SIGNAL_SELECTION_CHANGED signal
 *   - the menubar updates its indicator depending of the current selection
 *
 * MAIN_SIGNAL_SELECTION_CHANGED
 *   The signal is sent on the BaseWindow by the main window when the selection has
 *   changed. 'current' main window properties have been set to reflect this new
 *   selection.
 *   Args:
 *   - the list of selected items, may be NULL.
 *   Consumers:
 *   - All tabs should take advantage of this signal to enable/disable their
 *     page, setup the content of their widgets, and so on.
 *
 * TAB_UPDATABLE_SIGNAL_ITEM_UPDATED
 *   The signal is sent on the BaseWindow each time a widget is updated; the widget
 *   callback must setup the edited object with the new value, and then should call
 *   this signal with a flag indicating if the tree display should be refreshed now.
 *   Args:
 *   - an OR-ed list of modified flags, or 0 if not relevant
 *   Consumers are:
 *   - the main window checks the modification/validity status of the object
 *   - if the 'refresh tree display' flag is set, then the tree model refreshes
 *     the current row with current label and icon, then flags current row as
 *     modified
 *
 * MAIN_SIGNAL_ITEM_UPDATED
 *   The signal is sent on the BaseWindow after a data has been modified elsewhere
 *   that in a tab: either the label the label has been edited inline in the tree
 *   view, or a new i/o provider has been identified. The relevant NAObject has
 *   been updated accordingly.
 *   Args:
 *   - an OR-ed list of modified flags, or 0 if not relevant
 *   Consumers are:
 *   - IActionTab and ICommandTab should update their label widgets
 *   - IPropertiesTab updates its provider label
 *
 * TREE_SIGNAL_FOCUS_IN
 * TREE_SIGNAL_FOCUS_OUT
 * TREE_SIGNAL_CONTEXT_MENU
 * TREE_SIGNAL_COUNT_CHANGED
 * TREE_SIGNAL_LEVEL_ZERO_CHANGED
 * TREE_SIGNAL_MODIFIED_STATUS_CHANGED
 *
 * TAB_UPDATABLE_SIGNAL_PROVIDER_CHANGED
 *
 * Object
 */

#include "nact-application.h"
#include "nact-clipboard.h"
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
	BaseWindow             parent;
	NactMainWindowPrivate *private;
}
	NactMainWindow;

typedef struct _NactMainWindowClassPrivate   NactMainWindowClassPrivate;

typedef struct {
	/*< private >*/
	BaseWindowClass             parent;
	NactMainWindowClassPrivate *private;
}
	NactMainWindowClass;

/**
 * Signals emitted by the main window
 */
#define MAIN_SIGNAL_ITEM_UPDATED			"main-item-updated"
#define MAIN_SIGNAL_SELECTION_CHANGED		"main-selection-changed"

/**
 * The data which, when modified, should be redisplayed asap.
 * This is used by MAIN_SIGNAL_ITEM_UPDATED and TAB_UPDATABLE_SIGNAL_ITEM_UPDATED
 * signals.
 */
enum {
	MAIN_DATA_LABEL    = 1<<0,
	MAIN_DATA_ICON     = 1<<1,
	MAIN_DATA_PROVIDER = 1<<2,
};

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

void            nact_main_window_reload      ( NactMainWindow *window );
void            nact_main_window_block_reload( NactMainWindow *window );
gboolean        nact_main_window_quit        ( NactMainWindow *window );

G_END_DECLS

#endif /* __NACT_MAIN_WINDOW_H__ */
