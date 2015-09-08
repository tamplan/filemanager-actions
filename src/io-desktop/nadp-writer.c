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

#include <errno.h>
#include <string.h>

#include <api/fma-core-utils.h>
#include <api/fma-data-types.h>
#include <api/fma-object-api.h>
#include <api/fma-ifactory-provider.h>

#include "nadp-desktop-file.h"
#include "nadp-desktop-provider.h"
#include "nadp-formats.h"
#include "nadp-keys.h"
#include "nadp-utils.h"
#include "nadp-writer.h"
#include "nadp-xdg-dirs.h"

/* the association between an export format and the functions
 */
typedef struct {
	gchar  *format;
	void   *foo;
}
	ExportFormatFn;

static ExportFormatFn st_export_format_fn[] = {

	{ NADP_FORMAT_DESKTOP_V1,
					NULL },

	{ NULL }
};

static guint           write_item( const FMAIIOProvider *provider, const FMAObjectItem *item, FMADesktopFile *ndf, GSList **messages );

static void            desktop_weak_notify( FMADesktopFile *ndf, GObject *item );

static void            write_start_write_type( FMADesktopFile *ndp, FMAObjectItem *item );
static void            write_done_write_subitems_list( FMADesktopFile *ndp, FMAObjectItem *item );

static ExportFormatFn *find_export_format_fn( const gchar *format );

#ifdef NA_ENABLE_DEPRECATED
static ExportFormatFn *find_export_format_fn_from_quark( GQuark format );
#endif

/*
 * This is implementation of FMAIIOProvider::is_willing_to_write method
 */
gboolean
nadp_iio_provider_is_willing_to_write( const FMAIIOProvider *provider )
{
	return( TRUE );
}

/*
 * NadpDesktopProvider is able to write if user data dir exists (or
 * can be created) and is writable
 *
 * This is implementation of FMAIIOProvider::is_able_to_write method
 */
gboolean
nadp_iio_provider_is_able_to_write( const FMAIIOProvider *provider )
{
	static const gchar *thisfn = "nadp_writer_iio_provider_is_able_to_write";
	gboolean able_to;
	gchar *userdir;

	g_return_val_if_fail( NADP_IS_DESKTOP_PROVIDER( provider ), FALSE );

	able_to = FALSE;

	userdir = nadp_xdg_dirs_get_user_data_dir();

	if( g_file_test( userdir, G_FILE_TEST_IS_DIR )){
		able_to = fma_core_utils_dir_is_writable_path( userdir );

	} else if( g_mkdir_with_parents( userdir, 0750 )){
		g_warning( "%s: %s: %s", thisfn, userdir, g_strerror( errno ));

	} else {
		fma_core_utils_dir_list_perms( userdir, thisfn );
		able_to = fma_core_utils_dir_is_writable_path( userdir );
	}

	g_free( userdir );

	return( able_to );
}

/*
 * This is implementation of FMAIIOProvider::write_item method
 */
guint
nadp_iio_provider_write_item( const FMAIIOProvider *provider, const FMAObjectItem *item, GSList **messages )
{
	static const gchar *thisfn = "nadp_iio_provider_write_item";
	guint ret;
	FMADesktopFile *ndf;
	gchar *path;
	gchar *userdir;
	gchar *id;
	gchar *bname;
	GSList *subdirs;
	gchar *fulldir;
	gboolean dir_ok;

	ret = IIO_PROVIDER_CODE_PROGRAM_ERROR;

	g_return_val_if_fail( NADP_IS_DESKTOP_PROVIDER( provider ), ret );
	g_return_val_if_fail( FMA_IS_OBJECT_ITEM( item ), ret );

	if( fma_object_is_readonly( item )){
		g_warning( "%s: item=%p is read-only", thisfn, ( void * ) item );
		return( ret );
	}

	ndf = ( FMADesktopFile * ) fma_object_get_provider_data( item );

	/* write into the current key file and write it to current path */
	if( ndf ){
		g_return_val_if_fail( FMA_IS_DESKTOP_FILE( ndf ), ret );

	} else {
		userdir = nadp_xdg_dirs_get_user_data_dir();
		subdirs = fma_core_utils_slist_from_split( NADP_DESKTOP_PROVIDER_SUBDIRS, G_SEARCHPATH_SEPARATOR_S );
		fulldir = g_build_filename( userdir, ( gchar * ) subdirs->data, NULL );
		dir_ok = TRUE;

		if( !g_file_test( fulldir, G_FILE_TEST_IS_DIR )){
			if( g_mkdir_with_parents( fulldir, 0750 )){
				g_warning( "%s: %s: %s", thisfn, userdir, g_strerror( errno ));
				dir_ok = FALSE;
			} else {
				fma_core_utils_dir_list_perms( userdir, thisfn );
			}
		}
		g_free( userdir );
		fma_core_utils_slist_free( subdirs );

		if( dir_ok ){
			id = fma_object_get_id( item );
			bname = g_strdup_printf( "%s%s", id, FMA_DESKTOP_FILE_SUFFIX );
			g_free( id );
			path = g_build_filename( fulldir, bname, NULL );
			g_free( bname );
		}
		g_free( fulldir );

		if( dir_ok ){
			ndf = fma_desktop_file_new_for_write( path );
			fma_object_set_provider_data( item, ndf );
			g_object_weak_ref( G_OBJECT( item ), ( GWeakNotify ) desktop_weak_notify, ndf );
			g_free( path );
		}
	}

	if( ndf ){
		ret = write_item( provider, item, ndf, messages );
	}

	return( ret );
}

/*
 * actually writes the item to the existing FMADesktopFile
 * as we have chosen to take advantage of data factory management system
 * we do not need to enumerate each and every elementary data
 *
 * As we want keep comments between through multiple updates, we cannot
 * just delete the .desktop file and recreate it as we are doing for GConf.
 * Instead of that, we delete at end groups that have not been walked through
 * -> as a side effect, we lose comments inside of these groups :(
 */
static guint
write_item( const FMAIIOProvider *provider, const FMAObjectItem *item, FMADesktopFile *ndf, GSList **messages )
{
	static const gchar *thisfn = "nadp_iio_provider_write_item";
	guint ret;
	NadpDesktopProvider *self;

	g_debug( "%s: provider=%p (%s), item=%p (%s), ndf=%p, messages=%p",
			thisfn,
			( void * ) provider, G_OBJECT_TYPE_NAME( provider ),
			( void * ) item, G_OBJECT_TYPE_NAME( item ),
			( void * ) ndf,
			( void * ) messages );

	ret = IIO_PROVIDER_CODE_PROGRAM_ERROR;

	g_return_val_if_fail( FMA_IS_IIO_PROVIDER( provider ), ret );
	g_return_val_if_fail( NADP_IS_DESKTOP_PROVIDER( provider ), ret );
	g_return_val_if_fail( FMA_IS_IFACTORY_PROVIDER( provider ), ret );

	g_return_val_if_fail( FMA_IS_OBJECT_ITEM( item ), ret );
	g_return_val_if_fail( FMA_IS_IFACTORY_OBJECT( item ), ret );

	g_return_val_if_fail( FMA_IS_DESKTOP_FILE( ndf ), ret );

	self = NADP_DESKTOP_PROVIDER( provider );

	if( self->private->dispose_has_run ){
		return( IIO_PROVIDER_CODE_NOT_WILLING_TO_RUN );
	}

	ret = IIO_PROVIDER_CODE_OK;

	fma_ifactory_provider_write_item( FMA_IFACTORY_PROVIDER( provider ), ndf, FMA_IFACTORY_OBJECT( item ), messages );

	if( !fma_desktop_file_write( ndf )){
		ret = IIO_PROVIDER_CODE_WRITE_ERROR;
	}

	return( ret );
}

guint
nadp_iio_provider_delete_item( const FMAIIOProvider *provider, const FMAObjectItem *item, GSList **messages )
{
	static const gchar *thisfn = "nadp_iio_provider_delete_item";
	guint ret;
	NadpDesktopProvider *self;
	FMADesktopFile *ndf;
	gchar *uri;

	g_debug( "%s: provider=%p (%s), item=%p (%s), messages=%p",
			thisfn,
			( void * ) provider, G_OBJECT_TYPE_NAME( provider ),
			( void * ) item, G_OBJECT_TYPE_NAME( item ),
			( void * ) messages );

	ret = IIO_PROVIDER_CODE_PROGRAM_ERROR;

	g_return_val_if_fail( FMA_IS_IIO_PROVIDER( provider ), ret );
	g_return_val_if_fail( NADP_IS_DESKTOP_PROVIDER( provider ), ret );
	g_return_val_if_fail( FMA_IS_OBJECT_ITEM( item ), ret );

	self = NADP_DESKTOP_PROVIDER( provider );

	if( self->private->dispose_has_run ){
		return( IIO_PROVIDER_CODE_NOT_WILLING_TO_RUN );
	}

	ndf = ( FMADesktopFile * ) fma_object_get_provider_data( item );

	if( ndf ){
		g_return_val_if_fail( FMA_IS_DESKTOP_FILE( ndf ), ret );
		uri = fma_desktop_file_get_key_file_uri( ndf );
		if( nadp_utils_uri_delete( uri )){
			ret = IIO_PROVIDER_CODE_OK;
		}
		g_free( uri );

	} else {
		g_warning( "%s: FMADesktopFile is null", thisfn );
		ret = IIO_PROVIDER_CODE_OK;
	}

	return( ret );
}

static void
desktop_weak_notify( FMADesktopFile *ndf, GObject *item )
{
	static const gchar *thisfn = "nadp_writer_desktop_weak_notify";

	g_debug( "%s: ndf=%p (%s), item=%p (%s)",
			thisfn, ( void * ) ndf, G_OBJECT_TYPE_NAME( ndf ),
			( void * ) item, G_OBJECT_TYPE_NAME( item ));

	g_object_unref( ndf );
}

/*
 * Implementation of FMAIIOProvider::duplicate_data
 * Add a ref on FMADesktopFile data, so that unreffing origin object in NACT
 * does not invalid duplicated pointer
 */
guint
nadp_iio_provider_duplicate_data( const FMAIIOProvider *provider, FMAObjectItem *dest, const FMAObjectItem *source, GSList **messages )
{
	static const gchar *thisfn = "nadp_iio_provider_duplicate_data";
	guint ret;
	NadpDesktopProvider *self;
	FMADesktopFile *ndf;

	g_debug( "%s: provider=%p (%s), dest=%p (%s), source=%p (%s), messages=%p",
			thisfn,
			( void * ) provider, G_OBJECT_TYPE_NAME( provider ),
			( void * ) dest, G_OBJECT_TYPE_NAME( dest ),
			( void * ) source, G_OBJECT_TYPE_NAME( source ),
			( void * ) messages );

	ret = IIO_PROVIDER_CODE_PROGRAM_ERROR;

	g_return_val_if_fail( FMA_IS_IIO_PROVIDER( provider ), ret );
	g_return_val_if_fail( NADP_IS_DESKTOP_PROVIDER( provider ), ret );
	g_return_val_if_fail( FMA_IS_OBJECT_ITEM( dest ), ret );
	g_return_val_if_fail( FMA_IS_OBJECT_ITEM( source ), ret );

	self = NADP_DESKTOP_PROVIDER( provider );

	if( self->private->dispose_has_run ){
		return( IIO_PROVIDER_CODE_NOT_WILLING_TO_RUN );
	}

	ndf = ( FMADesktopFile * ) fma_object_get_provider_data( source );
	g_return_val_if_fail( ndf && FMA_IS_DESKTOP_FILE( ndf ), ret );
	fma_object_set_provider_data( dest, g_object_ref( ndf ));
	g_object_weak_ref( G_OBJECT( dest ), ( GWeakNotify ) desktop_weak_notify, ndf );

	return( IIO_PROVIDER_CODE_OK );
}

/**
 * nadp_writer_iexporter_export_to_buffer:
 * @instance: this #FMAIExporter instance.
 * @parms: a #FMAIExporterBufferParmsv2 structure.
 *
 * Export the specified 'item' to a newly allocated buffer.
 */
guint
nadp_writer_iexporter_export_to_buffer( const FMAIExporter *instance, FMAIExporterBufferParmsv2 *parms )
{
	static const gchar *thisfn = "nadp_writer_iexporter_export_to_buffer";
	guint code, write_code;
	ExportFormatFn *fmt;
	GKeyFile *key_file;
	FMADesktopFile *ndf;

	g_debug( "%s: instance=%p, parms=%p", thisfn, ( void * ) instance, ( void * ) parms );

	parms->buffer = NULL;
	code = FMA_IEXPORTER_CODE_OK;

	if( !parms->exported || !FMA_IS_OBJECT_ITEM( parms->exported )){
		code = FMA_IEXPORTER_CODE_INVALID_ITEM;
	}

	if( code == FMA_IEXPORTER_CODE_OK ){

#ifdef NA_ENABLE_DEPRECATED
		if( parms->version == 1 ){
			fmt = find_export_format_fn_from_quark((( FMAIExporterBufferParms * ) parms )->format );
		} else {
			fmt = find_export_format_fn( parms->format );
		}
#else
		fmt = find_export_format_fn( parms->format );
#endif

		if( !fmt ){
			code = FMA_IEXPORTER_CODE_INVALID_FORMAT;

		} else {
			ndf = fma_desktop_file_new();
			write_code = fma_ifactory_provider_write_item( FMA_IFACTORY_PROVIDER( instance ), ndf, FMA_IFACTORY_OBJECT( parms->exported ), &parms->messages );

			if( write_code != IIO_PROVIDER_CODE_OK ){
				code = FMA_IEXPORTER_CODE_ERROR;

			} else {
				key_file = fma_desktop_file_get_key_file( ndf );
				parms->buffer = g_key_file_to_data( key_file, NULL, NULL );
			}

			g_object_unref( ndf );
		}
	}

	g_debug( "%s: returning code=%u", thisfn, code );
	return( code );
}

/**
 * nadp_writer_iexporter_export_to_file:
 * @instance: this #FMAIExporter instance.
 * @parms: a #FMAIExporterFileParmsv2 structure.
 *
 * Export the specified 'item' to a newly created file.
 */
guint
nadp_writer_iexporter_export_to_file( const FMAIExporter *instance, FMAIExporterFileParmsv2 *parms )
{
	static const gchar *thisfn = "nadp_writer_iexporter_export_to_file";
	guint code, write_code;
	gchar *id, *folder_path, *dest_path;
	ExportFormatFn *fmt;
	FMADesktopFile *ndf;

	g_debug( "%s: instance=%p, parms=%p", thisfn, ( void * ) instance, ( void * ) parms );

	parms->basename = NULL;
	code = FMA_IEXPORTER_CODE_OK;

	if( !parms->exported || !FMA_IS_OBJECT_ITEM( parms->exported )){
		code = FMA_IEXPORTER_CODE_INVALID_ITEM;
	}

	if( code == FMA_IEXPORTER_CODE_OK ){

#ifdef NA_ENABLE_DEPRECATED
		if( parms->version == 1 ){
			fmt = find_export_format_fn_from_quark((( FMAIExporterFileParms * ) parms )->format );
		} else {
			fmt = find_export_format_fn( parms->format );
		}
#else
		fmt = find_export_format_fn( parms->format );
#endif

		if( !fmt ){
			code = FMA_IEXPORTER_CODE_INVALID_FORMAT;

		} else {
			id = fma_object_get_id( parms->exported );
			parms->basename = g_strdup_printf( "%s%s", id, FMA_DESKTOP_FILE_SUFFIX );
			g_free( id );

			folder_path = g_filename_from_uri( parms->folder, NULL, NULL );
			dest_path = g_strdup_printf( "%s/%s", folder_path, parms->basename );
			g_free( folder_path );

			ndf = fma_desktop_file_new_for_write( dest_path );
			write_code = fma_ifactory_provider_write_item( FMA_IFACTORY_PROVIDER( instance ), ndf, FMA_IFACTORY_OBJECT( parms->exported ), &parms->messages );

			if( write_code != IIO_PROVIDER_CODE_OK ){
				code = FMA_IEXPORTER_CODE_ERROR;

			} else if( !fma_desktop_file_write( ndf )){
				code = FMA_IEXPORTER_CODE_UNABLE_TO_WRITE;
			}

			g_free( dest_path );
			g_object_unref( ndf );
		}
	}

	g_debug( "%s: returning code=%u", thisfn, code );
	return( code );
}

guint
nadp_writer_ifactory_provider_write_start( const FMAIFactoryProvider *provider, void *writer_data,
							const FMAIFactoryObject *object, GSList **messages  )
{
	if( FMA_IS_OBJECT_ITEM( object )){
		write_start_write_type( FMA_DESKTOP_FILE( writer_data ), FMA_OBJECT_ITEM( object ));
	}

	return( IIO_PROVIDER_CODE_OK );
}

static void
write_start_write_type( FMADesktopFile *ndp, FMAObjectItem *item )
{
	fma_desktop_file_set_string(
			ndp,
			NADP_GROUP_DESKTOP,
			NADP_KEY_TYPE,
			FMA_IS_OBJECT_ACTION( item ) ? NADP_VALUE_TYPE_ACTION : NADP_VALUE_TYPE_MENU );
}

/*
 * when writing to .desktop file a profile which has both a path and parameters,
 * then concatenate these two fields to the 'Exec' key
 */
guint
nadp_writer_ifactory_provider_write_data(
				const FMAIFactoryProvider *provider, void *writer_data, const FMAIFactoryObject *object,
				const FMADataBoxed *boxed, GSList **messages )
{
	static const gchar *thisfn = "nadp_writer_ifactory_provider_write_data";
	FMADesktopFile *ndf;
	guint code;
	const FMADataDef *def;
	gchar *profile_id;
	gchar *group_name;
	gchar *str_value;
	gboolean bool_value;
	GSList *slist_value;
	guint uint_value;
	gchar *parms, *tmp;

	g_return_val_if_fail( FMA_IS_DESKTOP_FILE( writer_data ), IIO_PROVIDER_CODE_PROGRAM_ERROR );
	/*g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));*/

	code = IIO_PROVIDER_CODE_OK;
	ndf = FMA_DESKTOP_FILE( writer_data );
	def = fma_data_boxed_get_data_def( boxed );

	if( def->desktop_entry && strlen( def->desktop_entry )){

		if( FMA_IS_OBJECT_PROFILE( object )){
			profile_id = fma_object_get_id( object );
			group_name = g_strdup_printf( "%s %s", NADP_GROUP_PROFILE, profile_id );
			g_free( profile_id );

		} else {
			group_name = g_strdup( NADP_GROUP_DESKTOP );
		}

		if( !fma_data_boxed_is_default( boxed ) || def->write_if_default ){

			switch( def->type ){

				case FMA_DATA_TYPE_STRING:
					str_value = fma_boxed_get_string( FMA_BOXED( boxed ));

					if( !strcmp( def->name, FMAFO_DATA_PATH )){
						parms = fma_object_get_parameters( object );
						tmp = g_strdup_printf( "%s %s", str_value, parms );
						g_free( str_value );
						g_free( parms );
						str_value = tmp;
					}

					fma_desktop_file_set_string( ndf, group_name, def->desktop_entry, str_value );
					g_free( str_value );
					break;

				case FMA_DATA_TYPE_LOCALE_STRING:
					str_value = fma_boxed_get_string( FMA_BOXED( boxed ));
					fma_desktop_file_set_locale_string( ndf, group_name, def->desktop_entry, str_value );
					g_free( str_value );
					break;

				case FMA_DATA_TYPE_BOOLEAN:
					bool_value = GPOINTER_TO_UINT( fma_boxed_get_as_void( FMA_BOXED( boxed )));
					fma_desktop_file_set_boolean( ndf, group_name, def->desktop_entry, bool_value );
					break;

				case FMA_DATA_TYPE_STRING_LIST:
					slist_value = ( GSList * ) fma_boxed_get_as_void( FMA_BOXED( boxed ));
					fma_desktop_file_set_string_list( ndf, group_name, def->desktop_entry, slist_value );
					fma_core_utils_slist_free( slist_value );
					break;

				case FMA_DATA_TYPE_UINT:
					uint_value = GPOINTER_TO_UINT( fma_boxed_get_as_void( FMA_BOXED( boxed )));
					fma_desktop_file_set_uint( ndf, group_name, def->desktop_entry, uint_value );
					break;

				default:
					g_warning( "%s: unknown type=%u for %s", thisfn, def->type, def->name );
					code = IIO_PROVIDER_CODE_PROGRAM_ERROR;
			}

		} else {
			fma_desktop_file_remove_key( ndf, group_name, def->desktop_entry );
		}

		g_free( group_name );
	}

	return( code );
}

guint
nadp_writer_ifactory_provider_write_done( const FMAIFactoryProvider *provider, void *writer_data,
							const FMAIFactoryObject *object, GSList **messages  )
{
	if( FMA_IS_OBJECT_ITEM( object )){
		write_done_write_subitems_list( FMA_DESKTOP_FILE( writer_data ), FMA_OBJECT_ITEM( object ));
	}

	return( IIO_PROVIDER_CODE_OK );
}

static void
write_done_write_subitems_list( FMADesktopFile *ndp, FMAObjectItem *item )
{
	static const gchar *thisfn = "nadp_writer_write_done_write_subitems_list";
	GSList *subitems;
	GSList *profile_groups, *ip;
	gchar *tmp;

	subitems = fma_object_get_items_slist( item );
	tmp = g_strdup_printf( "%s (written subitems)", thisfn );
	fma_core_utils_slist_dump( tmp, subitems );
	g_free( tmp );

	fma_desktop_file_set_string_list(
			ndp,
			NADP_GROUP_DESKTOP,
			FMA_IS_OBJECT_ACTION( item ) ? NADP_KEY_PROFILES : NADP_KEY_ITEMS_LIST,
			subitems );

	profile_groups = fma_desktop_file_get_profiles( ndp );
	tmp = g_strdup_printf( "%s (existing profiles)", thisfn );
	fma_core_utils_slist_dump( tmp, profile_groups );
	g_free( tmp );

	for( ip = profile_groups ; ip ; ip = ip->next ){
		if( fma_core_utils_slist_count( subitems, ( const gchar * ) ip->data ) == 0 ){
			g_debug( "%s: deleting (removed) profile %s", thisfn, ( const gchar * ) ip->data );
			fma_desktop_file_remove_profile( ndp, ( const gchar * ) ip->data );
		}
	}

	fma_core_utils_slist_free( profile_groups );
	fma_core_utils_slist_free( subitems );
}

static ExportFormatFn *
find_export_format_fn( const gchar *format )
{
	ExportFormatFn *found;
	ExportFormatFn *i;

	found = NULL;
	i = st_export_format_fn;

	while( i->format && !found ){
		if( !strcmp( i->format, format )){
			found = i;
		}
		i++;
	}

	return( found );
}

#ifdef NA_ENABLE_DEPRECATED
static ExportFormatFn *
find_export_format_fn_from_quark( GQuark format )
{
	ExportFormatFn *found;
	ExportFormatFn *i;

	found = NULL;
	i = st_export_format_fn;

	while( i->format && !found ){
		if( g_quark_from_string( i->format ) == format ){
			found = i;
		}
		i++;
	}

	return( found );
}
#endif
