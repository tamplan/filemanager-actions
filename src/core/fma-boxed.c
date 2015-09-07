/*
 * FileManager-Actions
 * A file-manager extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
 *
 * FileManager-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * FileManager-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FileManager-Actions; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
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

#include <api/fma-boxed.h>
#include <api/fma-data-types.h>
#include <api/fma-core-utils.h>

/* private class data
 */
struct _FMABoxedClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* sBoxedDef:
 * This is the structure which fully defines the behavior of this data type.
 */
typedef struct {
	guint            type;
	const gchar     *label;
	gboolean      ( *are_equal )     ( const FMABoxed *, const FMABoxed * );
	void          ( *copy )          ( FMABoxed *, const FMABoxed * );
	void          ( *free )          ( FMABoxed * );
	void          ( *from_string )   ( FMABoxed *, const gchar * );
	void          ( *from_value )    ( FMABoxed *, const GValue * );
	void          ( *from_void )     ( FMABoxed *, const void * );
	gboolean      ( *to_bool )       ( const FMABoxed * );
	gconstpointer ( *to_pointer )    ( const FMABoxed * );
	gchar       * ( *to_string )     ( const FMABoxed * );
	GSList      * ( *to_string_list )( const FMABoxed * );
	guint         ( *to_uint )       ( const FMABoxed * );
	GList       * ( *to_uint_list )  ( const FMABoxed * );
	void          ( *to_value )      ( const FMABoxed *, GValue * );
	void        * ( *to_void )       ( const FMABoxed * );
}
	sBoxedDef;

/* private instance data
 */
struct _FMABoxedPrivate {
	gboolean         dispose_has_run;
	const sBoxedDef *def;
	gboolean         is_set;
	union {
		gboolean  boolean;
		void     *pointer;
		gchar    *string;
		GSList   *string_list;
		guint     uint;
		GList    *uint_list;
	} u;
};

#define LIST_SEPARATOR					";"
#define DEBUG							if( 0 ) g_debug

static GObjectClass *st_parent_class    = NULL;

static GType            register_type( void );
static void             class_init( FMABoxedClass *klass );
static void             instance_init( GTypeInstance *instance, gpointer klass );
static void             instance_dispose( GObject *object );
static void             instance_finalize( GObject *object );

static FMABoxed        *boxed_new( const sBoxedDef *def );
static const sBoxedDef *get_boxed_def( guint type );
static gchar          **string_to_array( const gchar *string );

static gboolean         bool_are_equal( const FMABoxed *a, const FMABoxed *b );
static void             bool_copy( FMABoxed *dest, const FMABoxed *src );
static void             bool_free( FMABoxed *boxed );
static void             bool_from_string( FMABoxed *boxed, const gchar *string );
static void             bool_from_value( FMABoxed *boxed, const GValue *value );
static void             bool_from_void( FMABoxed *boxed, const void *value );
static gchar           *bool_to_string( const FMABoxed *boxed );
static gboolean         bool_to_bool( const FMABoxed *boxed );
static gconstpointer    bool_to_pointer( const FMABoxed *boxed );
static gchar           *bool_to_string( const FMABoxed *boxed );
static void             bool_to_value( const FMABoxed *boxed, GValue *value );
static void            *bool_to_void( const FMABoxed *boxed );

static gboolean         pointer_are_equal( const FMABoxed *a, const FMABoxed *b );
static void             pointer_copy( FMABoxed *dest, const FMABoxed *src );
static void             pointer_free( FMABoxed *boxed );
static void             pointer_from_string( FMABoxed *boxed, const gchar *string );
static void             pointer_from_value( FMABoxed *boxed, const GValue *value );
static void             pointer_from_void( FMABoxed *boxed, const void *value );
static gconstpointer    pointer_to_pointer( const FMABoxed *boxed );
static gchar           *pointer_to_string( const FMABoxed *boxed );
static void             pointer_to_value( const FMABoxed *boxed, GValue *value );
static void            *pointer_to_void( const FMABoxed *boxed );

static gboolean         string_are_equal( const FMABoxed *a, const FMABoxed *b );
static void             string_copy( FMABoxed *dest, const FMABoxed *src );
static void             string_free( FMABoxed *boxed );
static void             string_from_string( FMABoxed *boxed, const gchar *string );
static void             string_from_value( FMABoxed *boxed, const GValue *value );
static void             string_from_void( FMABoxed *boxed, const void *value );
static gconstpointer    string_to_pointer( const FMABoxed *boxed );
static gchar           *string_to_string( const FMABoxed *boxed );
static void             string_to_value( const FMABoxed *boxed, GValue *value );
static void            *string_to_void( const FMABoxed *boxed );

static gboolean         string_list_are_equal( const FMABoxed *a, const FMABoxed *b );
static void             string_list_copy( FMABoxed *dest, const FMABoxed *src );
static void             string_list_free( FMABoxed *boxed );
static void             string_list_from_string( FMABoxed *boxed, const gchar *string );
static void             string_list_from_value( FMABoxed *boxed, const GValue *value );
static void             string_list_from_void( FMABoxed *boxed, const void *value );
static gconstpointer    string_list_to_pointer( const FMABoxed *boxed );
static gchar           *string_list_to_string( const FMABoxed *boxed );
static GSList          *string_list_to_string_list( const FMABoxed *boxed );
static void             string_list_to_value( const FMABoxed *boxed, GValue *value );
static void            *string_list_to_void( const FMABoxed *boxed );

static gboolean         locale_are_equal( const FMABoxed *a, const FMABoxed *b );

static gboolean         uint_are_equal( const FMABoxed *a, const FMABoxed *b );
static void             uint_copy( FMABoxed *dest, const FMABoxed *src );
static void             uint_free( FMABoxed *boxed );
static void             uint_from_string( FMABoxed *boxed, const gchar *string );
static void             uint_from_value( FMABoxed *boxed, const GValue *value );
static void             uint_from_void( FMABoxed *boxed, const void *value );
static gconstpointer    uint_to_pointer( const FMABoxed *boxed );
static gchar           *uint_to_string( const FMABoxed *boxed );
static guint            uint_to_uint( const FMABoxed *boxed );
static void             uint_to_value( const FMABoxed *boxed, GValue *value );
static void            *uint_to_void( const FMABoxed *boxed );

static gboolean         uint_list_are_equal( const FMABoxed *a, const FMABoxed *b );
static void             uint_list_copy( FMABoxed *dest, const FMABoxed *src );
static void             uint_list_free( FMABoxed *boxed );
static void             uint_list_from_string( FMABoxed *boxed, const gchar *string );
static void             uint_list_from_value( FMABoxed *boxed, const GValue *value );
static void             uint_list_from_void( FMABoxed *boxed, const void *value );
static gconstpointer    uint_list_to_pointer( const FMABoxed *boxed );
static gchar           *uint_list_to_string( const FMABoxed *boxed );
static GList           *uint_list_to_uint_list( const FMABoxed *boxed );
static void             uint_list_to_value( const FMABoxed *boxed, GValue *value );
static void            *uint_list_to_void( const FMABoxed *boxed );

static sBoxedDef st_boxed_def[] = {
		{ NA_DATA_TYPE_BOOLEAN,
				"boolean",
				bool_are_equal,
				bool_copy,
				bool_free,
				bool_from_string,
				bool_from_value,
				bool_from_void,
				bool_to_bool,
				bool_to_pointer,
				bool_to_string,
				NULL,
				NULL,
				NULL,
				bool_to_value,
				bool_to_void
				},
		{ NA_DATA_TYPE_POINTER,
				"pointer",
				pointer_are_equal,
				pointer_copy,
				pointer_free,
				pointer_from_string,
				pointer_from_value,
				pointer_from_void,
				NULL,
				pointer_to_pointer,
				pointer_to_string,
				NULL,
				NULL,
				NULL,
				pointer_to_value,
				pointer_to_void
				},
		{ NA_DATA_TYPE_STRING,
				"string",
				string_are_equal,
				string_copy,
				string_free,
				string_from_string,
				string_from_value,
				string_from_void,
				NULL,						/* to_bool */
				string_to_pointer,
				string_to_string,
				NULL,						/* to_string_list */
				NULL,						/* to_uint */
				NULL,						/* to_uint_list */
				string_to_value,
				string_to_void
				},
		{ NA_DATA_TYPE_STRING_LIST,
				"string_list",
				string_list_are_equal,
				string_list_copy,
				string_list_free,
				string_list_from_string,
				string_list_from_value,
				string_list_from_void,
				NULL,
				string_list_to_pointer,
				string_list_to_string,
				string_list_to_string_list,
				NULL,
				NULL,
				string_list_to_value,
				string_list_to_void
				},
		{ NA_DATA_TYPE_LOCALE_STRING,
				"locale_string",
				locale_are_equal,
				string_copy,
				string_free,
				string_from_string,
				string_from_value,
				string_from_void,
				NULL,						/* to_bool */
				string_to_pointer,
				string_to_string,
				NULL,						/* to_string_list */
				NULL,						/* to_uint */
				NULL,						/* to_uint_list */
				string_to_value,
				string_to_void
				},
		{ NA_DATA_TYPE_UINT,
				"uint",
				uint_are_equal,
				uint_copy,
				uint_free,
				uint_from_string,
				uint_from_value,
				uint_from_void,
				NULL,
				uint_to_pointer,
				uint_to_string,
				NULL,
				uint_to_uint,
				NULL,
				uint_to_value,
				uint_to_void
				},
		{ NA_DATA_TYPE_UINT_LIST,
				"uint_list",
				uint_list_are_equal,
				uint_list_copy,
				uint_list_free,
				uint_list_from_string,
				uint_list_from_value,
				uint_list_from_void,
				NULL,
				uint_list_to_pointer,
				uint_list_to_string,
				NULL,
				NULL,
				uint_list_to_uint_list,
				uint_list_to_value,
				uint_list_to_void
				},
		{ 0 }
};

GType
fma_boxed_get_type( void )
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
	static const gchar *thisfn = "fma_boxed_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( FMABoxedClass ),
		NULL,
		NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( FMABoxed ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "FMABoxed", &info, 0 );

	return( type );
}

static void
class_init( FMABoxedClass *klass )
{
	static const gchar *thisfn = "fma_boxed_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( FMABoxedClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "fma_boxed_instance_init";
	FMABoxed *self;

	g_return_if_fail( FMA_IS_BOXED( instance ));

	DEBUG( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self = FMA_BOXED( instance );

	self->private = g_new0( FMABoxedPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->def = NULL;
	self->private->is_set = FALSE;
}

static void
instance_dispose( GObject *object )
{
	FMABoxed *self;

	g_return_if_fail( FMA_IS_BOXED( object ));

	self = FMA_BOXED( object );

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
	static const gchar *thisfn = "fma_boxed_instance_finalize";
	FMABoxed *self;

	g_return_if_fail( FMA_IS_BOXED( object ));

	DEBUG( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	self = FMA_BOXED( object );

	if( self->private->def ){
		if( self->private->def->free ){
			( *self->private->def->free )( self );
		}
	}

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

static FMABoxed *
boxed_new( const sBoxedDef *def )
{
	FMABoxed *boxed;

	boxed = g_object_new( FMA_TYPE_BOXED, NULL );
	boxed->private->def = def;

	return( boxed );
}

static const sBoxedDef *
get_boxed_def( guint type )
{
	static const gchar *thisfn = "fma_boxed_get_boxed_def";
	sBoxedDef *def;

	for( def = st_boxed_def ; def->type ; ++def ){
		if( def->type == type ){
			return(( const sBoxedDef * ) def );
		}
	}

	g_warning( "%s: unmanaged data type: %d", thisfn, type );
	return( NULL );
}

/*
 * converts a string to an array of strings
 * accepts both:
 * - a semi-comma-separated list of strings (the last separator, if any, is not counted)
 * - a comma-separated list of strings between square brackets (à la GConf)
 *
 */
static gchar **
string_to_array( const gchar *string )
{
	gchar *sdup;
	gchar **array;

	array = NULL;

	if( string && strlen( string )){
		sdup = g_strstrip( g_strdup( string ));

		/* GConf-style string list [value,value]
		 */
		if( sdup[0] == '[' && sdup[strlen(sdup)-1] == ']' ){
			sdup[0] = ' ';
			sdup[strlen(sdup)-1] = ' ';
			sdup = g_strstrip( sdup );
			array = g_strsplit( sdup, ",", -1 );

		/* semi-comma-separated list of strings
		 */
		} else {
			if( g_str_has_suffix( string, LIST_SEPARATOR )){
				sdup[strlen(sdup)-1] = ' ';
				sdup = g_strstrip( sdup );
			}
			array = g_strsplit( sdup, LIST_SEPARATOR, -1 );
		}
		g_free( sdup );
	}

	return( array );
}

/**
 * fma_boxed_set_type:
 * @boxed: this #FMABoxed object.
 * @type: the required type as defined in fma-data-types.h
 *
 * Set the type of the just-allocated @boxed object.
 *
 * Since: 3.1
 */
void
fma_boxed_set_type( FMABoxed *boxed, guint type )
{
	g_return_if_fail( FMA_IS_BOXED( boxed ));
	g_return_if_fail( boxed->private->dispose_has_run == FALSE );
	g_return_if_fail( boxed->private->def == NULL );

	boxed->private->def = get_boxed_def( type );
}

/**
 * fma_boxed_are_equal:
 * @a: the first #FMABoxed object.
 * @b: the second #FMABoxed object.
 *
 * Returns: %TRUE if @a and @b are equal, %FALSE else.
 *
 * Since: 3.1
 */
gboolean
fma_boxed_are_equal( const FMABoxed *a, const FMABoxed *b )
{
	gboolean are_equal;

	g_return_val_if_fail( FMA_IS_BOXED( a ), FALSE );
	g_return_val_if_fail( a->private->dispose_has_run == FALSE, FALSE );
	g_return_val_if_fail( FMA_IS_BOXED( b ), FALSE );
	g_return_val_if_fail( b->private->dispose_has_run == FALSE, FALSE );
	g_return_val_if_fail( a->private->def, FALSE );
	g_return_val_if_fail( a->private->def == b->private->def, FALSE );
	g_return_val_if_fail( a->private->def->are_equal, FALSE );

	are_equal = FALSE;

	if( a->private->is_set == b->private->is_set ){
		are_equal = TRUE;
		if( a->private->is_set ){
			are_equal = ( *a->private->def->are_equal )( a, b );
		}
	}

	return( are_equal );
}

/**
 * fma_boxed_copy:
 * @boxed: the source #FMABoxed box.
 *
 * Returns: a copy of @boxed, as a newly allocated #FMABoxed which should
 * be g_object_unref() by the caller.
 *
 * Since: 3.1
 */
FMABoxed *
fma_boxed_copy( const FMABoxed *boxed )
{
	FMABoxed *dest;

	g_return_val_if_fail( FMA_IS_BOXED( boxed ), NULL );
	g_return_val_if_fail( boxed->private->dispose_has_run == FALSE, NULL );
	g_return_val_if_fail( boxed->private->def, NULL );
	g_return_val_if_fail( boxed->private->def->copy, NULL );

	dest = boxed_new( boxed->private->def );
	if( boxed->private->is_set ){
		( *boxed->private->def->copy )( dest, boxed );
		dest->private->is_set = TRUE;
	}

	return( dest );
}

/**
 * fma_boxed_dump:
 * @boxed: the #FMABoxed box to be dumped.
 *
 * Dumps the @boxed box.
 *
 * Since: 3.1
 */
void
fma_boxed_dump( const FMABoxed *boxed )
{
	static const gchar *thisfn = "fma_boxed_dump";
	gchar *str;

	g_return_if_fail( FMA_IS_BOXED( boxed ));
	g_return_if_fail( boxed->private->dispose_has_run == FALSE );
	g_return_if_fail( boxed->private->def );
	g_return_if_fail( boxed->private->def->to_string );

	str = ( boxed->private->is_set ) ? ( *boxed->private->def->to_string )( boxed ) : NULL;
	g_debug( "%s: boxed=%p, type=%u, is_set=%s, value=%s",
			thisfn, ( void * ) boxed, boxed->private->def->type,
			boxed->private->is_set ? "True":"False", str );
	g_free( str );
}

/**
 * fma_boxed_new_from_string:
 * @type: the type of the #FMABoxed to be allocated.
 * @string: the initial value of the #FMABoxed as a string.
 *
 * Allocates a new #FMABoxed of the specified @type, and initializes it
 * with @string.
 *
 * If the type is a list, then the last separator is automatically stripped.
 *
 * Returns: a newly allocated #FMABoxed, which should be g_object_unref()
 * by the caller, or %NULL if the type is unknowned, or does not provide
 * the 'from_string' function.
 *
 * Since: 3.1
 */
FMABoxed *
fma_boxed_new_from_string( guint type, const gchar *string )
{
	const sBoxedDef *def;
	FMABoxed *boxed;

	def = get_boxed_def( type );

	g_return_val_if_fail( def, NULL );
	g_return_val_if_fail( def->from_string, NULL );

	boxed = boxed_new( def );
	( *def->from_string )( boxed, string );
	boxed->private->is_set = TRUE;

	return( boxed );
}

/**
 * fma_boxed_get_boolean:
 * @boxed: the #FMABoxed structure.
 *
 * Returns: the boolean value if @boxed is of %NA_DATA_TYPE_BOOLEAN type,
 * %FALSE else.
 *
 * Since: 3.1
 */
gboolean
fma_boxed_get_boolean( const FMABoxed *boxed )
{
	gboolean value;

	g_return_val_if_fail( FMA_IS_BOXED( boxed ), FALSE );
	g_return_val_if_fail( boxed->private->dispose_has_run == FALSE, FALSE );
	g_return_val_if_fail( boxed->private->def, FALSE );
	g_return_val_if_fail( boxed->private->def->type == NA_DATA_TYPE_BOOLEAN, FALSE );
	g_return_val_if_fail( boxed->private->def->to_bool, FALSE );

	value = ( *boxed->private->def->to_bool )( boxed );

	return( value );
}

/**
 * fma_boxed_get_pointer:
 * @boxed: the #FMABoxed structure.
 *
 * Returns: a const pointer to the data if @boxed is of %NA_DATA_TYPE_POINTER
 * type, %NULL else.
 *
 * Since: 3.1
 */
gconstpointer
fma_boxed_get_pointer( const FMABoxed *boxed )
{
	gconstpointer value;

	g_return_val_if_fail( FMA_IS_BOXED( boxed ), NULL );
	g_return_val_if_fail( boxed->private->dispose_has_run == FALSE, NULL );
	g_return_val_if_fail( boxed->private->def, NULL );
	g_return_val_if_fail( boxed->private->def->to_pointer, NULL );

	value = ( *boxed->private->def->to_pointer )( boxed );

	return( value );
}

/**
 * fma_boxed_get_string:
 * @boxed: the #FMABoxed structure.
 *
 * Returns: a newly allocated string if @boxed is of %NA_DATA_TYPE_STRING
 * type, which should be g_free() by the caller, %NULL else.
 *
 * Since: 3.1
 */
gchar *
fma_boxed_get_string( const FMABoxed *boxed )
{
	gchar *value;

	g_return_val_if_fail( FMA_IS_BOXED( boxed ), NULL );
	g_return_val_if_fail( boxed->private->dispose_has_run == FALSE, NULL );
	g_return_val_if_fail( boxed->private->def, NULL );
	g_return_val_if_fail( boxed->private->def->to_string, NULL );

	value = ( *boxed->private->def->to_string )( boxed );

	return( value );
}

/**
 * fma_boxed_get_string_list:
 * @boxed: the #FMABoxed structure.
 *
 * Returns: a newly allocated string list if @boxed is of %NA_DATA_TYPE_STRING_LIST
 * type, which should be fma_core_utils_slist_free() by the caller, %NULL else.
 *
 * Since: 3.1
 */
GSList *
fma_boxed_get_string_list( const FMABoxed *boxed )
{
	GSList *value;

	g_return_val_if_fail( FMA_IS_BOXED( boxed ), NULL );
	g_return_val_if_fail( boxed->private->dispose_has_run == FALSE, NULL );
	g_return_val_if_fail( boxed->private->def, NULL );
	g_return_val_if_fail( boxed->private->def->type == NA_DATA_TYPE_STRING_LIST, NULL );
	g_return_val_if_fail( boxed->private->def->to_string_list, NULL );

	value = ( *boxed->private->def->to_string_list )( boxed );

	return( value );
}

/**
 * fma_boxed_get_uint:
 * @boxed: the #FMABoxed structure.
 *
 * Returns: an unsigned integer if @boxed is of %NA_DATA_TYPE_UINT type,
 * zero else.
 *
 * Since: 3.1
 */
guint
fma_boxed_get_uint( const FMABoxed *boxed )
{
	guint value;

	g_return_val_if_fail( FMA_IS_BOXED( boxed ), 0 );
	g_return_val_if_fail( boxed->private->dispose_has_run == FALSE, 0 );
	g_return_val_if_fail( boxed->private->def, 0 );
	g_return_val_if_fail( boxed->private->def->type == NA_DATA_TYPE_UINT, 0 );
	g_return_val_if_fail( boxed->private->def->to_uint, 0 );

	value = ( *boxed->private->def->to_uint )( boxed );

	return( value );
}

/**
 * fma_boxed_get_uint_list:
 * @boxed: the #FMABoxed structure.
 *
 * Returns: a newly allocated list if @boxed is of %NA_DATA_TYPE_UINT_LIST
 * type, which should be g_list_free() by the caller, %FALSE else.
 *
 * Since: 3.1
 */
GList *
fma_boxed_get_uint_list( const FMABoxed *boxed )
{
	GList *value;

	g_return_val_if_fail( FMA_IS_BOXED( boxed ), NULL );
	g_return_val_if_fail( boxed->private->dispose_has_run == FALSE, NULL );
	g_return_val_if_fail( boxed->private->def, NULL );
	g_return_val_if_fail( boxed->private->def->type == NA_DATA_TYPE_UINT_LIST, NULL );
	g_return_val_if_fail( boxed->private->def->to_uint_list, NULL );

	value = ( *boxed->private->def->to_uint_list )( boxed );

	return( value );
}

/**
 * fma_boxed_get_as_value:
 * @boxed: the #FMABoxed whose value is to be got.
 * @value: the #GValue which holds the string to be set.
 *
 * Setup @value with the content of the @boxed.
 *
 * Since: 3.1
 */
void
fma_boxed_get_as_value( const FMABoxed *boxed, GValue *value )
{
	g_return_if_fail( FMA_IS_BOXED( boxed ));
	g_return_if_fail( boxed->private->dispose_has_run == FALSE );
	g_return_if_fail( boxed->private->def );
	g_return_if_fail( boxed->private->def->to_value );

	( *boxed->private->def->to_value )( boxed, value );
}

/**
 * fma_boxed_get_as_void:
 * @boxed: the #FMABoxed whose value is to be got.
 *
 * Returns: the content of the @boxed.
 *
 * If of type NA_DATA_TYPE_STRING (resp. NA_DATA_TYPE_LOCALE_STRING,
 * NA_DATA_TYPE_STRING_LIST or NA_DATA_TYPE_UINT_LIST), then the content
 * is returned in a newly allocated value, which should be g_free() (resp.
 * g_free(), fma_core_utils_slist_free(), g_list_free()) by the caller.
 *
 * Since: 3.1
 */
void *
fma_boxed_get_as_void( const FMABoxed *boxed )
{
	g_return_val_if_fail( FMA_IS_BOXED( boxed ), NULL );
	g_return_val_if_fail( boxed->private->dispose_has_run == FALSE, NULL );
	g_return_val_if_fail( boxed->private->def, NULL );
	g_return_val_if_fail( boxed->private->def->to_void, NULL );

	return(( *boxed->private->def->to_void )( boxed ));
}

/**
 * fma_boxed_set_from_boxed:
 * @boxed: the #FMABoxed whose value is to be set.
 * @value: the source #FMABoxed.
 *
 * Copy value from @value to @boxed.
 *
 * Since: 3.1
 */
void
fma_boxed_set_from_boxed( FMABoxed *boxed, const FMABoxed *value )
{
	g_return_if_fail( FMA_IS_BOXED( boxed ));
	g_return_if_fail( boxed->private->dispose_has_run == FALSE );
	g_return_if_fail( FMA_IS_BOXED( value ));
	g_return_if_fail( value->private->dispose_has_run == FALSE );
	g_return_if_fail( boxed->private->def );
	g_return_if_fail( boxed->private->def == value->private->def );
	g_return_if_fail( boxed->private->def->copy );
	g_return_if_fail( boxed->private->def->free );

	( *boxed->private->def->free )( boxed );
	( *boxed->private->def->copy )( boxed, value );
	boxed->private->is_set = TRUE;
}

/**
 * fma_boxed_set_from_string:
 * @boxed: the #FMABoxed whose value is to be set.
 * @value: the string to be set.
 *
 * Evaluates the @value and set it to the @boxed.
 *
 * Since: 3.1
 */
void
fma_boxed_set_from_string( FMABoxed *boxed, const gchar *value )
{
	g_return_if_fail( FMA_IS_BOXED( boxed ));
	g_return_if_fail( boxed->private->dispose_has_run == FALSE );
	g_return_if_fail( boxed->private->def );
	g_return_if_fail( boxed->private->def->free );
	g_return_if_fail( boxed->private->def->from_string );

	( *boxed->private->def->free )( boxed );
	( *boxed->private->def->from_string )( boxed, value );
	boxed->private->is_set = TRUE;
}

/**
 * fma_boxed_set_from_value:
 * @boxed: the #FMABoxed whose value is to be set.
 * @value: the value whose content is to be got.
 *
 * Evaluates the @value and set it to the @boxed.
 *
 * Since: 3.1
 */
void
fma_boxed_set_from_value( FMABoxed *boxed, const GValue *value )
{
	g_return_if_fail( FMA_IS_BOXED( boxed ));
	g_return_if_fail( boxed->private->dispose_has_run == FALSE );
	g_return_if_fail( boxed->private->def );
	g_return_if_fail( boxed->private->def->free );
	g_return_if_fail( boxed->private->def->from_value );

	( *boxed->private->def->free )( boxed );
	( *boxed->private->def->from_value )( boxed, value );
	boxed->private->is_set = TRUE;
}

/**
 * fma_boxed_set_from_void:
 * @boxed: the #FMABoxed whose value is to be set.
 * @value: the value whose content is to be got.
 *
 * Evaluates the @value and set it to the @boxed.
 *
 * Since: 3.1
 */
void
fma_boxed_set_from_void( FMABoxed *boxed, const void *value )
{
	g_return_if_fail( FMA_IS_BOXED( boxed ));
	g_return_if_fail( boxed->private->dispose_has_run == FALSE );
	g_return_if_fail( boxed->private->def );
	g_return_if_fail( boxed->private->def->free );
	g_return_if_fail( boxed->private->def->from_void );

	( *boxed->private->def->free )( boxed );
	( *boxed->private->def->from_void )( boxed, value );
	boxed->private->is_set = TRUE;
}

static gboolean
bool_are_equal( const FMABoxed *a, const FMABoxed *b )
{
	return( a->private->u.boolean == b->private->u.boolean );
}

static void
bool_copy( FMABoxed *dest, const FMABoxed *src )
{
	dest->private->u.boolean = src->private->u.boolean;
}

static void
bool_free( FMABoxed *boxed )
{
	boxed->private->u.boolean = FALSE;
	boxed->private->is_set = FALSE;
}

static void
bool_from_string( FMABoxed *boxed, const gchar *string )
{
	boxed->private->u.boolean = fma_core_utils_boolean_from_string( string );
}

static void
bool_from_value( FMABoxed *boxed, const GValue *value )
{
	boxed->private->u.boolean = g_value_get_boolean( value );
}

static void
bool_from_void( FMABoxed *boxed, const void *value )
{
	boxed->private->u.boolean = GPOINTER_TO_UINT( value );
}

static gboolean
bool_to_bool( const FMABoxed *boxed )
{
	return( boxed->private->u.boolean );
}

static gconstpointer
bool_to_pointer( const FMABoxed *boxed )
{
	return(( gconstpointer ) GUINT_TO_POINTER( boxed->private->u.boolean ));
}

static gchar *
bool_to_string( const FMABoxed *boxed )
{
	return( g_strdup_printf( "%s", boxed->private->u.boolean ? "true":"false" ));
}

static void
bool_to_value( const FMABoxed *boxed, GValue *value )
{
	g_value_set_boolean( value, boxed->private->u.boolean );
}

static void *
bool_to_void( const FMABoxed *boxed )
{
	return( GUINT_TO_POINTER( boxed->private->u.boolean ));
}

static gboolean
pointer_are_equal( const FMABoxed *a, const FMABoxed *b )
{
	return( a->private->u.pointer == b->private->u.pointer );
}

/*
 * note that copying a pointer is not safe
 */
static void
pointer_copy( FMABoxed *dest, const FMABoxed *src )
{
	dest->private->u.pointer = src->private->u.pointer;
}

static void
pointer_free( FMABoxed *boxed )
{
	boxed->private->u.pointer = NULL;
	boxed->private->is_set = FALSE;
}

static void
pointer_from_string( FMABoxed *boxed, const gchar *pointer )
{
	g_warning( "fma_boxed_pointer_from_string: unrelevant function call" );
}

static void
pointer_from_value( FMABoxed *boxed, const GValue *value )
{
	boxed->private->u.pointer = g_value_get_pointer( value );
}

static void
pointer_from_void( FMABoxed *boxed, const void *value )
{
	boxed->private->u.pointer = ( void * ) value;
}

static gconstpointer
pointer_to_pointer( const FMABoxed *boxed )
{
	return( boxed->private->u.pointer );
}

static gchar *
pointer_to_string( const FMABoxed *boxed )
{
	return( g_strdup_printf( "%p", boxed->private->u.pointer ));
}

static void
pointer_to_value( const FMABoxed *boxed, GValue *value )
{
	g_value_set_pointer( value, boxed->private->u.pointer );
}

static void *
pointer_to_void( const FMABoxed *boxed )
{
	return( boxed->private->u.pointer );
}

static gboolean
string_are_equal( const FMABoxed *a, const FMABoxed *b )
{
	if( a->private->u.string && b->private->u.string ){
		return( strcmp( a->private->u.string, b->private->u.string ) == 0 );
	}
	if( !a->private->u.string && !b->private->u.string ){
		return( TRUE );
	}
	return( FALSE );
}

static void
string_copy( FMABoxed *dest, const FMABoxed *src )
{
	dest->private->u.string = g_strdup( src->private->u.string );
}

static void
string_free( FMABoxed *boxed )
{
	g_free( boxed->private->u.string );
	boxed->private->u.string = NULL;
	boxed->private->is_set = FALSE;
}

static void
string_from_string( FMABoxed *boxed, const gchar *string )
{
	boxed->private->u.string = g_strdup( string ? string : "" );
}

static void
string_from_value( FMABoxed *boxed, const GValue *value )
{
	if( g_value_get_string( value )){
		boxed->private->u.string = g_value_dup_string( value );
	} else {
		boxed->private->u.string = g_strdup( "" );
	}
}

static void
string_from_void( FMABoxed *boxed, const void *value )
{
	boxed->private->u.string = g_strdup( value ? ( const gchar * ) value : "" );
}

static gconstpointer
string_to_pointer( const FMABoxed *boxed )
{
	return(( gconstpointer ) boxed->private->u.string );
}

static gchar *
string_to_string( const FMABoxed *boxed )
{
	return( g_strdup( boxed->private->u.string ));
}

static void
string_to_value( const FMABoxed *boxed, GValue *value )
{
	gchar *str;

	str = string_to_string( boxed );
	g_value_set_string( value, str );
	g_free( str );
}

static void *
string_to_void( const FMABoxed *boxed )
{
	return(( void * ) string_to_string( boxed ));
}

/* the two string lists are equal if they have the same elements in the
 * same order
 */
static gboolean
string_list_are_equal( const FMABoxed *a, const FMABoxed *b )
{
	GSList *ia, *ib;
	gboolean diff = FALSE;

	guint na = g_slist_length( a->private->u.string_list );
	guint nb = g_slist_length( b->private->u.string_list );

	if( na != nb ) return( FALSE );

	for( ia=a->private->u.string_list, ib=b->private->u.string_list ; ia && ib && !diff ; ia=ia->next, ib=ib->next ){
		if( strcmp( ia->data, ib->data ) != 0 ){
			diff = TRUE;
		}
	}

	return( !diff );
}

static void
string_list_copy( FMABoxed *dest, const FMABoxed *src )
{
	if( dest->private->is_set ){
		string_list_free( dest );
	}
	dest->private->u.string_list = fma_core_utils_slist_duplicate( src->private->u.string_list );
	dest->private->is_set = TRUE;
}

static void
string_list_free( FMABoxed *boxed )
{
	fma_core_utils_slist_free( boxed->private->u.string_list );
	boxed->private->u.string_list = NULL;
	boxed->private->is_set = FALSE;
}

/*
 * accept string list both:
 * - as a semi-comma-separated list of strings
 * - as a comma-separated list of string, between two square brackets (à la GConf)
 */
static void
string_list_from_string( FMABoxed *boxed, const gchar *string )
{
	gchar **array;
	gchar **i;

	array = string_to_array( string );

	if( array ){
		i = ( gchar ** ) array;
		while( *i ){
			if( !fma_core_utils_slist_count( boxed->private->u.string_list, ( const gchar * )( *i ))){
				boxed->private->u.string_list = g_slist_prepend( boxed->private->u.string_list, g_strdup( *i ));
			}
			i++;
		}
		boxed->private->u.string_list = g_slist_reverse( boxed->private->u.string_list );
	}

	g_strfreev( array );
}

static void
string_list_from_value( FMABoxed *boxed, const GValue *value )
{
	string_list_from_void( boxed, ( const void * ) g_value_get_pointer( value ));
}

static void
string_list_from_void( FMABoxed *boxed, const void *value )
{
	GSList *value_slist;
	GSList *it;

	value_slist = ( GSList * ) value;
	for( it = value_slist ; it ; it = it->next ){
		if( !fma_core_utils_slist_count( boxed->private->u.string_list, ( const gchar * ) it->data )){
			boxed->private->u.string_list = g_slist_prepend( boxed->private->u.string_list, g_strdup(( const gchar * ) it->data ));
		}
	}
	boxed->private->u.string_list = g_slist_reverse( boxed->private->u.string_list );
}

static gconstpointer
string_list_to_pointer( const FMABoxed *boxed )
{
	return(( gconstpointer ) boxed->private->u.string_list );
}

static gchar *
string_list_to_string( const FMABoxed *boxed )
{
	GSList *is;
	GString *str = g_string_new( "" );
	gboolean first;

	first = TRUE;
	for( is = boxed->private->u.string_list ; is ; is = is->next ){
		if( !first ){
			str = g_string_append( str, LIST_SEPARATOR );
		}
		str = g_string_append( str, ( const gchar * ) is->data );
		first = FALSE;
	}

	return( g_string_free( str, FALSE ));
}

static GSList *
string_list_to_string_list( const FMABoxed *boxed )
{
	return( fma_core_utils_slist_duplicate( boxed->private->u.string_list ));
}

static void
string_list_to_value( const FMABoxed *boxed, GValue *value )
{
	g_value_set_pointer( value, fma_core_utils_slist_duplicate( boxed->private->u.string_list ));
}

static void *
string_list_to_void( const FMABoxed *boxed )
{
	void *value = NULL;

	if( boxed->private->u.string_list ){
		value = fma_core_utils_slist_duplicate( boxed->private->u.string_list );
	}

	return( value );
}

static gboolean
locale_are_equal( const FMABoxed *a, const FMABoxed *b )
{
	if( !a->private->u.string && !b->private->u.string ){
		return( TRUE );
	}
	if( !a->private->u.string || !b->private->u.string ){
		return( FALSE );
	}
	return( fma_core_utils_str_collate( a->private->u.string, b->private->u.string ) == 0 );
}

static gboolean
uint_are_equal( const FMABoxed *a, const FMABoxed *b )
{
	return( a->private->u.uint == b->private->u.uint );
}

static void
uint_copy( FMABoxed *dest, const FMABoxed *src )
{
	dest->private->u.uint = src->private->u.uint;
	dest->private->is_set = TRUE;
}

static void
uint_free( FMABoxed *boxed )
{
	boxed->private->u.uint = 0;
	boxed->private->is_set = FALSE;
}

static void
uint_from_string( FMABoxed *boxed, const gchar *string )
{
	boxed->private->u.uint = string ? atoi( string ) : 0;
}

static void
uint_from_value( FMABoxed *boxed, const GValue *value )
{
	boxed->private->u.uint = g_value_get_uint( value );
}

static void
uint_from_void( FMABoxed *boxed, const void *value )
{
	boxed->private->u.uint = GPOINTER_TO_UINT( value );
}

static gconstpointer
uint_to_pointer( const FMABoxed *boxed )
{
	return(( gconstpointer ) GUINT_TO_POINTER( boxed->private->u.uint ));
}

static gchar *
uint_to_string( const FMABoxed *boxed )
{
	return( g_strdup_printf( "%u", boxed->private->u.uint ));
}

static guint
uint_to_uint( const FMABoxed *boxed )
{
	return( boxed->private->u.uint );
}

static void
uint_to_value( const FMABoxed *boxed, GValue *value )
{
	g_value_set_uint( value, boxed->private->u.uint );
}

static void *
uint_to_void( const FMABoxed *boxed )
{
	return( GUINT_TO_POINTER( boxed->private->u.uint ));
}

/* compare uint list as string list:
 * if the two list do not have the same count, then one is lesser than the other
 * if they have same count and same elements in same order, they are equal
 * else just arbitrarily return -1
 */
static gboolean
uint_list_are_equal( const FMABoxed *a, const FMABoxed *b )
{
	GList *ia, *ib;
	gboolean diff = FALSE;

	guint na = g_list_length( a->private->u.uint_list );
	guint nb = g_list_length( b->private->u.uint_list );

	if( na != nb ) return( FALSE );

	for( ia=a->private->u.uint_list, ib=b->private->u.uint_list ; ia && ib && !diff ; ia=ia->next, ib=ib->next ){
		if( GPOINTER_TO_UINT( ia->data ) != GPOINTER_TO_UINT( ib->data )){
			diff = TRUE;
		}
	}

	return( !diff );
}

static void
uint_list_copy( FMABoxed *dest, const FMABoxed *src )
{
	GList *isrc;

	dest->private->u.uint_list = NULL;
	for( isrc = src->private->u.uint_list ; isrc ; isrc = isrc->next ){
		dest->private->u.uint_list = g_list_prepend( dest->private->u.uint_list, isrc->data );
	}
	dest->private->u.uint_list = g_list_reverse( dest->private->u.uint_list );
}

static void
uint_list_free( FMABoxed *boxed )
{
	g_list_free( boxed->private->u.uint_list );
	boxed->private->u.uint_list = NULL;
	boxed->private->is_set = FALSE;
}

static void
uint_list_from_string( FMABoxed *boxed, const gchar *string )
{
	gchar **array;
	gchar **i;

	array = string_to_array( string );

	if( array ){
		i = ( gchar ** ) array;
		while( *i ){
			boxed->private->u.uint_list = g_list_prepend( boxed->private->u.uint_list, GINT_TO_POINTER( atoi( *i )));
			i++;
		}
		boxed->private->u.uint_list = g_list_reverse( boxed->private->u.uint_list );
	} else {
		boxed->private->u.uint_list = NULL;
	}

	g_strfreev( array );
}

static void
uint_list_from_value( FMABoxed *boxed, const GValue *value )
{
	if( g_value_get_pointer( value )){
		boxed->private->u.uint_list = g_list_copy( g_value_get_pointer( value ));
	}
}

static void
uint_list_from_void( FMABoxed *boxed, const void *value )
{
	if( value ){
		boxed->private->u.uint_list = g_list_copy(( GList * ) value );
	}
}

static gconstpointer
uint_list_to_pointer( const FMABoxed *boxed )
{
	return(( gconstpointer ) boxed->private->u.uint_list );
}

static gchar *
uint_list_to_string( const FMABoxed *boxed )
{
	GList *is;
	GString *str = g_string_new( "" );
	gboolean first;

	first = TRUE;
	for( is = boxed->private->u.uint_list ; is ; is = is->next ){
		if( !first ){
			str = g_string_append( str, LIST_SEPARATOR );
		}
		g_string_append_printf( str, "%u", GPOINTER_TO_UINT( is->data ));
		first = FALSE;
	}

	return( g_string_free( str, FALSE ));
}

static GList *
uint_list_to_uint_list( const FMABoxed *boxed )
{
	return( g_list_copy( boxed->private->u.uint_list ));
}

static void
uint_list_to_value( const FMABoxed *boxed, GValue *value )
{
	g_value_set_pointer( value, g_list_copy( boxed->private->u.uint_list ));
}

static void *
uint_list_to_void( const FMABoxed *boxed )
{
	void *value = NULL;

	if( boxed->private->u.uint_list ){
		value = g_list_copy( boxed->private->u.uint_list );
	}

	return( value );
}
