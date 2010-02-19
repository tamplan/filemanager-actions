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
	gboolean      dispose_has_run;
	NadfIdType   *iddef ;
	union {
		gboolean  boolean;
		gchar    *string;
		GSList   *slist;
		void     *pointer;
		guint     uint;
	} u;
};

typedef struct {
	guint           type;
	GParamSpec * ( *spec )            ( const NadfIdType * );
	void         ( *free )            ( const NADataElement * );
	void         ( *dump )            ( const NADataElement * );
	gboolean     ( *are_equal )       ( const NADataElement *, const NADataElement * );
	gboolean     ( *is_valid )        ( const NADataElement * );
	void *       ( *get )             ( const NADataElement * );
	void         ( *get_via_value )   ( const NADataElement *, GValue *value );
	void         ( *set_from_element )( NADataElement *, const NADataElement * );
	void         ( *set_from_string ) ( NADataElement *, const gchar *string );
	void         ( *set_from_value )  ( NADataElement *, const GValue *value );
	void         ( *set_from_void )   ( NADataElement *, const void *value );
}
	DataElementFn;

static GObjectClass *st_parent_class   = NULL;

static GType register_type( void );
static void  class_init( NADataElementClass *klass );
static void  instance_init( GTypeInstance *instance, gpointer klass );
static void  instance_dispose( GObject *object );
static void  instance_finalize( GObject *object );

static DataElementFn *get_data_element_fn( guint type );

static GParamSpec *string_spec( const NadfIdType *idtype );
static void        string_free( const NADataElement *element );
static void        string_dump( const NADataElement *element );
static gboolean    string_are_equal( const NADataElement *a, const NADataElement *b );
static gboolean    string_is_valid( const NADataElement *element );
static void       *string_get( const NADataElement *element );
static void        string_get_via_value( const NADataElement *element, GValue *value );
static void        string_set_from_element( NADataElement *element, const NADataElement *source );
static void        string_set_from_string( NADataElement *element, const gchar *string );
static void        string_set_from_value( NADataElement *element, const GValue *value );
static void        string_set_from_void( NADataElement *element, const void *value );

static gboolean    locale_are_equal( const NADataElement *a, const NADataElement *b );
static gboolean    locale_is_valid( const NADataElement *element );

static GParamSpec *slist_spec( const NadfIdType *idtype );
static void        slist_free( const NADataElement *element );
static void        slist_dump( const NADataElement *element );
static gboolean    slist_are_equal( const NADataElement *a, const NADataElement *b );
static gboolean    slist_is_valid( const NADataElement *element );
static void       *slist_get( const NADataElement *element );
static void        slist_get_via_value( const NADataElement *element, GValue *value );
static void        slist_set_from_element( NADataElement *element, const NADataElement *source );
static void        slist_set_from_string( NADataElement *element, const gchar *string );
static void        slist_set_from_value( NADataElement *element, const GValue *value );
static void        slist_set_from_void( NADataElement *element, const void *value );

static GParamSpec *bool_spec( const NadfIdType *idtype );
static void        bool_free( const NADataElement *element );
static void        bool_dump( const NADataElement *element );
static gboolean    bool_are_equal( const NADataElement *a, const NADataElement *b );
static gboolean    bool_is_valid( const NADataElement *element );
static void       *bool_get( const NADataElement *element );
static void        bool_get_via_value( const NADataElement *element, GValue *value );
static void        bool_set_from_element( NADataElement *element, const NADataElement *source );
static void        bool_set_from_string( NADataElement *element, const gchar *string );
static void        bool_set_from_value( NADataElement *element, const GValue *value );
static void        bool_set_from_void( NADataElement *element, const void *value );

static GParamSpec *pointer_spec( const NadfIdType *idtype );
static void        pointer_free( const NADataElement *element );
static void        pointer_dump( const NADataElement *element );
static gboolean    pointer_are_equal( const NADataElement *a, const NADataElement *b );
static gboolean    pointer_is_valid( const NADataElement *element );
static void       *pointer_get( const NADataElement *element );
static void        pointer_get_via_value( const NADataElement *element, GValue *value );
static void        pointer_set_from_element( NADataElement *element, const NADataElement *source );
static void        pointer_set_from_string( NADataElement *element, const gchar *string );
static void        pointer_set_from_value( NADataElement *element, const GValue *value );
static void        pointer_set_from_void( NADataElement *element, const void *value );

static GParamSpec *uint_spec( const NadfIdType *idtype );
static void        uint_free( const NADataElement *element );
static void        uint_dump( const NADataElement *element );
static gboolean    uint_are_equal( const NADataElement *a, const NADataElement *b );
static gboolean    uint_is_valid( const NADataElement *element );
static void       *uint_get( const NADataElement *element );
static void        uint_get_via_value( const NADataElement *element, GValue *value );
static void        uint_set_from_element( NADataElement *element, const NADataElement *source );
static void        uint_set_from_string( NADataElement *element, const gchar *string );
static void        uint_set_from_value( NADataElement *element, const GValue *value );
static void        uint_set_from_void( NADataElement *element, const void *value );

static DataElementFn st_data_element_fn[] = {
		{ NADF_TYPE_STRING,
				string_spec,
				string_free,
				string_dump,
				string_are_equal,
				string_is_valid,
				string_get,
				string_get_via_value,
				string_set_from_element,
				string_set_from_string,
				string_set_from_value,
				string_set_from_void
				},
		{ NADF_TYPE_LOCALE_STRING,
				string_spec,
				string_free,
				string_dump,
				locale_are_equal,
				locale_is_valid,
				string_get,
				string_get_via_value,
				string_set_from_element,
				string_set_from_string,
				string_set_from_value,
				string_set_from_void
				},
		{ NADF_TYPE_STRING_LIST,
				slist_spec,
				slist_free,
				slist_dump,
				slist_are_equal,
				slist_is_valid,
				slist_get,
				slist_get_via_value,
				slist_set_from_element,
				slist_set_from_string,
				slist_set_from_value,
				slist_set_from_void
				},
		{ NADF_TYPE_BOOLEAN,
				bool_spec,
				bool_free,
				bool_dump,
				bool_are_equal,
				bool_is_valid,
				bool_get,
				bool_get_via_value,
				bool_set_from_element,
				bool_set_from_string,
				bool_set_from_value,
				bool_set_from_void
				},
		{ NADF_TYPE_POINTER,
				pointer_spec,
				pointer_free,
				pointer_dump,
				pointer_are_equal,
				pointer_is_valid,
				pointer_get,
				pointer_get_via_value,
				pointer_set_from_element,
				pointer_set_from_string,
				pointer_set_from_value,
				pointer_set_from_void
				},
		{ NADF_TYPE_UINT,
				uint_spec,
				uint_free,
				uint_dump,
				uint_are_equal,
				uint_is_valid,
				uint_get,
				uint_get_via_value,
				uint_set_from_element,
				uint_set_from_string,
				uint_set_from_value,
				uint_set_from_void
				},
		{ 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

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

	DataElementFn *fn = get_data_element_fn( self->private->iddef->type );
	if( fn->free ){
		( *fn->free )( self );
	}

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

static DataElementFn *
get_data_element_fn( guint type )
{
	static const gchar *thisfn = "na_data_element_get_data_element_fn";
	int i;
	DataElementFn *fn;

	fn = NULL;

	for( i = 0 ; st_data_element_fn[i].type && !fn ; ++i ){
		if( st_data_element_fn[i].type == type ){
			fn = st_data_element_fn+i;
		}
	}

	if( !fn ){
		g_warning( "%s: unmanaged type=%d", thisfn, type );
	}

	return( fn );
}

/**
 * na_data_element_new:
 * @iddef: the #NadfIdType definition structure for this element.
 *
 * Returns: a newly allocated #NADataElement.
 */
NADataElement *
na_data_element_new( const NadfIdType *iddef )
{
	NADataElement *element;

	element = g_object_new( NA_DATA_ELEMENT_TYPE, NULL );

	element->private->iddef = ( NadfIdType * ) iddef;

	return( element );
}

/**
 * na_data_element_dump:
 * @element: this #NADataElement object.
 *
 * Dump the content of @element.
 */
void
na_data_element_dump( const NADataElement *element )
{
	DataElementFn *fn;

	fn = get_data_element_fn( element->private->iddef->type );

	if( fn ){
		if( fn->dump ){
			( *fn->dump )( element );
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
	DataElementFn *fn;
	void *value;

	g_return_val_if_fail( NA_IS_DATA_ELEMENT( element ), NULL );

	value = NULL;

	if( !element->private->dispose_has_run ){

		fn = get_data_element_fn( element->private->iddef->type );

		if( fn ){
			if( fn->get ){
				value = ( *fn->get )( element );
			}
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
	DataElementFn *fn;

	g_return_if_fail( NA_IS_DATA_ELEMENT( element ));

	if( !element->private->dispose_has_run ){

		fn = get_data_element_fn( element->private->iddef->type );

		if( fn ){
			if( fn->get_via_value ){
				( *fn->get_via_value )( element, value );
			}
		}
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
	DataElementFn *fn;

	g_return_if_fail( NA_IS_DATA_ELEMENT( element ));
	g_return_if_fail( NA_IS_DATA_ELEMENT( value ));
	g_return_if_fail( element->private->iddef->type == value->private->iddef->type );

	if( !element->private->dispose_has_run ){

		fn = get_data_element_fn( element->private->iddef->type );

		if( fn ){
			if( fn->free ){
				( *fn->free )( element );
			}
			if( fn->set_from_element ){
				( *fn->set_from_element )( element, value );
			}
		}
	}
}

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
	DataElementFn *fn;

	g_return_if_fail( NA_IS_DATA_ELEMENT( element ));

	if( !element->private->dispose_has_run ){

		fn = get_data_element_fn( element->private->iddef->type );

		if( fn ){
			if( fn->free ){
				( *fn->free )( element );
			}
			if( fn->set_from_string ){
				( *fn->set_from_string )( element, value );
			}
		}
	}
}

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
	DataElementFn *fn;

	g_return_if_fail( NA_IS_DATA_ELEMENT( element ));

	if( !element->private->dispose_has_run ){

		fn = get_data_element_fn( element->private->iddef->type );

		if( fn ){
			if( fn->free ){
				( *fn->free )( element );
			}
			if( fn->set_from_value ){
				( *fn->set_from_value )( element, value );
			}
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
	DataElementFn *fn;

	g_return_if_fail( NA_IS_DATA_ELEMENT( element ));

	if( !element->private->dispose_has_run ){

		fn = get_data_element_fn( element->private->iddef->type );

		if( fn ){
			if( fn->free ){
				( *fn->free )( element );
			}
			if( fn->set_from_void ){
				( *fn->set_from_void )( element, value );
			}
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
	DataElementFn *fn;
	gboolean are_equal;

	g_return_val_if_fail( NA_IS_DATA_ELEMENT( a ), FALSE );
	g_return_val_if_fail( NA_IS_DATA_ELEMENT( b ), FALSE );

	are_equal = FALSE;

	if( !a->private->dispose_has_run &&
		!b->private->dispose_has_run ){

		if( a->private->iddef->type == b->private->iddef->type ){

			fn = get_data_element_fn( a->private->iddef->type );

			if( fn ){
				if( fn->are_equal ){
					are_equal = ( *fn->are_equal )( a, b );
				}
			}
		}
	}

	return( are_equal );
}

/**
 * na_data_element_is_valid:
 * @object: the #NADataElement object whose validity is to be checked.
 *
 * Returns: %TRUE if the element is valid, %FALSE else.
 */
gboolean
na_data_element_is_valid( const NADataElement *element )
{
	DataElementFn *fn;
	gboolean is_valid;

	g_return_val_if_fail( NA_IS_DATA_ELEMENT( element ), FALSE );

	is_valid = FALSE;

	if( !element->private->dispose_has_run ){

		fn = get_data_element_fn( element->private->iddef->type );

		if( fn ){
			if( fn->is_valid ){
				is_valid = ( *fn->is_valid )( element );
			}
		}
	}

	return( is_valid );
}

static GParamSpec *
string_spec( const NadfIdType *idtype )
{
	return( NULL );
}

static void
string_free( const NADataElement *element )
{
	if( element->private->iddef->free ){
		( *element->private->iddef->free )( element->private->u.string );
	} else {
		g_free( element->private->u.string );
	}
	element->private->u.string = NULL;
}

static void
string_dump( const NADataElement *element )
{
	g_debug( "na-data-element: %s=%s", element->private->iddef->name, element->private->u.string );
}

static gboolean
string_are_equal( const NADataElement *a, const NADataElement *b )
{
	if( !a->private->u.string && !b->private->u.string ){
		return( TRUE );
	}
	if( !a->private->u.string || !b->private->u.string ){
		return( FALSE );
	}
	return( strcmp( a->private->u.string, b->private->u.string ) == 0 );
}

static gboolean
string_is_valid( const NADataElement *element )
{
	return( element->private->u.string && strlen( element->private->u.string ) > 0 );
}

static void *
string_get( const NADataElement *element )
{
	void *value = NULL;

	if( element->private->u.string ){
		value = g_strdup( element->private->u.string );
	}

	return( value );
}

static void
string_get_via_value( const NADataElement *element, GValue *value )
{
	g_value_set_string( value, element->private->u.string );
}

static void
string_set_from_element( NADataElement *element, const NADataElement *source )
{
	element->private->u.string = g_strdup( source->private->u.string );
}

static void
string_set_from_string( NADataElement *element, const gchar *string )
{
	if( string ){
		element->private->u.string = g_strdup( string );
	}
}

static void
string_set_from_value( NADataElement *element, const GValue *value )
{
	if( g_value_get_string( value )){
		element->private->u.string = g_value_dup_string( value );
	}
}

static void
string_set_from_void( NADataElement *element, const void *value )
{
	if( value ){
		element->private->u.string = g_strdup(( const gchar * ) value );
	}
}

static gboolean
locale_are_equal( const NADataElement *a, const NADataElement *b )
{
	if( !a->private->u.string && !b->private->u.string ){
		return( TRUE );
	}
	if( !a->private->u.string || !b->private->u.string ){
		return( FALSE );
	}
	return( g_utf8_collate( a->private->u.string, b->private->u.string ) == 0 );
}

static gboolean
locale_is_valid( const NADataElement *element )
{
	return( element->private->u.string && g_utf8_strlen( element->private->u.string, -1 ) > 0 );
}

static GParamSpec *
slist_spec( const NadfIdType *idtype )
{
	return( NULL );
}

static void
slist_free( const NADataElement *element )
{
	if( element->private->iddef->free ){
		( *element->private->iddef->free )( element->private->u.slist );
	} else {
		na_core_utils_slist_free( element->private->u.slist );
	}
	element->private->u.slist = NULL;
}

static void
slist_dump( const NADataElement *element )
{
	g_debug( "na-data-element: %s=", element->private->iddef->name );
	na_core_utils_slist_dump( element->private->u.slist );
}

static gboolean
slist_are_equal( const NADataElement *a, const NADataElement *b )
{
	if( !a->private->u.slist && !b->private->u.slist ){
		return( TRUE );
	}
	if( !a->private->u.slist || !b->private->u.slist ){
		return( FALSE );
	}
	return( na_core_utils_slist_are_equal( a->private->u.slist, b->private->u.slist ));
}

static gboolean
slist_is_valid( const NADataElement *element )
{
	return( element->private->u.slist && g_slist_length( element->private->u.slist ) > 0 );
}

static void *
slist_get( const NADataElement *element )
{
	void *value = NULL;

	if( element->private->u.slist ){
		value = na_core_utils_slist_duplicate( element->private->u.slist );
	}

	return( value );
}

static void
slist_get_via_value( const NADataElement *element, GValue *value )
{
	g_value_set_pointer( value, na_core_utils_slist_duplicate( element->private->u.slist ));
}

static void
slist_set_from_element( NADataElement *element, const NADataElement *source )
{
	element->private->u.slist = na_core_utils_slist_duplicate( source->private->u.slist );
}

static void
slist_set_from_string( NADataElement *element, const gchar *string )
{
	if( string ){
		element->private->u.slist = g_slist_append( NULL, g_strdup( string ));
	}
}

static void
slist_set_from_value( NADataElement *element, const GValue *value )
{
	if( g_value_get_pointer( value )){
		element->private->u.slist = na_core_utils_slist_duplicate( g_value_get_pointer( value ));
	}
}

static void
slist_set_from_void( NADataElement *element, const void *value )
{
	if( value ){
		element->private->u.slist = na_core_utils_slist_duplicate(( GSList * ) value );
	}
}

static GParamSpec *
bool_spec( const NadfIdType *idtype )
{
	return( NULL );
}

static void
bool_free( const NADataElement *element )
{
	/* n/a */
}

static void
bool_dump( const NADataElement *element )
{
	g_debug( "na-data-element: %s=%s",
			element->private->iddef->name, element->private->u.boolean ? "True":"False" );
}

static gboolean
bool_are_equal( const NADataElement *a, const NADataElement *b )
{
	return( a->private->u.boolean == b->private->u.boolean );
}

static gboolean
bool_is_valid( const NADataElement *element )
{
	return( TRUE );
}

static void *
bool_get( const NADataElement *element )
{
	return( GUINT_TO_POINTER( element->private->u.boolean ));
}

static void
bool_get_via_value( const NADataElement *element, GValue *value )
{
	g_value_set_boolean( value, element->private->u.boolean );
}

static void
bool_set_from_element( NADataElement *element, const NADataElement *source )
{
	element->private->u.boolean = source->private->u.boolean;
}

static void
bool_set_from_string( NADataElement *element, const gchar *string )
{
	element->private->u.boolean = na_core_utils_boolean_from_string( string );
}

static void
bool_set_from_value( NADataElement *element, const GValue *value )
{
	element->private->u.boolean = g_value_get_boolean( value );
}

static void
bool_set_from_void( NADataElement *element, const void *value )
{
	element->private->u.boolean = GPOINTER_TO_UINT( value );
}

static GParamSpec *
pointer_spec( const NadfIdType *idtype )
{
	return( NULL );
}

static void
pointer_free( const NADataElement *element )
{
	if( element->private->iddef->free ){
		( *element->private->iddef->free )( element->private->u.pointer );
	}
	element->private->u.pointer = NULL;
}

static void
pointer_dump( const NADataElement *element )
{
	g_debug( "na-data-element: %s=%p",
			element->private->iddef->name, ( void * ) element->private->u.pointer );
}

static gboolean
pointer_are_equal( const NADataElement *a, const NADataElement *b )
{
	return( a->private->u.pointer == b->private->u.pointer );
}

static gboolean
pointer_is_valid( const NADataElement *element )
{
	return( element->private->u.pointer != NULL );
}

static void *
pointer_get( const NADataElement *element )
{
	return( element->private->u.pointer );
}

static void
pointer_get_via_value( const NADataElement *element, GValue *value )
{
	g_value_set_pointer( value, element->private->u.pointer );
}

static void
pointer_set_from_element( NADataElement *element, const NADataElement *source )
{
	element->private->u.pointer = source->private->u.pointer;
}

static void
pointer_set_from_string( NADataElement *element, const gchar *pointer )
{
}

static void
pointer_set_from_value( NADataElement *element, const GValue *value )
{
	element->private->u.pointer = g_value_get_pointer( value );
}

static void
pointer_set_from_void( NADataElement *element, const void *value )
{
	element->private->u.pointer = ( void * ) value;
}

static GParamSpec *
uint_spec( const NadfIdType *idtype )
{
	return( NULL );
}

static void
uint_free( const NADataElement *element )
{
	/* n/a */
}

static void
uint_dump( const NADataElement *element )
{
	g_debug( "na-data-element: %s=%d",
			element->private->iddef->name, element->private->u.uint );
}

static gboolean
uint_are_equal( const NADataElement *a, const NADataElement *b )
{
	return( a->private->u.uint == b->private->u.uint );
}

static gboolean
uint_is_valid( const NADataElement *element )
{
	return( element->private->u.uint > 0 );
}

static void *
uint_get( const NADataElement *element )
{
	return( GUINT_TO_POINTER( element->private->u.uint ));
}

static void
uint_get_via_value( const NADataElement *element, GValue *value )
{
	g_value_set_uint( value, element->private->u.uint );
}

static void
uint_set_from_element( NADataElement *element, const NADataElement *source )
{
	element->private->u.uint = source->private->u.uint;
}

static void
uint_set_from_string( NADataElement *element, const gchar *string )
{
	element->private->u.uint = atoi( string );
}

static void
uint_set_from_value( NADataElement *element, const GValue *value )
{
	element->private->u.uint = g_value_get_uint( value );
}

static void
uint_set_from_void( NADataElement *element, const void *value )
{
	element->private->u.uint = GPOINTER_TO_UINT( value );
}
