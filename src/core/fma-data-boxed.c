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

#include <libintl.h>
#include <stdlib.h>
#include <string.h>

#include <api/fma-core-utils.h>
#include <api/fma-gconf-utils.h>
#include <api/fma-data-def.h>
#include <api/fma-data-types.h>
#include <api/fma-data-boxed.h>

/* private class data
 */
struct _FMADataBoxedClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* additional features of our data types
 * (see FMABoxed class for primary features)
 */
typedef struct {
	guint           type;
	GParamSpec * ( *spec )      ( const FMADataDef * );
	gboolean     ( *is_default )( const FMADataBoxed * );
	gboolean     ( *is_valid )  ( const FMADataBoxed * );
}
	DataBoxedDef;

/* private instance data
 */
struct _FMADataBoxedPrivate {
	gboolean            dispose_has_run;
	const FMADataDef   *data_def ;
	const DataBoxedDef *boxed_def;
};

static GObjectClass *st_parent_class   = NULL;

static GType               register_type( void );
static void                class_init( FMADataBoxedClass *klass );
static void                instance_init( GTypeInstance *instance, gpointer klass );
static void                instance_dispose( GObject *object );
static void                instance_finalize( GObject *object );

static const DataBoxedDef *get_data_boxed_def( guint type );

static GParamSpec         *bool_spec( const FMADataDef *idtype );
static gboolean            bool_is_default( const FMADataBoxed *boxed );
static gboolean            bool_is_valid( const FMADataBoxed *boxed );

static GParamSpec         *pointer_spec( const FMADataDef *idtype );
static gboolean            pointer_is_default( const FMADataBoxed *boxed );
static gboolean            pointer_is_valid( const FMADataBoxed *boxed );

static GParamSpec         *string_spec( const FMADataDef *idtype );
static gboolean            string_is_default( const FMADataBoxed *boxed );
static gboolean            string_is_valid( const FMADataBoxed *boxed );

static GParamSpec         *string_list_spec( const FMADataDef *idtype );
static gboolean            string_list_is_default( const FMADataBoxed *boxed );
static gboolean            string_list_is_valid( const FMADataBoxed *boxed );

static gboolean            locale_is_default( const FMADataBoxed *boxed );
static gboolean            locale_is_valid( const FMADataBoxed *boxed );

static GParamSpec         *uint_spec( const FMADataDef *idtype );
static gboolean            uint_is_default( const FMADataBoxed *boxed );
static gboolean            uint_is_valid( const FMADataBoxed *boxed );

static GParamSpec         *uint_list_spec( const FMADataDef *idtype );
static gboolean            uint_list_is_default( const FMADataBoxed *boxed );
static gboolean            uint_list_is_valid( const FMADataBoxed *boxed );

static DataBoxedDef st_data_boxed_def[] = {
		{ FMA_DATA_TYPE_BOOLEAN,
				bool_spec,
				bool_is_default,
				bool_is_valid
				},
		{ FMA_DATA_TYPE_POINTER,
				pointer_spec,
				pointer_is_default,
				pointer_is_valid
				},
		{ FMA_DATA_TYPE_STRING,
				string_spec,
				string_is_default,
				string_is_valid
				},
		{ FMA_DATA_TYPE_STRING_LIST,
				string_list_spec,
				string_list_is_default,
				string_list_is_valid
				},
		{ FMA_DATA_TYPE_LOCALE_STRING,
				string_spec,
				locale_is_default,
				locale_is_valid
				},
		{ FMA_DATA_TYPE_UINT,
				uint_spec,
				uint_is_default,
				uint_is_valid
				},
		{ FMA_DATA_TYPE_UINT_LIST,
				uint_list_spec,
				uint_list_is_default,
				uint_list_is_valid
				},
		{ 0 }
};

GType
fma_data_boxed_get_type( void )
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
	static const gchar *thisfn = "fma_data_boxed_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( FMADataBoxedClass ),
		NULL,
		NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( FMADataBoxed ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( FMA_TYPE_BOXED, "FMADataBoxed", &info, 0 );

	return( type );
}

static void
class_init( FMADataBoxedClass *klass )
{
	static const gchar *thisfn = "fma_data_boxed_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( FMADataBoxedClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	FMADataBoxed *self;

	g_return_if_fail( FMA_IS_DATA_BOXED( instance ));

	self = FMA_DATA_BOXED( instance );

	self->private = g_new0( FMADataBoxedPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->data_def = NULL;
	self->private->boxed_def = NULL;
}

static void
instance_dispose( GObject *object )
{
	FMADataBoxed *self;

	g_return_if_fail( FMA_IS_DATA_BOXED( object ));

	self = FMA_DATA_BOXED( object );

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
	FMADataBoxed *self;

	g_return_if_fail( FMA_IS_DATA_BOXED( object ));

	self = FMA_DATA_BOXED( object );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

static const DataBoxedDef *
get_data_boxed_def( guint type )
{
	static const gchar *thisfn = "fma_data_boxed_get_data_boxed_def";
	int i;

	for( i = 0 ; st_data_boxed_def[i].type ; ++i ){
		if( st_data_boxed_def[i].type == type ){
			return(( const DataBoxedDef * ) st_data_boxed_def+i );
		}
	}

	g_warning( "%s: unmanaged data type=%d", thisfn, type );
	return( NULL );
}

/**
 * fma_data_boxed_new:
 * @def: the #FMADataDef definition structure for this boxed.
 *
 * Returns: a newly allocated #FMADataBoxed.
 *
 * Since: 2.30
 */
FMADataBoxed *
fma_data_boxed_new( const FMADataDef *def )
{
	FMADataBoxed *boxed;

	g_return_val_if_fail( def != NULL, NULL );

	boxed = g_object_new( FMA_TYPE_DATA_BOXED, NULL );
	fma_boxed_set_type( FMA_BOXED( boxed ), def->type );
	boxed->private->data_def = def;
	boxed->private->boxed_def = get_data_boxed_def( def->type );

	return( boxed );
}

/**
 * fma_data_boxed_get_data_def:
 * @boxed: this #FMADataBoxed object.
 *
 * Returns: a pointer to the #FMADataDef structure attached to the object.
 * Should never be %NULL.
 *
 * Since: 2.30
 */
const FMADataDef *
fma_data_boxed_get_data_def( const FMADataBoxed *boxed )
{
	const FMADataDef *def;

	g_return_val_if_fail( FMA_IS_DATA_BOXED( boxed ), NULL );

	def = NULL;

	if( !boxed->private->dispose_has_run ){

		def = boxed->private->data_def;
	}

	return( def );
}

/**
 * fma_data_boxed_set_data_def:
 * @boxed: this #FMADataBoxed object.
 * @def: the new #FMADataDef to be set.
 *
 * Changes the #FMADataDef a @boxed points to:
 * -> the new type must be the same that the previous one.
 * -> value is unchanged.
 *
 * Since: 2.30
 */
void
fma_data_boxed_set_data_def( FMADataBoxed *boxed, const FMADataDef *new_def )
{
	g_return_if_fail( FMA_IS_DATA_BOXED( boxed ));
	g_return_if_fail( boxed->private->data_def );
	g_return_if_fail( new_def );
	g_return_if_fail( new_def->type == boxed->private->data_def->type );

	if( !boxed->private->dispose_has_run ){

		boxed->private->data_def = ( FMADataDef * ) new_def;
	}
}

/**
 * fma_data_boxed_get_param_spec:
 * @def: a #FMADataDef definition structure.
 *
 * Returns: a #GParamSpec structure.
 *
 * Since: 2.30
 */
GParamSpec *
fma_data_boxed_get_param_spec( const FMADataDef *def )
{
	GParamSpec *spec;
	const DataBoxedDef *fn;

	g_return_val_if_fail( def != NULL, NULL );

	spec = NULL;
	fn = get_data_boxed_def( def->type );

	if( fn ){
		if( fn->spec ){
			spec = ( *fn->spec )( def );
		}
	}

	return( spec );
}

#ifdef FMA_ENABLE_DEPRECATED
/**
 * fma_data_boxed_are_equal:
 * @a: the first #FMADataBoxed object.
 * @b: the second #FMADataBoxed object.
 *
 * Returns: %TRUE if the two boxeds are equal, %FALSE else.
 *
 * Since: 2.30
 * Deprecated: 3.1: Use fma_boxed_are_equal() instead.
 */
gboolean
fma_data_boxed_are_equal( const FMADataBoxed *a, const FMADataBoxed *b )
{
	g_return_val_if_fail( FMA_IS_DATA_BOXED( a ), FALSE );
	g_return_val_if_fail( FMA_IS_DATA_BOXED( b ), FALSE );

	return( fma_boxed_are_equal( FMA_BOXED( a ), FMA_BOXED( b )));
}
#endif /* FMA_ENABLE_DEPRECATED */

/**
 * fma_data_boxed_is_default:
 * @boxed: this #FMADataBoxed object.
 *
 * Returns: %TRUE if the #FMADataBoxed holds its default value,
 * %FALSE else.
 *
 * Since: 2.30
 */
gboolean
fma_data_boxed_is_default( const FMADataBoxed *boxed )
{
	gboolean is_default;

	g_return_val_if_fail( FMA_IS_DATA_BOXED( boxed ), FALSE );
	g_return_val_if_fail( boxed->private->boxed_def, FALSE );
	g_return_val_if_fail( boxed->private->boxed_def->is_default, FALSE );

	is_default = FALSE;

	if( !boxed->private->dispose_has_run ){

		is_default = ( *boxed->private->boxed_def->is_default )( boxed );
	}

	return( is_default );
}

/**
 * fma_data_boxed_is_valid:
 * @boxed: the #FMADataBoxed object whose validity is to be checked.
 *
 * Returns: %TRUE if the boxed is valid, %FALSE else.
 *
 * Since: 2.30
 */
gboolean
fma_data_boxed_is_valid( const FMADataBoxed *boxed )
{
	gboolean is_valid;

	g_return_val_if_fail( FMA_IS_DATA_BOXED( boxed ), FALSE );
	g_return_val_if_fail( boxed->private->boxed_def, FALSE );
	g_return_val_if_fail( boxed->private->boxed_def->is_valid, FALSE );

	is_valid = FALSE;

	if( !boxed->private->dispose_has_run ){

		is_valid = ( *boxed->private->boxed_def->is_valid )( boxed );
	}

	return( is_valid );
}

#ifdef FMA_ENABLE_DEPRECATED
/**
 * fma_data_boxed_dump:
 * @boxed: this #FMADataBoxed object.
 *
 * Dump the content of @boxed.
 *
 * Since: 2.30
 * Deprecated: 3.1: Use fma_boxed_dump() instead.
 */
void
fma_data_boxed_dump( const FMADataBoxed *boxed )
{
	fma_boxed_dump( FMA_BOXED( boxed ));
}

/**
 * fma_data_boxed_get_as_string:
 * @boxed: the #FMADataBoxed whose value is to be set.
 *
 * Returns: the value of the @boxed, as a newly allocated string which
 * should be g_free() by the caller.
 *
 * Since: 2.30
 * Deprecated: 3.1: Use fma_boxed_get_string() instead.
 */
gchar *
fma_data_boxed_get_as_string( const FMADataBoxed *boxed )
{
	return( fma_boxed_get_string( FMA_BOXED( boxed )));
}

/**
 * fma_data_boxed_get_as_value:
 * @boxed: the #FMADataBoxed whose value is to be set.
 * @value: the string to be set.
 *
 * Setup @value with the content of the @boxed.
 *
 * Since: 2.30
 * Deprecated: 3.1: Use fma_boxed_get_as_value() instead.
 */
void
fma_data_boxed_get_as_value( const FMADataBoxed *boxed, GValue *value )
{
	fma_boxed_get_as_value( FMA_BOXED( boxed ), value );
}

/**
 * fma_data_boxed_get_as_void:
 * @boxed: the #FMADataBoxed whose value is to be set.
 *
 * Returns: the content of the @boxed.
 *
 * If of type NAFD_TYPE_STRING, NAFD_TYPE_LOCALE_STRING OR
 * NAFD_TYPE_STRING_LIST, then the content is returned in a newly
 * allocated value, which should be released by the caller.
 *
 * Since: 2.30
 * Deprecated: 3.1: Use fma_boxed_get_as_void() instead.
 */
void *
fma_data_boxed_get_as_void( const FMADataBoxed *boxed )
{
	return( fma_boxed_get_as_void( FMA_BOXED( boxed )));
}

/**
 * fma_data_boxed_set_from_boxed:
 * @boxed: the #FMADataBoxed whose value is to be set.
 * @value: the source #FMADataBoxed.
 *
 * Copy value from @value to @boxed.
 *
 * Since: 2.30
 * Deprecated: 3.1: Use fma_boxed_set_from_boxed() instead.
 */
void
fma_data_boxed_set_from_boxed( FMADataBoxed *boxed, const FMADataBoxed *value )
{
	fma_boxed_set_from_boxed( FMA_BOXED( boxed ), FMA_BOXED( value ));
}

/**
 * fma_data_boxed_set_from_string:
 * @boxed: the #FMADataBoxed whose value is to be set.
 * @value: the string to be set.
 *
 * Evaluates the @value and set it to the @boxed.
 *
 * Since: 2.30
 * Deprecated: 3.1: Use fma_boxed_set_from_string() instead.
 */
void
fma_data_boxed_set_from_string( FMADataBoxed *boxed, const gchar *value )
{
	fma_boxed_set_from_string( FMA_BOXED( boxed ), value );
}

/**
 * fma_data_boxed_set_from_value:
 * @boxed: the #FMADataBoxed whose value is to be set.
 * @value: the value whose content is to be got.
 *
 * Evaluates the @value and set it to the @boxed.
 *
 * Since: 2.30
 * Deprecated: 3.1: Use fma_boxed_set_from_value() instead.
 */
void
fma_data_boxed_set_from_value( FMADataBoxed *boxed, const GValue *value )
{
	fma_boxed_set_from_value( FMA_BOXED( boxed ), value );
}

/**
 * fma_data_boxed_set_from_void:
 * @boxed: the #FMADataBoxed whose value is to be set.
 * @value: the value whose content is to be got.
 *
 * Evaluates the @value and set it to the @boxed.
 *
 * Since: 2.30
 * Deprecated: 3.1: Use fma_boxed_set_from_void() instead.
 */
void
fma_data_boxed_set_from_void( FMADataBoxed *boxed, const void *value )
{
	fma_boxed_set_from_void( FMA_BOXED( boxed ), value );
}
#endif /* FMA_ENABLE_DEPRECATED */

static GParamSpec *
bool_spec( const FMADataDef *def )
{
	return( g_param_spec_boolean(
			def->name,
			gettext( def->short_label ),
			gettext( def->long_label ),
			fma_core_utils_boolean_from_string( def->default_value ),
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));
}

static gboolean
bool_is_default( const FMADataBoxed *boxed )
{
	gboolean is_default = FALSE;
	gboolean default_value;

	if( boxed->private->data_def->default_value && strlen( boxed->private->data_def->default_value )){
		default_value = fma_core_utils_boolean_from_string( boxed->private->data_def->default_value );
		is_default = ( default_value == fma_boxed_get_boolean( FMA_BOXED( boxed )));
	}

	return( is_default );
}

static gboolean
bool_is_valid( const FMADataBoxed *boxed )
{
	return( TRUE );
}

static GParamSpec *
pointer_spec( const FMADataDef *def )
{
	return( g_param_spec_pointer(
			def->name,
			gettext( def->short_label ),
			gettext( def->long_label ),
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));
}

/*
 * say that a pointer never has its default value
 * (essentially because there cannot be / one cannot set any relevant
 *  default value for a pointer)
 */
static gboolean
pointer_is_default( const FMADataBoxed *boxed )
{
	return( FALSE );
}

static gboolean
pointer_is_valid( const FMADataBoxed *boxed )
{
	gboolean is_valid = TRUE;
	gconstpointer pointer;

	if( boxed->private->data_def->mandatory ){
		pointer = fma_boxed_get_pointer( FMA_BOXED( boxed ));
		if( !pointer ){
			g_debug( "fma_data_boxed_pointer_is_valid: invalid %s: mandatory but null", boxed->private->data_def->name );
			is_valid = FALSE;
		}
	}

	return( is_valid );
}

static GParamSpec *
string_spec( const FMADataDef *def )
{
	return( g_param_spec_string(
			def->name,
			gettext( def->short_label ),
			gettext( def->long_label ),
			def->default_value,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));
}

static gboolean
string_is_default( const FMADataBoxed *boxed )
{
	gboolean is_default = FALSE;
	gchar *value = fma_boxed_get_string( FMA_BOXED( boxed ));

	if( boxed->private->data_def->default_value && strlen( boxed->private->data_def->default_value )){
		if( value && strlen( value )){
			/* default value is not null and string has something */
			is_default = ( strcmp( value, boxed->private->data_def->default_value ) == 0 );

		} else {
			/* default value is not null, but string is null */
			is_default = FALSE;
		}

	} else if( value && strlen( value )){
		/* default value is null, but string has something */
		is_default = FALSE;

	} else {
		/* default value and string are both null */
		is_default = TRUE;
	}
	g_free( value );

	return( is_default );
}

static gboolean
string_is_valid( const FMADataBoxed *boxed )
{
	gboolean is_valid = TRUE;

	if( boxed->private->data_def->mandatory ){
		gchar *value = fma_boxed_get_string( FMA_BOXED( boxed ));
		if( !value || !strlen( value )){
			g_debug( "fma_data_boxed_string_is_valid: invalid %s: mandatory but empty or null", boxed->private->data_def->name );
			is_valid = FALSE;
		}
		g_free( value );
	}

	return( is_valid );
}

static GParamSpec *
string_list_spec( const FMADataDef *def )
{
	return( g_param_spec_pointer(
			def->name,
			gettext( def->short_label ),
			gettext( def->long_label ),
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));
}

static gboolean
string_list_is_default( const FMADataBoxed *boxed )
{
	gboolean is_default = FALSE;
	gchar *value = fma_boxed_get_string( FMA_BOXED( boxed ));

	if( boxed->private->data_def->default_value && strlen( boxed->private->data_def->default_value )){
		if( value && strlen( value )){
			is_default = ( strcmp( value, boxed->private->data_def->default_value ) == 0 );
		} else {
			is_default = FALSE;
		}
	} else if( value && strlen( value )){
		is_default = FALSE;
	} else {
		is_default = TRUE;
	}
	g_free( value );

	return( is_default );
}

static gboolean
string_list_is_valid( const FMADataBoxed *boxed )
{
	gboolean is_valid = TRUE;

	if( boxed->private->data_def->mandatory ){
		gchar *value = fma_boxed_get_string( FMA_BOXED( boxed ));
		if( !value || !strlen( value )){
			g_debug( "fma_data_boxed_string_list_is_valid: invalid %s: mandatory but empty or null", boxed->private->data_def->name );
			is_valid = FALSE;
		}
	}

	return( is_valid );
}

static gboolean
locale_is_default( const FMADataBoxed *boxed )
{
	gboolean is_default = FALSE;
	gchar *value = fma_boxed_get_string( FMA_BOXED( boxed ));

	if( boxed->private->data_def->default_value && g_utf8_strlen( boxed->private->data_def->default_value, -1 )){
		if( value && strlen( value )){
			/* default value is not null and string has something */
			is_default = ( fma_core_utils_str_collate( value, boxed->private->data_def->default_value ) == 0 );

		} else {
			/* default value is not null, but string is null */
			is_default = FALSE;
		}
	} else if( value && g_utf8_strlen( value, -1 )){
		/* default value is null, but string has something */
		is_default = FALSE;

	} else {
		/* default value and string are both null */
		is_default = TRUE;
	}
	g_free( value );

	return( is_default );
}

static gboolean
locale_is_valid( const FMADataBoxed *boxed )
{
	gboolean is_valid = TRUE;

	if( boxed->private->data_def->mandatory ){
		gchar *value = fma_boxed_get_string( FMA_BOXED( boxed ));
		if( !value || !g_utf8_strlen( value, -1 )){
			g_debug( "fma_data_boxed_locale_is_valid: invalid %s: mandatory but empty or null", boxed->private->data_def->name );
			is_valid = FALSE;
		}
		g_free( value );
	}

	return( is_valid );
}

static GParamSpec *
uint_spec( const FMADataDef *def )
{
	return( g_param_spec_uint(
			def->name,
			gettext( def->short_label ),
			gettext( def->long_label ),
			0,
			UINT_MAX,
			atoi( def->default_value ),
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));
}

static gboolean
uint_is_default( const FMADataBoxed *boxed )
{
	gboolean is_default = FALSE;
	guint default_value;

	if( boxed->private->data_def->default_value ){
		default_value = atoi( boxed->private->data_def->default_value );
		is_default = ( fma_boxed_get_uint( FMA_BOXED( boxed )) == default_value );
	}

	return( is_default );
}

static gboolean
uint_is_valid( const FMADataBoxed *boxed )
{
	return( TRUE );
}

static GParamSpec *
uint_list_spec( const FMADataDef *def )
{
	return( g_param_spec_pointer(
			def->name,
			gettext( def->short_label ),
			gettext( def->long_label ),
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));
}

/*
 * we assume no default for uint list
 */
static gboolean
uint_list_is_default( const FMADataBoxed *boxed )
{
	return( FALSE );
}

static gboolean
uint_list_is_valid( const FMADataBoxed *boxed )
{
	gboolean is_valid = TRUE;

	if( boxed->private->data_def->mandatory ){
		gchar *value = fma_boxed_get_string( FMA_BOXED( boxed ));
		if( !value || !strlen( value )){
			g_debug( "fma_data_boxed_uint_list_is_valid: invalid %s: mandatory but empty or null", boxed->private->data_def->name );
			is_valid = FALSE;
		}
		g_free( value );
	}

	return( is_valid );
}
