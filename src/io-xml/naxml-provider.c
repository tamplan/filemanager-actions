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

#include <api/na-iio-factory.h>
#include <api/na-iexporter.h>
#include <api/na-iimporter.h>

#include "naxml-provider.h"
#include "naxml-reader.h"

/* private class data
 */
struct NAXMLProviderClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NAXMLProviderPrivate {
	gboolean dispose_has_run;
};

static NAExporterStr st_formats[] = {

	/* GCONF_SCHEMA_V1: a schema with owner, short and long descriptions;
	 * each action has its own schema addressed by the id
	 * (historical format up to v1.10.x serie)
	 */
	{ "GConfSchemaV1",
			N_( "Export as a full GConf schema (v_1) file" ),
			N_( "Export as a GConf schema file with full key descriptions" ),
			N_( "This used to be the historical export format. " \
				"The exported file may later be imported via :\n" \
				"- Import assistant of the Nautilus Actions Configuration Tool,\n" \
				"- or via the gconftool-2 --import-schema-file command-line tool." ) },

	/* GCONF_SCHEMA_V2: the lightest schema still compatible with gconftool-2 --install-schema-file
	 * (no owner, no short nor long descriptions) - introduced in v 1.11
	 */
	{ "GConfSchemaV2",
			N_( "Export as a light GConf _schema (v2) file" ),
			N_( "Export as a light GConf schema file" ),
			N_( "The exported file may later be imported via :\n" \
				"- Import assistant of the Nautilus Actions Configuration Tool,\n" \
				"- or via the gconftool-2 --import-schema-file command-line tool." ) },

	/* GCONF_ENTRY: not a schema, but a dump of the GConf entry
	 * introduced in v 1.11
	 */
	{ "GConfEntry",
			N_( "Export as a GConf _entry file" ),
			N_( "Export as a GConf entry file" ),
			N_( "This should be the preferred format for newly exported actions.\n" \
				"The exported file may later be imported via :\n" \
				"- Import assistant of the Nautilus Actions Configuration Tool,\n" \
				"- or via the gconftool-2 --load command-line tool." ) },

	{ NULL, NULL, NULL }
};

static GType         st_module_type = 0;
static GObjectClass *st_parent_class = NULL;

static void                 class_init( NAXMLProviderClass *klass );
static void                 instance_init( GTypeInstance *instance, gpointer klass );
static void                 instance_dispose( GObject *object );
static void                 instance_finalize( GObject *object );

static void                 iimporter_iface_init( NAIImporterInterface *iface );
static guint                iimporter_get_version( const NAIImporter *importer );

static void                 iexporter_iface_init( NAIExporterInterface *iface );
static guint                iexporter_get_version( const NAIExporter *exporter );
static const NAExporterStr *iexporter_get_formats( const NAIExporter *exporter );

static void                 iio_factory_iface_init( NAIIOFactoryInterface *iface );
static guint                iio_factory_get_version( const NAIIOFactory *factory );

GType
naxml_provider_get_type( void )
{
	return( st_module_type );
}

void
naxml_provider_register_type( GTypeModule *module )
{
	static const gchar *thisfn = "naxml_provider_register_type";

	static GTypeInfo info = {
		sizeof( NAXMLProviderClass ),
		NULL,
		NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NAXMLProvider ),
		0,
		( GInstanceInitFunc ) instance_init
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

	static const GInterfaceInfo iio_factory_iface_info = {
		( GInterfaceInitFunc ) iio_factory_iface_init,
		NULL,
		NULL
	};

	g_debug( "%s", thisfn );

	st_module_type = g_type_module_register_type( module, G_TYPE_OBJECT, "NAXMLProvider", &info, 0 );

	g_type_module_add_interface( module, st_module_type, NA_IIMPORTER_TYPE, &iimporter_iface_info );

	g_type_module_add_interface( module, st_module_type, NA_IEXPORTER_TYPE, &iexporter_iface_info );

	g_type_module_add_interface( module, st_module_type, NA_IIO_FACTORY_TYPE, &iio_factory_iface_info );
}

static void
class_init( NAXMLProviderClass *klass )
{
	static const gchar *thisfn = "naxml_provider_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NAXMLProviderClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "naxml_provider_instance_init";
	NAXMLProvider *self;

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );
	g_return_if_fail( NA_IS_XML_PROVIDER( instance ));
	self = NAXML_PROVIDER( instance );

	self->private = g_new0( NAXMLProviderPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "naxml_provider_instance_dispose";
	NAXMLProvider *self;

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));
	g_return_if_fail( NA_IS_XML_PROVIDER( object ));
	self = NAXML_PROVIDER( object );

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
	NAXMLProvider *self;

	g_assert( NA_IS_XML_PROVIDER( object ));
	self = NAXML_PROVIDER( object );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

static void
iimporter_iface_init( NAIImporterInterface *iface )
{
	static const gchar *thisfn = "naxml_provider_iimporter_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->get_version = iimporter_get_version;
	iface->import_uri = naxml_reader_import_uri;
}

static guint
iimporter_get_version( const NAIImporter *importer )
{
	return( 1 );
}

static void
iexporter_iface_init( NAIExporterInterface *iface )
{
	static const gchar *thisfn = "naxml_provider_iexporter_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->get_version = iexporter_get_version;
	iface->get_formats = iexporter_get_formats;
	iface->to_file = NULL;
	iface->to_buffer = NULL;
}

static guint
iexporter_get_version( const NAIExporter *exporter )
{
	return( 1 );
}

static const NAExporterStr *
iexporter_get_formats( const NAIExporter *exporter )
{
	return( st_formats );
}

static void
iio_factory_iface_init( NAIIOFactoryInterface *iface )
{
	static const gchar *thisfn = "naxml_provider_iio_factory_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->get_version = iio_factory_get_version;
	iface->read_start = NULL;
	iface->read_value = NULL;
	iface->read_done = NULL;
	iface->write_start = NULL;
	iface->write_value = NULL;
	iface->write_done = NULL;
}

static guint
iio_factory_get_version( const NAIIOFactory *factory )
{
	return( 1 );
}
