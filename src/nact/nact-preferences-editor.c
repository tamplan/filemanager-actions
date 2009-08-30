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

#include <glib/gi18n.h>

#include <common/na-iprefs.h>

#include "nact-application.h"
#include "nact-iprefs.h"
#include "nact-preferences-editor.h"

/* private class data
 */
struct NactPreferencesEditorClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NactPreferencesEditorPrivate {
	gboolean         dispose_has_run;
	NactWindow      *parent;
};

static GObjectClass *st_parent_class = NULL;

static GType    register_type( void );
static void     class_init( NactPreferencesEditorClass *klass );
static void     iprefs_iface_init( NAIPrefsInterface *iface );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_dispose( GObject *dialog );
static void     instance_finalize( GObject *dialog );

static NactPreferencesEditor *preferences_editor_new( BaseApplication *application );

static gchar   *do_get_iprefs_window_id( NactWindow *window );
static gchar   *do_get_dialog_name( BaseWindow *dialog );
static void     on_initial_load_dialog( BaseWindow *dialog );
static void     on_runtime_init_dialog( BaseWindow *dialog );
static void     on_all_widgets_showed( BaseWindow *dialog );
/*static void     setup_buttons( NactPreferencesEditor *dialog, gboolean is_modified );
static void     on_modified_field( NactWindow *dialog );*/
static void     on_sort_alpha_toggled( GtkToggleButton *button, NactWindow *window );
static void     on_add_about_toggled( GtkToggleButton *button, NactWindow *window );
static void     on_cancel_clicked( GtkButton *button, NactWindow *window );
static void     on_ok_clicked( GtkButton *button, NactWindow *window );
static void     save_preferences( NactPreferencesEditor *editor );
static gboolean on_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window );

GType
nact_preferences_editor_get_type( void )
{
	static GType dialog_type = 0;

	if( !dialog_type ){
		dialog_type = register_type();
	}

	return( dialog_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "nact_preferences_editor_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NactPreferencesEditorClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactPreferencesEditor ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	static const GInterfaceInfo prefs_iface_info = {
		( GInterfaceInitFunc ) iprefs_iface_init,
		NULL,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( NACT_WINDOW_TYPE, "NactPreferencesEditor", &info, 0 );

	g_type_add_interface_static( type, NA_IPREFS_TYPE, &prefs_iface_info );

	return( type );
}

static void
class_init( NactPreferencesEditorClass *klass )
{
	static const gchar *thisfn = "nact_preferences_editor_class_init";
	GObjectClass *object_class;
	BaseWindowClass *base_class;
	NactWindowClass *nact_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactPreferencesEditorClassPrivate, 1 );

	base_class = BASE_WINDOW_CLASS( klass );
	base_class->initial_load_toplevel = on_initial_load_dialog;
	base_class->runtime_init_toplevel = on_runtime_init_dialog;
	base_class->all_widgets_showed = on_all_widgets_showed;
	base_class->dialog_response = on_dialog_response;
	base_class->get_toplevel_name = do_get_dialog_name;

	nact_class = NACT_WINDOW_CLASS( klass );
	nact_class->get_iprefs_window_id = do_get_iprefs_window_id;
}

static void
iprefs_iface_init( NAIPrefsInterface *iface )
{
	static const gchar *thisfn = "nact_preferences_editor_iprefs_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_preferences_editor_instance_init";
	NactPreferencesEditor *self;

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );

	g_assert( NACT_IS_PREFERENCES_EDITOR( instance ));
	self = NACT_PREFERENCES_EDITOR( instance );

	self->private = g_new0( NactPreferencesEditorPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *dialog )
{
	static const gchar *thisfn = "nact_preferences_editor_instance_dispose";
	NactPreferencesEditor *self;

	g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );

	g_assert( NACT_IS_PREFERENCES_EDITOR( dialog ));
	self = NACT_PREFERENCES_EDITOR( dialog );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		G_OBJECT_CLASS( st_parent_class )->dispose( dialog );
	}
}

static void
instance_finalize( GObject *dialog )
{
	static const gchar *thisfn = "nact_preferences_editor_instance_finalize";
	NactPreferencesEditor *self;

	g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );

	g_assert( NACT_IS_PREFERENCES_EDITOR( dialog ));
	self = ( NactPreferencesEditor * ) dialog;

	g_free( self->private );

	/* chain call to parent class */
	if( st_parent_class->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( dialog );
	}
}

/*
 * Returns a newly allocated NactPreferencesEditor object.
 *
 * @parent: the BaseWindow parent of this dialog (usually, the main
 * toplevel window of the application).
 */
static NactPreferencesEditor *
preferences_editor_new( BaseApplication *application )
{
	return( g_object_new( NACT_PREFERENCES_EDITOR_TYPE, PROP_WINDOW_APPLICATION_STR, application, NULL ));
}

/**
 * Initializes and runs the dialog.
 *
 * @parent: the NactWindow parent of this dialog.
 * Usually the NactMainWindow.
 */
void
nact_preferences_editor_run( NactWindow *parent )
{
	static const gchar *thisfn = "nact_preferences_editor_run";
	BaseApplication *application;
	NactPreferencesEditor *editor;

	g_debug( "%s: parent=%p", thisfn, ( void * ) parent );
	g_assert( NACT_IS_WINDOW( parent ));

	application = BASE_APPLICATION( base_window_get_application( BASE_WINDOW( parent )));
	g_assert( NACT_IS_APPLICATION( application ));

	editor = preferences_editor_new( application );
	editor->private->parent = parent;

	base_window_run( BASE_WINDOW( editor ));
}

static gchar *
do_get_iprefs_window_id( NactWindow *window )
{
	return( g_strdup( "preferences-editor" ));
}

static gchar *
do_get_dialog_name( BaseWindow *dialog )
{
	return( g_strdup( "PreferencesDialog" ));
}

static void
on_initial_load_dialog( BaseWindow *dialog )
{
	static const gchar *thisfn = "nact_preferences_editor_on_initial_load_dialog";
	NactPreferencesEditor *editor;
	GtkWindow *toplevel;
	GtkWindow *parent_toplevel;

	/* call parent class at the very beginning */
	if( BASE_WINDOW_CLASS( st_parent_class )->initial_load_toplevel ){
		BASE_WINDOW_CLASS( st_parent_class )->initial_load_toplevel( dialog );
	}

	g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );
	g_assert( NACT_IS_PREFERENCES_EDITOR( dialog ));
	editor = NACT_PREFERENCES_EDITOR( dialog );

	toplevel = base_window_get_toplevel_dialog( dialog );
	parent_toplevel = base_window_get_toplevel_dialog( BASE_WINDOW( editor->private->parent ));
	gtk_window_set_transient_for( toplevel, parent_toplevel );
}

static void
on_runtime_init_dialog( BaseWindow *dialog )
{
	static const gchar *thisfn = "nact_preferences_editor_on_runtime_init_dialog";
	NactPreferencesEditor *editor;
	gboolean sort_alpha, add_about_item;
	GtkWidget *button;

	/* call parent class at the very beginning */
	if( BASE_WINDOW_CLASS( st_parent_class )->runtime_init_toplevel ){
		BASE_WINDOW_CLASS( st_parent_class )->runtime_init_toplevel( dialog );
	}

	g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );
	g_assert( NACT_IS_PREFERENCES_EDITOR( dialog ));
	editor = NACT_PREFERENCES_EDITOR( dialog );

	sort_alpha = na_iprefs_get_alphabetical_order( NA_IPREFS( editor ));
	button = base_window_get_widget( dialog, "SortAlphabeticalButton" );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( button ), sort_alpha );
	nact_window_signal_connect_by_name( NACT_WINDOW( editor ), "SortAlphabeticalButton", "toggled", G_CALLBACK( on_sort_alpha_toggled ));

	add_about_item = na_iprefs_get_add_about_item( NA_IPREFS( editor ));
	button = base_window_get_widget( dialog, "AddAboutButton" );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( button ), add_about_item );
	nact_window_signal_connect_by_name( NACT_WINDOW( editor ), "AddAboutButton", "toggled", G_CALLBACK( on_add_about_toggled ));

	nact_window_signal_connect_by_name( NACT_WINDOW( editor ), "CancelButton", "clicked", G_CALLBACK( on_cancel_clicked ));
	nact_window_signal_connect_by_name( NACT_WINDOW( editor ), "OKButton", "clicked", G_CALLBACK( on_ok_clicked ));

	/*setup_buttons( editor, FALSE );*/
}

static void
on_all_widgets_showed( BaseWindow *dialog )
{
	static const gchar *thisfn = "nact_preferences_editor_on_all_widgets_showed";

	/* call parent class at the very beginning */
	if( BASE_WINDOW_CLASS( st_parent_class )->all_widgets_showed ){
		BASE_WINDOW_CLASS( st_parent_class )->all_widgets_showed( dialog );
	}

	g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );
	g_assert( NACT_IS_PREFERENCES_EDITOR( dialog ));
}

/*
 * rationale:
 * - while the preferences are not modified, only the cancel
 *   button is activated (showed as close)
 * - when at least one preference has been modified, we have a OK and a
 *   cancel buttons
 */
/*static void
setup_buttons( NactPreferencesEditor *editor, gboolean is_modified )
{
	GtkWidget *cancel_stock = gtk_button_new_from_stock( GTK_STOCK_CANCEL );
	GtkWidget *close_stock = gtk_button_new_from_stock( GTK_STOCK_CLOSE );

	GtkWidget *cancel_button = base_window_get_widget( BASE_WINDOW( editor ), "CancelButton" );
	gtk_button_set_label( GTK_BUTTON( cancel_button ), is_modified ? _( "_Cancel" ) : _( "_Close" ));
	gtk_button_set_image( GTK_BUTTON( cancel_button ), is_modified ? gtk_button_get_image( GTK_BUTTON( cancel_stock )) : gtk_button_get_image( GTK_BUTTON( close_stock )));

	gtk_widget_destroy( close_stock );
	gtk_widget_destroy( cancel_stock );

	GtkWidget *ok_button = base_window_get_widget( BASE_WINDOW( editor ), "OKButton" );
	gtk_widget_set_sensitive( ok_button, is_modified );
}*/

/*static void
on_modified_field( NactWindow *window )
{
	static const gchar *thisfn = "nact_preferences_editor_on_modified_field";

	g_assert( NACT_IS_PREFERENCES_EDITOR( window ));
	NactPreferencesEditor *editor = ( NACT_PREFERENCES_EDITOR( window ));

	gboolean is_modified = is_edited_modified( editor );
	setup_dialog_title( editor, is_modified );

	gboolean can_save = is_modified &&
		(( editor->private->show_profile_item && nact_iprofile_item_has_label( window )) ||
				nact_imenu_item_has_label( window ));

	setup_buttons( editor, can_save );
}*/

static void
on_sort_alpha_toggled( GtkToggleButton *button, NactWindow *window )
{
	g_assert( NACT_IS_PREFERENCES_EDITOR( window ));
	/*NactPreferencesEditor *editor = NACT_PREFERENCES_EDITOR( window );*/
}

static void
on_add_about_toggled( GtkToggleButton *button, NactWindow *window )
{
	g_assert( NACT_IS_PREFERENCES_EDITOR( window ));
	/*NactPreferencesEditor *editor = NACT_PREFERENCES_EDITOR( window );*/
}

static void
on_cancel_clicked( GtkButton *button, NactWindow *window )
{
	GtkWindow *toplevel = base_window_get_toplevel_dialog( BASE_WINDOW( window ));
	gtk_dialog_response( GTK_DIALOG( toplevel ), GTK_RESPONSE_CLOSE );
}

static void
on_ok_clicked( GtkButton *button, NactWindow *window )
{
	GtkWindow *toplevel = base_window_get_toplevel_dialog( BASE_WINDOW( window ));
	gtk_dialog_response( GTK_DIALOG( toplevel ), GTK_RESPONSE_OK );
}

static void
save_preferences( NactPreferencesEditor *editor )
{
	GtkWidget *button;
	gboolean enabled;

	button = base_window_get_widget( BASE_WINDOW( editor ), "SortAlphabeticalButton" );
	enabled = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button ));
	na_iprefs_set_bool( NA_IPREFS( editor ), PREFS_DISPLAY_ALPHABETICAL_ORDER, enabled );

	button = base_window_get_widget( BASE_WINDOW( editor ), "AddAboutButton" );
	enabled = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button ));
	na_iprefs_set_bool( NA_IPREFS( editor ), PREFS_ADD_ABOUT_ITEM, enabled );
}

static gboolean
on_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window )
{
	static const gchar *thisfn = "nact_preferences_editor_on_dialog_response";
	NactPreferencesEditor *editor;

	g_debug( "%s: dialog=%p, code=%d, window=%p", thisfn, ( void * ) dialog, code, ( void * ) window );
	g_assert( NACT_IS_PREFERENCES_EDITOR( window ));
	editor = NACT_PREFERENCES_EDITOR( window );

	/*gboolean is_modified = is_edited_modified( editor );*/

	switch( code ){
		case GTK_RESPONSE_NONE:
		case GTK_RESPONSE_DELETE_EVENT:
		case GTK_RESPONSE_CLOSE:
		case GTK_RESPONSE_CANCEL:

			g_object_unref( editor );
			return( TRUE );
			break;

		case GTK_RESPONSE_OK:
			save_preferences( editor );
			g_object_unref( editor );
			return( TRUE );
			break;
	}

	return( FALSE );
}
