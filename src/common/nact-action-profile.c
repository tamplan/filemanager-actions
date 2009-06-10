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

#include <string.h>

#include <libgnomevfs/gnome-vfs.h>

#include <libnautilus-extension/nautilus-file-info.h>

#include "nact-action.h"
#include "nact-action-profile.h"
#include "nact-uti-lists.h"

/* private class data
 */
struct NactActionProfileClassPrivate {
};

/* private instance data
 */
struct NactActionProfilePrivate {
	gboolean  dispose_has_run;

	/* the NactAction object
	 */
	gpointer  action;

	/* profile properties
	 */
	gchar    *name;
	gchar    *label;
	gchar    *path;
	gchar    *parameters;
	gboolean  accept_multiple_files;
	GSList   *basenames;
	gboolean  is_dir;
	gboolean  is_file;
	gboolean  match_case;
	GSList   *mimetypes;
	GSList   *schemes;
};

/* private instance properties
 * please note that property names must have the same spelling as the
 * NactIIOProvider parameters
 */
enum {
	PROP_ACTION = 1,
	PROP_PROFILE_NAME,
	PROP_LABEL,
	PROP_PATH,
	PROP_PARAMETERS,
	PROP_ACCEPT_MULTIPLE,
	PROP_BASENAMES,
	PROP_ISDIR,
	PROP_ISFILE,
	PROP_MATCHCASE,
	PROP_MIMETYPES,
	PROP_SCHEMES
};

#define PROP_ACTION_STR					"action"
#define PROP_PROFILE_NAME_STR			"name"
#define PROP_LABEL_STR					"desc-name"
#define PROP_PATH_STR					"path"
#define PROP_PARAMETERS_STR				"parameters"
#define PROP_ACCEPT_MULTIPLE_STR		"accept-multiple-files"
#define PROP_BASENAMES_STR				"basenames"
#define PROP_ISDIR_STR					"isdir"
#define PROP_ISFILE_STR					"isfile"
#define PROP_MATCHCASE_STR				"matchcase"
#define PROP_MIMETYPES_STR				"mimetypes"
#define PROP_SCHEMES_STR				"schemes"

static NactObjectClass *st_parent_class = NULL;

static GType  register_type( void );
static void   class_init( NactActionProfileClass *klass );
static void   instance_init( GTypeInstance *instance, gpointer klass );
static void   instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void   instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void   instance_dispose( GObject *object );
static void   instance_finalize( GObject *object );

static void   do_dump( const NactObject *profile );
static void   do_dump_list( const gchar *thisfn, const gchar *label, GSList *list );
static gchar *do_get_id( const NactObject *object );
static gchar *do_get_label( const NactObject *object );
static int    validate_schemes( GSList* schemes2test, NautilusFileInfo* file );

GType
nact_action_profile_get_type( void )
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
	static GTypeInfo info = {
		sizeof( NactActionProfileClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactActionProfile ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	return( g_type_register_static( NACT_OBJECT_TYPE, "NactActionProfile", &info, 0 ));
}

static void
class_init( NactActionProfileClass *klass )
{
	static const gchar *thisfn = "nact_action_profile_class_init";
	g_debug( "%s: klass=%p", thisfn, klass );

	st_parent_class = g_type_class_peek_parent( klass );

	GObjectClass *object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
	object_class->set_property = instance_set_property;
	object_class->get_property = instance_get_property;

	GParamSpec *spec;
	spec = g_param_spec_pointer(
			PROP_ACTION_STR,
			PROP_ACTION_STR,
			"The NactAction object",
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_ACTION, spec );

	/* the id of the object is marked as G_PARAM_CONSTRUCT_ONLY */
	spec = g_param_spec_string(
			PROP_PROFILE_NAME_STR,
			PROP_PROFILE_NAME_STR,
			"Internal profile's name", "",
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_PROFILE_NAME, spec );

	spec = g_param_spec_string(
			PROP_LABEL_STR,
			PROP_LABEL_STR,
			"Displayable profile's name", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_LABEL, spec );

	spec = g_param_spec_string(
			PROP_PATH_STR,
			PROP_PATH_STR,
			"Command path", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_PATH, spec );

	spec = g_param_spec_string(
			PROP_PARAMETERS_STR,
			PROP_PARAMETERS_STR,
			"Command parameters", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_PARAMETERS, spec );

	spec = g_param_spec_boolean(
			PROP_ACCEPT_MULTIPLE_STR,
			PROP_ACCEPT_MULTIPLE_STR,
			"Whether apply when multiple files may be selected", TRUE,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_ACCEPT_MULTIPLE, spec );

	spec = g_param_spec_pointer(
			PROP_BASENAMES_STR,
			PROP_BASENAMES_STR,
			"Filenames mask",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_BASENAMES, spec );

	spec = g_param_spec_boolean(
			PROP_ISDIR_STR,
			PROP_ISDIR_STR,
			"Whether apply when a dir is selected", FALSE,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_ISDIR, spec );

	spec = g_param_spec_boolean(
			PROP_ISFILE_STR,
			PROP_ISFILE_STR,
			"Whether apply when a file is selected", TRUE,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_ISFILE, spec );

	spec = g_param_spec_boolean(
			PROP_MATCHCASE_STR,
			PROP_MATCHCASE_STR,
			"Whether the filenames are case sensitive", TRUE,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_MATCHCASE, spec );

	spec = g_param_spec_pointer(
			PROP_MIMETYPES_STR,
			PROP_MIMETYPES_STR,
			"List of selectable mimetypes",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_MIMETYPES, spec );

	spec = g_param_spec_pointer(
			PROP_SCHEMES_STR,
			PROP_SCHEMES_STR,
			"list of selectable schemes",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_SCHEMES, spec );

	klass->private = g_new0( NactActionProfileClassPrivate, 1 );

	NACT_OBJECT_CLASS( klass )->dump = do_dump;
	NACT_OBJECT_CLASS( klass )->get_id = do_get_id;
	NACT_OBJECT_CLASS( klass )->get_label = do_get_label;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_action_profile_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );

	g_assert( NACT_IS_ACTION_PROFILE( instance ));
	NactActionProfile* self = NACT_ACTION_PROFILE( instance );

	self->private = g_new0( NactActionProfilePrivate, 1 );
	self->private->dispose_has_run = FALSE;
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	g_assert( NACT_IS_ACTION_PROFILE( object ));
	NactActionProfile *self = NACT_ACTION_PROFILE( object );

	GSList *list;

	switch( property_id ){
		case PROP_ACTION:
			g_value_set_pointer( value, self->private->action );
			break;

		case PROP_PROFILE_NAME:
			g_value_set_string( value, self->private->name );
			break;

		case PROP_LABEL:
			g_value_set_string( value, self->private->label );
			break;

		case PROP_PATH:
			g_value_set_string( value, self->private->path );
			break;

		case PROP_PARAMETERS:
			g_value_set_string( value, self->private->parameters );
			break;

		case PROP_ACCEPT_MULTIPLE:
			g_value_set_boolean( value, self->private->accept_multiple_files );
			break;

		case PROP_BASENAMES:
			list = nactuti_duplicate_string_list( self->private->basenames );
			g_value_set_pointer( value, list );
			break;

		case PROP_ISDIR:
			g_value_set_boolean( value, self->private->is_dir );
			break;

		case PROP_ISFILE:
			g_value_set_boolean( value, self->private->is_file );
			break;

		case PROP_MATCHCASE:
			g_value_set_boolean( value, self->private->match_case );
			break;

		case PROP_MIMETYPES:
			list = nactuti_duplicate_string_list( self->private->mimetypes );
			g_value_set_pointer( value, list );
			break;

		case PROP_SCHEMES:
			list = nactuti_duplicate_string_list( self->private->schemes );
			g_value_set_pointer( value, list );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	g_assert( NACT_IS_ACTION_PROFILE( object ));
	NactActionProfile *self = NACT_ACTION_PROFILE( object );

	switch( property_id ){
		case PROP_ACTION:
			self->private->action = g_value_get_pointer( value );
			break;

		case PROP_PROFILE_NAME:
			g_free( self->private->name );
			self->private->name = g_value_dup_string( value );
			break;

		case PROP_LABEL:
			g_free( self->private->label );
			self->private->label = g_value_dup_string( value );
			break;

		case PROP_PATH:
			g_free( self->private->path );
			self->private->path = g_value_dup_string( value );
			break;

		case PROP_PARAMETERS:
			g_free( self->private->parameters );
			self->private->parameters = g_value_dup_string( value );
			break;

		case PROP_ACCEPT_MULTIPLE:
			self->private->accept_multiple_files = g_value_get_boolean( value );
			break;

		case PROP_BASENAMES:
			nactuti_free_string_list( self->private->basenames );
			self->private->basenames = nactuti_duplicate_string_list( g_value_get_pointer( value ));
			break;

		case PROP_ISDIR:
			self->private->is_dir = g_value_get_boolean( value );
			break;

		case PROP_ISFILE:
			self->private->is_file = g_value_get_boolean( value );
			break;

		case PROP_MATCHCASE:
			self->private->match_case = g_value_get_boolean( value );
			break;

		case PROP_MIMETYPES:
			nactuti_free_string_list( self->private->mimetypes );
			self->private->mimetypes = nactuti_duplicate_string_list( g_value_get_pointer( value ));
			break;

		case PROP_SCHEMES:
			nactuti_free_string_list( self->private->schemes );
			self->private->schemes = nactuti_duplicate_string_list( g_value_get_pointer( value ));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "nact_action_profile_instance_dispose";
	g_debug( "%s: object=%p", thisfn, object );

	g_assert( NACT_IS_ACTION_PROFILE( object ));
	NactActionProfile *self = NACT_ACTION_PROFILE( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		G_OBJECT_CLASS( st_parent_class )->dispose( object );
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "nact_action_profile_instance_finalize";
	g_debug( "%s: object=%p", thisfn, object );

	g_assert( NACT_IS_ACTION_PROFILE( object ));
	NactActionProfile *self = ( NactActionProfile * ) object;

	g_free( self->private->name );
	g_free( self->private->label );
	g_free( self->private->path );
	g_free( self->private->parameters );
	nactuti_free_string_list( self->private->basenames );
	nactuti_free_string_list( self->private->mimetypes );
	nactuti_free_string_list( self->private->schemes );

	/* chain call to parent class */
	if((( GObjectClass * ) st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * Allocates a new profile for an action.
 *
 * @action: the action to which the profile must be attached.
 *
 * @name: the internal name (identifier) of the profile.
 *
 * Returns the newly allocated NactActionProfile object.
 */
NactActionProfile *
nact_action_profile_new( const NactObject *action, const gchar *name )
{
	g_assert( NACT_IS_ACTION( action ));
	g_assert( name && strlen( name ));

	NactActionProfile *profile =
		g_object_new(
				NACT_ACTION_PROFILE_TYPE,
				PROP_ACTION_STR, action, PROP_PROFILE_NAME_STR, name, NULL );

	return( profile );
}

/**
 * Duplicates a profile.
 *
 * @profile: the profile to be duplicated.
 *
 * Returns the newly allocated NactActionProfile object.
 *
 * Note the duplicated profile has the same internal name (identifier)
 * as the initial one, and thus cannot be attached to the same action.
 */
NactActionProfile *
nact_action_profile_copy( const NactActionProfile *profile )
{
	g_assert( NACT_IS_ACTION_PROFILE( profile ));

	NactActionProfile *new =
		nact_action_profile_new( profile->private->action, profile->private->name );

	g_object_set( G_OBJECT( new ),
			PROP_LABEL_STR, profile->private->label,
			PROP_PATH_STR, profile->private->path,
			PROP_PARAMETERS_STR, profile->private->parameters,
			PROP_ACCEPT_MULTIPLE_STR, profile->private->accept_multiple_files,
			PROP_BASENAMES_STR, profile->private->basenames,
			PROP_ISDIR_STR, profile->private->is_dir,
			PROP_ISFILE_STR, profile->private->is_file,
			PROP_MATCHCASE_STR, profile->private->match_case,
			PROP_MIMETYPES_STR, profile->private->mimetypes,
			PROP_SCHEMES_STR, profile->private->schemes,
			NULL );

	return( new );
}

/**
 * Frees a profile.
 *
 * @profile: the NactActionProfile object to be freed.
 */
void
nact_action_profile_free( NactActionProfile *profile )
{
	g_assert( NACT_IS_ACTION_PROFILE( profile ));
	g_object_unref( profile );
}

static void
do_dump( const NactObject *object )
{
	static const gchar *thisfn = "nact_action_profile_do_dump";

	g_assert( NACT_IS_ACTION_PROFILE( object ));
	NactActionProfile *self = NACT_ACTION_PROFILE( object );

	if( st_parent_class->dump ){
		st_parent_class->dump( object );
	}

	g_debug( "%s:         profile_name='%s'", thisfn, self->private->name );
	g_debug( "%s:                label='%s'", thisfn, self->private->label );
	g_debug( "%s:                 path='%s'", thisfn, self->private->path );
	g_debug( "%s:           parameters='%s'", thisfn, self->private->parameters );
	g_debug( "%s: accept_multiple_file='%s'", thisfn, self->private->accept_multiple_files ? "True" : "False" );
	g_debug( "%s:               is_dir='%s'", thisfn, self->private->is_dir ? "True" : "False" );
	g_debug( "%s:              is_file='%s'", thisfn, self->private->is_file ? "True" : "False" );
	g_debug( "%s:           match_case='%s'", thisfn, self->private->match_case ? "True" : "False" );
	do_dump_list( thisfn, "basenames", self->private->basenames );
	do_dump_list( thisfn, "mimetypes", self->private->mimetypes );
	do_dump_list( thisfn, "  schemes", self->private->schemes );
}

static void
do_dump_list( const gchar *thisfn, const gchar *label, GSList *list )
{
	GString *str;
	str = g_string_new( "[" );
	GSList *item;
	for( item = list ; item != NULL ; item = item->next ){
		if( item != list ){
			g_string_append_printf( str, "," );
		}
		g_string_append_printf( str, "'%s'", ( gchar * ) item->data );
	}
	g_string_append_printf( str, "]" );
	g_debug( "%s:            %s=%s", thisfn, label, str->str );
	g_string_free( str, TRUE );
}

static gchar *
do_get_id( const NactObject *profile )
{
	g_assert( NACT_IS_ACTION_PROFILE( profile ));

	gchar *name;
	g_object_get( G_OBJECT( profile ), PROP_PROFILE_NAME_STR, &name, NULL );

	return( name );
}

/**
 * Returns the internal name (identifier) of the profile.
 *
 * @action: an NactActionProfile object.
 *
 * The returned string must be g_freed by the caller.
 */
gchar *
nact_action_profile_get_name( const NactActionProfile *profile )
{
	g_assert( NACT_IS_ACTION_PROFILE( profile ));
	return( nact_object_get_id( NACT_OBJECT( profile )));
}

static gchar *
do_get_label( const NactObject *profile )
{
	g_assert( NACT_IS_ACTION_PROFILE( profile ));

	gchar *label;
	g_object_get( G_OBJECT( profile ), PROP_LABEL_STR, &label, NULL );

	return( label );
}

/**
 * Returns the descriptive name (label) of the profile.
 *
 * @action: an NactAction object.
 *
 * The returned string must be g_freed by the caller.
 */
gchar *
nact_action_profile_get_label( const NactActionProfile *profile )
{
	g_assert( NACT_IS_ACTION_PROFILE( profile ));
	return( nact_object_get_label( NACT_OBJECT( profile )));
}

/**
 * Returns a pointer to the action for this profile.
 *
 * @profile: the NactActionProfile object whose parent action is to be
 * retrieved.
 *
 * Note that the returned NactNaction is owned by the profile. The
 * caller should not try to free or unref it.
 */
NactObject *
nact_action_profile_get_action( const NactActionProfile *profile )
{
	g_assert( NACT_IS_ACTION_PROFILE( profile ));

	gpointer action;
	g_object_get( G_OBJECT( profile ), PROP_ACTION_STR, &action, NULL );

	return( NACT_OBJECT( action ));
}

/**
 * Returns the path of the command in the profile.
 *
 * @profile: the NactActionProfile object whose command path is to be
 * retrieved.
 *
 * The returned string should be g_freed by the caller.
 */
gchar *
nact_action_profile_get_path( const NactActionProfile *profile )
{
	g_assert( NACT_IS_ACTION_PROFILE( profile ));

	gchar *path;
	g_object_get( G_OBJECT( profile ), PROP_PATH_STR, &path, NULL );

	return( path );
}

/**
 * Returns the parameters of the command in the profile.
 *
 * @profile: the NactActionProfile object whose command parameters are
 * to be retrieved.
 *
 * The returned string should be g_freed by the caller.
 */
gchar *
nact_action_profile_get_parameters( const NactActionProfile *profile )
{
	g_assert( NACT_IS_ACTION_PROFILE( profile ));

	gchar *parameters;
	g_object_get( G_OBJECT( profile ), PROP_PARAMETERS_STR, &parameters, NULL );

	return( parameters );
}

static int
validate_schemes( GSList* schemes2test, NautilusFileInfo* file )
{
	int retv = 0;
	GSList* iter;
	gboolean found = FALSE;

	iter = schemes2test;
	while (iter && !found)
	{
		gchar* scheme = nautilus_file_info_get_uri_scheme (file);

		if (g_ascii_strncasecmp (scheme, (gchar*)iter->data, strlen ((gchar*)iter->data)) == 0)
		{
			found = TRUE;
			retv = 1;
		}

		g_free (scheme);
		iter = iter->next;
	}

	return retv;
}

/**
 * Determines if the given profile is candidate to be displayed in the
 * Nautilus context menu, regarding the list of currently selected
 * items.
 *
 * @profile: the profile to be examined.
 *
 * @files: the list currently selected items, as provided by Nautilus.
 */
gboolean
nact_action_profile_is_candidate( const NactActionProfile *profile, GList* files )
{
	gboolean retv = FALSE;
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

	if (profile->private->basenames && profile->private->basenames->next != NULL &&
			g_ascii_strcasecmp ((gchar*)(profile->private->basenames->data), "*") == 0)
	{
		/* if the only pattern is '*' then all files will match, so it
		 * is not necessary to make the test for each of them
		 */
		test_basename = TRUE;
	}
	else
	{
		for (iter = profile->private->basenames; iter; iter = iter->next)
		{
			gchar* tmp_pattern = (gchar*)iter->data;
			if (!profile->private->match_case)
			{
				/* --> if case-insensitive asked, lower all the string
				 * since the pattern matching function don't manage it
				 * itself.
				 */
				tmp_pattern = g_ascii_strdown ((gchar*)iter->data, strlen ((gchar*)iter->data));
			}

			glob_patterns = g_list_append (glob_patterns, g_pattern_spec_new (tmp_pattern));

			if (!profile->private->match_case)
			{
				g_free (tmp_pattern);
			}
		}
	}

	if (profile->private->mimetypes && profile->private->mimetypes->next != NULL &&
			(g_ascii_strcasecmp ((gchar*)(profile->private->mimetypes->data), "*") == 0 ||
			 g_ascii_strcasecmp ((gchar*)(profile->private->mimetypes->data), "*/*") == 0))
	{
		/* if the only pattern is '*' or * / * then all mimetypes will
		 * match, so it is not necessary to make the test for each of them
		 */
		test_mimetype = TRUE;
	}
	else
	{
		for (iter = profile->private->mimetypes; iter; iter = iter->next)
		{
			glob_mime_patterns = g_list_append (glob_mime_patterns, g_pattern_spec_new ((gchar*)iter->data));
		}
	}

	for (iter1 = files; iter1; iter1 = iter1->next)
	{
		gchar* tmp_filename = nautilus_file_info_get_name ((NautilusFileInfo *)iter1->data);

		if (tmp_filename)
		{
			gchar* tmp_mimetype = nautilus_file_info_get_mime_type ((NautilusFileInfo *)iter1->data);

			if (!profile->private->match_case)
			{
				/* --> if case-insensitive asked, lower all the string
				 * since the pattern matching function don't manage it
				 * itself.
				 */
				gchar* tmp_filename2 = g_ascii_strdown (tmp_filename, strlen (tmp_filename));
				g_free (tmp_filename);
				tmp_filename = tmp_filename2;
			}

			/* --> for the moment we deal with all mimetypes case-insensitively */
			gchar* tmp_mimetype2 = g_ascii_strdown (tmp_mimetype, strlen (tmp_mimetype));
			g_free (tmp_mimetype);
			tmp_mimetype = tmp_mimetype2;

			if (nautilus_file_info_is_directory ((NautilusFileInfo *)iter1->data))
			{
				dir_count++;
			}
			else
			{
				file_count++;
			}

			scheme_ok_count += validate_schemes (profile->private->schemes, (NautilusFileInfo*)iter1->data);

			if (!test_basename) /* if it is already ok, skip the test to improve performance */
			{
				basename_match_ok = FALSE;
				iter2 = glob_patterns;
				while (iter2 && !basename_match_ok)
				{
					if (g_pattern_match_string ((GPatternSpec*)iter2->data, tmp_filename))
					{
						basename_match_ok = TRUE;
					}
					iter2 = iter2->next;
				}

				if (basename_match_ok)
				{
					glob_ok_count++;
				}
			}

			if (!test_mimetype) /* if it is already ok, skip the test to improve performance */
			{
				mimetype_match_ok = FALSE;
				iter2 = glob_mime_patterns;
				while (iter2 && !mimetype_match_ok)
				{
					if (g_pattern_match_string ((GPatternSpec*)iter2->data, tmp_mimetype))
					{
						mimetype_match_ok = TRUE;
					}
					iter2 = iter2->next;
				}

				if (mimetype_match_ok)
				{
					mime_glob_ok_count++;
				}
			}

			g_free (tmp_mimetype);
			g_free (tmp_filename);

		}

		total_count++;
	}

	if ((files != NULL) && (files->next == NULL) && (!profile->private->accept_multiple_files))
	{
		test_multiple_file = TRUE;
	}
	else if (profile->private->accept_multiple_files)
	{
		test_multiple_file = TRUE;
	}

	if (profile->private->is_dir && profile->private->is_file)
	{
		if (dir_count > 0 || file_count > 0)
		{
			test_file_type = TRUE;
		}
	}
	else if (profile->private->is_dir && !profile->private->is_file)
	{
		if (file_count == 0)
		{
			test_file_type = TRUE;
		}
	}
	else if (!profile->private->is_dir && profile->private->is_file)
	{
		if (dir_count == 0)
		{
			test_file_type = TRUE;
		}
	}

	if (scheme_ok_count == total_count)
	{
		test_scheme = TRUE;
	}


	if (!test_basename) /* if not already tested */
	{
		if (glob_ok_count == total_count)
		{
			test_basename = TRUE;
		}
	}

	if (!test_mimetype) /* if not already tested */
	{
		if (mime_glob_ok_count == total_count)
		{
			test_mimetype = TRUE;
		}
	}

	if (test_basename && test_mimetype && test_file_type && test_scheme && test_multiple_file)
	{
		retv = TRUE;
	}

	g_list_foreach (glob_patterns, (GFunc) g_pattern_spec_free, NULL);
	g_list_free (glob_patterns);
	g_list_foreach (glob_mime_patterns, (GFunc) g_pattern_spec_free, NULL);
	g_list_free (glob_mime_patterns);

	return retv;
}

/**
 * Expands the parameters path, in function of the found tokens.
 *
 * @profile: the selected profile.
 *
 * @files: the list of currently selected items, as provided by Nautilus.
 *
 * Valid parameters are :
 *
 * %u : gnome-vfs URI
 * %d : base dir of the selected file(s)/folder(s)
 * %f : the name of the selected file/folder or the 1st one if many are selected
 * %m : list of the basename of the selected files/directories separated by space.
 * %M : list of the selected files/directories with their complete path separated by space.
 * %s : scheme of the gnome-vfs URI
 * %h : hostname of the gnome-vfs URI
 * %U : username of the gnome-vfs URI
 * %% : a percent sign
 */
gchar *
nact_action_profile_parse_parameters( const NactActionProfile *profile, GList* files )
{
	gchar* retv = NULL;
	g_return_val_if_fail( NACT_IS_ACTION_PROFILE( profile ), NULL );

	if (files != NULL){
		const gchar *param_template = profile->private->parameters;
		GString* tmp_string = g_string_new ("");
		gchar* iter = g_strdup (param_template);
		gchar* old_iter = iter;
		/*int current_len = strlen (iter);*/
		gchar* uri = nautilus_file_info_get_uri ((NautilusFileInfo*)files->data);
		GnomeVFSURI* gvfs_uri = gnome_vfs_uri_new (uri);
		gchar* filename;
		gchar* dirname;
		gchar* scheme = nautilus_file_info_get_uri_scheme ((NautilusFileInfo*)files->data);
		gchar* hostname = g_strdup (gnome_vfs_uri_get_host_name (gvfs_uri));
		gchar* username = g_strdup (gnome_vfs_uri_get_user_name (gvfs_uri));
		gchar* file_list;
		gchar* path_file_list;
		GList* file_iter = NULL;
		GString* tmp_file_list;
		GString* tmp_path_file_list;
		gchar* tmp;
		gchar* tmp2;

		tmp = gnome_vfs_uri_extract_dirname (gvfs_uri);
		dirname = (gchar*)gnome_vfs_unescape_string ((const gchar*)tmp, "");
		g_free (tmp);

		tmp = nautilus_file_info_get_name ((NautilusFileInfo*)files->data);
		if (!tmp)
		{
			tmp = g_strdup ("");
		}

		filename = g_shell_quote (tmp);
		tmp2 = g_build_path ("/", dirname, tmp, NULL);
		g_free (tmp);
		tmp_file_list = g_string_new (filename);
		tmp = g_shell_quote (tmp2);
		tmp_path_file_list = g_string_new (tmp);
		g_free (tmp2);
		g_free (tmp);

		/* We already have the first item, so we start with the next one if any */
		for (file_iter = files->next; file_iter; file_iter = file_iter->next)
		{
			gchar* tmp_filename = nautilus_file_info_get_name ((NautilusFileInfo*)file_iter->data);
			gchar* tmp_uri = nautilus_file_info_get_uri ((NautilusFileInfo*)file_iter->data);
			GnomeVFSURI* tmp_gvfs_uri = gnome_vfs_uri_new (tmp_uri);
			tmp = gnome_vfs_uri_extract_dirname (tmp_gvfs_uri);
			gchar* tmp_dirname = (gchar*)gnome_vfs_unescape_string ((const gchar*)tmp, "");
			g_free (tmp);

			if (!tmp_filename)
			{
				tmp_filename = g_strdup ("");
			}

			gchar* quoted_tmp_filename = g_shell_quote (tmp_filename);
			g_string_append_printf (tmp_file_list, " %s", quoted_tmp_filename);

			tmp = g_build_path ("/", tmp_dirname, tmp_filename, NULL);
			tmp2 = g_shell_quote (tmp);
			g_string_append_printf (tmp_path_file_list, " %s", tmp2);

			g_free (tmp2);
			g_free (tmp);
			g_free (tmp_filename);
			g_free (quoted_tmp_filename);
			g_free (tmp_dirname);
			g_free (tmp_uri);
			gnome_vfs_uri_unref (tmp_gvfs_uri);
		}
		file_list = g_string_free (tmp_file_list, FALSE);
		path_file_list = g_string_free (tmp_path_file_list, FALSE);


		while ((iter = g_strstr_len (iter, strlen (iter), "%")))
		{
			tmp_string = g_string_append_len (tmp_string, old_iter, strlen (old_iter) - strlen (iter));
			switch (iter[1])
			{
				case 'u': /* gnome-vfs URI */
					tmp_string = g_string_append (tmp_string, uri);
					break;
				case 'd': /* base dir of the selected file(s)/folder(s) */
					tmp = g_shell_quote (dirname);
					tmp_string = g_string_append (tmp_string, tmp);
					g_free (tmp);
					break;
				case 'f': /* the basename of the selected file/folder or the 1st one if many are selected */
					tmp_string = g_string_append (tmp_string, filename);
					break;
				case 'm': /* list of the basename of the selected files/directories separated by space */
					tmp_string = g_string_append (tmp_string, file_list);
					break;
	 			case 'M': /* list of the selected files/directories with their complete path separated by space. */
					tmp_string = g_string_append (tmp_string, path_file_list);
					break;
				case 's': /* scheme of the gnome-vfs URI */
					tmp_string = g_string_append (tmp_string, scheme);
					break;
				case 'h': /* hostname of the gnome-vfs URI */
					tmp_string = g_string_append (tmp_string, hostname);
					break;
				case 'U': /* username of the gnome-vfs URI */
					tmp_string = g_string_append (tmp_string, username);
					break;
				case '%': /* a percent sign */
					tmp_string = g_string_append_c (tmp_string, '%');
					break;
			}
			iter+=2; /* skip the % sign and the character after. */
			old_iter = iter; /* store the new start of the string */
		}
		tmp_string = g_string_append_len (tmp_string, old_iter, strlen (old_iter));

		g_free (uri);
		g_free (dirname);
		g_free (filename);
		g_free (file_list);
		g_free (path_file_list);
		g_free (scheme);
		g_free (hostname);
		g_free (username);
		g_free (iter);
		gnome_vfs_uri_unref (gvfs_uri);

		retv = g_string_free (tmp_string, FALSE); /* return the content of the GString */
	}

	return retv;
}
