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
	gboolean dispose_has_run;
};

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

static void     on_about_button_clicked( GtkButton *button, gpointer user_data );
static void     on_add_button_clicked( GtkButton *button, gpointer user_data );
static void     on_edit_button_clicked( GtkButton *button, gpointer user_data );
static void     on_duplicate_button_clicked( GtkButton *button, gpointer user_data );
static void     on_delete_button_clicked( GtkButton *button, gpointer user_data );
static void     on_import_export_button_clicked( GtkButton *button, gpointer user_data );
static gboolean on_dialog_response( GtkDialog *dialog, gint response_id, BaseWindow *window );

static void     on_actions_changed( NAIPivotContainer *instance, gpointer user_data );

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

	nact_iactions_list_initial_load( NACT_WINDOW( window ));
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

	base_window_connect( window, "AboutButton", "clicked", G_CALLBACK( on_about_button_clicked ));
	base_window_connect( window, "AddActionButton", "clicked", G_CALLBACK( on_add_button_clicked ));
	base_window_connect( window, "EditActionButton", "clicked", G_CALLBACK( on_edit_button_clicked ));
	base_window_connect( window, "DuplicateActionButton", "clicked", G_CALLBACK( on_duplicate_button_clicked ));
	base_window_connect( window, "DeleteActionButton", "clicked", G_CALLBACK( on_delete_button_clicked ));
	base_window_connect( window, "ImExportButton", "clicked", G_CALLBACK( on_import_export_button_clicked ));
}

static void
on_actions_list_selection_changed( GtkTreeSelection *selection, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_actions_list_selection_changed";
	g_debug( "%s: selection=%p, user_data=%p", thisfn, selection, user_data );

	g_assert( BASE_IS_WINDOW( user_data ));
	BaseWindow *window = BASE_WINDOW( user_data );

	GtkWidget *edit_button = base_window_get_widget( window, "EditActionButton" );
	GtkWidget *delete_button = base_window_get_widget( window, "DeleteActionButton" );
	GtkWidget *duplicate_button = base_window_get_widget( window, "DuplicateActionButton" );

	gboolean enabled = ( gtk_tree_selection_count_selected_rows( selection ) > 0 );

	gtk_widget_set_sensitive( edit_button, enabled );
	gtk_widget_set_sensitive( delete_button, enabled );
	gtk_widget_set_sensitive( duplicate_button, enabled );
}

static gboolean
on_actions_list_double_click( GtkWidget *widget, GdkEventButton *event, gpointer user_data )
{
	g_assert( event->type == GDK_2BUTTON_PRESS );
	on_edit_button_clicked( NULL, user_data );
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

	GtkWidget *toplevel;
	g_object_get( G_OBJECT( wndmain ), PROP_WINDOW_TOPLEVEL_WIDGET_STR, &toplevel, NULL );

	gtk_show_about_dialog( GTK_WINDOW( toplevel ),
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

	/* TODO: reset focus to actions list */
}

/*
 * creating a new action
 * pwi 2009-05-19
 * I don't want the profiles feature spread wide while I'm not convinced
 * that it is useful and actually used.
 * so the new action is silently created with a default profile name
 */
static void
on_add_button_clicked( GtkButton *button, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_add_button_clicked";
	g_debug( "%s: button=%p, user_data=%p", thisfn, button, user_data );

	g_assert( NACT_IS_MAIN_WINDOW( user_data ));
	NactWindow *wndmain = NACT_WINDOW( user_data );

	nact_action_conditions_editor_run_editor( wndmain, NULL );

	/* TODO: set the selection to the newly created action
	 * or restore the previous selection - set focus to actions list */
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

	/* TODO: reset the selection to the edited action
	 * set focus to actions list */
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
		}

		g_free( label );

	} else {
		g_assert_not_reached();
	}

	/* TODO: set the selection to the newly created action
	 * set focus to actions list */
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
	/* TODO: set the selection to the previous action if any
	 * or to the next one - set focus to actions list */
}

static void
on_import_export_button_clicked( GtkButton *button, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_import_export_button_clicked";
	g_debug( "%s: button=%p, user_data=%p", thisfn, button, user_data );

	/*GtkWidget *nact_actions_list;

	if (nact_import_export_actions ())
	{
		nact_actions_list = nact_get_glade_widget ("ActionsList");
		fill_actions_list (nact_actions_list);
	}*/
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

			/* FIXME : update pref settings
			nact_prefs_set_main_dialog_size (GTK_WINDOW (dialog));
			nact_prefs_set_main_dialog_position (GTK_WINDOW (dialog));
			nact_prefs_save_preferences ();
			 */

			g_object_unref( window );
			return( TRUE );
			/*gtk_widget_destroy (GTK_WIDGET (dialog));
			nact_destroy_glade_objects ();
			gtk_main_quit ();*/
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
}

/*static gint
count_actions( BaseWindow *window )
{
	NactApplication *appli = NACT_APPLICATION( base_window_get_application( window ));
	NAPivot *pivot = NA_PIVOT( nact_application_get_pivot( appli ));
	GSList *actions = na_pivot_get_actions( pivot );
	return( g_slist_length( actions ));
}*/
