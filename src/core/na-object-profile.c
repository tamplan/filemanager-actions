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

#include <glib/gi18n.h>

#include <libnautilus-extension/nautilus-file-info.h>

#include <api/na-core-utils.h>
#include <api/na-idata-factory.h>
#include <api/na-object-api.h>

#include "na-io-factory.h"
#include "na-data-factory.h"
#include "na-dbus.h"
#include "na-gnome-vfs-uri.h"

/* private class data
 */
struct NAObjectProfileClassPrivate {
	void *empty;							/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NAObjectProfilePrivate {
	gboolean dispose_has_run;
};

											/* i18n: default label for a new profile */
#define DEFAULT_PROFILE						N_( "Default profile" )

#define PROFILE_NAME_PREFIX					"profile-"

extern NadfIdGroup profile_id_groups [];	/* defined in na-item-profile-enum.c */

static NAObjectIdClass *st_parent_class = NULL;

static GType    register_type( void );
static void     class_init( NAObjectProfileClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void     instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void     instance_dispose( GObject *object );
static void     instance_finalize( GObject *object );

static void     idata_factory_iface_init( NAIDataFactoryInterface *iface );
static guint    idata_factory_get_version( const NAIDataFactory *instance );
static gchar   *idata_factory_get_default( const NAIDataFactory *instance, const NadfIdType *iddef );
static void     idata_factory_read_done( NAIDataFactory *instance, const NAIIOFactory *reader, void *reader_data, GSList **messages );
static void     idata_factory_write_done( NAIDataFactory *instance, const NAIIOFactory *writer, void *writer_data, GSList **messages );

static gboolean is_target_background_candidate( const NAObjectProfile *profile, NautilusFileInfo *current_folder );
static gboolean is_target_toolbar_candidate( const NAObjectProfile *profile, NautilusFileInfo *current_folder );
static gboolean is_current_folder_inside( const NAObjectProfile *profile, NautilusFileInfo *current_folder );
static gboolean is_target_selection_candidate( const NAObjectProfile *profile, GList *files, gboolean from_nautilus );

static gchar   *parse_parameters( const NAObjectProfile *profile, gint target, GList* files, gboolean from_nautilus );
static gboolean tracked_is_directory( void *iter, gboolean from_nautilus );
static gchar   *tracked_to_basename( void *iter, gboolean from_nautilus );
static GFile   *tracked_to_location( void *iter, gboolean from_nautilus );
static gchar   *tracked_to_mimetype( void *iter, gboolean from_nautilus );
static gchar   *tracked_to_scheme( void *iter, gboolean from_nautilus );
static gchar   *tracked_to_uri( void *iter, gboolean from_nautilus );
static int      validate_schemes( GSList *schemes2test, void *iter, gboolean from_nautilus );

GType
na_object_profile_get_type( void )
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
	static const gchar *thisfn = "na_object_profile_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NAObjectProfileClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NAObjectProfile ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	static const GInterfaceInfo idata_factory_iface_info = {
		( GInterfaceInitFunc ) idata_factory_iface_init,
		NULL,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( NA_OBJECT_ID_TYPE, "NAObjectProfile", &info, 0 );

	g_type_add_interface_static( type, NA_IDATA_FACTORY_TYPE, &idata_factory_iface_info );

	na_io_factory_register( type, profile_id_groups );

	return( type );
}

static void
class_init( NAObjectProfileClass *klass )
{
	static const gchar *thisfn = "na_object_profile_class_init";
	GObjectClass *object_class;
	NAObjectClass *naobject_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->set_property = instance_set_property;
	object_class->get_property = instance_get_property;
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	naobject_class = NA_OBJECT_CLASS( klass );
	naobject_class->dump = NULL;
	naobject_class->copy = NULL;
	naobject_class->are_equal = NULL;
	naobject_class->is_valid = NULL;

	klass->private = g_new0( NAObjectProfileClassPrivate, 1 );

	na_data_factory_properties( object_class );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "na_object_profile_instance_init";
	NAObjectProfile *self;

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	g_return_if_fail( NA_IS_OBJECT_PROFILE( instance ));

	self = NA_OBJECT_PROFILE( instance );

	self->private = g_new0( NAObjectProfilePrivate, 1 );

	self->private->dispose_has_run = FALSE;

	na_data_factory_init( NA_IDATA_FACTORY( instance ));
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	g_return_if_fail( NA_IS_OBJECT_PROFILE( object ));
	g_return_if_fail( NA_IS_IDATA_FACTORY( object ));

	if( !NA_OBJECT_PROFILE( object )->private->dispose_has_run ){

		na_data_factory_set_value( NA_IDATA_FACTORY( object ), property_id, value, spec );
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	g_return_if_fail( NA_IS_OBJECT_PROFILE( object ));
	g_return_if_fail( NA_IS_IDATA_FACTORY( object ));

	if( !NA_OBJECT_PROFILE( object )->private->dispose_has_run ){

		na_data_factory_get_value( NA_IDATA_FACTORY( object ), property_id, value, spec );
	}
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "na_object_profile_instance_dispose";
	NAObjectProfile *self;

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	g_return_if_fail( NA_IS_OBJECT_PROFILE( object ));

	self = NA_OBJECT_PROFILE( object );

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
	static const gchar *thisfn = "na_object_profile_instance_finalize";
	NAObjectProfile *self;

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	g_return_if_fail( NA_IS_OBJECT_PROFILE( object ));

	self = NA_OBJECT_PROFILE( object );

	g_free( self->private );

	na_data_factory_finalize( NA_IDATA_FACTORY( object ));

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

static void
idata_factory_iface_init( NAIDataFactoryInterface *iface )
{
	static const gchar *thisfn = "na_object_menu_idata_factory_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->get_version = idata_factory_get_version;
	iface->get_default = idata_factory_get_default;
	iface->copy = NULL;
	iface->are_equal = NULL;
	iface->read_done = idata_factory_read_done;
	iface->write_done = idata_factory_write_done;
}

static guint
idata_factory_get_version( const NAIDataFactory *instance )
{
	return( 1 );
}

static gchar *
idata_factory_get_default( const NAIDataFactory *instance, const NadfIdType *iddef )
{
	gchar *value;

	value = NULL;

	switch( iddef->id ){

		case NADF_DATA_ID:
			value = g_strdup( PROFILE_NAME_PREFIX "zero" );
			break;

		case NADF_DATA_LABEL:
			value = g_strdup( DEFAULT_PROFILE );
			break;
	}

	return( value );
}

static void
idata_factory_read_done( NAIDataFactory *instance, const NAIIOFactory *reader, void *reader_data, GSList **messages )
{

}

static void
idata_factory_write_done( NAIDataFactory *instance, const NAIIOFactory *writer, void *writer_data, GSList **messages )
{

}

/**
 * na_object_profile_new:
 *
 * Allocates a new profile.
 *
 * Returns: the newly allocated #NAObjectProfile profile.
 */
NAObjectProfile *
na_object_profile_new( void )
{
	NAObjectProfile *profile;

	profile = g_object_new( NA_OBJECT_PROFILE_TYPE, NULL );

	return( profile );
}

/**
 * na_object_profile_is_candidate:
 * @profile: the #NAObjectProfile to be checked.
 * @target: the current target.
 * @files: the currently selected items, as provided by Nautilus.
 *
 * Determines if the given profile is candidate to be displayed in the
 * Nautilus context menu, regarding the list of currently selected
 * items.
 *
 * Returns: %TRUE if this profile succeeds to all tests and is so a
 * valid candidate to be displayed in Nautilus context menu, %FALSE
 * else.
 *
 * This method could have been left outside of the #NAObjectProfile
 * class, as it is only called by the plugin. Nonetheless, it is much
 * more easier to code here (because we don't need all get methods, nor
 * free the parameters after).
 */
gboolean
na_object_profile_is_candidate( const NAObjectProfile *profile, gint target, GList *files )
{
	gboolean is_candidate;

	g_return_val_if_fail( NA_IS_OBJECT_PROFILE( profile ), FALSE );

	if( !na_object_is_valid( profile )){
		return( FALSE );
	}

	is_candidate = FALSE;

	switch( target ){
		case ITEM_TARGET_BACKGROUND:
			is_candidate = is_target_background_candidate( profile, ( NautilusFileInfo * ) files->data );
			break;

		case ITEM_TARGET_TOOLBAR:
			is_candidate = is_target_toolbar_candidate( profile, ( NautilusFileInfo * ) files->data );
			break;

		case ITEM_TARGET_SELECTION:
		default:
			is_candidate = is_target_selection_candidate( profile, files, TRUE );
	}

	return( is_candidate );
}

/**
 * na_object_profile_is_candidate_for_tracked:
 * @profile: the #NAObjectProfile to be checked.
 * @files: the currently selected items, as a list of uris.
 *
 * Determines if the given profile is candidate to be displayed in the
 * Nautilus context menu, regarding the list of currently selected
 * items.
 *
 * Returns: %TRUE if this profile succeeds to all tests and is so a
 * valid candidate to be displayed in Nautilus context menu, %FALSE
 * else.
 *
 * The case where we only have URIs for target files is when we have
 * got this list through the org.nautilus_actions.DBus service (or
 * another equivalent) - typically for use in a command-line tool.
 */
gboolean
na_object_profile_is_candidate_for_tracked( const NAObjectProfile *profile, GList *tracked_items )
{
	gboolean is_candidate;

	g_return_val_if_fail( NA_IS_OBJECT_PROFILE( profile ), FALSE );

	if( !na_object_is_valid( profile )){
		return( FALSE );
	}

	is_candidate = is_target_selection_candidate( profile, tracked_items, FALSE );

	return( is_candidate );
}

static gboolean
is_target_background_candidate( const NAObjectProfile *profile, NautilusFileInfo *current_folder )
{
	gboolean is_candidate;

	is_candidate = is_current_folder_inside( profile, current_folder );

	return( is_candidate );
}

static gboolean
is_target_toolbar_candidate( const NAObjectProfile *profile, NautilusFileInfo *current_folder )
{
	gboolean is_candidate;

	is_candidate = is_current_folder_inside( profile, current_folder );

	return( is_candidate );
}

static gboolean
is_current_folder_inside( const NAObjectProfile *profile, NautilusFileInfo *current_folder )
{
	gboolean is_inside;
	GSList *folders, *ifold;
	const gchar *path;
	gchar *current_folder_uri;

	is_inside = FALSE;
	current_folder_uri = nautilus_file_info_get_uri( current_folder );
	folders = na_object_get_folders( profile );

	for( ifold = folders ; ifold && !is_inside ; ifold = ifold->next ){
		path = ( const gchar * ) ifold->data;
		if( path && g_utf8_strlen( path, -1 )){
			if( !strcmp( path, "*" )){
				is_inside = TRUE;
			} else {
				is_inside = g_str_has_prefix( current_folder_uri, path );
				g_debug( "na_object_profile_is_current_folder_inside: current_folder_uri=%s, path=%s, is_inside=%s", current_folder_uri, path, is_inside ? "True":"False" );
			}
		}
	}

	na_core_utils_slist_free( folders );
	g_free( current_folder_uri );

	return( is_inside );
}

static gboolean
is_target_selection_candidate( const NAObjectProfile *profile, GList *files, gboolean from_nautilus )
{
	gboolean retv = FALSE;
	GSList *basenames, *mimetypes, *schemes;
	gboolean matchcase, multiple, isdir, isfile;
	gboolean test_multiple_file = FALSE;
	gboolean test_file_type = FALSE;
	gboolean test_scheme = FALSE;
	gboolean test_basename = FALSE;
	gboolean test_mimetype = FALSE;
	GList* glob_patterns = NULL;
	GList* glob_mime_patterns = NULL;
	GSList* iter;
	GList* iter1;
	GList* iter2;
	guint dir_count = 0;
	guint file_count = 0;
	guint total_count = 0;
	guint scheme_ok_count = 0;
	guint glob_ok_count = 0;
	guint mime_glob_ok_count = 0;
	gboolean basename_match_ok = FALSE;
	gboolean mimetype_match_ok = FALSE;
	gchar *tmp_pattern, *tmp_filename, *tmp_filename2, *tmp_mimetype, *tmp_mimetype2;

	g_return_val_if_fail( NA_IS_OBJECT_PROFILE( profile ), FALSE );

	if( profile->private->dispose_has_run ){
		return( FALSE );
	}

	basenames = na_object_get_basenames( profile );
	matchcase = na_object_is_matchcase( profile );
	multiple = na_object_is_multiple( profile );
	isdir = na_object_is_dir( profile );
	isfile = na_object_is_file( profile );
	mimetypes = na_object_get_mimetypes( profile );
	schemes = na_object_get_schemes( profile );

	if( basenames && basenames->next != NULL &&
			g_ascii_strcasecmp(( gchar * )( basenames->data ), "*" ) == 0 ){
		/* if the only pattern is '*' then all files will match, so it
		 * is not necessary to make the test for each of them
		 */
		test_basename = TRUE;

	} else {
		for (iter = basenames ; iter ; iter = iter->next ){

			tmp_pattern = ( gchar * ) iter->data;
			if( !matchcase ){
				/* --> if case-insensitive asked, lower all the string
				 * since the pattern matching function don't manage it
				 * itself.
				 */
				tmp_pattern = g_ascii_strdown(( gchar * ) iter->data, strlen(( gchar * ) iter->data ));
			}

			glob_patterns = g_list_append( glob_patterns, g_pattern_spec_new( tmp_pattern ));

			if( !matchcase ){
				g_free( tmp_pattern );
			}
		}
	}

	if( mimetypes && mimetypes->next != NULL &&
			( g_ascii_strcasecmp(( gchar * )( mimetypes->data ), "*" ) == 0 ||
			  g_ascii_strcasecmp(( gchar * )( mimetypes->data), "*/*") == 0 )){
		/* if the only pattern is '*' or * / * then all mimetypes will
		 * match, so it is not necessary to make the test for each of them
		 */
		test_mimetype = TRUE;

	} else {
		for( iter = mimetypes ; iter ; iter = iter->next ){
			glob_mime_patterns = g_list_append( glob_mime_patterns, g_pattern_spec_new(( gchar * ) iter->data ));
		}
	}

	for( iter1 = files; iter1; iter1 = iter1->next ){

		tmp_filename = tracked_to_basename( iter1->data, from_nautilus );

		if( tmp_filename ){
			tmp_mimetype = tracked_to_mimetype( iter1->data, from_nautilus );

			if( !matchcase ){
				/* --> if case-insensitive asked, lower all the string
				 * since the pattern matching function don't manage it
				 * itself.
				 */
				tmp_filename2 = g_ascii_strdown( tmp_filename, strlen( tmp_filename ));
				g_free( tmp_filename );
				tmp_filename = tmp_filename2;
			}

			/* --> for the moment we deal with all mimetypes case-insensitively */
			tmp_mimetype2 = g_ascii_strdown( tmp_mimetype, strlen( tmp_mimetype ));
			g_free( tmp_mimetype );
			tmp_mimetype = tmp_mimetype2;

			if( tracked_is_directory( iter1->data, from_nautilus )){
				dir_count++;
			} else {
				file_count++;
			}

			scheme_ok_count += validate_schemes( schemes, iter1->data, from_nautilus );

			if( !test_basename ){ /* if it is already ok, skip the test to improve performance */
				basename_match_ok = FALSE;
				iter2 = glob_patterns;
				while( iter2 && !basename_match_ok ){
					if( g_pattern_match_string(( GPatternSpec * ) iter2->data, tmp_filename )){
						basename_match_ok = TRUE;
					}
					iter2 = iter2->next;
				}

				if( basename_match_ok ){
					glob_ok_count++;
				}
			}

			if( !test_mimetype ){ /* if it is already ok, skip the test to improve performance */
				mimetype_match_ok = FALSE;
				iter2 = glob_mime_patterns;
				while( iter2 && !mimetype_match_ok ){
					if (g_pattern_match_string(( GPatternSpec * ) iter2->data, tmp_mimetype )){
						mimetype_match_ok = TRUE;
					}
					iter2 = iter2->next;
				}

				if( mimetype_match_ok ){
					mime_glob_ok_count++;
				}
			}

			g_free( tmp_mimetype );
			g_free( tmp_filename );
		}

		total_count++;
	}

	if(( files != NULL ) && ( files->next == NULL ) && ( !multiple )){
		test_multiple_file = TRUE;

	} else if( multiple ){
		test_multiple_file = TRUE;
	}

	if( isdir && isfile ){
		if( dir_count > 0 || file_count > 0 ){
			test_file_type = TRUE;
		}
	} else if( isdir && !isfile ){
		if( file_count == 0 ){
			test_file_type = TRUE;
		}
	} else if( !isdir && isfile ){
		if( dir_count == 0 ){
			test_file_type = TRUE;
		}
	}

	if( scheme_ok_count == total_count ){
		test_scheme = TRUE;
	}

	if( !test_basename ){ /* if not already tested */
		if( glob_ok_count == total_count ){
			test_basename = TRUE;
		}
	}

	if( !test_mimetype ){ /* if not already tested */
		if( mime_glob_ok_count == total_count ){
			test_mimetype = TRUE;
		}
	}

	if( test_basename && test_mimetype && test_file_type && test_scheme && test_multiple_file ){
		retv = TRUE;
	}

	g_list_foreach (glob_patterns, (GFunc) g_pattern_spec_free, NULL);
	g_list_free (glob_patterns);
	g_list_foreach (glob_mime_patterns, (GFunc) g_pattern_spec_free, NULL);
	g_list_free (glob_mime_patterns);
	na_core_utils_slist_free( schemes );
	na_core_utils_slist_free( mimetypes );
	na_core_utils_slist_free( basenames );

	return retv;
}

/**
 * Expands the parameters path, in function of the found tokens.
 *
 * @profile: the selected profile.
 * @target: the current target.
 * @files: the list of currently selected items, as provided by Nautilus.
 *
 * Valid parameters are :
 *
 * %d : base dir of the (first) selected file(s)/folder(s)
 * %f : the name of the (first) selected file/folder
 * %h : hostname of the (first) URI
 * %m : list of the basename of the selected files/directories separated by space.
 * %M : list of the selected files/directories with their complete path separated by space.
 * %p : port number from the (first) URI
 * %R : space-separated list of URIs
 * %s : scheme of the (first) URI
 * %u : (first) URI
 * %U : username of the (first) URI
 * %% : a percent sign
 *
 * Adding a parameter requires updating of :
 * - nautilus-actions/private/na-action-profile.c:na_object_profile_parse_parameters()
 * - nautilus-actions/runtime/na-xml-names.h
 * - nautilus-actions/nact/nact-icommand-tab.c:parse_parameters()
 * - nautilus-actions/nact/nautilus-actions-config-tool.ui:LegendDialog
 */
gchar *
na_object_profile_parse_parameters( const NAObjectProfile *profile, gint target, GList* files )
{
	return( parse_parameters( profile, target, files, TRUE ));
}

/**
 * na_object_profile_parse_parameters_for_tracked:
 * @profile: the selected profile.
 * @tracked_items: current selection.
 */
gchar *
na_object_profile_parse_parameters_for_tracked( const NAObjectProfile *profile, GList *tracked_items )
{
	return( parse_parameters( profile, ITEM_TARGET_SELECTION, tracked_items, FALSE ));
}

/*
 * Expands the parameters path, in function of the found tokens.
 */
static gchar *
parse_parameters( const NAObjectProfile *profile, gint target, GList* files, gboolean from_nautilus )
{
	gchar *parsed = NULL;
	GString *string;
	GList *ifi;
	gboolean first;
	gchar *iuri, *ipath, *ibname;
	GFile *iloc;
	gchar *uri = NULL;
	gchar *scheme = NULL;
	gchar *dirname = NULL;
	gchar *filename = NULL;
	gchar *hostname = NULL;
	gchar *username = NULL;
	gint port_number = 0;
	GString *basename_list, *pathname_list, *uris_list;
	gchar *tmp, *iter, *old_iter;
	NAGnomeVFSURI *vfs;

	g_return_val_if_fail( NA_IS_OBJECT_PROFILE( profile ), NULL );

	if( profile->private->dispose_has_run ){
		return( NULL );
	}

	string = g_string_new( "" );
	basename_list = g_string_new( "" );
	pathname_list = g_string_new( "" );
	uris_list = g_string_new( "" );
	first = TRUE;

	for( ifi = files ; ifi ; ifi = ifi->next ){

		iuri = tracked_to_uri( ifi->data, from_nautilus );
		iloc = tracked_to_location( ifi->data, from_nautilus );
		ipath = g_file_get_path( iloc );
		ibname = g_file_get_basename( iloc );

		if( first ){

			vfs = g_new0( NAGnomeVFSURI, 1 );
			na_gnome_vfs_uri_parse( vfs, iuri );

			uri = g_strdup( iuri );
			dirname = g_path_get_dirname( ipath );
			scheme = g_strdup( vfs->scheme );
			filename = g_strdup( ibname );
			hostname = g_strdup( vfs->host_name );
			username = g_strdup( vfs->user_name );
			port_number = vfs->host_port;

			first = FALSE;
			na_gnome_vfs_uri_free( vfs );
		}

		tmp = g_shell_quote( ibname );
		g_string_append_printf( basename_list, " %s", tmp );
		g_free( tmp );

		tmp = g_shell_quote( ipath );
		g_string_append_printf( pathname_list, " %s", tmp );
		g_free( tmp );

		tmp = g_shell_quote( iuri );
		g_string_append_printf( uris_list, " %s", tmp );
		g_free( tmp );

		g_free( ibname );
		g_free( ipath );
		g_object_unref( iloc );
		g_free( iuri );
	}

	iter = na_object_get_parameters( profile );
	old_iter = iter;

	while(( iter = g_strstr_len( iter, strlen( iter ), "%" ))){

		string = g_string_append_len( string, old_iter, strlen( old_iter ) - strlen( iter ));
		switch( iter[1] ){

			/* base dir of the (first) selected item
			 */
			case 'd':
				tmp = g_shell_quote( dirname );
				string = g_string_append( string, tmp );
				g_free( tmp );
				break;

			/* basename of the (first) selected item
			 */
			case 'f':
				tmp = g_shell_quote( filename );
				string = g_string_append( string, tmp );
				g_free( tmp );
				break;

			/* hostname of the (first) URI
			 */
			case 'h':
				string = g_string_append( string, hostname );
				break;

			/* space-separated list of the basenames
			 */
			case 'm':
				string = g_string_append( string, basename_list->str );
				break;

			/* space-separated list of full pathnames
			 */
			case 'M':
				string = g_string_append( string, pathname_list->str );
				break;

			/* port number of the (first) URI
			 */
			case 'p':
				if( port_number > 0 ){
					g_string_append_printf( string, "%d", port_number );
				}
				break;

			/* list of URIs
			 */
			case 'R':
				string = g_string_append( string, uris_list->str );
				break;

			/* scheme of the (first) URI
			 */
			case 's':
				string = g_string_append( string, scheme );
				break;

			/* URI of the first item
			 */
			case 'u':
				string = g_string_append( string, uri );
				break;

			/* username of the (first) URI
			 */
			case 'U':
				string = g_string_append( string, username );
				break;

			/* a percent sign
			 */
			case '%':
				string = g_string_append_c( string, '%' );
				break;
		}

		iter += 2;			/* skip the % sign and the character after */
		old_iter = iter;	/* store the new start of the string */
	}

	string = g_string_append_len( string, old_iter, strlen( old_iter ));

	g_free( uri );
	g_free( dirname );
	g_free( scheme );
	g_free( hostname );
	g_free( username );
	g_free( iter );
	g_string_free( uris_list, TRUE );
	g_string_free( basename_list, TRUE );
	g_string_free( pathname_list, TRUE );

	parsed = g_string_free( string, FALSE );
	return( parsed );
}

static gboolean
tracked_is_directory( void *iter, gboolean from_nautilus )
{
	gboolean is_dir;
	GFile *file;
	GFileType type;

	if( from_nautilus ){
		is_dir = nautilus_file_info_is_directory(( NautilusFileInfo * ) iter );

	} else {
		file = g_file_new_for_uri((( NATrackedItem * ) iter )->uri );
		type = g_file_query_file_type( file, G_FILE_QUERY_INFO_NONE, NULL );
		is_dir = ( type == G_FILE_TYPE_DIRECTORY );
		g_object_unref( file );
	}

	return( is_dir );
}

static gchar *
tracked_to_basename( void *iter, gboolean from_nautilus )
{
	gchar *bname;
	GFile *file;

	if( from_nautilus ){
		bname = nautilus_file_info_get_name(( NautilusFileInfo * ) iter );

	} else {
		file = g_file_new_for_uri((( NATrackedItem * ) iter )->uri );
		bname = g_file_get_basename( file );
		g_object_unref( file );
	}

	return( bname );
}

static GFile *
tracked_to_location( void *iter, gboolean from_nautilus )
{
	GFile *file;

	if( from_nautilus ){
		file = nautilus_file_info_get_location(( NautilusFileInfo * ) iter );
	} else {
		file = g_file_new_for_uri((( NATrackedItem * ) iter )->uri );
	}

	return( file );
}

static gchar *
tracked_to_mimetype( void *iter, gboolean from_nautilus )
{
	gchar *type;
	NATrackedItem *tracked;
	GFile *file;
	GFileInfo *info;

	type = NULL;
	if( from_nautilus ){
		type = nautilus_file_info_get_mime_type(( NautilusFileInfo * ) iter );

	} else {
		tracked = ( NATrackedItem * ) iter;
		if( tracked->mimetype ){
			type = g_strdup( tracked->mimetype );

		} else {
			file = g_file_new_for_uri((( NATrackedItem * ) iter )->uri );
			info = g_file_query_info( file, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, G_FILE_QUERY_INFO_NONE, NULL, NULL );
			if( info ){
				type = g_strdup( g_file_info_get_content_type( info ));
				g_object_unref( info );
			}
			g_object_unref( file );
		}
	}

	return( type );
}

static gchar *
tracked_to_scheme( void *iter, gboolean from_nautilus )
{
	gchar *scheme;
	NAGnomeVFSURI *vfs;

	if( from_nautilus ){
		scheme = nautilus_file_info_get_uri_scheme(( NautilusFileInfo * ) iter );

	} else {
		vfs = g_new0( NAGnomeVFSURI, 1 );
		na_gnome_vfs_uri_parse( vfs, (( NATrackedItem * ) iter )->uri );
		scheme = g_strdup( vfs->scheme );
		na_gnome_vfs_uri_free( vfs );
	}

	return( scheme );
}

static gchar *
tracked_to_uri( void *iter, gboolean from_nautilus )
{
	gchar *uri;

	if( from_nautilus ){
		uri = nautilus_file_info_get_uri(( NautilusFileInfo * ) iter );
	} else {
		uri = g_strdup((( NATrackedItem * ) iter )->uri );
	}

	return( uri );
}

static int
validate_schemes( GSList* schemes2test, void* tracked_iter, gboolean from_nautilus )
{
	int retv = 0;
	GSList* iter;
	gboolean found = FALSE;
	gchar *scheme;

	iter = schemes2test;
	while( iter && !found ){

		scheme = tracked_to_scheme( tracked_iter, from_nautilus );

		if( g_ascii_strncasecmp( scheme, ( gchar * ) iter->data, strlen(( gchar * ) iter->data )) == 0 ){
			found = TRUE;
			retv = 1;
		}

		g_free( scheme );
		iter = iter->next;
	}

	return retv;
}
