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

#include <common/na-utils.h>

#include "nact-iadvanced-tab.h"
#include "nact-iprefs.h"

/* private interface data
 */
struct NactIAdvancedTabInterfacePrivate {
};

/* column ordering
 */
enum {
	SCHEMES_CHECKBOX_COLUMN = 0,
	SCHEMES_KEYWORD_COLUMN,
	SCHEMES_DESC_COLUMN,
	SCHEMES_N_COLUMN
};

static GType            register_type( void );
static void             interface_base_init( NactIAdvancedTabInterface *klass );
static void             interface_base_finalize( NactIAdvancedTabInterface *klass );

static NAActionProfile *v_get_edited_profile( NactWindow *window );
static void             v_field_modified( NactWindow *window );

static void             on_scheme_selection_toggled( GtkCellRendererToggle *renderer, gchar *path, gpointer user_data );
static void             on_scheme_keyword_edited( GtkCellRendererText *renderer, const gchar *path, const gchar *text, gpointer user_data );
static void             on_scheme_desc_edited( GtkCellRendererText *renderer, const gchar *path, const gchar *text, gpointer user_data );
static void             on_scheme_list_selection_changed( GtkTreeSelection *selection, gpointer user_data );
static void             on_add_scheme_clicked( GtkButton *button, gpointer user_data );
static void             on_remove_scheme_clicked( GtkButton *button, gpointer user_data );
static void             scheme_cell_edited( NactWindow *window, const gchar *path_string, const gchar *text, gint column, gboolean *state, gchar **old_text );
static GtkTreeView     *get_schemes_tree_view( NactWindow *window );
static GtkTreeModel    *get_schemes_tree_model( NactWindow *window );
static void             create_schemes_selection_list( NactWindow *window );
/*static gboolean         get_action_schemes_list( GtkTreeModel* scheme_model, GtkTreePath *path, GtkTreeIter* iter, gpointer data );*/
static GSList          *get_schemes_default_list( NactWindow *window );
static gboolean         reset_schemes_list( GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data );
static void             set_action_schemes( gchar *scheme, GtkTreeModel *model );
static GtkButton       *get_add_button( NactWindow *window );
static GtkButton       *get_remove_button( NactWindow *window );

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
	g_debug( "%s", thisfn );

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

	GType type = g_type_register_static( G_TYPE_INTERFACE, "NactIAdvancedTab", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( NactIAdvancedTabInterface *klass )
{
	static const gchar *thisfn = "nact_iadvanced_tab_interface_base_init";
	static gboolean initialized = FALSE;

	if( !initialized ){
		g_debug( "%s: klass=%p", thisfn, klass );

		klass->private = g_new0( NactIAdvancedTabInterfacePrivate, 1 );

		klass->get_edited_profile = NULL;
		klass->field_modified = NULL;

		initialized = TRUE;
	}
}

static void
interface_base_finalize( NactIAdvancedTabInterface *klass )
{
	static const gchar *thisfn = "nact_iadvanced_tab_interface_base_finalize";
	static gboolean finalized = FALSE ;

	if( !finalized ){
		g_debug( "%s: klass=%p", thisfn, klass );

		g_free( klass->private );

		finalized = TRUE;
	}
}

void
nact_iadvanced_tab_initial_load( NactWindow *dialog )
{
	static const gchar *thisfn = "nact_iadvanced_tab_initial_load";
	g_debug( "%s: dialog=%p", thisfn, dialog );

	create_schemes_selection_list( dialog );
}

void
nact_iadvanced_tab_runtime_init( NactWindow *dialog )
{
	static const gchar *thisfn = "nact_iadvanced_tab_runtime_init";
	g_debug( "%s: dialog=%p", thisfn, dialog );

	GtkTreeView *scheme_widget = get_schemes_tree_view( dialog );

	GtkTreeViewColumn *column = gtk_tree_view_get_column( scheme_widget, SCHEMES_CHECKBOX_COLUMN );
	GList *renderers = gtk_tree_view_column_get_cell_renderers( column );
	nact_window_signal_connect( dialog, G_OBJECT( renderers->data ), "toggled", G_CALLBACK( on_scheme_selection_toggled ));

	column = gtk_tree_view_get_column( scheme_widget, SCHEMES_KEYWORD_COLUMN );
	renderers = gtk_tree_view_column_get_cell_renderers( column );
	nact_window_signal_connect( dialog, G_OBJECT( renderers->data ), "edited", G_CALLBACK( on_scheme_keyword_edited ));

	column = gtk_tree_view_get_column( scheme_widget, SCHEMES_DESC_COLUMN );
	renderers = gtk_tree_view_column_get_cell_renderers( column );
	nact_window_signal_connect( dialog, G_OBJECT( renderers->data ), "edited", G_CALLBACK( on_scheme_desc_edited ));

	GtkButton *button = get_add_button( dialog );
	nact_window_signal_connect( dialog, G_OBJECT( button ), "clicked", G_CALLBACK( on_add_scheme_clicked ));
	GtkButton *remove_button = get_remove_button( dialog );
	nact_window_signal_connect( dialog, G_OBJECT( remove_button ), "clicked", G_CALLBACK( on_remove_scheme_clicked ));

	nact_window_signal_connect( dialog, G_OBJECT( gtk_tree_view_get_selection( scheme_widget )), "changed", G_CALLBACK( on_scheme_list_selection_changed ));
}

void
nact_iadvanced_tab_all_widgets_showed( NactWindow *dialog )
{
	static const gchar *thisfn = "nact_iadvanced_tab_all_widgets_showed";
	g_debug( "%s: dialog=%p", thisfn, dialog );
}

void
nact_iadvanced_tab_dispose( NactWindow *dialog )
{
	static const gchar *thisfn = "nact_iadvanced_tab_dispose";
	g_debug( "%s: dialog=%p", thisfn, dialog );
}

void
nact_iadvanced_tab_set_profile( NactWindow *dialog, NAActionProfile *profile )
{
	static const gchar *thisfn = "nact_iadvanced_tab_runtime_init";
	g_debug( "%s: dialog=%p, profile=%p", thisfn, dialog, profile );

	GtkTreeModel *scheme_model = get_schemes_tree_model( dialog );
	gtk_tree_model_foreach( scheme_model, ( GtkTreeModelForeachFunc ) reset_schemes_list, NULL );

	if( profile ){
		GSList *schemes = na_action_profile_get_schemes( profile );
		g_slist_foreach( schemes, ( GFunc ) set_action_schemes, scheme_model );
	}

	GtkTreeView *scheme_widget = get_schemes_tree_view( dialog );
	gtk_widget_set_sensitive( GTK_WIDGET( scheme_widget ), profile != NULL );

	GtkButton *add = get_add_button( dialog );
	gtk_widget_set_sensitive( GTK_WIDGET( add ), profile != NULL );

	GtkButton *remove = get_remove_button( dialog );
	gtk_widget_set_sensitive( GTK_WIDGET( remove ), profile != NULL );
}

static NAActionProfile *
v_get_edited_profile( NactWindow *window )
{
	g_assert( NACT_IS_IADVANCED_TAB( window ));

	if( NACT_IADVANCED_TAB_GET_INTERFACE( window )->get_edited_profile ){
		return( NACT_IADVANCED_TAB_GET_INTERFACE( window )->get_edited_profile( window ));
	}

	return( NULL );
}

static void
v_field_modified( NactWindow *window )
{
	g_assert( NACT_IS_IADVANCED_TAB( window ));

	if( NACT_IADVANCED_TAB_GET_INTERFACE( window )->field_modified ){
		NACT_IADVANCED_TAB_GET_INTERFACE( window )->field_modified( window );
	}
}

static void
on_scheme_selection_toggled( GtkCellRendererToggle *renderer, gchar *path, gpointer user_data )
{
	/*static const gchar *thisfn = "nact_iadvanced_tab_on_scheme_selection_toggled";*/
	/*g_debug( "%s: renderer=%p, path=%s, user_data=%p", thisfn, renderer, path, user_data );*/
	g_assert( NACT_IS_WINDOW( user_data ));
	NactWindow *dialog = NACT_WINDOW( user_data );

	NAActionProfile *edited = NA_ACTION_PROFILE( v_get_edited_profile( dialog ));
	if( edited ){
		GtkTreeModel *model = get_schemes_tree_model( dialog );
		GtkTreeIter iter;

		GtkTreePath *tree_path = gtk_tree_path_new_from_string( path );
		gtk_tree_model_get_iter( model, &iter, tree_path );
		gtk_tree_path_free( tree_path );

		gboolean state;
		gchar *scheme;
		gtk_tree_model_get( model, &iter, SCHEMES_CHECKBOX_COLUMN, &state, SCHEMES_KEYWORD_COLUMN, &scheme, -1 );

		/* gtk_tree_model_get: returns the previous state
		g_debug( "%s: gtk_tree_model_get returns keyword=%s state=%s", thisfn, scheme, state ? "True":"False" );*/

		gtk_list_store_set( GTK_LIST_STORE( model ), &iter, SCHEMES_CHECKBOX_COLUMN, !state, -1 );

		na_action_profile_set_scheme( edited, scheme, !state );

		g_free( scheme );
	}

	v_field_modified( dialog );
}

static void
on_scheme_keyword_edited( GtkCellRendererText *renderer, const gchar *path, const gchar *text, gpointer user_data )
{
	/*static const gchar *thisfn = "nact_iadvanced_tab_on_scheme_keyword_edited";*/
	/*g_debug( "%s: renderer=%p, path=%s, text=%s, user_data=%p", thisfn, renderer, path, text, user_data );*/

	g_assert( NACT_IS_WINDOW( user_data ));
	NactWindow *dialog = NACT_WINDOW( user_data );

	gboolean state = FALSE;
	gchar *old_text = NULL;
	scheme_cell_edited( dialog, path, text, SCHEMES_KEYWORD_COLUMN, &state, &old_text );

	if( state ){
		/*g_debug( "%s: old_scheme=%s", thisfn, old_text );*/
		NAActionProfile *edited = NA_ACTION_PROFILE( v_get_edited_profile( dialog ));
		na_action_profile_set_scheme( edited, old_text, FALSE );
		na_action_profile_set_scheme( edited, text, TRUE );
	}

	g_free( old_text );
	v_field_modified( dialog );
}

static void
on_scheme_desc_edited( GtkCellRendererText *renderer, const gchar *path, const gchar *text, gpointer user_data )
{
	/*static const gchar *thisfn = "nact_iadvanced_tab_on_scheme_desc_edited";
	g_debug( "%s: renderer=%p, path=%s, text=%s, user_data=%p", thisfn, renderer, path, text, user_data );*/

	g_assert( NACT_IS_WINDOW( user_data ));
	NactWindow *dialog = NACT_WINDOW( user_data );

	scheme_cell_edited( dialog, path, text, SCHEMES_DESC_COLUMN, NULL, NULL );
}

static void
on_scheme_list_selection_changed( GtkTreeSelection *selection, gpointer user_data )
{
	/*static const gchar *thisfn = "nact_iadvanced_tab_on_scheme_list_selection_changed";
	g_debug( "%s: selection=%p, user_data=%p", thisfn, selection, user_data );*/

	g_assert( NACT_IS_WINDOW( user_data ));
	NactWindow *dialog = NACT_WINDOW( user_data );

	GtkWidget *button = GTK_WIDGET( get_remove_button( dialog ));

	if( gtk_tree_selection_count_selected_rows( selection )){
		gtk_widget_set_sensitive( button, TRUE );
	} else {
		gtk_widget_set_sensitive( button, FALSE );
	}
}

/* TODO: set the selection on the newly created scheme */
static void
on_add_scheme_clicked( GtkButton *button, gpointer user_data )
{
	GtkTreeModel *model = get_schemes_tree_model( NACT_WINDOW( user_data ));
	GtkTreeIter row;
	gtk_list_store_append( GTK_LIST_STORE( model ), &row );
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
on_remove_scheme_clicked( GtkButton *button, gpointer user_data )
{
	g_assert( NACT_IS_WINDOW( user_data ));
	NactWindow *dialog = NACT_WINDOW( user_data );

	GtkTreeView *listview = get_schemes_tree_view( dialog );
	GtkTreeSelection *selection = gtk_tree_view_get_selection( listview );
	GtkTreeModel *model = get_schemes_tree_model( dialog );

	GList *selected_values_path = NULL;
	GtkTreeIter iter;
	GtkTreePath *path;
	GList *il;
	gboolean toggle_state;
	gchar *scheme;

	selected_values_path = gtk_tree_selection_get_selected_rows( selection, &model );

	for( il = selected_values_path ; il ; il = il->next ){
		path = ( GtkTreePath * ) il->data;
		gtk_tree_model_get_iter( model, &iter, path );
		gtk_tree_model_get( model, &iter, SCHEMES_CHECKBOX_COLUMN, &toggle_state, SCHEMES_KEYWORD_COLUMN, &scheme, -1 );
		gtk_list_store_remove( GTK_LIST_STORE( model ), &iter );

		if( toggle_state ){
			NAActionProfile *edited = NA_ACTION_PROFILE( v_get_edited_profile( dialog ));
			na_action_profile_set_scheme( edited, scheme, FALSE );
		}
	}

	g_list_foreach( selected_values_path, ( GFunc ) gtk_tree_path_free, NULL );
	g_list_free( selected_values_path );

	v_field_modified( dialog );
}

static void
scheme_cell_edited( NactWindow *window, const gchar *path_string, const gchar *text, gint column, gboolean *state, gchar **old_text )
{
	GtkTreeModel *model = get_schemes_tree_model( window );
	GtkTreeIter iter;

	GtkTreePath *path = gtk_tree_path_new_from_string( path_string );
	gtk_tree_model_get_iter( model, &iter, path );
	gtk_tree_path_free( path );

	if( state && old_text ){
		gtk_tree_model_get( model, &iter, SCHEMES_CHECKBOX_COLUMN, state, SCHEMES_KEYWORD_COLUMN, old_text, -1 );
	}

	gtk_list_store_set( GTK_LIST_STORE( model ), &iter, column, g_strdup( text ), -1 );

}

static GtkTreeView *
get_schemes_tree_view( NactWindow *window )
{
	return( GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( window ), "SchemesTreeView" )));
}

static GtkTreeModel *
get_schemes_tree_model( NactWindow *window )
{
	GtkTreeView *schemes_view = get_schemes_tree_view( window );
	return( gtk_tree_view_get_model( schemes_view ));
}

static void
create_schemes_selection_list( NactWindow *window )
{
	static const char *thisfn = "nact_iadvanced_tab_create_schemes_selection_list";
	g_debug( "%s: window=%p", thisfn, window );
	g_assert( NACT_IS_IADVANCED_TAB( window ));

	GtkWidget *listview = GTK_WIDGET( get_schemes_tree_view( window ));
	GSList* schemes_list = get_schemes_default_list( window );
	GtkListStore *model = gtk_list_store_new( SCHEMES_N_COLUMN, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING );

	GSList *iter;
	GtkTreeIter row;
	for( iter = schemes_list ; iter ; iter = iter->next ){

		gchar **tokens = g_strsplit(( gchar * ) iter->data, "|", 2 );
		gtk_list_store_append( model, &row );
		gtk_list_store_set( model, &row,
				SCHEMES_CHECKBOX_COLUMN, FALSE,
				SCHEMES_KEYWORD_COLUMN, tokens[0],
				SCHEMES_DESC_COLUMN, tokens[1],
				-1 );
		g_strfreev( tokens );
	}

	na_utils_free_string_list( schemes_list );

	gtk_tree_view_set_model( GTK_TREE_VIEW( listview ), GTK_TREE_MODEL( model ));
	g_object_unref( model );

	GtkCellRenderer *toggled_cell = gtk_cell_renderer_toggle_new();
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
			"scheme-selected",
			toggled_cell,
			"active", SCHEMES_CHECKBOX_COLUMN,
			NULL );
	gtk_tree_view_append_column( GTK_TREE_VIEW( listview ), column );
	/*g_debug( "%s: toggled_cell=%p", thisfn, toggled_cell );*/

	GtkCellRenderer *text_cell = gtk_cell_renderer_text_new();
	g_object_set( G_OBJECT( text_cell ), "editable", TRUE, NULL );
	column = gtk_tree_view_column_new_with_attributes(
			"scheme-code",
			text_cell,
			"text", SCHEMES_KEYWORD_COLUMN,
			NULL );
	gtk_tree_view_append_column( GTK_TREE_VIEW( listview ), column );

	text_cell = gtk_cell_renderer_text_new();
	g_object_set( G_OBJECT( text_cell ), "editable", TRUE, NULL );
	column = gtk_tree_view_column_new_with_attributes(
			"scheme-description",
			text_cell,
			"text", SCHEMES_DESC_COLUMN,
			NULL );
	gtk_tree_view_append_column( GTK_TREE_VIEW( listview ), column );
}

/*static gboolean
get_action_schemes_list( GtkTreeModel* scheme_model, GtkTreePath *path, GtkTreeIter* iter, gpointer data )
{
	static const char *thisfn = "nact_iadvanced_tab_get_action_schemes_list";

	GSList** list = data;
	gboolean toggle_state;
	gchar* scheme;

	gtk_tree_model_get (scheme_model, iter, SCHEMES_CHECKBOX_COLUMN, &toggle_state, SCHEMES_KEYWORD_COLUMN, &scheme, -1);

	if (toggle_state)
	{
		g_debug( "%s: adding '%s' scheme", thisfn, scheme );
		(*list) = g_slist_append ((*list), scheme);
	}
	else
	{
		g_free (scheme);
	}

	return FALSE; *//* Don't stop looping *//*
}*/

static GSList *
get_schemes_default_list( NactWindow *window )
{
	GSList *list = NULL;

	/* i18n notes : description of 'file' scheme */
	list = g_slist_append( list, g_strdup( _( "file|Local files")));
	/* i18n notes : description of 'sftp' scheme */
	list = g_slist_append( list, g_strdup( _( "sftp|SSH files")));
	/* i18n notes : description of 'smb' scheme */
	list = g_slist_append( list, g_strdup( _( "smb|Windows files")));
	/* i18n notes : description of 'ftp' scheme */
	list = g_slist_append( list, g_strdup( _( "ftp|FTP files")));
	/* i18n notes : description of 'dav' scheme */
	list = g_slist_append( list, g_strdup( _( "dav|WebDAV files")));

	return( list );
}

static gboolean
reset_schemes_list( GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data )
{
	gtk_list_store_set( GTK_LIST_STORE( model ), iter, SCHEMES_CHECKBOX_COLUMN, FALSE, -1 );

	return( FALSE ); /* don't stop looping */
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

static GtkButton *
get_add_button( NactWindow *window )
{
	return( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "AddSchemeButton" )));
}

static GtkButton *
get_remove_button( NactWindow *window )
{
	return( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "RemoveSchemeButton" )));
}
