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

#include <glib/gi18n.h>
#include <string.h>
#include <unique/unique.h>

#include "base-iunique.h"
#include "base-window.h"

/* private interface data
 */
struct _BaseIUniqueInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* BaseIUnique properties, set as data association against the GObject instance
 */
#define BASE_PROP_HANDLE				"base-prop-iunique-handle"
#define BASE_PROP_INITIALIZED			"base-prop-iunique-initialized"

static guint st_initializations = 0;	/* interface initialisation count */

static GType register_type( void );
static void  interface_base_init( BaseIUniqueInterface *klass );
static void  interface_base_finalize( BaseIUniqueInterface *klass );

static void  instance_init( BaseIUnique *instance );
static void  on_instance_finalized( gpointer user_data, BaseIUnique *instance );

#if 0
static UniqueResponse on_unique_message_received( UniqueApp *app, UniqueCommand command, UniqueMessageData *message, guint time, gpointer user_data );
#endif

static const gchar *m_get_application_name( const BaseIUnique *instance );

GType
base_iunique_get_type( void )
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
	static const gchar *thisfn = "base_iunique_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( BaseIUniqueInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "BaseIUnique", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( BaseIUniqueInterface *klass )
{
	static const gchar *thisfn = "base_iunique_interface_base_init";

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( BaseIUniqueInterfacePrivate, 1 );
	}

	st_initializations += 1;
}

static void
interface_base_finalize( BaseIUniqueInterface *klass )
{
	static const gchar *thisfn = "base_iunique_interface_base_finalize";

	st_initializations -= 1;

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );
	}
}

static void
instance_init( BaseIUnique *instance )
{
	static const gchar *thisfn = "base_iunique_instance_init";

	if( ! ( gboolean ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( instance ), BASE_PROP_INITIALIZED ))){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

		g_object_weak_ref( G_OBJECT( instance ), ( GWeakNotify ) on_instance_finalized, NULL );

		g_object_set_data( G_OBJECT( instance ), BASE_PROP_INITIALIZED, GUINT_TO_POINTER( TRUE ));
	}
}

static void
on_instance_finalized( gpointer user_data, BaseIUnique *instance )
{
	static const gchar *thisfn = "base_iunique_on_instance_finalized";

	g_debug( "%s: instance=%p, user_data=%p", thisfn, ( void * ) instance, ( void * ) user_data );

	UniqueApp *handle = ( UniqueApp * ) g_object_get_data( G_OBJECT( instance ), BASE_PROP_HANDLE );

	if( handle ){
		g_return_if_fail( UNIQUE_IS_APP( handle ));
		g_object_unref( handle );
	}
}

/*
 * Relying on libunique to detect if another instance is already running.
 *
 * A replacement is available with GLib 2.28 in GApplication, but only
 * GLib 2.30 (Fedora 16) provides a "non-unique" capability.
 */
gboolean
base_iunique_init_with_name( BaseIUnique *instance, const gchar *unique_app_name )
{
	static const gchar *thisfn = "base_iunique_init_with_name";
	gboolean ret;
	gboolean is_first;
	gchar *msg;
	UniqueApp *handle;

	g_return_val_if_fail( BASE_IS_IUNIQUE( instance ), FALSE );

	instance_init( instance );

	g_debug( "%s: instance=%p, unique_app_name=%s", thisfn, ( void * ) instance, unique_app_name );

	ret = TRUE;

	if( unique_app_name && strlen( unique_app_name )){

			handle = unique_app_new( unique_app_name, NULL );
			is_first = !unique_app_is_running( handle );

			if( !is_first ){
				unique_app_send_message( handle, UNIQUE_ACTIVATE, NULL );
				/* i18n: application name */
				msg = g_strdup_printf(
						_( "Another instance of %s is already running.\n"
							"Please switch back to it." ),
						m_get_application_name( instance ));
				base_window_display_error_dlg( NULL, _( "The application is not unique" ), msg );
				g_free( msg );
				ret = FALSE;
#if 0
			/* default from libunique is actually to activate the first window
			 * so we rely on the default..
			 */
			} else {
				g_signal_connect(
						handle,
						"message-received",
						G_CALLBACK( on_unique_message_received ),
						instance );
#endif
			} else {
				g_object_set_data( G_OBJECT( instance ), BASE_PROP_HANDLE, handle );
			}
	}

	return( ret );
}

#if 0
static UniqueResponse
on_unique_message_received(
		UniqueApp *app, UniqueCommand command, UniqueMessageData *message, guint time, BaseIUnique *instance )
{
	static const gchar *thisfn = "base_iunique_on_unique_message_received";
	UniqueResponse resp = UNIQUE_RESPONSE_OK;

	switch( command ){
		case UNIQUE_ACTIVATE:
			g_debug( "%s: received message UNIQUE_ACTIVATE", thisfn );
			break;
		default:
			resp = UNIQUE_RESPONSE_PASSTHROUGH;
			break;
	}

	return( resp );
}
#endif

static const gchar *
m_get_application_name( const BaseIUnique *instance )
{
	return( BASE_IUNIQUE_GET_INTERFACE( instance )->get_application_name( instance ));
}
