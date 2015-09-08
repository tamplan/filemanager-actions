/*
 * FileManager-Actions
 * A file-manager extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
 *
 * FileManager-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * FileManager-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FileManager-Actions; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
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

#include "api/fma-object-api.h"

#include "core/fma-gtk-utils.h"

#include "base-keysyms.h"
#include "fma-application.h"
#include "fma-main-window.h"
#include "nact-tree-view.h"
#include "nact-tree-model.h"
#include "nact-tree-ieditable.h"

/* private instance data
 */
struct _NactTreeViewPrivate {
	gboolean        dispose_has_run;

	/* properties set at instanciation time
	 */
	FMAMainWindow *window;

	/* initialization
	 */
	guint           mode;
	gboolean        notify_allowed;

	/* runtime data
	 */
	GtkTreeView    *tree_view;
};

/* signals
 */
enum {
	COUNT_CHANGED,
	FOCUS_IN,
	FOCUS_OUT,
	LEVEL_ZERO_CHANGED,
	MODIFIED_STATUS,
	SELECTION_CHANGED,
	OPEN_POPUP,
	LAST_SIGNAL
};

/* when toggle collapse/expand rows, we want all rows have the same
 * behavior, e.g. all rows collapse, or all rows expand
 * this behavior is fixed by the first rows which will actually be
 * toggled
 */
enum {
	TOGGLE_UNDEFINED,
	TOGGLE_COLLAPSE,
	TOGGLE_EXPAND
};

/* iter on selection prototype
 */
typedef gboolean ( *FnIterOnSelection )( NactTreeView *, GtkTreeModel *, GtkTreeIter *, FMAObject *, gpointer );

static gint          st_signals[ LAST_SIGNAL ] = { 0 };
static GObjectClass *st_parent_class           = NULL;

static GType      register_type( void );
static void       class_init( NactTreeViewClass *klass );
static void       tree_ieditable_iface_init( NactTreeIEditableInterface *iface, void *user_data );
static void       instance_init( GTypeInstance *instance, gpointer klass );
static void       instance_dispose( GObject *application );
static void       instance_finalize( GObject *application );
static void       initialize_gtk( NactTreeView *view );
static gboolean   on_button_press_event( GtkWidget *widget, GdkEventButton *event, NactTreeView *view );
static gboolean   on_focus_in( GtkWidget *widget, GdkEventFocus *event, NactTreeView *view );
static gboolean   on_focus_out( GtkWidget *widget, GdkEventFocus *event, NactTreeView *view );
static gboolean   on_key_pressed_event( GtkWidget *widget, GdkEventKey *event, NactTreeView *view );
static gboolean   on_popup_menu( GtkWidget *widget, NactTreeView *view );
static void       on_selection_changed( GtkTreeSelection *selection, NactTreeView *view );
static void       on_tree_view_realized( NactTreeView *treeview, void *empty );
static void       clear_selection( NactTreeView *view );
static void       on_selection_changed_cleanup_handler( NactTreeView *tview, GList *selected_items );
static void       display_label( GtkTreeViewColumn *column, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, NactTreeView *view );
static void       extend_selection_to_children( NactTreeView *view, GtkTreeModel *model, GtkTreeIter *parent );
static GList     *get_selected_items( NactTreeView *view );
static void       iter_on_selection( NactTreeView *view, FnIterOnSelection fn_iter, gpointer user_data );
static void       navigate_to_child( NactTreeView *view );
static void       navigate_to_parent( NactTreeView *view );
static void       do_open_popup( NactTreeView *view, GdkEventButton *event );
static void       select_row_at_path_by_string( NactTreeView *view, const gchar *path );
static void       toggle_collapse( NactTreeView *view );
static gboolean   toggle_collapse_iter( NactTreeView *view, GtkTreeModel *model, GtkTreeIter *iter, FMAObject *object, gpointer user_data );
static void       toggle_collapse_row( GtkTreeView *treeview, GtkTreePath *path, guint *toggle );

GType
nact_tree_view_get_type( void )
{
	static GType type = 0;

	static const GInterfaceInfo tree_ieditable_iface_info = {
		( GInterfaceInitFunc ) tree_ieditable_iface_init,
		NULL,
		NULL
	};

	if( !type ){
		type = register_type();
		g_type_add_interface_static( type, NACT_TREE_IEDITABLE_TYPE, &tree_ieditable_iface_info );
	}

	return( type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "nact_tree_view_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NactTreeViewClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactTreeView ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( GTK_TYPE_BIN, "NactTreeView", &info, 0 );

	return( type );
}

static void
class_init( NactTreeViewClass *klass )
{
	static const gchar *thisfn = "nact_tree_view_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	/**
	 * NactTreeView::tree-signal-count-changed:
	 *
	 * This signal is emitted on BaseWindow parent when the count of items
	 * has changed in the underlying tree store.
	 *
	 *   Counting rows is needed to maintain action sensitivities in the
	 *   menubar : at least 'Tools\Export' menu item depends if we have
	 *   exportables, i.e. at least one menu or action.
	 *   Also, the total count of menu/actions/profiles is displayed in the
	 *   statusbar.
	 *
	 *   Rows are first counted when the treeview is primarily filled, or
	 *   refilled on demand. Counters are then incremented (resp. decremented)
	 *   when inserting or pasting (resp. deleting) rows in the list.
	 *
	 * Signal args:
	 * - reset if %TRUE, add if %FALSE
	 * - menus count/increment (may be negative)
	 * - actions count/increment (may be negative)
	 * - profiles count/increment (may be negative)
	 *
	 * Handler prototype:
	 * void ( *handler )( BaseWindow *window, gboolean reset,
	 *                    guint menus_count, guint actions_count, guint profiles_count, gpointer user_data );
	 */
	st_signals[ COUNT_CHANGED ] = g_signal_new(
			TREE_SIGNAL_COUNT_CHANGED,
			NACT_TYPE_TREE_VIEW,
			G_SIGNAL_RUN_LAST,
			0,						/* no default handler */
			NULL,
			NULL,
			NULL,
			G_TYPE_NONE,
			4,
			G_TYPE_BOOLEAN, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT );

	/**
	 * NactTreeView::tree-signal-focus-in:
	 *
	 * This signal is emitted on the window when the view gains the focus.
	 * In particular, edition menu is disabled outside of the treeview.
	 *
	 * Signal args: none
	 *
	 * Handler prototype:
	 * void ( *handler )( BaseWindow *window, gpointer user_data )
	 */
	st_signals[ FOCUS_IN ] = g_signal_new(
			TREE_SIGNAL_FOCUS_IN,
			NACT_TYPE_TREE_VIEW,
			G_SIGNAL_RUN_LAST,
			0,
			NULL,
			NULL,
			NULL,
			G_TYPE_NONE,
			0 );

	/**
	 * NactTreeView::tree-signal-focus-out:
	 *
	 * This signal is emitted on the window when the view loses the focus.
	 * In particular, edition menu is disabled outside of the treeview.
	 *
	 * Signal args: none
	 *
	 * Handler prototype:
	 * void ( *handler )( BaseWindow *window, gpointer user_data )
	 */
	st_signals[ FOCUS_OUT ] = g_signal_new(
			TREE_SIGNAL_FOCUS_OUT,
			NACT_TYPE_TREE_VIEW,
			G_SIGNAL_RUN_LAST,
			0,
			NULL,
			NULL,
			NULL,
			G_TYPE_NONE,
			0 );

	/**
	 * NactTreeView::tree-signal-level-zero-changed:
	 *
	 * This signal is emitted on the BaseWindow each time the level zero
	 * has changed because an item has been removed or inserted, or when
	 * the level zero has been saved.
	 *
	 * Signal args:
	 * - whether the level zero has changed (%TRUE), or comes to be saved
	 *   (%FALSE).
	 *
	 * Handler prototype:
	 * void ( *handler )( BaseWindow *window, gboolean is_modified, gpointer user_data );
	 */
	st_signals[ LEVEL_ZERO_CHANGED ] = g_signal_new(
			TREE_SIGNAL_LEVEL_ZERO_CHANGED,
			NACT_TYPE_TREE_VIEW,
			G_SIGNAL_RUN_LAST,
			0,					/* no default handler */
			NULL,
			NULL,
			NULL,
			G_TYPE_NONE,
			1,
			G_TYPE_BOOLEAN );

	/**
	 * NactTreeView::tree-signal-modified-status-changed:
	 *
	 * This signal is emitted on the BaseWindow when the view detects that
	 * the count of modified FMAObjectItems has changed, when an item is
	 * deleted, or when the level zero is changed.
	 *
	 * The signal is actually emitted by the NactTreeIEditable interface
	 * which takes care of all edition features.
	 *
	 * Signal args:
	 * - the new modification status of the tree
	 *
	 * Handler prototype:
	 * void ( *handler )( BaseWindow *window, gboolean is_modified, gpointer user_data )
	 */
	st_signals[ MODIFIED_STATUS ] = g_signal_new(
			TREE_SIGNAL_MODIFIED_STATUS_CHANGED,
			NACT_TYPE_TREE_VIEW,
			G_SIGNAL_RUN_LAST,
			0,
			NULL,
			NULL,
			NULL,
			G_TYPE_NONE,
			1,
			G_TYPE_BOOLEAN );

	/**
	 * NactTreeView::tree-selection-changed:
	 *
	 * This signal is emitted on the treeview each time the selection
	 * has changed after having set the current item/profile/context
	 * properties.
	 *
	 * This way, we are sure that notebook edition tabs which required
	 * to have a current item/profile/context will have it, whenever
	 * they have connected to this 'selection-changed' signal.
	 *
	 * Signal args:
	 * - a #GList of currently selected #FMAObjectItems.
	 *
	 * Handler prototype:
	 *   void handler( NactTreeView *tview,
	 *   				GList       *selected,
	 *   				void        *user_data );
	 */
	st_signals[ SELECTION_CHANGED ] = g_signal_new_class_handler(
			TREE_SIGNAL_SELECTION_CHANGED,
			NACT_TYPE_TREE_VIEW,
			G_SIGNAL_RUN_CLEANUP,
			G_CALLBACK( on_selection_changed_cleanup_handler ),
			NULL,
			NULL,
			NULL,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER );

	/**
	 * NactTreeView::tree-signal-open-popup
	 *
	 * This signal is emitted on the treeview when the user right
	 * clicks somewhere (on an active zone).
	 *
	 * Signal args:
	 * - the GdkEvent
	 *
	 * Handler prototype:
	 *   void handler( NactTreeView *tview,
	 *   				GdkEvent    *event,
	 *   				void        *user_data );
	 */
	st_signals[ OPEN_POPUP ] = g_signal_new(
			TREE_SIGNAL_CONTEXT_MENU,
			NACT_TYPE_TREE_VIEW,
			G_SIGNAL_RUN_LAST,
			0,
			NULL,
			NULL,
			NULL,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER );
}

static void
tree_ieditable_iface_init( NactTreeIEditableInterface *iface, void *user_data )
{
	static const gchar *thisfn = "fma_main_window_tree_ieditable_iface_init";

	g_debug( "%s: iface=%p, user_data=%p", thisfn, ( void * ) iface, ( void * ) user_data );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_tree_view_instance_init";
	NactTreeView *self;

	g_return_if_fail( NACT_IS_TREE_VIEW( instance ));

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self = NACT_TREE_VIEW( instance );

	self->private = g_new0( NactTreeViewPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "nact_tree_view_instance_dispose";
	NactTreeView *self;

	g_return_if_fail( NACT_IS_TREE_VIEW( object ));

	self = NACT_TREE_VIEW( object );

	if( !self->private->dispose_has_run ){
		g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

		self->private->dispose_has_run = TRUE;

		if( self->private->mode == TREE_MODE_EDITION ){
			nact_tree_ieditable_terminate( NACT_TREE_IEDITABLE( self ));
		}

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *instance )
{
	static const gchar *thisfn = "nact_tree_view_instance_finalize";
	NactTreeView *self;

	g_return_if_fail( NACT_IS_TREE_VIEW( instance ));

	g_debug( "%s: instance=%p (%s)", thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	self = NACT_TREE_VIEW( instance );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( instance );
	}
}

/**
 * nact_tree_view_new:
 *
 * Returns: a newly allocated NactTreeView object, which will be owned
 * by the caller. It is useless to unref it as it will be automatically
 * destroyed at @window finalization.
 */
NactTreeView *
nact_tree_view_new( FMAMainWindow *main_window )
{
	NactTreeView *view;

	view = g_object_new( NACT_TYPE_TREE_VIEW, NULL );
	view->private->window = main_window;

	initialize_gtk( view );

	/* delay all other signal connections until the widget be realized */
	g_signal_connect( view, "realize", G_CALLBACK( on_tree_view_realized ), NULL );

	return( view );
}

static void
initialize_gtk( NactTreeView *view )
{
	static const gchar *thisfn = "nact_tree_view_initialize_gtk";
	NactTreeViewPrivate *priv;
	GtkWidget *scrolled, *tview;
	NactTreeModel *model;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;

	g_debug( "%s: view=%p", thisfn, ( void * ) view );

	priv = view->private;

	scrolled = gtk_scrolled_window_new( NULL, NULL );
	gtk_container_add( GTK_CONTAINER( view ), scrolled );
	gtk_scrolled_window_set_shadow_type( GTK_SCROLLED_WINDOW( scrolled ), GTK_SHADOW_IN );

	tview = gtk_tree_view_new();
	gtk_widget_set_hexpand( tview, TRUE );
	gtk_widget_set_vexpand( tview, TRUE );
	gtk_container_add( GTK_CONTAINER( scrolled ), tview );
	priv->tree_view = GTK_TREE_VIEW( tview );
	gtk_tree_view_set_headers_visible( priv->tree_view, FALSE );

	model = nact_tree_model_new( GTK_TREE_VIEW( tview ));
	nact_tree_model_set_main_window( model, priv->window );
	gtk_tree_view_set_model( GTK_TREE_VIEW( tview ), GTK_TREE_MODEL( model ));
	g_object_unref( model );

	/* create visible columns on the tree view
	 */
	column = gtk_tree_view_column_new_with_attributes(
			"icon",
			gtk_cell_renderer_pixbuf_new(),
			"pixbuf", TREE_COLUMN_ICON,
			NULL );
	gtk_tree_view_append_column( GTK_TREE_VIEW( tview ), column );

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
			"label",
			renderer,
			"text", TREE_COLUMN_LABEL,
			NULL );
	gtk_tree_view_column_set_sort_column_id( column, TREE_COLUMN_LABEL );
	gtk_tree_view_append_column( GTK_TREE_VIEW( tview ), column );

	/* allow multiple selection
	 */
	selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( tview ));
	gtk_tree_selection_set_mode( selection, GTK_SELECTION_MULTIPLE );
	g_signal_connect( selection, "changed", G_CALLBACK( on_selection_changed ), view );

	/* misc properties
	 */
	gtk_tree_view_set_enable_tree_lines( GTK_TREE_VIEW( tview ), TRUE );
}

/**
 * nact_tree_view_set_mnemonic:
 * @view: this #NactTreeView
 * @parent: a parent container of the mnemonic label
 * @widget_name: the name of the mnemonic label
 *
 * Setup the mnemonic label
 */
void
nact_tree_view_set_mnemonic( NactTreeView *view, GtkContainer *parent, const gchar *widget_name )
{
	NactTreeViewPrivate *priv;
	GtkWidget *label;

	g_return_if_fail( view && NACT_IS_TREE_VIEW( view ));
	g_return_if_fail( widget_name && g_utf8_strlen( widget_name, -1 ));

	priv = view->private;

	if( !priv->dispose_has_run ){

		label = fma_gtk_utils_find_widget_by_name( parent, widget_name );
		g_return_if_fail( label && GTK_IS_LABEL( label ));
		gtk_label_set_mnemonic_widget( GTK_LABEL( label ), GTK_WIDGET( priv->tree_view ));
	}
}

/**
 * nact_tree_view_set_edition_mode:
 * @view: this #NactTreeView
 * @mode: the edition mode
 *
 * Setup the edition mode
 */
void
nact_tree_view_set_edition_mode( NactTreeView *view, guint mode )
{
	NactTreeViewPrivate *priv;
	GtkTreeViewColumn *column;
	GList *renderers;
	GtkCellRenderer *renderer;
	GtkTreeModel *tmodel;

	g_return_if_fail( view && NACT_IS_TREE_VIEW( view ));

	priv = view->private;

	if( !priv->dispose_has_run ){

		priv->mode = mode;

		if( priv->mode == TREE_MODE_EDITION ){

			column = gtk_tree_view_get_column( priv->tree_view, TREE_COLUMN_LABEL );
			renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( column ));
			renderer = GTK_CELL_RENDERER( renderers->data );
			gtk_tree_view_column_set_cell_data_func(
					column, renderer, ( GtkTreeCellDataFunc ) display_label, view, NULL );

			nact_tree_ieditable_initialize(
					NACT_TREE_IEDITABLE( view ), priv->tree_view, priv->window );
		}

		tmodel = gtk_tree_view_get_model( priv->tree_view );
		g_return_if_fail( tmodel && NACT_IS_TREE_MODEL( tmodel ));
		nact_tree_model_set_edition_mode( NACT_TREE_MODEL( tmodel ), mode );
	}
}

static gboolean
on_button_press_event( GtkWidget *widget, GdkEventButton *event, NactTreeView *view )
{
	gboolean stop = FALSE;

	/* single click on right button */
	if( event->type == GDK_BUTTON_PRESS && event->button == 3 ){
		do_open_popup( view, event );
		stop = TRUE;
	}

	return( stop );
}

/*
 * focus is monitored to avoid an accelerator being pressed while on a tab
 * triggers an unwaited operation on the list
 * e.g. when editing an entry field on the tab, pressing Del should _not_
 * delete current row in the list!
 */
static gboolean
on_focus_in( GtkWidget *widget, GdkEventFocus *event, NactTreeView *view )
{
	gboolean stop = FALSE;

	g_signal_emit_by_name( view, TREE_SIGNAL_FOCUS_IN );

	return( stop );
}

static gboolean
on_focus_out( GtkWidget *widget, GdkEventFocus *event, NactTreeView *view )
{
	gboolean stop = FALSE;

	g_signal_emit_by_name( view, TREE_SIGNAL_FOCUS_OUT );

	return( stop );
}

static gboolean
on_key_pressed_event( GtkWidget *widget, GdkEventKey *event, NactTreeView *view )
{
	gboolean stop = FALSE;

	if( event->keyval == NACT_KEY_Return || event->keyval == NACT_KEY_KP_Enter ){
		toggle_collapse( view );
		stop = TRUE;
	}
	if( event->keyval == NACT_KEY_Right ){
		navigate_to_child( view );
		stop = TRUE;
	}
	if( event->keyval == NACT_KEY_Left ){
		navigate_to_parent( view );
		stop = TRUE;
	}

	return( stop );
}

/*
 * triggered by the "popup-menu" signal, itself triggered by the keybindings
 */
static gboolean
on_popup_menu( GtkWidget *widget, NactTreeView *view )
{
	do_open_popup( view, NULL );
	return( TRUE );
}
/*
 * handles the "changed" signal emitted on the GtkTreeSelection
 */
static void
on_selection_changed( GtkTreeSelection *selection, NactTreeView *view )
{
	static const gchar *thisfn = "nact_tree_view_on_selection_changed";
	GList *selected_items;

	if( view->private->notify_allowed ){
		g_debug( "%s: selection=%p, view=%p", thisfn, ( void * ) selection, ( void * ) view );
		selected_items = get_selected_items( view );
		g_signal_emit_by_name( view, TREE_SIGNAL_SELECTION_CHANGED, selected_items );
	}
}

/*
 * the NactTreeView is realized
 */
static void
on_tree_view_realized( NactTreeView *treeview, void *empty )
{
	NactTreeViewPrivate *priv;

	g_debug( "nact_tree_view_on_tree_view_realized" );

	priv = treeview->private;

	/* expand/collapse with the keyboard */
	g_signal_connect(
			priv->tree_view, "key-press-event", G_CALLBACK( on_key_pressed_event ), treeview );

	/* monitors whether the focus is on the view */
	g_signal_connect(
			priv->tree_view, "focus-in-event", G_CALLBACK( on_focus_in ), treeview );

	g_signal_connect(
			priv->tree_view, "focus-out-event", G_CALLBACK( on_focus_out ), treeview );

	/* monitors mouse events */
	g_signal_connect(
			priv->tree_view, "button-press-event", G_CALLBACK( on_button_press_event ), treeview );

	g_signal_connect(
			priv->tree_view, "popup-menu", G_CALLBACK( on_popup_menu ), treeview );

	/* force the treeview to have the focus at start
	 * and select the first row if it exists
	 */
	gtk_widget_grab_focus( GTK_WIDGET( priv->tree_view ));
}

/**
 * nact_tree_view_fill:
 * @view: this #NactTreeView instance.
 *
 * Fill the tree view with the provided list of items.
 *
 * Notification of selection changes is temporary suspended during the
 * fillup of the list, so that client is not cluttered with tons of
 * selection-changed messages.
 */
void
nact_tree_view_fill( NactTreeView *view, GList *items )
{
	static const gchar *thisfn = "nact_tree_view_fill";
	NactTreeModel *model;
	gint nb_menus, nb_actions, nb_profiles;

	g_return_if_fail( NACT_IS_TREE_VIEW( view ));

	if( !view->private->dispose_has_run ){
		g_debug( "%s: view=%p, items=%p (count=%u)",
				thisfn, ( void * ) view, ( void * ) items, g_list_length( items ));

		clear_selection( view );
		view->private->notify_allowed = FALSE;
		model = NACT_TREE_MODEL( gtk_tree_view_get_model( view->private->tree_view ));
		nact_tree_model_fill( model, items );
		g_debug( "%s: nact_tree_model_ref_count=%d", thisfn, G_OBJECT( model )->ref_count );

		view->private->notify_allowed = TRUE;
		fma_object_count_items( items, &nb_menus, &nb_actions, &nb_profiles );
		g_signal_emit_by_name( view, TREE_SIGNAL_COUNT_CHANGED, TRUE, nb_menus, nb_actions, nb_profiles );
		g_signal_emit_by_name( view, TREE_SIGNAL_MODIFIED_STATUS_CHANGED, FALSE );

		select_row_at_path_by_string( view, "0" );
	}
}

/**
 * nact_tree_view_are_notify_allowed:
 * @view: this #NactTreeView instance.
 *
 * Returns: %TRUE if notifications are allowed, %FALSE else.
 */
gboolean
nact_tree_view_are_notify_allowed( const NactTreeView *view )
{
	gboolean are_allowed;

	g_return_val_if_fail( NACT_IS_TREE_VIEW( view ), FALSE );

	are_allowed = FALSE;

	if( !view->private->dispose_has_run ){

		are_allowed = view->private->notify_allowed;
	}

	return( are_allowed );
}

/**
 * nact_tree_view_set_notify_allowed:
 * @view: this #NactTreeView instance.
 * @allow: whether the notifications are to be allowed.
 */
void
nact_tree_view_set_notify_allowed( NactTreeView *view, gboolean allow )
{
	g_return_if_fail( NACT_IS_TREE_VIEW( view ));

	if( !view->private->dispose_has_run ){

		view->private->notify_allowed = allow;
	}
}

/**
 * nact_tree_view_collapse_all:
 * @view: this #NactTreeView instance.
 *
 * Collapse all the tree hierarchy.
 */
void
nact_tree_view_collapse_all( const NactTreeView *view )
{
	g_return_if_fail( NACT_IS_TREE_VIEW( view ));

	if( !view->private->dispose_has_run ){

		gtk_tree_view_collapse_all( view->private->tree_view );
	}
}

/**
 * nact_tree_view_expand_all:
 * @view: this #NactTreeView instance.
 *
 * Collapse all the tree hierarchy.
 */
void
nact_tree_view_expand_all( const NactTreeView *view )
{
	g_return_if_fail( NACT_IS_TREE_VIEW( view ));

	if( !view->private->dispose_has_run ){

		gtk_tree_view_expand_all( view->private->tree_view );
	}
}

/**
 * nact_tree_view_get_item_by_id:
 * @view: this #NactTreeView instance.
 * @id: the searched #FMAObjectItem.
 *
 * Returns: a pointer on the searched #FMAObjectItem if it exists, or %NULL.
 *
 * The returned pointer is owned by the underlying tree store, and should
 * not be released by the caller.
 */
FMAObjectItem *
nact_tree_view_get_item_by_id( const NactTreeView *view, const gchar *id )
{
	FMAObjectItem *item;
	NactTreeModel *model;

	g_return_val_if_fail( NACT_IS_TREE_VIEW( view ), NULL );

	item = NULL;

	if( !view->private->dispose_has_run ){

		model = NACT_TREE_MODEL( gtk_tree_view_get_model( view->private->tree_view ));
		item = nact_tree_model_get_item_by_id( model, id );
	}

	return( item );
}

/**
 * nact_tree_view_get_items:
 * @view: this #NactTreeView instance.
 *
 * Returns: the content of the current tree as a newly allocated list
 * which should be fma_object_free_items() by the caller.
 */
GList *
nact_tree_view_get_items( const NactTreeView *view )
{
	return( nact_tree_view_get_items_ex( view, TREE_LIST_ALL ));
}

/**
 * nact_tree_view_get_items_ex:
 * @view: this #NactTreeView instance.
 * @mode: the list content
 *
 * Returns: the content of the current tree as a newly allocated list
 * which should be fma_object_free_items() by the caller.
 */
GList *
nact_tree_view_get_items_ex( const NactTreeView *view, guint mode )
{
	GList *items;
	NactTreeModel *model;
	GList *deleted;

	g_return_val_if_fail( NACT_IS_TREE_VIEW( view ), NULL );

	items = NULL;

	if( !view->private->dispose_has_run ){

		deleted = NULL;

		if( view->private->mode == TREE_MODE_EDITION ){
			if( mode & TREE_LIST_DELETED ){
				deleted = nact_tree_ieditable_get_deleted( NACT_TREE_IEDITABLE( view ));
			}
		}

		model = NACT_TREE_MODEL( gtk_tree_view_get_model( view->private->tree_view ));
		items = nact_tree_model_get_items( model, mode );

		items = g_list_concat( items, deleted );
	}

	return( items );
}

/**
 * nact_tree_view_select_row_at_path:
 * @view: this #NactTreeView object.
 * @path: the #GtkTreePath to be selected.
 *
 * Select the row at the required path, or the immediate previous, or
 * the next following, or eventually the immediate parent.
 *
 * If nothing can be selected (and notify is allowed), at least send a
 * message with an empty selection.
 */
void
nact_tree_view_select_row_at_path( NactTreeView *view, GtkTreePath *path )
{
	static const gchar *thisfn = "nact_tree_view_select_row_at_path";
	gchar *path_str;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gboolean something = FALSE;

	g_return_if_fail( NACT_IS_TREE_VIEW( view ));

	if( !view->private->dispose_has_run ){

		path_str = gtk_tree_path_to_string( path );
		g_debug( "%s: view=%p, path=%s", thisfn, ( void * ) view, path_str );
		g_free( path_str );

		if( path ){
			gtk_tree_view_expand_to_path( view->private->tree_view, path );
			model = gtk_tree_view_get_model( view->private->tree_view );

			if( gtk_tree_model_get_iter( model, &iter, path )){
				gtk_tree_view_set_cursor( view->private->tree_view, path, NULL, FALSE );
				something = TRUE;

			} else if( gtk_tree_path_prev( path ) && gtk_tree_model_get_iter( model, &iter, path )){
				gtk_tree_view_set_cursor( view->private->tree_view, path, NULL, FALSE );
				something = TRUE;

			} else {
				gtk_tree_path_next( path );
				if( gtk_tree_model_get_iter( model, &iter, path )){
					gtk_tree_view_set_cursor( view->private->tree_view, path, NULL, FALSE );
					something = TRUE;

				} else if( gtk_tree_path_get_depth( path ) > 1 &&
							gtk_tree_path_up( path ) &&
							gtk_tree_model_get_iter( model, &iter, path )){

								gtk_tree_view_set_cursor( view->private->tree_view, path, NULL, FALSE );
								something = TRUE;
				}
			}
		}

		if( !something ){
			if( view->private->notify_allowed ){
				g_signal_emit_by_name( view, TREE_SIGNAL_SELECTION_CHANGED, NULL );
			}
		}
	}
}

static void
clear_selection( NactTreeView *view )
{
	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection( view->private->tree_view );
	gtk_tree_selection_unselect_all( selection );
}

/*
 * signal cleanup handler
 */
static void
on_selection_changed_cleanup_handler( NactTreeView *tview, GList *selected_items )
{
	static const gchar *thisfn = "nact_tree_view_on_selection_changed_cleanup_handler";

	g_debug( "%s: tview=%p, selected_items=%p (count=%u)",
			thisfn, ( void * ) tview,
			( void * ) selected_items, g_list_length( selected_items ));

	fma_object_free_items( selected_items );
}

/*
 * item modified: italic
 * item not saveable (invalid): red
 */
static void
display_label( GtkTreeViewColumn *column, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, NactTreeView *view )
{
	FMAObject *object;
	gchar *label;

	g_return_if_fail( view->private->mode == TREE_MODE_EDITION );

	gtk_tree_model_get( model, iter, TREE_COLUMN_NAOBJECT, &object, -1 );

	if( object ){
		g_object_unref( object );
		g_return_if_fail( FMA_IS_OBJECT( object ));

		label = fma_object_get_label( object );
		g_object_set( cell, "style-set", FALSE, NULL );
		g_object_set( cell, "foreground-set", FALSE, NULL );

		if( fma_object_is_modified( object )){
			g_object_set( cell, "style", PANGO_STYLE_ITALIC, "style-set", TRUE, NULL );
		}

		if( !fma_object_is_valid( object )){
			g_object_set( cell, "foreground", "Red", "foreground-set", TRUE, NULL );
		}

		g_object_set( cell, "text", label, NULL );
		g_free( label );
	}
}

/*
 * when expanding a selected row which has children
 */
static void
extend_selection_to_children( NactTreeView *view, GtkTreeModel *model, GtkTreeIter *parent )
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	gboolean ok;

	selection = gtk_tree_view_get_selection( view->private->tree_view );
	ok = gtk_tree_model_iter_children( model, &iter, parent );

	while( ok ){
		GtkTreePath *path = gtk_tree_model_get_path( model, &iter );
		gtk_tree_selection_select_path( selection, path );
		gtk_tree_path_free( path );
		ok = gtk_tree_model_iter_next( model, &iter );
	}
}

/*
 * get_selected_items:
 * @view: this #NactTreeView instance.
 *
 * We acquire here a new reference on objects corresponding to actually
 * selected rows, and their children.
 *
 * Returns: the currently selected rows as a #GList of #FMAObjectItems
 * which should be fma_object_free_items() by the caller.
 */
static GList *
get_selected_items( NactTreeView *view )
{
	static const gchar *thisfn = "nact_tree_view_get_selected_items";
	GList *items;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	GList *it, *listrows;
	FMAObjectId *object;

	items = NULL;
	selection = gtk_tree_view_get_selection( view->private->tree_view );
	listrows = gtk_tree_selection_get_selected_rows( selection, &model );

	for( it = listrows ; it ; it = it->next ){
		path = ( GtkTreePath * ) it->data;
		gtk_tree_model_get_iter( model, &iter, path );
		gtk_tree_model_get( model, &iter, TREE_COLUMN_NAOBJECT, &object, -1 );
		items = g_list_prepend( items, fma_object_ref( object ));
		g_object_unref( object );
		g_debug( "%s: object=%p (%s) ref_count=%d",
				thisfn,
				( void * ) object, G_OBJECT_TYPE_NAME( object ), G_OBJECT( object )->ref_count );
	}

	g_list_foreach( listrows, ( GFunc ) gtk_tree_path_free, NULL );
	g_list_free( listrows );

	return( g_list_reverse( items ));
}

static void
iter_on_selection( NactTreeView *view, FnIterOnSelection fn_iter, gpointer user_data )
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *listrows, *ipath;
	GtkTreePath *path;
	GtkTreeIter iter;
	FMAObject *object;
	gboolean stop;

	stop = FALSE;
	selection = gtk_tree_view_get_selection( view->private->tree_view );
	listrows = gtk_tree_selection_get_selected_rows( selection, &model );
	listrows = g_list_reverse( listrows );

	for( ipath = listrows ; !stop && ipath ; ipath = ipath->next ){
		path = ( GtkTreePath * ) ipath->data;
		gtk_tree_model_get_iter( model, &iter, path );
		gtk_tree_model_get( model, &iter, TREE_COLUMN_NAOBJECT, &object, -1 );

		stop = fn_iter( view, model, &iter, object, user_data );

		g_object_unref( object );
	}

	g_list_foreach( listrows, ( GFunc ) gtk_tree_path_free, NULL );
	g_list_free( listrows );
}

/*
 * navigate_to_child:
 * @view: this #NactTreeView object.
 *
 * On right arrow, if collapsed, then expand
 * if already expanded, then goto first child
 *
 * Note:
 * a row which does not have any child is always considered as collapsed;
 * trying to expand it has no visible effect.
 */
static void
navigate_to_child( NactTreeView *view )
{
	GtkTreeSelection *selection;
	GList *listrows;
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	GtkTreePath *child_path;

	selection = gtk_tree_view_get_selection( view->private->tree_view );
	listrows = gtk_tree_selection_get_selected_rows( selection, &model );

	if( g_list_length( listrows ) == 1 ){
		path = ( GtkTreePath * ) listrows->data;

		if( !gtk_tree_view_row_expanded( view->private->tree_view, path )){
			gtk_tree_view_expand_row( view->private->tree_view, path, FALSE );

		} else {
			gtk_tree_model_get_iter( model, &iter, path );

			if( gtk_tree_model_iter_has_child( model, &iter )){
				child_path = gtk_tree_path_copy( path );
				gtk_tree_path_append_index( child_path, 0 );
				nact_tree_view_select_row_at_path( view, child_path );
				gtk_tree_path_free( child_path );
			}
		}
	}

	g_list_foreach( listrows, ( GFunc ) gtk_tree_path_free, NULL );
	g_list_free( listrows );
}

/*
 * navigate_to_parent:
 * @view: this #NactTreeView object.
 *
 * On left arrow, go to the parent.
 * if already on a parent, collapse it
 * if already collapsed, up to the next parent
 *
 * e.g. path="2:0", depth=2
 * this means that depth is always >= 1, with depth=1 being the root.
 */
static void
navigate_to_parent( NactTreeView *view )
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *listrows;
	GtkTreePath *path;
	GtkTreePath *parent_path;

	selection = gtk_tree_view_get_selection( view->private->tree_view );
	listrows = gtk_tree_selection_get_selected_rows( selection, &model );

	if( g_list_length( listrows ) == 1 ){
		path = ( GtkTreePath * ) listrows->data;

		if( gtk_tree_view_row_expanded( view->private->tree_view, path )){
			gtk_tree_view_collapse_row( view->private->tree_view, path );

		} else if( gtk_tree_path_get_depth( path ) > 1 ){
			parent_path = gtk_tree_path_copy( path );
			gtk_tree_path_up( parent_path );
			nact_tree_view_select_row_at_path( view, parent_path );
			gtk_tree_path_free( parent_path );
		}
	}

	g_list_foreach( listrows, ( GFunc ) gtk_tree_path_free, NULL );
	g_list_free( listrows );
}

static void
do_open_popup( NactTreeView *view, GdkEventButton *event )
{
	NactTreeViewPrivate *priv;
	GtkTreePath *path;

	priv = view->private;

	if( event ){
		if( gtk_tree_view_get_path_at_pos( priv->tree_view, event->x, event->y, &path, NULL, NULL, NULL )){
			nact_tree_view_select_row_at_path( view, path );
			gtk_tree_path_free( path );
		}
	}

	g_signal_emit_by_name( view, TREE_SIGNAL_CONTEXT_MENU, event );
}

/*
 * nact_tree_view_select_row_at_path_by_string:
 * @view: this #NactTreeView object.
 * @path: the #GtkTreePath to be selected.
 *
 * cf. nact_tree_view_select_row_at_path().
 */
static void
select_row_at_path_by_string( NactTreeView *view, const gchar *path_str )
{
	GtkTreePath *path;

	path = gtk_tree_path_new_from_string( path_str );
	nact_tree_view_select_row_at_path( view, path );
	gtk_tree_path_free( path );
}

/*
 * Toggle or collapse the current subtree.
 */
static void
toggle_collapse( NactTreeView *view )
{
	guint toggle = TOGGLE_UNDEFINED;

	iter_on_selection( view, ( FnIterOnSelection ) toggle_collapse_iter, &toggle );
}

static gboolean
toggle_collapse_iter( NactTreeView *view, GtkTreeModel *model,
						GtkTreeIter *iter, FMAObject *object, gpointer user_data )
{
	guint count;
	guint *toggle;

	toggle = ( guint * ) user_data;

	if( FMA_IS_OBJECT_ITEM( object )){
		GtkTreePath *path = gtk_tree_model_get_path( model, iter );
		count = fma_object_get_items_count( object );

		if(( count > 1 && FMA_IS_OBJECT_ACTION( object )) ||
			( count > 0 && FMA_IS_OBJECT_MENU( object ))){

			toggle_collapse_row( view->private->tree_view, path, toggle );
		}

		gtk_tree_path_free( path );

		/* do not extend selection to children */
		if( 0 ){
			if( *toggle == TOGGLE_EXPAND ){
				extend_selection_to_children( view, model, iter );
			}
		}
	}

	/* do not stop iteration */
	return( FALSE );
}

/*
 * toggle mode can be undefined, collapse or expand
 * it is set on the first row
 */
static void
toggle_collapse_row( GtkTreeView *treeview, GtkTreePath *path, guint *toggle )
{
	if( *toggle == TOGGLE_UNDEFINED ){
		*toggle = gtk_tree_view_row_expanded( treeview, path ) ? TOGGLE_COLLAPSE : TOGGLE_EXPAND;
	}

	if( *toggle == TOGGLE_COLLAPSE ){
		if( gtk_tree_view_row_expanded( treeview, path )){
			gtk_tree_view_collapse_row( treeview, path );
		}
	} else {
		if( !gtk_tree_view_row_expanded( treeview, path )){
			gtk_tree_view_expand_row( treeview, path, TRUE );
		}
	}
}
