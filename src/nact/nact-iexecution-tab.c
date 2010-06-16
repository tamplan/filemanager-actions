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

#include <api/na-object-api.h>

#include "nact-main-tab.h"
#include "nact-iexecution-tab.h"

/* private interface data
 */
struct NactIExecutionTabInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

static gboolean st_initialized = FALSE;
static gboolean st_finalized = FALSE;
static gboolean st_on_selection_change = FALSE;

static GType    register_type( void );
static void     interface_base_init( NactIExecutionTabInterface *klass );
static void     interface_base_finalize( NactIExecutionTabInterface *klass );

static void     on_tab_updatable_selection_changed( NactIExecutionTab *instance, gint count_selected );
static gboolean tab_set_sensitive( NactIExecutionTab *instance );

GType
nact_iexecution_tab_get_type( void )
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
	static const gchar *thisfn = "nact_iexecution_tab_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NactIExecutionTabInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "NactIExecutionTab", &info, 0 );

	g_type_interface_add_prerequisite( type, BASE_WINDOW_TYPE );

	return( type );
}

static void
interface_base_init( NactIExecutionTabInterface *klass )
{
	static const gchar *thisfn = "nact_iexecution_tab_interface_base_init";

	if( !st_initialized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NactIExecutionTabInterfacePrivate, 1 );

		st_initialized = TRUE;
	}
}

static void
interface_base_finalize( NactIExecutionTabInterface *klass )
{
	static const gchar *thisfn = "nact_iexecution_tab_interface_base_finalize";

	if( st_initialized && !st_finalized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		st_finalized = TRUE;

		g_free( klass->private );
	}
}

/**
 * nact_iexecution_tab_initial_load:
 * @window: this #NactIExecutionTab instance.
 *
 * Initializes the tab widget at initial load.
 */
void
nact_iexecution_tab_initial_load_toplevel( NactIExecutionTab *instance )
{
	static const gchar *thisfn = "nact_iexecution_tab_initial_load_toplevel";

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_IEXECUTION_TAB( instance ));

	if( st_initialized && !st_finalized ){
	}
}

/**
 * nact_iexecution_tab_runtime_init:
 * @window: this #NactIExecutionTab instance.
 *
 * Initializes the tab widget at each time the widget will be displayed.
 * Connect signals and setup runtime values.
 */
void
nact_iexecution_tab_runtime_init_toplevel( NactIExecutionTab *instance )
{
	static const gchar *thisfn = "nact_iexecution_tab_runtime_init_toplevel";

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_IEXECUTION_TAB( instance ));

	if( st_initialized && !st_finalized ){

		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( instance ),
				MAIN_WINDOW_SIGNAL_SELECTION_CHANGED,
				G_CALLBACK( on_tab_updatable_selection_changed ));
	}
}

void
nact_iexecution_tab_all_widgets_showed( NactIExecutionTab *instance )
{
	static const gchar *thisfn = "nact_iexecution_tab_all_widgets_showed";

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_IEXECUTION_TAB( instance ));

	if( st_initialized && !st_finalized ){
	}
}

/**
 * nact_iexecution_tab_dispose:
 * @window: this #NactIExecutionTab instance.
 *
 * Called at instance_dispose time.
 */
void
nact_iexecution_tab_dispose( NactIExecutionTab *instance )
{
	static const gchar *thisfn = "nact_iexecution_tab_dispose";

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_IEXECUTION_TAB( instance ));

	if( st_initialized && !st_finalized ){
	}
}

static void
on_tab_updatable_selection_changed( NactIExecutionTab *instance, gint count_selected )
{
	static const gchar *thisfn = "nact_iexecution_tab_on_tab_updatable_selection_changed";

	g_debug( "%s: instance=%p, count_selected=%d", thisfn, ( void * ) instance, count_selected );
	g_return_if_fail( NACT_IS_IEXECUTION_TAB( instance ));

	if( st_initialized && !st_finalized ){

		st_on_selection_change = TRUE;

		tab_set_sensitive( instance );

		st_on_selection_change = FALSE;
	}
}

static gboolean
tab_set_sensitive( NactIExecutionTab *instance )
{
	NAObjectProfile *profile;
	gboolean enable_tab;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_SELECTED_PROFILE, &profile,
			NULL );

	enable_tab = ( profile != NULL );
	nact_main_tab_enable_page( NACT_MAIN_WINDOW( instance ), TAB_EXECUTION, enable_tab );

	return( enable_tab );
}
