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

#include "nact-application.h"
#include "nact-icommand-tab.h"
#include "nact-iprefs.h"

/* private interface data
 */
struct NactICommandTabInterfacePrivate {
};

/* the GConf key used to read/write size and position of auxiliary dialogs
 */
#define IPREFS_LEGEND_DIALOG				"iconditions-legend-dialog"
#define IPREFS_COMMAND_CHOOSER				"iconditions-command-chooser"

/* a data set in the LegendDialog GObject
 */
#define LEGEND_DIALOG_IS_VISIBLE			"iconditions-legend-dialog-visible"

/* ICommandTab properties, set on the main window
 */
#define PROP_ICOMMAND_TAB_STATUS_CONTEXT	"iconditions-status-context"

static GType            register_type( void );
static void             interface_base_init( NactICommandTabInterface *klass );
static void             interface_base_finalize( NactICommandTabInterface *klass );

static GtkWidget       *v_get_status_bar( NactWindow *window );
static NAActionProfile *v_get_edited_profile( NactWindow *window );
static void             v_field_modified( NactWindow *window );
static void             v_get_isfiledir( NactWindow *window, gboolean *isfile, gboolean *isdir );
static gboolean         v_get_multiple( NactWindow *window );
static GSList          *v_get_schemes( NactWindow *window );

static void             on_label_changed( GtkEntry *entry, gpointer user_data );
static void             check_for_label( NactWindow *window, GtkEntry *entry, const gchar *label );
static void             set_label_label( NactWindow *window, const gchar *color );
static GtkWidget       *get_label_entry( NactWindow *window );
static void             on_path_changed( GtkEntry *entry, gpointer user_data );
static void             on_path_browse( GtkButton *button, gpointer user_data );
static GtkWidget       *get_path_entry( NactWindow *window );
static GtkButton       *get_path_button( NactWindow *window );
static void             on_parameters_changed( GtkEntry *entry, gpointer user_data );
static GtkWidget       *get_parameters_entry( NactWindow *window );
static void             update_example_label( NactWindow *window );
static gchar           *parse_parameters( NactWindow *window );
static void             on_legend_clicked( GtkButton *button, gpointer user_data );
static void             show_legend_dialog( NactWindow *window );
static void             hide_legend_dialog( NactWindow *window );
static GtkButton       *get_legend_button( NactWindow *window );
static GtkWindow       *get_legend_dialog( NactWindow *window );

static void             display_status( NactWindow *window, const gchar *status );
static void             hide_status( NactWindow *window );
static guint            get_status_context( NactWindow *window );
static void             set_status_context( NactWindow *window, guint context );

GType
nact_icommand_tab_get_type( void )
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
	static const gchar *thisfn = "nact_icommand_tab_register_type";
	g_debug( "%s", thisfn );

	static const GTypeInfo info = {
		sizeof( NactICommandTabInterface ),
		( GBaseInitFunc ) interface_base_init,
		( GBaseFinalizeFunc ) interface_base_finalize,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL
	};

	GType type = g_type_register_static( G_TYPE_INTERFACE, "NactICommandTab", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( NactICommandTabInterface *klass )
{
	static const gchar *thisfn = "nact_icommand_tab_interface_base_init";
	static gboolean initialized = FALSE;

	if( !initialized ){
		g_debug( "%s: klass=%p", thisfn, klass );

		klass->private = g_new0( NactICommandTabInterfacePrivate, 1 );

		klass->get_edited_profile = NULL;
		klass->field_modified = NULL;

		initialized = TRUE;
	}
}

static void
interface_base_finalize( NactICommandTabInterface *klass )
{
	static const gchar *thisfn = "nact_icommand_tab_interface_base_finalize";
	static gboolean finalized = FALSE ;

	if( !finalized ){
		g_debug( "%s: klass=%p", thisfn, klass );

		g_free( klass->private );

		finalized = TRUE;
	}
}

void
nact_icommand_tab_initial_load( NactWindow *dialog )
{
	static const gchar *thisfn = "nact_icommand_tab_initial_load";
	g_debug( "%s: dialog=%p", thisfn, dialog );

	GtkWidget *status_bar = v_get_status_bar( dialog );
	if( status_bar ){
		g_assert( GTK_IS_STATUSBAR( status_bar ));
		guint context = gtk_statusbar_get_context_id( GTK_STATUSBAR( status_bar ), "nact-iaction-tab" );
		set_status_context( dialog, context );
	}
}

void
nact_icommand_tab_runtime_init( NactWindow *dialog )
{
	static const gchar *thisfn = "nact_icommand_tab_runtime_init";
	g_debug( "%s: dialog=%p", thisfn, dialog );

	GtkWidget *label_entry = get_label_entry( dialog );
	nact_window_signal_connect( dialog, G_OBJECT( label_entry ), "changed", G_CALLBACK( on_label_changed ));

	GtkWidget *path_entry = get_path_entry( dialog );
	nact_window_signal_connect( dialog, G_OBJECT( path_entry ), "changed", G_CALLBACK( on_path_changed ));

	GtkButton *path_button = get_path_button( dialog );
	nact_window_signal_connect( dialog, G_OBJECT( path_button ), "clicked", G_CALLBACK( on_path_browse ));

	GtkWidget *parameters_entry = get_parameters_entry( dialog );
	nact_window_signal_connect( dialog, G_OBJECT( parameters_entry ), "changed", G_CALLBACK( on_parameters_changed ));

	GtkButton *legend_button = get_legend_button( dialog );
	nact_window_signal_connect( dialog, G_OBJECT( legend_button ), "clicked", G_CALLBACK( on_legend_clicked ));
}

/**
 * A good place to set focus to the first visible field.
 */
void
nact_icommand_tab_all_widgets_showed( NactWindow *dialog )
{
	static const gchar *thisfn = "nact_icommand_tab_all_widgets_showed";
	g_debug( "%s: dialog=%p", thisfn, dialog );
}

void
nact_icommand_tab_dispose( NactWindow *dialog )
{
	static const gchar *thisfn = "nact_icommand_tab_dispose";
	g_debug( "%s: dialog=%p", thisfn, dialog );

	hide_legend_dialog( dialog );
}

void
nact_icommand_tab_set_profile( NactWindow *dialog, const NAActionProfile *profile )
{
	static const gchar *thisfn = "nact_icommand_tab_set_profile";
	g_debug( "%s: dialog=%p, profile=%p", thisfn, dialog, profile );

	GtkWidget *label_entry = get_label_entry( dialog );
	gchar *label = profile ? na_action_profile_get_label( profile ) : g_strdup( "" );
	gtk_entry_set_text( GTK_ENTRY( label_entry ), label );
	gtk_widget_set_sensitive( label_entry, profile != NULL );
	check_for_label( dialog, GTK_ENTRY( label_entry ), label );
	g_free( label );

	GtkWidget *path_entry = get_path_entry( dialog );
	gchar *path = profile ? na_action_profile_get_path( profile ) : g_strdup( "" );
	gtk_entry_set_text( GTK_ENTRY( path_entry ), path );
	gtk_widget_set_sensitive( path_entry, profile != NULL );
	g_free( path );

	GtkWidget *parameters_entry = get_parameters_entry( dialog );
	gchar *parameters = profile ? na_action_profile_get_parameters( profile ) : g_strdup( "" );
	gtk_entry_set_text( GTK_ENTRY( parameters_entry ), parameters );
	gtk_widget_set_sensitive( parameters_entry, profile != NULL );
	g_free( parameters );

	GtkButton *path_button = get_path_button( dialog );
	gtk_widget_set_sensitive( GTK_WIDGET( path_button ), profile != NULL );

	GtkButton *legend_button = get_legend_button( dialog );
	gtk_widget_set_sensitive( GTK_WIDGET( legend_button ), profile != NULL );
}

/**
 * A profile can only be saved if it has at least a label.
 * Returns TRUE if the label of the profile is not empty.
 */
gboolean
nact_icommand_tab_has_label( NactWindow *window )
{
	GtkWidget *label_entry = get_label_entry( window );
	const gchar *label = gtk_entry_get_text( GTK_ENTRY( label_entry ));
	return( g_utf8_strlen( label, -1 ) > 0 );
}

static GtkWidget *
v_get_status_bar( NactWindow *window )
{
	if( NACT_ICOMMAND_TAB_GET_INTERFACE( window )->get_status_bar ){
		return( NACT_ICOMMAND_TAB_GET_INTERFACE( window )->get_status_bar( window ));
	}

	return( NULL );
}

static NAActionProfile *
v_get_edited_profile( NactWindow *window )
{
	g_assert( NACT_IS_ICOMMAND_TAB( window ));

	if( NACT_ICOMMAND_TAB_GET_INTERFACE( window )->get_edited_profile ){
		return( NACT_ICOMMAND_TAB_GET_INTERFACE( window )->get_edited_profile( window ));
	}

	return( NULL );
}

static void
v_field_modified( NactWindow *window )
{
	g_assert( NACT_IS_ICOMMAND_TAB( window ));

	if( NACT_ICOMMAND_TAB_GET_INTERFACE( window )->field_modified ){
		NACT_ICOMMAND_TAB_GET_INTERFACE( window )->field_modified( window );
	}
}

static void
v_get_isfiledir( NactWindow *window, gboolean *isfile, gboolean *isdir )
{
	g_assert( NACT_IS_ICOMMAND_TAB( window ));
	g_assert( isfile );
	g_assert( isdir );
	*isfile = FALSE;
	*isdir = FALSE;

	if( NACT_ICOMMAND_TAB_GET_INTERFACE( window )->get_isfiledir ){
		NACT_ICOMMAND_TAB_GET_INTERFACE( window )->get_isfiledir( window, isfile, isdir );
	}
}

static gboolean
v_get_multiple( NactWindow *window )
{
	g_assert( NACT_IS_ICOMMAND_TAB( window ));

	if( NACT_ICOMMAND_TAB_GET_INTERFACE( window )->get_multiple ){
		return( NACT_ICOMMAND_TAB_GET_INTERFACE( window )->get_multiple( window ));
	}

	return( FALSE );
}

static GSList *
v_get_schemes( NactWindow *window )
{
	g_assert( NACT_IS_ICOMMAND_TAB( window ));

	if( NACT_ICOMMAND_TAB_GET_INTERFACE( window )->get_schemes ){
		return( NACT_ICOMMAND_TAB_GET_INTERFACE( window )->get_schemes( window ));
	}

	return( NULL );
}

static void
on_label_changed( GtkEntry *entry, gpointer user_data )
{
	g_assert( NACT_IS_WINDOW( user_data ));
	NactWindow *dialog = NACT_WINDOW( user_data );

	NAActionProfile *edited = v_get_edited_profile( dialog );

	if( edited ){
		const gchar *label = gtk_entry_get_text( entry );
		na_action_profile_set_label( edited, label );
		v_field_modified( dialog );
		check_for_label( dialog, entry, label );
	}
}

static void
check_for_label( NactWindow *window, GtkEntry *entry, const gchar *label )
{
	hide_status( window );
	set_label_label( window, "black" );

	NAActionProfile *edited = v_get_edited_profile( window );

	if( edited && g_utf8_strlen( label, -1 ) == 0 ){
		/* i18n: status bar message when the profile label is empty */
		display_status( window, _( "Caution: a label is mandatory for the profile." ));
		set_label_label( window, "red" );
	}
}

static void
set_label_label( NactWindow *window, const gchar *color )
{
	GtkWidget *label = base_window_get_widget( BASE_WINDOW( window ), "ProfileLabelLabel" );
	/* i18n: label in front of the GtkEntry where user enters the profile label */
	gchar *text = g_markup_printf_escaped( "<span color=\"%s\">%s</span>", color, _( "_Label :" ));
	gtk_label_set_markup_with_mnemonic( GTK_LABEL( label ), text );
}

static GtkWidget *
get_label_entry( NactWindow *window )
{
	return( base_window_get_widget( BASE_WINDOW( window ), "ProfileLabelEntry" ));
}

static void
on_path_changed( GtkEntry *entry, gpointer user_data )
{
	g_assert( NACT_IS_WINDOW( user_data ));
	NactWindow *dialog = NACT_WINDOW( user_data );

	NAActionProfile *edited = NA_ACTION_PROFILE( v_get_edited_profile( dialog ));
	if( edited ){
		na_action_profile_set_path( edited, gtk_entry_get_text( entry ));
		v_field_modified( dialog );
	}

	update_example_label( dialog );
}

static void
on_path_browse( GtkButton *button, gpointer user_data )
{
	g_assert( NACT_IS_ICOMMAND_TAB( user_data ));
	gboolean set_current_location = FALSE;
	gchar *uri = NULL;

	GtkWidget *dialog = gtk_file_chooser_dialog_new(
			_( "Choosing a command" ),
			NULL,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			NULL
			);

	nact_iprefs_position_named_window( NACT_WINDOW( user_data ), GTK_WINDOW( dialog ), IPREFS_COMMAND_CHOOSER );

	GtkWidget *path_entry = get_path_entry( NACT_WINDOW( user_data ));
	const gchar *path = gtk_entry_get_text( GTK_ENTRY( path_entry ));

	if( path && strlen( path )){
		set_current_location = gtk_file_chooser_set_filename( GTK_FILE_CHOOSER( dialog ), path );

	} else {
		uri = nact_iprefs_get_iconditions_folder_uri( NACT_WINDOW( user_data ));
		gtk_file_chooser_set_current_folder_uri( GTK_FILE_CHOOSER( dialog ), uri );
		g_free( uri );
	}

	if( gtk_dialog_run( GTK_DIALOG( dialog )) == GTK_RESPONSE_ACCEPT ){
		gchar *filename = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER( dialog ));
		gtk_entry_set_text( GTK_ENTRY( path_entry ), filename );
	    g_free (filename);
	  }

	uri = gtk_file_chooser_get_current_folder_uri( GTK_FILE_CHOOSER( dialog ));
	nact_iprefs_save_iconditions_folder_uri( NACT_WINDOW( user_data ), uri );
	g_free( uri );

	nact_iprefs_save_named_window_position( NACT_WINDOW( user_data ), GTK_WINDOW( dialog ), IPREFS_COMMAND_CHOOSER );

	gtk_widget_destroy( dialog );
}

static GtkWidget *
get_path_entry( NactWindow *window )
{
	return( base_window_get_widget( BASE_WINDOW( window ), "CommandPathEntry" ));
}

static GtkButton *
get_path_button( NactWindow *window )
{
	return( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "CommandPathButton" )));
}

static void
on_parameters_changed( GtkEntry *entry, gpointer user_data )
{
	g_assert( NACT_IS_WINDOW( user_data ));
	NactWindow *dialog = NACT_WINDOW( user_data );

	NAActionProfile *edited = NA_ACTION_PROFILE( v_get_edited_profile( dialog ));
	if( edited ){
		na_action_profile_set_parameters( edited, gtk_entry_get_text( entry ));
		v_field_modified( dialog );
	}

	update_example_label( dialog );
}

static GtkWidget *
get_parameters_entry( NactWindow *window )
{
	return( base_window_get_widget( BASE_WINDOW( window ), "CommandParametersEntry" ));
}

static void
update_example_label( NactWindow *window )
{
	/*static const char *thisfn = "nact_iconditions_update_example_label";*/

	static const gchar *original_label = N_( "<i><b><span size=\"small\">e.g., %s</span></b></i>" );

	GtkWidget *example_widget = base_window_get_widget( BASE_WINDOW( window ), "CommandExampleLabel" );
	gchar *newlabel;

	if( v_get_edited_profile( window )){

		gchar *parameters = parse_parameters( window );
		/*g_debug( "%s: parameters=%s", thisfn, parameters );*/

		/* convert special xml chars (&, <, >,...) to avoid warnings
		 * generated by Pango parser
		 */
		newlabel = g_markup_printf_escaped( original_label, parameters );

		g_free( parameters );

	} else {
		newlabel = g_strdup( "" );
	}

	gtk_label_set_label( GTK_LABEL( example_widget ), newlabel );
	g_free( newlabel );
}

/*
 * Valid parameters :
 *
 * %d : base dir of the (first) selected file(s)/folder(s)
 * %f : the name of the (firstà selected file/folder
 * %h : hostname of the (first) URI
 * %m : list of the basename of the selected files/directories separated by space.
 * %M : list of the selected files/directories with their complete path separated by space.
 * %p : port number of the (first) URI
 * %R : space-separated list of selected URIs
 * %s : scheme of the (first) URI
 * %u : (first) URI
 * %U : username of the (first) URI
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
	gchar* ex_port_default = _( "8080" );
	gchar* ex_one = NULL;
	gchar* ex_list = NULL;
	gchar* ex_path_list = NULL;
	gchar* ex_uri_file1 = _( "file:///path/to/file1.text" );
	gchar* ex_uri_file2 = _( "file:///path/to/file2.text" );
	gchar* ex_uri_folder1 = _( "file:///path/to/a/dir" );
	gchar* ex_uri_folder2 = _( "file:///path/to/another/dir" );
	gchar* ex_uri_list = NULL;
	gchar* ex_scheme;
	gchar* ex_host;

	const gchar* command = gtk_entry_get_text( GTK_ENTRY( get_path_entry( window )));
	const gchar* param_template = gtk_entry_get_text( GTK_ENTRY( get_parameters_entry( window )));

	gchar* iter = g_strdup( param_template );
	gchar* old_iter = iter;
	gchar* tmp;
	gchar* separator;
	gchar* start;

	g_string_append_printf( tmp_string, "%s ", command );

	gboolean is_file, is_dir;
	v_get_isfiledir( window, &is_file, &is_dir );

	gboolean accept_multiple = v_get_multiple( window );
	GSList *scheme_list = v_get_schemes( window );

	separator = g_strdup_printf( " %s/", ex_path );
	start = g_strdup_printf( "%s/", ex_path );

	if( accept_multiple ){
		if( is_file && is_dir ){
			ex_one = ex_files[0];
			ex_list = na_utils_gstring_joinv( NULL, " ", ex_mixed );
			ex_path_list = na_utils_gstring_joinv( start, separator, ex_mixed );
			ex_uri_list = g_strjoin( " ", ex_uri_file1, ex_uri_folder1, NULL );

		} else if( is_dir ){
			ex_one = ex_dirs[0];
			ex_list = na_utils_gstring_joinv( NULL, " ", ex_dirs );
			ex_path_list = na_utils_gstring_joinv( start, separator, ex_dirs );
			ex_uri_list = g_strjoin( " ", ex_uri_folder1, ex_uri_folder2, NULL );

		} else if( is_file ){
			ex_one = ex_files[0];
			ex_list = na_utils_gstring_joinv( NULL, " ", ex_files );
			ex_path_list = na_utils_gstring_joinv( start, separator, ex_files );
			ex_uri_list = g_strjoin( " ", ex_uri_file1, ex_uri_file2, NULL );
		}
	} else {
		if( is_dir && !is_file ){
			ex_one = ex_one_dir;
			ex_uri_list = g_strdup( ex_uri_folder1 );

		} else {
			ex_one = ex_one_file;
			ex_uri_list = g_strdup( ex_uri_file1 );
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

			case 'd': /* base dir of the (first) selected file(s)/folder(s) */
				tmp_string = g_string_append( tmp_string, ex_path );
				break;

			case 'f': /* the basename of the (first) selected file/folder */
				tmp_string = g_string_append( tmp_string, ex_one );
				break;

			case 'h': /* hostname of the (first) URI */
				tmp_string = g_string_append( tmp_string, ex_host );
				break;

			case 'm': /* list of the basename of the selected files/directories separated by space */
				tmp_string = g_string_append( tmp_string, ex_list );
				break;

			case 'M': /* list of the selected files/directories with their complete path separated by space. */
				tmp_string = g_string_append( tmp_string, ex_path_list );
				break;

			case 'p': /* port number of the (first) URI */
				tmp_string = g_string_append( tmp_string, ex_port_default );
				break;

			case 'R': /* space-separated list of selected URIs */
				tmp_string = g_string_append( tmp_string, ex_uri_list );
				break;

			case 's': /* scheme of the (first) URI */
				tmp_string = g_string_append( tmp_string, ex_scheme );
				break;

			case 'u': /* (first) URI */
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

	na_utils_free_string_list( scheme_list );

	g_free( ex_list );
	g_free( ex_path_list );
	g_free( ex_uri_list );
	g_free( iter );

	return( g_string_free( tmp_string, FALSE ));
}

static void
on_legend_clicked( GtkButton *button, gpointer user_data )
{
	g_assert( NACT_IS_ICOMMAND_TAB( user_data ));

	if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button ))){
		show_legend_dialog( NACT_WINDOW( user_data ));

	} else {
		hide_legend_dialog( NACT_WINDOW( user_data ));
	}
}

static void
show_legend_dialog( NactWindow *window )
{
	GtkWindow *legend_dialog = get_legend_dialog( window );
	gtk_window_set_deletable( legend_dialog, FALSE );

	GtkWindow *toplevel = base_window_get_toplevel_dialog( BASE_WINDOW( window ));
	gtk_window_set_transient_for( GTK_WINDOW( legend_dialog ), toplevel );

	nact_iprefs_position_named_window( window, legend_dialog, IPREFS_LEGEND_DIALOG );
	gtk_widget_show( GTK_WIDGET( legend_dialog ));

	g_object_set_data( G_OBJECT( legend_dialog ), LEGEND_DIALOG_IS_VISIBLE, GINT_TO_POINTER( TRUE ));
}

static void
hide_legend_dialog( NactWindow *window )
{
	GtkWindow *legend_dialog = get_legend_dialog( window );
	gboolean is_visible = GPOINTER_TO_INT( g_object_get_data( G_OBJECT( legend_dialog ), LEGEND_DIALOG_IS_VISIBLE ));

	if( is_visible ){
		g_assert( GTK_IS_WINDOW( legend_dialog ));
		nact_iprefs_save_named_window_position( window, legend_dialog, IPREFS_LEGEND_DIALOG );
		gtk_widget_hide( GTK_WIDGET( legend_dialog ));

		/* set the legend button state consistent for when the dialog is
		 * hidden by another mean (eg. close the edit profile dialog)
		 */
		GtkButton *legend_button = get_legend_button( window );
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( legend_button ), FALSE );

		g_object_set_data( G_OBJECT( legend_dialog ), LEGEND_DIALOG_IS_VISIBLE, GINT_TO_POINTER( FALSE ));
	}
}

static GtkButton *
get_legend_button( NactWindow *window )
{
	return( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "CommandLegendButton" )));
}

static GtkWindow *
get_legend_dialog( NactWindow *window )
{
	return( base_window_get_dialog( BASE_WINDOW( window ), "LegendDialog" ));
}

static void
display_status( NactWindow *window, const gchar *status )
{
	GtkWidget *bar = v_get_status_bar( window );
	guint context = get_status_context( window );
	gtk_statusbar_push( GTK_STATUSBAR( bar ), context, status );
}

static void
hide_status( NactWindow *window )
{
	GtkWidget *bar = v_get_status_bar( window );
	guint context = get_status_context( window );
	gtk_statusbar_pop( GTK_STATUSBAR( bar ), context );
}

static guint
get_status_context( NactWindow *window )
{
	return( GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( window ), PROP_ICOMMAND_TAB_STATUS_CONTEXT )));
}

static void
set_status_context( NactWindow *window, guint context )
{
	g_object_set_data( G_OBJECT( window ), PROP_ICOMMAND_TAB_STATUS_CONTEXT, GUINT_TO_POINTER( context ));
}
