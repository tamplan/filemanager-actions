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

#include "na-gnome-vfs-uri.h"
#include "na-selected-info.h"

/* private class data
 */
struct NASelectedInfoClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NASelectedInfoPrivate {
	gboolean       dispose_has_run;
	gchar         *uri;
	NAGnomeVFSURI *vfs;
	GFile         *location;
	gchar         *mimetype;
	GFileType      file_type;
	gboolean       can_read;
	gboolean       can_write;
	gboolean       can_execute;
	gchar         *owner;
};


static GObjectClass *st_parent_class = NULL;

static GType           register_type( void );
static void            class_init( NASelectedInfoClass *klass );
static void            instance_init( GTypeInstance *instance, gpointer klass );
static void            instance_dispose( GObject *object );
static void            instance_finalize( GObject *object );

static void            dump( const NASelectedInfo *nsi );
static NASelectedInfo *new_from_nautilus_file_info( NautilusFileInfo *item );
static NASelectedInfo *new_from_uri( const gchar *uri, const gchar *mimetype, gchar **errmsg );
static gboolean        query_file_attributes( NASelectedInfo *info, gchar **errmsg );

GType
na_selected_info_get_type( void )
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
	static const gchar *thisfn = "na_selected_info_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NASelectedInfoClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NASelectedInfo ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "NASelectedInfo", &info, 0 );

	return( type );
}

static void
class_init( NASelectedInfoClass *klass )
{
	static const gchar *thisfn = "na_selected_info_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NASelectedInfoClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "na_selected_info_instance_init";
	NASelectedInfo *self;

	g_return_if_fail( NA_IS_SELECTED_INFO( instance ));
	self = NA_SELECTED_INFO( instance );

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self->private = g_new0( NASelectedInfoPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->uri = NULL;
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "na_selected_info_instance_dispose";
	NASelectedInfo *self;

	g_return_if_fail( NA_IS_SELECTED_INFO( object ));
	self = NA_SELECTED_INFO( object );

	if( !self->private->dispose_has_run ){

		g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

		self->private->dispose_has_run = TRUE;

		g_object_unref( self->private->location );
		na_gnome_vfs_uri_free( self->private->vfs );

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "na_selected_info_instance_finalize";
	NASelectedInfo *self;

	g_return_if_fail( NA_IS_SELECTED_INFO( object ));
	self = NA_SELECTED_INFO( object );

	g_debug( "%s: object=%p", thisfn, ( void * ) object );

	g_free( self->private->uri );
	g_free( self->private->mimetype );
	g_free( self->private->owner );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * na_selected_info_get_list_from_item:
 * @item: a #NautilusFileInfo item
 *
 * Returns: a #GList list which contains a #NASelectedInfo item with the
 * same URI that the @item.
 */
GList *
na_selected_info_get_list_from_item( NautilusFileInfo *item )
{
	GList *selected;

	NASelectedInfo *info = new_from_nautilus_file_info( item );
	selected = g_list_prepend( NULL, info );

	return( selected );
}

/**
 * na_selected_info_get_list_from_list:
 * @nautilus_selection: a #GList list of #NautilusFileInfo items.
 *
 * Returns: a #GList list of #NASelectedInfo items whose URI correspond
 * to those of @nautilus_selection.
 */
GList *
na_selected_info_get_list_from_list( GList *nautilus_selection )
{
	GList *selected;
	GList *it;

	selected = NULL;

	for( it = nautilus_selection ; it ; it = it->next ){
		NASelectedInfo *info = new_from_nautilus_file_info( NAUTILUS_FILE_INFO( it->data ));
		selected = g_list_prepend( selected, info );
	}

	return( g_list_reverse( selected ));
}

/**
 * na_selected_info_copy_list:
 * @files: a #GList list of #NASelectedInfo items.
 *
 * Returns: a copy of the provided @files list.
 */
GList *
na_selected_info_copy_list( GList *files )
{
	GList *copy;
	GList *l;

	copy = g_list_copy( files );

	for( l = copy ; l != NULL ; l = l->next ){
		g_object_ref( G_OBJECT( l->data ));
	}

	return( copy );
}

/**
 * na_selected_info_free_list:
 * @files: a #GList of #NASelectedInfo items.
 *
 * Frees up the #GList @files.
 */
void
na_selected_info_free_list( GList *files )
{
	g_list_foreach( files, ( GFunc ) g_object_unref, NULL );
	g_list_free( files );
}

/**
 * na_selected_info_get_location:
 * @nsi: this #NASelectedInfo object.
 *
 * Returns: a new reference to the #GFile location.
 *
 * The returned location should be g_object_unref() by the caller.
 */
GFile *
na_selected_info_get_location( const NASelectedInfo *nsi )
{
	GFile *location;

	g_return_val_if_fail( NA_IS_SELECTED_INFO( nsi ), NULL );

	location = NULL;

	if( !nsi->private->dispose_has_run ){

		location = g_object_ref( nsi->private->location );
	}

	return( location );
}

/**
 * na_selected_info_get_basename:
 * @nsi: this #NASelectedInfo object.
 *
 * Returns: the basename of the file associated with this
 * #NASelectedInfo object, as a newly allocated string which
 * must be g_free() by the caller.
 */
gchar *
na_selected_info_get_basename( const NASelectedInfo *nsi )
{
	gchar *basename;

	g_return_val_if_fail( NA_IS_SELECTED_INFO( nsi ), NULL );

	basename = NULL;

	if( !nsi->private->dispose_has_run ){

		basename = g_strdup( g_path_get_basename( nsi->private->vfs->path ));
	}

	return( basename );
}

/**
 * na_selected_info_get_dirname:
 * @nsi: this #NASelectedInfo object.
 *
 * Returns: the dirname of the file associated with this
 * #NASelectedInfo object, as a newly allocated string which
 * must be g_free() by the caller.
 */
gchar *
na_selected_info_get_dirname( const NASelectedInfo *nsi )
{
	gchar *dirname;

	g_return_val_if_fail( NA_IS_SELECTED_INFO( nsi ), NULL );

	dirname = NULL;

	if( !nsi->private->dispose_has_run ){

		dirname = g_strdup( g_path_get_dirname( nsi->private->vfs->path ));
	}

	return( dirname );
}

/**
 * na_selected_info_get_mime_type:
 * @nsi: this #NASelectedInfo object.
 *
 * Returns: the mime type associated with this #NASelectedInfo object,
 * as a newly allocated string which should be g_free() by the caller.
 */
gchar *
na_selected_info_get_mime_type( const NASelectedInfo *nsi )
{
	gchar *mimetype;

	g_return_val_if_fail( NA_IS_SELECTED_INFO( nsi ), NULL );

	mimetype = NULL;

	if( !nsi->private->dispose_has_run ){

		if( nsi->private->mimetype ){
			mimetype = g_strdup( nsi->private->mimetype );
		}
	}

	return( mimetype );
}

/**
 * na_selected_info_get_path:
 * @nsi: this #NASelectedInfo object.
 *
 * Returns: the filename of the item.
 */
gchar *
na_selected_info_get_path( const NASelectedInfo *nsi )
{
	gchar *path;

	g_return_val_if_fail( NA_IS_SELECTED_INFO( nsi ), NULL );

	path = NULL;

	if( !nsi->private->dispose_has_run ){

		path = g_strdup( nsi->private->vfs->path );
	}

	return( path );
}

/**
 * na_selected_info_get_uri:
 * @nsi: this #NASelectedInfo object.
 *
 * Returns: the URI associated with this #NASelectedInfo object, as a
 * newly allocated string which should be g_free() by the caller.
 */
gchar *
na_selected_info_get_uri( const NASelectedInfo *nsi )
{
	gchar *uri;

	g_return_val_if_fail( NA_IS_SELECTED_INFO( nsi ), NULL );

	uri = NULL;

	if( !nsi->private->dispose_has_run ){

		uri = g_strdup( nsi->private->uri );
	}

	return( uri );
}

/**
 * na_selected_info_get_uri_scheme:
 * @nsi: this #NASelectedInfo object.
 *
 * Returns: the scheme associated to this @nsi object, as a
 * newly allocated string which should be g_free() by the caller.
 */
gchar *
na_selected_info_get_uri_scheme( const NASelectedInfo *nsi )
{
	gchar *scheme;

	g_return_val_if_fail( NA_IS_SELECTED_INFO( nsi ), NULL );

	scheme = NULL;

	if( !nsi->private->dispose_has_run ){

		scheme = g_strdup( nsi->private->vfs->scheme );
	}

	return( scheme );
}

/**
 * na_selected_info_is_directory:
 * @nsi: this #NASelectedInfo object.
 *
 * Returns: %TRUE if the item is a directory, %FALSE else.
 */
gboolean
na_selected_info_is_directory( const NASelectedInfo *nsi )
{
	gboolean is_dir;

	g_return_val_if_fail( NA_IS_SELECTED_INFO( nsi ), FALSE );

	is_dir = FALSE;

	if( !nsi->private->dispose_has_run ){

		is_dir = ( nsi->private->file_type == G_FILE_TYPE_DIRECTORY );
	}

	return( is_dir );
}

/**
 * na_selected_info_is_executable:
 * @nsi: this #NASelectedInfo object.
 *
 * Returns: %TRUE if the item is executable by the user, %FALSE else.
 */
gboolean
na_selected_info_is_executable( const NASelectedInfo *nsi )
{
	gboolean is_exe;

	g_return_val_if_fail( NA_IS_SELECTED_INFO( nsi ), FALSE );

	is_exe = FALSE;

	if( !nsi->private->dispose_has_run ){

		is_exe = nsi->private->can_execute;
	}

	return( is_exe );
}

/**
 * na_selected_info_is_local:
 * @nsi: this #NASelectedInfo object.
 *
 * Returns: %TRUE if the item is on a local filesystem, %FALSE else.
 */
gboolean
na_selected_info_is_local( const NASelectedInfo *nsi )
{
	gboolean is_local;
	gchar *scheme;

	g_return_val_if_fail( NA_IS_SELECTED_INFO( nsi ), FALSE );

	is_local = FALSE;

	if( !nsi->private->dispose_has_run ){

		scheme = na_selected_info_get_uri_scheme( nsi );
		is_local = ( strcmp( scheme, "file" ) == 0 );
		g_free( scheme );
	}

	return( is_local );
}

/**
 * na_selected_info_is_owner:
 * @nsi: this #NASelectedInfo object.
 * @user: the user to be tested against the owner of the @nsi object.
 *
 * Returns: %TRUE if the item is a owner, %FALSE else.
 */
gboolean
na_selected_info_is_owner( const NASelectedInfo *nsi, const gchar *user )
{
	gboolean is_owner;

	g_return_val_if_fail( NA_IS_SELECTED_INFO( nsi ), FALSE );

	is_owner = FALSE;

	if( !nsi->private->dispose_has_run ){

		is_owner = ( strcmp( nsi->private->owner, user ) == 0 );
	}

	return( is_owner );
}

/**
 * na_selected_info_is_readable:
 * @nsi: this #NASelectedInfo object.
 *
 * Returns: %TRUE if the item is a readable, %FALSE else.
 */
gboolean
na_selected_info_is_readable( const NASelectedInfo *nsi )
{
	gboolean is_readable;

	g_return_val_if_fail( NA_IS_SELECTED_INFO( nsi ), FALSE );

	is_readable = FALSE;

	if( !nsi->private->dispose_has_run ){

		is_readable = nsi->private->can_read;
	}

	return( is_readable );
}

/**
 * na_selected_info_is_writable:
 * @nsi: this #NASelectedInfo object.
 *
 * Returns: %TRUE if the item is a writable, %FALSE else.
 */
gboolean
na_selected_info_is_writable( const NASelectedInfo *nsi )
{
	gboolean is_writable;

	g_return_val_if_fail( NA_IS_SELECTED_INFO( nsi ), FALSE );

	is_writable = FALSE;

	if( !nsi->private->dispose_has_run ){

		is_writable = nsi->private->can_write;
	}

	return( is_writable );
}

/**
 * na_selected_info_create_for_uri:
 * @uri: an URI.
 * @mimetype: the corresponding Nautilus mime type, or %NULL.
 * @errmsg: a pointer to a string which will contain an error message on
 *  return.
 *
 * Returns: a newly allocated #NASelectedInfo object for the given @uri.
 */
NASelectedInfo *
na_selected_info_create_for_uri( const gchar *uri, const gchar *mimetype, gchar **errmsg )
{
	static const gchar *thisfn = "na_selected_info_create_for_uri";

	g_debug( "%s: uri=%s, mimetype=%s", thisfn, uri, mimetype );

	NASelectedInfo *obj = new_from_uri( uri, mimetype, errmsg );

	return( obj );
}

static void
dump( const NASelectedInfo *nsi )
{
	static const gchar *thisfn = "na_selected_info_dump";

	g_debug( "%s:            uri=%s", thisfn, nsi->private->uri );
	g_debug( "%s:       mimetype=%s", thisfn, nsi->private->mimetype );
	g_debug( "%s:      vfs->path=%s", thisfn, nsi->private->vfs->path );
	g_debug( "%s: vfs->host_name=%s", thisfn, nsi->private->vfs->host_name );
	g_debug( "%s: vfs->host_port=%d", thisfn, nsi->private->vfs->host_port );
	g_debug( "%s: vfs->user_name=%s", thisfn, nsi->private->vfs->user_name );
	g_debug( "%s:  vfs->password=%s", thisfn, nsi->private->vfs->password );
}

static NASelectedInfo *
new_from_nautilus_file_info( NautilusFileInfo *item )
{
	gchar *uri = nautilus_file_info_get_uri( item );
	gchar *mimetype = nautilus_file_info_get_mime_type( item );
	NASelectedInfo *info = new_from_uri( uri, mimetype, NULL );
	g_free( mimetype );
	g_free( uri );

	return( info );
}

static NASelectedInfo *
new_from_uri( const gchar *uri, const gchar *mimetype, gchar **errmsg )
{
	NASelectedInfo *info = g_object_new( NA_SELECTED_INFO_TYPE, NULL );

	info->private->uri = g_strdup( uri );
	if( mimetype ){
		info->private->mimetype = g_strdup( mimetype );
	}

	info->private->location = g_file_new_for_uri( uri );
	info->private->vfs = g_new0( NAGnomeVFSURI, 1 );
	na_gnome_vfs_uri_parse( info->private->vfs, info->private->uri );

	if( query_file_attributes( info, errmsg )){
		dump( info );

	} else {
		g_object_unref( info );
		info = NULL;
	}

	return( info );
}

static gboolean
query_file_attributes( NASelectedInfo *nsi, gchar **errmsg )
{
	static const gchar *thisfn = "na_selected_info_query_file_attributes";
	GError *error;

	error = NULL;
	GFileInfo *info = g_file_query_info( nsi->private->location,
			G_FILE_ATTRIBUTE_STANDARD_TYPE
				"," G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE
				"," G_FILE_ATTRIBUTE_ACCESS_CAN_READ
				"," G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE
				"," G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE
				"," G_FILE_ATTRIBUTE_OWNER_USER,
			G_FILE_QUERY_INFO_NONE, NULL, &error );

	if( error ){
		if( errmsg ){
			*errmsg = g_strdup_printf( "Error when querying informations for %s URI: %s", nsi->private->uri, error->message );
		} else {
			g_warning( "%s: g_file_query_info: %s", thisfn, error->message );
		}
		g_error_free( error );
		return( FALSE );
	}

	if( !nsi->private->mimetype ){
		nsi->private->mimetype = g_strdup( g_file_info_get_attribute_as_string( info, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ));
	}

	nsi->private->file_type = ( GFileType ) g_file_info_get_attribute_uint32( info, G_FILE_ATTRIBUTE_STANDARD_TYPE );

	nsi->private->can_read = g_file_info_get_attribute_boolean( info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ );
	nsi->private->can_write = g_file_info_get_attribute_boolean( info, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE );
	nsi->private->can_execute = g_file_info_get_attribute_boolean( info, G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE );

	nsi->private->owner = g_strdup( g_file_info_get_attribute_as_string( info, G_FILE_ATTRIBUTE_OWNER_USER ));

	g_object_unref( info );

	return( TRUE );
}
