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

#include "na-object-api.h"
#include "na-obj-action.h"
#include "na-obj-profile.h"
#include "na-utils.h"

/* private class data
 */
struct NAObjectActionClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NAObjectActionPrivate {
	gboolean dispose_has_run;

	/* action properties
	 */
	gchar   *version;

	/* dynamically set when reading the actions from the I/O storage
	 * subsystem
	 * defaults to FALSE unless a write has already returned an error
	 */
	gboolean read_only;
};

/* action properties
 */
enum {
	NAACTION_PROP_VERSION_ID = 1,
	NAACTION_PROP_READONLY_ID
};

#define NAACTION_PROP_VERSION			"na-action-version"
#define NAACTION_PROP_READONLY			"na-action-read-only"

static NAObjectItemClass *st_parent_class = NULL;

static GType     register_type( void );
static void      class_init( NAObjectActionClass *klass );
static void      instance_init( GTypeInstance *instance, gpointer klass );
static void      instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void      instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void      instance_dispose( GObject *object );
static void      instance_finalize( GObject *object );

static void      object_dump( const NAObject *object );
static gchar    *object_get_clipboard_id( const NAObject *action );
static NAObject *object_new( const NAObject *action );
static void      object_copy( NAObject *target, const NAObject *source );
static gboolean  object_are_equal( const NAObject *a, const NAObject *b );
static gboolean  object_is_valid( const NAObject *object );

GType
na_object_action_get_type( void )
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
	static const gchar *thisfn = "na_object_register_type";

	static GTypeInfo info = {
		sizeof( NAObjectActionClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NAObjectAction ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	return( g_type_register_static( NA_OBJECT_ITEM_TYPE, "NAObjectAction", &info, 0 ));
}

static void
class_init( NAObjectActionClass *klass )
{
	static const gchar *thisfn = "na_object_class_init";
	GObjectClass *object_class;
	NAObjectClass *naobject_class;
	GParamSpec *spec;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
	object_class->set_property = instance_set_property;
	object_class->get_property = instance_get_property;

	spec = g_param_spec_string(
			NAACTION_PROP_VERSION,
			"Version",
			"Version of the schema", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, NAACTION_PROP_VERSION_ID, spec );

	spec = g_param_spec_boolean(
			NAACTION_PROP_READONLY,
			"Read-only flag",
			"Is this action only readable", FALSE,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, NAACTION_PROP_READONLY_ID, spec );

	klass->private = g_new0( NAObjectActionClassPrivate, 1 );

	naobject_class = NA_OBJECT_CLASS( klass );
	naobject_class->dump = object_dump;
	naobject_class->get_clipboard_id = object_get_clipboard_id;
	naobject_class->ref = NULL;
	naobject_class->new = object_new;
	naobject_class->copy = object_copy;
	naobject_class->are_equal = object_are_equal;
	naobject_class->is_valid = object_is_valid;
	naobject_class->get_childs = NULL;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	/*static const gchar *thisfn = "na_object_action_instance_init";*/
	NAObjectAction *self;

	/*g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );*/
	g_return_if_fail( NA_IS_OBJECT_ACTION( instance ));
	self = NA_OBJECT_ACTION( instance );

	self->private = g_new0( NAObjectActionPrivate, 1 );

	self->private->dispose_has_run = FALSE;

	/* initialize suitable default values
	 */
	self->private->version = g_strdup( NAUTILUS_ACTIONS_CONFIG_VERSION );
	self->private->read_only = FALSE;
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	NAObjectAction *self;

	g_return_if_fail( NA_IS_OBJECT_ACTION( object ));
	g_return_if_fail( !NA_OBJECT_ACTION( object )->private->dispose_has_run );
	self = NA_OBJECT_ACTION( object );

	switch( property_id ){
		case NAACTION_PROP_VERSION_ID:
			g_value_set_string( value, self->private->version );
			break;

		case NAACTION_PROP_READONLY_ID:
			g_value_set_boolean( value, self->private->read_only );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	NAObjectAction *self;

	g_return_if_fail( NA_IS_OBJECT_ACTION( object ));
	g_return_if_fail( !NA_OBJECT_ACTION( object )->private->dispose_has_run );
	self = NA_OBJECT_ACTION( object );

	switch( property_id ){
		case NAACTION_PROP_VERSION_ID:
			g_free( self->private->version );
			self->private->version = g_value_dup_string( value );
			break;

		case NAACTION_PROP_READONLY_ID:
			self->private->read_only = g_value_get_boolean( value );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_dispose( GObject *object )
{
	/*static const gchar *thisfn = "na_object_instance_dispose";*/
	NAObjectAction *self;

	/*g_debug( "%s: object=%p", thisfn, ( void * ) object );*/

	g_return_if_fail( NA_IS_OBJECT_ACTION( object ));
	self = NA_OBJECT_ACTION( object );
	g_return_if_fail( !self->private->dispose_has_run );

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
	/*static const gchar *thisfn = "na_object_instance_finalize";*/
	NAObjectAction *self;

	/*g_debug( "%s: object=%p", thisfn, ( void * ) object );*/

	g_return_if_fail( NA_IS_OBJECT_ACTION( object ));
	self = ( NAObjectAction * ) object;

	g_free( self->private->version );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * na_object_action_new:
 *
 * Allocates a new #NAObjectAction object.
 *
 * The new #NAObjectAction object is initialized with suitable default values,
 * but without any profile.
 *
 * Returns: the newly allocated #NAObjectAction object.
 */
NAObjectAction *
na_object_action_new( void )
{
	NAObjectAction *action;

	action = g_object_new( NA_OBJECT_ACTION_TYPE, NULL );

	na_object_set_new_id( NA_OBJECT_ITEM( action ));

	/* i18n: default label for a new action */
	na_object_set_label( action, NA_OBJECT_ACTION_DEFAULT_LABEL );

	return( action );
}

/**
 * na_object_action_new_with_profile:
 *
 * Allocates a new #NAObjectAction object along with a default profile.
 *
 * Return: the newly allocated #NAObjectAction action.
 */
NAObjectAction *
na_object_action_new_with_profile( void )
{
	NAObjectAction *action;
	NAObjectProfile *profile;

	action = na_object_action_new();

	profile = na_object_profile_new();

	/* i18n: name of the default profile when creating an action */
	na_object_set_label( profile, _( "Default profile" ));
	na_object_action_attach_profile( action, profile );

	return( action );
}

/**
 * na_object_action_get_version:
 * @action: the #NAObjectAction object to be requested.
 *
 * Returns the version of the description of the action, as found when
 * reading it from the I/O storage subsystem.
 *
 * Returns: the version of the action as a newly allocated string. This
 * returned string must be g_free() by the caller.
 *
 * See na_object_set_version() for some rationale about version.
 */
gchar *
na_object_action_get_version( const NAObjectAction *action )
{
	gchar *version;

	g_return_val_if_fail( NA_IS_OBJECT_ACTION( action ), NULL );
	g_return_val_if_fail( !action->private->dispose_has_run, NULL );

	g_object_get( G_OBJECT( action ), NAACTION_PROP_VERSION, &version, NULL );

	return( version );
}

/**
 * na_object_action_is_readonly:
 * @action: the #NAObjectAction object to be requested.
 *
 * Is the specified action only readable ?
 * Or, in other words, may this action be edited and then saved to the
 * original I/O storage subsystem ?
 *
 * Returns: %TRUE if the action is editable, %FALSE else.
 */
gboolean
na_object_action_is_readonly( const NAObjectAction *action )
{
	gboolean readonly;

	g_return_val_if_fail( NA_IS_OBJECT_ACTION( action ), FALSE );
	g_return_val_if_fail( !action->private->dispose_has_run, FALSE );

	g_object_get( G_OBJECT( action ), NAACTION_PROP_READONLY, &readonly, NULL );

	return( readonly );
}

/**
 * na_object_action_set_version:
 * @action: the #NAObjectAction object to be updated.
 * @label: the label to be set.
 *
 * Sets a new version for the action.
 *
 * #NAObjectAction takes a copy of the provided version. This later may so be
 * g_free() by the caller after this function returns.
 *
 * The version describes the schema of the informations in the I/O
 * storage subsystem.
 *
 * Version is stored in the #NAObjectAction object as readen from the I/O
 * storage subsystem, even if the #NAObjectAction object itself only reflects
 * the lastest known version. Conversion is made at load time (cf.
 * na_gconf_load_action()).
 */
void
na_object_action_set_version( NAObjectAction *action, const gchar *version )
{
	g_return_if_fail( NA_IS_OBJECT_ACTION( action ));
	g_return_if_fail( !action->private->dispose_has_run );

	g_object_set( G_OBJECT( action ), NAACTION_PROP_VERSION, version, NULL );
}

/**
 * na_object_action_set_readonly:
 * @action: the #NAObjectAction object to be updated.
 * @readonly: the indicator to be set.
 *
 * Sets whether the action is readonly.
 */
void
na_object_action_set_readonly( NAObjectAction *action, gboolean readonly )
{
	g_return_if_fail( NA_IS_OBJECT_ACTION( action ));
	g_return_if_fail( !action->private->dispose_has_run );

	g_object_set( G_OBJECT( action ), NAACTION_PROP_READONLY, readonly, NULL );
}

/**
 * na_object_action_get_new_profile_name:
 * @action: the #NAObjectAction object which will receive a new profile.
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
na_object_action_get_new_profile_name( const NAObjectAction *action )
{
	int i;
	gboolean ok = FALSE;
	gchar *candidate = NULL;

	g_return_val_if_fail( NA_IS_OBJECT_ACTION( action ), NULL );
	g_return_val_if_fail( !action->private->dispose_has_run, NULL );

	for( i=1 ; !ok ; ++i ){
		g_free( candidate );
		candidate = g_strdup_printf( "%s%d", OBJECT_PROFILE_PREFIX, i );
		if( !na_object_get_item( action, candidate )){
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
 * na_object_action_attach_profile:
 * @action: the #NAObjectAction action to which the profile will be attached.
 * @profile: the #NAObjectProfile profile to be attached to @action.
 *
 * Adds a profile at the end of the list of profiles.
 */
void
na_object_action_attach_profile( NAObjectAction *action, NAObjectProfile *profile )
{
	g_return_if_fail( NA_IS_OBJECT_ACTION( action ));
	g_return_if_fail( !action->private->dispose_has_run );
	g_return_if_fail( NA_IS_OBJECT_PROFILE( profile ));

	na_object_append_item( action, profile );
	na_object_profile_set_action( profile, action );
}

static void
object_dump( const NAObject *action )
{
	static const gchar *thisfn = "na_object_action_object_dump";
	NAObjectAction *self;

	g_return_if_fail( NA_IS_OBJECT_ACTION( action ));
	self = NA_OBJECT_ACTION( action );
	g_return_if_fail( !self->private->dispose_has_run );

	g_debug( "%s:   version='%s'", thisfn, self->private->version );
	g_debug( "%s: read-only='%s'", thisfn, self->private->read_only ? "True" : "False" );
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

static NAObject *
object_new( const NAObject *action )
{
	return( NA_OBJECT( na_object_action_new()));
}

void
object_copy( NAObject *target, const NAObject *source )
{
	gchar *version;
	gboolean readonly;
	GList *profiles, *ip;

	g_return_if_fail( NA_IS_OBJECT_ACTION( target ));
	g_return_if_fail( !NA_OBJECT_ACTION( target )->private->dispose_has_run );
	g_return_if_fail( NA_IS_OBJECT_ACTION( source ));
	g_return_if_fail( !NA_OBJECT_ACTION( source )->private->dispose_has_run );

	g_object_get( G_OBJECT( source ),
			NAACTION_PROP_VERSION, &version,
			NAACTION_PROP_READONLY, &readonly,
			NULL );

	g_object_set( G_OBJECT( target ),
			NAACTION_PROP_VERSION, version,
			NAACTION_PROP_READONLY, readonly,
			NULL );

	g_free( version );

	/* profiles have been copied (duplicated) as subitems by parent class
	 * we have to attach new profiles to target action
	 */
	profiles = na_object_get_items( target );
	for( ip = profiles ; ip ; ip = ip->next ){
		na_object_profile_set_action( NA_OBJECT_PROFILE( ip->data ), NA_OBJECT_ACTION( target ));
	}
	na_object_free_items( profiles );

	/*g_debug( "na_object_action_object_copy: end" );*/
}

static gboolean
object_are_equal( const NAObject *a, const NAObject *b )
{
	NAObjectAction *first, *second;
	gboolean equal = TRUE;

	g_return_val_if_fail( NA_IS_OBJECT_ACTION( a ), FALSE );
	g_return_val_if_fail( !NA_OBJECT_ACTION( a )->private->dispose_has_run, FALSE );
	first = NA_OBJECT_ACTION( a );

	g_return_val_if_fail( NA_IS_OBJECT_ACTION( b ), FALSE );
	g_return_val_if_fail( !NA_OBJECT_ACTION( b )->private->dispose_has_run, FALSE );
	second = NA_OBJECT_ACTION( b );

	if( equal ){
		equal = ( g_utf8_collate( first->private->version, second->private->version ) == 0 );
	}

	/*g_debug( "na_object_action_are_equal: %s", equal ? "True":"False" );*/
	return( equal );
}

/*
 * a valid NAObjectAction requires a not null, not empty label
 * this is checked here as NAObjectId doesn't have this condition
 *
 * and at least one profile
 * checked here because NAObjectItem doesn't have this condition
 */
gboolean
object_is_valid( const NAObject *action )
{
	gchar *label;
	gboolean is_valid = TRUE;

	g_return_val_if_fail( NA_IS_OBJECT_ACTION( action ), FALSE );
	g_return_val_if_fail( !NA_OBJECT_ACTION( action )->private->dispose_has_run, FALSE );

	if( is_valid ){
		label = na_object_get_label( NA_OBJECT_ACTION( action ));
		is_valid = ( label && g_utf8_strlen( label, -1 ) > 0 );
		g_free( label );
	}

	if( is_valid ){
		is_valid = ( na_object_get_items_count( action ) >= 1 );
	}

	return( is_valid );
}
