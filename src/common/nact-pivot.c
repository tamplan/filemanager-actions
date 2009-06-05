/*
 * Nautilus Pivots
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
#include "nact-pivot.h"
#include "nact-iio-provider.h"

struct NactPivotPrivate {
	gboolean  dispose_has_run;

	/* list of interface providers
	 * needs to be in the instance rather than in the class to be able
	 * to pass NactPivot object to the IO provider, so that the later
	 * is able to have access to the former (and its list of actions)
	 */
	GSList   *providers;

	/* list of actions
	 */
	GSList   *actions;
};

struct NactPivotClassPrivate {
};

static GObjectClass   *st_parent_class = NULL;

static GType   register_type( void );
static void    class_init( NactPivotClass *klass );
static void    instance_init( GTypeInstance *instance, gpointer klass );
static GSList *register_interface_providers( const NactPivot *pivot );
static void    instance_dispose( GObject *object );
static void    instance_finalize( GObject *object );

NactPivot *
nact_pivot_new( void )
{
	return( g_object_new( NACT_PIVOT_TYPE, NULL ));
}

GType
nact_pivot_get_type( void )
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
		sizeof( NactPivotClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactPivot ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	return( g_type_register_static( G_TYPE_OBJECT, "NactPivot", &info, 0 ));
}

static void
class_init( NactPivotClass *klass )
{
	static const gchar *thisfn = "nact_pivot_class_init";
	g_debug( "%s: klass=%p", thisfn, klass );

	st_parent_class = g_type_class_peek_parent( klass );

	GObjectClass *object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactPivotClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_pivot_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );

	g_assert( NACT_IS_PIVOT( instance ));
	NactPivot* self = NACT_PIVOT( instance );

	self->private = g_new0( NactPivotPrivate, 1 );
	self->private->dispose_has_run = FALSE;
	self->private->providers = register_interface_providers( self );
	self->private->actions = nact_iio_provider_load_actions( G_OBJECT( self ));
}

static GSList *
register_interface_providers( const NactPivot *pivot )
{
	static const gchar *thisfn = "nact_pivot_register_interface_providers";
	g_debug( "%s", thisfn );

	GSList *list = NULL;

	list = g_slist_prepend( list, nact_gconf_new( G_OBJECT( pivot )));

	return( list );
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "nact_pivot_instance_dispose";
	g_debug( "%s: object=%p", thisfn, object );

	g_assert( NACT_IS_PIVOT( object ));
	NactPivot *self = NACT_PIVOT( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* release list of actions */
		GSList *ia;
		for( ia = self->private->actions ; ia ; ia = ia->next ){
			g_object_unref( G_OBJECT( ia->data ));
		}
		g_slist_free( self->private->actions );
		self->private->actions = NULL;

		/* chain up to the parent class */
		G_OBJECT_CLASS( st_parent_class )->dispose( object );
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "nact_pivot_instance_finalize";
	g_debug( "%s: object=%p", thisfn, object );

	g_assert( NACT_IS_PIVOT( object ));
	NactPivot *self = ( NactPivot * ) object;

	/* release the interface providers */
	GSList *ip;
	for( ip = self->private->providers ; ip ; ip = ip->next ){
		g_object_unref( G_OBJECT( ip->data ));
	}
	g_slist_free( self->private->providers );
	self->private->providers = NULL;

	/* chain call to parent class */
	if((( GObjectClass * ) st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * Returns the list of providers of the required interface.
 */
GSList *
nact_pivot_get_providers( const NactPivot *pivot, GType type )
{
	static const gchar *thisfn = "nact_pivot_get_providers";
	g_debug( "%s", thisfn );

	g_assert( NACT_IS_PIVOT( pivot ));

	GSList *list = NULL;
	GSList *ip;
	for( ip = pivot->private->providers ; ip ; ip = ip->next ){
		if( G_TYPE_CHECK_INSTANCE_TYPE( G_OBJECT( ip->data ), type )){
			list = g_slist_prepend( list, ip->data );
		}
	}

	return( list );
}
