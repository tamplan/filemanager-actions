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

#include <api/na-object-api.h>

#include "base-keysyms.h"
#include "nact-application.h"
#include "nact-marshal.h"
#include "nact-tree-view.h"
#include "nact-tree-model.h"

/* private class data
 */
struct _NactTreeViewClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _NactTreeViewPrivate {
	gboolean       dispose_has_run;

	/* properties set at instanciation time
	 */
	BaseWindow    *window;
	gchar         *widget_name;
	guint          mode;

	/* runtime data
	 */
	GtkTreeView   *tree_view;
	gboolean       notify_allowed;
};

/* instance properties
 */
enum {
	TREE_PROP_0,

	TREE_PROP_WINDOW_ID,
	TREE_PROP_WIDGET_NAME_ID,
	TREE_PROP_MODE_ID,

	TREE_PROP_N_PROPERTIES
};

/* signals
 */
enum {
	CONTENT_CHANGED,
	CONTEXT_MENU,
	COUNT_CHANGED,
	FOCUS_IN,
	FOCUS_OUT,
	MODIFIED_COUNT,
	SELECTION_CHANGED,
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
typedef gboolean ( *FnIterOnSelection )( NactTreeView *, GtkTreeModel *, GtkTreeIter *, NAObject *, gpointer );

#define WINDOW_DATA_TREE_VIEW				"window-data-tree-view"

#define VIEW_WINDOW_VOID( window ) \
	g_return_if_fail( BASE_IS_WINDOW( window )); \
	NactTreeView *view = ( NactTreeView * ) g_object_get_data( G_OBJECT( window ), WINDOW_DATA_TREE_VIEW ); \
	g_return_if_fail( NACT_IS_TREE_VIEW( view ));

#define VIEW_WINDOW_VALUE( window, value ) \
	g_return_val_if_fail( BASE_IS_WINDOW( window ), value ); \
	NactTreeView *view = ( NactTreeView * ) g_object_get_data( G_OBJECT( window ), WINDOW_DATA_TREE_VIEW ); \
	g_return_val_if_fail( NACT_IS_TREE_VIEW( view ), value );

static gint          st_signals[ LAST_SIGNAL ] = { 0 };
static GObjectClass *st_parent_class           = NULL;

static GType    register_type( void );
static void     class_init( NactTreeViewClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void     instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void     instance_constructed( GObject *object );
static void     instance_dispose( GObject *application );
static void     instance_finalize( GObject *application );

static void     on_base_initialize_gtk( BaseWindow *window, GtkWindow *toplevel, gpointer user_data );
static void     on_base_initialize_view( BaseWindow *window, gpointer user_data );
static void     on_base_all_widgets_showed( BaseWindow *window, gpointer user_data );
static gboolean on_button_press_event( GtkWidget *widget, GdkEventButton *event, BaseWindow *window );
static gboolean on_focus_in( GtkWidget *widget, GdkEventFocus *event, BaseWindow *window );
static gboolean on_focus_out( GtkWidget *widget, GdkEventFocus *event, BaseWindow *window );
static gboolean on_key_pressed_event( GtkWidget *widget, GdkEventKey *event, BaseWindow *window );
static void     on_treeview_selection_changed( GtkTreeSelection *selection, BaseWindow *window );
static void     on_content_changed_cleanup_handler( BaseWindow *window, NactTreeView *view, NAObject *edited );
static void     on_selection_changed_cleanup_handler( BaseWindow *window, NactTreeView *view, GList *selected_items );
static void     clear_selection( NactTreeView *view );
static void     extend_selection_to_children( NactTreeView *view, GtkTreeModel *model, GtkTreeIter *parent );
static GList   *get_selected_items( NactTreeView *view );
static void     iter_on_selection( NactTreeView *view, FnIterOnSelection fn_iter, gpointer user_data );
static void     navigate_to_child( NactTreeView *view );
static void     navigate_to_parent( NactTreeView *view );
static void     open_popup( BaseWindow *window, NactTreeView *view, GdkEventButton *event );
static void     select_row_at_path( NactTreeView *view, GtkTreePath *path );
static void     select_row_at_path_by_string( NactTreeView *view, const gchar *path );
static void     toggle_collapse( NactTreeView *view );
static gboolean toggle_collapse_iter( NactTreeView *view, GtkTreeModel *model, GtkTreeIter *iter, NAObject *object, gpointer user_data );
static void     toggle_collapse_row( GtkTreeView *treeview, GtkTreePath *path, guint *toggle );
static void     on_finalizing_window( NactTreeView *view, GObject *window );

GType
nact_tree_view_get_type( void )
{
	static GType type = 0;

	if( !type ){
		type = register_type();
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

	type = g_type_register_static( G_TYPE_OBJECT, "NactTreeView", &info, 0 );

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
	object_class->get_property = instance_get_property;
	object_class->set_property = instance_set_property;
	object_class->constructed = instance_constructed;
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	g_object_class_install_property( object_class, TREE_PROP_WINDOW_ID,
			g_param_spec_pointer(
					TREE_PROP_WINDOW,
					"BaseWindow",
					"The BaseWindow parent",
					G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	g_object_class_install_property( object_class, TREE_PROP_WIDGET_NAME_ID,
			g_param_spec_string(
					TREE_PROP_WIDGET_NAME,
					"Widget name",
					"The name of GtkTreeView widget",
					"",
					G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	g_object_class_install_property( object_class, TREE_PROP_MODE_ID,
			g_param_spec_uint(
					TREE_PROP_MODE,
					"Management mode",
					"Management mode of the tree view, selection or edition",
					0,
					TREE_MODE_N_MODES,
					TREE_MODE_SELECTION,
					G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	/**
	 * NactTreeView::tree-signal-content-changed:
	 *
	 * This signal is emitted on the BaseWindow parent when the content
	 * of the tree changes. It may change in two occurrences:
	 * - the whole model is refilled with a new items list;
	 * - a row is edited.
	 *
	 * Signal args:
	 * - a pointer on the view
	 * - a poointer on the edited NAObject element, or NULL if the whole
	 *   list has changed.
	 *
	 * Handler prototype:
	 * void ( *handler )( BaseWindow *window, NactTreeView *view, NAObject *edited, gpointer user_data );
	 */
	st_signals[ CONTENT_CHANGED ] = g_signal_new_class_handler(
			TREE_SIGNAL_CONTENT_CHANGED,
			G_TYPE_OBJECT,
			G_SIGNAL_RUN_CLEANUP,
			G_CALLBACK( on_content_changed_cleanup_handler ),
			NULL,
			NULL,
			nact_cclosure_marshal_VOID__POINTER_POINTER,
			G_TYPE_NONE,
			2,
			G_TYPE_POINTER, G_TYPE_POINTER );

	/**
	 * NactTreeView::tree-signal-open-popup
	 *
	 * This signal is emitted on the BaseWindow parent when the user right
	 * clicks on the tree view.
	 *
	 * Signal args:
	 * - a pointer on the view
	 * - the GdkEvent.
	 *
	 * Handler prototype:
	 * void ( *handler )( BaseWindow *window, NactTreeView *view, GdkEvent *event, gpointer user_data );
	 */
	st_signals[ CONTEXT_MENU ] = g_signal_new(
			TREE_SIGNAL_CONTEXT_MENU,
			G_TYPE_OBJECT,
			G_SIGNAL_RUN_LAST,
			0,
			NULL,
			NULL,
			nact_cclosure_marshal_VOID__POINTER_POINTER,
			G_TYPE_NONE,
			2,
			G_TYPE_POINTER, G_TYPE_POINTER );

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
	 * - a pointer on the view
	 * - reset if %TRUE, add if %FALSE
	 * - menus count/increment (may be negative)
	 * - actions count/increment (may be negative)
	 * - profiles count/increment (may be negative)
	 *
	 * Handler prototype:
	 * void ( *handler )( BaseWindow *window, NactTreeView *view, gboolean reset,
	 *                    guint menus_count, guint actions_count, guint profiles_count, gpointer user_data );
	 */
	st_signals[ COUNT_CHANGED ] = g_signal_new(
			TREE_SIGNAL_COUNT_CHANGED,
			G_TYPE_OBJECT,
			G_SIGNAL_RUN_LAST,
			0,						/* no default handler */
			NULL,
			NULL,
			nact_cclosure_marshal_VOID__POINTER_BOOLEAN_INT_INT_INT,
			G_TYPE_NONE,
			5,
			G_TYPE_POINTER, G_TYPE_BOOLEAN, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT );

	/**
	 * NactTreeView::tree-signal-focus-in:
	 *
	 * This signal is emitted on the BaseWindow when the view gains the
	 * focus. In particular, edition menu is disabled outside of the treeview.
	 *
	 * Signal args:
	 * - a pointer on the view
	 *
	 * Handler prototype:
	 * void ( *handler )( BaseWindow *window, NactTreeView *view, gpointer user_data )
	 */
	st_signals[ FOCUS_IN ] = g_signal_new(
			TREE_SIGNAL_FOCUS_IN,
			G_TYPE_OBJECT,
			G_SIGNAL_RUN_LAST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER );

	/**
	 * NactTreeView::tree-signal-focus-out:
	 *
	 * This signal is emitted on the BaseWindow when the view loses the
	 * focus. In particular, edition menu is disabled outside of the treeview.
	 *
	 * Signal args:
	 * - a pointer on the view
	 *
	 * Handler prototype:
	 * void ( *handler )( BaseWindow *window, NactTreeView *view, gpointer user_data )
	 */
	st_signals[ FOCUS_OUT ] = g_signal_new(
			TREE_SIGNAL_FOCUS_OUT,
			G_TYPE_OBJECT,
			G_SIGNAL_RUN_LAST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER );

	/**
	 * NactTreeView::tree-signal-modified-count-changed:
	 *
	 * This signal is emitted on the BaseWindow when the view detects that
	 * the count of modified NAObjectItems has changed.
	 * The signal is actually emitted by the NactTreeIEditable interface
	 * which takes care of all edition features.
	 *
	 * Having the order of items at level zero changed counts for one.
	 *
	 * Signal args:
	 * - a pointer on the view
	 * - the new count of modified items
	 *
	 * Handler prototype:
	 * void ( *handler )( BaseWindow *window, NactTreeView *view, guint count, gpointer user_data )
	 */
	st_signals[ MODIFIED_COUNT ] = g_signal_new(
			TREE_SIGNAL_MODIFIED_COUNT_CHANGED,
			G_TYPE_OBJECT,
			G_SIGNAL_RUN_LAST,
			0,
			NULL,
			NULL,
			nact_cclosure_marshal_VOID__POINTER_INT,
			G_TYPE_NONE,
			2,
			G_TYPE_POINTER, G_TYPE_INT );

	/**
	 * NactTreeView::tree-signal-selection-changed:
	 *
	 * This signal is emitted on the BaseWindow parent each time the selection
	 * has changed in the treeview.
	 *
	 * Signal args:
	 * - a pointer on the view
	 * - a #GList of currently selected #NAObjectItems.
	 *
	 * Handler prototype:
	 * void ( *handler )( BaseWindow *window, NactTreeView *view, GList *selected, gpointer user_data );
	 */
	st_signals[ SELECTION_CHANGED ] = g_signal_new_class_handler(
			TREE_SIGNAL_SELECTION_CHANGED,
			G_TYPE_OBJECT,
			G_SIGNAL_RUN_CLEANUP,
			G_CALLBACK( on_selection_changed_cleanup_handler ),
			NULL,
			NULL,
			nact_cclosure_marshal_VOID__POINTER_POINTER,
			G_TYPE_NONE,
			2,
			G_TYPE_POINTER, G_TYPE_POINTER );

	klass->private = g_new0( NactTreeViewClassPrivate, 1 );
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
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	NactTreeView *self;

	g_return_if_fail( NACT_IS_TREE_VIEW( object ));
	self = NACT_TREE_VIEW( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case TREE_PROP_WINDOW_ID:
				g_value_set_pointer( value, self->private->window );
				break;

			case TREE_PROP_WIDGET_NAME_ID:
				g_value_set_string( value, self->private->widget_name );
				break;

			case TREE_PROP_MODE_ID:
				g_value_set_uint( value, self->private->mode );
				break;

			default:
				G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
				break;
		}
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	NactTreeView *self;

	g_return_if_fail( NACT_IS_TREE_VIEW( object ));
	self = NACT_TREE_VIEW( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case TREE_PROP_WINDOW_ID:
				self->private->window = g_value_get_pointer( value );
				break;

			case TREE_PROP_WIDGET_NAME_ID:
				g_free( self->private->widget_name );
				self->private->widget_name = g_value_dup_string( value );
				break;

			case TREE_PROP_MODE_ID:
				self->private->mode = g_value_get_uint( value );
				break;

			default:
				G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
				break;
		}
	}
}

static void
instance_constructed( GObject *object )
{
	static const gchar *thisfn = "nact_tree_view_instance_constructed";
	NactTreeView *self;

	g_return_if_fail( NACT_IS_TREE_VIEW( object ));
	self = NACT_TREE_VIEW( object );

	if( !self->private->dispose_has_run ){
		g_debug( "%s: object=%p", thisfn, ( void * ) object );

		base_window_signal_connect( self->private->window,
				G_OBJECT( self->private->window ), BASE_SIGNAL_INITIALIZE_GTK, G_CALLBACK( on_base_initialize_gtk ));

		base_window_signal_connect( self->private->window,
				G_OBJECT( self->private->window ), BASE_SIGNAL_INITIALIZE_WINDOW, G_CALLBACK( on_base_initialize_view ));

		base_window_signal_connect( self->private->window,
				G_OBJECT( self->private->window ), BASE_SIGNAL_ALL_WIDGETS_SHOWED, G_CALLBACK( on_base_all_widgets_showed ));

		g_object_set_data( G_OBJECT( self->private->window ), WINDOW_DATA_TREE_VIEW, self );

		g_object_weak_ref( G_OBJECT( self->private->window ), ( GWeakNotify ) on_finalizing_window, self );

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->constructed ){
			G_OBJECT_CLASS( st_parent_class )->constructed( object );
		}
	}
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

	g_free( self->private->widget_name );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( instance );
	}
}

/**
 * nact_tree_view_new:
 * @window: the BaseWindow which embeds the tree view.
 * @treeview_name: the GtkTreeView widget name.
 * @mode: management mode.
 *
 * Returns: a newly allocated NactTreeView object, which will be owned
 * by the caller. It is useless to unref it as it will automatically
 * auto-destroys itself at @window finalization.
 */
NactTreeView *
nact_tree_view_new( BaseWindow *window, const gchar *treeview_name, NactTreeMode mode )
{
	NactTreeView *view;

	view = g_object_new( NACT_TREE_VIEW_TYPE,
			TREE_PROP_WINDOW,      window,
			TREE_PROP_WIDGET_NAME, treeview_name,
			TREE_PROP_MODE,        mode,
			NULL );

	return( view );
}

static void
on_base_initialize_gtk( BaseWindow *window, GtkWindow *toplevel, gpointer user_data )
{
	static const gchar *thisfn = "nact_tree_view_on_base_initialize_gtk";
	GtkTreeView *treeview;
	GtkWidget *label;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;

	VIEW_WINDOW_VOID( window );

	g_debug( "%s: window=%p (%s), toplevel=%p (%s), user_data=%p",
			thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ),
			( void * ) toplevel, G_OBJECT_TYPE_NAME( toplevel ), ( void * ) user_data );

	if( !view->private->dispose_has_run ){

		treeview = GTK_TREE_VIEW( base_window_get_widget( window, view->private->widget_name ));
		nact_tree_model_new( window, treeview, view->private->mode );

		/* associates the ItemsView to the label */
		label = base_window_get_widget( window, "ActionsListLabel" );
		gtk_label_set_mnemonic_widget( GTK_LABEL( label ), GTK_WIDGET( treeview ));

		/* create visible columns on the tree view
		 */
		column = gtk_tree_view_column_new_with_attributes(
				"icon",
				gtk_cell_renderer_pixbuf_new(),
				"pixbuf", TREE_COLUMN_ICON,
				NULL );
		gtk_tree_view_append_column( treeview, column );

		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(
				"label",
				renderer,
				"text", TREE_COLUMN_LABEL,
				NULL );
		gtk_tree_view_column_set_sort_column_id( column, TREE_COLUMN_LABEL );
		gtk_tree_view_append_column( treeview, column );

		/* allow multiple selection
		 */
		selection = gtk_tree_view_get_selection( treeview );
		gtk_tree_selection_set_mode( selection, GTK_SELECTION_MULTIPLE );

		/* misc properties
		 */
		gtk_tree_view_set_enable_tree_lines( treeview, TRUE );
	}
}

static void
on_base_initialize_view( BaseWindow *window, gpointer user_data )
{
	static const gchar *thisfn = "nact_tree_view_on_base_initialize_view";
	GtkTreeView *treeview;
	GtkTreeSelection *selection;

	VIEW_WINDOW_VOID( window );

	g_debug( "%s: window=%p (%s), user_data=%p",
			thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ), ( void * ) user_data );

	if( !view->private->dispose_has_run ){

		treeview = GTK_TREE_VIEW( base_window_get_widget( window, view->private->widget_name ));
		view->private->tree_view = treeview;

		/* monitors the selection */
		selection = gtk_tree_view_get_selection( treeview );
		base_window_signal_connect( window,
				G_OBJECT( selection ), "changed", G_CALLBACK( on_treeview_selection_changed ));

		/* expand/collapse with the keyboard */
		base_window_signal_connect( window,
				G_OBJECT( treeview ), "key-press-event", G_CALLBACK( on_key_pressed_event ));

		/* monitors whether the focus is on the view */
		base_window_signal_connect( window,
				G_OBJECT( treeview ), "focus-in-event", G_CALLBACK( on_focus_in ));

		base_window_signal_connect( window,
				G_OBJECT( treeview ), "focus-out-event", G_CALLBACK( on_focus_out ));

		/* monitors mouse events */
		base_window_signal_connect( window,
				G_OBJECT( treeview ), "button-press-event", G_CALLBACK( on_button_press_event ));
	}
}

static void
on_base_all_widgets_showed( BaseWindow *window, gpointer user_data )
{
	static const gchar *thisfn = "nact_tree_view_on_base_all_widgets_showed";

	VIEW_WINDOW_VOID( window );

	g_debug( "%s: window=%p (%s), user_data=%p",
			thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ), ( void * ) user_data );

	if( !view->private->dispose_has_run ){

		/* force the treeview to have the focus at start
		 * and select the first row if it exists
		 */
		gtk_widget_grab_focus( GTK_WIDGET( view->private->tree_view ));
		view->private->notify_allowed = TRUE;
		select_row_at_path_by_string( view, "0" );
	}
}

static gboolean
on_button_press_event( GtkWidget *widget, GdkEventButton *event, BaseWindow *window )
{
	gboolean stop = FALSE;

	VIEW_WINDOW_VALUE( window, FALSE );

	/* single click on right button */
	if( event->type == GDK_BUTTON_PRESS && event->button == 3 ){
		open_popup( window, view, event );
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
on_focus_in( GtkWidget *widget, GdkEventFocus *event, BaseWindow *window )
{
	gboolean stop = FALSE;

	VIEW_WINDOW_VALUE( window, FALSE );

	g_signal_emit_by_name( window, TREE_SIGNAL_FOCUS_IN, view );

	return( stop );
}

static gboolean
on_focus_out( GtkWidget *widget, GdkEventFocus *event, BaseWindow *window )
{
	gboolean stop = FALSE;

	VIEW_WINDOW_VALUE( window, FALSE );

	g_signal_emit_by_name( window, TREE_SIGNAL_FOCUS_OUT, view );

	return( stop );
}

static gboolean
on_key_pressed_event( GtkWidget *widget, GdkEventKey *event, BaseWindow *window )
{
	gboolean stop = FALSE;

	VIEW_WINDOW_VALUE( window, FALSE );

	if( !view->private->dispose_has_run ){

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
	}

	return( stop );
}

/*
 * handles the "changed" signal emitted on the GtkTreeSelection
 */
static void
on_treeview_selection_changed( GtkTreeSelection *selection, BaseWindow *window )
{
	static const gchar *thisfn = "nact_tree_view_on_treeview_selection_changed";
	GList *selected_items;

	VIEW_WINDOW_VOID( window );

	g_debug( "%s: selection=%p, window=%p", thisfn, ( void * ) selection, ( void * ) window );

	if( !view->private->dispose_has_run ){

		if( view->private->notify_allowed ){
			selected_items = get_selected_items( view );
			g_signal_emit_by_name( window, TREE_SIGNAL_SELECTION_CHANGED, view, selected_items );
		}
	}
}

/*
 * cleanup handler for our TREE_SIGNAL_CONTENT_CHANGED signal
 */
static void
on_content_changed_cleanup_handler( BaseWindow *window, NactTreeView *view, NAObject *edited )
{
	static const gchar *thisfn = "nact_tree_view_on_content_changed_cleanup_handler";

	g_debug( "%s: window=%p, view=%p, edited=%p",
			thisfn, ( void * ) window, ( void * ) view, ( void * ) edited );

	if( edited ){
		na_object_unref( edited );
	}
}

/*
 * cleanup handler for our TREE_SIGNAL_SELECTION_CHANGED signal
 */
static void
on_selection_changed_cleanup_handler( BaseWindow *window, NactTreeView *view, GList *selected_items )
{
	static const gchar *thisfn = "nact_tree_view_on_selection_changed_cleanup_handler";

	g_debug( "%s: window=%p, view=%p, selected_items=%p (count=%u)",
			thisfn, ( void * ) window, ( void * ) view,
			( void * ) selected_items, g_list_length( selected_items ));

	na_object_free_items( selected_items );
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
	gboolean prev;
	gint nb_menus, nb_actions, nb_profiles;

	g_return_if_fail( NACT_IS_TREE_VIEW( view ));

	if( !view->private->dispose_has_run ){
		g_debug( "%s: view=%p, items=%p (count=%u)",
				thisfn, ( void * ) view, ( void * ) items, g_list_length( items ));

		clear_selection( view );
		prev = view->private->notify_allowed;
		view->private->notify_allowed = FALSE;
		model = NACT_TREE_MODEL( gtk_tree_view_get_model( view->private->tree_view ));
		nact_tree_model_fill( model, items );
		view->private->notify_allowed = prev;

		if( view->private->notify_allowed ){
			g_signal_emit_by_name( view->private->window, TREE_SIGNAL_CONTENT_CHANGED, view, NULL );

			nb_menus = nb_actions = nb_profiles = 0;
			na_object_item_count_items( items, &nb_menus, &nb_actions, &nb_profiles, TRUE );
			g_signal_emit_by_name( view->private->window,
					TREE_SIGNAL_COUNT_CHANGED, view, TRUE, nb_menus, nb_actions, nb_profiles );
		}

		select_row_at_path_by_string( view, "0" );
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
 * @id: the searched #NAObjectItem.
 *
 * Returns: a pointer on the searched #NAObjectItem if it exists, or %NULL.
 *
 * The returned pointer is owned by the underlying tree store, and should
 * not be released by the caller.
 */
NAObjectItem *
nact_tree_view_get_item_by_id( const NactTreeView *view, const gchar *id )
{
	NAObjectItem *item;
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
 * which should be na_object_free_items() by the caller.
 */
GList *
nact_tree_view_get_items( const NactTreeView *view )
{
	GList *items;
	NactTreeModel *model;

	g_return_val_if_fail( NACT_IS_TREE_VIEW( view ), NULL );

	items = NULL;

	if( !view->private->dispose_has_run ){

		model = NACT_TREE_MODEL( gtk_tree_view_get_model( view->private->tree_view ));
		items = nact_tree_model_get_items( model, TREE_LIST_ALL );
	}

	return( items );
}

static void
clear_selection( NactTreeView *view )
{
	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection( view->private->tree_view );
	gtk_tree_selection_unselect_all( selection );
}

/*
 * when expanding a selected row which has childs
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
 * Returns: the currently selected rows as a #GList of #NAObjectItems
 * which should be na_object_free_items() by the caller.
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
	NAObjectId *object;

	items = NULL;
	selection = gtk_tree_view_get_selection( view->private->tree_view );
	listrows = gtk_tree_selection_get_selected_rows( selection, &model );

	for( it = listrows ; it ; it = it->next ){
		path = ( GtkTreePath * ) it->data;
		gtk_tree_model_get_iter( model, &iter, path );
		gtk_tree_model_get( model, &iter, TREE_COLUMN_NAOBJECT, &object, -1 );
		g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));
		items = g_list_prepend( items, na_object_ref( object ));
		g_object_unref( object );
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
	NAObject *object;
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
				select_row_at_path( view, child_path );
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
			select_row_at_path( view, parent_path );
			gtk_tree_path_free( parent_path );
		}
	}

	g_list_foreach( listrows, ( GFunc ) gtk_tree_path_free, NULL );
	g_list_free( listrows );
}

static void
open_popup( BaseWindow *window, NactTreeView *view, GdkEventButton *event )
{
	GtkTreePath *path;

	if( gtk_tree_view_get_path_at_pos( view->private->tree_view, event->x, event->y, &path, NULL, NULL, NULL )){
		select_row_at_path( view, path );
		gtk_tree_path_free( path );
	}

	g_signal_emit_by_name( window, TREE_SIGNAL_CONTEXT_MENU, view, event );
}

/*
 * Select the row at the required path, or the immediate previous, or
 * the next following, or eventually the immediate parent.
 *
 * If nothing can be selected (and notify is allowed), at least send a
 * message with an empty selection.
 */
static void
select_row_at_path( NactTreeView *view, GtkTreePath *path )
{
	static const gchar *thisfn = "nact_tree_view_select_row_at_path";
	gchar *path_str;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gboolean something = FALSE;

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
			g_signal_emit_by_name( view->private->window, TREE_SIGNAL_SELECTION_CHANGED, view, NULL );
		}
	}
}

static void
select_row_at_path_by_string( NactTreeView *view, const gchar *path_str )
{
	GtkTreePath *path;

	path = gtk_tree_path_new_from_string( path_str );
	select_row_at_path( view, path );
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
						GtkTreeIter *iter, NAObject *object, gpointer user_data )
{
	guint count;
	guint *toggle;

	toggle = ( guint * ) user_data;

	if( NA_IS_OBJECT_ITEM( object )){
		GtkTreePath *path = gtk_tree_model_get_path( model, iter );
		count = na_object_get_items_count( object );

		if(( count > 1 && NA_IS_OBJECT_ACTION( object )) ||
			( count > 0 && NA_IS_OBJECT_MENU( object ))){

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

static void
on_finalizing_window( NactTreeView *view, GObject *window )
{
	static const gchar *thisfn = "nact_tree_view_on_finalizing_window";

	g_return_if_fail( NACT_IS_TREE_VIEW( view ));

	g_debug( "%s: view=%p (%s), window=%p",
			thisfn, ( void * ) view, G_OBJECT_TYPE_NAME( view ), ( void * ) window );

	g_object_unref( view );
}
