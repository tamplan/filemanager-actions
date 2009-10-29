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
#include <uuid/uuid.h>

#include "na-action.h"
#include "na-action-profile.h"
#include "na-utils.h"

/* private class data
 */
struct NAActionClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NAActionPrivate {
	gboolean       dispose_has_run;

	/* action properties
	 */
	gchar         *version;
	gboolean       enabled;

	/* list of action's profiles as NAActionProfile objects
	 *  (thanks, Frederic ;-))
	 */
	GSList        *profiles;

	/* dynamically set when reading the actions from the I/O storage
	 * subsystem
	 * defaults to FALSE unless a write has already returned an error
	 */
	gboolean       read_only;

	/* the original provider
	 * required to be able to edit/delete the action
	 */
	NAIIOProvider *provider;
};

/* action properties
 */
enum {
	PROP_NAACTION_VERSION = 1,
	PROP_NAACTION_ENABLED,
	PROP_NAACTION_READONLY,
	PROP_NAACTION_PROVIDER
};

#define PROP_NAACTION_VERSION_STR		"na-action-version"
#define PROP_NAACTION_ENABLED_STR		"na-action-enabled"
#define PROP_NAACTION_READONLY_STR		"na-action-read-only"
#define PROP_NAACTION_PROVIDER_STR		"na-action-provider"

#define NA_ACTION_LATEST_VERSION		"2.0"

static NAObjectClass *st_parent_class = NULL;

static GType     register_type( void );
static void      class_init( NAActionClass *klass );
static void      instance_init( GTypeInstance *instance, gpointer klass );
static void      instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void      instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void      instance_dispose( GObject *object );
static void      instance_finalize( GObject *object );

static NAObject *object_new( const NAObject *object );
static void      object_copy( NAObject *target, const NAObject *source );
static gboolean  object_are_equal( const NAObject *a, const NAObject *b );
static gboolean  object_is_valid( const NAObject *action );
static void      object_dump( const NAObject *action );
static gchar    *object_get_clipboard_id( const NAObject *object );

static void      free_profiles( NAAction *action );

GType
na_action_get_type( void )
{
	static GType action_type = 0;

	if( !action_type ){
		action_type = register_type();
	}

	return( action_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "na_action_register_type";

	static GTypeInfo info = {
		sizeof( NAActionClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NAAction ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	return( g_type_register_static( NA_OBJECT_ITEM_TYPE, "NAAction", &info, 0 ));
}

static void
class_init( NAActionClass *klass )
{
	static const gchar *thisfn = "na_action_class_init";
	GObjectClass *object_class;
	GParamSpec *spec;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
	object_class->set_property = instance_set_property;
	object_class->get_property = instance_get_property;

	spec = g_param_spec_string(
			PROP_NAACTION_VERSION_STR,
			"Version",
			"Version of the schema", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_NAACTION_VERSION, spec );

	spec = g_param_spec_boolean(
			PROP_NAACTION_ENABLED_STR,
			"Enabled",
			"Whether this action is enabled", TRUE,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_NAACTION_ENABLED, spec );

	spec = g_param_spec_boolean(
			PROP_NAACTION_READONLY_STR,
			"Read-only flag",
			"Is this action only readable", FALSE,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_NAACTION_READONLY, spec );

	spec = g_param_spec_pointer(
			PROP_NAACTION_PROVIDER_STR,
			"Original provider",
			"Original provider",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_NAACTION_PROVIDER, spec );

	klass->private = g_new0( NAActionClassPrivate, 1 );

	NA_OBJECT_CLASS( klass )->new = object_new;
	NA_OBJECT_CLASS( klass )->copy = object_copy;
	NA_OBJECT_CLASS( klass )->are_equal = object_are_equal;
	NA_OBJECT_CLASS( klass )->is_valid = object_is_valid;
	NA_OBJECT_CLASS( klass )->dump = object_dump;
	NA_OBJECT_CLASS( klass )->get_clipboard_id = object_get_clipboard_id;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	/*static const gchar *thisfn = "na_action_instance_init";*/
	NAAction *self;

	/*g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );*/
	g_assert( NA_IS_ACTION( instance ));
	self = NA_ACTION( instance );

	self->private = g_new0( NAActionPrivate, 1 );

	self->private->dispose_has_run = FALSE;

	/* initialize suitable default values
	 */
	self->private->version = g_strdup( NA_ACTION_LATEST_VERSION );
	self->private->enabled = TRUE;
	self->private->read_only = FALSE;
	self->private->provider = NULL;
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	NAAction *self;

	g_assert( NA_IS_ACTION( object ));
	self = NA_ACTION( object );

	switch( property_id ){
		case PROP_NAACTION_VERSION:
			g_value_set_string( value, self->private->version );
			break;

		case PROP_NAACTION_ENABLED:
			g_value_set_boolean( value, self->private->enabled );
			break;

		case PROP_NAACTION_READONLY:
			g_value_set_boolean( value, self->private->read_only );
			break;

		case PROP_NAACTION_PROVIDER:
			g_value_set_pointer( value, self->private->provider );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	NAAction *self;

	g_assert( NA_IS_ACTION( object ));
	self = NA_ACTION( object );

	switch( property_id ){
		case PROP_NAACTION_VERSION:
			g_free( self->private->version );
			self->private->version = g_value_dup_string( value );
			break;

		case PROP_NAACTION_ENABLED:
			self->private->enabled = g_value_get_boolean( value );
			break;

		case PROP_NAACTION_READONLY:
			self->private->read_only = g_value_get_boolean( value );
			break;

		case PROP_NAACTION_PROVIDER:
			self->private->provider = g_value_get_pointer( value );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_dispose( GObject *object )
{
	/*static const gchar *thisfn = "na_action_instance_dispose";*/
	NAAction *self;

	/*g_debug( "%s: object=%p", thisfn, ( void * ) object );*/

	g_assert( NA_IS_ACTION( object ));
	self = NA_ACTION( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* release the profiles */
		free_profiles( self );

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	/*static const gchar *thisfn = "na_action_instance_finalize";*/
	NAAction *self;

	/*g_debug( "%s: object=%p", thisfn, ( void * ) object );*/

	g_assert( NA_IS_ACTION( object ));
	self = ( NAAction * ) object;

	g_free( self->private->version );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * na_action_new:
 *
 * Allocates a new #NAAction object.
 *
 * The new #NAAction object is initialized with suitable default values,
 * but without any profile.
 *
 * Returns: the newly allocated #NAAction object.
 */
NAAction *
na_action_new( void )
{
	NAAction *action;

	action = g_object_new( NA_ACTION_TYPE, NULL );

	na_action_set_new_uuid( action );

	/* i18n: default label for a new action */
	na_action_set_label( action, NA_ACTION_DEFAULT_LABEL );

	return( action );
}

/**
 * na_action_new_with_profile:
 *
 * Allocates a new #NAAction object along with a default profile.
 *
 * Return: the newly allocated #NAAction action.
 */
NAAction *
na_action_new_with_profile( void )
{
	NAAction *action;
	NAActionProfile *profile;

	action = na_action_new();

	profile = na_action_profile_new();

	na_action_attach_profile( action, profile );

	return( action );
}

/**
 * na_action_get_uuid:
 * @action: the #NAAction object to be requested.
 *
 * Returns the globally unique identifier (UUID) of the action.
 *
 * Returns: the uuid of the action as a newly allocated string. This
 * returned string must be g_free() by the caller.
 *
 * See na_action_set_uuid() for some rationale about uuid.
 */
gchar *
na_action_get_uuid( const NAAction *action )
{
	return( na_object_get_id( NA_OBJECT( action )));
}

/**
 * na_action_get_label:
 * @action: the #NAAction object to be requested.
 *
 * Returns the label of the action.
 *
 * Returns: the label of the action as a newly allocated string. This
 * returned string must be g_free() by the caller.
 *
 * See na_action_set_label() for some rationale about label.
 */
gchar *
na_action_get_label( const NAAction *action )
{
	return( na_object_get_label( NA_OBJECT( action )));
}

/**
 * na_action_get_version:
 * @action: the #NAAction object to be requested.
 *
 * Returns the version of the description of the action, as found when
 * reading it from the I/O storage subsystem.
 *
 * Returns: the version of the action as a newly allocated string. This
 * returned string must be g_free() by the caller.
 *
 * See na_action_set_version() for some rationale about version.
 */
gchar *
na_action_get_version( const NAAction *action )
{
	gchar *version;

	g_assert( NA_IS_ACTION( action ));

	g_object_get( G_OBJECT( action ), PROP_NAACTION_VERSION_STR, &version, NULL );

	return( version );
}

/**
 * na_action_get_tooltip:
 * @action: the #NAAction object to be requested.
 *
 * Returns the tooltip which will be display in the Nautilus context
 * menu item for this action.
 *
 * Returns: the tooltip of the action as a newly allocated string. This
 * returned string must be g_free() by the caller.
 */
gchar *
na_action_get_tooltip( const NAAction *action )
{
	return( na_object_item_get_tooltip( NA_OBJECT_ITEM( action )));
}

/**
 * na_action_get_icon:
 * @action: the #NAAction object to be requested.
 *
 * Returns the name of the icon attached to the Nautilus context menu
 * item for this action.
 *
 * Returns: the icon name as a newly allocated string. This returned
 * string must be g_free() by the caller.
 */
gchar *
na_action_get_icon( const NAAction *action )
{
	return( na_object_item_get_icon( NA_OBJECT_ITEM( action )));
}

/*
 * TODO: remove this function
 */
gchar *
na_action_get_verified_icon_name( const NAAction *action )
{
	return( na_object_item_get_verified_icon_name( NA_OBJECT_ITEM( action )));
}

/**
 * na_action_is_enabled:
 * @action: the #NAAction object to be requested.
 *
 * Is the specified action enabled ?
 * When disabled, the action is never candidate to any selection
 *
 * Returns: %TRUE if the action is enabled, %FALSE else.
 */
gboolean
na_action_is_enabled( const NAAction *action )
{
	gboolean enabled;

	g_assert( NA_IS_ACTION( action ));

	g_object_get( G_OBJECT( action ), PROP_NAACTION_ENABLED_STR, &enabled, NULL );

	return( enabled );
}

/**
 * na_action_is_readonly:
 * @action: the #NAAction object to be requested.
 *
 * Is the specified action only readable ?
 * Or, in other words, may this action be edited and then saved to the
 * original I/O storage subsystem ?
 *
 * Returns: %TRUE if the action is editable, %FALSE else.
 */
gboolean
na_action_is_readonly( const NAAction *action )
{
	gboolean readonly;

	g_assert( NA_IS_ACTION( action ));

	g_object_get( G_OBJECT( action ), PROP_NAACTION_READONLY_STR, &readonly, NULL );

	return( readonly );
}

/**
 * na_action_get_provider:
 * @action: the #NAAction object to be requested.
 *
 * Returns the initial provider of the action (or the last which has
 * accepted a write operation). At the time of this request, this is
 * the most probable provider willing to accept a next writing
 * operation.
 *
 * Returns: a #NAIIOProvider object. The reference is
 * owned by #NAPivot pivot and should be g_object_unref() by the
 * caller.
 */
NAIIOProvider *
na_action_get_provider( const NAAction *action )
{
	NAIIOProvider *provider;

	g_assert( NA_IS_ACTION( action ));

	g_object_get( G_OBJECT( action ), PROP_NAACTION_PROVIDER_STR, &provider, NULL );

	return( provider );
}

/**
 * na_action_set_new_uuid:
 * @action: the #NAAction object to be updated.
 *
 * Set a new UUID for the action.
 */
void
na_action_set_new_uuid( NAAction *action )
{
	uuid_t uuid;
	gchar uuid_str[64];

	g_assert( NA_IS_ACTION( action ));

	uuid_generate( uuid );
	uuid_unparse_lower( uuid, uuid_str );

	na_object_set_id( NA_OBJECT( action ), uuid_str );
}

/**
 * na_action_set_uuid:
 * @action: the #NAAction object to be updated.
 * @uuid: the uuid to be set.
 *
 * Sets a new uuid for the action.
 *
 * #NAAction takes a copy of the provided UUID. This later may so be
 * g_free() by the caller after this function returns.
 *
 * This uuid is only required when writing the action to GConf in order
 * easily have unique subdirectories.
 *
 * This is an ASCII, case insensitive, string.
 *
 * UUID is transfered through import/export operations.
 *
 * Note that a user may import an action, translate it and then
 * reexport it : we so may have two different actions with the same
 * uuid.
 */
void
na_action_set_uuid( NAAction *action, const gchar *uuid )
{
	na_object_set_id( NA_OBJECT( action ), uuid );
}

/**
 * na_action_set_label:
 * @action: the #NAAction object to be updated.
 * @label: the label to be set.
 *
 * Sets a new label for the action.
 *
 * #NAAction takes a copy of the provided label. This later may so be
 * g_free() by the caller after this function returns.
 *
 * The user knows its actions through their labels, as this is the main
 * visible part (with the icon) in Nautilus context menu and in the
 * NACT ui.
 */
void
na_action_set_label( NAAction *action, const gchar *label )
{
	na_object_set_label( NA_OBJECT( action ), label );
}

/**
 * na_action_set_version:
 * @action: the #NAAction object to be updated.
 * @label: the label to be set.
 *
 * Sets a new version for the action.
 *
 * #NAAction takes a copy of the provided version. This later may so be
 * g_free() by the caller after this function returns.
 *
 * The version describes the schema of the informations in the I/O
 * storage subsystem.
 *
 * Version is stored in the #NAAction object as readen from the I/O
 * storage subsystem, even if the #NAAction object itself only reflects
 * the lastest known version. Conversion is made at load time (cf.
 * na_gconf_load_action()).
 */
void
na_action_set_version( NAAction *action, const gchar *version )
{
	g_assert( NA_IS_ACTION( action ));

	g_object_set( G_OBJECT( action ), PROP_NAACTION_VERSION_STR, version, NULL );
}

/**
 * na_action_set_tooltip:
 * @action: the #NAAction object to be updated.
 * @tooltip: the tooltip to be set.
 *
 * Sets a new tooltip for the action. Tooltip will be displayed by
 * Nautilus when the user move its mouse over the Nautilus context menu
 * item.
 *
 * #NAAction takes a copy of the provided tooltip. This later may so be
 * g_free() by the caller after this function returns.
 */
void
na_action_set_tooltip( NAAction *action, const gchar *tooltip )
{
	na_object_item_set_tooltip( NA_OBJECT_ITEM( action ), tooltip );
}

/**
 * na_action_set_icon:
 * @action: the #NAAction object to be updated.
 * @icon: the icon name to be set.
 *
 * Sets a new icon name for the action.
 *
 * #NAAction takes a copy of the provided icon name. This later may so
 * be g_free() by the caller after this function returns.
 */
void
na_action_set_icon( NAAction *action, const gchar *icon )
{
	na_object_item_set_icon( NA_OBJECT_ITEM( action ), icon );
}

/**
 * na_action_set_enabled:
 * @action: the #NAAction object to be updated.
 * @enabled: the indicator to be set.
 *
 * Sets whether the action is enabled or not.
 */
void
na_action_set_enabled( NAAction *action, gboolean enabled )
{
	g_assert( NA_IS_ACTION( action ));

	g_object_set( G_OBJECT( action ), PROP_NAACTION_ENABLED_STR, enabled, NULL );
}

/**
 * na_action_set_readonly:
 * @action: the #NAAction object to be updated.
 * @readonly: the indicator to be set.
 *
 * Sets whether the action is readonly.
 */
void
na_action_set_readonly( NAAction *action, gboolean readonly )
{
	g_assert( NA_IS_ACTION( action ));

	g_object_set( G_OBJECT( action ), PROP_NAACTION_READONLY_STR, readonly, NULL );
}

/**
 * na_action_set_provider:
 * @action: the #NAAction object to be updated.
 * @provider: the #NAIIOProvider to be set.
 *
 * Sets the I/O provider for this #NAAction.
 */
void
na_action_set_provider( NAAction *action, const NAIIOProvider *provider )
{
	g_assert( NA_IS_ACTION( action ));

	g_object_set( G_OBJECT( action ), PROP_NAACTION_PROVIDER_STR, provider, NULL );
}

/**
 * na_action_get_new_profile_name:
 * @action: the #NAAction object which will receive a new profile.
 *
 * Returns a name suitable as a new profile name.
 *
 * The search is made by iterating over the standard profile name
 * prefix : basically, we increment a counter until finding a unique
 * name. The provided name is so only suitable for the specified
 * @action.
 *
 * Returns: a newly allocated profile name, which should be g_free() by
 * the caller.
 */
gchar *
na_action_get_new_profile_name( const NAAction *action )
{
	int i;
	gboolean ok = FALSE;
	gchar *candidate = NULL;

	g_assert( NA_IS_ACTION( action ));

	for( i=1 ; !ok ; ++i ){
		g_free( candidate );
		candidate = g_strdup_printf( "%s%d", ACTION_PROFILE_PREFIX, i );
		if( !na_action_get_profile( action, candidate )){
			ok = TRUE;
		}
	}

	if( !ok ){
		g_free( candidate );
		candidate = NULL;
	}

	return( candidate );
}

/**
 * na_action_get_profile:
 * @action: the #NAAction object which is to be requested.
 * @name: the name of the searched profile.
 *
 * Returns the required profile.
 *
 * Returns: a pointer to the #NAActionProfile profile with the required
 * name.
 *
 * The returned #NAActionProfile is owned by the @action object ; the
 * caller should not try to g_free() nor g_object_unref() it.
 */
NAActionProfile *
na_action_get_profile( const NAAction *action, const gchar *name )
{
	NAActionProfile *found = NULL;
	GSList *ip;
	NAActionProfile *iprofile;
	gchar *iname;

	g_assert( NA_IS_ACTION( action ));

	for( ip = action->private->profiles ; ip && !found ; ip = ip->next ){
		iprofile = NA_ACTION_PROFILE( ip->data );
		iname = na_action_profile_get_name( iprofile );
		if( !strcmp( name, iname )){
			found = iprofile;
		}
		g_free( iname );
	}

	return( found );
}

/**
 * na_action_attach_profile:
 * @action: the #NAAction action to which the profile will be attached.
 * @profile: the #NAActionProfile profile to be attached to @action.
 *
 * Adds a profile at the end of the list of profiles.
 */
void
na_action_attach_profile( NAAction *action, NAActionProfile *profile )
{
	g_assert( NA_IS_ACTION( action ));
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	action->private->profiles = g_slist_append( action->private->profiles, ( gpointer ) profile );

	na_action_profile_set_action( profile, action );
}

/**
 * na_action_remove_profile:
 * @action: the #NAAction action from which the profile will be removed.
 * @profile: the #NAActionProfile profile to be removed from @action.
 *
 * Removes a profile from the list of profiles.
 */
void
na_action_remove_profile( NAAction *action, NAActionProfile *profile )
{
	g_assert( NA_IS_ACTION( action ));
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	action->private->profiles = g_slist_remove( action->private->profiles, ( gconstpointer ) profile );
}

/**
 * na_action_get_profiles:
 * @action: the #NAAction action whose profiles has to be retrieved.
 *
 * Returns the list of profiles of the action.
 *
 * Returns: a #GSList of #NAActionProfile objects. The returned pointer
 * is owned by the @action object ; the caller should not try to
 * g_free() nor g_object_unref() it.
 */
GSList *
na_action_get_profiles( const NAAction *action )
{
	g_assert( NA_IS_ACTION( action ));

	return( action->private->profiles );
}

/**
 * na_action_set_profiles:
 * @action: the #NAAction action whose profiles has to be set.
 * @list: a #GSList list of #NAActionProfile objects to be installed in
 * the @action.
 *
 * Sets the list of the profiles for the action.
 *
 * The provided list removes and replaces the previous profiles list.
 * This list is then copied to the action, and thus can then be safely
 * na_action_free_profiles() by the caller.
 */
void
na_action_set_profiles( NAAction *action, GSList *list )
{
	GSList *ip;
	NAObject *new_profile;

	g_assert( NA_IS_ACTION( action ));

	free_profiles( action );

	for( ip = list ; ip ; ip = ip->next ){
		new_profile = na_object_duplicate( NA_OBJECT( ip->data ));
		na_action_attach_profile( action, NA_ACTION_PROFILE( new_profile ));
	}
}

/**
 * na_action_free_profiles:
 * @list: a #GSList list of #NAActionProfile objects.
 *
 * Frees a profiles list.
 */
void
na_action_free_profiles( GSList *list )
{
	GSList *ip;

	for( ip = list ; ip ; ip = ip->next ){
		g_object_unref( NA_ACTION_PROFILE( ip->data ));
	}

	g_slist_free( list );
}

/**
 * na_action_get_profiles_count:
 * @action: the #NAAction action whose profiles has to be counted.
 *
 * Returns the number of profiles which are defined for the action.
 *
 * Returns: the number of profiles defined for @action.
 */
guint
na_action_get_profiles_count( const NAAction *action )
{
	g_assert( NA_IS_ACTION( action ));

	return( g_slist_length( action->private->profiles ));
}

static NAObject *
object_new( const NAObject *action )
{
	return( NA_OBJECT( na_action_new()));
}

void
object_copy( NAObject *target, const NAObject *source )
{
	gchar *version;
	gboolean enabled, readonly;
	gpointer provider;
	GSList *ip;
	NAActionProfile *profile;

	if( st_parent_class->copy ){
		st_parent_class->copy( target, source );
	}

	g_assert( NA_IS_ACTION( source ));
	g_assert( NA_IS_ACTION( target ));

	g_object_get( G_OBJECT( source ),
			PROP_NAACTION_VERSION_STR, &version,
			PROP_NAACTION_ENABLED_STR, &enabled,
			PROP_NAACTION_READONLY_STR, &readonly,
			PROP_NAACTION_PROVIDER_STR, &provider,
			NULL );

	g_object_set( G_OBJECT( target ),
			PROP_NAACTION_VERSION_STR, version,
			PROP_NAACTION_ENABLED_STR, enabled,
			PROP_NAACTION_READONLY_STR, readonly,
			PROP_NAACTION_PROVIDER_STR, provider,
			NULL );

	g_free( version );

	na_action_free_profiles( NA_ACTION( target )->private->profiles );
	NA_ACTION( target )->private->profiles = NULL;

	for( ip = NA_ACTION( source )->private->profiles ; ip ; ip = ip->next ){
		profile = NA_ACTION_PROFILE( na_object_duplicate( NA_OBJECT( ip->data )));
		na_action_attach_profile( NA_ACTION( target ), profile );
	}
}

static gboolean
object_are_equal( const NAObject *a, const NAObject *b )
{
	NAAction *first, *second;
	gboolean equal = TRUE;
	NAActionProfile *first_profile, *second_profile;
	gchar *first_name, *second_name;

	if( equal ){
		if( st_parent_class->are_equal ){
			equal = st_parent_class->are_equal( a, b );
		}
	}

	g_assert( NA_IS_ACTION( a ));
	first = NA_ACTION( a );

	g_assert( NA_IS_ACTION( b ));
	second = NA_ACTION( b );

	if( equal ){
		equal = ( g_utf8_collate( first->private->version, second->private->version ) == 0 );
	}

	if( equal ){
		equal = ( first->private->enabled && second->private->enabled ) ||
				( !first->private->enabled && !second->private->enabled );
	}

	if( equal ){
		equal = ( g_slist_length( first->private->profiles ) == g_slist_length( second->private->profiles ));
	}

	if( equal ){
		GSList *ip;
		for( ip = first->private->profiles ; ip && equal ; ip = ip->next ){
			first_profile = NA_ACTION_PROFILE( ip->data );
			first_name = na_action_profile_get_name( first_profile );
			second_profile = NA_ACTION_PROFILE( na_action_get_profile( second, first_name ));
			if( second_profile ){
				equal = na_object_are_equal( NA_OBJECT( first_profile ), NA_OBJECT( second_profile ));
			} else {
				equal = FALSE;
			}
			g_free( first_name );
		}
	}

	if( equal ){
		GSList *ip;
		for( ip = second->private->profiles ; ip && equal ; ip = ip->next ){
			second_profile = NA_ACTION_PROFILE( ip->data );
			second_name = na_action_profile_get_name( second_profile );
			first_profile = NA_ACTION_PROFILE( na_action_get_profile( first, second_name ));
			if( first_profile ){
				equal = na_object_are_equal( NA_OBJECT( first_profile ), NA_OBJECT( second_profile ));
			} else {
				equal = FALSE;
			}
			g_free( second_name );
		}
	}

	return( equal );
}

/*
 * a valid NAAction requires a not null, not empty label
 * this is checked here as NAObject doesn't have this condition
 */
gboolean
object_is_valid( const NAObject *action )
{
	gchar *label;
	gboolean is_valid = TRUE;
	GSList *ip;

	if( is_valid ){
		if( st_parent_class->is_valid ){
			is_valid = st_parent_class->is_valid( action );
		}
	}

	g_assert( NA_IS_ACTION( action ));

	if( is_valid ){
		label = na_action_get_label( NA_ACTION( action ));
		is_valid = ( label && g_utf8_strlen( label, -1 ) > 0 );
		g_free( label );
	}

	for( ip = NA_ACTION( action )->private->profiles ; ip && is_valid ; ip = ip->next ){
		is_valid = na_object_is_valid( NA_OBJECT( ip->data ));
	}

	return( is_valid );
}

static void
object_dump( const NAObject *action )
{
	static const gchar *thisfn = "na_action_object_dump";
	NAAction *self;
	GSList *item;

	if( st_parent_class->dump ){
		st_parent_class->dump( action );
	}

	g_assert( NA_IS_ACTION( action ));
	self = NA_ACTION( action );

	g_debug( "%s:   version='%s'", thisfn, self->private->version );
	g_debug( "%s:   enabled='%s'", thisfn, self->private->enabled ? "True" : "False" );
	g_debug( "%s: read-only='%s'", thisfn, self->private->read_only ? "True" : "False" );
	g_debug( "%s:  provider=%p", thisfn, ( void * ) self->private->provider );

	/* dump profiles */
	g_debug( "%s: %d profile(s) at %p", thisfn, na_action_get_profiles_count( self ), ( void * ) self->private->profiles );
	for( item = self->private->profiles ;	item != NULL ; item = item->next ){
		na_object_dump(( const NAObject * ) item->data );
	}
}

static void
free_profiles( NAAction *action )
{
	na_action_free_profiles( action->private->profiles );

	action->private->profiles = NULL;
}

static gchar *
object_get_clipboard_id( const NAObject *action )
{
	gchar *uuid;
	gchar *clipboard_id;

	uuid = na_object_get_id( action );
	clipboard_id = g_strdup_printf( "A:%s", uuid );
	g_free( uuid );

	return( clipboard_id );
}
