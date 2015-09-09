/*
 * FileManager-Actions
 * A file-manager extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
 *
 * FileManager-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * FileManager-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FileManager-Actions; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
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

#include "fma-ioption.h"

/* private interface data
 */
struct _FMAIOptionInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* data set against the instance
 *
 * Initialization here mainly means setting the weak ref against the instance.
 */
typedef struct {
	gboolean initialized;
}
	IOptionData;

#define IOPTION_PROP_DATA				"prop-ioption-data"

static guint st_initializations = 0;	/* interface initialization count */

static GType        register_type( void );
static void         interface_base_init( FMAIOptionInterface *iface );
static void         interface_base_finalize( FMAIOptionInterface *iface );

static guint        ioption_get_version( const FMAIOption *instance );
static IOptionData *get_ioption_data( FMAIOption *instance );
static void         on_instance_finalized( gpointer user_data, FMAIOption *instance );

/**
 * fma_ioption_get_type:
 *
 * Returns: the #GType type of this interface.
 */
GType
fma_ioption_get_type( void )
{
	static GType type = 0;

	if( !type ){
		type = register_type();
	}

	return( type );
}

/*
 * fma_ioption_register_type:
 *
 * Registers this interface.
 */
static GType
register_type( void )
{
	static const gchar *thisfn = "fma_ioption_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( FMAIOptionInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "FMAIOption", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( FMAIOptionInterface *iface )
{
	static const gchar *thisfn = "fma_ioption_interface_base_init";

	if( !st_initializations ){

		g_debug( "%s: iface=%p (%s)", thisfn, ( void * ) iface, G_OBJECT_CLASS_NAME( iface ));

		iface->private = g_new0( FMAIOptionInterfacePrivate, 1 );

		iface->get_version = ioption_get_version;
	}

	st_initializations += 1;
}

static void
interface_base_finalize( FMAIOptionInterface *iface )
{
	static const gchar *thisfn = "fma_ioption_interface_base_finalize";

	st_initializations -= 1;

	if( !st_initializations ){

		g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

		g_free( iface->private );
	}
}

static guint
ioption_get_version( const FMAIOption *instance )
{
	return( 1 );
}

static IOptionData *
get_ioption_data( FMAIOption *instance )
{
	IOptionData *data;

	data = ( IOptionData * ) g_object_get_data( G_OBJECT( instance ), IOPTION_PROP_DATA );

	if( !data ){
		data = g_new0( IOptionData, 1 );
		g_object_set_data( G_OBJECT( instance ), IOPTION_PROP_DATA, data );
		g_object_weak_ref( G_OBJECT( instance ), ( GWeakNotify ) on_instance_finalized, NULL );

		data->initialized = TRUE;
	}

	return( data );
}

static void
on_instance_finalized( gpointer user_data, FMAIOption *instance )
{
	static const gchar *thisfn = "fma_ioption_on_instance_finalized";
	IOptionData *data;

	g_debug( "%s: user_data=%p, instance=%p", thisfn, ( void * ) user_data, ( void * ) instance );

	data = get_ioption_data( instance );

	g_free( data );
}

/*
 * fma_ioption_get_id:
 * @option: this #FMAIOption instance.
 *
 * Returns: the string identifier of the format, as a newly
 * allocated string which should be g_free() by the caller.
 */
gchar *
fma_ioption_get_id( const FMAIOption *option )
{
	gchar *id;

	g_return_val_if_fail( FMA_IS_IOPTION( option ), NULL );

	get_ioption_data( FMA_IOPTION( option ));
	id = NULL;

	if( FMA_IOPTION_GET_INTERFACE( option )->get_id ){
		id = FMA_IOPTION_GET_INTERFACE( option )->get_id( option );
	}

	return( id );
}

/*
 * fma_ioption_get_label:
 * @option: this #FMAIOption instance.
 *
 * Returns: the UTF-8 localizable label of the format, as a newly
 * allocated string which should be g_free() by the caller.
 */
gchar *
fma_ioption_get_label( const FMAIOption *option )
{
	gchar *label;

	g_return_val_if_fail( FMA_IS_IOPTION( option ), NULL );

	get_ioption_data( FMA_IOPTION( option ));
	label = NULL;

	if( FMA_IOPTION_GET_INTERFACE( option )->get_label ){
		label = FMA_IOPTION_GET_INTERFACE( option )->get_label( option );
	}

	return( label );
}

/*
 * fma_ioption_get_description:
 * @format: this #FMAExportFormat object.
 *
 * Returns: the UTF-8 localizable description of the format, as a newly
 * allocated string which should be g_free() by the caller.
 */
gchar *
fma_ioption_get_description( const FMAIOption *option )
{
	gchar *description;

	g_return_val_if_fail( FMA_IS_IOPTION( option ), NULL );

	get_ioption_data( FMA_IOPTION( option ));
	description = NULL;

	if( FMA_IOPTION_GET_INTERFACE( option )->get_description ){
		description = FMA_IOPTION_GET_INTERFACE( option )->get_description( option );
	}

	return( description );
}

/*
 * fma_ioption_get_pixbuf:
 * @option: this #FMAIOption instance.
 *
 * Returns: a new reference to the #GdkPixbuf image associated with this format,
 * or %NULL.
 */
GdkPixbuf *
fma_ioption_get_pixbuf( const FMAIOption *option )
{
	GdkPixbuf *pixbuf;

	g_return_val_if_fail( FMA_IS_IOPTION( option ), NULL );

	get_ioption_data( FMA_IOPTION( option ));
	pixbuf = NULL;

	if( FMA_IOPTION_GET_INTERFACE( option )->get_pixbuf ){
		pixbuf = FMA_IOPTION_GET_INTERFACE( option )->get_pixbuf( option );
	}

	return( pixbuf );
}
