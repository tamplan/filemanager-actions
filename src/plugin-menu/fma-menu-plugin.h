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

#ifndef __PLUGIN_MENU_FMA_MENU_PLUGIN_H__
#define __PLUGIN_MENU_FMA_MENU_PLUGIN_H__

/**
 * SECTION: filemanager-actions
 * @title: FMAMenuPlugin
 * @short_description: The FMAMenuPlugin plugin class definition
 * @include: plugin-menu/fma-menu-plugin.h
 *
 * This is the class which handles the file manager menu plugin.
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define FMA_MENU_PLUGIN_TYPE                ( fma_menu_plugin_get_type())
#define FMA_MENU_PLUGIN( object )           ( G_TYPE_CHECK_INSTANCE_CAST(( object ), FMA_MENU_PLUGIN_TYPE, FMAMenuPlugin ))
#define FMA_MENU_PLUGIN_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST(( klass ), FMA_MENU_PLUGIN_TYPE, FMAMenuPluginClass ))
#define FMA_IS_MENU_PLUGIN( object )        ( G_TYPE_CHECK_INSTANCE_TYPE(( object ), FMA_MENU_PLUGIN_TYPE ))
#define FMA_IS_MENU_PLUGIN_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_MENU_PLUGIN_TYPE ))
#define FMA_MENU_PLUGIN_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_MENU_PLUGIN_TYPE, FMAMenuPluginClass ))

typedef struct _FMAMenuPluginPrivate        FMAMenuPluginPrivate;

typedef struct {
	/*< private >*/
	GObject               parent;
	FMAMenuPluginPrivate *private;
}
	FMAMenuPlugin;

typedef struct _FMAMenuPluginClassPrivate   FMAMenuPluginClassPrivate;

typedef struct {
	/*< private >*/
	GObjectClass               parent;
	FMAMenuPluginClassPrivate *private;
}
	FMAMenuPluginClass;

GType    fma_menu_plugin_get_type     ( void );
void     fma_menu_plugin_register_type( GTypeModule *module );

G_END_DECLS

#endif /* __PLUGIN_MENU_FMA_MENU_PLUGIN_H__ */
