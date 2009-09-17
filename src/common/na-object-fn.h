/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009 Pierre Wieser and others (see AUTHORS)
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

#ifndef __NA_OBJECT_FN_H__
#define __NA_OBJECT_FN_H__

/**
 * SECTION: na_object
 * @short_description: #NAObject public functions definition.
 * @include: common/na-object-fn.h
 *
 * Define here the public functions of the #NAObject class.
 *
 * Note that most users of the class should rather use macros defined
 * in na-object-api.h
 */

#include "na-object-class.h"

G_BEGIN_DECLS

/* NAObject
 */
void      na_object_object_dump( const NAObject *object );
gchar    *na_object_object_get_clipboard_id( const NAObject *object );
NAObject *na_object_object_ref( const NAObject *object );
void      na_object_object_copy( NAObject *target, const NAObject *source );

GList    *na_object_get_hierarchy( const NAObject *object );
void      na_object_free_hierarchy( GList *hierarchy );

/* NAIDuplicable
 */
NAObject *na_object_iduplicable_duplicate( const NAObject *object );

void      na_object_iduplicable_check_edition_status( const NAObject *object );
gboolean  na_object_iduplicable_are_equal( const NAObject *a, const NAObject *b );
gboolean  na_object_iduplicable_is_modified( const NAObject *object );
gboolean  na_object_iduplicable_is_valid( const NAObject *object );

NAObject *na_object_iduplicable_get_origin( const NAObject *object );
void      na_object_iduplicable_set_origin( NAObject *object, const NAObject *origin );

G_END_DECLS

#endif /* __NA_OBJECT_FN_H__ */
