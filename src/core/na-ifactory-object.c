/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010 Pierre Wieser and others (see AUTHORS)
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

#include <string.h>

#include <api/na-ifactory-object.h>

#include "na-factory-object.h"

/* private interface data
 */
struct NAIFactoryObjectInterfacePrivate {
	void *empty;					/* so that gcc -pedantic is happy */
};

gboolean                   ifactory_object_initialized = FALSE;
gboolean                   ifactory_object_finalized   = FALSE;
#if 0
NAIFactoryObjectInterface *ifactory_object_klass       = NULL;
#endif

static GType register_type( void );
static void  interface_base_init( NAIFactoryObjectInterface *klass );
static void  interface_base_finalize( NAIFactoryObjectInterface *klass );

static guint ifactory_object_get_version( const NAIFactoryObject *instance );

/**
 * Registers the GType of this interface.
 */
GType
na_ifactory_object_get_type( void )
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
	static const gchar *thisfn = "na_ifactory_object_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NAIFactoryObjectInterface ),
		( GBaseInitFunc ) interface_base_init,
		( GBaseFinalizeFunc ) interface_base_finalize,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_INTERFACE, "NAIFactoryObject", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( NAIFactoryObjectInterface *klass )
{
	static const gchar *thisfn = "na_ifactory_object_interface_base_init";

	if( !ifactory_object_initialized ){

		g_debug( "%s: klass=%p (%s)", thisfn, ( void * ) klass, G_OBJECT_CLASS_NAME( klass ));

		klass->private = g_new0( NAIFactoryObjectInterfacePrivate, 1 );

		klass->get_version = ifactory_object_get_version;
		klass->get_groups = NULL;
		klass->copy = NULL;
		klass->are_equal = NULL;
		klass->is_valid = NULL;
		klass->read_start = NULL;
		klass->read_done = NULL;
		klass->write_start = NULL;
		klass->write_done = NULL;

#if 0
		ifactory_object_klass = klass;
#endif

		ifactory_object_initialized = TRUE;
	}
}

static void
interface_base_finalize( NAIFactoryObjectInterface *klass )
{
	static const gchar *thisfn = "na_ifactory_object_interface_base_finalize";
#if 0
	GList *ip;
#endif

	if( ifactory_object_initialized && !ifactory_object_finalized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		ifactory_object_finalized = TRUE;

#if 0
		for( ip = klass->private->registered ; ip ; ip = ip->next ){
			NADataImplement *known = ( NADataImplement * ) ip->data;
			g_free( known );
		}
		g_list_free( klass->private->registered );
#endif

		g_free( klass->private );
	}
}

static guint
ifactory_object_get_version( const NAIFactoryObject *instance )
{
	return( 1 );
}

#if 0
/**
 * na_ifactory_object_get_data_def_from_name:
 * @object: a #NAIFactoryObject object.
 * @name: the name of the elementary we are searching for.
 *
 * Returns: The #NADataDef structure which defines this @name for this @object,
 * or %NULL.
 */
NADataDef *
na_ifactory_object_get_data_def_from_name( const NAIFactoryObject *object, const gchar *name )
{
	g_return_val_if_fail( NA_IS_IFACTORY_OBJECT( object ), NULL );

	if( ifactory_object_initialized && !ifactory_object_finalized ){

		NADataGroup *groups = na_factory_object_get_data_groups( object );
		while( groups->group ){

			NADataDef *def = groups->def;
			if( def ){
				while( def->name ){

					if( !strcmp( def->name, name )){
						return( def );
					}

					def++;
				}
			}

			groups++;
		}
	}

	return( NULL );
}

/**
 * na_ifactory_object_get_data_def_from_gconf_key:
 * @object: a #NAIFactoryObject object.
 * @entry: the name of the GConf entry we are searching for.
 *
 * Returns: the #NADataDef structure which defines this GConf @entry,
 * or %NULL if not found.
 */
NADataDef *
na_ifactory_object_get_data_def_from_gconf_key( const NAIFactoryObject *object, const gchar *entry )
{
	g_return_val_if_fail( NA_IS_IFACTORY_OBJECT( object ), NULL );

	if( ifactory_object_initialized && !ifactory_object_finalized ){

		NADataGroup *groups = na_factory_object_get_data_groups( object );
		while( groups->group ){

			NADataDef *def = groups->def;
			if( def ){
				while( def->name ){

					if( def->gconf_entry && !strcmp( def->gconf_entry, entry )){
						return( def );
					}

					def++;
				}
			}

			groups++;
		}
	}

	return( NULL );
}
#endif

/**
 * na_ifactory_object_get_as_void:
 * @object: this #NAIFactoryObject instance.
 * @name: the elementary data whose value is to be got.
 *
 * Returns: the searched value.
 *
 * If the type of the value is NAFD_TYPE_STRING, NAFD_TYPE_LOCALE_STRING,
 * or NAFD_TYPE_STRING_LIST, then the returned value is a newly allocated
 * one and should be g_free() (resp. na_core_utils_slist_free()) by the
 * caller.
 */
void *
na_ifactory_object_get_as_void( const NAIFactoryObject *object, const gchar *name )
{
	g_return_val_if_fail( NA_IS_IFACTORY_OBJECT( object ), NULL );

	return( na_factory_object_get_as_void( object, name ));
}

#if 0
/**
 * na_ifactory_object_set_from_string:
 * @object: this #NAIFactoryObject instance.
 * @data_id: the elementary data whose value is to be set.
 * @data: the value to set.
 *
 * Set the elementary data with the given value.
 */
void
na_ifactory_object_set_from_string( NAIFactoryObject *object, guint data_id, const gchar *data )
{
	g_return_if_fail( NA_IS_IFACTORY_OBJECT( object ));

	na_factory_object_set_from_string( object, data_id, data );
}
#endif

/**
 * na_ifactory_object_set_from_void:
 * @object: this #NAIFactoryObject instance.
 * @name: the name of the elementary data whose value is to be set.
 * @data: the value to set.
 *
 * Set the elementary data with the given value.
 */
void
na_ifactory_object_set_from_void( NAIFactoryObject *object, const gchar *name, const void *data )
{
	g_return_if_fail( NA_IS_IFACTORY_OBJECT( object ));

	na_factory_object_set_from_void( object, name, data );
}
