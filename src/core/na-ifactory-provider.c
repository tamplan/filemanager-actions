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

#include <api/na-iio-provider.h>
#include <api/na-ifactory-provider.h>

#include "na-factory-object.h"
#include "na-factory-provider.h"

/**
 * SECTION: ifactory-provider
 * @title: NAIFactoryProvider
 * @short_description: The Data Factory Management System
 * @include: nautilus-actions/na-ifactory_provider.h
 *
 * &prodname; has to deal with a relatively great number of elementary datas,
 * reading them from different supports, storing and displaying them,
 * then re-writing these same datas, with several output formats, and so on.
 *
 * This has rapidly become a pain, if not just a bug generator.
 * Each new data must be described, have a schema to be stored in
 * (historical storage subsystem) GConf; import and export assistants
 * must be carefully updated to export the new data...
 *
 * The #NAIFactoryProvider aims to simplify and organize all the work
 * which must be done around each and every elementary data. It is based
 * on three main things:
 *
 * <orderedlist>
 *   <listitem>
 *     <formalpara>
 *       <title>Elementary datas are banalized.</title>
 *       <para>
 *         whether they are a string, an integer, a boolean, a simple
 *         or double-linked list, each elementary data is encapsuled
 *         into a #NADataBoxed, small sort of structure (incidentally,
 *         which acts almost as the new GLib #GVariant, but too late,
 *         guys :)).
 *       </para>
 *     </formalpara>
 *   </listitem>
 *   <listitem>
 *     <formalpara>
 *       <title>Our objects are de-structured.</title>
 *       <para>
 *         Instead of organizing our elementary datas into structures,
 *         our objects are just flat lists of #NADataBoxed.
 *       </para>
 *     </formalpara>
 *   </listitem>
 *   <listitem>
 *     <formalpara>
 *       <title>A full, centralized, data dictionary is defined.</title>
 *       <para>
 *         Now that our elementary datas are banalized and de-structured,
 *         it is simple enough to describe each of these datas with all
 *         iss properties in one single, centralized, place.
 *       </para>
 *     </formalpara>
 *   </listitem>
 * </orderedlist>
 *
 * Of course, I/O providers are good candidates to be users of this
 * #NAIFactoryProvider interface.
 *
 * Without this interface, each and every I/O provider must,
 * for example when reading an item, have the list of data to be
 * readen for each item, then read each individual data, then
 * organize them in a I/O structure..
 * Each time a new data is added to an object, as e.g. a new condition,
 * then all available I/O providers must be updated: read the data,
 * write the data, then display the data, and so on..
 *
 * With this #NAIFactoryProvider interface, the I/O provider has just to
 * deal with reading/writing elementary types. It does need to know that
 * it will have to read, name, tooltip, description. It just needs to know
 * how to read a string.
 * And while we do not introduce another data type, the I/O provider
 * does not need any maintenance even if we add lot of new data, conditions
 * labels, and so on.
 *
 * So, this is the interface used by data factory management system for
 * providing serialization/unserialization services. This interface may
 * be implemented by I/O providers which would take advantage of this
 * system.
 *
 * <refsect2>
 *  <title>Versions historic</title>
 *  <table>
 *    <title>Historic of the versions of the #NAIFactoryProvider interface</title>
 *    <tgroup rowsep="1" colsep="1" align="center" cols="3">
 *      <colspec colname="na-version" />
 *      <colspec colname="api-version" />
 *      <colspec colname="current" />
 *      <thead>
 *        <row>
 *          <entry>&prodname; version</entry>
 *          <entry>#NAIFactoryProvider interface version</entry>
 *          <entry></entry>
 *        </row>
 *      </thead>
 *      <tbody>
 *        <row>
 *          <entry>since 2.30</entry>
 *          <entry>1</entry>
 *          <entry>current version</entry>
 *        </row>
 *      </tbody>
 *    </tgroup>
 *  </table>
 * </refsect2>
 */

/* private interface data
 */
struct _NAIFactoryProviderInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

gboolean ifactory_provider_initialized = FALSE;
gboolean ifactory_provider_finalized   = FALSE;

static GType register_type( void );
static void  interface_base_init( NAIFactoryProviderInterface *klass );
static void  interface_base_finalize( NAIFactoryProviderInterface *klass );

static guint ifactory_provider_get_version( const NAIFactoryProvider *instance );

static void  v_factory_provider_read_start( const NAIFactoryProvider *reader, void *reader_data, NAIFactoryObject *serializable, GSList **messages );
static void  v_factory_provider_read_done( const NAIFactoryProvider *reader, void *reader_data, NAIFactoryObject *serializable, GSList **messages );
static guint v_factory_provider_write_start( const NAIFactoryProvider *writer, void *writer_data, NAIFactoryObject *serializable, GSList **messages );
static guint v_factory_provider_write_done( const NAIFactoryProvider *writer, void *writer_data, NAIFactoryObject *serializable, GSList **messages );

/**
 * na_ifactory_provider_get_type:
 *
 * Registers the GType of this interface.
 *
 * Returns: the #NAIFactoryProvider #GType.
 */
GType
na_ifactory_provider_get_type( void )
{
	static GType object_type = 0;

	if( !object_type ){
		object_type = register_type();
	}

	return( object_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "na_ifactory_provider_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NAIFactoryProviderInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "NAIFactoryProvider", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( NAIFactoryProviderInterface *klass )
{
	static const gchar *thisfn = "na_ifactory_provider_interface_base_init";

	if( !ifactory_provider_initialized ){

		g_debug( "%s: klass=%p (%s)", thisfn, ( void * ) klass, G_OBJECT_CLASS_NAME( klass ));

		klass->private = g_new0( NAIFactoryProviderInterfacePrivate, 1 );

		klass->get_version = ifactory_provider_get_version;
		klass->read_start = NULL;
		klass->read_data = NULL;
		klass->read_done = NULL;
		klass->write_start = NULL;
		klass->write_data = NULL;
		klass->write_done = NULL;

		ifactory_provider_initialized = TRUE;
	}
}

static void
interface_base_finalize( NAIFactoryProviderInterface *klass )
{
	static const gchar *thisfn = "na_ifactory_provider_interface_base_finalize";

	if( ifactory_provider_initialized && !ifactory_provider_finalized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		ifactory_provider_finalized = TRUE;

		g_free( klass->private );
	}
}

static guint
ifactory_provider_get_version( const NAIFactoryProvider *instance )
{
	return( 1 );
}

/**
 * na_ifactory_provider_read_item:
 * @reader: the instance which implements this #NAIFactoryProvider interface.
 * @reader_data: instance data which will be provided back to the interface
 *  methods
 * @object: the #NAIFactoryObject object to be unserialilzed.
 * @messages: a pointer to a #GSList list of strings; the implementation
 *  may append messages to this list, but shouldn't reinitialize it.
 *
 * This function is to be called by a #NAIIOProvider which would wish read
 * its items. The function takes care of collecting and structuring data,
 * while the callback interface methods #NAIFactoryProviderInterface.read_start(),
 * #NAIFactoryProviderInterface.read_data() and #NAIFactoryProviderInterface.read_done()
 * just have to fill a given #NADataBoxed with the ad-hoc data type.
 *
 * <example>
 *   <programlisting>
 *     &lcomment;
 *      * allocate the object to be readen
 *      &rcomment;
 *     NAObjectItem *item = NA_OBJECT_ITEM( na_object_action_new());
 *     &lcomment;
 *      * some data we may need to have access to in callback methods
 *      &rcomment;
 *     void *data;
 *     &lcomment;
 *      * now call interface function
 *      &rcomment;
 *     na_ifactory_provider_read_item(
 *         NA_IFACTORY_PROVIDER( provider ),
 *         data,
 *         NA_IFACTORY_OBJECT( item ),
 *         messages );
 *   </programlisting>
 * </example>
 *
 * Since: Nautilus-Actions v 2.30, NAIFactoryProvider interface v 1.
 */
void
na_ifactory_provider_read_item( const NAIFactoryProvider *reader, void *reader_data, NAIFactoryObject *object, GSList **messages )
{
	g_return_if_fail( NA_IS_IFACTORY_PROVIDER( reader ));
	g_return_if_fail( NA_IS_IFACTORY_OBJECT( object ));

	if( ifactory_provider_initialized && !ifactory_provider_finalized ){

		v_factory_provider_read_start( reader, reader_data, object, messages );
		na_factory_object_read_item( object, reader, reader_data, messages );
		v_factory_provider_read_done( reader, reader_data, object, messages );
	}
}

/**
 * na_ifactory_provider_write_item:
 * @writer: the instance which implements this #NAIFactoryProvider interface.
 * @writer_data: instance data.
 * @object: the #NAIFactoryObject derived object to be serialized.
 * @messages: a pointer to a #GSList list of strings; the implementation
 *  may append messages to this list, but shouldn't reinitialize it.
 *
 * This function is to be called by a #NAIIOProvider which would wish write
 * an item. The function takes care of collecting and writing elementary data.
 *
 * Returns: a NAIIOProvider operation return code.
 *
 * Since: Nautilus-Actions v 2.30, NAIFactoryProvider interface v 1.
 */
guint
na_ifactory_provider_write_item( const NAIFactoryProvider *writer, void *writer_data, NAIFactoryObject *object, GSList **messages )
{
	static const gchar *thisfn = "na_ifactory_provider_write_item";
	guint code;

	g_return_val_if_fail( NA_IS_IFACTORY_PROVIDER( writer ), NA_IIO_PROVIDER_CODE_PROGRAM_ERROR );
	g_return_val_if_fail( NA_IS_IFACTORY_OBJECT( object ), NA_IIO_PROVIDER_CODE_PROGRAM_ERROR );

	code = NA_IIO_PROVIDER_CODE_NOT_WILLING_TO_RUN;

	if( ifactory_provider_initialized && !ifactory_provider_finalized ){

		g_debug( "%s: writer=%p, writer_data=%p, object=%p (%s)",
				thisfn, ( void * ) writer, ( void * ) writer_data, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

		code = v_factory_provider_write_start( writer, writer_data, object, messages );

		if( code == NA_IIO_PROVIDER_CODE_OK ){
			code = na_factory_object_write_item( object, writer, writer_data, messages );
		}

		if( code == NA_IIO_PROVIDER_CODE_OK ){
			code = v_factory_provider_write_done( writer, writer_data, object, messages );
		}
	}

	return( code );
}

static void
v_factory_provider_read_start( const NAIFactoryProvider *reader, void *reader_data, NAIFactoryObject *serializable, GSList **messages )
{
	if( NA_IFACTORY_PROVIDER_GET_INTERFACE( reader )->read_start ){
		NA_IFACTORY_PROVIDER_GET_INTERFACE( reader )->read_start( reader, reader_data, serializable, messages );
	}
}

static void
v_factory_provider_read_done( const NAIFactoryProvider *reader, void *reader_data, NAIFactoryObject *serializable, GSList **messages )
{
	if( NA_IFACTORY_PROVIDER_GET_INTERFACE( reader )->read_done ){
		NA_IFACTORY_PROVIDER_GET_INTERFACE( reader )->read_done( reader, reader_data, serializable, messages );
	}
}

static guint
v_factory_provider_write_start( const NAIFactoryProvider *writer, void *writer_data, NAIFactoryObject *serializable, GSList **messages )
{
	guint code = NA_IIO_PROVIDER_CODE_OK;

	if( NA_IFACTORY_PROVIDER_GET_INTERFACE( writer )->write_start ){
		code = NA_IFACTORY_PROVIDER_GET_INTERFACE( writer )->write_start( writer, writer_data, serializable, messages );
	}

	return( code );
}

static guint
v_factory_provider_write_done( const NAIFactoryProvider *writer, void *writer_data, NAIFactoryObject *serializable, GSList **messages )
{
	guint code = NA_IIO_PROVIDER_CODE_OK;

	if( NA_IFACTORY_PROVIDER_GET_INTERFACE( writer )->write_done ){
		code = NA_IFACTORY_PROVIDER_GET_INTERFACE( writer )->write_done( writer, writer_data, serializable, messages );
	}

	return( code );
}
