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

#ifndef __FILE_MANAGER_ACTIONS_API_DATA_BOXED_H__
#define __FILE_MANAGER_ACTIONS_API_DATA_BOXED_H__

/**
 * SECTION: data-boxed
 * @title: FMADataBoxed
 * @short_description: The Data Factory Element Class Definition
 * @include: file-manager-actions/fma-data-boxed.h
 *
 * The object which encapsulates an elementary data of #NAIFactoryObject.
 * A #FMADataBoxed object has a type and a value.
 *
 * #FMADataBoxed class is derived from #FMABoxed one, and implements the same
 * types that those defined in na-data-types.h.
 *
 * Additionally, #FMADataBoxed class holds the #NADataDef data definition
 * suitable for a NAFactoryObject object. It such provides default value
 * and validity status.
 *
 * Since: 2.30
 */

#include <glib-object.h>

#include "fma-boxed.h"
#include "na-data-def.h"

G_BEGIN_DECLS

#define FMA_TYPE_DATA_BOXED                ( fma_data_boxed_get_type())
#define FMA_DATA_BOXED( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_DATA_BOXED, FMADataBoxed ))
#define FMA_DATA_BOXED_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_DATA_BOXED, FMADataBoxedClass ))
#define FMA_IS_DATA_BOXED( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_DATA_BOXED ))
#define FMA_IS_DATA_BOXED_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_DATA_BOXED ))
#define FMA_DATA_BOXED_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_DATA_BOXED, FMADataBoxedClass ))

typedef struct _FMADataBoxedPrivate        FMADataBoxedPrivate;

typedef struct {
	/*< private >*/
	FMABoxed             parent;
	FMADataBoxedPrivate *private;
}
	FMADataBoxed;

typedef struct _FMADataBoxedClassPrivate   FMADataBoxedClassPrivate;

typedef struct {
	/*< private >*/
	FMABoxedClass             parent;
	FMADataBoxedClassPrivate *private;
}
	FMADataBoxedClass;

GType             fma_data_boxed_get_type( void );

FMADataBoxed     *fma_data_boxed_new            ( const NADataDef *def );

const NADataDef *fma_data_boxed_get_data_def   ( const FMADataBoxed *boxed );
void              fma_data_boxed_set_data_def   ( FMADataBoxed *boxed, const NADataDef *def );

GParamSpec       *fma_data_boxed_get_param_spec ( const NADataDef *def );

gboolean          fma_data_boxed_is_default     ( const FMADataBoxed *boxed );
gboolean          fma_data_boxed_is_valid       ( const FMADataBoxed *boxed );

/* These functions are deprecated starting with 3.1.0
 */
#ifdef NA_ENABLE_DEPRECATED
gboolean          fma_data_boxed_are_equal      ( const FMADataBoxed *a, const FMADataBoxed *b );
void              fma_data_boxed_dump           ( const FMADataBoxed *boxed );
gchar            *fma_data_boxed_get_as_string  ( const FMADataBoxed *boxed );
void              fma_data_boxed_get_as_value   ( const FMADataBoxed *boxed, GValue *value );
void             *fma_data_boxed_get_as_void    ( const FMADataBoxed *boxed );
void              fma_data_boxed_set_from_boxed ( FMADataBoxed *boxed, const FMADataBoxed *value );
void              fma_data_boxed_set_from_string( FMADataBoxed *boxed, const gchar *value );
void              fma_data_boxed_set_from_value ( FMADataBoxed *boxed, const GValue *value );
void              fma_data_boxed_set_from_void  ( FMADataBoxed *boxed, const void *value );
#endif /* NA_ENABLE_DEPRECATED */

G_END_DECLS

#endif /* __FILE_MANAGER_ACTIONS_API_DATA_BOXED_H__ */
