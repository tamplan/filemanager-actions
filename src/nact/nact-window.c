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

#include <glib.h>
#include <glib/gi18n.h>

#include <common/na-object-api.h>
#include <common/na-iprefs.h>
#include <common/na-iio-provider.h>
#include <common/na-utils.h>

#include "nact-application.h"
#include "nact-window.h"

/* private class data
 */
struct NactWindowClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NactWindowPrivate {
	gboolean dispose_has_run;
};

static BaseWindowClass *st_parent_class = NULL;

static GType  register_type( void );
static void   class_init( NactWindowClass *klass );
static void   instance_init( GTypeInstance *instance, gpointer klass );
static void   instance_dispose( GObject *application );
static void   instance_finalize( GObject *application );

GType
nact_window_get_type( void )
{
	static GType window_type = 0;

	if( !window_type ){
		window_type = register_type();
	}

	return( window_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "nact_window_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NactWindowClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactWindow ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( BASE_WINDOW_TYPE, "NactWindow", &info, 0 );

	return( type );
}

static void
class_init( NactWindowClass *klass )
{
	static const gchar *thisfn = "nact_window_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactWindowClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_window_instance_init";
	NactWindow *self;

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );

	g_assert( NACT_IS_WINDOW( instance ));
	self = NACT_WINDOW( instance );

	self->private = g_new0( NactWindowPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *window )
{
	static const gchar *thisfn = "nact_window_instance_dispose";
	NactWindow *self;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );

	g_assert( NACT_IS_WINDOW( window ));
	self = NACT_WINDOW( window );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( window );
		}
	}
}

static void
instance_finalize( GObject *window )
{
	static const gchar *thisfn = "nact_window_instance_finalize";
	NactWindow *self;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );

	g_assert( NACT_IS_WINDOW( window ));
	self = ( NactWindow * ) window;

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( window );
	}
}

/**
 * Returns a pointer to the list of actions.
 */
NAPivot *
nact_window_get_pivot( NactWindow *window )
{
	NactApplication *application;
	NAPivot *pivot;

	g_object_get( G_OBJECT( window ), BASE_WINDOW_PROP_APPLICATION, &application, NULL );
	g_return_val_if_fail( NACT_IS_APPLICATION( application ), NULL );

	pivot = nact_application_get_pivot( application );
	g_return_val_if_fail( NA_IS_PIVOT( pivot ), NULL );

	return( pivot );
}

/**
 * nact_window_save_item:
 * @window: this #NactWindow instance.
 * @item: the #NAObjectItem to be saved.
 *
 * Saves a modified item (action or menu) to the I/O storage subsystem.
 *
 * An action is always written at once, with all its profiles.
 *
 * Writing a menu only involves writing its NAObjectItem properties,
 * along with the list and the order of its subitems, but not the
 * subitems themselves (because they may be unmodified)
 */
gboolean
nact_window_save_item( NactWindow *window, NAObjectItem *item )
{
	static const gchar *thisfn = "nact_window_save_item";
	NAPivot *pivot;
	gchar *msg = NULL;
	guint ret;

	g_debug( "%s: window=%p (%s), item=%p (%s)", thisfn,
			( void * ) window, G_OBJECT_TYPE_NAME( window ),
			( void * ) item, G_OBJECT_TYPE_NAME( item ));

	pivot = nact_window_get_pivot( window );
	g_assert( NA_IS_PIVOT( pivot ));

	na_object_dump_norec( item );

	ret = na_pivot_write_item( pivot, NA_OBJECT( item ), &msg );

	if( msg ){
		base_window_error_dlg(
				BASE_WINDOW( window ),
				GTK_MESSAGE_WARNING, _( "An error has occured when trying to save the item" ), msg );
		g_free( msg );
	}

	return( ret == NA_IIO_PROVIDER_WRITE_OK );
}

/**
 * nact_window_delete_item:
 * @window: this #NactWindow object.
 * @item: the item (action or menu) to delete.
 *
 * Deleted an item from the I/O storage subsystem.
 */
gboolean
nact_window_delete_item( NactWindow *window, NAObjectItem *item )
{
	static const gchar *thisfn = "nact_window_delete_item";
	NAPivot *pivot;
	gchar *msg = NULL;
	guint ret;

	g_debug( "%s: window=%p, item=%p", thisfn, ( void * ) window, ( void * ) item );

	pivot = nact_window_get_pivot( window );
	g_assert( NA_IS_PIVOT( pivot ));

	na_object_dump_norec( item );

	ret = na_pivot_delete_item( pivot, NA_OBJECT( item ), &msg );

	if( msg ){
		base_window_error_dlg(
				BASE_WINDOW( window ),
				GTK_MESSAGE_WARNING, _( "An error has occured when trying to delete the item" ), msg );
		g_free( msg );
	}

	return( ret == NA_IIO_PROVIDER_WRITE_OK );
}

/**
 * nact_window_write_level_zero:
 * @window: this #NactWindow-derived instance.
 * @items: full current tree of items in #NactIActionsList treeview.
 *
 * Writes as a GConf preference order and content of level zero items.
 */
void
nact_window_write_level_zero( NactWindow *window, GList *items )
{
	static const gchar *thisfn = "nact_window_write_level_zero";
	GList *it;
	gchar *id;
	GSList *content;
	NactApplication *application;
	NAPivot *pivot;

	g_debug( "%s: window=%p, items=%p (%d items)", thisfn, ( void * ) window, ( void * ) items, g_list_length( items ));
	g_return_if_fail( NACT_IS_WINDOW( window ));

	content = NULL;
	for( it = items ; it ; it = it->next ){
		id = na_object_get_id( it->data );
		content = g_slist_prepend( content, id );
	}
	content = g_slist_reverse( content );

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	pivot = nact_application_get_pivot( application );
	na_iprefs_set_level_zero_items( NA_IPREFS( pivot ), content );

	na_utils_free_string_list( content );
}

/**
 * nact_window_count_level_zero_items:
 */
void
nact_window_count_level_zero_items( GList *items, guint *actions, guint *profiles, guint *menus )
{
	GList *it;

	g_return_if_fail( actions );
	g_return_if_fail( profiles );
	g_return_if_fail( menus );

	*actions = 0;
	*profiles = 0;
	*menus = 0;

	for( it = items ; it ; it = it->next ){
		if( NA_IS_OBJECT_ACTION( it->data )){
			*actions += 1;
		} else if( NA_IS_OBJECT_PROFILE( it->data )){
			*profiles += 1;
		} else if( NA_IS_OBJECT_MENU( it->data )){
			*menus += 1;
		}
	}
}

/**
 * nact_window_warn_modified:
 * @window: this #NactWindow instance.
 *
 * Emits a warning if the action has been modified.
 *
 * Returns: %TRUE if the user confirms he wants to quit.
 */
gboolean
nact_window_warn_modified( NactWindow *window )
{
	gchar *first;
	gchar *second;
	gboolean ok;

	first = g_strdup_printf( _( "Some items have been modified." ));
	second = g_strdup( _( "Are you sure you want to quit without saving them ?" ));

	ok = base_window_yesno_dlg( BASE_WINDOW( window ), GTK_MESSAGE_QUESTION, first, second );

	g_free( second );
	g_free( first );

	return( ok );
}
