/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010 Pierre Wieser and others (see AUTHORS)
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

#include <api/na-core-utils.h>
#include <api/na-iio-provider.h>
#include <api/na-iio-factory.h>

#include "nadp-desktop-provider.h"
#include "nadp-keys.h"
#include "nadp-reader.h"
#include "nadp-writer.h"

/* private class data
 */
struct NadpDesktopProviderClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

static GType         st_module_type = 0;
static GObjectClass *st_parent_class = NULL;

static void    class_init( NadpDesktopProviderClass *klass );
static void    instance_init( GTypeInstance *instance, gpointer klass );
static void    instance_dispose( GObject *object );
static void    instance_finalize( GObject *object );

static void    iio_provider_iface_init( NAIIOProviderInterface *iface );
static gchar  *iio_provider_get_id( const NAIIOProvider *provider );
static gchar  *iio_provider_get_name( const NAIIOProvider *provider );
static guint   iio_provider_get_version( const NAIIOProvider *provider );

static void    iio_factory_iface_init( NAIIOFactoryInterface *iface );
static guint   iio_factory_get_version( const NAIIOFactory *reader );
static void    iio_factory_read_start( const NAIIOFactory *reader, void *reader_data, NAIDataFactory *serializable, GSList **messages );
static GValue *iio_factory_read_value( const NAIIOFactory *reader, void *reader_data, const NadfIdType *iddef, GSList **messages );
static void    iio_factory_read_done( const NAIIOFactory *reader, void *reader_data, NAIDataFactory *serializable, GSList **messages );

GType
nadp_desktop_provider_get_type( void )
{
	return( st_module_type );
}

void
nadp_desktop_provider_register_type( GTypeModule *module )
{
	static const gchar *thisfn = "nadp_desktop_provider_register_type";

	static GTypeInfo info = {
		sizeof( NadpDesktopProviderClass ),
		NULL,
		NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NadpDesktopProvider ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	static const GInterfaceInfo iio_provider_iface_info = {
		( GInterfaceInitFunc ) iio_provider_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo iio_factory_iface_info = {
		( GInterfaceInitFunc ) iio_factory_iface_init,
		NULL,
		NULL
	};

	g_debug( "%s", thisfn );

	st_module_type = g_type_module_register_type( module, G_TYPE_OBJECT, "NadpDesktopProvider", &info, 0 );

	g_type_module_add_interface( module, st_module_type, NA_IIO_PROVIDER_TYPE, &iio_provider_iface_info );

	g_type_module_add_interface( module, st_module_type, NA_IIO_FACTORY_TYPE, &iio_factory_iface_info );
}

static void
class_init( NadpDesktopProviderClass *klass )
{
	static const gchar *thisfn = "nadp_desktop_provider_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NadpDesktopProviderClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nadp_desktop_provider_instance_init";
	NadpDesktopProvider *self;

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );
	g_return_if_fail( NADP_IS_DESKTOP_PROVIDER( instance ));
	self = NADP_DESKTOP_PROVIDER( instance );

	self->private = g_new0( NadpDesktopProviderPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "nadp_desktop_provider_instance_dispose";
	NadpDesktopProvider *self;

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));
	g_return_if_fail( NADP_IS_DESKTOP_PROVIDER( object ));
	self = NADP_DESKTOP_PROVIDER( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	NadpDesktopProvider *self;

	g_assert( NADP_IS_DESKTOP_PROVIDER( object ));
	self = NADP_DESKTOP_PROVIDER( object );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

static void
iio_provider_iface_init( NAIIOProviderInterface *iface )
{
	static const gchar *thisfn = "nadp_desktop_provider_iio_provider_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->get_version = iio_provider_get_version;
	iface->get_id = iio_provider_get_id;
	iface->get_name = iio_provider_get_name;
	iface->read_items = nadp_iio_provider_read_items;
	iface->is_willing_to_write = nadp_iio_provider_is_willing_to_write;
	iface->is_able_to_write = nadp_iio_provider_is_able_to_write;
	iface->write_item = nadp_iio_provider_write_item;
	iface->delete_item = nadp_iio_provider_delete_item;
}

static guint
iio_provider_get_version( const NAIIOProvider *provider )
{
	return( 1 );
}

static gchar *
iio_provider_get_id( const NAIIOProvider *provider )
{
	return( g_strdup( "na-desktop" ));
}

static gchar *
iio_provider_get_name( const NAIIOProvider *provider )
{
	return( g_strdup( _( "Nautilus-Actions Desktop I/O Provider" )));
}

static void
iio_factory_iface_init( NAIIOFactoryInterface *iface )
{
	static const gchar *thisfn = "nadp_desktop_provider_iio_factory_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->get_version = iio_factory_get_version;
	iface->read_start = iio_factory_read_start;
	iface->read_value = iio_factory_read_value;
	iface->read_done = iio_factory_read_done;
	iface->write_start = NULL;
	iface->write_value = NULL;
	iface->write_done = NULL;
}

static guint
iio_factory_get_version( const NAIIOFactory *reader )
{
	return( 1 );
}

/*
 * called before starting with reading an object
 */
static void
iio_factory_read_start( const NAIIOFactory *reader, void *reader_data, NAIDataFactory *serializable, GSList **messages )
{
	static const gchar *thisfn = "nadp_desktop_provider_iio_factory_read_start";
	/*NAObjectProfile *profile;*/

	g_debug( "%s: reader=%p (%s), reader_data=%p, serializable=%p (%s), messages=%p",
			thisfn,
			( void * ) reader, G_OBJECT_TYPE_NAME( reader ),
			( void * ) reader_data,
			( void * ) serializable, G_OBJECT_TYPE_NAME( serializable ),
			( void * ) messages );

	g_return_if_fail( NA_IS_IIO_FACTORY( reader ));
	g_return_if_fail( NADP_IS_DESKTOP_PROVIDER( reader ));
	g_return_if_fail( NA_IS_IDATA_FACTORY( serializable ));

	if( !NADP_DESKTOP_PROVIDER( reader )->private->dispose_has_run ){

	}
}

/*
 * reading any data from a desktop file requires:
 * - a NadpDesktopFile object which has been initialized with the .desktop file
 *   -> has been attached to the NAObjectItem in get_item() above
 * - the data type (+ reading default value)
 * - group and key names
 */
static GValue *
iio_factory_read_value( const NAIIOFactory *reader, void *reader_data, const NadfIdType *iddef, GSList **messages )
{
	static const gchar *thisfn = "nadp_desktop_provider_iio_factory_read_value";
	NadpReaderData *nrd;
	GValue *value;
	gchar *group, *key;
	gchar *msg;
	gchar *str_value;
	gboolean bool_value;
	GSList *slist_value;

	/*g_debug( "%s: reader=%p (%s), reader_data=%p, iddef=%p, messages=%p",
			thisfn,
			( void * ) reader, G_OBJECT_TYPE_NAME( reader ),
			( void * ) reader_data,
			( void * ) iddef,
			( void * ) messages );*/

	g_return_val_if_fail( NA_IS_IIO_FACTORY( reader ), NULL );
	g_return_val_if_fail( NADP_IS_DESKTOP_PROVIDER( reader ), NULL );
	g_return_val_if_fail( iddef->serializable, NULL );

	value = NULL;

	if( !NADP_DESKTOP_PROVIDER( reader )->private->dispose_has_run ){

		nrd = ( NadpReaderData * ) reader_data;
		g_return_val_if_fail( NADP_IS_DESKTOP_FILE( nrd->ndf ), NULL );

		if( nadp_keys_get_group_and_key( iddef, &group, &key )){

			value = g_new0( GValue, 1 );

			switch( iddef->type ){

				case NADF_TYPE_LOCALE_STRING:
					g_value_init( value, G_TYPE_STRING );
					str_value = nadp_desktop_file_get_locale_string( nrd->ndf, group, key, iddef->default_value );
					g_value_set_string( value, str_value );
					g_free( str_value );
					break;

				case NADF_TYPE_STRING:
					g_value_init( value, G_TYPE_STRING );
					str_value = nadp_desktop_file_get_string( nrd->ndf, group, key, iddef->default_value );
					g_value_set_string( value, str_value );
					g_free( str_value );
					break;

				case NADF_TYPE_BOOLEAN:
					g_value_init( value, G_TYPE_BOOLEAN );
					bool_value = nadp_desktop_file_get_boolean( nrd->ndf, group, key, na_core_utils_boolean_from_string( iddef->default_value ));
					g_value_set_boolean( value, bool_value );
					break;

				case NADF_TYPE_STRING_LIST:
					g_value_init( value, G_TYPE_POINTER );
					slist_value = nadp_desktop_file_get_string_list( nrd->ndf, group, key, iddef->default_value );
					g_value_set_pointer( value, slist_value );
					break;

				default:
					msg = g_strdup_printf( "%s: %d: unknown data type.", thisfn, iddef->type );
					g_warning( "%s", msg );
					*messages = g_slist_append( *messages, msg );
			}

			g_free( key );
			g_free( group );
		}
	}

	return( value );
}

/*
 * called when each NAIDataFactory object has been readen
 * nothing to do here
 */
static void
iio_factory_read_done( const NAIIOFactory *reader, void *reader_data, NAIDataFactory *serializable, GSList **messages )
{
	static const gchar *thisfn = "nadp_desktop_provider_iio_factory_read_done";
	/*NAObjectProfile *profile;*/

	g_debug( "%s: reader=%p (%s), reader_data=%p, serializable=%p (%s), messages=%p",
			thisfn,
			( void * ) reader, G_OBJECT_TYPE_NAME( reader ),
			( void * ) reader_data,
			( void * ) serializable, G_OBJECT_TYPE_NAME( serializable ),
			( void * ) messages );

	g_return_if_fail( NA_IS_IIO_FACTORY( reader ));
	g_return_if_fail( NADP_IS_DESKTOP_PROVIDER( reader ));
	g_return_if_fail( NA_IS_IDATA_FACTORY( serializable ));

	if( !NADP_DESKTOP_PROVIDER( reader )->private->dispose_has_run ){

	}
}
