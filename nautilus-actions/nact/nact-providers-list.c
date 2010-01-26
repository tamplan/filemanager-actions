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

#include <gconf/gconf-client.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>

#include <api/na-object-api.h>

#include <runtime/na-io-provider.h>
#include <runtime/na-iprefs.h>
#include <runtime/na-utils.h>

#include "nact-application.h"
#include "nact-gtk-utils.h"
#include "nact-providers-list.h"

/* column ordering
 */
enum {
	PROVIDERS_READABLE_COLUMN = 0,
	PROVIDERS_WRITABLE_COLUMN,
	PROVIDERS_LIBELLE_COLUMN,
	PROVIDERS_N_COLUMN
};

#define PROVIDERS_LIST_TREEVIEW			"nact-providers-list-treeview"

static gboolean st_on_selection_change = FALSE;

static void       init_view_setup_defaults( GtkTreeView *treeview, BaseWindow *window );
static void       init_view_connect_signals( GtkTreeView *treeview, BaseWindow *window );
static void       init_view_select_first_row( GtkTreeView *treeview );

/*static gboolean   iter_for_reset( GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data );*/
/*static void       iter_for_setup( gchar *scheme, GtkTreeModel *model );
static gboolean   iter_for_get( GtkTreeModel* scheme_model, GtkTreePath *path, GtkTreeIter* iter, GSList **schemes_list );*/
static GList     *get_list_providers( GtkTreeView *treeview );
static gboolean   get_list_providers_iter( GtkTreeModel *model, GtkTreePath *path, GtkTreeIter* iter, GList **list );

static void       on_selection_changed( GtkTreeSelection *selection, BaseWindow *window );
static void       on_readable_toggled( GtkCellRendererToggle *renderer, gchar *path, BaseWindow *window );
static void       on_writable_toggled( GtkCellRendererToggle *renderer, gchar *path, BaseWindow *window );
static void       on_up_clicked( GtkButton *button, BaseWindow *window );
static void       on_down_clicked( GtkButton *button, BaseWindow *window );

static GtkButton *get_up_button( BaseWindow *window );
static GtkButton *get_down_button( BaseWindow *window );
/*static GSList    *get_gconf_subdirs( GConfClient *gconf, const gchar *path );
static void       free_gconf_subdirs( GSList *subdirs );
static void       free_gslist( GSList *list );*/

/**
 * nact_providers_list_create_providers_list:
 * @treeview: the #GtkTreeView.
 *
 * Create the treeview model when initially loading the widget from
 * the UI manager.
 */
void
nact_providers_list_create_model( GtkTreeView *treeview )
{
	static const char *thisfn = "nact_providers_list_create_model";
	GtkListStore *model;
	GtkCellRenderer *toggled_cell;
	GtkTreeViewColumn *column;
	GtkCellRenderer *text_cell;
	GtkTreeSelection *selection;

	g_debug( "%s: treeview=%p", thisfn, ( void * ) treeview );
	g_return_if_fail( GTK_IS_TREE_VIEW( treeview ));

	model = gtk_list_store_new( PROVIDERS_N_COLUMN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_STRING );
	gtk_tree_view_set_model( treeview, GTK_TREE_MODEL( model ));
	g_object_unref( model );

	toggled_cell = gtk_cell_renderer_toggle_new();
	column = gtk_tree_view_column_new_with_attributes(
			_( "To be read" ),
			toggled_cell,
			"active", PROVIDERS_READABLE_COLUMN,
			NULL );
	gtk_tree_view_append_column( treeview, column );

	toggled_cell = gtk_cell_renderer_toggle_new();
	column = gtk_tree_view_column_new_with_attributes(
			_( "Writable" ),
			toggled_cell,
			"active", PROVIDERS_WRITABLE_COLUMN,
			NULL );
	gtk_tree_view_append_column( treeview, column );

	text_cell = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
			_( "I/O Provider" ),
			text_cell,
			"text", PROVIDERS_LIBELLE_COLUMN,
			NULL );
	gtk_tree_view_append_column( treeview, column );

	gtk_tree_view_set_headers_visible( treeview, TRUE );

	selection = gtk_tree_view_get_selection( treeview );
	gtk_tree_selection_set_mode( selection, GTK_SELECTION_BROWSE );
}

/**
 * nact_providers_list_init_view:
 * @treeview: the #GtkTreeView.
 * @window: the parent #BaseWindow which embeds the view.
 *
 * Connects signals at runtime initialization of the widget, and setup
 * current default values.
 */
void
nact_providers_list_init_view( GtkTreeView *treeview, BaseWindow *window )
{
	static const gchar *thisfn = "nact_providers_list_init_view";

	g_debug( "%s: treeview=%p, window=%p", thisfn, ( void * ) treeview, ( void * ) window );
	g_return_if_fail( BASE_IS_WINDOW( window ));
	g_return_if_fail( GTK_IS_TREE_VIEW( treeview ));

	g_object_set_data( G_OBJECT( window ), PROVIDERS_LIST_TREEVIEW, treeview );

	init_view_setup_defaults( treeview, window );
	init_view_connect_signals( treeview, window );

	init_view_select_first_row( treeview );
}

static void
init_view_setup_defaults( GtkTreeView *treeview, BaseWindow *window )
{
	NactApplication *application;
	NAPivot *pivot;
	GtkListStore *model;
	GList *providers, *iter;
	GtkTreeIter row;
	gchar *libelle;

	model = GTK_LIST_STORE( gtk_tree_view_get_model( treeview ));

	application = NACT_APPLICATION( base_window_get_application( window ));
	pivot = nact_application_get_pivot( application );
	providers = na_io_provider_get_providers_list( pivot );

	for( iter = providers ; iter ; iter = iter->next ){

		gtk_list_store_append( model, &row );
		libelle = na_io_provider_get_name( NA_IO_PROVIDER( iter->data ));
		gtk_list_store_set( model, &row,
				PROVIDERS_READABLE_COLUMN, na_io_provider_is_to_be_read( NA_IO_PROVIDER( iter->data )),
				PROVIDERS_WRITABLE_COLUMN, na_io_provider_is_writable( NA_IO_PROVIDER( iter->data )),
				PROVIDERS_LIBELLE_COLUMN, libelle,
				-1 );
		g_free( libelle );
	}
}

static void
init_view_connect_signals( GtkTreeView *treeview, BaseWindow *window )
{
	GtkTreeViewColumn *column;
	GList *renderers;
	GtkButton *up_button, *down_button;

	column = gtk_tree_view_get_column( treeview, PROVIDERS_READABLE_COLUMN );
	renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( column ));
	base_window_signal_connect(
			window,
			G_OBJECT( renderers->data ),
			"toggled",
			G_CALLBACK( on_readable_toggled ));

	column = gtk_tree_view_get_column( treeview, PROVIDERS_WRITABLE_COLUMN );
	renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( column ));
	base_window_signal_connect(
			window,
			G_OBJECT( renderers->data ),
			"toggled",
			G_CALLBACK( on_writable_toggled ));

	up_button = get_up_button( window );
	base_window_signal_connect(
			window,
			G_OBJECT( up_button ),
			"clicked",
			G_CALLBACK( on_up_clicked ));

	down_button = get_down_button( window );
	base_window_signal_connect(
			window,
			G_OBJECT( down_button ),
			"clicked",
			G_CALLBACK( on_down_clicked ));

	base_window_signal_connect(
			window,
			G_OBJECT( gtk_tree_view_get_selection( treeview )),
			"changed",
			G_CALLBACK( on_selection_changed ));
}

static void
init_view_select_first_row( GtkTreeView *treeview )
{
	GtkTreeSelection *selection;
	GtkTreePath *path;

	path = gtk_tree_path_new_first();
	selection = gtk_tree_view_get_selection( treeview );
	gtk_tree_selection_select_path( selection, path );
	gtk_tree_path_free( path );
}

/**
 * nact_providers_list_save:
 * @window: the #BaseWindow which embeds this treeview.
 *
 * Save the I/O provider status as a GConf preference.
 */
void
nact_providers_list_save( BaseWindow *window )
{
	GtkTreeView *treeview;
	GList *providers;
	NactApplication *application;
	NAPivot *pivot;

	treeview = GTK_TREE_VIEW( g_object_get_data( G_OBJECT( window ), PROVIDERS_LIST_TREEVIEW ));
	providers = get_list_providers( treeview );
	application = NACT_APPLICATION( base_window_get_application( window ));
	pivot = nact_application_get_pivot( application );

	/*na_iprefs_write_string_list( NA_IPREFS( pivot ), "schemes", schemes );

	na_utils_free_string_list( schemes );*/
}

static GList *
get_list_providers( GtkTreeView *treeview )
{
	GList *list = NULL;
	GtkTreeModel *model;

	model = gtk_tree_view_get_model( treeview );
	gtk_tree_model_foreach( model, ( GtkTreeModelForeachFunc ) get_list_providers_iter, &list );

	return( list );
}

static gboolean
get_list_providers_iter( GtkTreeModel *model, GtkTreePath *path, GtkTreeIter* iter, GList **list )
{
	/*gchar *keyword;
	gchar *description;
	gchar *scheme;

	gtk_tree_model_get( model, iter, PROVIDERS_KEYWORD_COLUMN, &keyword, PROVIDERS_DESC_COLUMN, &description, -1 );
	scheme = g_strdup_printf( "%s|%s", keyword, description );
	g_free( description );
	g_free( keyword );

	( *list ) = g_slist_append(( *list ), scheme );*/

	return( FALSE ); /* don't stop looping */
}

/**
 * nact_providers_list_dispose:
 * @treeview: the #GtkTreeView.
 */
void
nact_providers_list_dispose( BaseWindow *window )
{
	static const gchar *thisfn = "nact_providers_list_dispose";
	GtkTreeView *treeview;
	GtkTreeModel *model;
	GtkTreeSelection *selection;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );

	treeview = GTK_TREE_VIEW( g_object_get_data( G_OBJECT( window ), PROVIDERS_LIST_TREEVIEW ));
	model = gtk_tree_view_get_model( treeview );
	selection = gtk_tree_view_get_selection( treeview );

	gtk_tree_selection_unselect_all( selection );
	gtk_list_store_clear( GTK_LIST_STORE( model ));
}

static void
on_selection_changed( GtkTreeSelection *selection, BaseWindow *window )
{
	/*static const gchar *thisfn = "nact_providers_list_on_selection_changed";*/
	GtkTreeView *treeview;

	/*g_debug( "%s: selection=%p, window=%p", thisfn, ( void * ) selection, ( void * ) window );*/

	/*g_debug( "%s: getting data on window=%p", thisfn, ( void * ) window );*/
	treeview = GTK_TREE_VIEW( g_object_get_data( G_OBJECT( window ), PROVIDERS_LIST_TREEVIEW ));

	/*button = get_remove_button( window );
	gtk_widget_set_sensitive( GTK_WIDGET( button ), editable && gtk_tree_selection_count_selected_rows( selection ) > 0);
	*/
}

static void
on_readable_toggled( GtkCellRendererToggle *renderer, gchar *path, BaseWindow *window )
{
	GtkTreeView *treeview;
	GtkTreeModel *model;

	if( !st_on_selection_change ){

		treeview = GTK_TREE_VIEW( g_object_get_data( G_OBJECT( window ), PROVIDERS_LIST_TREEVIEW ));
		model = gtk_tree_view_get_model( treeview );
	}
}

static void
on_writable_toggled( GtkCellRendererToggle *renderer, gchar *path, BaseWindow *window )
{
	GtkTreeView *treeview;
	GtkTreeModel *model;

	if( !st_on_selection_change ){

		treeview = GTK_TREE_VIEW( g_object_get_data( G_OBJECT( window ), PROVIDERS_LIST_TREEVIEW ));
		model = gtk_tree_view_get_model( treeview );
	}
}

static void
on_up_clicked( GtkButton *button, BaseWindow *window )
{
}

static void
on_down_clicked( GtkButton *button, BaseWindow *window )
{
}

static GtkButton *
get_up_button( BaseWindow *window )
{
	GtkButton *button;

	button = GTK_BUTTON( base_window_get_widget( window, "ProviderButtonUp" ));

	return( button );
}

static GtkButton *
get_down_button( BaseWindow *window )
{
	GtkButton *button;

	button = GTK_BUTTON( base_window_get_widget( window, "ProviderButtonDown" ));

	return( button );
}

#if 0
/*
 * free_subdirs:
 * @subdirs: a list of subdirs as returned by get_subdirs().
 *
 * Release the list of subdirs.
 */
static void
free_gconf_subdirs( GSList *subdirs )
{
	free_gslist( subdirs );
}

/*
 * free_gslist:
 * @list: the GSList to be freed.
 *
 * Frees a GSList of strings.
 */
static void
free_gslist( GSList *list )
{
	g_slist_foreach( list, ( GFunc ) g_free, NULL );
	g_slist_free( list );
}
#endif
