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

#include "nact-action.h"
#include "nact-action-profile.h"
#include "nact-uti-lists.h"

struct NactActionPrivate {
	gboolean  dispose_has_run;

	/* action properties
	 */
	gchar    *uuid;
	gchar    *version;
	gchar    *label;
	gchar    *tooltip;
	gchar    *icon;

	/* list of action's profiles as NactActionProfile objects
	 *  (thanks, Frederic ;-))
	 */
	GSList   *profiles;
};

struct NactActionClassPrivate {
};

enum {
	PROP_UUID = 1,
	PROP_VERSION,
	PROP_LABEL,
	PROP_TOOLTIP,
	PROP_ICON
};

/* please note that property names must have the same spelling as the
 * NactIIOProvider parameters
 */
#define PROP_UUID_STR		"uuid"
#define PROP_VERSION_STR	"version"
#define PROP_LABEL_STR		"label"
#define PROP_TOOLTIP_STR	"tooltip"
#define PROP_ICON_STR		"icon"

static NactObjectClass *st_parent_class = NULL;

static GType    register_type( void );
static void     class_init( NactActionClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_dispose( GObject *object );
static void     instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void     instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void     instance_finalize( GObject *object );

static void     free_profiles( NactAction *action );

static void     do_dump( const NactObject *action );

/**
 * Allocate a new NactAction object.
 *
 * @provider: a gpointer to the instance which implements the storage
 * subsystem for this action ; the @provider must implement the
 * NactIIOProvider interface.
 *
 * @data: a gpointer to some data internal to the provider ; the
 * provider may use it to store and retrieve its data on a per-object
 * basis.
 */
NactAction *
nact_action_new( const gchar *uuid )
{
	NactAction *action = g_object_new( NACT_ACTION_TYPE, PROP_UUID_STR, uuid, NULL );
	return( action );
}

GType
nact_action_get_type( void )
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
		sizeof( NactActionClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactAction ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	return( g_type_register_static( NACT_OBJECT_TYPE, "NactAction", &info, 0 ));
}

static void
class_init( NactActionClass *klass )
{
	static const gchar *thisfn = "nact_action_class_init";
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
			PROP_UUID_STR,
			PROP_UUID_STR,
			"Globally unique identifier (UUID) of the action", "",
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_UUID, spec );

	spec = g_param_spec_string(
			PROP_VERSION_STR,
			PROP_VERSION_STR,
			"Version of the schema", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_VERSION, spec );

	spec = g_param_spec_string(
			PROP_LABEL_STR,
			PROP_LABEL_STR,
			"Context menu displayable label", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_LABEL, spec );

	spec = g_param_spec_string(
			PROP_TOOLTIP_STR,
			PROP_TOOLTIP_STR,
			"Context menu tooltip", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_TOOLTIP, spec );

	spec = g_param_spec_string(
			PROP_ICON_STR,
			PROP_ICON_STR,
			"Context menu displayable icon", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_ICON, spec );

	klass->private = g_new0( NactActionClassPrivate, 1 );

	NACT_OBJECT_CLASS( klass )->dump = do_dump;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	/*static const gchar *thisfn = "nact_action_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );*/

	g_assert( NACT_IS_ACTION( instance ));
	NactAction* self = NACT_ACTION( instance );

	self->private = g_new0( NactActionPrivate, 1 );
	self->private->dispose_has_run = FALSE;
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	g_assert( NACT_IS_ACTION( object ));
	NactAction *self = NACT_ACTION( object );

	switch( property_id ){
		case PROP_UUID:
			g_value_set_string( value, self->private->uuid );
			break;

		case PROP_VERSION:
			g_value_set_string( value, self->private->version );
			break;

		case PROP_LABEL:
			g_value_set_string( value, self->private->label );
			break;

		case PROP_TOOLTIP:
			g_value_set_string( value, self->private->tooltip );
			break;

		case PROP_ICON:
			g_value_set_string( value, self->private->icon );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	g_assert( NACT_IS_ACTION( object ));
	NactAction *self = NACT_ACTION( object );

	switch( property_id ){
		case PROP_UUID:
			g_free( self->private->uuid );
			self->private->uuid = g_value_dup_string( value );
			break;

		case PROP_VERSION:
			g_free( self->private->version );
			self->private->version = g_value_dup_string( value );
			break;

		case PROP_LABEL:
			g_free( self->private->label );
			self->private->label = g_value_dup_string( value );
			break;

		case PROP_TOOLTIP:
			g_free( self->private->tooltip );
			self->private->tooltip = g_value_dup_string( value );
			break;

		case PROP_ICON:
			g_free( self->private->icon );
			self->private->icon = g_value_dup_string( value );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "nact_action_instance_dispose";
	g_debug( "%s: object=%p", thisfn, object );

	g_assert( NACT_IS_ACTION( object ));
	NactAction *self = NACT_ACTION( object );

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
	static const gchar *thisfn = "nact_action_instance_finalize";
	g_debug( "%s: object=%p", thisfn, object );

	g_assert( NACT_IS_ACTION( object ));
	NactAction *self = ( NactAction * ) object;

	g_free( self->private->uuid );
	g_free( self->private->version );
	g_free( self->private->label );
	g_free( self->private->tooltip );
	g_free( self->private->icon );

	/* chain call to parent class */
	if((( GObjectClass * ) st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

static void
free_profiles( NactAction *action )
{
	g_assert( NACT_IS_ACTION( action ));

	GSList *ip;
	for( ip = action->private->profiles ; ip ; ip = ip->next ){
		g_object_unref( NACT_ACTION_PROFILE( ip->data ));
	}
	g_slist_free( action->private->profiles );
	action->private->profiles = NULL;
}

/**
 * Create a new NactAction object, with the given parm and value.
 *
 * Note that the parm may actually be a profile's parm.
 */
NactAction *
nact_action_create( const gchar *key, const gchar *parm, const NactPivotValue *value )
{
	static const gchar *thisfn = "nact_action_create";
	g_debug( "%s: key='%s', parm='%s', value=%p", thisfn, key, parm, value );

	NactAction *action = g_object_new( NACT_ACTION_TYPE, NULL );
	nact_action_update( action, parm, value );
	return( action );
}

/**
 * Update the given parameter of an action.
 */
void
nact_action_update( NactAction *action, const gchar *parm, const NactPivotValue *value )
{
	static const gchar *thisfn = "nact_action_update";
	g_debug( "%s: action=%p, parm='%s', value=%p", thisfn, action, parm, value );
}

static void
do_dump( const NactObject *action )
{
	static const gchar *thisfn = "nact_action_do_dump";

	g_assert( NACT_IS_ACTION( action ));
	NactAction *self = NACT_ACTION( action );

	if( st_parent_class->dump ){
		st_parent_class->dump( action );
	}

	g_debug( "%s:    uuid='%s'", thisfn, self->private->uuid );
	g_debug( "%s: version='%s'", thisfn, self->private->version );
	g_debug( "%s:   label='%s'", thisfn, self->private->label );
	g_debug( "%s: tooltip='%s'", thisfn, self->private->tooltip );
	g_debug( "%s:    icon='%s'", thisfn, self->private->icon );

	/* dump profiles */
	g_debug( "%s: %d profile(s) at %p", thisfn, nact_action_get_profiles_count( self ), self->private->profiles );
	GSList *item;
	for( item = self->private->profiles ;	item != NULL ; item = item->next ){
		nact_object_dump(( const NactObject * ) item->data );
	}
}

/**
 * Return the globally unique identifier (UUID) of the action.
 *
 * @action: an NactAction object.
 *
 * The returned string must be g_free by the caller.
 */
gchar *
nact_action_get_uuid( const NactAction *action )
{
	g_assert( NACT_IS_ACTION( action ));

	gchar *uuid;
	g_object_get( G_OBJECT( action ), PROP_UUID_STR, &uuid, NULL );

	return( uuid );
}

/**
 * Return the label of the context menu item for the action.
 *
 * @action: an NactAction object.
 *
 * The returned string must be g_free by the caller.
 */
gchar *
nact_action_get_label( const NactAction *action )
{
	g_assert( NACT_IS_ACTION( action ));

	gchar *label;
	g_object_get( G_OBJECT( action ), PROP_LABEL_STR, &label, NULL );

	return( label );
}

/**
 * Return the tooltip attached to the context menu item for the action.
 *
 * @action: an NactAction object.
 *
 * The returned string must be g_free by the caller.
 */
gchar *
nact_action_get_tooltip( const NactAction *action )
{
	g_assert( NACT_IS_ACTION( action ));

	gchar *tooltip;
	g_object_get( G_OBJECT( action ), PROP_TOOLTIP_STR, &tooltip, NULL );

	return( tooltip );
}

/**
 * Return the icon name attached to the context menu item for the
 * action.
 *
 * @action: an NactAction object.
 *
 * When not null, the returned string must be g_free by the caller.
 */
gchar *
nact_action_get_verified_icon_name( const NactAction *action )
{
	g_assert( NACT_IS_ACTION( action ));

	gchar *icon_name;
	g_object_get( G_OBJECT( action ), PROP_ICON_STR, &icon_name, NULL );

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
 * Returns the list of profiles of the actions as a GSList of
 * NactActionProfile GObjects.
 *
 * @action: the action whose profiles has to be retrieved.
 *
 * The returned pointer is owned by the @action object ; the caller
 * should not try to free or unref it.
 */
GSList *
nact_action_get_profiles( const NactAction *action )
{
	g_assert( NACT_IS_ACTION( action ));

	return( action->private->profiles );
}

/**
 * Set the list of the profiles for the action.
 */
void
nact_action_set_profiles( NactAction *action, GSList *list )
{
	g_assert( NACT_IS_ACTION( action ));

	free_profiles( action );
	action->private->profiles = list;
}

guint
nact_action_get_profiles_count( const NactAction *action )
{
	g_assert( NACT_IS_ACTION( action ));

	return( g_slist_length( action->private->profiles ));
}

GSList *
nact_action_get_profile_ids( const NactAction *action )
{
	GSList *item;
	GSList *profile_names = NULL;
	gchar *name;
	NactActionProfile *profile;

	g_assert( NACT_IS_ACTION( action ));

	for( item = action->private->profiles ; item != NULL ; item = item->next ){
		profile = ( NactActionProfile * ) item->data;
		g_object_get( G_OBJECT( profile ), "name", &name, NULL );
		profile_names = g_slist_prepend( profile_names, name );
	}

	return( profile_names );
}

void
nact_action_free_profile_ids( GSList *list )
{
	nactuti_free_string_list( list );
}

NactObject*
nact_action_get_profile( const NactAction *action, const gchar *profile_name )
{
	g_assert( NACT_IS_ACTION( action ));
	g_assert( profile_name && strlen( profile_name ));

	GSList *item;
	NactActionProfile *profile;
	NactActionProfile *found = NULL;
	gchar *name;

	for( item = action->private->profiles ; item != NULL && found == NULL ; item = item->next ){
		profile = NACT_ACTION_PROFILE( item->data );
		g_object_get( G_OBJECT( profile ), "name", &name, NULL );
		if( !g_strcmp0( name, profile_name )){
			found = profile;
		}
		g_free( name );
	}
	return( NACT_OBJECT( found ));
}
