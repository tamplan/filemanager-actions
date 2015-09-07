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

#ifndef __FILE_MANAGER_ACTIONS_API_ICONTEXT_H__
#define __FILE_MANAGER_ACTIONS_API_ICONTEXT_H__

/**
 * SECTION: icontext
 * @title: FMAIContext
 * @short_description: The Contextual Interface
 * @include: file-manager-actions/fma-icontext.h
 *
 * This interface is implemented by all #FMAObject -derived objects
 * whose the display in the Nautilus context menu is subject to some
 * conditions.
 *
 * Implementors, typically #FMAObjectAction, #FMAObjectProfile and
 * #FMAObjectMenu, host the required data as #FMADataBoxed in a dedicated
 * FMA_FACTORY_CONDITIONS_GROUP data group.
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define FMA_TYPE_ICONTEXT                      ( fma_icontext_get_type())
#define FMA_ICONTEXT( instance )               ( G_TYPE_CHECK_INSTANCE_CAST( instance, FMA_TYPE_ICONTEXT, FMAIContext ))
#define FMA_IS_ICONTEXT( instance )            ( G_TYPE_CHECK_INSTANCE_TYPE( instance, FMA_TYPE_ICONTEXT ))
#define FMA_ICONTEXT_GET_INTERFACE( instance ) ( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), FMA_TYPE_ICONTEXT, FMAIContextInterface ))

typedef struct _FMAIContext                    FMAIContext;
typedef struct _FMAIContextInterfacePrivate    FMAIContextInterfacePrivate;

/**
 * FMAIContextInterface:
 * @is_candidate: determines if the given FMAObject-derived object is
 *                candidate to display in Nautilus.
 *
 * This interface manages all conditions relevant to a displayable status
 * in Nautilus.
 */
typedef struct {
	/*< private >*/
	GTypeInterface               parent;
	FMAIContextInterfacePrivate *private;

	/*< public >*/
	/**
	 * is_candidate:
	 * @object: this FMAIContext object.
	 * @target: the initial target which triggered this function's stack.
	 *  This target is defined in fma-object-item.h.
	 * @selection: the current selection as a GList of NautilusFileInfo.
	 *
	 * The FMAIContext implementor may take advantage of this
	 * virtual function to check for its own specific data. Only if the
	 * implementor does return %TRUE (or just doesn't implement this
	 * virtual), the conditions themselves will be checked.
	 *
	 * Returns: %TRUE if the @object may be a potential candidate, %FALSE
	 * else.
	 *
	 * Since: 2.30
	 */
	gboolean ( *is_candidate )( FMAIContext *object, guint target, GList *selection );
}
	FMAIContextInterface;

GType    fma_icontext_get_type( void );

gboolean fma_icontext_are_equal       ( const FMAIContext *a, const FMAIContext *b );
gboolean fma_icontext_is_candidate    ( const FMAIContext *context, guint target, GList *selection );
gboolean fma_icontext_is_valid        ( const FMAIContext *context );

void     fma_icontext_check_mimetypes ( const FMAIContext *context );

void     fma_icontext_copy            ( FMAIContext *context, const FMAIContext *source );
void     fma_icontext_read_done       ( FMAIContext *context );
void     fma_icontext_set_scheme      ( FMAIContext *context, const gchar *scheme, gboolean selected );
void     fma_icontext_set_only_desktop( FMAIContext *context, const gchar *desktop, gboolean selected );
void     fma_icontext_set_not_desktop ( FMAIContext *context, const gchar *desktop, gboolean selected );
void     fma_icontext_replace_folder  ( FMAIContext *context, const gchar *old, const gchar *new );

G_END_DECLS

#endif /* __FILE_MANAGER_ACTIONS_API_ICONTEXT_H__ */
