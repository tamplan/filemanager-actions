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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <runtime/na-object-api.h>
#include <runtime/na-object-priv.h>

#include "na-object-api.h"

static GList *v_get_childs( const NAObject *object );

/**
 * na_object_iduplicable_get_origin:
 * @object: the #NAObject object whose status is requested.
 *
 * Returns the original object which was at the origin of @object.
 *
 * Returns: a #NAObject, or NULL.
 */
NAObject *
na_object_iduplicable_get_origin( const NAObject *object )
{
	NAObject *origin = NULL;

	g_return_val_if_fail( NA_IS_OBJECT( object ), NULL );

	if( !object->private->dispose_has_run ){
		/* do not use NA_OBJECT macro as we may return a (valid) NULL value */
		origin = ( NAObject * ) na_iduplicable_get_origin( NA_IDUPLICABLE( object ));
	}

	return( origin );
}

/**
 * na_object_iduplicable_set_origin:
 * @object: the #NAObject object whose origin is to be set.
 * @origin: a #NAObject which will be set as the new origin of @object.
 *
 * Sets the new origin of @object, and of all its childs.
 *
 * Be warned: but recursively reinitializing the origin to NULL, this
 * function may cause difficult to solve issues.
 */
void
na_object_iduplicable_set_origin( NAObject *object, const NAObject *origin )
{
	GList *childs, *ic;

	g_return_if_fail( NA_IS_OBJECT( object ));
	g_return_if_fail( NA_IS_OBJECT( origin ) || !origin );

	if( !object->private->dispose_has_run &&
		( !origin || !origin->private->dispose_has_run )){

			na_iduplicable_set_origin( NA_IDUPLICABLE( object ), NA_IDUPLICABLE( origin ));

			childs = v_get_childs( object );
			for( ic = childs ; ic ; ic = ic->next ){
				na_object_iduplicable_set_origin( NA_OBJECT( ic->data ), origin );
			}
	}
}

/**
 * na_object_object_reset_origin:
 * @object: a #NAObject-derived object.
 * @origin: must be a duplication of @object.
 *
 * Recursively reset origin of @object and its childs to @origin and
 * its childs), so that @origin appear as the actual origin of @object.
 *
 * The origin of @origin itself is set to NULL.
 *
 * This only works if @origin has just been duplicated from @object,
 * and thus we do not have to check if childs lists are equal.
 */
void
na_object_object_reset_origin( NAObject *object, const NAObject *origin )
{
	GList *origin_childs, *iorig;
	GList *object_childs, *iobj;
	NAObject *orig_object;

	g_return_if_fail( NA_IS_OBJECT( origin ));
	g_return_if_fail( NA_IS_OBJECT( object ));

	if( !object->private->dispose_has_run && !origin->private->dispose_has_run ){

		origin_childs = v_get_childs( origin );
		object_childs = v_get_childs( object );
		for( iorig = origin_childs, iobj = object_childs ; iorig && iobj ; iorig = iorig->next, iobj = iobj->next ){
			orig_object = na_object_get_origin( iorig->data );
			g_return_if_fail( orig_object == iobj->data );
			na_object_reset_origin( iobj->data, iorig->data );
		}

		orig_object = na_object_get_origin( origin );
		g_return_if_fail( orig_object == object );
		na_iduplicable_set_origin( NA_IDUPLICABLE( object ), NA_IDUPLICABLE( origin ));
		na_iduplicable_set_origin( NA_IDUPLICABLE( origin ), NULL );
	}
}

/**
 * na_object_object_reset_status:
 * @object: a #NAObject-derived object.
 *
 * Recursively reset status of @object and its childs.
 *
 * This is typically used when we want display a clean list of current
 * items (e.g. in Export assistant).
 */
void
na_object_object_reset_status( NAObject *object )
{
	GList *object_childs, *iobj;

	g_return_if_fail( NA_IS_OBJECT( object ));

	if( !object->private->dispose_has_run ){

		object_childs = v_get_childs( object );
		for( iobj = object_childs ; iobj ; iobj = iobj->next ){
			na_iduplicable_reset_status( NA_IDUPLICABLE( iobj->data ));
		}

		na_iduplicable_reset_status( NA_IDUPLICABLE( object ));
	}
}

static GList *
v_get_childs( const NAObject *object )
{
	return( na_object_most_derived_get_childs( object ));
}
