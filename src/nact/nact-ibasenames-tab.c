/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010 Pierre Wieser and others (see AUTHORS)
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

#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>

#include <api/na-object-api.h>

#include "nact-main-tab.h"
#include "nact-ibasenames-tab.h"

/* private interface data
 */
struct NactIBasenamesTabInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* column ordering
 */
enum {
	BASENAMES_ITEM_COLUMN = 0,
	BASENAMES_MUST_MATCH_COLUMN,
	BASENAMES_MUST_NOT_MATCH_COLUMN,
	BASENAMES_N_COLUMN
};

static gboolean st_initialized = FALSE;
static gboolean st_finalized = FALSE;
static gboolean st_on_selection_change = FALSE;

static GType    register_type( void );
static void     interface_base_init( NactIBasenamesTabInterface *klass );
static void     interface_base_finalize( NactIBasenamesTabInterface *klass );

static void     on_add_filter_clicked( GtkButton *button, BaseWindow *window );
static void     on_filter_clicked( GtkTreeViewColumn *treeviewcolumn, BaseWindow *window );
static void     on_filter_edited( GtkCellRendererText *renderer, const gchar *path, const gchar *text, BaseWindow *window );
static gboolean on_key_pressed_event( GtkWidget *widget, GdkEventKey *event, BaseWindow *window );
static void     on_must_match_clicked( GtkTreeViewColumn *treeviewcolumn, BaseWindow *window );
static void     on_must_match_toggled( GtkCellRendererToggle *cell_renderer, gchar *path, BaseWindow *window );
static void     on_must_not_match_clicked( GtkTreeViewColumn *treeviewcolumn, BaseWindow *window );
static void     on_must_not_match_toggled( GtkCellRendererToggle *cell_renderer, gchar *path, BaseWindow *window );
static void     on_remove_filter_clicked( GtkButton *button, BaseWindow *window );
static void     on_selection_changed( GtkTreeSelection *selection, BaseWindow *window );
static void     on_tab_updatable_selection_changed( NactIBasenamesTab *instance, gint count_selected );
static void     on_tab_updatable_enable_tab( NactIBasenamesTab *instance, NAObjectItem *item );

static void     iter_for_setup( gchar *filter, GtkTreeModel *model );
static gboolean tab_set_sensitive( NactIBasenamesTab *instance );

GType
nact_ibasenames_tab_get_type( void )
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
	static const gchar *thisfn = "nact_ibasenames_tab_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NactIBasenamesTabInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "NactIBasenamesTab", &info, 0 );

	g_type_interface_add_prerequisite( type, BASE_WINDOW_TYPE );

	return( type );
}

static void
interface_base_init( NactIBasenamesTabInterface *klass )
{
	static const gchar *thisfn = "nact_ibasenames_tab_interface_base_init";

	if( !st_initialized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NactIBasenamesTabInterfacePrivate, 1 );

		st_initialized = TRUE;
	}
}

static void
interface_base_finalize( NactIBasenamesTabInterface *klass )
{
	static const gchar *thisfn = "nact_ibasenames_tab_interface_base_finalize";

	if( st_initialized && !st_finalized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		st_finalized = TRUE;

		g_free( klass->private );
	}
}

/**
 * nact_ibasenames_tab_initial_load:
 * @window: this #NactIBasenamesTab instance.
 *
 * Initializes the tab widget at initial load.
 */
void
nact_ibasenames_tab_initial_load_toplevel( NactIBasenamesTab *instance )
{
	static const gchar *thisfn = "nact_ibasenames_tab_initial_load_toplevel";
	GtkListStore *model;
	GtkTreeView *listview;
	GtkCellRenderer *text_cell, *radio_cell;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_IBASENAMES_TAB( instance ));

	if( st_initialized && !st_finalized ){

		model = gtk_list_store_new( BASENAMES_N_COLUMN, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN );
		listview = GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( instance ), "BasenamesTreeView" ));
		gtk_tree_view_set_model( listview, GTK_TREE_MODEL( model ));
		g_object_unref( model );

		text_cell = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(
				_( "Basename filter" ),
				text_cell,
				"text", BASENAMES_ITEM_COLUMN,
				NULL );
		gtk_tree_view_append_column( listview, column );
		gtk_tree_sortable_set_sort_column_id( GTK_TREE_SORTABLE( model ), BASENAMES_ITEM_COLUMN, GTK_SORT_ASCENDING );

		radio_cell = gtk_cell_renderer_toggle_new();
		gtk_cell_renderer_toggle_set_radio( GTK_CELL_RENDERER_TOGGLE( radio_cell ), TRUE );
		column = gtk_tree_view_column_new_with_attributes(
				_( "Must match one" ),
				radio_cell,
				"active", BASENAMES_MUST_MATCH_COLUMN,
				NULL );
		gtk_tree_view_append_column( listview, column );

		radio_cell = gtk_cell_renderer_toggle_new();
		gtk_cell_renderer_toggle_set_radio( GTK_CELL_RENDERER_TOGGLE( radio_cell ), TRUE );
		column = gtk_tree_view_column_new_with_attributes(
				_( "Must not match any" ),
				radio_cell,
				"active", BASENAMES_MUST_NOT_MATCH_COLUMN,
				NULL );
		gtk_tree_view_append_column( listview, column );

		column = gtk_tree_view_column_new();
		gtk_tree_view_append_column( listview, column );

		gtk_tree_view_set_headers_visible( listview, TRUE );
		gtk_tree_view_set_headers_clickable( listview, TRUE );

		selection = gtk_tree_view_get_selection( listview );
		gtk_tree_selection_set_mode( selection, GTK_SELECTION_BROWSE );
	}
}

/**
 * nact_ibasenames_tab_runtime_init:
 * @window: this #NactIBasenamesTab instance.
 *
 * Initializes the tab widget at each time the widget will be displayed.
 * Connect signals and setup runtime values.
 */
void
nact_ibasenames_tab_runtime_init_toplevel( NactIBasenamesTab *instance )
{
	static const gchar *thisfn = "nact_ibasenames_tab_runtime_init_toplevel";
	GtkTreeView *listview;
	GtkTreeViewColumn *column;
	GList *renderers;
	GtkWidget *add_button, *remove_button;

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_IBASENAMES_TAB( instance ));

	if( st_initialized && !st_finalized ){

		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( instance ),
				MAIN_WINDOW_SIGNAL_SELECTION_CHANGED,
				G_CALLBACK( on_tab_updatable_selection_changed ));

		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( instance ),
				TAB_UPDATABLE_SIGNAL_ENABLE_TAB,
				G_CALLBACK( on_tab_updatable_enable_tab ));

		listview = GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( instance ), "BasenamesTreeview" ));
		column = gtk_tree_view_get_column( listview, BASENAMES_ITEM_COLUMN );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( column ),
				"clicked",
				G_CALLBACK( on_filter_clicked ));

		renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( column ));
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( renderers->data ),
				"edited",
				G_CALLBACK( on_filter_edited ));

		column = gtk_tree_view_get_column( listview, BASENAMES_MUST_MATCH_COLUMN );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( column ),
				"clicked",
				G_CALLBACK( on_must_match_clicked ));

		renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( column ));
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( renderers->data ),
				"toggled",
				G_CALLBACK( on_must_match_toggled ));

		column = gtk_tree_view_get_column( listview, BASENAMES_MUST_NOT_MATCH_COLUMN );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( column ),
				"clicked",
				G_CALLBACK( on_must_not_match_clicked ));

		renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( column ));
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( renderers->data ),
				"toggled",
				G_CALLBACK( on_must_not_match_toggled ));

		add_button = base_window_get_widget( BASE_WINDOW( instance ), "AddBasenameButton");
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( add_button ),
				"clicked",
				G_CALLBACK( on_add_filter_clicked ));

		remove_button = base_window_get_widget( BASE_WINDOW( instance ), "RemoveBasenameButton");
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( remove_button ),
				"clicked",
				G_CALLBACK( on_remove_filter_clicked ));

		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( gtk_tree_view_get_selection( listview )),
				"changed",
				G_CALLBACK( on_selection_changed ));

		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( listview ),
				"key-press-event",
				G_CALLBACK( on_key_pressed_event ));
	}
}

void
nact_ibasenames_tab_all_widgets_showed( NactIBasenamesTab *instance )
{
	static const gchar *thisfn = "nact_ibasenames_tab_all_widgets_showed";

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_IBASENAMES_TAB( instance ));

	if( st_initialized && !st_finalized ){
	}
}

/**
 * nact_ibasenames_tab_dispose:
 * @window: this #NactIBasenamesTab instance.
 *
 * Called at instance_dispose time.
 */
void
nact_ibasenames_tab_dispose( NactIBasenamesTab *instance )
{
	static const gchar *thisfn = "nact_ibasenames_tab_dispose";

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_IBASENAMES_TAB( instance ));

	if( st_initialized && !st_finalized ){
	}
}

static void
on_add_filter_clicked( GtkButton *button, BaseWindow *window )
{
}

static void
on_filter_clicked( GtkTreeViewColumn *treeviewcolumn, BaseWindow *window )
{
	static const gchar *thisfn = "nact_ibasenames_tab_on_filter_clicked";

	g_debug( "%s", thisfn );
}

static void
on_filter_edited( GtkCellRendererText *renderer, const gchar *path, const gchar *text, BaseWindow *window )
{
}

static gboolean
on_key_pressed_event( GtkWidget *widget, GdkEventKey *event, BaseWindow *window )
{
	gboolean stop;

	stop = FALSE;

	if( event->keyval == GDK_F2 ){
		/*edit_inline( window );*/
		stop = TRUE;
	}

	if( event->keyval == GDK_Insert || event->keyval == GDK_KP_Insert ){
		/*insert_new_row( window );*/
		stop = TRUE;
	}

	if( event->keyval == GDK_Delete || event->keyval == GDK_KP_Delete ){
		/*delete_current_row( window );*/
		stop = TRUE;
	}

	return( stop );
}

static void
on_must_match_clicked( GtkTreeViewColumn *treeviewcolumn, BaseWindow *window )
{
	static const gchar *thisfn = "nact_ibasenames_tab_on_must_match_clicked";

	g_debug( "%s", thisfn );
}

static void
on_must_match_toggled( GtkCellRendererToggle *cell_renderer, gchar *path, BaseWindow *window )
{
}

static void
on_must_not_match_clicked( GtkTreeViewColumn *treeviewcolumn, BaseWindow *window )
{
	static const gchar *thisfn = "nact_ibasenames_tab_on_must_not_match_clicked";

	g_debug( "%s", thisfn );
}

static void
on_must_not_match_toggled( GtkCellRendererToggle *cell_renderer, gchar *path, BaseWindow *window )
{
}

static void
on_remove_filter_clicked( GtkButton *button, BaseWindow *window )
{
}

static void
on_selection_changed( GtkTreeSelection *selection, BaseWindow *window )
{
}

/*
 * basically we are using here a rather common scheme:
 * - object has a GSList of strings, each of one being a basename description,
 *   which may be negated
 * - split this list into two list boxes, the first for positive assertion,
 *   and the second for negative ones
 * - let these two lists be updated by the user
 * - update the object with a concatenation of the listbox contents
 */
static void
on_tab_updatable_selection_changed( NactIBasenamesTab *instance, gint count_selected )
{
	static const gchar *thisfn = "nact_ibasenames_tab_on_tab_updatable_selection_changed";
	NAObjectItem *item;
	NAObjectProfile *profile;
	GSList *basenames;
	gboolean editable;
	GtkTreeView *listview;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreePath *path;

	g_return_if_fail( NACT_IS_IBASENAMES_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p, count_selected=%d", thisfn, ( void * ) instance, count_selected );

		g_object_get(
				G_OBJECT( instance ),
				TAB_UPDATABLE_PROP_EDITED_ACTION, &item,
				TAB_UPDATABLE_PROP_EDITED_PROFILE, &profile,
				TAB_UPDATABLE_PROP_EDITABLE, &editable,
				NULL );

		basenames = NULL;

		if( profile ){
			basenames = na_object_get_basenames( profile );
		} else {
			basenames = na_object_get_basenames( item );
		}

		st_on_selection_change = TRUE;

		listview = GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( instance ), "BasenamesTreeview" ));
		model = gtk_tree_view_get_model( listview );
		selection = gtk_tree_view_get_selection( listview );
		gtk_tree_selection_unselect_all( selection );
		gtk_list_store_clear( GTK_LIST_STORE( model ));

		if( basenames ){
			g_slist_foreach( basenames, ( GFunc ) iter_for_setup, model );
		}

		st_on_selection_change = FALSE;

		path = gtk_tree_path_new_first();
		if( path ){
			selection = gtk_tree_view_get_selection( listview );
			gtk_tree_selection_select_path( selection, path );
			gtk_tree_path_free( path );
		}
	}
}

static void
on_tab_updatable_enable_tab( NactIBasenamesTab *instance, NAObjectItem *item )
{
	static const gchar *thisfn = "nact_ibasenames_tab_on_tab_updatable_enable_tab";

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p, item=%p", thisfn, ( void * ) instance, ( void * ) item );
		g_return_if_fail( NACT_IS_IBASENAMES_TAB( instance ));

		tab_set_sensitive( instance );
	}
}

static void
iter_for_setup( gchar *basename, GtkTreeModel *model )
{
	GtkTreeIter iter;
	gchar *tmp, *filter;
	gboolean positive;
	gboolean negative;

	filter = g_strstrip( g_strdup( basename ));
	positive = FALSE;
	negative = FALSE;

	if( filter[0] == '!' ){
		tmp = g_strdup( filter+1 );
		g_free( filter );
		filter = tmp;
		negative = TRUE;

	} else {
		positive = TRUE;
	}

	gtk_list_store_append( GTK_LIST_STORE( model ), &iter );
	gtk_list_store_set(
			GTK_LIST_STORE( model ),
			&iter,
			BASENAMES_ITEM_COLUMN, filter,
			BASENAMES_MUST_MATCH_COLUMN, positive,
			BASENAMES_MUST_NOT_MATCH_COLUMN, negative,
			-1 );

	g_free( filter );
}

static gboolean
tab_set_sensitive( NactIBasenamesTab *instance )
{
	NAObjectProfile *profile;
	gboolean enable_tab;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_PROFILE, &profile,
			NULL );

	enable_tab = ( profile != NULL );
	nact_main_tab_enable_page( NACT_MAIN_WINDOW( instance ), TAB_BASENAMES, enable_tab );
	nact_main_tab_enable_page( NACT_MAIN_WINDOW( instance ), TAB_BASENAMES, TRUE );

	return( enable_tab );
}
