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
#include <glib/gi18n.h>

#include <common/na-pivot.h>
#include <common/na-iio-provider.h>

#include "nact-application.h"
#include "nact-iprefs.h"
#include "nact-window.h"

/* private class data
 */
struct NactWindowClassPrivate {
};

/* private instance data
 */
struct NactWindowPrivate {
	gboolean dispose_has_run;
	GSList  *signals;
};

/* connected signal, to be disconnected at NactWindow dispose
 */
typedef struct {
	gpointer instance;
	gulong   handler_id;
}
	NactWindowRecordedSignal;

static GObjectClass *st_parent_class = NULL;

static GType  register_type( void );
static void   class_init( NactWindowClass *klass );
static void   iprefs_iface_init( NactIPrefsInterface *iface );
static void   instance_init( GTypeInstance *instance, gpointer klass );
static void   instance_dispose( GObject *application );
static void   instance_finalize( GObject *application );

static gchar *v_get_iprefs_window_id( NactWindow *window );

static void   on_runtime_init_toplevel( BaseWindow *window );
static void   on_all_widgets_showed( BaseWindow *dialog );

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

	GType type = g_type_register_static( BASE_WINDOW_TYPE, "NactWindow", &info, 0 );

	/* implement IPrefs interface
	 */
	static const GInterfaceInfo prefs_iface_info = {
		( GInterfaceInitFunc ) iprefs_iface_init,
		NULL,
		NULL
	};

	g_type_add_interface_static( type, NACT_IPREFS_TYPE, &prefs_iface_info );

	return( type );
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

	klass->private = g_new0( NactWindowClassPrivate, 1 );

	klass->get_iprefs_window_id = v_get_iprefs_window_id;

	BaseWindowClass *base_class = BASE_WINDOW_CLASS( klass );
	base_class->runtime_init_toplevel = on_runtime_init_toplevel;
	base_class->all_widgets_showed = on_all_widgets_showed;
}

static void
iprefs_iface_init( NactIPrefsInterface *iface )
{
	static const gchar *thisfn = "nact_window_iprefs_iface_init";
	g_debug( "%s: iface=%p", thisfn, iface );

	iface->get_iprefs_window_id = v_get_iprefs_window_id;
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
	self->private->signals = NULL;
}

static void
instance_dispose( GObject *window )
{
	static const gchar *thisfn = "nact_window_instance_dispose";
	g_debug( "%s: window=%p", thisfn, window );

	g_assert( NACT_IS_WINDOW( window ));
	NactWindow *self = NACT_WINDOW( window );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		nact_iprefs_save_window_position( NACT_WINDOW( window ));

		GSList *is;
		for( is = self->private->signals ; is ; is = is->next ){
			NactWindowRecordedSignal *str = ( NactWindowRecordedSignal * ) is->data;
			g_signal_handler_disconnect( str->instance, str->handler_id );
			/*g_debug( "%s: disconnecting signal handler %p:%lu", thisfn, str->instance, str->handler_id );*/
			g_free( str );
		}
		g_slist_free( self->private->signals );

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
	NactWindow *self = ( NactWindow * ) window;

	g_free( self->private );

	/* chain call to parent class */
	if( st_parent_class->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( window );
	}
}

static gchar *
v_get_iprefs_window_id( NactWindow *window )
{
	g_assert( NACT_IS_IPREFS( window ));

	if( NACT_WINDOW_GET_CLASS( window )->get_iprefs_window_id ){
		return( NACT_WINDOW_GET_CLASS( window )->get_iprefs_window_id( window ));
	}

	return( NULL );
}

static void
on_runtime_init_toplevel( BaseWindow *window )
{
	static const gchar *thisfn = "nact_window_on_runtime_init_toplevel";

	/* call parent class at the very beginning */
	if( BASE_WINDOW_CLASS( st_parent_class )->runtime_init_toplevel ){
		BASE_WINDOW_CLASS( st_parent_class )->runtime_init_toplevel( window );
	}

	g_debug( "%s: window=%p", thisfn, window );
	g_assert( NACT_IS_WINDOW( window ));

	nact_iprefs_position_window( NACT_WINDOW( window ));
}

static void
on_all_widgets_showed( BaseWindow *dialog )
{
	static const gchar *thisfn = "nact_window_on_all_widgets_showed";

	/* call parent class at the very beginning */
	if( BASE_WINDOW_CLASS( st_parent_class )->all_widgets_showed ){
		BASE_WINDOW_CLASS( st_parent_class )->all_widgets_showed( dialog );
	}

	g_debug( "%s: dialog=%p", thisfn, dialog );
}

/**
 * Returns a pointer to the list of actions.
 */
GObject *
nact_window_get_pivot( NactWindow *window )
{
	NactApplication *application;
	g_object_get( G_OBJECT( window ), PROP_WINDOW_APPLICATION_STR, &application, NULL );
	g_return_val_if_fail( NACT_IS_APPLICATION( application ), NULL );

	GObject *pivot = nact_application_get_pivot( application );
	g_return_val_if_fail( NA_IS_PIVOT( pivot ), NULL );

	return( pivot );
}

/**
 * Returns a pointer to the specified action.
 */
GObject *
nact_window_get_action( NactWindow *window, const gchar *uuid )
{
	NAPivot *pivot = NA_PIVOT( nact_window_get_pivot( window ));
	g_return_val_if_fail( NA_IS_PIVOT( pivot ), NULL );

	GObject *action = na_pivot_get_action( pivot, uuid );
	return( action );
}

/**
 * Saves a modified action to the I/O storage subsystem.
 *
 * @window: this NactWindow object.
 *
 * @action: the modified action.
 */
gboolean
nact_window_save_action( NactWindow *window, const NAAction *action )
{
	NAPivot *pivot = NA_PIVOT( nact_window_get_pivot( window ));
	g_assert( NA_IS_PIVOT( pivot ));

	na_object_dump( NA_OBJECT( action ));

	gchar *msg = NULL;
	guint ret = na_pivot_write_action( pivot, G_OBJECT( action ), &msg );
	if( msg ){
		base_window_error_dlg(
				BASE_WINDOW( window ),
				GTK_MESSAGE_WARNING, _( "An error has occured when trying to save the action" ), msg );
		g_free( msg );
	}

	return( ret == NA_IIO_PROVIDER_WRITE_OK );
}

/**
 * Emits a warning if the action has been modified.
 *
 * @window: this NactWindow object.
 *
 * @action: the modified action.
 *
 * Returns TRUE if the user confirms he wants to quit.
 */
gboolean
nact_window_warn_action_modified( NactWindow *window, const NAAction *action )
{
	gchar *label = na_action_get_label( action );

	gchar *first;
	if( label && strlen( label )){
		first = g_strdup_printf( _( "The action \"%s\" has been modified." ), label );
	} else {
		first = g_strdup( _( "The newly created action has been modified" ));
	}
	gchar *second = g_strdup( _( "Are you sure you want to quit without saving it ?" ));

	gboolean ok = base_window_yesno_dlg( BASE_WINDOW( window ), GTK_MESSAGE_QUESTION, first, second );

	g_free( second );
	g_free( first );
	g_free( label );

	return( ok );
}

/**
 * Returns a pointer to the list of actions.
 */
GSList *
nact_window_get_actions( NactWindow *window )
{
	NAPivot *pivot = NA_PIVOT( nact_window_get_pivot( window ));
	g_return_val_if_fail( NA_IS_PIVOT( pivot ), NULL );

	GSList *actions = na_pivot_get_actions( pivot );
	return( actions );
}

/**
 * Records a connected signal, to be disconnected at NactWindow dispose.
 */
void
nact_window_signal_connect( NactWindow *window, GObject *instance, const gchar *signal, GCallback fn )
{
	/*static const gchar *thisfn = "nact_window_signal_connect";*/

	gulong handler_id = g_signal_connect( instance, signal, fn, window );

	NactWindowRecordedSignal *str = g_new0( NactWindowRecordedSignal, 1 );
	str->instance = instance;
	str->handler_id = handler_id;
	window->private->signals = g_slist_prepend( window->private->signals, str );

	/*g_debug( "%s: connecting signal handler %p:%lu", thisfn, instance, handler_id );*/
}
