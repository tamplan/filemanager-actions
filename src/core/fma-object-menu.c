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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <string.h>

#include <api/fma-iio-provider.h>
#include <api/fma-ifactory-object.h>
#include <api/fma-object-api.h>

#include "na-factory-provider.h"
#include "fma-factory-object.h"

/* private class data
 */
struct _FMAObjectMenuClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _FMAObjectMenuPrivate {
	gboolean dispose_has_run;
};

/* i18n: default label for a new menu */
#define NEW_NAUTILUS_MENU				N_( "New Nautilus menu" )

extern FMADataGroup menu_data_groups [];			/* defined in na-item-menu-factory.c */

static FMAObjectItemClass *st_parent_class = NULL;

static GType        register_type( void );
static void         class_init( FMAObjectMenuClass *klass );
static void         instance_init( GTypeInstance *instance, gpointer klass );
static void         instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void         instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void         instance_dispose( GObject *object );
static void         instance_finalize( GObject *object );

static void         object_dump( const FMAObject *object );

static void         ifactory_object_iface_init( FMAIFactoryObjectInterface *iface, void *user_data );
static guint        ifactory_object_get_version( const FMAIFactoryObject *instance );
static FMADataGroup *ifactory_object_get_groups( const FMAIFactoryObject *instance );
static void         ifactory_object_read_done( FMAIFactoryObject *instance, const FMAIFactoryProvider *reader, void *reader_data, GSList **messages );
static guint        ifactory_object_write_start( FMAIFactoryObject *instance, const FMAIFactoryProvider *writer, void *writer_data, GSList **messages );
static guint        ifactory_object_write_done( FMAIFactoryObject *instance, const FMAIFactoryProvider *writer, void *writer_data, GSList **messages );

static void         icontext_iface_init( FMAIContextInterface *iface, void *user_data );
static gboolean     icontext_is_candidate( FMAIContext *object, guint target, GList *selection );

GType
fma_object_menu_get_type( void )
{
	static GType menu_type = 0;

	if( menu_type == 0 ){

		menu_type = register_type();
	}

	return( menu_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "fma_object_menu_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( FMAObjectMenuClass ),
		NULL,
		NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( FMAObjectMenu ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	static const GInterfaceInfo icontext_iface_info = {
		( GInterfaceInitFunc ) icontext_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo ifactory_object_iface_info = {
		( GInterfaceInitFunc ) ifactory_object_iface_init,
		NULL,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( FMA_TYPE_OBJECT_ITEM, "FMAObjectMenu", &info, 0 );

	g_type_add_interface_static( type, FMA_TYPE_ICONTEXT, &icontext_iface_info );

	g_type_add_interface_static( type, FMA_TYPE_IFACTORY_OBJECT, &ifactory_object_iface_info );

	return( type );
}

static void
class_init( FMAObjectMenuClass *klass )
{
	static const gchar *thisfn = "fma_object_menu_class_init";
	GObjectClass *object_class;
	FMAObjectClass *naobject_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->set_property = instance_set_property;
	object_class->get_property = instance_get_property;
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	naobject_class = FMA_OBJECT_CLASS( klass );
	naobject_class->dump = object_dump;

	klass->private = g_new0( FMAObjectMenuClassPrivate, 1 );

	fma_factory_object_define_properties( object_class, menu_data_groups );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "fma_object_menu_instance_init";
	FMAObjectMenu *self;

	g_return_if_fail( FMA_IS_OBJECT_MENU( instance ));

	self = FMA_OBJECT_MENU( instance );

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self->private = g_new0( FMAObjectMenuPrivate, 1 );
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	g_return_if_fail( FMA_IS_OBJECT_MENU( object ));
	g_return_if_fail( FMA_IS_IFACTORY_OBJECT( object ));

	if( !FMA_OBJECT_MENU( object )->private->dispose_has_run ){

		fma_factory_object_get_as_value( FMA_IFACTORY_OBJECT( object ), g_quark_to_string( property_id ), value );
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	g_return_if_fail( FMA_IS_OBJECT_MENU( object ));
	g_return_if_fail( FMA_IS_IFACTORY_OBJECT( object ));

	if( !FMA_OBJECT_MENU( object )->private->dispose_has_run ){

		fma_factory_object_set_from_value( FMA_IFACTORY_OBJECT( object ), g_quark_to_string( property_id ), value );
	}
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "fma_object_menu_instance_dispose";
	FMAObjectMenu *self;

	g_return_if_fail( FMA_IS_OBJECT_MENU( object ));

	self = FMA_OBJECT_MENU( object );

	if( !self->private->dispose_has_run ){

		g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

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
	static const gchar *thisfn = "fma_object_menu_instance_finalize";
	FMAObjectMenu *self;

	g_return_if_fail( FMA_IS_OBJECT_MENU( object ));

	self = FMA_OBJECT_MENU( object );

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

static void
object_dump( const FMAObject *object )
{
	static const char *thisfn = "fma_object_menu_object_dump";
	FMAObjectMenu *self;

	g_return_if_fail( FMA_IS_OBJECT_MENU( object ));

	self = FMA_OBJECT_MENU( object );

	if( !self->private->dispose_has_run ){
		g_debug( "%s: object=%p (%s, ref_count=%d)", thisfn,
				( void * ) object, G_OBJECT_TYPE_NAME( object ), G_OBJECT( object )->ref_count );

		/* chain up to the parent class */
		if( FMA_OBJECT_CLASS( st_parent_class )->dump ){
			FMA_OBJECT_CLASS( st_parent_class )->dump( object );
		}

		g_debug( "+- end of dump" );
	}
}

static void
ifactory_object_iface_init( FMAIFactoryObjectInterface *iface, void *user_data )
{
	static const gchar *thisfn = "fma_object_menu_ifactory_object_iface_init";

	g_debug( "%s: iface=%p, user_data=%p", thisfn, ( void * ) iface, ( void * ) user_data );

	iface->get_version = ifactory_object_get_version;
	iface->get_groups = ifactory_object_get_groups;
	iface->read_done = ifactory_object_read_done;
	iface->write_start = ifactory_object_write_start;
	iface->write_done = ifactory_object_write_done;
}

static guint
ifactory_object_get_version( const FMAIFactoryObject *instance )
{
	return( 1 );
}

static FMADataGroup *
ifactory_object_get_groups( const FMAIFactoryObject *instance )
{
	return( menu_data_groups );
}

static void
ifactory_object_read_done( FMAIFactoryObject *instance, const FMAIFactoryProvider *reader, void *reader_data, GSList **messages )
{
	g_debug( "fma_object_menu_ifactory_object_read_done: instance=%p", ( void * ) instance );

	fma_object_item_deals_with_version( FMA_OBJECT_ITEM( instance ));

	/* prepare the context after the reading
	 */
	fma_icontext_read_done( FMA_ICONTEXT( instance ));

	/* last, set menu defaults
	 */
	fma_factory_object_set_defaults( instance );
}

static guint
ifactory_object_write_start( FMAIFactoryObject *instance, const FMAIFactoryProvider *writer, void *writer_data, GSList **messages )
{
	fma_object_item_rebuild_children_slist( FMA_OBJECT_ITEM( instance ));

	return( FMA_IIO_PROVIDER_CODE_OK );
}

static guint
ifactory_object_write_done( FMAIFactoryObject *instance, const FMAIFactoryProvider *writer, void *writer_data, GSList **messages )
{
	return( FMA_IIO_PROVIDER_CODE_OK );
}

static void
icontext_iface_init( FMAIContextInterface *iface, void *user_data )
{
	static const gchar *thisfn = "fma_object_menu_icontext_iface_init";

	g_debug( "%s: iface=%p, user_data=%p", thisfn, ( void * ) iface, ( void * ) user_data );

	iface->is_candidate = icontext_is_candidate;
}

static gboolean
icontext_is_candidate( FMAIContext *object, guint target, GList *selection )
{
	return( TRUE );
}

/**
 * fma_object_menu_new:
 *
 * Allocates a new #FMAObjectMenu object.
 *
 * Returns: the newly allocated #FMAObjectMenu object.
 *
 * Since: 2.30
 */
FMAObjectMenu *
fma_object_menu_new( void )
{
	FMAObjectMenu *menu;

	menu = g_object_new( FMA_TYPE_OBJECT_MENU, NULL );

	return( menu );
}

/**
 * fma_object_menu_new_with_defaults:
 *
 * Allocates a new #FMAObjectMenu object, and setup default values.
 *
 * Returns: the newly allocated #FMAObjectMenu object.
 *
 * Since: 2.30
 */
FMAObjectMenu *
fma_object_menu_new_with_defaults( void )
{
	FMAObjectMenu *menu = fma_object_menu_new();
	fma_object_set_new_id( menu, NULL );
	fma_object_set_label( menu, gettext( NEW_NAUTILUS_MENU ));
	fma_factory_object_set_defaults( FMA_IFACTORY_OBJECT( menu ));

	return( menu );
}
