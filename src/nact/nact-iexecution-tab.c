/*
 * Nautilus-Actions
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

#include <string.h>

#include <api/na-object-api.h>

#include "nact-gtk-utils.h"
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

static void     on_normal_mode_toggled( GtkToggleButton *togglebutton, NactIExecutionTab *instance );
static void     on_terminal_mode_toggled( GtkToggleButton *togglebutton, NactIExecutionTab *instance );
static void     on_embedded_mode_toggled( GtkToggleButton *togglebutton, NactIExecutionTab *instance );
static void     on_display_mode_toggled( GtkToggleButton *togglebutton, NactIExecutionTab *instance );
static void     execution_mode_toggle( NactIExecutionTab *instance, GtkToggleButton *togglebutton, GCallback cb, const gchar *mode );
static void     on_startup_notify_toggled( GtkToggleButton *togglebutton, NactIExecutionTab *instance );
static void     on_startup_class_changed( GtkEntry *entry, NactIExecutionTab *instance );
static void     on_execute_as_changed( GtkEntry *entry, NactIExecutionTab *instance );

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

	g_return_if_fail( NACT_IS_IEXECUTION_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
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
	GtkWidget *widget;

	g_return_if_fail( NACT_IS_IEXECUTION_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( instance ),
				MAIN_WINDOW_SIGNAL_SELECTION_CHANGED,
				G_CALLBACK( on_tab_updatable_selection_changed ));

		widget = base_window_get_widget( BASE_WINDOW( instance ), "ExecutionModeNormal" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( widget ),
				"toggled",
				G_CALLBACK( on_normal_mode_toggled ));

		widget = base_window_get_widget( BASE_WINDOW( instance ), "ExecutionModeTerminal" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( widget ),
				"toggled",
				G_CALLBACK( on_terminal_mode_toggled ));

		widget = base_window_get_widget( BASE_WINDOW( instance ), "ExecutionModeEmbedded" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( widget ),
				"toggled",
				G_CALLBACK( on_embedded_mode_toggled ));

		widget = base_window_get_widget( BASE_WINDOW( instance ), "ExecutionModeDisplayOutput" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( widget ),
				"toggled",
				G_CALLBACK( on_display_mode_toggled ));

		widget = base_window_get_widget( BASE_WINDOW( instance ), "StartupNotifyButton" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( widget ),
				"toggled",
				G_CALLBACK( on_startup_notify_toggled ));

		widget = base_window_get_widget( BASE_WINDOW( instance ), "StartupWMClassEntry" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( widget ),
				"changed",
				G_CALLBACK( on_startup_class_changed ));

		widget = base_window_get_widget( BASE_WINDOW( instance ), "ExecuteAsEntry" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( widget ),
				"changed",
				G_CALLBACK( on_execute_as_changed ));
	}
}

void
nact_iexecution_tab_all_widgets_showed( NactIExecutionTab *instance )
{
	static const gchar *thisfn = "nact_iexecution_tab_all_widgets_showed";

	g_return_if_fail( NACT_IS_IEXECUTION_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
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

	g_return_if_fail( NACT_IS_IEXECUTION_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	}
}

static void
on_tab_updatable_selection_changed( NactIExecutionTab *instance, gint count_selected )
{
	static const gchar *thisfn = "nact_iexecution_tab_on_tab_updatable_selection_changed";
	NAObjectProfile *profile;
	gboolean editable;
	gboolean enable_tab;
	gchar *mode;
	GtkWidget *normal_toggle, *terminal_toggle, *embedded_toggle, *display_toggle;
	gboolean notify;
	GtkWidget *notify_check;
	gchar *class, *user;
	GtkWidget *entry;

	g_return_if_fail( NACT_IS_IEXECUTION_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p, count_selected=%d", thisfn, ( void * ) instance, count_selected );

		g_object_get(
				G_OBJECT( instance ),
				TAB_UPDATABLE_PROP_SELECTED_PROFILE, &profile,
				TAB_UPDATABLE_PROP_EDITABLE, &editable,
				NULL );

		enable_tab = ( profile != NULL );
		nact_main_tab_enable_page( NACT_MAIN_WINDOW( instance ), TAB_EXECUTION, enable_tab );

		st_on_selection_change = TRUE;

		normal_toggle = base_window_get_widget( BASE_WINDOW( instance ), "ExecutionModeNormal" );
		terminal_toggle = base_window_get_widget( BASE_WINDOW( instance ), "ExecutionModeTerminal" );
		embedded_toggle = base_window_get_widget( BASE_WINDOW( instance ), "ExecutionModeEmbedded" );
		display_toggle = base_window_get_widget( BASE_WINDOW( instance ), "ExecutionModeDisplayOutput" );

		mode = profile ? na_object_get_execution_mode( profile ) : g_strdup( "Normal" );
		gtk_toggle_button_set_inconsistent( GTK_TOGGLE_BUTTON( normal_toggle ), profile == NULL );

		if( !strcmp( mode, "Normal" )){
			nact_gtk_utils_radio_set_initial_state(
					GTK_RADIO_BUTTON( normal_toggle ),
					G_CALLBACK( on_normal_mode_toggled ), instance, editable, ( profile == NULL ));

		} else if( !strcmp( mode, "Terminal" )){
			nact_gtk_utils_radio_set_initial_state(
					GTK_RADIO_BUTTON( terminal_toggle ),
					G_CALLBACK( on_terminal_mode_toggled ), instance, editable, ( profile == NULL ));

		} else if( !strcmp( mode, "Embedded" )){
			nact_gtk_utils_radio_set_initial_state(
					GTK_RADIO_BUTTON( embedded_toggle ),
					G_CALLBACK( on_embedded_mode_toggled ), instance, editable, ( profile == NULL ));

		} else if( !strcmp( mode, "DisplayOutput" )){
			nact_gtk_utils_radio_set_initial_state(
					GTK_RADIO_BUTTON( display_toggle ),
					G_CALLBACK( on_display_mode_toggled ), instance, editable, ( profile == NULL ));

		} else {
			g_warning( "%s: unable to setup execution mode '%s'", thisfn, mode );
		}

		g_free( mode );

		notify = profile ? na_object_get_startup_notify( profile ) : FALSE;
		notify_check = base_window_get_widget( BASE_WINDOW( instance ), "StartupNotifyButton" );
		nact_gtk_utils_set_editable( G_OBJECT( notify_check ), editable );
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( notify_check ), notify );

		class = profile ? na_object_get_startup_class( profile ) : g_strdup( "" );
		entry = base_window_get_widget( BASE_WINDOW( instance ), "StartupWMClassEntry" );
		gtk_entry_set_text( GTK_ENTRY( entry ), class );
		nact_gtk_utils_set_editable( G_OBJECT( entry ), editable );
		g_free( class );

		user = profile ? na_object_get_execute_as( profile ) : g_strdup( "" );
		entry = base_window_get_widget( BASE_WINDOW( instance ), "ExecuteAsEntry" );
		gtk_entry_set_text( GTK_ENTRY( entry ), user );
		nact_gtk_utils_set_editable( G_OBJECT( entry ), editable );
		g_free( user );

		st_on_selection_change = FALSE;
	}
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
	NAObjectProfile *profile;
	gboolean editable;
	gboolean active;
	gboolean is_normal;
	GtkWidget *widget;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_SELECTED_PROFILE, &profile,
			TAB_UPDATABLE_PROP_EDITABLE, &editable,
			NULL );

	if( profile ){
		active = gtk_toggle_button_get_active( toggle_button );

		if( editable ){
			if( active ){
				na_object_set_execution_mode( profile, mode );

				is_normal = ( strcmp( mode, "Normal" ) == 0 );
				widget = base_window_get_widget( BASE_WINDOW( instance ), "StartupNotifyButton" );
				gtk_widget_set_sensitive( widget, is_normal );
				widget = base_window_get_widget( BASE_WINDOW( instance ), "StartupWMClassEntry" );
				gtk_widget_set_sensitive( widget, is_normal );

				g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, profile, FALSE );
			}

		} else {
			nact_gtk_utils_radio_reset_initial_state( GTK_RADIO_BUTTON( toggle_button ), cb );
		}
	}
}

static void
on_startup_notify_toggled( GtkToggleButton *toggle_button, NactIExecutionTab *instance )
{
	NAObjectProfile *profile;
	gboolean editable;
	gboolean active;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_SELECTED_PROFILE, &profile,
			TAB_UPDATABLE_PROP_EDITABLE, &editable,
			NULL );

	if( profile ){
		active = gtk_toggle_button_get_active( toggle_button );

		if( editable ){
			na_object_set_startup_notify( profile, active );
			g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, profile, FALSE );

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
	NAObjectProfile *profile;
	const gchar *text;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_SELECTED_PROFILE, &profile,
			NULL );

	if( profile ){
		text = gtk_entry_get_text( entry );
		na_object_set_startup_class( profile, text );
		g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, profile, FALSE );
	}
}

static void
on_execute_as_changed( GtkEntry *entry, NactIExecutionTab *instance )
{
	NAObjectProfile *profile;
	const gchar *text;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_SELECTED_PROFILE, &profile,
			NULL );

	if( profile ){
		text = gtk_entry_get_text( entry );
		na_object_set_execute_as( profile, text );
		g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, profile, FALSE );
	}
}
