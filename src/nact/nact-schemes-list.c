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

#include <gconf/gconf-client.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>

#include <api/na-core-utils.h>
#include <api/na-object-api.h>

#include <core/na-iprefs.h>

#include "nact-application.h"
#include "nact-gtk-utils.h"
#include "nact-main-tab.h"
#include "nact-schemes-list.h"

/* data attached to the treeview widget on initial load
 * at this time, only treeview and mode are set
 * on runtime init, the current window is associated to the widget,
 *  and so, indirectly, to this data
 * at this time, window is set
 */
typedef struct {
	GtkTreeView        *treeview;		/* set when allocating the data */
	guint               mode;			/* set when creating the model */
	BaseWindow         *window;			/* set when initializating the view */
	pf_new_selection_cb pf_on_sel_changed;
	void               *user_data;
}
	SchemesListData;

/* column ordering in the model
 */
enum {
	SCHEMES_KEYWORD_COLUMN = 0,
	SCHEMES_DESC_COLUMN,
	SCHEMES_ALREADY_USED_COLUMN,
	SCHEMES_N_COLUMN
};

#define SCHEMES_LIST_DATA				"nact-schemes-list-data"
#define SCHEMES_LIST_TREEVIEW			"nact-schemes-list-treeview"

static void             init_view_setup_defaults( SchemesListData *data );
static GSList          *init_view_get_default_list( SchemesListData *data );
static GSList          *init_view_get_default_default_list( SchemesListData *data );
static void             init_view_connect_signals( SchemesListData *data );
static void             init_view_select_first_row( SchemesListData *data );

static gboolean         setup_values_iter( GtkTreeModel *model, GtkTreePath *path, GtkTreeIter* iter, GSList *schemes );

static GSList          *get_list_schemes( GtkTreeView *treeview );
static gboolean         get_list_schemes_iter( GtkTreeModel *model, GtkTreePath *path, GtkTreeIter* iter, GSList **list );

static gboolean         on_key_pressed_event( GtkWidget *widget, GdkEventKey *event, BaseWindow *window );
static void             on_selection_changed( GtkTreeSelection *selection, BaseWindow *window );
static void             on_add_clicked( GtkButton *button, BaseWindow *window );
static void             on_remove_clicked( GtkButton *button, BaseWindow *window );
static void             on_desc_edited( GtkCellRendererText *renderer, const gchar *path, const gchar *text, BaseWindow *window );
static void             on_keyword_edited( GtkCellRendererText *renderer, const gchar *path, const gchar *text, BaseWindow *window );

static void             edit_cell( BaseWindow *window, const gchar *path_string, const gchar *text, gint column, gboolean *state, gchar **old_text );
static void             edit_inline( BaseWindow *window );
static void             insert_new_row( BaseWindow *window );
static void             delete_row( BaseWindow *window );
static void             display_keyword( GtkTreeViewColumn *column, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, SchemesListData *data );
static void             display_description( GtkTreeViewColumn *column, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, SchemesListData *data );
static void             display_label( GtkTreeViewColumn *column, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, SchemesListData *data, guint column_id );

static GtkButton       *get_add_button( BaseWindow *window );
static GtkButton       *get_remove_button( BaseWindow *window );
static SchemesListData *get_schemes_list_data( GtkTreeView *treeview );

/**
 * nact_schemes_list_create_schemes_list:
 * @treeview: the #GtkTreeView.
 * @mode: whether we are opening this listview for preferences edition,
 *  or to add a new scheme from the default list.
 *
 * Create the treeview model when initially loading the widget from
 * the UI manager. Associates the SchemesListData structure to the widget.
 */
void
nact_schemes_list_create_model( GtkTreeView *treeview, guint mode )
{
	static const char *thisfn = "nact_schemes_list_create_model";
	GtkListStore *model;
	GtkTreeViewColumn *column;
	GtkCellRenderer *text_cell;
	GtkTreeSelection *selection;
	SchemesListData *data;

	g_return_if_fail( GTK_IS_TREE_VIEW( treeview ));
	g_debug( "%s: treeview=%p, mode=%d", thisfn, ( void * ) treeview, mode );

	data = get_schemes_list_data( treeview );
	data->mode = mode;

	model = gtk_list_store_new( SCHEMES_N_COLUMN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN );
	gtk_tree_view_set_model( treeview, GTK_TREE_MODEL( model ));
	g_object_unref( model );

	text_cell = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
			"scheme-code",
			text_cell,
			"text", SCHEMES_KEYWORD_COLUMN,
			NULL );
	gtk_tree_view_append_column( treeview, column );
	gtk_tree_sortable_set_sort_column_id( GTK_TREE_SORTABLE( model ), SCHEMES_KEYWORD_COLUMN, GTK_SORT_ASCENDING );
	gtk_tree_view_column_set_cell_data_func(
			column, text_cell, ( GtkTreeCellDataFunc ) display_keyword, data, NULL );

	text_cell = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
			"scheme-description",
			text_cell,
			"text", SCHEMES_DESC_COLUMN,
			NULL );
	gtk_tree_view_append_column( treeview, column );
	gtk_tree_view_column_set_cell_data_func(
			column, text_cell, ( GtkTreeCellDataFunc ) display_description, data, NULL );

	gtk_tree_view_set_headers_visible( treeview, FALSE );

	selection = gtk_tree_view_get_selection( treeview );
	gtk_tree_selection_set_mode( selection, GTK_SELECTION_BROWSE );
}

/**
 * nact_schemes_list_init_view:
 * @treeview: the #GtkTreeView.
 * @window: the parent #BaseWindow which embeds the view.
 * @pf: a callback function which will be called on selection change.
 * @user_data: user data to be passed to the callback function.
 *
 * Connects signals at runtime initialization of the widget, and displays
 * the current default list of schemes.
 *
 * When mode is for preferences, this is all that is required for runtime
 * initialization.
 *
 * When mode is for add from defaults, i.e. when editing #NAIContext schemes
 * conditions, then #nact_schemes_list_setup_values() must also be called in
 * order to actually setup the already used schemes.
 */
void
nact_schemes_list_init_view( GtkTreeView *treeview, BaseWindow *window, pf_new_selection_cb pf, void *user_data )
{
	static const gchar *thisfn = "nact_schemes_list_init_view";
	SchemesListData *data;

	g_debug( "%s: treeview=%p, window=%p",
			thisfn,
			( void * ) treeview,
			( void * ) window );

	g_return_if_fail( BASE_IS_WINDOW( window ));
	g_return_if_fail( GTK_IS_TREE_VIEW( treeview ));

	g_object_set_data( G_OBJECT( window ), SCHEMES_LIST_TREEVIEW, treeview );

	data = get_schemes_list_data( treeview );
	data->window = window;
	data->pf_on_sel_changed = pf;
	data->user_data = user_data;

	init_view_setup_defaults( data );
	init_view_connect_signals( data );
}

static void
init_view_setup_defaults( SchemesListData *data )
{
	GtkListStore *model;
	GSList *schemes, *iter;
	GtkTreeIter row;
	gchar **tokens;

	model = GTK_LIST_STORE( gtk_tree_view_get_model( data->treeview ));

	schemes = init_view_get_default_list( data );

	for( iter = schemes ; iter ; iter = iter->next ){

		tokens = g_strsplit(( gchar * ) iter->data, "|", 2 );
		gtk_list_store_append( model, &row );
		gtk_list_store_set( model, &row,
				SCHEMES_KEYWORD_COLUMN, tokens[0],
				SCHEMES_DESC_COLUMN, tokens[1],
				SCHEMES_ALREADY_USED_COLUMN, FALSE,
				-1 );
		g_strfreev( tokens );
	}

	na_core_utils_slist_free( schemes );
}

/*
 * return default schemes list
 * the returned list must be released with #na_core_utils_slist_free()
 */
static GSList *
init_view_get_default_list( SchemesListData *data )
{
	GSList *list = NULL;
	NactApplication *application;
	NAUpdater *updater;

	application = NACT_APPLICATION( base_window_get_application( data->window ));
	updater = nact_application_get_updater( application );
	list = na_iprefs_read_string_list( NA_IPREFS( updater ), "schemes", NULL );

	if( !list ){
		list = init_view_get_default_default_list( data );
	}

	return( list );
}

static GSList *
init_view_get_default_default_list( SchemesListData *data )
{
	GSList *list = NULL;

	/* i18n notes : description of 'file' scheme */
	list = g_slist_append( list, g_strdup_printf( "file|%s", _( "Local files")));
	/* i18n notes : description of 'sftp' scheme */
	list = g_slist_append( list, g_strdup_printf( "sftp|%s", _( "SSH files")));
	/* i18n notes : description of 'smb' scheme */
	list = g_slist_append( list, g_strdup_printf( "smb|%s", _( "Windows files")));
	/* i18n notes : description of 'ftp' scheme */
	list = g_slist_append( list, g_strdup_printf( "ftp|%s", _( "FTP files")));
	/* i18n notes : description of 'dav' scheme */
	list = g_slist_append( list, g_strdup_printf( "dav|%s", _( "WebDAV files")));

	return( list );
}

static void
init_view_connect_signals( SchemesListData *data )
{
	GtkTreeViewColumn *column;
	GList *renderers;
	GtkButton *add_button, *remove_button;

	base_window_signal_connect(
			data->window,
			G_OBJECT( gtk_tree_view_get_selection( data->treeview )),
			"changed",
			G_CALLBACK( on_selection_changed ));

	if( data->mode == SCHEMES_LIST_FOR_PREFERENCES ){

		column = gtk_tree_view_get_column( data->treeview, SCHEMES_KEYWORD_COLUMN );
		renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( column ));
		base_window_signal_connect(
				data->window,
				G_OBJECT( renderers->data ),
				"edited",
				G_CALLBACK( on_keyword_edited ));

		column = gtk_tree_view_get_column( data->treeview, SCHEMES_DESC_COLUMN );
		renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( column ));
		base_window_signal_connect(
				data->window,
				G_OBJECT( renderers->data ),
				"edited",
				G_CALLBACK( on_desc_edited ));

		add_button = get_add_button( data->window );
		base_window_signal_connect(
				data->window,
				G_OBJECT( add_button ),
				"clicked",
				G_CALLBACK( on_add_clicked ));

		remove_button = get_remove_button( data->window );
		base_window_signal_connect(
				data->window,
				G_OBJECT( remove_button ),
				"clicked",
				G_CALLBACK( on_remove_clicked ));

		base_window_signal_connect(
				data->window,
				G_OBJECT( data->treeview ),
				"key-press-event",
				G_CALLBACK( on_key_pressed_event ));
	}
}

static void
init_view_select_first_row( SchemesListData *data )
{
	GtkTreeSelection *selection;
	GtkTreePath *path;

	path = gtk_tree_path_new_first();
	selection = gtk_tree_view_get_selection( data->treeview );
	gtk_tree_selection_select_path( selection, path );
	gtk_tree_path_free( path );
}

/**
 * nact_schemes_list_save_values:
 * @window: the #BaseWindow which embeds this treeview.
 * @schemes: a #GSList of already used schemes.
 *
 * Set the used schemes for the current #NAIContext.
 */
void
nact_schemes_list_setup_values( BaseWindow *window, GSList *schemes )
{
	GtkTreeView *treeview;
	GtkTreeModel *model;

	treeview = GTK_TREE_VIEW( g_object_get_data( G_OBJECT( window ), SCHEMES_LIST_TREEVIEW ));
	model = gtk_tree_view_get_model( treeview );
	gtk_tree_model_foreach( model, ( GtkTreeModelForeachFunc ) setup_values_iter, schemes );
}

static gboolean
setup_values_iter( GtkTreeModel *model, GtkTreePath *path, GtkTreeIter* iter, GSList *schemes )
{
	gchar *keyword;
	gchar *description, *new_description;

	gtk_tree_model_get( model, iter, SCHEMES_KEYWORD_COLUMN, &keyword, SCHEMES_DESC_COLUMN, &description, -1 );

	if( na_core_utils_slist_find_negated( schemes, keyword )){
		/* i18n: add a comment when a scheme is already used by current item */
		new_description = g_strdup_printf( _( "%s (already used)"), description );
		gtk_list_store_set( GTK_LIST_STORE( model ), iter, SCHEMES_DESC_COLUMN, new_description, SCHEMES_ALREADY_USED_COLUMN, TRUE, -1 );
		g_free( new_description );
	}

	g_free( description );
	g_free( keyword );

	return( FALSE ); /* don't stop looping */
}

/**
 * nact_schemes_list_show_all:
 * @window: the #BaseWindow which embeds this treeview.
 *
 * Update visibility of widgets after all widgets are showed.
 */
void
nact_schemes_list_show_all( BaseWindow *window )
{
	GtkTreeView *listview;
	SchemesListData *data;
	GtkButton *button;

	g_return_if_fail( BASE_IS_WINDOW( window ));

	listview = GTK_TREE_VIEW( g_object_get_data( G_OBJECT( window ), SCHEMES_LIST_TREEVIEW ));
	data = get_schemes_list_data( listview );

	button = get_add_button( window );
#if(( GTK_MAJOR_VERSION > 2 ) || ( GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 18 ))
	gtk_widget_set_visible( GTK_WIDGET( button ), data->mode == SCHEMES_LIST_FOR_PREFERENCES );
#else
	if( data->mode == SCHEMES_LIST_FOR_PREFERENCES ){
		gtk_widget_show( GTK_WIDGET( button ));
	} else {
		gtk_widget_hide( GTK_WIDGET( button ));
	}
#endif
	gtk_widget_set_sensitive( GTK_WIDGET( button ), data->mode == SCHEMES_LIST_FOR_PREFERENCES );

	button = get_remove_button( window );
#if(( GTK_MAJOR_VERSION > 2 ) || ( GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 18 ))
	gtk_widget_set_visible( GTK_WIDGET( button ), data->mode == SCHEMES_LIST_FOR_PREFERENCES );
#else
	if( data->mode == SCHEMES_LIST_FOR_PREFERENCES ){
		gtk_widget_show( GTK_WIDGET( button ));
	} else {
		gtk_widget_hide( GTK_WIDGET( button ));
	}
#endif
	gtk_widget_set_sensitive( GTK_WIDGET( button ), FALSE );

	init_view_select_first_row( data );
}

/**
 * nact_schemes_list_get_current_scheme:
 * @window: the #BaseWindow which embeds this treeview.
 *
 * Returns: the currently selected scheme, if any, as a newly allocated
 * string which should be g_free() by the caller.
 */
gchar *
nact_schemes_list_get_current_scheme( BaseWindow *window )
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *rows;
	GtkTreePath *path;
	GtkTreeIter iter;
	gchar *keyword;

	treeview = GTK_TREE_VIEW( g_object_get_data( G_OBJECT( window ), SCHEMES_LIST_TREEVIEW ));
	selection = gtk_tree_view_get_selection( treeview );
	rows = gtk_tree_selection_get_selected_rows( selection, &model );
	keyword = NULL;

	if( g_list_length( rows ) == 1 ){
		path = ( GtkTreePath * ) rows->data;
		gtk_tree_model_get_iter( model, &iter, path );
		gtk_tree_model_get( model, &iter, SCHEMES_KEYWORD_COLUMN, &keyword, -1 );
	}

	return( keyword );
}

/**
 * nact_schemes_list_save_defaults:
 * @window: the #BaseWindow which embeds this treeview.
 *
 * Save the list of schemes as a GConf preference.
 *
 * Default schemes are saved under a 'schemes' key as a list of strings,
 * where each string is of the form 'keyword|description'.
 */
void
nact_schemes_list_save_defaults( BaseWindow *window )
{
	GtkTreeView *treeview;
	GSList *schemes;
	NactApplication *application;
	NAUpdater *updater;

	treeview = GTK_TREE_VIEW( g_object_get_data( G_OBJECT( window ), SCHEMES_LIST_TREEVIEW ));
	schemes = get_list_schemes( treeview );

	application = NACT_APPLICATION( base_window_get_application( window ));
	updater = nact_application_get_updater( application );
	na_iprefs_write_string_list( NA_IPREFS( updater ), "schemes", schemes );

	na_core_utils_slist_free( schemes );
}

static GSList *
get_list_schemes( GtkTreeView *treeview )
{
	GSList *list = NULL;
	GtkTreeModel *model;

	model = gtk_tree_view_get_model( treeview );
	gtk_tree_model_foreach( model, ( GtkTreeModelForeachFunc ) get_list_schemes_iter, &list );

	return( list );
}

static gboolean
get_list_schemes_iter( GtkTreeModel *model, GtkTreePath *path, GtkTreeIter* iter, GSList **list )
{
	gchar *keyword;
	gchar *description;
	gchar *scheme;

	gtk_tree_model_get( model, iter, SCHEMES_KEYWORD_COLUMN, &keyword, SCHEMES_DESC_COLUMN, &description, -1 );
	scheme = g_strdup_printf( "%s|%s", keyword, description );
	g_free( description );
	g_free( keyword );

	( *list ) = g_slist_append(( *list ), scheme );

	return( FALSE ); /* don't stop looping */
}

/**
 * nact_schemes_list_dispose:
 * @treeview: the #GtkTreeView.
 */
void
nact_schemes_list_dispose( BaseWindow *window )
{
	static const gchar *thisfn = "nact_schemes_list_dispose";
	GtkTreeView *treeview;
	GtkTreeModel *model;
	GtkTreeSelection *selection;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );

	treeview = GTK_TREE_VIEW( g_object_get_data( G_OBJECT( window ), SCHEMES_LIST_TREEVIEW ));
	model = gtk_tree_view_get_model( treeview );
	selection = gtk_tree_view_get_selection( treeview );

	gtk_tree_selection_unselect_all( selection );
	gtk_list_store_clear( GTK_LIST_STORE( model ));
}

static gboolean
on_key_pressed_event( GtkWidget *widget, GdkEventKey *event, BaseWindow *window )
{
	gboolean stop;

	/*g_debug( "nact_schemes_list_on_key_pressed_event" );*/

	stop = FALSE;

	if( event->keyval == GDK_F2 ){
		edit_inline( window );
		stop = TRUE;
	}

	if( event->keyval == GDK_Insert || event->keyval == GDK_KP_Insert ){
		insert_new_row( window );
		stop = TRUE;
	}

	if( event->keyval == GDK_Delete || event->keyval == GDK_KP_Delete ){
		delete_row( window );
		stop = TRUE;
	}

	return( stop );
}

static void
on_selection_changed( GtkTreeSelection *selection, BaseWindow *window )
{
	/*static const gchar *thisfn = "nact_schemes_list_on_selection_changed";*/
	GtkButton *button;
	GtkTreeView *listview;
	SchemesListData *data;
	GtkTreeModel *model;
	GList *rows;
	GtkTreePath *path;
	GtkTreeIter iter;
	gchar *keyword;
	gboolean used;

	/*g_debug( "%s: selection=%p, window=%p", thisfn, ( void * ) selection, ( void * ) window );*/

	button = get_remove_button( window );
	gtk_widget_set_sensitive( GTK_WIDGET( button ), gtk_tree_selection_count_selected_rows( selection ) > 0);

	listview = GTK_TREE_VIEW( g_object_get_data( G_OBJECT( window ), SCHEMES_LIST_TREEVIEW ));
	data = ( SchemesListData * ) g_object_get_data( G_OBJECT( listview ), SCHEMES_LIST_DATA );

	if( data->pf_on_sel_changed ){
		rows = gtk_tree_selection_get_selected_rows( selection, &model );
		keyword = NULL;
		used = FALSE;

		if( g_list_length( rows ) == 1 ){
			path = ( GtkTreePath * ) rows->data;
			gtk_tree_model_get_iter( model, &iter, path );
			gtk_tree_model_get( model, &iter, SCHEMES_KEYWORD_COLUMN, &keyword, SCHEMES_ALREADY_USED_COLUMN, &used, -1 );
		}

		data->pf_on_sel_changed( keyword, used, data->user_data );

		g_free( keyword );
	}
}

static void
on_add_clicked( GtkButton *button, BaseWindow *window )
{
	insert_new_row( window );
}

static void
on_remove_clicked( GtkButton *button, BaseWindow *window )
{
	delete_row( window );
}

static void
on_desc_edited( GtkCellRendererText *renderer, const gchar *path, const gchar *text, BaseWindow *window )
{
	static const gchar *thisfn = "nact_schemes_list_on_desc_edited";

	g_debug( "%s: renderer=%p, path=%s, text=%s, window=%p",
			thisfn, ( void * ) renderer, path, text, ( void * ) window );

	edit_cell( window, path, text, SCHEMES_DESC_COLUMN, NULL, NULL );
}

static void
on_keyword_edited( GtkCellRendererText *renderer, const gchar *path, const gchar *text, BaseWindow *window )
{
	edit_cell( window, path, text, SCHEMES_KEYWORD_COLUMN, NULL, NULL );
}

static void
edit_cell( BaseWindow *window, const gchar *path_string, const gchar *text, gint column, gboolean *state, gchar **old_text )
{
	GtkTreeView *treeview;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;

	treeview = GTK_TREE_VIEW( g_object_get_data( G_OBJECT( window ), SCHEMES_LIST_TREEVIEW ));
	model = gtk_tree_view_get_model( treeview );
	path = gtk_tree_path_new_from_string( path_string );
	gtk_tree_model_get_iter( model, &iter, path );
	gtk_tree_path_free( path );

	if( state && old_text ){
		gtk_tree_model_get( model, &iter, SCHEMES_ALREADY_USED_COLUMN, state, SCHEMES_KEYWORD_COLUMN, old_text, -1 );
	}

	gtk_list_store_set( GTK_LIST_STORE( model ), &iter, column, text, -1 );
}

static void
edit_inline( BaseWindow *window )
{
	static const gchar *thisfn = "nact_schemes_list_edit_inline";
	GtkTreeView *listview;
	GtkTreeSelection *selection;
	GList *listrows;
	GtkTreePath *path;
	GtkTreeViewColumn *column;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );

	listview = GTK_TREE_VIEW( g_object_get_data( G_OBJECT( window ), SCHEMES_LIST_TREEVIEW ));
	selection = gtk_tree_view_get_selection( listview );
	listrows = gtk_tree_selection_get_selected_rows( selection, NULL );

	if( g_list_length( listrows ) == 1 ){
		gtk_tree_view_get_cursor( listview, &path, &column );
		gtk_tree_view_set_cursor( listview, path, column, TRUE );
		gtk_tree_path_free( path );
	}

	g_list_foreach( listrows, ( GFunc ) gtk_tree_path_free, NULL );
	g_list_free( listrows );
}

static void
insert_new_row( BaseWindow *window )
{
	GtkTreeView *listview;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;
	GtkTreeViewColumn *column;

	listview = GTK_TREE_VIEW( g_object_get_data( G_OBJECT( window ), SCHEMES_LIST_TREEVIEW ));
	model = gtk_tree_view_get_model( listview );

	gtk_list_store_insert_with_values( GTK_LIST_STORE( model ), &iter, 0,
			/* i18n notes : scheme name set for a new entry in the scheme list */
			SCHEMES_KEYWORD_COLUMN, _( "new-scheme" ),
			SCHEMES_DESC_COLUMN, _( "New scheme description" ),
			SCHEMES_ALREADY_USED_COLUMN, FALSE,
			-1 );

	path = gtk_tree_model_get_path( model, &iter );
	column = gtk_tree_view_get_column( listview, SCHEMES_KEYWORD_COLUMN );
	gtk_tree_view_set_cursor( listview, path, column, TRUE );
	gtk_tree_path_free( path );
}

static void
delete_row( BaseWindow *window )
{
	GtkTreeView *listview;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *rows;
	GtkTreeIter iter;
	GtkTreePath *path;

	listview = GTK_TREE_VIEW( g_object_get_data( G_OBJECT( window ), SCHEMES_LIST_TREEVIEW ));
	selection = gtk_tree_view_get_selection( listview );
	model = gtk_tree_view_get_model( listview );

	rows = gtk_tree_selection_get_selected_rows( selection, &model );

	if( g_list_length( rows ) == 1 ){
		path = ( GtkTreePath * ) rows->data;
		gtk_tree_model_get_iter( model, &iter, path );
		gtk_list_store_remove( GTK_LIST_STORE( model ), &iter );

		if( gtk_tree_model_get_iter( model, &iter, path ) ||
			gtk_tree_path_prev( path )){
			gtk_tree_view_set_cursor( listview, path, NULL, FALSE );
		}
	}

	g_list_foreach( rows, ( GFunc ) gtk_tree_path_free, NULL );
	g_list_free( rows );
}

static void
display_keyword( GtkTreeViewColumn *column, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, SchemesListData *data )
{
	display_label( column, cell, model, iter, data, SCHEMES_KEYWORD_COLUMN );
}

static void
display_description( GtkTreeViewColumn *column, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, SchemesListData *data )
{
	display_label( column, cell, model, iter, data, SCHEMES_DESC_COLUMN );
}

static void
display_label( GtkTreeViewColumn *column, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, SchemesListData *data, guint column_id )
{
	gboolean used;

	gtk_tree_model_get( model, iter, SCHEMES_ALREADY_USED_COLUMN, &used, -1 );
	g_object_set( cell, "style-set", FALSE, NULL );

	if( used ){
		g_object_set( cell, "style", PANGO_STYLE_ITALIC, "style-set", TRUE, NULL );
	}
}

static GtkButton *
get_add_button( BaseWindow *window )
{
	GtkButton *button;

	button = GTK_BUTTON( base_window_get_widget( window, "AddSchemeButton" ));

	return( button );
}

static GtkButton *
get_remove_button( BaseWindow *window )
{
	GtkButton *button;

	button = GTK_BUTTON( base_window_get_widget( window, "RemoveSchemeButton" ));

	return( button );
}

static SchemesListData *
get_schemes_list_data( GtkTreeView *treeview )
{
	SchemesListData *data;

	data = ( SchemesListData * ) g_object_get_data( G_OBJECT( treeview ), SCHEMES_LIST_DATA );

	if( data == NULL ){
		data = g_new0( SchemesListData, 1 );
		g_object_set_data( G_OBJECT( treeview ), SCHEMES_LIST_DATA, data );
		data->treeview = treeview;
	}

	return( data );
}
