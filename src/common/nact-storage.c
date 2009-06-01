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
#include "nact-gconf.h"
#include "nact-storage.h"

static GObjectClass *st_parent_class = NULL;

static GType   register_type( void );
static void    class_init( NactStorageClass *klass );
static void    instance_init( GTypeInstance *instance, gpointer klass );
static void    instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void    instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void    instance_dispose( GObject *object );
static void    instance_finalize( GObject *object );
static void    do_dump( const NactStorage *object );

struct NactStoragePrivate {
	gboolean dispose_has_run;
	int      origin;
};

struct NactStorageClassPrivate {
};

enum {
	PROP_ORIGIN = 1
};

GType
nact_storage_get_type( void )
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
	static GTypeInfo info = {
		sizeof( NactStorageClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactStorage ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	return( g_type_register_static( G_TYPE_OBJECT, "NactStorage", &info, 0 ));
}

static void
class_init( NactStorageClass *klass )
{
	static const gchar *thisfn = "nact_storage_class_init";
	g_debug( "%s: klass=%p", thisfn, klass );

	st_parent_class = g_type_class_peek_parent( klass );

	GObjectClass *object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
	object_class->set_property = instance_set_property;
	object_class->get_property = instance_get_property;

	GParamSpec *spec;
	spec = g_param_spec_int(
			"origin",
			"origin",
			"Internal identifiant of the storage subsystem", 0, ORIGIN_LAST, 0,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_ORIGIN, spec );

	klass->private = g_new0( NactStorageClassPrivate, 1 );

	klass->do_dump = do_dump;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	g_assert( NACT_IS_STORAGE( instance ));
	NactStorage *self = NACT_STORAGE( instance );

	self->private = g_new0( NactStoragePrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->origin = 0;
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	g_assert( NACT_IS_STORAGE( object ));
	NactStorage *self = NACT_STORAGE( object );

	switch( property_id ){
		case PROP_ORIGIN:
			g_value_set_int( value, self->private->origin );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	g_assert( NACT_IS_STORAGE( object ));
	NactStorage *self = NACT_STORAGE( object );

	switch( property_id ){
		case PROP_ORIGIN:
			self->private->origin = g_value_get_int( value );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_dispose( GObject *object )
{
	g_assert( NACT_IS_STORAGE( object ));
	NactStorage *self = NACT_STORAGE( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		G_OBJECT_CLASS( st_parent_class )->dispose( object );
	}
}

static void
instance_finalize( GObject *object )
{
	g_assert( NACT_IS_STORAGE( object ));
	/*NactStorage *self = ( NactStorage * ) object;*/

	/* chain call to parent class */
	if( st_parent_class->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * Returns the id of the object as a newly allocated string.
 *
 * This is a pure virtual function, which has to be implemented by
 * the derived class.
 *
 * @object: object whose id is to be returned.
 *
 * The returned string has to be g_free by the caller.
 */
gchar *
nact_storage_get_id( const NactStorage *object )
{
	g_assert( NACT_IS_STORAGE( object ));
	return( NACT_STORAGE_GET_CLASS( object )->get_id( object ));
}

static void
do_dump( const NactStorage *object )
{
	static const char *thisfn = "nact_storage_do_dump";

	g_assert( NACT_IS_STORAGE( object ));

	g_debug( "%s: origin=%d", thisfn, object->private->origin );
}

/**
 * Dump the content of the object via g_debug output.
 *
 * This is a virtual function which may be implemented by the derived
 * class ; the derived class may also call its parent class to get a
 * dump of parent object.
 *
 * @object: object to be dumped.
 */
void
nact_storage_dump( const NactStorage *object )
{
	g_assert( NACT_IS_STORAGE( object ));
	NACT_STORAGE_GET_CLASS( object )->do_dump( object );
}

GSList *
nact_storage_load_action_ids( void )
{
	static const gchar *thisfn = "nact_storage_load_action_ids";
	g_debug( "%s", thisfn );
	return( nact_gconf_load_uuids());
}

gboolean
nact_storage_load_action_properties( NactStorage *action )
{
	g_assert( NACT_IS_STORAGE( action ));
	return( nact_gconf_load_action_properties( action ));
}

void
nact_storage_free_action_ids( GSList *list )
{
	static const gchar *thisfn = "nact_storage_free_action_ids";
	g_debug( "%s: list=%p", thisfn, list );
	nact_gconf_free_uuids( list );
}

GSList *
nact_storage_load_profile_ids( NactStorage *action )
{
	g_assert( NACT_IS_STORAGE( action ));
	return( nact_gconf_load_profile_names( action ));
}

gboolean
nact_storage_load_profile_properties( NactStorage *profile )
{
	g_assert( NACT_IS_STORAGE( profile ));
	return( nact_gconf_load_profile_properties( profile ));
}

void
nact_storage_free_profile_ids( GSList *list )
{
	return( nact_gconf_free_profile_names( list ));
}
