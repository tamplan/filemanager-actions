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

#include <common/na-object-api.h>
#include <common/na-object-action-class.h>
#include <common/na-iio-provider.h>
#include <common/na-utils.h>

#include "base-iprefs.h"
#include "base-application.h"
#include "nact-iactions-list.h"
#include "nact-assistant-import.h"
#include "nact-xml-reader.h"

/* Import Assistant
 *
 * pos.  type     title
 * ---   -------  --------------------------------------------------
 *   0   Intro    Introduction
 *   1   Content  Selection of the files
 *   2   Content  Duplicate management: what to do with duplicates ?
 *   2   Confirm  Display the selected files before import
 *   3   Summary  Import is done: summary of the done operations
 */

enum {
	ASSIST_PAGE_INTRO = 0,
	ASSIST_PAGE_FILES_SELECTION,
	ASSIST_PAGE_DUPLICATES,
	ASSIST_PAGE_CONFIRM,
	ASSIST_PAGE_DONE
};

typedef struct {
	gchar          *uri;
	NAObjectAction *action;
	GSList         *msg;
}
	ImportUriStruct;

/* private class data
 */
struct NactAssistantImportClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NactAssistantImportPrivate {
	gboolean  dispose_has_run;
	GSList   *results;
	GSList   *actions;
};

#define IPREFS_IMPORT_ACTIONS_FOLDER_URI		"import-folder-uri"
#define IPREFS_IMPORT_ACTIONS_IMPORT_MODE		"import-mode"

static BaseAssistantClass *st_parent_class = NULL;

static GType    register_type( void );
static void     class_init( NactAssistantImportClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_dispose( GObject *application );
static void     instance_finalize( GObject *application );

static NactAssistantImport *assist_new( BaseApplication *application );

static gchar   *window_get_iprefs_window_id( BaseWindow *window );
static gchar   *window_get_dialog_name( BaseWindow *dialog );

static void     on_runtime_init_dialog( NactAssistantImport *dialog, gpointer user_data );
static void     runtime_init_intro( NactAssistantImport *window, GtkAssistant *assistant );
static void     runtime_init_file_selector( NactAssistantImport *window, GtkAssistant *assistant );
static void     on_file_selection_changed( GtkFileChooser *chooser, gpointer user_data );
static gboolean has_readable_files( GSList *uris );
static void     runtime_init_duplicates( NactAssistantImport *window, GtkAssistant *assistant );
static void     set_import_mode( NactAssistantImport *window, gint mode );

static void     assistant_prepare( BaseAssistant *window, GtkAssistant *assistant, GtkWidget *page );
static void     prepare_confirm( NactAssistantImport *window, GtkAssistant *assistant, GtkWidget *page );
static gint     get_import_mode( NactAssistantImport *window );
static gchar   *add_import_mode( NactAssistantImport *window, const gchar *text );
static void     assistant_apply( BaseAssistant *window, GtkAssistant *assistant );
static void     prepare_importdone( NactAssistantImport *window, GtkAssistant *assistant, GtkWidget *page );
static void     free_results( GSList *list );

GType
nact_assistant_import_get_type( void )
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
	static const gchar *thisfn = "nact_assistant_import_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NactAssistantImportClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactAssistantImport ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( BASE_ASSISTANT_TYPE, "NactAssistantImport", &info, 0 );

	return( type );
}

static void
class_init( NactAssistantImportClass *klass )
{
	static const gchar *thisfn = "nact_assistant_import_class_init";
	GObjectClass *object_class;
	BaseWindowClass *base_class;
	BaseAssistantClass *assist_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactAssistantImportClassPrivate, 1 );

	base_class = BASE_WINDOW_CLASS( klass );
	base_class->get_toplevel_name = window_get_dialog_name;
	base_class->get_iprefs_window_id = window_get_iprefs_window_id;

	assist_class = BASE_ASSISTANT_CLASS( klass );
	assist_class->apply = assistant_apply;
	assist_class->prepare = assistant_prepare;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_assistant_import_instance_init";
	NactAssistantImport *self;

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );
	g_return_if_fail( NACT_IS_ASSISTANT_IMPORT( instance ));
	self = NACT_ASSISTANT_IMPORT( instance );

	self->private = g_new0( NactAssistantImportPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->results = NULL;

	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			BASE_WINDOW_SIGNAL_RUNTIME_INIT,
			G_CALLBACK( on_runtime_init_dialog ));
}

static void
instance_dispose( GObject *window )
{
	static const gchar *thisfn = "nact_assistant_import_instance_dispose";
	NactAssistantImport *self;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );
	g_return_if_fail( NACT_IS_ASSISTANT_IMPORT( window ));
	self = NACT_ASSISTANT_IMPORT( window );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( window );
		}
	}
}

static void
instance_finalize( GObject *window )
{
	static const gchar *thisfn = "nact_assistant_import_instance_finalize";
	NactAssistantImport *self;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );
	g_return_if_fail( NACT_IS_ASSISTANT_IMPORT( window ));
	self = NACT_ASSISTANT_IMPORT( window );

	free_results( self->private->results );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( window );
	}
}

static NactAssistantImport *
assist_new( BaseApplication *application )
{
	return( g_object_new( NACT_ASSISTANT_IMPORT_TYPE, BASE_WINDOW_PROP_APPLICATION, application, NULL ));
}

/**
 * nact_assistant_import_run:
 * @main: the #NactMainWindow parent window of this assistant.
 *
 * Run the assistant.
 */
void
nact_assistant_import_run( BaseWindow *main_window )
{
	BaseApplication *appli;
	NactAssistantImport *assist;

	appli = BASE_APPLICATION( base_window_get_application( main_window ));

	assist = assist_new( appli );
	g_object_set( G_OBJECT( assist ), BASE_WINDOW_PROP_PARENT, main_window, NULL );
	g_object_set( G_OBJECT( assist ), BASE_WINDOW_PROP_HAS_OWN_BUILDER, TRUE, NULL );

	base_window_run( BASE_WINDOW( assist ));

	/*g_object_unref( assist );*/
}

static gchar *
window_get_iprefs_window_id( BaseWindow *window )
{
	return( g_strdup( "import-assistant" ));
}

static gchar *
window_get_dialog_name( BaseWindow *dialog )
{
	return( g_strdup( "ImportAssistant" ));
}

static void
on_runtime_init_dialog( NactAssistantImport *dialog, gpointer user_data )
{
	static const gchar *thisfn = "nact_assistant_import_on_runtime_init_dialog";
	GtkAssistant *assistant;

	g_debug( "%s: dialog=%p, user_data=%p", thisfn, ( void * ) dialog, ( void * ) user_data );
	g_return_if_fail( NACT_IS_ASSISTANT_IMPORT( dialog ));

	if( !dialog->private->dispose_has_run ){

		base_assistant_set_cancel_on_esc( BASE_ASSISTANT( dialog ), TRUE );
		base_assistant_set_warn_on_esc( BASE_ASSISTANT( dialog ), TRUE );

		assistant = GTK_ASSISTANT( base_window_get_toplevel( BASE_WINDOW( dialog )));

		runtime_init_intro( dialog, assistant );
		runtime_init_file_selector( dialog, assistant );
		runtime_init_duplicates( dialog, assistant );
	}
}

static void
runtime_init_intro( NactAssistantImport *window, GtkAssistant *assistant )
{
	static const gchar *thisfn = "nact_assistant_import_runtime_init_intro";
	GtkWidget *page;

	page = gtk_assistant_get_nth_page( assistant, ASSIST_PAGE_INTRO );

	g_debug( "%s: window=%p, assistant=%p, page=%p",
			thisfn, ( void * ) window, ( void * ) assistant, ( void * ) page );

	gtk_assistant_set_page_complete( assistant, page, TRUE );
}

static void
runtime_init_file_selector( NactAssistantImport *window, GtkAssistant *assistant )
{
	static const gchar *thisfn = "nact_assistant_import_runtime_init_file_selector";
	GtkWidget *page;
	GtkWidget *chooser;
	gchar *uri;

	page = gtk_assistant_get_nth_page( assistant, ASSIST_PAGE_FILES_SELECTION );

	g_debug( "%s: window=%p, assistant=%p, page=%p",
			thisfn, ( void * ) window, ( void * ) assistant, ( void * ) page );

	chooser = base_window_get_widget( BASE_WINDOW( window ), "ImportFileChooser" );
	uri = base_iprefs_get_string( BASE_WINDOW( window ), IPREFS_IMPORT_ACTIONS_FOLDER_URI );
	if( uri && strlen( uri )){
		gtk_file_chooser_set_current_folder_uri( GTK_FILE_CHOOSER( chooser ), uri );
	}
	g_free( uri );

	base_window_signal_connect(
			BASE_WINDOW( window ),
			G_OBJECT( chooser ),
			"selection-changed",
			G_CALLBACK( on_file_selection_changed ));

	gtk_assistant_set_page_complete( assistant, page, FALSE );
}

static void
on_file_selection_changed( GtkFileChooser *chooser, gpointer user_data )
{
	/*static const gchar *thisfn = "nact_assistant_import_on_file_selection_changed";
	g_debug( "%s: chooser=%p, user_data=%p", thisfn, chooser, user_data );*/

	GtkAssistant *assistant;
	gint pos;
	GSList *uris;
	gboolean enabled;
	gchar *folder;
	GtkWidget *content;

	g_assert( NACT_IS_ASSISTANT_IMPORT( user_data ));
	assistant = GTK_ASSISTANT( base_window_get_toplevel( BASE_WINDOW( user_data )));
	pos = gtk_assistant_get_current_page( assistant );
	if( pos == ASSIST_PAGE_FILES_SELECTION ){

		uris = gtk_file_chooser_get_uris( chooser );
		enabled = has_readable_files( uris );

		if( enabled ){
			folder = gtk_file_chooser_get_current_folder_uri( GTK_FILE_CHOOSER( chooser ));
			base_iprefs_set_string( BASE_WINDOW( user_data ), IPREFS_IMPORT_ACTIONS_FOLDER_URI, folder );
			g_free( folder );
		}

		na_utils_free_string_list( uris );

		content = gtk_assistant_get_nth_page( assistant, pos );
		gtk_assistant_set_page_complete( assistant, content, enabled );
		gtk_assistant_update_buttons_state( assistant );
	}
}

static gboolean
has_readable_files( GSList *uris )
{
	static const gchar *thisfn = "nact_assistant_import_has_readable_files";
	GSList *iuri;
	int readables = 0;
	gchar *uri;
	GFile *file;
	GFileInfo *info;
	GFileType type;
	GError *error = NULL;
	gboolean readable;

	for( iuri = uris ; iuri ; iuri = iuri->next ){

		uri = ( gchar * ) iuri->data;
		if( !strlen( uri )){
			continue;
		}

		file = g_file_new_for_uri( uri );
		info = g_file_query_info( file,
				G_FILE_ATTRIBUTE_ACCESS_CAN_READ "," G_FILE_ATTRIBUTE_STANDARD_TYPE,
				G_FILE_QUERY_INFO_NONE, NULL, &error );

		if( error ){
			g_warning( "%s: g_file_query_info error: %s", thisfn, error->message );
			g_error_free( error );
			g_object_unref( file );
			continue;
		}

		type = g_file_info_get_file_type( info );
		if( type != G_FILE_TYPE_REGULAR ){
			g_warning( "%s: %s is not a file", thisfn, uri );
			g_object_unref( info );
			continue;
		}

		readable = g_file_info_get_attribute_boolean( info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ );
		if( !readable ){
			g_warning( "%s: %s is not readable", thisfn, uri );
			g_object_unref( info );
			continue;
		}

		readables += 1;
		g_object_unref( info );
	}

	return( readables > 0 );
}

static void
runtime_init_duplicates( NactAssistantImport *window, GtkAssistant *assistant )
{
	static const gchar *thisfn = "nact_assistant_import_runtime_init_duplicates";
	GtkWidget *page;
	gint mode;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );

	mode = base_iprefs_get_int( BASE_WINDOW( window ), IPREFS_IMPORT_ACTIONS_IMPORT_MODE );
	set_import_mode( window, mode );

	page = gtk_assistant_get_nth_page( assistant, ASSIST_PAGE_DUPLICATES );
	gtk_assistant_set_page_complete( assistant, page, TRUE );
}

static void
set_import_mode( NactAssistantImport *window, gint mode )
{
	GtkToggleButton *button;

	switch( mode ){
		case RENUMBER_MODE:
			button = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "RenumberButton" ));
			gtk_toggle_button_set_active( button, TRUE );
			break;

		case OVERRIDE_MODE:
			button = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "OverrideButton" ));
			gtk_toggle_button_set_active( button, TRUE );
			break;

		case NO_IMPORT_MODE:
		default:
			button = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "NoImportButton" ));
			gtk_toggle_button_set_active( button, TRUE );
			break;
	}
}

static void
assistant_prepare( BaseAssistant *window, GtkAssistant *assistant, GtkWidget *page )
{
	static const gchar *thisfn = "nact_assistant_import_assistant_prepare";
	GtkAssistantPageType type;

	g_debug( "%s: window=%p, assistant=%p, page=%p",
			thisfn, ( void * ) window, ( void * ) assistant, ( void * ) page );

	type = gtk_assistant_get_page_type( assistant, page );

	switch( type ){
		case GTK_ASSISTANT_PAGE_CONFIRM:
			prepare_confirm( NACT_ASSISTANT_IMPORT( window ), assistant, page );
			break;

		case GTK_ASSISTANT_PAGE_SUMMARY:
			prepare_importdone( NACT_ASSISTANT_IMPORT( window ), assistant, page );
			break;

		default:
			break;
	}
}

static void
prepare_confirm( NactAssistantImport *window, GtkAssistant *assistant, GtkWidget *page )
{
	static const gchar *thisfn = "nact_assistant_import_prepare_confirm";
	gchar *text, *tmp;
	GtkWidget *chooser;
	GSList *uris, *is;

	g_debug( "%s: window=%p, assistant=%p, page=%p",
			thisfn, ( void * ) window, ( void * ) assistant, ( void * ) page );

	/* i18n: the title of the confirm page of the import assistant */
	text = g_strdup( _( "About to import selected files:" ));
	tmp = g_strdup_printf( "<b>%s</b>\n\n", text );
	g_free( text );
	text = tmp;

	chooser = base_window_get_widget( BASE_WINDOW( window ), "ImportFileChooser" );
	uris = gtk_file_chooser_get_uris( GTK_FILE_CHOOSER( chooser ));

	for( is = uris ; is ; is = is->next ){
		tmp = g_strdup_printf( "%s\t%s\n", text, ( gchar * ) is->data );
		g_free( text );
		text = tmp;
	}

	tmp = add_import_mode( window, text );
	g_free( text );
	text = tmp;

	gtk_label_set_markup( GTK_LABEL( page ), text );
	g_free( text );

	gtk_assistant_set_page_complete( assistant, page, TRUE );
}

static gint
get_import_mode( NactAssistantImport *window )
{
	GtkToggleButton *no_import_button;
	GtkToggleButton *renumber_button;
	GtkToggleButton *override_button;
	gint mode;

	no_import_button = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "NoImportButton" ));
	renumber_button = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "RenumberButton" ));
	override_button = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "OverrideButton" ));

	if( gtk_toggle_button_get_active( no_import_button )){
		mode = NO_IMPORT_MODE;
	} else if( gtk_toggle_button_get_active( renumber_button )){
		mode = RENUMBER_MODE;
	} else {
		g_return_val_if_fail( gtk_toggle_button_get_active( override_button ), 0 );
		mode = OVERRIDE_MODE;
	}

	return( mode );
}

static gchar *
add_import_mode( NactAssistantImport *window, const gchar *text )
{
	gint mode;
	gchar *label1, *label2;
	gchar *result;

	mode = get_import_mode( window );
	label1 = NULL;
	label2 = NULL;
	result = NULL;

	switch( mode ){
		case NO_IMPORT_MODE:
			label1 = g_strdup( gtk_button_get_label( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "NoImportButton" ))));
			label2 = g_strdup( gtk_label_get_text( GTK_LABEL( base_window_get_widget( BASE_WINDOW( window ), "NoImportLabel"))));
			break;

		case RENUMBER_MODE:
			label1 = g_strdup( gtk_button_get_label( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "RenumberButton" ))));
			label2 = g_strdup( gtk_label_get_text( GTK_LABEL( base_window_get_widget( BASE_WINDOW( window ), "RenumberLabel"))));
			break;

		case OVERRIDE_MODE:
			label1 = g_strdup( gtk_button_get_label( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "OverrideButton" ))));
			label2 = g_strdup( gtk_label_get_text( GTK_LABEL( base_window_get_widget( BASE_WINDOW( window ), "OverrideLabel"))));
			break;

		default:
			break;
	}

	if( label1 ){
		result = g_strdup_printf( "%s\n\n<b>%s</b>\n\n%s", text, label1, label2 );
		g_free( label2 );
		g_free( label1 );
	}

	return( result );
}

/*
 * do import here
 */
static void
assistant_apply( BaseAssistant *wnd, GtkAssistant *assistant )
{
	static const gchar *thisfn = "nact_assistant_import_assistant_apply";
	NactAssistantImport *window;
	GtkWidget *chooser;
	GSList *uris, *is, *msg;
	NAObjectAction *action;
	ImportUriStruct *str;
	GList *items;
	BaseWindow *mainwnd;
	gint mode;

	g_debug( "%s: window=%p, assistant=%p", thisfn, ( void * ) wnd, ( void * ) assistant );
	g_assert( NACT_IS_ASSISTANT_IMPORT( wnd ));
	window = NACT_ASSISTANT_IMPORT( wnd );

	chooser = base_window_get_widget( BASE_WINDOW( window ), "ImportFileChooser" );
	uris = gtk_file_chooser_get_uris( GTK_FILE_CHOOSER( chooser ));
	mode = get_import_mode( window );

	g_object_get( G_OBJECT( wnd ), BASE_WINDOW_PROP_PARENT, &mainwnd, NULL );

	for( is = uris ; is ; is = is->next ){

		msg = NULL;
		action = nact_xml_reader_import( BASE_WINDOW( window ), ( const gchar * ) is->data, mode, &msg );

		str = g_new0( ImportUriStruct, 1 );
		str->uri = g_strdup(( const gchar * ) is->data );
		str->action = action;
		str->msg = na_utils_duplicate_string_list( msg );
		na_utils_free_string_list( msg );

		window->private->results = g_slist_prepend( window->private->results, str );

		items = g_list_prepend( NULL, action );
		nact_iactions_list_insert_items( NACT_IACTIONS_LIST( mainwnd ), items, NULL );
		na_object_free_items( items );
	}

	na_utils_free_string_list( uris );
}

static void
prepare_importdone( NactAssistantImport *window, GtkAssistant *assistant, GtkWidget *page )
{
	static const gchar *thisfn = "nact_assistant_import_prepare_importdone";
	gchar *text, *tmp, *text2;
	gchar *bname, *uuid, *label;
	GSList *is, *im;
	ImportUriStruct *str;
	GFile *file;
	gint mode;

	g_debug( "%s: window=%p, assistant=%p, page=%p",
			thisfn, ( void * ) window, ( void * ) assistant, ( void * ) page );

	/* i18n: result of the import assistant */
	text = g_strdup( _( "Selected files have been imported:" ));

	tmp = g_strdup_printf( "<b>%s</b>\n\n", text );
	g_free( text );
	text = tmp;

	for( is = window->private->results ; is ; is = is->next ){

		str = ( ImportUriStruct * ) is->data;

		file = g_file_new_for_uri( str->uri );
		bname = g_file_get_basename( file );
		g_object_unref( file );
		tmp = g_strdup_printf( "%s\t%s\n", text, bname );
		g_free( text );
		text = tmp;
		g_free( bname );

		if( str->action ){
			uuid = na_object_get_id( str->action );
			label = na_object_get_label( str->action );
			/* i18n: this is the globally unique identifier and the label of the newly imported action */
			text2 = g_strdup_printf( _( "UUID: %s\t%s" ), uuid, label);
			g_free( label );
			g_free( uuid );
			tmp = g_strdup_printf( "%s\t\t%s\n", text, text2 );

			window->private->actions = g_slist_prepend( window->private->actions, str->action );

		} else {
			/* i18n: just indicate that the import of this file was unsuccessfull */
			text2 = g_strdup( _( "NOT OK" ));
			tmp = g_strdup_printf( "%s\t\t %s\n", text, text2 );
			g_free( text2 );
		}

		g_free( text );
		text = tmp;

		/* add messages if any */
		for( im = str->msg ; im ; im = im->next ){
			tmp = g_strdup_printf( "%s\t\t%s\n", text, ( const char * ) im->data );
			g_free( text );
			text = tmp;
		}

		/* add a blank line between two actions */
		tmp = g_strdup_printf( "%s\n", text );
		g_free( text );
		text = tmp;
	}

	gtk_label_set_markup( GTK_LABEL( page ), text );
	g_free( text );

	mode = get_import_mode( window );
	base_iprefs_set_int( BASE_WINDOW( window ), IPREFS_IMPORT_ACTIONS_IMPORT_MODE, mode );

	gtk_assistant_set_page_complete( assistant, page, TRUE );
	base_assistant_set_warn_on_cancel( BASE_ASSISTANT( window ), FALSE );
}

static void
free_results( GSList *list )
{
	GSList *is;
	ImportUriStruct *str;

	for( is = list ; is ; is = is->next ){
		str = ( ImportUriStruct * ) is->data;
		g_free( str->uri );
		na_utils_free_string_list( str->msg );
	}

	g_slist_free( list );
}
