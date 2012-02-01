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

#include <api/na-object-api.h>

#include "nact-main-tab.h"
#include "nact-match-list.h"
#include "nact-imimetypes-tab.h"

/* private interface data
 */
struct _NactIMimetypesTabInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* the identifier of this notebook page in the Match dialog
 */
#define ITAB_NAME						"mimetypes"

static guint st_initializations = 0;	/* interface initialization count */

static GType   register_type( void );
static void    interface_base_init( NactIMimetypesTabInterface *klass );
static void    interface_base_finalize( NactIMimetypesTabInterface *klass );

static void    on_base_initialize_gtk( NactIMimetypesTab *instance, GtkWindow *toplevel, gpointer user_data );
static void    on_base_initialize_window( NactIMimetypesTab *instance, gpointer user_data );

static void    on_main_selection_changed( BaseWindow *window, GList *selected_items, gpointer user_data );

static GSList *get_mimetypes( void *context );
static void    set_mimetypes( void *context, GSList *filters );

static void    on_instance_finalized( gpointer user_data, NactIMimetypesTab *instance );

GType
nact_imimetypes_tab_get_type( void )
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
	static const gchar *thisfn = "nact_imimetypes_tab_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NactIMimetypesTabInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "NactIMimetypesTab", &info, 0 );

	g_type_interface_add_prerequisite( type, BASE_TYPE_WINDOW );

	return( type );
}

static void
interface_base_init( NactIMimetypesTabInterface *klass )
{
	static const gchar *thisfn = "nact_imimetypes_tab_interface_base_init";

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NactIMimetypesTabInterfacePrivate, 1 );
	}

	st_initializations += 1;
}

static void
interface_base_finalize( NactIMimetypesTabInterface *klass )
{
	static const gchar *thisfn = "nact_imimetypes_tab_interface_base_finalize";

	st_initializations -= 1;

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );
	}
}

/*
 * nact_imimetypes_tab_init:
 * @instance: this #NactIMimetypesTab instance.
 *
 * Initialize the interface
 * Connect to #BaseWindow signals
 */
void
nact_imimetypes_tab_init( NactIMimetypesTab *instance )
{
	static const gchar *thisfn = "nact_imimetypes_tab_init";

	g_return_if_fail( NACT_IS_IMIMETYPES_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn,
			( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			BASE_SIGNAL_INITIALIZE_GTK,
			G_CALLBACK( on_base_initialize_gtk ));

	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			BASE_SIGNAL_INITIALIZE_WINDOW,
			G_CALLBACK( on_base_initialize_window ));

	nact_main_tab_init( NACT_MAIN_WINDOW( instance ), TAB_MIMETYPES );

	g_object_weak_ref( G_OBJECT( instance ), ( GWeakNotify ) on_instance_finalized, NULL );
}

/*
 * on_base_initialize_gtk:
 * @window: this #NactIMimetypesTab instance.
 *
 * Initializes the tab widget at initial load.
 */
static void
on_base_initialize_gtk( NactIMimetypesTab *instance, GtkWindow *toplevel, void *user_data )
{
	static const gchar *thisfn = "nact_imimetypes_tab_on_base_initialize_gtk";

	g_return_if_fail( NACT_IS_IMIMETYPES_TAB( instance ));

	g_debug( "%s: instance=%p (%s), toplevel=%p, user_data=%p",
			thisfn,
			( void * ) instance, G_OBJECT_TYPE_NAME( instance ),
			( void * ) toplevel,
			( void * ) user_data );

	nact_match_list_init_with_args(
			BASE_WINDOW( instance ),
			ITAB_NAME,
			TAB_MIMETYPES,
			base_window_get_widget( BASE_WINDOW( instance ), "MimetypesTreeView" ),
			base_window_get_widget( BASE_WINDOW( instance ), "AddMimetypeButton" ),
			base_window_get_widget( BASE_WINDOW( instance ), "RemoveMimetypeButton" ),
			( pget_filters ) get_mimetypes,
			( pset_filters ) set_mimetypes,
			NULL,
			NULL,
			MATCH_LIST_MUST_MATCH_ONE_OF,
			_( "Mimetype filter" ),
			TRUE );
}

/*
 * on_base_initialize_window:
 * @window: this #NactIMimetypesTab instance.
 *
 * Initializes the tab widget at each time the widget will be displayed.
 * Connect signals and setup runtime values.
 */
static void
on_base_initialize_window( NactIMimetypesTab *instance, void *user_data )
{
	static const gchar *thisfn = "nact_imimetypes_tab_on_base_initialize_window";

	g_return_if_fail( NACT_IS_IMIMETYPES_TAB( instance ));

	g_debug( "%s: instance=%p (%s), user_data=%p",
			thisfn,
			( void * ) instance, G_OBJECT_TYPE_NAME( instance ),
			( void * ) user_data );

	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			MAIN_SIGNAL_SELECTION_CHANGED,
			G_CALLBACK( on_main_selection_changed ));
}

static void
on_main_selection_changed( BaseWindow *window, GList *selected_items, gpointer user_data )
{
	/* nothing to do here */
}

static GSList *
get_mimetypes( void *context )
{
	return( na_object_get_mimetypes( context ));
}

static void
set_mimetypes( void *context, GSList *filters )
{
	na_object_set_mimetypes( context, filters );
}

static void
on_instance_finalized( gpointer user_data, NactIMimetypesTab *instance )
{
	static const gchar *thisfn = "nact_imimetypes_tab_on_instance_finalized";

	g_debug( "%s: instance=%p, user_data=%p", thisfn, ( void * ) instance, ( void * ) user_data );
}
