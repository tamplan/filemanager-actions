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

#include "core/fma-gtk-utils.h"

#include "nact-main-tab.h"
#include "nact-main-window.h"
#include "nact-match-list.h"
#include "fma-add-capability-dialog.h"
#include "fma-icapabilities-tab.h"

/* private interface data
 */
struct _FMAICapabilitiesTabInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

#define ITAB_NAME						"capabilities"

static guint st_initializations = 0;	/* interface initialization count */

static GType   register_type( void );
static void    interface_base_init( FMAICapabilitiesTabInterface *klass );
static void    interface_base_finalize( FMAICapabilitiesTabInterface *klass );
static void    initialize_gtk( FMAICapabilitiesTab *instance );
static void    initialize_window( FMAICapabilitiesTab *instance );
static void    on_tree_selection_changed( NactTreeView *tview, GList *selected_items, FMAICapabilitiesTab *instance );
static void    on_add_clicked( GtkButton *button, FMAICapabilitiesTab *instance );
static GSList *get_capabilities( FMAIContext *context );
static void    set_capabilities( FMAIContext *context, GSList *list );
static void    on_instance_finalized( gpointer user_data, FMAICapabilitiesTab *instance );

GType
fma_icapabilities_tab_get_type( void )
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
	static const gchar *thisfn = "fma_icapabilities_tab_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( FMAICapabilitiesTabInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "FMAICapabilitiesTab", &info, 0 );

	g_type_interface_add_prerequisite( type, GTK_TYPE_APPLICATION_WINDOW );

	return( type );
}

static void
interface_base_init( FMAICapabilitiesTabInterface *klass )
{
	static const gchar *thisfn = "fma_icapabilities_tab_interface_base_init";

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( FMAICapabilitiesTabInterfacePrivate, 1 );
	}

	st_initializations += 1;
}

static void
interface_base_finalize( FMAICapabilitiesTabInterface *klass )
{
	static const gchar *thisfn = "fma_icapabilities_tab_interface_base_finalize";

	st_initializations -= 1;

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );
	}
}

/**
 * fma_icapabilities_tab_init:
 * @instance: this #FMAICapabilitiesTab instance.
 *
 * Initialize the interface
 * Connect to #BaseWindow signals
 */
void
fma_icapabilities_tab_init( FMAICapabilitiesTab *instance )
{
	static const gchar *thisfn = "fma_icapabilities_tab_init";

	g_return_if_fail( FMA_IS_ICAPABILITIES_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn,
			( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	nact_main_tab_init( NACT_MAIN_WINDOW( instance ), TAB_CAPABILITIES );
	initialize_gtk( instance );
	initialize_window( instance );

	g_object_weak_ref( G_OBJECT( instance ), ( GWeakNotify ) on_instance_finalized, NULL );
}

static void
initialize_gtk( FMAICapabilitiesTab *instance )
{
	static const gchar *thisfn = "fma_icapabilities_tab_initialize_gtk";

	g_return_if_fail( FMA_IS_ICAPABILITIES_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	nact_match_list_init_with_args(
			NACT_MAIN_WINDOW( instance ),
			ITAB_NAME,
			TAB_CAPABILITIES,
			fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "CapabilitiesTreeView" ),
			fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "AddCapabilityButton" ),
			fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "RemoveCapabilityButton" ),
			( pget_filters ) get_capabilities,
			( pset_filters ) set_capabilities,
			( pon_add_cb ) on_add_clicked,
			NULL,
			MATCH_LIST_MUST_MATCH_ALL_OF,
			_( "Capability filter" ),
			FALSE );
}

static void
initialize_window( FMAICapabilitiesTab *instance )
{
	static const gchar *thisfn = "fma_icapabilities_tab_initialize_window";
	NactTreeView *tview;

	g_return_if_fail( FMA_IS_ICAPABILITIES_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	tview = nact_main_window_get_items_view( NACT_MAIN_WINDOW( instance ));

	g_signal_connect(
			tview, TREE_SIGNAL_SELECTION_CHANGED,
			G_CALLBACK( on_tree_selection_changed ), instance );
}

static void
on_tree_selection_changed( NactTreeView *tview, GList *selected_items, FMAICapabilitiesTab *instance )
{
	FMAIContext *context;
	gboolean editable;
	gboolean enable_tab;

	g_object_get( G_OBJECT( instance ),
			MAIN_PROP_CONTEXT, &context, MAIN_PROP_EDITABLE, &editable,
			NULL );

	enable_tab = ( context != NULL );
	nact_main_tab_enable_page( NACT_MAIN_WINDOW( instance ), TAB_CAPABILITIES, enable_tab );
}

static void
on_add_clicked( GtkButton *button, FMAICapabilitiesTab *instance )
{
	FMAIContext *context;
	GSList *capabilities;
	gchar *new_cap;

	g_object_get( G_OBJECT( instance ), MAIN_PROP_CONTEXT, &context, NULL );

	if( context ){
		capabilities = nact_match_list_get_rows( NACT_MAIN_WINDOW( instance ), ITAB_NAME );
		new_cap = fma_add_capability_dialog_run( NACT_MAIN_WINDOW( instance ), capabilities );

		if( new_cap ){
			nact_match_list_insert_row( NACT_MAIN_WINDOW( instance ), ITAB_NAME, new_cap, FALSE, FALSE );
			g_free( new_cap );
		}

		fma_core_utils_slist_free( capabilities );
	}
}

static GSList *
get_capabilities( FMAIContext *context )
{
	return( fma_object_get_capabilities( context ));
}

static void
set_capabilities( FMAIContext *context, GSList *list )
{
	fma_object_set_capabilities( context, list );
}

static void
on_instance_finalized( gpointer user_data, FMAICapabilitiesTab *instance )
{
	static const gchar *thisfn = "fma_icapabilities_tab_on_instance_finalized";

	g_debug( "%s: instance=%p, user_data=%p", thisfn, ( void * ) instance, ( void * ) user_data );
}
