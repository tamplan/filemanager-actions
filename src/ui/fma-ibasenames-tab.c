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

#include "core/fma-gtk-utils.h"

#include "base-gtk-utils.h"
#include "fma-ibasenames-tab.h"
#include "fma-main-tab.h"
#include "fma-main-window.h"
#include "fma-match-list.h"

/* private interface data
 */
struct _FMAIBasenamesTabInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* the identifier of this notebook page in the Match dialog
 */
#define ITAB_NAME						"basenames"

/* data set against the instance
 */
typedef struct {
	gboolean on_selection_change;
}
	IBasenamesData;

#define IBASENAMES_TAB_PROP_DATA		"fma-ibasenames-tab-data"

static guint    st_initializations = 0;	/* interface initialization count */

static GType           register_type( void );
static void            interface_base_init( FMAIBasenamesTabInterface *klass );
static void            interface_base_finalize( FMAIBasenamesTabInterface *klass );
static void            initialize_gtk( FMAIBasenamesTab *instance );
static void            initialize_window( FMAIBasenamesTab *instance );
static void            on_tree_selection_changed( FMATreeView *tview, GList *selected_items, FMAIBasenamesTab *instance );
static void            on_matchcase_toggled( GtkToggleButton *button, FMAIBasenamesTab *instance );
static GSList         *get_basenames( void *context );
static void            set_basenames( void *context, GSList *filters );
static IBasenamesData *get_ibasenames_data( FMAIBasenamesTab *instance );
static void            on_instance_finalized( gpointer user_data, FMAIBasenamesTab *instance );

GType
fma_ibasenames_tab_get_type( void )
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
	static const gchar *thisfn = "fma_ibasenames_tab_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( FMAIBasenamesTabInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "FMAIBasenamesTab", &info, 0 );

	g_type_interface_add_prerequisite( type, GTK_TYPE_APPLICATION_WINDOW );

	return( type );
}

static void
interface_base_init( FMAIBasenamesTabInterface *klass )
{
	static const gchar *thisfn = "fma_ibasenames_tab_interface_base_init";

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( FMAIBasenamesTabInterfacePrivate, 1 );
	}

	st_initializations += 1;
}

static void
interface_base_finalize( FMAIBasenamesTabInterface *klass )
{
	static const gchar *thisfn = "fma_ibasenames_tab_interface_base_finalize";

	st_initializations -= 1;

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );
	}
}

/*
 * fma_ibasenames_tab_init:
 * @instance: this #FMAIBasenamesTab instance.
 *
 * Initialize the interface
 * Connect to #BaseWindow signals
 */
void
fma_ibasenames_tab_init( FMAIBasenamesTab *instance )
{
	static const gchar *thisfn = "fma_ibasenames_tab_init";
	IBasenamesData *data;

	g_return_if_fail( FMA_IS_IBASENAMES_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn,
			( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	fma_main_tab_init( FMA_MAIN_WINDOW( instance ), TAB_BASENAMES );
	initialize_gtk( instance );
	initialize_window( instance );

	data = get_ibasenames_data( instance );
	data->on_selection_change = FALSE;

	g_object_weak_ref( G_OBJECT( instance ), ( GWeakNotify ) on_instance_finalized, NULL );
}

/*
 * on_base_initialize_gtk:
 * @window: this #FMAIBasenamesTab instance.
 *
 * Initializes the tab widget at initial load.
 */
static void
initialize_gtk( FMAIBasenamesTab *instance )
{
	static const gchar *thisfn = "fma_ibasenames_tab_initialize_gtk";

	g_return_if_fail( instance && FMA_IS_IBASENAMES_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	fma_match_list_init_with_args(
			FMA_MAIN_WINDOW( instance ),
			ITAB_NAME,
			TAB_BASENAMES,
			fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "BasenamesTreeView" ),
			fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "AddBasenameButton" ),
			fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "RemoveBasenameButton" ),
			( pget_filters ) get_basenames,
			( pset_filters ) set_basenames,
			NULL,
			NULL,
			MATCH_LIST_MUST_MATCH_ONE_OF,
			_( "Basename filter" ),
			TRUE );
}

/*
 * on_base_initialize_window:
 * @window: this #FMAIBasenamesTab instance.
 *
 * Initializes the tab widget at each time the widget will be displayed.
 * Connect signals and setup runtime values.
 */
static void
initialize_window( FMAIBasenamesTab *instance )
{
	static const gchar *thisfn = "fma_ibasenames_tab_initialize_window";
	FMATreeView *tview;

	g_return_if_fail( instance && FMA_IS_IBASENAMES_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	tview = fma_main_window_get_items_view( FMA_MAIN_WINDOW( instance ));
	g_signal_connect(
			tview, TREE_SIGNAL_SELECTION_CHANGED,
			G_CALLBACK( on_tree_selection_changed ), instance );

	fma_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "BasenamesMatchcaseButton",
			"toggled", G_CALLBACK( on_matchcase_toggled ), instance );
}

static void
on_tree_selection_changed( FMATreeView *tview, GList *selected_items, FMAIBasenamesTab *instance )
{
	FMAIContext *context;
	gboolean editable;
	gboolean enable_tab;
	GtkToggleButton *matchcase_button;
	gboolean matchcase;
	IBasenamesData *data;

	g_object_get( G_OBJECT( instance ),
			MAIN_PROP_CONTEXT, &context, MAIN_PROP_EDITABLE, &editable,
			NULL );

	enable_tab = ( context != NULL );
	fma_main_tab_enable_page( FMA_MAIN_WINDOW( instance ), TAB_BASENAMES, enable_tab );

	data = get_ibasenames_data( FMA_IBASENAMES_TAB( instance ));
	data->on_selection_change = TRUE;

	matchcase_button = GTK_TOGGLE_BUTTON( fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "BasenamesMatchcaseButton" ));
	matchcase = context ? fma_object_get_matchcase( context ) : FALSE;
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( matchcase_button ), matchcase );
	base_gtk_utils_set_editable( G_OBJECT( matchcase_button ), editable );

	data->on_selection_change = FALSE;
}

static void
on_matchcase_toggled( GtkToggleButton *button, FMAIBasenamesTab *instance )
{
	FMAIContext *context;
	gboolean editable;
	gboolean matchcase;
	IBasenamesData *data;

	data = get_ibasenames_data( FMA_IBASENAMES_TAB( instance ));

	if( !data->on_selection_change ){
		g_object_get( G_OBJECT( instance ),
				MAIN_PROP_CONTEXT, &context, MAIN_PROP_EDITABLE, &editable,
				NULL );

		if( context ){
			matchcase = gtk_toggle_button_get_active( button );

			if( editable ){
				fma_object_set_matchcase( context, matchcase );
				g_signal_emit_by_name( G_OBJECT( instance ), MAIN_SIGNAL_ITEM_UPDATED, context, 0 );

			} else {
				g_signal_handlers_block_by_func(( gpointer ) button, on_matchcase_toggled, instance );
				gtk_toggle_button_set_active( button, !matchcase );
				g_signal_handlers_unblock_by_func(( gpointer ) button, on_matchcase_toggled, instance );
			}
		}
	}
}

static GSList *
get_basenames( void *context )
{
	return( fma_object_get_basenames( context ));
}

static void
set_basenames( void *context, GSList *filters )
{
	fma_object_set_basenames( context, filters );
}

static IBasenamesData *
get_ibasenames_data( FMAIBasenamesTab *instance )
{
	IBasenamesData *data;

	data = ( IBasenamesData * ) g_object_get_data( G_OBJECT( instance ), IBASENAMES_TAB_PROP_DATA );

	if( !data ){
		data = g_new0( IBasenamesData, 1 );
		g_object_set_data( G_OBJECT( instance ), IBASENAMES_TAB_PROP_DATA, data );
	}

	return( data );
}

static void
on_instance_finalized( gpointer user_data, FMAIBasenamesTab *instance )
{
	static const gchar *thisfn = "fma_ibasenames_tab_on_instance_finalized";
	IBasenamesData *data;

	g_debug( "%s: instance=%p, user_data=%p", thisfn, ( void * ) instance, ( void * ) user_data );

	data = get_ibasenames_data( instance );

	g_free( data );
}
