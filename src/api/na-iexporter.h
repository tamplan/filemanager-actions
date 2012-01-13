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

#ifndef __NAUTILUS_ACTIONS_API_NA_IEXPORTER_H__
#define __NAUTILUS_ACTIONS_API_NA_IEXPORTER_H__

/**
 * SECTION: iexporter
 * @title: NAIExporter
 * @short_description: The Export Interface v 2
 * @include: nautilus-actions/na-iexporter.h
 *
 * The #NAIExporter interface exports items to the outside world.
 *
 * <refsect2>
 *  <title>Versions historic</title>
 *  <table>
 *    <title>Historic of the versions of the #NAIExporter interface</title>
 *    <tgroup rowsep="1" colsep="1" align="center" cols="3">
 *      <colspec colname="na-version" />
 *      <colspec colname="api-version" />
 *      <colspec colname="current" />
 *      <colspec colname="deprecated" />
 *      <thead>
 *        <row>
 *          <entry>&prodname; version</entry>
 *          <entry>#NAIExporter interface version</entry>
 *          <entry></entry>
 *          <entry></entry>
 *        </row>
 *      </thead>
 *      <tbody>
 *        <row>
 *          <entry>from 2.30 to 3.1.5</entry>
 *          <entry>1</entry>
 *          <entry></entry>
 *          <entry>deprecated</entry>
 *        </row>
 *        <row>
 *          <entry>since 3.2</entry>
 *          <entry>2</entry>
 *          <entry>current version</entry>
 *          <entry></entry>
 *        </row>
 *      </tbody>
 *    </tgroup>
 *  </table>
 * </refsect2>
 */

#include <gdk-pixbuf/gdk-pixbuf.h>
#include "na-object-item.h"

G_BEGIN_DECLS

#define NA_TYPE_IEXPORTER                      ( na_iexporter_get_type())
#define NA_IEXPORTER( instance )               ( G_TYPE_CHECK_INSTANCE_CAST( instance, NA_TYPE_IEXPORTER, NAIExporter ))
#define NA_IS_IEXPORTER( instance )            ( G_TYPE_CHECK_INSTANCE_TYPE( instance, NA_TYPE_IEXPORTER ))
#define NA_IEXPORTER_GET_INTERFACE( instance ) ( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NA_TYPE_IEXPORTER, NAIExporterInterface ))

typedef struct _NAIExporter                    NAIExporter;
typedef struct _NAIExporterInterfacePrivate    NAIExporterInterfacePrivate;
typedef struct _NAIExporterFileParms           NAIExporterFileParms;
typedef struct _NAIExporterBufferParms         NAIExporterBufferParms;

#ifdef NA_ENABLE_DEPRECATED
/**
 * NAIExporterFormat:
 * @format:      format identifier (ascii).
 * @label:       short label to be displayed in dialog (UTF-8 localized)
 * @description: full description of the format (UTF-8 localized);
 *               mainly used in the export assistant.
 *
 * This structure describes a supported output format.
 * It must be provided by each #NAIExporter implementation
 * (see e.g. <filename>src/io-xml/naxml-formats.c</filename>).
 *
 * When listing available export formats, the instance returns a #GList
 * of these structures.
 *
 * Deprecated: 3.2
 */
typedef struct {
	gchar     *format;
	gchar     *label;
	gchar     *description;
}
	NAIExporterFormat;
#endif

/**
 * NAIExporterFormatExt:
 * @version:     the version of this #NAIExporterFormatExt structure (2).
 * @provider:    the #NAIExporter provider for this format.
 * @format:      format identifier (ascii, allocated by the Nautilus-Actions team).
 * @label:       short label to be displayed in dialog (UTF-8 localized)
 * @description: full description of the format (UTF-8 localized);
 *               mainly used as a tooltip.
 * @pixbuf:      an image to be associated with this export format;
 *               this pixbuf is supposed to be rendered with GTK_ICON_SIZE_DIALOG size.
 *
 * This structure describes a supported output format.
 * It must be provided by each #NAIExporter implementation
 * (see e.g. <filename>src/io-xml/naxml-formats.c</filename>).
 *
 * When listing available export formats, the instance returns a #GList
 * of these structures.
 *
 * <refsect2>
 *  <title>Versions historic</title>
 *  <table>
 *    <title>Historic of the versions of the #NAIExporteeFormat structure</title>
 *    <tgroup rowsep="1" colsep="1" align="center" cols="3">
 *      <colspec colname="na-version" />
 *      <colspec colname="api-version" />
 *      <colspec colname="current" />
 *      <thead>
 *        <row>
 *          <entry>&prodname; version</entry>
 *          <entry>#NAIExporterFormat structure version</entry>
 *          <entry></entry>
 *        </row>
 *      </thead>
 *      <tbody>
 *        <row>
 *          <entry>since 2.30</entry>
 *          <entry>1</entry>
 *          <entry></entry>
 *        </row>
 *        <row>
 *          <entry>since 3.2</entry>
 *          <entry>2</entry>
 *          <entry>current version</entry>
 *        </row>
 *      </tbody>
 *    </tgroup>
 *  </table>
 * </refsect2>
 *
 * Since: 3.2
 */
typedef struct {
	guint        version;
	NAIExporter *provider;
	gchar       *format;
	gchar       *label;
	gchar       *description;
	GdkPixbuf   *pixbuf;
}
	NAIExporterFormatExt;

/**
 * NAIExporterInterface:
 * @get_version:  returns the version of this interface the plugin implements.
 * @get_name:     returns the public plugin name.
 * @get_formats:  returns the list of supported formats.
 * @free_formats: free a list of formats
 * @to_file:      exports an item to a file.
 * @to_buffer:    exports an item to a buffer.
 *
 * This defines the interface that a #NAIExporter should implement.
 */
typedef struct {
	/*< private >*/
	GTypeInterface               parent;
	NAIExporterInterfacePrivate *private;

	/*< public >*/
	/**
	 * get_version:
	 * @instance: this #NAIExporter instance.
	 *
	 * Returns: the version of this interface supported by the I/O provider.
	 *
	 * Defaults to 1.
	 *
	 * Since: 2.30
	 */
	guint   ( *get_version )( const NAIExporter *instance );

	/**
	 * get_name:
	 * @instance: this #NAIExporter instance.
	 *
	 * Returns: the name to be displayed for this instance, as a
	 * newly allocated string which should be g_free() by the caller.
	 *
	 * Since: 2.30
	 */
	gchar * ( *get_name )   ( const NAIExporter *instance );

	/**
	 * get_formats:
	 * @instance: this #NAIExporter instance.
	 *
	 * To avoid any collision, the format id is allocated by the
	 * Nautilus-Actions maintainer team. If you wish develop a new
	 * export format, and so need a new format id, please contact the
	 * maintainers (see #nautilus-actions.doap).
	 *
	 * Returns:
	 * <itemizedlist>
	 *   <listitem>
	 *     <formalpara>
	 *       <title>
	 *         Interface v1:
	 *       </title>
	 *       <para>
	 *         a null-terminated list of #NAIExporterFormat structures
	 *         which describes the formats supported by this #NAIExporter
	 *         provider.
	 *       </para>
	 *       <para>
	 *         The returned list is owned by the #NAIExporter provider,
	 *         and should not be freed nor released by the caller.
	 *       </para>
	 *   </listitem>
	 *   <listitem>
	 *     <formalpara>
	 *       <title>
	 *         Interface v2:
	 *       </title>
	 *       <para>
	 *         a #GList of #NAIExporterFormatExt structures
	 *         which describes the formats supported by this #NAIExporter
	 *         provider.
	 *       </para>
	 *       <para>
	 *         The caller should then invoke the free_formats() method
	 *         in order the provider be able to release the resources
	 *         allocated to the list.
	 *       </para>
	 *   </listitem>
	 * </itemizedlist>
	 *
	 * Defaults to %NULL (no format at all).
	 *
	 * Since: 2.30
	 */
	void *  ( *get_formats )( const NAIExporter *instance );

	/**
	 * free_formats:
	 * @instance: this #NAIExporter instance.
	 * @formats: a null-terminated list of #NAIExporterFormatExt structures,
	 *  as returned by get_formats() method.
	 *
	 * Free the resources allocated to the @formats list.
	 *
	 * Since: 3.2
	 */
	void    ( *free_formats )( const NAIExporter *instance, GList *formats );

	/**
	 * to_file:
	 * @instance: this #NAIExporter instance.
	 * @parms: a #NAIExporterFileParms structure.
	 *
	 * Exports the specified 'exported' to the target 'folder' in the required
	 * 'format'.
	 *
	 * Returns: the #NAIExporterExportStatus status of the operation.
	 *
	 * Since: 2.30
	 */
	guint   ( *to_file )    ( const NAIExporter *instance, NAIExporterFileParms *parms );

	/**
	 * to_buffer:
	 * @instance: this #NAIExporter instance.
	 * @parms: a #NAIExporterFileParms structure.
	 *
	 * Exports the specified 'exported' to a newly allocated 'buffer' in
	 * the required 'format'. The allocated 'buffer' should be g_free()
	 * by the caller.
	 *
	 * Returns: the #NAIExporterExportStatus status of the operation.
	 *
	 * Since: 2.30
	 */
	guint   ( *to_buffer )  ( const NAIExporter *instance, NAIExporterBufferParms *parms );
}
	NAIExporterInterface;

/**
 * NAIExporterExportStatus:
 * @NA_IEXPORTER_CODE_OK:              export OK.
 * @NA_IEXPORTER_CODE_INVALID_ITEM:    exported item was found invalid.
 * @NA_IEXPORTER_CODE_INVALID_TARGET:  selected target was found invalid.
 * @NA_IEXPORTER_CODE_INVALID_FORMAT:  asked format was found invalid.
 * @NA_IEXPORTER_CODE_UNABLE_TO_WRITE: unable to write the item.
 * @NA_IEXPORTER_CODE_ERROR:           other undetermined error.
 *
 * The reasons for which an item may not have been exported
 */
typedef enum {
	NA_IEXPORTER_CODE_OK = 0,
	NA_IEXPORTER_CODE_INVALID_ITEM,
	NA_IEXPORTER_CODE_INVALID_TARGET,
	NA_IEXPORTER_CODE_INVALID_FORMAT,
	NA_IEXPORTER_CODE_UNABLE_TO_WRITE,
	NA_IEXPORTER_CODE_ERROR,
}
	NAIExporterExportStatus;

#ifdef NA_ENABLE_DEPRECATED
/**
 * NAIExporterFileParms:
 * @version:  version of this structure (input, since v 1)
 * @exported: exported NAObjectItem-derived object (input, since v 1)
 * @folder:   URI of the target folder (input, since v 1)
 * @format:   export format as a GQuark (input, since v 1)
 * @basename: basename of the exported file (output, since v 1)
 * @messages: a #GSList list of localized strings;
 *            the provider may append messages to this list,
 *            but shouldn't reinitialize it
 *            (input/output, since v 1).
 *
 * The structure that the plugin receives as a parameter of
 * #NAIExporterInterface.to_file () interface method.
 *
 * Deprecated: 3.2
 */
struct _NAIExporterFileParmsv1 {
	guint         version;
	NAObjectItem *exported;
	gchar        *folder;
	GQuark        format;
	gchar        *basename;
	GSList       *messages;
};

typedef struct _NAIExporterFileParmsv1         NAIExporterFileParmsv1;

/**
 * NAIExporterBufferParms:
 * @version:  version of this structure (input, since v 1)
 * @exported: exported NAObjectItem-derived object (input, since v 1)
 * @format:   export format as a GQuark (input, since v 1)
 * @buffer:   buffer which contains the exported object (output, since v 1)
 * @messages: a #GSList list of localized strings;
 *            the provider may append messages to this list,
 *            but shouldn't reinitialize it
 *            (input/output, since v 1).
 *
 * The structure that the plugin receives as a parameter of
 * #NAIExporterInterface.to_buffer () interface method.
 *
 * Deprecated: 3.2
 */
struct _NAIExporterBufferParmsv1 {
	guint         version;
	NAObjectItem *exported;
	GQuark        format;
	gchar        *buffer;
	GSList       *messages;
};

typedef struct _NAIExporterBufferParmsv1       NAIExporterBufferParmsv1;
#endif

/**
 * NAIExporterFileParms:
 * @version:  version of this structure (input, since v 1, currently equal to 2)
 * @exported: exported NAObjectItem-derived object (input, since v 1)
 * @folder:   URI of the target folder (input, since v 1)
 * @format:   export format string identifier (input, since v 2)
 * @basename: basename of the exported file (output, since v 1)
 * @messages: a #GSList list of localized strings;
 *            the provider may append messages to this list,
 *            but shouldn't reinitialize it
 *            (input/output, since v 1).
 *
 * The structure that the plugin receives as a parameter of
 * #NAIExporterInterface.to_file () interface method.
 *
 * Since: 3.2
 */
struct _NAIExporterFileParms {
	guint         version;
	NAObjectItem *exported;
	gchar        *folder;
	gchar        *format;
	gchar        *basename;
	GSList       *messages;
};

/**
 * NAIExporterBufferParms:
 * @version:  version of this structure (input, since v 1, currently equal to 2)
 * @exported: exported NAObjectItem-derived object (input, since v 1)
 * @format:   export format string identifier (input, since v 2)
 * @buffer:   buffer which contains the exported object (output, since v 1)
 * @messages: a #GSList list of localized strings;
 *            the provider may append messages to this list,
 *            but shouldn't reinitialize it
 *            (input/output, since v 1).
 *
 * The structure that the plugin receives as a parameter of
 * #NAIExporterInterface.to_buffer () interface method.
 *
 * Since: 3.2
 */
struct _NAIExporterBufferParms {
	guint         version;
	NAObjectItem *exported;
	gchar        *format;
	gchar        *buffer;
	GSList       *messages;
};

GType na_iexporter_get_type( void );

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_API_NA_IEXPORTER_H__ */
