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

#include <errno.h>

#include "nadp-desktop-file.h"
#include "nadp-desktop-provider.h"
#include "nadp-write.h"
#include "nadp-utils.h"
#include "nadp-xdg-data-dirs.h"

static guint write_item( const NAIIOProvider *provider, const NAObjectItem *item, NadpDesktopFile *ndf, GSList **messages );

/*
 * API function: should only be called through NAIIOProvider interface
 */
gboolean
nadp_iio_provider_is_willing_to_write( const NAIIOProvider *provider )
{
	return( TRUE );
}

/*
 * NadpDesktopProvider is able to write if user data dir exists (or
 * can be created) and is writable
 *
 * API function: should only be called through NAIIOProvider interface
 */
gboolean
nadp_iio_provider_is_able_to_write( const NAIIOProvider *provider )
{
	static const gchar *thisfn = "nadp_write_iio_provider_is_able_to_write";
	gboolean able_to;
	gchar *userdir;
	GSList *messages;

	able_to = FALSE;
	messages = NULL;

	g_return_val_if_fail( NADP_IS_DESKTOP_PROVIDER( provider ), able_to );

	userdir = nadp_xdg_data_dirs_get_user_dir( NADP_DESKTOP_PROVIDER( provider ), &messages );

	if( g_file_test( userdir, G_FILE_TEST_IS_DIR )){
		able_to = nadp_utils_is_writable_dir( userdir );

	} else if( g_mkdir_with_parents( userdir, 0700 )){
		g_warning( "%s: %s: %s", thisfn, userdir, g_strerror( errno ));

	} else {
		able_to = nadp_utils_is_writable_dir( userdir );
	}

	g_free( userdir );

	return( able_to );
}

/*
 * the item comes from being readen from a desktop file
 * -> see if this desktop file is writable ?
 *
 * This is only used to setup the 'read-only' initial status of the
 * NAObjectItem - We don't care of all events which can suddenly make
 * this item becomes readonly (eventually we will deal for errors,
 * and reset the flag at this time)
 *
 * Internal function: do not call from outside the instance.
 */
gboolean
nadp_iio_provider_is_writable( const NAIIOProvider *provider, const NAObjectItem *item )
{
	static const gchar *thisfn = "nadp_iio_provider_is_writable";
	gboolean writable;
	NadpDesktopFile *ndf;
	gchar *path;

	writable = FALSE;
	g_return_val_if_fail( NADP_IS_DESKTOP_PROVIDER( provider ), writable );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), writable );

	if( NA_IS_OBJECT_MENU( item )){
		g_warning( "%s: menu are not yet handled by Desktop provider", thisfn );
		return( FALSE );
	}

	ndf = ( NadpDesktopFile * ) na_object_get_provider_data( item );

	if( ndf ){
		g_return_val_if_fail( NADP_IS_DESKTOP_FILE( ndf ), writable );
		path = nadp_desktop_file_get_key_file_path( ndf );
		writable = nadp_utils_is_writable_file( path );
		g_free( path );
	}

	return( writable );
}

guint
nadp_iio_provider_write_item( const NAIIOProvider *provider, const NAObjectItem *item, GSList **messages )
{
	static const gchar *thisfn = "nadp_iio_provider_write_item";
	guint ret;
	NadpDesktopFile *ndf;
	gchar *path;
	gchar *userdir;
	gchar *id;
	gchar *bname;
	GSList *subdirs;
	gchar *fulldir;
	gboolean dir_ok;

	ret = NA_IIO_PROVIDER_CODE_PROGRAM_ERROR;

	g_return_val_if_fail( NADP_IS_DESKTOP_PROVIDER( provider ), ret );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), ret );

	if( na_object_is_readonly( item )){
		g_warning( "%s: item=%p is read-only", thisfn, ( void * ) item );
		return( ret );
	}

	ndf = ( NadpDesktopFile * ) na_object_get_provider_data( item );

	/* write into the current key file and write it to current path */
	if( ndf ){
		g_return_val_if_fail( NADP_IS_DESKTOP_FILE( ndf ), ret );

	} else {
		userdir = nadp_xdg_data_dirs_get_user_dir( NADP_DESKTOP_PROVIDER( provider ), messages );
		subdirs = nadp_utils_split_path_list( NADP_DESKTOP_PROVIDER_SUBDIRS );
		fulldir = g_build_filename( userdir, ( gchar * ) subdirs->data, NULL );
		dir_ok = TRUE;
		if( !g_file_test( fulldir, G_FILE_TEST_IS_DIR )){
			if( g_mkdir_with_parents( fulldir, 0700 )){
				g_warning( "%s: %s: %s", thisfn, userdir, g_strerror( errno ));
				dir_ok = FALSE;
			}
		}
		g_free( userdir );
		nadp_utils_gslist_free( subdirs );

		if( dir_ok ){
			id = na_object_get_id( item );
			bname = g_strdup_printf( "%s%s", id, NADP_DESKTOP_SUFFIX );
			g_free( id );
			path = g_build_filename( fulldir, bname, NULL );
			g_free( bname );
		}
		g_free( fulldir );

		if( dir_ok ){
			ndf = nadp_desktop_file_new_for_write( path );
			g_object_set_data( G_OBJECT( item ), "nadp-desktop-file", ndf );
			g_object_weak_ref( G_OBJECT( item ), ( GWeakNotify ) g_object_unref, ndf );
			g_free( path );
		}
	}

	if( ndf ){
		ret = write_item( provider, item, ndf, messages );
	}

	return( ret );
}

/*
 *
 */
static guint
write_item( const NAIIOProvider *provider, const NAObjectItem *item, NadpDesktopFile *ndf, GSList **messages )
{
	static const gchar *thisfn = "nadp_iio_provider_write_item";
	guint ret;
	NadpDesktopProvider *self;
	gchar *label;
	gchar *tooltip;
	gchar *icon;
	gboolean enabled;

	g_debug( "%s: provider=%p (%s), item=%p (%s), messages=%p",
			thisfn,
			( void * ) provider, G_OBJECT_TYPE_NAME( provider ),
			( void * ) item, G_OBJECT_TYPE_NAME( item ),
			( void * ) messages );

	ret = NA_IIO_PROVIDER_CODE_PROGRAM_ERROR;

	g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider ), ret );
	g_return_val_if_fail( NADP_IS_DESKTOP_PROVIDER( provider ), ret );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), ret );

	self = NADP_DESKTOP_PROVIDER( provider );

	if( self->private->dispose_has_run ){
		return( NA_IIO_PROVIDER_CODE_NOT_WILLING_TO_RUN );
	}

	ret = NA_IIO_PROVIDER_CODE_OK;

	label = na_object_get_label( item );
	nadp_desktop_file_set_label( ndf, label );
	g_free( label );

	tooltip = na_object_get_tooltip( item );
	nadp_desktop_file_set_tooltip( ndf, tooltip );
	g_free( tooltip );

	icon = na_object_get_icon( item );
	nadp_desktop_file_set_icon( ndf, icon );
	g_free( icon );

	enabled = na_object_is_enabled( item );
	nadp_desktop_file_set_enabled( ndf, enabled );

	if( !nadp_desktop_file_write( ndf )){
		ret = NA_IIO_PROVIDER_CODE_WRITE_ERROR;
	}

	return( ret );
}

guint
nadp_iio_provider_delete_item( const NAIIOProvider *provider, const NAObjectItem *item, GSList **messages )
{
	static const gchar *thisfn = "nadp_iio_provider_delete_item";
	guint ret;
	NadpDesktopProvider *self;
	NadpDesktopFile *ndf;
	gchar *path;

	g_debug( "%s: provider=%p (%s), item=%p (%s), messages=%p",
			thisfn,
			( void * ) provider, G_OBJECT_TYPE_NAME( provider ),
			( void * ) item, G_OBJECT_TYPE_NAME( item ),
			( void * ) messages );

	ret = NA_IIO_PROVIDER_CODE_PROGRAM_ERROR;

	g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider ), ret );
	g_return_val_if_fail( NADP_IS_DESKTOP_PROVIDER( provider ), ret );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), ret );

	self = NADP_DESKTOP_PROVIDER( provider );

	if( self->private->dispose_has_run ){
		return( NA_IIO_PROVIDER_CODE_NOT_WILLING_TO_RUN );
	}

	ndf = ( NadpDesktopFile * ) na_object_get_provider_data( item );

	if( ndf ){
		g_return_val_if_fail( NADP_IS_DESKTOP_FILE( ndf ), ret );
		path = nadp_desktop_file_get_key_file_path( ndf );
		if( nadp_utils_delete_file( path )){
			ret = NA_IIO_PROVIDER_CODE_OK;
		}
		g_free( path );

	} else {
		g_warning( "%s: NadpDesktopFile is null", thisfn );
		ret = NA_IIO_PROVIDER_CODE_OK;
	}

	return( ret );
}
