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

#ifndef __CORE_FMA_IMPORT_MODE_H__
#define __CORE_FMA_IMPORT_MODE_H__

/* @title: FMAImportMode
 * @short_description: The #FMAImportMode Class Definition
 * @include: core/fma-import-mode.h
 *
 * This class gathers and manages the different import modes we are able
 * to deal with.
 */

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib.h>

G_BEGIN_DECLS

#define FMA_TYPE_IMPORT_MODE                ( fma_import_mode_get_type())
#define FMA_IMPORT_MODE( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_IMPORT_MODE, FMAImportMode ))
#define FMA_IMPORT_MODE_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_IMPORT_MODE, FMAImportModeClass ))
#define FMA_IS_IMPORT_MODE( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_IMPORT_MODE ))
#define FMA_IS_IMPORT_MODE_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_IMPORT_MODE ))
#define FMA_IMPORT_MODE_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_IMPORT_MODE, FMAImportModeClass ))

typedef struct _FMAImportModePrivate        FMAImportModePrivate;

typedef struct {
	GObject               parent;
	FMAImportModePrivate *private;
}
	FMAImportMode;

typedef struct _FMAImportModeClassPrivate   FMAImportModeClassPrivate;

typedef struct {
	GObjectClass               parent;
	FMAImportModeClassPrivate *private;
}
	FMAImportModeClass;

GType          fma_import_mode_get_type( void );

/* FMAImportMode properties
 *
 */
#define FMA_IMPORT_PROP_MODE			"fma-import-mode-prop-mode"
#define FMA_IMPORT_PROP_LABEL			"fma-import-mode-prop-label"
#define FMA_IMPORT_PROP_DESCRIPTION		"fma-import-mode-prop-description"
#define FMA_IMPORT_PROP_IMAGE			"fma-import-mode-prop-image"

FMAImportMode *fma_import_mode_new     ( guint mode_id );

guint          fma_import_mode_get_id  ( const FMAImportMode *mode );

G_END_DECLS

#endif /* __CORE_FMA_IMPORT_MODE_H__ */
