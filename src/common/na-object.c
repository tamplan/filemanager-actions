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

#include "na-object.h"
#include "na-iduplicable.h"

/* private class data
 */
struct NAObjectClassPrivate {
};

/* private instance data
 */
struct NAObjectPrivate {
	gboolean  dispose_has_run;
	gchar    *id;
	gchar    *label;
};

/* instance properties
 */
#define PROP_NAOBJECT_ID_STR			"na-object-id"
#define PROP_NAOBJECT_LABEL_STR			"na-object-label"

static GObjectClass *st_parent_class = NULL;

static GType     register_type( void );
static void      class_init( NAObjectClass *klass );
static void      iduplicable_iface_init( NAIDuplicableInterface *iface );
static void      instance_init( GTypeInstance *instance, gpointer klass );
static void      instance_constructed( GObject *object );
static void      instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void      instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void      instance_dispose( GObject *object );
static void      instance_finalize( GObject *object );

static NAObject *v_duplicate( const NAObject *object );
static void      v_copy( NAObject *target, const NAObject *source );
static gboolean  v_are_equal( const NAObject *a, const NAObject *b );
static gboolean  v_is_valid( const NAObject *object );

static void      do_dump( const NAObject *object );
static void      do_copy( NAObject *target, const NAObject *source );
static gboolean  do_are_equal( const NAObject *a, const NAObject *b );
static gboolean  do_is_valid( const NAObject *object );

static NAObject *iduplicable_duplicate( const NAObject *object );
static gboolean  iduplicable_are_equal( const NAObject *a, const NAObject *b );
static gboolean  iduplicable_is_valid( const NAObject *object );

GType
na_object_get_type( void )
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
	static const gchar *thisfn = "na_object_register_type";
	g_debug( "%s", thisfn );

	static GTypeInfo info = {
		sizeof( NAObjectClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NAObject ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	GType type = g_type_register_static( G_TYPE_OBJECT, "NAObject", &info, 0 );

	/* implements IDuplicable interface
	 */
	static const GInterfaceInfo idupicable_iface_info = {
		( GInterfaceInitFunc ) iduplicable_iface_init,
		NULL,
		NULL
	};

	g_type_add_interface_static( type, NA_IDUPLICABLE_TYPE, &idupicable_iface_info );

	return( type );
}

static void
class_init( NAObjectClass *klass )
{
	static const gchar *thisfn = "na_object_class_init";
	g_debug( "%s: klass=%p", thisfn, klass );

	st_parent_class = g_type_class_peek_parent( klass );

	GObjectClass *object_class = G_OBJECT_CLASS( klass );
	object_class->constructed = instance_constructed;
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
	object_class->set_property = instance_set_property;
	object_class->get_property = instance_get_property;

	GParamSpec *spec;
	spec = g_param_spec_string(
			PROP_NAOBJECT_ID_STR,
			"NAObject identifiant",
			"Internal identifiant of the NAObject object (ASCII, case insensitive)", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_NAOBJECT_ID, spec );

	spec = g_param_spec_string(
			PROP_NAOBJECT_LABEL_STR,
			"NAObject libelle",
			"Libelle of the NAObject object (UTF-8, localizable)", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_NAOBJECT_LABEL, spec );

	klass->private = g_new0( NAObjectClassPrivate, 1 );

	klass->dump = do_dump;
	klass->duplicate = NULL;
	klass->copy = do_copy;
	klass->are_equal = do_are_equal;
	klass->is_valid = do_is_valid;
}

static void
iduplicable_iface_init( NAIDuplicableInterface *iface )
{
	static const gchar *thisfn = "na_object_iduplicable_iface_init";
	g_debug( "%s: iface=%p", thisfn, iface );

	iface->duplicate = iduplicable_duplicate;
	iface->are_equal = iduplicable_are_equal;
	iface->is_valid = iduplicable_is_valid;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	/*static const gchar *thisfn = "na_object_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );*/

	g_assert( NA_IS_OBJECT( instance ));
	NAObject *self = NA_OBJECT( instance );

	self->private = g_new0( NAObjectPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_constructed( GObject *object )
{
	na_iduplicable_init( NA_OBJECT( object ));

	/* chain call to parent class */
	if( st_parent_class->constructed ){
		G_OBJECT_CLASS( st_parent_class )->constructed( object );
	}
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	g_assert( NA_IS_OBJECT( object ));
	NAObject *self = NA_OBJECT( object );

	switch( property_id ){
		case PROP_NAOBJECT_ID:
			g_value_set_string( value, self->private->id );
			break;

		case PROP_NAOBJECT_LABEL:
			g_value_set_string( value, self->private->label );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	g_assert( NA_IS_OBJECT( object ));
	NAObject *self = NA_OBJECT( object );

	switch( property_id ){
		case PROP_NAOBJECT_ID:
			g_free( self->private->id );
			self->private->id = g_value_dup_string( value );
			break;

		case PROP_NAOBJECT_LABEL:
			g_free( self->private->label );
			self->private->label = g_value_dup_string( value );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_dispose( GObject *object )
{
	g_assert( NA_IS_OBJECT( object ));
	NAObject *self = NA_OBJECT( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		G_OBJECT_CLASS( st_parent_class )->dispose( object );
	}
}

static void
instance_finalize( GObject *object )
{
	g_assert( NA_IS_OBJECT( object ));
	NAObject *self = ( NAObject * ) object;

	g_free( self->private->id );
	g_free( self->private->label );

	g_free( self->private );

	/* chain call to parent class */
	if( st_parent_class->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * na_object_dump:
 * @object: the #NAObject object to be dumped.
 *
 * Dumps via g_debug the content of the object.
 */
void
na_object_dump( const NAObject *object )
{
	if( object ){
		g_assert( NA_IS_OBJECT( object ));

		NA_OBJECT_GET_CLASS( object )->dump( object );
	}
}

/**
 * na_object_duplicate:
 * @object: the #NAObject object to be dumped.
 *
 * Exactly duplicates a #NAObject-derived object.
 *
 * Returns: the new #NAObject.
 */
NAObject *
na_object_duplicate( const NAObject *object )
{
	g_assert( NA_IS_OBJECT( object ));

	return( na_iduplicable_duplicate( object ));
}

/**
 * na_object_copy:
 * @target: the #NAObject-derived object which will receive data.
 * @source: the #NAObject-derived object which will provide data.
 *
 * Copies data and properties from @source to @target.
 */
void
na_object_copy( NAObject *target, const NAObject *source )
{
	g_assert( NA_IS_OBJECT( target ));
	g_assert( NA_IS_OBJECT( source ));

	v_copy( target, source );
}

/**
 * na_object_check_edited_status:
 * @object: the #NAObject object to be checked.
 *
 * Checks for the edition status of @object.
 *
 * Internally set some properties which may be requested later. This
 * two-steps check-request let us optimize some work in the UI.
 */
void
na_object_check_edited_status( const NAObject *object )
{
	g_assert( NA_IS_OBJECT( object ));

	na_iduplicable_check_edited_status( object );
}

/**
 * na_object_are_equal:
 * @a: a first #NAObject object.
 * @b: a second #NAObject object to be compared to the first one.
 *
 * Compares the two #NAObject objects.
 *
 * At least when it finds that @a and @b are equal, each derived
 * class should call its parent class to give it an opportunity to
 * detect a difference.
 *
 * Returns: %TRUE if @a and @b are identical, %FALSE else.
 */
gboolean
na_object_are_equal( const NAObject *a, const NAObject *b )
{
	g_assert( NA_IS_OBJECT( a ));
	g_assert( NA_IS_OBJECT( b ));

	return( v_are_equal( a, b ));
}

/**
 * na_object_get_is_valid:
 * @object: the #NAObject object whose validity is to be checked.
 *
 * Checks for the validity of @object.
 *
 * Returns: %TRUE is @object is valid, %FALSE else.
 */
gboolean
na_object_is_valid( const NAObject *object )
{
	g_assert( NA_IS_OBJECT( object ));

	return( v_is_valid( object ));
}

/**
 * na_object_get_origin:
 * @object: the #NAObject object whose status is requested.
 *
 * Returns the original object which was at the origin of @object.
 *
 * Returns: a #NAObject, or NULL.
 */
NAObject *
na_object_get_origin( const NAObject *object )
{
	g_assert( NA_IS_OBJECT( object ));

	return( na_iduplicable_get_origin( object ));
}

/**
 * na_object_get_modified_status:
 * @object: the #NAObject object whose status is requested.
 *
 * Returns the current modification status of @object.
 *
 * This suppose that @object has been previously duplicated in order
 * to get benefits provided by the IDuplicable interface.
 *
 * This suppose also that the edition status of @object has previously
 * been checked via na_object_check_edited_status().
 *
 * Returns: %TRUE is the provided object has been modified regarding to
 * the original one, %FALSE else.
 */
gboolean
na_object_get_modified_status( const NAObject *object )
{
	g_assert( NA_IS_OBJECT( object ));

	return( na_iduplicable_is_modified( object ));
}

/**
 * na_object_get_valid_status:
 * @object: the #NAObject object whose status is requested.
 *
 * Returns the current validity status of @object.
 *
 * This suppose that @object has been previously duplicated in order
 * to get benefits provided by the IDuplicable interface.
 *
 * This suppose also that the edition status of @object has previously
 * been checked via na_object_check_edited_status().
 *
 * Returns: %TRUE is the provided object is valid, %FALSE else.
 */
gboolean
na_object_get_valid_status( const NAObject *object )
{
	g_assert( NA_IS_OBJECT( object ));

	return( na_iduplicable_is_valid( object ));
}

/**
 * na_object_set_origin:
 * @object: the #NAObject object whose status is requested.
 * @origin: a #NAObject which will be set as the new origin of @object.
 *
 * Sets the new origin of @object.
 */
void
na_object_set_origin( NAObject *object, const NAObject *origin )
{
	g_assert( NA_IS_OBJECT( object ));

	na_iduplicable_set_origin( object, origin );
}

/**
 * na_object_get_id:
 * @object: the #NAObject object whose internal identifiant is
 * requested.
 *
 * Returns the internal identifiant of @object.
 *
 * Returns: the internal identifiant of @object as a new string. The
 * returned string is an ASCII, case insensitive, string. It should be
 * g_free() by the caller.
 */
gchar *
na_object_get_id( const NAObject *object )
{
	g_assert( NA_IS_OBJECT( object ));
	gchar *id;
	g_object_get( G_OBJECT( object ), PROP_NAOBJECT_ID_STR, &id, NULL );
	return( id );
}

/**
 * na_object_get_label:
 * @object: the #NAObject object whose label is requested.
 *
 * Returns the label of @object.
 *
 * Returns: the label of @object as a new string. The returned string
 * is an UTF_8 string. It should be g_free() by the caller.
 */
gchar *
na_object_get_label( const NAObject *object )
{
	g_assert( NA_IS_OBJECT( object ));
	gchar *label;
	g_object_get( G_OBJECT( object ), PROP_NAOBJECT_LABEL_STR, &label, NULL );
	return( label );
}

/**
 * na_object_set_id:
 * @object: the #NAObject object whose internal identifiant is to be
 * set.
 * @id: internal identifiant to be set.
 *
 * Sets the internal identifiant of @object by taking a copy of the
 * provided one.
 */
void
na_object_set_id( NAObject *object, const gchar *id )
{
	g_assert( NA_IS_OBJECT( object ));
	g_object_set( G_OBJECT( object ), PROP_NAOBJECT_ID_STR, id, NULL );
}

/**
 * na_object_set_label:
 * @object: the #NAObject object whose label is to be set.
 * @label: label to be set.
 *
 * Sets the label of @object by taking a copy of the provided one.
 */
void
na_object_set_label( NAObject *object, const gchar *label )
{
	g_assert( NA_IS_OBJECT( object ));
	g_object_set( G_OBJECT( object ), PROP_NAOBJECT_LABEL_STR, label, NULL );
}

static NAObject *
v_duplicate( const NAObject *object )
{
	if( NA_OBJECT_GET_CLASS( object )->duplicate ){
		return( NA_OBJECT_GET_CLASS( object )->duplicate( object ));
	}

	return( NULL );
}

static void
v_copy( NAObject *target, const NAObject *source )
{
	if( NA_OBJECT_GET_CLASS( target )->copy ){
		NA_OBJECT_GET_CLASS( target )->copy( target, source );
	}
}

static gboolean
v_are_equal( const NAObject *a, const NAObject *b )
{
	if( NA_OBJECT_GET_CLASS( a )->are_equal ){
		return( NA_OBJECT_GET_CLASS( a )->are_equal( a, b ));
	}

	return( FALSE );
}

static gboolean
v_is_valid( const NAObject *object )
{
	if( NA_OBJECT_GET_CLASS( object )->is_valid ){
		return( NA_OBJECT_GET_CLASS( object )->is_valid( object ));
	}

	return( TRUE );
}

static void
do_dump( const NAObject *object )
{
	static const char *thisfn = "na_object_do_dump";

	g_assert( NA_IS_OBJECT( object ));

	g_debug( "%s: object=%p", thisfn, object );
	g_debug( "%s:     id=%s", thisfn, object->private->id );
	g_debug( "%s:  label=%s", thisfn, object->private->label );

	na_iduplicable_dump( object );
}

static void
do_copy( NAObject *target, const NAObject *source )
{
	gchar *id = na_object_get_id( source );
	na_object_set_id( target, id );
	g_free( id );

	gchar *label = na_object_get_label( source );
	na_object_set_label( target, label );
	g_free( label );
}

static gboolean
do_are_equal( const NAObject *a, const NAObject *b )
{
	if( g_ascii_strcasecmp( a->private->id, b->private->id )){
		return( FALSE );
	}
	if( g_utf8_collate( a->private->label, b->private->label )){
		return( FALSE );
	}
	return( TRUE );
}

static gboolean
do_is_valid( const NAObject *object )
{
	return( object->private->id && strlen( object->private->id ));
}

static NAObject *
iduplicable_duplicate( const NAObject *object )
{
	return( v_duplicate( object ));
}

static gboolean
iduplicable_are_equal( const NAObject *a, const NAObject *b )
{
	return( v_are_equal( a, b ));
}

static gboolean
iduplicable_is_valid( const NAObject *object )
{
	return( v_is_valid( object ));
}
