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
#include <gtk/gtk.h>
#include <string.h>

#include <api/na-object-api.h>

#include <private/na-iduplicable.h>

#include <runtime/na-iprefs.h>
#include <runtime/na-pivot.h>
#include <runtime/na-utils.h>
#include <runtime/na-xml-names.h>
#include <runtime/na-xml-writer.h>

#include "base-iprefs.h"
#include "nact-application.h"
#include "nact-main-window.h"
#include "nact-assistant-export.h"
#include "nact-assistant-export-ask.h"
#include "nact-iactions-list.h"

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
struct NactAssistantExportClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NactAssistantExportPrivate {
	gboolean  dispose_has_run;
	gchar    *uri;
	GList    *results;
};

typedef struct {
	NAObjectAction *action;
	GSList         *msg;
	gchar          *fname;
	gint            format;
}
	ExportStruct;

#define IPREFS_EXPORT_ACTIONS_FOLDER_URI		"export-folder-uri"

static BaseAssistantClass *st_parent_class = NULL;

static GType           register_type( void );
static void            class_init( NactAssistantExportClass *klass );
static void            iactions_list_iface_init( NactIActionsListInterface *iface );
static void            instance_init( GTypeInstance *instance, gpointer klass );
static void            instance_dispose( GObject *application );
static void            instance_finalize( GObject *application );

static NactAssistantExport *assist_new( BaseWindow *parent );

static gchar          *window_get_iprefs_window_id( BaseWindow *window );
static gchar          *window_get_toplevel_name( BaseWindow *dialog );
static gchar          *window_get_ui_filename( BaseWindow *dialog );

static void            on_initial_load_dialog( NactAssistantExport *dialog, gpointer user_data );
static void            on_runtime_init_dialog( NactAssistantExport *dialog, gpointer user_data );

static void            assist_initial_load_intro( NactAssistantExport *window, GtkAssistant *assistant );
static void            assist_runtime_init_intro( NactAssistantExport *window, GtkAssistant *assistant );

static void            assist_initial_load_actions_list( NactAssistantExport *window, GtkAssistant *assistant );
static void            assist_runtime_init_actions_list( NactAssistantExport *window, GtkAssistant *assistant );
static void            on_iactions_list_selection_changed( NactIActionsList *instance, GSList *selected_items );
static gchar          *on_iactions_list_get_treeview_name( NactIActionsList *instance );

static void            assist_initial_load_target_folder( NactAssistantExport *window, GtkAssistant *assistant );
static void            assist_runtime_init_target_folder( NactAssistantExport *window, GtkAssistant *assistant );
static GtkFileChooser *get_folder_chooser( NactAssistantExport *window );
static void            on_folder_selection_changed( GtkFileChooser *chooser, gpointer user_data );

static void            assist_initial_load_format( NactAssistantExport *window, GtkAssistant *assistant );
static void            assist_runtime_init_format( NactAssistantExport *window, GtkAssistant *assistant );
static GtkWidget      *get_gconfschemav1_button( NactAssistantExport *window );
static GtkWidget      *get_gconfschemav2_button( NactAssistantExport *window );
static GtkWidget      *get_gconfdump_button( NactAssistantExport *window );
static GtkWidget      *get_ask_button( NactAssistantExport *window );
static gint            get_export_format( NactAssistantExport *window );

static void            assist_initial_load_confirm( NactAssistantExport *window, GtkAssistant *assistant );
static void            assist_runtime_init_confirm( NactAssistantExport *window, GtkAssistant *assistant );

static void            assist_initial_load_exportdone( NactAssistantExport *window, GtkAssistant *assistant );
static void            assist_runtime_init_exportdone( NactAssistantExport *window, GtkAssistant *assistant );

static void            assistant_prepare( BaseAssistant *window, GtkAssistant *assistant, GtkWidget *page );
static void            assist_prepare_confirm( NactAssistantExport *window, GtkAssistant *assistant, GtkWidget *page );
static void            assistant_apply( BaseAssistant *window, GtkAssistant *assistant );
static void            assist_prepare_exportdone( NactAssistantExport *window, GtkAssistant *assistant, GtkWidget *page );
static void            free_results( GList *list );

GType
nact_assistant_export_get_type( void )
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
	static const gchar *thisfn = "nact_assistant_export_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NactAssistantExportClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactAssistantExport ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	static const GInterfaceInfo iactions_list_iface_info = {
		( GInterfaceInitFunc ) iactions_list_iface_init,
		NULL,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( BASE_ASSISTANT_TYPE, "NactAssistantExport", &info, 0 );

	g_type_add_interface_static( type, NACT_IACTIONS_LIST_TYPE, &iactions_list_iface_info );

	return( type );
}

static void
class_init( NactAssistantExportClass *klass )
{
	static const gchar *thisfn = "nact_assistant_export_class_init";
	GObjectClass *object_class;
	BaseWindowClass *base_class;
	BaseAssistantClass *assist_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactAssistantExportClassPrivate, 1 );

	base_class = BASE_WINDOW_CLASS( klass );
	base_class->get_iprefs_window_id = window_get_iprefs_window_id;
	base_class->get_toplevel_name = window_get_toplevel_name;
	base_class->get_ui_filename = window_get_ui_filename;

	assist_class = BASE_ASSISTANT_CLASS( klass );
	assist_class->apply = assistant_apply;
	assist_class->prepare = assistant_prepare;
}

static void
iactions_list_iface_init( NactIActionsListInterface *iface )
{
	static const gchar *thisfn = "nact_assistant_export_iactions_list_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->selection_changed = on_iactions_list_selection_changed;
	iface->get_treeview_name = on_iactions_list_get_treeview_name;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_assistant_export_instance_init";
	NactAssistantExport *self;

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );
	g_assert( NACT_IS_ASSISTANT_EXPORT( instance ));
	self = NACT_ASSISTANT_EXPORT( instance );

	self->private = g_new0( NactAssistantExportPrivate, 1 );

	self->private->dispose_has_run = FALSE;

	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			BASE_WINDOW_SIGNAL_INITIAL_LOAD,
			G_CALLBACK( on_initial_load_dialog ));

	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			BASE_WINDOW_SIGNAL_RUNTIME_INIT,
			G_CALLBACK( on_runtime_init_dialog ));
}

static void
instance_dispose( GObject *window )
{
	static const gchar *thisfn = "nact_assistant_export_instance_dispose";
	NactAssistantExport *self;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );
	g_return_if_fail( NACT_IS_ASSISTANT_EXPORT( window ));
	self = NACT_ASSISTANT_EXPORT( window );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		nact_iactions_list_dispose( NACT_IACTIONS_LIST( window ));

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( window );
		}
	}
}

static void
instance_finalize( GObject *window )
{
	static const gchar *thisfn = "nact_assistant_export_instance_finalize";
	NactAssistantExport *self;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );
	g_return_if_fail( NACT_IS_ASSISTANT_EXPORT( window ));
	self = NACT_ASSISTANT_EXPORT( window );

	free_results( self->private->results );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( window );
	}
}

static NactAssistantExport *
assist_new( BaseWindow *parent )
{
	return( g_object_new( NACT_ASSISTANT_EXPORT_TYPE, BASE_WINDOW_PROP_PARENT, parent, NULL ));
}

/**
 * Run the assistant.
 *
 * @main: the main window of the application.
 */
void
nact_assistant_export_run( BaseWindow *main_window )
{
	NactAssistantExport *assist;

	assist = assist_new( main_window );
	g_object_set( G_OBJECT( assist ), BASE_WINDOW_PROP_HAS_OWN_BUILDER, TRUE, NULL );

	base_window_run( BASE_WINDOW( assist ));
}

static gchar *
window_get_iprefs_window_id( BaseWindow *window )
{
	return( g_strdup( "export-assistant" ));
}

static gchar *
window_get_toplevel_name( BaseWindow *dialog )
{
	return( g_strdup( "ExportAssistant" ));
}

static gchar *
window_get_ui_filename( BaseWindow *dialog )
{
	return( g_strdup( PKGDATADIR "/nact-assistant-export.ui" ));
}

static void
on_initial_load_dialog( NactAssistantExport *dialog, gpointer user_data )
{
	static const gchar *thisfn = "nact_assistant_export_on_initial_load_dialog";
	GtkAssistant *assistant;
	NactApplication *application;
	NAPivot *pivot;
	gboolean esc_quit, esc_confirm;

	g_debug( "%s: dialog=%p, user_data=%p", thisfn, ( void * ) dialog, ( void * ) user_data );
	g_return_if_fail( NACT_IS_ASSISTANT_EXPORT( dialog ));

	assistant = GTK_ASSISTANT( base_window_get_toplevel( BASE_WINDOW( dialog )));

	assist_initial_load_intro( dialog, assistant );
	assist_initial_load_actions_list( dialog, assistant );
	assist_initial_load_target_folder( dialog, assistant );
	assist_initial_load_format( dialog, assistant );
	assist_initial_load_confirm( dialog, assistant );
	assist_initial_load_exportdone( dialog, assistant );

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( dialog )));
	pivot = nact_application_get_pivot( application );
	esc_quit = na_iprefs_read_bool( NA_IPREFS( pivot ), IPREFS_ASSIST_ESC_QUIT, TRUE );
	base_assistant_set_cancel_on_esc( BASE_ASSISTANT( dialog ), esc_quit );
	esc_confirm = na_iprefs_read_bool( NA_IPREFS( pivot ), IPREFS_ASSIST_ESC_CONFIRM, TRUE );
	base_assistant_set_warn_on_esc( BASE_ASSISTANT( dialog ), esc_confirm );
}

static void
on_runtime_init_dialog( NactAssistantExport *dialog, gpointer user_data )
{
	static const gchar *thisfn = "nact_assistant_export_on_runtime_init_dialog";
	GtkAssistant *assistant;

	g_debug( "%s: dialog=%p, user_data=%p", thisfn, ( void * ) dialog, ( void * ) user_data );
	g_assert( NACT_IS_ASSISTANT_EXPORT( dialog ));

	assistant = GTK_ASSISTANT( base_window_get_toplevel( BASE_WINDOW( dialog )));

	assist_runtime_init_intro( dialog, assistant );
	assist_runtime_init_actions_list( dialog, assistant );
	assist_runtime_init_target_folder( dialog, assistant );
	assist_runtime_init_format( dialog, assistant );
	assist_runtime_init_confirm( dialog, assistant );
	assist_runtime_init_exportdone( dialog, assistant );
}

static void
assist_initial_load_intro( NactAssistantExport *window, GtkAssistant *assistant )
{
}

static void
assist_runtime_init_intro( NactAssistantExport *window, GtkAssistant *assistant )
{
	static const gchar *thisfn = "nact_assistant_export_runtime_init_intro";
	GtkWidget *content;

	g_debug( "%s: window=%p, assistant=%p", thisfn, ( void * ) window, ( void * ) assistant );

	content = gtk_assistant_get_nth_page( assistant, ASSIST_PAGE_INTRO );
	gtk_assistant_set_page_complete( assistant, content, TRUE );
}

static void
assist_initial_load_actions_list( NactAssistantExport *window, GtkAssistant *assistant )
{
	g_assert( NACT_IS_IACTIONS_LIST( window ));

	nact_iactions_list_set_management_mode( NACT_IACTIONS_LIST( window ), IACTIONS_LIST_MANAGEMENT_MODE_EXPORT );
	nact_iactions_list_initial_load_toplevel( NACT_IACTIONS_LIST( window ));
}

static void
assist_runtime_init_actions_list( NactAssistantExport *window, GtkAssistant *assistant )
{
	BaseWindow *parent;
	GList *tree;
	GtkWidget *content;

	parent = base_window_get_parent( BASE_WINDOW( window ));
	g_assert( NACT_IS_MAIN_WINDOW( parent ));
	g_assert( NACT_IS_IACTIONS_LIST( parent ));
	tree = nact_iactions_list_bis_get_items( NACT_IACTIONS_LIST( parent ));

	nact_iactions_list_runtime_init_toplevel( NACT_IACTIONS_LIST( window ), tree );

	content = gtk_assistant_get_nth_page( assistant, ASSIST_PAGE_ACTIONS_SELECTION );

	gtk_assistant_set_page_complete( assistant, content, FALSE );
}

static void
on_iactions_list_selection_changed( NactIActionsList *instance, GSList *selected_items )
{
	/*static const gchar *thisfn = "nact_assistant_export_on_actions_list_selection_changed";
	g_debug( "%s: selection=%p, user_data=%p", thisfn, selection, user_data );*/

	GtkAssistant *assistant;
	gint pos;
	gboolean enabled;
	GtkWidget *content;

	g_assert( NACT_IS_ASSISTANT_EXPORT( instance ));
	assistant = GTK_ASSISTANT( base_window_get_toplevel( BASE_WINDOW( instance )));
	pos = gtk_assistant_get_current_page( assistant );
	if( pos == ASSIST_PAGE_ACTIONS_SELECTION ){

		enabled = ( g_slist_length( selected_items ) > 0 );

		content = gtk_assistant_get_nth_page( assistant, pos );
		gtk_assistant_set_page_complete( assistant, content, enabled );
		gtk_assistant_update_buttons_state( assistant );
	}
}

static gchar *
on_iactions_list_get_treeview_name( NactIActionsList *instance )
{
	gchar *name = NULL;

	g_return_val_if_fail( NACT_IS_ASSISTANT_EXPORT( instance ), NULL );

	name = g_strdup( "ActionsList" );

	return( name );
}

static void
assist_initial_load_target_folder( NactAssistantExport *window, GtkAssistant *assistant )
{
	GtkFileChooser *chooser = get_folder_chooser( window );
	gtk_file_chooser_set_action( GTK_FILE_CHOOSER( chooser ), GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER );
	gtk_file_chooser_set_select_multiple( GTK_FILE_CHOOSER( chooser ), FALSE );
}

static void
assist_runtime_init_target_folder( NactAssistantExport *window, GtkAssistant *assistant )
{
	GtkFileChooser *chooser;
	NactApplication *application;
	NAPivot *pivot;
	gchar *uri;
	GtkWidget *content;

	chooser = get_folder_chooser( window );
	gtk_file_chooser_unselect_all( chooser );

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	pivot = nact_application_get_pivot( application );

	uri = na_iprefs_read_string( NA_IPREFS( pivot ), IPREFS_EXPORT_ACTIONS_FOLDER_URI, "file:///tmp" );
	if( uri && strlen( uri )){
		gtk_file_chooser_set_uri( GTK_FILE_CHOOSER( chooser ), uri );
	}
	g_free( uri );

	base_window_signal_connect(
			BASE_WINDOW( window ),
			G_OBJECT( chooser ),
			"selection-changed",
			G_CALLBACK( on_folder_selection_changed ));

	content = gtk_assistant_get_nth_page( assistant, ASSIST_PAGE_FOLDER_SELECTION );
	gtk_assistant_set_page_complete( assistant, content, FALSE );
}

static GtkFileChooser *
get_folder_chooser( NactAssistantExport *window )
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
	static const gchar *thisfn = "nact_assistant_export_on_folder_selection_changed";
	GtkAssistant *assistant;
	NactApplication *application;
	NAPivot *pivot;
	gint pos;
	gchar *uri;
	gboolean enabled;
	NactAssistantExport *assist;
	GtkWidget *content;

	g_debug( "%s: chooser=%p, user_data=%p", thisfn, ( void * ) chooser, ( void * ) user_data );
	g_assert( NACT_IS_ASSISTANT_EXPORT( user_data ));
	assist = NACT_ASSISTANT_EXPORT( user_data );

	assistant = GTK_ASSISTANT( base_window_get_toplevel( BASE_WINDOW( assist )));
	pos = gtk_assistant_get_current_page( assistant );
	if( pos == ASSIST_PAGE_FOLDER_SELECTION ){

		uri = gtk_file_chooser_get_uri( chooser );
		g_debug( "%s: uri=%s", thisfn, uri );
		enabled = ( uri && strlen( uri ) && na_utils_is_writable_dir( uri ));

		if( enabled ){
			g_free( assist->private->uri );
			assist->private->uri = g_strdup( uri );
			application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( assist )));
			pivot = nact_application_get_pivot( application );
			na_iprefs_write_string( NA_IPREFS( pivot ), IPREFS_EXPORT_ACTIONS_FOLDER_URI, uri );
		}

		g_free( uri );

		content = gtk_assistant_get_nth_page( assistant, pos );
		gtk_assistant_set_page_complete( assistant, content, enabled );
		gtk_assistant_update_buttons_state( assistant );
	}
}

static void
assist_initial_load_format( NactAssistantExport *window, GtkAssistant *assistant )
{
}

static void
assist_runtime_init_format( NactAssistantExport *window, GtkAssistant *assistant )
{
	GtkToggleButton *button;
	GtkWidget *content;
	NactApplication *application;
	NAPivot *pivot;
	gint format;

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	pivot = nact_application_get_pivot( application );
	format = na_iprefs_get_export_format( NA_IPREFS( pivot ), IPREFS_EXPORT_FORMAT );

	switch( format ){
		case IPREFS_EXPORT_FORMAT_GCONF_SCHEMA_V1:
			button = GTK_TOGGLE_BUTTON( get_gconfschemav1_button( window ));
			gtk_toggle_button_set_active( button, TRUE );
			break;

		case IPREFS_EXPORT_FORMAT_GCONF_SCHEMA_V2:
			button = GTK_TOGGLE_BUTTON( get_gconfschemav2_button( window ));
			gtk_toggle_button_set_active( button, TRUE );
			break;

		case IPREFS_EXPORT_FORMAT_ASK:
			button = GTK_TOGGLE_BUTTON( get_ask_button( window ));
			gtk_toggle_button_set_active( button, TRUE );
			break;

		case IPREFS_EXPORT_FORMAT_GCONF_ENTRY:
		default:
			button = GTK_TOGGLE_BUTTON( get_gconfdump_button( window ));
			gtk_toggle_button_set_active( button, TRUE );
			break;
	}

	content = gtk_assistant_get_nth_page( assistant, ASSIST_PAGE_FORMAT_SELECTION );
	gtk_assistant_set_page_complete( assistant, content, TRUE );
}

static GtkWidget *
get_gconfschemav1_button( NactAssistantExport *window )
{
	return( base_window_get_widget( BASE_WINDOW( window ), "ExportSchemaV1Button" ));
}

static GtkWidget *
get_gconfschemav2_button( NactAssistantExport *window )
{
	return( base_window_get_widget( BASE_WINDOW( window ), "ExportSchemaV2Button" ));
}

static GtkWidget *
get_gconfdump_button( NactAssistantExport *window )
{
	return( base_window_get_widget( BASE_WINDOW( window ), "ExportGConfDumpButton" ));
}

static GtkWidget *
get_ask_button( NactAssistantExport *window )
{
	return( base_window_get_widget( BASE_WINDOW( window ), "ExportAskButton" ));
}

static gint
get_export_format( NactAssistantExport *window )
{
	GtkToggleButton *gconf_schema_v1_button;
	GtkToggleButton *gconf_schema_v2_button;
	GtkToggleButton *ask_button;
	gint format;

	gconf_schema_v1_button = GTK_TOGGLE_BUTTON( get_gconfschemav1_button( window ));
	gconf_schema_v2_button = GTK_TOGGLE_BUTTON( get_gconfschemav2_button( window ));
	ask_button = GTK_TOGGLE_BUTTON( get_ask_button( window ));

	format = IPREFS_EXPORT_FORMAT_GCONF_ENTRY;
	if( gtk_toggle_button_get_active( gconf_schema_v1_button )){
		format = IPREFS_EXPORT_FORMAT_GCONF_SCHEMA_V1;
	} else if( gtk_toggle_button_get_active( gconf_schema_v2_button )){
		format = IPREFS_EXPORT_FORMAT_GCONF_SCHEMA_V2;
	} else if( gtk_toggle_button_get_active( ask_button )){
		format = IPREFS_EXPORT_FORMAT_ASK;
	}

	return( format );
}

static void
assist_initial_load_confirm( NactAssistantExport *window, GtkAssistant *assistant )
{
}

static void
assist_runtime_init_confirm( NactAssistantExport *window, GtkAssistant *assistant )
{
}

static void
assist_initial_load_exportdone( NactAssistantExport *window, GtkAssistant *assistant )
{
}

static void
assist_runtime_init_exportdone( NactAssistantExport *window, GtkAssistant *assistant )
{
}

static void
assistant_prepare( BaseAssistant *window, GtkAssistant *assistant, GtkWidget *page )
{
	/*static const gchar *thisfn = "nact_assistant_export_on_prepare";
	g_debug( "%s: window=%p, assistant=%p, page=%p", thisfn, window, assistant, page );*/

	GtkAssistantPageType type = gtk_assistant_get_page_type( assistant, page );

	switch( type ){
		case GTK_ASSISTANT_PAGE_CONFIRM:
			assist_prepare_confirm( NACT_ASSISTANT_EXPORT( window ), assistant, page );
			break;

		case GTK_ASSISTANT_PAGE_SUMMARY:
			assist_prepare_exportdone( NACT_ASSISTANT_EXPORT( window ), assistant, page );
			break;

		default:
			break;
	}
}

static void
assist_prepare_confirm( NactAssistantExport *window, GtkAssistant *assistant, GtkWidget *page )
{
	static const gchar *thisfn = "nact_assistant_export_prepare_confirm";
	gchar *text, *tmp, *text2;
	gchar *label1, *label2, *label3;
	GList *actions, *ia;
	NactApplication *application;
	NAPivot *pivot;
	gint format;

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	pivot = nact_application_get_pivot( application );

	g_debug( "%s: window=%p, assistant=%p, page=%p",
			thisfn, ( void * ) window, ( void * ) assistant, ( void * ) page );

	/* i18n: this is the title of the confirm page of the export assistant */
	text = g_strdup( _( "About to export selected actions:" ));
	tmp = g_strdup_printf( "<b>%s</b>\n\n", text );
	g_free( text );
	text = tmp;

	actions = nact_iactions_list_bis_get_selected_items( NACT_IACTIONS_LIST( window ));

	for( ia = actions ; ia ; ia = ia->next ){
		tmp = g_strdup_printf( "%s\t%s\n", text, na_object_get_label( ia->data ));
		g_free( text );
		text = tmp;
	}

	na_object_free_items_list( actions );

	g_assert( window->private->uri && strlen( window->private->uri ));

	/* i18n: all exported actions go to one destination folder */
	text2 = g_strdup( _( "Into the destination folder:" ));
	tmp = g_strdup_printf( "%s\n\n<b>%s</b>\n\n\t%s", text, text2, window->private->uri );
	g_free( text2 );
	g_free( text );
	text = tmp;

	label1 = NULL;
	label2 = NULL;
	format = get_export_format( window );
	switch( format ){
		case IPREFS_EXPORT_FORMAT_GCONF_SCHEMA_V1:
			label1 = g_strdup( gtk_button_get_label( GTK_BUTTON( get_gconfschemav1_button( window ))));
			label2 = g_strdup( gtk_label_get_text( GTK_LABEL( base_window_get_widget( BASE_WINDOW( window ), "ExportSchemaV1Label"))));
			break;

		case IPREFS_EXPORT_FORMAT_GCONF_SCHEMA_V2:
			label1 = g_strdup( gtk_button_get_label( GTK_BUTTON( get_gconfschemav2_button( window ))));
			label2 = g_strdup( gtk_label_get_text( GTK_LABEL( base_window_get_widget( BASE_WINDOW( window ), "ExportSchemaV2Label"))));
			break;

		case IPREFS_EXPORT_FORMAT_GCONF_ENTRY:
			label1 = g_strdup( gtk_button_get_label( GTK_BUTTON( get_gconfdump_button( window ))));
			label2 = g_strdup( gtk_label_get_text( GTK_LABEL( base_window_get_widget( BASE_WINDOW( window ), "ExportGConfDumpLabel"))));
			break;

		case IPREFS_EXPORT_FORMAT_ASK:
			label1 = g_strdup( gtk_button_get_label( GTK_BUTTON( get_ask_button( window ))));
			label2 = g_strdup( gtk_label_get_text( GTK_LABEL( base_window_get_widget( BASE_WINDOW( window ), "ExportAskLabel"))));
			break;

		default:
			g_return_if_reached();
			break;
	}
	na_iprefs_set_export_format( NA_IPREFS( pivot ), IPREFS_EXPORT_FORMAT, format );

	label3 = na_utils_prefix_strings( "\t", label2 );
	tmp = g_strdup_printf( "%s\n\n<b>%s</b>\n\n%s", text, label1, label3 );
	g_free( label3 );
	g_free( label2 );
	g_free( label1 );
	g_free( text );
	text = tmp;

	gtk_label_set_markup( GTK_LABEL( page ), text );
	g_free( text );

	gtk_assistant_set_page_complete( assistant, page, TRUE );
}

/*
 * As of 1.11, nact_gconf_writer doesn't return any error message.
 * An error is simply indicated by returning a null filename.
 * So we provide a general error message.
 */
static void
assistant_apply( BaseAssistant *wnd, GtkAssistant *assistant )
{
	static const gchar *thisfn = "nact_assistant_export_on_apply";
	NactAssistantExport *window;
	NactApplication *application;
	NAPivot *pivot;
	GList *actions, *ia;
	ExportStruct *str;

	g_debug( "%s: window=%p, assistant=%p", thisfn, ( void * ) wnd, ( void * ) assistant );
	g_assert( NACT_IS_ASSISTANT_EXPORT( wnd ));
	window = NACT_ASSISTANT_EXPORT( wnd );

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	pivot = nact_application_get_pivot( application );

	actions = nact_iactions_list_bis_get_selected_items( NACT_IACTIONS_LIST( window ));

	g_assert( window->private->uri && strlen( window->private->uri ));

	for( ia = actions ; ia ; ia = ia->next ){
		str = g_new0( ExportStruct, 1 );
		window->private->results = g_list_append( window->private->results, str );

		str->action = NA_OBJECT_ACTION( na_iduplicable_get_origin( NA_IDUPLICABLE( ia->data )));

		str->format = na_iprefs_get_export_format( NA_IPREFS( pivot ), IPREFS_EXPORT_FORMAT );
		if( str->format == IPREFS_EXPORT_FORMAT_ASK ){
			str->format = nact_assistant_export_ask_user( BASE_WINDOW( wnd ), str->action );
			if( str->format == IPREFS_EXPORT_NO_EXPORT ){
				str->msg = g_slist_append( NULL, g_strdup( _( "Export canceled due to user action." )));
			}
		}

		if( str->format != IPREFS_EXPORT_NO_EXPORT ){
			str->fname = na_xml_writer_export( str->action, window->private->uri, str->format, &str->msg );
		}
	}

	na_object_free_items_list( actions );
}

static void
assist_prepare_exportdone( NactAssistantExport *window, GtkAssistant *assistant, GtkWidget *page )
{
	static const gchar *thisfn = "nact_assistant_export_prepare_exportdone";
	gchar *text, *tmp;
	GList *ir;
	ExportStruct *str;
	gchar *label;
	GSList *is;
	gint errors;

	g_debug( "%s: window=%p, assistant=%p, page=%p",
			thisfn, ( void * ) window, ( void * ) assistant, ( void * ) page );

	/* i18n: result of the export assistant */
	text = g_strdup( _( "Selected actions have been proceeded :" ));
	tmp = g_strdup_printf( "<b>%s</b>\n\n", text );
	g_free( text );
	text = tmp;

	errors = 0;

	for( ir = window->private->results ; ir ; ir = ir->next ){
		str = ( ExportStruct * ) ir->data;

		label = na_object_get_label( str->action );
		tmp = g_strdup_printf( "%s\t%s\n", text, label );
		g_free( text );
		text = tmp;
		g_free( label );

		if( str->fname ){
			/* i18n: action as been successfully exported to <filename> */
			tmp = g_strdup_printf( "%s\t\t%s\n\t\t%s\n", text, _( "Successfully exported as" ), str->fname );
			g_free( text );
			text = tmp;

		} else if( str->format != IPREFS_EXPORT_NO_EXPORT ){
			errors += 1;
		}

		/* add messages */
		for( is = str->msg ; is ; is = is->next ){
			tmp = g_strdup_printf( "%s\t\t%s\n", text, ( gchar * ) is->data );
			g_free( text );
			text = tmp;
		}

		/* add a blank line between two actions */
		tmp = g_strdup_printf( "%s\n", text );
		g_free( text );
		text = tmp;
	}

	if( errors ){
		text = g_strdup_printf( "%s%s", text,
				_( "You may not have write permissions on selected folder." ));
		g_free( text );
		text = tmp;
	}

	gtk_label_set_markup( GTK_LABEL( page ), text );
	g_free( text );

	gtk_assistant_set_page_complete( assistant, page, TRUE );
	base_assistant_set_warn_on_cancel( BASE_ASSISTANT( window ), FALSE );
	base_assistant_set_warn_on_esc( BASE_ASSISTANT( window ), FALSE );
}

static void
free_results( GList *list )
{
	GList *ir;
	ExportStruct *str;

	for( ir = list ; ir ; ir = ir->next ){
		str = ( ExportStruct * ) ir->data;
		g_free( str->fname );
		na_utils_free_string_list( str->msg );
	}

	g_list_free( list );
}
