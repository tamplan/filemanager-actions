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

#include <gio/gio.h>
#include <string.h>

#include "nadp-desktop-file.h"
#include "nadp-utils.h"

/* private class data
 */
struct NadpDesktopFileClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NadpDesktopFilePrivate {
	gboolean   dispose_has_run;
	gchar     *id;
	gchar     *path;
	GKeyFile  *key_file;
};

static GObjectClass *st_parent_class = NULL;

static GType            register_type( void );
static void             class_init( NadpDesktopFileClass *klass );
static void             instance_init( GTypeInstance *instance, gpointer klass );
static void             instance_dispose( GObject *object );
static void             instance_finalize( GObject *object );

static NadpDesktopFile *ndf_new( const gchar *path );
static gboolean         check_key_file( NadpDesktopFile *ndf );

GType
nadp_desktop_file_get_type( void )
{
	static GType class_type = 0;

	if( !class_type ){
		class_type = register_type();
	}

	return( class_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "nadp_desktop_file_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NadpDesktopFileClass ),
		NULL,
		NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NadpDesktopFile ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "NadpDesktopFile", &info, 0 );

	return( type );
}

static void
class_init( NadpDesktopFileClass *klass )
{
	static const gchar *thisfn = "nadp_desktop_file_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NadpDesktopFileClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nadp_desktop_file_instance_init";
	NadpDesktopFile *self;

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );
	g_return_if_fail( NADP_IS_DESKTOP_FILE( instance ));
	self = NADP_DESKTOP_FILE( instance );

	self->private = g_new0( NadpDesktopFilePrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->key_file = g_key_file_new();
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "nadp_desktop_file_instance_dispose";
	NadpDesktopFile *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_return_if_fail( NADP_IS_DESKTOP_FILE( object ));
	self = NADP_DESKTOP_FILE( object );

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
	NadpDesktopFile *self;

	g_assert( NADP_IS_DESKTOP_FILE( object ));
	self = NADP_DESKTOP_FILE( object );

	if( self->private->id ){
		g_free( self->private->id );
		self->private->id = NULL;
	}

	if( self->private->path ){
		g_free( self->private->path );
		self->private->path = NULL;
	}

	if( self->private->key_file ){
		g_key_file_free( self->private->key_file );
		self->private->key_file = NULL;
	}

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/*
 * ndf_new:
 * @path: the full pathname of a .desktop file.
 *
 * Retuns: a newly allocated #NadpDesktopFile object.
 */
static NadpDesktopFile *
ndf_new( const gchar *path )
{
	NadpDesktopFile *ndf;

	ndf = g_object_new( NADP_DESKTOP_FILE_TYPE, NULL );

	ndf->private->id = nadp_utils_path2id( path );
	ndf->private->path = g_strdup( path );

	return( ndf );
}

/**
 * nadp_desktop_file_new_for_write:
 * @path: the full pathname of a .desktop file.
 *
 * Retuns: a newly allocated #NadpDesktopFile object.
 */
NadpDesktopFile *
nadp_desktop_file_new_for_write( const gchar *path )
{
	static const gchar *thisfn = "nadp_desktop_file_new_for_write";
	NadpDesktopFile *ndf;

	ndf = NULL;
	g_debug( "%s: path=%s", thisfn, path );
	g_return_val_if_fail( path && g_utf8_strlen( path, -1 ) && g_path_is_absolute( path ), ndf );

	ndf = ndf_new( path );

	g_key_file_set_string( ndf->private->key_file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_TYPE, G_KEY_FILE_DESKTOP_TYPE_APPLICATION );

	return( ndf );
}

/**
 * nadp_desktop_file_new_from_path:
 * @path: the full pathname of a .desktop file.
 *
 * Retuns: a newly allocated #NadpDesktopFile object.
 *
 * Key file has been loaded, and first validity checks made.
 */
NadpDesktopFile *
nadp_desktop_file_new_from_path( const gchar *path )
{
	static const gchar *thisfn = "nadp_desktop_file_new_from_path";
	NadpDesktopFile *ndf;
	GError *error;

	ndf = NULL;
	g_debug( "%s: path=%s", thisfn, path );
	g_return_val_if_fail( path && g_utf8_strlen( path, -1 ) && g_path_is_absolute( path ), ndf );

	ndf = ndf_new( path );

	error = NULL;
	g_key_file_load_from_file( ndf->private->key_file, path, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, &error );
	if( error ){
		g_warning( "%s: %s: %s", thisfn, path, error->message );
		g_error_free( error );
		g_object_unref( ndf );
		return( NULL );
	}

	if( !check_key_file( ndf )){
		g_object_unref( ndf );
		return( NULL );
	}

	return( ndf );
}

static gboolean
check_key_file( NadpDesktopFile *ndf )
{
	static const gchar *thisfn = "nadp_desktop_file_check_key_file";
	gboolean ret;
	gchar *start_group;
	gchar *type;
	GError *error;

	ret = TRUE;

	/* start group must be 'Desktop Entry' */
	start_group = g_key_file_get_start_group( ndf->private->key_file );
	if( strcmp( start_group, G_KEY_FILE_DESKTOP_GROUP )){
		g_warning( "%s: %s: invalid start group, found %s, waited for %s",
				thisfn, ndf->private->path, start_group, G_KEY_FILE_DESKTOP_GROUP );
		ret = FALSE;
	}
	g_free( start_group );

	/* Type is required
	 * only deal with 'Application' type */
	if( ret ){
		error = NULL;
		type = g_key_file_get_string( ndf->private->key_file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_TYPE, &error );
		if( error ){
			g_warning( "%s: %s: %s", thisfn, ndf->private->path, error->message );
			g_error_free( error );
			ret = FALSE;
		} else if( strcmp( type, G_KEY_FILE_DESKTOP_TYPE_APPLICATION )){
			ret = FALSE;
		}
		g_free( type );
	}

	return( ret );
}

/**
 * nadp_desktop_file_get_key_file_path:
 * @ndf: the #NadpDesktopFile instance.
 *
 * Returns: the full pathname of the key file, as a newly allocated
 * string which should be g_free() by the caller.
 */
gchar *
nadp_desktop_file_get_key_file_path( const NadpDesktopFile *ndf )
{
	gchar *path;

	path = NULL;
	g_return_val_if_fail( NADP_IS_DESKTOP_FILE( ndf ), path );

	if( !ndf->private->dispose_has_run ){
		path = g_strdup( ndf->private->path );
	}

	return( path );
}

/**
 * nadp_desktop_file_get_id:
 * @ndf: the #NadpDesktopFile instance.
 *
 * Returns: the id of the file, as a newly allocated string which should
 * be g_free() by the caller.
 */
gchar *
nadp_desktop_file_get_id( const NadpDesktopFile *ndf )
{
	gchar *id;

	id = NULL;
	g_return_val_if_fail( NADP_IS_DESKTOP_FILE( ndf ), id );

	if( !ndf->private->dispose_has_run ){
		id = g_strdup( ndf->private->id );
	}

	return( id );
}

/**
 * nadp_desktop_file_get_label:
 * @ndf: the #NadpDesktopFile instance.
 *
 * Returns: the label of the action, as a newly allocated string which
 * should be g_free() by the caller.
 */
gchar *
nadp_desktop_file_get_label( const NadpDesktopFile *ndf )
{
	gchar *label;

	label = NULL;
	g_return_val_if_fail( NADP_IS_DESKTOP_FILE( ndf ), label );

	if( !ndf->private->dispose_has_run ){
		label = g_key_file_get_locale_string(
				ndf->private->key_file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_NAME, NULL, NULL );

		if( !label ){
			label = g_strdup( "" );
		}
	}

	return( label );
}

/**
 * nadp_desktop_file_get_tooltip:
 * @ndf: the #NadpDesktopFile instance.
 *
 * Returns: the tooltip of the action, as a newly allocated string which
 * should be g_free() by the caller.
 */
gchar *
nadp_desktop_file_get_tooltip( const NadpDesktopFile *ndf )
{
	gchar *tooltip;

	tooltip = NULL;
	g_return_val_if_fail( NADP_IS_DESKTOP_FILE( ndf ), tooltip );

	if( !ndf->private->dispose_has_run ){
		tooltip = g_key_file_get_locale_string(
				ndf->private->key_file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_COMMENT, NULL, NULL );

		if( !tooltip ){
			tooltip = g_strdup( "" );
		}
	}

	return( tooltip );
}

/**
 * nadp_desktop_file_get_icon:
 * @ndf: the #NadpDesktopFile instance.
 *
 * Returns: the icon of the action, as a newly allocated string which
 * should be g_free() by the caller.
 */
gchar *
nadp_desktop_file_get_icon( const NadpDesktopFile *ndf )
{
	gchar *icon;

	icon = NULL;
	g_return_val_if_fail( NADP_IS_DESKTOP_FILE( ndf ), icon );

	if( !ndf->private->dispose_has_run ){
		icon = g_key_file_get_locale_string(
				ndf->private->key_file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_ICON, NULL, NULL );

		if( !icon ){
			icon = g_strdup( "" );
		}
	}

	return( icon );
}

/**
 * nadp_desktop_file_get_enabled:
 * @ndf: the #NadpDesktopFile instance.
 *
 * Returns: %TRUE if the action is enabled, %FALSE else.
 *
 * Defaults to TRUE if the key is not specified.
 */
gboolean
nadp_desktop_file_get_enabled( const NadpDesktopFile *ndf )
{
	gboolean enabled;

	enabled = TRUE;
	g_return_val_if_fail( NADP_IS_DESKTOP_FILE( ndf ), enabled );

	if( !ndf->private->dispose_has_run ){

		if( g_key_file_has_key(
				ndf->private->key_file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_NO_DISPLAY, NULL )){

			enabled = !g_key_file_get_boolean(
				ndf->private->key_file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_NO_DISPLAY, NULL );
		}
	}

	return( enabled );
}

/**
 * nadp_desktop_file_set_label:
 * @ndf: the #NadpDesktopFile instance.
 * @label: the label to be set.
 *
 * Sets the label.
 */
void
nadp_desktop_file_set_label( NadpDesktopFile *ndf, const gchar *label )
{
	char **locales;

	g_return_if_fail( NADP_IS_DESKTOP_FILE( ndf ));

	if( !ndf->private->dispose_has_run ){

		locales = ( char ** ) g_get_language_names();
		g_key_file_set_locale_string(
				ndf->private->key_file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_NAME, locales[0], label );
	}
}

/**
 * nadp_desktop_file_set_tooltip:
 * @ndf: the #NadpDesktopFile instance.
 * @tooltip: the tooltip to be set.
 *
 * Sets the tooltip.
 */
void
nadp_desktop_file_set_tooltip( NadpDesktopFile *ndf, const gchar *tooltip )
{
	char **locales;

	g_return_if_fail( NADP_IS_DESKTOP_FILE( ndf ));

	if( !ndf->private->dispose_has_run ){

		locales = ( char ** ) g_get_language_names();
		g_key_file_set_locale_string(
				ndf->private->key_file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_COMMENT, locales[0], tooltip );
	}
}

/**
 * nadp_desktop_file_set_icon:
 * @ndf: the #NadpDesktopFile instance.
 * @icon: the icon name or path to be set.
 *
 * Sets the icon.
 */
void
nadp_desktop_file_set_icon( NadpDesktopFile *ndf, const gchar *icon )
{
	char **locales;

	g_return_if_fail( NADP_IS_DESKTOP_FILE( ndf ));

	if( !ndf->private->dispose_has_run ){

		locales = ( char ** ) g_get_language_names();
		g_key_file_set_locale_string(
				ndf->private->key_file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_ICON, locales[0], icon );
	}
}

/**
 * nadp_desktop_file_set_enabled:
 * @ndf: the #NadpDesktopFile instance.
 * @enabled: whether the action is enabled.
 *
 * Sets the enabled status of the item.
 *
 * Note that the NoDisplay key has an inversed logic regards to enabled
 * status.
 */
void
nadp_desktop_file_set_enabled( NadpDesktopFile *ndf, gboolean enabled )
{
	g_return_if_fail( NADP_IS_DESKTOP_FILE( ndf ));

	if( !ndf->private->dispose_has_run ){

		g_key_file_set_boolean(
				ndf->private->key_file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_NO_DISPLAY, !enabled );
	}
}

/**
 * nadp_desktop_file_write:
 * @ndf: the #NadpDesktopFile instance.
 *
 * Writes the key file to the disk.
 *
 * Returns: %TRUE if write is ok, %FALSE else.
 */
gboolean
nadp_desktop_file_write( NadpDesktopFile *ndf )
{
	static const gchar *thisfn = "nadp_desktop_file_write";
	gboolean ret;
	gchar *data;
	GFile *file;
	GFileOutputStream *stream;
	GError *error;

	ret = FALSE;
	error = NULL;
	g_return_val_if_fail( NADP_IS_DESKTOP_FILE( ndf ), ret );

	if( !ndf->private->dispose_has_run ){

		data = g_key_file_to_data( ndf->private->key_file, NULL, NULL );
		file = g_file_new_for_path( ndf->private->path );

		stream = g_file_replace( file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, &error );
		if( error ){
			g_warning( "%s: g_file_replace: %s", thisfn, error->message );
			g_error_free( error );
			if( stream ){
				g_object_unref( stream );
			}
			g_object_unref( file );
			g_free( data );
			return( FALSE );
		}

		g_output_stream_write( G_OUTPUT_STREAM( stream ), data, g_utf8_strlen( data, -1 ), NULL, &error );
		if( error ){
			g_warning( "%s: g_output_stream_write: %s", thisfn, error->message );
			g_error_free( error );
			g_object_unref( stream );
			g_object_unref( file );
			g_free( data );
			return( FALSE );
		}

		g_output_stream_close( G_OUTPUT_STREAM( stream ), NULL, &error );
		if( error ){
			g_warning( "%s: g_output_stream_close: %s", thisfn, error->message );
			g_error_free( error );
			g_object_unref( stream );
			g_object_unref( file );
			g_free( data );
			return( FALSE );
		}

		g_object_unref( stream );
		g_object_unref( file );
		g_free( data );
	}

	return( TRUE );
}
