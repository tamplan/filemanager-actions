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

#include <common/na-action.h>
#include <common/na-action-profile.h>
#include <common/na-utils.h>

#include "nact-iprofile-conditions.h"

/* private interface data
 */
struct NactIProfileConditionsInterfacePrivate {
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
static void          interface_base_init( NactIProfileConditionsInterface *klass );
static void          interface_base_finalize( NactIProfileConditionsInterface *klass );

static GObject      *v_get_edited_profile( NactWindow *window );
static void          v_field_modified( NactWindow *window );

static gchar        *basenames_to_text( GSList *basenames );
static GSList       *text_to_basenames( const gchar *text );
static GtkTreeView  *get_schemes_tree_view( NactWindow *window );
static GtkTreeModel *get_schemes_tree_model( NactWindow *window );
static void          create_schemes_selection_list( NactWindow *window );
static GSList       *get_schemes_default_list( NactWindow *window );

static void          on_path_changed( GtkEntry *entry, gpointer user_data );
static void          on_parameters_changed( GtkEntry *entry, gpointer user_data );
static void          on_basenames_changed( GtkEntry *entry, gpointer user_data );
static void          on_scheme_selection_toggled( GtkCellRendererToggle *renderer, gchar *path, gpointer user_data );
static void          on_scheme_keyword_edited( GtkCellRendererText *renderer, const gchar *path, const gchar *text, gpointer user_data );
static void          on_scheme_desc_edited( GtkCellRendererText *renderer, const gchar *path, const gchar *text, gpointer user_data );
static void          on_scheme_list_selection_changed( GtkTreeSelection *selection, gpointer user_data );

GType
nact_iprofile_conditions_get_type( void )
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
	static const gchar *thisfn = "nact_iprofile_conditions_register_type";
	g_debug( "%s", thisfn );

	static const GTypeInfo info = {
		sizeof( NactIProfileConditionsInterface ),
		( GBaseInitFunc ) interface_base_init,
		( GBaseFinalizeFunc ) interface_base_finalize,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL
	};

	GType type = g_type_register_static( G_TYPE_INTERFACE, "NactIProfileConditions", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( NactIProfileConditionsInterface *klass )
{
	static const gchar *thisfn = "nact_iprofile_conditions_interface_base_init";
	static gboolean initialized = FALSE;

	if( !initialized ){
		g_debug( "%s: klass=%p", thisfn, klass );

		klass->private = g_new0( NactIProfileConditionsInterfacePrivate, 1 );

		klass->get_edited_profile = NULL;
		klass->field_modified = NULL;

		initialized = TRUE;
	}
}

static void
interface_base_finalize( NactIProfileConditionsInterface *klass )
{
	static const gchar *thisfn = "nact_iprofile_conditions_interface_base_finalize";
	static gboolean finalized = FALSE ;

	if( !finalized ){
		g_debug( "%s: klass=%p", thisfn, klass );

		g_free( klass->private );

		finalized = TRUE;
	}
}

void
nact_iprofile_conditions_initial_load( NactWindow *dialog, NAActionProfile *profile )
{
	create_schemes_selection_list( dialog );
}

void
nact_iprofile_conditions_runtime_init( NactWindow *dialog, NAActionProfile *profile )
{
	static const gchar *thisfn = "nact_iprofile_conditions_runtime_init";
	g_debug( "%s: dialog=%p, profile=%p", thisfn, dialog, profile );

	GtkWidget *path_widget = base_window_get_widget( BASE_WINDOW( dialog ), "CommandPathEntry" );
	nact_window_signal_connect( dialog, G_OBJECT( path_widget ), "changed", G_CALLBACK( on_path_changed ));
	gchar *path = na_action_profile_get_path( profile );
	gtk_entry_set_text( GTK_ENTRY( path_widget ), path );
	g_free( path );

	GtkWidget *button = base_window_get_widget( BASE_WINDOW( dialog ), "PathBrowseButton" );
	/* TODO: implement path browse button */
	gtk_widget_set_sensitive( button, FALSE );

	GtkWidget *parameters_widget = base_window_get_widget( BASE_WINDOW( dialog ), "CommandParamsEntry" );
	nact_window_signal_connect( dialog, G_OBJECT( parameters_widget ), "changed", G_CALLBACK( on_parameters_changed ));
	gchar *parameters = na_action_profile_get_parameters( profile );
	gtk_entry_set_text( GTK_ENTRY( parameters_widget ), parameters );
	g_free( parameters );

	button = base_window_get_widget( BASE_WINDOW( dialog ), "LegendButton" );
	/* TODO: implement legend button */
	gtk_widget_set_sensitive( button, FALSE );

	GtkWidget *basenames_widget = base_window_get_widget( BASE_WINDOW( dialog ), "PatternEntry" );
	nact_window_signal_connect( dialog, G_OBJECT( basenames_widget ), "changed", G_CALLBACK( on_basenames_changed ));
	GSList *basenames = na_action_profile_get_basenames( profile );
	gchar *basenames_text = basenames_to_text( basenames );
	gtk_entry_set_text( GTK_ENTRY( basenames_widget ), basenames_text );
	g_free( basenames_text );
	na_utils_free_string_list( basenames );

	/* TODO: match case */
	/* TODO: mime types */
	/* TODO: file/dir */
	/* TODO: multiple selection */

	GtkTreeView *scheme_widget = get_schemes_tree_view( dialog );

	/* TODO: set schemes */
	GtkTreeViewColumn *column = gtk_tree_view_get_column( scheme_widget, SCHEMES_CHECKBOX_COLUMN );
	GList *renderers = gtk_tree_view_column_get_cell_renderers( column );
	nact_window_signal_connect( dialog, G_OBJECT( renderers->data ), "toggled", G_CALLBACK( on_scheme_selection_toggled ));

	column = gtk_tree_view_get_column( scheme_widget, SCHEMES_KEYWORD_COLUMN );
	renderers = gtk_tree_view_column_get_cell_renderers( column );
	nact_window_signal_connect( dialog, G_OBJECT( renderers->data ), "edited", G_CALLBACK( on_scheme_keyword_edited ));

	column = gtk_tree_view_get_column( scheme_widget, SCHEMES_DESC_COLUMN );
	renderers = gtk_tree_view_column_get_cell_renderers( column );
	nact_window_signal_connect( dialog, G_OBJECT( renderers->data ), "edited", G_CALLBACK( on_scheme_desc_edited ));

	nact_window_signal_connect( dialog, G_OBJECT( gtk_tree_view_get_selection( scheme_widget )), "changed", G_CALLBACK( on_scheme_list_selection_changed ));

	/* TODO: add scheme */
	/* TODO: remove scheme */
}

static GObject *
v_get_edited_profile( NactWindow *window )
{
	g_assert( NACT_IS_IPROFILE_CONDITIONS( window ));

	if( NACT_IPROFILE_CONDITIONS_GET_INTERFACE( window )->get_edited_profile ){
		return( NACT_IPROFILE_CONDITIONS_GET_INTERFACE( window )->get_edited_profile( window ));
	}

	return( NULL );
}

static void
v_field_modified( NactWindow *window )
{
	g_assert( NACT_IS_IPROFILE_CONDITIONS( window ));

	if( NACT_IPROFILE_CONDITIONS_GET_INTERFACE( window )->field_modified ){
		NACT_IPROFILE_CONDITIONS_GET_INTERFACE( window )->field_modified( window );
	}
}

static gchar *
basenames_to_text( GSList *basenames )
{
	GSList *ib;
	gchar *tmp;
	gchar *text = g_strdup( "" );

	for( ib = basenames ; ib ; ib = ib->next ){
		if( strlen( text )){
			tmp = g_strdup_printf( "%s; ", text );
			g_free( text );
			text = tmp;
		}
		tmp = g_strdup_printf( "%s%s", text, ( gchar * ) ib->data );
		g_free( text );
		text = tmp;
	}

	return( text );
}

static GSList *
text_to_basenames( const gchar *text )
{
	GSList *basenames = NULL;
	gchar **tokens, **iter;
	gchar *tmp;
	gchar *source = g_strdup( text );

	tmp = g_strstrip( source );
	if( !strlen( tmp )){
		basenames = g_slist_append( basenames, g_strdup( "*" ));

	} else {
		tokens = g_strsplit( source, ";", -1 );
		iter = tokens;

		while( *iter ){
			tmp = g_strstrip( *iter );
			basenames = g_slist_append( basenames, g_strdup( tmp ));
			iter++;
		}

		g_strfreev( tokens );
	}

	g_free( source );
	return( basenames );
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
	static const char *thisfn = "nact_iprofile_conditions_create_schemes_selection_list";
	g_debug( "%s: window=%p", thisfn, window );
	g_assert( NACT_IS_IPROFILE_CONDITIONS( window ));

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

static void
on_path_changed( GtkEntry *entry, gpointer user_data )
{
	g_assert( NACT_IS_WINDOW( user_data ));
	NactWindow *dialog = NACT_WINDOW( user_data );

	NAActionProfile *edited = NA_ACTION_PROFILE( v_get_edited_profile( dialog ));
	na_action_profile_set_path( edited, gtk_entry_get_text( entry ));

	v_field_modified( dialog );
}

static void
on_parameters_changed( GtkEntry *entry, gpointer user_data )
{
	g_assert( NACT_IS_WINDOW( user_data ));
	NactWindow *dialog = NACT_WINDOW( user_data );

	NAActionProfile *edited = NA_ACTION_PROFILE( v_get_edited_profile( dialog ));
	na_action_profile_set_parameters( edited, gtk_entry_get_text( entry ));

	v_field_modified( dialog );
}

static void
on_basenames_changed( GtkEntry *entry, gpointer user_data )
{
	g_assert( NACT_IS_WINDOW( user_data ));
	NactWindow *dialog = NACT_WINDOW( user_data );

	NAActionProfile *edited = NA_ACTION_PROFILE( v_get_edited_profile( dialog ));
	const gchar *text = gtk_entry_get_text( entry );
	GSList *basenames = text_to_basenames( text );
	na_action_profile_set_basenames( edited, basenames );
	na_utils_free_string_list( basenames );

	v_field_modified( dialog );
}

static void
on_scheme_selection_toggled( GtkCellRendererToggle *renderer, gchar *path, gpointer user_data )
{
	/*static const gchar *thisfn = "nact_iprofile_conditions_on_scheme_selection_changed";
	g_debug( "%s: renderer=%p, path=%s, user_data=%p", thisfn, renderer, path, user_data );*/
	g_assert( NACT_IS_WINDOW( user_data ));
	NactWindow *dialog = NACT_WINDOW( user_data );

	GtkTreeModel *model = get_schemes_tree_model( dialog );
	GtkTreePath *tree_path = gtk_tree_path_new_from_string( path );
	GtkTreeIter iter;
	gboolean state;
	gtk_tree_model_get_iter( model, &iter, tree_path );
	gtk_tree_model_get( model, &iter, SCHEMES_CHECKBOX_COLUMN, &state, -1 );
	gtk_list_store_set( GTK_LIST_STORE( model ), &iter, SCHEMES_CHECKBOX_COLUMN, !state, -1 );
	gtk_tree_path_free( tree_path );

	/* TODO set profile scheme selection */
	/*NAActionProfile *edited = NA_ACTION_PROFILE( v_get_edited_profile( dialog ));*/
	/*na_action_set_label( edited, gtk_entry_get_text( entry ));*/

	v_field_modified( dialog );
}

static void
on_scheme_keyword_edited( GtkCellRendererText *renderer, const gchar *path, const gchar *text, gpointer user_data )
{
	static const gchar *thisfn = "nact_iprofile_conditions_on_scheme_keyword_edited";
	g_debug( "%s: renderer=%p, path=%s, text=%s, user_data=%p", thisfn, renderer, path, text, user_data );

	g_assert( NACT_IS_WINDOW( user_data ));
	NactWindow *dialog = NACT_WINDOW( user_data );

	v_field_modified( dialog );
}

static void
on_scheme_desc_edited( GtkCellRendererText *renderer, const gchar *path, const gchar *text, gpointer user_data )
{
	static const gchar *thisfn = "nact_iprofile_conditions_on_scheme_desc_edited";
	g_debug( "%s: renderer=%p, path=%s, text=%s, user_data=%p", thisfn, renderer, path, text, user_data );

	g_assert( NACT_IS_WINDOW( user_data ));
	NactWindow *dialog = NACT_WINDOW( user_data );

	v_field_modified( dialog );
}

static void
on_scheme_list_selection_changed( GtkTreeSelection *selection, gpointer user_data )
{
	static const gchar *thisfn = "nact_iprofile_conditions_on_scheme_list_selection_changed";
	g_debug( "%s: selection=%p, user_data=%p", thisfn, selection, user_data );

	g_assert( NACT_IS_WINDOW( user_data ));
	NactWindow *dialog = NACT_WINDOW( user_data );

	v_field_modified( dialog );
}
