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

#include <api/fma-core-utils.h>
#include <api/fma-gconf-utils.h>
#include <api/fma-object-api.h>

#include "fma-io-provider.h"
#include "fma-settings.h"
#include "fma-updater.h"

/* private class data
 */
struct _FMAUpdaterClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _FMAUpdaterPrivate {
	gboolean dispose_has_run;
	gboolean are_preferences_locked;
	gboolean is_level_zero_writable;
};

static FMAPivotClass *st_parent_class = NULL;

static GType    register_type( void );
static void     class_init( FMAUpdaterClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_dispose( GObject *object );
static void     instance_finalize( GObject *object );

static gboolean are_preferences_locked( const FMAUpdater *updater );
static gboolean is_level_zero_writable( const FMAUpdater *updater );
static void     set_writability_status( FMAObjectItem *item, const FMAUpdater *updater );

GType
fma_updater_get_type( void )
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
	static const gchar *thisfn = "fma_updater_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( FMAUpdaterClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( FMAUpdater ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( FMA_TYPE_PIVOT, "FMAUpdater", &info, 0 );

	return( type );
}

static void
class_init( FMAUpdaterClass *klass )
{
	static const gchar *thisfn = "fma_updater_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( FMAUpdaterClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "fma_updater_instance_init";
	FMAUpdater *self;

	g_return_if_fail( FMA_IS_UPDATER( instance ));

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self = FMA_UPDATER( instance );

	self->private = g_new0( FMAUpdaterPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "fma_updater_instance_dispose";
	FMAUpdater *self;

	g_return_if_fail( FMA_IS_UPDATER( object ));

	self = FMA_UPDATER( object );

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
	static const gchar *thisfn = "fma_updater_instance_finalize";
	FMAUpdater *self;

	g_return_if_fail( FMA_IS_UPDATER( object ));

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	self = FMA_UPDATER( object );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/*
 * fma_updater_new:
 *
 * Returns: a newly allocated #FMAUpdater object.
 */
FMAUpdater *
fma_updater_new( void )
{
	static const gchar *thisfn = "fma_updater_new";
	FMAUpdater *updater;

	g_debug( "%s", thisfn );

	updater = g_object_new( FMA_TYPE_UPDATER, NULL );

	updater->private->are_preferences_locked = are_preferences_locked( updater );
	updater->private->is_level_zero_writable = is_level_zero_writable( updater );

	g_debug( "%s: is_level_zero_writable=%s",
			thisfn,
			updater->private->is_level_zero_writable ? "True":"False" );

	return( updater );
}

static gboolean
are_preferences_locked( const FMAUpdater *updater )
{
	gboolean are_locked;
	gboolean mandatory;

	are_locked = fma_settings_get_boolean( IPREFS_ADMIN_PREFERENCES_LOCKED, NULL, &mandatory );

	return( are_locked && mandatory );
}

static gboolean
is_level_zero_writable( const FMAUpdater *updater )
{
	GSList *level_zero;
	gboolean mandatory;

	level_zero = fma_settings_get_string_list( IPREFS_ITEMS_LEVEL_ZERO_ORDER, NULL, &mandatory );

	fma_core_utils_slist_free( level_zero );

	g_debug( "fma_updater_is_level_zero_writable: IPREFS_ITEMS_LEVEL_ZERO_ORDER: mandatory=%s",
			mandatory ? "True":"False" );

	return( !mandatory );
}

/*
 * fma_updater_check_item_writability_status:
 * @updater: this #FMAUpdater object.
 * @item: the #FMAObjectItem to be written.
 *
 * Compute and set the writability status of the @item.
 *
 * For an item be actually writable:
 * - the item must not be itself in a read-only store, which has been
 *   checked when first reading it
 * - the provider must be willing (resp. able) to write
 * - the provider must not has been locked by the admin, nor by the user
 *
 * If the item does not have a parent, then the level zero must be writable.
 */
void
fma_updater_check_item_writability_status( const FMAUpdater *updater, const FMAObjectItem *item )
{
	gboolean writable;
	FMAIOProvider *provider;
	FMAObjectItem *parent;
	guint reason;

	g_return_if_fail( FMA_IS_UPDATER( updater ));
	g_return_if_fail( FMA_IS_OBJECT_ITEM( item ));

	writable = FALSE;
	reason = IIO_PROVIDER_STATUS_UNDETERMINED;

	if( !updater->private->dispose_has_run ){

		writable = TRUE;
		reason = IIO_PROVIDER_STATUS_WRITABLE;

		/* Writability status of the item has been determined at load time
		 * (cf. e.g. io-desktop/nadp-reader.c:read_done_item_is_writable()).
		 * Though I'm plenty conscious that this status is subject to many
		 * changes during the life of the item (e.g. by modifying permissions
		 * on the underlying store), it is just more efficient to not reevaluate
		 * this status each time we need it, and enough for our needs..
		 */
		if( writable ){
			if( fma_object_is_readonly( item )){
				writable = FALSE;
				reason = IIO_PROVIDER_STATUS_ITEM_READONLY;
			}
		}

		if( writable ){
			provider = fma_object_get_provider( item );
			if( provider ){
				writable = fma_io_provider_is_finally_writable( provider, &reason );

			/* the get_writable_provider() api already takes care of above checks
			 */
			} else {
				provider = fma_io_provider_find_writable_io_provider( FMA_PIVOT( updater ));
				if( !provider ){
					writable = FALSE;
					reason = IIO_PROVIDER_STATUS_NO_PROVIDER_FOUND;
				}
			}
		}

		/* if needed, the level zero must be writable
		 */
		if( writable ){
			parent = ( FMAObjectItem * ) fma_object_get_parent( item );
			if( !parent ){
				if( updater->private->is_level_zero_writable ){
					reason = IIO_PROVIDER_STATUS_LEVEL_ZERO;
				}
			}
		}
	}

	fma_object_set_writability_status( item, writable, reason );
}

/*
 * fma_updater_are_preferences_locked:
 * @updater: the #FMAUpdater application object.
 *
 * Returns: %TRUE if preferences have been globally locked down by an
 * admin, %FALSE else.
 */
gboolean
fma_updater_are_preferences_locked( const FMAUpdater *updater )
{
	gboolean are_locked;

	g_return_val_if_fail( FMA_IS_UPDATER( updater ), TRUE );

	are_locked = TRUE;

	if( !updater->private->dispose_has_run ){

		are_locked = updater->private->are_preferences_locked;
	}

	return( are_locked );
}

/*
 * fma_updater_is_level_zero_writable:
 * @updater: the #FMAUpdater application object.
 *
 * As of 3.1.0, level-zero is written as a user preference.
 *
 * This function considers that the level_zero is writable if it is not
 * a mandatory preference.
 * Whether preferences themselves are or not globally locked is not
 * considered here (as imho, level zero is not really and semantically
 * part of user preferences).
 *
 * This function only considers the case of the level zero itself.
 * It does not take into account whether the i/o provider (if any)
 * is writable, or if the item iself is not read only.
 *
 * Returns: %TRUE if we are able to update the level-zero list of items,
 * %FALSE else.
 */
gboolean
fma_updater_is_level_zero_writable( const FMAUpdater *updater )
{
	gboolean is_writable;

	g_return_val_if_fail( FMA_IS_UPDATER( updater ), FALSE );

	is_writable = FALSE;

	if( !updater->private->dispose_has_run ){

		is_writable = updater->private->is_level_zero_writable;
	}

	return( is_writable );
}

/*
 * fma_updater_append_item:
 * @updater: this #FMAUpdater object.
 * @item: a #FMAObjectItem-derived object to be appended to the tree.
 *
 * Append a new item at the end of the global tree.
 */
void
fma_updater_append_item( FMAUpdater *updater, FMAObjectItem *item )
{
	GList *tree;

	g_return_if_fail( FMA_IS_UPDATER( updater ));
	g_return_if_fail( FMA_IS_OBJECT_ITEM( item ));

	if( !updater->private->dispose_has_run ){

		g_object_get( G_OBJECT( updater ), PIVOT_PROP_TREE, &tree, NULL );
		tree = g_list_append( tree, item );
		g_object_set( G_OBJECT( updater ), PIVOT_PROP_TREE, tree, NULL );
	}
}

/*
 * fma_updater_insert_item:
 * @updater: this #FMAUpdater object.
 * @item: a #FMAObjectItem-derived object to be inserted in the tree.
 * @parent_id: the id of the parent, or %NULL.
 * @pos: the position in the children of the parent, starting at zero, or -1.
 *
 * Insert a new item in the global tree.
 */
void
fma_updater_insert_item( FMAUpdater *updater, FMAObjectItem *item, const gchar *parent_id, gint pos )
{
	GList *tree;
	FMAObjectItem *parent;

	g_return_if_fail( FMA_IS_UPDATER( updater ));
	g_return_if_fail( FMA_IS_OBJECT_ITEM( item ));

	if( !updater->private->dispose_has_run ){

		parent = NULL;
		g_object_get( G_OBJECT( updater ), PIVOT_PROP_TREE, &tree, NULL );

		if( parent_id ){
			parent = fma_pivot_get_item( FMA_PIVOT( updater ), parent_id );
		}

		if( parent ){
			fma_object_insert_at( parent, item, pos );

		} else {
			tree = g_list_append( tree, item );
			g_object_set( G_OBJECT( updater ), PIVOT_PROP_TREE, tree, NULL );
		}
	}
}

/*
 * fma_updater_remove_item:
 * @updater: this #FMAPivot instance.
 * @item: the #FMAObjectItem to be removed from the list.
 *
 * Removes a #FMAObjectItem from the hierarchical tree. Does not delete it.
 */
void
fma_updater_remove_item( FMAUpdater *updater, FMAObject *item )
{
	GList *tree;
	FMAObjectItem *parent;

	g_return_if_fail( FMA_IS_PIVOT( updater ));

	if( !updater->private->dispose_has_run ){

		g_debug( "fma_updater_remove_item: updater=%p, item=%p (%s)",
				( void * ) updater,
				( void * ) item, G_IS_OBJECT( item ) ? G_OBJECT_TYPE_NAME( item ) : "(null)" );

		parent = fma_object_get_parent( item );
		if( parent ){
			tree = fma_object_get_items( parent );
			tree = g_list_remove( tree, ( gconstpointer ) item );
			fma_object_set_items( parent, tree );

		} else {
			g_object_get( G_OBJECT( updater ), PIVOT_PROP_TREE, &tree, NULL );
			tree = g_list_remove( tree, ( gconstpointer ) item );
			g_object_set( G_OBJECT( updater ), PIVOT_PROP_TREE, tree, NULL );
		}
	}
}

/**
 * fma_updater_should_pasted_be_relabeled:
 * @updater: this #FMAUpdater instance.
 * @object: the considered #FMAObject-derived object.
 *
 * Whether the specified object should be relabeled when pasted ?
 *
 * Returns: %TRUE if the object should be relabeled, %FALSE else.
 */
gboolean
fma_updater_should_pasted_be_relabeled( const FMAUpdater *updater, const FMAObject *item )
{
	static const gchar *thisfn = "fma_updater_should_pasted_be_relabeled";
	gboolean relabel;

	if( FMA_IS_OBJECT_MENU( item )){
		relabel = fma_settings_get_boolean( IPREFS_RELABEL_DUPLICATE_MENU, NULL, NULL );

	} else if( FMA_IS_OBJECT_ACTION( item )){
		relabel = fma_settings_get_boolean( IPREFS_RELABEL_DUPLICATE_ACTION, NULL, NULL );

	} else if( FMA_IS_OBJECT_PROFILE( item )){
		relabel = fma_settings_get_boolean( IPREFS_RELABEL_DUPLICATE_PROFILE, NULL, NULL );

	} else {
		g_warning( "%s: unknown item type at %p", thisfn, ( void * ) item );
		g_return_val_if_reached( FALSE );
	}

	return( relabel );
}

/*
 * fma_updater_load_items:
 * @updater: this #FMAUpdater instance.
 *
 * Loads the items, updating simultaneously their writability status.
 *
 * Returns: a pointer (not a ref) on the loaded tree.
 *
 * Since: 3.1
 */
GList *
fma_updater_load_items( FMAUpdater *updater )
{
	static const gchar *thisfn = "fma_updater_load_items";
	GList *tree;

	g_return_val_if_fail( FMA_IS_UPDATER( updater ), NULL );

	tree = NULL;

	if( !updater->private->dispose_has_run ){
		g_debug( "%s: updater=%p (%s)", thisfn, ( void * ) updater, G_OBJECT_TYPE_NAME( updater ));

		fma_pivot_load_items( FMA_PIVOT( updater ));
		tree = fma_pivot_get_items( FMA_PIVOT( updater ));
		g_list_foreach( tree, ( GFunc ) set_writability_status, ( gpointer ) updater );
	}

	return( tree );
}

static void
set_writability_status( FMAObjectItem *item, const FMAUpdater *updater )
{
	GList *children;

	fma_updater_check_item_writability_status( updater, item );

	if( FMA_IS_OBJECT_MENU( item )){
		children = fma_object_get_items( item );
		g_list_foreach( children, ( GFunc ) set_writability_status, ( gpointer ) updater );
	}
}

/*
 * fma_updater_write_item:
 * @updater: this #FMAUpdater instance.
 * @item: a #FMAObjectItem to be written down to the storage subsystem.
 * @messages: the I/O provider can allocate and store here its error
 * messages.
 *
 * Writes an item (an action or a menu).
 *
 * Returns: the #FMAIIOProvider return code.
 */
guint
fma_updater_write_item( const FMAUpdater *updater, FMAObjectItem *item, GSList **messages )
{
	guint ret;

	ret = IIO_PROVIDER_CODE_PROGRAM_ERROR;

	g_return_val_if_fail( FMA_IS_UPDATER( updater ), ret );
	g_return_val_if_fail( FMA_IS_OBJECT_ITEM( item ), ret );
	g_return_val_if_fail( messages, ret );

	if( !updater->private->dispose_has_run ){

		FMAIOProvider *provider = fma_object_get_provider( item );

		if( !provider ){
			provider = fma_io_provider_find_writable_io_provider( FMA_PIVOT( updater ));
			g_return_val_if_fail( provider, IIO_PROVIDER_STATUS_NO_PROVIDER_FOUND );
		}

		if( provider ){
			ret = fma_io_provider_write_item( provider, item, messages );
		}
	}

	return( ret );
}

/*
 * fma_updater_delete_item:
 * @updater: this #FMAUpdater instance.
 * @item: the #FMAObjectItem to be deleted from the storage subsystem.
 * @messages: the I/O provider can allocate and store here its error
 * messages.
 *
 * Deletes an item, action or menu, from the I/O storage subsystem.
 *
 * Returns: the #FMAIIOProvider return code.
 *
 * Note that a new item, not already written to an I/O subsystem,
 * doesn't have any attached provider. We so do nothing and return OK...
 */
guint
fma_updater_delete_item( const FMAUpdater *updater, const FMAObjectItem *item, GSList **messages )
{
	guint ret;
	FMAIOProvider *provider;

	g_return_val_if_fail( FMA_IS_UPDATER( updater ), IIO_PROVIDER_CODE_PROGRAM_ERROR );
	g_return_val_if_fail( FMA_IS_OBJECT_ITEM( item ), IIO_PROVIDER_CODE_PROGRAM_ERROR );
	g_return_val_if_fail( messages, IIO_PROVIDER_CODE_PROGRAM_ERROR );

	ret = IIO_PROVIDER_CODE_OK;

	if( !updater->private->dispose_has_run ){

		provider = fma_object_get_provider( item );

		/* provider may be NULL if the item has been deleted from the UI
		 * without having been ever saved
		 */
		if( provider ){
			g_return_val_if_fail( FMA_IS_IO_PROVIDER( provider ), IIO_PROVIDER_CODE_PROGRAM_ERROR );
			ret = fma_io_provider_delete_item( provider, item, messages );
		}
	}

	return( ret );
}
