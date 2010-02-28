/*
 * Nautilus ObjectActions
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

#include <api/na-iio-provider.h>
#include <api/na-object-api.h>

#include "na-factory-provider.h"
#include "na-factory-object.h"

/* private class data
 */
struct NAObjectActionClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NAObjectActionPrivate {
	gboolean dispose_has_run;
};

/* while iterating when searching for obsoleted boxed
 */
typedef struct {
	NAObjectProfile *profile;
	GList           *moved;
}
	IterForObsoletedParms;

/* i18n: default label for a new action */
#define NEW_NAUTILUS_ACTION				N_( "New Nautilus action" )

extern NADataGroup action_data_groups [];		/* defined in na-item-action-factory.c */

static NAObjectItemClass *st_parent_class = NULL;

static GType        register_type( void );
static void         class_init( NAObjectActionClass *klass );
static void         instance_init( GTypeInstance *instance, gpointer klass );
static void         instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void         instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void         instance_dispose( GObject *object );
static void         instance_finalize( GObject *object );

static void         object_copy( NAObject *target, const NAObject *source, gboolean recursive );
static gboolean     object_is_valid( const NAObject *object );

static void         ifactory_object_iface_init( NAIFactoryObjectInterface *iface );
static guint        ifactory_object_get_version( const NAIFactoryObject *instance );
static NADataGroup *ifactory_object_get_groups( const NAIFactoryObject *instance );
static gboolean     ifactory_object_are_equal( const NAIFactoryObject *a, const NAIFactoryObject *b );
static gboolean     ifactory_object_is_valid( const NAIFactoryObject *object );
static void         ifactory_object_read_done( NAIFactoryObject *instance, const NAIFactoryProvider *reader, void *reader_data, GSList **messages );
static guint        ifactory_object_write_start( NAIFactoryObject *instance, const NAIFactoryProvider *writer, void *writer_data, GSList **messages );
static guint        ifactory_object_write_done( NAIFactoryObject *instance, const NAIFactoryProvider *writer, void *writer_data, GSList **messages );

static gboolean     check_for_obsoleted_iter( const NAIFactoryObject *object, NADataBoxed *boxed, IterForObsoletedParms *parms );

static gboolean     object_object_is_valid( const NAObjectAction *action );
static gboolean     is_valid_label( const NAObjectAction *action );
static gboolean     is_valid_toolbar_label( const NAObjectAction *action );

GType
na_object_action_get_type( void )
{
	static GType action_type = 0;

	if( action_type == 0 ){

		action_type = register_type();
	}

	return( action_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "na_object_action_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NAObjectActionClass ),
		NULL,
		NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NAObjectAction ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	static const GInterfaceInfo ifactory_object_iface_info = {
		( GInterfaceInitFunc ) ifactory_object_iface_init,
		NULL,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( NA_OBJECT_ITEM_TYPE, "NAObjectAction", &info, 0 );

	g_type_add_interface_static( type, NA_IFACTORY_OBJECT_TYPE, &ifactory_object_iface_info );

	return( type );
}

static void
class_init( NAObjectActionClass *klass )
{
	static const gchar *thisfn = "na_object_action_class_init";
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
	naobject_class->copy = object_copy;
	naobject_class->are_equal = NULL;
	naobject_class->is_valid = object_is_valid;

	klass->private = g_new0( NAObjectActionClassPrivate, 1 );

	na_factory_object_define_properties( object_class, action_data_groups );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "na_object_action_instance_init";
	NAObjectAction *self;

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	g_return_if_fail( NA_IS_OBJECT_ACTION( instance ));

	self = NA_OBJECT_ACTION( instance );

	self->private = g_new0( NAObjectActionPrivate, 1 );
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	g_return_if_fail( NA_IS_OBJECT_ACTION( object ));
	g_return_if_fail( NA_IS_IFACTORY_OBJECT( object ));

	if( !NA_OBJECT_ACTION( object )->private->dispose_has_run ){

		na_factory_object_get_as_value( NA_IFACTORY_OBJECT( object ), g_quark_to_string( property_id ), value );
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	g_return_if_fail( NA_IS_OBJECT_ACTION( object ));
	g_return_if_fail( NA_IS_IFACTORY_OBJECT( object ));

	if( !NA_OBJECT_ACTION( object )->private->dispose_has_run ){

		na_factory_object_set_from_value( NA_IFACTORY_OBJECT( object ), g_quark_to_string( property_id ), value );
	}
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "na_object_action_instance_dispose";
	NAObjectAction *self;

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	g_return_if_fail( NA_IS_OBJECT_ACTION( object ));

	self = NA_OBJECT_ACTION( object );

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
	static const gchar *thisfn = "na_object_action_instance_finalize";
	NAObjectAction *self;

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	g_return_if_fail( NA_IS_OBJECT_ACTION( object ));

	self = NA_OBJECT_ACTION( object );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

static void
object_copy( NAObject *target, const NAObject *source, gboolean recursive )
{
	g_return_if_fail( NA_IS_OBJECT_ACTION( target ));
	g_return_if_fail( NA_IS_OBJECT_ACTION( source ));

	if( !NA_OBJECT_ACTION( target )->private->dispose_has_run &&
		!NA_OBJECT_ACTION( source )->private->dispose_has_run ){

		na_factory_object_copy( NA_IFACTORY_OBJECT( target ), NA_IFACTORY_OBJECT( source ));
	}
}

static gboolean
object_is_valid( const NAObject *object )
{
	g_return_val_if_fail( NA_IS_OBJECT_ACTION( object ), FALSE );

	return( object_object_is_valid( NA_OBJECT_ACTION( object )));
}

static void
ifactory_object_iface_init( NAIFactoryObjectInterface *iface )
{
	static const gchar *thisfn = "na_object_action_ifactory_object_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->get_version = ifactory_object_get_version;
	iface->get_groups = ifactory_object_get_groups;
	iface->copy = NULL;
	iface->are_equal = ifactory_object_are_equal;
	iface->is_valid = ifactory_object_is_valid;
	iface->read_start = NULL;
	iface->read_done = ifactory_object_read_done;
	iface->write_start = ifactory_object_write_start;
	iface->write_done = ifactory_object_write_done;
}

static guint
ifactory_object_get_version( const NAIFactoryObject *instance )
{
	return( 1 );
}

static NADataGroup *
ifactory_object_get_groups( const NAIFactoryObject *instance )
{
	return( action_data_groups );
}

static gboolean
ifactory_object_are_equal( const NAIFactoryObject *a, const NAIFactoryObject *b )
{
	return( na_object_item_are_equal( NA_OBJECT_ITEM( a ), NA_OBJECT_ITEM( b )));
}

static gboolean
ifactory_object_is_valid( const NAIFactoryObject *object )
{
	g_return_val_if_fail( NA_IS_OBJECT_ACTION( object ), FALSE );

	return( object_object_is_valid( NA_OBJECT_ACTION( object )));
}

static void
ifactory_object_read_done( NAIFactoryObject *instance, const NAIFactoryProvider *reader, void *reader_data, GSList **messages )
{
	IterForObsoletedParms parms;
	GList *ibox;

	g_debug( "na_object_action_ifactory_object_read_done: instance=%p", ( void * ) instance );

	/* do we have a pre-v2 action ?
	 *  i.e. an action without profile, with some data in its body
	 *  -> do we have read some obsoleted data which are now in the profile
	 */
	parms.profile = na_object_profile_new();
	parms.moved = NULL;

	na_factory_object_iter_on_boxed( instance, ( NAFactoryObjectIterBoxedFn ) check_for_obsoleted_iter, &parms );

	if( parms.moved ){
		na_object_set_id( parms.profile, "profile-pre-v2" );
		na_object_set_label( parms.profile, _( "Profile automatically created from pre-v2 action" ));
		na_object_attach_profile( instance, parms.profile );

		for( ibox = parms.moved ; ibox ; ibox = ibox->next ){
			na_factory_object_move_boxed( NA_IFACTORY_OBJECT( parms.profile ), instance, NA_DATA_BOXED( ibox->data ));
		}

	} else {
		g_object_unref( parms.profile );
	}

	na_factory_object_set_defaults( instance );
}

static guint
ifactory_object_write_start( NAIFactoryObject *instance, const NAIFactoryProvider *writer, void *writer_data, GSList **messages )
{
	na_object_item_factory_write_start( NA_OBJECT_ITEM( instance ));

	return( NA_IIO_PROVIDER_CODE_OK );
}

static guint
ifactory_object_write_done( NAIFactoryObject *instance, const NAIFactoryProvider *writer, void *writer_data, GSList **messages )
{
	return( NA_IIO_PROVIDER_CODE_OK );
}

static gboolean
check_for_obsoleted_iter( const NAIFactoryObject *object, NADataBoxed *boxed, IterForObsoletedParms *parms )
{
	NADataDef *action_def = na_data_boxed_get_data_def( boxed );

	if( action_def->obsoleted ){
		NADataDef *profile_def = na_factory_object_get_data_def( NA_IFACTORY_OBJECT( parms->profile ), action_def->name );

		if( profile_def && !profile_def->obsoleted ){
			g_debug( "na_object_action_check_for_obsoleted_iter: " \
					 "boxed=%p (%s) marked to be moved from action body to profile",
							 ( void * ) boxed, action_def->name );
			parms->moved =g_list_prepend( parms->moved, boxed );
		}
	}

	return( FALSE );
}

static gboolean
object_object_is_valid( const NAObjectAction *action )
{
	gboolean is_valid;
	GList *profiles, *ip;
	gint valid_profiles;

	is_valid = FALSE;

	if( !action->private->dispose_has_run ){

		is_valid = TRUE;

		if( is_valid ){
			if( na_object_is_target_toolbar( action )){
				is_valid = is_valid_toolbar_label( action );
			}
		}

		if( is_valid ){
			if( na_object_is_target_selection( action ) || na_object_is_target_background( action )){
				is_valid = is_valid_label( action );
			}
		}

		if( is_valid ){
			valid_profiles = 0;
			profiles = na_object_get_items( action );
			for( ip = profiles ; ip && !valid_profiles ; ip = ip->next ){
				if( na_object_is_valid( ip->data )){
					valid_profiles += 1;
				}
			}
			is_valid = ( valid_profiles > 0 );
			if( !is_valid ){
				na_object_debug_invalid( action, "no valid profile" );
			}
		}
	}

	return( is_valid );
}

static gboolean
is_valid_label( const NAObjectAction *action )
{
	gboolean is_valid;
	gchar *label;

	label = na_object_get_label( action );
	is_valid = ( label && g_utf8_strlen( label, -1 ) > 0 );
	g_free( label );

	if( !is_valid ){
		na_object_debug_invalid( action, "label" );
	}

	return( is_valid );
}

static gboolean
is_valid_toolbar_label( const NAObjectAction *action )
{
	gboolean is_valid;
	gchar *label;

	label = na_object_get_toolbar_label( action );
	is_valid = ( label && g_utf8_strlen( label, -1 ) > 0 );
	g_free( label );

	if( !is_valid ){
		na_object_debug_invalid( action, "toolbar-label" );
	}

	return( is_valid );
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

	return( action );
}

/**
 * na_object_action_new_with_profile:
 *
 * Allocates a new #NAObjectAction object along with a default profile.
 *
 * Returns: the newly allocated #NAObjectAction action.
 */
NAObjectAction *
na_object_action_new_with_profile( void )
{
	NAObjectAction *action;
	NAObjectProfile *profile;

	action = na_object_action_new();
	profile = na_object_profile_new();
	na_object_action_attach_profile( action, profile );

	return( action );
}

/**
 * na_object_action_new_with_defaults:
 *
 * Allocates a new #NAObjectAction object along with a default profile.
 * These two objects have suitable default values.
 *
 * Returns: the newly allocated #NAObjectAction action.
 */
NAObjectAction *
na_object_action_new_with_defaults( void )
{
	NAObjectAction *action;
	NAObjectProfile *profile;

	action = na_object_action_new();
	na_object_set_new_id( action, NULL );
	na_object_set_label( action, NEW_NAUTILUS_ACTION );
	na_object_set_toolbar_label( action, NEW_NAUTILUS_ACTION );
	na_factory_object_set_defaults( NA_IFACTORY_OBJECT( action ));

	profile = na_object_profile_new_with_defaults();
	na_object_action_attach_profile( action, profile );

	return( action );
}

/**
 * na_object_action_get_new_profile_name:
 * @action: the #NAObjectAction object which will receive a new profile.
 *
 * Returns a name suitable as a new profile name.
 *
 * The search is made by iterating over the standard profile name
 * prefix : basically, we increment a counter until finding a name
 * which is not yet allocated. The provided name is so only suitable
 * for the specified @action.
 *
 * Returns: a newly allocated profile name, which should be g_free() by
 * the caller.
 *
 * When inserting a list of profiles in the action, we iter first for
 * new names, before actually do the insertion. We so keep the last
 * allocated name to avoid to allocate the same one twice.
 */
gchar *
na_object_action_get_new_profile_name( const NAObjectAction *action )
{
	int i;
	gboolean ok = FALSE;
	gchar *candidate = NULL;
	guint last_allocated;

	g_return_val_if_fail( NA_IS_OBJECT_ACTION( action ), NULL );

	if( !action->private->dispose_has_run ){

		last_allocated = na_object_get_last_allocated( action );

		for( i = last_allocated + 1 ; !ok ; ++i ){
			g_free( candidate );
			candidate = g_strdup_printf( "profile-%d", i );

			if( !na_object_get_item( action, candidate )){
				ok = TRUE;
				na_object_set_last_allocated( action, i );
			}
		}

		if( !ok ){
			g_free( candidate );
			candidate = NULL;
		}
	}

	/*g_debug( "returning candidate=%s", candidate );*/
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
	g_return_if_fail( NA_IS_OBJECT_PROFILE( profile ));

	if( !action->private->dispose_has_run ){

		na_object_append_item( action, profile );
		na_object_set_parent( profile, action );
	}
}

/**
 * na_object_action_is_candidate:
 * @action: the #NAObjectAction to be tested.
 * @target: the current target.
 *
 * Returns: %TRUE if the @action may be candidate for this @target.
 */
gboolean
na_object_action_is_candidate( const NAObjectAction *action, gint target )
{
	gboolean is_candidate = FALSE;

#if 0
	g_return_val_if_fail( NA_IS_OBJECT_ACTION( action ), is_candidate );

	if( !action->private->dispose_has_run ){

		is_candidate =
			( action->private->target_selection && target == ITEM_TARGET_SELECTION ) ||
			( action->private->target_background && target == ITEM_TARGET_BACKGROUND ) ||
			( action->private->target_toolbar && target == ITEM_TARGET_TOOLBAR );
	}
#endif

	return( is_candidate );
}
