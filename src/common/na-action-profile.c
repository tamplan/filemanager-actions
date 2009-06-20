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

/* instance properties
 * please note that property names must have the same spelling as the
 * NactIIOProvider parameters
 */
enum {
	PROP_PROFILE_ACTION = 1,
	PROP_PROFILE_NAME,
	PROP_PROFILE_LABEL,
	PROP_PROFILE_PATH,
	PROP_PROFILE_PARAMETERS,
	PROP_PROFILE_ACCEPT_MULTIPLE,
	PROP_PROFILE_BASENAMES,
	PROP_PROFILE_ISDIR,
	PROP_PROFILE_ISFILE,
	PROP_PROFILE_MATCHCASE,
	PROP_PROFILE_MIMETYPES,
	PROP_PROFILE_SCHEMES
};

static NAObjectClass *st_parent_class = NULL;

static GType  register_type( void );
static void   class_init( NAActionProfileClass *klass );
static void   instance_init( GTypeInstance *instance, gpointer klass );
static void   instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void   instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void   instance_dispose( GObject *object );
static void   instance_finalize( GObject *object );

static void   do_dump( const NAObject *profile );
static void   do_dump_list( const gchar *thisfn, const gchar *label, GSList *list );
static gchar *do_get_id( const NAObject *object );
static gchar *do_get_label( const NAObject *object );
static int    validate_schemes( GSList* schemes2test, NautilusFileInfo* file );

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
			PROP_PROFILE_ACTION_STR,
			PROP_PROFILE_ACTION_STR,
			"The NAAction object",
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_PROFILE_ACTION, spec );

	/* the id of the object is marked as G_PARAM_CONSTRUCT_ONLY */
	spec = g_param_spec_string(
			PROP_PROFILE_NAME_STR,
			PROP_PROFILE_NAME_STR,
			"Internal profile's name", "",
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_PROFILE_NAME, spec );

	spec = g_param_spec_string(
			PROP_PROFILE_LABEL_STR,
			PROP_PROFILE_LABEL_STR,
			"Displayable profile's name", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_PROFILE_LABEL, spec );

	spec = g_param_spec_string(
			PROP_PROFILE_PATH_STR,
			PROP_PROFILE_PATH_STR,
			"Command path", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_PROFILE_PATH, spec );

	spec = g_param_spec_string(
			PROP_PROFILE_PARAMETERS_STR,
			PROP_PROFILE_PARAMETERS_STR,
			"Command parameters", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_PROFILE_PARAMETERS, spec );

	spec = g_param_spec_boolean(
			PROP_PROFILE_ACCEPT_MULTIPLE_STR,
			PROP_PROFILE_ACCEPT_MULTIPLE_STR,
			"Whether apply when multiple files may be selected", TRUE,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_PROFILE_ACCEPT_MULTIPLE, spec );

	spec = g_param_spec_pointer(
			PROP_PROFILE_BASENAMES_STR,
			PROP_PROFILE_BASENAMES_STR,
			"Filenames mask",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_PROFILE_BASENAMES, spec );

	spec = g_param_spec_boolean(
			PROP_PROFILE_ISDIR_STR,
			PROP_PROFILE_ISDIR_STR,
			"Whether apply when a dir is selected", FALSE,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_PROFILE_ISDIR, spec );

	spec = g_param_spec_boolean(
			PROP_PROFILE_ISFILE_STR,
			PROP_PROFILE_ISFILE_STR,
			"Whether apply when a file is selected", TRUE,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_PROFILE_ISFILE, spec );

	spec = g_param_spec_boolean(
			PROP_PROFILE_MATCHCASE_STR,
			PROP_PROFILE_MATCHCASE_STR,
			"Whether the filenames are case sensitive", TRUE,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_PROFILE_MATCHCASE, spec );

	spec = g_param_spec_pointer(
			PROP_PROFILE_MIMETYPES_STR,
			PROP_PROFILE_MIMETYPES_STR,
			"List of selectable mimetypes",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_PROFILE_MIMETYPES, spec );

	spec = g_param_spec_pointer(
			PROP_PROFILE_SCHEMES_STR,
			PROP_PROFILE_SCHEMES_STR,
			"list of selectable schemes",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_PROFILE_SCHEMES, spec );

	klass->private = g_new0( NAActionProfileClassPrivate, 1 );

	NA_OBJECT_CLASS( klass )->dump = do_dump;
	NA_OBJECT_CLASS( klass )->get_id = do_get_id;
	NA_OBJECT_CLASS( klass )->get_label = do_get_label;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "na_action_profile_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );

	g_assert( NA_IS_ACTION_PROFILE( instance ));
	NAActionProfile* self = NA_ACTION_PROFILE( instance );

	self->private = g_new0( NAActionProfilePrivate, 1 );

	self->private->dispose_has_run = FALSE;

	/* initialize suitable default values
	 * i18n: default label for the default profile
	 */
	self->private->label = g_strdup( _( "Default profile" ));
	self->private->path = g_strdup( "" );
	self->private->parameters = g_strdup( "" );
	self->private->accept_multiple_files = FALSE;
	self->private->basenames = NULL;
	self->private->basenames = g_slist_append( self->private->basenames, g_strdup( "*" ));
	self->private->is_dir = FALSE;
	self->private->is_file = TRUE;
	self->private->match_case = TRUE;
	self->private->mimetypes = NULL;
	self->private->mimetypes = g_slist_append( self->private->mimetypes, g_strdup( "*/*" ));
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
		case PROP_PROFILE_ACTION:
			g_value_set_pointer( value, self->private->action );
			break;

		case PROP_PROFILE_NAME:
			g_value_set_string( value, self->private->name );
			break;

		case PROP_PROFILE_LABEL:
			g_value_set_string( value, self->private->label );
			break;

		case PROP_PROFILE_PATH:
			g_value_set_string( value, self->private->path );
			break;

		case PROP_PROFILE_PARAMETERS:
			g_value_set_string( value, self->private->parameters );
			break;

		case PROP_PROFILE_ACCEPT_MULTIPLE:
			g_value_set_boolean( value, self->private->accept_multiple_files );
			break;

		case PROP_PROFILE_BASENAMES:
			list = na_utils_duplicate_string_list( self->private->basenames );
			g_value_set_pointer( value, list );
			break;

		case PROP_PROFILE_ISDIR:
			g_value_set_boolean( value, self->private->is_dir );
			break;

		case PROP_PROFILE_ISFILE:
			g_value_set_boolean( value, self->private->is_file );
			break;

		case PROP_PROFILE_MATCHCASE:
			g_value_set_boolean( value, self->private->match_case );
			break;

		case PROP_PROFILE_MIMETYPES:
			list = na_utils_duplicate_string_list( self->private->mimetypes );
			g_value_set_pointer( value, list );
			break;

		case PROP_PROFILE_SCHEMES:
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
		case PROP_PROFILE_ACTION:
			self->private->action = g_value_get_pointer( value );
			break;

		case PROP_PROFILE_NAME:
			g_free( self->private->name );
			self->private->name = g_value_dup_string( value );
			break;

		case PROP_PROFILE_LABEL:
			g_free( self->private->label );
			self->private->label = g_value_dup_string( value );
			break;

		case PROP_PROFILE_PATH:
			g_free( self->private->path );
			self->private->path = g_value_dup_string( value );
			break;

		case PROP_PROFILE_PARAMETERS:
			g_free( self->private->parameters );
			self->private->parameters = g_value_dup_string( value );
			break;

		case PROP_PROFILE_ACCEPT_MULTIPLE:
			self->private->accept_multiple_files = g_value_get_boolean( value );
			break;

		case PROP_PROFILE_BASENAMES:
			na_utils_free_string_list( self->private->basenames );
			self->private->basenames = na_utils_duplicate_string_list( g_value_get_pointer( value ));
			break;

		case PROP_PROFILE_ISDIR:
			self->private->is_dir = g_value_get_boolean( value );
			break;

		case PROP_PROFILE_ISFILE:
			self->private->is_file = g_value_get_boolean( value );
			break;

		case PROP_PROFILE_MATCHCASE:
			self->private->match_case = g_value_get_boolean( value );
			break;

		case PROP_PROFILE_MIMETYPES:
			na_utils_free_string_list( self->private->mimetypes );
			self->private->mimetypes = na_utils_duplicate_string_list( g_value_get_pointer( value ));
			break;

		case PROP_PROFILE_SCHEMES:
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

	g_free( self->private->name );
	g_free( self->private->label );
	g_free( self->private->path );
	g_free( self->private->parameters );
	na_utils_free_string_list( self->private->basenames );
	na_utils_free_string_list( self->private->mimetypes );
	na_utils_free_string_list( self->private->schemes );

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
 * Returns the newly allocated NAActionProfile object.
 */
NAActionProfile *
na_action_profile_new( const NAObject *action, const gchar *name )
{
	g_assert( NA_IS_ACTION( action ));
	g_assert( name && strlen( name ));

	NAActionProfile *profile =
		g_object_new(
				NA_ACTION_PROFILE_TYPE,
				PROP_PROFILE_ACTION_STR, action, PROP_PROFILE_NAME_STR, name, NULL );

	return( profile );
}

/**
 * Duplicates a profile.
 *
 * @profile: the profile to be duplicated.
 *
 * Returns the newly allocated NAActionProfile object.
 *
 * Note the duplicated profile has the same internal name (identifier)
 * as the initial one, and thus cannot be attached to the same action.
 */
NAActionProfile *
na_action_profile_copy( const NAActionProfile *profile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	NAActionProfile *new =
		na_action_profile_new( profile->private->action, profile->private->name );

	g_object_set( G_OBJECT( new ),
			PROP_PROFILE_LABEL_STR, profile->private->label,
			PROP_PROFILE_PATH_STR, profile->private->path,
			PROP_PROFILE_PARAMETERS_STR, profile->private->parameters,
			PROP_PROFILE_ACCEPT_MULTIPLE_STR, profile->private->accept_multiple_files,
			PROP_PROFILE_BASENAMES_STR, profile->private->basenames,
			PROP_PROFILE_ISDIR_STR, profile->private->is_dir,
			PROP_PROFILE_ISFILE_STR, profile->private->is_file,
			PROP_PROFILE_MATCHCASE_STR, profile->private->match_case,
			PROP_PROFILE_MIMETYPES_STR, profile->private->mimetypes,
			PROP_PROFILE_SCHEMES_STR, profile->private->schemes,
			NULL );

	return( new );
}

/**
 * Frees a profile.
 *
 * @profile: the NAActionProfile object to be freed.
 */
void
na_action_profile_free( NAActionProfile *profile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));
	g_object_unref( profile );
}

static void
do_dump( const NAObject *object )
{
	static const gchar *thisfn = "na_action_profile_do_dump";

	g_assert( NA_IS_ACTION_PROFILE( object ));
	NAActionProfile *self = NA_ACTION_PROFILE( object );

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
do_get_id( const NAObject *profile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	gchar *name;
	g_object_get( G_OBJECT( profile ), PROP_PROFILE_NAME_STR, &name, NULL );

	return( name );
}

/**
 * Returns the internal name (identifier) of the profile.
 *
 * @action: an NAActionProfile object.
 *
 * The returned string must be g_freed by the caller.
 */
gchar *
na_action_profile_get_name( const NAActionProfile *profile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));
	return( na_object_get_id( NA_OBJECT( profile )));
}

static gchar *
do_get_label( const NAObject *profile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	gchar *label;
	g_object_get( G_OBJECT( profile ), PROP_PROFILE_LABEL_STR, &label, NULL );

	return( label );
}

/**
 * Returns the descriptive name (label) of the profile.
 *
 * @action: an NAAction object.
 *
 * The returned string must be g_freed by the caller.
 */
gchar *
na_action_profile_get_label( const NAActionProfile *profile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));
	return( na_object_get_label( NA_OBJECT( profile )));
}

/**
 * Returns a pointer to the action for this profile.
 *
 * @profile: the NAActionProfile object whose parent action is to be
 * retrieved.
 *
 * Note that the returned NactNaction is owned by the profile. The
 * caller should not try to free or unref it.
 */
NAObject *
na_action_profile_get_action( const NAActionProfile *profile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	gpointer action;
	g_object_get( G_OBJECT( profile ), PROP_PROFILE_ACTION_STR, &action, NULL );

	return( NA_OBJECT( action ));
}

/**
 * Returns the path of the command in the profile.
 *
 * @profile: the NAActionProfile object whose command path is to be
 * retrieved.
 *
 * The returned string should be g_freed by the caller.
 */
gchar *
na_action_profile_get_path( const NAActionProfile *profile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	gchar *path;
	g_object_get( G_OBJECT( profile ), PROP_PROFILE_PATH_STR, &path, NULL );

	return( path );
}

/**
 * Returns the parameters of the command in the profile.
 *
 * @profile: the NAActionProfile object whose command parameters are
 * to be retrieved.
 *
 * The returned string should be g_freed by the caller.
 */
gchar *
na_action_profile_get_parameters( const NAActionProfile *profile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	gchar *parameters;
	g_object_get( G_OBJECT( profile ), PROP_PROFILE_PARAMETERS_STR, &parameters, NULL );

	return( parameters );
}

/**
 * Returns the list of basenames this profile applies to.
 *
 * @profile: this NAActionProfile object.
 *
 * The returned GSList should be freed by the caller by calling
 * na_utils_free_string_list.
 */
GSList *
na_action_profile_get_basenames( const NAActionProfile *profile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	GSList *basenames;
	g_object_get( G_OBJECT( profile ), PROP_PROFILE_BASENAMES_STR, &basenames, NULL );

	return( basenames );
}

/**
 * Are specified basenames case sensitive ?
 *
 * @profile: this NAActionProfile object.
 */
gboolean
na_action_profile_get_matchcase( const NAActionProfile *profile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	gboolean matchcase;
	g_object_get( G_OBJECT( profile ), PROP_PROFILE_MATCHCASE_STR, &matchcase, NULL );

	return( matchcase );
}

/**
 * Returns the list of mimetypes this profile applies to.
 *
 * @profile: this NAActionProfile object.
 *
 * The returned GSList should be freed by the caller by calling
 * na_utils_free_string_list.
 */
GSList *
na_action_profile_get_mimetypes( const NAActionProfile *profile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	GSList *mimetypes;
	g_object_get( G_OBJECT( profile ), PROP_PROFILE_MIMETYPES_STR, &mimetypes, NULL );

	return( mimetypes );
}

/**
 * Does this profile apply to files ?
 *
 * @profile: this NAActionProfile object.
 */
gboolean
na_action_profile_get_is_file( const NAActionProfile *profile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	gboolean isfile;
	g_object_get( G_OBJECT( profile ), PROP_PROFILE_ISFILE_STR, &isfile, NULL );

	return( isfile );
}

/**
 * Does this profile apply to directories ?
 *
 * @profile: this NAActionProfile object.
 */
gboolean
na_action_profile_get_is_dir( const NAActionProfile *profile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	gboolean isdir;
	g_object_get( G_OBJECT( profile ), PROP_PROFILE_ISDIR_STR, &isdir, NULL );

	return( isdir );
}

/**
 * Does this profile apply on a multiple selection ?
 *
 * @profile: this NAActionProfile object.
 */
gboolean
na_action_profile_get_multiple( const NAActionProfile *profile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	gboolean multiple;
	g_object_get( G_OBJECT( profile ), PROP_PROFILE_ACCEPT_MULTIPLE_STR, &multiple, NULL );

	return( multiple );
}

/**
 * Returns the list of schemes this profile applies to.
 *
 * @profile: this NAActionProfile object.
 *
 * The returned GSList should be freed by the caller by calling
 * na_utils_free_string_list.
 */
GSList *
na_action_profile_get_schemes( const NAActionProfile *profile )
{
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	GSList *schemes;
	g_object_get( G_OBJECT( profile ), PROP_PROFILE_SCHEMES_STR, &schemes, NULL );

	return( schemes );
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
na_action_profile_is_candidate( const NAActionProfile *profile, GList* files )
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
			/* TODO
			 * pwi 2009-06-10
			 * don't know how to get hostname or username from GFile uri
			 */
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
