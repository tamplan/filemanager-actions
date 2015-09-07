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

#include "api/fma-core-utils.h"
#include "api/fma-object-api.h"

#include "core/na-gtk-utils.h"

#include "base-gtk-utils.h"
#include "nact-main-tab.h"
#include "nact-main-window.h"
#include "nact-match-list.h"
#include "nact-add-scheme-dialog.h"
#include "nact-ischemes-tab.h"

/* private interface data
 */
struct _NactISchemesTabInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* the identifier of this notebook page in the Match dialog
 */
#define ITAB_NAME						"schemes"

static guint st_initializations = 0;	/* interface initialization count */

static GType   register_type( void );
static void    interface_base_init( NactISchemesTabInterface *klass );
static void    interface_base_finalize( NactISchemesTabInterface *klass );
static void    initialize_gtk( NactISchemesTab *instance );
static void    initialize_window( NactISchemesTab *instance );
static void    on_tree_selection_changed( NactTreeView *tview, GList *selected_items, NactISchemesTab *instance );
static void    on_add_from_defaults( GtkButton *button, NactISchemesTab *instance );
static GSList *get_schemes( void *context );
static void    set_schemes( void *context, GSList *filters );
static void    on_instance_finalized( gpointer user_data, NactISchemesTab *instance );

GType
nact_ischemes_tab_get_type( void )
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
	static const gchar *thisfn = "nact_ischemes_tab_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NactISchemesTabInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "NactISchemesTab", &info, 0 );

	g_type_interface_add_prerequisite( type, GTK_TYPE_APPLICATION_WINDOW );

	return( type );
}

static void
interface_base_init( NactISchemesTabInterface *klass )
{
	static const gchar *thisfn = "nact_ischemes_tab_interface_base_init";

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NactISchemesTabInterfacePrivate, 1 );
	}

	st_initializations += 1;
}

static void
interface_base_finalize( NactISchemesTabInterface *klass )
{
	static const gchar *thisfn = "nact_ischemes_tab_interface_base_finalize";

	st_initializations -= 1;

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );
	}
}

/**
 * nact_ischemes_tab_init:
 * @instance: this #NactISchemesTab instance.
 *
 * Initialize the interface
 * Connect to #BaseWindow signals
 */
void
nact_ischemes_tab_init( NactISchemesTab *instance )
{
	static const gchar *thisfn = "nact_ischemes_tab_init";

	g_return_if_fail( NACT_IS_ISCHEMES_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn,
			( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	nact_main_tab_init( NACT_MAIN_WINDOW( instance ), TAB_SCHEMES );
	initialize_gtk( instance );
	initialize_window( instance );

	g_object_weak_ref( G_OBJECT( instance ), ( GWeakNotify ) on_instance_finalized, NULL );
}

static void
initialize_gtk( NactISchemesTab *instance )
{
	static const gchar *thisfn = "nact_ischemes_tab_initialize_gtk";

	g_return_if_fail( NACT_IS_ISCHEMES_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	nact_match_list_init_with_args(
			NACT_MAIN_WINDOW( instance ),
			ITAB_NAME,
			TAB_SCHEMES,
			na_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "SchemesTreeView" ),
			na_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "AddSchemeButton" ),
			na_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "RemoveSchemeButton" ),
			( pget_filters ) get_schemes,
			( pset_filters ) set_schemes,
			NULL,
			NULL,
			MATCH_LIST_MUST_MATCH_ONE_OF,
			_( "Scheme filter" ),
			TRUE );
}

static void
initialize_window( NactISchemesTab *instance )
{
	static const gchar *thisfn = "nact_ischemes_tab_initialize_window";
	NactTreeView *tview;

	g_return_if_fail( NACT_IS_ISCHEMES_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	tview = nact_main_window_get_items_view( NACT_MAIN_WINDOW( instance ));

	g_signal_connect(
			tview, TREE_SIGNAL_SELECTION_CHANGED,
			G_CALLBACK( on_tree_selection_changed ), instance );

	na_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "AddFromDefaultButton",
			"clicked", G_CALLBACK( on_add_from_defaults ), instance );
}

static void
on_tree_selection_changed( NactTreeView *tview, GList *selected_items, NactISchemesTab *instance )
{
	FMAIContext *context;
	gboolean editable;
	gboolean enable_tab;
	GtkWidget *button;

	g_object_get( G_OBJECT( instance ),
			MAIN_PROP_CONTEXT, &context, MAIN_PROP_EDITABLE, &editable,
			NULL );

	enable_tab = ( context != NULL );
	nact_main_tab_enable_page( NACT_MAIN_WINDOW( instance ), TAB_SCHEMES, enable_tab );

	button = na_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "AddFromDefaultButton" );
	base_gtk_utils_set_editable( G_OBJECT( button ), editable );
}

static void
on_add_from_defaults( GtkButton *button, NactISchemesTab *instance )
{
	GSList *schemes;
	gchar *new_scheme;
	FMAIContext *context;

	g_object_get( G_OBJECT( instance ), MAIN_PROP_CONTEXT, &context, NULL );
	g_return_if_fail( context );

	schemes = nact_match_list_get_rows( NACT_MAIN_WINDOW( instance ), ITAB_NAME );
	new_scheme = nact_add_scheme_dialog_run( NACT_MAIN_WINDOW( instance ), schemes );
	fma_core_utils_slist_free( schemes );

	if( new_scheme ){
		nact_match_list_insert_row( NACT_MAIN_WINDOW( instance ), ITAB_NAME, new_scheme, FALSE, FALSE );
		g_free( new_scheme );
	}
}

static GSList *
get_schemes( void *context )
{
	return( fma_object_get_schemes( context ));
}

static void
set_schemes( void *context, GSList *filters )
{
	fma_object_set_schemes( context, filters );
}

static void
on_instance_finalized( gpointer user_data, NactISchemesTab *instance )
{
	static const gchar *thisfn = "nact_ischemes_tab_on_instance_finalized";

	g_debug( "%s: instance=%p, user_data=%p", thisfn, ( void * ) instance, ( void * ) user_data );
}
