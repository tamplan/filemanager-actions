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
#include <common/na-action-profile.h>

#include "nact-action-profiles-editor.h"
#include "nact-profile-conditions-editor.h"
#include "nact-imenu-item.h"
#include "nact-iprofiles-list.h"

/* private class data
 */
struct NactActionProfilesEditorClassPrivate {
};

/* private instance data
 */
struct NactActionProfilesEditorPrivate {
	gboolean    dispose_has_run;
	NAAction   *original;
	NAAction   *edited;
	gboolean    is_new;
};

static GObjectClass *st_parent_class = NULL;

static GType    register_type( void );
static void     class_init( NactActionProfilesEditorClass *klass );
static void     imenu_item_iface_init( NactIMenuItemInterface *iface );
static void     iprofiles_list_iface_init( NactIProfilesListInterface *iface );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_dispose( GObject *dialog );
static void     instance_finalize( GObject *dialog );

static NactActionProfilesEditor *action_profiles_editor_new( NactWindow *parent );

static gchar   *do_get_iprefs_window_id( NactWindow *window );
static gchar   *do_get_dialog_name( BaseWindow *dialog );
static void     on_initial_load_dialog( BaseWindow *dialog );
static void     on_runtime_init_dialog( BaseWindow *dialog );
static void     on_all_widgets_showed( BaseWindow *dialog );
static void     setup_dialog_title( NactActionProfilesEditor *dialog, gboolean is_modified );
static void     setup_buttons( NactActionProfilesEditor *dialog, gboolean can_save );

static void     on_profiles_list_selection_changed( GtkTreeSelection *selection, gpointer user_data );
static gboolean on_profiles_list_double_click( GtkWidget *widget, GdkEventButton *event, gpointer data );
static gboolean on_profiles_list_enter_key_pressed( GtkWidget *widget, GdkEventKey *event, gpointer data );
static void     on_modified_field( NactWindow *dialog );
static void     on_new_button_clicked( GtkButton *button, gpointer user_data );
static void     on_edit_button_clicked( GtkButton *button, gpointer user_data );
static void     on_duplicate_button_clicked( GtkButton *button, gpointer user_data );
static void     on_delete_button_clicked( GtkButton *button, gpointer user_data );
static void     on_cancel_clicked( GtkButton *button, gpointer user_data );
static gboolean on_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window );

static GSList  *do_get_profiles( NactWindow *window );
static GObject *get_edited_action( NactWindow *window );
static gboolean is_edited_modified( NactActionProfilesEditor *dialog );
/*static void     do_set_current_profile( NactActionProfilesEditor *dialog, const NAActionProfile *profile );*/

GType
nact_action_profiles_editor_get_type( void )
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
	static const gchar *thisfn = "nact_action_profiles_editor_register_type";
	g_debug( "%s", thisfn );

	static GTypeInfo info = {
		sizeof( NactActionProfilesEditorClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactActionProfilesEditor ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	GType type = g_type_register_static( NACT_WINDOW_TYPE, "NactActionProfilesEditor", &info, 0 );

	/* implement IMenuItem interface
	 */
	static const GInterfaceInfo imenu_item_iface_info = {
		( GInterfaceInitFunc ) imenu_item_iface_init,
		NULL,
		NULL
	};

	g_type_add_interface_static( type, NACT_IMENU_ITEM_TYPE, &imenu_item_iface_info );

	/* implement IProfilesList interface
	 */
	static const GInterfaceInfo iprofiles_list_iface_info = {
		( GInterfaceInitFunc ) iprofiles_list_iface_init,
		NULL,
		NULL
	};

	g_type_add_interface_static( type, NACT_IPROFILES_LIST_TYPE, &iprofiles_list_iface_info );

	return( type );
}

static void
class_init( NactActionProfilesEditorClass *klass )
{
	static const gchar *thisfn = "nact_action_profiles_editor_class_init";
	g_debug( "%s: klass=%p", thisfn, klass );

	st_parent_class = g_type_class_peek_parent( klass );

	GObjectClass *object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactActionProfilesEditorClassPrivate, 1 );

	BaseWindowClass *base_class = BASE_WINDOW_CLASS( klass );
	base_class->get_toplevel_name = do_get_dialog_name;
	base_class->initial_load_toplevel = on_initial_load_dialog;
	base_class->runtime_init_toplevel = on_runtime_init_dialog;
	base_class->all_widgets_showed = on_all_widgets_showed;
	base_class->dialog_response = on_dialog_response;

	NactWindowClass *nact_class = NACT_WINDOW_CLASS( klass );
	nact_class->get_iprefs_window_id = do_get_iprefs_window_id;
}

static void
imenu_item_iface_init( NactIMenuItemInterface *iface )
{
	static const gchar *thisfn = "nact_action_profiles_editor_imenu_item_iface_init";
	g_debug( "%s: iface=%p", thisfn, iface );

	iface->get_edited_action = get_edited_action;
	iface->field_modified = on_modified_field;
}

static void
iprofiles_list_iface_init( NactIProfilesListInterface *iface )
{
	static const gchar *thisfn = "nact_action_profiles_editor_iprofiles_list_iface_init";
	g_debug( "%s: iface=%p", thisfn, iface );

	iface->get_profiles = do_get_profiles;
	iface->on_selection_changed = on_profiles_list_selection_changed;
	iface->on_double_click = on_profiles_list_double_click;
	iface->on_enter_key_pressed = on_profiles_list_enter_key_pressed;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_action_profiles_editor_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );

	g_assert( NACT_IS_ACTION_PROFILES_EDITOR( instance ));
	NactActionProfilesEditor *self = NACT_ACTION_PROFILES_EDITOR( instance );

	self->private = g_new0( NactActionProfilesEditorPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->original = NULL;
	self->private->edited = NULL;
}

static void
instance_dispose( GObject *dialog )
{
	static const gchar *thisfn = "nact_action_profiles_editor_instance_dispose";
	g_debug( "%s: dialog=%p", thisfn, dialog );

	g_assert( NACT_IS_ACTION_PROFILES_EDITOR( dialog ));
	NactActionProfilesEditor *self = NACT_ACTION_PROFILES_EDITOR( dialog );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		nact_imenu_item_dispose( NACT_WINDOW( dialog ));

		g_object_unref( self->private->original );
		g_object_unref( self->private->edited );

		/* chain up to the parent class */
		G_OBJECT_CLASS( st_parent_class )->dispose( dialog );
	}
}

static void
instance_finalize( GObject *dialog )
{
	static const gchar *thisfn = "nact_action_profiles_editor_instance_finalize";
	g_debug( "%s: dialog=%p", thisfn, dialog );

	g_assert( NACT_IS_ACTION_PROFILES_EDITOR( dialog ));
	NactActionProfilesEditor *self = ( NactActionProfilesEditor * ) dialog;

	g_free( self->private );

	/* chain call to parent class */
	if( st_parent_class->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( dialog );
	}
}

/*
 * Returns a newly allocated NactActionProfilesEditor object.
 *
 * @parent: the BaseWindow parent of this dialog (usually, the main
 * toplevel window of the application).
 */
static NactActionProfilesEditor *
action_profiles_editor_new( NactWindow *parent )
{
	return( g_object_new( NACT_ACTION_PROFILES_EDITOR_TYPE, PROP_WINDOW_PARENT_STR, parent, NULL ));
}

/**
 * Initializes and runs the dialog.
 *
 * @parent: is the BaseWindow parent of this dialog (usually, the main
 * toplevel window of the application).
 *
 * @user_data: a pointer to the NAAction to edit, or NULL. If NULL, a
 * new NAAction is created.
 *
 * Returns TRUE if the NAAction has been edited and saved, or FALSE if
 * there has been no modification at all.
 */
void
nact_action_profiles_editor_run_editor( NactWindow *parent, gpointer user_data )
{
	NactActionProfilesEditor *dialog = action_profiles_editor_new( parent );

	g_assert( NA_IS_ACTION( user_data ));
	NAAction *action = NA_ACTION( user_data );
	g_assert( na_action_get_profiles_count( action ) > 1 );

	dialog->private->original = na_action_duplicate( action );
	dialog->private->edited = na_action_duplicate( dialog->private->original );

	base_window_run( BASE_WINDOW( dialog ));
}

static gchar *
do_get_iprefs_window_id( NactWindow *window )
{
	return( g_strdup( "action-profiles-editor" ));
}

static gchar *
do_get_dialog_name( BaseWindow *dialog )
{
	return( g_strdup( "ActionProfilesDialog"));
}

static void
on_initial_load_dialog( BaseWindow *dialog )
{
	static const gchar *thisfn = "nact_action_profiles_editor_on_initial_load_dialog";

	/* call parent class at the very beginning */
	if( BASE_WINDOW_CLASS( st_parent_class )->initial_load_toplevel ){
		BASE_WINDOW_CLASS( st_parent_class )->initial_load_toplevel( dialog );
	}

	g_debug( "%s: dialog=%p", thisfn, dialog );
	g_assert( NACT_IS_ACTION_PROFILES_EDITOR( dialog ));
	NactActionProfilesEditor *editor = NACT_ACTION_PROFILES_EDITOR( dialog );

	nact_imenu_item_initial_load( NACT_WINDOW( editor ));

	g_assert( NACT_IS_IPROFILES_LIST( editor ));
	nact_iprofiles_list_initial_load( NACT_WINDOW( editor ));
	nact_iprofiles_list_set_multiple_selection( NACT_WINDOW( editor ), FALSE );
	nact_iprofiles_list_set_send_selection_changed_on_fill_list( NACT_WINDOW( editor ), FALSE );

	/* label alignements */
	GtkSizeGroup *label_group = gtk_size_group_new( GTK_SIZE_GROUP_HORIZONTAL );
	nact_imenu_item_size_labels( NACT_WINDOW( editor ), G_OBJECT( label_group ));
	g_object_unref( label_group );
}

static void
on_runtime_init_dialog( BaseWindow *dialog )
{
	static const gchar *thisfn = "nact_action_profiles_editor_on_runtime_init_dialog";

	/* call parent class at the very beginning */
	if( BASE_WINDOW_CLASS( st_parent_class )->runtime_init_toplevel ){
		BASE_WINDOW_CLASS( st_parent_class )->runtime_init_toplevel( dialog );
	}

	g_debug( "%s: dialog=%p", thisfn, dialog );
	g_assert( NACT_IS_ACTION_PROFILES_EDITOR( dialog ));
	NactActionProfilesEditor *editor = NACT_ACTION_PROFILES_EDITOR( dialog );

	setup_dialog_title( editor, FALSE );

	nact_imenu_item_runtime_init( NACT_WINDOW( editor ), editor->private->edited );
	nact_iprofiles_list_runtime_init( NACT_WINDOW( editor ));

	nact_window_signal_connect_by_name( NACT_WINDOW( editor ), "NewProfileButton", "clicked", G_CALLBACK( on_new_button_clicked ));
	nact_window_signal_connect_by_name( NACT_WINDOW( editor ), "EditProfileButton", "clicked", G_CALLBACK( on_edit_button_clicked ));
	nact_window_signal_connect_by_name( NACT_WINDOW( editor ), "DuplicateProfileButton", "clicked", G_CALLBACK( on_duplicate_button_clicked ));
	nact_window_signal_connect_by_name( NACT_WINDOW( editor ), "DeleteProfileButton", "clicked", G_CALLBACK( on_delete_button_clicked ));

	nact_window_signal_connect_by_name( NACT_WINDOW( editor ), "CancelButton", "clicked", G_CALLBACK( on_cancel_clicked ));
}

static void
on_all_widgets_showed( BaseWindow *dialog )
{
	static const gchar *thisfn = "nact_action_profiles_editor_on_all_widgets_showed";

	/* call parent class at the very beginning */
	if( BASE_WINDOW_CLASS( st_parent_class )->all_widgets_showed ){
		BASE_WINDOW_CLASS( st_parent_class )->all_widgets_showed( dialog );
	}

	g_debug( "%s: dialog=%p", thisfn, dialog );

	nact_imenu_item_all_widgets_showed( NACT_WINDOW( dialog ));
}

static void
setup_dialog_title( NactActionProfilesEditor *dialog, gboolean is_modified )
{
	gchar *label = na_action_get_label( dialog->private->original );
	gchar *title = g_strdup_printf( _( "Editing \"%s\" action" ), label );
	g_free( label );

	if( is_modified ){
		gchar *tmp = g_strdup_printf( "*%s", title );
		g_free( title );
		title = tmp;
	}

	GtkWindow *toplevel = base_window_get_toplevel_dialog( BASE_WINDOW( dialog ));
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
setup_buttons( NactActionProfilesEditor *dialog, gboolean can_save )
{
	GtkWidget *cancel_button = gtk_button_new_from_stock( GTK_STOCK_CANCEL );
	GtkWidget *close_button = gtk_button_new_from_stock( GTK_STOCK_CLOSE );
	GtkWidget *button = base_window_get_widget( BASE_WINDOW( dialog ), "CancelButton1" );
	gtk_button_set_label( GTK_BUTTON( button ), can_save ? _( "_Cancel" ) : _( "_Close" ));
	gtk_button_set_image( GTK_BUTTON( button ), can_save ? gtk_button_get_image( GTK_BUTTON( cancel_button )) : gtk_button_get_image( GTK_BUTTON( close_button )));
	gtk_widget_destroy( cancel_button );
	gtk_widget_destroy( close_button );

	button = base_window_get_widget( BASE_WINDOW( dialog ), "SaveButton" );
	gtk_widget_set_sensitive( button, can_save );
}

static void
on_profiles_list_selection_changed( GtkTreeSelection *selection, gpointer user_data )
{
	/*static const gchar *thisfn = "nact_main_window_on_profiles_list_selection_changed";
	g_debug( "%s: selection=%p, user_data=%p", thisfn, selection, user_data );*/

	g_assert( NACT_IS_ACTION_PROFILES_EDITOR( user_data ));
	BaseWindow *window = BASE_WINDOW( user_data );

	GtkWidget *edit_button = base_window_get_widget( window, "EditProfileButton" );
	GtkWidget *delete_button = base_window_get_widget( window, "DeleteProfileButton" );
	GtkWidget *duplicate_button = base_window_get_widget( window, "DuplicateProfileButton" );

	gboolean enabled = ( gtk_tree_selection_count_selected_rows( selection ) > 0 );

	gtk_widget_set_sensitive( edit_button, enabled );
	gtk_widget_set_sensitive( delete_button, enabled );
	gtk_widget_set_sensitive( duplicate_button, enabled );

	/*NAActionProfile *profile = NA_ACTION_PROFILE( nact_iprofiles_list_get_selected_profile( NACT_WINDOW( window )));
	do_set_current_profile( NACT_ACTION_PROFILES_EDITOR( window ), profile );*/
}

static gboolean
on_profiles_list_double_click( GtkWidget *widget, GdkEventButton *event, gpointer user_data )
{
	g_assert( event->type == GDK_2BUTTON_PRESS );

	on_edit_button_clicked( NULL, user_data );

	return( TRUE );
}

static gboolean
on_profiles_list_enter_key_pressed( GtkWidget *widget, GdkEventKey *event, gpointer user_data )
{
	on_edit_button_clicked( NULL, user_data );

	return( TRUE );
}

static void
on_modified_field( NactWindow *window )
{
	/*static const gchar *thisfn = "nact_action_profiles_editor_on_modified_field";*/

	g_assert( NACT_IS_ACTION_PROFILES_EDITOR( window ));
	NactActionProfilesEditor *dialog = ( NACT_ACTION_PROFILES_EDITOR( window ));

	gboolean is_modified = is_edited_modified( dialog );
	setup_dialog_title( dialog, is_modified );

	gboolean can_save = is_modified && nact_imenu_item_has_label( window );
	setup_buttons( dialog, can_save );
}

/*
 * creating a new profile
 */
static void
on_new_button_clicked( GtkButton *button, gpointer user_data )
{
	g_assert( NACT_IS_ACTION_PROFILES_EDITOR( user_data ));
	NactWindow *window = NACT_WINDOW( user_data );
	NactActionProfilesEditor *editor = NACT_ACTION_PROFILES_EDITOR( window );

	nact_profile_conditions_editor_run_editor( window, editor->private->edited, NULL );

	nact_iprofiles_list_set_focus( window );
}

static void
on_edit_button_clicked( GtkButton *button, gpointer user_data )
{

}

static void
on_delete_button_clicked( GtkButton *button, gpointer user_data )
{

}

static void
on_duplicate_button_clicked( GtkButton *button, gpointer user_data )
{

}

static void
on_cancel_clicked( GtkButton *button, gpointer user_data )
{
	GtkWindow *toplevel = base_window_get_toplevel_dialog( BASE_WINDOW( user_data ));
	gtk_dialog_response( GTK_DIALOG( toplevel ), GTK_RESPONSE_CLOSE );
}

static gboolean
on_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window )
{
	static const gchar *thisfn = "nact_action_profiles_editor_on_dialog_response";
	g_debug( "%s: dialog=%p, code=%d, window=%p", thisfn, dialog, code, window );

	g_assert( NACT_IS_ACTION_PROFILES_EDITOR( window ));
	NactActionProfilesEditor *editor = NACT_ACTION_PROFILES_EDITOR( window );

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
					/*do_set_current_action( NACT_ACTION_PROFILES_EDITOR( window ));*/
				}
			}
			break;
	}

	return( FALSE );
}

static GSList *
do_get_profiles( NactWindow *window )
{
	g_assert( NACT_IS_ACTION_PROFILES_EDITOR( window ));
	NactActionProfilesEditor *editor = NACT_ACTION_PROFILES_EDITOR( window );

	return( na_action_get_profiles( editor->private->edited ));
}

static GObject *
get_edited_action( NactWindow *window )
{
	g_assert( NACT_IS_ACTION_PROFILES_EDITOR( window ));
	return( G_OBJECT( NACT_ACTION_PROFILES_EDITOR( window )->private->edited ));
}

static gboolean
is_edited_modified( NactActionProfilesEditor *dialog )
{
	return( !na_action_are_equal( dialog->private->original, dialog->private->edited ));
}

/*static void
do_set_current_profile( NactActionProfilesEditor *dialog, const NAActionProfile *profile )
{
	gchar *uuid = na_action_get_uuid( dialog->private->original );
	gchar *label = na_action_get_label( dialog->private->original );
	nact_window_set_current_action( dialog->private->parent, uuid, label );
	g_free( label );
	g_free( uuid );
}*/
