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

#include <glib/gi18n.h>
#include <string.h>

#include "na-iduplicable.h"
#include "na-object-api.h"
#include "na-object-id-priv.h"

/* private class data
 */
struct NAObjectIdClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* object properties
 */
enum {
	NAOBJECT_ID_PROP_ID_ID = 1,
	NAOBJECT_ID_PROP_LABEL_ID
};

/* instance properties
 */
#define NAOBJECT_ID_PROP_ID				"na-object-id"
#define NAOBJECT_ID_PROP_LABEL			"na-object-label"

static NAObjectClass *st_parent_class = NULL;

static GType    register_type( void );
static void     class_init( NAObjectIdClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void     instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void     instance_dispose( GObject *object );
static void     instance_finalize( GObject *object );

static void     object_dump( const NAObject *object);
static void     object_copy( NAObject *target, const NAObject *source );
static gboolean object_are_equal( const NAObject *a, const NAObject *b );
static gboolean object_is_valid( const NAObject *object );

static gchar   *v_new_id( const NAObjectId *object, const NAObjectId *new_parent );
static gchar   *most_derived_new_id( const NAObjectId *object, const NAObjectId *new_parent );

GType
na_object_id_get_type( void )
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
	static const gchar *thisfn = "na_object_id_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NAObjectIdClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NAObjectId ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( NA_OBJECT_TYPE, "NAObjectId", &info, 0 );

	return( type );
}

static void
class_init( NAObjectIdClass *klass )
{
	static const gchar *thisfn = "na_object_id_class_init";
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
			NAOBJECT_ID_PROP_ID,
			"NAObjectId identifiant",
			"Internal identifiant of the NAObjectId object (ASCII, case insensitive)", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, NAOBJECT_ID_PROP_ID_ID, spec );

	spec = g_param_spec_string(
			NAOBJECT_ID_PROP_LABEL,
			"NAObjectId libelle",
			"Libelle of the NAObjectId object (UTF-8, localizable)", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, NAOBJECT_ID_PROP_LABEL_ID, spec );

	klass->private = g_new0( NAObjectIdClassPrivate, 1 );

	naobject_class = NA_OBJECT_CLASS( klass );
	naobject_class->dump = object_dump;
	naobject_class->new = NULL;
	naobject_class->copy = object_copy;
	naobject_class->are_equal = object_are_equal;
	naobject_class->is_valid = object_is_valid;
	naobject_class->get_childs = NULL;

	klass->new_id = NULL;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	/*static const gchar *thisfn = "na_object_id_instance_init";*/
	NAObjectId *self;

	/*g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );*/
	g_return_if_fail( NA_IS_OBJECT_ID( instance ));
	self = NA_OBJECT_ID( instance );

	self->private = g_new0( NAObjectIdPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	NAObjectId *self;

	g_return_if_fail( NA_IS_OBJECT_ID( object ));
	self = NA_OBJECT_ID( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case NAOBJECT_ID_PROP_ID_ID:
				g_value_set_string( value, self->private->id );
				break;

			case NAOBJECT_ID_PROP_LABEL_ID:
				g_value_set_string( value, self->private->label );
				break;

			default:
				G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
				break;
		}
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	NAObjectId *self;

	g_return_if_fail( NA_IS_OBJECT_ID( object ));
	self = NA_OBJECT_ID( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case NAOBJECT_ID_PROP_ID_ID:
				g_free( self->private->id );
				self->private->id = g_value_dup_string( value );
				break;

			case NAOBJECT_ID_PROP_LABEL_ID:
				g_free( self->private->label );
				self->private->label = g_value_dup_string( value );
				break;

			default:
				G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
				break;
		}
	}
}

static void
instance_dispose( GObject *object )
{
	/*static const gchar *thisfn = "na_object_id_instance_dispose";*/
	NAObjectId *self;

	/*g_debug( "%s: object=%p", thisfn, ( void * ) object );*/
	g_return_if_fail( NA_IS_OBJECT_ID( object ));
	self = NA_OBJECT_ID( object );

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
	NAObjectId *self;

	g_return_if_fail( NA_IS_OBJECT_ID( object ));
	self = NA_OBJECT_ID( object );

	g_free( self->private->id );
	g_free( self->private->label );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * na_object_id_get_id:
 * @object: the #NAObjectId object whose internal identifiant is
 * requested.
 *
 * Returns: the internal identifiant of @object as a new string.
 *
 * The returned string is an ASCII, case insensitive, string.
 * It should be g_free() by the caller.
 */
gchar *
na_object_id_get_id( const NAObjectId *object )
{
	gchar *id = NULL;

	g_return_val_if_fail( NA_IS_OBJECT_ID( object ), NULL );

	if( !object->private->dispose_has_run ){
		g_object_get( G_OBJECT( object ), NAOBJECT_ID_PROP_ID, &id, NULL );
	}

	return( id );
}

/**
 * na_object_id_get_label:
 * @object: the #NAObjectId object whose label is requested.
 *
 * Returns: the label of @object as a new string.
 *
 * The returned string is an UTF_8 localizable string.
 * It should be g_free() by the caller.
 */
gchar *
na_object_id_get_label( const NAObjectId *object )
{
	gchar *label = NULL;

	g_return_val_if_fail( NA_IS_OBJECT_ID( object ), NULL );

	if( !object->private->dispose_has_run ){
		g_object_get( G_OBJECT( object ), NAOBJECT_ID_PROP_LABEL, &label, NULL );
	}

	return( label );
}

/**
 * na_object_id_set_id:
 * @object: the #NAObjectId object whose internal identifiant is to be
 * set.
 * @id: internal identifiant to be set.
 *
 * Sets the internal identifiant of @object by taking a copy of the
 * provided one.
 */
void
na_object_id_set_id( NAObjectId *object, const gchar *id )
{
	g_return_if_fail( NA_IS_OBJECT_ID( object ));

	if( !object->private->dispose_has_run ){
		g_object_set( G_OBJECT( object ), NAOBJECT_ID_PROP_ID, id, NULL );
	}
}

/**
 * na_object_id_set_new_id:
 * @object: the #NAObjectId object whose internal identifiant is to be
 * set.
 * @new_parent: if @object is a #NAObjectProfile, then @new_parent
 * should be set to the #NAObjectActio new parent. Else, it would not
 * be possible to allocate a new profile id compatible with already
 * existing ones.
 *
 * Request a new id to the derived class, and set it.
 */
void
na_object_id_set_new_id( NAObjectId *object, const NAObjectId *new_parent )
{
	gchar *id;

	g_return_if_fail( NA_IS_OBJECT_ID( object ));
	g_return_if_fail( !new_parent || NA_IS_OBJECT_ID( new_parent ));

	if( !object->private->dispose_has_run ){

		id = v_new_id( object, new_parent );

		if( id ){
			g_object_set( G_OBJECT( object ), NAOBJECT_ID_PROP_ID, id, NULL );
			g_free( id );
		}
	}
}

/**
 * na_object_id_set_label:
 * @object: the #NAObjectId object whose label is to be set.
 * @label: label to be set.
 *
 * Sets the label of @object by taking a copy of the provided one.
 */
void
na_object_id_set_label( NAObjectId *object, const gchar *label )
{
	g_return_if_fail( NA_IS_OBJECT_ID( object ));

	if( !object->private->dispose_has_run ){
		g_object_set( G_OBJECT( object ), NAOBJECT_ID_PROP_LABEL, label, NULL );
	}
}

static void
object_dump( const NAObject *object )
{
	static const char *thisfn = "na_object_id_object_dump";

	g_return_if_fail( NA_IS_OBJECT_ID( object ));

	if( !NA_OBJECT_ID( object )->private->dispose_has_run ){

		g_debug( "%s:    id=%s", thisfn, NA_OBJECT_ID( object )->private->id );
		g_debug( "%s: label=%s", thisfn, NA_OBJECT_ID( object )->private->label );
	}
}

static void
object_copy( NAObject *target, const NAObject *source )
{
	gchar *id, *label;

	g_return_if_fail( NA_IS_OBJECT_ID( target ));
	g_return_if_fail( NA_IS_OBJECT_ID( source ));

	if( !NA_OBJECT_ID( target )->private->dispose_has_run &&
		!NA_OBJECT_ID( source )->private->dispose_has_run ){

			g_object_get( G_OBJECT( source ),
					NAOBJECT_ID_PROP_ID, &id,
					NAOBJECT_ID_PROP_LABEL, &label,
					NULL );

			g_object_set( G_OBJECT( target ),
					NAOBJECT_ID_PROP_ID, id,
					NAOBJECT_ID_PROP_LABEL, label,
					NULL );

			g_free( id );
			g_free( label );
	}
}

static gboolean
object_are_equal( const NAObject *a, const NAObject *b )
{
	gboolean equal = TRUE;

	g_return_val_if_fail( NA_IS_OBJECT_ID( a ), FALSE );
	g_return_val_if_fail( NA_IS_OBJECT_ID( b ), FALSE );

	if( !NA_OBJECT_ID( a )->private->dispose_has_run &&
		!NA_OBJECT_ID( b )->private->dispose_has_run ){

			if( equal ){
				if( g_ascii_strcasecmp( NA_OBJECT_ID( a )->private->id, NA_OBJECT_ID( b )->private->id )){
					/*g_debug( "a->id=%s, b->id=%s", NA_OBJECT_ID( a )->private->id, NA_OBJECT_ID( b )->private->id );*/
					equal = FALSE;
				}
			}

			if( equal ){
				if( g_utf8_collate( NA_OBJECT_ID( a )->private->label, NA_OBJECT_ID( b )->private->label )){
					/*g_debug( "a->label=%s, b->label=%s", NA_OBJECT_ID( a )->private->label, NA_OBJECT_ID( b )->private->label );*/
					equal = FALSE;
				}
			}

#if NA_IDUPLICABLE_EDITION_STATUS_DEBUG
			g_debug( "na_object_id_object_are_equal: a=%p (%s), b=%p (%s), are_equal=%s",
					( void * ) a, G_OBJECT_TYPE_NAME( a ),
					( void * ) b, G_OBJECT_TYPE_NAME( b ),
					equal ? "True":"False" );
#endif
	}

	return( equal );
}

/*
 * from NAObjectId point of view, a valid object requires an id
 * (not null, not empty)
 */
static gboolean
object_is_valid( const NAObject *object )
{
	gboolean valid = TRUE;

	g_return_val_if_fail( NA_IS_OBJECT_ID( object ), FALSE );

	if( !NA_OBJECT_ID( object )->private->dispose_has_run ){

		if( valid ){
			valid = ( NA_OBJECT_ID( object )->private->id && strlen( NA_OBJECT_ID( object )->private->id ));
		}
	}

	return( valid );
}

static gchar *
v_new_id( const NAObjectId *object, const NAObjectId *new_parent )
{
	return( most_derived_new_id( object, new_parent ));
}

static gchar *
most_derived_new_id( const NAObjectId *object, const NAObjectId *new_parent )
{
	gchar *new_id;
	GList *hierarchy, *ih;
	gboolean found;

	found = FALSE;
	new_id = NULL;
	hierarchy = g_list_reverse( na_object_get_hierarchy( NA_OBJECT( object )));
	/*g_debug( "na_object_id_most_derived_id: object=%p (%s)",
					( void * ) object, G_OBJECT_TYPE_NAME( object ));*/

	for( ih = hierarchy ; ih && !found ; ih = ih->next ){
		if( NA_OBJECT_ID_CLASS( ih->data )->new_id ){
			new_id = NA_OBJECT_ID_CLASS( ih->data )->new_id( object, new_parent );
			found = TRUE;
		}
		if( G_OBJECT_CLASS_TYPE( ih->data ) == NA_OBJECT_ID_TYPE ){
			break;
		}
	}

	na_object_free_hierarchy( hierarchy );

	return( new_id );
}
