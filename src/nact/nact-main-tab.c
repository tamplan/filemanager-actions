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

#include "nact-window.h"
#include "nact-itab-updatable.h"

/* private interface data
 */
struct NactITabUpdatableInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* signals
 */
enum {
	SELECTION_UPDATED,
	LAST_SIGNAL
};

static gint  st_signals[ LAST_SIGNAL ] = { 0 };

static GType register_type( void );
static void  interface_base_init( NactITabUpdatableInterface *klass );
static void  interface_base_finalize( NactITabUpdatableInterface *klass );

GType
nact_itab_updatable_get_type( void )
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
	static const gchar *thisfn = "nact_itab_updatable_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NactITabUpdatableInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "NactITabUpdatable", &info, 0 );

	g_type_interface_add_prerequisite( type, NACT_WINDOW_TYPE );

	return( type );
}

static void
interface_base_init( NactITabUpdatableInterface *klass )
{
	static const gchar *thisfn = "nact_itab_updatable_interface_base_init";
	static gboolean initialized = FALSE;

	if( !initialized ){
		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NactITabUpdatableInterfacePrivate, 1 );

		/**
		 * "nact-tab-updatable-selection-updated":
		 *
		 * This signal is emitted to inform updatable tabs that a new
		 * item has been selected, and the displays should reflect it.
		 *
		 * NactMainWindow emits this signal with user_data = count_selected.
		 */
		st_signals[ SELECTION_UPDATED ] = g_signal_new(
				ITAB_UPDATABLE_SIGNAL_SELECTION_UPDATED,
				G_TYPE_OBJECT,
				G_SIGNAL_RUN_LAST,
				0,					/* no default handler */
				NULL,
				NULL,
				g_cclosure_marshal_VOID__POINTER,
				G_TYPE_NONE,
				1,
				G_TYPE_POINTER );

		initialized = TRUE;
	}
}

static void
interface_base_finalize( NactITabUpdatableInterface *klass )
{
	static const gchar *thisfn = "nact_itab_updatable_interface_base_finalize";
	static gboolean finalized = FALSE ;

	if( !finalized ){
		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );

		finalized = TRUE;
	}
}

/*void
nact_itab_updatable_update_selection( NactITabUpdatable *instance )
{
	v_selection_updated( instance );
}

static void
v_selection_updated( NactITabUpdatable *instance )
{
	static const gchar *thisfn = "nact_itab_updatable_v_selection_updated";

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

	if( NACT_ITAB_UPDATABLE_GET_INTERFACE( instance )->selection_updated ){
		NACT_ITAB_UPDATABLE_GET_INTERFACE( instance )->selection_updated( instance );
	}
}*/
