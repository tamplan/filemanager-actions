/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011 Pierre Wieser and others (see AUTHORS)
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

#include <api/na-object-api.h>

#include "nact-main-tab.h"
#include "nact-gtk-utils.h"
#include "nact-match-list.h"
#include "nact-ibasenames-tab.h"

/* private interface data
 */
struct NactIBasenamesTabInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

#define ITAB_NAME						"basenames"

static gboolean st_initialized = FALSE;
static gboolean st_finalized = FALSE;
static gboolean st_on_selection_change = FALSE;

static GType   register_type( void );
static void    interface_base_init( NactIBasenamesTabInterface *klass );
static void    interface_base_finalize( NactIBasenamesTabInterface *klass );

static void    on_tab_updatable_selection_changed( BaseWindow *window, gint count_selected );

static void    on_matchcase_toggled( GtkToggleButton *button, BaseWindow *window );
static GSList *get_basenames( void *context );
static void    set_basenames( void *context, GSList *filters );

GType
nact_ibasenames_tab_get_type( void )
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
	static const gchar *thisfn = "nact_ibasenames_tab_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NactIBasenamesTabInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "NactIBasenamesTab", &info, 0 );

	g_type_interface_add_prerequisite( type, BASE_WINDOW_TYPE );

	return( type );
}

static void
interface_base_init( NactIBasenamesTabInterface *klass )
{
	static const gchar *thisfn = "nact_ibasenames_tab_interface_base_init";

	if( !st_initialized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NactIBasenamesTabInterfacePrivate, 1 );

		st_initialized = TRUE;
	}
}

static void
interface_base_finalize( NactIBasenamesTabInterface *klass )
{
	static const gchar *thisfn = "nact_ibasenames_tab_interface_base_finalize";

	if( st_initialized && !st_finalized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		st_finalized = TRUE;

		g_free( klass->private );
	}
}

/**
 * nact_ibasenames_tab_initial_load:
 * @window: this #NactIBasenamesTab instance.
 *
 * Initializes the tab widget at initial load.
 */
void
nact_ibasenames_tab_initial_load_toplevel( NactIBasenamesTab *instance )
{
	static const gchar *thisfn = "nact_ibasenames_tab_initial_load_toplevel";
	GtkWidget *list, *add, *remove;

	g_return_if_fail( NACT_IS_IBASENAMES_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

		list = base_window_get_widget( BASE_WINDOW( instance ), "BasenamesTreeView" );
		add = base_window_get_widget( BASE_WINDOW( instance ), "AddBasenameButton" );
		remove = base_window_get_widget( BASE_WINDOW( instance ), "RemoveBasenameButton" );

		nact_match_list_create_model(
				BASE_WINDOW( instance ),
				ITAB_NAME,
				TAB_BASENAMES,
				list, add, remove,
				( pget_filters ) get_basenames,
				( pset_filters ) set_basenames,
				NULL,
				MATCH_LIST_MUST_MATCH_ONE_OF,
				_( "Basename filter" ), TRUE );
	}
}

/**
 * nact_ibasenames_tab_runtime_init:
 * @window: this #NactIBasenamesTab instance.
 *
 * Initializes the tab widget at each time the widget will be displayed.
 * Connect signals and setup runtime values.
 */
void
nact_ibasenames_tab_runtime_init_toplevel( NactIBasenamesTab *instance )
{
	static const gchar *thisfn = "nact_ibasenames_tab_runtime_init_toplevel";
	GtkWidget *button;

	g_return_if_fail( NACT_IS_IBASENAMES_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( instance ),
				MAIN_WINDOW_SIGNAL_SELECTION_CHANGED,
				G_CALLBACK( on_tab_updatable_selection_changed ));

		button = base_window_get_widget( BASE_WINDOW( instance ), "BasenamesMatchcaseButton" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( button ),
				"toggled",
				G_CALLBACK( on_matchcase_toggled ));

		nact_match_list_init_view( BASE_WINDOW( instance ), ITAB_NAME );
	}
}

void
nact_ibasenames_tab_all_widgets_showed( NactIBasenamesTab *instance )
{
	static const gchar *thisfn = "nact_ibasenames_tab_all_widgets_showed";

	g_return_if_fail( NACT_IS_IBASENAMES_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	}
}

/**
 * nact_ibasenames_tab_dispose:
 * @window: this #NactIBasenamesTab instance.
 *
 * Called at instance_dispose time.
 */
void
nact_ibasenames_tab_dispose( NactIBasenamesTab *instance )
{
	static const gchar *thisfn = "nact_ibasenames_tab_dispose";

	g_return_if_fail( NACT_IS_IBASENAMES_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

		nact_match_list_dispose( BASE_WINDOW( instance ), ITAB_NAME );
	}
}

static void
on_tab_updatable_selection_changed( BaseWindow *window, gint count_selected )
{
	NAIContext *context;
	gboolean editable;
	GtkToggleButton *matchcase_button;
	gboolean matchcase;

	if( st_initialized && !st_finalized ){

		context = nact_main_tab_get_context( NACT_MAIN_WINDOW( window ), &editable );

		st_on_selection_change = TRUE;

		nact_match_list_on_selection_changed( window, ITAB_NAME, count_selected );

		matchcase_button = GTK_TOGGLE_BUTTON( base_window_get_widget( window, "BasenamesMatchcaseButton" ));
		matchcase = context ? na_object_get_matchcase( context ) : FALSE;
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( matchcase_button ), matchcase );
		nact_gtk_utils_set_editable( G_OBJECT( matchcase_button ), editable );

		st_on_selection_change = FALSE;
	}
}

static void
on_matchcase_toggled( GtkToggleButton *button, BaseWindow *window )
{
	NAIContext *context;
	gboolean editable;
	gboolean matchcase;

	if( !st_on_selection_change ){
		context = nact_main_tab_get_context( NACT_MAIN_WINDOW( window ), &editable );

		if( context ){
			matchcase = gtk_toggle_button_get_active( button );

			if( editable ){
				na_object_set_matchcase( context, matchcase );
				g_signal_emit_by_name( G_OBJECT( window ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, context, FALSE );

			} else {
				g_signal_handlers_block_by_func(( gpointer ) button, on_matchcase_toggled, window );
				gtk_toggle_button_set_active( button, !matchcase );
				g_signal_handlers_unblock_by_func(( gpointer ) button, on_matchcase_toggled, window );
			}
		}
	}
}

static GSList *
get_basenames( void *context )
{
	return( na_object_get_basenames( context ));
}

static void
set_basenames( void *context, GSList *filters )
{
	na_object_set_basenames( context, filters );
}
