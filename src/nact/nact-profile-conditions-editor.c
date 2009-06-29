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
#include "nact-profile-conditions-editor.h"
#include "nact-iprofile-conditions.h"
#include "nact-main-window.h"

/* private class data
 */
struct NactProfileConditionsEditorClassPrivate {
};

/* private instance data
 */
struct NactProfileConditionsEditorPrivate {
	gboolean  dispose_has_run;
	NAAction *action;
	gboolean  is_new;
};

static GObjectClass *st_parent_class = NULL;

static GType    register_type( void );
static void     class_init( NactProfileConditionsEditorClass *klass );
static void     iprofile_conditions_iface_init( NactIProfileConditionsInterface *iface );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_dispose( GObject *dialog );
static void     instance_finalize( GObject *dialog );

static NactProfileConditionsEditor *profile_conditions_editor_new( BaseApplication *application );

static gchar   *do_get_dialog_name( BaseWindow *dialog );
static void     on_initial_load_dialog( BaseWindow *dialog );
static void     on_runtime_init_dialog( BaseWindow *dialog );
static void     init_dialog_title( NactProfileConditionsEditor *dialog );
static gboolean on_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window );

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
	base_class->initial_load_toplevel = on_initial_load_dialog;
	base_class->runtime_init_toplevel = on_runtime_init_dialog;
	base_class->dialog_response = on_dialog_response;
	base_class->get_toplevel_name = do_get_dialog_name;
}

static void
iprofile_conditions_iface_init( NactIProfileConditionsInterface *iface )
{
	static const gchar *thisfn = "nact_profile_conditions_editor_iprofile_conditions_iface_init";
	g_debug( "%s: iface=%p", thisfn, iface );
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
	/*NactProfileConditionsEditor *self = ( NactProfileConditionsEditor * ) dialog;*/

	/* chain call to parent class */
	if( st_parent_class->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( dialog );
	}
}

/**
 * Returns a newly allocated NactProfileConditionsEditor object.
 *
 * @parent: is the BaseWindow parent of this dialog (usually, the main
 * toplevel window of the application).
 */
static NactProfileConditionsEditor *
profile_conditions_editor_new( BaseApplication *application )
{
	return( g_object_new( NACT_PROFILE_CONDITIONS_EDITOR_TYPE, PROP_WINDOW_APPLICATION_STR, application, NULL ));
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
gboolean
nact_profile_conditions_editor_run_editor( NactWindow *parent, gpointer user_data )
{
	g_assert( NACT_IS_MAIN_WINDOW( parent ));

	BaseApplication *application = BASE_APPLICATION( base_window_get_application( BASE_WINDOW( parent )));
	g_assert( NACT_IS_APPLICATION( application ));

	NactProfileConditionsEditor *dialog = profile_conditions_editor_new( application );

	g_assert( NA_IS_ACTION( user_data ) || !user_data );
	NAAction *action = NA_ACTION( user_data );

	if( !action ){
		dialog->private->action = na_action_new( NULL );
		dialog->private->is_new = TRUE;

	} else {
		dialog->private->action = na_action_duplicate( action );
		dialog->private->is_new = FALSE;
	}

	base_window_run( BASE_WINDOW( dialog ));

	g_object_unref( dialog->private->action );
	return( TRUE );
}

static gchar *
do_get_dialog_name( BaseWindow *dialog )
{
	/*g_debug( "nact_profile_conditions_editor_do_get_dialog_name" );*/
	return( g_strdup( "EditActionDialogExt"));
}

static void
on_initial_load_dialog( BaseWindow *dialog )
{
	static const gchar *thisfn = "nact_profile_conditions_editor_on_initial_load_dialog";
	g_debug( "%s: dialog=%p", thisfn, dialog );

	g_assert( NACT_IS_PROFILE_CONDITIONS_EDITOR( dialog ));
	NactProfileConditionsEditor *window = NACT_PROFILE_CONDITIONS_EDITOR( dialog );

	init_dialog_title( window );
	/*nact_iprofile_conditions_initial_load( NACT_WINDOW( window ), window->private->action );*/
}

static void
on_runtime_init_dialog( BaseWindow *dialog )
{
	static const gchar *thisfn = "nact_profile_conditions_editor_on_runtime_init_dialog";
	g_debug( "%s: dialog=%p", thisfn, dialog );

	g_assert( NACT_IS_PROFILE_CONDITIONS_EDITOR( dialog ));
	NactProfileConditionsEditor *window = NACT_PROFILE_CONDITIONS_EDITOR( dialog );

	init_dialog_title( window );
	/*nact_iprofile_conditions_runtime_init( NACT_WINDOW( window ), window->private->action );*/
}

static void
init_dialog_title( NactProfileConditionsEditor *dialog )
{
	GtkWindow *toplevel = base_window_get_toplevel_widget( BASE_WINDOW( dialog ));

	if( dialog->private->is_new ){
		gtk_window_set_title( toplevel, _( "Adding a new action" ));

	} else {
		gchar *label = na_action_get_label( dialog->private->action );
		gchar* title = g_strdup_printf( _( "Editing \"%s\" action" ), label );
		gtk_window_set_title( toplevel, title );
		g_free( label );
		g_free( title );
	}
}

static gboolean
on_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window )
{
	static const gchar *thisfn = "nact_profile_conditions_editor_on_dialog_response";
	g_debug( "%s: dialog=%p, code=%d, window=%p", thisfn, dialog, code, window );

	g_assert( NACT_IS_PROFILE_CONDITIONS_EDITOR( window ));

	switch( code ){
		case GTK_RESPONSE_NONE:
		case GTK_RESPONSE_DELETE_EVENT:
		case GTK_RESPONSE_CLOSE:
			g_object_unref( window );
			break;
	}

	return( TRUE );
}
