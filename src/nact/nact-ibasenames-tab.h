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

#ifndef __NACT_IBASENAMES_TAB_H__
#define __NACT_IBASENAMES_TAB_H__

/**
 * SECTION: nact_ibasenames_tab
 * @short_description: #NactIBasenamesTab interface declaration.
 * @include: nact/nact-ibasenames-tab.h
 *
 * This interface implements all the widgets which define the
 * basenames-based conditions.
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define NACT_TYPE_IBASENAMES_TAB                      ( nact_ibasenames_tab_get_type())
#define NACT_IBASENAMES_TAB( instance )               ( G_TYPE_CHECK_INSTANCE_CAST( instance, NACT_TYPE_IBASENAMES_TAB, NactIBasenamesTab ))
#define NACT_IS_IBASENAMES_TAB( instance )            ( G_TYPE_CHECK_INSTANCE_TYPE( instance, NACT_TYPE_IBASENAMES_TAB ))
#define NACT_IBASENAMES_TAB_GET_INTERFACE( instance ) ( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NACT_TYPE_IBASENAMES_TAB, NactIBasenamesTabInterface ))

typedef struct _NactIBasenamesTab                     NactIBasenamesTab;
typedef struct _NactIBasenamesTabInterfacePrivate     NactIBasenamesTabInterfacePrivate;

typedef struct {
	/*< private >*/
	GTypeInterface                     parent;
	NactIBasenamesTabInterfacePrivate *private;
}
	NactIBasenamesTabInterface;

GType nact_ibasenames_tab_get_type( void );

void  nact_ibasenames_tab_init    ( NactIBasenamesTab *instance );

G_END_DECLS

#endif /* __NACT_IBASENAMES_TAB_H__ */
