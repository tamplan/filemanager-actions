/*
 * Nautilus Actions
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

#include <stdlib.h>
#include <string.h>

#include <api/na-core-utils.h>
#include <api/na-idata-factory-str.h>

#include "na-data-element.h"

/* private class data
 */
struct NADataElementClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NADataElementPrivate {
	gboolean dispose_has_run;
	guint    type;
	union {
		gboolean  boolean;
		gchar    *string;
		GSList   *slist;
		void     *pointer;
		guint     uint;
	} u;
};

static GObjectClass *st_parent_class   = NULL;

static GType register_type( void );
static void  class_init( NADataElementClass *klass );
static void  instance_init( GTypeInstance *instance, gpointer klass );
static void  instance_dispose( GObject *object );
static void  instance_finalize( GObject *object );

GType
na_data_element_get_type( void )
{
	static GType item_type = 0;

	if( item_type == 0 ){
		item_type = register_type();
	}

	return( item_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "na_data_element_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NADataElementClass ),
		NULL,
		NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NADataElement ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "NADataElement", &info, 0 );

	return( type );
}

static void
class_init( NADataElementClass *klass )
{
	static const gchar *thisfn = "na_data_element_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NADataElementClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "na_data_element_instance_init";
	NADataElement *self;

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	g_return_if_fail( NA_IS_DATA_ELEMENT( instance ));

	self = NA_DATA_ELEMENT( instance );

	self->private = g_new0( NADataElementPrivate, 1 );
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "na_data_element_instance_dispose";
	NADataElement *self;

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	g_return_if_fail( NA_IS_DATA_ELEMENT( object ));

	self = NA_DATA_ELEMENT( object );

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
	NADataElement *self;

	g_return_if_fail( NA_IS_DATA_ELEMENT( object ));

	self = NA_DATA_ELEMENT( object );

	switch( self->private->type ){

		case NADF_TYPE_STRING:
		case NADF_TYPE_LOCALE_STRING:
			g_free( self->private->u.string );
			break;

		case NADF_TYPE_STRING_LIST:
			na_core_utils_slist_free( self->private->u.slist );
			break;

		case NADF_TYPE_BOOLEAN:
		case NADF_TYPE_POINTER:
			break;
	}

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * na_data_element_new:
 * @type: a #NAIDataFactory standard type.
 *
 * Returns: a newly allocated #NADataElement.
 */
NADataElement *
na_data_element_new( guint type )
{
	NADataElement *element;

	element = g_object_new( NA_DATA_ELEMENT_TYPE, NULL );

	element->private->type = type;

	return( element );
}

/**
 * na_data_element_dump:
 * @element: this #NADataElement object.
 * @name: the name attributed to this element.
 *
 * Dump the content of @element.
 */
void
na_data_element_dump( const NADataElement *element, const gchar *name )
{
	static const gchar *thisfn = "na_data_element_dump";

	switch( element->private->type ){

		case NADF_TYPE_STRING:
		case NADF_TYPE_LOCALE_STRING:
			g_debug( "%s: %s=%s", thisfn, name, element->private->u.string );
			break;

		case NADF_TYPE_STRING_LIST:
			g_debug( "%s: %s:", thisfn, name );
			na_core_utils_slist_dump( element->private->u.slist );
			break;

		case NADF_TYPE_BOOLEAN:
			g_debug( "%s: %s=%s", thisfn, name, element->private->u.boolean ? "True":"False" );
			break;

		case NADF_TYPE_POINTER:
			g_debug( "%s: %s=%p", thisfn, name, ( void * ) element->private->u.pointer );
			break;

		case NADF_TYPE_UINT:
			g_debug( "%s: %s=%d", thisfn, name, element->private->u.uint );
			break;

		default:
			g_warning( "%s: unmanaged type=%d", thisfn, element->private->type );

	}
}

/**
 * na_data_element_set:
 * @element: the #NADataElement whose value is to be set.
 * @value: the source #NADataElement.
 *
 * Copy value from @value to @element.
 */
void
na_data_element_set( NADataElement *element, const NADataElement *value )
{
	static const gchar *thisfn = "na_data_element_set";

	g_return_if_fail( NA_IS_DATA_ELEMENT( element ));
	g_return_if_fail( NA_IS_DATA_ELEMENT( value ));
	g_return_if_fail( element->private->type != value->private->type );

	if( !element->private->dispose_has_run ){

		switch( element->private->type ){

			case NADF_TYPE_STRING:
			case NADF_TYPE_LOCALE_STRING:
				g_free( element->private->u.string );
				element->private->u.string = g_strdup( value->private->u.string );
				break;

			case NADF_TYPE_STRING_LIST:
				na_core_utils_slist_free( element->private->u.slist );
				element->private->u.slist = na_core_utils_slist_duplicate( value->private->u.slist );
				break;

			case NADF_TYPE_BOOLEAN:
				element->private->u.boolean = value->private->u.boolean;
				break;

			case NADF_TYPE_POINTER:
				element->private->u.pointer = value->private->u.pointer;
				break;

			case NADF_TYPE_UINT:
				element->private->u.uint = value->private->u.uint;
				break;

			default:
				g_warning( "%s: unmanaged type=%d", thisfn, element->private->type );
		}
	}
}

#if 0
/**
 * na_data_element_set_from_boolean:
 * @element: the #NADataElement whose value is to be set.
 * @value: the boolean to be set.
 *
 * Set the boolean, if @element is of type NADF_TYPE_BOOLEAN.
 */
void
na_data_element_set_from_boolean( NADataElement *element, gboolean value )
{
	static const gchar *thisfn = "na_data_element_set_from_boolean";

	g_return_if_fail( NA_IS_DATA_ELEMENT( element ));

	if( !element->private->dispose_has_run ){

		if( element->private->type == NADF_TYPE_BOOLEAN ){
			element->private->u.boolean = value;

		} else {
			g_warning( "%s: element is of type %d", thisfn, element->private->type );
		}
	}
}
#endif

/**
 * na_data_element_set_from_string:
 * @element: the #NADataElement whose value is to be set.
 * @value: the string to be set.
 *
 * Evaluates the @value and set it to the @element.
 */
void
na_data_element_set_from_string( NADataElement *element, const gchar *value )
{
	static const gchar *thisfn = "na_data_element_set_from_string";

	g_return_if_fail( NA_IS_DATA_ELEMENT( element ));

	if( !element->private->dispose_has_run ){

		switch( element->private->type ){

			case NADF_TYPE_STRING:
			case NADF_TYPE_LOCALE_STRING:
				g_free( element->private->u.string );
				element->private->u.string = g_strdup( value );
				break;

			case NADF_TYPE_STRING_LIST:
				na_core_utils_slist_free( element->private->u.slist );
				element->private->u.slist = g_slist_append( NULL, g_strdup( value ));
				break;

			case NADF_TYPE_BOOLEAN:
				element->private->u.boolean = na_core_utils_boolean_from_string( value );
				break;

			/* only a NULL value may be relevant here
			 */
			case NADF_TYPE_POINTER:
				element->private->u.pointer = NULL;
				break;

			case NADF_TYPE_UINT:
				element->private->u.uint = atoi( value );
				break;

			default:
				g_warning( "%s: unmanaged type=%d", thisfn, element->private->type );
		}
	}
}

#if 0
/**
 * na_data_element_set_from_slist:
 * @element: the #NADataElement whose value is to be set.
 * @value: the string list to be set.
 *
 * Set the string list, if @element is of type NADF_TYPE_STRING_LIST.
 */
void
na_data_element_set_from_slist( NADataElement *element, GSList *value )
{
	static const gchar *thisfn = "na_data_element_set_from_slist";

	g_return_if_fail( NA_IS_DATA_ELEMENT( element ));

	if( !element->private->dispose_has_run ){

		if( element->private->type == NADF_TYPE_STRING_LIST ){
			na_core_utils_slist_free( element->private->u.slist );
			element->private->u.slist = na_core_utils_slist_duplicate( value );

		} else {
			g_warning( "%s: element is of type=%d", thisfn, element->private->type );
		}
	}
}
#endif

/**
 * na_data_element_set_from_value:
 * @element: the #NADataElement whose value is to be set.
 * @value: the value whose content is to be got.
 *
 * Evaluates the @value and set it to the @element.
 */
void
na_data_element_set_from_value( NADataElement *element, const GValue *value )
{
	static const gchar *thisfn = "na_data_element_set_from_value";

	g_return_if_fail( NA_IS_DATA_ELEMENT( element ));

	if( !element->private->dispose_has_run ){

		switch( element->private->type ){

			case NADF_TYPE_STRING:
			case NADF_TYPE_LOCALE_STRING:
				g_free( element->private->u.string );
				element->private->u.string = g_value_dup_string( value );
				break;

			case NADF_TYPE_STRING_LIST:
				na_core_utils_slist_free( element->private->u.slist );
				element->private->u.slist = na_core_utils_slist_duplicate( g_value_get_pointer( value ));
				break;

			case NADF_TYPE_BOOLEAN:
				element->private->u.boolean = g_value_get_boolean( value );
				break;

			case NADF_TYPE_POINTER:
				element->private->u.pointer = g_value_get_pointer( value );
				break;

			case NADF_TYPE_UINT:
				element->private->u.uint = g_value_get_uint( value );
				break;

			default:
				g_warning( "%s: unmanaged type=%d", thisfn, element->private->type );
		}
	}
}

/**
 * na_data_element_set_from_void:
 * @element: the #NADataElement whose value is to be set.
 * @value: the value whose content is to be got.
 *
 * Evaluates the @value and set it to the @element.
 */
void
na_data_element_set_from_void( NADataElement *element, const void *value )
{
	static const gchar *thisfn = "na_data_element_set_from_void";

	g_return_if_fail( NA_IS_DATA_ELEMENT( element ));

	if( !element->private->dispose_has_run ){

		switch( element->private->type ){

			case NADF_TYPE_STRING:
			case NADF_TYPE_LOCALE_STRING:
				g_free( element->private->u.string );
				element->private->u.string = g_strdup(( const gchar * ) value );
				break;

			case NADF_TYPE_STRING_LIST:
				na_core_utils_slist_free( element->private->u.slist );
				element->private->u.slist = na_core_utils_slist_duplicate(( GSList * ) value );
				break;

			case NADF_TYPE_BOOLEAN:
				element->private->u.boolean = GPOINTER_TO_UINT( value );
				break;

			case NADF_TYPE_POINTER:
				element->private->u.pointer = ( void * ) value;
				break;

			case NADF_TYPE_UINT:
				element->private->u.uint = GPOINTER_TO_UINT( value );
				break;

			default:
				g_warning( "%s: unmanaged type=%d", thisfn, element->private->type );
		}
	}
}

/**
 * na_data_element_get:
 * @element: the #NADataElement whose value is to be set.
 *
 * Returns: the content of the @element.
 *
 * If of type NADF_TYPE_STRING, NADF_TYPE_LOCALE_STRING OR
 * NADF_TYPE_STRING_LIST, then the content is returned in a newly
 * allocated value, which should be released by the caller.
 */
void *
na_data_element_get( const NADataElement *element )
{
	static const gchar *thisfn = "na_data_element_set_to_value";
	void *value;

	g_return_val_if_fail( NA_IS_DATA_ELEMENT( element ), NULL );

	value = NULL;

	if( !element->private->dispose_has_run ){

		switch( element->private->type ){

			case NADF_TYPE_STRING:
			case NADF_TYPE_LOCALE_STRING:
				value = g_strdup( element->private->u.string );
				break;

			case NADF_TYPE_STRING_LIST:
				value = na_core_utils_slist_duplicate( element->private->u.slist );
				break;

			case NADF_TYPE_BOOLEAN:
				value = GUINT_TO_POINTER( element->private->u.boolean );
				break;

			case NADF_TYPE_POINTER:
				value = element->private->u.pointer;
				break;

			case NADF_TYPE_UINT:
				value = GUINT_TO_POINTER( element->private->u.uint );
				break;

			default:
				g_warning( "%s: unmanaged type=%d", thisfn, element->private->type );
		}
	}

	return( value );
}

/**
 * na_data_element_set_from_value:
 * @element: the #NADataElement whose value is to be set.
 * @value: the string to be set.
 *
 * Setup @value with the content of the @element.
 */
void
na_data_element_set_to_value( const NADataElement *element, GValue *value )
{
	static const gchar *thisfn = "na_data_element_set_to_value";

	g_return_if_fail( NA_IS_DATA_ELEMENT( element ));

	if( !element->private->dispose_has_run ){

		switch( element->private->type ){

			case NADF_TYPE_STRING:
			case NADF_TYPE_LOCALE_STRING:
				g_value_set_string( value, element->private->u.string );
				break;

			case NADF_TYPE_STRING_LIST:
				g_value_set_pointer( value, na_core_utils_slist_duplicate( element->private->u.slist ));
				break;

			case NADF_TYPE_BOOLEAN:
				g_value_set_boolean( value, element->private->u.boolean );
				break;

			case NADF_TYPE_POINTER:
				g_value_set_pointer( value, element->private->u.pointer );
				break;

			case NADF_TYPE_UINT:
				g_value_set_uint( value, element->private->u.uint );
				break;

			default:
				g_warning( "%s: unmanaged type=%d", thisfn, element->private->type );
		}
	}
}

/**
 * na_data_element_are_equal:
 * @a: the first #NADataElement object.
 * @b: the second #NADataElement object.
 *
 * Returns: %TRUE if the two elements are equal, %FALSE else.
 */
gboolean
na_data_element_are_equal( const NADataElement *a, const NADataElement *b )
{
	static const gchar *thisfn = "na_data_element_are_equal";
	gboolean are_equal;

	g_return_val_if_fail( NA_IS_DATA_ELEMENT( a ), FALSE );
	g_return_val_if_fail( NA_IS_DATA_ELEMENT( b ), FALSE );

	are_equal = FALSE;

	if( !a->private->dispose_has_run &&
		!b->private->dispose_has_run ){

		if( a->private->type == b->private->type ){

			are_equal = TRUE;

			switch( a->private->type ){

				case NADF_TYPE_STRING:
					are_equal = ( strcmp( a->private->u.string, b->private->u.string ) == 0 );
					break;

				case NADF_TYPE_LOCALE_STRING:
					are_equal = ( g_utf8_collate( a->private->u.string, b->private->u.string ) == 0 );
					break;

				case NADF_TYPE_STRING_LIST:
					are_equal = na_core_utils_slist_are_equal( a->private->u.slist, b->private->u.slist );
					break;

				case NADF_TYPE_BOOLEAN:
					are_equal = ( a->private->u.boolean == b->private->u.boolean );
					break;

				case NADF_TYPE_POINTER:
					are_equal = ( a->private->u.pointer == b->private->u.pointer );
					break;

				case NADF_TYPE_UINT:
					are_equal = ( a->private->u.uint == b->private->u.uint );
					break;

				default:
					g_warning( "%s: unmanaged type=%d", thisfn, a->private->type );
					are_equal = FALSE;
			}
		}
	}

	return( are_equal );
}
