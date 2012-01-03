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

#include "na-ioption.h"

/* private interface data
 */
struct _NAIOptionInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

static guint st_initializations = 0;	/* interface initialization count */

#define IOPTION_DATA_INITIALIZED		"ioption-data-initialized"

static GType    register_type( void );
static void     interface_init( NAIOptionInterface *iface );
static void     interface_finalize( NAIOptionInterface *iface );

static guint    ioption_get_version( const NAIOption *instance );
static void     check_for_initialized_instance( NAIOption *instance );
static void     on_instance_finalized( gpointer user_data, GObject *instance );
static gboolean option_get_initialized( NAIOption *instance );
static void     option_set_initialized( NAIOption *instance, gboolean initialized );

/**
 * na_ioption_get_type:
 *
 * Returns: the #GType type of this interface.
 */
GType
na_ioption_get_type( void )
{
	static GType type = 0;

	if( !type ){
		type = register_type();
	}

	return( type );
}

/*
 * na_ioption_register_type:
 *
 * Registers this interface.
 */
static GType
register_type( void )
{
	static const gchar *thisfn = "na_ioption_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NAIOptionInterface ),
		( GBaseInitFunc ) interface_init,
		( GBaseFinalizeFunc ) interface_finalize,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_INTERFACE, "NAIOption", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_init( NAIOptionInterface *iface )
{
	static const gchar *thisfn = "na_ioption_interface_init";

	if( !st_initializations ){

		g_debug( "%s: iface=%p (%s)", thisfn, ( void * ) iface, G_OBJECT_CLASS_NAME( iface ));

		iface->private = g_new0( NAIOptionInterfacePrivate, 1 );

		iface->get_version = ioption_get_version;
	}

	st_initializations += 1;
}

static void
interface_finalize( NAIOptionInterface *iface )
{
	static const gchar *thisfn = "na_ioption_interface_finalize";

	st_initializations -= 1;

	if( !st_initializations ){

		g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

		g_free( iface->private );
	}
}

static guint
ioption_get_version( const NAIOption *instance )
{
	return( 1 );
}

/*
 * na_ioption_instance_init:
 * @instance: the object which implements this #NAIOptionsList interface.
 *
 * Initialize all #NAIOptionsList-relative properties of the implementation
 * object at instanciation time.
 */
static void
check_for_initialized_instance( NAIOption *instance )
{
	static const gchar *thisfn = "na_ioption_check_for_initialized_instance";

	if( !option_get_initialized( instance )){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

		g_object_weak_ref( G_OBJECT( instance ), ( GWeakNotify ) on_instance_finalized, NULL );

		option_set_initialized( instance, TRUE );
	}
}

static void
on_instance_finalized( gpointer user_data, GObject *instance )
{
	static const gchar *thisfn = "na_ioption_on_instance_finalized";

	g_debug( "%s: user_data=%p, instance=%p", thisfn, ( void * ) user_data, ( void * ) instance );
}

/* whether the instance has been initialized
 *
 * initializing the instance let us register a 'weak notify' signal on the instance
 * we will so be able to free any allocated resources when the instance will be
 * finalized
 */
static gboolean
option_get_initialized( NAIOption *instance )
{
	gboolean initialized;

	initialized = ( gboolean ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( instance ), IOPTION_DATA_INITIALIZED ));

	return( initialized );
}

static void
option_set_initialized( NAIOption *instance, gboolean initialized )
{
	g_object_set_data( G_OBJECT( instance ), IOPTION_DATA_INITIALIZED, GUINT_TO_POINTER( initialized ));
}

/*
 * na_ioption_get_id:
 * @option: this #NAIOption instance.
 *
 * Returns: the string identifier of the format, as a newly
 * allocated string which should be g_free() by the caller.
 */
gchar *
na_ioption_get_id( const NAIOption *option )
{
	gchar *id;

	g_return_val_if_fail( NA_IS_IOPTION( option ), NULL );

	check_for_initialized_instance( NA_IOPTION( option ));
	id = NULL;

	if( NA_IOPTION_GET_INTERFACE( option )->get_id ){
		id = NA_IOPTION_GET_INTERFACE( option )->get_id( option );
	}

	return( id );
}

/*
 * na_ioption_get_label:
 * @option: this #NAIOption instance.
 *
 * Returns: the UTF-8 localizable label of the format, as a newly
 * allocated string which should be g_free() by the caller.
 */
gchar *
na_ioption_get_label( const NAIOption *option )
{
	gchar *label;

	g_return_val_if_fail( NA_IS_IOPTION( option ), NULL );

	check_for_initialized_instance( NA_IOPTION( option ));
	label = NULL;

	if( NA_IOPTION_GET_INTERFACE( option )->get_label ){
		label = NA_IOPTION_GET_INTERFACE( option )->get_label( option );
	}

	return( label );
}

/*
 * na_ioption_get_description:
 * @format: this #NAExportFormat object.
 *
 * Returns: the UTF-8 localizable description of the format, as a newly
 * allocated string which should be g_free() by the caller.
 */
gchar *
na_ioption_get_description( const NAIOption *option )
{
	gchar *description;

	g_return_val_if_fail( NA_IS_IOPTION( option ), NULL );

	check_for_initialized_instance( NA_IOPTION( option ));
	description = NULL;

	if( NA_IOPTION_GET_INTERFACE( option )->get_description ){
		description = NA_IOPTION_GET_INTERFACE( option )->get_description( option );
	}

	return( description );
}

/*
 * na_ioption_get_pixbuf:
 * @option: this #NAIOption instance.
 *
 * Returns: a new reference to the #GdkPixbuf image associated with this format,
 * or %NULL.
 */
GdkPixbuf *
na_ioption_get_pixbuf( const NAIOption *option )
{
	GdkPixbuf *pixbuf;

	g_return_val_if_fail( NA_IS_IOPTION( option ), NULL );

	check_for_initialized_instance( NA_IOPTION( option ));
	pixbuf = NULL;

	if( NA_IOPTION_GET_INTERFACE( option )->get_pixbuf ){
		pixbuf = NA_IOPTION_GET_INTERFACE( option )->get_pixbuf( option );
	}

	return( pixbuf );
}
