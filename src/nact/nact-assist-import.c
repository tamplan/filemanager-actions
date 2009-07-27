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
#include <common/na-iio-provider.h>
#include <common/na-utils.h>

#include "base-application.h"
#include "nact-assist-import.h"
#include "nact-gconf-reader.h"
#include "nact-iprefs.h"

/* Import Assistant
 *
 * pos.  type     title
 * ---   -------  ------------------------------------
 *   0   Intro    Introduction
 *   1   Content  Selection of the files
 *   2   Confirm  Display the selected files before import
 *   3   Summary  Import is done: summary of the done operations
 */

enum {
	ASSIST_PAGE_INTRO = 0,
	ASSIST_PAGE_FILES_SELECTION,
	ASSIST_PAGE_CONFIRM,
	ASSIST_PAGE_DONE
};

typedef struct {
	gchar    *uri;
	NAAction *action;
	GSList   *msg;
}
	ImportUriStruct;

/* private class data
 */
struct NactAssistImportClassPrivate {
};

/* private instance data
 */
struct NactAssistImportPrivate {
	gboolean  dispose_has_run;
	GSList   *results;
	GSList   *actions;
};

static GObjectClass *st_parent_class = NULL;

static GType             register_type( void );
static void              class_init( NactAssistImportClass *klass );
static void              instance_init( GTypeInstance *instance, gpointer klass );
static void              instance_dispose( GObject *application );
static void              instance_finalize( GObject *application );

static NactAssistImport *assist_new( BaseApplication *application );

static gchar            *do_get_iprefs_window_id( NactWindow *window );
static gchar            *do_get_dialog_name( BaseWindow *dialog );
static void              on_runtime_init_dialog( BaseWindow *dialog );
static void              runtime_init_intro( NactAssistImport *window, GtkAssistant *assistant );
static void              runtime_init_file_selector( NactAssistImport *window, GtkAssistant *assistant );
static void              on_file_selection_changed( GtkFileChooser *chooser, gpointer user_data );
static gboolean          has_readable_files( GSList *uris );
static void              on_prepare( NactAssistant *window, GtkAssistant *assistant, GtkWidget *page );
static void              prepare_confirm( NactAssistImport *window, GtkAssistant *assistant, GtkWidget *page );
static void              prepare_importdone( NactAssistImport *window, GtkAssistant *assistant, GtkWidget *page );
static void              do_import( NactAssistImport *window, GtkAssistant *assistant );
static void              free_results( GSList *list );

GType
nact_assist_import_get_type( void )
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
	static const gchar *thisfn = "nact_assist_import_register_type";
	g_debug( "%s", thisfn );

	static GTypeInfo info = {
		sizeof( NactAssistImportClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactAssistImport ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	GType type = g_type_register_static( NACT_ASSISTANT_TYPE, "NactAssistImport", &info, 0 );

	return( type );
}

static void
class_init( NactAssistImportClass *klass )
{
	static const gchar *thisfn = "nact_assist_import_class_init";
	g_debug( "%s: klass=%p", thisfn, klass );

	st_parent_class = g_type_class_peek_parent( klass );

	GObjectClass *object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactAssistImportClassPrivate, 1 );

	BaseWindowClass *base_class = BASE_WINDOW_CLASS( klass );
	base_class->get_toplevel_name = do_get_dialog_name;
	base_class->runtime_init_toplevel = on_runtime_init_dialog;

	NactWindowClass *nact_class = NACT_WINDOW_CLASS( klass );
	nact_class->get_iprefs_window_id = do_get_iprefs_window_id;

	NactAssistantClass *assist_class = NACT_ASSISTANT_CLASS( klass );
	assist_class->on_assistant_prepare = on_prepare;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_assist_import_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );

	g_assert( NACT_IS_ASSIST_IMPORT( instance ));
	NactAssistImport *self = NACT_ASSIST_IMPORT( instance );

	self->private = g_new0( NactAssistImportPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->results = NULL;
}

static void
instance_dispose( GObject *window )
{
	static const gchar *thisfn = "nact_assist_import_instance_dispose";
	g_debug( "%s: window=%p", thisfn, window );

	g_assert( NACT_IS_ASSIST_IMPORT( window ));
	NactAssistImport *self = NACT_ASSIST_IMPORT( window );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		G_OBJECT_CLASS( st_parent_class )->dispose( window );
	}
}

static void
instance_finalize( GObject *window )
{
	static const gchar *thisfn = "nact_assist_import_instance_finalize";
	g_debug( "%s: window=%p", thisfn, window );

	g_assert( NACT_IS_ASSIST_IMPORT( window ));
	NactAssistImport *self = ( NactAssistImport * ) window;

	free_results( self->private->results );

	g_free( self->private );

	/* chain call to parent class */
	if( st_parent_class->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( window );
	}
}

static NactAssistImport *
assist_new( BaseApplication *application )
{
	return( g_object_new( NACT_ASSIST_IMPORT_TYPE, PROP_WINDOW_APPLICATION_STR, application, NULL ));
}

/**
 * Run the assistant.
 *
 * @main: the main window of the application.
 */
GSList *
nact_assist_import_run( NactWindow *main_window )
{
	BaseApplication *appli = BASE_APPLICATION( base_window_get_application( BASE_WINDOW( main_window )));

	NactAssistImport *assist = assist_new( appli );

	g_object_set( G_OBJECT( assist ), PROP_WINDOW_PARENT_STR, main_window, NULL );

	base_window_run( BASE_WINDOW( assist ));

	return( assist->private->actions );
}

static gchar *
do_get_iprefs_window_id( NactWindow *window )
{
	return( g_strdup( "import-assistant" ));
}

static gchar *
do_get_dialog_name( BaseWindow *dialog )
{
	return( g_strdup( "ImportAssistant" ));
}

static void
on_runtime_init_dialog( BaseWindow *dialog )
{
	static const gchar *thisfn = "nact_assist_import_on_runtime_init_dialog";

	/* call parent class at the very beginning */
	if( BASE_WINDOW_CLASS( st_parent_class )->runtime_init_toplevel ){
		BASE_WINDOW_CLASS( st_parent_class )->runtime_init_toplevel( dialog );
	}

	g_debug( "%s: dialog=%p", thisfn, dialog );
	g_assert( NACT_IS_ASSIST_IMPORT( dialog ));
	NactAssistImport *window = NACT_ASSIST_IMPORT( dialog );

	GtkAssistant *assistant = GTK_ASSISTANT( base_window_get_toplevel_dialog( dialog ));

	runtime_init_intro( window, assistant );
	runtime_init_file_selector( window, assistant );
}

static void
runtime_init_intro( NactAssistImport *window, GtkAssistant *assistant )
{
	static const gchar *thisfn = "nact_assist_import_runtime_init_intro";

	GtkWidget *content = gtk_assistant_get_nth_page( assistant, ASSIST_PAGE_INTRO );

	g_debug( "%s: window=%p, assistant=%p, content=%p", thisfn, window, assistant, content );

	gtk_assistant_set_page_complete( assistant, content, TRUE );
}

static void
runtime_init_file_selector( NactAssistImport *window, GtkAssistant *assistant )
{
	static const gchar *thisfn = "nact_assist_import_runtime_init_file_selector";

	GtkWidget *chooser = gtk_assistant_get_nth_page( assistant, ASSIST_PAGE_FILES_SELECTION );

	g_debug( "%s: window=%p, assistant=%p, chooser=%p", thisfn, window, assistant, chooser );

	gchar *uri = nact_iprefs_get_import_folder_uri( NACT_WINDOW( window ));
	if( uri && strlen( uri )){
		gtk_file_chooser_set_current_folder_uri( GTK_FILE_CHOOSER( chooser ), uri );
	}
	g_free( uri );

	nact_window_signal_connect( NACT_WINDOW( window ), G_OBJECT( chooser ), "selection-changed", G_CALLBACK( on_file_selection_changed ));

	gtk_assistant_set_page_complete( assistant, chooser, FALSE );
}

static void
on_file_selection_changed( GtkFileChooser *chooser, gpointer user_data )
{
	/*static const gchar *thisfn = "nact_assist_import_on_file_selection_changed";
	g_debug( "%s: chooser=%p, user_data=%p", thisfn, chooser, user_data );*/

	g_assert( NACT_IS_ASSIST_IMPORT( user_data ));
	GtkAssistant *assistant = GTK_ASSISTANT( base_window_get_toplevel_dialog( BASE_WINDOW( user_data )));
	gint pos = gtk_assistant_get_current_page( assistant );
	if( pos == ASSIST_PAGE_FILES_SELECTION ){

		GSList *uris = gtk_file_chooser_get_uris( chooser );
		gboolean enabled = has_readable_files( uris );

		if( enabled ){
			gchar *folder = gtk_file_chooser_get_current_folder_uri( GTK_FILE_CHOOSER( chooser ));
			nact_iprefs_save_import_folder_uri( NACT_WINDOW( user_data ), folder );
			g_free( folder );
		}

		na_utils_free_string_list( uris );

		GtkWidget *content = gtk_assistant_get_nth_page( assistant, pos );
		gtk_assistant_set_page_complete( assistant, content, enabled );
		gtk_assistant_update_buttons_state( assistant );
	}
}

static gboolean
has_readable_files( GSList *uris )
{
	static const gchar *thisfn = "nact_assist_import_has_readable_files";

	GSList *iuri;
	int readables = 0;

	for( iuri = uris ; iuri ; iuri = iuri->next ){

		gchar *uri = ( gchar * ) iuri->data;
		if( !strlen( uri )){
			continue;
		}

		GFile *file = g_file_new_for_uri( uri );
		GError *error = NULL;
		GFileInfo *info = g_file_query_info( file,
				G_FILE_ATTRIBUTE_ACCESS_CAN_READ "," G_FILE_ATTRIBUTE_STANDARD_TYPE,
				G_FILE_QUERY_INFO_NONE, NULL, &error );

		if( error ){
			g_warning( "%s: g_file_query_info error: %s", thisfn, error->message );
			g_error_free( error );
			g_object_unref( file );
			continue;
		}

		GFileType type = g_file_info_get_file_type( info );
		if( type != G_FILE_TYPE_REGULAR ){
			g_warning( "%s: %s is not a file", thisfn, uri );
			g_object_unref( info );
			continue;
		}

		gboolean readable = g_file_info_get_attribute_boolean( info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ );
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
on_prepare( NactAssistant *window, GtkAssistant *assistant, GtkWidget *page )
{
	static const gchar *thisfn = "nact_assist_import_on_prepare";
	g_debug( "%s: window=%p, assistant=%p, page=%p", thisfn, window, assistant, page );

	GtkAssistantPageType type = gtk_assistant_get_page_type( assistant, page );

	switch( type ){
		case GTK_ASSISTANT_PAGE_CONFIRM:
			prepare_confirm( NACT_ASSIST_IMPORT( window ), assistant, page );
			break;

		case GTK_ASSISTANT_PAGE_SUMMARY:
			prepare_importdone( NACT_ASSIST_IMPORT( window ), assistant, page );
			break;

		default:
			break;
	}
}

static void
prepare_confirm( NactAssistImport *window, GtkAssistant *assistant, GtkWidget *page )
{
	static const gchar *thisfn = "nact_assist_import_prepare_confirm";
	g_debug( "%s: window=%p, assistant=%p, page=%p", thisfn, window, assistant, page );

	gchar *text = g_strdup( _( "<b>About to import selected files :</b>\n\n" ));
	gchar *tmp;

	GtkWidget *chooser = gtk_assistant_get_nth_page( assistant, ASSIST_PAGE_FILES_SELECTION );
	GSList *uris = gtk_file_chooser_get_uris( GTK_FILE_CHOOSER( chooser ));
	GSList *is;

	for( is = uris ; is ; is = is->next ){
		tmp = g_strdup_printf( "%s\t%s\n", text, ( gchar * ) is->data );
		g_free( text );
		text = tmp;
	}

	gtk_label_set_markup( GTK_LABEL( page ), text );
	g_free( text );

	gtk_assistant_set_page_complete( assistant, page, TRUE );
}

static void
prepare_importdone( NactAssistImport *window, GtkAssistant *assistant, GtkWidget *page )
{
	static const gchar *thisfn = "nact_assist_import_prepare_importdone";
	g_debug( "%s: window=%p, assistant=%p, page=%p", thisfn, window, assistant, page );

	do_import( window, assistant );

	gchar *text, *tmp;
	GSList *is;

	text = g_strdup( _( "<b>Selected files have been imported :</b>\n\n" ));

	for( is = window->private->results ; is ; is = is->next ){

		ImportUriStruct *str = ( ImportUriStruct * ) is->data;

		GFile *file = g_file_new_for_uri( str->uri );
		gchar *bname = g_file_get_basename( file );
		g_object_unref( file );
		tmp = g_strdup_printf( _( "%s\t%s\n\n" ), text, bname );
		g_free( text );
		text = tmp;
		g_free( bname );

		if( str->action ){
			gchar *uuid = na_action_get_uuid( str->action );
			gchar *label = na_action_get_label( str->action );
			tmp = g_strdup_printf( _( "%s\t\tUUID: %s\t%s\n\n" ), text, uuid, label );
			g_free( label );
			g_free( uuid );

			window->private->actions = g_slist_prepend( window->private->actions, str->action );

		} else {
			tmp = g_strdup_printf( "%s\t\t NOT OK\n\n", text );
		}

		g_free( text );
		text = tmp;

		GSList *im;
		for( im = str->msg ; im ; im = im->next ){
			tmp = g_strdup_printf( "%s\t\t%s\n", text, ( const char * ) im->data );
			g_free( text );
			text = tmp;
		}

		tmp = g_strdup_printf( "%s\n", text );
		g_free( text );
		text = tmp;
	}

	gtk_label_set_markup( GTK_LABEL( page ), text );
	g_free( text );

	gtk_assistant_set_page_complete( assistant, page, TRUE );
	nact_assistant_set_warn_on_cancel( NACT_ASSISTANT( window ), FALSE );
}

static void
do_import( NactAssistImport *window, GtkAssistant *assistant )
{
	static const gchar *thisfn = "nact_assist_import_do_import";
	g_debug( "%s: window=%p", thisfn, window );

	GtkWidget *chooser = gtk_assistant_get_nth_page( assistant, ASSIST_PAGE_FILES_SELECTION );
	GSList *uris = gtk_file_chooser_get_uris( GTK_FILE_CHOOSER( chooser ));
	GSList *is, *msg;

	for( is = uris ; is ; is = is->next ){

		msg = NULL;
		NAAction *action = nact_gconf_reader_import( NACT_WINDOW( window ), ( const gchar * ) is->data, &msg );

		ImportUriStruct *str = g_new0( ImportUriStruct, 1 );
		str->uri = g_strdup(( const gchar * ) is->data );
		str->action = action;
		str->msg = na_utils_duplicate_string_list( msg );

		window->private->results = g_slist_prepend( window->private->results, str );

		na_utils_free_string_list( msg );
	}

	na_utils_free_string_list( uris );
}

static void
free_results( GSList *list )
{
	GSList *is;
	for( is = list ; is ; is = is->next ){
		ImportUriStruct *str = ( ImportUriStruct * ) is->data;
		g_free( str->uri );
		na_utils_free_string_list( str->msg );
	}

	g_slist_free( list );
}
