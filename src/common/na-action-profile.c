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
#include <glib/gi18n.h>

#include <libnautilus-extension/nautilus-file-info.h>

#include "na-action.h"
#include "na-action-profile.h"
#include "na-utils.h"

/* private class data
 */
struct NAActionProfileClassPrivate {
};

/* private instance data
 */
struct NAActionProfilePrivate {
	gboolean  dispose_has_run;

	/* the NAAction object
	 */
	NAAction *action;

	/* profile properties
	 */
	gchar    *path;
	gchar    *parameters;
	GSList   *basenames;
	gboolean  match_case;
	GSList   *mimetypes;
	gboolean  is_file;
	gboolean  is_dir;
	gboolean  accept_multiple;
	GSList   *schemes;
};

/* profile properties
 */
enum {
	PROP_NAPROFILE_ACTION = 1,
	PROP_NAPROFILE_NAME,
	PROP_NAPROFILE_LABEL,
	PROP_NAPROFILE_PATH,
	PROP_NAPROFILE_PARAMETERS,
	PROP_NAPROFILE_BASENAMES,
	PROP_NAPROFILE_MATCHCASE,
	PROP_NAPROFILE_MIMETYPES,
	PROP_NAPROFILE_ISFILE,
	PROP_NAPROFILE_ISDIR,
	PROP_NAPROFILE_ACCEPT_MULTIPLE,
	PROP_NAPROFILE_SCHEMES
};

#define PROP_NAPROFILE_ACTION_STR				"na-profile-action"
#define PROP_NAPROFILE_NAME_STR					"na-profile-name"
#define PROP_NAPROFILE_LABEL_STR				"na-profile-desc-name"
#define PROP_NAPROFILE_PATH_STR					"na-profile-path"
#define PROP_NAPROFILE_PARAMETERS_STR			"na-profile-parameters"
#define PROP_NAPROFILE_BASENAMES_STR			"na-profile-basenames"
#define PROP_NAPROFILE_MATCHCASE_STR			"na-profile-matchcase"
#define PROP_NAPROFILE_MIMETYPES_STR			"na-profile-mimetypes"
#define PROP_NAPROFILE_ISFILE_STR				"na-profile-isfile"
#define PROP_NAPROFILE_ISDIR_STR				"na-profile-isdir"
#define PROP_NAPROFILE_ACCEPT_MULTIPLE_STR		"na-profile-accept-multiple"
#define PROP_NAPROFILE_SCHEMES_STR				"na-profile-schemes"

static NAObjectClass *st_parent_class = NULL;

static GType     register_type( void );
static void      class_init( NAActionProfileClass *klass );
static void      instance_init( GTypeInstance *instance, gpointer klass );
static void      instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void      instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void      instance_dispose( GObject *object );
static void      instance_finalize( GObject *object );

static void      object_dump( const NAObject *action );
static void      object_dump_list( const gchar *thisfn, const gchar *label, GSList *list );
static NAObject *object_duplicate( const NAObject *action );
static void      object_copy( NAObject *target, const NAObject *source );
static gboolean  object_are_equal( const NAObject *a, const NAObject *b );
static gboolean  object_is_valid( const NAObject *action );

static int       validate_schemes( GSList* schemes2test, NautilusFileInfo* file );

GType
na_action_profile_get_type( void )
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
	static const gchar *thisfn = "na_action_profile_register_type";
	g_debug( "%s", thisfn );

	static GTypeInfo info = {
		sizeof( NAActionProfileClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NAActionProfile ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	return( g_type_register_static( NA_OBJECT_TYPE, "NAActionProfile", &info, 0 ));
}

static void
class_init( NAActionProfileClass *klass )
{
	static const gchar *thisfn = "na_action_profile_class_init";
	g_debug( "%s: klass=%p", thisfn, klass );

	st_parent_class = g_type_class_peek_parent( klass );

	GObjectClass *object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
	object_class->set_property = instance_set_property;
	object_class->get_property = instance_get_property;

	GParamSpec *spec;
	spec = g_param_spec_pointer(
			PROP_NAPROFILE_ACTION_STR,
			"NAAction attachment",
			"The NAAction action to which this profile belongs",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_NAPROFILE_ACTION, spec );

	spec = g_param_spec_string(
			PROP_NAPROFILE_NAME_STR,
			"Profile name",
			"Internal profile identifiant (ASCII, case insensitive)", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_NAPROFILE_NAME, spec );

	spec = g_param_spec_string(
			PROP_NAPROFILE_LABEL_STR,
			"Profile label",
			"Displayable profile's label (UTF-8, localizable)", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_NAPROFILE_LABEL, spec );

	spec = g_param_spec_string(
			PROP_NAPROFILE_PATH_STR,
			"Command path",
			"Command path", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_NAPROFILE_PATH, spec );

	spec = g_param_spec_string(
			PROP_NAPROFILE_PARAMETERS_STR,
			"Command parameters",
			"Command parameters", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_NAPROFILE_PARAMETERS, spec );

	spec = g_param_spec_pointer(
			PROP_NAPROFILE_BASENAMES_STR,
			"Filenames mask",
			"Filenames mask",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_NAPROFILE_BASENAMES, spec );

	spec = g_param_spec_boolean(
			PROP_NAPROFILE_MATCHCASE_STR,
			"Match case",
			"Whether the filenames are case sensitive", TRUE,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_NAPROFILE_MATCHCASE, spec );

	spec = g_param_spec_pointer(
			PROP_NAPROFILE_MIMETYPES_STR,
			"Mimetypes",
			"List of selectable mimetypes",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_NAPROFILE_MIMETYPES, spec );

	spec = g_param_spec_boolean(
			PROP_NAPROFILE_ISFILE_STR,
			"Only files",
			"Whether apply when only files are selected", TRUE,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_NAPROFILE_ISFILE, spec );

	spec = g_param_spec_boolean(
			PROP_NAPROFILE_ISDIR_STR,
			"Only dirs",
			"Whether apply when only dirs are selected", FALSE,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_NAPROFILE_ISDIR, spec );

	spec = g_param_spec_boolean(
			PROP_NAPROFILE_ACCEPT_MULTIPLE_STR,
			"Accept multiple selection",
			"Whether apply when multiple files or folders are selected", TRUE,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_NAPROFILE_ACCEPT_MULTIPLE, spec );

	spec = g_param_spec_pointer(
			PROP_NAPROFILE_SCHEMES_STR,
			"Schemes",
			"list of selectable schemes",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_NAPROFILE_SCHEMES, spec );

	klass->private = g_new0( NAActionProfileClassPrivate, 1 );

	NA_OBJECT_CLASS( klass )->dump = object_dump;
	NA_OBJECT_CLASS( klass )->duplicate = object_duplicate;
	NA_OBJECT_CLASS( klass )->copy = object_copy;
	NA_OBJECT_CLASS( klass )->are_equal = object_are_equal;
	NA_OBJECT_CLASS( klass )->is_valid = object_is_valid;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	/*static const gchar *thisfn = "na_action_profile_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );*/

	g_assert( NA_IS_ACTION_PROFILE( instance ));
	NAActionProfile* self = NA_ACTION_PROFILE( instance );

	self->private = g_new0( NAActionProfilePrivate, 1 );

	self->private->dispose_has_run = FALSE;

	/* initialize suitable default values
	 */
	self->private->path = g_strdup( "" );
	self->private->parameters = g_strdup( "" );
	self->private->basenames = NULL;
	self->private->basenames = g_slist_append( self->private->basenames, g_strdup( "*" ));
	self->private->match_case = TRUE;
	self->private->mimetypes = NULL;
	self->private->mimetypes = g_slist_append( self->private->mimetypes, g_strdup( "*/*" ));
	self->private->is_file = TRUE;
	self->private->is_dir = FALSE;
	self->private->accept_multiple = FALSE;
	self->private->schemes = NULL;
	self->private->schemes = g_slist_append( self->private->schemes, g_strdup( "file" ));
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	g_assert( NA_IS_ACTION_PROFILE( object ));
	NAActionProfile *self = NA_ACTION_PROFILE( object );

	GSList *list;

	switch( property_id ){
		case PROP_NAPROFILE_ACTION:
			g_value_set_pointer( value, self->private->action );
			break;

		case PROP_NAPROFILE_NAME:
			G_OBJECT_CLASS( st_parent_class )->get_property( object, PROP_NAOBJECT_ID, value, spec );
			break;

		case PROP_NAPROFILE_LABEL:
			G_OBJECT_CLASS( st_parent_class )->get_property( object, PROP_NAOBJECT_LABEL, value, spec );
			break;

		case PROP_NAPROFILE_PATH:
			g_value_set_string( value, self->private->path );
			break;

		case PROP_NAPROFILE_PARAMETERS:
			g_value_set_string( value, self->private->parameters );
			break;

		case PROP_NAPROFILE_BASENAMES:
			list = na_utils_duplicate_string_list( self->private->basenames );
			g_value_set_pointer( value, list );
			break;

		case PROP_NAPROFILE_MATCHCASE:
			g_value_set_boolean( value, self->private->match_case );
			break;

		case PROP_NAPROFILE_MIMETYPES:
			list = na_utils_duplicate_string_list( self->private->mimetypes );
			g_value_set_pointer( value, list );
			break;

		case PROP_NAPROFILE_ISFILE:
			g_value_set_boolean( value, self->private->is_file );
			break;

		case PROP_NAPROFILE_ISDIR:
			g_value_set_boolean( value, self->private->is_dir );
			break;

		case PROP_NAPROFILE_ACCEPT_MULTIPLE:
			g_value_set_boolean( value, self->private->accept_multiple );
			break;

		case PROP_NAPROFILE_SCHEMES:
			list = na_utils_duplicate_string_list( self->private->schemes );
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
	g_assert( NA_IS_ACTION_PROFILE( object ));
	NAActionProfile *self = NA_ACTION_PROFILE( object );

	switch( property_id ){
		case PROP_NAPROFILE_ACTION:
			self->private->action = g_value_get_pointer( value );
			break;

		case PROP_NAPROFILE_NAME:
			G_OBJECT_CLASS( st_parent_class )->set_property( object, PROP_NAOBJECT_ID, value, spec );
			break;

		case PROP_NAPROFILE_LABEL:
			G_OBJECT_CLASS( st_parent_class )->set_property( object, PROP_NAOBJECT_LABEL, value, spec );
			break;

		case PROP_NAPROFILE_PATH:
			g_free( self->private->path );
			self->private->path = g_value_dup_string( value );
			break;

		case PROP_NAPROFILE_PARAMETERS:
			g_free( self->private->parameters );
			self->private->parameters = g_value_dup_string( value );
			break;

		case PROP_NAPROFILE_BASENAMES:
			na_utils_free_string_list( self->private->basenames );
			self->private->basenames = na_utils_duplicate_string_list( g_value_get_pointer( value ));
			break;

		case PROP_NAPROFILE_MATCHCASE:
			self->private->match_case = g_value_get_boolean( value );
			break;

		case PROP_NAPROFILE_MIMETYPES:
			na_utils_free_string_list( self->private->mimetypes );
			self->private->mimetypes = na_utils_duplicate_string_list( g_value_get_pointer( value ));
			break;

		case PROP_NAPROFILE_ISFILE:
			self->private->is_file = g_value_get_boolean( value );
			break;

		case PROP_NAPROFILE_ISDIR:
			self->private->is_dir = g_value_get_boolean( value );
			break;

		case PROP_NAPROFILE_ACCEPT_MULTIPLE:
			self->private->accept_multiple = g_value_get_boolean( value );
			break;

		case PROP_NAPROFILE_SCHEMES:
			na_utils_free_string_list( self->private->schemes );
			self->private->schemes = na_utils_duplicate_string_list( g_value_get_pointer( value ));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "na_action_profile_instance_dispose";
	g_debug( "%s: object=%p", thisfn, object );

	g_assert( NA_IS_ACTION_PROFILE( object ));
	NAActionProfile *self = NA_ACTION_PROFILE( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		G_OBJECT_CLASS( st_parent_class )->dispose( object );
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "na_action_profile_instance_finalize";
	g_debug( "%s: object=%p", thisfn, object );

	g_assert( NA_IS_ACTION_PROFILE( object ));
	NAActionProfile *self = ( NAActionProfile * ) object;

	g_free( self->private->path );
	g_free( self->private->parameters );
	na_utils_free_string_list( self->private->basenames );
	na_utils_free_string_list( self->private->mimetypes );
	na_utils_free_string_list( self->private->schemes );

	g_free( self->private );

	/* chain call to parent class */
	if((( GObjectClass * ) st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * na_action_profile_new:
 *
 * Allocates a new profile of the given name.
 *
 * Returns: the newly allocated #NAActionProfile profile.
 */
NAActionProfile *
na_action_profile_new( void )
{
	NAActionProfile *profile = g_object_new( NA_ACTION_PROFILE_TYPE, NULL );

	na_action_profile_set_name( profile, ACTION_PROFILE_PREFIX "zero" );

	/* i18n: default label for a new profile */
	na_action_profile_set_label( profile, _( "Default profile" ));

	return( profile );
}

/**
 * na_action_profile_get_action:
 * @profile: the #NAActionProfile to be requested.
 *
 * Returns a pointer to the action to which this profile is attached,
 * or NULL if the profile has never been attached.
 *
 * Returns: a #NAAction pointer.
 *
 * Note that the returned #NAAction pointer is owned by the profile.
 * The caller should not try to g_free() nor g_object_unref() it.
 */
NAAction *
na_action_profile_get_action( const NAActionProfile *profile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	NAAction *action;
	g_object_get( G_OBJECT( profile ), PROP_NAPROFILE_ACTION_STR, &action, NULL );

	return( action );
}

/**
 * na_action_profile_get_name:
 * @profile: the #NAActionProfile to be requested.
 *
 * Returns the internal name (identifier) of the profile.
 *
 * Returns: the name of the profile as a newly allocated string.
 * The returned string must be g_free() by the caller.
 *
 * See na_action_profile_set_name() for some rationales about name.
 */
gchar *
na_action_profile_get_name( const NAActionProfile *profile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	gchar *id;
	g_object_get( G_OBJECT( profile ), PROP_NAPROFILE_NAME_STR, &id, NULL );
	return( id );
}

/**
 * na_action_profile_get_label:
 * @profile: the #NAActionProfile to be requested.
 *
 * Returns the descriptive name (label) of the profile.
 *
 * Returns: the label of the profile as a newly allocated string.
 * The returned string must be g_free() by the caller.
 *
 * See na_action_profile_set_label() for some rationale about label.
 */
gchar *
na_action_profile_get_label( const NAActionProfile *profile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	gchar *label;
	g_object_get( G_OBJECT( profile ), PROP_NAPROFILE_LABEL_STR, &label, NULL );
	return( label );
}

/**
 * na_action_profile_get_path:
 * @profile: the #NAActionProfile to be requested.
 *
 * Returns the path of the command attached to the profile.
 *
 * Returns: the command path as a newly allocated string. The returned
 * string must be g_free() by the caller.
 */
gchar *
na_action_profile_get_path( const NAActionProfile *profile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	gchar *path;
	g_object_get( G_OBJECT( profile ), PROP_NAPROFILE_PATH_STR, &path, NULL );

	return( path );
}

/**
 * na_action_profile_get_parameters:
 * @profile: the #NAActionProfile to be requested.
 *
 * Returns the parameters of the command attached to the profile.
 *
 * Returns: the command parameters as a newly allocated string. The
 * returned string must be g_free() by the caller.
 */
gchar *
na_action_profile_get_parameters( const NAActionProfile *profile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	gchar *parameters;
	g_object_get( G_OBJECT( profile ), PROP_NAPROFILE_PARAMETERS_STR, &parameters, NULL );

	return( parameters );
}

/**
 * na_action_profile_get_basenames:
 * @profile: the #NAActionProfile to be requested.
 *
 * Returns the basenames of the files to which the profile applies.
 *
 * Returns: a GSList of newly allocated strings. The list must be
 * na_utils_free_string_list() by the caller.
 *
 * See na_action_profile_set_basenames() for some rationale about
 * basenames.
 */
GSList *
na_action_profile_get_basenames( const NAActionProfile *profile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	GSList *basenames;
	g_object_get( G_OBJECT( profile ), PROP_NAPROFILE_BASENAMES_STR, &basenames, NULL );

	return( basenames );
}

/**
 * na_action_profile_get_matchcase:
 * @profile: the #NAActionProfile to be requested.
 *
 * Are specified basenames case sensitive ?
 *
 * Returns: %TRUE if the provided filenames are case sensitive, %FALSE
 * else.
 *
 * See na_action_profile_set_matchcase() for some rationale about case
 * sensitivity.
 */
gboolean
na_action_profile_get_matchcase( const NAActionProfile *profile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	gboolean matchcase;
	g_object_get( G_OBJECT( profile ), PROP_NAPROFILE_MATCHCASE_STR, &matchcase, NULL );

	return( matchcase );
}

/**
 * na_action_profile_get_mimetypes:
 * @profile: the #NAActionProfile to be requested.
 *
 * Returns the list of mimetypes this profile applies to.
 *
 * Returns: a GSList of newly allocated strings. The list must be
 * na_utils_free_string_list() by the caller.
 *
 * See na_action_profile_set_mimetypes() for some rationale about
 * mimetypes.
 */
GSList *
na_action_profile_get_mimetypes( const NAActionProfile *profile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	GSList *mimetypes;
	g_object_get( G_OBJECT( profile ), PROP_NAPROFILE_MIMETYPES_STR, &mimetypes, NULL );

	return( mimetypes );
}

/**
 * na_action_profile_get_is_file:
 * @profile: the #NAActionProfile to be requested.
 *
 * Does this profile apply if the selection contains files ?
 *
 * Returns: %TRUE if it applies, %FALSE else.
 *
 * See na_action_profile_set_isfiledir() for some rationale about file
 * selection.
 */
gboolean
na_action_profile_get_is_file( const NAActionProfile *profile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	gboolean isfile;
	g_object_get( G_OBJECT( profile ), PROP_NAPROFILE_ISFILE_STR, &isfile, NULL );

	return( isfile );
}

/**
 * na_action_profile_get_is_dir:
 * @profile: the #NAActionProfile to be requested.
 *
 * Does this profile apply if the selection contains folders ?
 *
 * Returns: %TRUE if it applies, %FALSE else.
 *
 * See na_action_profile_set_isfiledir() for some rationale about file
 * selection.
 */
gboolean
na_action_profile_get_is_dir( const NAActionProfile *profile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	gboolean isdir;
	g_object_get( G_OBJECT( profile ), PROP_NAPROFILE_ISDIR_STR, &isdir, NULL );

	return( isdir );
}

/**
 * na_action_profile_get_multiple:
 * @profile: the #NAActionProfile to be requested.
 *
 * Does this profile apply if selection contains multiple files or
 * folders ?
 *
 * Returns: %TRUE if it applies, %FALSE else.
 *
 * See na_action_profile_set_multiple() for some rationale about
 * multiple selection.
 */
gboolean
na_action_profile_get_multiple( const NAActionProfile *profile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	gboolean multiple;
	g_object_get( G_OBJECT( profile ), PROP_NAPROFILE_ACCEPT_MULTIPLE_STR, &multiple, NULL );

	return( multiple );
}

/**
 * na_action_profile_get_schemes:
 * @profile: the #NAActionProfile to be requested.
 *
 * Returns the list of schemes this profile applies to.
 *
 * Returns: a GSList of newly allocated strings. The list must be
 * na_utils_free_string_list() by the caller.
 *
 * See na_action_profile_set_schemes() for some rationale about
 * schemes.
 */
GSList *
na_action_profile_get_schemes( const NAActionProfile *profile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	GSList *schemes;
	g_object_get( G_OBJECT( profile ), PROP_NAPROFILE_SCHEMES_STR, &schemes, NULL );

	return( schemes );
}

/**
 * na_action_profile_set_action:
 * @profile: the #NAActionProfile to be updated.
 * @action: the #NAAction action to which this profile is attached.
 *
 * Sets the action to which this profile is attached.
 */
void
na_action_profile_set_action( NAActionProfile *profile, const NAAction *action )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	g_object_set( G_OBJECT( profile ), PROP_NAPROFILE_ACTION_STR, action, NULL );
}

/**
 * na_action_profile_set_name:
 * @profile: the #NAActionProfile to be updated.
 * @name: the name to be set.
 *
 * Sets the name for this profile.
 *
 * #NAActionProfile takes a copy of the provided name. This later may
 * so be g_free() by the caller after this function returns.
 *
 * The profile name is an ASCII, case insensitive, string which
 * uniquely identifies the profile inside of the action.
 *
 * This function doesn't check for the unicity of the name. And this
 * unicity will never be checked until we try to write the action to
 * GConf.
 */
void
na_action_profile_set_name( NAActionProfile *profile, const gchar *name )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	g_object_set( G_OBJECT( profile ), PROP_NAPROFILE_NAME_STR, name, NULL );
}

/**
 * na_action_profile_set_label:
 * @profile: the #NAActionProfile to be updated.
 * @label: the label to be set.
 *
 * Sets the label for this profile.
 *
 * #NAActionProfile takes a copy of the provided label. This later may
 * so be g_free() by the caller after this function returns.
 *
 * The label of the #NAActionProfile is an UTF-8 string.
 */
void
na_action_profile_set_label( NAActionProfile *profile, const gchar *label )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	g_object_set( G_OBJECT( profile ), PROP_NAPROFILE_LABEL_STR, label, NULL );
}

/**
 * na_action_profile_set_path:
 * @profile: the #NAActionProfile to be updated.
 * @path: the command path to be set.
 *
 * Sets the path of the command for this profile.
 *
 * #NAActionProfile takes a copy of the provided path. This later may
 * so be g_free() by the caller after this function returns.
 */
void
na_action_profile_set_path( NAActionProfile *profile, const gchar *path )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	g_object_set( G_OBJECT( profile ), PROP_NAPROFILE_PATH_STR, path, NULL );
}

/**
 * na_action_profile_set_parameters:
 * @profile: the #NAActionProfile to be updated.
 * @parameters : the command parameters to be set.
 *
 * Sets the parameters of the command for this profile.
 *
 * #NAActionProfile takes a copy of the provided parameters. This later
 * may so be g_free() by the caller after this function returns.
 */
void
na_action_profile_set_parameters( NAActionProfile *profile, const gchar *parameters )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	g_object_set( G_OBJECT( profile ), PROP_NAPROFILE_PARAMETERS_STR, parameters, NULL );
}

/**
 * na_action_profile_set_basenames:
 * @profile: the #NAActionProfile to be updated.
 * @basenames : the basenames to be set.
 *
 * Sets the basenames of the elements on which this profile applies.
 *
 * #NAActionProfile takes a copy of the provided basenames. This later
 * may so be na_utils_free_string_list() by the caller after this
 * function returns.
 *
 * The basenames list defaults to the single element "*", which means
 * that the profile will apply to all basenames.
 */
void
na_action_profile_set_basenames( NAActionProfile *profile, GSList *basenames )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	g_object_set( G_OBJECT( profile ), PROP_NAPROFILE_BASENAMES_STR, basenames, NULL );
}

/**
 * na_action_profile_set_matchcase:
 * @profile: the #NAActionProfile to be updated.
 * @matchcase : whether the basenames are case sensitive or not.
 *
 * Sets the 'match_case' flag, indicating if specified basename
 * patterns are, or not, case sensitive.
 *
 * This value defaults to %TRUE, which means that basename patterns
 * default to be case sensitive.
 */
void
na_action_profile_set_matchcase( NAActionProfile *profile, gboolean matchcase )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	g_object_set( G_OBJECT( profile ), PROP_NAPROFILE_MATCHCASE_STR, matchcase, NULL );
}

/**
 * na_action_profile_set_mimetypes:
 * @profile: the #NAActionProfile to be updated.
 * @mimetypes: list of mimetypes to be matched.
 *
 * Sets the mimetypes on which this profile applies.
 *
 * #NAActionProfile takes a copy of the provided mimetypes. This later
 * may so be na_utils_free_string_list() by the caller after this
 * function returns.
 *
 * The mimetypes list defaults to the single element "* / *", which
 * means that the profile will apply to all types of files.
 */
void
na_action_profile_set_mimetypes( NAActionProfile *profile, GSList *mimetypes )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	g_object_set( G_OBJECT( profile ), PROP_NAPROFILE_MIMETYPES_STR, mimetypes, NULL );
}

/**
 * na_action_profile_set_isfile:
 * @profile: the #NAActionProfile to be updated.
 * @isfile: whether the profile applies only to files.
 *
 * Sets the 'isfile' flag on which this profile applies.
 */
void
na_action_profile_set_isfile( NAActionProfile *profile, gboolean isfile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	g_object_set( G_OBJECT( profile ), PROP_NAPROFILE_ISFILE_STR, isfile, NULL );
}

/**
 * na_action_profile_set_isdir:
 * @profile: the #NAActionProfile to be updated.
 * @isdir: the profile applies only to folders.
 *
 * Sets the 'isdir' flag on which this profile applies.
 */
void
na_action_profile_set_isdir( NAActionProfile *profile, gboolean isdir )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	g_object_set( G_OBJECT( profile ), PROP_NAPROFILE_ISDIR_STR, isdir, NULL );
}

/**
 * na_action_profile_set_isfiledir:
 * @profile: the #NAActionProfile to be updated.
 * @isfile: whether the profile applies only to files.
 * @isdir: the profile applies only to folders.
 *
 * Sets the 'isfile' and 'isdir' flags on which this profile applies.
 *
 * File selection defaults to %TRUE.
 *
 * Folder selection defaults to %FALSE, which means that this profile will
 * not apply if the selection contains folders.
 */
void
na_action_profile_set_isfiledir( NAActionProfile *profile, gboolean isfile, gboolean isdir )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	g_object_set( G_OBJECT( profile ), PROP_NAPROFILE_ISFILE_STR, isfile, PROP_NAPROFILE_ISDIR_STR, isdir, NULL );
}

/**
 * na_action_profile_set_multiple:
 * @profile: the #NAActionProfile to be updated.
 * @multiple: TRUE if it does.
 *
 * Sets if this profile accept multiple selection ?
 *
 * This value defaults to %FALSE, which means that this profile will
 * not apply if the selection contains more than one element.
 */
void
na_action_profile_set_multiple( NAActionProfile *profile, gboolean multiple )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	g_object_set( G_OBJECT( profile ), PROP_NAPROFILE_ACCEPT_MULTIPLE_STR, multiple, NULL );
}

/**
 * na_action_profile_set_scheme:
 * @profile: the #NAActionProfile to be updated.
 * @scheme: name of the scheme.
 * @selected: whether this scheme is candidate to this profile.
 *
 * Sets the status of a scheme relative to this profile.
 */
void
na_action_profile_set_scheme( NAActionProfile *profile, const gchar *scheme, gboolean selected )
{
	/*static const gchar *thisfn = "na_action_profile_set_scheme";*/

	g_assert( NA_IS_ACTION_PROFILE( profile ));

	gboolean exist = na_utils_find_in_list( profile->private->schemes, scheme );
	/*g_debug( "%s: scheme=%s exist=%s", thisfn, scheme, exist ? "True":"False" );*/

	if( selected && !exist ){
		profile->private->schemes = g_slist_prepend( profile->private->schemes, g_strdup( scheme ));
	}
	if( !selected && exist ){
		profile->private->schemes = na_utils_remove_ascii_from_string_list( profile->private->schemes, scheme );
	}
}

/**
 * na_action_profile_set_schemes:
 * @profile: the #NAActionProfile to be updated.
 * @schemes: list of schemes which apply.
 *
 * Sets the schemes on which this profile applies.
 *
 * #NAActionProfile takes a copy of the provided mimetypes. This later
 * may so be na_utils_free_string_list() by the caller after this
 * function returns.
 *
 * The schemes list defaults to the single element "file", which means
 * that the profile will only apply to local files.
 */
void
na_action_profile_set_schemes( NAActionProfile *profile, GSList *schemes )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	g_object_set( G_OBJECT( profile ), PROP_NAPROFILE_SCHEMES_STR, schemes, NULL );
}

/**
 * na_action_profile_is_candidate:
 * @profile: the #NAActionProfile to be checked.
 * @files: the currently selected items, as provided by Nautilus.
 *
 * Determines if the given profile is candidate to be displayed in the
 * Nautilus context menu, regarding the list of currently selected
 * items.
 *
 * Returns: %TRUE if this profile succeeds to all tests and is so a
 * valid candidate to be displayed in Nautilus context menu, %FALSE
 * else.
 */
gboolean
na_action_profile_is_candidate( const NAActionProfile *profile, GList* files )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

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

	if ((files != NULL) && (files->next == NULL) && (!profile->private->accept_multiple))
	{
		test_multiple_file = TRUE;
	}
	else if (profile->private->accept_multiple)
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
 * %d : base dir of the (first) selected file(s)/folder(s)
 * %f : the name of the (first) selected file/folder
 * %h : hostname of the (first) GVfs URI
 * %m : list of the basename of the selected files/directories separated by space.
 * %M : list of the selected files/directories with their complete path separated by space.
 * %s : scheme of the (first) GVfs URI
 * %u : (first) GVfs URI
 * %U : username of the (first) GVfs URI
 * %% : a percent sign
 */
gchar *
na_action_profile_parse_parameters( const NAActionProfile *profile, GList* files )
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
	GString *basename_list, *pathname_list;
	gchar *tmp;

	g_return_val_if_fail( NA_IS_ACTION_PROFILE( profile ), NULL );

	string = g_string_new( "" );
	basename_list = g_string_new( "" );
	pathname_list = g_string_new( "" );
	first = TRUE;

	for( ifi = files ; ifi ; ifi = ifi->next ){

		iuri = nautilus_file_info_get_uri(( NautilusFileInfo * ) ifi->data );
		iloc = nautilus_file_info_get_location(( NautilusFileInfo * ) ifi->data );
		ipath = g_file_get_path( iloc );
		ibname = g_file_get_basename( iloc );

		if( first ){

			uri = g_strdup( iuri );
			dirname = g_path_get_dirname( ipath );
			scheme = nautilus_file_info_get_uri_scheme(( NautilusFileInfo * ) ifi->data );
			filename = g_strdup( ibname );

			/*hostname = g_strdup( gnome_vfs_uri_get_host_name( gvfs_uri ));
			username = g_strdup( gnome_vfs_uri_get_user_name( gvfs_uri ));*/
			/* TODO get hostname or username from GFile uri */
			hostname = NULL;
			username = NULL;

			first = FALSE;
		}

		tmp = g_shell_quote( ibname );

		g_string_append_printf( basename_list, " %s", tmp );
		g_free( tmp );

		tmp = g_shell_quote( ipath );
		g_string_append_printf( pathname_list, " %s", tmp );
		g_free( tmp );

		g_free( ibname );
		g_free( ipath );
		g_object_unref( iloc );
		g_free( iuri );
	}

	gchar *iter = g_strdup( profile->private->parameters );
	gchar *old_iter = iter;

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

			/* hostname of the (first) GVfs URI
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

			/* scheme of the (first) GVfs URI
			 */
			case 's':
				string = g_string_append( string, scheme );
				break;

			/* GVfs URI of the first item
			 */
			case 'u':
				string = g_string_append( string, uri );
				break;

			/* username of the (first) GVfs URI
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
	g_string_free( basename_list, TRUE );
	g_string_free( pathname_list, TRUE );

	parsed = g_string_free( string, FALSE );
	return( parsed );
}

static void
object_dump( const NAObject *object )
{
	static const gchar *thisfn = "na_action_profile_object_dump";

	g_assert( NA_IS_ACTION_PROFILE( object ));
	NAActionProfile *self = NA_ACTION_PROFILE( object );

	if( st_parent_class->dump ){
		st_parent_class->dump( object );
	}

	g_debug( "%s:          action=%p", thisfn, self->private->action );
	g_debug( "%s:            path='%s'", thisfn, self->private->path );
	g_debug( "%s:      parameters='%s'", thisfn, self->private->parameters );
	g_debug( "%s: accept_multiple='%s'", thisfn, self->private->accept_multiple ? "True" : "False" );
	g_debug( "%s:          is_dir='%s'", thisfn, self->private->is_dir ? "True" : "False" );
	g_debug( "%s:         is_file='%s'", thisfn, self->private->is_file ? "True" : "False" );
	g_debug( "%s:      match_case='%s'", thisfn, self->private->match_case ? "True" : "False" );
	object_dump_list( thisfn, "basenames", self->private->basenames );
	object_dump_list( thisfn, "mimetypes", self->private->mimetypes );
	object_dump_list( thisfn, "  schemes", self->private->schemes );
}

static void
object_dump_list( const gchar *thisfn, const gchar *label, GSList *list )
{
	gchar *string = na_utils_gslist_to_schema( list );
	g_debug( "%s:       %s=%s", thisfn, label, string );
	g_free( string );
}

static NAObject *
object_duplicate( const NAObject *profile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	NAObject *duplicate = NA_OBJECT( na_action_profile_new());

	na_object_copy( duplicate, profile );

	return( duplicate );
}

static void
object_copy( NAObject *target, const NAObject *source )
{
	g_assert( NA_IS_ACTION_PROFILE( target ));
	g_assert( NA_IS_ACTION_PROFILE( source ));

	gchar *path, *parameters;
	gboolean matchcase, isfile, isdir, multiple;
	GSList *basenames, *mimetypes, *schemes;

	g_object_get( G_OBJECT( source ),
			PROP_NAPROFILE_PATH_STR, &path,
			PROP_NAPROFILE_PARAMETERS_STR, &parameters,
			PROP_NAPROFILE_BASENAMES_STR, &basenames,
			PROP_NAPROFILE_MATCHCASE_STR, &matchcase,
			PROP_NAPROFILE_MIMETYPES_STR, &mimetypes,
			PROP_NAPROFILE_ISFILE_STR, &isfile,
			PROP_NAPROFILE_ISDIR_STR, &isdir,
			PROP_NAPROFILE_ACCEPT_MULTIPLE_STR, &multiple,
			PROP_NAPROFILE_SCHEMES_STR, &schemes,
			NULL );

	g_object_set( G_OBJECT( target ),
			PROP_NAPROFILE_PATH_STR, path,
			PROP_NAPROFILE_PARAMETERS_STR, parameters,
			PROP_NAPROFILE_BASENAMES_STR, basenames,
			PROP_NAPROFILE_MATCHCASE_STR, matchcase,
			PROP_NAPROFILE_MIMETYPES_STR, mimetypes,
			PROP_NAPROFILE_ISFILE_STR, isfile,
			PROP_NAPROFILE_ISDIR_STR, isdir,
			PROP_NAPROFILE_ACCEPT_MULTIPLE_STR, multiple,
			PROP_NAPROFILE_SCHEMES_STR, schemes,
			NULL );

	g_free( path );
	g_free( parameters );
	na_utils_free_string_list( basenames );
	na_utils_free_string_list( mimetypes );
	na_utils_free_string_list( schemes );

	if( st_parent_class->copy ){
		st_parent_class->copy( target, source );
	}
}

gboolean
object_are_equal( const NAObject *a, const NAObject *b )
{
	g_assert( NA_IS_ACTION_PROFILE( a ));
	g_assert( NA_IS_ACTION_PROFILE( b ));

	NAActionProfile *first = NA_ACTION_PROFILE( a );
	NAActionProfile *second = NA_ACTION_PROFILE( b );

	gboolean equal =
		( g_utf8_collate( first->private->path, second->private->path ) == 0 ) &&
		( g_utf8_collate( first->private->parameters, second->private->parameters ) == 0 );

	if( equal ){
		equal = (( first->private->accept_multiple && second->private->accept_multiple ) ||
				( !first->private->accept_multiple && !second->private->accept_multiple ));
	}
	if( equal ){
		equal = (( first->private->is_dir && second->private->is_dir ) ||
				( !first->private->is_dir && !second->private->is_dir ));
	}
	if( equal ){
		equal = (( first->private->is_file && second->private->is_file ) ||
				( !first->private->is_file && !second->private->is_file ));
	}
	if( equal ){
		equal = na_utils_string_lists_are_equal( first->private->basenames, second->private->basenames ) &&
				na_utils_string_lists_are_equal( first->private->mimetypes, second->private->mimetypes ) &&
				na_utils_string_lists_are_equal( first->private->schemes, second->private->schemes );
	}
	if( equal ){
		if( st_parent_class->are_equal ){
			equal = st_parent_class->are_equal( a, b );
		}
	}

	return( equal );
}

gboolean
object_is_valid( const NAObject *profile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	gchar *label;
	g_object_get( G_OBJECT( profile ), PROP_NAPROFILE_LABEL_STR, &label, NULL );
	gboolean is_valid = ( label && g_utf8_strlen( label, -1 ) > 0 );
	g_free( label );

	if( is_valid ){
		if( st_parent_class->is_valid ){
			is_valid = st_parent_class->is_valid( profile );
		}
	}

	return( is_valid );
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
