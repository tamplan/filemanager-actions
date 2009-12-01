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

#include <errno.h>

#include "nadp-desktop-file.h"
#include "nadp-desktop-provider.h"
#include "nadp-write.h"
#include "nadp-utils.h"
#include "nadp-xdg-data-dirs.h"

static guint write_item( const NAIIOProvider *provider, const NAObjectItem *item, NadpDesktopFile *ndf, GSList **messages );

/*
 * NadpDesktopProvider is willing to write if user data dir exists (or
 * can be created) and is writable
 */
gboolean
nadp_iio_provider_is_willing_to_write( const NAIIOProvider *provider )
{
	static const gchar *thisfn = "nadp_write_iio_provider_is_willing_to_write";
	gboolean willing_to;
	gchar *userdir;
	GSList *messages;

	willing_to = FALSE;
	messages = NULL;
	g_debug( "%s: provider=%p", thisfn, ( void * ) provider );
	g_return_val_if_fail( NADP_IS_DESKTOP_PROVIDER( provider ), willing_to );

	userdir = nadp_xdg_data_dirs_get_user_dir( NADP_DESKTOP_PROVIDER( provider ), &messages );

	if( g_file_test( userdir, G_FILE_TEST_IS_DIR )){
		willing_to = nadp_utils_is_writable_dir( userdir );

	} else if( g_mkdir_with_parents( userdir, 0700 )){
		g_warning( "%s: %s: %s", thisfn, userdir, g_strerror( errno ));

	} else {
		willing_to = nadp_utils_is_writable_dir( userdir );
	}

	g_free( userdir );

	return( willing_to );
}

/*
 * the item has been initially readen from a desktop file
 * -> see if this initial desktop file is writable ?
 *
 * else we are going to write a new desktop file in user data dir
 * -> see writability status of the desktop provider
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

	ndf = ( NadpDesktopFile * ) g_object_get_data( G_OBJECT( item ), "nadp-desktop-file" );

	if( ndf ){
		g_return_val_if_fail( NADP_IS_DESKTOP_FILE( ndf ), writable );
		path = nadp_desktop_file_get_key_file_path( ndf );
		writable = nadp_utils_is_writable_file( path );
		g_free( path );

	} else {
		writable = nadp_iio_provider_is_willing_to_write( provider );
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

	ret = NA_IIO_PROVIDER_NOT_WRITABLE;
	g_return_val_if_fail( NADP_IS_DESKTOP_PROVIDER( provider ), ret );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), ret );

	if( na_object_is_readonly( item )){
		g_warning( "%s: item=%p is read-only", thisfn, ( void * ) item );
		return( ret );
	}

	ndf = ( NadpDesktopFile * ) g_object_get_data( G_OBJECT( item ), "nadp-desktop-file" );

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
	guint ret;
	gchar *label;
	gchar *tooltip;

	ret = NA_IIO_PROVIDER_WRITE_OK;

	label = na_object_get_label( item );
	nadp_desktop_file_set_label( ndf, label );
	g_free( label );

	tooltip = na_object_get_tooltip( item );
	nadp_desktop_file_set_tooltip( ndf, tooltip );
	g_free( tooltip );

	if( !nadp_desktop_file_write( ndf )){
		ret = NA_IIO_PROVIDER_WRITE_ERROR;
	}

	return( ret );
}

guint
nadp_iio_provider_delete_item( const NAIIOProvider *provider, const NAObjectItem *item, GSList **messages )
{
	static const gchar *thisfn = "nadp_iio_provider_delete_item";
	guint ret;
	NadpDesktopFile *ndf;
	gchar *path;

	ret = NA_IIO_PROVIDER_NOT_WILLING_TO_WRITE;
	g_return_val_if_fail( NADP_IS_DESKTOP_PROVIDER( provider ), ret );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), ret );

	if( na_object_is_readonly( item )){
		g_warning( "%s: item=%p is read-only", thisfn, ( void * ) item );
		return( NA_IIO_PROVIDER_NOT_WRITABLE );
	}

	ndf = ( NadpDesktopFile * ) g_object_get_data( G_OBJECT( item ), "nadp-desktop-file" );

	if( ndf ){
		g_return_val_if_fail( NADP_IS_DESKTOP_FILE( ndf ), ret );
		path = nadp_desktop_file_get_key_file_path( ndf );
		if( nadp_utils_delete_file( path )){
			ret = NA_IIO_PROVIDER_WRITE_OK;
		}
		g_free( path );

	} else {
		ret = NA_IIO_PROVIDER_WRITE_OK;
	}

	return( ret );
}
