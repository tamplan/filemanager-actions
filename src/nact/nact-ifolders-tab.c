/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011, 2012 Pierre Wieser and others (see AUTHORS)
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

#include <core/na-iprefs.h>

#include "base-window.h"
#include "base-gtk-utils.h"
#include "nact-application.h"
#include "nact-main-tab.h"
#include "nact-match-list.h"
#include "nact-ifolders-tab.h"

/* private interface data
 */
struct _NactIFoldersTabInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

#define ITAB_NAME						"folders"

static guint st_initializations = 0;	/* interface initialization count */

static GType   register_type( void );
static void    interface_base_init( NactIFoldersTabInterface *klass );
static void    interface_base_finalize( NactIFoldersTabInterface *klass );

static void    on_main_selection_changed( NactIFoldersTab *instance, GList *selected_items, gpointer user_data );

static void    on_browse_folder_clicked( GtkButton *button, BaseWindow *window );
static GSList *get_folders( void *context );
static void    set_folders( void *context, GSList *filters );

GType
nact_ifolders_tab_get_type( void )
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
	static const gchar *thisfn = "nact_ifolders_tab_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NactIFoldersTabInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "NactIFoldersTab", &info, 0 );

	g_type_interface_add_prerequisite( type, BASE_WINDOW_TYPE );

	return( type );
}

static void
interface_base_init( NactIFoldersTabInterface *klass )
{
	static const gchar *thisfn = "nact_ifolders_tab_interface_base_init";

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NactIFoldersTabInterfacePrivate, 1 );
	}

	st_initializations += 1;
}

static void
interface_base_finalize( NactIFoldersTabInterface *klass )
{
	static const gchar *thisfn = "nact_ifolders_tab_interface_base_finalize";

	st_initializations -= 1;

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );
	}
}

void
nact_ifolders_tab_initial_load_toplevel( NactIFoldersTab *instance )
{
	static const gchar *thisfn = "nact_ifolders_tab_initial_load_toplevel";
	GtkWidget *list, *add, *remove;

	g_return_if_fail( NACT_IS_IFOLDERS_TAB( instance ));

	g_debug( "%s: instance=%p (%s)", thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	list = base_window_get_widget( BASE_WINDOW( instance ), "FoldersTreeView" );
	add = base_window_get_widget( BASE_WINDOW( instance ), "AddFolderButton" );
	remove = base_window_get_widget( BASE_WINDOW( instance ), "RemoveFolderButton" );

	nact_match_list_create_model(
			BASE_WINDOW( instance ),
			ITAB_NAME,
			TAB_FOLDERS,
			list, add, remove,
			( pget_filters ) get_folders,
			( pset_filters ) set_folders,
			NULL,
			MATCH_LIST_MUST_MATCH_ONE_OF,
			_( "Folder filter" ), TRUE );
}

void
nact_ifolders_tab_runtime_init_toplevel( NactIFoldersTab *instance )
{
	static const gchar *thisfn = "nact_ifolders_tab_runtime_init_toplevel";
	GtkWidget *button;

	g_return_if_fail( NACT_IS_IFOLDERS_TAB( instance ));

	g_debug( "%s: instance=%p (%s)", thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			MAIN_SIGNAL_SELECTION_CHANGED,
			G_CALLBACK( on_main_selection_changed ));

	nact_match_list_init_view( BASE_WINDOW( instance ), ITAB_NAME );

	button = base_window_get_widget( BASE_WINDOW( instance ), "FolderBrowseButton" );
	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( button ),
			"clicked",
			G_CALLBACK( on_browse_folder_clicked ));
}

void
nact_ifolders_tab_all_widgets_showed( NactIFoldersTab *instance )
{
	static const gchar *thisfn = "nact_ifolders_tab_all_widgets_showed";

	g_return_if_fail( NACT_IS_IFOLDERS_TAB( instance ));

	g_debug( "%s: instance=%p (%s)", thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));
}

void
nact_ifolders_tab_dispose( NactIFoldersTab *instance )
{
	static const gchar *thisfn = "nact_ifolders_tab_dispose";

	g_return_if_fail( NACT_IS_IFOLDERS_TAB( instance ));

	g_debug( "%s: instance=%p (%s)", thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	nact_match_list_dispose( BASE_WINDOW( instance ), ITAB_NAME );
}

static void
on_main_selection_changed( NactIFoldersTab *instance, GList *selected_items, gpointer user_data )
{
	guint count_selected;
	NAIContext *context;
	gboolean editable;
	GtkWidget *button;

	count_selected = g_list_length( selected_items );
	nact_match_list_on_selection_changed( BASE_WINDOW( instance ), ITAB_NAME, count_selected );

	g_object_get( G_OBJECT( instance ),
			MAIN_PROP_CONTEXT, &context, MAIN_PROP_EDITABLE, &editable,
			NULL );

	button = base_window_get_widget( BASE_WINDOW( instance ), "FolderBrowseButton" );
	base_gtk_utils_set_editable( G_OBJECT( button ), editable );
}

static void
on_browse_folder_clicked( GtkButton *button, BaseWindow *window )
{
#if 0
	/* this is the code I sent to gtk-app-devel list
	 * to know why one is not able to just enter '/' in the location entry
	 */
	GtkWidget *dialog;
	gchar *path;

	dialog = gtk_file_chooser_dialog_new( _( "Select a folder" ),
			NULL,
			GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			NULL );

	gtk_file_chooser_set_filename( GTK_FILE_CHOOSER( dialog ), "/" );

	if( gtk_dialog_run( GTK_DIALOG( dialog )) == GTK_RESPONSE_ACCEPT ){
		path = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER( dialog ));
		g_debug( "nact_ifolders_tab_on_add_folder_clicked: path=%s", path );
		g_free( path );
	}

	gtk_widget_destroy( dialog );
#endif

	gchar *uri, *path;
	GtkWindow *toplevel;
	GtkWidget *dialog;

	uri = NULL;
	toplevel = base_window_get_gtk_toplevel( window );

	/* i18n: title of the FileChoose dialog when selecting an URI which
	 * will be compare to Nautilus 'current_folder'
	 */
	dialog = gtk_file_chooser_dialog_new( _( "Select a folder" ),
			toplevel,
			GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			NULL );

	base_gtk_utils_restore_window_position( window, NA_IPREFS_FOLDER_CHOOSER_WSP );

	uri = na_settings_get_string( NA_IPREFS_FOLDER_CHOOSER_URI, NULL, NULL );
	if( uri && g_utf8_strlen( uri, -1 )){
		gtk_file_chooser_set_current_folder_uri( GTK_FILE_CHOOSER( dialog ), uri );
	}
	g_free( uri );

	if( gtk_dialog_run( GTK_DIALOG( dialog )) == GTK_RESPONSE_ACCEPT ){
		uri = gtk_file_chooser_get_current_folder_uri( GTK_FILE_CHOOSER( dialog ));
		na_settings_set_string( NA_IPREFS_FOLDER_CHOOSER_URI, uri );

		path = g_filename_from_uri( uri, NULL, NULL );
		nact_match_list_insert_row( window, ITAB_NAME, path, FALSE, FALSE );
		g_free( path );

		g_free( uri );
	}

	base_gtk_utils_save_window_position( window, NA_IPREFS_FOLDER_CHOOSER_WSP );

	gtk_widget_destroy( dialog );
}

static GSList *
get_folders( void *context )
{
	return( na_object_get_folders( context ));
}

static void
set_folders( void *context, GSList *filters )
{
	na_object_set_folders( context, filters );
}
