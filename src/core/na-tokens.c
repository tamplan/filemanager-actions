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

#include <glib/gi18n.h>
#include <string.h>

#include <api/na-core-utils.h>
#include <api/na-object-api.h>

#include "na-gnome-vfs-uri.h"
#include "na-selected-info.h"
#include "na-tokens.h"

/* private class data
 */
struct _NATokensClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _NATokensPrivate {
	gboolean dispose_has_run;

	guint    count;

	GSList  *uris;
	gchar   *uris_str;
	GSList  *filenames;
	gchar   *filenames_str;
	GSList  *basedirs;
	gchar   *basedirs_str;
	GSList  *basenames;
	gchar   *basenames_str;
	GSList  *basenames_woext;
	gchar   *basenames_woext_str;
	GSList  *exts;
	gchar   *exts_str;
	GSList  *mimetypes;
	gchar   *mimetypes_str;

	gchar   *hostname;
	gchar   *username;
	guint    port;
	gchar   *scheme;
};

static GObjectClass *st_parent_class = NULL;

static GType     register_type( void );
static void      class_init( NATokensClass *klass );
static void      instance_init( GTypeInstance *instance, gpointer klass );
static void      instance_dispose( GObject *object );
static void      instance_finalize( GObject *object );

static NATokens *build_string_lists( NATokens *tokens );
static gchar    *build_string_lists_item( GSList **pslist );
static void      execute_action_command( const gchar *command, const NAObjectProfile *profile );
static gboolean  is_singular_exec( const NATokens *tokens, const gchar *exec );
static gchar    *parse_singular( const NATokens *tokens, const gchar *input, guint i, gboolean utf8 );

GType
na_tokens_get_type( void )
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
	static const gchar *thisfn = "na_tokens_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NATokensClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NATokens ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "NATokens", &info, 0 );

	return( type );
}

static void
class_init( NATokensClass *klass )
{
	static const gchar *thisfn = "na_tokens_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NATokensClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "na_tokens_instance_init";
	NATokens *self;

	g_return_if_fail( NA_IS_TOKENS( instance ));
	self = NA_TOKENS( instance );

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self->private = g_new0( NATokensPrivate, 1 );

	self->private->uris = NULL;
	self->private->uris_str = NULL;
	self->private->filenames = NULL;
	self->private->filenames_str = NULL;
	self->private->basedirs = NULL;
	self->private->basedirs_str = NULL;
	self->private->basenames = NULL;
	self->private->basenames_str = NULL;
	self->private->basenames_woext = NULL;
	self->private->basenames_woext_str = NULL;
	self->private->exts = NULL;
	self->private->exts_str = NULL;
	self->private->mimetypes = NULL;
	self->private->mimetypes_str = NULL;

	self->private->hostname = NULL;
	self->private->username = NULL;
	self->private->port = 0;
	self->private->scheme = NULL;

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "na_tokens_instance_dispose";
	NATokens *self;

	g_return_if_fail( NA_IS_TOKENS( object ));
	self = NA_TOKENS( object );

	if( !self->private->dispose_has_run ){

		g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

		self->private->dispose_has_run = TRUE;

		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "na_tokens_instance_finalize";
	NATokens *self;

	g_return_if_fail( NA_IS_TOKENS( object ));
	self = NA_TOKENS( object );

	g_debug( "%s: object=%p", thisfn, ( void * ) object );

	g_free( self->private->scheme );
	g_free( self->private->username );
	g_free( self->private->hostname );

	g_free( self->private->mimetypes_str );
	na_core_utils_slist_free( self->private->mimetypes );
	g_free( self->private->exts_str );
	na_core_utils_slist_free( self->private->exts );
	g_free( self->private->basenames_woext_str );
	na_core_utils_slist_free( self->private->basenames_woext );
	g_free( self->private->basenames_str );
	na_core_utils_slist_free( self->private->basenames );
	g_free( self->private->basedirs_str );
	na_core_utils_slist_free( self->private->basedirs );
	g_free( self->private->filenames_str );
	na_core_utils_slist_free( self->private->filenames );
	g_free( self->private->uris_str );
	na_core_utils_slist_free( self->private->uris );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/*
 * na_tokens_new_for_example:
 *
 * Returns: a new #NATokens object initialized with fake values for two
 * regular files, in order to be used as an example of an expanded command
 * line.
 */
NATokens *
na_tokens_new_for_example( void )
{
	NATokens *tokens;
	const gchar *ex_uri1 = _( "file:///path/to/file1.mid" );
	const gchar *ex_uri2 = _( "file:///path/to/file2.jpeg" );
	const gchar *ex_mimetype1 = _( "audio/x-midi" );
	const gchar *ex_mimetype2 = _( "image/jpeg" );
	const guint  ex_port = 8080;
	const gchar *ex_host = _( "test.example.net" );
	const gchar *ex_user = _( "user" );
	NAGnomeVFSURI *vfs;
	gchar *dirname, *bname, *bname_woext, *ext;
	GSList *is;
	gboolean first;

	tokens = g_object_new( NA_TOKENS_TYPE, NULL );
	first = TRUE;
	tokens->private->count = 2;

	tokens->private->uris = g_slist_append( tokens->private->uris, g_strdup( ex_uri1 ));
	tokens->private->uris = g_slist_append( tokens->private->uris, g_strdup( ex_uri2 ));

	for( is = tokens->private->uris ; is ; is = is->next ){
		vfs = g_new0( NAGnomeVFSURI, 1 );
		na_gnome_vfs_uri_parse( vfs, is->data );

		tokens->private->filenames = g_slist_append( tokens->private->filenames, g_strdup( vfs->path ));
		dirname = g_path_get_dirname( vfs->path );
		tokens->private->basedirs = g_slist_append( tokens->private->basedirs, dirname );
		bname = g_path_get_basename( vfs->path );
		tokens->private->basenames = g_slist_append( tokens->private->basenames, bname );
		na_core_utils_dir_split_ext( bname, &bname_woext, &ext );
		tokens->private->basenames_woext = g_slist_append( tokens->private->basenames_woext, bname_woext );
		tokens->private->exts = g_slist_append( tokens->private->exts, ext );

		if( first ){
			tokens->private->scheme = g_strdup( vfs->scheme );
			first = FALSE;
		}

		na_gnome_vfs_uri_free( vfs );
	}

	tokens->private->mimetypes = g_slist_append( tokens->private->mimetypes, g_strdup( ex_mimetype1 ));
	tokens->private->mimetypes = g_slist_append( tokens->private->mimetypes, g_strdup( ex_mimetype2 ));

	tokens->private->hostname = g_strdup( ex_host );
	tokens->private->username = g_strdup( ex_user );
	tokens->private->port = ex_port;

	return( build_string_lists( tokens ));
}

/*
 * na_tokens_new_from_selection:
 * @selection: a #GList list of #NASelectedInfo objects.
 *
 * Returns: a new #NATokens object which holds all possible tokens.
 */
NATokens *
na_tokens_new_from_selection( GList *selection )
{
	static const gchar *thisfn = "na_tokens_new_from_selection";
	NATokens *tokens;
	GList *it;
	gchar *uri, *filename, *basedir, *basename, *bname_woext, *ext, *mimetype;
	GFile *location;
	gboolean first;
	NAGnomeVFSURI *vfs;

	g_debug( "%s: selection=%p (count=%d)", thisfn, ( void * ) selection, g_list_length( selection ));

	first = TRUE;
	tokens = g_object_new( NA_TOKENS_TYPE, NULL );

	tokens->private->count = g_list_length( selection );

	for( it = selection ; it ; it = it->next ){
		location = na_selected_info_get_location( NA_SELECTED_INFO( it->data ));
		mimetype = na_selected_info_get_mime_type( NA_SELECTED_INFO( it->data ));

		uri = na_selected_info_get_uri( NA_SELECTED_INFO( it->data ));
		filename = g_file_get_path( location );
		basedir = filename ? g_path_get_dirname( filename ) : NULL;
		basename = g_file_get_basename( location );
		na_core_utils_dir_split_ext( basename, &bname_woext, &ext );

		g_debug( "%s: uri=%s, filename=%s, basedir=%s, basename=%s, bname_woext=%s, ext=%s",
				thisfn, uri, filename, basedir, basename, bname_woext, ext );

		if( first ){
			vfs = g_new0( NAGnomeVFSURI, 1 );
			na_gnome_vfs_uri_parse( vfs, uri );

			tokens->private->hostname = g_strdup( vfs->host_name );
			tokens->private->username = g_strdup( vfs->user_name );
			tokens->private->port = vfs->host_port;
			tokens->private->scheme = g_strdup( vfs->scheme );

			na_gnome_vfs_uri_free( vfs );
			first = FALSE;
		}

		tokens->private->uris = g_slist_prepend( tokens->private->uris, uri );
		tokens->private->filenames = g_slist_prepend( tokens->private->filenames, filename );
		tokens->private->basedirs = g_slist_prepend( tokens->private->basedirs, basedir );
		tokens->private->basenames = g_slist_prepend( tokens->private->basenames, basename );
		tokens->private->basenames_woext = g_slist_prepend( tokens->private->basenames_woext, bname_woext );
		tokens->private->exts = g_slist_prepend( tokens->private->exts, ext );
		tokens->private->mimetypes = g_slist_prepend( tokens->private->mimetypes, mimetype );

		g_object_unref( location );
	}

	return( build_string_lists( tokens ));
}

/*
 * na_tokens_parse_parameters:
 * @tokens: a #NATokens object.
 * @string: the input string, may or may not contain tokens.
 * @utf8: whether the @input string is UTF-8 encoded, or a standard ASCII string.
 *
 * Expands the parameters in the given string.
 *
 * Returns: a copy of @input string with tokens expanded, as a newly
 * allocated string which should be g_free() by the caller.
 */
gchar *
na_tokens_parse_parameters( const NATokens *tokens, const gchar *string, gboolean utf8 )
{
	return( parse_singular( tokens, string, 0, utf8 ));
}

/*
 * na_tokens_execute_action:
 * @tokens: a #NATokens object.
 * @profile: the #NAObjectProfile to be executed.
 *
 * Execute the given action, regarding the context described by @tokens.
 */
void
na_tokens_execute_action( const NATokens *tokens, const NAObjectProfile *profile )
{
	gchar *path, *parameters, *exec, *command;
	gboolean singular;
	guint i;

	path = na_object_get_path( profile );
	parameters = na_object_get_parameters( profile );
	exec = g_strdup_printf( "%s %s", path, parameters );
	g_free( parameters );
	g_free( path );

	singular = is_singular_exec( tokens, exec );

	if( singular ){
		for( i = 0 ; i < tokens->private->count ; ++i ){
			command = parse_singular( tokens, exec, i, FALSE );
			execute_action_command( command, profile );
			g_free( command );
		}

	} else {
		command = na_tokens_parse_parameters( tokens, exec, FALSE );
		execute_action_command( command, profile );
		g_free( command );
	}


	g_free( exec );
}

static NATokens *
build_string_lists( NATokens *tokens )
{
	tokens->private->uris_str            = build_string_lists_item( &tokens->private->uris );
	tokens->private->filenames_str       = build_string_lists_item( &tokens->private->filenames );
	tokens->private->basedirs_str        = build_string_lists_item( &tokens->private->basedirs );
	tokens->private->basenames_str       = build_string_lists_item( &tokens->private->basenames );
	tokens->private->basenames_woext_str = build_string_lists_item( &tokens->private->basenames_woext );
	tokens->private->exts_str            = build_string_lists_item( &tokens->private->exts );
	tokens->private->mimetypes_str       = build_string_lists_item( &tokens->private->mimetypes );

	return( tokens );
}

static gchar *
build_string_lists_item( GSList **pslist )
{
	*pslist = g_slist_reverse( *pslist );

	gchar *str = na_core_utils_slist_join_at_end( *pslist, " " );

	return( str );
}

static void
execute_action_command( const gchar *command, const NAObjectProfile *profile )
{
	static const gchar *thisfn = "nautilus_actions_execute_action_command";

	g_debug( "%s: command=%s, profile=%p", thisfn, command, ( void * ) profile );

	g_spawn_command_line_async( command, NULL );
}

/*
 * na_tokens_is_singular_exec:
 * @tokens: the current #NATokens object.
 * @exec: the to be executed command-line before having been parsed
 *
 * Returns: %TRUE if the first relevant parameter found in @exec
 * command-line is of singular form, %FALSE else.
 */
static gboolean
is_singular_exec( const NATokens *tokens, const gchar *exec )
{
	gboolean singular;
	gboolean found;
	gchar *iter;

	singular = FALSE;
	found = FALSE;
	iter = ( gchar * ) exec;

	while(( iter = g_strstr_len( iter, -1, "%" )) != NULL && !found ){

		switch( iter[1] ){
			case 'b':
			case 'd':
			case 'f':
			case 'm':
			case 'o':
			case 'u':
			case 'w':
			case 'x':
				found = TRUE;
				singular = TRUE;
				break;

			case 'B':
			case 'D':
			case 'F':
			case 'M':
			case 'O':
			case 'U':
			case 'W':
			case 'X':
				found = TRUE;
				singular = FALSE;
				break;

			/* all other parameters are irrelevant according to DES-EMA
			 * c: selection count
			 * h: hostname
			 * n: username
			 * p: port
			 * s: scheme
			 * %: %
			 */
		}

		iter += 2;			/* skip the % sign and the character after */
	}

	return( singular );
}

/*
 * na_tokens_parse_singular:
 * @tokens: a #NATokens object.
 * @input: the input string, may or may not contain tokens.
 * @i: the number of the iteration in a multiple selection, starting with zero.
 * @utf8: whether the @input string is UTF-8 encoded, or a standard ASCII
 *  string.
 *
 * A command is said of 'singular form' when its first parameter is not
 * of plural form. In the case of a multiple selection, singular form
 * commands are executed one time for each element of the selection
 */
static gchar *
parse_singular( const NATokens *tokens, const gchar *input, guint i, gboolean utf8 )
{
	GString *output;
	gchar *iter, *prev_iter, *tmp;
	const gchar *nth;

	output = g_string_new( "" );

	/* return NULL if input is NULL
	 */
	if( !input ){
		return( g_string_free( output, TRUE ));
	}

	/* return an empty string if input is empty
	 */
	if( utf8 ){
		if( !g_utf8_strlen( input, -1 )){
			return( g_string_free( output, FALSE ));
		}
	} else {
		if( !strlen( input )){
			return( g_string_free( output, FALSE ));
		}
	}

	iter = ( gchar * ) input;
	prev_iter = iter;

	while(( iter = g_strstr_len( iter, -1, "%" ))){
		output = g_string_append_len( output, prev_iter, strlen( prev_iter ) - strlen( iter ));

		switch( iter[1] ){
			case 'b':
				if( tokens->private->basenames ){
					nth = ( const gchar * ) g_slist_nth_data( tokens->private->basenames, i );
					if( nth ){
						tmp = g_shell_quote( nth );
						output = g_string_append( output, tmp );
						g_free( tmp );
					}
				}
				break;

			case 'B':
				if( tokens->private->basenames_str ){
					output = g_string_append( output, tokens->private->basenames_str );
				}
				break;

			case 'c':
				g_string_append_printf( output, "%d", tokens->private->count );
				break;

			case 'd':
				if( tokens->private->basedirs ){
					nth = ( const gchar * ) g_slist_nth_data( tokens->private->basedirs, i );
					if( nth ){
						tmp = g_shell_quote( nth );
						output = g_string_append( output, tmp );
						g_free( tmp );
					}
				}
				break;

			case 'D':
				if( tokens->private->basedirs_str ){
					output = g_string_append( output, tokens->private->basedirs_str );
				}
				break;

			case 'f':
				if( tokens->private->filenames ){
					nth = ( const gchar * ) g_slist_nth_data( tokens->private->filenames, i );
					if( nth ){
						tmp = g_shell_quote( nth );
						output = g_string_append( output, tmp );
						g_free( tmp );
					}
				}
				break;

			case 'F':
				if( tokens->private->filenames_str ){
					output = g_string_append( output, tokens->private->filenames_str );
				}
				break;

			case 'h':
				if( tokens->private->hostname ){
					tmp = g_shell_quote( tokens->private->hostname );
					output = g_string_append( output, tmp );
					g_free( tmp );
				}
				break;

			case 'm':
				if( tokens->private->mimetypes ){
					nth = ( const gchar * ) g_slist_nth_data( tokens->private->mimetypes, i );
					if( nth ){
						tmp = g_shell_quote( nth );
						output = g_string_append( output, tmp );
						g_free( tmp );
					}
				}
				break;

			case 'M':
				if( tokens->private->mimetypes_str ){
					output = g_string_append( output, tokens->private->mimetypes_str );
				}
				break;

			/* no-op operators */
			case 'o':
			case 'O':
				break;

			case 'n':
				if( tokens->private->username ){
					tmp = g_shell_quote( tokens->private->username );
					output = g_string_append( output, tmp );
					g_free( tmp );
				}
				break;

			case 'p':
				g_string_append_printf( output, "%d", tokens->private->port );
				break;

			case 's':
				if( tokens->private->scheme ){
					tmp = g_shell_quote( tokens->private->scheme );
					output = g_string_append( output, tmp );
					g_free( tmp );
				}
				break;

			case 'u':
				if( tokens->private->uris ){
					nth = ( const gchar * ) g_slist_nth_data( tokens->private->uris, i );
					if( nth ){
						tmp = g_shell_quote( nth );
						output = g_string_append( output, tmp );
						g_free( tmp );
					}
				}
				break;

			case 'U':
				if( tokens->private->uris_str ){
					output = g_string_append( output, tokens->private->uris_str );
				}
				break;

			case 'w':
				if( tokens->private->basenames_woext ){
					nth = ( const gchar * ) g_slist_nth_data( tokens->private->basenames_woext, i );
					if( nth ){
						tmp = g_shell_quote( nth );
						output = g_string_append( output, tmp );
						g_free( tmp );
					}
				}
				break;

			case 'W':
				if( tokens->private->basenames_woext_str ){
					output = g_string_append( output, tokens->private->basenames_woext_str );
				}
				break;

			case 'x':
				if( tokens->private->exts ){
					nth = ( const gchar * ) g_slist_nth_data( tokens->private->exts, i );
					if( nth ){
						tmp = g_shell_quote( nth );
						output = g_string_append( output, tmp );
						g_free( tmp );
					}
				}
				break;

			case 'X':
				if( tokens->private->exts_str ){
					output = g_string_append( output, tokens->private->exts_str );
				}
				break;

			/* a percent sign
			 */
			case '%':
				output = g_string_append_c( output, '%' );
				break;
		}

		iter += 2;			/* skip the % sign and the character after */
		prev_iter = iter;	/* store the new start of the string */
	}

	output = g_string_append_len( output, prev_iter, strlen( prev_iter ));

	return( g_string_free( output, FALSE ));
}
