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

#include <glib.h>

#include "nact-window.h"

#define GLADE_FILE				GLADEDIR "/nautilus-actions-config.glade"

/* private class data
 */
struct NactWindowClassPrivate {
};

/* private instance data
 */
struct NactWindowPrivate {
	gboolean   dispose_has_run;
};

static GObjectClass *st_parent_class = NULL;

static GType  register_type( void );
static void   class_init( NactWindowClass *klass );
static void   instance_init( GTypeInstance *instance, gpointer klass );
/*static void   instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void   instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );*/
static void   instance_dispose( GObject *application );
static void   instance_finalize( GObject *application );

static void   init_window( BaseWindow *window );
static gchar *get_glade_file( BaseWindow *window );

GType
nact_window_get_type( void )
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
	static const gchar *thisfn = "nact_window_register_type";
	g_debug( "%s", thisfn );

	g_type_init();

	static GTypeInfo info = {
		sizeof( NactWindowClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactWindow ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	return( g_type_register_static( BASE_WINDOW_TYPE, "NactWindow", &info, 0 ));
}

static void
class_init( NactWindowClass *klass )
{
	static const gchar *thisfn = "nact_window_class_init";
	g_debug( "%s: klass=%p", thisfn, klass );

	st_parent_class = g_type_class_peek_parent( klass );

	GObjectClass *object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
	/*object_class->get_property = instance_get_property;
	object_class->set_property = instance_set_property;*/

	klass->private = g_new0( NactWindowClassPrivate, 1 );

	BaseWindowClass *base_class = BASE_WINDOW_CLASS( klass );
	base_class->init_window = init_window;
	base_class->get_glade_file = get_glade_file;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_window_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );

	g_assert( NACT_IS_WINDOW( instance ));
	NactWindow *self = NACT_WINDOW( instance );

	self->private = g_new0( NactWindowPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

/*static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	g_assert( NACT_IS_WINDOW( object ));
	NactWindow *self = NACT_WINDOW( object );

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
	g_assert( NACT_IS_WINDOW( object ));
	NactWindow *self = NACT_WINDOW( object );

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
	static const gchar *thisfn = "nact_window_instance_dispose";
	g_debug( "%s: window=%p", thisfn, window );

	g_assert( NACT_IS_WINDOW( window ));
	NactWindow *self = NACT_WINDOW( window );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		G_OBJECT_CLASS( st_parent_class )->dispose( window );
	}
}

static void
instance_finalize( GObject *window )
{
	static const gchar *thisfn = "nact_window_instance_finalize";
	g_debug( "%s: window=%p", thisfn, window );

	g_assert( NACT_IS_WINDOW( window ));
	/*NactWindow *self = ( NactWindow * ) window;*/

	/* chain call to parent class */
	if( st_parent_class->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( window );
	}
}

static void
init_window( BaseWindow *window )
{
	/* do nothing */
}

static gchar *
get_glade_file( BaseWindow *window )
{
	return( g_strdup( GLADE_FILE ));
}
