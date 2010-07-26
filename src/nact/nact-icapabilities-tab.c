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

#include <glib/gi18n.h>

#include <api/na-core-utils.h>
#include <api/na-object-api.h>

#include "nact-main-tab.h"
#include "nact-match-list.h"
#include "nact-add-capability-dialog.h"
#include "nact-icapabilities-tab.h"

/* private interface data
 */
struct NactICapabilitiesTabInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

#define ITAB_NAME						"capabilities"

static gboolean st_initialized = FALSE;
static gboolean st_finalized = FALSE;

static GType   register_type( void );
static void    interface_base_init( NactICapabilitiesTabInterface *klass );
static void    interface_base_finalize( NactICapabilitiesTabInterface *klass );

static void    on_tab_updatable_selection_changed( NactICapabilitiesTab *instance, gint count_selected );

static void    on_add_clicked( GtkButton *button, BaseWindow *window );
static GSList *get_capabilities( NAIContext *context );
static void    set_capabilities( NAIContext *context, GSList *list );

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

	g_type_interface_add_prerequisite( type, BASE_WINDOW_TYPE );

	return( type );
}

static void
interface_base_init( NactICapabilitiesTabInterface *klass )
{
	static const gchar *thisfn = "nact_icapabilities_tab_interface_base_init";

	if( !st_initialized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NactICapabilitiesTabInterfacePrivate, 1 );

		st_initialized = TRUE;
	}
}

static void
interface_base_finalize( NactICapabilitiesTabInterface *klass )
{
	static const gchar *thisfn = "nact_icapabilities_tab_interface_base_finalize";

	if( st_initialized && !st_finalized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		st_finalized = TRUE;

		g_free( klass->private );
	}
}

void
nact_icapabilities_tab_initial_load_toplevel( NactICapabilitiesTab *instance )
{
	static const gchar *thisfn = "nact_icapabilities_tab_initial_load_toplevel";
	GtkWidget *list, *add, *remove;

	g_return_if_fail( NACT_IS_ICAPABILITIES_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

		list = base_window_get_widget( BASE_WINDOW( instance ), "CapabilitiesTreeView" );
		add = base_window_get_widget( BASE_WINDOW( instance ), "AddCapabilityButton" );
		remove = base_window_get_widget( BASE_WINDOW( instance ), "RemoveCapabilityButton" );

		nact_match_list_create_model( BASE_WINDOW( instance ),
				ITAB_NAME, TAB_CAPABILITIES,
				list, add, remove,
				( pget_filters ) get_capabilities,
				( pset_filters ) set_capabilities,
				( pon_add_cb ) on_add_clicked,
				MATCH_LIST_MUST_MATCH_ALL_OF,
				_( "Capability filter" ));
	}
}

void
nact_icapabilities_tab_runtime_init_toplevel( NactICapabilitiesTab *instance )
{
	static const gchar *thisfn = "nact_icapabilities_tab_runtime_init_toplevel";

	g_return_if_fail( NACT_IS_ICAPABILITIES_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

		nact_match_list_init_view( BASE_WINDOW( instance ), ITAB_NAME );

		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( instance ),
				MAIN_WINDOW_SIGNAL_SELECTION_CHANGED,
				G_CALLBACK( on_tab_updatable_selection_changed ));
	}
}

void
nact_icapabilities_tab_all_widgets_showed( NactICapabilitiesTab *instance )
{
	static const gchar *thisfn = "nact_icapabilities_tab_all_widgets_showed";

	g_return_if_fail( NACT_IS_ICAPABILITIES_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	}
}

void
nact_icapabilities_tab_dispose( NactICapabilitiesTab *instance )
{
	static const gchar *thisfn = "nact_icapabilities_tab_dispose";

	g_return_if_fail( NACT_IS_ICAPABILITIES_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

		nact_match_list_dispose( BASE_WINDOW( instance ), ITAB_NAME );
	}
}

static void
on_tab_updatable_selection_changed( NactICapabilitiesTab *instance, gint count_selected )
{
	nact_match_list_on_selection_changed( BASE_WINDOW( instance ), ITAB_NAME, count_selected );
}

static void
on_add_clicked( GtkButton *button, BaseWindow *window )
{
	NAIContext *context;
	GSList *capabilities;
	gchar *new_cap;

	context = nact_main_tab_get_context( NACT_MAIN_WINDOW( window ), NULL );

	if( context ){
		capabilities = nact_match_list_get_rows( window, ITAB_NAME );
		new_cap = nact_add_capability_dialog_run( window, capabilities );

		if( new_cap ){
			nact_match_list_insert_row( window, ITAB_NAME, new_cap, FALSE, FALSE );
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
