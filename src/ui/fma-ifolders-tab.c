/*
 * FileManager-Actions
 * A file-manager extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
 *
 * FileManager-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * FileManager-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FileManager-Actions; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
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

#include "api/fma-core-utils.h"
#include "api/fma-object-api.h"

#include "core/fma-gtk-utils.h"

#include "base-gtk-utils.h"
#include "fma-application.h"
#include "fma-main-tab.h"
#include "fma-main-window.h"
#include "nact-match-list.h"
#include "fma-ifolders-tab.h"

/* private interface data
 */
struct _FMAIFoldersTabInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* the identifier of this notebook page in the Match dialog
 */
#define ITAB_NAME						"folders"

static guint st_initializations = 0;	/* interface initialization count */

static GType   register_type( void );
static void    interface_base_init( FMAIFoldersTabInterface *klass );
static void    interface_base_finalize( FMAIFoldersTabInterface *klass );
static void    initialize_gtk( FMAIFoldersTab *instance );
static void    initialize_window( FMAIFoldersTab *instance );
static void    on_tree_selection_changed( NactTreeView *tview, GList *selected_items, FMAIFoldersTab *instance );
static void    on_browse_folder_clicked( GtkButton *button, FMAIFoldersTab *instance );
static GSList *get_folders( void *context );
static void    set_folders( void *context, GSList *filters );
static void    on_instance_finalized( gpointer user_data, FMAIFoldersTab *instance );

GType
fma_ifolders_tab_get_type( void )
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
	static const gchar *thisfn = "fma_ifolders_tab_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( FMAIFoldersTabInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "FMAIFoldersTab", &info, 0 );

	g_type_interface_add_prerequisite( type, GTK_TYPE_APPLICATION_WINDOW );

	return( type );
}

static void
interface_base_init( FMAIFoldersTabInterface *klass )
{
	static const gchar *thisfn = "fma_ifolders_tab_interface_base_init";

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( FMAIFoldersTabInterfacePrivate, 1 );
	}

	st_initializations += 1;
}

static void
interface_base_finalize( FMAIFoldersTabInterface *klass )
{
	static const gchar *thisfn = "fma_ifolders_tab_interface_base_finalize";

	st_initializations -= 1;

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );
	}
}

/**
 * fma_ifolders_tab_init:
 * @instance: this #FMAIFoldersTab instance.
 *
 * Initialize the interface
 * Connect to #BaseWindow signals
 */
void
fma_ifolders_tab_init( FMAIFoldersTab *instance )
{
	static const gchar *thisfn = "fma_ifolders_tab_init";

	g_return_if_fail( FMA_IS_IFOLDERS_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn,
			( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	fma_main_tab_init( FMA_MAIN_WINDOW( instance ), TAB_FOLDERS );
	initialize_gtk( instance );
	initialize_window( instance );

	g_object_weak_ref( G_OBJECT( instance ), ( GWeakNotify ) on_instance_finalized, NULL );
}

static void
initialize_gtk( FMAIFoldersTab *instance )
{
	static const gchar *thisfn = "fma_ifolders_tab_initialize_gtk";

	g_return_if_fail( FMA_IS_IFOLDERS_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	nact_match_list_init_with_args(
			FMA_MAIN_WINDOW( instance ),
			ITAB_NAME,
			TAB_FOLDERS,
			fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "FoldersTreeView" ),
			fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "AddFolderButton" ),
			fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "RemoveFolderButton" ),
			( pget_filters ) get_folders,
			( pset_filters ) set_folders,
			NULL,
			NULL,
			MATCH_LIST_MUST_MATCH_ONE_OF,
			_( "Folder filter" ),
			TRUE );
}

static void
initialize_window( FMAIFoldersTab *instance )
{
	static const gchar *thisfn = "fma_ifolders_tab_initialize_window";
	NactTreeView *tview;

	g_return_if_fail( FMA_IS_IFOLDERS_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	tview = fma_main_window_get_items_view( FMA_MAIN_WINDOW( instance ));

	g_signal_connect(
			tview, TREE_SIGNAL_SELECTION_CHANGED,
			G_CALLBACK( on_tree_selection_changed ), instance );

	fma_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "FolderBrowseButton",
			"clicked", G_CALLBACK( on_browse_folder_clicked ), instance );
}

static void
on_tree_selection_changed( NactTreeView *tview, GList *selected_items, FMAIFoldersTab *instance )
{
	FMAIContext *context;
	gboolean editable;
	gboolean enable_tab;
	GtkWidget *button;

	g_object_get( G_OBJECT( instance ),
			MAIN_PROP_CONTEXT, &context, MAIN_PROP_EDITABLE, &editable,
			NULL );

	enable_tab = ( context != NULL );
	fma_main_tab_enable_page( FMA_MAIN_WINDOW( instance ), TAB_FOLDERS, enable_tab );

	button = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "FolderBrowseButton" );
	base_gtk_utils_set_editable( G_OBJECT( button ), editable );
}

static void
on_browse_folder_clicked( GtkButton *button, FMAIFoldersTab *instance )
{
	gchar *uri, *path;
	GtkWidget *dialog;

	uri = NULL;

	/* i18n: title of the FileChoose dialog when selecting an URI which
	 * will be compare to Nautilus 'current_folder'
	 */
	dialog = gtk_file_chooser_dialog_new( _( "Select a folder" ),
			GTK_WINDOW( instance ),
			GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
			_( "_Cancel" ), GTK_RESPONSE_CANCEL,
			_( "_Open" ), GTK_RESPONSE_ACCEPT,
			NULL );

	fma_gtk_utils_restore_window_position( GTK_WINDOW( dialog ), IPREFS_FOLDER_CHOOSER_WSP );

	uri = fma_settings_get_string( IPREFS_FOLDER_CHOOSER_URI, NULL, NULL );
	if( uri && g_utf8_strlen( uri, -1 )){
		gtk_file_chooser_set_current_folder_uri( GTK_FILE_CHOOSER( dialog ), uri );
	}
	g_free( uri );

	if( gtk_dialog_run( GTK_DIALOG( dialog )) == GTK_RESPONSE_ACCEPT ){
		uri = gtk_file_chooser_get_current_folder_uri( GTK_FILE_CHOOSER( dialog ));
		fma_settings_set_string( IPREFS_FOLDER_CHOOSER_URI, uri );
		path = g_filename_from_uri( uri, NULL, NULL );
		nact_match_list_insert_row( FMA_MAIN_WINDOW( instance ), ITAB_NAME, path, FALSE, FALSE );
		g_free( path );
		g_free( uri );
	}

	fma_gtk_utils_restore_window_position( GTK_WINDOW( dialog ), IPREFS_FOLDER_CHOOSER_WSP );

	gtk_widget_destroy( dialog );
}

static GSList *
get_folders( void *context )
{
	return( fma_object_get_folders( context ));
}

static void
set_folders( void *context, GSList *filters )
{
	fma_object_set_folders( context, filters );
}

static void
on_instance_finalized( gpointer user_data, FMAIFoldersTab *instance )
{
	static const gchar *thisfn = "fma_ifolders_tab_on_instance_finalized";

	g_debug( "%s: instance=%p, user_data=%p", thisfn, ( void * ) instance, ( void * ) user_data );
}
