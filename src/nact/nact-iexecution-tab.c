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

#include <string.h>

#include "api/fma-object-api.h"

#include "core/na-gtk-utils.h"

#include "base-gtk-utils.h"
#include "nact-main-tab.h"
#include "nact-main-window.h"
#include "nact-iexecution-tab.h"

/* private interface data
 */
struct _NactIExecutionTabInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* data set against the instance
 */
typedef struct {
	gboolean on_selection_change;
}
	IExecutionData;

#define IEXECUTION_TAB_PROP_DATA		"nact-iexecution-tab-data"

static guint st_initializations = 0;	/* interface initialization count */

static GType           register_type( void );
static void            interface_base_init( NactIExecutionTabInterface *klass );
static void            interface_base_finalize( NactIExecutionTabInterface *klass );
static void            initialize_window( NactIExecutionTab *instance );
static void            on_tree_selection_changed( NactTreeView *tview, GList *selected_items, NactIExecutionTab *instance );
static void            on_normal_mode_toggled( GtkToggleButton *togglebutton, NactIExecutionTab *instance );
static void            on_terminal_mode_toggled( GtkToggleButton *togglebutton, NactIExecutionTab *instance );
static void            on_embedded_mode_toggled( GtkToggleButton *togglebutton, NactIExecutionTab *instance );
static void            on_display_mode_toggled( GtkToggleButton *togglebutton, NactIExecutionTab *instance );
static void            execution_mode_toggle( NactIExecutionTab *instance, GtkToggleButton *togglebutton, GCallback cb, const gchar *mode );
static void            on_startup_notify_toggled( GtkToggleButton *togglebutton, NactIExecutionTab *instance );
static void            on_startup_class_changed( GtkEntry *entry, NactIExecutionTab *instance );
static void            on_execute_as_changed( GtkEntry *entry, NactIExecutionTab *instance );
static IExecutionData *get_iexecution_data( NactIExecutionTab *instance );
static void            on_instance_finalized( gpointer user_data, NactIExecutionTab *instance );

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

	g_type_interface_add_prerequisite( type, GTK_TYPE_APPLICATION_WINDOW );

	return( type );
}

static void
interface_base_init( NactIExecutionTabInterface *klass )
{
	static const gchar *thisfn = "nact_iexecution_tab_interface_base_init";

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NactIExecutionTabInterfacePrivate, 1 );
	}

	st_initializations += 1;
}

static void
interface_base_finalize( NactIExecutionTabInterface *klass )
{
	static const gchar *thisfn = "nact_iexecution_tab_interface_base_finalize";

	st_initializations -= 1;

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );
	}
}

/**
 * nact_iexecution_tab_init:
 * @instance: this #NactIExecutionTab instance.
 *
 * Initialize the interface
 * Connect to #BaseWindow signals
 */
void
nact_iexecution_tab_init( NactIExecutionTab *instance )
{
	static const gchar *thisfn = "nact_iexecution_tab_init";
	IExecutionData *data;

	g_return_if_fail( instance && NACT_IS_IEXECUTION_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn,
			( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	nact_main_tab_init( NACT_MAIN_WINDOW( instance ), TAB_EXECUTION );
	initialize_window( instance );

	data = get_iexecution_data( instance );
	data->on_selection_change = FALSE;

	g_object_weak_ref( G_OBJECT( instance ), ( GWeakNotify ) on_instance_finalized, NULL );
}

/*
 * on_base_initialize_window:
 * @window: this #NactIExecutionTab instance.
 *
 * Initializes the tab widget at each time the widget will be displayed.
 * Connect signals and setup runtime values.
 */
static void
initialize_window( NactIExecutionTab *instance )
{
	static const gchar *thisfn = "nact_iexecution_tab_initialize_window";
	NactTreeView *tview;

	g_return_if_fail( NACT_IS_IEXECUTION_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	tview = nact_main_window_get_items_view( NACT_MAIN_WINDOW( instance ));

	g_signal_connect(
			tview, TREE_SIGNAL_SELECTION_CHANGED,
			G_CALLBACK( on_tree_selection_changed ), instance );

	na_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "ExecutionModeNormal",
			"toggled", G_CALLBACK( on_normal_mode_toggled ), instance );

	na_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "ExecutionModeTerminal",
			"toggled", G_CALLBACK( on_terminal_mode_toggled ), instance );

	na_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "ExecutionModeEmbedded",
			"toggled", G_CALLBACK( on_embedded_mode_toggled ), instance );

	na_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "ExecutionModeDisplayOutput",
			"toggled", G_CALLBACK( on_display_mode_toggled ), instance );

	na_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "StartupNotifyButton",
			"toggled", G_CALLBACK( on_startup_notify_toggled ), instance );

	na_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "StartupWMClassEntry",
			"changed", G_CALLBACK( on_startup_class_changed ), instance );

	na_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "ExecuteAsEntry",
			"changed", G_CALLBACK( on_execute_as_changed ), instance );
}

static void
on_tree_selection_changed( NactTreeView *tview, GList *selected_items, NactIExecutionTab *instance )
{
	static const gchar *thisfn = "nact_iexecution_tab_on_tree_selection_changed";
	FMAObjectProfile *profile;
	gboolean editable;
	gboolean enable_tab;
	gchar *mode;
	GtkWidget *normal_toggle, *terminal_toggle, *embedded_toggle, *display_toggle;
	gboolean notify;
	GtkWidget *notify_check, *frame;
	gchar *class, *user;
	GtkWidget *entry;
	IExecutionData *data;

	g_return_if_fail( NACT_IS_IEXECUTION_TAB( instance ));

	g_debug( "%s: tview=%p, selected_items=%p (count=%d), instance=%p (%s)",
			thisfn, tview,
			( void * ) selected_items, g_list_length( selected_items ),
			( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	g_object_get(
			G_OBJECT( instance ),
			MAIN_PROP_PROFILE, &profile,
			MAIN_PROP_EDITABLE, &editable,
			NULL );

	enable_tab = ( profile != NULL );
	nact_main_tab_enable_page( NACT_MAIN_WINDOW( instance ), TAB_EXECUTION, enable_tab );

	data = get_iexecution_data( instance );
	data->on_selection_change = TRUE;

	normal_toggle = na_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ExecutionModeNormal" );
	terminal_toggle = na_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ExecutionModeTerminal" );
	embedded_toggle = na_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ExecutionModeEmbedded" );
	display_toggle = na_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ExecutionModeDisplayOutput" );

	mode = profile ? fma_object_get_execution_mode( profile ) : g_strdup( "Normal" );
	gtk_toggle_button_set_inconsistent( GTK_TOGGLE_BUTTON( normal_toggle ), profile == NULL );

	if( !strcmp( mode, "Normal" )){
		base_gtk_utils_radio_set_initial_state(
				GTK_RADIO_BUTTON( normal_toggle ),
				G_CALLBACK( on_normal_mode_toggled ), instance, editable, ( profile != NULL ));

	} else if( !strcmp( mode, "Terminal" )){
		base_gtk_utils_radio_set_initial_state(
				GTK_RADIO_BUTTON( terminal_toggle ),
				G_CALLBACK( on_terminal_mode_toggled ), instance, editable, ( profile != NULL ));

	} else if( !strcmp( mode, "Embedded" )){
		base_gtk_utils_radio_set_initial_state(
				GTK_RADIO_BUTTON( embedded_toggle ),
				G_CALLBACK( on_embedded_mode_toggled ), instance, editable, ( profile != NULL ));

	} else if( !strcmp( mode, "DisplayOutput" )){
		base_gtk_utils_radio_set_initial_state(
				GTK_RADIO_BUTTON( display_toggle ),
				G_CALLBACK( on_display_mode_toggled ), instance, editable, ( profile != NULL ));

	} else {
		g_warning( "%s: unable to setup execution mode '%s'", thisfn, mode );
	}

	g_free( mode );

	frame = na_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "StartupModeFrame" );
	gtk_widget_set_sensitive( frame, FALSE );

	notify = profile ? fma_object_get_startup_notify( profile ) : FALSE;
	notify_check = na_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "StartupNotifyButton" );
	base_gtk_utils_set_editable( G_OBJECT( notify_check ), editable );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( notify_check ), notify );

	class = profile ? fma_object_get_startup_class( profile ) : g_strdup( "" );
	entry = na_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "StartupWMClassEntry" );
	gtk_entry_set_text( GTK_ENTRY( entry ), class );
	base_gtk_utils_set_editable( G_OBJECT( entry ), editable );
	g_free( class );

	frame = na_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "UserFrame" );
	gtk_widget_set_sensitive( frame, FALSE );

	user = profile ? fma_object_get_execute_as( profile ) : g_strdup( "" );
	entry = na_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ExecuteAsEntry" );
	gtk_entry_set_text( GTK_ENTRY( entry ), user );
	base_gtk_utils_set_editable( G_OBJECT( entry ), editable );
	g_free( user );

	data->on_selection_change = FALSE;
}

static void
on_normal_mode_toggled( GtkToggleButton *togglebutton, NactIExecutionTab *instance )
{
	execution_mode_toggle( instance, togglebutton, G_CALLBACK( on_normal_mode_toggled ), "Normal" );
}

static void
on_terminal_mode_toggled( GtkToggleButton *togglebutton, NactIExecutionTab *instance )
{
	execution_mode_toggle( instance, togglebutton, G_CALLBACK( on_terminal_mode_toggled ), "Terminal" );
}

static void
on_embedded_mode_toggled( GtkToggleButton *togglebutton, NactIExecutionTab *instance )
{
	execution_mode_toggle( instance, togglebutton, G_CALLBACK( on_embedded_mode_toggled ), "Embedded" );
}

static void
on_display_mode_toggled( GtkToggleButton *togglebutton, NactIExecutionTab *instance )
{
	execution_mode_toggle( instance, togglebutton, G_CALLBACK( on_display_mode_toggled ), "DisplayOutput" );
}

static void
execution_mode_toggle( NactIExecutionTab *instance, GtkToggleButton *toggle_button, GCallback cb, const gchar *mode )
{
	FMAObjectProfile *profile;
	gboolean editable;
	gboolean active;
	gboolean is_normal;
	GtkWidget *widget;

	g_object_get(
			G_OBJECT( instance ),
			MAIN_PROP_PROFILE, &profile,
			MAIN_PROP_EDITABLE, &editable,
			NULL );

	if( profile ){
		active = gtk_toggle_button_get_active( toggle_button );

		if( editable ){
			if( active ){
				fma_object_set_execution_mode( profile, mode );

				is_normal = ( strcmp( mode, "Normal" ) == 0 );
				widget = na_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "StartupNotifyButton" );
				gtk_widget_set_sensitive( widget, is_normal );
				widget = na_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "StartupWMClassEntry" );
				gtk_widget_set_sensitive( widget, is_normal );

				g_signal_emit_by_name( G_OBJECT( instance ), MAIN_SIGNAL_ITEM_UPDATED, profile, 0 );
			}

		} else {
			base_gtk_utils_radio_reset_initial_state( GTK_RADIO_BUTTON( toggle_button ), cb );
		}
	}
}

static void
on_startup_notify_toggled( GtkToggleButton *toggle_button, NactIExecutionTab *instance )
{
	FMAObjectProfile *profile;
	gboolean editable;
	gboolean active;

	g_object_get(
			G_OBJECT( instance ),
			MAIN_PROP_PROFILE, &profile,
			MAIN_PROP_EDITABLE, &editable,
			NULL );

	if( profile ){
		active = gtk_toggle_button_get_active( toggle_button );

		if( editable ){
			fma_object_set_startup_notify( profile, active );
			g_signal_emit_by_name( G_OBJECT( instance ), MAIN_SIGNAL_ITEM_UPDATED, profile, 0 );

		} else {
			g_signal_handlers_block_by_func(( gpointer ) toggle_button, on_startup_notify_toggled, instance );
			gtk_toggle_button_set_active( toggle_button, !active );
			g_signal_handlers_unblock_by_func(( gpointer ) toggle_button, on_startup_notify_toggled, instance );
		}
	}
}

static void
on_startup_class_changed( GtkEntry *entry, NactIExecutionTab *instance )
{
	FMAObjectProfile *profile;
	const gchar *text;

	g_object_get(
			G_OBJECT( instance ),
			MAIN_PROP_PROFILE, &profile,
			NULL );

	if( profile ){
		text = gtk_entry_get_text( entry );
		fma_object_set_startup_class( profile, text );
		g_signal_emit_by_name( G_OBJECT( instance ), MAIN_SIGNAL_ITEM_UPDATED, profile, 0 );
	}
}

static void
on_execute_as_changed( GtkEntry *entry, NactIExecutionTab *instance )
{
	FMAObjectProfile *profile;
	const gchar *text;

	g_object_get(
			G_OBJECT( instance ),
			MAIN_PROP_PROFILE, &profile,
			NULL );

	if( profile ){
		text = gtk_entry_get_text( entry );
		fma_object_set_execute_as( profile, text );
		g_signal_emit_by_name( G_OBJECT( instance ), MAIN_SIGNAL_ITEM_UPDATED, profile, 0 );
	}
}

static IExecutionData *
get_iexecution_data( NactIExecutionTab *instance )
{
	IExecutionData *data;

	data = ( IExecutionData * ) g_object_get_data( G_OBJECT( instance ), IEXECUTION_TAB_PROP_DATA );

	if( !data ){
		data = g_new0( IExecutionData, 1 );
		g_object_set_data( G_OBJECT( instance ), IEXECUTION_TAB_PROP_DATA, data );
	}

	return( data );
}

static void
on_instance_finalized( gpointer user_data, NactIExecutionTab *instance )
{
	static const gchar *thisfn = "nact_iexecution_tab_on_instance_finalized";
	IExecutionData *data;

	g_debug( "%s: instance=%p, user_data=%p", thisfn, ( void * ) instance, ( void * ) user_data );

	data = get_iexecution_data( instance );

	g_free( data );
}
