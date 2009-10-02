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

/* private class data
 */
struct NAObjectIdClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/**
 * na_object_id_set_for_copy:
 * @object: the #NAObjectId object to be copied.
 * @relabel: whether this item should be relabeled ?
 *
 * Prepares @object to be copied, allocating to it a new uuid if apply,
 * and relabeling it as "Copy of ..." if applies.
 */
void
na_object_id_set_for_copy( NAObjectId *object, gboolean relabel )
{
	gchar *new_label;

	g_return_if_fail( NA_IS_OBJECT_ID( object ));

	if( !object->private->dispose_has_run ){

		/* TODO: review the set_for_copy function */
		na_object_id_set_new_id( object, NULL );

		if( relabel ){
			/* i18n: copied items have a label as 'Copy of original label' */
			new_label = g_strdup_printf( _( "Copy of %s" ), object->private->label );
			g_free( object->private->label );
			object->private->label = new_label;
		}
	}
}
