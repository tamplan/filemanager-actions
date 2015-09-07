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

#ifndef __FILE_MANAGER_ACTIONS_API_FMA_BOXED_H__
#define __FILE_MANAGER_ACTIONS_API_FMA_BOXED_H__

/**
 * SECTION: boxed
 * @title: FMABoxed
 * @short_description: The FMABoxed Structure
 * @include: file-manager-actions/fma-boxed.h
 *
 * The FMABoxed structure is a way of handling various types of data in an
 * opaque structure.
 *
 * Since: 3.1
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define FMA_TYPE_BOXED                ( fma_boxed_get_type())
#define FMA_BOXED( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_BOXED, FMABoxed ))
#define FMA_BOXED_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_BOXED, FMABoxedClass ))
#define FMA_IS_BOXED( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_BOXED))
#define FMA_IS_BOXED_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_BOXED))
#define FMA_BOXED_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_BOXED, FMABoxedClass ))

typedef struct _FMABoxedPrivate       FMABoxedPrivate;

typedef struct {
	/*< private >*/
	GObject          parent;
	FMABoxedPrivate *private;
}
	FMABoxed;

typedef struct _FMABoxedClassPrivate  FMABoxedClassPrivate;

typedef struct {
	/*< private >*/
	GObjectClass          parent;
	FMABoxedClassPrivate *private;
}
	FMABoxedClass;

GType         fma_boxed_get_type       ( void );
void          fma_boxed_set_type       ( FMABoxed *boxed, guint type );

gboolean      fma_boxed_are_equal      ( const FMABoxed *a, const FMABoxed *b );
FMABoxed     *fma_boxed_copy           ( const FMABoxed *boxed );
void          fma_boxed_dump           ( const FMABoxed *boxed );
FMABoxed     *fma_boxed_new_from_string( guint type, const gchar *string );

gboolean      fma_boxed_get_boolean    ( const FMABoxed *boxed );
gconstpointer fma_boxed_get_pointer    ( const FMABoxed *boxed );
gchar        *fma_boxed_get_string     ( const FMABoxed *boxed );
GSList       *fma_boxed_get_string_list( const FMABoxed *boxed );
guint         fma_boxed_get_uint       ( const FMABoxed *boxed );
GList        *fma_boxed_get_uint_list  ( const FMABoxed *boxed );
void          fma_boxed_get_as_value   ( const FMABoxed *boxed, GValue *value );
void         *fma_boxed_get_as_void    ( const FMABoxed *boxed );

void          fma_boxed_set_from_boxed ( FMABoxed *boxed, const FMABoxed *value );
void          fma_boxed_set_from_string( FMABoxed *boxed, const gchar *value );
void          fma_boxed_set_from_value ( FMABoxed *boxed, const GValue *value );
void          fma_boxed_set_from_void  ( FMABoxed *boxed, const void *value );

G_END_DECLS

#endif /* __FILE_MANAGER_ACTIONS_API_FMA_BOXED_H__ */
