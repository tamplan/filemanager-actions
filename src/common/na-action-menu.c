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

#include "na-action.h"
#include "na-action-menu.h"

/* private class data
 */
struct NAActionMenuClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NAActionMenuPrivate {
	gboolean dispose_has_run;
};

static NAObjectClass *st_parent_class = NULL;

static GType     register_type( void );
static void      class_init( NAActionMenuClass *klass );
static void      instance_init( GTypeInstance *instance, gpointer klass );
static void      instance_dispose( GObject *object );
static void      instance_finalize( GObject *object );

static NAObject *object_new( const NAObject *menu );
static void      object_copy( NAObject *target, const NAObject *source );
static gboolean  object_are_equal( const NAObject *a, const NAObject *b );
static gboolean  object_is_valid( const NAObject *menu );
static void      object_dump( const NAObject *menu );
static gchar    *object_get_clipboard_id( const NAObject *menu );

GType
na_action_menu_get_type( void )
{
	static GType action_type = 0;

	if( !action_type ){
		action_type = register_type();
	}

	return( action_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "na_action_menu_register_type";

	static GTypeInfo info = {
		sizeof( NAActionMenuClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NAActionMenu ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	return( g_type_register_static( NA_OBJECT_ITEM_TYPE, "NAActionMenu", &info, 0 ));
}

static void
class_init( NAActionMenuClass *klass )
{
	static const gchar *thisfn = "na_action_menu_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NAActionMenuClassPrivate, 1 );

	NA_OBJECT_CLASS( klass )->new = object_new;
	NA_OBJECT_CLASS( klass )->copy = object_copy;
	NA_OBJECT_CLASS( klass )->are_equal = object_are_equal;
	NA_OBJECT_CLASS( klass )->is_valid = object_is_valid;
	NA_OBJECT_CLASS( klass )->dump = object_dump;
	NA_OBJECT_CLASS( klass )->get_clipboard_id = object_get_clipboard_id;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "na_action_menu_instance_init";
	NAActionMenu *self;

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );
	g_assert( NA_IS_ACTION_MENU( instance ));
	self = NA_ACTION_MENU( instance );

	self->private = g_new0( NAActionMenuPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "na_action_menu_instance_dispose";
	NAActionMenu *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );

	g_assert( NA_IS_ACTION_MENU( object ));
	self = NA_ACTION_MENU( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		G_OBJECT_CLASS( st_parent_class )->dispose( object );
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "na_action_menu_instance_finalize";
	NAActionMenu *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_assert( NA_IS_ACTION_MENU( object ));
	self = ( NAActionMenu * ) object;

	g_free( self->private );

	/* chain call to parent class */
	if((( GObjectClass * ) st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * na_action_menu_new:
 *
 * Allocates a new #NAActionMenu object.
 *
 * The new #NAActionMenu object is initialized with suitable default values,
 * but without any profile.
 *
 * Returns: the newly allocated #NAActionMenu object.
 */
NAActionMenu *
na_action_menu_new( void )
{
	NAActionMenu *menu;

	menu = g_object_new( NA_ACTION_MENU_TYPE, NULL );

	na_action_set_new_uuid( NA_ACTION( menu ));

	na_object_set_label( NA_OBJECT( menu ), NA_ACTION_MENU_DEFAULT_LABEL );

	return( menu );
}

static NAObject *
object_new( const NAObject *menu )
{
	return( NA_OBJECT( na_action_menu_new()));
}

void
object_copy( NAObject *target, const NAObject *source )
{
	if( st_parent_class->copy ){
		st_parent_class->copy( target, source );
	}

	g_assert( NA_IS_ACTION_MENU( source ));
	g_assert( NA_IS_ACTION_MENU( target ));
}

static gboolean
object_are_equal( const NAObject *a, const NAObject *b )
{
	gboolean equal = TRUE;

	if( equal ){
		if( st_parent_class->are_equal ){
			equal = st_parent_class->are_equal( a, b );
		}
	}

	g_assert( NA_IS_ACTION_MENU( a ));
	g_assert( NA_IS_ACTION_MENU( b ));

	return( equal );
}

/*
 * a valid NAActionMenu requires a not null, not empty label
 * this is checked here as NAObject doesn't have this condition
 */
gboolean
object_is_valid( const NAObject *menu )
{
	gchar *label;
	gboolean is_valid = TRUE;

	if( is_valid ){
		if( st_parent_class->is_valid ){
			is_valid = st_parent_class->is_valid( menu );
		}
	}

	g_assert( NA_IS_ACTION_MENU( menu ));

	if( is_valid ){
		label = na_object_get_label( menu );
		is_valid = ( label && g_utf8_strlen( label, -1 ) > 0 );
		g_free( label );
	}

	return( is_valid );
}

static void
object_dump( const NAObject *menu )
{
	static const gchar *thisfn = "na_action_menu_object_dump";
	/*NAActionMenu *self;*/

	if( st_parent_class->dump ){
		st_parent_class->dump( menu );
	}

	g_assert( NA_IS_ACTION_MENU( menu ));
	/*self = NA_ACTION_MENU( menu );*/

	g_debug( "%s: (nothing to dump)", thisfn );
}

static gchar *
object_get_clipboard_id( const NAObject *menu )
{
	gchar *uuid;
	gchar *clipboard_id;

	uuid = na_object_get_id( menu );
	clipboard_id = g_strdup_printf( "M:%s", uuid );
	g_free( uuid );

	return( clipboard_id );
}
