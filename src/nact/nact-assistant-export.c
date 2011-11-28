/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011 Pierre Wieser and others (see AUTHORS)
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

#include <api/na-core-utils.h>
#include <api/na-object-api.h>

#include <core/na-gtk-utils.h>
#include <core/na-iprefs.h>
#include <core/na-exporter.h>

#include "nact-application.h"
#include "nact-main-window.h"
#include "nact-assistant-export.h"
#include "nact-export-ask.h"
#include "nact-export-format.h"
#include "nact-tree-view.h"

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
struct _NactAssistantExportClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _NactAssistantExportPrivate {
	gboolean      dispose_has_run;
	NactTreeView *items_view;
	gboolean      preferences_locked;
	gchar        *uri;
	GList        *selected_items;
	GList        *results;
};

typedef struct {
	NAObjectItem *item;
	GSList       *msg;
	gchar        *fname;
	GQuark        format;
}
	ExportStruct;

static const gchar        *st_xmlui_filename = PKGDATADIR "/nact-assistant-export.ui";
static const gchar        *st_toplevel_name  = "ExportAssistant";
static const gchar        *st_wsp_name       = NA_IPREFS_EXPORT_ASSISTANT_WSP;

static BaseAssistantClass *st_parent_class   = NULL;

static GType           register_type( void );
static void            class_init( NactAssistantExportClass *klass );
static void            instance_init( GTypeInstance *instance, gpointer klass );
static void            instance_dispose( GObject *application );
static void            instance_finalize( GObject *application );

static void            on_base_initialize_gtk_toplevel( NactAssistantExport *dialog, GtkAssistant *toplevel, gpointer user_data );
static void            on_base_initialize_base_window( NactAssistantExport *dialog, gpointer user_data );
static void            on_base_all_widgets_showed( NactAssistantExport *dialog, gpointer user_data );

static void            assist_runtime_init_intro( NactAssistantExport *window, GtkAssistant *assistant );

static void            assist_runtime_init_actions_list( NactAssistantExport *window, GtkAssistant *assistant );
static void            on_tree_view_selection_changed( NactAssistantExport *window, GList *selected_items, gpointer user_data );
static void            assist_initial_load_target_folder( NactAssistantExport *window, GtkAssistant *assistant );
static void            assist_runtime_init_target_folder( NactAssistantExport *window, GtkAssistant *assistant );
static GtkFileChooser *get_folder_chooser( NactAssistantExport *window );
static void            on_folder_selection_changed( GtkFileChooser *chooser, gpointer user_data );

static void            assist_initial_load_format( NactAssistantExport *window, GtkAssistant *assistant );
static void            assist_runtime_init_format( NactAssistantExport *window, GtkAssistant *assistant );
static NAExportFormat *get_export_format( NactAssistantExport *window );
static GtkWidget      *get_box_container( NactAssistantExport *window );

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

	g_debug( "%s", thisfn );

	type = g_type_register_static( BASE_ASSISTANT_TYPE, "NactAssistantExport", &info, 0 );

	return( type );
}

static void
class_init( NactAssistantExportClass *klass )
{
	static const gchar *thisfn = "nact_assistant_export_class_init";
	GObjectClass *object_class;
	BaseAssistantClass *assist_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactAssistantExportClassPrivate, 1 );

	assist_class = BASE_ASSISTANT_CLASS( klass );
	assist_class->apply = assistant_apply;
	assist_class->prepare = assistant_prepare;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_assistant_export_instance_init";
	NactAssistantExport *self;

	g_return_if_fail( NACT_IS_ASSISTANT_EXPORT( instance ));

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self = NACT_ASSISTANT_EXPORT( instance );

	self->private = g_new0( NactAssistantExportPrivate, 1 );

	self->private->dispose_has_run = FALSE;

	base_window_signal_connect( BASE_WINDOW( instance ),
			G_OBJECT( instance ), BASE_SIGNAL_INITIALIZE_GTK, G_CALLBACK( on_base_initialize_gtk_toplevel ));

	base_window_signal_connect( BASE_WINDOW( instance ),
			G_OBJECT( instance ), BASE_SIGNAL_INITIALIZE_WINDOW, G_CALLBACK( on_base_initialize_base_window ));

	base_window_signal_connect( BASE_WINDOW( instance ),
			G_OBJECT( instance ), BASE_SIGNAL_ALL_WIDGETS_SHOWED, G_CALLBACK( on_base_all_widgets_showed ));
}

static void
instance_dispose( GObject *window )
{
	static const gchar *thisfn = "nact_assistant_export_instance_dispose";
	NactAssistantExport *self;

	g_return_if_fail( NACT_IS_ASSISTANT_EXPORT( window ));

	self = NACT_ASSISTANT_EXPORT( window );

	if( !self->private->dispose_has_run ){
		g_debug( "%s: window=%p (%s)", thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ));

		self->private->dispose_has_run = TRUE;

		g_object_unref( self->private->items_view );

		if( self->private->selected_items ){
			self->private->selected_items = na_object_free_items( self->private->selected_items );
		}

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

	g_return_if_fail( NACT_IS_ASSISTANT_EXPORT( window ));

	g_debug( "%s: window=%p (%s)", thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ));

	self = NACT_ASSISTANT_EXPORT( window );

	free_results( self->private->results );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( window );
	}
}

/**
 * Run the assistant.
 *
 * @main: the main window of the application.
 */
void
nact_assistant_export_run( BaseWindow *main_window )
{
	NactAssistantExport *assistant;
	gboolean esc_quit, esc_confirm;

	g_return_if_fail( NACT_IS_MAIN_WINDOW( main_window ));

	esc_quit = na_settings_get_boolean( NA_IPREFS_ASSISTANT_ESC_QUIT, NULL, NULL );
	esc_confirm = na_settings_get_boolean( NA_IPREFS_ASSISTANT_ESC_CONFIRM, NULL, NULL );

	assistant = g_object_new( NACT_ASSISTANT_EXPORT_TYPE,
			BASE_PROP_PARENT,          main_window,
			BASE_PROP_HAS_OWN_BUILDER, TRUE,
			BASE_PROP_XMLUI_FILENAME,  st_xmlui_filename,
			BASE_PROP_TOPLEVEL_NAME,   st_toplevel_name,
			BASE_PROP_WSP_NAME,        st_wsp_name,
			BASE_PROP_QUIT_ON_ESCAPE,  esc_quit,
			BASE_PROP_WARN_ON_ESCAPE,  esc_confirm,
			NULL );

	base_window_run( BASE_WINDOW( assistant ));
}

static void
on_base_initialize_gtk_toplevel( NactAssistantExport *dialog, GtkAssistant *assistant, gpointer user_data )
{
	static const gchar *thisfn = "nact_assistant_export_on_base_initialize_gtk_toplevel";
	GtkWidget *page;
	gboolean are_locked, mandatory;

	g_return_if_fail( NACT_IS_ASSISTANT_EXPORT( dialog ));

	if( !dialog->private->dispose_has_run ){
		g_debug( "%s: dialog=%p, assistant=%p, user_data=%p",
				thisfn, ( void * ) dialog, ( void * ) assistant, ( void * ) user_data );

		page = gtk_assistant_get_nth_page( assistant, ASSIST_PAGE_ACTIONS_SELECTION );
		dialog->private->items_view =
				nact_tree_view_new( BASE_WINDOW( dialog ),
						GTK_CONTAINER( page ), "ActionsList", TREE_MODE_SELECTION );

		are_locked = na_settings_get_boolean( NA_IPREFS_ADMIN_PREFERENCES_LOCKED, NULL, &mandatory );
		dialog->private->preferences_locked = are_locked && mandatory;

		assist_initial_load_target_folder( dialog, assistant );
		assist_initial_load_format( dialog, assistant );
	}
}

static void
on_base_initialize_base_window( NactAssistantExport *dialog, gpointer user_data )
{
	static const gchar *thisfn = "nact_assistant_export_on_base_initialize_base_window";
	GtkAssistant *assistant;

	g_return_if_fail( NACT_IS_ASSISTANT_EXPORT( dialog ));

	if( !dialog->private->dispose_has_run ){
		g_debug( "%s: dialog=%p, user_data=%p", thisfn, ( void * ) dialog, ( void * ) user_data );

		base_window_signal_connect( BASE_WINDOW( dialog ),
				G_OBJECT( dialog ), TREE_SIGNAL_SELECTION_CHANGED, G_CALLBACK( on_tree_view_selection_changed ));

		assistant = GTK_ASSISTANT( base_window_get_gtk_toplevel( BASE_WINDOW( dialog )));

		assist_runtime_init_intro( dialog, assistant );
		assist_runtime_init_actions_list( dialog, assistant );
		assist_runtime_init_target_folder( dialog, assistant );
		assist_runtime_init_format( dialog, assistant );
	}
}

static void
on_base_all_widgets_showed( NactAssistantExport *dialog, gpointer user_data )
{
	static const gchar *thisfn = "nact_assistant_export_on_base_all_widgets_showed";
	NactMainWindow *main_window;
	NactTreeView *main_items_view;
	GList *items;

	g_return_if_fail( NACT_IS_ASSISTANT_EXPORT( dialog ));

	if( !dialog->private->dispose_has_run ){
		g_debug( "%s: dialog=%p, user_data=%p", thisfn, ( void * ) dialog, ( void * ) user_data );

		/* setup the data here so that we are sure all companion objects
		 * have connected their signal handlers
		 */
		main_window = NACT_MAIN_WINDOW( base_window_get_parent( BASE_WINDOW( dialog )));
		main_items_view = nact_main_window_get_items_view( main_window );
		items = nact_tree_view_get_items( main_items_view );
		nact_tree_view_fill( dialog->private->items_view, items );
	}
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
assist_runtime_init_actions_list( NactAssistantExport *window, GtkAssistant *assistant )
{
	GtkWidget *content;

	content = gtk_assistant_get_nth_page( assistant, ASSIST_PAGE_ACTIONS_SELECTION );
	gtk_assistant_set_page_complete( assistant, content, FALSE );
}

static void
on_tree_view_selection_changed( NactAssistantExport *window, GList *selected_items, gpointer user_data )
{
	static const gchar *thisfn = "nact_assistant_export_on_tree_view_selection_changed";
	GtkAssistant *assistant;
	gint pos;
	gboolean enabled;
	GtkWidget *content;

	g_return_if_fail( NACT_IS_ASSISTANT_EXPORT( window ));

	g_debug( "%s: window=%p, selected_items=%p (count=%d), user_data=%p",
			thisfn, ( void * ) window,
			( void * ) selected_items, g_list_length( selected_items ), ( void * ) user_data );

	if( window->private->selected_items ){
		window->private->selected_items = na_object_free_items( window->private->selected_items );
	}

	assistant = GTK_ASSISTANT( base_window_get_gtk_toplevel( BASE_WINDOW( window )));
	pos = gtk_assistant_get_current_page( assistant );

	if( pos == ASSIST_PAGE_ACTIONS_SELECTION ){
		enabled = ( g_list_length( selected_items ) > 0 );
		window->private->selected_items = na_object_copyref_items( selected_items );
		content = gtk_assistant_get_nth_page( assistant, pos );
		gtk_assistant_set_page_complete( assistant, content, enabled );
		gtk_assistant_update_buttons_state( assistant );
	}
}

static void
assist_initial_load_target_folder( NactAssistantExport *window, GtkAssistant *assistant )
{
	GtkFileChooser *chooser = get_folder_chooser( window );
	gtk_file_chooser_set_action( GTK_FILE_CHOOSER( chooser ), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER );
	gtk_file_chooser_set_create_folders( GTK_FILE_CHOOSER( chooser ), TRUE );
	gtk_file_chooser_set_select_multiple( GTK_FILE_CHOOSER( chooser ), FALSE );
}

static void
assist_runtime_init_target_folder( NactAssistantExport *window, GtkAssistant *assistant )
{
	GtkFileChooser *chooser;
	gchar *uri;
	GtkWidget *content;

	chooser = get_folder_chooser( window );
	gtk_file_chooser_unselect_all( chooser );

	uri = na_settings_get_string( NA_IPREFS_EXPORT_ASSISTANT_URI, NULL, NULL );
	if( uri && strlen( uri )){
		gtk_file_chooser_set_current_folder_uri( GTK_FILE_CHOOSER( chooser ), uri );
	}
	g_free( uri );

	base_window_signal_connect( BASE_WINDOW( window ),
			G_OBJECT( chooser ), "selection-changed", G_CALLBACK( on_folder_selection_changed ));

	content = gtk_assistant_get_nth_page( assistant, ASSIST_PAGE_FOLDER_SELECTION );
	gtk_assistant_set_page_complete( assistant, content, FALSE );
}

static GtkFileChooser *
get_folder_chooser( NactAssistantExport *window )
{
	GtkAssistant *assistant = GTK_ASSISTANT( base_window_get_gtk_toplevel( BASE_WINDOW( window )));
	GtkWidget *page = gtk_assistant_get_nth_page( assistant, ASSIST_PAGE_FOLDER_SELECTION );
	return( GTK_FILE_CHOOSER( na_gtk_utils_search_for_child_widget( GTK_CONTAINER( page ), "ExportFolderChooser" )));
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
	gint pos;
	gchar *uri;
	gboolean enabled;
	NactAssistantExport *assist;
	GtkWidget *content;

	g_debug( "%s: chooser=%p, user_data=%p", thisfn, ( void * ) chooser, ( void * ) user_data );
	g_assert( NACT_IS_ASSISTANT_EXPORT( user_data ));
	assist = NACT_ASSISTANT_EXPORT( user_data );

	assistant = GTK_ASSISTANT( base_window_get_gtk_toplevel( BASE_WINDOW( assist )));
	pos = gtk_assistant_get_current_page( assistant );
	if( pos == ASSIST_PAGE_FOLDER_SELECTION ){

		uri = gtk_file_chooser_get_current_folder_uri( chooser );
		g_debug( "%s: uri=%s", thisfn, uri );
		enabled = ( uri && strlen( uri ) && na_core_utils_dir_is_writable_uri( uri ));

		if( enabled ){
			g_free( assist->private->uri );
			assist->private->uri = g_strdup( uri );
			na_settings_set_string( NA_IPREFS_EXPORT_ASSISTANT_URI, uri );
		}

		g_free( uri );

		content = gtk_assistant_get_nth_page( assistant, pos );
		gtk_assistant_set_page_complete( assistant, content, enabled );
		gtk_assistant_update_buttons_state( assistant );
	}

	g_debug( "%s: quitting", thisfn );
}

static void
assist_initial_load_format( NactAssistantExport *window, GtkAssistant *assistant )
{
	NactApplication *application;
	NAUpdater *updater;
	GtkWidget *container;

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	updater = nact_application_get_updater( application );
	container = get_box_container( window );

	nact_export_format_init_display( container,
			NA_PIVOT( updater ), EXPORT_FORMAT_DISPLAY_ASSISTANT, !window->private->preferences_locked );
}

static void
assist_runtime_init_format( NactAssistantExport *window, GtkAssistant *assistant )
{
	GtkWidget *container;
	GtkWidget *page;
	GQuark format;
	gboolean mandatory;

	format = na_iprefs_get_export_format( NA_IPREFS_EXPORT_PREFERRED_FORMAT, &mandatory );
	container = get_box_container( window );
#ifdef NA_MAINTAINER_MODE
	na_gtk_utils_dump_children( GTK_CONTAINER( container ));
#endif
	nact_export_format_select( container, !mandatory, format );

	page = gtk_assistant_get_nth_page( assistant, ASSIST_PAGE_FORMAT_SELECTION );
	gtk_assistant_set_page_complete( assistant, page, TRUE );
}

static NAExportFormat *
get_export_format( NactAssistantExport *window )
{
	GtkWidget *container;
	NAExportFormat *format;

	container = get_box_container( window );
	format = nact_export_format_get_selected( container );

	return( format );
}

static GtkWidget *
get_box_container( NactAssistantExport *window )
{
	GtkAssistant *assistant = GTK_ASSISTANT( base_window_get_gtk_toplevel( BASE_WINDOW( window )));
	GtkWidget *page = gtk_assistant_get_nth_page( assistant, ASSIST_PAGE_FORMAT_SELECTION );
	return( na_gtk_utils_search_for_child_widget( GTK_CONTAINER( page ), "AssistantExportFormatVBox" ));
}

static void
assistant_prepare( BaseAssistant *window, GtkAssistant *assistant, GtkWidget *page )
{
	static const gchar *thisfn = "nact_assistant_export_on_prepare";

	g_debug( "%s: window=%p, assistant=%p, page=%p", thisfn, window, assistant, page );

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
	GString *text;
	gchar *label_item;
	gchar *label11, *label12;
	gchar *label21, *label22;
	GList *it;
	NAExportFormat *format;
	GtkLabel *confirm_label;

	g_debug( "%s: window=%p, assistant=%p, page=%p",
			thisfn, ( void * ) window, ( void * ) assistant, ( void * ) page );

	/* i18n: this is the title of the confirm page of the export assistant */
	text = g_string_new( "" );
	g_string_printf( text, "<b>%s</b>\n\n", _( "About to export selected items:" ));

	for( it = window->private->selected_items ; it ; it = it->next ){
		label_item = na_object_get_label( it->data );
		g_string_append_printf( text, "\t%s\n", label_item );
		g_free( label_item );
	}

	g_assert( window->private->uri && strlen( window->private->uri ));

	/* i18n: all exported actions go to one destination folder */
	g_string_append_printf( text,
			"\n<b>%s</b>\n\n\t%s\n", _( "Into the destination folder:" ), window->private->uri );

	label11 = NULL;
	label21 = NULL;
	format = get_export_format( window );
	label11 = na_export_format_get_label( format );
	label21 = na_export_format_get_description( format );
	na_iprefs_set_export_format( NA_IPREFS_EXPORT_PREFERRED_FORMAT, na_export_format_get_quark( format ));
	label12 = na_core_utils_str_remove_char( label11, "_" );
	label22 = na_core_utils_str_add_prefix( "\t", label21 );
	g_string_append_printf( text, "\n<b>%s</b>\n\n%s", label12, label22 );
	g_free( label22 );
	g_free( label21 );
	g_free( label12 );
	g_free( label11 );

	confirm_label = GTK_LABEL( na_gtk_utils_search_for_child_widget( GTK_CONTAINER( page ), "AssistantExportConfirmLabel" ));
	gtk_label_set_markup( confirm_label, text->str );
	g_string_free( text, TRUE );

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
	GList *ia;
	ExportStruct *str;
	NactApplication *application;
	NAUpdater *updater;
	gboolean first;

	g_return_if_fail( NACT_IS_ASSISTANT_EXPORT( wnd ));

	g_debug( "%s: window=%p, assistant=%p", thisfn, ( void * ) wnd, ( void * ) assistant );

	window = NACT_ASSISTANT_EXPORT( wnd );

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	updater = nact_application_get_updater( application );
	first = TRUE;

	g_return_if_fail( window->private->uri && strlen( window->private->uri ));

	for( ia = window->private->selected_items ; ia ; ia = ia->next ){
		str = g_new0( ExportStruct, 1 );
		window->private->results = g_list_append( window->private->results, str );

		str->item = NA_OBJECT_ITEM( na_object_get_origin( NA_IDUPLICABLE( ia->data )));

		str->format = na_iprefs_get_export_format( NA_IPREFS_EXPORT_PREFERRED_FORMAT, NULL );

		if( str->format == IPREFS_EXPORT_FORMAT_ASK ){
			str->format = nact_export_ask_user( BASE_WINDOW( wnd ), str->item, first );

			if( str->format == IPREFS_EXPORT_NO_EXPORT ){
				str->msg = g_slist_append( NULL, g_strdup( _( "Export canceled due to user action." )));
			}
		}

		if( str->format != IPREFS_EXPORT_NO_EXPORT ){
			str->fname = na_exporter_to_file( NA_PIVOT( updater ), str->item, window->private->uri, str->format, &str->msg );
		}

		first = FALSE;
	}
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
	GtkTextView *summary_textview;
	GtkTextBuffer *summary_buffer;
	GtkTextTag *title_tag;
	GtkTextIter start, end;
	gint title_len;

	g_debug( "%s: window=%p, assistant=%p, page=%p",
			thisfn, ( void * ) window, ( void * ) assistant, ( void * ) page );

	/* i18n: result of the export assistant */
	text = g_strdup( _( "Selected actions have been proceeded :" ));
	title_len = g_utf8_strlen( text, -1 );
	tmp = g_strdup_printf( "%s\n\n", text );
	g_free( text );
	text = tmp;

	errors = 0;

	for( ir = window->private->results ; ir ; ir = ir->next ){
		str = ( ExportStruct * ) ir->data;

		label = na_object_get_label( str->item );
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

	summary_textview = GTK_TEXT_VIEW( na_gtk_utils_search_for_child_widget( GTK_CONTAINER( page ), "AssistantExportSummaryTextView" ));
	summary_buffer = gtk_text_view_get_buffer( summary_textview );
	gtk_text_buffer_set_text( summary_buffer, text, -1 );
	g_free( text );

	title_tag = gtk_text_buffer_create_tag( summary_buffer, "title",
			"weight", PANGO_WEIGHT_BOLD,
			NULL );

	gtk_text_buffer_get_iter_at_offset( summary_buffer, &start, 0 );
	gtk_text_buffer_get_iter_at_offset( summary_buffer, &end, title_len );
	gtk_text_buffer_apply_tag( summary_buffer, title_tag, &start, &end );

	g_object_unref( title_tag );

	gtk_assistant_set_page_complete( assistant, page, TRUE );
	g_object_set( G_OBJECT( window ), BASE_PROP_WARN_ON_ESCAPE, FALSE, NULL );
}

static void
free_results( GList *list )
{
	GList *ir;
	ExportStruct *str;

	for( ir = list ; ir ; ir = ir->next ){
		str = ( ExportStruct * ) ir->data;
		g_free( str->fname );
		na_core_utils_slist_free( str->msg );
	}

	g_list_free( list );
}
