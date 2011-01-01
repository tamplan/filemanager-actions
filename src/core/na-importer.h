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

#ifndef __CORE_NA_IMPORTER_H__
#define __CORE_NA_IMPORTER_H__

/* @title: NAIImporter
 * @short_description: The #NAIImporter Internal Functions
 * @include: core/na-importer.h
 *
 * Internal Nautilus-Actions code should never directly call a
 * #NAIImporter interface method, but rather should call the
 * corresponding na_importer_xxx() function.
 */

#include <gtk/gtk.h>

#include <api/na-iimporter.h>
#include <api/na-object-item.h>

#include <core/na-pivot.h>

G_BEGIN_DECLS

typedef struct {
	GtkWindow         *parent;			/* the parent window, if any */
	GSList            *uris;			/* the list of uris of the files to be imported */
	guint              mode;			/* asked import mode */
	NAIImporterCheckFn check_fn;		/* a function to check the existence of the imported id */
	void              *check_fn_data;	/* data function */
	GList             *results;			/* a #GList of newly allocated NAImporterResult structures,
										   one for each imported uri, which should be
										   na_importer_free_result() by the caller */
}
	NAImporterParms;

typedef struct {
	gchar             *uri;				/* the imported uri */
	guint              mode;			/* the actual import mode in effect for this import */
	gboolean           exist;			/* whether the imported Id already existed */
	NAObjectItem      *imported;		/* eventually imported NAObjectItem-derived object, or %NULL */
	GSList            *messages;		/* a #GSList list of localized strings */
}
	NAImporterResult;

guint na_importer_import_from_list( const NAPivot *pivot, NAImporterParms *parms );

void  na_importer_free_result( NAImporterResult *result );

G_END_DECLS

#endif /* __CORE_NA_IMPORTER_H__ */
