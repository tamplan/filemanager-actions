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

#ifndef __UI_FMA_IMIMETYPES_TAB_H__
#define __UI_FMA_IMIMETYPES_TAB_H__

/**
 * SECTION: fma_imimetypes_tab
 * @short_description: #FMAIMimetypesTab interface declaration.
 * @include: ui/fma-imimetypes-tab.h
 *
 * This interface implements all the widgets which define the
 * mimetypes-based conditions.
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define FMA_TYPE_IMIMETYPES_TAB                      ( fma_imimetypes_tab_get_type())
#define FMA_IMIMETYPES_TAB( instance )               ( G_TYPE_CHECK_INSTANCE_CAST( instance, FMA_TYPE_IMIMETYPES_TAB, FMAIMimetypesTab ))
#define FMA_IS_IMIMETYPES_TAB( instance )            ( G_TYPE_CHECK_INSTANCE_TYPE( instance, FMA_TYPE_IMIMETYPES_TAB ))
#define FMA_IMIMETYPES_TAB_GET_INTERFACE( instance ) ( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), FMA_TYPE_IMIMETYPES_TAB, FMAIMimetypesTabInterface ))

typedef struct _FMAIMimetypesTab                     FMAIMimetypesTab;
typedef struct _FMAIMimetypesTabInterfacePrivate     FMAIMimetypesTabInterfacePrivate;

typedef struct {
	/*< private >*/
	GTypeInterface                    parent;
	FMAIMimetypesTabInterfacePrivate *private;
}
	FMAIMimetypesTabInterface;

GType fma_imimetypes_tab_get_type( void );

void  fma_imimetypes_tab_init    ( FMAIMimetypesTab *instance );

G_END_DECLS

#endif /* __UI_FMA_IMIMETYPES_TAB_H__ */
