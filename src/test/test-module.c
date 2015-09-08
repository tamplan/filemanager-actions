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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* this is a module to test how to use the GLib dynamic-loading functions
 * it tries to load, then unload, a test-module-plugin.so plugin
 *
 * more specifically, I am interested on releasing allocated resources
 */

#include <stdlib.h>

#include <glib-object.h>
#include <gmodule.h>

#define PLUGIN_NAME								"test_module_plugin"


#if 0
/* this is first version
 */
static GModule *load_plugin( void );
static void     call_plugin_fn( GModule *module );
static void     unload_plugin( GModule *module );

int
main( int argc, char **argv )
{
	GModule *module;

#if !GLIB_CHECK_VERSION( 2,36, 0 )
	g_type_init();
#endif

	/* dynamically load the module */
	module = load_plugin();

	if( module ){
		/* call a function in the module */
		call_plugin_fn( module );

		/* unload the module */
		unload_plugin( module );
	}

	return( 0 );
}

static GModule *
load_plugin( void )
{
	gchar *module_path;
	GModule *module;

	module = NULL;

	if( g_module_supported()){

		module_path = g_module_build_path( PKGLIBDIR, PLUGIN_NAME );
		module = g_module_open( module_path, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL );
		if( !module ){
			g_printerr( "%s: %s\n", module_path, g_module_error());
		}
		g_free( module_path );
	}

	return( module );
}

static void
call_plugin_fn( GModule *module )
{
	typedef void ( *PluginFn )( GModule *module );
	PluginFn plugin_fn;

	if( !g_module_symbol( module, "say_hello", ( gpointer * ) &plugin_fn )){
		g_printerr( "%s\n", g_module_error());
		return;
	}
	if( !plugin_fn ){
		g_printerr( "%s\n", g_module_error());
		return;
	}
	plugin_fn( module );
}

static void
unload_plugin( GModule *module )
{
	if( !g_module_close( module )){
		g_printerr( "%s\n", g_module_error());
	}
}
#endif

/* version 2
 * Having the plugin register dynamic GTypes require a GTypeModule
 * This GTtypeModule is provided by the program as a GTypeModule-derived object
 * FMAModule embeds the GModule
 *
 * Result:
 *
 * - the main program (the loader) should define a GTypeModule derived class
 *
 * - the GTypeModule derived class (here FMAModule) embeds a GModule pointer
 *
 * - when loading plugins:
 *
 *   > allocate a new GTypeModule derived object for each module
 *     setup the path here
 *
 *   > g_type_module_use() it
 *     this triggers the on_load() virtual method on GTypeModule derived class
 *       in on_load_derived(), g_module_open() the plugin and check the API
 *
 *       on_load_derived() may return FALSE
 *       while dynamic types have not been registered, we are always safe to unref
 *       the allocated GTypeModule derived object
 *       setup the GModule pointer on the loaded library
 *
 *     so, if g_module_use() returns FALSE, just unref the object
 *
 *     so, g_module_use() cannot be called from instance_init or
 *     instance_constructed (which have no return value)
 *
 * At the end, it is impossible to release the GTypeModule objects.
 * But we can safely unuse their loaded libraries.
 *
 * The main program does not known which GType or GInterface the plugin
 * declares.
 * Nautilus defines a get_types() API, and then allocates an object for each
 * returned type. It is then easy to check if the object implements a given
 * interface.
 * Nautilus never release these objects.
 * We may also ask the plugin to just allocate itself its own management object,
 * returning it to the program (returning a pointer is possible because we are
 * in the same process).
 */

#define FMA_TYPE_MODULE                  ( fma_module_get_type())
#define FMA_MODULE( object )             ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_MODULE, FMAModule ))
#define FMA_MODULE_CLASS( klass )        ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_MODULE, FMAModuleClass ))
#define FMA_IS_MODULE( object )          ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_MODULE ))
#define FMA_IS_MODULE_CLASS( klass )     ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_MODULE ))
#define FMA_MODULE_GET_CLASS( object )   ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_MODULE, FMAModuleClass ))

typedef struct _FMAModulePrivate         FMAModulePrivate;
typedef struct _FMAModuleClassPrivate    FMAModuleClassPrivate;

typedef struct {
	/*< private >*/
	GTypeModule      parent;
	FMAModulePrivate *private;
}
	FMAModule;

typedef struct {
	/*< private >*/
	GTypeModuleClass      parent;
	FMAModuleClassPrivate *private;
}
	FMAModuleClass;

GType    fma_module_get_type               ( void );

/* private class data
 */
struct _FMAModuleClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _FMAModulePrivate {
	gboolean  dispose_has_run;
	GModule  *plugin;
};

static GTypeModuleClass *st_parent_class = NULL;

static GType     register_type( void );
static void      class_init( FMAModuleClass *klass );
static void      instance_init( GTypeInstance *instance, gpointer klass );
static void      instance_dispose( GObject *object );
static void      instance_finalize( GObject *object );

static FMAModule *load_plugin( void );
static gboolean  on_module_load( GTypeModule *module );
static void      call_plugin_fn( FMAModule *module );
static void      on_unload_plugin( GTypeModule *module );

GType
fma_module_get_type( void )
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
	static const gchar *thisfn = "fma_module_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( FMAModuleClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( FMAModule ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_TYPE_MODULE, "FMAModule", &info, 0 );

	return( type );
}

static void
class_init( FMAModuleClass *klass )
{
	static const gchar *thisfn = "fma_module_class_init";
	GObjectClass *object_class;
	GTypeModuleClass *module_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	module_class = G_TYPE_MODULE_CLASS( klass );
	module_class->load = on_module_load;
	module_class->unload = on_unload_plugin;

	klass->private = g_new0( FMAModuleClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "fma_module_instance_init";
	FMAModule *self;

	g_return_if_fail( FMA_IS_MODULE( instance ));

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self = FMA_MODULE( instance );

	self->private = g_new0( FMAModulePrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "fma_module_instance_dispose";
	FMAModule *self;

	g_return_if_fail( FMA_IS_MODULE( object ));

	self = FMA_MODULE( object );

	if( !self->private->dispose_has_run ){

		g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

		self->private->dispose_has_run = TRUE;

		/* should trigger unload of the plugin */
		/* refused by GLib
		 * GLib-GObject-WARNING **: gtypemodule.c:111: unsolicitated invocation of g_object_dispose() on GTypeModule
		 */
		/*g_type_module_unuse( G_TYPE_MODULE( self ));*/

		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "fma_module_instance_finalize";
	FMAModule *self;

	g_return_if_fail( FMA_IS_MODULE( object ));

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	self = FMA_MODULE( object );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

int
main( int argc, char **argv )
{
	FMAModule *module;

#if !GLIB_CHECK_VERSION( 2,36, 0 )
	g_type_init();
#endif

	/* dynamically load the module */
	module = load_plugin();

	if( module ){
		/* call a function in the module */
		call_plugin_fn( module );

		/* try to just unref the FMAModule */
		/* not ok */
		/*g_object_unref( module );*/

		/* try to unload the plugin */
		g_type_module_unuse( G_TYPE_MODULE( module ));
		/* then unref the object */
		/* not ok
		 * it happens that releasing the GTypeModule after having registered a GType or a
		 * GInterface is impossible
		 * see http://git.gnome.org/browse/glib/tree/gobject/gtypemodule.c
		 * and http://library.gnome.org/devel/gobject/unstable/GTypeModule.html#g-type-module-unuse
		 */
		/*g_object_unref( module );*/
	}

	return( 0 );
}

static FMAModule *
load_plugin( void )
{
	FMAModule *module;

	module = NULL;

	if( g_module_supported()){

		module = g_object_new( FMA_TYPE_MODULE, NULL );
		g_debug( "test_module_load_plugin: module=%p", ( void * ) module );

		if( !g_type_module_use( G_TYPE_MODULE( module ))){
			g_object_unref( module );
		}
	}

	return( module );
}

static gboolean
on_module_load( GTypeModule *module )
{
	gboolean ok;
	gchar *module_path;
	FMAModule *fma_module = FMA_MODULE( module );

	g_debug( "test_module_on_module_load" );

	ok = TRUE;
	module_path = g_module_build_path( PKGLIBDIR, PLUGIN_NAME );

	g_debug( "test_module_on_module_load: opening the library" );
	fma_module->private->plugin = g_module_open( module_path, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL );

	if( !fma_module->private->plugin ){
		g_printerr( "%s: %s\n", module_path, g_module_error());
		ok = FALSE;
	}

	g_free( module_path );

	return( ok );
}

static void
call_plugin_fn( FMAModule *module )
{
	typedef void ( *PluginInit )( FMAModule *module );
	PluginInit plugin_fn;

	if( !g_module_symbol( module->private->plugin, "plugin_init", ( gpointer * ) &plugin_fn )){
		g_printerr( "%s\n", g_module_error());
		return;
	}
	if( !plugin_fn ){
		g_printerr( "%s\n", g_module_error());
		return;
	}
	plugin_fn( module );
}

static void
on_unload_plugin( GTypeModule *module )
{
	g_debug( "test_module_on_unload_plugin" );
	if( !g_module_close( FMA_MODULE( module )->private->plugin )){
		g_printerr( "%s\n", g_module_error());
	}
}
