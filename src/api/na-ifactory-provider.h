/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011 Pierre Wieser and others (see AUTHORS)
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

#ifndef __NAUTILUS_ACTIONS_API_NA_IFACTORY_PROVIDER_H__
#define __NAUTILUS_ACTIONS_API_NA_IFACTORY_PROVIDER_H__

/**
 * SECTION: ifactory-provider
 * @title: NAIFactoryProvider
 * @short_description: The Data Factory Provider Interface v 1
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

#include "na-data-boxed.h"
#include "na-ifactory-object.h"
#include "na-ifactory-provider-provider.h"

G_BEGIN_DECLS

#define NA_IFACTORY_PROVIDER_TYPE                      ( na_ifactory_provider_get_type())
#define NA_IFACTORY_PROVIDER( instance )               ( G_TYPE_CHECK_INSTANCE_CAST( instance, NA_IFACTORY_PROVIDER_TYPE, NAIFactoryProvider ))
#define NA_IS_IFACTORY_PROVIDER( instance )            ( G_TYPE_CHECK_INSTANCE_TYPE( instance, NA_IFACTORY_PROVIDER_TYPE ))
#define NA_IFACTORY_PROVIDER_GET_INTERFACE( instance ) ( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NA_IFACTORY_PROVIDER_TYPE, NAIFactoryProviderInterface ))

typedef struct _NAIFactoryProviderInterfacePrivate     NAIFactoryProviderInterfacePrivate;

/**
 * NAIFactoryProviderInterface:
 * @get_version: returns the version of this interface the plugin implements.
 * @read_start:  triggered just before reading an item.
 * @read_data:   reads an item.
 * @read_done:   triggered at the end of item reading.
 * @write_start: triggered just before writing an item.
 * @write_data:  writes an item.
 * @write_done:  triggered at the end of item writing.
 *
 * This defines the interface that a #NAIFactoryProvider may implement.
 */
typedef struct {
	/*< private >*/
	GTypeInterface                      parent;
	NAIFactoryProviderInterfacePrivate *private;

	/*< public >*/
	/**
	 * get_version:
	 * @instance: this #NAIFactoryProvider instance.
	 *
	 * Defaults to 1.
	 *
	 * Returns: the version of this interface supported by @instance implementation.
	 *
	 * Since: 2.30
	 */
	guint         ( *get_version )( const NAIFactoryProvider *instance );

	/**
	 * read_start:
	 * @reader: this #NAIFactoryProvider instance.
	 * @reader_data: the data associated to this instance, as provided
	 *  when na_ifactory_provider_read_item() was called.
	 * @object: the #NAIFactoryObject object which comes to be readen.
	 * @messages: a pointer to a #GSList list of strings; the provider
	 *  may append messages to this list, but shouldn't reinitialize it.
	 *
	 * API called by #NAIFactoryObject just before starting with reading data.
	 *
	 * Since: 2.30
	 */
	void          ( *read_start ) ( const NAIFactoryProvider *reader, void *reader_data, const NAIFactoryObject *object, GSList **messages  );

	/**
	 * read_data:
	 * @reader: this #NAIFactoryProvider instance.
	 * @reader_data: the data associated to this instance, as provided
	 *  when na_ifactory_provider_read_item() was called.
	 * @object: the #NAIFactoryobject being unserialized.
	 * @def: a #NADataDef structure which identifies the data to be unserialized.
	 * @messages: a pointer to a #GSList list of strings; the provider
	 *  may append messages to this list, but shouldn't reinitialize it.
	 *
	 * This method must be implemented in order any data be read.
	 *
	 * Returns: a newly allocated NADataBoxed which contains the readen value.
	 * Should return %NULL if data is not found.
	 *
	 * Since: 2.30
	 */
	NADataBoxed * ( *read_data )  ( const NAIFactoryProvider *reader, void *reader_data, const NAIFactoryObject *object, const NADataDef *def, GSList **messages );

	/**
	 * read_done:
	 * @reader: this #NAIFactoryProvider instance.
	 * @reader_data: the data associated to this instance, as provided
	 *  when na_ifactory_provider_read_item() was called.
	 * @object: the #NAIFactoryObject object which comes to be readen.
	 * @messages: a pointer to a #GSList list of strings; the provider
	 *  may append messages to this list, but shouldn't reinitialize it.
	 *
	 * API called by #NAIFactoryObject when all data have been readen.
	 * Implementor may take advantage of this to do some cleanup.
	 *
	 * Since: 2.30
	 */
	void          ( *read_done )  ( const NAIFactoryProvider *reader, void *reader_data, const NAIFactoryObject *object, GSList **messages  );

	/**
	 * write_start:
	 * @writer: this #NAIFactoryProvider instance.
	 * @writer_data: the data associated to this instance.
	 * @object: the #NAIFactoryObject object which comes to be written.
	 * @messages: a pointer to a #GSList list of strings; the provider
	 *  may append messages to this list, but shouldn't reinitialize it.
	 *
	 * API called by #NAIFactoryObject just before starting with writing data.
	 *
	 * Returns: a NAIIOProvider operation return code.
	 *
	 * Since: 2.30
	 */
	guint         ( *write_start )( const NAIFactoryProvider *writer, void *writer_data, const NAIFactoryObject *object, GSList **messages  );

	/**
	 * write_data:
	 * @writer: this #NAIFactoryProvider instance.
	 * @writer_data: the data associated to this instance.
	 * @object: the #NAIFactoryObject object being written.
	 * @def: the description of the data to be written.
	 * @value: the #NADataBoxed to be written down.
	 * @messages: a pointer to a #GSList list of strings; the provider
	 *  may append messages to this list, but shouldn't reinitialize it.
	 *
	 * Write the data embedded in @value down to @instance.
	 *
	 * This method must be implemented in order any data be written.
	 *
	 * Returns: a NAIIOProvider operation return code.
	 *
	 * Since: 2.30
	 */
	guint         ( *write_data ) ( const NAIFactoryProvider *writer, void *writer_data, const NAIFactoryObject *object, const NADataBoxed *boxed, GSList **messages );

	/**
	 * write_done:
	 * @writer: this #NAIFactoryProvider instance.
	 * @writer_data: the data associated to this instance.
	 * @object: the #NAIFactoryObject object which comes to be written.
	 * @messages: a pointer to a #GSList list of strings; the provider
	 *  may append messages to this list, but shouldn't reinitialize it.
	 *
	 * API called by #NAIFactoryObject when all data have been written.
	 * Implementor may take advantage of this to do some cleanup.
	 *
	 * Returns: a NAIIOProvider operation return code.
	 *
	 * Since: 2.30
	 */
	guint         ( *write_done ) ( const NAIFactoryProvider *writer, void *writer_data, const NAIFactoryObject *object, GSList **messages  );
}
	NAIFactoryProviderInterface;

GType na_ifactory_provider_get_type( void );

void  na_ifactory_provider_read_item ( const NAIFactoryProvider *reader, void *reader_data, NAIFactoryObject *object, GSList **messages );
guint na_ifactory_provider_write_item( const NAIFactoryProvider *writer, void *writer_data, NAIFactoryObject *object, GSList **messages );

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_API_NA_IFACTORY_PROVIDER_H__ */
