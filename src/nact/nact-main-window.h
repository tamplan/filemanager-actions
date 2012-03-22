/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2012 Pierre Wieser and others (see AUTHORS)
 *
 * Nautilus-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Nautilus-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nautilus-Actions; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
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
 *  +-> nact_main_window_new()
 *  |   |
 *  |   +-> NactMainWindow::instance_contructed()
 *  |   |   |
 *  |   |   +-> connect to base-init-gtk       [window]
 *  |   |   |              base-init-window    [window]
 *  |   |   |              base-show-widgets   [window]
 *  |   |   |              pivot-items-changed [updater]
 *  |   |   |              tab-item-updated    [window]
 *  |   |   |
 *  |   |   +-> nact_menubar_new()
 *  |   |   |   |
 *  |   |   |   +-> NactMenubar::nact_menubar_new()
 *  |   |   |   |   |
 *  |   |   |   |   +-> nact_sort_buttons_new()
 *  |   |   |   |   +-> connect to base-init-window [window]
 *  |   |   |   |   |
 *  |   |   |   |  <-
 *  |   |   |  <-
 *  |   |   |
 *  |   |   +-> nact_clipboard_new()
 *  |   |   |
 *  |   |   +-> initialize each notebook tab
 *  |   |   |
 *  |   |   |   NactMainWindow has connected to "base-init-window" signal _before_
 *  |   |   |   other widgets have been created or initialized. We so are sure that
 *  |   |   |   the NactMainWindow handlers will be called first.
 *  |   |  <-
 *  |   |
 *  |   +-> base_window_init()
 *  |   |   |
 *  |   |   +-> setup builder
 *  |   |   +-> load gtk toplevel
 *  |   |   +-> emit signal base-initialize-gtk
 *  |   |   |
 *  |   |   |   [H]-> NactMainWindow::on_base_initialize_gtk()
 *  |   |   |         |
 *  |   |   |         +-> nact_tree_view_new()
 *  |   |   |         |   |
 *  |   |   |         |   +-> NactTreeView::instance_contructed()
 *  |   |   |         |   |   |
 *  |   |   |         |   |   +-> connect to base-init-window  [window]
 *  |   |   |         |   |   |              base-show-widgets [window]
 *  |   |   |         |   |   |
 *  |   |   |         |   |   +-> initialize_gtk()
 *  |   |   |         |   |   |   |
 *  |   |   |         |   |   |   +-> nact_tree_model_new()
 *  |   |   |         |   |   |   |   |
 *  |   |   |         |   |   |   |   +-> NactTreeModel::nact_tree_model_new()
 *  |   |   |         |   |   |   |   |
 *  |   |   |         |   |   |   |  <-
 *  |   |   |         |   |   |  <-
 *  |   |   |         |   |  <-
 *  |   |   |         |  <-
 *  |   |   |         |
 *  |   |   |         +-> nact_main_statusbar_initialize_load_toplevel()
 *  |   |   |
 *  |   |   |   [H]-> NactIxxxxxxTab::on_base_initialize_gtk()
 *  |   |   |
 *  |   |   +-> emit signal base-initialize-window
 *  |   |   |
 *  |   |   |   [H]-> NactMainWindow::on_base_initialize_base_window()
 *  |   |   |         |
 *  |   |   |         +-> connect to delete-event
 *  |   |   |         |              base-quit-requested
 *  |   |   |         |
 *  |   |   |         +-> connect to tree-selection-changed
 *  |   |   |         |              tree-modified-status-changed
 *  |   |   |         |
 *  |   |   |         |   Because NactMainWindow was the first class to connect to
 *  |   |   |         |   "base-initialize-window" signal, then this handler has been
 *  |   |   |         |   the first to be called. And so the "tree-selection-changed"
 *  |   |   |         |   signal handler will also the first to be called, and we can
 *  |   |   |         |   safely rely on that.
 *  |   |   |        <-
 *  |   |   |
 *  |   |   |   [H]-> NactIxxxxxxTab::on_base_initialize_window()
 *  |   |   |
 *  |   |   |   [H]-> NactTreeView::on_base_initialize_view()
 *  |   |   |         |
 *  |   |   |         +-> monitors the selection in the tree
 *  |   |   |         |   in order to be able to send the "tree-selection-changed" signal
 *  |   |   |         |
 *  |   |   |         +- nact_tree_ieditable_initialize()
 *  |   |   |         |
 *  |   |   |        <-
 *  |   |   |
 *  |   |   +-> emit signal base-show-widgets
 *  |   |   |
 *  |   |   |   [H]-> NactMainWindow::on_base_show_widgets()
 *  |   |   |         |
 *  |   |   |         +-> load items from pivot
 *  |   |   |
 *  |   |   |   [H]-> NactIxxxxxxTab::on_base_show_widgets()
 *  |   |   |
 *  |   |   +-> gtk_widget_show_all()
 *  |   |   |
 *  |   |  <-
 *  |  <-
 *  |
 *  +-> gtk_main()
 *  |
 * [X] End of initialization process
 *
 * Some signals and their usages
 * =============================
 * MAIN_SIGNAL_SELECTION_CHANGED
 *   The signal is sent by the tree view on the BaseWindow each time the
 *   selection has changed. By construction (cf. initialization process),
 *   the main window handler is the first to be triggered by this signal:
 *   it sets the 'current' main window properties to reflect this new
 *   selection.
 *   Args:
 *   - the list of selected items, may be NULL.
 *   Consumers:
 *   - all tabs should take advantage of this signal to enable/disable
 *     their page, setup the content of their widgets, and so on.
 *   - the menubar updates its indicator depending of the current selection
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
 *   that in a tab: either the label has been edited inline in the tree view,
 *   or a new i/o provider has been identified. The relevant NAObject has
 *   been updated accordingly.
 *   Args:
 *   - an OR-ed list of modified flags, or 0 if not relevant
 *   Consumers are:
 *   - IActionTab and ICommandTab should update their label widgets
 *   - IPropertiesTab updates its provider label
 *
 * MAIN_SIGNAL_CONTEXT_MENU
 *   Opens the specified context menu.
 *
 * TREE_SIGNAL_FOCUS_IN
 * TREE_SIGNAL_FOCUS_OUT
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

#define NACT_TYPE_MAIN_WINDOW                ( nact_main_window_get_type())
#define NACT_MAIN_WINDOW( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_TYPE_MAIN_WINDOW, NactMainWindow ))
#define NACT_MAIN_WINDOW_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, NACT_TYPE_MAIN_WINDOW, NactMainWindowClass ))
#define NACT_IS_MAIN_WINDOW( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_TYPE_MAIN_WINDOW ))
#define NACT_IS_MAIN_WINDOW_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), NACT_TYPE_MAIN_WINDOW ))
#define NACT_MAIN_WINDOW_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), NACT_TYPE_MAIN_WINDOW, NactMainWindowClass ))

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
#define MAIN_SIGNAL_CONTEXT_MENU			"main-signal-open-popup"

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
