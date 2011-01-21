/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011 Pierre Wieser and others (see AUTHORS)
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

#include <api/na-core-utils.h>
#include <api/na-iio-provider.h>
#include <api/na-object-api.h>

#include <core/na-io-provider.h>
#include <core/na-iprefs.h>

#include "nact-application.h"
#include "nact-window.h"

/* private class data
 */
struct _NactWindowClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _NactWindowPrivate {
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

	g_return_if_fail( NACT_IS_WINDOW( instance ));

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self = NACT_WINDOW( instance );

	self->private = g_new0( NactWindowPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *window )
{
	static const gchar *thisfn = "nact_window_instance_dispose";
	NactWindow *self;

	g_return_if_fail( NACT_IS_WINDOW( window ));

	self = NACT_WINDOW( window );

	if( !self->private->dispose_has_run ){

		g_debug( "%s: window=%p (%s)", thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ));

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

	g_return_if_fail( NACT_IS_WINDOW( window ));

	g_debug( "%s: window=%p (%s)", thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ));

	self = NACT_WINDOW( window );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( window );
	}
}

/**
 * nact_window_has_writable_providers:
 * @window: this #NactWindow instance.
 *
 * Returns: %TRUE if at least one I/O provider is writable, %FALSE else.
 */
gboolean
nact_window_has_writable_providers( NactWindow *window )
{
	gboolean has_writables;
	NactApplication *application;
	NAUpdater *updater;
	NAIOProvider *provider;

	has_writables = FALSE;

	g_return_val_if_fail( NACT_IS_WINDOW( window ), has_writables );

	if( !window->private->dispose_has_run ){

		application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
		updater = nact_application_get_updater( application );
		provider = na_io_provider_find_writable_io_provider( NA_PIVOT( updater ));

		if( provider ){
			has_writables = TRUE;
		}
	}

	return( has_writables );
}

#if 0
/**
 * nact_window_is_item_writable:
 * @window: this #NactWindow object.
 * @item: the #NAObjectItem to be evaluated.
 * @reason: the reason for what @item may not be writable.
 *
 * Returns: %TRUE: if @item is actually writable, given the current
 * status of its provider, %FALSE else.
 *
 * For an item be actually writable:
 * - the item must not be itself in a read-only store, which has been
 *   checked when first reading it
 * - the provider must be willing (resp. able) to write
 * - the provider must not has been locked by the admin
 * - the writability of the provider must not have been removed by the user
 * - the whole configuration must not have been locked by the admin.
 */
gboolean
nact_window_is_item_writable( const NactWindow *window, const NAObjectItem *item, guint *reason )
{
	gboolean writable;
	NactApplication *application;
	NAUpdater *updater;
	NAIOProvider *provider;

	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), FALSE );

	writable = FALSE;
	if( reason ){
		*reason = NA_IIO_PROVIDER_STATUS_UNDETERMINED;
	}

	if( !window->private->dispose_has_run ){

		application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
		updater = nact_application_get_updater( application );

		writable = TRUE;
		if( reason ){
			*reason = NA_IIO_PROVIDER_STATUS_WRITABLE;
		}

		if( writable ){
			if( na_object_is_readonly( item )){
				writable = FALSE;
				if( reason ){
					*reason = NA_IIO_PROVIDER_STATUS_ITEM_READONLY;
				}
			}
		}

		if( writable ){
			provider = na_object_get_provider( item );
			if( provider ){
				if( !na_io_provider_is_willing_to_write( provider )){
					writable = FALSE;
					if( reason ){
						*reason = NA_IIO_PROVIDER_STATUS_PROVIDER_NOT_WILLING_TO;
					}
				} else if( na_io_provider_is_locked_by_admin( provider, NA_IPREFS( updater ))){
					writable = FALSE;
					if( reason ){
						*reason = NA_IIO_PROVIDER_STATUS_PROVIDER_LOCKED_BY_ADMIN;
					}
				} else if( !na_io_provider_is_conf_writable( provider, NA_IPREFS( updater ))){
					writable = FALSE;
					if( reason ){
						*reason = NA_IIO_PROVIDER_STATUS_PROVIDER_LOCKED_BY_USER;
					}
				} else if( na_pivot_is_configuration_locked_by_admin( NA_PIVOT( updater ))){
					writable = FALSE;
					if( reason ){
						*reason = NA_IIO_PROVIDER_STATUS_CONFIGURATION_LOCKED_BY_ADMIN;
					}
				} else if( !na_io_provider_has_write_api( provider )){
					writable = FALSE;
					if( reason ){
						*reason = NA_IIO_PROVIDER_STATUS_NO_API;
					}
				}

			/* the get_writable_provider() api already takes above checks
			 */
			} else {
				provider = na_io_provider_find_writable_io_provider( NA_PIVOT( updater ));
				if( !provider ){
					writable = FALSE;
					if( reason ){
						*reason = NA_IIO_PROVIDER_STATUS_NO_PROVIDER_FOUND;
					}
				}
			}
		}
	}

	return( writable );
}
#endif

/**
 * nact_window_save_item:
 * @window: this #NactWindow instance.
 * @item: the #NAObjectItem to be saved.
 * @msg: a pointer to a location where wi may allocate a new error message.
 *
 * Saves a modified item (action or menu) to the I/O storage subsystem.
 *
 * An action is always written at once, with all its profiles.
 *
 * Writing a menu only involves writing its NAObjectItem properties,
 * along with the list and the order of its subitems, but not the
 * subitems themselves (because they may be unmodified).
 *
 * 2010-12-13 pwi v 3.0.3
 * As we are displaying a summary of errors from the calling function,
 * we no more care here of displaying each individual error message.
 */
gboolean
nact_window_save_item( NactWindow *window, NAObjectItem *item, gchar **msg )
{
	static const gchar *thisfn = "nact_window_save_item";
	gboolean save_ok = FALSE;
	NactApplication *application;
	NAUpdater *updater;
	GSList *messages = NULL;
	guint ret;

	g_debug( "%s: window=%p, item=%p (%s)", thisfn,
			( void * ) window, ( void * ) item, G_OBJECT_TYPE_NAME( item ));
	g_return_val_if_fail( NACT_IS_WINDOW( window ), FALSE );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), FALSE );

	if( !window->private->dispose_has_run ){

		application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
		updater = nact_application_get_updater( application );

		ret = na_updater_write_item( updater, item, &messages );
		g_debug( "nact_window_save_item: ret=%d", ret );

		if( messages ){
			*msg = na_core_utils_slist_join_at_end( messages, "\n" );
			na_core_utils_slist_free( messages );

		} else if( ret != NA_IIO_PROVIDER_CODE_OK ){
			*msg = na_io_provider_get_return_code_label( ret );
		}

		save_ok = ( ret == NA_IIO_PROVIDER_CODE_OK );
	}

	return( save_ok );
}

/**
 * nact_window_delete_item:
 * @window: this #NactWindow object.
 * @item: the item (action or menu) to delete.
 *
 * Deleted an item from the I/O storage subsystem.
 *
 * 2010-12-13 pwi v 3.0.3
 * As we are displaying a summary of errors from the calling function,
 * we no more care here of displaying each individual error message.
 */
gboolean
nact_window_delete_item( NactWindow *window, const NAObjectItem *item, gchar **msg )
{
	static const gchar *thisfn = "nact_window_delete_item";
	gboolean delete_ok = FALSE;
	NactApplication *application;
	NAUpdater *updater;
	GSList *messages = NULL;
	guint ret;

	g_debug( "%s: window=%p, item=%p (%s)", thisfn,
			( void * ) window, ( void * ) item, G_OBJECT_TYPE_NAME( item ));
	g_return_val_if_fail( NACT_IS_WINDOW( window ), FALSE );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), FALSE );

	if( !window->private->dispose_has_run ){

		application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
		updater = nact_application_get_updater( application );

		na_object_dump_norec( item );

		ret = na_updater_delete_item( updater, item, &messages );

		if( messages ){
			*msg = na_core_utils_slist_join_at_end( messages, "\n" );
			na_core_utils_slist_free( messages );

		} else if( ret != NA_IIO_PROVIDER_CODE_OK ){
			*msg = na_io_provider_get_return_code_label( ret );
		}

		delete_ok = ( ret == NA_IIO_PROVIDER_CODE_OK );
	}

	return( delete_ok );
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
	gboolean confirm = FALSE;
	gchar *first;
	gchar *second;

	g_return_val_if_fail( NACT_IS_WINDOW( window ), FALSE );

	if( !window->private->dispose_has_run ){

		first = g_strdup_printf( _( "Some items have been modified." ));
		second = g_strdup( _( "Are you sure you want to quit without saving them ?" ));

		confirm = base_window_display_yesno_dlg( BASE_WINDOW( window ), first, second );

		g_free( second );
		g_free( first );
	}

	return( confirm );
}
