/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2012 Pierre Wieser and others (see AUTHORS)
 *
 * Nautilus-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General  Public  License  as
 * published by the Free Software Foundation; either  version  2  of
 * the License, or (at your option) any later version.
 *
 * Nautilus-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even  the  implied  warranty  of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See  the  GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public  License
 * along with Nautilus-Actions; see the file  COPYING.  If  not,  see
 * <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Frederic Ruaudel <grumz@grumz.net>
 *   Rodrigo Moya <rodrigo@gnome-db.org>
 *   Pierre Wieser <pwieser@trychlos.org>
 *   ... and many others (see AUTHORS)
 */

#ifndef __NACT_IEXECUTION_TAB_H__
#define __NACT_IEXECUTION_TAB_H__

/**
 * SECTION: nact_iexecution_tab
 * @short_description: #NactIExecutionTab interface declaration.
 * @include: nact/nact-iexecution-tab.h
 *
 * This interface implements all the widgets which define the
 * actual action to be executed.
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define NACT_TYPE_IEXECUTION_TAB                      ( nact_iexecution_tab_get_type())
#define NACT_IEXECUTION_TAB( instance )               ( G_TYPE_CHECK_INSTANCE_CAST( instance, NACT_TYPE_IEXECUTION_TAB, NactIExecutionTab ))
#define NACT_IS_IEXECUTION_TAB( instance )            ( G_TYPE_CHECK_INSTANCE_TYPE( instance, NACT_TYPE_IEXECUTION_TAB ))
#define NACT_IEXECUTION_TAB_GET_INTERFACE( instance ) ( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NACT_TYPE_IEXECUTION_TAB, NactIExecutionTabInterface ))

typedef struct _NactIExecutionTab                     NactIExecutionTab;
typedef struct _NactIExecutionTabInterfacePrivate     NactIExecutionTabInterfacePrivate;

typedef struct {
	/*< private >*/
	GTypeInterface                     parent;
	NactIExecutionTabInterfacePrivate *private;
}
	NactIExecutionTabInterface;

GType nact_iexecution_tab_get_type( void );

void  nact_iexecution_tab_init    ( NactIExecutionTab *instance );

G_END_DECLS

#endif /* __NACT_IEXECUTION_TAB_H__ */
