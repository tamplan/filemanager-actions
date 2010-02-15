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

#include <string.h>

#include <api/na-iio-provider.h>
#include <api/na-object-api.h>
#include <api/na-core-utils.h>
#include <api/na-gconf-utils.h>

#include "nagp-gconf-provider.h"
#include "nagp-writer.h"
#include "nagp-keys.h"

static gboolean write_item_action( NagpGConfProvider *gconf, const NAObjectAction *action, GSList **message );
static gboolean write_item_menu( NagpGConfProvider *gconf, const NAObjectMenu *menu, GSList **message );
static gboolean write_object_item( NagpGConfProvider *gconf, const NAObjectItem *item, GSList **message );

static gboolean write_str( NagpGConfProvider *gconf, const gchar *uuid, const gchar *name, const gchar *key, gchar *value, GSList **message );
static gboolean write_bool( NagpGConfProvider *gconf, const gchar *uuid, const gchar *name, const gchar *key, gboolean value, GSList **message );
static gboolean write_list( NagpGConfProvider *gconf, const gchar *uuid, const gchar *name, const gchar *key, GSList *value, GSList **message );

static gboolean remove_key( NagpGConfProvider *provider, const gchar *uuid, const gchar *key, GSList **messages );

/*
 * API function: should only be called through NAIIOProvider interface
 */
gboolean
nagp_iio_provider_is_willing_to_write( const NAIIOProvider *provider )
{
	return( TRUE );
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
	/*static const gchar *thisfn = "nagp_iio_provider_is_able_to_write";*/
	static const gchar *path = "/apps/nautilus-actions/foo";
	NagpGConfProvider *self;
	gboolean able_to = FALSE;

	/*g_debug( "%s: provider=%p", thisfn, ( void * ) provider );*/
	g_return_val_if_fail( NAGP_IS_GCONF_PROVIDER( provider ), FALSE );
	g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider ), FALSE );

	self = NAGP_GCONF_PROVIDER( provider );

	if( !self->private->dispose_has_run ){

		if( !na_gconf_utils_write_string( self->private->gconf, path, "1", NULL )){
			able_to = FALSE;

		} else if( !gconf_client_recursive_unset( self->private->gconf, path, 0, NULL )){
			able_to = FALSE;

		} else {
			able_to = TRUE;
		}
	}

	/*g_debug( "%s: provider=%p, able_to=%s", thisfn, ( void * ) provider, able_to ? "True":"False" );*/
	return( able_to );
}

/*
 *
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

		if( NA_IS_OBJECT_ACTION( item )){
			if( !write_item_action( self, NA_OBJECT_ACTION( item ), messages )){
				return( NA_IIO_PROVIDER_CODE_WRITE_ERROR );
			}
		}

		if( NA_IS_OBJECT_MENU( item )){
			if( !write_item_menu( self, NA_OBJECT_MENU( item ), messages )){
				return( NA_IIO_PROVIDER_CODE_WRITE_ERROR );
			}
		}
	}

	gconf_client_suggest_sync( self->private->gconf, NULL );

	return( ret );
}

static gboolean
write_item_action( NagpGConfProvider *provider, const NAObjectAction *action, GSList **messages )
{
	gchar *uuid, *name;
	gboolean ret;
	GList *profiles, *ip;
	NAObjectProfile *profile;

	uuid = na_object_get_id( action );

	ret =
		write_object_item( provider, NA_OBJECT_ITEM( action ), messages ) &&
		write_str( provider, uuid, NULL, NAGP_ENTRY_VERSION, na_object_get_version( action ), messages ) &&
		write_bool( provider, uuid, NULL, NAGP_ENTRY_TARGET_SELECTION, na_object_is_target_selection( action ), messages ) &&
		write_bool( provider, uuid, NULL, NAGP_ENTRY_TARGET_BACKGROUND, na_object_is_target_background( action ), messages ) &&
		write_bool( provider, uuid, NULL, NAGP_ENTRY_TARGET_TOOLBAR, na_object_is_target_toolbar( action ), messages ) &&
		write_str( provider, uuid, NULL, NAGP_ENTRY_TOOLBAR_LABEL, na_object_get_toolbar_label( action ), messages ) &&
		write_str( provider, uuid, NULL, NAGP_ENTRY_TYPE, g_strdup( NAGP_VALUE_TYPE_ACTION ), messages );

	/* key was used between 2.29.1 and 2.29.4, but is removed since 2.29.5 */
	remove_key( provider, uuid, NAGP_ENTRY_TOOLBAR_SAME_LABEL, messages );

	profiles = na_object_get_items( action );

	for( ip = profiles ; ip && ret ; ip = ip->next ){

		profile = NA_OBJECT_PROFILE( ip->data );
		name = na_object_get_id( profile );

		ret =
			write_str( provider, uuid, name, NAGP_ENTRY_PROFILE_LABEL, na_object_get_label( profile ), messages ) &&
			write_str( provider, uuid, name, NAGP_ENTRY_PATH, na_object_get_path( profile ), messages ) &&
			write_str( provider, uuid, name, NAGP_ENTRY_PARAMETERS, na_object_get_parameters( profile ), messages ) &&
			write_list( provider, uuid, name, NAGP_ENTRY_BASENAMES, na_object_get_basenames( profile ), messages ) &&
			write_bool( provider, uuid, name, NAGP_ENTRY_MATCHCASE, na_object_is_matchcase( profile ), messages ) &&
			write_list( provider, uuid, name, NAGP_ENTRY_MIMETYPES, na_object_get_mimetypes( profile ), messages ) &&
			write_bool( provider, uuid, name, NAGP_ENTRY_ISFILE, na_object_is_file( profile ), messages ) &&
			write_bool( provider, uuid, name, NAGP_ENTRY_ISDIR, na_object_is_dir( profile ), messages ) &&
			write_bool( provider, uuid, name, NAGP_ENTRY_MULTIPLE, na_object_is_multiple( profile ), messages ) &&
			write_list( provider, uuid, name, NAGP_ENTRY_SCHEMES, na_object_get_schemes( profile ), messages ) &&
			write_list( provider, uuid, name, NAGP_ENTRY_FOLDERS, na_object_get_folders( profile ), messages );

		g_free( name );
	}

	g_free( uuid );

	return( ret );
}

static gboolean
write_item_menu( NagpGConfProvider *provider, const NAObjectMenu *menu, GSList **messages )
{
	gboolean ret;
	gchar *uuid;

	uuid = na_object_get_id( menu );

	ret =
		write_object_item( provider, NA_OBJECT_ITEM( menu ), messages ) &&
		write_str( provider, uuid, NULL, NAGP_ENTRY_TYPE, g_strdup( NAGP_VALUE_TYPE_MENU ), messages );

	g_free( uuid );

	return( ret );
}

static gboolean
write_object_item( NagpGConfProvider *provider, const NAObjectItem *item, GSList **messages )
{
	gchar *uuid;
	gboolean ret;

	uuid = na_object_get_id( NA_OBJECT( item ));

	ret =
		write_str( provider, uuid, NULL, NAGP_ENTRY_LABEL, na_object_get_label( NA_OBJECT( item )), messages ) &&
		write_str( provider, uuid, NULL, NAGP_ENTRY_TOOLTIP, na_object_get_tooltip( item ), messages ) &&
		write_str( provider, uuid, NULL, NAGP_ENTRY_ICON, na_object_get_icon( item ), messages ) &&
		write_bool( provider, uuid, NULL, NAGP_ENTRY_ENABLED, na_object_is_enabled( item ), messages ) &&
		write_list( provider, uuid, NULL, NAGP_ENTRY_ITEMS_LIST, na_object_build_items_slist( item ), messages );

	g_free( uuid );
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

	if( ret == NA_IIO_PROVIDER_CODE_OK ){
		path = gconf_concat_dir_and_key( NAGP_CONFIGURATIONS_PATH, uuid );
		gconf_client_recursive_unset( self->private->gconf, path, 0, &error );
		if( error ){
			g_warning( "%s: path=%s, error=%s", thisfn, path, error->message );
			*messages = g_slist_append( *messages, g_strdup( error->message ));
			g_error_free( error );
			error = NULL;
			ret = NA_IIO_PROVIDER_CODE_DELETE_CONFIG_ERROR;
		}
	}

	gconf_client_suggest_sync( self->private->gconf, NULL );

	g_free( path );
	g_free( uuid );

	return( ret );
}

static gboolean
write_str( NagpGConfProvider *provider, const gchar *uuid, const gchar *name, const gchar *key, gchar *value, GSList **messages )
{
	gchar *path;
	gboolean ret;
	gchar *msg;

	if( name && strlen( name )){
		path = g_strdup_printf( "%s/%s/%s/%s", NAGP_CONFIGURATIONS_PATH, uuid, name, key );
	} else {
		path = g_strdup_printf( "%s/%s/%s", NAGP_CONFIGURATIONS_PATH, uuid, key );
	}

	msg = NULL;
	ret = na_gconf_utils_write_string( provider->private->gconf, path, value, &msg );
	if( msg ){
		*messages = g_slist_append( *messages, g_strdup( msg ));
		g_free( msg );
	}

	g_free( value );
	g_free( path );

	return( ret );
}

static gboolean
write_bool( NagpGConfProvider *provider, const gchar *uuid, const gchar *name, const gchar *key, gboolean value, GSList **messages )
{
	gboolean ret;
	gchar *path;
	gchar *msg;

	if( name && strlen( name )){
		path = g_strdup_printf( "%s/%s/%s/%s", NAGP_CONFIGURATIONS_PATH, uuid, name, key );
	} else {
		path = g_strdup_printf( "%s/%s/%s", NAGP_CONFIGURATIONS_PATH, uuid, key );
	}

	msg = NULL;
	ret = na_gconf_utils_write_bool( provider->private->gconf, path, value, &msg );
	if( msg ){
		*messages = g_slist_append( *messages, g_strdup( msg ));
		g_free( msg );
	}

	g_free( path );

	return( ret );
}

static gboolean
write_list( NagpGConfProvider *provider, const gchar *uuid, const gchar *name, const gchar *key, GSList *value, GSList **messages )
{
	gboolean ret;
	gchar *path;
	gchar *msg;

	if( name && strlen( name )){
		path = g_strdup_printf( "%s/%s/%s/%s", NAGP_CONFIGURATIONS_PATH, uuid, name, key );
	} else {
		path = g_strdup_printf( "%s/%s/%s", NAGP_CONFIGURATIONS_PATH, uuid, key );
	}

	msg = NULL;
	ret = na_gconf_utils_write_string_list( provider->private->gconf, path, value, &msg );
	if( msg ){
		*messages = g_slist_append( *messages, g_strdup( msg ));
		g_free( msg );
	}

	na_core_utils_slist_free( value );
	g_free( path );

	return( ret );
}

static gboolean
remove_key( NagpGConfProvider *provider, const gchar *uuid, const gchar *key, GSList **messages )
{
	gboolean ret;
	gchar *path;
	gchar *msg;

	path = g_strdup_printf( "%s/%s/%s", NAGP_CONFIGURATIONS_PATH, uuid, key );
	msg = NULL;

	ret = na_gconf_utils_remove_entry( provider->private->gconf, path, &msg );

	if( msg ){
		*messages = g_slist_append( *messages, g_strdup( msg ));
		g_free( msg );
	}

	g_free( path );

	return( ret );
}
