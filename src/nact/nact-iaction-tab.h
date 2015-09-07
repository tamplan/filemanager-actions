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

#ifndef __NACT_IACTION_TAB_H__
#define __NACT_IACTION_TAB_H__

/**
 * SECTION: nact_iaction_tab
 * @short_description: #NactIActionTab interface definition.
 * @include: nact/nact-iaction-tab.h
 *
 * This interface implements the "Nautilus Menu Item" tab of the notebook.
 *
 * Entry fields are enabled, as soon as an edited item has been set as a
 * property of the main window, Fields are those of NAObjectItem (i.e.
 * not NAObjectProfile, not NAIContext, but may be NAObjectAction specific).
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define NACT_TYPE_IACTION_TAB                      ( nact_iaction_tab_get_type())
#define NACT_IACTION_TAB( instance )               ( G_TYPE_CHECK_INSTANCE_CAST( instance, NACT_TYPE_IACTION_TAB, NactIActionTab ))
#define NACT_IS_IACTION_TAB( instance )            ( G_TYPE_CHECK_INSTANCE_TYPE( instance, NACT_TYPE_IACTION_TAB ))
#define NACT_IACTION_TAB_GET_INTERFACE( instance ) ( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NACT_TYPE_IACTION_TAB, NactIActionTabInterface ))

typedef struct _NactIActionTab                     NactIActionTab;
typedef struct _NactIActionTabInterfacePrivate     NactIActionTabInterfacePrivate;

typedef struct {
	/*< private >*/
	GTypeInterface                  parent;
	NactIActionTabInterfacePrivate *private;
}
	NactIActionTabInterface;

GType    nact_iaction_tab_get_type ( void );

void     nact_iaction_tab_init     ( NactIActionTab *instance );

gboolean nact_iaction_tab_has_label( NactIActionTab *instance );

G_END_DECLS

#endif /* __NACT_IACTION_TAB_H__ */
