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

#include <glib/gi18n.h>
#include <string.h>

#include <api/na-object-api.h>

#include "nact-main-tab.h"
#include "nact-ienvironment-tab.h"

/* private interface data
 */
struct NactIEnvironmentTabInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* column ordering
 */
enum {
	ENV_BOOL_COLUMN = 0,
	ENV_LABEL_COLUMN,
	ENV_KEYWORD_COLUMN,
	N_COLUMN
};

typedef struct {
	gchar *keyword;
	gchar *label;
}
	EnvStruct;

static EnvStruct st_envs[] = {
	{ "GNOME", N_( "GNOME desktop" ) },
	{ "KDE",   N_( "KDE desktop" ) },
	{ "ROX",   N_( "ROX desktop" ) },
	{ "XFCE",  N_( "XFCE desktop" ) },
	{ "Old",   N_( "Legacy systems" ) },
	{ NULL }
};

static gboolean st_initialized = FALSE;
static gboolean st_finalized = FALSE;
static gboolean st_on_selection_change = FALSE;

static GType    register_type( void );
static void     interface_base_init( NactIEnvironmentTabInterface *klass );
static void     interface_base_finalize( NactIEnvironmentTabInterface *klass );

static void     on_tab_updatable_selection_changed( NactIEnvironmentTab *instance, gint count_selected );

static void     on_show_always_toggled( GtkToggleButton *togglebutton, NactIEnvironmentTab *instance );
static void     on_only_show_toggled( GtkToggleButton *togglebutton, NactIEnvironmentTab *instance );
static void     on_do_not_show_toggled( GtkToggleButton *togglebutton, NactIEnvironmentTab *instance );
static void     on_desktop_toggled( GtkCellRendererToggle *renderer, gchar *path, BaseWindow *window );
static void     on_try_exec_changed( GtkEntry *entry, NactIEnvironmentTab *instance );
static void     on_try_exec_browse( GtkButton *button, NactIEnvironmentTab *instance );
static void     on_show_if_registered_changed( GtkEntry *entry, NactIEnvironmentTab *instance );
static void     on_show_if_true_changed( GtkEntry *entry, NactIEnvironmentTab *instance );
static void     on_show_if_running_changed( GtkEntry *entry, NactIEnvironmentTab *instance );
static void     on_show_if_running_browse( GtkButton *button, NactIEnvironmentTab *instance );

static gboolean tab_set_sensitive( NactIEnvironmentTab *instance, NAIContext *context );

GType
nact_ienvironment_tab_get_type( void )
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
	static const gchar *thisfn = "nact_ienvironment_tab_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NactIEnvironmentTabInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "NactIEnvironmentTab", &info, 0 );

	g_type_interface_add_prerequisite( type, BASE_WINDOW_TYPE );

	return( type );
}

static void
interface_base_init( NactIEnvironmentTabInterface *klass )
{
	static const gchar *thisfn = "nact_ienvironment_tab_interface_base_init";

	if( !st_initialized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NactIEnvironmentTabInterfacePrivate, 1 );

		st_initialized = TRUE;
	}
}

static void
interface_base_finalize( NactIEnvironmentTabInterface *klass )
{
	static const gchar *thisfn = "nact_ienvironment_tab_interface_base_finalize";

	if( st_initialized && !st_finalized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		st_finalized = TRUE;

		g_free( klass->private );
	}
}

/**
 * nact_ienvironment_tab_initial_load:
 * @window: this #NactIEnvironmentTab instance.
 *
 * Initializes the tab widget at initial load.
 */
void
nact_ienvironment_tab_initial_load_toplevel( NactIEnvironmentTab *instance )
{
	static const gchar *thisfn = "nact_ienvironment_tab_initial_load_toplevel";
	GtkTreeView *listview;
	GtkListStore *model;
	GtkCellRenderer *check_cell, *text_cell;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;

	g_return_if_fail( NACT_IS_IENVIRONMENT_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

		listview = GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( instance ), "EnvironmentsTreeView" ));
		model = gtk_list_store_new( N_COLUMN, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING );
		gtk_tree_view_set_model( listview, GTK_TREE_MODEL( model ));
		g_object_unref( model );

		check_cell = gtk_cell_renderer_toggle_new();
		column = gtk_tree_view_column_new_with_attributes(
				"boolean",
				check_cell,
				"active", ENV_BOOL_COLUMN,
				NULL );
		gtk_tree_view_append_column( listview, column );

		text_cell = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(
				"label",
				text_cell,
				"text", ENV_LABEL_COLUMN,
				NULL );
		gtk_tree_view_append_column( listview, column );

		gtk_tree_view_set_headers_visible( listview, FALSE );

		selection = gtk_tree_view_get_selection( listview );
		gtk_tree_selection_set_mode( selection, GTK_SELECTION_BROWSE );
	}
}

/**
 * nact_ienvironment_tab_runtime_init:
 * @window: this #NactIEnvironmentTab instance.
 *
 * Initializes the tab widget at each time the widget will be displayed.
 * Connect signals and setup runtime values.
 */
void
nact_ienvironment_tab_runtime_init_toplevel( NactIEnvironmentTab *instance )
{
	static const gchar *thisfn = "nact_ienvironment_tab_runtime_init_toplevel";
	GtkWidget *button, *entry;
	GtkTreeView *listview;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeViewColumn *column;
	GList *renderers;
	guint i;

	g_return_if_fail( NACT_IS_IENVIRONMENT_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( instance ),
				MAIN_WINDOW_SIGNAL_SELECTION_CHANGED,
				G_CALLBACK( on_tab_updatable_selection_changed ));

		button = base_window_get_widget( BASE_WINDOW( instance ), "ShowAlwaysButton" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( button ),
				"toggled",
				G_CALLBACK( on_show_always_toggled ));

		button = base_window_get_widget( BASE_WINDOW( instance ), "OnlyShowButton" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( button ),
				"toggled",
				G_CALLBACK( on_only_show_toggled ));

		button = base_window_get_widget( BASE_WINDOW( instance ), "DoNotShowButton" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( button ),
				"toggled",
				G_CALLBACK( on_do_not_show_toggled ));

		listview = GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( instance ), "EnvironmentsTreeView" ));
		model = gtk_tree_view_get_model( listview );

		for( i = 0 ; st_envs[i].keyword ; ++i ){
			gtk_list_store_append( GTK_LIST_STORE( model ), &iter );
			gtk_list_store_set(
					GTK_LIST_STORE( model ),
					&iter,
					ENV_BOOL_COLUMN, FALSE,
					ENV_LABEL_COLUMN, st_envs[i].label,
					ENV_KEYWORD_COLUMN, st_envs[i].keyword,
					-1 );
		}

		column = gtk_tree_view_get_column( listview, ENV_BOOL_COLUMN );
		renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( column ));
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( renderers->data ),
				"toggled",
				G_CALLBACK( on_desktop_toggled ));


		entry = base_window_get_widget( BASE_WINDOW( instance ), "TryExecEntry" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( entry ),
				"changed",
				G_CALLBACK( on_try_exec_changed ));

		button = base_window_get_widget( BASE_WINDOW( instance ), "TryExecButton" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( button ),
				"clicked",
				G_CALLBACK( on_try_exec_browse ));

		entry = base_window_get_widget( BASE_WINDOW( instance ), "ShowIfRegisteredEntry" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( entry ),
				"changed",
				G_CALLBACK( on_show_if_registered_changed ));

		entry = base_window_get_widget( BASE_WINDOW( instance ), "ShowIfTrueEntry" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( entry ),
				"changed",
				G_CALLBACK( on_show_if_true_changed ));

		entry = base_window_get_widget( BASE_WINDOW( instance ), "ShowIfRunningEntry" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( entry ),
				"changed",
				G_CALLBACK( on_show_if_running_changed ));

		button = base_window_get_widget( BASE_WINDOW( instance ), "ShowIfRunningButton" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( button ),
				"clicked",
				G_CALLBACK( on_show_if_running_browse ));
	}
}

void
nact_ienvironment_tab_all_widgets_showed( NactIEnvironmentTab *instance )
{
	static const gchar *thisfn = "nact_ienvironment_tab_all_widgets_showed";

	g_return_if_fail( NACT_IS_IENVIRONMENT_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	}
}

/**
 * nact_ienvironment_tab_dispose:
 * @window: this #NactIEnvironmentTab instance.
 *
 * Called at instance_dispose time.
 */
void
nact_ienvironment_tab_dispose( NactIEnvironmentTab *instance )
{
	static const gchar *thisfn = "nact_ienvironment_tab_dispose";
	GtkTreeView *listview;
	GtkTreeModel *model;
	GtkTreeSelection *selection;

	g_return_if_fail( NACT_IS_IENVIRONMENT_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

		listview = GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( instance ), "EnvironmentsTreeView" ));
		model = gtk_tree_view_get_model( listview );
		selection = gtk_tree_view_get_selection( listview );
		gtk_tree_selection_unselect_all( selection );
		gtk_list_store_clear( GTK_LIST_STORE( model ));
	}
}

static void
on_tab_updatable_selection_changed( NactIEnvironmentTab *instance, gint count_selected )
{
	static const gchar *thisfn = "nact_ienvironment_tab_on_tab_updatable_selection_changed";
	GtkTreeView *listview;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;
	GtkTreeSelection *selection;
	gboolean next_ok, found;
	GtkWidget *appear_button;
	NAObjectItem *item;
	NAObjectProfile *profile;
	NAIContext *context;
	gboolean editable;
	GSList *show, *notshow, *checked, *ic;
	gchar *keyword;

	g_return_if_fail( NACT_IS_IENVIRONMENT_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p, count_selected=%d", thisfn, ( void * ) instance, count_selected );

		st_on_selection_change = TRUE;

		/* reinitialize the list OnlyShowIn/NotShowIn
		 */
		listview = GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( instance ), "EnvironmentsTreeView" ));
		model = gtk_tree_view_get_model( listview );

		if( gtk_tree_model_get_iter_first( model, &iter )){
			next_ok = TRUE;
			while( next_ok ){
				gtk_list_store_set( GTK_LIST_STORE( model ), &iter, ENV_BOOL_COLUMN, FALSE, -1 );
				next_ok = gtk_tree_model_iter_next( model, &iter );
			}
		}

		/* setup the tab for current context
		 */
		g_object_get(
				G_OBJECT( instance ),
				TAB_UPDATABLE_PROP_SELECTED_ITEM, &item,
				TAB_UPDATABLE_PROP_EDITED_PROFILE, &profile,
				TAB_UPDATABLE_PROP_EDITABLE, &editable,
				NULL );

		context = ( profile ? NA_ICONTEXT( profile ) : ( NAIContext * ) item );

		tab_set_sensitive( instance, context );

		show = NULL;
		notshow = NULL;
		checked = NULL;
		found = FALSE;
		appear_button = base_window_get_widget( BASE_WINDOW( instance ), "ShowAlwaysButton" );

		if( context ){
			show = na_object_get_only_show_in( context );
			if( show && g_slist_length( show )){
				appear_button = base_window_get_widget( BASE_WINDOW( instance ), "OnlyShowButton" );
				checked = show;
			} else {
				notshow = na_object_get_not_show_in( context );
				if( notshow && g_slist_length( notshow )){
					appear_button = base_window_get_widget( BASE_WINDOW( instance ), "DoNotShowButton" );
					checked = notshow;
				}
			}
		}

		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( appear_button ), TRUE );

		for( ic = checked ; ic ; ic = ic->next ){
			if( strlen( ic->data )){
				if( gtk_tree_model_get_iter_first( model, &iter )){
					next_ok = TRUE;
					found = FALSE;
					while( next_ok && !found ){
						gtk_tree_model_get( model, &iter, ENV_KEYWORD_COLUMN, &keyword, -1 );
						if( !strcmp( keyword, ic->data )){
							gtk_list_store_set( GTK_LIST_STORE( model ), &iter, ENV_BOOL_COLUMN, TRUE, -1 );
							found = TRUE;
						}
						g_free( keyword );
						if( !found ){
							next_ok = gtk_tree_model_iter_next( model, &iter );
						}
					}
				}
				if( !found ){
					g_warning( "%s: unable to set %s environment", thisfn, ( const gchar * ) ic->data );
				}
			}
		}

		st_on_selection_change = FALSE;

		if( context ){
			path = gtk_tree_path_new_first();
			if( path ){
				selection = gtk_tree_view_get_selection( listview );
				gtk_tree_selection_select_path( selection, path );
				gtk_tree_path_free( path );
			}
		}
	}
}

static void
on_show_always_toggled( GtkToggleButton *toggle_button, NactIEnvironmentTab *instance )
{
	static const gchar *thisfn = "nact_ienvironment_tab_on_show_always_toggled";

	g_debug( "%s: toggle_button=%p (active=%s), instance=%p",
			thisfn,
			( void * ) toggle_button, gtk_toggle_button_get_active( toggle_button ) ? "True":"False",
			( void * ) instance );
}

static void
on_only_show_toggled( GtkToggleButton *toggle_button, NactIEnvironmentTab *instance )
{
	static const gchar *thisfn = "nact_ienvironment_tab_on_only_show_toggled";

	g_debug( "%s: toggle_button=%p (active=%s), instance=%p",
			thisfn,
			( void * ) toggle_button, gtk_toggle_button_get_active( toggle_button ) ? "True":"False",
			( void * ) instance );
}

static void
on_do_not_show_toggled( GtkToggleButton *toggle_button, NactIEnvironmentTab *instance )
{
	static const gchar *thisfn = "nact_ienvironment_tab_on_do_not_show_toggled";

	g_debug( "%s: toggle_button=%p (active=%s), instance=%p",
			thisfn,
			( void * ) toggle_button, gtk_toggle_button_get_active( toggle_button ) ? "True":"False",
			( void * ) instance );
}

static void
on_desktop_toggled( GtkCellRendererToggle *renderer, gchar *path, BaseWindow *window )
{
	static const gchar *thisfn = "nact_ienvironment_tab_on_desktop_toggled";
	GtkTreeView *listview;
	gboolean editable;
	NAObjectItem *item;
	NAObjectProfile *profile;
	NAIContext *context;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *tree_path;
	gboolean state;
	gchar *desktop;

	g_debug( "%s: renderer=%p, path=%s, window=%p", thisfn, ( void * ) renderer, path, ( void * ) window );

	if( !st_on_selection_change ){

		listview = GTK_TREE_VIEW( base_window_get_widget( window, "EnvironmentsTreeView" ));
		model = gtk_tree_view_get_model( listview );
		tree_path = gtk_tree_path_new_from_string( path );
		gtk_tree_model_get_iter( model, &iter, tree_path );
		gtk_tree_path_free( tree_path );
		gtk_tree_model_get( model, &iter, ENV_BOOL_COLUMN, &state, ENV_KEYWORD_COLUMN, &desktop, -1 );

		g_object_get(
				G_OBJECT( window ),
				TAB_UPDATABLE_PROP_SELECTED_ITEM, &item,
				TAB_UPDATABLE_PROP_EDITED_PROFILE, &profile,
				TAB_UPDATABLE_PROP_EDITABLE, &editable,
				NULL );

		context = ( profile ? NA_ICONTEXT( profile ) : ( NAIContext * ) item );

		if( !editable ){
			g_signal_handlers_block_by_func(( gpointer ) renderer, on_desktop_toggled, window );
			gtk_cell_renderer_toggle_set_active( renderer, state );
			g_signal_handlers_unblock_by_func(( gpointer ) renderer, on_desktop_toggled, window );

		} else {
			gtk_list_store_set( GTK_LIST_STORE( model ), &iter, ENV_BOOL_COLUMN, !state, -1 );
			/*
			if( g_object_class_find_property( G_OBJECT_GET_CLASS( window ), TAB_UPDATABLE_PROP_EDITED_PROFILE )){
				g_object_get(
						G_OBJECT( window ),
						TAB_UPDATABLE_PROP_EDITED_PROFILE, &edited,
						NULL );
				if( edited ){
					na_object_set_scheme( edited, scheme, !state );
					g_signal_emit_by_name( G_OBJECT( window ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, edited, FALSE );
				}
			}
			*/
		}

		g_free( desktop );
	}
}

static void
on_try_exec_changed( GtkEntry *entry, NactIEnvironmentTab *instance )
{
}

static void
on_try_exec_browse( GtkButton *button, NactIEnvironmentTab *instance )
{
}

static void
on_show_if_registered_changed( GtkEntry *entry, NactIEnvironmentTab *instance )
{
}

static void
on_show_if_true_changed( GtkEntry *entry, NactIEnvironmentTab *instance )
{
}

static void
on_show_if_running_changed( GtkEntry *entry, NactIEnvironmentTab *instance )
{
}

static void
on_show_if_running_browse( GtkButton *button, NactIEnvironmentTab *instance )
{
}

static gboolean
tab_set_sensitive( NactIEnvironmentTab *instance, NAIContext *context )
{
	gboolean enable_tab;

	enable_tab = ( context != NULL );
	nact_main_tab_enable_page( NACT_MAIN_WINDOW( instance ), TAB_ENVIRONMENT, enable_tab );

	return( enable_tab );
}
