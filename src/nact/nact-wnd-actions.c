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
#include <gtk/gtkbutton.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkmessagedialog.h>
#include <gtk/gtktreeview.h>
#include <glade/glade-xml.h>

#include <common/na-action.h>
#include <common/na-action-profile.h>
#include <common/na-pivot.h>

#include "nact-application.h"
#include "nact-wnd-actions.h"
#include "nact-iactions-list.h"

#include "nact-editor.h"
#include "nact-import-export.h"
#include "nact-prefs.h"
#include "nact-action-editor.h"
#include "nact-utils.h"

/* private class data
 */
struct NactWndActionsClassPrivate {
};

/* private instance data
 */
struct NactWndActionsPrivate {
	gboolean         dispose_has_run;
};

static GObjectClass    *st_parent_class = NULL;

/*static NactApplication *st_application = NULL;
static NAPivot         *st_pivot = NULL;*/

static GType    register_type( void );
static void     class_init( NactWndActionsClass *klass );
static void     iactions_list_iface_init( NactIActionsListInterface *iface );
static void     instance_init( GTypeInstance *instance, gpointer klass );
/*static void     instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void     instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );*/
static void     instance_dispose( GObject *application );
static void     instance_finalize( GObject *application );

static void     init_window( BaseWindow *window );
static void     on_actions_list_selection_changed( GtkTreeSelection *selection, gpointer user_data );
static gboolean on_actions_list_double_click( GtkWidget *widget, GdkEventButton *event, gpointer data );

#if(( GTK_MAJOR_VERSION == 2 ) && ( GTK_MINOR_VERSION == 4 ))
 static void   init_edit_button_widget( BaseWindow *window );
#endif
#if((( GTK_MAJOR_VERSION == 2 ) && ( GTK_MINOR_VERSION >= 6 )) || ( GTK_MAJOR_VERSION > 2 ))
 static void   init_about_button_widget( BaseWindow *window );
#endif

static void     on_add_button_clicked( GtkButton *button, gpointer user_data );
static void     on_edit_button_clicked( GtkButton *button, gpointer user_data );
static void     on_duplicate_button_clicked( GtkButton *button, gpointer user_data );
static void     on_delete_button_clicked( GtkButton *button, gpointer user_data );
static void     on_import_export_button_clicked( GtkButton *button, gpointer user_data );
static void     on_dialog_response( GtkDialog *dialog, gint response_id, gpointer user_data );

GType
nact_wnd_actions_get_type( void )
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
	static const gchar *thisfn = "nact_wnd_actions_register_type";
	g_debug( "%s", thisfn );

	g_type_init();

	static GTypeInfo info = {
		sizeof( NactWndActionsClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactWndActions ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	GType type = g_type_register_static( NACT_WINDOW_TYPE, "NactWndActions", &info, 0 );

	static const GInterfaceInfo iactions_list_iface_info = {
		( GInterfaceInitFunc ) iactions_list_iface_init,
		NULL,
		NULL
	};

	g_type_add_interface_static( type, NACT_IACTIONS_LIST_TYPE, &iactions_list_iface_info );

	return( type );
}

static void
class_init( NactWndActionsClass *klass )
{
	static const gchar *thisfn = "nact_wnd_actions_class_init";
	g_debug( "%s: klass=%p", thisfn, klass );

	st_parent_class = g_type_class_peek_parent( klass );

	GObjectClass *object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
	/*object_class->get_property = instance_get_property;
	object_class->set_property = instance_set_property;*/

	klass->private = g_new0( NactWndActionsClassPrivate, 1 );

	BaseWindowClass *base_class = BASE_WINDOW_CLASS( klass );
	base_class->init_window = init_window;
}

static void
iactions_list_iface_init( NactIActionsListInterface *iface )
{
	static const gchar *thisfn = "nact_wnd_actions_iactions_list_iface_init";
	g_debug( "%s: iface=%p", thisfn, iface );

	iface->init_widget = NULL;
	iface->on_selection_changed = on_actions_list_selection_changed;
	iface->on_double_click = on_actions_list_double_click;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_wnd_actions_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );

	g_assert( NACT_IS_WND_ACTIONS( instance ));
	NactWndActions *self = NACT_WND_ACTIONS( instance );

	self->private = g_new0( NactWndActionsPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

/*static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	g_assert( NACT_IS_WND_ACTIONS( object ));
	NactWndActions *self = NACT_WND_ACTIONS( object );

	switch( property_id ){
		case PROP_ARGC:
			g_value_set_int( value, self->private->argc );
			break;

		case PROP_ARGV:
			g_value_set_pointer( value, self->private->argv );
			break;

		case PROP_UNIQUE_NAME:
			g_value_set_string( value, self->private->unique_name );
			break;

		case PROP_UNIQUE_APP:
			g_value_set_pointer( value, self->private->unique_app );
			break;

		case PROP_MAIN_WINDOW:
			g_value_set_pointer( value, self->private->main_window );
			break;

		case PROP_DLG_NAME:
			g_value_set_string( value, self->private->application_name );
			break;

		case PROP_ICON_NAME:
			g_value_set_string( value, self->private->icon_name );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	g_assert( NACT_IS_WND_ACTIONS( object ));
	NactWndActions *self = NACT_WND_ACTIONS( object );

	switch( property_id ){
		case PROP_ARGC:
			self->private->argc = g_value_get_int( value );
			break;

		case PROP_ARGV:
			self->private->argv = g_value_get_pointer( value );
			break;

		case PROP_UNIQUE_NAME:
			g_free( self->private->unique_name );
			self->private->unique_name = g_value_dup_string( value );
			break;

		case PROP_UNIQUE_APP:
			self->private->unique_app = g_value_get_pointer( value );
			break;

		case PROP_MAIN_WINDOW:
			self->private->main_window = g_value_get_pointer( value );
			break;

		case PROP_DLG_NAME:
			g_free( self->private->application_name );
			self->private->application_name = g_value_dup_string( value );
			break;

		case PROP_ICON_NAME:
			g_free( self->private->icon_name );
			self->private->icon_name = g_value_dup_string( value );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}*/

static void
instance_dispose( GObject *window )
{
	static const gchar *thisfn = "nact_wnd_actions_instance_dispose";
	g_debug( "%s: window=%p", thisfn, window );

	g_assert( NACT_IS_WND_ACTIONS( window ));
	NactWndActions *self = NACT_WND_ACTIONS( window );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		G_OBJECT_CLASS( st_parent_class )->dispose( window );
	}
}

static void
instance_finalize( GObject *window )
{
	static const gchar *thisfn = "nact_wnd_actions_instance_finalize";
	g_debug( "%s: window=%p", thisfn, window );

	g_assert( NACT_IS_WND_ACTIONS( window ));
	/*NactWndActions *self = ( NactWndActions * ) window;*/

	/* chain call to parent class */
	if( st_parent_class->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( window );
	}
}

/**
 * Returns a newly allocated NactWndActions object.
 */
NactWndActions *
nact_wnd_actions_new( GObject *application )
{
	g_assert( NACT_IS_APPLICATION( application ));

	return( g_object_new( NACT_WND_ACTIONS_TYPE, PROP_WINDOW_APPLICATION_STR, application, NULL ));
}

static void
init_window( BaseWindow *window )
{
	static const gchar *thisfn = "nact_wnd_actions_init_window";
	g_debug( "%s: window=%p", thisfn, window );

	g_assert( NACT_IS_WND_ACTIONS( window ));
	/*NactWndActions *wnd = NACT_WND_ACTIONS( window );*/

	GtkWidget *dialog = base_window_load_widget( window, "ActionsDialog" );

	nact_iactions_list_init( window );

#if(( GTK_MAJOR_VERSION == 2 ) && ( GTK_MINOR_VERSION == 4 ))
	/* Fix a stock icon bug with GTK+ 2.4 */
	init_edit_button_widget( window );
#endif

#if((( GTK_MAJOR_VERSION == 2 ) && ( GTK_MINOR_VERSION >= 6 )) || ( GTK_MAJOR_VERSION > 2 ))
	/* Add about dialog for GTK+ >= 2.6 */
	init_about_button_widget( window );
#endif

	base_window_connect( window, "AddActionButton", "clicked", G_CALLBACK( on_add_button_clicked ));
	base_window_connect( window, "EditActionButton", "clicked", G_CALLBACK( on_edit_button_clicked ));
	base_window_connect( window, "DuplicateActionButton", "clicked", G_CALLBACK( on_duplicate_button_clicked ));
	base_window_connect( window, "DeleteActionButton", "clicked", G_CALLBACK( on_delete_button_clicked ));
	base_window_connect( window, "ImExportButton", "clicked", G_CALLBACK( on_import_export_button_clicked ));

	/* display the dialog */
	g_signal_connect( G_OBJECT( dialog ), "response", G_CALLBACK( on_dialog_response ), window );
	gtk_widget_show( dialog );
}

#if(( GTK_MAJOR_VERSION == 2 ) && ( GTK_MINOR_VERSION == 4 ))
static void
init_edit_button_widget( BaseWindow *window )
{
	/* Fix a stock icon bug with GTK+ 2.4 */
	GtkWidget *button = base_window_get_widget( window, "EditActionButton" );
	gtk_button_set_use_stock( GTK_BUTTON( button ), FALSE );
	gtk_button_set_use_underline( GTK_BUTTON( button ), TRUE );
	/* i18n: "Edit" action button label forced for Gtk 2.4 */
	gtk_button_set_label( GTK_BUTTON( button ), _( "_Edit" ));
}
#endif

#if((( GTK_MAJOR_VERSION == 2 ) && ( GTK_MINOR_VERSION >= 6 )) || ( GTK_MAJOR_VERSION > 2 ))
static void
init_about_button_widget( BaseWindow *window )
{
	/* Add about dialog for GTK+ >= 2.6 */
	GtkWidget *button = base_window_get_widget( window, "AboutButton" );
	gtk_widget_show( button );
}
#endif

static void
on_actions_list_selection_changed( GtkTreeSelection *selection, gpointer user_data )
{
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
	static const gchar *thisfn = "nact_wnd_actions_on_add_button_clicked";
	g_debug( "%s: button=%p, user_data=%p", thisfn, button, user_data );

	if( nact_action_editor_new()){
		nact_iactions_list_fill( BASE_WINDOW( user_data ));
	}
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
	static const gchar *thisfn = "nact_wnd_actions_on_edit_button_clicked";
	g_debug( "%s: button=%p, user_data=%p", thisfn, button, user_data );

	/*GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkWidget *nact_actions_list;
	GtkTreeModel* model;*/

	/*NAAction *action = nact_iactions_list_get_selected_action( BASE_WINDOW( user_data ));*/

	/*nact_actions_list = nact_get_glade_widget ("ActionsList");

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (nact_actions_list));

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gchar *uuid;
		NAAction *action;

		gtk_tree_model_get (model, &iter, IACTIONS_LIST_UUID_COLUMN, &uuid, -1);

		action = NA_ACTION( na_pivot_get_action( st_pivot, uuid ));*/

		/*NautilusActionsConfigAction *action;
		if( action ){
			guint count = na_action_get_profiles_count( action );
			if( count > 1 ){
				if (nact_editor_edit_action (( NautilusActionsConfigAction *) action))
					fill_actions_list (nact_actions_list);
			} else {
				if( nact_action_editor_edit( ( NautilusActionsConfigAction *) action ))
					fill_actions_list( nact_actions_list );
			}
		}*/

		/*g_free (uuid);
	}*/
}

static void
on_duplicate_button_clicked( GtkButton *button, gpointer user_data )
{
	static const gchar *thisfn = "nact_wnd_actions_on_duplicate_button_clicked";
	g_debug( "%s: button=%p, user_data=%p", thisfn, button, user_data );

	/*GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkWidget *nact_actions_list;
	GtkTreeModel* model;
	gchar *error = NULL;
	gchar *tmp, *label;

	nact_actions_list = nact_get_glade_widget ("ActionsList");

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (nact_actions_list));

	if (gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		gchar *uuid;
		NAAction *action;
		NAAction* new_action;

		gtk_tree_model_get (model, &iter, IACTIONS_LIST_UUID_COLUMN, &uuid, -1);

		action = NA_ACTION( na_pivot_get_action( st_pivot, uuid ));
		new_action = na_action_duplicate( action );
		na_action_set_new_uuid( new_action );*/

		/*if( nautilus_actions_config_add_action( NAUTILUS_ACTIONS_CONFIG (config), new_action, &error )){*/
		/*if( na_pivot_write_action( st_pivot, G_OBJECT( new_action ), &error )){
			fill_actions_list (nact_actions_list);
		} else {*/
			/* i18n notes: will be displayed in a dialog */
			/*label = na_action_get_label( action );
			tmp = g_strdup_printf (_("Can't duplicate action '%s'!"), label);
			nautilus_actions_display_error( tmp, error );
			g_free( error );
			g_free( label );
			g_free( tmp );
		}

		g_free( uuid );
	}*/
}

static void
on_delete_button_clicked( GtkButton *button, gpointer user_data )
{
	static const gchar *thisfn = "nact_wnd_actions_on_delete_button_clicked";
	g_debug( "%s: button=%p, user_data=%p", thisfn, button, user_data );

	/*GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkWidget *nact_actions_list;
	GtkTreeModel* model;

	nact_actions_list = nact_get_glade_widget ("ActionsList");

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (nact_actions_list));

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gchar *uuid;

		gtk_tree_model_get (model, &iter, IACTIONS_LIST_UUID_COLUMN, &uuid, -1);*/
		/*nautilus_actions_config_remove_action (NAUTILUS_ACTIONS_CONFIG (config), uuid);*/
		/*fill_actions_list (nact_actions_list);

		g_free (uuid);
	}*/
}

static void
on_import_export_button_clicked( GtkButton *button, gpointer user_data )
{
	static const gchar *thisfn = "nact_wnd_actions_on_import_export_button_clicked";
	g_debug( "%s: button=%p, user_data=%p", thisfn, button, user_data );

	/*GtkWidget *nact_actions_list;

	if (nact_import_export_actions ())
	{
		nact_actions_list = nact_get_glade_widget ("ActionsList");
		fill_actions_list (nact_actions_list);
	}*/
}

static void
on_dialog_response( GtkDialog *dialog, gint response_id, gpointer user_data )
{
	GtkWidget *about_dialog;
	BaseWindow *window;
	/*GtkWidget *paste_button = nact_get_glade_widget_from ("PasteProfileButton", GLADE_EDIT_DIALOG_WIDGET);*/
	g_debug( "dialog_response_cb: response_id=%d", response_id );

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

			window = BASE_WINDOW( g_object_get_data( G_OBJECT( dialog ), "base-window" ));
			g_object_unref( window );
			/*gtk_widget_destroy (GTK_WIDGET (dialog));
			nact_destroy_glade_objects ();
			gtk_main_quit ();*/
			break;

		case GTK_RESPONSE_HELP:
#if((( GTK_MAJOR_VERSION == 2 ) && ( GTK_MINOR_VERSION >= 6 )) || ( GTK_MAJOR_VERSION > 2 ))
			about_dialog = nact_get_glade_widget_from ("AboutDialog", GLADE_ABOUT_DIALOG_WIDGET);
			gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (about_dialog), PACKAGE_VERSION);
			gtk_about_dialog_set_logo_icon_name (GTK_ABOUT_DIALOG (about_dialog), "nautilus-actions");
			gtk_dialog_run (GTK_DIALOG (about_dialog));
			gtk_widget_hide (about_dialog);
#endif
			break;
	}
}
