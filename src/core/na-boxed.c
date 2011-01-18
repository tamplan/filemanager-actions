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
#include <api/na-core-utils.h>

/* private structure data
 */
struct _NABoxed {
	guint    type;
	gboolean is_set;
	union {
		gboolean  boolean;
		gchar    *string;
		GSList   *string_list;
		void     *pointer;
		guint     uint;
		GList    *uint_list;
	} u;
};

typedef struct {
	guint        type;
	const gchar *label;
	int       ( *compare )        ( const NABoxed *, const NABoxed * );
	void      ( *copy )           ( NABoxed *, const NABoxed * );
	void      ( *free )           ( NABoxed * );
	void      ( *from_string )    ( NABoxed *, const gchar * );
	void      ( *from_array )     ( NABoxed *, const gchar ** );
	gboolean  ( *get_bool )       ( const NABoxed * );
	gpointer  ( *get_pointer )    ( const NABoxed * );
	gchar   * ( *get_string )     ( const NABoxed * );
	GSList  * ( *get_string_list )( const NABoxed * );
	guint     ( *get_uint )       ( const NABoxed * );
	GList   * ( *get_uint_list )  ( const NABoxed * );
}
	BoxedDef;

static NABoxed        *boxed_new( void );
static const BoxedDef *get_boxed_def( guint type );
static const gchar    *get_type_label( guint type );

static int      string_compare( const NABoxed *a, const NABoxed *b );
static void     string_copy( NABoxed *dest, const NABoxed *src );
static void     string_free( NABoxed *boxed );
static void     string_from_string( NABoxed *boxed, const gchar *string );
static gpointer string_get_pointer( const NABoxed *boxed );
static gchar   *string_get_string( const NABoxed *boxed );

static int      string_list_compare( const NABoxed *a, const NABoxed *b );
static void     string_list_copy( NABoxed *dest, const NABoxed *src );
static void     string_list_free( NABoxed *boxed );
static void     string_list_from_string( NABoxed *boxed, const gchar *string );
static void     string_list_from_array( NABoxed *boxed, const gchar **array );
static gpointer string_list_get_pointer( const NABoxed *boxed );
static GSList  *string_list_get_string_list( const NABoxed *boxed );

static int      bool_compare( const NABoxed *a, const NABoxed *b );
static void     bool_copy( NABoxed *dest, const NABoxed *src );
static void     bool_free( NABoxed *boxed );
static void     bool_from_string( NABoxed *boxed, const gchar *string );
static gboolean bool_get_bool( const NABoxed *boxed );
static gpointer bool_get_pointer( const NABoxed *boxed );

static int      uint_compare( const NABoxed *a, const NABoxed *b );
static void     uint_copy( NABoxed *dest, const NABoxed *src );
static void     uint_free( NABoxed *boxed );
static void     uint_from_string( NABoxed *boxed, const gchar *string );
static gpointer uint_get_pointer( const NABoxed *boxed );
static guint    uint_get_uint( const NABoxed *boxed );

static int      uint_list_compare( const NABoxed *a, const NABoxed *b );
static void     uint_list_copy( NABoxed *dest, const NABoxed *src );
static void     uint_list_free( NABoxed *boxed );
static void     uint_list_from_string( NABoxed *boxed, const gchar *string );
static void     uint_list_from_array( NABoxed *boxed, const gchar **array );
static gpointer uint_list_get_pointer( const NABoxed *boxed );
static GList   *uint_list_get_uint_list( const NABoxed *boxed );

static BoxedDef st_boxed_def[] = {
		{ NA_BOXED_TYPE_STRING,
				"string",
				string_compare,
				string_copy,
				string_free,
				string_from_string,
				NULL,
				NULL,
				string_get_pointer,
				string_get_string,
				NULL,
				NULL,
				NULL
				},
		{ NA_BOXED_TYPE_STRING_LIST,
				"ascii strings list",
				string_list_compare,
				string_list_copy,
				string_list_free,
				string_list_from_string,
				string_list_from_array,
				NULL,
				string_list_get_pointer,
				NULL,
				string_list_get_string_list,
				NULL,
				NULL
				},
		{ NA_BOXED_TYPE_BOOLEAN,
				"boolean",
				bool_compare,
				bool_copy,
				bool_free,
				bool_from_string,
				NULL,
				bool_get_bool,
				bool_get_pointer,
				NULL,
				NULL,
				NULL,
				NULL
				},
		{ NA_BOXED_TYPE_UINT,
				"unsigned integer",
				uint_compare,
				uint_copy,
				uint_free,
				uint_from_string,
				NULL,
				NULL,
				uint_get_pointer,
				NULL,
				NULL,
				uint_get_uint,
				NULL
				},
		{ NA_BOXED_TYPE_UINT_LIST,
				"unsigned integers list",
				uint_list_compare,
				uint_list_copy,
				uint_list_free,
				uint_list_from_string,
				uint_list_from_array,
				NULL,
				uint_list_get_pointer,
				NULL,
				NULL,
				NULL,
				uint_list_get_uint_list
				},
		{ 0 }
};

static NABoxed *
boxed_new( void )
{
	NABoxed *boxed = g_new0( NABoxed, 1 );

	g_debug( "na_boxed_new: boxed=%p", ( void * ) boxed );

	return( boxed );
}

static const BoxedDef *
get_boxed_def( guint type )
{
	static const gchar *thisfn = "na_boxed_get_boxed_def";
	BoxedDef *def;

	def = st_boxed_def;
	while( def->type ){
		if( def->type == type ){
			return( def );
		}
		def++;
	}

	g_warning( "%s: unmanaged NABoxed type: %d", thisfn, type );
	return( NULL );
}

/*
 * get_type_label:
 * @type: the #NABoxed type.
 *
 * Returns: the label of this type.
 *
 * The returned label is owned by the data factory management system, and
 * should not be released by the caller.
 *
 * Since: 3.1.0
 */
static const gchar *
get_type_label( guint type )
{
	static const gchar *thisfn = "na_boxed_get_type_label";
	BoxedDef *def;

	def = st_boxed_def;
	while( def->type ){
		if( def->type == type ){
			return( def->label );
		}
		def++;
	}

	g_warning( "%s: unmanaged NABoxed type: %d", thisfn, type );
	return( NULL );
}

/**
 * na_boxed_compare:
 * @a: the first #NABoxed object.
 * @b: the second #NABoxed object.
 *
 * Returns: -1 if @a is lesser than @b, 0 if @a and @b have the same value,
 * 1 if @a is greater than @b.
 *
 * Since: 3.1.0
 */
int
na_boxed_compare( const NABoxed *a, const NABoxed *b )
{
	static const gchar *thisfn = "na_boxed_compare";
	const BoxedDef *def;
	int result;

	result = 0;

	if( a->type != b->type ){
		g_warning( "%s: unable to compare: a is of type '%s' while b is of type %s",
				thisfn, get_type_label( a->type ), get_type_label( b->type ));

	} else if( a->is_set && !b->is_set ){
		result = 1;

	} else if( !a->is_set && b->is_set ){
		result = -1;

	} else if( a->is_set && b->is_set ){
		def = get_boxed_def( a->type );
		if( def ){
			if( def->compare ){
				result = ( *def->compare )( a, b );
			} else {
				g_warning( "%s: unable to compare: '%s' type does not provide 'compare' function",
						thisfn, def->label );
			}
		}
	}

	return( result );
}

/**
 * na_boxed_copy:
 * @boxed: the source #NABoxed box.
 *
 * Returns: a copy of @boxed, as a newly allocated #NABoxed which should
 * be na_boxed_free() by the caller.
 *
 * Since: 3.1.0
 */
NABoxed *
na_boxed_copy( const NABoxed *boxed )
{
	static const gchar *thisfn = "na_boxed_copy";
	NABoxed *dest;
	const BoxedDef *def;

	dest = NULL;
	def = get_boxed_def( boxed->type );
	if( def ){
		if( def->copy ){
			dest = boxed_new();
			dest->type = boxed->type;
			dest->is_set = FALSE;
			if( boxed->is_set ){
				( *def->copy )( dest, boxed );
			}
		} else {
			g_warning( "%s: unable to copy: '%s' type does not provide 'copy' function",
					thisfn, def->label );
		}
	}

	return( dest );
}

/**
 * na_boxed_free:
 * @boxed: the #NABoxed to be released.
 *
 * Free the memory associated with @boxed.
 *
 * If the data type is not managed, of no free() function is registered
 * for it, then the @boxed is left unchanged.
 *
 * Since: 3.1.0
 */
void
na_boxed_free( NABoxed *boxed )
{
	static const gchar *thisfn = "na_boxed_free";
	const BoxedDef *def;

	if( boxed ){
		def = get_boxed_def( boxed->type );
		if( def ){
			if( def->free ){
				( *def->free )( boxed );
				g_debug( "na_boxed_free: boxed=%p", ( void * ) boxed );
				g_free( boxed );
			} else {
				g_warning( "%s: unable to free the content: '%s' type does not provide 'free' function",
						thisfn, def->label );
			}
		}
	}
}

/**
 * na_boxed_new_from_string:
 * @type: the type of the #NABoxed to be allocated.
 * @string: the initial value of the #NABoxed as a string.
 *
 * Allocates a new #NABoxed of the specified @type, and initializes it
 * with @string.
 *
 * Returns: a newly allocated #NABoxed, which should be na_boxed_free()
 * by the caller.
 *
 * Since: 3.1.0
 */
NABoxed *
na_boxed_new_from_string( guint type, const gchar *string )
{
	static const gchar *thisfn = "na_boxed_new_from_string";
	const BoxedDef *def;
	NABoxed *boxed;

	boxed = NULL;
	def = get_boxed_def( type );
	if( def ){
		if( def->from_string ){
			boxed = boxed_new();
			boxed->type = type;
			( *def->from_string )( boxed, string );
		} else {
			g_warning( "%s: unable to initialize the content: '%s' type does not provide 'from_string' function",
					thisfn, def->label );
		}
	}

	return( boxed );
}

/**
 * na_boxed_new_from_string_with_sep:
 * @type: the type of the #NABoxed to be allocated.
 * @string: the initial value of the #NABoxed as a string.
 * @sep: the separator.
 *
 * Allocates a new #NABoxed of the specified @type, and initializes it
 * with @string.
 *
 * This function is rather oriented to list types.
 *
 * Returns: a newly allocated #NABoxed, which should be na_boxed_free()
 * by the caller.
 *
 * Since: 3.1.0
 */
NABoxed *
na_boxed_new_from_string_with_sep( guint type, const gchar *string, const gchar *sep )
{
	static const gchar *thisfn = "na_boxed_new_from_string_with_sep";
	const BoxedDef *def;
	NABoxed *boxed;
	gchar *sdup;
	gchar **array;

	boxed = NULL;

	def = get_boxed_def( type );
	if( def ){
		if( def->from_array ){
			boxed = boxed_new();
			boxed->type = type;
			if( string && strlen( string )){
				sdup = g_strdup( string );
				if( g_str_has_suffix( string, sep )){
					sdup[strlen(sdup)-strlen(sep)] = '\0';
					sdup = g_strstrip( sdup );
				}
				array = g_strsplit( sdup, sep, -1 );
				( *def->from_array )( boxed, ( const gchar ** ) array );
				g_strfreev( array );
				g_free( sdup );
			}
		} else {
			g_warning( "%s: unable to initialize the content: '%s' type does not provide 'from_array' function",
					thisfn, def->label );
		}
	}

	return( boxed );
}

/**
 * na_boxed_get_boolean:
 * @boxed: the #NABoxed structure.
 *
 * Returns: the boolean value if @boxed is of %NA_BOXED_TYPE_BOOLEAN type,
 * %FALSE else.
 *
 * Since: 3.1.0
 */
gboolean
na_boxed_get_boolean( const NABoxed *boxed )
{
	static const gchar *thisfn = "na_boxed_get_boolean";
	const BoxedDef *def;
	gboolean value;

	value = FALSE;
	if( boxed && boxed->type == NA_BOXED_TYPE_BOOLEAN && boxed->is_set ){
		def = get_boxed_def( boxed->type );
		if( def ){
			if( def->get_bool ){
				value = ( *def->get_bool )( boxed );
			} else {
				g_warning( "%s: unable to get the value: '%s' type does not provide 'get_bool' function",
						thisfn, def->label );
			}
		}
	}

	return( value );
}

/**
 * na_boxed_get_pointer:
 * @boxed: the #NABoxed structure.
 *
 * Returns: a const pointer to the raw data.
 *
 * Since: 3.1.0
 */
gconstpointer
na_boxed_get_pointer( const NABoxed *boxed )
{
	static const gchar *thisfn = "na_boxed_get_pointer";
	const BoxedDef *def;
	gpointer value;

	value = NULL;
	if( boxed && boxed->is_set ){
		def = get_boxed_def( boxed->type );
		if( def ){
			if( def->get_pointer ){
				value = ( *def->get_pointer )( boxed );
			} else {
				g_warning( "%s: unable to get the value: '%s' type does not provide 'get_pointer' function",
						thisfn, def->label );
			}
		}
	}

	return(( gconstpointer ) value );
}

/**
 * na_boxed_get_string:
 * @boxed: the #NABoxed structure.
 *
 * Returns: a newly allocated string if @boxed is of %NA_BOXED_TYPE_STRING
 * type, which should be g_free() by the caller, %FALSE else.
 *
 * Since: 3.1.0
 */
gchar *
na_boxed_get_string( const NABoxed *boxed )
{
	static const gchar *thisfn = "na_boxed_get_string";
	const BoxedDef *def;
	gchar *value;

	value = NULL;
	if( boxed && boxed->type == NA_BOXED_TYPE_STRING && boxed->is_set ){
		def = get_boxed_def( boxed->type );
		if( def ){
			if( def->get_string ){
				value = ( *def->get_string )( boxed );
			} else {
				g_warning( "%s: unable to get the value: '%s' type does not provide 'get_string' function",
						thisfn, def->label );
			}
		}
	}

	return( value );
}

/**
 * na_boxed_get_string_list:
 * @boxed: the #NABoxed structure.
 *
 * Returns: a newly allocated string list if @boxed is of %NA_BOXED_TYPE_STRING_LIST
 * type, which should be na_core_utils_slist_free() by the caller, %FALSE else.
 *
 * Since: 3.1.0
 */
GSList *
na_boxed_get_string_list( const NABoxed *boxed )
{
	static const gchar *thisfn = "na_boxed_get_string_list";
	const BoxedDef *def;
	GSList *value;

	value = NULL;
	if( boxed && boxed->type == NA_BOXED_TYPE_STRING_LIST && boxed->is_set ){
		def = get_boxed_def( boxed->type );
		if( def ){
			if( def->get_string_list ){
				value = ( *def->get_string_list )( boxed );
			} else {
				g_warning( "%s: unable to get the value: '%s' type does not provide 'get_string_list' function",
						thisfn, def->label );
			}
		}
	}

	return( value );
}

/**
 * na_boxed_get_uint:
 * @boxed: the #NABoxed structure.
 *
 * Returns: an unsigned integer if @boxed is of %NA_BOXED_TYPE_UINT type.
 *
 * Since: 3.1.0
 */
guint
na_boxed_get_uint( const NABoxed *boxed )
{
	static const gchar *thisfn = "na_boxed_get_uint";
	const BoxedDef *def;
	guint value;

	value = 0;
	if( boxed && boxed->type == NA_BOXED_TYPE_UINT && boxed->is_set ){
		def = get_boxed_def( boxed->type );
		if( def ){
			if( def->get_uint ){
				value = ( *def->get_uint )( boxed );
			} else {
				g_warning( "%s: unable to get the value: '%s' type does not provide 'get_uint' function",
						thisfn, def->label );
			}
		}
	}

	return( value );
}

/**
 * na_boxed_get_uint_list:
 * @boxed: the #NABoxed structure.
 *
 * Returns: a newly allocated list if @boxed is of %NA_BOXED_TYPE_UINT_LIST
 * type, which should be g_list_free() by the caller, %FALSE else.
 *
 * Since: 3.1.0
 */
GList *
na_boxed_get_uint_list( const NABoxed *boxed )
{
	static const gchar *thisfn = "na_boxed_get_uint_list";
	const BoxedDef *def;
	GList *value;

	value = NULL;
	if( boxed && boxed->type == NA_BOXED_TYPE_UINT_LIST && boxed->is_set ){
		def = get_boxed_def( boxed->type );
		if( def ){
			if( def->get_uint_list ){
				value = ( *def->get_uint_list )( boxed );
			} else {
				g_warning( "%s: unable to get the value: '%s' type does not provide 'get_uint_list' function",
						thisfn, def->label );
			}
		}
	}

	return( value );
}

static int
string_compare( const NABoxed *a, const NABoxed *b )
{
	return( strcmp( a->u.string, b->u.string ));
}

static void
string_copy( NABoxed *dest, const NABoxed *src )
{
	if( dest->is_set ){
		string_free( dest );
	}
	dest->u.string = g_strdup( src->u.string );
	dest->is_set = TRUE;
}

static void
string_free( NABoxed *boxed )
{
	g_free( boxed->u.string );
	boxed->u.string = NULL;
	boxed->is_set = FALSE;
}

static void
string_from_string( NABoxed *boxed, const gchar *string )
{
	if( boxed->is_set ){
		string_free( boxed );
	}
	boxed->u.string = string ? g_strdup( string ) : NULL;
	boxed->is_set = TRUE;
}

static gpointer
string_get_pointer( const NABoxed *boxed )
{
	return( boxed->u.string );
}

static gchar *
string_get_string( const NABoxed *boxed )
{
	return( g_strdup( boxed->u.string ));
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

	guint na = g_slist_length( a->u.string_list );
	guint nb = g_slist_length( b->u.string_list );

	if( na < nb ) return -1;
	if( na > nb ) return  1;

	for( ia=a->u.string_list, ib=b->u.string_list ; ia && ib && !diff ; ia=ia->next, ib=ib->next ){
		if( strcmp( ia->data, ib->data ) != 0 ){
			diff = TRUE;
		}
	}

	return( diff ? -1 : 0 );
}

static void
string_list_copy( NABoxed *dest, const NABoxed *src )
{
	if( dest->is_set ){
		string_list_free( dest );
	}
	dest->u.string_list = na_core_utils_slist_duplicate( src->u.string_list );
	dest->is_set = TRUE;
}

static void
string_list_free( NABoxed *boxed )
{
	na_core_utils_slist_free( boxed->u.string_list );
	boxed->u.string_list = NULL;
	boxed->is_set = FALSE;
}

static void
string_list_from_string( NABoxed *boxed, const gchar *string )
{
	if( boxed->is_set ){
		string_list_free( boxed );
	}
	boxed->u.string_list = string ? g_slist_append( NULL, g_strdup( string )) : NULL;
	boxed->is_set = TRUE;
}

static void
string_list_from_array( NABoxed *boxed, const gchar **array )
{
	gchar **i;

	if( boxed->is_set ){
		string_list_free( boxed );
	}
	if( array ){
		i = ( gchar ** ) array;
		while( *i ){
			boxed->u.string_list = g_slist_prepend( boxed->u.string_list, g_strdup( *i ));
			i++;
		}
		boxed->u.string_list = g_slist_reverse( boxed->u.string_list );
	} else {
		boxed->u.string_list = NULL;
	}
	boxed->is_set = TRUE;
}

static gpointer
string_list_get_pointer( const NABoxed *boxed )
{
	return( boxed->u.string_list );
}

static GSList *
string_list_get_string_list( const NABoxed *boxed )
{
	return( na_core_utils_slist_duplicate( boxed->u.string_list ));
}

/* don't know how to compare two booleans
 * just say if they are equal or not
 */
static int
bool_compare( const NABoxed *a, const NABoxed *b )
{
	return( a->u.boolean == b->u.boolean ? 0 : 1 );
}

static void
bool_copy( NABoxed *dest, const NABoxed *src )
{
	dest->u.boolean = src->u.boolean;
	dest->is_set = TRUE;
}

static void
bool_free( NABoxed *boxed )
{
	boxed->u.boolean = FALSE;
	boxed->is_set = FALSE;
}

static void
bool_from_string( NABoxed *boxed, const gchar *string )
{
	if( boxed->is_set ){
		bool_free( boxed );
	}
	boxed->u.boolean = ( string ? ( strcasecmp( string, "true" ) == 0 || atoi( string ) != 0 ) : FALSE );
	boxed->is_set = TRUE;
}

static gboolean
bool_get_bool( const NABoxed *boxed )
{
	return( boxed->u.boolean );
}

static gpointer
bool_get_pointer( const NABoxed *boxed )
{
	return( GUINT_TO_POINTER( boxed->u.boolean ));
}

static int
uint_compare( const NABoxed *a, const NABoxed *b )
{
	if( a->u.uint < b->u.uint ) return -1;
	if( a->u.uint > b->u.uint ) return  1;
	return( 0 );
}

static void
uint_copy( NABoxed *dest, const NABoxed *src )
{
	dest->u.uint = src->u.uint;
	dest->is_set = TRUE;
}

static void
uint_free( NABoxed *boxed )
{
	boxed->u.uint = FALSE;
	boxed->is_set = FALSE;
}

static void
uint_from_string( NABoxed *boxed, const gchar *string )
{
	if( boxed->is_set ){
		uint_free( boxed );
	}
	boxed->u.uint = string ? atoi( string ) : 0;
	boxed->is_set = TRUE;
}

static gpointer
uint_get_pointer( const NABoxed *boxed )
{
	return( GUINT_TO_POINTER( boxed->u.uint ));
}

static guint
uint_get_uint( const NABoxed *boxed )
{
	return( boxed->u.uint );
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

	guint na = g_list_length( a->u.uint_list );
	guint nb = g_list_length( b->u.uint_list );

	if( na < nb ) return -1;
	if( na > nb ) return  1;

	for( ia=a->u.uint_list, ib=b->u.uint_list ; ia && ib && !diff ; ia=ia->next, ib=ib->next ){
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

	if( dest->is_set ){
		uint_list_free( dest );
	}
	dest->u.uint_list = NULL;
	for( isrc = src->u.uint_list ; isrc ; isrc = isrc->next ){
		dest->u.uint_list = g_list_prepend( dest->u.uint_list, isrc->data );
	}
	dest->u.uint_list = g_list_reverse( dest->u.uint_list );
	dest->is_set = TRUE;
}

static void
uint_list_free( NABoxed *boxed )
{
	g_list_free( boxed->u.uint_list );
	boxed->u.uint_list = NULL;
	boxed->is_set = FALSE;
}

static void
uint_list_from_string( NABoxed *boxed, const gchar *string )
{
	if( boxed->is_set ){
		uint_list_free( boxed );
	}
	boxed->u.uint_list = string ? g_list_append( NULL, GINT_TO_POINTER( atoi( string ))) : NULL;
	boxed->is_set = TRUE;
}

static void
uint_list_from_array( NABoxed *boxed, const gchar **array )
{
	gchar **i;

	if( boxed->is_set ){
		uint_list_free( boxed );
	}
	if( array ){
		i = ( gchar ** ) array;
		while( *i ){
			boxed->u.uint_list = g_list_prepend( boxed->u.uint_list, GINT_TO_POINTER( atoi( *i )));
			i++;
		}
		boxed->u.uint_list = g_list_reverse( boxed->u.uint_list );
	} else {
		boxed->u.uint_list = NULL;
	}
	boxed->is_set = TRUE;
}

static gpointer
uint_list_get_pointer( const NABoxed *boxed )
{
	return( boxed->u.uint_list );
}

static GList *
uint_list_get_uint_list( const NABoxed *boxed )
{
	return( g_list_copy( boxed->u.uint_list ));
}
