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

#include <common/na-action.h>

#include "nact-application.h"
#include "nact-action-profile.h"
#include "nact-main-window.h"

/* private class data
 */
struct NactActionProfileClassPrivate {
};

/* private instance data
 */
struct NactActionProfilePrivate {
	gboolean  dispose_has_run;
	NAAction *action;
};

static GObjectClass *st_parent_class = NULL;

static GType  register_type( void );
static void   class_init( NactActionProfileClass *klass );
static void   instance_init( GTypeInstance *instance, gpointer klass );
static void   instance_dispose( GObject *dialog );
static void   instance_finalize( GObject *dialog );

static NactActionProfile *action_profile_new( BaseApplication *application );

static gchar *do_get_dialog_name( BaseWindow *dialog );
static void   on_init_dialog( BaseWindow *dialog );
static void   on_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window );

GType
nact_action_profile_get_type( void )
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
	static const gchar *thisfn = "nact_action_profile_register_type";
	g_debug( "%s", thisfn );

	static GTypeInfo info = {
		sizeof( NactActionProfileClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactActionProfile ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	GType type = g_type_register_static( NACT_WINDOW_TYPE, "NactActionProfile", &info, 0 );

	return( type );
}

static void
class_init( NactActionProfileClass *klass )
{
	static const gchar *thisfn = "nact_action_profile_class_init";
	g_debug( "%s: klass=%p", thisfn, klass );

	st_parent_class = g_type_class_peek_parent( klass );

	GObjectClass *object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
	/*object_class->get_property = instance_get_property;
	object_class->set_property = instance_set_property;*/

	klass->private = g_new0( NactActionProfileClassPrivate, 1 );

	BaseWindowClass *base_class = BASE_WINDOW_CLASS( klass );
	base_class->on_init_widget = on_init_dialog;
	base_class->on_dialog_response = on_dialog_response;
	base_class->get_toplevel_name = do_get_dialog_name;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_action_profile_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );

	g_assert( NACT_IS_ACTION_PROFILE( instance ));
	NactActionProfile *self = NACT_ACTION_PROFILE( instance );

	self->private = g_new0( NactActionProfilePrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

/*static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	g_assert( NACT_IS_ACTION_PROFILE( object ));
	NactActionProfile *self = NACT_ACTION_PROFILE( object );

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
	g_assert( NACT_IS_ACTION_PROFILE( object ));
	NactActionProfile *self = NACT_ACTION_PROFILE( object );

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
instance_dispose( GObject *dialog )
{
	static const gchar *thisfn = "nact_action_profile_instance_dispose";
	g_debug( "%s: dialog=%p", thisfn, dialog );

	g_assert( NACT_IS_ACTION_PROFILE( dialog ));
	NactActionProfile *self = NACT_ACTION_PROFILE( dialog );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		G_OBJECT_CLASS( st_parent_class )->dispose( dialog );
	}
}

static void
instance_finalize( GObject *dialog )
{
	static const gchar *thisfn = "nact_action_profile_instance_finalize";
	g_debug( "%s: dialog=%p", thisfn, dialog );

	g_assert( NACT_IS_ACTION_PROFILE( dialog ));
	/*NactActionProfile *self = ( NactActionProfile * ) dialog;*/

	/* chain call to parent class */
	if( st_parent_class->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( dialog );
	}
}

/**
 * Returns a newly allocated NactActionProfile object.
 *
 * @parent: is the BaseWindow parent of this dialog (usually, the main
 * toplevel window of the application).
 */
static NactActionProfile *
action_profile_new( BaseApplication *application )
{
	return( g_object_new( NACT_ACTION_PROFILE_TYPE, PROP_WINDOW_APPLICATION_STR, application, NULL ));
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
nact_action_profile_run_editor( NactWindow *parent, gpointer user_data )
{
	g_assert( NACT_IS_MAIN_WINDOW( parent ));

	BaseApplication *application = BASE_APPLICATION( base_window_get_application( BASE_WINDOW( parent )));
	g_assert( NACT_IS_APPLICATION( application ));

	NactActionProfile *dialog = action_profile_new( application );

	g_assert( NA_IS_ACTION( user_data ) || !user_data );
	dialog->private->action = NA_ACTION( user_data );

	base_window_run( BASE_WINDOW( dialog ));

	return( TRUE );
}

static gchar *
do_get_dialog_name( BaseWindow *dialog )
{
	/*g_debug( "nact_action_profile_do_get_dialog_name" );*/
	return( g_strdup( "EditActionDialogExt"));
}

static void
on_init_dialog( BaseWindow *dialog )
{
	static const gchar *thisfn = "nact_action_profile_on_init_dialog";
	g_debug( "%s: dialog=%p", thisfn, dialog );
}

static void
on_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window )
{
	static const gchar *thisfn = "nact_action_profile_on_dialog_response";
	g_debug( "%s: dialog=%p, code=%d, window=%p", thisfn, dialog, code, window );

	g_assert( NACT_IS_ACTION_PROFILE( window ));

	switch( code ){
		case GTK_RESPONSE_NONE:
		case GTK_RESPONSE_DELETE_EVENT:
		case GTK_RESPONSE_CLOSE:
			g_object_unref( window );
			break;
	}
}
