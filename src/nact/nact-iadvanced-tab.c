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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <string.h>

#include <common/na-object-api.h>
#include <common/na-obj-action-class.h>
#include <common/na-obj-profile.h>
#include <common/na-utils.h>

#include "base-window.h"
#include "base-iprefs.h"
#include "nact-main-tab.h"
#include "nact-iadvanced-tab.h"

/* private interface data
 */
struct NactIAdvancedTabInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* column ordering
 */
enum {
	SCHEMES_CHECKBOX_COLUMN = 0,
	SCHEMES_KEYWORD_COLUMN,
	SCHEMES_DESC_COLUMN,
	SCHEMES_N_COLUMN
};

static GType         register_type( void );
static void          interface_base_init( NactIAdvancedTabInterface *klass );
static void          interface_base_finalize( NactIAdvancedTabInterface *klass );

static void          initial_load_create_schemes_selection_list( NactIAdvancedTab *instance );
static void          runtime_init_connect_signals( NactIAdvancedTab *instance, GtkTreeView *listview );
static void          runtime_init_setup_values( NactIAdvancedTab *instance, GtkTreeView *listview );
static void          on_tab_updatable_selection_updated( NactIAdvancedTab *instance, gint count_selected );
static gboolean      get_action_schemes_list( GtkTreeModel* scheme_model, GtkTreePath *path, GtkTreeIter* iter, GSList **schemes_list );
static GtkButton    *get_add_button( NactIAdvancedTab *instance );
static GtkButton    *get_button( NactIAdvancedTab *instance, const gchar *name );
static GtkButton    *get_remove_button( NactIAdvancedTab *instance );
static GSList       *get_schemes_default_list( NactIAdvancedTab *instance );
static GtkTreeModel *get_schemes_tree_model( NactIAdvancedTab *instance );
static GtkTreeView  *get_schemes_tree_view( NactIAdvancedTab *instance );
static void          on_add_scheme_clicked( GtkButton *button, NactIAdvancedTab *instance );
static void          on_remove_scheme_clicked( GtkButton *button, NactIAdvancedTab *instance );
static void          on_scheme_desc_edited( GtkCellRendererText *renderer, const gchar *path, const gchar *text, NactIAdvancedTab *instance );
static void          on_scheme_keyword_edited( GtkCellRendererText *renderer, const gchar *path, const gchar *text, NactIAdvancedTab *instance );
static void          on_scheme_list_selection_changed( GtkTreeSelection *selection, NactIAdvancedTab *instance );
static void          on_scheme_selection_toggled( GtkCellRendererToggle *renderer, gchar *path, NactIAdvancedTab *instance );
static gboolean      reset_schemes_list( GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data );
static void          scheme_cell_edited( NactIAdvancedTab *instance, const gchar *path_string, const gchar *text, gint column, gboolean *state, gchar **old_text );
static void          set_action_schemes( gchar *scheme, GtkTreeModel *model );

GType
nact_iadvanced_tab_get_type( void )
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
	static const gchar *thisfn = "nact_iadvanced_tab_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NactIAdvancedTabInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "NactIAdvancedTab", &info, 0 );

	g_type_interface_add_prerequisite( type, BASE_WINDOW_TYPE );

	return( type );
}

static void
interface_base_init( NactIAdvancedTabInterface *klass )
{
	static const gchar *thisfn = "nact_iadvanced_tab_interface_base_init";
	static gboolean initialized = FALSE;

	if( !initialized ){
		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NactIAdvancedTabInterfacePrivate, 1 );

		initialized = TRUE;
	}
}

static void
interface_base_finalize( NactIAdvancedTabInterface *klass )
{
	static const gchar *thisfn = "nact_iadvanced_tab_interface_base_finalize";
	static gboolean finalized = FALSE ;

	if( !finalized ){
		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );

		finalized = TRUE;
	}
}

void
nact_iadvanced_tab_initial_load_toplevel( NactIAdvancedTab *instance )
{
	static const gchar *thisfn = "nact_iadvanced_tab_initial_load_toplevel";

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

	initial_load_create_schemes_selection_list( instance );
}

/*
 * create the listview
 * initializes it with default values
 */
static void
initial_load_create_schemes_selection_list( NactIAdvancedTab *instance )
{
	static const char *thisfn = "nact_iadvanced_tab_initial_load_create_schemes_selection_list";
	GtkTreeView *listview;
	GtkListStore *model;
	GtkCellRenderer *toggled_cell;
	GtkTreeViewColumn *column;
	GtkCellRenderer *text_cell;

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

	model = gtk_list_store_new( SCHEMES_N_COLUMN, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING );
	listview = get_schemes_tree_view( instance );
	gtk_tree_view_set_model( listview, GTK_TREE_MODEL( model ));
	g_object_unref( model );

	toggled_cell = gtk_cell_renderer_toggle_new();
	column = gtk_tree_view_column_new_with_attributes(
			"scheme-selected",
			toggled_cell,
			"active", SCHEMES_CHECKBOX_COLUMN,
			NULL );
	gtk_tree_view_append_column( listview, column );

	text_cell = gtk_cell_renderer_text_new();
	g_object_set( G_OBJECT( text_cell ), "editable", TRUE, NULL );
	column = gtk_tree_view_column_new_with_attributes(
			"scheme-code",
			text_cell,
			"text", SCHEMES_KEYWORD_COLUMN,
			NULL );
	gtk_tree_view_append_column( listview, column );

	text_cell = gtk_cell_renderer_text_new();
	g_object_set( G_OBJECT( text_cell ), "editable", TRUE, NULL );
	column = gtk_tree_view_column_new_with_attributes(
			"scheme-description",
			text_cell,
			"text", SCHEMES_DESC_COLUMN,
			NULL );
	gtk_tree_view_append_column( listview, column );
}

void
nact_iadvanced_tab_runtime_init_toplevel( NactIAdvancedTab *instance )
{
	static const gchar *thisfn = "nact_iadvanced_tab_runtime_init_toplevel";
	GtkTreeView *listview;

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

	listview = get_schemes_tree_view( instance );

	runtime_init_connect_signals( instance, listview );

	runtime_init_setup_values( instance, listview );
}

static void
runtime_init_connect_signals( NactIAdvancedTab *instance, GtkTreeView *listview )
{
	static const gchar *thisfn = "nact_iadvanced_tab_runtime_init_connect_signals";
	GtkTreeViewColumn *column;
	GList *renderers;
	GtkButton *add_button, *remove_button;

	g_debug( "%s: instance=%p, listview=%p", thisfn, ( void * ) instance, ( void * ) listview );

	column = gtk_tree_view_get_column( listview, SCHEMES_CHECKBOX_COLUMN );
	renderers = gtk_tree_view_column_get_cell_renderers( column );
	g_signal_connect(
			G_OBJECT( renderers->data ),
			"toggled",
			G_CALLBACK( on_scheme_selection_toggled ),
			instance );

	column = gtk_tree_view_get_column( listview, SCHEMES_KEYWORD_COLUMN );
	renderers = gtk_tree_view_column_get_cell_renderers( column );
	g_signal_connect(
			G_OBJECT( renderers->data ),
			"edited",
			G_CALLBACK( on_scheme_keyword_edited ),
			instance );

	column = gtk_tree_view_get_column( listview, SCHEMES_DESC_COLUMN );
	renderers = gtk_tree_view_column_get_cell_renderers( column );
	g_signal_connect(
			G_OBJECT( renderers->data ),
			"edited",
			G_CALLBACK( on_scheme_desc_edited ),
			instance );

	add_button = get_add_button( instance );
	g_signal_connect(
			G_OBJECT( add_button ),
			"clicked",
			G_CALLBACK( on_add_scheme_clicked ),
			instance );

	remove_button = get_remove_button( instance );
	g_signal_connect(
			G_OBJECT( remove_button ),
			"clicked",
			G_CALLBACK( on_remove_scheme_clicked ),
			instance );

	g_signal_connect(
			G_OBJECT( gtk_tree_view_get_selection( listview )),
			"changed",
			G_CALLBACK( on_scheme_list_selection_changed ),
			instance );

	g_signal_connect(
			G_OBJECT( instance ),
			TAB_UPDATABLE_SIGNAL_SELECTION_UPDATED,
			G_CALLBACK( on_tab_updatable_selection_updated ),
			instance );
}

static void
runtime_init_setup_values( NactIAdvancedTab *instance, GtkTreeView *listview )
{
	static const gchar *thisfn = "nact_iadvanced_tab_runtime_init_setup_values";
	GtkListStore *model;
	GSList *schemes_list;
	GSList *iter;
	GtkTreeIter row;
	gchar **tokens;

	g_debug( "%s: instance=%p, listview=%p", thisfn, ( void * ) instance, ( void * ) listview );

	model = GTK_LIST_STORE( gtk_tree_view_get_model( listview ));

	schemes_list = get_schemes_default_list( instance );

	for( iter = schemes_list ; iter ; iter = iter->next ){

		tokens = g_strsplit(( gchar * ) iter->data, "|", 2 );
		gtk_list_store_append( model, &row );
		gtk_list_store_set( model, &row,
				SCHEMES_CHECKBOX_COLUMN, FALSE,
				SCHEMES_KEYWORD_COLUMN, tokens[0],
				SCHEMES_DESC_COLUMN, tokens[1],
				-1 );
		g_strfreev( tokens );
	}

	na_utils_free_string_list( schemes_list );
}

void
nact_iadvanced_tab_all_widgets_showed( NactIAdvancedTab *instance )
{
	static const gchar *thisfn = "nact_iadvanced_tab_all_widgets_showed";

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
}

void
nact_iadvanced_tab_dispose( NactIAdvancedTab *instance )
{
	static const gchar *thisfn = "nact_iadvanced_tab_dispose";

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
}

/**
 * Returns selected schemes as a list of strings.
 * The caller should call na_utils_free_string_list after use.
 */
GSList *
nact_iadvanced_tab_get_schemes( NactIAdvancedTab *instance )
{
	GSList *list = NULL;
	GtkTreeModel* scheme_model;

	scheme_model = get_schemes_tree_model( instance );
	gtk_tree_model_foreach( scheme_model, ( GtkTreeModelForeachFunc ) get_action_schemes_list, &list );

	return( list );
}

static void
on_tab_updatable_selection_updated( NactIAdvancedTab *instance, gint count_selected )
{
	static const gchar *thisfn = "nact_iadvanced_tab_on_tab_updatable_selection_updated";
	NAObjectProfile *profile = NULL;
	GtkTreeModel *scheme_model;
	GSList *schemes;
	GtkTreeView *listview;
	GtkButton *add, *remove;

	g_debug( "%s: instance=%p, count_selected=%d", thisfn, ( void * ) instance, count_selected );

	scheme_model = get_schemes_tree_model( instance );
	gtk_tree_model_foreach( scheme_model, ( GtkTreeModelForeachFunc ) reset_schemes_list, NULL );

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_PROFILE, &profile,
			NULL );

	if( profile ){
		schemes = na_object_profile_get_schemes( profile );
		g_slist_foreach( schemes, ( GFunc ) set_action_schemes, scheme_model );
	}

	listview = get_schemes_tree_view( instance );
	gtk_widget_set_sensitive( GTK_WIDGET( listview ), profile != NULL );

	add = get_add_button( instance );
	gtk_widget_set_sensitive( GTK_WIDGET( add ), profile != NULL );

	remove = get_remove_button( instance );
	gtk_widget_set_sensitive( GTK_WIDGET( remove ), profile != NULL );
}

/*
 * CommandExampleLabel is updated each time a field is modified
 * And at each time, we need the list of selected schemes
 */
static gboolean
get_action_schemes_list( GtkTreeModel* scheme_model, GtkTreePath *path, GtkTreeIter* iter, GSList **schemes_list )
{
	/*static const char *thisfn = "nact_iadvanced_tab_get_action_schemes_list";*/

	gboolean toggle_state;
	gchar* scheme;

	gtk_tree_model_get( scheme_model, iter, SCHEMES_CHECKBOX_COLUMN, &toggle_state, SCHEMES_KEYWORD_COLUMN, &scheme, -1 );

	if( toggle_state ){
		/*g_debug( "%s: adding '%s' scheme", thisfn, scheme );*/
		( *schemes_list ) = g_slist_append(( *schemes_list ), scheme );
	}

	 /* don't stop looping */
	return( FALSE );
}

static GtkButton *
get_add_button( NactIAdvancedTab *instance )
{
	return( get_button( instance, "AddSchemeButton" ));
}

static GtkButton *
get_button( NactIAdvancedTab *instance, const gchar *name )
{
	GtkWidget *button;

	button = base_window_get_widget( BASE_WINDOW( instance ), name );
	g_assert( GTK_IS_BUTTON( button ));

	return( GTK_BUTTON( button ));
}

static GtkButton *
get_remove_button( NactIAdvancedTab *instance )
{
	return( get_button( instance, "RemoveSchemeButton" ));
}

/*
 * return default schemes list
 * the returned list must be released with na_utils_free_string_list()
 */
static GSList *
get_schemes_default_list( NactIAdvancedTab *instance )
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

static GtkTreeModel *
get_schemes_tree_model( NactIAdvancedTab *instance )
{
	GtkTreeView *listview;
	GtkTreeModel *model;

	listview = get_schemes_tree_view( instance );
	model = gtk_tree_view_get_model( listview );

	return( model );
}

static GtkTreeView *
get_schemes_tree_view( NactIAdvancedTab *instance )
{
	GtkWidget *treeview;

	treeview = base_window_get_widget( BASE_WINDOW( instance ), "SchemesTreeView" );
	g_assert( GTK_IS_TREE_VIEW( treeview ));

	return( GTK_TREE_VIEW( treeview ));
}

/* TODO: set the selection on the newly created scheme */
static void
on_add_scheme_clicked( GtkButton *button, NactIAdvancedTab *instance )
{
	GtkTreeModel *model = get_schemes_tree_model( instance );
	GtkTreeIter row;

	gtk_list_store_append(
			GTK_LIST_STORE( model ),
			&row );

	gtk_list_store_set(
			GTK_LIST_STORE( model ),
			&row,
			SCHEMES_CHECKBOX_COLUMN, FALSE,
			/* i18n notes : scheme name set for a new entry in the scheme list */
			SCHEMES_KEYWORD_COLUMN, _( "new-scheme" ),
			SCHEMES_DESC_COLUMN, _( "New scheme description" ),
			-1 );
}

static void
on_remove_scheme_clicked( GtkButton *button, NactIAdvancedTab *instance )
{
	NAObjectProfile *edited;
	GtkTreeView *listview;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *selected_values_path = NULL;
	GtkTreeIter iter;
	GtkTreePath *path;
	GList *il;
	gboolean toggle_state;
	gchar *scheme;

	listview = get_schemes_tree_view( instance );
	selection = gtk_tree_view_get_selection( listview );
	model = get_schemes_tree_model( instance );

	selected_values_path = gtk_tree_selection_get_selected_rows( selection, &model );

	for( il = selected_values_path ; il ; il = il->next ){
		path = ( GtkTreePath * ) il->data;
		gtk_tree_model_get_iter( model, &iter, path );
		gtk_tree_model_get( model, &iter, SCHEMES_CHECKBOX_COLUMN, &toggle_state, SCHEMES_KEYWORD_COLUMN, &scheme, -1 );
		gtk_list_store_remove( GTK_LIST_STORE( model ), &iter );

		if( toggle_state ){
			g_object_get(
					G_OBJECT( instance ),
					TAB_UPDATABLE_PROP_EDITED_PROFILE, &edited,
					NULL );
			na_object_profile_set_scheme( edited, scheme, FALSE );
		}

		g_free( scheme );
	}

	g_list_foreach( selected_values_path, ( GFunc ) gtk_tree_path_free, NULL );
	g_list_free( selected_values_path );

	/*g_signal_emit_by_name( G_OBJECT( window ), NACT_SIGNAL_MODIFIED_FIELD );*/
}

static void
on_scheme_desc_edited( GtkCellRendererText *renderer, const gchar *path, const gchar *text, NactIAdvancedTab *instance )
{
	/*static const gchar *thisfn = "nact_iadvanced_tab_on_scheme_desc_edited";
	g_debug( "%s: renderer=%p, path=%s, text=%s, user_data=%p", thisfn, renderer, path, text, user_data );*/

	scheme_cell_edited( instance, path, text, SCHEMES_DESC_COLUMN, NULL, NULL );
}

static void
on_scheme_keyword_edited( GtkCellRendererText *renderer, const gchar *path, const gchar *text, NactIAdvancedTab *instance )
{
	/*static const gchar *thisfn = "nact_iadvanced_tab_on_scheme_keyword_edited";*/
	/*g_debug( "%s: renderer=%p, path=%s, text=%s, user_data=%p", thisfn, renderer, path, text, user_data );*/

	gboolean state = FALSE;
	gchar *old_text = NULL;
	NAObjectProfile *edited;

	scheme_cell_edited( instance, path, text, SCHEMES_KEYWORD_COLUMN, &state, &old_text );

	if( state ){
		/*g_debug( "%s: old_scheme=%s", thisfn, old_text );*/
		g_object_get(
				G_OBJECT( instance ),
				TAB_UPDATABLE_PROP_EDITED_PROFILE, &edited,
				NULL );
		na_object_profile_set_scheme( edited, old_text, FALSE );
		na_object_profile_set_scheme( edited, text, TRUE );
	}

	g_free( old_text );

	/*g_signal_emit_by_name( G_OBJECT( window ), NACT_SIGNAL_MODIFIED_FIELD );*/
}

static void
on_scheme_list_selection_changed( GtkTreeSelection *selection, NactIAdvancedTab *instance )
{
	/*static const gchar *thisfn = "nact_iadvanced_tab_on_scheme_list_selection_changed";
	g_debug( "%s: selection=%p, user_data=%p", thisfn, selection, user_data );*/

	GtkButton *button;

	button = get_remove_button( instance );

	if( gtk_tree_selection_count_selected_rows( selection )){
		gtk_widget_set_sensitive( GTK_WIDGET( button ), TRUE );
	} else {
		gtk_widget_set_sensitive( GTK_WIDGET( button ), FALSE );
	}
}

static void
on_scheme_selection_toggled( GtkCellRendererToggle *renderer, gchar *path, NactIAdvancedTab *instance )
{
	/*static const gchar *thisfn = "nact_iadvanced_tab_on_scheme_selection_toggled";*/
	/*g_debug( "%s: renderer=%p, path=%s, user_data=%p", thisfn, renderer, path, user_data );*/

	NAObjectProfile *edited;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *tree_path;
	gboolean state;
	gchar *scheme;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_PROFILE, &edited,
			NULL );

	if( edited ){
		model = get_schemes_tree_model( instance );

		tree_path = gtk_tree_path_new_from_string( path );
		gtk_tree_model_get_iter( model, &iter, tree_path );
		gtk_tree_path_free( tree_path );

		gtk_tree_model_get( model, &iter, SCHEMES_CHECKBOX_COLUMN, &state, SCHEMES_KEYWORD_COLUMN, &scheme, -1 );

		/* gtk_tree_model_get: returns the previous state
		g_debug( "%s: gtk_tree_model_get returns keyword=%s state=%s", thisfn, scheme, state ? "True":"False" );*/

		gtk_list_store_set( GTK_LIST_STORE( model ), &iter, SCHEMES_CHECKBOX_COLUMN, !state, -1 );

		na_object_profile_set_scheme( edited, scheme, !state );

		g_free( scheme );

		/*g_signal_emit_by_name( G_OBJECT( window ), NACT_SIGNAL_MODIFIED_FIELD );*/
	}
}

static gboolean
reset_schemes_list( GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data )
{
	gtk_list_store_set( GTK_LIST_STORE( model ), iter, SCHEMES_CHECKBOX_COLUMN, FALSE, -1 );

	return( FALSE ); /* don't stop looping */
}

static void
scheme_cell_edited( NactIAdvancedTab *instance, const gchar *path_string, const gchar *text, gint column, gboolean *state, gchar **old_text )
{
	GtkTreeModel *model = get_schemes_tree_model( instance );
	GtkTreeIter iter;
	GtkTreePath *path;

	path = gtk_tree_path_new_from_string( path_string );
	gtk_tree_model_get_iter( model, &iter, path );
	gtk_tree_path_free( path );

	if( state && old_text ){
		gtk_tree_model_get( model, &iter, SCHEMES_CHECKBOX_COLUMN, state, SCHEMES_KEYWORD_COLUMN, old_text, -1 );
	}

	gtk_list_store_set( GTK_LIST_STORE( model ), &iter, column, text, -1 );
}

static void
set_action_schemes( gchar *scheme, GtkTreeModel *model )
{
	GtkTreeIter iter;
	gboolean iter_ok = FALSE;
	gboolean found = FALSE;
	gchar *i_scheme;

	iter_ok = gtk_tree_model_get_iter_first( model, &iter );

	while( iter_ok && !found ){
		gtk_tree_model_get( model, &iter, SCHEMES_KEYWORD_COLUMN, &i_scheme, -1 );

		if( g_ascii_strcasecmp( scheme, i_scheme) == 0 ){
			gtk_list_store_set( GTK_LIST_STORE( model ), &iter, SCHEMES_CHECKBOX_COLUMN, TRUE, -1 );
			found = TRUE;
		}

		g_free( i_scheme );
		iter_ok = gtk_tree_model_iter_next( model, &iter );
	}

	if( !found ){
		gtk_list_store_append( GTK_LIST_STORE( model ), &iter );
		gtk_list_store_set(
				GTK_LIST_STORE( model ),
				&iter,
				SCHEMES_CHECKBOX_COLUMN, TRUE,
				SCHEMES_KEYWORD_COLUMN, scheme,
				SCHEMES_DESC_COLUMN, "",
				-1 );
	}
}
