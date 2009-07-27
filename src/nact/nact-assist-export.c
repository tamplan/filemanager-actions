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
#include <gtk/gtk.h>
#include <string.h>

#include <common/na-action.h>
#include <common/na-utils.h>

#include "base-application.h"
#include "nact-main-window.h"
#include "nact-assist-export.h"
#include "nact-gconf-writer.h"
#include "nact-iactions-list.h"
#include "nact-iprefs.h"
#include "nact-import-export-format.h"

/* Export Assistant
 *
 * pos.  type     title
 * ---   -------  ------------------------------------
 *   0   Intro    Introduction
 *   1   Content  Selection of the actions
 *   2   Content  Selection of the target folder
 *   3   Content  Selection of the export format
 *   4   Confirm  Summary of the operations to be done
 *   5   Summary  Export is done: summary of the done operations
 */

enum {
	ASSIST_PAGE_INTRO = 0,
	ASSIST_PAGE_ACTIONS_SELECTION,
	ASSIST_PAGE_FOLDER_SELECTION,
	ASSIST_PAGE_FORMAT_SELECTION,
	ASSIST_PAGE_CONFIRM,
	ASSIST_PAGE_DONE
};

/* private class data
 */
struct NactAssistExportClassPrivate {
};

/* private instance data
 */
struct NactAssistExportPrivate {
	gboolean        dispose_has_run;
	NactMainWindow *main_window;
	gchar          *uri;
	GSList         *fnames;
	gint            errors;
	gchar          *reason;
	gint            format;
};

static GObjectClass *st_parent_class = NULL;

static GType             register_type( void );
static void              class_init( NactAssistExportClass *klass );
static void              iactions_list_iface_init( NactIActionsListInterface *iface );
static void              instance_init( GTypeInstance *instance, gpointer klass );
static void              instance_dispose( GObject *application );
static void              instance_finalize( GObject *application );

static NactAssistExport *assist_new( BaseApplication *application );

static gchar            *do_get_iprefs_window_id( NactWindow *window );
static gchar            *do_get_dialog_name( BaseWindow *dialog );
static GSList           *get_actions( NactWindow *window );
static void              on_initial_load_dialog( BaseWindow *dialog );
static void              on_runtime_init_dialog( BaseWindow *dialog );
static void              on_all_widgets_showed( BaseWindow *dialog );
static void              on_apply( NactAssistant *window, GtkAssistant *assistant );
static void              on_prepare( NactAssistant *window, GtkAssistant *assistant, GtkWidget *page );

static void              assist_initial_load_intro( NactAssistExport *window, GtkAssistant *assistant );
static void              assist_runtime_init_intro( NactAssistExport *window, GtkAssistant *assistant );

static void              assist_initial_load_actions_list( NactAssistExport *window, GtkAssistant *assistant );
static void              assist_runtime_init_actions_list( NactAssistExport *window, GtkAssistant *assistant );
static void              on_actions_list_selection_changed( GtkTreeSelection *selection, gpointer user_data );

static void              assist_initial_load_target_folder( NactAssistExport *window, GtkAssistant *assistant );
static void              assist_runtime_init_target_folder( NactAssistExport *window, GtkAssistant *assistant );
static GtkFileChooser   *get_folder_chooser( NactAssistExport *window );
static void              on_folder_selection_changed( GtkFileChooser *chooser, gpointer user_data );
static gboolean          is_writable_dir( const gchar *uri );

static void              assist_initial_load_format( NactAssistExport *window, GtkAssistant *assistant );
static void              assist_runtime_init_format( NactAssistExport *window, GtkAssistant *assistant );
static void              on_format_toggled( GtkToggleButton *button, gpointer user_data );
static GtkWidget        *get_gconfschemav1_button( NactWindow *window );
static GtkWidget        *get_gconfschemav2_button( NactWindow *window );
static GtkWidget        *get_gconfdump_button( NactWindow *window );

static void              assist_initial_load_confirm( NactAssistExport *window, GtkAssistant *assistant );
static void              assist_runtime_init_confirm( NactAssistExport *window, GtkAssistant *assistant );
static void              assist_prepare_confirm( NactAssistExport *window, GtkAssistant *assistant, GtkWidget *page );

static void              assist_initial_load_exportdone( NactAssistExport *window, GtkAssistant *assistant );
static void              assist_runtime_init_exportdone( NactAssistExport *window, GtkAssistant *assistant );
static void              assist_prepare_exportdone( NactAssistExport *window, GtkAssistant *assistant, GtkWidget *page );

static void              do_export( NactAssistExport *window );

#ifdef NACT_MAINTAINER_MODE
static void              dump( NactAssistExport *window );
#endif

GType
nact_assist_export_get_type( void )
{
	static GType window_type = 0;

	if( !window_type ){
		window_type = register_type();
	}

	return( window_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "nact_assist_export_register_type";
	g_debug( "%s", thisfn );

	static GTypeInfo info = {
		sizeof( NactAssistExportClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactAssistExport ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	GType type = g_type_register_static( NACT_ASSISTANT_TYPE, "NactAssistExport", &info, 0 );

	/* implement IActionsList interface
	 */
	static const GInterfaceInfo iactions_list_iface_info = {
		( GInterfaceInitFunc ) iactions_list_iface_init,
		NULL,
		NULL
	};

	g_type_add_interface_static( type, NACT_IACTIONS_LIST_TYPE, &iactions_list_iface_info );

	return( type );
}

static void
class_init( NactAssistExportClass *klass )
{
	static const gchar *thisfn = "nact_assist_export_class_init";
	g_debug( "%s: klass=%p", thisfn, klass );

	st_parent_class = g_type_class_peek_parent( klass );

	GObjectClass *object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactAssistExportClassPrivate, 1 );

	BaseWindowClass *base_class = BASE_WINDOW_CLASS( klass );
	base_class->get_toplevel_name = do_get_dialog_name;
	base_class->initial_load_toplevel = on_initial_load_dialog;
	base_class->runtime_init_toplevel = on_runtime_init_dialog;
	base_class->all_widgets_showed = on_all_widgets_showed;

	NactWindowClass *nact_class = NACT_WINDOW_CLASS( klass );
	nact_class->get_iprefs_window_id = do_get_iprefs_window_id;

	NactAssistantClass *assist_class = NACT_ASSISTANT_CLASS( klass );
	assist_class->on_assistant_apply = on_apply;
	assist_class->on_assistant_prepare = on_prepare;
}

static void
iactions_list_iface_init( NactIActionsListInterface *iface )
{
	static const gchar *thisfn = "nact_assist_export_iactions_list_iface_init";
	g_debug( "%s: iface=%p", thisfn, iface );

	iface->get_actions = get_actions;
	iface->on_selection_changed = on_actions_list_selection_changed;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_assist_export_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );

	g_assert( NACT_IS_ASSIST_EXPORT( instance ));
	NactAssistExport *self = NACT_ASSIST_EXPORT( instance );

	self->private = g_new0( NactAssistExportPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->fnames = NULL;
	self->private->errors = 0;
}

static void
instance_dispose( GObject *window )
{
	static const gchar *thisfn = "nact_assist_export_instance_dispose";
	g_debug( "%s: window=%p", thisfn, window );

	g_assert( NACT_IS_ASSIST_EXPORT( window ));
	NactAssistExport *self = NACT_ASSIST_EXPORT( window );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		G_OBJECT_CLASS( st_parent_class )->dispose( window );
	}
}

static void
instance_finalize( GObject *window )
{
	static const gchar *thisfn = "nact_assist_export_instance_finalize";
	g_debug( "%s: window=%p", thisfn, window );

	g_assert( NACT_IS_ASSIST_EXPORT( window ));
	NactAssistExport *self = ( NactAssistExport * ) window;

	g_free( self->private->uri );
	na_utils_free_string_list( self->private->fnames );
	g_free( self->private->reason );

	g_free( self->private );

	/* chain call to parent class */
	if( st_parent_class->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( window );
	}
}

static NactAssistExport *
assist_new( BaseApplication *application )
{
	return( g_object_new( NACT_ASSIST_EXPORT_TYPE, PROP_WINDOW_APPLICATION_STR, application, NULL ));
}

/**
 * Run the assistant.
 *
 * @main: the main window of the application.
 */
void
nact_assist_export_run( NactWindow *main_window )
{
	BaseApplication *appli = BASE_APPLICATION( base_window_get_application( BASE_WINDOW( main_window )));

	NactAssistExport *assist = assist_new( appli );

	assist->private->main_window = NACT_MAIN_WINDOW( main_window );

	base_window_run( BASE_WINDOW( assist ));
}

static gchar *
do_get_iprefs_window_id( NactWindow *window )
{
	return( g_strdup( "export-assistant" ));
}

static gchar *
do_get_dialog_name( BaseWindow *dialog )
{
	return( g_strdup( "ExportAssistant" ));
}

static GSList *
get_actions( NactWindow *window )
{
	return( nact_main_window_get_actions( NACT_ASSIST_EXPORT( window )->private->main_window ));
}

static void
on_initial_load_dialog( BaseWindow *dialog )
{
	static const gchar *thisfn = "nact_assist_export_on_initial_load_dialog";

	/* call parent class at the very beginning */
	if( BASE_WINDOW_CLASS( st_parent_class )->initial_load_toplevel ){
		BASE_WINDOW_CLASS( st_parent_class )->initial_load_toplevel( dialog );
	}

	g_debug( "%s: dialog=%p", thisfn, dialog );
	g_assert( NACT_IS_ASSIST_EXPORT( dialog ));
	NactAssistExport *window = NACT_ASSIST_EXPORT( dialog );

	GtkAssistant *assistant = GTK_ASSISTANT( base_window_get_toplevel_dialog( dialog ));

	assist_initial_load_intro( window, assistant );
	assist_initial_load_actions_list( window, assistant );
	assist_initial_load_target_folder( window, assistant );
	assist_initial_load_format( window, assistant );
	assist_initial_load_confirm( window, assistant );
	assist_initial_load_exportdone( window, assistant );
}

static void
on_runtime_init_dialog( BaseWindow *dialog )
{
	static const gchar *thisfn = "nact_assist_export_on_runtime_init_dialog";

	/* call parent class at the very beginning */
	if( BASE_WINDOW_CLASS( st_parent_class )->runtime_init_toplevel ){
		BASE_WINDOW_CLASS( st_parent_class )->runtime_init_toplevel( dialog );
	}

	g_debug( "%s: dialog=%p", thisfn, dialog );
	g_assert( NACT_IS_ASSIST_EXPORT( dialog ));
	NactAssistExport *window = NACT_ASSIST_EXPORT( dialog );

	GtkAssistant *assistant = GTK_ASSISTANT( base_window_get_toplevel_dialog( dialog ));

	assist_runtime_init_intro( window, assistant );
	assist_runtime_init_actions_list( window, assistant );
	assist_runtime_init_target_folder( window, assistant );
	assist_runtime_init_format( window, assistant );
	assist_runtime_init_confirm( window, assistant );
	assist_runtime_init_exportdone( window, assistant );
}

static void
on_all_widgets_showed( BaseWindow *dialog )
{
}

/*
 * As of 1.11, nact_gconf_writer doesn't return any error message.
 * An error is simply indicated by returning a null filename.
 * So we provide a general error message.
 *
 * apply signal is ran from the confirm page _after_ the prepare signal
 * of the summary page ; it is so almost useless to do anything here if
 * we want show the result on the summary...
 *
 * see http://bugzilla.gnome.org/show_bug.cgi?id=589745
 */
static void
on_apply( NactAssistant *window, GtkAssistant *assistant )
{
	static const gchar *thisfn = "nact_assist_export_on_apply";
	g_debug( "%s: window=%p, assistant=%p", thisfn, window, assistant );
}

static void
on_prepare( NactAssistant *window, GtkAssistant *assistant, GtkWidget *page )
{
	/*static const gchar *thisfn = "nact_assist_export_on_prepare";
	g_debug( "%s: window=%p, assistant=%p, page=%p", thisfn, window, assistant, page );*/

	GtkAssistantPageType type = gtk_assistant_get_page_type( assistant, page );

	switch( type ){
		case GTK_ASSISTANT_PAGE_CONFIRM:
			assist_prepare_confirm( NACT_ASSIST_EXPORT( window ), assistant, page );
			break;

		case GTK_ASSISTANT_PAGE_SUMMARY:
			assist_prepare_exportdone( NACT_ASSIST_EXPORT( window ), assistant, page );
			break;

		default:
			break;
	}
}

static void
assist_initial_load_intro( NactAssistExport *window, GtkAssistant *assistant )
{
}

static void
assist_runtime_init_intro( NactAssistExport *window, GtkAssistant *assistant )
{
	static const gchar *thisfn = "nact_assist_export_runtime_init_intro";
	g_debug( "%s: window=%p, assistant=%p", thisfn, window, assistant );

	GtkWidget *content = gtk_assistant_get_nth_page( assistant, ASSIST_PAGE_INTRO );
	gtk_assistant_set_page_complete( assistant, content, TRUE );
}

static void
assist_initial_load_actions_list( NactAssistExport *window, GtkAssistant *assistant )
{
	g_assert( NACT_IS_IACTIONS_LIST( window ));
	nact_iactions_list_initial_load( NACT_WINDOW( window ));
	nact_iactions_list_set_edition_mode( NACT_WINDOW( window ), FALSE );
	nact_iactions_list_set_multiple_selection( NACT_WINDOW( window ), TRUE );
	nact_iactions_list_set_send_selection_changed_on_fill_list( NACT_WINDOW( window ), FALSE );
}

static void
assist_runtime_init_actions_list( NactAssistExport *window, GtkAssistant *assistant )
{
	nact_iactions_list_runtime_init( NACT_WINDOW( window ));

	GtkWidget *content = gtk_assistant_get_nth_page( assistant, ASSIST_PAGE_ACTIONS_SELECTION );
	gtk_assistant_set_page_complete( assistant, content, FALSE );
}

static void
on_actions_list_selection_changed( GtkTreeSelection *selection, gpointer user_data )
{
	/*static const gchar *thisfn = "nact_assist_export_on_actions_list_selection_changed";
	g_debug( "%s: selection=%p, user_data=%p", thisfn, selection, user_data );*/

	g_assert( NACT_IS_ASSIST_EXPORT( user_data ));
	GtkAssistant *assistant = GTK_ASSISTANT( base_window_get_toplevel_dialog( BASE_WINDOW( user_data )));
	gint pos = gtk_assistant_get_current_page( assistant );
	if( pos == ASSIST_PAGE_ACTIONS_SELECTION ){

		gboolean enabled = ( gtk_tree_selection_count_selected_rows( selection ) > 0 );

		GtkWidget *content = gtk_assistant_get_nth_page( assistant, pos );
		gtk_assistant_set_page_complete( assistant, content, enabled );
		gtk_assistant_update_buttons_state( assistant );
	}
}

static void
assist_initial_load_target_folder( NactAssistExport *window, GtkAssistant *assistant )
{
	GtkFileChooser *chooser = get_folder_chooser( window );
	gtk_file_chooser_set_action( GTK_FILE_CHOOSER( chooser ), GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER );
	gtk_file_chooser_set_select_multiple( GTK_FILE_CHOOSER( chooser ), FALSE );
}

static void
assist_runtime_init_target_folder( NactAssistExport *window, GtkAssistant *assistant )
{
	GtkFileChooser *chooser = get_folder_chooser( window );
	gtk_file_chooser_unselect_all( chooser );

	gchar *uri = nact_iprefs_get_export_folder_uri( NACT_WINDOW( window ));
	if( uri && strlen( uri )){
		gtk_file_chooser_set_uri( GTK_FILE_CHOOSER( chooser ), uri );
	}
	g_free( uri );

	nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( chooser ), "selection-changed", G_CALLBACK( on_folder_selection_changed ));

	GtkWidget *content = gtk_assistant_get_nth_page( assistant, ASSIST_PAGE_FOLDER_SELECTION );
	gtk_assistant_set_page_complete( assistant, content, FALSE );
}

static GtkFileChooser *
get_folder_chooser( NactAssistExport *window )
{
	return( GTK_FILE_CHOOSER( base_window_get_widget( BASE_WINDOW( window ), "ExportFolderChooser" )));
}

/*
 * we check the selected uri for writability
 * this is always subject to become invalid before actually writing
 * but this is better than nothing, doesn't ?
 */
static void
on_folder_selection_changed( GtkFileChooser *chooser, gpointer user_data )
{
	static const gchar *thisfn = "nact_assist_export_on_folder_selection_changed";
	g_debug( "%s: chooser=%p, user_data=%p", thisfn, chooser, user_data );

	g_assert( NACT_IS_ASSIST_EXPORT( user_data ));
	GtkAssistant *assistant = GTK_ASSISTANT( base_window_get_toplevel_dialog( BASE_WINDOW( user_data )));
	gint pos = gtk_assistant_get_current_page( assistant );
	if( pos == ASSIST_PAGE_FOLDER_SELECTION ){

		gchar *uri = gtk_file_chooser_get_uri( chooser );
		g_debug( "%s: uri=%s", thisfn, uri );
		gboolean enabled = ( uri && strlen( uri ) && is_writable_dir( uri ));

		if( enabled ){
			NactAssistExport *assist = NACT_ASSIST_EXPORT( user_data );
			g_free( assist->private->uri );
			assist->private->uri = g_strdup( uri );
			nact_iprefs_save_export_folder_uri( NACT_WINDOW( user_data ), uri );
		}

		g_free( uri );

		GtkWidget *content = gtk_assistant_get_nth_page( assistant, pos );
		gtk_assistant_set_page_complete( assistant, content, enabled );
		gtk_assistant_update_buttons_state( assistant );
	}
}

static gboolean
is_writable_dir( const gchar *uri )
{
	static const gchar *thisfn = "nact_assist_export_is_writable_dir";

	if( !uri || !strlen( uri )){
		return( FALSE );
	}

	GFile *file = g_file_new_for_uri( uri );
	GError *error = NULL;
	GFileInfo *info = g_file_query_info( file,
			G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE "," G_FILE_ATTRIBUTE_STANDARD_TYPE,
			G_FILE_QUERY_INFO_NONE, NULL, &error );

	if( error ){
		g_warning( "%s: g_file_query_info error: %s", thisfn, error->message );
		g_error_free( error );
		g_object_unref( file );
		return( FALSE );
	}

	GFileType type = g_file_info_get_file_type( info );
	if( type != G_FILE_TYPE_DIRECTORY ){
		g_warning( "%s: %s is not a directory", thisfn, uri );
		g_object_unref( info );
		return( FALSE );
	}

	gboolean writable = g_file_info_get_attribute_boolean( info, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE );
	if( !writable ){
		g_warning( "%s: %s is not writable", thisfn, uri );
	}
	g_object_unref( info );

	return( writable );
}

static void
assist_initial_load_format( NactAssistExport *window, GtkAssistant *assistant )
{
}

static void
assist_runtime_init_format( NactAssistExport *window, GtkAssistant *assistant )
{
	GtkWidget *button = get_gconfschemav1_button( NACT_WINDOW( window ));
	nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( button ), "toggled", G_CALLBACK( on_format_toggled ));

	button = get_gconfschemav2_button( NACT_WINDOW( window ));
	nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( button ), "toggled", G_CALLBACK( on_format_toggled ));

	button = get_gconfdump_button( NACT_WINDOW( window ));
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( button ), TRUE );
	window->private->format = FORMAT_GCONFENTRY;
	nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( button ), "toggled", G_CALLBACK( on_format_toggled ));

	GtkWidget *content = gtk_assistant_get_nth_page( assistant, ASSIST_PAGE_FORMAT_SELECTION );
	gtk_assistant_set_page_complete( assistant, content, TRUE );
}

static void
on_format_toggled( GtkToggleButton *button, gpointer user_data )
{
	g_assert( NACT_IS_WINDOW( user_data ));
	NactWindow *window = NACT_WINDOW( user_data );

	if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( get_gconfschemav1_button( window )))){
		NACT_ASSIST_EXPORT( window )->private->format = FORMAT_GCONFSCHEMAFILE_V1;

	} else if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( get_gconfschemav2_button( window )))){
		NACT_ASSIST_EXPORT( window )->private->format = FORMAT_GCONFSCHEMAFILE_V2;

	} else if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( get_gconfdump_button( window )))){
		NACT_ASSIST_EXPORT( window )->private->format = FORMAT_GCONFENTRY;
	}
}

static GtkWidget *
get_gconfschemav1_button( NactWindow *window )
{
	return( base_window_get_widget( BASE_WINDOW( window ), "ExportSchemaV1Button" ));
}

static GtkWidget *
get_gconfschemav2_button( NactWindow *window )
{
	return( base_window_get_widget( BASE_WINDOW( window ), "ExportSchemaV2Button" ));
}

static GtkWidget *
get_gconfdump_button( NactWindow *window )
{
	return( base_window_get_widget( BASE_WINDOW( window ), "ExportGConfDumpButton" ));
}

static void
assist_initial_load_confirm( NactAssistExport *window, GtkAssistant *assistant )
{
}

static void
assist_runtime_init_confirm( NactAssistExport *window, GtkAssistant *assistant )
{
}

static void
assist_prepare_confirm( NactAssistExport *window, GtkAssistant *assistant, GtkWidget *page )
{
	static const gchar *thisfn = "nact_assist_export_prepare_confirm";
	g_debug( "%s: window=%p, assistant=%p, page=%p", thisfn, window, assistant, page );

#ifdef NACT_MAINTAINER_MODE
	dump( window );
#endif

	gchar *text = g_strdup( _( "<b>About to export selected actions :</b>\n\n" ));

	GSList *actions = nact_iactions_list_get_selected_actions( NACT_WINDOW( window ));
	GSList *ia;
	gchar *tmp;

	for( ia = actions ; ia ; ia = ia->next ){
		tmp = g_strdup_printf( "%s\t%s\n", text, na_action_get_label( NA_ACTION( ia->data )));
		g_free( text );
		text = tmp;
	}

	g_assert( window->private->uri && strlen( window->private->uri ));

	tmp = g_strdup_printf( _( "%s\n\n<b>Into the destination folder :</b>\n\n\t%s" ), text, window->private->uri );
	g_free( text );
	text = tmp;

	gchar *label1 = NULL;
	gchar *label2 = NULL;
	switch( window->private->format ){
		case FORMAT_GCONFSCHEMAFILE_V1:
			label1 = g_strdup( gtk_button_get_label( GTK_BUTTON( get_gconfschemav1_button( NACT_WINDOW( window )))));
			label2 = g_strdup( gtk_label_get_text( GTK_LABEL( base_window_get_widget( BASE_WINDOW( window ), "ExportSchemaV1Label"))));
			break;

		case FORMAT_GCONFSCHEMAFILE_V2:
			label1 = g_strdup( gtk_button_get_label( GTK_BUTTON( get_gconfschemav2_button( NACT_WINDOW( window )))));
			label2 = g_strdup( gtk_label_get_text( GTK_LABEL( base_window_get_widget( BASE_WINDOW( window ), "ExportSchemaV2Label"))));
			break;

		case FORMAT_GCONFENTRY:
			label1 = g_strdup( gtk_button_get_label( GTK_BUTTON( get_gconfdump_button( NACT_WINDOW( window )))));
			label2 = g_strdup( gtk_label_get_text( GTK_LABEL( base_window_get_widget( BASE_WINDOW( window ), "ExportGConfDumpLabel"))));
			break;

		default:
			break;
	}

	tmp = g_strdup_printf( _( "%s\n\n<b>%s</b>\n\n%s" ), text, label1, label2 );
	g_free( label2 );
	g_free( label1 );
	g_free( text );
	text = tmp;

	gtk_label_set_markup( GTK_LABEL( page ), text );
	g_free( text );

	gtk_assistant_set_page_complete( assistant, page, TRUE );
}

static void
assist_initial_load_exportdone( NactAssistExport *window, GtkAssistant *assistant )
{
}

static void
assist_runtime_init_exportdone( NactAssistExport *window, GtkAssistant *assistant )
{
}

static void
assist_prepare_exportdone( NactAssistExport *window, GtkAssistant *assistant, GtkWidget *page )
{
	static const gchar *thisfn = "nact_assist_export_prepare_exportdone";
	g_debug( "%s: window=%p, assistant=%p, page=%p", thisfn, window, assistant, page );

	do_export( window );

#ifdef NACT_MAINTAINER_MODE
	dump( window );
#endif

	gchar *text, *tmp;

	if( window->private->errors ){
		text = g_strdup_printf(
				_( "<b>One or more errors have been detected when exporting actions.</b>\n\n%s" ),
				window->private->reason );

	} else {
		text = g_strdup( _( "<b>Selected actions have been successfully exported...</b>\n\n" ));

		tmp = g_strdup_printf( _( "%s<b>... in folder :</b>\n\n\t%s/\n\n" ), text, window->private->uri );
		g_free( text );
		text = tmp;

		tmp = g_strdup_printf( _( "%s<b>... as files :</b>\n\n" ), text );
		g_free( text );
		text = tmp;

		GSList *ifn;
		for( ifn = window->private->fnames ; ifn ; ifn = ifn->next ){
			GFile *file = g_file_new_for_uri(( gchar * ) ifn->data );
			gchar *bname = g_file_get_basename( file );
			tmp = g_strdup_printf( "%s\t%s\n", text, bname );
			g_free( bname );
			g_object_unref( file );
			g_free( text );
			text = tmp;
		}
	}

	gtk_label_set_markup( GTK_LABEL( page ), text );
	g_free( text );

	gtk_assistant_set_page_complete( assistant, page, TRUE );
	nact_assistant_set_warn_on_cancel( NACT_ASSISTANT( window ), FALSE );
}

static void
do_export( NactAssistExport *window )
{
	static const gchar *thisfn = "nact_assist_export_do_export";
	g_debug( "%s: window=%p", thisfn, window );

	GSList *actions = nact_iactions_list_get_selected_actions( NACT_WINDOW( window ));
	GSList *ia;
	gchar *msg = NULL;
	gchar *reason = NULL;
	gchar *tmp;

	g_assert( window->private->uri && strlen( window->private->uri ));

	for( ia = actions ; ia ; ia = ia->next ){
		NAAction *action = NA_ACTION( ia->data );
		gchar *fname = nact_gconf_writer_export( action, window->private->uri, window->private->format, &msg );

		if( fname && strlen( fname )){
			window->private->fnames = g_slist_prepend( window->private->fnames, fname );
			g_debug( "%s: fname=%s", thisfn, fname );

		} else {
			window->private->errors += 1;
			if( msg ){
				if( reason ){
					tmp = g_strdup_printf( "%s\n", reason );
					g_free( reason );
					reason = tmp;
				}
				tmp = g_strdup_printf( "%s%s", reason, msg );
				g_free( reason );
				reason = tmp;
				g_free( msg );
			}
		}
	}

	if( window->private->errors ){
		if( !reason ){
			reason = g_strdup( _( "You may not have writing permissions on selected folder." ));
		}
		window->private->reason = reason;
	}

	g_slist_free( actions );
}

#ifdef NACT_MAINTAINER_MODE
static void
dump( NactAssistExport *window )
{
	static const gchar *thisfn = "nact_assist_export_dump";
	g_debug( "%s:          window=%p", thisfn, window );
	g_debug( "%s:         private=%p", thisfn, window->private );
	g_debug( "%s: dispose_has_run=%s", thisfn, window->private->dispose_has_run ? "True":"False" );
	g_debug( "%s:             uri=%s", thisfn, window->private->uri );
	g_debug( "%s:          errors=%d", thisfn, window->private->errors );
	na_utils_dump_string_list( window->private->fnames );
}
#endif
