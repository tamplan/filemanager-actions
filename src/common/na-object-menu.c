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

#include "na-iduplicable.h"
#include "na-object-api.h"
#include "na-object-action.h"
#include "na-object-menu.h"
#include "na-utils.h"

/* private class data
 */
struct NAObjectMenuClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NAObjectMenuPrivate {
	gboolean dispose_has_run;

	/* this is the list of subitems as a list of id strings
	 * as readen from IIOProviders
	 */
	GSList  *items_ids;
};

static NAObjectClass *st_parent_class = NULL;

static GType     register_type( void );
static void      class_init( NAObjectMenuClass *klass );
static void      instance_init( GTypeInstance *instance, gpointer klass );
static void      instance_dispose( GObject *object );
static void      instance_finalize( GObject *object );

static void      object_dump( const NAObject *menu );
static gchar    *object_get_clipboard_id( const NAObject *menu );
static NAObject *object_new( const NAObject *menu );
static void      object_copy( NAObject *target, const NAObject *source );
static gboolean  object_are_equal( const NAObject *a, const NAObject *b );
static gboolean  object_is_valid( const NAObject *menu );

GType
na_object_menu_get_type( void )
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
	static const gchar *thisfn = "na_object_menu_register_type";

	static GTypeInfo info = {
		sizeof( NAObjectMenuClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NAObjectMenu ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	return( g_type_register_static( NA_OBJECT_ITEM_TYPE, "NAObjectMenu", &info, 0 ));
}

static void
class_init( NAObjectMenuClass *klass )
{
	static const gchar *thisfn = "na_object_menu_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NAObjectMenuClassPrivate, 1 );

	NA_OBJECT_CLASS( klass )->dump = object_dump;
	NA_OBJECT_CLASS( klass )->get_clipboard_id = object_get_clipboard_id;
	NA_OBJECT_CLASS( klass )->ref = NULL;
	NA_OBJECT_CLASS( klass )->new = object_new;
	NA_OBJECT_CLASS( klass )->copy = object_copy;
	NA_OBJECT_CLASS( klass )->are_equal = object_are_equal;
	NA_OBJECT_CLASS( klass )->is_valid = object_is_valid;
	NA_OBJECT_CLASS( klass )->get_childs = NULL;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	/*static const gchar *thisfn = "na_object_menu_instance_init";*/
	NAObjectMenu *self;

	/*g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );*/
	g_return_if_fail( NA_IS_OBJECT_MENU( instance ));
	self = NA_OBJECT_MENU( instance );

	self->private = g_new0( NAObjectMenuPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *object )
{
	/*static const gchar *thisfn = "na_object_menu_instance_dispose";*/
	NAObjectMenu *self;

	/*g_debug( "%s: object=%p", thisfn, ( void * ) object );*/
	g_return_if_fail( NA_IS_OBJECT_MENU( object ));
	self = NA_OBJECT_MENU( object );

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
	/*static const gchar *thisfn = "na_object_menu_instance_finalize";*/
	NAObjectMenu *self;

	/*g_debug( "%s: object=%p", thisfn, ( void * ) object );*/
	g_return_if_fail( NA_IS_OBJECT_MENU( object ));
	self = NA_OBJECT_MENU( object );

	/* release string list of subitems */
	na_utils_free_string_list( self->private->items_ids );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * na_object_menu_new:
 *
 * Allocates a new #NAObjectMenu object.
 *
 * The new #NAObjectMenu object is initialized with suitable default values,
 * but without any profile.
 *
 * Returns: the newly allocated #NAObjectMenu object.
 */
NAObjectMenu *
na_object_menu_new( void )
{
	NAObjectMenu *menu;

	menu = g_object_new( NA_OBJECT_MENU_TYPE, NULL );

	na_object_set_new_id( menu );
	na_object_set_label( menu, NA_OBJECT_MENU_DEFAULT_LABEL );

	return( menu );
}

/**
 * na_object_menu_get_items_list:
 * @menu: this #NAObjectMenu object.
 *
 * Returns: the items_ids string list, as readen from the IIOProvider.
 *
 * The returned list should be na_utils_free_string_list() by the caller.
 */
GSList *
na_object_menu_get_items_list( const NAObjectMenu *menu )
{
	GSList *list = NULL;

	g_return_val_if_fail( NA_IS_OBJECT_MENU( menu ), NULL );

	if( !menu->private->dispose_has_run ){
		list = na_utils_duplicate_string_list( menu->private->items_ids );
	}

	return( list );
}

/**
 * na_object_menu_rebuild_items_list:
 * @menu: this #NAObjectMenu object.
 *
 * Returns: a string list which contains the ordered list of ids of
 * subitems.
 *
 * Note that the returned list is built on each call to this function,
 * and is so an exact image of the current situation.
 *
 * The returned list should be na_utils_free_string_list() by the caller.
 */
GSList *
na_object_menu_rebuild_items_list( const NAObjectMenu *menu )
{
	GSList *list = NULL;
	GList *items, *it;
	gchar *uuid;

	g_return_val_if_fail( NA_IS_OBJECT_MENU( menu ), NULL );

	if( !menu->private->dispose_has_run ){

		items = na_object_get_items( menu );

		for( it = items ; it ; it = it->next ){
			NAObjectItem *item = NA_OBJECT_ITEM( it->data );
			uuid = na_object_get_id( item );
			list = g_slist_prepend( list, uuid );
		}

		na_object_free_items( items );

		list = g_slist_reverse( list );
	}

	return( list );
}

/**
 * na_object_menu_set_items_list:
 * @menu: this #NAObjectMenu object.
 * @items: an ordered list of UUID of subitems.
 *
 * Set the internal list of uuids of subitems.
 *
 * This function takes a copy of the provided list. This later may so
 * be safely released by the caller after this function has returned.
 */
void
na_object_menu_set_items_list( NAObjectMenu *menu, GSList *items )
{
	g_return_if_fail( NA_IS_OBJECT_MENU( menu ));

	if( !menu->private->dispose_has_run ){

		na_utils_free_string_list( menu->private->items_ids );
		menu->private->items_ids = na_utils_duplicate_string_list( items );
	}
}

static void
object_dump( const NAObject *menu )
{
	static const gchar *thisfn = "na_object_menu_object_dump";
	/*NAObjectMenu *self;*/

	g_return_if_fail( NA_IS_OBJECT_MENU( menu ));

	if( !NA_OBJECT_MENU( menu )->private->dispose_has_run ){

		g_debug( "%s: (nothing to dump)", thisfn );
	}
}

static gchar *
object_get_clipboard_id( const NAObject *menu )
{
	gchar *uuid;
	gchar *clipboard_id = NULL;

	g_return_val_if_fail( NA_IS_OBJECT_MENU( menu ), NULL );

	if( !NA_OBJECT_MENU( menu )->private->dispose_has_run ){

		uuid = na_object_get_id( menu );
		clipboard_id = g_strdup_printf( "M:%s", uuid );
		g_free( uuid );
	}

	return( clipboard_id );
}

static NAObject *
object_new( const NAObject *menu )
{
	return( NA_OBJECT( na_object_menu_new()));
}

static void
object_copy( NAObject *target, const NAObject *source )
{
	g_return_if_fail( NA_IS_OBJECT_MENU( target ));
	g_return_if_fail( NA_IS_OBJECT_MENU( source ));

	if( !NA_OBJECT_MENU( target )->private->dispose_has_run &&
		!NA_OBJECT_MENU( source )->private->dispose_has_run ){

		/* nothing to do */
	}
}

static gboolean
object_are_equal( const NAObject *a, const NAObject *b )
{
	gboolean equal = TRUE;

	g_return_val_if_fail( NA_IS_OBJECT_MENU( a ), FALSE );
	g_return_val_if_fail( NA_IS_OBJECT_MENU( b ), FALSE );

	if( !NA_OBJECT_MENU( a )->private->dispose_has_run &&
		!NA_OBJECT_MENU( b )->private->dispose_has_run ){

		/* nothing to compare */
	}

	return( equal );
}

/*
 * a valid NAObjectMenu requires a not null, not empty label
 * this is checked here as NAObject doesn't have this condition
 */
static gboolean
object_is_valid( const NAObject *menu )
{
	gchar *label;
	gboolean is_valid = TRUE;

	g_return_val_if_fail( NA_IS_OBJECT_MENU( menu ), FALSE );

	if( !NA_OBJECT_MENU( menu )->private->dispose_has_run ){

		if( is_valid ){
			label = na_object_get_label( menu );
			is_valid = ( label && g_utf8_strlen( label, -1 ) > 0 );
			g_free( label );
		}
	}

	return( is_valid );
}
