/*
 * FileManager-Actions
 * A file-manager extension which offers configurable context menu modules.
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

#ifndef __CORE_FMA_MODULE_H__
#define __CORE_FMA_MODULE_H__

/* @title: FMAModule
 * @short_description: The #FMAModule Class Definition
 * @include: core/fma-module.h
 *
 * The FMAModule class manages FileManager-Actions extensions as dynamically
 * loadable modules (plugins).
 *
 * FMAModule
 *  +- is derived from GTypeModule
 *      +- which itself implements GTypePlugin
 *
 * Each FMAModule physically corresponds to a dynamically loadable library
 * (i.e. a plugin). A FMAModule implements one or more interfaces, and/or
 * provides one or more services.
 *
 * Interfaces (resp. services) are implemented (resp. provided) by GObjects
 * which are dynamically instantiated at plugin initial-load time.
 *
 * So the dynamic is as follows:
 * - FMAPivot scans for the PKGLIBDIR directory, trying to dynamically
 *   load all found libraries
 * - to be considered as a N-A plugin, a library must implement some
 *   functions (see api/na-api.h)
 * - for each found plugin, FMAPivot calls na_api_list_types() which
 *   returns the type of GObjects implemented in the plugin
 * - FMAPivot dynamically instantiates a GObject for each returned GType.
 *
 * After that, when FMAPivot wants to access, say to FMAIIOProvider
 * interfaces, it asks each module for its list of objects which implement
 * this given interface.
 * Interface API is then called against the returned GObject.
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define FMA_TYPE_MODULE                ( fma_module_get_type())
#define FMA_MODULE( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_MODULE, FMAModule ))
#define FMA_MODULE_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_MODULE, FMAModuleClass ))
#define FMA_IS_MODULE( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_MODULE ))
#define FMA_IS_MODULE_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_MODULE ))
#define FMA_MODULE_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_MODULE, FMAModuleClass ))

typedef struct _FMAModulePrivate       FMAModulePrivate;

typedef struct {
	/*< private >*/
	GTypeModule       parent;
	FMAModulePrivate *private;
}
	FMAModule;

typedef struct _FMAModuleClassPrivate  FMAModuleClassPrivate;

typedef struct {
	/*< private >*/
	GTypeModuleClass       parent;
	FMAModuleClassPrivate *private;
}
	FMAModuleClass;

GType    fma_module_get_type               ( void );

void     fma_module_dump                   ( const FMAModule *module );
GList   *fma_module_load_modules           ( void );

GList   *fma_module_get_extensions_for_type( GList *modules, GType type );
void     fma_module_free_extensions_list   ( GList *extensions );

gboolean fma_module_has_id                 ( FMAModule *module, const gchar *id );

void     fma_module_release_modules        ( GList *modules );

G_END_DECLS

#endif /* __CORE_FMA_MODULE_H__ */
