/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011 Pierre Wieser and others (see AUTHORS)
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
#include <strings.h>

#include <api/na-boxed.h>
#include <api/na-data-types.h>
#include <api/na-core-utils.h>

/* private class data
 */
struct _NABoxedClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* BoxedDef:
 * the structure which fully defines the behavior of this data type
 */
typedef struct {
	guint            type;
	const gchar     *label;
	int           ( *compare )        ( const NABoxed *, const NABoxed * );
	void          ( *copy )           ( NABoxed *, const NABoxed * );
	gchar       * ( *dump )           ( const NABoxed * );
	void          ( *free )           ( NABoxed * );
	void          ( *from_string )    ( NABoxed *, const gchar * );
	gboolean      ( *get_bool )       ( const NABoxed * );
	gconstpointer ( *get_pointer )    ( const NABoxed * );
	gchar       * ( *get_string )     ( const NABoxed * );
	GSList      * ( *get_string_list )( const NABoxed * );
	guint         ( *get_uint )       ( const NABoxed * );
	GList       * ( *get_uint_list )  ( const NABoxed * );
}
	BoxedDef;

/* private instance data
 */
struct _NABoxedPrivate {
	gboolean        dispose_has_run;
	const BoxedDef *def;
	gboolean        is_set;
	union {
		gboolean    boolean;
		gchar      *string;
		GSList     *string_list;
		void       *pointer;
		guint       uint;
		GList      *uint_list;
	} u;
};

#define LIST_SEPARATOR						";"

static GObjectClass *st_parent_class   = NULL;

static GType           register_type( void );
static void            class_init( NABoxedClass *klass );
static void            instance_init( GTypeInstance *instance, gpointer klass );
static void            instance_dispose( GObject *object );
static void            instance_finalize( GObject *object );

static NABoxed        *boxed_new( const BoxedDef *def );
static const BoxedDef *get_boxed_def( guint type );
static gchar         **string_to_array( const gchar *string );

static int             string_compare( const NABoxed *a, const NABoxed *b );
static void            string_copy( NABoxed *dest, const NABoxed *src );
static gchar          *string_dump( const NABoxed *boxed );
static void            string_free( NABoxed *boxed );
static void            string_from_string( NABoxed *boxed, const gchar *string );
static gchar          *string_get_string( const NABoxed *boxed );

static int             string_list_compare( const NABoxed *a, const NABoxed *b );
static void            string_list_copy( NABoxed *dest, const NABoxed *src );
static gchar          *string_list_dump( const NABoxed *boxed );
static void            string_list_free( NABoxed *boxed );
static void            string_list_from_string( NABoxed *boxed, const gchar *string );
static GSList         *string_list_get_string_list( const NABoxed *boxed );

static int             bool_compare( const NABoxed *a, const NABoxed *b );
static void            bool_copy( NABoxed *dest, const NABoxed *src );
static gchar          *bool_dump( const NABoxed *boxed );
static void            bool_free( NABoxed *boxed );
static void            bool_from_string( NABoxed *boxed, const gchar *string );
static gboolean        bool_get_bool( const NABoxed *boxed );

static int             uint_compare( const NABoxed *a, const NABoxed *b );
static void            uint_copy( NABoxed *dest, const NABoxed *src );
static gchar          *uint_dump( const NABoxed *boxed );
static void            uint_free( NABoxed *boxed );
static void            uint_from_string( NABoxed *boxed, const gchar *string );
static guint           uint_get_uint( const NABoxed *boxed );

static int             uint_list_compare( const NABoxed *a, const NABoxed *b );
static void            uint_list_copy( NABoxed *dest, const NABoxed *src );
static gchar          *uint_list_dump( const NABoxed *boxed );
static void            uint_list_free( NABoxed *boxed );
static void            uint_list_from_string( NABoxed *boxed, const gchar *string );
static GList          *uint_list_get_uint_list( const NABoxed *boxed );

static BoxedDef st_boxed_def[] = {
		{ NA_DATA_TYPE_STRING,
				"string",
				string_compare,
				string_copy,
				string_dump,
				string_free,
				string_from_string,
				NULL,
				NULL,
				string_get_string,
				NULL,
				NULL,
				NULL
				},
		{ NA_DATA_TYPE_STRING_LIST,
				"string_list",
				string_list_compare,
				string_list_copy,
				string_list_dump,
				string_list_free,
				string_list_from_string,
				NULL,
				NULL,
				NULL,
				string_list_get_string_list,
				NULL,
				NULL
				},
		{ NA_DATA_TYPE_BOOLEAN,
				"boolean",
				bool_compare,
				bool_copy,
				bool_dump,
				bool_free,
				bool_from_string,
				bool_get_bool,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL
				},
		{ NA_DATA_TYPE_UINT,
				"uint",
				uint_compare,
				uint_copy,
				uint_dump,
				uint_free,
				uint_from_string,
				NULL,
				NULL,
				NULL,
				NULL,
				uint_get_uint,
				NULL
				},
		{ NA_DATA_TYPE_UINT_LIST,
				"uint_list",
				uint_list_compare,
				uint_list_copy,
				uint_list_dump,
				uint_list_free,
				uint_list_from_string,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				uint_list_get_uint_list
				},
		{ 0 }
};

GType
na_boxed_get_type( void )
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
	static const gchar *thisfn = "na_boxed_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NABoxedClass ),
		NULL,
		NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NABoxed ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "NABoxed", &info, 0 );

	return( type );
}

static void
class_init( NABoxedClass *klass )
{
	static const gchar *thisfn = "na_boxed_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NABoxedClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "na_boxed_instance_init";
	NABoxed *self;

	g_return_if_fail( NA_IS_BOXED( instance ));

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self = NA_BOXED( instance );

	self->private = g_new0( NABoxedPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->def = NULL;
	self->private->is_set = FALSE;
}

static void
instance_dispose( GObject *object )
{
	NABoxed *self;

	g_return_if_fail( NA_IS_BOXED( object ));

	self = NA_BOXED( object );

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
	static const gchar *thisfn = "na_boxed_instance_finalize";
	NABoxed *self;

	g_return_if_fail( NA_IS_BOXED( object ));

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	self = NA_BOXED( object );

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

static NABoxed *
boxed_new( const BoxedDef *def )
{
	NABoxed *boxed;

	boxed = g_object_new( NA_BOXED_TYPE, NULL );
	boxed->private->def = def;

	return( boxed );
}

static const BoxedDef *
get_boxed_def( guint type )
{
	static const gchar *thisfn = "na_boxed_get_boxed_def";
	BoxedDef *def;

	for( def = st_boxed_def ; def->type ; ++def ){
		if( def->type == type ){
			return(( const BoxedDef * ) def );
		}
	}

	g_warning( "%s: unmanaged data type: %d", thisfn, type );
	return( NULL );
}

/*
 * converts a string to an array of string
 * the last separator, if any, is not counted
 */
static gchar **
string_to_array( const gchar *string )
{
	gchar *sdup;
	gchar **array;

	array = NULL;

	if( string && strlen( string )){
		sdup = g_strdup( string );
		if( g_str_has_suffix( string, LIST_SEPARATOR )){
			sdup[strlen(sdup)-1] = '\0';
			sdup = g_strstrip( sdup );
		}
		array = g_strsplit( sdup, LIST_SEPARATOR, -1 );
		g_free( sdup );
	}

	return( array );
}

/**
 * na_boxed_compare:
 * @a: the first #NABoxed object.
 * @b: the second #NABoxed object.
 *
 * Returns:
 *  <itemizedlist>
 *   <listitem>
 *    <para>
 *     -1 if @a is lesser than @b;
 *    </para>
 *   </listitem>
 *   <listitem>
 *    <para>
 *     0 if @a and @b have the same value;
 *    </para>
 *   </listitem>
 *   <listitem>
 *    <para>
 *     +1 if @a is greater than @b.
 *    </para>
 *   </listitem>
 *  </itemizedlist>
 *
 * Also returns zero as an irrelevant value if @a (resp. @b) is not set, or %NULL,
 * or already disposed, or @a and @b do not have the same elementary data type.
 *
 * Since: 3.1.0
 */
int
na_boxed_compare( const NABoxed *a, const NABoxed *b )
{
	int result;

	g_return_val_if_fail( NA_IS_BOXED( a ), 0 );
	g_return_val_if_fail( a->private->dispose_has_run == FALSE, 0 );
	g_return_val_if_fail( NA_IS_BOXED( b ), 0 );
	g_return_val_if_fail( b->private->dispose_has_run == FALSE, 0 );
	g_return_val_if_fail( a->private->def, 0 );
	g_return_val_if_fail( a->private->def == b->private->def, 0 );
	g_return_val_if_fail( a->private->def->compare, 0 );

	result = 0;

	if( a->private->is_set && b->private->is_set ){
		result = ( *a->private->def->compare )( a, b );

	} else if( a->private->is_set && !b->private->is_set ){
		result = 1;

	} else if( !a->private->is_set && b->private->is_set ){
		result = -1;
	}

	return( result );
}

/**
 * na_boxed_copy:
 * @boxed: the source #NABoxed box.
 *
 * Returns: a copy of @boxed, as a newly allocated #NABoxed which should
 * be g_object_unref() by the caller.
 *
 * Since: 3.1.0
 */
NABoxed *
na_boxed_copy( const NABoxed *boxed )
{
	NABoxed *dest;

	g_return_val_if_fail( NA_IS_BOXED( boxed ), NULL );
	g_return_val_if_fail( boxed->private->dispose_has_run == FALSE, NULL );
	g_return_val_if_fail( boxed->private->def, NULL );
	g_return_val_if_fail( boxed->private->def->copy, NULL );

	dest = boxed_new( boxed->private->def );
	if( boxed->private->is_set ){
		( *boxed->private->def->copy )( dest, boxed );
	}

	return( dest );
}

/**
 * na_boxed_dump:
 * @boxed: the #NABoxed box to be dumped.
 *
 * Dumps the @boxed box.
 *
 * Since: 3.1.0
 */
void
na_boxed_dump( const NABoxed *boxed )
{
	static const gchar *thisfn = "na_boxed_dump";
	gchar *str;

	g_return_if_fail( NA_IS_BOXED( boxed ));
	g_return_if_fail( boxed->private->dispose_has_run == FALSE );
	g_return_if_fail( boxed->private->def );
	g_return_if_fail( boxed->private->def->dump );

	str = ( boxed->private->is_set ) ? ( *boxed->private->def->dump )( boxed ) : NULL;
	g_debug( "%s: boxed=%p, type=%u, is_set=%s, value=%s",
			thisfn, ( void * ) boxed, boxed->private->def->type,
			boxed->private->is_set ? "True":"False", str );
	g_free( str );
}

/**
 * na_boxed_new_from_string:
 * @type: the type of the #NABoxed to be allocated.
 * @string: the initial value of the #NABoxed as a string.
 *
 * Allocates a new #NABoxed of the specified @type, and initializes it
 * with @string.
 *
 * If the type is a list, then the last separator is automatically stripped.
 *
 * Returns: a newly allocated #NABoxed, which should be g_object_unref()
 * by the caller, or %NULL if the type is unknowned, or does not provide
 * the 'from_string' function.
 *
 * Since: 3.1.0
 */
NABoxed *
na_boxed_new_from_string( guint type, const gchar *string )
{
	const BoxedDef *def;
	NABoxed *boxed;

	def = get_boxed_def( type );

	g_return_val_if_fail( def, NULL );
	g_return_val_if_fail( def->from_string, NULL );

	boxed = boxed_new( def );
	( *def->from_string )( boxed, string );

	return( boxed );
}

/**
 * na_boxed_get_boolean:
 * @boxed: the #NABoxed structure.
 *
 * Returns: the boolean value if @boxed is of %NA_DATA_TYPE_BOOLEAN type,
 * %FALSE else.
 *
 * Since: 3.1.0
 */
gboolean
na_boxed_get_boolean( const NABoxed *boxed )
{
	gboolean value;

	g_return_val_if_fail( NA_IS_BOXED( boxed ), FALSE );
	g_return_val_if_fail( boxed->private->dispose_has_run == FALSE, FALSE );
	g_return_val_if_fail( boxed->private->def, FALSE );
	g_return_val_if_fail( boxed->private->def->type == NA_DATA_TYPE_BOOLEAN, FALSE );
	g_return_val_if_fail( boxed->private->def->get_bool, FALSE );

	value = ( *boxed->private->def->get_bool )( boxed );

	return( value );
}

/**
 * na_boxed_get_pointer:
 * @boxed: the #NABoxed structure.
 *
 * Returns: a const pointer to the data if @boxed is of %NA_DATA_TYPE_POINTER
 * type, %NULL else.
 *
 * Since: 3.1.0
 */
gconstpointer
na_boxed_get_pointer( const NABoxed *boxed )
{
	gconstpointer value;

	g_return_val_if_fail( NA_IS_BOXED( boxed ), NULL );
	g_return_val_if_fail( boxed->private->dispose_has_run == FALSE, NULL );
	g_return_val_if_fail( boxed->private->def, NULL );
	g_return_val_if_fail( boxed->private->def->type == NA_DATA_TYPE_POINTER, NULL );
	g_return_val_if_fail( boxed->private->def->get_pointer, NULL );

	value = ( *boxed->private->def->get_pointer )( boxed );

	return(( gconstpointer ) value );
}

/**
 * na_boxed_get_string:
 * @boxed: the #NABoxed structure.
 *
 * Returns: a newly allocated string if @boxed is of %NA_DATA_TYPE_STRING
 * type, which should be g_free() by the caller, %NULL else.
 *
 * Since: 3.1.0
 */
gchar *
na_boxed_get_string( const NABoxed *boxed )
{
	gchar *value;

	g_return_val_if_fail( NA_IS_BOXED( boxed ), NULL );
	g_return_val_if_fail( boxed->private->dispose_has_run == FALSE, NULL );
	g_return_val_if_fail( boxed->private->def, NULL );
	g_return_val_if_fail( boxed->private->def->type == NA_DATA_TYPE_STRING, NULL );
	g_return_val_if_fail( boxed->private->def->get_string, NULL );

	value = ( *boxed->private->def->get_string )( boxed );

	return( value );
}

/**
 * na_boxed_get_string_list:
 * @boxed: the #NABoxed structure.
 *
 * Returns: a newly allocated string list if @boxed is of %NA_DATA_TYPE_STRING_LIST
 * type, which should be na_core_utils_slist_free() by the caller, %NULL else.
 *
 * Since: 3.1.0
 */
GSList *
na_boxed_get_string_list( const NABoxed *boxed )
{
	GSList *value;

	g_return_val_if_fail( NA_IS_BOXED( boxed ), NULL );
	g_return_val_if_fail( boxed->private->dispose_has_run == FALSE, NULL );
	g_return_val_if_fail( boxed->private->def, NULL );
	g_return_val_if_fail( boxed->private->def->type == NA_DATA_TYPE_STRING_LIST, NULL );
	g_return_val_if_fail( boxed->private->def->get_string_list, NULL );

	value = ( *boxed->private->def->get_string_list )( boxed );

	return( value );
}

/**
 * na_boxed_get_uint:
 * @boxed: the #NABoxed structure.
 *
 * Returns: an unsigned integer if @boxed is of %NA_DATA_TYPE_UINT type,
 * zero else.
 *
 * Since: 3.1.0
 */
guint
na_boxed_get_uint( const NABoxed *boxed )
{
	guint value;

	g_return_val_if_fail( NA_IS_BOXED( boxed ), 0 );
	g_return_val_if_fail( boxed->private->dispose_has_run == FALSE, 0 );
	g_return_val_if_fail( boxed->private->def, 0 );
	g_return_val_if_fail( boxed->private->def->type == NA_DATA_TYPE_UINT, 0 );
	g_return_val_if_fail( boxed->private->def->get_uint, 0 );

	value = ( *boxed->private->def->get_uint )( boxed );

	return( value );
}

/**
 * na_boxed_get_uint_list:
 * @boxed: the #NABoxed structure.
 *
 * Returns: a newly allocated list if @boxed is of %NA_DATA_TYPE_UINT_LIST
 * type, which should be g_list_free() by the caller, %FALSE else.
 *
 * Since: 3.1.0
 */
GList *
na_boxed_get_uint_list( const NABoxed *boxed )
{
	GList *value;

	g_return_val_if_fail( NA_IS_BOXED( boxed ), NULL );
	g_return_val_if_fail( boxed->private->dispose_has_run == FALSE, NULL );
	g_return_val_if_fail( boxed->private->def, NULL );
	g_return_val_if_fail( boxed->private->def->type == NA_DATA_TYPE_UINT_LIST, NULL );
	g_return_val_if_fail( boxed->private->def->get_uint_list, NULL );

	value = ( *boxed->private->def->get_uint_list )( boxed );

	return( value );
}

static int
string_compare( const NABoxed *a, const NABoxed *b )
{
	return( strcmp( a->private->u.string, b->private->u.string ));
}

static void
string_copy( NABoxed *dest, const NABoxed *src )
{
	if( dest->private->is_set ){
		string_free( dest );
	}
	dest->private->u.string = g_strdup( src->private->u.string );
	dest->private->is_set = TRUE;
}

static gchar *
string_dump( const NABoxed *boxed )
{
	return( g_strdup( boxed->private->u.string ));
}

static void
string_free( NABoxed *boxed )
{
	g_free( boxed->private->u.string );
	boxed->private->u.string = NULL;
	boxed->private->is_set = FALSE;
}

static void
string_from_string( NABoxed *boxed, const gchar *string )
{
	if( boxed->private->is_set ){
		string_free( boxed );
	}
	boxed->private->u.string = string ? g_strdup( string ) : NULL;
	boxed->private->is_set = TRUE;
}

static gchar *
string_get_string( const NABoxed *boxed )
{
	return( g_strdup( boxed->private->u.string ));
}

/* the two string lists are equal if they have the same elements in the
 * same order
 * if not, we compare the length of the lists
 *
 * don't know what to say for two lists which have the same count of elements,
 * but different contents; just arbitrarily return -1
 */
static int
string_list_compare( const NABoxed *a, const NABoxed *b )
{
	GSList *ia, *ib;
	gboolean diff = FALSE;

	guint na = g_slist_length( a->private->u.string_list );
	guint nb = g_slist_length( b->private->u.string_list );

	if( na < nb ) return -1;
	if( na > nb ) return  1;

	for( ia=a->private->u.string_list, ib=b->private->u.string_list ; ia && ib && !diff ; ia=ia->next, ib=ib->next ){
		if( strcmp( ia->data, ib->data ) != 0 ){
			diff = TRUE;
		}
	}

	return( diff ? -1 : 0 );
}

static void
string_list_copy( NABoxed *dest, const NABoxed *src )
{
	if( dest->private->is_set ){
		string_list_free( dest );
	}
	dest->private->u.string_list = na_core_utils_slist_duplicate( src->private->u.string_list );
	dest->private->is_set = TRUE;
}

static gchar *
string_list_dump( const NABoxed *boxed )
{
	return( na_core_utils_slist_join_at_end( boxed->private->u.string_list, LIST_SEPARATOR ));
}

static void
string_list_free( NABoxed *boxed )
{
	na_core_utils_slist_free( boxed->private->u.string_list );
	boxed->private->u.string_list = NULL;
	boxed->private->is_set = FALSE;
}

static void
string_list_from_string( NABoxed *boxed, const gchar *string )
{
	gchar **array;
	gchar **i;

	if( boxed->private->is_set ){
		string_list_free( boxed );
	}

	array = string_to_array( string );

	if( array ){
		i = ( gchar ** ) array;
		while( *i ){
			boxed->private->u.string_list = g_slist_prepend( boxed->private->u.string_list, g_strdup( *i ));
			i++;
		}
		boxed->private->u.string_list = g_slist_reverse( boxed->private->u.string_list );
	} else {
		boxed->private->u.string_list = NULL;
	}

	g_strfreev( array );
	boxed->private->is_set = TRUE;
}

static GSList *
string_list_get_string_list( const NABoxed *boxed )
{
	return( na_core_utils_slist_duplicate( boxed->private->u.string_list ));
}

/* assume that FALSE < TRUE
 */
static int
bool_compare( const NABoxed *a, const NABoxed *b )
{
	if( a->private->u.boolean == b->private->u.boolean ){
		return( 0 );
	}
	if( !a->private->u.boolean ){
		return( -1 );
	}
	return( 1 );
}

static void
bool_copy( NABoxed *dest, const NABoxed *src )
{
	dest->private->u.boolean = src->private->u.boolean;
	dest->private->is_set = TRUE;
}

static gchar *
bool_dump( const NABoxed *boxed )
{
	return( g_strdup( boxed->private->u.boolean ? "True":"False" ));
}

static void
bool_free( NABoxed *boxed )
{
	boxed->private->u.boolean = FALSE;
	boxed->private->is_set = FALSE;
}

static void
bool_from_string( NABoxed *boxed, const gchar *string )
{
	if( boxed->private->is_set ){
		bool_free( boxed );
	}
	boxed->private->u.boolean = ( string ? ( strcasecmp( string, "true" ) == 0 || atoi( string ) != 0 ) : FALSE );
	boxed->private->is_set = TRUE;
}

static gboolean
bool_get_bool( const NABoxed *boxed )
{
	return( boxed->private->u.boolean );
}

static int
uint_compare( const NABoxed *a, const NABoxed *b )
{
	if( a->private->u.uint < b->private->u.uint ) return -1;
	if( a->private->u.uint > b->private->u.uint ) return  1;
	return( 0 );
}

static void
uint_copy( NABoxed *dest, const NABoxed *src )
{
	dest->private->u.uint = src->private->u.uint;
	dest->private->is_set = TRUE;
}

static gchar *
uint_dump( const NABoxed *boxed )
{
	return( g_strdup_printf( "%u", boxed->private->u.uint ));
}

static void
uint_free( NABoxed *boxed )
{
	boxed->private->u.uint = 0;
	boxed->private->is_set = FALSE;
}

static void
uint_from_string( NABoxed *boxed, const gchar *string )
{
	if( boxed->private->is_set ){
		uint_free( boxed );
	}
	boxed->private->u.uint = string ? atoi( string ) : 0;
	boxed->private->is_set = TRUE;
}

static guint
uint_get_uint( const NABoxed *boxed )
{
	return( boxed->private->u.uint );
}

/* compare uint list as string list:
 * if the two list do not have the same count, then one is lesser than the other
 * if they have same count and same elements in same order, they are equal
 * else just arbitrarily return -1
 */
static int
uint_list_compare( const NABoxed *a, const NABoxed *b )
{
	GList *ia, *ib;
	gboolean diff = FALSE;

	guint na = g_list_length( a->private->u.uint_list );
	guint nb = g_list_length( b->private->u.uint_list );

	if( na < nb ) return -1;
	if( na > nb ) return  1;

	for( ia=a->private->u.uint_list, ib=b->private->u.uint_list ; ia && ib && !diff ; ia=ia->next, ib=ib->next ){
		if( GPOINTER_TO_UINT( ia->data ) != GPOINTER_TO_UINT( ib->data )){
			diff = TRUE;
		}
	}

	return( diff ? -1 : 0 );
}

static void
uint_list_copy( NABoxed *dest, const NABoxed *src )
{
	GList *isrc;

	if( dest->private->is_set ){
		uint_list_free( dest );
	}
	dest->private->u.uint_list = NULL;
	for( isrc = src->private->u.uint_list ; isrc ; isrc = isrc->next ){
		dest->private->u.uint_list = g_list_prepend( dest->private->u.uint_list, isrc->data );
	}
	dest->private->u.uint_list = g_list_reverse( dest->private->u.uint_list );
	dest->private->is_set = TRUE;
}

static gchar *
uint_list_dump( const NABoxed *boxed )
{
	GString *str;
	GList *i;

	str = g_string_new( "" );
	for( i = boxed->private->u.uint_list ; i ; i = i->next ){
		if( strlen( str->str )){
			str = g_string_append( str, LIST_SEPARATOR );
		}
		g_string_append_printf( str, "%u", GPOINTER_TO_UINT( i->data ));
	}

	return( g_string_free( str, FALSE ));
}

static void
uint_list_free( NABoxed *boxed )
{
	g_list_free( boxed->private->u.uint_list );
	boxed->private->u.uint_list = NULL;
	boxed->private->is_set = FALSE;
}

static void
uint_list_from_string( NABoxed *boxed, const gchar *string )
{
	gchar **array;
	gchar **i;

	if( boxed->private->is_set ){
		uint_list_free( boxed );
	}

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
	boxed->private->is_set = TRUE;
}

static GList *
uint_list_get_uint_list( const NABoxed *boxed )
{
	return( g_list_copy( boxed->private->u.uint_list ));
}
