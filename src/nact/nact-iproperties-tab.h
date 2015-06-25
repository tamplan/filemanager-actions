/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
 *
 * Nautilus-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Nautilus-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nautilus-Actions; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Frederic Ruaudel <grumz@grumz.net>
 *   Rodrigo Moya <rodrigo@gnome-db.org>
 *   Pierre Wieser <pwieser@trychlos.org>
 *   ... and many others (see AUTHORS)
 */

#ifndef __NACT_IPROPERTIES_TAB_H__
#define __NACT_IPROPERTIES_TAB_H__

/**
 * SECTION: nact_iproperties_tab
 * @short_description: #NactIPropertiesTab interface definition.
 * @include: nact/nact-iproperties-tab.h
 *
 * This interface implements the "Properties" tab of the notebook.
 *
 * Entry fields are enabled, as soon as an edited item has been set a a
 * property of the main window,
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define NACT_TYPE_IPROPERTIES_TAB                      ( nact_iproperties_tab_get_type())
#define NACT_IPROPERTIES_TAB( instance )               ( G_TYPE_CHECK_INSTANCE_CAST( instance, NACT_TYPE_IPROPERTIES_TAB, NactIPropertiesTab ))
#define NACT_IS_IPROPERTIES_TAB( instance )            ( G_TYPE_CHECK_INSTANCE_TYPE( instance, NACT_TYPE_IPROPERTIES_TAB ))
#define NACT_IPROPERTIES_TAB_GET_INTERFACE( instance ) ( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NACT_TYPE_IPROPERTIES_TAB, NactIPropertiesTabInterface ))

typedef struct _NactIPropertiesTab                     NactIPropertiesTab;
typedef struct _NactIPropertiesTabInterfacePrivate     NactIPropertiesTabInterfacePrivate;

typedef struct {
	/*< private >*/
	GTypeInterface                      parent;
	NactIPropertiesTabInterfacePrivate *private;
}
	NactIPropertiesTabInterface;

GType nact_iproperties_tab_get_type( void );

void  nact_iproperties_tab_init    ( NactIPropertiesTab *instance );

G_END_DECLS

#endif /* __NACT_IPROPERTIES_TAB_H__ */
