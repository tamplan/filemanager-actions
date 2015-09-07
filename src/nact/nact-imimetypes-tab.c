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

#include "api/fma-object-api.h"

#include "core/na-gtk-utils.h"

#include "nact-main-tab.h"
#include "nact-main-window.h"
#include "nact-match-list.h"
#include "nact-imimetypes-tab.h"

/* private interface data
 */
struct _NactIMimetypesTabInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* the identifier of this notebook page in the Match dialog
 */
#define ITAB_NAME						"mimetypes"

static guint st_initializations = 0;	/* interface initialization count */

static GType   register_type( void );
static void    interface_base_init( NactIMimetypesTabInterface *klass );
static void    interface_base_finalize( NactIMimetypesTabInterface *klass );
static void    initialize_gtk( NactIMimetypesTab *instance );
static void    initialize_window( NactIMimetypesTab *instance );
static void    on_tree_selection_changed( NactTreeView *tview, GList *selected_items, NactIMimetypesTab *instance );
static GSList *get_mimetypes( void *context );
static void    set_mimetypes( void *context, GSList *filters );
static void    on_instance_finalized( gpointer user_data, NactIMimetypesTab *instance );

GType
nact_imimetypes_tab_get_type( void )
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
	static const gchar *thisfn = "nact_imimetypes_tab_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NactIMimetypesTabInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "NactIMimetypesTab", &info, 0 );

	g_type_interface_add_prerequisite( type, GTK_TYPE_APPLICATION_WINDOW );

	return( type );
}

static void
interface_base_init( NactIMimetypesTabInterface *klass )
{
	static const gchar *thisfn = "nact_imimetypes_tab_interface_base_init";

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NactIMimetypesTabInterfacePrivate, 1 );
	}

	st_initializations += 1;
}

static void
interface_base_finalize( NactIMimetypesTabInterface *klass )
{
	static const gchar *thisfn = "nact_imimetypes_tab_interface_base_finalize";

	st_initializations -= 1;

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );
	}
}

/*
 * nact_imimetypes_tab_init:
 * @instance: this #NactIMimetypesTab instance.
 *
 * Initialize the interface
 * Connect to #BaseWindow signals
 */
void
nact_imimetypes_tab_init( NactIMimetypesTab *instance )
{
	static const gchar *thisfn = "nact_imimetypes_tab_init";

	g_return_if_fail( NACT_IS_IMIMETYPES_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn,
			( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	nact_main_tab_init( NACT_MAIN_WINDOW( instance ), TAB_MIMETYPES );
	initialize_gtk( instance );
	initialize_window( instance );

	g_object_weak_ref( G_OBJECT( instance ), ( GWeakNotify ) on_instance_finalized, NULL );
}

/*
 * initialize_gtk:
 * @window: this #NactIMimetypesTab instance.
 *
 * Initializes the tab widget at initial load.
 */
static void
initialize_gtk( NactIMimetypesTab *instance )
{
	static const gchar *thisfn = "nact_imimetypes_tab_initialize_gtk";

	g_return_if_fail( NACT_IS_IMIMETYPES_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	nact_match_list_init_with_args(
			NACT_MAIN_WINDOW( instance ),
			ITAB_NAME,
			TAB_MIMETYPES,
			na_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "MimetypesTreeView" ),
			na_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "AddMimetypeButton" ),
			na_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "RemoveMimetypeButton" ),
			( pget_filters ) get_mimetypes,
			( pset_filters ) set_mimetypes,
			NULL,
			NULL,
			MATCH_LIST_MUST_MATCH_ONE_OF,
			_( "Mimetype filter" ),
			TRUE );
}

/*
 * initialize_window:
 * @window: this #NactIMimetypesTab instance.
 *
 * Initializes the tab widget at each time the widget will be displayed.
 * Connect signals and setup runtime values.
 */
static void
initialize_window( NactIMimetypesTab *instance )
{
	static const gchar *thisfn = "nact_imimetypes_tab_initialize_window";
	NactTreeView *tview;

	g_return_if_fail( NACT_IS_IMIMETYPES_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	tview = nact_main_window_get_items_view( NACT_MAIN_WINDOW( instance ));

	g_signal_connect(
			tview, TREE_SIGNAL_SELECTION_CHANGED,
			G_CALLBACK( on_tree_selection_changed ), instance );
}

static void
on_tree_selection_changed( NactTreeView *tview, GList *selected_items, NactIMimetypesTab *instance )
{
	FMAIContext *context;
	gboolean editable;
	gboolean enable_tab;

	g_object_get( G_OBJECT( instance ),
			MAIN_PROP_CONTEXT, &context, MAIN_PROP_EDITABLE, &editable,
			NULL );

	enable_tab = ( context != NULL );
	nact_main_tab_enable_page( NACT_MAIN_WINDOW( instance ), TAB_MIMETYPES, enable_tab );
}

static GSList *
get_mimetypes( void *context )
{
	return( fma_object_get_mimetypes( context ));
}

static void
set_mimetypes( void *context, GSList *filters )
{
	fma_object_set_mimetypes( context, filters );
}

static void
on_instance_finalized( gpointer user_data, NactIMimetypesTab *instance )
{
	static const gchar *thisfn = "nact_imimetypes_tab_on_instance_finalized";

	g_debug( "%s: instance=%p, user_data=%p", thisfn, ( void * ) instance, ( void * ) user_data );
}
