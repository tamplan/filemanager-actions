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

#ifndef __FILEMANAGER_ACTIONS_API_IIMPORTER_H__
#define __FILEMANAGER_ACTIONS_API_IIMPORTER_H__

/**
 * SECTION: iimporter
 * @title: FMAIImporter
 * @short_description: The Import Interface
 * @include: filemanager-actions/fma-iimporter.h
 *
 * The #FMAIImporter interface imports items from the outside world
 * into &prodname; repository (see #FMAIIOProvider interface for how
 * these items will be later managed to be store somewhere).
 *
 * In version 1 of the #FMAIImporter interface, &prodname; used to
 * provide the implementation with all was needed to be able to manage
 * the import process up to be ready for insertion, including the
 * deduplication if required.
 *
 * This used to put on the implementation the responsability to check
 * for the unicity of the imported identifier, maybe asking the caller
 * (via provided callback functions) what to do with it, maybe
 * reallocating a new identifier, and so on...
 *
 * Starting with &prodname; version 3.2, this interface is bumped to
 * a version 2 which greatly simplifies it.
 *
 * The I/O provider which implements the #FMAIImporter interface is no
 * more required to check for existence of the imported items, but this
 * check is pushed back to the caller responsability.
 *
 * Rationale is that only the caller is able to check against a valid
 * repository in its current import context, while the #FMAIImporter
 * provider should only be responsible to import an item in memory.
 *
 * <refsect2>
 *  <title>Versions historic</title>
 *  <table>
 *    <title>Historic of the versions of the #FMAIImporter interface</title>
 *    <tgroup rowsep="1" colsep="1" align="center" cols="3">
 *      <colspec colname="fma-version" />
 *      <colspec colname="api-version" />
 *      <colspec colname="current" />
 *      <colspec colname="deprecated" />
 *      <thead>
 *        <row>
 *          <entry>&prodname; version</entry>
 *          <entry>#FMAIImporter interface version</entry>
 *          <entry></entry>
 *          <entry></entry>
 *        </row>
 *      </thead>
 *      <tbody>
 *        <row>
 *          <entry>from 2.30 up to 3.1.5</entry>
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

#include "fma-object-item.h"

G_BEGIN_DECLS

#define FMA_TYPE_IIMPORTER                      ( fma_iimporter_get_type())
#define FMA_IIMPORTER( instance )               ( G_TYPE_CHECK_INSTANCE_CAST( instance, FMA_TYPE_IIMPORTER, FMAIImporter ))
#define FMA_IS_IIMPORTER( instance )            ( G_TYPE_CHECK_INSTANCE_TYPE( instance, FMA_TYPE_IIMPORTER ))
#define FMA_IIMPORTER_GET_INTERFACE( instance ) ( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), FMA_TYPE_IIMPORTER, FMAIImporterInterface ))

typedef struct _FMAIImporter                    FMAIImporter;
typedef struct _FMAIImporterInterfacePrivate    FMAIImporterInterfacePrivate;

/**
 * FMAIImporterInterface:
 * @get_version:     [should] returns the version of this interface that the
 *                            plugin implements.
 * @import_from_uri: [should] imports an item.
 *
 * This defines the interface that a #FMAIImporter should implement.
 */
typedef struct {
	/*< private >*/
	GTypeInterface                parent;
	FMAIImporterInterfacePrivate *private;

	/*< public >*/
	/**
	 * get_version:
	 * @instance: the FMAIImporter provider.
	 *
	 * FileManager-Actions calls this method each time it needs to know
	 * which version of this interface the plugin implements.
	 *
	 * If this method is not implemented by the plugin,
	 * FileManager-Actions considers that the plugin only implements
	 * the version 1 of the FMAIImporter interface.
	 *
	 * Return value: if implemented, this method must return the version
	 * number of this interface the I/O provider is supporting.
	 *
	 * Defaults to 1.
	 *
	 * Since: 2.30
	 */
	guint ( *get_version )    ( const FMAIImporter *instance );

	/**
	 * import_from_uri:
	 * @instance: the FMAIImporter provider.
	 * @parms: a FMAIImporterImportFromUriParms structure.
	 *
	 * Imports an item.
	 *
	 * If the provider implements the version 1 of this interface, then
	 * @parms is supposed to map to FMAIImporterImportFromUriParms structure.
	 *
	 * Contrarily, if the provider implements the version 2 of the interface,
	 * then @parms is expected to map to a FMAIImporterImportFromUriParmsv2
	 * structure.
	 *
	 * Return value: the return code of the operation.
	 *
	 * Since: 2.30
	 */
	guint ( *import_from_uri )( const FMAIImporter *instance, void *parms );
}
	FMAIImporterInterface;

#ifdef FMA_ENABLE_DEPRECATED
/**
 * FMAIImporterCheckFn:
 * @imported: the currently imported #FMAObjectItem -derived object.
 * @fn_data: some data to be passed to the function.
 *
 * In version 1 of the interface, this function may be provided by
 * the caller in order the #FMAIImporter provider be able to check for
 * pre-existence of the imported item.
 * This function should return the already existing item which has the
 * same id than the currently being imported one, or %NULL if the
 * imported id will be unique.
 *
 * If this function is not provided, then the #FMAIImporter provider will not
 * be able to check for duplicates. In this case, the id of the imported item
 * should be systematically regenerated as a unique id, regardless of the
 * asked import mode.
 *
 * Standard FMA callers provide a function which checks for the existance
 * of the newly imported item :
 * <itemizedlist>
 *   <listitem>
 *     <para>
 *       first among the current list of just imported items
 *     </para>
 *   </listitem>
 *   <listitem>
 *     <para>
 *       and then amon the items currently visible in the main window.
 *     </para>
 *   </listitem>
 * </itemizedlist>
 * Items which may have been loaded by FMAPivot at the start of the
 * application, and deleted meanwhile, are just ignored.
 *
 * Returns: the already existing #FMAObjectItem with same id, or %NULL.
 *
 * Since: 2.30
 * Deprecated: 3.2
 */
typedef FMAObjectItem * ( *FMAIImporterCheckFn )( const FMAObjectItem *, void * );

/**
 * FMAIImporterAskUserFn:
 * @imported: the currently imported #FMAObjectItem.
 * @existing: an already existing #FMAObjectItem with same id.
 * @fn_data: some data to be passed to the function.
 *
 * In version 1 of the interface, this function may be provided by the
 * caller as a convenience way for the #FMAIImporter to ask the user to
 * know what to do in the case of a duplicate id.
 *
 * If this function is not provided, and the #FMAIImporter does not have
 * any other way to ask the user, then a 'no import' policy should be
 * preferred when managing duplicate identifiers.
 *
 * Returns: the import mode chosen by the user, which must not be
 * %IMPORTER_MODE_ASK.
 *
 * Since: 2.30
 * Deprecated: 3.2
 */
typedef guint ( *FMAIImporterAskUserFn )( const FMAObjectItem *, const FMAObjectItem *, void * );

/**
 * FMAIImporterManageImportModeParms:
 * @version:       [in] the version of the structure content, equals to 1;
 *                      since structure version 1.
 * @imported:      [in] the imported #FMAObjectItem -derived object;
 *                      since structure version 1.
 * @asked_mode:    [in] asked import mode;
 *                      since structure version 1.
 * @check_fn:      [in] a #FMAIImporterCheckFn function to check the existence of the imported id;
 *                      since structure version 1.
 * @check_fn_data: [in] @check_fn data;
 *                      since structure version 1.
 * @ask_fn:        [in] a #FMAIImporterAskUserFn function to ask the user what to do in case of a duplicate id;
 *                      since structure version 1.
 * @ask_fn_data:   [in] @ask_fn data;
 *                      since structure version 1.
 * @exist:         [out] whether the imported Id already existed;
 *                      since structure version 1.
 * @import_mode:   [out] actually used import mode;
 *                      since structure version 1.
 * @messages:      [in/out] a #GSList list of localized strings;
 *                      the provider may append messages to this list, but shouldn't reinitialize it;
 *                      since structure version 1.
 *
 * This structure allows all used parameters when managing the import mode
 * to be passed and received through a single structure.
 *
 * Since: 2.30
 * Deprecated: 3.2
 */
typedef struct {
	guint                 version;
	FMAObjectItem        *imported;
	guint                 asked_mode;
	FMAIImporterCheckFn   check_fn;
	void                 *check_fn_data;
	FMAIImporterAskUserFn ask_fn;
	void                 *ask_fn_data;
	gboolean              exist;
	guint                 import_mode;
	GSList               *messages;
}
	FMAIImporterManageImportModeParms;

/**
 * FMAIImporterImportMode:
 * @IMPORTER_MODE_NO_IMPORT: a "do not import" mode.
 * @IMPORTER_MODE_RENUMBER:  reallocate a new id when the imported one already exists.
 * @IMPORTER_MODE_OVERRIDE:  override the existing id with the imported one.
 * @IMPORTER_MODE_ASK:       ask the user for what to do with this particular item.
 *
 * Define the mode of an import operation.
 *
 * Deprecated: 3.2
 */
typedef enum {
	IMPORTER_MODE_NO_IMPORT = 1,
	IMPORTER_MODE_RENUMBER,
	IMPORTER_MODE_OVERRIDE,
	IMPORTER_MODE_ASK
}
	FMAIImporterImportMode;

guint fma_iimporter_manage_import_mode( FMAIImporterManageImportModeParms *parms );

/**
 * FMAIImporterImportFromUriParms:
 * @version:       [in] the version of this structure;
 *                      since structure version 1.
 * @uri:           [in] uri of the file to be imported;
 *                      since structure version 1.
 * @asked_mode:    [in] asked import mode;
 *                      since structure version 1.
 * @exist:         [out] whether the imported Id already existed;
 *                      since structure version 1.
 * @import_mode:   [out] actually used import mode;
 *                      since structure version 1.
 * @imported:      [out] the imported #FMAObjectItem -derived object, or %NULL;
 *                      since structure version 1.
 * @check_fn:      [in] a FMAIImporterCheckFn() function to check the existence
 *                      of the imported id;
 *                      since structure version 1.
 * @check_fn_data: [in] @check_fn data;
 *                      since structure version 1.
 * @ask_fn:        [in] a FMAIImporterAskUserFn() function to ask the user what to
 *                      do in case of a duplicate id;
 *                      since structure version 1.
 * @ask_fn_data:   [in] @ask_fn data;
 *                      since structure version 1.
 * @messages:      [in/out] a #GSList list of localized strings;
 *                      the provider may append messages to this list, but
 *                      shouldn't reinitialize it;
 *                      since structure version 1.
 *
 * This structure allows all used parameters when importing from an URI
 * to be passed and received through a single structure.
 *
 * Since: 2.30
 * Deprecated: 3.2
 */
typedef struct {
	guint                 version;
	gchar                *uri;
	guint                 asked_mode;
	gboolean              exist;
	guint                 import_mode;
	FMAObjectItem        *imported;
	FMAIImporterCheckFn   check_fn;
	void                 *check_fn_data;
	FMAIImporterAskUserFn ask_fn;
	void                 *ask_fn_data;
	GSList               *messages;
}
	FMAIImporterImportFromUriParms;

#endif /* FMA_ENABLE_DEPRECATED */

/**
 * FMAIImporterImportStatus:
 * @IMPORTER_CODE_OK:                import ok.
 * @IMPORTER_CODE_PROGRAM_ERROR:     a program error has been detected.
 *                                   You should open a bug in
 *                                   <ulink url="https://bugzilla.gnome.org/enter_bug.cgi?product=filemanager-actions">Bugzilla</ulink>.
 * @IMPORTER_CODE_NOT_WILLING_TO:    the plugin is not willing to import the uri.
 * @IMPORTER_CODE_NO_ITEM_ID:        item id not found.
 * @IMPORTER_CODE_NO_ITEM_TYPE:      item type not found.
 * @IMPORTER_CODE_UNKNOWN_ITEM_TYPE: unknown item type.
 * @IMPORTER_CODE_CANCELLED:         operation cancelled by the user.
 * @IMPORTER_CODE_NOT_LOADABLE:      the file is considered as not loadable at all.
 *                                   This is not a matter of which I/O provider has been tried,
 *                                   but the file is empty, or too big, or eventually not a
 *                                   regular file.
 *
 * Define the return status of an import operation.
 */
typedef enum {
	IMPORTER_CODE_OK = 0,
	IMPORTER_CODE_PROGRAM_ERROR,
	IMPORTER_CODE_NOT_WILLING_TO,
	IMPORTER_CODE_NO_ITEM_ID,
	IMPORTER_CODE_NO_ITEM_TYPE,
	IMPORTER_CODE_UNKNOWN_ITEM_TYPE,
	IMPORTER_CODE_CANCELLED,
	IMPORTER_CODE_NOT_LOADABLE
}
	FMAIImporterImportStatus;

/**
 * FMAIImporterImportFromUriParmsv2:
 * @version:       [in] the version of the structure, equals to 2;
 *                      since structure version 1.
 * @content:       [in] the version of the description content, equals to 1;
 *                      since structure version 2.
 * @uri:           [in] uri of the file to be imported;
 *                      since structure version 1.
 * @imported:      [out] the imported #FMAObjectItem -derived object, or %NULL;
 *                      since structure version 1.
 * @messages:      [in/out] a #GSList list of localized strings;
 *                      the provider may append messages to this list, but
 *                      shouldn't reinitialize it;
 *                      since structure version 1.
 *
 * This structure allows all used parameters when importing from an URI
 * to be passed and received through a single structure.
 *
 * Since: 3.2
 */
typedef struct {
	guint          version;
	guint          content;
	const gchar   *uri;
	FMAObjectItem *imported;
	GSList        *messages;
}
	FMAIImporterImportFromUriParmsv2;

GType fma_iimporter_get_type       ( void );

guint fma_iimporter_import_from_uri( const FMAIImporter *importer, FMAIImporterImportFromUriParmsv2 *parms );

G_END_DECLS

#endif /* __FILEMANAGER_ACTIONS_API_IIMPORTER_H__ */
