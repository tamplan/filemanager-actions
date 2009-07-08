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

#include <stdlib.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <common/na-action.h>
#include <common/na-action-profile.h>
#include <common/na-pivot.h>
#include <common/na-iio-provider.h>
#include <common/na-ipivot-container.h>

#include "nact-application.h"
#include "nact-action-conditions-editor.h"
#include "nact-action-profiles-editor.h"
#include "nact-action-profiles-editor.h"
#include "nact-gconf-schema-writer.h"
#include "nact-iactions-list.h"
#include "nact-iprefs.h"
#include "nact-main-window.h"

/* private class data
 */
struct NactMainWindowClassPrivate {
};

/* private instance data
 */
struct NactMainWindowPrivate {
	gboolean  dispose_has_run;
	gboolean  export_mode;
	gchar    *current_uuid;
	gchar    *current_label;
};

/* the GConf key used to read/write size and position of auxiliary dialogs
 */
#define IPREFS_IMPORT_ACTIONS		"main-window-import-actions"
#define IPREFS_EXPORT_ACTIONS		"main-window-export-actions"

static GObjectClass *st_parent_class = NULL;

static GType    register_type( void );
static void     class_init( NactMainWindowClass *klass );
static void     iactions_list_iface_init( NactIActionsListInterface *iface );
static void     ipivot_container_iface_init( NAIPivotContainerInterface *iface );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_dispose( GObject *application );
static void     instance_finalize( GObject *application );

static gchar   *get_iprefs_window_id( NactWindow *window );
static gchar   *get_toplevel_name( BaseWindow *window );
static void     on_initial_load_toplevel( BaseWindow *window );
static void     on_runtime_init_toplevel( BaseWindow *window );

static void     on_actions_list_selection_changed( GtkTreeSelection *selection, gpointer user_data );
static gboolean on_actions_list_double_click( GtkWidget *widget, GdkEventButton *event, gpointer data );
static gboolean on_actions_list_enter_key_pressed( GtkWidget *widget, GdkEventKey *event, gpointer data );

static void     on_about_button_clicked( GtkButton *button, gpointer user_data );
static void     on_new_button_clicked( GtkButton *button, gpointer user_data );
static void     on_edit_button_clicked( GtkButton *button, gpointer user_data );
static void     on_duplicate_button_clicked( GtkButton *button, gpointer user_data );
static void     on_delete_button_clicked( GtkButton *button, gpointer user_data );
static void     on_import_button_clicked( GtkButton *button, gpointer user_data );
static void     on_export_button_clicked( GtkButton *button, gpointer user_data );
static void     on_saveas_button_clicked( GtkButton *button, gpointer user_data );
static gboolean on_dialog_response( GtkDialog *dialog, gint response_id, BaseWindow *window );

static void     on_actions_changed( NAIPivotContainer *instance, gpointer user_data );

static void     set_current_action( NactMainWindow *window, const NAAction *action );
static void     do_set_current_action( NactWindow *window, const gchar *uuid, const gchar *label );
static void     set_export_mode( NactWindow *window, gboolean mode );
static void     setup_buttons( NactWindow *window );
static void     do_import_actions( NactMainWindow *window, const gchar *filename );
static void     do_export_actions( NactMainWindow *window, const gchar *folder );

/*static gint     count_actions( BaseWindow *window );*/

GType
nact_main_window_get_type( void )
{
	static GType window_type = 0;

	if( !window_type ){
		window_type = register_type();
	}

	return( window_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "nact_main_window_register_type";
	g_debug( "%s", thisfn );

	static GTypeInfo info = {
		sizeof( NactMainWindowClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactMainWindow ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	GType type = g_type_register_static( NACT_WINDOW_TYPE, "NactMainWindow", &info, 0 );

	/* implement IActionsList interface
	 */
	static const GInterfaceInfo iactions_list_iface_info = {
		( GInterfaceInitFunc ) iactions_list_iface_init,
		NULL,
		NULL
	};

	g_type_add_interface_static( type, NACT_IACTIONS_LIST_TYPE, &iactions_list_iface_info );

	/* implement IPivotContainer interface
	 */
	static const GInterfaceInfo pivot_container_iface_info = {
		( GInterfaceInitFunc ) ipivot_container_iface_init,
		NULL,
		NULL
	};

	g_type_add_interface_static( type, NA_IPIVOT_CONTAINER_TYPE, &pivot_container_iface_info );

	return( type );
}

static void
class_init( NactMainWindowClass *klass )
{
	static const gchar *thisfn = "nact_main_window_class_init";
	g_debug( "%s: klass=%p", thisfn, klass );

	st_parent_class = g_type_class_peek_parent( klass );

	GObjectClass *object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactMainWindowClassPrivate, 1 );

	BaseWindowClass *base_class = BASE_WINDOW_CLASS( klass );
	base_class->initial_load_toplevel = on_initial_load_toplevel;
	base_class->runtime_init_toplevel = on_runtime_init_toplevel;
	base_class->dialog_response = on_dialog_response;
	base_class->get_toplevel_name = get_toplevel_name;

	NactWindowClass *nact_class = NACT_WINDOW_CLASS( klass );
	nact_class->get_iprefs_window_id = get_iprefs_window_id;
	nact_class->set_current_action = do_set_current_action;
}

static void
iactions_list_iface_init( NactIActionsListInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_iactions_list_iface_init";
	g_debug( "%s: iface=%p", thisfn, iface );

	iface->initial_load_widget = NULL;
	iface->runtime_init_widget = NULL;
	iface->on_selection_changed = on_actions_list_selection_changed;
	iface->on_double_click = on_actions_list_double_click;
	iface->on_enter_key_pressed = on_actions_list_enter_key_pressed;
}

static void
ipivot_container_iface_init( NAIPivotContainerInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_ipivot_container_iface_init";
	g_debug( "%s: iface=%p", thisfn, iface );

	iface->on_actions_changed = on_actions_changed;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_main_window_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );

	g_assert( NACT_IS_MAIN_WINDOW( instance ));
	NactMainWindow *self = NACT_MAIN_WINDOW( instance );

	self->private = g_new0( NactMainWindowPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *window )
{
	static const gchar *thisfn = "nact_main_window_instance_dispose";
	g_debug( "%s: window=%p", thisfn, window );

	g_assert( NACT_IS_MAIN_WINDOW( window ));
	NactMainWindow *self = NACT_MAIN_WINDOW( window );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		g_free( self->private->current_uuid );
		g_free( self->private->current_label );

		/* chain up to the parent class */
		G_OBJECT_CLASS( st_parent_class )->dispose( window );
	}
}

static void
instance_finalize( GObject *window )
{
	static const gchar *thisfn = "nact_main_window_instance_finalize";
	g_debug( "%s: window=%p", thisfn, window );

	g_assert( NACT_IS_MAIN_WINDOW( window ));
	NactMainWindow *self = ( NactMainWindow * ) window;

	g_free( self->private );

	/* chain call to parent class */
	if( st_parent_class->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( window );
	}
}

/**
 * Returns a newly allocated NactMainWindow object.
 */
NactMainWindow *
nact_main_window_new( GObject *application )
{
	g_assert( NACT_IS_APPLICATION( application ));

	return( g_object_new( NACT_MAIN_WINDOW_TYPE, PROP_WINDOW_APPLICATION_STR, application, NULL ));
}

static gchar *
get_iprefs_window_id( NactWindow *window )
{
	return( g_strdup( "main-window" ));
}

static gchar *
get_toplevel_name( BaseWindow *window )
{
	return( g_strdup( "ActionsDialog" ));
}

static void
on_initial_load_toplevel( BaseWindow *window )
{
	static const gchar *thisfn = "nact_main_window_on_initial_load_toplevel";

	/* call parent class at the very beginning */
	if( BASE_WINDOW_CLASS( st_parent_class )->initial_load_toplevel ){
		BASE_WINDOW_CLASS( st_parent_class )->initial_load_toplevel( window );
	}

	g_debug( "%s: window=%p", thisfn, window );
	g_assert( NACT_IS_MAIN_WINDOW( window ));
	/*NactMainWindow *wnd = NACT_MAIN_WINDOW( window );*/

	g_assert( NACT_IS_IACTIONS_LIST( window ));
	nact_iactions_list_initial_load( NACT_WINDOW( window ));
	nact_iactions_list_set_multiple_selection( NACT_WINDOW( window ), FALSE );
	nact_iactions_list_set_send_selection_changed_on_fill_list( NACT_WINDOW( window ), FALSE );
}

static void
on_runtime_init_toplevel( BaseWindow *window )
{
	static const gchar *thisfn = "nact_main_window_on_runtime_init_toplevel";

	/* call parent class at the very beginning */
	if( BASE_WINDOW_CLASS( st_parent_class )->runtime_init_toplevel ){
		BASE_WINDOW_CLASS( st_parent_class )->runtime_init_toplevel( window );
	}

	g_debug( "%s: window=%p", thisfn, window );
	g_assert( NACT_IS_MAIN_WINDOW( window ));
	/*NactMainWindow *wnd = NACT_MAIN_WINDOW( window );*/

	nact_iactions_list_runtime_init( NACT_WINDOW( window ));

	nact_window_signal_connect_by_name( NACT_WINDOW( window ), "AboutButton", "clicked", G_CALLBACK( on_about_button_clicked ));
	nact_window_signal_connect_by_name( NACT_WINDOW( window ), "NewActionButton", "clicked", G_CALLBACK( on_new_button_clicked ));
	nact_window_signal_connect_by_name( NACT_WINDOW( window ), "EditActionButton", "clicked", G_CALLBACK( on_edit_button_clicked ));
	nact_window_signal_connect_by_name( NACT_WINDOW( window ), "DuplicateActionButton", "clicked", G_CALLBACK( on_duplicate_button_clicked ));
	nact_window_signal_connect_by_name( NACT_WINDOW( window ), "DeleteActionButton", "clicked", G_CALLBACK( on_delete_button_clicked ));
	nact_window_signal_connect_by_name( NACT_WINDOW( window ), "ImportButton", "clicked", G_CALLBACK( on_import_button_clicked ));
	nact_window_signal_connect_by_name( NACT_WINDOW( window ), "ExportButton", "clicked", G_CALLBACK( on_export_button_clicked ));
	nact_window_signal_connect_by_name( NACT_WINDOW( window ), "SaveAsButton", "clicked", G_CALLBACK( on_saveas_button_clicked ));

	setup_buttons( NACT_WINDOW( window ));
}

static void
on_actions_list_selection_changed( GtkTreeSelection *selection, gpointer user_data )
{
	/*static const gchar *thisfn = "nact_main_window_on_actions_list_selection_changed";
	g_debug( "%s: selection=%p, user_data=%p", thisfn, selection, user_data );*/

	g_assert( NACT_IS_MAIN_WINDOW( user_data ));
	BaseWindow *window = BASE_WINDOW( user_data );

	if( !NACT_MAIN_WINDOW( window )->private->export_mode ){

		GtkWidget *edit_button = base_window_get_widget( window, "EditActionButton" );
		GtkWidget *delete_button = base_window_get_widget( window, "DeleteActionButton" );
		GtkWidget *duplicate_button = base_window_get_widget( window, "DuplicateActionButton" );

		gboolean enabled = ( gtk_tree_selection_count_selected_rows( selection ) > 0 );

		gtk_widget_set_sensitive( edit_button, enabled );
		gtk_widget_set_sensitive( delete_button, enabled );
		gtk_widget_set_sensitive( duplicate_button, enabled );

		NAAction *action = NA_ACTION( nact_iactions_list_get_selected_action( NACT_WINDOW( window )));
		set_current_action( NACT_MAIN_WINDOW( window ), action );
	}
}

static gboolean
on_actions_list_double_click( GtkWidget *widget, GdkEventButton *event, gpointer user_data )
{
	g_assert( event->type == GDK_2BUTTON_PRESS );

	if( !NACT_MAIN_WINDOW( user_data )->private->export_mode ){
		on_edit_button_clicked( NULL, user_data );
	}

	return( TRUE );
}

static gboolean
on_actions_list_enter_key_pressed( GtkWidget *widget, GdkEventKey *event, gpointer user_data )
{
	if( !NACT_MAIN_WINDOW( user_data )->private->export_mode ){
		on_edit_button_clicked( NULL, user_data );
	}

	return( TRUE );
}

/* TODO: make the website url and the mail addresses clickables
 */
static void
on_about_button_clicked( GtkButton *button, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_about_button_clicked";
	g_debug( "%s: button=%p, user_data=%p", thisfn, button, user_data );

	g_assert( BASE_IS_WINDOW( user_data ));
	BaseWindow *wndmain = BASE_WINDOW( user_data );

	BaseApplication *appli;
	g_object_get( G_OBJECT( wndmain ), PROP_WINDOW_APPLICATION_STR, &appli, NULL );
	gchar *icon_name = base_application_get_icon_name( appli );

	static const gchar *artists[] = {
		N_( "Ulisse Perusin <uli.peru@gmail.com>" ),
		NULL
	};

	static const gchar *authors[] = {
		N_( "Frederic Ruaudel <grumz@grumz.net>" ),
		N_( "Rodrigo Moya <rodrigo@gnome-db.org>" ),
		N_( "Pierre Wieser <pwieser@trychlos.org>" ),
		NULL
	};

	static const gchar *documenters[] = {
		NULL
	};

	static gchar *license[] = {
		N_( "Nautilus Actions Configuration Tool is free software; you can "
			"redistribute it and/or modify it under the terms of the GNU General "
			"Public License as published by the Free Software Foundation; either "
			"version 2 of the License, or (at your option) any later version." ),
		N_( "Nautilus Actions Configuration Tool is distributed in the hope that it "
			"will be useful, but WITHOUT ANY WARRANTY; without even the implied "
			"warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See "
			"the GNU General Public License for more details." ),
		N_( "You should have received a copy of the GNU General Public License along "
			"with Nautilus Actions Configuration Tool ; if not, write to the Free "
			"Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, "
			"MA 02110-1301, USA." ),
		NULL
	};
	gchar *license_i18n = g_strjoinv( "\n\n", license );

	GtkWindow *toplevel = base_window_get_toplevel_dialog( wndmain );

	gtk_show_about_dialog( toplevel,
			"artists", artists,
			"authors", authors,
			"comments", _( "A graphical tool to create and edit your Nautilus actions." ),
			"copyright", _( "Copyright \xc2\xa9 2005-2007 Frederic Ruaudel <grumz@grumz.net>\nCopyright \xc2\xa9 2009 Pierre Wieser <pwieser@trychlos.org>" ),
			"documenters", documenters,
			"translator-credits", _( "The GNOME Translation Project <gnome-i18n@gnome.org>" ),
			"license", license_i18n,
			"wrap-license", TRUE,
			"logo-icon-name", icon_name,
			"version", PACKAGE_VERSION,
			"website", "http://www.nautilus-actions.org",
			NULL );

	g_free( license_i18n );
	g_free( icon_name );

	nact_iactions_list_set_focus( NACT_WINDOW( wndmain ));
}

/*
 * creating a new action
 * pwi 2009-05-19
 * I don't want the profiles feature spread wide while I'm not convinced
 * that it is useful and actually used.
 * so the new action is silently created with a default profile name
 */
static void
on_new_button_clicked( GtkButton *button, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_new_button_clicked";
	g_debug( "%s: button=%p, user_data=%p", thisfn, button, user_data );

	g_assert( NACT_IS_MAIN_WINDOW( user_data ));
	NactWindow *wndmain = NACT_WINDOW( user_data );

	nact_action_conditions_editor_run_editor( wndmain, NULL );

	nact_iactions_list_set_focus( wndmain );
}

/*
 * editing an existing action
 * pwi 2009-05-19
 * I don't want the profiles feature spread wide while I'm not convinced
 * that it is useful and actually used.
 * so :
 * - if there is only one profile, the user will be directed to a dialog
 *   box which includes all needed fields, but without any profile notion
 * - if there are more than one profile, one can assume that the user has
 *   found a use to the profiles, and let him edit them
 */
static void
on_edit_button_clicked( GtkButton *button, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_edit_button_clicked";
	g_debug( "%s: button=%p, user_data=%p", thisfn, button, user_data );

	g_assert( NACT_IS_MAIN_WINDOW( user_data ));
	NactWindow *wndmain = NACT_WINDOW( user_data );

	NAAction *action = NA_ACTION( nact_iactions_list_get_selected_action( wndmain ));

	if( action ){
		guint count = na_action_get_profiles_count( action );

		if( count > 1 ){
			nact_action_profiles_editor_run_editor( wndmain, action );

		} else {
			nact_action_conditions_editor_run_editor( wndmain, action );
		}

	} else {
		g_assert_not_reached();
	}

	nact_iactions_list_set_focus( wndmain );
}

static void
on_duplicate_button_clicked( GtkButton *button, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_duplicate_button_clicked";
	g_debug( "%s: button=%p, user_data=%p", thisfn, button, user_data );

	g_assert( NACT_IS_MAIN_WINDOW( user_data ));
	NactWindow *wndmain = NACT_WINDOW( user_data );

	NAAction *action = NA_ACTION( nact_iactions_list_get_selected_action( wndmain ));
	if( action ){

		NAAction *duplicate = na_action_duplicate( action );
		na_action_set_new_uuid( duplicate );
		gchar *label = na_action_get_label( action );
		gchar *label2 = g_strdup_printf( _( "Copy of %s"), label );
		na_action_set_label( duplicate, label2 );
		g_free( label2 );

		gchar *msg = NULL;
		NAPivot *pivot = NA_PIVOT( nact_window_get_pivot( wndmain ));
		if( na_pivot_write_action( pivot, G_OBJECT( duplicate ), &msg ) != NA_IIO_PROVIDER_WRITE_OK ){

			gchar *first = g_strdup_printf( _( "Unable to duplicate \"%s\" action." ), label );
			base_window_error_dlg( BASE_WINDOW( wndmain ), GTK_MESSAGE_ERROR, first, msg );
			g_free( first );
			g_free( msg );

		} else {
			set_current_action( NACT_MAIN_WINDOW( wndmain ), duplicate );
		}

		g_object_unref( duplicate );
		g_free( label );

	} else {
		g_assert_not_reached();
	}

	nact_iactions_list_set_focus( wndmain );
}

static void
on_delete_button_clicked( GtkButton *button, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_delete_button_clicked";
	g_debug( "%s: button=%p, user_data=%p", thisfn, button, user_data );

	g_assert( NACT_IS_MAIN_WINDOW( user_data ));
	NactWindow *wndmain = NACT_WINDOW( user_data );

	NAAction *action = NA_ACTION( nact_iactions_list_get_selected_action( wndmain ));
	if( action ){

		gchar *label = na_action_get_label( action );
		gchar *sure = g_strdup_printf( _( "Are you sure you want to delete \"%s\" action ?" ), label );
		if( base_window_yesno_dlg( BASE_WINDOW( wndmain ), GTK_MESSAGE_WARNING, sure, NULL )){

			gchar *msg = NULL;
			NAPivot *pivot = NA_PIVOT( nact_window_get_pivot( wndmain ));
			if( na_pivot_delete_action( pivot, G_OBJECT( action ), &msg ) != NA_IIO_PROVIDER_WRITE_OK ){

				gchar *first = g_strdup_printf( _( "Unable to delete \"%s\" action." ), label );
				base_window_error_dlg( BASE_WINDOW( wndmain ), GTK_MESSAGE_ERROR, first, msg );
				g_free( first );
				g_free( msg );
			}
		}
		g_free( sure );
		g_free( label );

	} else {
		g_assert_not_reached();
	}

	nact_iactions_list_set_focus( wndmain );
}

static void
on_import_button_clicked( GtkButton *button, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_import_button_clicked";
	g_debug( "%s: button=%p, user_data=%p", thisfn, button, user_data );

	g_assert( NACT_IS_MAIN_WINDOW( user_data ));
	NactWindow *wndmain = NACT_WINDOW( user_data );

	GtkWidget *dialog = gtk_file_chooser_dialog_new(
			_( "Importing new actions" ),
			NULL,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			NULL
			);

	nact_iprefs_position_named_window( NACT_WINDOW( user_data ), GTK_WINDOW( dialog ), IPREFS_IMPORT_ACTIONS );
	gchar *uri = nact_iprefs_get_import_folder_uri( NACT_WINDOW( user_data ));
	gtk_file_chooser_set_current_folder_uri( GTK_FILE_CHOOSER( dialog ), uri );
	g_free( uri );

	if( gtk_dialog_run( GTK_DIALOG( dialog )) == GTK_RESPONSE_ACCEPT ){
		gchar *filename = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER( dialog ));
		do_import_actions( NACT_MAIN_WINDOW( wndmain ), filename );
	    g_free (filename);
	  }

	uri = gtk_file_chooser_get_current_folder_uri( GTK_FILE_CHOOSER( dialog ));
	nact_iprefs_save_import_folder_uri( NACT_WINDOW( user_data ), uri );
	g_free( uri );

	nact_iprefs_save_named_window_position( NACT_WINDOW( user_data ), GTK_WINDOW( dialog ), IPREFS_IMPORT_ACTIONS );

	gtk_widget_destroy( dialog );

	nact_iactions_list_set_focus( wndmain );
}

/*
 * ExportButton is a toggle button
 * When activated (selection-for-export mode), all other buttons are
 * disabled, but 'SaveAs' ; the ActionsList accept multiple selection
 */
static void
on_export_button_clicked( GtkButton *button, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_export_button_clicked";
	g_debug( "%s: button=%p, user_data=%p", thisfn, button, user_data );

	gboolean export = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button ));
	set_export_mode( NACT_WINDOW( user_data ), export );

	g_assert( NACT_IS_MAIN_WINDOW( user_data ));
	NactWindow *wndmain = NACT_WINDOW( user_data );

	nact_iactions_list_set_focus( wndmain );
}

static void
on_saveas_button_clicked( GtkButton *button, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_saveas_button_clicked";
	g_debug( "%s: button=%p, user_data=%p", thisfn, button, user_data );

	g_assert( NACT_IS_MAIN_WINDOW( user_data ));
	NactWindow *wndmain = NACT_WINDOW( user_data );

	GtkWidget *dialog = gtk_file_chooser_dialog_new(
			_( "Selecting a folder in which selected actions are to be saved" ),
			NULL,
			GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK, GTK_RESPONSE_OK,
			NULL
			);

	nact_iprefs_position_named_window( wndmain, GTK_WINDOW( dialog ), IPREFS_EXPORT_ACTIONS );
	gchar *uri = nact_iprefs_get_export_folder_uri( wndmain );
	gtk_file_chooser_set_uri( GTK_FILE_CHOOSER( dialog ), uri );
	g_free( uri );

	if( gtk_dialog_run( GTK_DIALOG( dialog )) == GTK_RESPONSE_OK ){
		uri = gtk_file_chooser_get_uri( GTK_FILE_CHOOSER( dialog ));
		do_export_actions( NACT_MAIN_WINDOW( wndmain ), uri );
	  }

	nact_iprefs_save_export_folder_uri( wndmain, uri );
	g_free( uri );

	nact_iprefs_save_named_window_position( wndmain, GTK_WINDOW( dialog ), IPREFS_EXPORT_ACTIONS );
	gtk_widget_destroy( dialog );

	GtkWidget *export_button = base_window_get_widget( BASE_WINDOW( user_data ), "ExportButton" );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( export_button ), FALSE );
}

static gboolean
on_dialog_response( GtkDialog *dialog, gint response_id, BaseWindow *window )
{
	static const gchar *thisfn = "nact_main_window_on_dialog_response";
	g_debug( "%s: dialog=%p, response_id=%d, window=%p", thisfn, dialog, response_id, window );
	g_assert( NACT_IS_MAIN_WINDOW( window ));

	/*GtkWidget *paste_button = nact_get_glade_widget_from ("PasteProfileButton", GLADE_EDIT_DIALOG_WIDGET);*/

	switch( response_id ){
		case GTK_RESPONSE_NONE:
		case GTK_RESPONSE_DELETE_EVENT:
		case GTK_RESPONSE_CLOSE:
			/* Free any profile in the clipboard */
			/*nautilus_actions_config_action_profile_free (g_object_steal_data (G_OBJECT (paste_button), "profile"));*/

			g_object_unref( window );
			return( TRUE );
			break;
	}

	return( FALSE );
}

static void
on_actions_changed( NAIPivotContainer *instance, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_actions_changed";
	g_debug( "%s: instance=%p, user_data=%p", thisfn, instance, user_data );

	g_assert( NACT_IS_MAIN_WINDOW( instance ));
	NactMainWindow *self = NACT_MAIN_WINDOW( instance );

	if( !self->private->dispose_has_run ){
		nact_iactions_list_fill( NACT_WINDOW( instance ));
	}

	nact_iactions_list_set_selection(
			NACT_WINDOW( self ), self->private->current_uuid, self->private->current_label );
}

static void
set_current_action( NactMainWindow *window, const NAAction *action )
{
	g_free( window->private->current_uuid );
	window->private->current_uuid = NULL;

	g_free( window->private->current_label );
	window->private->current_label = NULL;

	if( action ){
		g_assert( NA_IS_ACTION( action ));
		window->private->current_uuid = na_action_get_uuid( action );
		window->private->current_label = na_action_get_label( action );
	}
}

static void
do_set_current_action( NactWindow *window, const gchar *uuid, const gchar *label )
{
	g_debug( "nact_main_window_do_set_current_action: window=%p, uuid=%s, label=%s", window, uuid, label );
	g_free( NACT_MAIN_WINDOW( window )->private->current_uuid );
	g_free( NACT_MAIN_WINDOW( window )->private->current_label );
	NACT_MAIN_WINDOW( window )->private->current_uuid = g_strdup( uuid );
	NACT_MAIN_WINDOW( window )->private->current_label = g_strdup( label );
}

static void
set_export_mode( NactWindow *window, gboolean mode )
{
	g_assert( NACT_IS_MAIN_WINDOW( window ));
	NactMainWindow *self = NACT_MAIN_WINDOW( window );

	nact_iactions_list_set_multiple_selection( window, mode );
	self->private->export_mode = mode;
	setup_buttons( window );
}

static void
setup_buttons( NactWindow *window )
{
	g_assert( NACT_IS_MAIN_WINDOW( window ));
	NactMainWindow *self = NACT_MAIN_WINDOW( window );

	GtkWidget *new_button = base_window_get_widget( BASE_WINDOW( window ), "NewActionButton" );
	GtkWidget *edit_button = base_window_get_widget( BASE_WINDOW( window ), "EditActionButton" );
	GtkWidget *duplicate_button = base_window_get_widget( BASE_WINDOW( window ), "DuplicateActionButton" );
	GtkWidget *delete_button = base_window_get_widget( BASE_WINDOW( window ), "DeleteActionButton" );
	GtkWidget *import_button = base_window_get_widget( BASE_WINDOW( window ), "ImportButton" );
	GtkWidget *saveas_button = base_window_get_widget( BASE_WINDOW( window ), "SaveAsButton" );
	GtkWidget *close_button = base_window_get_widget( BASE_WINDOW( window ), "CloseButton" );

	gtk_widget_set_sensitive( new_button, !self->private->export_mode );
	gtk_widget_set_sensitive( edit_button, !self->private->export_mode );
	gtk_widget_set_sensitive( delete_button, !self->private->export_mode );
	gtk_widget_set_sensitive( duplicate_button, !self->private->export_mode );
	gtk_widget_set_sensitive( import_button, !self->private->export_mode );
	gtk_widget_set_sensitive( close_button, !self->private->export_mode );

	gtk_widget_set_sensitive( saveas_button, self->private->export_mode );

	GtkWidget *label = base_window_get_widget( BASE_WINDOW( window ), "ExportModeLabel" );
	gchar *text = g_strdup( "" );
	if( self->private->export_mode ){
		g_free( text );
		text = g_strdup( _( "Export mode toggled.\n"
				"Please, select actions to be exported (multiple selection is authorized).\n" ));
	}
	gtk_label_set_label( GTK_LABEL( label ), text );
	g_free( text );
}

static void
do_import_actions( NactMainWindow *window, const gchar *filename )
{
	static const gchar *thisfn = "nact_main_window_do_import_actions";
	g_debug( "%s: window=%p, filename=%p", thisfn, window, filename );
}

static void
do_export_actions( NactMainWindow *window, const gchar *folder )
{
	static const gchar *thisfn = "nact_main_window_do_export_actions";
	g_debug( "%s: window=%p, folder=%p", thisfn, window, folder );

	GSList *actions = nact_iactions_list_get_selected_actions( NACT_WINDOW( window ));
	GSList *ia;
	gchar *msg = NULL;
	gchar *reason = NULL;
	gchar *tmp;

	for( ia = actions ; ia ; ia = ia->next ){
		NAAction *action = NA_ACTION( ia->data );
		nact_gconf_schema_writer_export( action, folder, &msg );
		if( msg ){
			if( reason ){
				tmp = g_strdup_printf( "%s\n", reason );
				g_free( reason );
				reason = tmp;
			}
			tmp = g_strdup_printf( "%s%s", reason, msg );
			g_free( reason );
			reason = tmp;
		}
		g_free( msg );
	}

	if( reason ){
		base_window_error_dlg( BASE_WINDOW( window ), GTK_MESSAGE_WARNING,
				_( "One or more errors have been detected when exporting actions." ), reason );
		g_free( reason );
	}

	g_slist_free( actions );
}

/*static gint
count_actions( BaseWindow *window )
{
	NactApplication *appli = NACT_APPLICATION( base_window_get_application( window ));
	NAPivot *pivot = NA_PIVOT( nact_application_get_pivot( appli ));
	GSList *actions = na_pivot_get_actions( pivot );
	return( g_slist_length( actions ));
}*/
