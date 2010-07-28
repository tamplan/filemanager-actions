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

#ifndef __NAUTILUS_ACTIONS_API_NA_IIMPORTER_H__
#define __NAUTILUS_ACTIONS_API_NA_IIMPORTER_H__

/**
 * SECTION: na_iimporter
 * @short_description: #NAIImporter interface definition.
 * @include: nautilus-actions/na-iimporter.h
 *
 * The #NAIImporter interface imports items from the outside world.
 *
 * Nautilus-Actions v 2.30 - API version:  1
 */

#include "na-object-item.h"

G_BEGIN_DECLS

#define NA_IIMPORTER_TYPE						( na_iimporter_get_type())
#define NA_IIMPORTER( instance )				( G_TYPE_CHECK_INSTANCE_CAST( instance, NA_IIMPORTER_TYPE, NAIImporter ))
#define NA_IS_IIMPORTER( instance )				( G_TYPE_CHECK_INSTANCE_TYPE( instance, NA_IIMPORTER_TYPE ))
#define NA_IIMPORTER_GET_INTERFACE( instance )	( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NA_IIMPORTER_TYPE, NAIImporterInterface ))

typedef struct NAIImporter                      NAIImporter;
typedef struct NAIImporterInterfacePrivate      NAIImporterInterfacePrivate;

typedef struct NAIImporterImportFromUriParms    NAIImporterImportFromUriParms;
typedef struct NAIImporterManageImportModeParms NAIImporterManageImportModeParms;

typedef struct {
	GTypeInterface               parent;
	NAIImporterInterfacePrivate *private;

	/**
	 * get_version:
	 * @instance: the #NAIImporter provider.
	 *
	 * Returns: the version of this interface supported by the I/O provider.
	 *
	 * Defaults to 1.
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
	 */
	guint ( *import_from_uri )( const NAIImporter *instance, NAIImporterImportFromUriParms *parms );
}
	NAIImporterInterface;

/* import mode
 */
enum {
	IMPORTER_MODE_NO_IMPORT = 1,		/* this is a "do not import anything" mode */
	IMPORTER_MODE_RENUMBER,
	IMPORTER_MODE_OVERRIDE,
	IMPORTER_MODE_ASK
};

/* return code
 */
enum {
	IMPORTER_CODE_OK = 0,
	IMPORTER_CODE_PROGRAM_ERROR,
	IMPORTER_CODE_NOT_WILLING_TO,
	IMPORTER_CODE_NO_ITEM_ID,
	IMPORTER_CODE_NO_ITEM_TYPE,
	IMPORTER_CODE_UNKNOWN_ITEM_TYPE,
	IMPORTER_CODE_CANCELLED
};

/**
 * This function may be provided by the caller in order the #NAIImporter
 * provider be able to check for pre-existence of the imported item.
 * This function should return the already existing item which has the
 * same id than the currently being imported one, or %NULL if the
 * imported id will be unique.
 * If this function is not provided, then the #NAIImporter provider will not
 * be able to check for duplicates. In this case, the id of the imported item
 * should be systematically regenerated as a unique id (uuid), regardless of
 * the asked import mode.
 *
 * (E): - currently imported item
 *      - fn_data
 * (S): - already existing item with same id, or %NULL.
 */
typedef NAObjectItem * ( *NAIImporterCheckFn )  ( const NAObjectItem *, void *fn_data );

/**
 * This function may be provided by the caller in order the #NAIImporter
 * provider be able to ask the user to know what to do in the case of a
 * duplicate id.
 * This function should return an mode import (not ASK!).
 * If this function is not provided, then the #NAIImporter provider will
 * not be able to ask the user. In this case, the duplicated id should be
 * systematically regenerated as a unique id (uuid).
 *
 * (E): - currently imported item
 *      - already existing item with same id
 *      - fn_data
 * (S): - import mode choosen by the user
 */
typedef guint          ( *NAIImporterAskUserFn )( const NAObjectItem *, const NAObjectItem *, void *fn_data );

/*
 * parameters used in input/output are passed or received through a single structure
 */
struct NAIImporterImportFromUriParms {
	guint                version;		/* i 1: version of this structure */
	gchar               *uri;			/* i 1: uri of the file to be imported */
	guint                asked_mode;	/* i 1: asked import mode */
	gboolean             exist;			/*  o1: whether the imported Id already existed */
	guint                import_mode;	/*  o1: actually used import mode */
	NAObjectItem        *imported;		/*  o1: the imported NAObjectItem-derived object, or %NULL */
	NAIImporterCheckFn   check_fn;		/* i 1: a function to check the existence of the imported id */
	void                *check_fn_data;	/* i 1: data function */
	NAIImporterAskUserFn ask_fn;		/* i 1: a function to ask the user what to do in case of a duplicate id */
	void                *ask_fn_data;	/* i 1: data function */
	GSList              *messages;		/* io1: a #GSList list of localized strings;
										 *       the provider may append messages to this list,
										 *       but shouldn't reinitialize it. */
};

/*
 * parameters used when managing import mode
 */
struct NAIImporterManageImportModeParms {
	guint                version;		/* i 1: version of this structure */
	NAObjectItem        *imported;		/* i 1: the imported NAObjectItem-derived object */
	guint                asked_mode;	/* i 1: asked import mode */
	NAIImporterCheckFn   check_fn;		/* i 1: a function to check the existence of the imported id */
	void                *check_fn_data;	/* i 1: data function */
	NAIImporterAskUserFn ask_fn;		/* i 1: a function to ask the user what to do in case of a duplicate id */
	void                *ask_fn_data;	/* i 1: data function */
	gboolean             exist;			/*  o1: whether the imported Id already existed */
	guint                import_mode;	/*  o1: actually used import mode */
	GSList              *messages;		/* io1: a #GSList list of localized strings;
										 *       the provider may append messages to this list,
										 *       but shouldn't reinitialize it. */
};

GType na_iimporter_get_type( void );

guint na_iimporter_import_from_uri( const NAIImporter *importer, NAIImporterImportFromUriParms *parms );

guint na_iimporter_manage_import_mode( NAIImporterManageImportModeParms *parms );

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_API_NA_IIMPORTER_H__ */
