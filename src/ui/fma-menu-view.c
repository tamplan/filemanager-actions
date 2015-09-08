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

#include "core/fma-gtk-utils.h"
#include "core/fma-iprefs.h"

#include "fma-main-window.h"
#include "fma-menu.h"
#include "fma-menu-view.h"

/* defines the toolbar properties
 */
typedef struct {
	guint        id;
	const gchar *prefs_key;
	const gchar *action_name;
	const gchar *toolbar_name;
	GtkWidget   *toolbar;				/* loaded at run time */
}
	sToolbarProps;

static sToolbarProps st_toolbar_props[] = {
	{ TOOLBAR_FILE_ID,  IPREFS_MAIN_TOOLBAR_FILE_DISPLAY,  "toolbar-file",  "file-toolbar",  NULL },
	{ TOOLBAR_EDIT_ID,  IPREFS_MAIN_TOOLBAR_EDIT_DISPLAY,  "toolbar-edit",  "edit-toolbar",  NULL },
	{ TOOLBAR_TOOLS_ID, IPREFS_MAIN_TOOLBAR_TOOLS_DISPLAY, "toolbar-tools", "tools-toolbar", NULL },
	{ TOOLBAR_HELP_ID,  IPREFS_MAIN_TOOLBAR_HELP_DISPLAY,  "toolbar-help",  "help-toolbar",  NULL }
};

static const gchar *st_toolbar_ui       = PKGUIDIR "/fma-toolbar.ui";

/* associates the target string (from the XML ui definition)
 *  to the desired GTK_POS
 */
typedef struct {
	const gchar *target;
	guint        pos;
}
	sNotebookTabsProps;

static sNotebookTabsProps st_notebook_tabs_props[] = {
	{ "top",    GTK_POS_TOP },
	{ "right",  GTK_POS_RIGHT },
	{ "bottom", GTK_POS_BOTTOM },
	{ "left",   GTK_POS_LEFT },
};

static void                setup_toolbars_submenu( FMAMainWindow *window );
static void                setup_toolbar( FMAMainWindow *window, GtkBuilder *builder, guint toolbar_id );
static sToolbarProps      *get_toolbar_properties_by_id( guint toolbar_id );
static sToolbarProps      *get_toolbar_properties_by_name( const gchar *action_name );
#if 0
static void                reorder_toolbars( GtkWidget *hbox, sToolbarProps *props );
#endif
static void                setup_notebook_tab_position_submenu( FMAMainWindow *window );
static sNotebookTabsProps *get_notebook_tabs_properties_by_target( const gchar *target );
static sNotebookTabsProps *get_notebook_tabs_properties_by_pos( guint pos );
void                       set_notebook_tabs_position( FMAMainWindow *main_window, guint pos );

/**
 * fma_menu_view_init:
 * @window: the #FMAMainWindow main window.
 *
 * Setup the toolbars menu at creation time.
 */
void
fma_menu_view_init( FMAMainWindow *window )
{
	setup_toolbars_submenu( window );
	setup_notebook_tab_position_submenu( window );
}

/**
 * fma_menu_view_update_sensitivities:
 * @window: the #FMAMainWindow main window.
 *
 * Update sensitivity of items of the View menu.
 */
void
fma_menu_view_update_sensitivities( FMAMainWindow *window )
{
	sMenuData *sdata;
	guint count_list;

	sdata = fma_menu_get_data( window );

	/* expand all/collapse all requires at least one item in the list */
	count_list = sdata->count_menus + sdata->count_actions + sdata->count_profiles;
	fma_menu_enable_item( window, "expand", count_list > 0 );
	fma_menu_enable_item( window, "collapse", count_list > 0 );
}

/*
 * Setup the initial display of the standard main toolbars.
 *
 * This actually only setup the initial state of the toggle options in
 * View > Toolbars menu; when an option is activated, this will trigger
 * the 'on_view_toolbar_activated()' which will actually display the
 * toolbar.
 */
void
setup_toolbars_submenu( FMAMainWindow *window )
{
	GtkBuilder *builder;

	builder = gtk_builder_new_from_file( st_toolbar_ui );

	setup_toolbar( window, builder, TOOLBAR_FILE_ID );
	setup_toolbar( window, builder, TOOLBAR_EDIT_ID );
	setup_toolbar( window, builder, TOOLBAR_TOOLS_ID );
	setup_toolbar( window, builder, TOOLBAR_HELP_ID );

	g_object_unref( builder );
}

static void
setup_toolbar( FMAMainWindow *window, GtkBuilder *builder, guint toolbar_id )
{
	sToolbarProps *props;
	gboolean is_active;
	GAction *action;

	props = get_toolbar_properties_by_id( toolbar_id );
	g_return_if_fail( props && props->id == toolbar_id );

	/* load and ref the toolbar from the UI file */
	props->toolbar = ( GtkWidget * ) g_object_ref( gtk_builder_get_object( builder, props->toolbar_name ));
	g_return_if_fail( props->toolbar && GTK_IS_TOOLBAR( props->toolbar ));

	/* display the toolbar depending it is active or not */
	is_active = fma_settings_get_boolean( props->prefs_key, NULL, NULL );
	if( is_active ){
		action = g_action_map_lookup_action( G_ACTION_MAP( window ), props->action_name );
		g_action_change_state( action, g_variant_new_boolean( is_active ));
	}
}

static sToolbarProps *
get_toolbar_properties_by_id( guint toolbar_id )
{
	static const gchar *thisfn = "fma_menu_view_get_toolbar_properties_by_id";
	guint i;

	for( i=0 ; i < G_N_ELEMENTS( st_toolbar_props ) ; ++i ){
		if( st_toolbar_props[i].id == toolbar_id ){
			return( &st_toolbar_props[i] );
		}
	}

	g_warning( "%s: unable to find toolbar properties for id=%d", thisfn, toolbar_id );
	return( NULL );
}

/**
 * fma_menu_view_toolbar_display:
 * @main_window: the #FMAMainWindow main window.
 * @action_name: the action name.
 * @visible: whether the toolbar must be displayed or hidden.
 */
void
fma_menu_view_toolbar_display( FMAMainWindow *main_window, const gchar *action_name, gboolean visible )
{
	sToolbarProps *props;
	GtkWidget *parent;

	props = get_toolbar_properties_by_name( action_name );
	g_return_if_fail( props );

	parent = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( main_window ), "main-toolbar" );
	g_return_if_fail( parent && GTK_IS_CONTAINER( parent ));

	if( visible ){
		gtk_grid_attach( GTK_GRID( parent ), props->toolbar, props->id-1, 0, 1, 1 );

	} else {
		gtk_container_remove( GTK_CONTAINER( parent ), props->toolbar );
	}

	gtk_widget_show_all( parent );
	fma_settings_set_boolean( props->prefs_key, visible );
}

static sToolbarProps *
get_toolbar_properties_by_name( const gchar *action_name )
{
	static const gchar *thisfn = "fma_menu_view_get_toolbar_properties_by_name";
	guint i;

	for( i=0 ; i < G_N_ELEMENTS( st_toolbar_props ) ; ++i ){
		if( g_utf8_collate( st_toolbar_props[i].action_name, action_name ) == 0 ){
			return( &st_toolbar_props[i] );
		}
	}

	g_warning( "%s: unable to find toolbar properties for action name=%s", thisfn, action_name );
	return( NULL );
}

#if 0
/*
 * @hbox: the GtkHBox container
 * @toolbar_id: toolbar identifier
 * @handle: the #GtkToolbar widget
 *
 * reposition the newly activated toolbar in handle
 * so that the relative positions of toolbars are respected in hbox
 */
static void
reorder_toolbars( GtkWidget *hbox, sToolbarProps *props )
{
	int this_canonic_rel_pos;
	int i;
	GList *children, *ic;
	int pos;
	int canonic_pos;

	this_canonic_rel_pos = 0;
	for( i = 0 ; i < G_N_ELEMENTS( toolbar_pos ); ++ i ){
		if( toolbar_pos[i] == toolbar_id ){
			this_canonic_rel_pos = i;
			break;
		}
	}
	g_object_set_data( G_OBJECT( handle ), "toolbar-canonic-pos", GINT_TO_POINTER( this_canonic_rel_pos ));

	pos = 0;
	children = gtk_container_get_children( GTK_CONTAINER( hbox ));
	for( ic = children ; ic ; ic = ic->next ){
		canonic_pos = GPOINTER_TO_INT( g_object_get_data( G_OBJECT( ic->data ), "toolbar-canonic-pos" ));
		if( canonic_pos >= this_canonic_rel_pos ){
			break;
		}
		pos += 1;
	}

	gtk_box_reorder_child( GTK_BOX( hbox ), handle, pos );
}
#endif

/*
 * Setup the initial display of the notebook tabs labels.
 *
 * ***
 * This actually only setup the initial state of the toggle options in
 * View > Toolbars menu; when an option is activated, this will trigger
 * the 'on_view_toolbar_activated()' which will actually display the
 * toolbar.
 * ***
 */
void
setup_notebook_tab_position_submenu( FMAMainWindow *window )
{
	guint pos;
	sNotebookTabsProps *props;
	GAction *action;

	pos = fma_iprefs_get_tabs_pos( NULL );
	props = get_notebook_tabs_properties_by_pos( pos );
	g_return_if_fail( props );

	action = g_action_map_lookup_action( G_ACTION_MAP( window ), "tab-position" );
	g_action_change_state( action, g_variant_new_string( props->target ));
}

#if 0
/*
 * When activating one of the GtkRadioAction which handles the position
 * of the notebook tabs
 * @action: the first GtkRadioAction of the group
 * @current: the activated GtkRadioAction
 *
 * This function is triggered once each time we are activating an item of
 * the menu, after having set the "current_value" to the new value. All
 * GtkRadioButtons items share the same "current_value".
 */
void
fma_menu_view_on_tabs_pos_changed( GtkRadioAction *action, GtkRadioAction *current, BaseWindow *window )
{
	GtkNotebook *notebook;
	guint new_pos;

	notebook = GTK_NOTEBOOK( base_window_get_widget( BASE_WINDOW( window ), "MainNotebook" ));
	new_pos = gtk_radio_action_get_current_value( action );
	gtk_notebook_set_tab_pos( notebook, new_pos );
}
#endif

/**
 * fma_menu_view_notebook_tab_display:
 * @main_window: the #FMAMainWindow main window.
 * @action_name: the action name.
 * @target: the targeted position.
 */
void
fma_menu_view_notebook_tab_display( FMAMainWindow *main_window, const gchar *action_name, const gchar *target )
{
	sNotebookTabsProps *props;

	props = get_notebook_tabs_properties_by_target( target );
	g_return_if_fail( props );
	set_notebook_tabs_position( main_window, props->pos );
}

/*
 * returns the sNotebookTabsProperties structure for the specified target
 * which should be top, right, bottom or left
 */
static sNotebookTabsProps *
get_notebook_tabs_properties_by_target( const gchar *target )
{
	static const gchar *thisfn = "fma_menu_view_get_notebook_tabs_properties_by_target";
	guint i;

	for( i=0 ; i<G_N_ELEMENTS( st_notebook_tabs_props ) ; ++i ){
		if( g_utf8_collate( st_notebook_tabs_props[i].target, target ) == 0 ){
			return( &st_notebook_tabs_props[i] );
		}
	}

	g_warning( "%s: unable to find properties for target=%s", thisfn, target );
	return( NULL );
}

/*
 * returns the sNotebookTabsProperties structure for the specified position
 */
static sNotebookTabsProps *
get_notebook_tabs_properties_by_pos( guint pos )
{
	static const gchar *thisfn = "fma_menu_view_get_notebook_tabs_properties_by_pos";
	guint i;

	for( i=0 ; i<G_N_ELEMENTS( st_notebook_tabs_props ) ; ++i ){
		if( st_notebook_tabs_props[i].pos == pos ){
			return( &st_notebook_tabs_props[i] );
		}
	}

	g_warning( "%s: unable to find properties for pos=%u", thisfn, pos );
	return( NULL );
}

/*
 * Set the position of the main notebook tabs
 */
void
set_notebook_tabs_position( FMAMainWindow *main_window, guint pos )
{
	GtkNotebook *notebook;

	notebook = GTK_NOTEBOOK( fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( main_window ), "main-notebook" ));
	gtk_notebook_set_tab_pos( notebook, pos );
	fma_iprefs_set_tabs_pos( pos );
}
