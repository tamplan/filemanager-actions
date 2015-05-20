/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2014 Pierre Wieser and others (see AUTHORS)
 *
 * Nautilus-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Nautilus-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nautilus-Actions; see the file COPYING. If not, see
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

#include "api/na-core-utils.h"
#include "api/na-object-api.h"

#include "core/na-gtk-utils.h"

#include "nact-main-tab.h"
#include "nact-main-window.h"
#include "nact-match-list.h"
#include "nact-add-capability-dialog.h"
#include "nact-icapabilities-tab.h"

/* private interface data
 */
struct _NactICapabilitiesTabInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

#define ITAB_NAME						"capabilities"

static guint st_initializations = 0;	/* interface initialization count */

static GType   register_type( void );
static void    interface_base_init( NactICapabilitiesTabInterface *klass );
static void    interface_base_finalize( NactICapabilitiesTabInterface *klass );
static void    initialize_gtk( NactICapabilitiesTab *instance );
static void    initialize_window( NactICapabilitiesTab *instance );
static void    on_tree_selection_changed( NactTreeView *tview, GList *selected_items, NactICapabilitiesTab *instance );
static void    on_add_clicked( GtkButton *button, NactICapabilitiesTab *instance );
static GSList *get_capabilities( NAIContext *context );
static void    set_capabilities( NAIContext *context, GSList *list );
static void    on_instance_finalized( gpointer user_data, NactICapabilitiesTab *instance );

GType
nact_icapabilities_tab_get_type( void )
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
	static const gchar *thisfn = "nact_icapabilities_tab_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NactICapabilitiesTabInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "NactICapabilitiesTab", &info, 0 );

	g_type_interface_add_prerequisite( type, GTK_TYPE_APPLICATION_WINDOW );

	return( type );
}

static void
interface_base_init( NactICapabilitiesTabInterface *klass )
{
	static const gchar *thisfn = "nact_icapabilities_tab_interface_base_init";

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NactICapabilitiesTabInterfacePrivate, 1 );
	}

	st_initializations += 1;
}

static void
interface_base_finalize( NactICapabilitiesTabInterface *klass )
{
	static const gchar *thisfn = "nact_icapabilities_tab_interface_base_finalize";

	st_initializations -= 1;

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );
	}
}

/**
 * nact_icapabilities_tab_init:
 * @instance: this #NactICapabilitiesTab instance.
 *
 * Initialize the interface
 * Connect to #BaseWindow signals
 */
void
nact_icapabilities_tab_init( NactICapabilitiesTab *instance )
{
	static const gchar *thisfn = "nact_icapabilities_tab_init";

	g_return_if_fail( NACT_IS_ICAPABILITIES_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn,
			( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	nact_main_tab_init( NACT_MAIN_WINDOW( instance ), TAB_CAPABILITIES );
	initialize_gtk( instance );
	initialize_window( instance );

	g_object_weak_ref( G_OBJECT( instance ), ( GWeakNotify ) on_instance_finalized, NULL );
}

static void
initialize_gtk( NactICapabilitiesTab *instance )
{
	static const gchar *thisfn = "nact_icapabilities_tab_initialize_gtk";

	g_return_if_fail( NACT_IS_ICAPABILITIES_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	nact_match_list_init_with_args(
			NACT_MAIN_WINDOW( instance ),
			ITAB_NAME,
			TAB_CAPABILITIES,
			na_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "CapabilitiesTreeView" ),
			na_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "AddCapabilityButton" ),
			na_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "RemoveCapabilityButton" ),
			( pget_filters ) get_capabilities,
			( pset_filters ) set_capabilities,
			( pon_add_cb ) on_add_clicked,
			NULL,
			MATCH_LIST_MUST_MATCH_ALL_OF,
			_( "Capability filter" ),
			FALSE );
}

static void
initialize_window( NactICapabilitiesTab *instance )
{
	static const gchar *thisfn = "nact_icapabilities_tab_initialize_window";
	NactTreeView *tview;

	g_return_if_fail( NACT_IS_ICAPABILITIES_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	tview = nact_main_window_get_items_view( NACT_MAIN_WINDOW( instance ));

	g_signal_connect(
			tview, TREE_SIGNAL_SELECTION_CHANGED,
			G_CALLBACK( on_tree_selection_changed ), instance );
}

static void
on_tree_selection_changed( NactTreeView *tview, GList *selected_items, NactICapabilitiesTab *instance )
{
	NAIContext *context;
	gboolean editable;
	gboolean enable_tab;

	g_object_get( G_OBJECT( instance ),
			MAIN_PROP_CONTEXT, &context, MAIN_PROP_EDITABLE, &editable,
			NULL );

	enable_tab = ( context != NULL );
	nact_main_tab_enable_page( NACT_MAIN_WINDOW( instance ), TAB_CAPABILITIES, enable_tab );
}

static void
on_add_clicked( GtkButton *button, NactICapabilitiesTab *instance )
{
	NAIContext *context;
	GSList *capabilities;
	gchar *new_cap;

	g_object_get( G_OBJECT( instance ), MAIN_PROP_CONTEXT, &context, NULL );

	if( context ){
		capabilities = nact_match_list_get_rows( NACT_MAIN_WINDOW( instance ), ITAB_NAME );
		new_cap = nact_add_capability_dialog_run( NACT_MAIN_WINDOW( instance ), capabilities );

		if( new_cap ){
			nact_match_list_insert_row( NACT_MAIN_WINDOW( instance ), ITAB_NAME, new_cap, FALSE, FALSE );
			g_free( new_cap );
		}

		na_core_utils_slist_free( capabilities );
	}
}

static GSList *
get_capabilities( NAIContext *context )
{
	return( na_object_get_capabilities( context ));
}

static void
set_capabilities( NAIContext *context, GSList *list )
{
	na_object_set_capabilities( context, list );
}

static void
on_instance_finalized( gpointer user_data, NactICapabilitiesTab *instance )
{
	static const gchar *thisfn = "nact_icapabilities_tab_on_instance_finalized";

	g_debug( "%s: instance=%p, user_data=%p", thisfn, ( void * ) instance, ( void * ) user_data );
}
