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

#ifndef __FILEMANAGER_ACTIONS_API_OBJECT_MENU_H__
#define __FILEMANAGER_ACTIONS_API_OBJECT_MENU_H__

/**
 * SECTION: object-menu
 * @title: FMAObjectMenu
 * @short_description: The Menu Class Definition
 * @include: filemanager-actions/fma-object-menu.h
 */

#include "fma-object-item.h"

G_BEGIN_DECLS

#define FMA_TYPE_OBJECT_MENU                ( fma_object_menu_get_type())
#define FMA_OBJECT_MENU( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_OBJECT_MENU, FMAObjectMenu ))
#define FMA_OBJECT_MENU_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_OBJECT_MENU, FMAObjectMenuClass ))
#define FMA_IS_OBJECT_MENU( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_OBJECT_MENU ))
#define FMA_IS_OBJECT_MENU_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_OBJECT_MENU ))
#define FMA_OBJECT_MENU_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_OBJECT_MENU, FMAObjectMenuClass ))

typedef struct _FMAObjectMenuPrivate        FMAObjectMenuPrivate;

typedef struct {
	/*< private >*/
	FMAObjectItem         parent;
	FMAObjectMenuPrivate *private;
}
	FMAObjectMenu;

typedef struct _FMAObjectMenuClassPrivate   FMAObjectMenuClassPrivate;

typedef struct {
	/*< private >*/
	FMAObjectItemClass         parent;
	FMAObjectMenuClassPrivate *private;
}
	FMAObjectMenuClass;

GType          fma_object_menu_get_type         ( void );

FMAObjectMenu *fma_object_menu_new              ( void );
FMAObjectMenu *fma_object_menu_new_with_defaults( void );

G_END_DECLS

#endif /* __FILEMANAGER_ACTIONS_API_OBJECT_MENU_H__ */
