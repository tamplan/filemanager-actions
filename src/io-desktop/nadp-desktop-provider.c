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
#include <api/na-ifactory-provider.h>

#include "nadp-desktop-provider.h"
#include "nadp-keys.h"
#include "nadp-monitor.h"
#include "nadp-reader.h"
#include "nadp-writer.h"

/* private class data
 */
struct NadpDesktopProviderClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

extern NAIExporterFormat nadp_formats[];

static GType         st_module_type = 0;
static GObjectClass *st_parent_class = NULL;
static guint         st_timeout_msec = 100;
static guint         st_timeout_usec = 100000;

static void                     class_init( NadpDesktopProviderClass *klass );
static void                     instance_init( GTypeInstance *instance, gpointer klass );
static void                     instance_dispose( GObject *object );
static void                     instance_finalize( GObject *object );

static void                     iio_provider_iface_init( NAIIOProviderInterface *iface );
static gchar                   *iio_provider_get_id( const NAIIOProvider *provider );
static gchar                   *iio_provider_get_name( const NAIIOProvider *provider );
static guint                    iio_provider_get_version( const NAIIOProvider *provider );

static void                     ifactory_provider_iface_init( NAIFactoryProviderInterface *iface );
static guint                    ifactory_provider_get_version( const NAIFactoryProvider *reader );

static void                     iimporter_iface_init( NAIImporterInterface *iface );
static guint                    iimporter_get_version( const NAIImporter *importer );

static void                     iexporter_iface_init( NAIExporterInterface *iface );
static guint                    iexporter_get_version( const NAIExporter *exporter );
static gchar                   *iexporter_get_name( const NAIExporter *exporter );
static const NAIExporterFormat *iexporter_get_formats( const NAIExporter *exporter );

static gboolean                 on_monitor_timeout( NadpDesktopProvider *provider );
static gulong                   time_val_diff( const GTimeVal *recent, const GTimeVal *old );

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

	static const GInterfaceInfo ifactory_provider_iface_info = {
		( GInterfaceInitFunc ) ifactory_provider_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo iimporter_iface_info = {
		( GInterfaceInitFunc ) iimporter_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo iexporter_iface_info = {
		( GInterfaceInitFunc ) iexporter_iface_init,
		NULL,
		NULL
	};

	g_debug( "%s", thisfn );

	st_module_type = g_type_module_register_type( module, G_TYPE_OBJECT, "NadpDesktopProvider", &info, 0 );

	g_type_module_add_interface( module, st_module_type, NA_IIO_PROVIDER_TYPE, &iio_provider_iface_info );

	g_type_module_add_interface( module, st_module_type, NA_IFACTORY_PROVIDER_TYPE, &ifactory_provider_iface_info );

	g_type_module_add_interface( module, st_module_type, NA_IIMPORTER_TYPE, &iimporter_iface_info );

	g_type_module_add_interface( module, st_module_type, NA_IEXPORTER_TYPE, &iexporter_iface_info );
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
	self->private->monitors = NULL;
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

		nadp_desktop_provider_release_monitors( self );

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
	iface->duplicate_data = nadp_iio_provider_duplicate_data;
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
ifactory_provider_iface_init( NAIFactoryProviderInterface *iface )
{
	static const gchar *thisfn = "nadp_desktop_provider_ifactory_provider_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->get_version = ifactory_provider_get_version;
	iface->read_start = nadp_reader_ifactory_provider_read_start;
	iface->read_data = nadp_reader_ifactory_provider_read_data;
	iface->read_done = nadp_reader_ifactory_provider_read_done;
	iface->write_start = nadp_writer_ifactory_provider_write_start;
	iface->write_data = nadp_writer_ifactory_provider_write_data;
	iface->write_done = nadp_writer_ifactory_provider_write_done;
}

static guint
ifactory_provider_get_version( const NAIFactoryProvider *reader )
{
	return( 1 );
}

static void
iimporter_iface_init( NAIImporterInterface *iface )
{
	static const gchar *thisfn = "nadp_desktop_provider_iimporter_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->get_version = iimporter_get_version;
	iface->import_from_uri = nadp_reader_iimporter_import_from_uri;
}

static guint
iimporter_get_version( const NAIImporter *importer )
{
	return( 1 );
}

static void
iexporter_iface_init( NAIExporterInterface *iface )
{
	static const gchar *thisfn = "nadp_desktop_iexporter_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->get_version = iexporter_get_version;
	iface->get_name = iexporter_get_name;
	iface->get_formats = iexporter_get_formats;
	iface->to_file = nadp_writer_iexporter_export_to_file;
	iface->to_buffer = nadp_writer_iexporter_export_to_buffer;
}

static guint
iexporter_get_version( const NAIExporter *exporter )
{
	return( 1 );
}

static gchar *
iexporter_get_name( const NAIExporter *exporter )
{
	return( g_strdup( "NA Desktop Exporter" ));
}

static const NAIExporterFormat *
iexporter_get_formats( const NAIExporter *exporter )
{
	return( nadp_formats );
}

/**
 * nadp_desktop_provider_add_monitor:
 * @provider: this #NadpDesktopProvider object.
 * @dir: the path to the directory to be monitored. May not exist.
 *
 * Installs a GIO monitor on the given directory.
 */
void
nadp_desktop_provider_add_monitor( NadpDesktopProvider *provider, const gchar *dir )
{
	NadpMonitor *monitor;

	g_return_if_fail( NADP_IS_DESKTOP_PROVIDER( provider ));

	if( !provider->private->dispose_has_run ){

		monitor = nadp_monitor_new( provider, dir );
		provider->private->monitors = g_list_prepend( provider->private->monitors, monitor );
	}
}

/**
 * nadp_desktop_provider_on_monitor_event:
 * @provider: this #NadpDesktopProvider object.
 *
 * Factorize events received from GIO when monitoring desktop directories.
 */
void
nadp_desktop_provider_on_monitor_event( NadpDesktopProvider *provider )
{
	static const gchar *thisfn = "nadp_desktop_provider_on_monitor_event";
	GTimeVal now;

	g_return_if_fail( NADP_IS_DESKTOP_PROVIDER( provider ));

	if( !provider->private->dispose_has_run ){

		g_get_current_time( &now );
		g_debug( "%s: time=%ld.%ld", thisfn, now.tv_sec, now.tv_usec );

		g_get_current_time( &provider->private->last_event );

		if( !provider->private->event_source_id ){
			provider->private->event_source_id =
				g_timeout_add( st_timeout_msec, ( GSourceFunc ) on_monitor_timeout, provider );
		}
	}
}

/**
 * nadp_desktop_provider_release_monitors:
 * @provider: this #NadpDesktopProvider object.
 *
 * Release previously set desktop monitors.
 */
void
nadp_desktop_provider_release_monitors( NadpDesktopProvider *provider )
{
	g_return_if_fail( NADP_IS_DESKTOP_PROVIDER( provider ));

	if( provider->private->monitors ){

		g_list_foreach( provider->private->monitors, ( GFunc ) g_object_unref, NULL );
		g_list_free( provider->private->monitors );
		provider->private->monitors = NULL;
	}
}

static gboolean
on_monitor_timeout( NadpDesktopProvider *provider )
{
	static const gchar *thisfn = "nadp_desktop_provider_on_monitor_timeout";
	GTimeVal now;
	gulong diff;

	g_debug( "%s", thisfn );

	g_get_current_time( &now );
	diff = time_val_diff( &now, &provider->private->last_event );
	if( diff < st_timeout_usec ){
		g_debug( "%s: unexpired timeout: returning True", thisfn );
		return( TRUE );
	}

	g_debug( "%s: expired timeout: advertising IIOProvider, returning False", thisfn );
	na_iio_provider_item_changed( NA_IIO_PROVIDER( provider ));
	provider->private->event_source_id = 0;
	return( FALSE );
}

/*
 * returns the difference in microseconds.
 */
static gulong
time_val_diff( const GTimeVal *recent, const GTimeVal *old )
{
	gulong microsec = 1000000 * ( recent->tv_sec - old->tv_sec );
	microsec += recent->tv_usec  - old->tv_usec;
	return( microsec );
}
