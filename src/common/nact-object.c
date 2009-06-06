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

#include "nact-object.h"
#include "nact-uti-lists.h"

struct NactObjectPrivate {
	gboolean dispose_has_run;
};

struct NactObjectClassPrivate {
};

static GObjectClass *st_parent_class = NULL;

static GType   register_type( void );
static void    class_init( NactObjectClass *klass );
static void    instance_init( GTypeInstance *instance, gpointer klass );
static void    instance_dispose( GObject *object );
static void    instance_finalize( GObject *object );
static void    do_dump( const NactObject *object );

GType
nact_object_get_type( void )
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
		sizeof( NactObjectClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactObject ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	return( g_type_register_static( G_TYPE_OBJECT, "NactObject", &info, 0 ));
}

static void
class_init( NactObjectClass *klass )
{
	static const gchar *thisfn = "nact_object_class_init";
	g_debug( "%s: klass=%p", thisfn, klass );

	st_parent_class = g_type_class_peek_parent( klass );

	GObjectClass *object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactObjectClassPrivate, 1 );

	klass->dump = do_dump;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	/*static const gchar *thisfn = "nact_object_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );*/

	g_assert( NACT_IS_OBJECT( instance ));
	NactObject *self = NACT_OBJECT( instance );

	self->private = g_new0( NactObjectPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *object )
{
	g_assert( NACT_IS_OBJECT( object ));
	NactObject *self = NACT_OBJECT( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		G_OBJECT_CLASS( st_parent_class )->dispose( object );
	}
}

static void
instance_finalize( GObject *object )
{
	g_assert( NACT_IS_OBJECT( object ));
	/*NactObject *self = ( NactObject * ) object;*/

	/* chain call to parent class */
	if( st_parent_class->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

static void
do_dump( const NactObject *object )
{
	static const char *thisfn = "nact_object_do_dump";
	g_assert( NACT_IS_OBJECT( object ));
	g_debug( "%s: object=%p", thisfn, object );
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
nact_object_dump( const NactObject *object )
{
	g_assert( NACT_IS_OBJECT( object ));

	NACT_OBJECT_GET_CLASS( object )->dump( object );
}
