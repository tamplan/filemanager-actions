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

#ifndef __FILEMANAGER_ACTIONS_API_EXTENSION_H__
#define __FILEMANAGER_ACTIONS_API_EXTENSION_H__

/**
 * SECTION: extension
 * @title: Plugins
 * @short_description: The FileManager-Actions Extension Interface Definition v 1
 * @include: filemanager-actions/fma-extension.h
 *
 * &prodname; accepts extensions as dynamically loadable libraries
 * (aka plugins).
 *
 * As of today, &prodname; may be extended in the following areas:
 *  <itemizedlist>
 *    <listitem>
 *      <formalpara>
 *        <title>
 *          Storing menus and actions in a specific storage subsystem
 *        </title>
 *        <para>
 *          This extension is provided via the public
 *          <link linkend="FMAIIOProvider">FMAIIOProvider</link>
 *          interface; it takes care of reading and writing menus
 *          and actions to a specific storage subsystem.
 *        </para>
 *      </formalpara>
 *    </listitem>
 *    <listitem>
 *      <formalpara>
 *        <title>
 *          Exporting menus and actions
 *        </title>
 *        <para>
 *          This extension is provided via the public
 *          <link linkend="FMAIExporter">FMAIExporter</link>
 *          interface; it takes care of exporting menus and actions
 *          to the filesystem from the &prodname; Configuration Tool
 *          user interface.
 *        </para>
 *      </formalpara>
 *    </listitem>
 *    <listitem>
 *      <formalpara>
 *        <title>
 *          Importing menus and actions
 *        </title>
 *        <para>
 *          This extension is provided via the public
 *          <link linkend="FMAIImporter">FMAIImporter</link>
 *          interface; it takes care of importing menus and actions
 *          from the filesystem into the &prodname; Configuration Tool
 *          user interface.
 *        </para>
 *      </formalpara>
 *    </listitem>
 *  </itemizedlist>
 *
 * In order to be recognized as a valid &prodname; plugin, the library
 * must at least export the functions described as mandatory in this
 * extension API.
 *
 * <refsect2>
 *   <title>Developing a &prodname; plugin</title>
 *   <refsect3>
 *     <title>Building the dynamically loadable library</title>
 *       <para>
 * The suggested way of producing a dynamically loadable library is to
 * use
 * <application><ulink url="http://www.gnu.org/software/autoconf/">autoconf</ulink></application>,
 * <application><ulink url="http://www.gnu.org/software/automake/">automake</ulink></application>
 * and
 * <application><ulink url="http://www.gnu.org/software/libtool/">libtool</ulink></application>
 * GNU tools.
 *       </para>
 *       <para>
 * In this case, it should be enough to use the <option>-module</option>
 * option in your <filename>Makefile.am</filename>, as in:
 * <programlisting>
 *   libfma_io_desktop_la_LDFLAGS = -module -no-undefined -avoid-version
 * </programlisting>
 *       </para>
 *    </refsect3>
 *    <refsect3>
 *      <title>Installing the library</title>
 *       <para>
 * At startup time, &prodname; searches for its candidate libraries in
 * <filename>PKGLIBDIR</filename> directory, which most often happens to
 * be <filename>/usr/lib/filemanager-actions/</filename> or
 * <filename>/usr/lib64/filemanager-actions/</filename>,
 * depending of your system.
 *       </para>
 *   </refsect3>
 * </refsect2>
 *
 * <refsect2>
 *  <title>Versions historic</title>
 *  <table>
 *    <title>Historic of the versions of this extension API</title>
 *    <tgroup rowsep="1" colsep="1" align="center" cols="3">
 *      <colspec colname="fma-version" />
 *      <colspec colname="api-version" />
 *      <colspec colname="current" />
 *      <thead>
 *        <row>
 *          <entry>&prodname; version</entry>
 *          <entry>extension API version</entry>
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

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * fma_extension_startup:
 * @module: the #GTypeModule of the plugin library being loaded.
 *
 * This function is called by the FileManager-Actions plugin manager when
 * the plugin library is first loaded in memory. The library may so take
 * advantage of this call by initializing itself, registering its
 * internal #GType types, etc.
 *
 * This function is mandatory: a FileManager-Actions extension must
 * implement this function in order to be considered as a valid candidate
 * to dynamic load.
 *
 * <example>
 *   <programlisting>
 *     static GType st_module_type = 0;
 *
 *     gboolean
 *     fma_extension_startup( GTypeModule *plugin )
 *     {
 *         static GTypeInfo info = {
 *             sizeof( FMADesktopProviderClass ),
 *             NULL,
 *             NULL,
 *             ( GClassInitFunc ) class_init,
 *             NULL,
 *             NULL,
 *             sizeof( FMADesktopProvider ),
 *             0,
 *             ( GInstanceInitFunc ) instance_init
 *         };
 *
 *         static const GInterfaceInfo iio_provider_iface_info = {
 *             ( GInterfaceInitFunc ) iio_provider_iface_init,
 *             NULL,
 *             NULL
 *         };
 *
 *         st_module_type = g_type_module_register_type( plugin, G_TYPE_OBJECT, "FMADesktopProvider", &amp;info, 0 );
 *
 *         g_type_module_add_interface( plugin, st_module_type, FMA_TYPE_IIO_PROVIDER, &amp;iio_provider_iface_info );
 *
 *         return( TRUE );
 *     }
 *   </programlisting>
 * </example>
 *
 * Returns: %TRUE if the initialization is successful, %FALSE else.
 * In this later case, the library is unloaded and no more considered.
 *
 * Since: 2.30
 */
gboolean fma_extension_startup    ( GTypeModule *module );

/**
 * fma_extension_get_version:
 *
 * This function is called by the &prodname; program each time
 * it needs to know which version of this API the plugin
 * implements.
 *
 * If this function is not exported by the library,
 * the plugin manager considers that the library only implements the
 * version 1 of this extension API.
 *
 * Returns: the version of this API supported by the module.
 *
 * Since: 2.30
 */
guint    fma_extension_get_version( void );

/**
 * fma_extension_list_types:
 * @types: the address where to store the zero-terminated array of
 *  instantiable #GType types this library implements.
 *
 * Returned #GType types must already have been registered in the
 * #GType system (e.g. at #fma_extension_startup() time), and the objects
 * they describe may implement one or more of the interfaces defined in
 * this FileManager-Actions public API.
 *
 * The FileManager-Actions plugin manager will instantiate one #GTypeInstance-
 * derived object for each returned #GType type, and associate these objects
 * to this library.
 *
 * This function is mandatory: a FileManager-Actions extension must
 * implement this function in order to be considered as a valid candidate
 * to dynamic load.
 *
 * <example>
 *   <programlisting>
 *     &lcomment; the count of GType types provided by this extension
 *      * each new GType type must
 *      * - be registered in fma_extension_startup()
 *      * - be addressed in fma_extension_list_types().
 *      &rcomment;
 *     #define TYPES_COUNT    1
 *
 *     guint
 *     fma_extension_list_types( const GType **types )
 *     {
 *          static GType types_list [1+TYPES_COUNT];
 *
 *          &lcomment; FMA_TYPE_DESKTOP_PROVIDER has been previously
 *           * registered in fma_extension_startup function
 *           &rcomment;
 *          types_list[0] = FMA_TYPE_DESKTOP_PROVIDER;
 *
 *          types_list[TYPES_COUNT] = 0;
 *          *types = types_list;
 *
 *          return( TYPES_COUNT );
 *     }
 *   </programlisting>
 * </example>
 *
 * Returns: the number of #GType types returned in the @types array, not
 * counting the terminating zero item.
 *
 * Since: 2.30
 */
guint    fma_extension_list_types ( const GType **types );

/**
 * fma_extension_shutdown:
 *
 * This function is called by FileManager-Actions when it is about to
 * shutdown itself.
 *
 * The dynamically loaded library may take advantage of this call to
 * release any resource, handle, and so on, it may have previously
 * allocated.
 *
 * This function is mandatory: a FileManager-Actions extension must
 * implement this function in order to be considered as a valid
 * candidate to dynamic load.
 *
 * Since: 2.30
 */
void     fma_extension_shutdown   ( void );

G_END_DECLS

#endif /* __FILEMANAGER_ACTIONS_API_EXTENSION_H__ */
