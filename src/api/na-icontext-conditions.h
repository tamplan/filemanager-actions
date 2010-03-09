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

#ifndef __NAUTILUS_ACTIONS_API_NA_ICONTEXT_CONDITIONS_H__
#define __NAUTILUS_ACTIONS_API_NA_ICONTEXT_CONDITIONS_H__

/**
 * SECTION: na_icontext_conditions
 * @short_description: #NAIContextConditions interface definition.
 * @include: nautilus-actions/na-icontext_conditions.h
 *
 * This interface is implemented by all #NAObject-derived objects
 * which must met some conditions in order to be displayed in the
 * Nautilus context menu.
 *
 * Implementors, typically actions, profiles and menus, host the required
 * data as #NADataBoxed in a dedicated NA_FACTORY_CONDITIONS_GROUP
 * data group.
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define NA_ICONTEXT_CONDITIONS_TYPE							( na_icontext_conditions_get_type())
#define NA_ICONTEXT_CONDITIONS( instance )					( G_TYPE_CHECK_INSTANCE_CAST( instance, NA_ICONTEXT_CONDITIONS_TYPE, NAIContextConditions ))
#define NA_IS_ICONTEXT_CONDITIONS( instance )				( G_TYPE_CHECK_INSTANCE_TYPE( instance, NA_ICONTEXT_CONDITIONS_TYPE ))
#define NA_ICONTEXT_CONDITIONS_GET_INTERFACE( instance )	( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NA_ICONTEXT_CONDITIONS_TYPE, NAIContextConditionsInterface ))

typedef struct NAIContextConditions                 NAIContextConditions;

typedef struct NAIContextConditionsInterfacePrivate NAIContextConditionsInterfacePrivate;

typedef struct {
	GTypeInterface                        parent;
	NAIContextConditionsInterfacePrivate *private;

	/**
	 * is_candidate:
	 * @object: this #NAIContextConditions object.
	 * @target: the initial target which triggered this function's stack.
	 *  This target is defined in na-object-item.h.
	 * @selection: the current selection as a #GList of #NautilusFileInfo.
	 *
	 * Returns: %TRUE if the @object may be a potential candidate, %FALSE
	 * else.
	 *
	 * The #NAIContextConditions implementor may take advantage of this
	 * virtual function to check for its own specific data. Only if the
	 * implementor does return %TRUE (or just doesn't implement this
	 * virtual), the conditions themselves will be checked.
	 */
	gboolean ( *is_candidate )( NAIContextConditions *object, guint target, GList *selection );
}
	NAIContextConditionsInterface;

GType    na_icontext_conditions_get_type( void );

gboolean na_icontext_conditions_is_candidate( const NAIContextConditions *object, guint target, GList *selection );
gboolean na_icontext_conditions_is_valid    ( const NAIContextConditions *object );

void     na_icontext_conditions_set_scheme    ( NAIContextConditions *object, const gchar *scheme, gboolean selected );
void     na_icontext_conditions_replace_folder( NAIContextConditions *object, const gchar *old, const gchar *new );

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_API_NA_ICONTEXT_CONDITIONS_H__ */
