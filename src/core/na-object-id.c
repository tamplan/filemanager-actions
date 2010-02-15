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

#include <api/na-object-api.h>

/* private class data
 */
struct NAObjectIdClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NAObjectIdPrivate {
	gboolean   dispose_has_run;

	gchar     *id;
};

#define NA_OBJECT_PROP_ID				"na-object-prop-id"

static NAObjectClass *st_parent_class = NULL;

static GType register_type( void );
static void  class_init( NAObjectIdClass *klass );
static void  instance_init( GTypeInstance *instance, gpointer klass );
static void  instance_dispose( GObject *object );
static void  instance_finalize( GObject *object );

GType
na_object_id_get_type( void )
{
	static GType item_type = 0;

	if( item_type == 0 ){
		item_type = register_type();
	}

	return( item_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "na_object_id_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NAObjectIdClass ),
		NULL,
		NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NAObjectId ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( NA_OBJECT_TYPE, "NAObjectId", &info, 0 );

	return( type );
}

static void
class_init( NAObjectIdClass *klass )
{
	static const gchar *thisfn = "na_object_id_class_init";
	GObjectClass *object_class;
	NAObjectClass *naobject_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	naobject_class = NA_OBJECT_CLASS( klass );
	naobject_class->dump = NULL;
	naobject_class->copy = NULL;
	naobject_class->are_equal = NULL;
	naobject_class->is_valid = NULL;

	klass->private = g_new0( NAObjectIdClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "na_object_id_instance_init";
	NAObjectId *self;

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	g_return_if_fail( NA_IS_OBJECT_ID( instance ));

	self = NA_OBJECT_ID( instance );

	self->private = g_new0( NAObjectIdPrivate, 1 );
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "na_object_id_instance_dispose";
	NAObjectId *self;

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	g_return_if_fail( NA_IS_OBJECT_ID( object ));

	self = NA_OBJECT_ID( object );

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
	NAObjectId *self;

	g_return_if_fail( NA_IS_OBJECT_ID( object ));

	self = NA_OBJECT_ID( object );

	g_free( self->private->id );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * na_object_id_sort_alpha_asc:
 * @a: first #NAObjectId.
 * @b: second #NAObjectId.
 *
 * Sort the objects in alphabetical ascending order of their label.
 *
 * Returns:
 * -1 if @a must be sorted before @b,
 *  0 if @a and @b are equal from the local point of view,
 *  1 if @a must be sorted after @b.
 */
gint
na_object_id_sort_alpha_asc( const NAObjectId *a, const NAObjectId *b )
{
	gchar *label_a, *label_b;
	gint compare;

	label_a = na_object_get_label( a );
	label_b = na_object_get_label( b );

	compare = g_utf8_collate( label_a, label_b );

	g_free( label_b );
	g_free( label_a );

	return( compare );
}

/**
 * na_object_id_sort_alpha_desc:
 * @a: first #NAObjectId.
 * @b: second #NAObjectId.
 *
 * Sort the objects in alphabetical descending order of their label.
 *
 * Returns:
 * -1 if @a must be sorted before @b,
 *  0 if @a and @b are equal from the local point of view,
 *  1 if @a must be sorted after @b.
 */
gint
na_object_id_sort_alpha_desc( const NAObjectId *a, const NAObjectId *b )
{
	return( -1 * na_object_id_sort_alpha_asc( a, b ));
}
