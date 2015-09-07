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

#ifndef __FILE_MANAGER_ACTIONS_API_IEXPORTER_H__
#define __FILE_MANAGER_ACTIONS_API_IEXPORTER_H__

/**
 * SECTION: iexporter
 * @title: FMAIExporter
 * @short_description: The Export Interface
 * @include: file-manager-actions/fma-iexporter.h
 *
 * The #FMAIExporter interface exports items to the outside world. Each
 * implementation may provide one or more formats.
 *
 * <refsect2>
 *  <title>Export format identifier</title>
 *  <para>
 *   For its own internal needs, &prodname; requires that each export
 *   format have its own identifier, as an ASCII string.
 *  </para>
 *  <para>
 *   In order to avoid any collision, this export format identifier is
 *   allocated by the &prodname; maintainers team. If you wish provide
 *   yourself a new export format, and so need a new export format identifier,
 *   please contact the maintainers (see file-manager-actions.doap at the
 *   root of the source tree).
 *  </para>
 *  <para>
 *   Below is a list of currently allocated export format identifiers.
 *   This list has been last updated on 2010, July 28th.
 *  </para>
 *  <table>
 *   <title>Currently allocated export format identifiers</title>
 *    <tgroup rowsep="1" colsep="1" cols="4">
 *      <colspec colname="id" />
 *      <colspec colname="label" />
 *      <colspec colname="holder" />
 *      <colspec colname="allocated" align="center" />
 *      <thead>
 *        <row>
 *          <entry>Identifier</entry>
 *          <entry>Name</entry>
 *          <entry>Holder</entry>
 *          <entry>Allocated on</entry>
 *        </row>
 *      </thead>
 *      <tbody>
 *        <row>
 *          <entry><literal>Ask</literal></entry>
 *          <entry>Reserved for &prodname; internal needs</entry>
 *          <entry>&prodname;</entry>
 *          <entry>2010-02-15</entry>
 *        </row>
 *        <row>
 *          <entry><literal>Desktop1</literal></entry>
 *          <entry>NA Desktop module</entry>
 *          <entry>&prodname;</entry>
 *          <entry>2010-07-28</entry>
 *        </row>
 *        <row>
 *          <entry><literal>GConfSchemaV1</literal></entry>
 *          <entry>NA XML module</entry>
 *          <entry>&prodname;</entry>
 *          <entry>2010-02-15</entry>
 *        </row>
 *        <row>
 *          <entry><literal>GConfSchemaV2</literal></entry>
 *          <entry>NA XML module</entry>
 *          <entry>&prodname;</entry>
 *          <entry>2010-02-15</entry>
 *        </row>
 *        <row>
 *          <entry><literal>GConfEntry</literal></entry>
 *          <entry>NA XML module</entry>
 *          <entry>&prodname;</entry>
 *          <entry>2010-02-15</entry>
 *        </row>
 *      </tbody>
 *    </tgroup>
 *  </table>
 * </refsect2>
 *
 * <refsect2>
 *  <title>Versions historic</title>
 *  <table>
 *    <title>Historic of the versions of the #FMAIExporter interface</title>
 *    <tgroup rowsep="1" colsep="1" align="center" cols="3">
 *      <colspec colname="na-version" />
 *      <colspec colname="api-version" />
 *      <colspec colname="current" />
 *      <colspec colname="deprecated" />
 *      <thead>
 *        <row>
 *          <entry>&prodname; version</entry>
 *          <entry>#FMAIExporter interface version</entry>
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

#define FMA_TYPE_IEXPORTER                      ( fma_iexporter_get_type())
#define FMA_IEXPORTER( instance )               ( G_TYPE_CHECK_INSTANCE_CAST( instance, FMA_TYPE_IEXPORTER, FMAIExporter ))
#define FMA_IS_IEXPORTER( instance )            ( G_TYPE_CHECK_INSTANCE_TYPE( instance, FMA_TYPE_IEXPORTER ))
#define FMA_IEXPORTER_GET_INTERFACE( instance ) ( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), FMA_TYPE_IEXPORTER, FMAIExporterInterface ))

typedef struct _FMAIExporter                    FMAIExporter;
typedef struct _FMAIExporterInterfacePrivate    FMAIExporterInterfacePrivate;

#ifdef NA_ENABLE_DEPRECATED
/**
 * FMAIExporterFormat:
 * @format:      format identifier (ascii).
 * @label:       short label to be displayed in dialog (UTF-8 localized)
 * @description: full description of the format (UTF-8 localized);
 *               mainly used in the export assistant.
 *
 * This structure describes a supported output format.
 * It must be provided by each #FMAIExporter implementation
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
	FMAIExporterFormat;

/**
 * FMAIExporterFileParms:
 * @version:  [in] version of this structure;
 *                 since structure version 1.
 * @exported: [in] exported NAObjectItem-derived object;
 *                 since structure version 1.
 * @folder:   [in] URI of the target folder;
 *                 since structure version 1.
 * @format:   [in] export format as a GQuark;
 *                 since structure version 1.
 * @basename: [out] basename of the exported file;
 *                 since structure version 1.
 * @messages: [in/out] a #GSList list of localized strings;
 *                 the provider may append messages to this list,
 *                 but shouldn't reinitialize it;
 *                 since structure version 1.
 *
 * The structure that the implementation receives as a parameter of
 * #FMAIExporterInterface.to_file () interface method.
 *
 * Deprecated: 3.2
 */
typedef struct {
	guint         version;
	NAObjectItem *exported;
	gchar        *folder;
	GQuark        format;
	gchar        *basename;
	GSList       *messages;
}
	FMAIExporterFileParms;

/**
 * FMAIExporterBufferParms:
 * @version:  [in] version of this structure;
 *                 since structure version 1.
 * @exported: [in] exported NAObjectItem-derived object;
 *                 since structure version 1.
 * @format:   [in] export format as a GQuark;
 *                 since structure version 1.
 * @buffer:   [out] buffer which contains the exported object;
 *                 since structure version 1.
 * @messages: [in/out] a #GSList list of localized strings;
 *                 the provider may append messages to this list,
 *                 but shouldn't reinitialize it;
 *                 since structure version 1.
 *
 * The structure that the plugin receives as a parameter of
 * #FMAIExporterInterface.to_buffer () interface method.
 *
 * Deprecated: 3.2
 */
typedef struct {
	guint          version;
	NAObjectItem *exported;
	GQuark         format;
	gchar         *buffer;
	GSList        *messages;
}
	FMAIExporterBufferParms;

#endif /* NA_ENABLE_DEPRECATED */

/**
 * FMAIExporterFormatv2:
 * @version:     the version of this #FMAIExporterFormatv2 structure;
 *               equals to 2;
 *               since structure version 1.
 * @provider:    the #FMAIExporter provider for this format;
 *               since structure version 2.
 * @format:      format identifier (ascii, allocated by the FileManager-Actions team);
 *               since structure version 2.
 * @label:       short label to be displayed in dialog (UTF-8 localized);
 *               since structure version 2.
 * @description: full description of the format (UTF-8 localized);
 *               mainly used as a tooltip;
 *               since structure version 2.
 * @pixbuf:      an image to be associated with this export format;
 *               this pixbuf is supposed to be rendered with GTK_ICON_SIZE_DIALOG size;
 *               since structure version 2.
 *
 * This structure describes a supported output format.
 * It must be provided by each #FMAIExporter implementation
 * (see e.g. <filename>src/io-xml/naxml-formats.c</filename>).
 *
 * When listing available export formats, the @provider must return a #GList
 * of these structures.
 *
 * <refsect2>
 *  <title>Versions historic</title>
 *  <table>
 *    <title>Historic of the versions of the #FMAIExporterFormatv2 structure</title>
 *    <tgroup rowsep="1" colsep="1" align="center" cols="3">
 *      <colspec colname="na-version" />
 *      <colspec colname="api-version" />
 *      <colspec colname="current" />
 *      <thead>
 *        <row>
 *          <entry>&prodname; version</entry>
 *          <entry>#FMAIExporterFormatv2 structure version</entry>
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
	guint         version;
	FMAIExporter *provider;
	gchar        *format;
	gchar        *label;
	gchar        *description;
	GdkPixbuf    *pixbuf;
}
	FMAIExporterFormatv2;

/**
 * FMAIExporterFileParmsv2:
 * @version:  [in] version of this structure;
 *                 equals to 2;
 *                 since structure version 1.
 * @content:  [in] version of the content of this structure;
 *                 equals to 1;
 *                 since structure version 2.
 * @exported: [in] exported NAObjectItem-derived object;
 *                 since structure version 1.
 * @folder:   [in] URI of the target folder;
 *                 since structure version 1.
 * @format:   [in] export format string identifier;
 *                 since structure version 1.
 * @basename: [out] basename of the exported file;
 *                 since structure version 1.
 * @messages: [in/out] a #GSList list of localized strings;
 *                 the provider may append messages to this list,
 *                 but shouldn't reinitialize it;
 *                 since structure version 1.
 *
 * The structure that the plugin receives as a parameter of
 * #FMAIExporterInterface.to_file () interface method.
 *
 * Since: 3.2
 */
typedef struct {
	guint          version;
	guint          content;
	NAObjectItem *exported;
	gchar         *folder;
	gchar         *format;
	gchar         *basename;
	GSList        *messages;
}
	FMAIExporterFileParmsv2;

/**
 * FMAIExporterBufferParmsv2:
 * @version:  [in] version of this structure;
 *                 equals to 2;
 *                 since structure version 1.
 * @content:  [in] version of the content of this structure;
 *                 equals to 1;
 *                 since structure version 2.
 * @exported: [in] exported NAObjectItem-derived object;
 *                 since structure version 1.
 * @format:   [in] export format string identifier;
 *                 since structure version 2.
 * @buffer:   [out] buffer which contains the exported object;
 *                 since structure version 1.
 * @messages: [in/out] a #GSList list of localized strings;
 *                 the provider may append messages to this list,
 *                 but shouldn't reinitialize it;
 *                 since structure version 1.
 *
 * The structure that the plugin receives as a parameter of
 * #FMAIExporterInterface.to_buffer () interface method.
 *
 * Since: 3.2
 */
typedef struct {
	guint          version;
	guint          content;
	NAObjectItem *exported;
	gchar         *format;
	gchar         *buffer;
	GSList        *messages;
}
	FMAIExporterBufferParmsv2;

/**
 * FMAIExporterInterface:
 * @get_version:  [should] returns the version of this interface the plugin implements.
 * @get_name:     [should] returns the public plugin name.
 * @get_formats:  [should] returns the list of supported formats.
 * @free_formats: [should] free a list of formats
 * @to_file:      [should] exports an item to a file.
 * @to_buffer:    [should] exports an item to a buffer.
 *
 * This defines the interface that a #FMAIExporter should implement.
 */
typedef struct {
	/*< private >*/
	GTypeInterface                parent;
	FMAIExporterInterfacePrivate *private;

	/*< public >*/
	/**
	 * get_version:
	 * @instance: this FMAIExporter instance.
	 *
	 * FileManager-Actions calls this method each time it needs to know
	 * which version of this interface the plugin implements.
	 *
	 * If this method is not implemented by the plugin,
	 * FileManager-Actions considers that the plugin only implements
	 * the version 1 of the NAIImporter interface.
	 *
	 * Return value: if implemented, this method must return the version
	 * number of this interface the I/O provider is supporting.
	 *
	 * Defaults to 1.
	 *
	 * Since: 2.30
	 */
	guint   ( *get_version )( const FMAIExporter *instance );

	/**
	 * get_name:
	 * @instance: this FMAIExporter instance.
	 *
	 * Return value: if implemented, the method should return the name to be
	 * displayed, as a newly allocated string which will be g_free() by the
	 * caller.
	 *
	 * This may be the name of the module itself, but this also may be a
	 * special name the modules gives to this interface.
	 *
	 * Defaults to a NULL string.
	 *
	 * Since: 2.30
	 */
	gchar * ( *get_name )   ( const FMAIExporter *instance );

	/**
	 * get_formats:
	 * @instance: this FMAIExporter instance.
	 *
	 * For its own internal needs, FileManager-Actions requires each export
	 * format has its own unique identifier (in fact, just a small ASCII
	 * string).
	 *
	 * To avoid any collision, the format identifier is allocated by the
	 * FileManager-Actions maintainers team. If you wish develop a new export
	 * format, and so need a new format identifier, please contact the
	 * maintainers (see file-manager-actions.doap).
	 *
	 * Return value:
	 * - Interface v1:
	 *   a null-terminated list of FMAIExporterFormat structures
	 *   which describes the formats supported by this FMAIExporter
	 *   provider.
	 *   The returned list is owned by the FMAIExporter provider,
	 *   and should not be freed nor released by the caller.
	 *
	 * - Interface v2:
	 *   a GList of FMAIExporterFormatv2 structures
	 *   which describes the formats supported by this FMAIExporter
	 *   provider.
	 *   The caller should then invoke the free_formats() method
	 *   in order the provider be able to release the resources
	 *   allocated to the list.
	 *
	 * Defaults to NULL (no format at all).
	 *
	 * Since: 2.30
	 */
	void *  ( *get_formats )( const FMAIExporter *instance );

	/**
	 * free_formats:
	 * @instance: this FMAIExporter instance.
	 * @formats: a null-terminated list of FMAIExporterFormatv2 structures,
	 *  as returned by get_formats() method above.
	 *
	 * Free the resources allocated to the @formats list.
	 *
	 * Since: 3.2
	 */
	void    ( *free_formats )( const FMAIExporter *instance, GList *formats );

	/**
	 * to_file:
	 * @instance: this FMAIExporter instance.
	 * @parms: a FMAIExporterFileParmsv2 structure.
	 *
	 * Exports the specified 'exported' to the target 'folder' in the required
	 * 'format'.
	 *
	 * Return value: the FMAIExporterExportStatus status of the operation.
	 *
	 * Since: 2.30
	 */
	guint   ( *to_file )    ( const FMAIExporter *instance, FMAIExporterFileParmsv2 *parms );

	/**
	 * to_buffer:
	 * @instance: this FMAIExporter instance.
	 * @parms: a FMAIExporterFileParmsv2 structure.
	 *
	 * Exports the specified 'exported' to a newly allocated 'buffer' in
	 * the required 'format'. The allocated 'buffer' will be g_free()
	 * by the caller.
	 *
	 * Return value: the FMAIExporterExportStatus status of the operation.
	 *
	 * Since: 2.30
	 */
	guint   ( *to_buffer )  ( const FMAIExporter *instance, FMAIExporterBufferParmsv2 *parms );
}
	FMAIExporterInterface;

/**
 * FMAIExporterExportStatus:
 * @FMA_IEXPORTER_CODE_OK:              export OK.
 * @FMA_IEXPORTER_CODE_INVALID_ITEM:    exported item was found invalid.
 * @FMA_IEXPORTER_CODE_INVALID_TARGET:  selected target was found invalid.
 * @FMA_IEXPORTER_CODE_INVALID_FORMAT:  asked format was found invalid.
 * @FMA_IEXPORTER_CODE_UNABLE_TO_WRITE: unable to write the item.
 * @FMA_IEXPORTER_CODE_ERROR:           other undetermined error.
 *
 * The reasons for which an item may not have been exported
 */
typedef enum {
	FMA_IEXPORTER_CODE_OK = 0,
	FMA_IEXPORTER_CODE_INVALID_ITEM,
	FMA_IEXPORTER_CODE_INVALID_TARGET,
	FMA_IEXPORTER_CODE_INVALID_FORMAT,
	FMA_IEXPORTER_CODE_UNABLE_TO_WRITE,
	FMA_IEXPORTER_CODE_ERROR,
}
	FMAIExporterExportStatus;

GType fma_iexporter_get_type( void );

G_END_DECLS

#endif /* __FILE_MANAGER_ACTIONS_API_IEXPORTER_H__ */
