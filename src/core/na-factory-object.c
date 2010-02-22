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

#include <stdarg.h>
#include <stdlib.h>

#include <api/na-core-utils.h>
#include <api/na-ifactory-provider.h>

#include "na-data-element.h"
#include "na-factory-object.h"
#include "na-factory-provider.h"

typedef gboolean ( *IdGroupIterFunc )( NadfIdType *iddef, void *user_data );

/* while iterating on read/write item
 */
typedef struct {
	NAIFactoryObject *object;
	NAIFactoryProvider   *reader;
	void           *reader_data;
	GSList        **messages;
}
	NadfRWIter;

/* while iterating on set defaults
 */
typedef struct {
	NAIFactoryObject *object;
	gboolean        creation;
}
	NadfNewIter;

/* object values are set on a list of this structure
 */
typedef struct {
	NadfIdType    *iddef;
	NADataElement *element;
}
	NadfDataValue;

#define NA_IFACTORY_OBJECT_PROP_DATA				"na-ifactory-object-prop-data"

extern gboolean ifactory_object_initialized;		/* defined in na-ifactory-object.c */
extern gboolean ifactory_object_finalized;		/* defined in na-ifactory-object.c */

static gboolean       define_class_properties_iter( const NadfIdType *iddef, GObjectClass *class );
static gboolean       factory_object_init_iter( const NadfIdType *iddef, NAIFactoryObject *object );
static gchar         *v_get_default( const NAIFactoryObject *object, const NadfIdType *iddef );
static void           v_copy( NAIFactoryObject *target, const NAIFactoryObject *source );
static gboolean       v_are_equal( const NAIFactoryObject *a, const NAIFactoryObject *b );
static gboolean       v_is_valid( const NAIFactoryObject *object );
static void           factory_object_read_data( NAIFactoryObject *serializable, const NAIFactoryProvider *reader, void *reader_data, NadfIdGroup *groups, GSList **messages );
static gboolean       factory_object_read_data_iter( NadfIdType *iddef, NadfRWIter *iter );
static void           v_read_start( NAIFactoryObject *serializable, const NAIFactoryProvider *reader, void *reader_data, GSList **messages );
static void           v_read_done( NAIFactoryObject *serializable, const NAIFactoryProvider *reader, void *reader_data, GSList **messages );
static void           factory_object_write_data( NAIFactoryObject *serializable, const NAIFactoryProvider *writer, void *writer_data, NadfIdGroup *groups, GSList **messages );
static gboolean       factory_object_write_data_iter( NadfIdType *iddef, NadfRWIter *iter );
static void           v_write_start( NAIFactoryObject *serializable, const NAIFactoryProvider *reader, void *reader_data, GSList **messages );
static void           v_write_done( NAIFactoryObject *serializable, const NAIFactoryProvider *reader, void *reader_data, GSList **messages );
static NADataElement *data_element_from_id( const NAIFactoryObject *object, guint data_id );
static void           iter_on_id_groups( const NadfIdGroup *idgroups, gboolean serializable_only, IdGroupIterFunc pfn, void *user_data );
static void           free_gvalue( GValue *value, guint type );

/**
 * na_factory_object_properties:
 * @class: the #GObjectClass.
 *
 * Initializes the serializable properties.
 */
void
na_factory_object_properties( GObjectClass *class )
{
	static const gchar *thisfn = "na_factory_object_properties";
	NadfIdGroup *groups;

	if( ifactory_object_initialized && !ifactory_object_finalized ){

		g_debug( "%s: class=%p (%s)",
				thisfn, ( void * ) class, G_OBJECT_CLASS_NAME( class ));

		g_return_if_fail( G_IS_OBJECT_CLASS( class ));

		/* define class properties
		 */
		groups = na_factory_provider_get_groups( G_OBJECT_CLASS_TYPE( class ));
		if( groups ){
			iter_on_id_groups(
					groups,
					FALSE,
					( IdGroupIterFunc ) &define_class_properties_iter,
					class );
		}
	}
}

static gboolean
define_class_properties_iter( const NadfIdType *iddef, GObjectClass *class )
{
	static const gchar *thisfn = "na_factory_object_define_class_properties_iter";
	gboolean stop;
	GParamSpec *spec;

	g_debug( "%s: iddef=%s", thisfn, iddef->name );

	stop = FALSE;

	switch( iddef->type ){

		case NADF_TYPE_LOCALE_STRING:
		case NADF_TYPE_STRING:
		case NADF_TYPE_BOOLEAN:
		case NADF_TYPE_STRING_LIST:
		case NADF_TYPE_POINTER:
			spec = g_param_spec_pointer(
					iddef->name,
					iddef->short_label,
					iddef->long_label,
					G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
			g_object_class_install_property( class, iddef->id, spec );
			break;

		case NADF_TYPE_UINT:
			spec = g_param_spec_uint(
					iddef->name,
					iddef->short_label,
					iddef->long_label,
					0, UINT_MAX, atoi( iddef->default_value ),
					G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
			g_object_class_install_property( class, iddef->id, spec );
			break;

		default:
			g_debug( "%s: type=%d", thisfn, iddef->type );
			g_return_val_if_reached( FALSE );
	}

	return( stop );
}

/**
 * na_factory_object_new:
 * @type: the GType type of the object we want allocate.
 *
 * Returns: a newly allocated #NAObject-derived object, or %NULL.
 *
 * The function checks that @type has been previously registered in the
 * data factory management system (cf. #na_factory_object_init_class()),
 * and if so invoke an empty constructor with this @type.
 */
NAIFactoryObject *
na_factory_object_new( GType type )
{
	NAIFactoryObject *object;
	NadfIdGroup *groups;

	object = NULL;

	/* check that @type has been registered
	 */
	groups = na_factory_provider_get_groups( type );
	if( groups ){

		object = NA_IFACTORY_OBJECT( g_object_new( type, NULL ));
	}

	return( object );
}

/**
 * na_factory_object_init:
 * @object: the #NAIFactoryObject being initialized.
 *
 * Initializes properties attached to the @object.
 *
 * This essentially consists of creating a #NADataElement for each
 * defined elementary data, initializing it to its default value.
 */
void
na_factory_object_init( NAIFactoryObject *object )
{
	static const gchar *thisfn = "na_factory_object_init";
	NadfIdGroup *groups;

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));
	g_return_if_fail( NA_IS_IFACTORY_OBJECT( object ));

	groups = na_factory_provider_get_groups( G_OBJECT_TYPE( object ));
	if( groups ){

		iter_on_id_groups( groups, FALSE, ( IdGroupIterFunc ) &factory_object_init_iter, object );
	}
}

static gboolean
factory_object_init_iter( const NadfIdType *iddef, NAIFactoryObject *object )
{
	gboolean stop;
	GList *list;
	NADataElement *element;
	NadfDataValue *data;
	gchar *default_value;

	stop = FALSE;

	default_value = v_get_default( object, iddef );
	element = na_data_element_new( iddef );
	na_data_element_set_from_string( element, ( const void * )( default_value ? default_value : iddef->default_value ));
	g_free( default_value );

	data = g_new0( NadfDataValue, 1 );
	data->iddef = ( NadfIdType * ) iddef;
	data->element = element;

	list = g_object_get_data( G_OBJECT( object ), NA_IFACTORY_OBJECT_PROP_DATA );
	list = g_list_prepend( list, data );
	g_object_set_data( G_OBJECT( object ), NA_IFACTORY_OBJECT_PROP_DATA, list );

	return( stop );
}

/**
 * na_factory_object_copy:
 * @target: the target #NAIFactoryObject instance.
 * @source: the source #NAIFactoryObject instance.
 *
 * Copies one instance to another.
 */
void
na_factory_object_copy( NAIFactoryObject *target, const NAIFactoryObject *source )
{
	GList *src_list, *isrc;
	NadfDataValue *src_data;
	NADataElement *tgt_element;

	src_list = g_object_get_data( G_OBJECT( source ), NA_IFACTORY_OBJECT_PROP_DATA );
	for( isrc = src_list ; isrc ; isrc = isrc->next ){

		src_data = ( NadfDataValue * ) isrc->data;
		if( src_data->iddef->copyable ){

			tgt_element = data_element_from_id( target, src_data->iddef->id );
			if( tgt_element ){

				na_data_element_set( tgt_element, src_data->element );
			}
		}
	}

	v_copy( target, source );
}

static gchar *
v_get_default( const NAIFactoryObject *object, const NadfIdType *iddef )
{
	gchar *default_value;

	default_value = NULL;

	if( NA_IFACTORY_OBJECT_GET_INTERFACE( object )->get_default ){
		default_value = NA_IFACTORY_OBJECT_GET_INTERFACE( object )->get_default( object, iddef );
	}

	return( default_value );
}

static void
v_copy( NAIFactoryObject *target, const NAIFactoryObject *source )
{
	if( NA_IFACTORY_OBJECT_GET_INTERFACE( target )->copy ){
		NA_IFACTORY_OBJECT_GET_INTERFACE( target )->copy( target, source );
	}
}

/**
 * na_factory_object_are_equal:
 * @a: the first #NAIFactoryObject instance.
 * @b: the second #NAIFactoryObject isntance.
 *
 * Returns: %TRUE if @a is equal to @b, %FALSE else.
 */
gboolean
na_factory_object_are_equal( const NAIFactoryObject *a, const NAIFactoryObject *b )
{
	gboolean are_equal;
	GList *a_list, *b_list, *ia;
	NadfDataValue *a_data;
	NADataElement *b_element;

	are_equal = FALSE;

	a_list = g_object_get_data( G_OBJECT( a ), NA_IFACTORY_OBJECT_PROP_DATA );
	b_list = g_object_get_data( G_OBJECT( b ), NA_IFACTORY_OBJECT_PROP_DATA );

	if( g_list_length( a_list ) == g_list_length( b_list )){

		are_equal = TRUE;
		for( ia = a_list ; ia && are_equal ; ia = ia->next ){

			a_data = ( NadfDataValue * ) ia->data;
			if( a_data->iddef->comparable ){

				b_element = data_element_from_id( b, a_data->iddef->id );
				if( b_element ){
					are_equal = na_data_element_are_equal( a_data->element, b_element);

				} else {
					are_equal = FALSE;
				}
			}
		}
	}

	if( are_equal ){
		are_equal = v_are_equal( a, b );
	}

	return( are_equal );
}

static gboolean
v_are_equal( const NAIFactoryObject *a, const NAIFactoryObject *b )
{
	gboolean are_equal;

	are_equal = TRUE;

	if( NA_IFACTORY_OBJECT_GET_INTERFACE( a )->are_equal ){
		are_equal = NA_IFACTORY_OBJECT_GET_INTERFACE( a )->are_equal( a, b );
	}

	return( are_equal );
}

/**
 * na_factory_object_is_valid:
 * @object: the #NAIFactoryObject instance whose validity is to be checked.
 *
 * Returns: %TRUE if @object is valid, %FALSE else.
 */
gboolean
na_factory_object_is_valid( const NAIFactoryObject *object )
{
	static const gchar *thisfn = "na_factory_object_is_valid";
	gboolean is_valid;
	GList *list_values, *iv;
	NadfDataValue *a_data;

	g_debug( "%s: object=%p (%s)",
			thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	g_return_val_if_fail( NA_IS_IFACTORY_OBJECT( object ), FALSE );

	list_values = g_object_get_data( G_OBJECT( object ), NA_IFACTORY_OBJECT_PROP_DATA );
	is_valid = TRUE;

	for( iv = list_values ; iv && is_valid ; iv = iv->next ){

		a_data = ( NadfDataValue * ) iv->data;
		if( a_data->iddef->mandatory ){

			is_valid = na_data_element_is_valid( a_data->element );
			if( !is_valid ){
				g_debug( "%s: invalid element: %s", thisfn, a_data->iddef->name );
			}
		}
	}

	if( is_valid ){
		is_valid = v_is_valid( object );
	}

	return( is_valid );
}

static gboolean
v_is_valid( const NAIFactoryObject *object )
{
	gboolean is_valid;

	is_valid = TRUE;

	if( NA_IFACTORY_OBJECT_GET_INTERFACE( object )->is_valid ){
		is_valid = NA_IFACTORY_OBJECT_GET_INTERFACE( object )->is_valid( object );
	}

	return( is_valid );
}

/**
 * na_factory_object_dump:
 * @object: this #NAIFactoryObject instance.
 *
 * Dumps the content of @object.
 */
void
na_factory_object_dump( const NAIFactoryObject *object )
{
	GList *list, *it;
	NadfDataValue *str;

	list = g_object_get_data( G_OBJECT( object ), NA_IFACTORY_OBJECT_PROP_DATA );

	for( it = list ; it ; it = it->next ){

		str = ( NadfDataValue * ) it->data;
		na_data_element_dump( str->element );
	}
}

/**
 * na_factory_object_finalize:
 * @object: the #NAIFactoryObject being finalized.
 *
 * Clears all data associated with this @object.
 */
void
na_factory_object_finalize( NAIFactoryObject *object )
{
	GList *list, *it;
	NadfDataValue *str;

	list = g_object_get_data( G_OBJECT( object ), NA_IFACTORY_OBJECT_PROP_DATA );

	for( it = list ; it ; it = it->next ){

		str = ( NadfDataValue * ) it->data;
		g_object_unref( str->element );
		g_free( str );
	}

	g_list_free( list );
}

/**
 * na_factory_object_read:
 * @serializable: this #NAIFactoryObject instance.
 * @reader: the #NAIFactoryProvider which is at the origin of this read.
 * @reader_data: reader data.
 * @messages: a pointer to a #GSList list of strings; the implementation
 *  may append messages to this list, but shouldn't reinitialize it.
 *
 * Unserializes the object.
 */
void
na_factory_object_read( NAIFactoryObject *serializable, const NAIFactoryProvider *reader, void *reader_data, GSList **messages )
{
	static const gchar *thisfn = "na_factory_object_read";
	NadfIdGroup *groups;
	gchar *msg;

	if( ifactory_object_initialized && !ifactory_object_finalized ){

		g_return_if_fail( NA_IS_IFACTORY_OBJECT( serializable ));
		g_return_if_fail( NA_IS_IFACTORY_PROVIDER( reader ));

		groups = na_factory_provider_get_groups( G_OBJECT_TYPE( serializable ));

		if( groups ){
			v_read_start( serializable, reader, reader_data, messages );
			factory_object_read_data( serializable, reader, reader_data, groups, messages );
			v_read_done( serializable, reader, reader_data, messages );

		} else {
			msg = g_strdup_printf( "%s: instance %s doesn't return any NadfIdGroup structure",
					thisfn, G_OBJECT_TYPE_NAME( serializable ));
			g_warning( "%s", msg );
			*messages = g_slist_append( *messages, msg );
		}
	}
}

/*
 * factory_object_read_data:
 * @serializable: this #NAIFactoryObject instance.
 * @reader: the #NAIFactoryProvider which is at the origin of this read.
 * @reader_data: reader data.
 * @groups: the list of NadfIdGroup structure which define @serializable.
 * @messages: a pointer to a #GSList list of strings; the implementation
 *  may append messages to this list, but shouldn't reinitialize it.
 *
 * Unserializes the object.
 */
static void
factory_object_read_data( NAIFactoryObject *serializable, const NAIFactoryProvider *reader, void *reader_data, NadfIdGroup *groups, GSList **messages )
{
	NadfRWIter *iter;

	iter = g_new0( NadfRWIter, 1 );
	iter->object = serializable;
	iter->reader = ( NAIFactoryProvider * ) reader;
	iter->reader_data = reader_data;
	iter->messages = messages;

	iter_on_id_groups( groups, TRUE, ( IdGroupIterFunc ) &factory_object_read_data_iter, iter );

	g_free( iter );
}

static gboolean
factory_object_read_data_iter( NadfIdType *iddef, NadfRWIter *iter )
{
	gboolean stop;
	GValue *value;
	NADataElement *element;

	stop = FALSE;

	value = na_factory_provider_read_value( iter->reader, iter->reader_data, iddef, iter->messages );
	if( value ){

		element = data_element_from_id( iter->object, iddef->id );
		if( element ){

			na_data_element_set_from_value( element, value );
		}

		free_gvalue( value, iddef->type );
	}

	return( stop );
}

static void
v_read_start( NAIFactoryObject *serializable, const NAIFactoryProvider *reader, void *reader_data, GSList **messages )
{
	if( NA_IFACTORY_OBJECT_GET_INTERFACE( serializable )->read_start ){
		NA_IFACTORY_OBJECT_GET_INTERFACE( serializable )->read_start( serializable, reader, reader_data, messages );
	}
}

static void
v_read_done( NAIFactoryObject *serializable, const NAIFactoryProvider *reader, void *reader_data, GSList **messages )
{
	if( NA_IFACTORY_OBJECT_GET_INTERFACE( serializable )->read_done ){
		NA_IFACTORY_OBJECT_GET_INTERFACE( serializable )->read_done( serializable, reader, reader_data, messages );
	}
}

/**
 * na_factory_object_write:
 * @serializable: this #NAIFactoryObject instance.
 * @writer: the #NAIFactoryProvider which is at the origin of this write.
 * @writer_data: writer data.
 * @messages: a pointer to a #GSList list of strings; the implementation
 *  may append messages to this list, but shouldn't reinitialize it.
 *
 * Serializes the object down to the @writer.
 */
void
na_factory_object_write( NAIFactoryObject *serializable, const NAIFactoryProvider *writer, void *writer_data, GSList **messages )
{
	static const gchar *thisfn = "na_factory_object_write";
	NadfIdGroup *groups;
	gchar *msg;

	g_return_if_fail( NA_IS_IFACTORY_OBJECT( serializable ));
	g_return_if_fail( NA_IS_IFACTORY_PROVIDER( writer ));

	groups = na_factory_provider_get_groups( G_OBJECT_TYPE( serializable ));

	if( groups ){
		v_write_start( serializable, writer, writer_data, messages );
		factory_object_write_data( serializable, writer, writer_data, groups, messages );
		v_write_done( serializable, writer, writer_data, messages );

	} else {
		msg = g_strdup_printf( "%s: instance %s doesn't return any NadfIdGroup structure",
				thisfn, G_OBJECT_TYPE_NAME( serializable ));
		g_warning( "%s", msg );
		*messages = g_slist_append( *messages, msg );
	}
}

/*
 * factory_object_write_data:
 * @serializable: this #NAIFactoryObject instance.
 * @writer: the #NAIFactoryProvider which is at the origin of this writ.
 * @writer_data: writer data.
 * @groups: the list of NadfIdGroup structure which define @serializable.
 * @messages: a pointer to a #GSList list of strings; the implementation
 *  may append messages to this list, but shouldn't reinitialize it.
 *
 * Serializes the object.
 */
static void
factory_object_write_data( NAIFactoryObject *serializable, const NAIFactoryProvider *writer, void *writer_data, NadfIdGroup *groups, GSList **messages )
{
	NadfRWIter *iter;

	iter = g_new0( NadfRWIter, 1 );
	iter->object = serializable;
	iter->reader = ( NAIFactoryProvider * ) writer;
	iter->reader_data = writer_data;
	iter->messages = messages;

	iter_on_id_groups( groups, TRUE, ( IdGroupIterFunc ) &factory_object_write_data_iter, iter );

	g_free( iter );
}

static gboolean
factory_object_write_data_iter( NadfIdType *iddef, NadfRWIter *iter )
{
	gboolean stop;

	stop = FALSE;

	/*na_factory_provider_set_value( iter->reader, iter->reader_data, iddef, iter->messages );*/

	return( stop );
}

static void
v_write_start( NAIFactoryObject *serializable, const NAIFactoryProvider *writer, void *writer_data, GSList **messages )
{
	if( NA_IFACTORY_OBJECT_GET_INTERFACE( serializable )->write_start ){
		NA_IFACTORY_OBJECT_GET_INTERFACE( serializable )->write_start( serializable, writer, writer_data, messages );
	}
}

static void
v_write_done( NAIFactoryObject *serializable, const NAIFactoryProvider *writer, void *writer_data, GSList **messages )
{
	if( NA_IFACTORY_OBJECT_GET_INTERFACE( serializable )->write_done ){
		NA_IFACTORY_OBJECT_GET_INTERFACE( serializable )->write_done( serializable, writer, writer_data, messages );
	}
}

/**
 * na_factory_object_set_from_string:
 * @object: this #NAIFactoryObject instance.
 * @property_id: the elementary data id.
 * @string: the string to be set in the element.
 *
 * Set the @object with the @string.
 */
void
na_factory_object_set_from_string( NAIFactoryObject *object, guint data_id, const gchar *string )
{
	static const gchar *thisfn = "na_factory_object_set_from_string";
	NADataElement *element;

	g_return_if_fail( NA_IS_IFACTORY_OBJECT( object ));

	element = data_element_from_id( object, data_id );
	if( element ){
		na_data_element_set_from_string( element, string );

	} else {
		g_warning( "%s: unknown property id %d", thisfn, data_id );
	}
}

/**
 * na_factory_object_set_from_value:
 * @object: this #NAIFactoryObject instance.
 * @property_id: the elementary data id.
 * @value: the #GValue whose content is to be got.
 *
 * Get from the @value the content to be set in the #NADataElement
 * attached to @property_id.
 *
 * This is to be readen as "set data element from value".
 */
void
na_factory_object_set_from_value( NAIFactoryObject *object, guint data_id, const GValue *value )
{
	static const gchar *thisfn = "na_factory_object_set_from_value";
	NADataElement *element;

	g_return_if_fail( NA_IS_IFACTORY_OBJECT( object ));

	element = data_element_from_id( object, data_id );
	if( element ){
		na_data_element_set_from_value( element, value );

	} else {
		g_warning( "%s: unknown property id %d", thisfn, data_id );
	}
}

/**
 * na_factory_object_set_from_void:
 * @object: this #NAIFactoryObject instance.
 * @data_id: the elementary data whose value is to be set.
 * @data: the value to set.
 *
 * Set the elementary data with the given value.
 */
void
na_factory_object_set_from_void( NAIFactoryObject *object, guint data_id, const void *data )
{
	static const gchar *thisfn = "na_factory_object_set_from_void";
	NADataElement *element;

	/*g_debug( "%s: object=%p (%s), data_id=%d, data=%p",
			thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ), data_id, ( void * ) data );*/

	g_return_if_fail( NA_IS_IFACTORY_OBJECT( object ));

	element = data_element_from_id( object, data_id );
	if( element ){
		na_data_element_set_from_void( element, data );

	} else {
		g_warning( "%s: unknown property id %d", thisfn, data_id );
	}
}

/**
 * na_factory_object_get:
 * @object: this #NAIFactoryObject instance.
 * @data_id: the elementary data whose value is to be got.
 *
 * Returns: the searched value.
 *
 * If the type of the value is NADF_TYPE_STRING, NADF_TYPE_LOCALE_STRING,
 * or NADF_TYPE_STRING_LIST, then the returned value is a newly allocated
 * one and should be g_free() (resp. na_core_utils_slist_free()) by the
 * caller.
 */
void *
na_factory_object_get( const NAIFactoryObject *object, guint data_id )
{
	static const gchar *thisfn = "na_factory_object_get";
	void *value;
	NADataElement *element;

	g_return_val_if_fail( NA_IS_IFACTORY_OBJECT( object ), NULL );

	value = NULL;

	element = data_element_from_id( object, data_id );
	if( element ){
		value = na_data_element_get( element );

	} else {
		g_warning( "%s: unknown property id %d", thisfn, data_id );
	}

	return( value );
}

/**
 * na_factory_object_set_value:
 * @object: this #NAIFactoryObject instance.
 * @property_id: the elementary data id.
 * @value: the #GValue to be set.
 * @spec: the #GParamSpec which describes this data.
 *
 * Set the @value with the current content of the #NADataElement attached
 * to @property_id.
 *
 * This is to be readen as "set value from data element".
 */
void
na_factory_object_set_value( const NAIFactoryObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	static const gchar *thisfn = "na_factory_object_set_value";
	NADataElement *element;

	g_return_if_fail( NA_IS_IFACTORY_OBJECT( object ));

	element = data_element_from_id( object, property_id );
	if( element ){
		na_data_element_set_to_value( element, value );

	} else {
		g_warning( "%s: unknown property id %d", thisfn, property_id );
	}
}

static NADataElement *
data_element_from_id( const NAIFactoryObject *object, guint data_id )
{
	GList *list, *ip;
	NADataElement *element;

	element = NULL;

	list = g_object_get_data( G_OBJECT( object ), NA_IFACTORY_OBJECT_PROP_DATA );

	for( ip = list ; ip && !element ; ip = ip->next ){
		NadfDataValue *ndv = ( NadfDataValue * ) ip->data;
		if( ndv->iddef->id == data_id ){
			element = ndv->element;
		}
	}

	return( element );
}

/*
 * the iter function must return TRUE to stops the enumeration
 */
static void
iter_on_id_groups( const NadfIdGroup *groups, gboolean serializable_only, IdGroupIterFunc pfn, void *user_data )
{
	NadfIdType *iddef;
	gboolean stop;

	stop = FALSE;

	while( groups->idgroup && !stop ){

		if( groups->iddef ){

			iddef = groups->iddef;
			while( iddef->id && !stop ){

				/*g_debug( "serializable_only=%s, iddef->serializable=%s",
						serializable_only ? "True":"False", iddef->serializable ? "True":"False" );*/

				if( !serializable_only || iddef->serializable ){
					stop = ( *pfn )( iddef, user_data );
				}

				iddef++;
			}
		}

		groups++;
	}
}

static void
free_gvalue( GValue *value, guint type )
{
	GSList *slist;

	if( type == NADF_TYPE_STRING_LIST ){
		slist = g_value_get_pointer( value );
		na_core_utils_slist_free( slist );
	}

	g_value_unset( value );
	g_free( value );
}
