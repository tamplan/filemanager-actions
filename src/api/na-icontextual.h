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

#ifndef __NAUTILUS_ACTIONS_API_NA_ICONTEXTUAL_H__
#define __NAUTILUS_ACTIONS_API_NA_ICONTEXTUAL_H__

/**
 * SECTION: na_icontextual
 * @short_description: #NAIContextual interface definition.
 * @include: nautilus-actions/na-icontextual.h
 *
 * This interface is implemented by all #NAObject-derived objects
 * whose the display in the Nautilus context menu is subject to some
 * conditions.
 *
 * Implementors, typically actions, profiles and menus, host the required
 * data as #NADataBoxed in a dedicated NA_FACTORY_CONDITIONS_GROUP
 * data group.
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define NA_ICONTEXTUAL_TYPE							( na_icontextual_get_type())
#define NA_ICONTEXTUAL( instance )					( G_TYPE_CHECK_INSTANCE_CAST( instance, NA_ICONTEXTUAL_TYPE, NAIContextual ))
#define NA_IS_ICONTEXTUAL( instance )				( G_TYPE_CHECK_INSTANCE_TYPE( instance, NA_ICONTEXTUAL_TYPE ))
#define NA_ICONTEXTUAL_GET_INTERFACE( instance )	( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NA_ICONTEXTUAL_TYPE, NAIContextualInterface ))

typedef struct NAIContextual                 NAIContextual;

typedef struct NAIContextualInterfacePrivate NAIContextualInterfacePrivate;

typedef struct {
	GTypeInterface                 parent;
	NAIContextualInterfacePrivate *private;

	/**
	 * is_candidate:
	 * @object: this #NAIContextual object.
	 * @target: the initial target which triggered this function's stack.
	 *  This target is defined in na-object-item.h.
	 * @selection: the current selection as a #GList of #NautilusFileInfo.
	 *
	 * Returns: %TRUE if the @object may be a potential candidate, %FALSE
	 * else.
	 *
	 * The #NAIContextual implementor may take advantage of this
	 * virtual function to check for its own specific data. Only if the
	 * implementor does return %TRUE (or just doesn't implement this
	 * virtual), the conditions themselves will be checked.
	 */
	gboolean ( *is_candidate )( NAIContextual *object, guint target, GList *selection );
}
	NAIContextualInterface;

GType    na_icontextual_get_type( void );

gboolean na_icontextual_is_candidate( const NAIContextual *object, guint target, GList *selection );
gboolean na_icontextual_is_valid    ( const NAIContextual *object );

void     na_icontextual_set_scheme    ( NAIContextual *object, const gchar *scheme, gboolean selected );
void     na_icontextual_replace_folder( NAIContextual *object, const gchar *old, const gchar *new );

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_API_NA_ICONTEXTUAL_H__ */
