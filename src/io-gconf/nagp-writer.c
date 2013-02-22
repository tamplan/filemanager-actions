/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2013 Pierre Wieser and others (see AUTHORS)
 *
 * Nautilus-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Nautilus-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nautilus-Actions; see the file COPYING. If not, see
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

#include <string.h>

#include <api/na-data-def.h>
#include <api/na-data-types.h>
#include <api/na-iio-provider.h>
#include <api/na-ifactory-provider.h>
#include <api/na-object-api.h>
#include <api/na-core-utils.h>
#include <api/na-gconf-utils.h>

#include "nagp-gconf-provider.h"
#include "nagp-writer.h"
#include "nagp-keys.h"

#ifdef NA_ENABLE_DEPRECATED
static void write_start_write_type( NagpGConfProvider *provider, NAObjectItem *item );
static void write_start_write_version( NagpGConfProvider *provider, NAObjectItem *item );
#endif

/*
 * API function: should only be called through NAIIOProvider interface
 */
gboolean
nagp_iio_provider_is_willing_to_write( const NAIIOProvider *provider )
{
#ifdef NA_ENABLE_DEPRECATED
	return( TRUE );
#else
	return( FALSE );
#endif
}

/*
 * Rationale: gconf reads its storage path from /etc/gconf/2/path ;
 * there is there a 'xml:readwrite:$(HOME)/.gconf' line, but I do not
 * known any way to get it programatically, so an admin may have set a
 * readwrite space elsewhere..
 *
 * So, we try to write a 'foo' key somewhere: if it is ok, then the
 * provider is supposed able to write...
 *
 * API function: should only be called through NAIIOProvider interface
 */
gboolean
nagp_iio_provider_is_able_to_write( const NAIIOProvider *provider )
{
#ifdef NA_ENABLE_DEPRECATED
	static const gchar *thisfn = "nagp_iio_provider_is_able_to_write";
	static const gchar *path = "/apps/nautilus-actions/foo";
	NagpGConfProvider *self;
	gboolean able_to = FALSE;

	/*g_debug( "%s: provider=%p", thisfn, ( void * ) provider );*/
	g_return_val_if_fail( NAGP_IS_GCONF_PROVIDER( provider ), FALSE );
	g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider ), FALSE );

	self = NAGP_GCONF_PROVIDER( provider );

	if( !self->private->dispose_has_run ){

		if( !na_gconf_utils_write_string( self->private->gconf, path, "foo", NULL )){
			able_to = FALSE;

		} else {
			gchar *str = na_gconf_utils_read_string( self->private->gconf, path, FALSE, NULL );
			if( strcmp( str, "foo" )){
				able_to = FALSE;

			} else if( !gconf_client_recursive_unset( self->private->gconf, path, 0, NULL )){
				able_to = FALSE;

			} else {
				able_to = TRUE;
			}

			g_free( str );
		}
	}

	gconf_client_suggest_sync( self->private->gconf, NULL );

	g_debug( "%s: provider=%p, able_to=%s", thisfn, ( void * ) provider, able_to ? "True":"False" );
	return( able_to );
#else
	return( FALSE );
#endif
}

#ifdef NA_ENABLE_DEPRECATED
/*
 * update an existing item or write a new one
 * in all cases, it is much more easy to delete the existing  entries
 * before trying to write the new ones
 */
guint
nagp_iio_provider_write_item( const NAIIOProvider *provider, const NAObjectItem *item, GSList **messages )
{
	static const gchar *thisfn = "nagp_gconf_provider_iio_provider_write_item";
	NagpGConfProvider *self;
	guint ret;

	g_debug( "%s: provider=%p (%s), item=%p (%s), messages=%p",
			thisfn,
			( void * ) provider, G_OBJECT_TYPE_NAME( provider ),
			( void * ) item, G_OBJECT_TYPE_NAME( item ),
			( void * ) messages );

	ret = NA_IIO_PROVIDER_CODE_PROGRAM_ERROR;

	g_return_val_if_fail( NAGP_IS_GCONF_PROVIDER( provider ), ret );
	g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider ), ret );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), ret );

	self = NAGP_GCONF_PROVIDER( provider );

	if( self->private->dispose_has_run ){
		return( NA_IIO_PROVIDER_CODE_NOT_WILLING_TO_RUN );
	}

	ret = nagp_iio_provider_delete_item( provider, item, messages );

	if( ret == NA_IIO_PROVIDER_CODE_OK ){
		na_ifactory_provider_write_item( NA_IFACTORY_PROVIDER( provider ), NULL, NA_IFACTORY_OBJECT( item ), messages );
	}

	gconf_client_suggest_sync( self->private->gconf, NULL );

	return( ret );
}

/*
 * also delete the schema which may be directly attached to this action
 * cf. http://bugzilla.gnome.org/show_bug.cgi?id=325585
 */
guint
nagp_iio_provider_delete_item( const NAIIOProvider *provider, const NAObjectItem *item, GSList **messages )
{
	static const gchar *thisfn = "nagp_gconf_provider_iio_provider_delete_item";
	NagpGConfProvider *self;
	guint ret;
	gchar *uuid, *path;
	GError *error = NULL;

	g_debug( "%s: provider=%p (%s), item=%p (%s), messages=%p",
			thisfn,
			( void * ) provider, G_OBJECT_TYPE_NAME( provider ),
			( void * ) item, G_OBJECT_TYPE_NAME( item ),
			( void * ) messages );

	ret = NA_IIO_PROVIDER_CODE_PROGRAM_ERROR;

	g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider ), ret );
	g_return_val_if_fail( NAGP_IS_GCONF_PROVIDER( provider ), ret );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), ret );

	self = NAGP_GCONF_PROVIDER( provider );

	if( self->private->dispose_has_run ){
		return( NA_IIO_PROVIDER_CODE_NOT_WILLING_TO_RUN );
	}

	ret = NA_IIO_PROVIDER_CODE_OK;
	uuid = na_object_get_id( NA_OBJECT( item ));

	/* GCONF_UNSET_INCLUDING_SCHEMA_NAMES seems mean: including the name
	 * of the schemas which is embedded in the GConfEntry - this doesn't
	 * mean including the schemas themselves
	 */
	if( ret == NA_IIO_PROVIDER_CODE_OK ){
		path = gconf_concat_dir_and_key( NAGP_CONFIGURATIONS_PATH, uuid );
		gconf_client_recursive_unset( self->private->gconf, path, GCONF_UNSET_INCLUDING_SCHEMA_NAMES, &error );
		if( error ){
			g_warning( "%s: path=%s, error=%s", thisfn, path, error->message );
			*messages = g_slist_append( *messages, g_strdup( error->message ));
			g_error_free( error );
			error = NULL;
			ret = NA_IIO_PROVIDER_CODE_DELETE_CONFIG_ERROR;
		}
		gconf_client_suggest_sync( self->private->gconf, NULL );
		g_free( path );
	}

	if( ret == NA_IIO_PROVIDER_CODE_OK ){
		path = gconf_concat_dir_and_key( NAGP_SCHEMAS_PATH, uuid );
		gconf_client_recursive_unset( self->private->gconf, path, 0, &error );
		if( error ){
			g_warning( "%s: path=%s, error=%s", thisfn, path, error->message );
			*messages = g_slist_append( *messages, g_strdup( error->message ));
			g_error_free( error );
			error = NULL;
			ret = NA_IIO_PROVIDER_CODE_DELETE_SCHEMAS_ERROR;
		}
		g_free( path );
		gconf_client_suggest_sync( self->private->gconf, NULL );
	}

	g_free( uuid );

	return( ret );
}

guint
nagp_writer_write_start( const NAIFactoryProvider *writer, void *writer_data,
							const NAIFactoryObject *object, GSList **messages  )
{
	if( NA_IS_OBJECT_ITEM( object )){
		write_start_write_type( NAGP_GCONF_PROVIDER( writer ), NA_OBJECT_ITEM( object ));
		write_start_write_version( NAGP_GCONF_PROVIDER( writer ), NA_OBJECT_ITEM( object ));
	}

	return( NA_IIO_PROVIDER_CODE_OK );
}

static void
write_start_write_type( NagpGConfProvider *provider, NAObjectItem *item )
{
	gchar *id, *path;

	id = na_object_get_id( item );
	path = g_strdup_printf( "%s/%s/%s", NAGP_CONFIGURATIONS_PATH, id, NAGP_ENTRY_TYPE );

	na_gconf_utils_write_string(
			provider->private->gconf,
			path,
			NA_IS_OBJECT_ACTION( item ) ? NAGP_VALUE_TYPE_ACTION : NAGP_VALUE_TYPE_MENU,
			NULL );

	g_free( path );
	g_free( id );
}

static void
write_start_write_version( NagpGConfProvider *provider, NAObjectItem *item )
{
	gchar *id, *path;
	guint iversion;

	id = na_object_get_id( item );
	path = g_strdup_printf( "%s/%s/%s", NAGP_CONFIGURATIONS_PATH, id, NAGP_ENTRY_IVERSION );

	iversion = na_object_get_iversion( item );
	na_gconf_utils_write_int( provider->private->gconf, path, iversion, NULL );

	g_free( path );
	g_free( id );
}

guint
nagp_writer_write_data( const NAIFactoryProvider *provider, void *writer_data,
									const NAIFactoryObject *object, const NADataBoxed *boxed,
									GSList **messages )
{
	static const gchar *thisfn = "nagp_writer_write_data";
	guint code;
	const NADataDef *def;
	gchar *this_id;
	gchar *this_path, *path;
	gchar *msg;
	gchar *str_value;
	gboolean bool_value;
	GSList *slist_value;
	guint uint_value;
	GConfClient *gconf;

	/*g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));*/

	msg = NULL;
	code = NA_IIO_PROVIDER_CODE_OK;
	def = na_data_boxed_get_data_def( boxed );

	if( !na_data_boxed_is_default( boxed ) || def->write_if_default ){

		if( NA_IS_OBJECT_PROFILE( object )){
			NAObjectItem *parent = NA_OBJECT_ITEM( na_object_get_parent( object ));
			gchar *parent_id = na_object_get_id( parent );
			gchar *id = na_object_get_id( object );
			this_id = g_strdup_printf( "%s/%s", parent_id, id );
			g_free( id );
			g_free( parent_id );

		} else {
			this_id = na_object_get_id( object );
		}

		this_path = gconf_concat_dir_and_key( NAGP_CONFIGURATIONS_PATH, this_id );
		path = gconf_concat_dir_and_key( this_path, def->gconf_entry );

		gconf = NAGP_GCONF_PROVIDER( provider )->private->gconf;

		switch( def->type ){

			case NA_DATA_TYPE_STRING:
				str_value = na_boxed_get_string( NA_BOXED( boxed ));
				na_gconf_utils_write_string( gconf, path, str_value, &msg );
				if( msg ){
					*messages = g_slist_append( *messages, msg );
					code = NA_IIO_PROVIDER_CODE_WRITE_ERROR;
				}
				g_free( str_value );
				break;

			case NA_DATA_TYPE_LOCALE_STRING:
				str_value = na_boxed_get_string( NA_BOXED( boxed ));
				na_gconf_utils_write_string( gconf, path, str_value, &msg );
				if( msg ){
					*messages = g_slist_append( *messages, msg );
					code = NA_IIO_PROVIDER_CODE_WRITE_ERROR;
				}
				g_free( str_value );
				break;

			case NA_DATA_TYPE_BOOLEAN:
				bool_value = GPOINTER_TO_UINT( na_boxed_get_as_void( NA_BOXED( boxed )));
				na_gconf_utils_write_bool( gconf, path, bool_value, &msg );
				if( msg ){
					*messages = g_slist_append( *messages, msg );
					code = NA_IIO_PROVIDER_CODE_WRITE_ERROR;
				}
				break;

			case NA_DATA_TYPE_STRING_LIST:
				slist_value = ( GSList * ) na_boxed_get_as_void( NA_BOXED( boxed ));
				na_gconf_utils_write_string_list( gconf, path, slist_value, &msg );
				if( msg ){
					*messages = g_slist_append( *messages, msg );
					code = NA_IIO_PROVIDER_CODE_WRITE_ERROR;
				}
				na_core_utils_slist_free( slist_value );
				break;

			case NA_DATA_TYPE_UINT:
				uint_value = GPOINTER_TO_UINT( na_boxed_get_as_void( NA_BOXED( boxed )));
				na_gconf_utils_write_int( gconf, path, uint_value, &msg );
				if( msg ){
					*messages = g_slist_append( *messages, msg );
					code = NA_IIO_PROVIDER_CODE_WRITE_ERROR;
				}
				break;

			default:
				g_warning( "%s: unknown type=%u for %s", thisfn, def->type, def->name );
				code = NA_IIO_PROVIDER_CODE_PROGRAM_ERROR;
		}

		/*g_debug( "%s: gconf=%p, code=%u, path=%s", thisfn, ( void * ) gconf, code, path );*/

		g_free( msg );
		g_free( path );
		g_free( this_path );
		g_free( this_id );
	}

	return( code );
}

guint
nagp_writer_write_done( const NAIFactoryProvider *writer, void *writer_data,
									const NAIFactoryObject *object,
									GSList **messages  )
{
	return( NA_IIO_PROVIDER_CODE_OK );
}
#endif /* NA_ENABLE_DEPRECATED */
