/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
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

#include "core/na-gtk-utils.h"

#include "nact-main-window.h"
#include "nact-menu.h"
#include "nact-menu-view.h"

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
	{ TOOLBAR_FILE_ID,  NA_IPREFS_MAIN_TOOLBAR_FILE_DISPLAY,  "toolbar-file",  "file-toolbar",  NULL },
	{ TOOLBAR_EDIT_ID,  NA_IPREFS_MAIN_TOOLBAR_EDIT_DISPLAY,  "toolbar-edit",  "edit-toolbar",  NULL },
	{ TOOLBAR_TOOLS_ID, NA_IPREFS_MAIN_TOOLBAR_TOOLS_DISPLAY, "toolbar-tools", "tools-toolbar", NULL },
	{ TOOLBAR_HELP_ID,  NA_IPREFS_MAIN_TOOLBAR_HELP_DISPLAY,  "toolbar-help",  "help-toolbar",  NULL }
};

static const gchar *st_toolbar_ui       = PKGUIDIR "/nact-toolbar.ui";

static void           setup_toolbars_submenu( NactMainWindow *window );
static void           setup_toolbar( NactMainWindow *window, GtkBuilder *builder, guint toolbar_id );
static sToolbarProps *get_toolbar_properties_by_id( guint toolbar_id );
static sToolbarProps *get_toolbar_properties_by_name( const gchar *action_name );
#if 0
static void           reorder_toolbars( GtkWidget *hbox, sToolbarProps *props );
#endif

/**
 * nact_menu_view_init:
 * @window: the #NactMainWindow main window.
 *
 * Update sensitivity of items of the View menu.
 */
void
nact_menu_view_init( NactMainWindow *window )
{
	setup_toolbars_submenu( window );
}

/**
 * nact_menu_view_update_sensitivities:
 * @window: the #NactMainWindow main window.
 *
 * Update sensitivity of items of the View menu.
 */
void
nact_menu_view_update_sensitivities( NactMainWindow *window )
{
	sMenuData *sdata;
	guint count_list;

	sdata = nact_menu_get_data( window );

	/* expand all/collapse all requires at least one item in the list */
	count_list = sdata->count_menus + sdata->count_actions + sdata->count_profiles;
	nact_menu_enable_item( window, "expand", count_list > 0 );
	nact_menu_enable_item( window, "collapse", count_list > 0 );
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
setup_toolbars_submenu( NactMainWindow *window )
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
setup_toolbar( NactMainWindow *window, GtkBuilder *builder, guint toolbar_id )
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
	is_active = na_settings_get_boolean( props->prefs_key, NULL, NULL );
	if( is_active ){
		action = g_action_map_lookup_action( G_ACTION_MAP( window ), props->action_name );
		g_action_change_state( action, g_variant_new_boolean( is_active ));
	}
}

static sToolbarProps *
get_toolbar_properties_by_id( guint toolbar_id )
{
	static const gchar *thisfn = "nact_menu_view_get_toolbar_properties_by_id";
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
 * nact_menu_view_toolbar_display:
 * @main_window: the #NactMainWindow main window.
 * @action_name: the action name.
 * @visible: whether the toolbar must be displayed or hidden.
 */
void
nact_menu_view_toolbar_display( NactMainWindow *main_window, const gchar *action_name, gboolean visible )
{
	sToolbarProps *props;
	GtkWidget *parent;

	props = get_toolbar_properties_by_name( action_name );
	g_return_if_fail( props );

	parent = na_gtk_utils_find_widget_by_name( GTK_CONTAINER( main_window ), "main-toolbar" );
	g_return_if_fail( parent && GTK_IS_CONTAINER( parent ));

	if( visible ){
		gtk_grid_attach( GTK_GRID( parent ), props->toolbar, props->id-1, 0, 1, 1 );

	} else {
		gtk_container_remove( GTK_CONTAINER( parent ), props->toolbar );
	}

	gtk_widget_show_all( parent );
	na_settings_set_boolean( props->prefs_key, visible );
}

static sToolbarProps *
get_toolbar_properties_by_name( const gchar *action_name )
{
	static const gchar *thisfn = "nact_menu_view_get_toolbar_properties_by_name";
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
 * When activating one of the GtkRadioAction which handles the position
 * of the notebook tabs
 * @action: the first GtkRadioAction of the group
 * @current: the activated GtkRadioAction
 *
 * This function is triggered once each time we are activating an item of
 * the menu, after having set the "current_value" to the new value. All
 * GtkRadioButtons items share the same "current_value".
 */
#if 0
void
nact_menu_view_on_tabs_pos_changed( GtkRadioAction *action, GtkRadioAction *current, BaseWindow *window )
{
	GtkNotebook *notebook;
	guint new_pos;

	notebook = GTK_NOTEBOOK( base_window_get_widget( BASE_WINDOW( window ), "MainNotebook" ));
	new_pos = gtk_radio_action_get_current_value( action );
	gtk_notebook_set_tab_pos( notebook, new_pos );
}
#endif
