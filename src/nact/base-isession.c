/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011, 2012 Pierre Wieser and others (see AUTHORS)
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

#include "base-application.h"
#include "base-isession.h"
#include "egg-sm-client.h"

/* private interface data
 */
struct _BaseISessionInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private properties, set against the instance
 */
typedef struct {
	EggSMClient *sm_client;
	gulong       sm_client_quit_handler_id;
	gulong       sm_client_quit_requested_handler_id;
}
	ISessionStr;

/* Above ISessionStr struct is set as a BaseISession pseudo-property
 * of the instance.
 */
#define BASE_PROP_ISESSION				"base-prop-isession"

static guint st_initializations = 0;	/* interface initialisation count */

static GType        register_type( void );
static void         interface_base_init( BaseISessionInterface *klass );
static void         interface_base_finalize( BaseISessionInterface *klass );

static void         on_instance_finalized( gpointer user_data, BaseISession *instance );
static void         client_quit_requested_cb( EggSMClient *client, BaseISession *instance );
static void         client_quit_cb( EggSMClient *client, BaseISession *instance );
static ISessionStr *get_isession_str( BaseISession *instance );

GType
base_isession_get_type( void )
{
	static GType iface_type = 0;

	if( !iface_type ){
		iface_type = register_type();
	}

	return( iface_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "base_isession_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( BaseISessionInterface ),
		( GBaseInitFunc ) interface_base_init,
		( GBaseFinalizeFunc ) interface_base_finalize,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_INTERFACE, "BaseISession", &info, 0 );

	g_type_interface_add_prerequisite( type, BASE_APPLICATION_TYPE );

	return( type );
}

static void
interface_base_init( BaseISessionInterface *klass )
{
	static const gchar *thisfn = "base_isession_interface_base_init";

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( BaseISessionInterfacePrivate, 1 );
	}

	st_initializations += 1;
}

static void
interface_base_finalize( BaseISessionInterface *klass )
{
	static const gchar *thisfn = "base_isession_interface_base_finalize";

	st_initializations -= 1;

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );
	}
}

void
base_isession_init( BaseISession *instance )
{
	static const gchar *thisfn = "base_isession_init";
	ISessionStr *str;

	g_return_if_fail( BASE_IS_ISESSION( instance ));

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

	str = get_isession_str( instance );

	/* set a weak reference to be advertised when the instance is finalized
	 */
	g_object_weak_ref( G_OBJECT( instance ), ( GWeakNotify ) on_instance_finalized, NULL );

	/* initialize the session manager
	 */
	egg_sm_client_set_mode( EGG_SM_CLIENT_MODE_NO_RESTART );
	str->sm_client = egg_sm_client_get();
	egg_sm_client_startup();
	g_debug( "%s: sm_client=%p", thisfn, ( void * ) str->sm_client );

	str->sm_client_quit_handler_id =
			g_signal_connect(
					str->sm_client,
					"quit-requested",
					G_CALLBACK( client_quit_requested_cb ),
					instance );

	str->sm_client_quit_requested_handler_id =
			g_signal_connect(
					str->sm_client,
					"quit",
					G_CALLBACK( client_quit_cb ),
					instance );
}

static void
on_instance_finalized( gpointer user_data, BaseISession *instance )
{
	static const gchar *thisfn = "base_isession_on_instance_finalized";
	ISessionStr *str;

	g_debug( "%s: instance=%p, user_data=%p", thisfn, ( void * ) instance, ( void * ) user_data );

	str = get_isession_str( instance );

	if( str->sm_client_quit_handler_id &&
		g_signal_handler_is_connected( str->sm_client, str->sm_client_quit_handler_id )){
			g_signal_handler_disconnect( str->sm_client, str->sm_client_quit_handler_id  );
	}

	if( str->sm_client_quit_requested_handler_id &&
		g_signal_handler_is_connected( str->sm_client, str->sm_client_quit_requested_handler_id )){
			g_signal_handler_disconnect( str->sm_client, str->sm_client_quit_requested_handler_id  );
	}

	if( str->sm_client ){
		g_object_unref( str->sm_client );
	}

	g_free( str );
}

/*
 * the session manager advertises us that the session is about to exit
 */
static void
client_quit_requested_cb( EggSMClient *client, BaseISession *instance )
{
	static const gchar *thisfn = "base_isession_client_quit_requested_cb";
	gboolean willing_to = TRUE;

	g_return_if_fail( BASE_IS_ISESSION( instance ));

	g_debug( "%s: client=%p, instance=%p", thisfn, ( void * ) client, ( void * ) instance );

#if 0
		if( application->private->main_window ){

				g_return_if_fail( BASE_IS_WINDOW( application->private->main_window ));
				willing_to = base_window_is_willing_to_quit( application->private->main_window );
		}
#endif

	egg_sm_client_will_quit( client, willing_to );
}

/*
 * cleanly terminate the main window when exiting the session
 */
static void
client_quit_cb( EggSMClient *client, BaseISession *instance )
{
	static const gchar *thisfn = "base_isession_client_quit_cb";

	g_return_if_fail( BASE_IS_ISESSION( instance ));

	g_debug( "%s: client=%p, instance=%p", thisfn, ( void * ) client, ( void * ) instance );

#if 0
		if( application->private->main_window ){

				g_return_if_fail( BASE_IS_WINDOW( application->private->main_window ));
				g_object_unref( application->private->main_window );
				application->private->main_window = NULL;
		}
#endif
}

static ISessionStr *
get_isession_str( BaseISession *instance )
{
	ISessionStr *str;

	str = ( ISessionStr * ) g_object_get_data( G_OBJECT( instance ), BASE_PROP_ISESSION );

	if( !str ){
		str = g_new0( ISessionStr, 1 );
		g_object_set_data( G_OBJECT( instance ), BASE_PROP_ISESSION, str );
	}

	return( str );
}
