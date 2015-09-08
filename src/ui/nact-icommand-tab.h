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

#ifndef __UI_NACT_ICOMMAND_TAB_H__
#define __UI_NACT_ICOMMAND_TAB_H__

/**
 * SECTION: nact_icommand_tab
 * @short_description: #NactICommandTab interface declaration.
 * @include: ui/nact-icommand-tab.h
 *
 * This interface implements all the widgets which define the
 * actual action to be executed (from FMAObjectProfile).
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define NACT_TYPE_ICOMMAND_TAB                      ( nact_icommand_tab_get_type())
#define NACT_ICOMMAND_TAB( instance )               ( G_TYPE_CHECK_INSTANCE_CAST( instance, NACT_TYPE_ICOMMAND_TAB, NactICommandTab ))
#define NACT_IS_ICOMMAND_TAB( instance )            ( G_TYPE_CHECK_INSTANCE_TYPE( instance, NACT_TYPE_ICOMMAND_TAB ))
#define NACT_ICOMMAND_TAB_GET_INTERFACE( instance ) ( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NACT_TYPE_ICOMMAND_TAB, NactICommandTabInterface ))

typedef struct _NactICommandTab                     NactICommandTab;
typedef struct _NactICommandTabInterfacePrivate     NactICommandTabInterfacePrivate;

typedef struct {
	/*< private >*/
	GTypeInterface                   parent;
	NactICommandTabInterfacePrivate *private;
}
	NactICommandTabInterface;

GType nact_icommand_tab_get_type( void );

void  nact_icommand_tab_init    ( NactICommandTab *instance );

G_END_DECLS

#endif /* __UI_NACT_ICOMMAND_TAB_H__ */
