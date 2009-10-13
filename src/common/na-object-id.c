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

#include <glib/gi18n.h>
#include <string.h>

#include <runtime/na-object-api.h>
#include <runtime/na-object-id-priv.h>

#include "na-object-api.h"
#include "na-iprefs.h"

/**
 * na_object_id_check_status_up:
 * @object: the object at the start of the hierarchy.
 *
 * Checks for modification and validity status of the @object, its
 * parent, the parent of its parent, etc. up to the top of the hierarchy.
 *
 * Returns: %TRUE if at least one of the status has changed, %FALSE else.
 *
 * Checking the modification of any of the status should be more
 * efficient that systematically force the display of the item.
 */
gboolean
na_object_id_check_status_up( NAObjectId *object )
{
	gboolean changed;
	gboolean was_modified, is_modified;
	gboolean was_valid, is_valid;

	g_return_val_if_fail( NA_OBJECT_ID( object ), FALSE );

	changed = FALSE;

	if( !object->private->dispose_has_run ){

		was_modified = na_object_is_modified( object );
		was_valid = na_object_is_valid( object );

		na_iduplicable_check_status( NA_IDUPLICABLE( object ));

		is_modified = na_object_is_modified( object );
		is_valid = na_object_is_valid( object );

		if( object->private->parent ){
			na_object_id_check_status_up( NA_OBJECT_ID( object->private->parent ));
		}

		changed =
			( was_modified && !is_modified ) ||
			( !was_modified && is_modified ) ||
			( was_valid && !is_valid ) ||
			( !was_valid && is_valid );
	}

	return( changed );
}

/**
 * na_object_id_get_topmost_parent:
 * @object: the #NAObject whose parent is searched.
 *
 * Returns: the topmost parent, maybe @object itself.
 */
NAObjectId *
na_object_id_get_topmost_parent( NAObjectId *object )
{
	NAObjectId *parent;

	g_return_val_if_fail( NA_IS_OBJECT_ID( object ), NULL );

	parent = object;

	if( !object->private->dispose_has_run ){

		while( parent->private->parent ){
			parent = NA_OBJECT_ID( parent->private->parent );
		}
	}

	return( parent );
}

/**
 * na_object_id_prepare_for_paste:
 * @object: the #NAObjectId object to be pasted.
 * @pivot; the #NAPivot instance which let us access to preferences.
 * @relabel: whether this item should be renumbered ?
 * @action: if @object is a #NAObjectProfile, the attached #NAObjectAction.
 *
 * Prepares @object to be pasted.
 *
 * If a #NAObjectProfile, then @object is attached to the specified
 * #NAObjectAction @action. The identifier is always renumbered to be
 * suitable with the already existing profiles.
 *
 * If a #NAObjectAction or a #NAObjectMenu, a new UUID is allocated if
 * and only if @relabel is %TRUE.
 *
 * Actual relabeling takes place if @relabel is %TRUE, depending of the
 * user preferences.
 */
void
na_object_id_prepare_for_paste( NAObjectId *object, NAPivot *pivot, gboolean renumber, NAObjectAction *action )
{
	static const gchar *thisfn = "na_object_id_prepare_for_paste";
	gboolean user_relabel;

	g_return_if_fail( NA_IS_OBJECT_ID( object ));
	g_return_if_fail( NA_IS_PIVOT( pivot ));
	g_return_if_fail( !action || NA_IS_OBJECT_ACTION( action ));

	if( !object->private->dispose_has_run ){

		user_relabel = FALSE;

		if( NA_IS_OBJECT_MENU( object )){
			user_relabel = na_iprefs_read_bool( NA_IPREFS( pivot ), IPREFS_RELABEL_MENUS, FALSE );
		} else if( NA_IS_OBJECT_ACTION( object )){
			user_relabel = na_iprefs_read_bool( NA_IPREFS( pivot ), IPREFS_RELABEL_ACTIONS, FALSE );
		} else if( NA_IS_OBJECT_PROFILE( object )){
			user_relabel = na_iprefs_read_bool( NA_IPREFS( pivot ), IPREFS_RELABEL_PROFILES, FALSE );
		} else {
			g_warning( "%s: unknown object type at %p", thisfn, ( void * ) object );
			g_return_if_reached();
		}

		if( NA_IS_OBJECT_PROFILE( object )){
			na_object_set_parent( object, action );
			na_object_set_new_id( object, action );
			if( renumber && user_relabel ){
				na_object_set_copy_of_label( object );
			}

		} else {
			if( renumber ){
				na_object_set_new_id( object, NULL );
				if( user_relabel ){
					na_object_set_copy_of_label( object );
				}
			}
		}
	}
}

/**
 * na_object_id_set_copy_of_label:
 * @object: the #NAObjectId object whose label is to be changed.
 *
 * Sets the 'Copy of' label.
 */
void
na_object_id_set_copy_of_label( NAObjectId *object )
{
	gchar *new_label;

	g_return_if_fail( NA_IS_OBJECT_ID( object ));

	if( !object->private->dispose_has_run ){

		/* i18n: copied items have a label as 'Copy of original label' */
		new_label = g_strdup_printf( _( "Copy of %s" ), object->private->label );
		g_free( object->private->label );
		object->private->label = new_label;
	}
}
