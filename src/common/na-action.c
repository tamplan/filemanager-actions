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
};

/* private instance data
 */
struct NAActionPrivate {
	gboolean  dispose_has_run;

	/* action properties
	 */
	gchar    *uuid;
	gchar    *version;
	gchar    *label;
	gchar    *tooltip;
	gchar    *icon;

	/* list of action's profiles as NAActionProfile objects
	 *  (thanks, Frederic ;-))
	 */
	GSList   *profiles;

	/* dynamically set when reading the actions from the I/O storage
	 * subsystem
	 * defaults to FALSE unless a write has already returned an error
	 */
	gboolean  read_only;

	/* the original provider
	 * required to be able to edit/delete the action
	 */
	gpointer  provider;
};

/* instance properties
 * please note that property names must have the same spelling as the
 * NactIIOProvider parameters
 */
enum {
	PROP_ACTION_UUID = 1,
	PROP_ACTION_VERSION,
	PROP_ACTION_LABEL,
	PROP_ACTION_TOOLTIP,
	PROP_ACTION_ICON,
	PROP_ACTION_READONLY,
	PROP_ACTION_PROVIDER
};

#define NA_ACTION_LATEST_VERSION	"2.0"

static NAObjectClass *st_parent_class = NULL;

static GType  register_type( void );
static void   class_init( NAActionClass *klass );
static void   instance_init( GTypeInstance *instance, gpointer klass );
static void   instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void   instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void   instance_dispose( GObject *object );
static void   instance_finalize( GObject *object );

static void   do_dump( const NAObject *action );
static gchar *do_get_id( const NAObject *action );
static gchar *do_get_label( const NAObject *action );

static void   free_profiles( NAAction *action );

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

	return( g_type_register_static( NA_OBJECT_TYPE, "NAAction", &info, 0 ));
}

static void
class_init( NAActionClass *klass )
{
	static const gchar *thisfn = "na_action_class_init";
	g_debug( "%s: klass=%p", thisfn, klass );

	st_parent_class = g_type_class_peek_parent( klass );

	GObjectClass *object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
	object_class->set_property = instance_set_property;
	object_class->get_property = instance_get_property;

	GParamSpec *spec;

	/* the id of the object is marked as G_PARAM_CONSTRUCT_ONLY */
	spec = g_param_spec_string(
			PROP_ACTION_UUID_STR,
			PROP_ACTION_UUID_STR,
			"Globally unique identifier (UUID) of the action", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_ACTION_UUID, spec );

	spec = g_param_spec_string(
			PROP_ACTION_VERSION_STR,
			PROP_ACTION_VERSION_STR,
			"Version of the schema", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_ACTION_VERSION, spec );

	spec = g_param_spec_string(
			PROP_ACTION_LABEL_STR,
			PROP_ACTION_LABEL_STR,
			"Context menu displayable label", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_ACTION_LABEL, spec );

	spec = g_param_spec_string(
			PROP_ACTION_TOOLTIP_STR,
			PROP_ACTION_TOOLTIP_STR,
			"Context menu tooltip", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_ACTION_TOOLTIP, spec );

	spec = g_param_spec_string(
			PROP_ACTION_ICON_STR,
			PROP_ACTION_ICON_STR,
			"Context menu displayable icon", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_ACTION_ICON, spec );

	spec = g_param_spec_boolean(
			PROP_ACTION_READONLY_STR,
			PROP_ACTION_READONLY_STR,
			"Is this action only readable", FALSE,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_ACTION_READONLY, spec );

	spec = g_param_spec_pointer(
			PROP_ACTION_PROVIDER_STR,
			PROP_ACTION_PROVIDER_STR,
			"Original provider",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_ACTION_PROVIDER, spec );

	klass->private = g_new0( NAActionClassPrivate, 1 );

	NA_OBJECT_CLASS( klass )->dump = do_dump;
	NA_OBJECT_CLASS( klass )->get_id = do_get_id;
	NA_OBJECT_CLASS( klass )->get_label = do_get_label;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "na_action_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );

	g_assert( NA_IS_ACTION( instance ));
	NAAction* self = NA_ACTION( instance );

	self->private = g_new0( NAActionPrivate, 1 );

	self->private->dispose_has_run = FALSE;

	/* initialize suitable default values
	 */
	self->private->version = g_strdup( NA_ACTION_LATEST_VERSION );
	self->private->label = g_strdup( "" );
	self->private->tooltip = g_strdup( "" );
	self->private->icon = g_strdup( "" );
	self->private->read_only = FALSE;
	self->private->provider = NULL;
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	g_assert( NA_IS_ACTION( object ));
	NAAction *self = NA_ACTION( object );

	switch( property_id ){
		case PROP_ACTION_UUID:
			g_value_set_string( value, self->private->uuid );
			break;

		case PROP_ACTION_VERSION:
			g_value_set_string( value, self->private->version );
			break;

		case PROP_ACTION_LABEL:
			g_value_set_string( value, self->private->label );
			break;

		case PROP_ACTION_TOOLTIP:
			g_value_set_string( value, self->private->tooltip );
			break;

		case PROP_ACTION_ICON:
			g_value_set_string( value, self->private->icon );
			break;

		case PROP_ACTION_READONLY:
			g_value_set_boolean( value, self->private->read_only );
			break;

		case PROP_ACTION_PROVIDER:
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
	g_assert( NA_IS_ACTION( object ));
	NAAction *self = NA_ACTION( object );

	switch( property_id ){
		case PROP_ACTION_UUID:
			g_free( self->private->uuid );
			self->private->uuid = g_value_dup_string( value );
			break;

		case PROP_ACTION_VERSION:
			g_free( self->private->version );
			self->private->version = g_value_dup_string( value );
			break;

		case PROP_ACTION_LABEL:
			g_free( self->private->label );
			self->private->label = g_value_dup_string( value );
			break;

		case PROP_ACTION_TOOLTIP:
			g_free( self->private->tooltip );
			self->private->tooltip = g_value_dup_string( value );
			break;

		case PROP_ACTION_ICON:
			g_free( self->private->icon );
			self->private->icon = g_value_dup_string( value );
			break;

		case PROP_ACTION_READONLY:
			self->private->read_only = g_value_get_boolean( value );
			break;

		case PROP_ACTION_PROVIDER:
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
	static const gchar *thisfn = "na_action_instance_dispose";
	g_debug( "%s: object=%p", thisfn, object );

	g_assert( NA_IS_ACTION( object ));
	NAAction *self = NA_ACTION( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* release the profiles */
		free_profiles( self );

		/* chain up to the parent class */
		G_OBJECT_CLASS( st_parent_class )->dispose( object );
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "na_action_instance_finalize";
	g_debug( "%s: object=%p", thisfn, object );

	g_assert( NA_IS_ACTION( object ));
	NAAction *self = ( NAAction * ) object;

	g_free( self->private->uuid );
	g_free( self->private->version );
	g_free( self->private->label );
	g_free( self->private->tooltip );
	g_free( self->private->icon );

	g_free( self->private );

	/* chain call to parent class */
	if((( GObjectClass * ) st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * Allocates a new NAAction object.
 *
 * @uuid: the globally unique identifier (UUID) of the action.
 * If NULL, a new UUID is allocated for this action.
 *
 * Return a newly allocated NAAction object.
 */
NAAction *
na_action_new( const gchar *uuid )
{
	NAAction *action = g_object_new( NA_ACTION_TYPE, PROP_ACTION_UUID_STR, uuid, NULL );

	if( !uuid || !strlen( uuid )){
		na_action_set_new_uuid( action );
	}

	return( action );
}

/**
 * Allocates a new NAAction object along with a default profile.
 *
 * Return a newly allocated NAAction object.
 */
NAAction *
na_action_new_with_profile( void )
{
	NAAction *action = na_action_new( NULL );

	NAActionProfile *profile = na_action_profile_new( NA_OBJECT( action ), ACTION_PROFILE_PREFIX "zero" );

	action->private->profiles = g_slist_prepend( action->private->profiles, profile );

	return( action );
}

/**
 * Allocates a new NAAction object, and initializes it as an exact
 * copy of the specified action.
 *
 * @action: the action to be duplicated.
 *
 * Return a newly allocated NAAction object.
 *
 * Please note than "an exact copy" here means that the newly allocated
 * returned object has the _same_ UUID than the original one.
 */
NAAction *
na_action_duplicate( const NAAction *action )
{
	g_assert( NA_IS_ACTION( action ));

	gchar *uuid = do_get_id( NA_OBJECT( action ));
	NAAction *duplicate = g_object_new( NA_ACTION_TYPE, PROP_ACTION_UUID_STR, uuid, NULL );
	g_free( uuid );

	duplicate->private->version = g_strdup( action->private->version );
	duplicate->private->label = g_strdup( action->private->label );
	duplicate->private->tooltip = g_strdup( action->private->tooltip );
	duplicate->private->icon = g_strdup( action->private->icon );
	duplicate->private->read_only = action->private->read_only;
	duplicate->private->provider = action->private->provider;

	GSList *ip;
	for( ip = action->private->profiles ; ip ; ip = ip->next ){
		duplicate->private->profiles =
			g_slist_prepend(
					duplicate->private->profiles,
					na_action_profile_duplicate( duplicate, NA_ACTION_PROFILE( ip->data )));
	}

	return( duplicate );
}

static void
do_dump( const NAObject *action )
{
	static const gchar *thisfn = "na_action_do_dump";

	g_assert( NA_IS_ACTION( action ));
	NAAction *self = NA_ACTION( action );

	if( st_parent_class->dump ){
		st_parent_class->dump( action );
	}

	g_debug( "%s:      uuid='%s'", thisfn, self->private->uuid );
	g_debug( "%s:   version='%s'", thisfn, self->private->version );
	g_debug( "%s:     label='%s'", thisfn, self->private->label );
	g_debug( "%s:   tooltip='%s'", thisfn, self->private->tooltip );
	g_debug( "%s:      icon='%s'", thisfn, self->private->icon );
	g_debug( "%s: read-only='%s'", thisfn, self->private->read_only ? "True" : "False" );
	g_debug( "%s:  provider=%p", thisfn, self->private->provider );

	/* dump profiles */
	g_debug( "%s: %d profile(s) at %p", thisfn, na_action_get_profiles_count( self ), self->private->profiles );
	GSList *item;
	for( item = self->private->profiles ;	item != NULL ; item = item->next ){
		na_object_dump(( const NAObject * ) item->data );
	}
}

static gchar *
do_get_id( const NAObject *action )
{
	g_assert( NA_IS_ACTION( action ));

	gchar *uuid;
	g_object_get( G_OBJECT( action ), PROP_ACTION_UUID_STR, &uuid, NULL );

	return( uuid );
}

/**
 * Returns the globally unique identifier (UUID) of the action.
 *
 * @action: an NAAction object.
 *
 * The returned string must be g_freed by the caller.
 */
gchar *
na_action_get_uuid( const NAAction *action )
{
	g_assert( NA_IS_ACTION( action ));
	return( na_object_get_id( NA_OBJECT( action )));
}

/**
 * Returns the version attached to the action.
 *
 * @action: an NAAction object.
 *
 * The returned string must be g_freed by the caller.
 *
 * The version is always upgraded to the latest when the action is
 * readen from the I/O provider. So we could assert here that the
 * returned version is also the latest.
 */
gchar *
na_action_get_version( const NAAction *action )
{
	g_assert( NA_IS_ACTION( action ));

	gchar *version;
	g_object_get( G_OBJECT( action ), PROP_ACTION_VERSION_STR, &version, NULL );

	return( version );
}

static gchar *
do_get_label( const NAObject *action )
{
	g_assert( NA_IS_ACTION( action ));

	gchar *label;
	g_object_get( G_OBJECT( action ), PROP_ACTION_LABEL_STR, &label, NULL );

	return( label );
}

/**
 * Returns the label of the action.
 *
 * @action: an NAAction object.
 *
 * The returned string must be g_freed by the caller.
 */
gchar *
na_action_get_label( const NAAction *action )
{
	g_assert( NA_IS_ACTION( action ));
	return( na_object_get_label( NA_OBJECT( action )));
}

/**
 * Returns the tooltip attached to the context menu item for the
 * action.
 *
 * @action: an NAAction object.
 *
 * The returned string must be g_freed by the caller.
 */
gchar *
na_action_get_tooltip( const NAAction *action )
{
	g_assert( NA_IS_ACTION( action ));

	gchar *tooltip;
	g_object_get( G_OBJECT( action ), PROP_ACTION_TOOLTIP_STR, &tooltip, NULL );

	return( tooltip );
}

/**
 * Returns the name of the icon attached to the context menu item for
 * the action.
 *
 * @action: an NAAction object.
 *
 * The returned string must be g_freed by the caller.
 */
gchar *
na_action_get_icon( const NAAction *action )
{
	g_assert( NA_IS_ACTION( action ));

	gchar *icon;
	g_object_get( G_OBJECT( action ), PROP_ACTION_ICON_STR, &icon, NULL );

	return( icon );
}

/**
 * Returns the icon name attached to the context menu item for the
 * action.
 *
 * @action: an NAAction object.
 *
 * When not NULL, the returned string must be g_free by the caller.
 */
gchar *
na_action_get_verified_icon_name( const NAAction *action )
{
	g_assert( NA_IS_ACTION( action ));

	gchar *icon_name;
	g_object_get( G_OBJECT( action ), PROP_ACTION_ICON_STR, &icon_name, NULL );

	if( icon_name[0] == '/' ){
		if( !g_file_test( icon_name, G_FILE_TEST_IS_REGULAR )){
			g_free( icon_name );
			return NULL;
		}
	} else if( strlen( icon_name ) == 0 ){
		g_free( icon_name );
		return NULL;
	}

	return( icon_name );
}

/**
 * Is the specified action only readable ?
 * Or, in other words, may this action be edited and then saved ?
 *
 * @action: an NAAction object.
 */
gboolean
na_action_is_readonly( const NAAction *action )
{
	g_assert( NA_IS_ACTION( action ));
	return( action->private->read_only );
}

/**
 * Returns the initial provider of the action (or the last which has
 * accepted a write).
 *
 * @action: an NAAction object.
 */
gpointer
na_action_get_provider( const NAAction *action )
{
	g_assert( NA_IS_ACTION( action ));
	return( action->private->provider );
}

/**
 * Set a new UUID for the action.
 *
 * @action: action whose UUID is to be set.
 */
void
na_action_set_new_uuid( NAAction *action )
{
	g_assert( NA_IS_ACTION( action ));
	uuid_t uuid;
	gchar uuid_str[64];

	uuid_generate( uuid );
	uuid_unparse_lower( uuid, uuid_str );

	g_object_set( G_OBJECT( action ), PROP_ACTION_UUID_STR, uuid_str, NULL );
}

/**
 * Set a new uuid for the action.
 *
 * @action: action whose uuid is to be set.
 *
 * @uuid: new uuid.
 */
void
na_action_set_uuid( NAAction *action, const gchar *uuid )
{
	g_assert( NA_IS_ACTION( action ));
	g_object_set( G_OBJECT( action ), PROP_ACTION_UUID_STR, uuid, NULL );
}

/**
 * Set a new version for the action.
 *
 * @action: action whose version is to be set.
 *
 * @version: new version.
 */
void
na_action_set_version( NAAction *action, const gchar *version )
{
	g_assert( NA_IS_ACTION( action ));
	g_object_set( G_OBJECT( action ), PROP_ACTION_VERSION_STR, version, NULL );
}

/**
 * Set a new label for the action.
 *
 * @action: action whose label is to be set.
 *
 * @label: new label.
 */
void
na_action_set_label( NAAction *action, const gchar *label )
{
	g_assert( NA_IS_ACTION( action ));
	g_object_set( G_OBJECT( action ), PROP_ACTION_LABEL_STR, label, NULL );
}

/**
 * Set a new tooltip for the action.
 *
 * @action: action whose tooltip is to be set.
 *
 * @tooltip: new tooltip.
 */
void
na_action_set_tooltip( NAAction *action, const gchar *tooltip )
{
	g_assert( NA_IS_ACTION( action ));
	g_object_set( G_OBJECT( action ), PROP_ACTION_TOOLTIP_STR, tooltip, NULL );
}

/**
 * Set a new icon for the action.
 *
 * @action: action whose icon name is to be set.
 *
 * @icon: new icon name.
 */
void
na_action_set_icon( NAAction *action, const gchar *icon )
{
	g_assert( NA_IS_ACTION( action ));
	g_object_set( G_OBJECT( action ), PROP_ACTION_ICON_STR, icon, NULL );
}

/**
 * Are the two actions the sames (excluding UUID) ?
 *
 * @first: first action to check.
 *
 * @second: second action to be compared to @first.
 */
gboolean
na_action_are_equal( NAAction *first, NAAction *second )
{
	gboolean equal =
		( g_utf8_collate( first->private->label, second->private->label ) == 0 ) &&
		( g_utf8_collate( first->private->tooltip, second->private->tooltip ) == 0 ) &&
		( g_utf8_collate( first->private->icon, second->private->icon ) == 0 );

	if( equal ){
		equal = ( g_slist_length( first->private->profiles ) == g_slist_length( second->private->profiles ));
	}
	if( equal ){
		GSList *ip;
		for( ip = first->private->profiles ; ip && equal ; ip = ip->next ){
			NAActionProfile *first_profile = NA_ACTION_PROFILE( ip->data );
			gchar *first_name = na_action_profile_get_name( first_profile );
			NAActionProfile *second_profile = NA_ACTION_PROFILE( na_action_get_profile( second, first_name ));
			if( second_profile ){
				equal = na_action_profile_are_equal( first_profile, second_profile );
			} else {
				equal = FALSE;
			}
			g_free( first_name );
		}
	}
	if( equal ){
		GSList *ip;
		for( ip = second->private->profiles ; ip && equal ; ip = ip->next ){
			NAActionProfile *second_profile = NA_ACTION_PROFILE( ip->data );
			gchar *second_name = na_action_profile_get_name( second_profile );
			NAActionProfile *first_profile = NA_ACTION_PROFILE( na_action_get_profile( first, second_name ));
			if( first_profile ){
				equal = na_action_profile_are_equal( first_profile, second_profile );
			} else {
				equal = FALSE;
			}
			g_free( second_name );
		}
	}
	return( equal );
}

/**
 * Returns the profile with the required name.
 *
 * @action: the action whose profiles has to be retrieved.
 *
 * @name: name of the required profile.
 *
 * The returned pointer is owned by the @action object ; the caller
 * should not try to free or unref it.
 */
NAObject *
na_action_get_profile( const NAAction *action, const gchar *name )
{
	g_assert( NA_IS_ACTION( action ));
	NAObject *found = NULL;
	GSList *ip;

	for( ip = action->private->profiles ; ip && !found ; ip = ip->next ){
		NAActionProfile *iprofile = NA_ACTION_PROFILE( ip->data );
		gchar *iname = na_action_profile_get_name( iprofile );
		if( !g_ascii_strcasecmp( name, iname )){
			found = NA_OBJECT( iprofile );
		}
		g_free( iname );
	}

	return( found );
}

/**
 * Add a profile at the end of the list of profiles.
 *
 * @action: the action.
 *
 * @profile: the added profile.
 */
void
na_action_add_profile( NAAction *action, NAObject *profile )
{
	g_assert( NA_IS_ACTION( action ));
	g_assert( NA_IS_ACTION_PROFILE( profile ));

	action->private->profiles = g_slist_append( action->private->profiles, profile );
}

/**
 * Returns the list of profiles of the actions as a GSList of
 * NAActionProfile GObjects.
 *
 * @action: the action whose profiles has to be retrieved.
 *
 * The returned pointer is owned by the @action object ; the caller
 * should not try to free or unref it.
 */
GSList *
na_action_get_profiles( const NAAction *action )
{
	g_assert( NA_IS_ACTION( action ));

	return( action->private->profiles );
}

/**
 * Set the list of the profiles for the action.
 *
 * @action: the action whose profiles has to be retrieved.
 *
 * @list: a list of NAActionProfile objects to be installed in the
 * action.
 *
 * The provided list is copied to the action, and thus can then be
 * safely freed (see na_action_free_profiles) by the caller.
 */
void
na_action_set_profiles( NAAction *action, GSList *list )
{
	g_assert( NA_IS_ACTION( action ));

	free_profiles( action );
	GSList *ip;
	for( ip = list ; ip ; ip = ip->next ){
		action->private->profiles = g_slist_prepend(
							action->private->profiles,
							na_action_profile_duplicate( action, NA_ACTION_PROFILE( ip->data ))
		);
	}
}

/**
 * Returns the number of profiles which are defined for the action.
 *
 * @action: the action whose profiles has to be retrieved.
 */
guint
na_action_get_profiles_count( const NAAction *action )
{
	g_assert( NA_IS_ACTION( action ));

	return( g_slist_length( action->private->profiles ));
}

static void
free_profiles( NAAction *action )
{
	g_assert( NA_IS_ACTION( action ));

	na_action_free_profiles( action->private->profiles );

	action->private->profiles = NULL;
}

/**
 * Frees a profiles list.
 *
 * @list: a list of NAActionProfile objects.
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
