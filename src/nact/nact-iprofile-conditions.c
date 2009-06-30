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

static void          on_path_changed( GtkEntry *entry, gpointer user_data );
static void          on_path_browse( GtkButton *button, gpointer user_data );
static GtkWidget    *get_path_widget( NactWindow *window );
static void          on_parameters_changed( GtkEntry *entry, gpointer user_data );
static GtkWidget    *get_parameters_widget( NactWindow *window );
static void          update_example_label( NactWindow *window );
static gchar        *parse_parameters( NactWindow *window );
static void          on_legend_clicked( GtkButton *button, gpointer user_data );
static void          show_legend_dialog( NactWindow *window );
static void          hide_legend_dialog( NactWindow *window );
static GtkButton    *get_legend_button( NactWindow *window );
static GtkWindow    *get_legend_dialog( NactWindow *window );

static void          on_basenames_changed( GtkEntry *entry, gpointer user_data );
static GtkWidget    *get_basenames_widget( NactWindow *window );
static void          on_matchcase_toggled( GtkToggleButton *button, gpointer user_data );
static GtkButton    *get_matchcase_button( NactWindow *window );
static void          on_mimetypes_changed( GtkEntry *entry, gpointer user_data );
static GtkWidget    *get_mimetypes_widget( NactWindow *window );
static void          on_isfiledir_toggled( GtkToggleButton *button, gpointer user_data );
static void          get_isfiledir( NactWindow *window, gboolean *isfile, gboolean *isdir );
static void          set_isfiledir( NactWindow *window, gboolean isfile, gboolean isdir );
static GtkButton    *get_isfile_button( NactWindow *window );
static GtkButton    *get_isdir_button( NactWindow *window );
static GtkButton    *get_both_button( NactWindow *window );
static void          on_multiple_toggled( GtkToggleButton *button, gpointer user_data );
static GtkButton    *get_multiple_button( NactWindow *window );

static void          on_scheme_selection_toggled( GtkCellRendererToggle *renderer, gchar *path, gpointer user_data );
static void          on_scheme_keyword_edited( GtkCellRendererText *renderer, const gchar *path, const gchar *text, gpointer user_data );
static void          on_scheme_desc_edited( GtkCellRendererText *renderer, const gchar *path, const gchar *text, gpointer user_data );
static void          on_scheme_list_selection_changed( GtkTreeSelection *selection, gpointer user_data );
static void          on_add_scheme_clicked( GtkButton *button, gpointer user_data );
static void          on_remove_scheme_clicked( GtkButton *button, gpointer user_data );
static void          scheme_cell_edited( NactWindow *window, const gchar *path_string, const gchar *text, gint column, gboolean *state, gchar **old_text );
static GtkTreeView  *get_schemes_tree_view( NactWindow *window );
static GtkTreeModel *get_schemes_tree_model( NactWindow *window );
static void          create_schemes_selection_list( NactWindow *window );
static gboolean      get_action_schemes_list( GtkTreeModel* scheme_model, GtkTreePath *path, GtkTreeIter* iter, gpointer data );
static GSList       *get_schemes_default_list( NactWindow *window );
static gboolean      reset_schemes_list( GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data );
static void          set_action_schemes( gchar *scheme, GtkTreeModel *model );
static GtkButton    *get_remove_button( NactWindow *window );

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

	GtkWidget *path_widget = get_path_widget( dialog );
	nact_window_signal_connect( dialog, G_OBJECT( path_widget ), "changed", G_CALLBACK( on_path_changed ));
	gchar *path = na_action_profile_get_path( profile );
	gtk_entry_set_text( GTK_ENTRY( path_widget ), path );
	g_free( path );

	GtkWidget *button = base_window_get_widget( BASE_WINDOW( dialog ), "PathBrowseButton" );
	nact_window_signal_connect( dialog, G_OBJECT( button ), "clicked", G_CALLBACK( on_path_browse ));

	GtkWidget *parameters_widget = get_parameters_widget( dialog );
	nact_window_signal_connect( dialog, G_OBJECT( parameters_widget ), "changed", G_CALLBACK( on_parameters_changed ));
	gchar *parameters = na_action_profile_get_parameters( profile );
	gtk_entry_set_text( GTK_ENTRY( parameters_widget ), parameters );
	g_free( parameters );

	button = GTK_WIDGET( get_legend_button( dialog ));
	nact_window_signal_connect( dialog, G_OBJECT( button ), "clicked", G_CALLBACK( on_legend_clicked ));

	GtkWidget *basenames_widget = get_basenames_widget( dialog );
	nact_window_signal_connect( dialog, G_OBJECT( basenames_widget ), "changed", G_CALLBACK( on_basenames_changed ));
	GSList *basenames = na_action_profile_get_basenames( profile );
	gchar *basenames_text = na_utils_string_list_to_text( basenames );
	gtk_entry_set_text( GTK_ENTRY( basenames_widget ), basenames_text );
	g_free( basenames_text );
	na_utils_free_string_list( basenames );

	GtkButton *matchcase_button = get_matchcase_button( dialog );
	nact_window_signal_connect( dialog, G_OBJECT( matchcase_button ), "toggled", G_CALLBACK( on_matchcase_toggled ));
	gboolean matchcase = na_action_profile_get_matchcase( profile );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( matchcase_button ), matchcase );

	GtkWidget *mimetypes_widget = get_mimetypes_widget( dialog );
	nact_window_signal_connect( dialog, G_OBJECT( mimetypes_widget ), "changed", G_CALLBACK( on_mimetypes_changed ));
	GSList *mimetypes = na_action_profile_get_mimetypes( profile );
	gchar *mimetypes_text = na_utils_string_list_to_text( mimetypes );
	gtk_entry_set_text( GTK_ENTRY( mimetypes_widget ), mimetypes_text );
	g_free( mimetypes_text );
	na_utils_free_string_list( mimetypes );

	GtkButton *isfile_button = get_isfile_button( dialog );
	nact_window_signal_connect( dialog, G_OBJECT( isfile_button ), "toggled", G_CALLBACK( on_isfiledir_toggled ));
	GtkButton *isdir_button = get_isdir_button( dialog );
	nact_window_signal_connect( dialog, G_OBJECT( isdir_button ), "toggled", G_CALLBACK( on_isfiledir_toggled ));
	GtkButton *both_button = get_both_button( dialog );
	nact_window_signal_connect( dialog, G_OBJECT( both_button ), "toggled", G_CALLBACK( on_isfiledir_toggled ));
	gboolean isfile = na_action_profile_get_is_file( profile );
	gboolean isdir = na_action_profile_get_is_dir( profile );
	set_isfiledir( dialog, isfile, isdir );

	GtkButton *multiple_button = get_multiple_button( dialog );
	nact_window_signal_connect( dialog, G_OBJECT( multiple_button ), "toggled", G_CALLBACK( on_multiple_toggled ));
	gboolean multiple = na_action_profile_get_multiple( profile );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( multiple_button ), multiple );

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

	button = base_window_get_widget( BASE_WINDOW( dialog ), "AddSchemeButton" );
	nact_window_signal_connect( dialog, G_OBJECT( button ), "clicked", G_CALLBACK( on_add_scheme_clicked ));
	GtkButton *remove_button = get_remove_button( dialog );
	nact_window_signal_connect( dialog, G_OBJECT( remove_button ), "clicked", G_CALLBACK( on_remove_scheme_clicked ));

	nact_window_signal_connect( dialog, G_OBJECT( gtk_tree_view_get_selection( scheme_widget )), "changed", G_CALLBACK( on_scheme_list_selection_changed ));

	GtkTreeModel *scheme_model = get_schemes_tree_model( dialog );
	gtk_tree_model_foreach( scheme_model, ( GtkTreeModelForeachFunc ) reset_schemes_list, NULL );
	GSList *schemes = na_action_profile_get_schemes( profile );
	g_slist_foreach( schemes, ( GFunc ) set_action_schemes, scheme_model );
}

void
nact_iprofile_conditions_all_widgets_showed( NactWindow *dialog )
{
}

void
nact_iprofile_conditions_dispose( NactWindow *dialog )
{
	hide_legend_dialog( dialog );
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

static void
on_path_changed( GtkEntry *entry, gpointer user_data )
{
	g_assert( NACT_IS_WINDOW( user_data ));
	NactWindow *dialog = NACT_WINDOW( user_data );

	NAActionProfile *edited = NA_ACTION_PROFILE( v_get_edited_profile( dialog ));
	na_action_profile_set_path( edited, gtk_entry_get_text( entry ));

	update_example_label( dialog );
	v_field_modified( dialog );
}

/* TODO: keep trace of last browsed dir */
static void
on_path_browse( GtkButton *button, gpointer user_data )
{
	g_assert( NACT_IS_IPROFILE_CONDITIONS( user_data ));
	gboolean set_current_location = FALSE;

	GtkWidget *dialog = gtk_file_chooser_dialog_new(
			_( "Choosing a command" ),
			NULL,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			NULL
			);

	GtkWidget *path_widget = get_path_widget( NACT_WINDOW( user_data ));
	const gchar *path = gtk_entry_get_text( GTK_ENTRY( path_widget ));
	if( path && strlen( path )){
		set_current_location = gtk_file_chooser_set_filename( GTK_FILE_CHOOSER( dialog ), path );
	}

	if( gtk_dialog_run( GTK_DIALOG( dialog )) == GTK_RESPONSE_ACCEPT ){
		gchar *filename = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER( dialog ));
		gtk_entry_set_text( GTK_ENTRY( path_widget ), filename );
	    g_free (filename);
	  }

	gtk_widget_destroy( dialog );
}

static GtkWidget *
get_path_widget( NactWindow *window )
{
	return( base_window_get_widget( BASE_WINDOW( window ), "CommandPathEntry" ));
}

static void
on_parameters_changed( GtkEntry *entry, gpointer user_data )
{
	g_assert( NACT_IS_WINDOW( user_data ));
	NactWindow *dialog = NACT_WINDOW( user_data );

	NAActionProfile *edited = NA_ACTION_PROFILE( v_get_edited_profile( dialog ));
	na_action_profile_set_parameters( edited, gtk_entry_get_text( entry ));

	update_example_label( dialog );
	v_field_modified( dialog );
}

static GtkWidget *
get_parameters_widget( NactWindow *window )
{
	return( base_window_get_widget( BASE_WINDOW( window ), "CommandParamsEntry" ));
}

static void
update_example_label( NactWindow *window )
{
	/*static const char *thisfn = "nact_iprofile_conditions_update_example_label";*/

	static const gchar *original_label = N_( "<i><b><span size=\"small\">e.g., %s</span></b></i>" );

	GtkWidget *example_widget = base_window_get_widget( BASE_WINDOW( window ), "LabelExample" );

	gchar *parameters = parse_parameters( window );
	/*g_debug( "%s: parameters=%s", thisfn, parameters );*/

	/* convert special xml chars (&, <, >,...) to avoid warnings
	 * generated by Pango parser
	 */
	gchar *new_label = g_markup_printf_escaped( original_label, parameters );

	gtk_label_set_label( GTK_LABEL( example_widget ), new_label );
	g_free( new_label );
	g_free( parameters );
}

/*
 * Valid parameters :
 *
 * %d : base dir of the selected file(s)/folder(s)
 * %f : the name of the selected file/folder or the 1st one if many are selected
 * %h : hostname of the GVfs URI
 * %m : list of the basename of the selected files/directories separated by space.
 * %M : list of the selected files/directories with their complete path separated by space.
 * %s : scheme of the GVfs URI
 * %u : GVfs URI
 * %U : username of the GVfs URI
 * %% : a percent sign
 */
static gchar *
parse_parameters( NactWindow *window )
{
	GString* tmp_string = g_string_new( "" );

	/* i18n notes: example strings for the command preview */
	gchar* ex_path = _( "/path/to" );
	gchar* ex_files[] = { N_( "file1.txt" ), N_( "file2.txt" ), NULL };
	gchar* ex_dirs[] = { N_(" folder1" ), N_( "folder2" ), NULL };
	gchar* ex_mixed[] = { N_(" file1.txt" ), N_( "folder1" ), NULL };
	gchar* ex_scheme_default = "file";
	gchar* ex_host_default = _( "test.example.net" );
	gchar* ex_one_file = _( "file.txt" );
	gchar* ex_one_dir = _( "folder" );
	gchar* ex_one = NULL;
	gchar* ex_list = NULL;
	gchar* ex_path_list = NULL;
	gchar* ex_scheme;
	gchar* ex_host;

	const gchar* command = gtk_entry_get_text( GTK_ENTRY( get_path_widget( window )));
	const gchar* param_template = gtk_entry_get_text( GTK_ENTRY( get_parameters_widget( window )));

	gchar* iter = g_strdup( param_template );
	gchar* old_iter = iter;
	gchar* tmp;
	gchar* separator;
	gchar* start;
	GSList* scheme_list = NULL;

	g_string_append_printf( tmp_string, "%s ", command );

	gboolean is_file, is_dir;
	get_isfiledir( window, &is_file, &is_dir );

	gboolean accept_multiple = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "AcceptMultipleButton" )));

	GtkTreeModel* scheme_model = get_schemes_tree_model( window );
	gtk_tree_model_foreach( scheme_model, ( GtkTreeModelForeachFunc ) get_action_schemes_list, &scheme_list );

	separator = g_strdup_printf( " %s/", ex_path );
	start = g_strdup_printf( "%s/", ex_path );

	if( accept_multiple ){
		if( is_file && is_dir ){
			ex_one = ex_files[0];
			ex_list = na_utils_gstring_joinv( NULL, " ", ex_mixed );
			ex_path_list = na_utils_gstring_joinv( start, separator, ex_mixed );

		} else if( is_dir ){
			ex_one = ex_dirs[0];
			ex_list = na_utils_gstring_joinv( NULL, " ", ex_dirs );
			ex_path_list = na_utils_gstring_joinv( start, separator, ex_dirs );

		} else if( is_file ){
			ex_one = ex_files[0];
			ex_list = na_utils_gstring_joinv( NULL, " ", ex_files );
			ex_path_list = na_utils_gstring_joinv( start, separator, ex_files );
		}
	} else {
		if( is_dir && !is_file ){
			ex_one = ex_one_dir;

		} else {
			ex_one = ex_one_file;
		}
		ex_list = g_strdup( ex_one );
		ex_path_list = g_strjoin( "/", ex_path, ex_one, NULL );
	}

	g_free (start);
	g_free (separator);

	if( scheme_list != NULL ){
		ex_scheme = ( gchar * ) scheme_list->data;
		if( g_ascii_strcasecmp( ex_scheme, "file" ) == 0 ){
			if( g_slist_length( scheme_list ) > 1 ){
				ex_scheme = ( gchar * ) scheme_list->next->data;
				ex_host = ex_host_default;
			} else {
				ex_host = "";
			}
		} else {
			ex_host = ex_host_default;
		}
	} else {
		ex_scheme = ex_scheme_default;
		ex_host = "";
	}

	while(( iter = g_strstr_len( iter, strlen( iter ), "%" ))){
		tmp_string = g_string_append_len( tmp_string, old_iter, strlen( old_iter ) - strlen( iter ));
		switch( iter[1] ){

			case 'd': /* base dir of the selected file(s)/folder(s) */
				tmp_string = g_string_append( tmp_string, ex_path );
				break;

			case 'f': /* the basename of the selected file/folder or the 1st one if many are selected */
				tmp_string = g_string_append( tmp_string, ex_one );
				break;

			case 'h': /* hostname of the GVfs URI */
				tmp_string = g_string_append( tmp_string, ex_host );
				break;

			case 'm': /* list of the basename of the selected files/directories separated by space */
				tmp_string = g_string_append( tmp_string, ex_list );
				break;

			case 'M': /* list of the selected files/directories with their complete path separated by space. */
				tmp_string = g_string_append( tmp_string, ex_path_list );
				break;

			case 's': /* scheme of the GVfs URI */
				tmp_string = g_string_append( tmp_string, ex_scheme );
				break;

			case 'u': /* GVfs URI */
				tmp = g_strjoin( NULL, ex_scheme, "://", ex_path, "/", ex_one, NULL );
				tmp_string = g_string_append( tmp_string, tmp );
				g_free( tmp );
				break;

			case 'U': /* username of the GVfs URI */
				tmp_string = g_string_append( tmp_string, "root" );
				break;

			case '%': /* a percent sign */
				tmp_string = g_string_append_c( tmp_string, '%' );
				break;
		}
		iter+=2; /* skip the % sign and the character after. */
		old_iter = iter; /* store the new start of the string */
	}
	tmp_string = g_string_append_len( tmp_string, old_iter, strlen( old_iter ));

	g_free( ex_list );
	g_free( ex_path_list );
	g_free( iter );

	return( g_string_free( tmp_string, FALSE ));
}

static void
on_legend_clicked( GtkButton *button, gpointer user_data )
{
	g_assert( NACT_IS_IPROFILE_CONDITIONS( user_data ));

	if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button ))){
		show_legend_dialog( NACT_WINDOW( user_data ));

	} else {
		hide_legend_dialog( NACT_WINDOW( user_data ));
	}
}

/* TODO: get back the last position saved */
static void
show_legend_dialog( NactWindow *window )
{
	GtkWindow *legend_dialog = get_legend_dialog( window );
	gtk_window_set_deletable( legend_dialog, FALSE );

	GtkWindow *toplevel = base_window_get_toplevel_widget( BASE_WINDOW( window ));
	gtk_window_set_transient_for( GTK_WINDOW( legend_dialog ), toplevel );

	gtk_widget_show( GTK_WIDGET( legend_dialog ));
}

/* TODO: save the current position */
static void
hide_legend_dialog( NactWindow *window )
{
	GtkWindow *legend_dialog = get_legend_dialog( window );
	gtk_widget_hide( GTK_WIDGET( legend_dialog ));

	/* set the legend button state consistent for when the dialog is
	 * hidden by another mean (eg. close the edit profile dialog)
	 */
	GtkButton *legend_button = get_legend_button( window );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( legend_button ), FALSE );
}

static GtkButton *
get_legend_button( NactWindow *window )
{
	return( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "LegendButton" )));
}

static GtkWindow *
get_legend_dialog( NactWindow *window )
{
	return( GTK_WINDOW( base_window_get_widget( BASE_WINDOW( window ), "LegendDialog" )));
}

static void
on_basenames_changed( GtkEntry *entry, gpointer user_data )
{
	g_assert( NACT_IS_WINDOW( user_data ));
	NactWindow *dialog = NACT_WINDOW( user_data );

	NAActionProfile *edited = NA_ACTION_PROFILE( v_get_edited_profile( dialog ));
	const gchar *text = gtk_entry_get_text( entry );
	GSList *basenames = na_utils_text_to_string_list( text );
	na_action_profile_set_basenames( edited, basenames );
	na_utils_free_string_list( basenames );

	v_field_modified( dialog );
}

static GtkWidget *
get_basenames_widget( NactWindow *window )
{
	return( base_window_get_widget( BASE_WINDOW( window ), "PatternEntry" ));
}

static void
on_matchcase_toggled( GtkToggleButton *button, gpointer user_data )
{
	g_assert( NACT_IS_WINDOW( user_data ));
	NactWindow *dialog = NACT_WINDOW( user_data );

	NAActionProfile *edited = NA_ACTION_PROFILE( v_get_edited_profile( dialog ));
	gboolean matchcase = gtk_toggle_button_get_active( button );
	na_action_profile_set_matchcase( edited, matchcase );

	v_field_modified( dialog );
}

static GtkButton *
get_matchcase_button( NactWindow *window )
{
	return( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "MatchCaseButton" )));
}

static void
on_mimetypes_changed( GtkEntry *entry, gpointer user_data )
{
	g_assert( NACT_IS_WINDOW( user_data ));
	NactWindow *dialog = NACT_WINDOW( user_data );

	NAActionProfile *edited = NA_ACTION_PROFILE( v_get_edited_profile( dialog ));
	const gchar *text = gtk_entry_get_text( entry );
	GSList *mimetypes = na_utils_text_to_string_list( text );
	na_action_profile_set_mimetypes( edited, mimetypes );
	na_utils_free_string_list( mimetypes );

	v_field_modified( dialog );
}

static GtkWidget *
get_mimetypes_widget( NactWindow *window )
{
	return( base_window_get_widget( BASE_WINDOW( window ), "MimeTypeEntry" ));
}

/*
 * Note that this callback is triggered twice: first, for the
 * deactivated button, then a second time for the newly activated one.
 * I don't know what to do to be triggered only once..?
 */
static void
on_isfiledir_toggled( GtkToggleButton *button, gpointer user_data )
{
	/*static const gchar *thisfn = "nact_iprofile_conditions_on_isfiledir_toggled";*/

	g_assert( NACT_IS_WINDOW( user_data ));
	NactWindow *dialog = NACT_WINDOW( user_data );

	gboolean isfile, isdir;
	get_isfiledir( dialog, &isfile, &isdir );
	NAActionProfile *edited = NA_ACTION_PROFILE( v_get_edited_profile( dialog ));
	na_action_profile_set_isfiledir( edited, isfile, isdir );

	v_field_modified( dialog );
}

static void
get_isfiledir( NactWindow *window, gboolean *isfile, gboolean *isdir )
{
	g_assert( isfile );
	g_assert( isdir );

	gboolean both = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( get_both_button( window )));
	if( both ){
		*isfile = TRUE;
		*isdir = TRUE;
	} else {
		*isfile = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( get_isfile_button( window )));
		*isdir = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( get_isdir_button( window )));
	}
}

static void
set_isfiledir( NactWindow *window, gboolean isfile, gboolean isdir )
{
	if( isfile && isdir ){
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( get_both_button( window )), TRUE );

	} else if( isfile ){
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( get_isfile_button( window )), TRUE );

	} else if( isdir ){
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( get_isdir_button( window )), TRUE );
	}
}

static GtkButton *
get_isfile_button( NactWindow *window )
{
	return( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "OnlyFilesButton" )));
}

static GtkButton *
get_isdir_button( NactWindow *window )
{
	return( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "OnlyFoldersButton" )));
}

static GtkButton *
get_both_button( NactWindow *window )
{
	return( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "BothButton" )));
}

static void
on_multiple_toggled( GtkToggleButton *button, gpointer user_data )
{
	g_assert( NACT_IS_WINDOW( user_data ));
	NactWindow *dialog = NACT_WINDOW( user_data );

	NAActionProfile *edited = NA_ACTION_PROFILE( v_get_edited_profile( dialog ));
	gboolean multiple = gtk_toggle_button_get_active( button );
	na_action_profile_set_multiple( edited, multiple );

	v_field_modified( dialog );
}

static GtkButton *
get_multiple_button( NactWindow *window )
{
	return( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "AcceptMultipleButton" )));
}

static void
on_scheme_selection_toggled( GtkCellRendererToggle *renderer, gchar *path, gpointer user_data )
{
	/*static const gchar *thisfn = "nact_iprofile_conditions_on_scheme_selection_toggled";*/
	/*g_debug( "%s: renderer=%p, path=%s, user_data=%p", thisfn, renderer, path, user_data );*/
	g_assert( NACT_IS_WINDOW( user_data ));
	NactWindow *dialog = NACT_WINDOW( user_data );

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

	NAActionProfile *edited = NA_ACTION_PROFILE( v_get_edited_profile( dialog ));
	na_action_profile_set_scheme( edited, scheme, !state );

	g_free( scheme );

	v_field_modified( dialog );
}

static void
on_scheme_keyword_edited( GtkCellRendererText *renderer, const gchar *path, const gchar *text, gpointer user_data )
{
	/*static const gchar *thisfn = "nact_iprofile_conditions_on_scheme_keyword_edited";*/
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
	/*static const gchar *thisfn = "nact_iprofile_conditions_on_scheme_desc_edited";
	g_debug( "%s: renderer=%p, path=%s, text=%s, user_data=%p", thisfn, renderer, path, text, user_data );*/

	g_assert( NACT_IS_WINDOW( user_data ));
	NactWindow *dialog = NACT_WINDOW( user_data );

	scheme_cell_edited( dialog, path, text, SCHEMES_DESC_COLUMN, NULL, NULL );
}

static void
on_scheme_list_selection_changed( GtkTreeSelection *selection, gpointer user_data )
{
	/*static const gchar *thisfn = "nact_iprofile_conditions_on_scheme_list_selection_changed";
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

static gboolean
get_action_schemes_list( GtkTreeModel* scheme_model, GtkTreePath *path, GtkTreeIter* iter, gpointer data )
{
	static const char *thisfn = "nact_iprofile_conditions_get_action_schemes_list";

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

	return FALSE; /* Don't stop looping */
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
get_remove_button( NactWindow *window )
{
	return( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "RemoveSchemeButton" )));
}
