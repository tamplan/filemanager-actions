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

#ifndef __NAUTILUS_ACTIONS_API_NA_IIMPORTER_H__
#define __NAUTILUS_ACTIONS_API_NA_IIMPORTER_H__

/**
 * SECTION: iimporter
 * @title: NAIImporter
 * @short_description: The Import Interface v 1
 * @include: nautilus-actions/na-iimporter.h
 *
 * The #NAIImporter interface imports items from the outside world.
 *
 * <refsect2>
 *  <title>Versions historic</title>
 *  <table>
 *    <title>Historic of the versions of the #NAIImporter interface</title>
 *    <tgroup rowsep="1" colsep="1" align="center" cols="3">
 *      <colspec colname="na-version" />
 *      <colspec colname="api-version" />
 *      <colspec colname="current" />
 *      <thead>
 *        <row>
 *          <entry>&prodname; version</entry>
 *          <entry>#NAIImporter interface version</entry>
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

#include "na-object-item.h"

G_BEGIN_DECLS

#define NA_IIMPORTER_TYPE                      ( na_iimporter_get_type())
#define NA_IIMPORTER( instance )               ( G_TYPE_CHECK_INSTANCE_CAST( instance, NA_IIMPORTER_TYPE, NAIImporter ))
#define NA_IS_IIMPORTER( instance )            ( G_TYPE_CHECK_INSTANCE_TYPE( instance, NA_IIMPORTER_TYPE ))
#define NA_IIMPORTER_GET_INTERFACE( instance ) ( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NA_IIMPORTER_TYPE, NAIImporterInterface ))

typedef struct _NAIImporter                      NAIImporter;
typedef struct _NAIImporterInterfacePrivate      NAIImporterInterfacePrivate;
typedef struct _NAIImporterImportFromUriParms    NAIImporterImportFromUriParms;
typedef struct _NAIImporterManageImportModeParms NAIImporterManageImportModeParms;

/**
 * NAIImporterInterface:
 * @get_version:     returns the version of this interface that the
 *                   plugin implements.
 * @import_from_uri: imports an item.
 *
 * This defines the interface that a #NAIImporter should implement.
 */
typedef struct {
	/*< private >*/
	GTypeInterface               parent;
	NAIImporterInterfacePrivate *private;

	/*< public >*/
	/**
	 * get_version:
	 * @instance: the #NAIImporter provider.
	 *
	 * This method is called by the &prodname; program each time
	 * it needs to know which version of this interface the plugin
	 * implements.
	 *
	 * If this method is not implemented by the plugin,
	 * the &prodname; program considers that the plugin only implements
	 * the version 1 of the #NAIImporter interface.
	 *
	 * Returns: the version of this interface supported by the plugin.
	 *
	 * Since: 2.30
	 */
	guint ( *get_version )    ( const NAIImporter *instance );

	/**
	 * import_from_uri:
	 * @instance: the #NAIImporter provider.
	 * @parms: a #NAIImporterImportFromUriParms structure.
	 *
	 * Imports an item.
	 *
	 * Returns: the return code of the operation.
	 *
	 * Since: 2.30
	 */
	guint ( *import_from_uri )( const NAIImporter *instance, NAIImporterImportFromUriParms *parms );
}
	NAIImporterInterface;

/**
 * NAIImporterImportMode:
 * @IMPORTER_MODE_NO_IMPORT: a "do not import anything" mode.
 * @IMPORTER_MODE_RENUMBER:  reallocate a new id when the imported one
 *                           already exists.
 * @IMPORTER_MODE_OVERRIDE:  override the existing id with the imported
 *                           one.
 * @IMPORTER_MODE_ASK:       ask the user for what to do with this particular
 *                           item.
 *
 * Define the mode of an import operation.
 */
typedef enum {
	IMPORTER_MODE_NO_IMPORT = 1,
	IMPORTER_MODE_RENUMBER,
	IMPORTER_MODE_OVERRIDE,
	IMPORTER_MODE_ASK
}
	NAIImporterImportMode;

/**
 * NAIImporterImportStatus:
 * @IMPORTER_CODE_OK:                import ok.
 * @IMPORTER_CODE_PROGRAM_ERROR:     a program error has been detected.
 *                                   You should open a bug in
 *                                   <ulink url="https://bugzilla.gnome.org/enter_bug.cgi?product=nautilus-actions">Bugzilla</ulink>.
 * @IMPORTER_CODE_NOT_WILLING_TO:    the plugin is not willing to import
 *                                   anything.
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
	NAIImporterImportStatus;

/**
 * NAIImporterCheckFn:
 * @imported: the currently imported #NAObjectItem -derived object.
 * @fn_data: some data to be passed to the function.
 *
 * This function may be provided by the caller in order the #NAIImporter
 * provider be able to check for pre-existence of the imported item.
 * This function should return the already existing item which has the
 * same id than the currently being imported one, or %NULL if the
 * imported id will be unique.
 *
 * If this function is not provided, then the #NAIImporter provider will not
 * be able to check for duplicates. In this case, the id of the imported item
 * should be systematically regenerated as a unique id, regardless of the
 * asked import mode.
 *
 * Standard N-A callers provide a function which checks for the existance
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
 * Items which may have been loaded by NAPivot at the start of the
 * application, and deleted meanwhile, are just ignored.
 *
 * Returns: the already existing #NAObjectItem with same id, or %NULL.
 *
 * Since: 2.30
 */
typedef NAObjectItem * ( *NAIImporterCheckFn )( const NAObjectItem *, void * );

/**
 * NAIImporterAskUserFn:
 * @imported: the currently imported #NAObjectItem.
 * @existing: an already existing #NAObjectItem with same id.
 * @fn_data: some data to be passed to the function.
 *
 * This function may be provided by the caller as a convenience way for
 * the #NAIImporter to ask the user to know what to do in the case of a
 * duplicate id.
 *
 * If this function is not provided, and the #NAIImporter does not have
 * any other way to ask the user, then a 'no import' policy should be
 * preferred when managing duplicate identifiers.
 *
 * Returns: the import mode chosen by the user, which must not be
 * %IMPORTER_MODE_ASK.
 *
 * Since: 2.30
 */
typedef guint ( *NAIImporterAskUserFn )( const NAObjectItem *, const NAObjectItem *, void * );

/**
 * NAIImporterImportFromUriParms:
 * @version:       the version of this structure, currently equals to 1.
 *                 input;
 *                 since version 1 of the structure.
 * @uri:           uri of the file to be imported.
 *                 input;
 *                 since version 1 of the structure.
 * @asked_mode:    asked import mode.
 *                 input;
 *                 since version 1 of the structure.
 * @exist:         whether the imported Id already existed.
 *                 output;
 *                 since version 1 of the structure.
 * @import_mode:   actually used import mode.
 *                 output;
 *                 since version 1 of the structure.
 * @imported:      the imported #NAObjectItem -derived object, or %NULL.
 *                 output;
 *                 since version 1 of the structure.
 * @check_fn:      a NAIImporterCheckFn() function to check the existence of the imported id.
 *                 input;
 *                 since version 1 of the structure.
 * @check_fn_data: @check_fn data
 *                 input;
 *                 since version 1 of the structure.
 * @ask_fn:        a NAIImporterAskUserFn() function to ask the user what to do in case of a duplicate id
 *                 input;
 *                 since version 1 of the structure.
 * @ask_fn_data:   @ask_fn data
 *                 input;
 *                 since version 1 of the structure.
 * @messages:      a #GSList list of localized strings;
 *                 the provider may append messages to this list, but shouldn't reinitialize it
 *                 input/output;
 *                 since version 1 of the structure.
 *
 * This structure allows all used parameters when importing from an URI
 * to be passed and received through a single structure.
 *
 * Since: 2.30
 */
struct _NAIImporterImportFromUriParms {
	guint                version;
	gchar               *uri;
	guint                asked_mode;
	gboolean             exist;
	guint                import_mode;
	NAObjectItem        *imported;
	NAIImporterCheckFn   check_fn;
	void                *check_fn_data;
	NAIImporterAskUserFn ask_fn;
	void                *ask_fn_data;
	GSList              *messages;
};

/**
 * NAIImporterManageImportModeParms:
 * @version:       the version of this structure, currently equals to 1.
 *                 input;
 *                 since version 1 of the structure.
 * @imported:      the imported #NAObjectItem -derived object
 * @asked_mode:    asked import mode
 * @check_fn:      a #NAIImporterCheckFn function to check the existence of the imported id.
 *                 input;
 *                 since version 1 of the structure.
 * @check_fn_data: @check_fn data
 *                 input;
 *                 since version 1 of the structure.
 * @ask_fn:        a #NAIImporterAskUserFn function to ask the user what to do in case of a duplicate id
 *                 input;
 *                 since version 1 of the structure.
 * @ask_fn_data:   @ask_fn data
 *                 input;
 *                 since version 1 of the structure.
 * @exist:         whether the imported Id already existed
 *                 output;
 *                 since version 1 of the structure.
 * @import_mode:   actually used import mode
 *                 output;
 *                 since version 1 of the structure.
 * @messages:      a #GSList list of localized strings;
 *                 the provider may append messages to this list, but shouldn't reinitialize it
 *                 input/output;
 *                 since version 1 of the structure.
 *
 * This structure allows all used parameters when managing the import mode
 * to be passed and received through a single structure.
 *
 * Since: 2.30
 */
struct _NAIImporterManageImportModeParms {
	guint                version;
	NAObjectItem        *imported;
	guint                asked_mode;
	NAIImporterCheckFn   check_fn;
	void                *check_fn_data;
	NAIImporterAskUserFn ask_fn;
	void                *ask_fn_data;
	gboolean             exist;
	guint                import_mode;
	GSList              *messages;
};

GType na_iimporter_get_type( void );

guint na_iimporter_import_from_uri( const NAIImporter *importer, NAIImporterImportFromUriParms *parms );

guint na_iimporter_manage_import_mode( NAIImporterManageImportModeParms *parms );

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_API_NA_IIMPORTER_H__ */
