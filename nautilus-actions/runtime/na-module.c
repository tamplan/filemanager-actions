/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009 Pierre Wieser and others (see AUTHORS)
 *
 * This Program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This Program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this Library; see the file COPYING.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors:
 *   Frederic Ruaudel <grumz@grumz.net>
 *   Rodrigo Moya <rodrigo@gnome-db.org>
 *   Pierre Wieser <pwieser@trychlos.org>
 *   ... and many others (see AUTHORS)
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gmodule.h>

#include "na-module.h"

/* private class data
 */
struct NAModuleClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NAModulePrivate {
	gboolean  dispose_has_run;
	gchar    *path;
	GModule  *library;
	GList    *objects;

	/* api
	 */
	gboolean      ( * initialize )( GTypeModule *module );
	gint          ( * list_types )( const GType **types );
	const gchar * ( * get_name )  ( GType type );
	void          ( * shutdown )  ( void );
};

static GTypeModuleClass *st_parent_class = NULL;

static GType     register_type( void );
static void      class_init( NAModuleClass *klass );
static void      instance_init( GTypeInstance *instance, gpointer klass );
static void      instance_dispose( GObject *object );
static void      instance_finalize( GObject *object );

static gboolean  module_load( GTypeModule *gmodule );
static gboolean  module_load_check( NAModule *module, const gchar *symbol, gpointer *pfn );
static void      module_unload( GTypeModule *gmodule );

static void      add_module_objects( NAModule *module );
static void      add_module_type( NAModule *module, GType type );
static NAModule *module_new( const gchar *filename );
static void      module_object_weak_notify( NAModule *module, GObject *object );
static void      set_name( NAModule *module );

GType
na_module_get_type( void )
{
	static GType object_type = 0;

	if( !object_type ){
		object_type = register_type();
	}

	return( object_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "na_module_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NAModuleClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NAModule ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_TYPE_MODULE, "NAModule", &info, 0 );

	return( type );
}

static void
class_init( NAModuleClass *klass )
{
	static const gchar *thisfn = "na_module_class_init";
	GObjectClass *object_class;
	GTypeModuleClass *module_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	module_class = G_TYPE_MODULE_CLASS( klass );
	module_class->load = module_load;
	module_class->unload = module_unload;

	klass->private = g_new0( NAModuleClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "na_module_instance_init";
	NAModule *self;

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );
	g_return_if_fail( NA_IS_MODULE( instance ));
	self = NA_MODULE( instance );

	self->private = g_new0( NAModulePrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "na_module_instance_dispose";
	NAModule *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_return_if_fail( NA_IS_MODULE( object ));
	self = NA_MODULE( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "na_module_instance_finalize";
	NAModule *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_return_if_fail( NA_IS_MODULE( object ));
	self = NA_MODULE( object );

	g_free( self->private->path );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/*
 * triggered by GTypeModule base class when first loading the library
 * which is itself triggered by module_new:g_type_module_use()
 */
static gboolean
module_load( GTypeModule *gmodule )
{
	static const gchar *thisfn = "na_module_module_load";
	NAModule *module;
	gboolean loaded;

	g_debug( "%s: gmodule=%p", thisfn, ( void * ) gmodule );
	g_return_val_if_fail( G_IS_TYPE_MODULE( gmodule ), FALSE );

	module = NA_MODULE( gmodule );

	module->private->library = g_module_open( module->private->path, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL );

	if( !module->private->library ){
		g_warning( "%s: g_module_open: path=%s, error=%s", thisfn, module->private->path, g_module_error());
		return( FALSE );
	}

	loaded =
		module_load_check( module, "na_api_module_init", ( gpointer * ) &module->private->initialize) &&
		module_load_check( module, "na_api_module_list_types", ( gpointer * ) &module->private->list_types ) &&
		module_load_check( module, "na_api_module_get_name", ( gpointer * ) &module->private->get_name ) &&
		module_load_check( module, "na_api_module_shutdown", ( gpointer * ) &module->private->shutdown ) &&
		module->private->initialize( gmodule );

	return( loaded );
}

static gboolean
module_load_check( NAModule *module, const gchar *symbol, gpointer *pfn )
{
	static const gchar *thisfn = "na_module_module_load_check";
	gboolean ok;

	ok = g_module_symbol( module->private->library, symbol, pfn );
	if( !ok ){
		g_debug("%s: %s: symbol not found in %s", thisfn, symbol, module->private->path );
		g_module_close( module->private->library );
	}

	return( ok );
}

static void
module_unload( GTypeModule *gmodule )
{
	static const gchar *thisfn = "na_module_module_unload";
	NAModule *module;

	g_debug( "%s: gmodule=%p", thisfn, ( void * ) gmodule );
	g_return_if_fail( G_IS_TYPE_MODULE( gmodule ));

	module = NA_MODULE( gmodule );

	module->private->shutdown();

	g_module_close( module->private->library );

	module->private->initialize = NULL;
	module->private->list_types = NULL;
	module->private->get_name = NULL;
	module->private->shutdown = NULL;
}

/**
 * na_modules_load_modules:
 *
 * Load availables dynamic libraries.
 *
 * Returns: a #GList of #NAModule, each object representing a dynamically
 * loaded library.
 */
GList *
na_modules_load_modules( void )
{
	static const gchar *thisfn = "na_modules_load_modules";
	const gchar *dirname = PKGLIBDIR;
	GList *modules;
	GDir *api_dir;
	GError *error;
	const gchar *entry;
	gchar *fname;
	NAModule *module;

	g_debug( "%s", thisfn );

	modules = NULL;
	error = NULL;

	api_dir = g_dir_open( dirname, 0, &error );
	if( error ){
		g_warning( "%s: g_dir_open: %s", thisfn, error->message );
		g_error_free( error );
		error = NULL;

	} else {
		while(( entry = g_dir_read_name( api_dir )) != NULL ){
			if( g_str_has_suffix( entry, ".so" )){
				fname = g_build_filename( dirname, entry, NULL );
				module = module_new( fname );
				if( module ){
					modules = g_list_prepend( modules, module );
				}
				g_free( fname );
			}
		}
		g_dir_close( api_dir );
		modules = g_list_reverse( modules );
	}

	return( modules );
}

/**
 * na_module_get_extensions_for_type:
 * @type: the serched GType.
 *
 * Returns: a list of loaded modules willing to deal with requested @type.
 */
GList *
na_module_get_extensions_for_type( GList *modules, GType type )
{
	GList *willing_to, *im;

	willing_to = NULL;

	for( im = modules; im ; im = im->next ){
		if( G_TYPE_CHECK_INSTANCE_TYPE( G_OBJECT( im->data ), type )){
			g_object_ref( im->data );
			willing_to = g_list_prepend( willing_to, im->data );
		}
	}

	willing_to = g_list_reverse( willing_to );

	return( willing_to );
}

/**
 * na_module_get_name:
 * @module: the #NAModule instance corresponding to a dynamically
 *  loaded library.
 * @type: one the #GType this @module advertizes it implements.
 *
 * Returns: the name the #NAModule @module applies to itself for this
 * @type, as a newly allocated string which should be g_free() by the
 * caller.
 */
gchar *
na_module_get_name( NAModule *module, GType type )
{
	gchar *name = NULL;

	g_return_val_if_fail( NA_IS_MODULE( module ), name );

	name = g_strdup( module->private->get_name( type ));

	return( name );
}

/**
 * na_module_release_modules:
 * @modules: the list of loaded modules.
 *
 * Release resources allocated to the loaded modules.
 */
void
na_module_release_modules( GList *modules )
{
	GList *im;

	for( im = modules ; im ; im = im->next ){
		g_object_unref( NA_MODULE( im->data ));
	}

	g_list_free( modules );
}

static void
add_module_objects( NAModule *module )
{
	const GType *types;
	gint count, i;

	count = module->private->list_types( &types );
	module->private->objects = NULL;

	for( i = 0 ; i < count ; i++ ){
		if( types[i] ){
			add_module_type( module, types[i] );
		}
	}

	module->private->objects = g_list_reverse( module->private->objects );
}

static void
add_module_type( NAModule *module, GType type )
{
	GObject *object;

	object = g_object_new( type, NULL );

	g_object_weak_ref( object,
			( GWeakNotify ) module_object_weak_notify,
			module );

	module->private->objects = g_list_prepend( module->private->objects, object );
}

static NAModule *
module_new( const gchar *fname )
{
	NAModule *module;

	module = g_object_new( NA_MODULE_TYPE, NULL );
	module->private->path = g_strdup( fname );

	if( g_type_module_use( G_TYPE_MODULE( module ))){
		add_module_objects( module );
		set_name( module );
		g_type_module_unuse( G_TYPE_MODULE( module ));

	} else {
		g_object_unref( module );
		module = NULL;
	}

	return( module );
}

static void
module_object_weak_notify( NAModule *module, GObject *object )
{
	module->private->objects = g_list_remove( module->private->objects, object );
}

static void
set_name( NAModule *module )
{
	/* do we should set internal GTypeModule name here ? */
}
