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

#include "nact-profile-conditions-editor.h"
#include "nact-iprofile-item.h"
#include "nact-iconditions.h"

/* private class data
 */
struct NactProfileConditionsEditorClassPrivate {
};

/* private instance data
 */
struct NactProfileConditionsEditorPrivate {
	gboolean         dispose_has_run;
	NAAction        *original_action;
	NAActionProfile *original_profile;
	gboolean         saved;
	gboolean         is_new;
	NAAction        *edited_action;
	NAActionProfile *edited_profile;
};

static GObjectClass *st_parent_class = NULL;

static GType    register_type( void );
static void     class_init( NactProfileConditionsEditorClass *klass );
static void     iprofile_item_iface_init( NactIConditionsInterface *iface );
static void     iconditions_iface_init( NactIConditionsInterface *iface );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_dispose( GObject *dialog );
static void     instance_finalize( GObject *dialog );

static NactProfileConditionsEditor *profile_conditions_editor_new( NactWindow *parent );

static gchar   *do_get_iprefs_window_id( NactWindow *window );
static gchar   *do_get_dialog_name( BaseWindow *dialog );
static void     on_initial_load_dialog( BaseWindow *dialog );
static void     on_runtime_init_dialog( BaseWindow *dialog );
static void     on_all_widgets_showed( BaseWindow *dialog );
static void     setup_dialog_title( NactProfileConditionsEditor *dialog, gboolean is_modified );
static void     setup_buttons( NactProfileConditionsEditor *dialog, gboolean can_save );
static void     on_modified_field( NactWindow *dialog );
static gboolean on_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window );
static GObject *get_edited_profile( NactWindow *window );
static gboolean is_edited_modified( NactProfileConditionsEditor *editor );

GType
nact_profile_conditions_editor_get_type( void )
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
	static const gchar *thisfn = "nact_profile_conditions_editor_register_type";
	g_debug( "%s", thisfn );

	static GTypeInfo info = {
		sizeof( NactProfileConditionsEditorClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactProfileConditionsEditor ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	GType type = g_type_register_static( NACT_WINDOW_TYPE, "NactProfileConditionsEditor", &info, 0 );

	/* implement IProfileItem interface
	 */
	static const GInterfaceInfo iprofile_item_iface_info = {
		( GInterfaceInitFunc ) iprofile_item_iface_init,
		NULL,
		NULL
	};

	g_type_add_interface_static( type, NACT_IPROFILE_ITEM_TYPE, &iprofile_item_iface_info );

	/* implement IConditions interface
	 */
	static const GInterfaceInfo iconditions_iface_info = {
		( GInterfaceInitFunc ) iconditions_iface_init,
		NULL,
		NULL
	};

	g_type_add_interface_static( type, NACT_ICONDITIONS_TYPE, &iconditions_iface_info );

	return( type );
}

static void
class_init( NactProfileConditionsEditorClass *klass )
{
	static const gchar *thisfn = "nact_profile_conditions_editor_class_init";
	g_debug( "%s: klass=%p", thisfn, klass );

	st_parent_class = g_type_class_peek_parent( klass );

	GObjectClass *object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactProfileConditionsEditorClassPrivate, 1 );

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
iprofile_item_iface_init( NactIConditionsInterface *iface )
{
	static const gchar *thisfn = "nact_profile_conditions_editor_iprofile_item_iface_init";
	g_debug( "%s: iface=%p", thisfn, iface );

	iface->get_edited_profile = get_edited_profile;
	iface->field_modified = on_modified_field;
}

static void
iconditions_iface_init( NactIConditionsInterface *iface )
{
	static const gchar *thisfn = "nact_profile_conditions_editor_iconditions_iface_init";
	g_debug( "%s: iface=%p", thisfn, iface );

	iface->get_edited_profile = get_edited_profile;
	iface->field_modified = on_modified_field;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_profile_conditions_editor_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );

	g_assert( NACT_IS_PROFILE_CONDITIONS_EDITOR( instance ));
	NactProfileConditionsEditor *self = NACT_PROFILE_CONDITIONS_EDITOR( instance );

	self->private = g_new0( NactProfileConditionsEditorPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *dialog )
{
	static const gchar *thisfn = "nact_profile_conditions_editor_instance_dispose";
	g_debug( "%s: dialog=%p", thisfn, dialog );

	g_assert( NACT_IS_PROFILE_CONDITIONS_EDITOR( dialog ));
	NactProfileConditionsEditor *self = NACT_PROFILE_CONDITIONS_EDITOR( dialog );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		g_object_unref( self->private->original_action );
		g_object_unref( self->private->edited_action );

		/* chain up to the parent class */
		G_OBJECT_CLASS( st_parent_class )->dispose( dialog );
	}
}

static void
instance_finalize( GObject *dialog )
{
	static const gchar *thisfn = "nact_profile_conditions_editor_instance_finalize";
	g_debug( "%s: dialog=%p", thisfn, dialog );

	g_assert( NACT_IS_PROFILE_CONDITIONS_EDITOR( dialog ));
	NactProfileConditionsEditor *self = ( NactProfileConditionsEditor * ) dialog;

	g_free( self->private );

	/* chain call to parent class */
	if( st_parent_class->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( dialog );
	}
}

/*
 * Returns a newly allocated NactProfileConditionsEditor object.
 *
 * @parent: is the BaseWindow parent of this dialog (usually, the main
 * toplevel window of the application).
 */
static NactProfileConditionsEditor *
profile_conditions_editor_new( NactWindow *parent )
{
	return( g_object_new( NACT_PROFILE_CONDITIONS_EDITOR_TYPE, PROP_WINDOW_PARENT_STR, parent, NULL ));
}

/**
 * Initializes and runs the dialog.
 *
 * @parent: is the BaseWindow parent of this dialog (usually, the main
 * toplevel window of the application).
 *
 * @action: the NAAction to which belongs the profile to edit.
 *
 * @profile: the NAActionProfile to be edited, or NULL to create a new one.
 *
 * Returns the modified action, or NULL if no modification has been
 * validated.
 */
NAAction *
nact_profile_conditions_editor_run_editor( NactWindow *parent, NAAction *action, NAActionProfile *profile )
{
	NactProfileConditionsEditor *editor = profile_conditions_editor_new( parent );

	g_assert( action );
	g_assert( NA_IS_ACTION( action ));
	g_assert( NA_IS_ACTION_PROFILE( profile ) || !profile );

	gchar *name;
	editor->private->original_action = na_action_duplicate( action );

	if( !profile ){
		name = na_action_get_new_profile_name( action );
		NAActionProfile *new_profile = na_action_profile_new( NA_OBJECT( editor->private->original_action ), name );
		na_action_add_profile( editor->private->original_action, NA_OBJECT( new_profile ));
		editor->private->original_profile = new_profile;
		editor->private->is_new = TRUE;

	} else {
		name = na_action_profile_get_name( profile );
		editor->private->original_profile = NA_ACTION_PROFILE( na_action_get_profile( editor->private->original_action, name ));
		editor->private->is_new = FALSE;
	}

	editor->private->edited_action = na_action_duplicate( editor->private->original_action );
	editor->private->edited_profile = NA_ACTION_PROFILE( na_action_get_profile( editor->private->edited_action, name ));

	g_free( name );

	base_window_run( BASE_WINDOW( editor ));

	return( editor->private->saved ? editor->private->original_action : NULL );
}

static gchar *
do_get_iprefs_window_id( NactWindow *window )
{
	return( g_strdup( "profile-conditions-editor" ));
}

static gchar *
do_get_dialog_name( BaseWindow *dialog )
{
	return( g_strdup( "ProfileConditionsDialog"));
}

static void
on_initial_load_dialog( BaseWindow *dialog )
{
	static const gchar *thisfn = "nact_profile_conditions_editor_on_initial_load_dialog";

	/* call parent class at the very beginning */
	if( BASE_WINDOW_CLASS( st_parent_class )->initial_load_toplevel ){
		BASE_WINDOW_CLASS( st_parent_class )->initial_load_toplevel( dialog );
	}

	g_debug( "%s: dialog=%p", thisfn, dialog );

	g_assert( NACT_IS_PROFILE_CONDITIONS_EDITOR( dialog ));
	NactProfileConditionsEditor *editor = NACT_PROFILE_CONDITIONS_EDITOR( dialog );

	nact_iprofile_item_initial_load( NACT_WINDOW( editor ), editor->private->edited_profile );
	nact_iconditions_initial_load( NACT_WINDOW( editor ), editor->private->edited_profile );

	/* label alignements */
	GtkSizeGroup *label_group = gtk_size_group_new( GTK_SIZE_GROUP_HORIZONTAL );
	nact_iprofile_item_size_labels( NACT_WINDOW( editor ), G_OBJECT( label_group ));
	nact_iconditions_size_labels( NACT_WINDOW( editor ), G_OBJECT( label_group ));
	g_object_unref( label_group );

	/* buttons size */
	GtkSizeGroup *button_group = gtk_size_group_new( GTK_SIZE_GROUP_HORIZONTAL );
	nact_iprofile_item_size_buttons( NACT_WINDOW( editor ), G_OBJECT( button_group ));
	nact_iconditions_size_buttons( NACT_WINDOW( editor ), G_OBJECT( button_group ));
	g_object_unref( button_group );
}

static void
on_runtime_init_dialog( BaseWindow *dialog )
{
	static const gchar *thisfn = "nact_profile_conditions_editor_on_runtime_init_dialog";

	/* call parent class at the very beginning */
	if( BASE_WINDOW_CLASS( st_parent_class )->runtime_init_toplevel ){
		BASE_WINDOW_CLASS( st_parent_class )->runtime_init_toplevel( dialog );
	}

	g_debug( "%s: dialog=%p", thisfn, dialog );

	g_assert( NACT_IS_PROFILE_CONDITIONS_EDITOR( dialog ));
	NactProfileConditionsEditor *editor = NACT_PROFILE_CONDITIONS_EDITOR( dialog );

	setup_dialog_title( editor, FALSE );

	nact_iprofile_item_runtime_init( NACT_WINDOW( editor ), editor->private->edited_profile );
	nact_iconditions_runtime_init( NACT_WINDOW( editor ), editor->private->edited_profile );
}

static void
on_all_widgets_showed( BaseWindow *dialog )
{
	static const gchar *thisfn = "nact_profile_conditions_editor_on_all_widgets_showed";

	/* call parent class at the very beginning */
	if( BASE_WINDOW_CLASS( st_parent_class )->all_widgets_showed ){
		BASE_WINDOW_CLASS( st_parent_class )->all_widgets_showed( dialog );
	}

	g_debug( "%s: dialog=%p", thisfn, dialog );

	GtkNotebook *notebook = GTK_NOTEBOOK( base_window_get_widget( dialog, "Notebook" ));
	gtk_notebook_set_current_page( notebook, 0 );

	nact_iprofile_item_all_widgets_showed( NACT_WINDOW( dialog ));
	nact_iconditions_all_widgets_showed( NACT_WINDOW( dialog ));
}

static void
setup_dialog_title( NactProfileConditionsEditor *editor, gboolean is_modified )
{
	gchar *title;
	gchar *label;

	if( editor->private->is_new ){
		/* i18n: title of the window when adding a new profile to an existing action */
		title = g_strdup( _( "Adding a new profile" ));

	} else {
		label = na_action_profile_get_label( editor->private->original_profile );
		/* i18n: title of the window when editing a profile */
		title = g_strdup_printf( _( "Editing \"%s\" profile" ), label );
		g_free( label );
	}

	if( is_modified ){
		gchar *tmp = g_strdup_printf( "*%s", title );
		g_free( title );
		title = tmp;
	}

	GtkWindow *toplevel = base_window_get_toplevel_dialog( BASE_WINDOW( editor ));
	gtk_window_set_title( toplevel, title );
	g_free( title );
}

/*
 * rationale:
 * - while the action or the profile are not modified, only the cancel
 *   button is activated (showed as close)
 * - when the mono-profile action is modified, we have a save and a
 *   cancel buttons (a label is mandatory to enable the save button)
 * - when editing a profile, we have a OK and an cancel buttons
 *   (profile label is mandatory) as the actual save will take place in
 *   the NactActionProfilesEditor parent window
 */
static void
setup_buttons( NactProfileConditionsEditor *editor, gboolean can_save )
{
	GtkWidget *cancel_button = gtk_button_new_from_stock( GTK_STOCK_CANCEL );
	GtkWidget *close_button = gtk_button_new_from_stock( GTK_STOCK_CLOSE );

	GtkWidget *dlg_cancel = base_window_get_widget( BASE_WINDOW( editor ), "CancelButton2" );
	gtk_button_set_label( GTK_BUTTON( dlg_cancel ), can_save ? _( "_Cancel" ) : _( "_Close" ));
	gtk_button_set_image( GTK_BUTTON( dlg_cancel ), can_save ? gtk_button_get_image( GTK_BUTTON( cancel_button )) : gtk_button_get_image( GTK_BUTTON( close_button )));

	gtk_widget_destroy( close_button );
	gtk_widget_destroy( cancel_button );

	GtkWidget *dlg_ok = base_window_get_widget( BASE_WINDOW( editor ), "OKButton" );
	gtk_widget_set_sensitive( dlg_ok, can_save );
}

static void
on_modified_field( NactWindow *window )
{
	/*static const gchar *thisfn = "nact_profile_conditions_editor_on_modified_field";*/

	g_assert( NACT_IS_PROFILE_CONDITIONS_EDITOR( window ));
	NactProfileConditionsEditor *editor = ( NACT_PROFILE_CONDITIONS_EDITOR( window ));

	gboolean is_modified = is_edited_modified( editor );
	setup_dialog_title( editor, is_modified );

	gboolean can_save = is_modified && nact_iprofile_item_has_label( window );

	setup_buttons( editor, can_save );
}

static gboolean
on_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window )
{
	static const gchar *thisfn = "nact_profile_conditions_editor_on_dialog_response";
	g_debug( "%s: dialog=%p, code=%d, window=%p", thisfn, dialog, code, window );

	g_assert( NACT_IS_PROFILE_CONDITIONS_EDITOR( window ));
	NactProfileConditionsEditor *editor = NACT_PROFILE_CONDITIONS_EDITOR( window );

	gboolean is_modified = is_edited_modified( editor );

	switch( code ){
		case GTK_RESPONSE_NONE:
		case GTK_RESPONSE_DELETE_EVENT:
		case GTK_RESPONSE_CLOSE:
		case GTK_RESPONSE_CANCEL:
			if( !is_modified ||
					nact_window_warn_profile_modified( NACT_WINDOW( dialog ), editor->private->original_profile )){

					g_object_unref( window );
					return( TRUE );
			}
			break;

		case GTK_RESPONSE_OK:
			if( is_modified ){
				g_object_unref( editor->private->original_action );
				editor->private->original_action = na_action_duplicate( editor->private->edited_action );
				gchar *name = na_action_profile_get_name( editor->private->edited_profile );
				editor->private->original_profile = NA_ACTION_PROFILE( na_action_get_profile( editor->private->original_action, name ));
				g_free( name );
				editor->private->is_new = FALSE;
				editor->private->saved = TRUE;
				on_modified_field( NACT_WINDOW( dialog ));
			}
			break;
	}

	return( FALSE );
}

static GObject *
get_edited_profile( NactWindow *window )
{
	g_assert( NACT_IS_PROFILE_CONDITIONS_EDITOR( window ));
	return( G_OBJECT( NACT_PROFILE_CONDITIONS_EDITOR( window )->private->edited_profile ));
}

static gboolean
is_edited_modified( NactProfileConditionsEditor *editor )
{
	return( !na_action_are_equal( editor->private->original_action, editor->private->edited_action ));
}
