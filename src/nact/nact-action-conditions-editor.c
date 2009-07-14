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

#include <common/na-action.h>

#include "nact-application.h"
#include "nact-action-conditions-editor.h"
#include "nact-imenu-item.h"
#include "nact-iprofile-conditions.h"
#include "nact-iprefs.h"
#include "nact-main-window.h"

/* private class data
 */
struct NactActionConditionsEditorClassPrivate {
};

/* private instance data
 */
struct NactActionConditionsEditorPrivate {
	gboolean    dispose_has_run;
	NactWindow *parent;
	NAAction   *original;
	NAAction   *edited;
	gboolean    is_new;
};

static GObjectClass *st_parent_class = NULL;

static GType    register_type( void );
static void     class_init( NactActionConditionsEditorClass *klass );
static void     imenu_item_iface_init( NactIMenuItemInterface *iface );
static void     iprofile_conditions_iface_init( NactIProfileConditionsInterface *iface );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_dispose( GObject *dialog );
static void     instance_finalize( GObject *dialog );

static NactActionConditionsEditor *action_conditions_editor_new( BaseApplication *application );

static gchar   *do_get_iprefs_window_id( NactWindow *window );
static gchar   *do_get_dialog_name( BaseWindow *dialog );
static void     on_initial_load_dialog( BaseWindow *dialog );
static void     on_runtime_init_dialog( BaseWindow *dialog );
static void     on_all_widgets_showed( BaseWindow *dialog );
static void     setup_dialog_title( NactActionConditionsEditor *dialog, gboolean is_modified );
static void     setup_buttons( NactActionConditionsEditor *dialog, gboolean can_save );
static void     on_modified_field( NactWindow *dialog );
static void     on_cancel_clicked( GtkButton *button, gpointer user_data );
static void     on_save_clicked( GtkButton *button, gpointer user_data );
static gboolean on_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window );

static GObject *get_edited_action( NactWindow *window );
static GObject *get_edited_profile( NactWindow *window );
static gboolean is_edited_modified( NactActionConditionsEditor *dialog );

GType
nact_action_conditions_editor_get_type( void )
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
	static const gchar *thisfn = "nact_action_conditions_editor_register_type";
	g_debug( "%s", thisfn );

	static GTypeInfo info = {
		sizeof( NactActionConditionsEditorClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactActionConditionsEditor ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	GType type = g_type_register_static( NACT_WINDOW_TYPE, "NactActionConditionsEditor", &info, 0 );

	/* implement IMenuItem interface
	 */
	static const GInterfaceInfo imenu_item_iface_info = {
		( GInterfaceInitFunc ) imenu_item_iface_init,
		NULL,
		NULL
	};

	g_type_add_interface_static( type, NACT_IMENU_ITEM_TYPE, &imenu_item_iface_info );

	/* implement IProfileConditions interface
	 */
	static const GInterfaceInfo iprofile_conditions_iface_info = {
		( GInterfaceInitFunc ) iprofile_conditions_iface_init,
		NULL,
		NULL
	};

	g_type_add_interface_static( type, NACT_IPROFILE_CONDITIONS_TYPE, &iprofile_conditions_iface_info );

	return( type );
}

static void
class_init( NactActionConditionsEditorClass *klass )
{
	static const gchar *thisfn = "nact_action_conditions_editor_class_init";
	g_debug( "%s: klass=%p", thisfn, klass );

	st_parent_class = g_type_class_peek_parent( klass );

	GObjectClass *object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactActionConditionsEditorClassPrivate, 1 );

	BaseWindowClass *base_class = BASE_WINDOW_CLASS( klass );
	base_class->initial_load_toplevel = on_initial_load_dialog;
	base_class->runtime_init_toplevel = on_runtime_init_dialog;
	base_class->all_widgets_showed = on_all_widgets_showed;
	base_class->dialog_response = on_dialog_response;
	base_class->get_toplevel_name = do_get_dialog_name;

	NactWindowClass *nact_class = NACT_WINDOW_CLASS( klass );
	nact_class->get_iprefs_window_id = do_get_iprefs_window_id;
}

static void
imenu_item_iface_init( NactIMenuItemInterface *iface )
{
	static const gchar *thisfn = "nact_action_conditions_editor_imenu_item_iface_init";
	g_debug( "%s: iface=%p", thisfn, iface );

	iface->get_edited_action = get_edited_action;
	iface->field_modified = on_modified_field;
}

static void
iprofile_conditions_iface_init( NactIProfileConditionsInterface *iface )
{
	static const gchar *thisfn = "nact_action_conditions_editor_iprofile_conditions_iface_init";
	g_debug( "%s: iface=%p", thisfn, iface );

	iface->get_edited_profile = get_edited_profile;
	iface->field_modified = on_modified_field;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_action_conditions_editor_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );

	g_assert( NACT_IS_ACTION_CONDITIONS_EDITOR( instance ));
	NactActionConditionsEditor *self = NACT_ACTION_CONDITIONS_EDITOR( instance );

	self->private = g_new0( NactActionConditionsEditorPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->original = NULL;
	self->private->edited = NULL;
}

static void
instance_dispose( GObject *dialog )
{
	static const gchar *thisfn = "nact_action_conditions_editor_instance_dispose";
	g_debug( "%s: dialog=%p", thisfn, dialog );

	g_assert( NACT_IS_ACTION_CONDITIONS_EDITOR( dialog ));
	NactActionConditionsEditor *self = NACT_ACTION_CONDITIONS_EDITOR( dialog );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		nact_imenu_item_dispose( NACT_WINDOW( dialog ));
		nact_iprofile_conditions_dispose( NACT_WINDOW( dialog ));

		g_object_unref( self->private->original );
		g_object_unref( self->private->edited );

		/* chain up to the parent class */
		G_OBJECT_CLASS( st_parent_class )->dispose( dialog );
	}
}

static void
instance_finalize( GObject *dialog )
{
	static const gchar *thisfn = "nact_action_conditions_editor_instance_finalize";
	g_debug( "%s: dialog=%p", thisfn, dialog );

	g_assert( NACT_IS_ACTION_CONDITIONS_EDITOR( dialog ));
	NactActionConditionsEditor *self = ( NactActionConditionsEditor * ) dialog;

	g_free( self->private );

	/* chain call to parent class */
	if( st_parent_class->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( dialog );
	}
}

/*
 * Returns a newly allocated NactActionConditionsEditor object.
 *
 * @parent: the BaseWindow parent of this dialog (usually, the main
 * toplevel window of the application).
 */
static NactActionConditionsEditor *
action_conditions_editor_new( BaseApplication *application )
{
	return( g_object_new( NACT_ACTION_CONDITIONS_EDITOR_TYPE, PROP_WINDOW_APPLICATION_STR, application, NULL ));
}

/**
 * Initializes and runs the dialog.
 *
 * @parent: is the BaseWindow parent of this dialog (usually, the main
 * toplevel window of the application).
 *
 * @user_data: a pointer to the NAAction to edit, or NULL. If NULL, a
 * new NAAction is created.
 */
void
nact_action_conditions_editor_run_editor( NactWindow *parent, gpointer user_data )
{
	static const gchar *thisfn = "nact_action_conditions_editor_run_editor";
	g_debug( "%s: parent=%p, user_data=%p", thisfn, parent, user_data );

	g_assert( NACT_IS_MAIN_WINDOW( parent ));

	BaseApplication *application = BASE_APPLICATION( base_window_get_application( BASE_WINDOW( parent )));
	g_assert( NACT_IS_APPLICATION( application ));

	NactActionConditionsEditor *dialog = action_conditions_editor_new( application );
	dialog->private->parent = parent;

	g_assert( NA_IS_ACTION( user_data ) || !user_data );
	NAAction *action = NA_ACTION( user_data );

	if( !action ){
		dialog->private->original = na_action_new_with_profile();
		dialog->private->is_new = TRUE;

	} else {
		dialog->private->original = na_action_duplicate( action );
		dialog->private->is_new = FALSE;
	}

	dialog->private->edited = na_action_duplicate( dialog->private->original );

	g_assert( na_action_get_profiles_count( dialog->private->original ) == 1 );
	g_assert( na_action_get_profiles_count( dialog->private->edited ) == 1 );

	base_window_run( BASE_WINDOW( dialog ));
}

static gchar *
do_get_iprefs_window_id( NactWindow *window )
{
	return( g_strdup( "action-conditions-editor" ));
}

static gchar *
do_get_dialog_name( BaseWindow *dialog )
{
	/*g_debug( "nact_action_conditions_editor_do_get_dialog_name" );*/
	return( g_strdup( "EditActionDialogExt" ));
}

static void
on_initial_load_dialog( BaseWindow *dialog )
{
	static const gchar *thisfn = "nact_action_conditions_editor_on_initial_load_dialog";

	/* call parent class at the very beginning */
	if( BASE_WINDOW_CLASS( st_parent_class )->initial_load_toplevel ){
		BASE_WINDOW_CLASS( st_parent_class )->initial_load_toplevel( dialog );
	}

	g_debug( "%s: dialog=%p", thisfn, dialog );
	g_assert( NACT_IS_ACTION_CONDITIONS_EDITOR( dialog ));
	NactActionConditionsEditor *window = NACT_ACTION_CONDITIONS_EDITOR( dialog );

	nact_imenu_item_initial_load( NACT_WINDOW( window ), window->private->edited );

	NAActionProfile *profile = NA_ACTION_PROFILE( na_action_get_profiles( window->private->edited )->data );
	nact_iprofile_conditions_initial_load( NACT_WINDOW( window ), profile );

	/* label alignements */
	/*GtkSizeGroup *label_group = gtk_size_group_new( GTK_SIZE_GROUP_HORIZONTAL );
	nact_imenu_item_size_labels( NACT_WINDOW( window ), G_OBJECT( label_group ));
	nact_iprofile_conditions_size_labels( NACT_WINDOW( window ), G_OBJECT( label_group ));
	g_object_unref( label_group );*/

	/* buttons size
	 * nb: while label sizing group works well with Glade 3.3 and GtkBuilder,
	 * it doesn't with button size - so sizing them by code
	 */
	GtkSizeGroup *button_group = gtk_size_group_new( GTK_SIZE_GROUP_HORIZONTAL );
	nact_imenu_item_size_buttons( NACT_WINDOW( window ), G_OBJECT( button_group ));
	nact_iprofile_conditions_size_buttons( NACT_WINDOW( window ), G_OBJECT( button_group ));
	g_object_unref( button_group );
}

static void
on_runtime_init_dialog( BaseWindow *dialog )
{
	static const gchar *thisfn = "nact_action_conditions_editor_on_runtime_init_dialog";

	/* call parent class at the very beginning */
	if( BASE_WINDOW_CLASS( st_parent_class )->runtime_init_toplevel ){
		BASE_WINDOW_CLASS( st_parent_class )->runtime_init_toplevel( dialog );
	}

	g_debug( "%s: dialog=%p", thisfn, dialog );
	g_assert( NACT_IS_ACTION_CONDITIONS_EDITOR( dialog ));
	NactActionConditionsEditor *window = NACT_ACTION_CONDITIONS_EDITOR( dialog );

	setup_dialog_title( window, FALSE );

	nact_imenu_item_runtime_init( NACT_WINDOW( window ), window->private->edited );

	/*na_object_dump( NA_OBJECT( window->private->edited ));*/
	NAActionProfile *profile = NA_ACTION_PROFILE( na_action_get_profiles( window->private->edited )->data );
	nact_iprofile_conditions_runtime_init( NACT_WINDOW( window ), profile );

	nact_window_signal_connect_by_name( NACT_WINDOW( window ), "CancelButton", "clicked", G_CALLBACK( on_cancel_clicked ));
	nact_window_signal_connect_by_name( NACT_WINDOW( window ), "SaveButton", "clicked", G_CALLBACK( on_save_clicked ));
}

static void
on_all_widgets_showed( BaseWindow *dialog )
{
	static const gchar *thisfn = "nact_action_conditions_editor_on_all_widgets_showed";

	/* call parent class at the very beginning */
	if( BASE_WINDOW_CLASS( st_parent_class )->all_widgets_showed ){
		BASE_WINDOW_CLASS( st_parent_class )->all_widgets_showed( dialog );
	}

	g_debug( "%s: dialog=%p", thisfn, dialog );

	GtkNotebook *notebook = GTK_NOTEBOOK( base_window_get_widget( dialog, "notebook2" ));
	gtk_notebook_set_current_page( notebook, 0 );

	nact_imenu_item_all_widgets_showed( NACT_WINDOW( dialog ));
	nact_iprofile_conditions_all_widgets_showed( NACT_WINDOW( dialog ));
}

static void
setup_dialog_title( NactActionConditionsEditor *dialog, gboolean is_modified )
{
	GtkWindow *toplevel = base_window_get_toplevel_dialog( BASE_WINDOW( dialog ));

	gchar *title;
	if( dialog->private->is_new ){
		title = g_strdup( _( "Adding a new action" ));
	} else {
		gchar *label = na_action_get_label( dialog->private->original );
		title = g_strdup_printf( _( "Editing \"%s\" action" ), label );
		g_free( label );
	}

	if( is_modified ){
		gchar *tmp = g_strdup_printf( "*%s", title );
		g_free( title );
		title = tmp;
	}

	gtk_window_set_title( toplevel, title );
	g_free( title );
}

/*
 * rationale:
 * while the action is not modified, only the cancel button is activated
 * when the action has been modified, we have a save and a cancel buttons
 * + a label is mandatory to enable the save button
 */
static void
setup_buttons( NactActionConditionsEditor *dialog, gboolean can_save )
{
	GtkWidget *cancel_button = gtk_button_new_from_stock( GTK_STOCK_CANCEL );
	GtkWidget *close_button = gtk_button_new_from_stock( GTK_STOCK_CLOSE );
	GtkWidget *button = base_window_get_widget( BASE_WINDOW( dialog ), "CancelButton" );
	gtk_button_set_label( GTK_BUTTON( button ), can_save ? _( "_Cancel" ) : _( "_Close" ));
	gtk_button_set_image( GTK_BUTTON( button ), can_save ? gtk_button_get_image( GTK_BUTTON( cancel_button )) : gtk_button_get_image( GTK_BUTTON( close_button )));
	gtk_widget_destroy( cancel_button );
	gtk_widget_destroy( close_button );

	button = base_window_get_widget( BASE_WINDOW( dialog ), "SaveButton" );
	gtk_widget_set_sensitive( button, can_save );
}

static void
on_modified_field( NactWindow *window )
{
	/*static const gchar *thisfn = "nact_action_conditions_editor_on_modified_field";*/

	g_assert( NACT_IS_ACTION_CONDITIONS_EDITOR( window ));
	NactActionConditionsEditor *dialog = ( NACT_ACTION_CONDITIONS_EDITOR( window ));

	gboolean is_modified = is_edited_modified( dialog );
	/*g_debug( "%s: is_modified=%s", thisfn, is_modified ? "True":"False" );*/
	setup_dialog_title( dialog, is_modified );

	gboolean can_save = is_modified && nact_imenu_item_has_label( window );
	setup_buttons( dialog, can_save );
}

static void
on_cancel_clicked( GtkButton *button, gpointer user_data )
{
	GtkWindow *toplevel = base_window_get_toplevel_dialog( BASE_WINDOW( user_data ));
	gtk_dialog_response( GTK_DIALOG( toplevel ), GTK_RESPONSE_CLOSE );
}

static void
on_save_clicked( GtkButton *button, gpointer user_data )
{
	GtkWindow *toplevel = base_window_get_toplevel_dialog( BASE_WINDOW( user_data ));
	gtk_dialog_response( GTK_DIALOG( toplevel ), GTK_RESPONSE_OK );
}

static gboolean
on_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window )
{
	static const gchar *thisfn = "nact_action_conditions_editor_on_dialog_response";
	g_debug( "%s: dialog=%p, code=%d, window=%p", thisfn, dialog, code, window );

	g_assert( NACT_IS_ACTION_CONDITIONS_EDITOR( window ));
	NactActionConditionsEditor *editor = NACT_ACTION_CONDITIONS_EDITOR( window );

	gboolean is_modified = is_edited_modified( editor );

	switch( code ){
		case GTK_RESPONSE_NONE:
		case GTK_RESPONSE_DELETE_EVENT:
		case GTK_RESPONSE_CLOSE:
		case GTK_RESPONSE_CANCEL:
			if( !is_modified ||
				nact_window_warn_action_modified( NACT_WINDOW( editor ), editor->private->original )){
					g_object_unref( window );
					return( TRUE );
			}
			break;

		case GTK_RESPONSE_OK:
			if( is_modified ){
				if( nact_window_save_action( NACT_WINDOW( editor ), editor->private->edited )){
					g_object_unref( editor->private->original );
					editor->private->original = na_action_duplicate( editor->private->edited );
					editor->private->is_new = FALSE;
					on_modified_field( NACT_WINDOW( editor ));
					nact_window_set_current_action( editor->private->parent, editor->private->original );
				}
			}
			break;
	}

	return( FALSE );
}

static GObject *
get_edited_action( NactWindow *window )
{
	g_assert( NACT_IS_ACTION_CONDITIONS_EDITOR( window ));
	return( G_OBJECT( NACT_ACTION_CONDITIONS_EDITOR( window )->private->edited ));
}

static GObject *
get_edited_profile( NactWindow *window )
{
	g_assert( NACT_IS_ACTION_CONDITIONS_EDITOR( window ));
	return( G_OBJECT( na_action_get_profiles( NACT_ACTION_CONDITIONS_EDITOR( window )->private->edited )->data ));
}

static gboolean
is_edited_modified( NactActionConditionsEditor *dialog )
{
	return( !na_action_are_equal( dialog->private->original, dialog->private->edited ));
}
