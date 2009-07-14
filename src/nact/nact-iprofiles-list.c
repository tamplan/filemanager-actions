/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009 Pierre Wieser and others (see AUTHORS)
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

#include <gdk/gdkkeysyms.h>

#include <common/na-action.h>
#include <common/na-action-profile.h>

#include "nact-application.h"
#include "nact-iprofiles-list.h"

/* private interface data
 */
struct NactIProfilesListInterfacePrivate {
};

/* column ordering
 */
enum {
	IPROFILES_LIST_LABEL_COLUMN = 0,
	IPROFILES_LIST_PROFILE_COLUMN,
	IPROFILES_LIST_N_COLUMN
};

/* data set against GObject
 */
#define ACCEPT_MULTIPLE_SELECTION		"iprofiles-list-accept-multiple-selection"
#define IS_FILLING_LIST					"iprofiles-list-is-filling-list"
#define SEND_SELECTION_CHANGED_MESSAGE	"iprofiles-list-send-selection-changed-message"

static GType      register_type( void );
static void       interface_base_init( NactIProfilesListInterface *klass );
static void       interface_base_finalize( NactIProfilesListInterface *klass );

static GSList    *v_get_profiles( NactWindow *window );
static void       v_on_selection_changed( GtkTreeSelection *selection, gpointer user_data );
static gboolean   v_on_button_press_event( GtkWidget *widget, GdkEventButton *event, gpointer data );
static gboolean   v_on_key_pressed_event( GtkWidget *widget, GdkEventKey *event, gpointer data );

static GtkWidget *get_profiles_list_widget( NactWindow *window );

GType
nact_iprofiles_list_get_type( void )
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
	static const gchar *thisfn = "nact_iprofiles_list_register_type";
	g_debug( "%s", thisfn );

	static const GTypeInfo info = {
		sizeof( NactIProfilesListInterface ),
		( GBaseInitFunc ) interface_base_init,
		( GBaseFinalizeFunc ) interface_base_finalize,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL
	};

	GType type = g_type_register_static( G_TYPE_INTERFACE, "NactIProfilesList", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( NactIProfilesListInterface *klass )
{
	static const gchar *thisfn = "nact_iprofiles_list_interface_base_init";
	static gboolean initialized = FALSE;

	if( !initialized ){
		g_debug( "%s: klass=%p", thisfn, klass );

		klass->private = g_new0( NactIProfilesListInterfacePrivate, 1 );

		initialized = TRUE;
	}
}

static void
interface_base_finalize( NactIProfilesListInterface *klass )
{
	static const gchar *thisfn = "nact_iprofiles_list_interface_base_finalize";
	static gboolean finalized = FALSE ;

	if( !finalized ){
		g_debug( "%s: klass=%p", thisfn, klass );

		g_free( klass->private );

		finalized = TRUE;
	}
}

/**
 * Allocates and initializes the ActionsList widget.
 */
void
nact_iprofiles_list_initial_load( NactWindow *window )
{
	static const gchar *thisfn = "nact_iprofiles_list_initial_load";
	g_debug( "%s: window=%p", thisfn, window );

	g_assert( NACT_IS_IPROFILES_LIST( window ));
	g_assert( NACT_IS_WINDOW( window ));

	nact_iprofiles_list_set_send_selection_changed_on_fill_list( window, FALSE );
	nact_iprofiles_list_set_is_filling_list( window, FALSE );

	GtkListStore *model;
	GtkTreeViewColumn *column;

	GtkWidget *widget = get_profiles_list_widget( window );

	/* create the model */
	model = gtk_list_store_new( IPROFILES_LIST_N_COLUMN, G_TYPE_STRING, NA_ACTION_PROFILE_TYPE );
	gtk_tree_view_set_model( GTK_TREE_VIEW( widget ), GTK_TREE_MODEL( model ));
	nact_iprofiles_list_fill( window );
	g_object_unref( model );

	/* create visible columns on the tree view */
	column = gtk_tree_view_column_new_with_attributes(
			"label", gtk_cell_renderer_text_new(), "text", IPROFILES_LIST_LABEL_COLUMN, NULL );
	gtk_tree_view_append_column( GTK_TREE_VIEW( widget ), column );
}

/**
 * Allocates and initializes the ProfilesList widget.
 */
void
nact_iprofiles_list_runtime_init( NactWindow *window )
{
	static const gchar *thisfn = "nact_iprofiles_list_do_runtime_init_widget";
	g_debug( "%s: window=%p", thisfn, window );

	g_assert( NACT_IS_IPROFILES_LIST( window ));
	g_assert( NACT_IS_WINDOW( window ));

	GtkWidget *widget = get_profiles_list_widget( window );
	g_assert( GTK_IS_WIDGET( widget ));

	/* set up selection */
	nact_window_signal_connect(
			window,
			G_OBJECT( gtk_tree_view_get_selection( GTK_TREE_VIEW( widget ))),
			"changed",
			G_CALLBACK( v_on_selection_changed ));

	/* catch press 'Enter' */
	nact_window_signal_connect(
			window,
			G_OBJECT( widget ),
			"key-press-event",
			G_CALLBACK( v_on_key_pressed_event ));

	/* catch double-click */
	nact_window_signal_connect(
			window,
			G_OBJECT( widget ),
			"button-press-event",
			G_CALLBACK( v_on_button_press_event ));

	/* clear the selection */
	GtkTreeSelection *selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( widget ));
	gtk_tree_selection_unselect_all( selection );
}

/**
 * Fill the listbox with current profiles.
 */
void
nact_iprofiles_list_fill( NactWindow *window )
{
	static const gchar *thisfn = "nact_iprofiles_list_do_fill_profiles_list";
	g_debug( "%s: window=%p", thisfn, window );

	g_assert( NACT_IS_IPROFILES_LIST( window ));
	g_assert( NACT_IS_WINDOW( window ));

	nact_iprofiles_list_set_is_filling_list( window, TRUE );

	GtkWidget *widget = get_profiles_list_widget( window );
	GtkListStore *model = GTK_LIST_STORE( gtk_tree_view_get_model( GTK_TREE_VIEW( widget )));
	gtk_list_store_clear( model );

	GSList *profiles = v_get_profiles( window );
	GSList *ip;
	/*g_debug( "%s: actions has %d elements", thisfn, g_slist_length( actions ));*/

	for( ip = profiles ; ip ; ip = ip->next ){

		GtkTreeIter iter;
		NAActionProfile *profile = NA_ACTION_PROFILE( ip->data );
		gchar *label = na_action_profile_get_label( profile );

		gtk_list_store_append( model, &iter );
		gtk_list_store_set( model, &iter,
				    IPROFILES_LIST_LABEL_COLUMN, label,
				    IPROFILES_LIST_PROFILE_COLUMN, profile,
				    -1);

		g_free( label );
	}

	nact_iprofiles_list_set_is_filling_list( window, FALSE );
}

/**
 * Reset the focus on the ProfilesList listbox.
 */
void
nact_iprofiles_list_set_focus( NactWindow *window )
{
	GtkWidget *list = get_profiles_list_widget( window );
	gtk_widget_grab_focus( list );
}

/**
 * Returns the currently selected profile.
 */
GObject *
nact_iprofiles_list_get_selected_profile( NactWindow *window )
{
	GtkWidget *treeview = get_profiles_list_widget( window );
	GtkTreeSelection *selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( treeview ));

	GtkTreeModel *model;
	GtkTreeIter iter;
	GObject *profile;

	GList *listrows = gtk_tree_selection_get_selected_rows( selection, &model );
	GtkTreePath *path = ( GtkTreePath * ) listrows->data;
	gtk_tree_model_get_iter( model, &iter, path );

	gtk_tree_model_get( model, &iter, IPROFILES_LIST_PROFILE_COLUMN, &profile, -1 );

	g_list_foreach( listrows, ( GFunc ) gtk_tree_path_free, NULL );
	g_list_free( listrows );

	return( profile );
}

/**
 * Does the IProfilesList box support multiple selection ?
 */
void
nact_iprofiles_list_set_multiple_selection( NactWindow *window, gboolean multiple )
{
	g_assert( NACT_IS_IPROFILES_LIST( window ));
	g_object_set_data( G_OBJECT( window ), ACCEPT_MULTIPLE_SELECTION, GINT_TO_POINTER( multiple ));

	GtkWidget *list = get_profiles_list_widget( window );
	GtkTreeSelection *selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( list ));
	gtk_tree_selection_set_mode( selection, multiple ? GTK_SELECTION_MULTIPLE : GTK_SELECTION_SINGLE );
}

/**
 * Should the IProfilesList interface trigger a 'on_selection_changed'
 * message when the ActionsList list is filled ?
 */
void
nact_iprofiles_list_set_send_selection_changed_on_fill_list( NactWindow *window, gboolean send_message )
{
	g_assert( NACT_IS_IPROFILES_LIST( window ));
	g_object_set_data( G_OBJECT( window ), SEND_SELECTION_CHANGED_MESSAGE, GINT_TO_POINTER( send_message ));
}

/**
 * Is the IProfilesList interface currently filling the ActionsList ?
 */
void
nact_iprofiles_list_set_is_filling_list( NactWindow *window, gboolean is_filling )
{
	g_assert( NACT_IS_IPROFILES_LIST( window ));
	g_object_set_data( G_OBJECT( window ), IS_FILLING_LIST, GINT_TO_POINTER( is_filling ));
}

static GSList *
v_get_profiles( NactWindow *window )
{
	g_assert( NACT_IS_IPROFILES_LIST( window ));
	NactIProfilesList *instance = NACT_IPROFILES_LIST( window );

	if( NACT_IPROFILES_LIST_GET_INTERFACE( instance )->get_profiles ){
		return( NACT_IPROFILES_LIST_GET_INTERFACE( instance )->get_profiles( window ));
	}

	return( NULL );
}

static void
v_on_selection_changed( GtkTreeSelection *selection, gpointer user_data )
{
	g_assert( NACT_IS_IPROFILES_LIST( user_data ));
	g_assert( BASE_IS_WINDOW( user_data ));

	NactIProfilesList *instance = NACT_IPROFILES_LIST( user_data );

	g_assert( NACT_IS_WINDOW( user_data ));
	gboolean send_message = GPOINTER_TO_INT( g_object_get_data( G_OBJECT( user_data ), SEND_SELECTION_CHANGED_MESSAGE ));
	gboolean is_filling = GPOINTER_TO_INT( g_object_get_data( G_OBJECT( user_data ), IS_FILLING_LIST ));

	if( send_message || !is_filling ){
		if( NACT_IPROFILES_LIST_GET_INTERFACE( instance )->on_selection_changed ){
			NACT_IPROFILES_LIST_GET_INTERFACE( instance )->on_selection_changed( selection, user_data );
		}
	}
}

static gboolean
v_on_button_press_event( GtkWidget *widget, GdkEventButton *event, gpointer user_data )
{
	/*static const gchar *thisfn = "nact_iprofiles_list_v_on_button_pres_event";
	g_debug( "%s: widget=%p, event=%p, user_data=%p", thisfn, widget, event, user_data );*/

	g_assert( NACT_IS_IPROFILES_LIST( user_data ));
	g_assert( NACT_IS_WINDOW( user_data ));

	gboolean stop = FALSE;
	NactIProfilesList *instance = NACT_IPROFILES_LIST( user_data );

	if( NACT_IPROFILES_LIST_GET_INTERFACE( instance )->on_button_press_event ){
		stop = NACT_IPROFILES_LIST_GET_INTERFACE( instance )->on_button_press_event( widget, event, user_data );
	}

	if( !stop ){
		if( event->type == GDK_2BUTTON_PRESS ){
			if( NACT_IPROFILES_LIST_GET_INTERFACE( instance )->on_double_click ){
				stop = NACT_IPROFILES_LIST_GET_INTERFACE( instance )->on_double_click( widget, event, user_data );
			}
		}
	}
	return( stop );
}

static gboolean
v_on_key_pressed_event( GtkWidget *widget, GdkEventKey *event, gpointer user_data )
{
	/*static const gchar *thisfn = "nact_iprofiles_list_v_on_key_pressed_event";
	g_debug( "%s: widget=%p, event=%p, user_data=%p", thisfn, widget, event, user_data );*/

	g_assert( NACT_IS_IPROFILES_LIST( user_data ));
	g_assert( NACT_IS_WINDOW( user_data ));
	g_assert( event->type == GDK_KEY_PRESS );

	gboolean stop = FALSE;
	NactIProfilesList *instance = NACT_IPROFILES_LIST( user_data );

	if( NACT_IPROFILES_LIST_GET_INTERFACE( instance )->on_key_pressed_event ){
		stop = NACT_IPROFILES_LIST_GET_INTERFACE( instance )->on_key_pressed_event( widget, event, user_data );
	}

	if( !stop ){
		if( event->keyval == GDK_Return || event->keyval == GDK_KP_Enter ){
			if( NACT_IPROFILES_LIST_GET_INTERFACE( instance )->on_enter_key_pressed ){
				stop = NACT_IPROFILES_LIST_GET_INTERFACE( instance )->on_enter_key_pressed( widget, event, user_data );
			}
		}
	}
	return( stop );
}

static GtkWidget *
get_profiles_list_widget( NactWindow *window )
{
	return( base_window_get_widget( BASE_WINDOW( window ), "ProfilesList" ));
}
