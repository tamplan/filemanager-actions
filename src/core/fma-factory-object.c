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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <api/fma-core-utils.h>
#include <api/fma-data-boxed.h>
#include <api/fma-data-types.h>
#include <api/fma-iio-provider.h>
#include <api/fma-ifactory-provider.h>
#include <api/fma-object-api.h>

#include "fma-factory-object.h"
#include "fma-factory-provider.h"

typedef gboolean ( *FMADataDefIterFunc )( FMADataDef *def, void *user_data );

enum {
	DATA_DEF_ITER_SET_PROPERTIES = 1,
	DATA_DEF_ITER_SET_DEFAULTS,
	DATA_DEF_ITER_IS_VALID,
	DATA_DEF_ITER_READ_ITEM,
};

/* while iterating on read item
 */
typedef struct {
	FMAIFactoryObject   *object;
	FMAIFactoryProvider *reader;
	void                *reader_data;
	GSList             **messages;
}
	NafoReadIter;

/* while iterating on write item
 */
typedef struct {
	FMAIFactoryProvider *writer;
	void                *writer_data;
	GSList             **messages;
	guint                code;
}
	NafoWriteIter;

/* while iterating on is_valid
 */
typedef struct {
	FMAIFactoryObject *object;
	gboolean           is_valid;
}
	NafoValidIter;

/* while iterating on set defaults
 */
typedef struct {
	FMAIFactoryObject *object;
}
	NafoDefaultIter;

extern gboolean                   ifactory_object_initialized;
extern gboolean                   ifactory_object_finalized;

static gboolean      define_class_properties_iter( const FMADataDef *def, GObjectClass *class );
static gboolean      set_defaults_iter( FMADataDef *def, NafoDefaultIter *data );
static gboolean      is_valid_mandatory_iter( const FMADataDef *def, NafoValidIter *data );
static gboolean      read_data_iter( FMADataDef *def, NafoReadIter *iter );
static gboolean      write_data_iter( const FMAIFactoryObject *object, FMADataBoxed *boxed, NafoWriteIter *iter );

static FMADataGroup *v_get_groups( const FMAIFactoryObject *object );
static void          v_copy( FMAIFactoryObject *target, const FMAIFactoryObject *source );
static gboolean      v_are_equal( const FMAIFactoryObject *a, const FMAIFactoryObject *b );
static gboolean      v_is_valid( const FMAIFactoryObject *object );
static void          v_read_start( FMAIFactoryObject *serializable, const FMAIFactoryProvider *reader, void *reader_data, GSList **messages );
static void          v_read_done( FMAIFactoryObject *serializable, const FMAIFactoryProvider *reader, void *reader_data, GSList **messages );
static guint         v_write_start( FMAIFactoryObject *serializable, const FMAIFactoryProvider *reader, void *reader_data, GSList **messages );
static guint         v_write_done( FMAIFactoryObject *serializable, const FMAIFactoryProvider *reader, void *reader_data, GSList **messages );

static void          attach_boxed_to_object( FMAIFactoryObject *object, FMADataBoxed *boxed );
static void          free_data_boxed_list( FMAIFactoryObject *object );
static void          iter_on_data_defs( const FMADataGroup *idgroups, guint mode, FMADataDefIterFunc pfn, void *user_data );

/*
 * fma_factory_object_define_properties:
 * @class: the #GObjectClass.
 * @groups: the list of #FMADataGroup structure which define the data of the class.
 *
 * Initializes all the properties for the class.
 */
void
fma_factory_object_define_properties( GObjectClass *class, const FMADataGroup *groups )
{
	static const gchar *thisfn = "fma_factory_object_define_properties";

	g_return_if_fail( G_IS_OBJECT_CLASS( class ));

	g_debug( "%s: class=%p (%s)",
			thisfn, ( void * ) class, G_OBJECT_CLASS_NAME( class ));

	/* define class properties
	 */
	iter_on_data_defs( groups, DATA_DEF_ITER_SET_PROPERTIES, ( FMADataDefIterFunc ) define_class_properties_iter, class );
}

static gboolean
define_class_properties_iter( const FMADataDef *def, GObjectClass *class )
{
	static const gchar *thisfn = "fma_factory_object_define_class_properties_iter";
	gboolean stop;
	GParamSpec *spec;

	g_debug( "%s: def=%p (%s)", thisfn, ( void * ) def, def->name );

	stop = FALSE;

	spec = fma_data_boxed_get_param_spec( def );

	if( spec ){
		g_object_class_install_property( class, g_quark_from_string( def->name ), spec );

	} else {
		g_warning( "%s: type=%d: unable to get a spec", thisfn, def->type );
	}

	return( stop );
}

/*
 * fma_factory_object_get_data_def:
 * @object: this #FMAIFactoryObject object.
 * @name: the searched name.
 *
 * Returns: the #FMADataDef structure which describes this @name, or %NULL.
 */
FMADataDef *
fma_factory_object_get_data_def( const FMAIFactoryObject *object, const gchar *name )
{
	FMADataDef *def;

	g_return_val_if_fail( FMA_IS_IFACTORY_OBJECT( object ), NULL );

	def = NULL;

	FMADataGroup *groups = v_get_groups( object );
	while( groups->group ){

		FMADataDef *def = groups->def;
		if( def ){
			while( def->name ){

				if( !strcmp( def->name, name )){
					return( def );
				}
				def++;
			}
		}
		groups++;
	}

	return( def );
}

/*
 * fma_factory_object_get_data_groups:
 * @object: the #FMAIFactoryObject instance.
 *
 * Returns: a pointer to the list of #FMADataGroup which define the data.
 */
FMADataGroup *
fma_factory_object_get_data_groups( const FMAIFactoryObject *object )
{
	FMADataGroup *groups;

	g_return_val_if_fail( FMA_IS_IFACTORY_OBJECT( object ), NULL );

	groups = v_get_groups( object );

	return( groups );
}

/*
 * fma_factory_object_iter_on_boxed:
 * @object: this #FMAIFactoryObject object.
 * @pfn: the function to be called.
 * @user_data: data to be provided to the user function.
 *
 * Iterate on each #FMADataBoxed attached to the @object.
 *
 * The @fn called function may return %TRUE to stop the iteration.
 */
void
fma_factory_object_iter_on_boxed( const FMAIFactoryObject *object, FMAFactoryObjectIterBoxedFn pfn, void *user_data )
{
	GList *list, *ibox;
	gboolean stop;

	g_return_if_fail( FMA_IS_IFACTORY_OBJECT( object ));

	list = g_object_get_data( G_OBJECT( object ), FMA_IFACTORY_OBJECT_PROP_DATA );
	/*g_debug( "list=%p (count=%u)", ( void * ) list, g_list_length( list ));*/
	stop = FALSE;

	for( ibox = list ; ibox && !stop ; ibox = ibox->next ){
		stop = ( *pfn )( object, FMA_DATA_BOXED( ibox->data ), user_data );
	}
}

/*
 * fma_factory_object_get_default:
 * @object: this #FMAIFactoryObject object.
 * @name: the searched name.
 *
 * Returns: the default value for this @object, as a newly allocated
 * string which should be g_free() by the caller.
 */
gchar *
fma_factory_object_get_default( FMAIFactoryObject *object, const gchar *name )
{
	static const gchar *thisfn = "fma_factory_object_set_defaults";
	gchar *value;
	FMADataDef *def;

	g_return_val_if_fail( FMA_IS_IFACTORY_OBJECT( object ), NULL );

	value = NULL;

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	def = fma_factory_object_get_data_def( object, name );
	if( def ){
		value = g_strdup( def->default_value );
	}

	return( value );
}

/*
 * fma_factory_object_set_defaults:
 * @object: this #FMAIFactoryObject object.
 *
 * Implement default values in this new @object.
 */
void
fma_factory_object_set_defaults( FMAIFactoryObject *object )
{
	static const gchar *thisfn = "fma_factory_object_set_defaults";
	FMADataGroup *groups;
	NafoDefaultIter *iter_data;

	g_return_if_fail( FMA_IS_IFACTORY_OBJECT( object ));

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	groups = v_get_groups( object );
	if( !groups ){
		g_warning( "%s: no FMADataGroup found for %s", thisfn, G_OBJECT_TYPE_NAME( object ));

	} else {
		iter_data = g_new0( NafoDefaultIter, 1 );
		iter_data->object = object;

		iter_on_data_defs( groups, DATA_DEF_ITER_SET_DEFAULTS, ( FMADataDefIterFunc ) set_defaults_iter, iter_data );

		g_free( iter_data );
	}
}

/*
 * because this function is called very early in the FMAIFactoryObject life,
 * we assume here that if a FMADataBoxed has been allocated, then this is
 * most probably because it is set. Thus a 'null' value is not considered
 * as an 'unset' value.
 */
static gboolean
set_defaults_iter( FMADataDef *def, NafoDefaultIter *data )
{
	FMADataBoxed *boxed = fma_ifactory_object_get_data_boxed( data->object, def->name );

	if( !boxed ){
		boxed = fma_data_boxed_new( def );
		attach_boxed_to_object( data->object, boxed );
		fma_boxed_set_from_string( FMA_BOXED( boxed ), def->default_value );
	}

	/* do not stop */
	return( FALSE );
}

/*
 * fma_factory_object_move_boxed:
 * @target: the target #FMAIFactoryObject instance.
 * @source: the source #FMAIFactoryObject instance.
 * @boxed: a #FMADataBoxed.
 *
 * Move the @boxed from @source to @target, detaching from @source list
 * to be attached to @target one.
 */
void
fma_factory_object_move_boxed( FMAIFactoryObject *target, const FMAIFactoryObject *source, FMADataBoxed *boxed )
{
	g_return_if_fail( FMA_IS_IFACTORY_OBJECT( target ));
	g_return_if_fail( FMA_IS_IFACTORY_OBJECT( source ));

	GList *src_list = g_object_get_data( G_OBJECT( source ), FMA_IFACTORY_OBJECT_PROP_DATA );

	if( g_list_find( src_list, boxed )){
		src_list = g_list_remove( src_list, boxed );
		g_object_set_data( G_OBJECT( source ), FMA_IFACTORY_OBJECT_PROP_DATA, src_list );

		attach_boxed_to_object( target, boxed );

		const FMADataDef *src_def = fma_data_boxed_get_data_def( boxed );
		FMADataDef *tgt_def = fma_factory_object_get_data_def( target, src_def->name );
		fma_data_boxed_set_data_def( boxed, tgt_def );
	}
}

/*
 * fma_factory_object_copy:
 * @target: the target #FMAIFactoryObject instance.
 * @source: the source #FMAIFactoryObject instance.
 *
 * Copies one instance to another.
 * Takes care of not overriding provider data.
 */
void
fma_factory_object_copy( FMAIFactoryObject *target, const FMAIFactoryObject *source )
{
	static const gchar *thisfn = "fma_factory_object_copy";
	GList *dest_list, *idest, *inext;
	GList *src_list, *isrc;
	FMADataBoxed *boxed;
	const FMADataDef *def;
	void *provider, *provider_data;

	g_return_if_fail( FMA_IS_IFACTORY_OBJECT( target ));
	g_return_if_fail( FMA_IS_IFACTORY_OBJECT( source ));

	g_debug( "%s: target=%p (%s), source=%p (%s)",
			thisfn,
			( void * ) target, G_OBJECT_TYPE_NAME( target ),
			( void * ) source, G_OBJECT_TYPE_NAME( source ));

	/* first remove copyable data from target
	 */
	provider = fma_object_get_provider( target );
	provider_data = fma_object_get_provider_data( target );

	idest = dest_list = g_object_get_data( G_OBJECT( target ), FMA_IFACTORY_OBJECT_PROP_DATA );
	while( idest ){
		boxed = FMA_DATA_BOXED( idest->data );
		inext = idest->next;
		def = fma_data_boxed_get_data_def( boxed );
		if( def->copyable ){
			dest_list = g_list_remove_link( dest_list, idest );
			g_object_unref( idest->data );
		}
		idest = inext;
	}
	g_object_set_data( G_OBJECT( target ), FMA_IFACTORY_OBJECT_PROP_DATA, dest_list );

	/* only then copy copyable data from source
	 */
	src_list = g_object_get_data( G_OBJECT( source ), FMA_IFACTORY_OBJECT_PROP_DATA );
	for( isrc = src_list ; isrc ; isrc = isrc->next ){
		boxed = FMA_DATA_BOXED( isrc->data );
		def = fma_data_boxed_get_data_def( boxed );
		if( def->copyable ){
			FMADataBoxed *tgt_boxed = fma_ifactory_object_get_data_boxed( target, def->name );
			if( !tgt_boxed ){
				tgt_boxed = fma_data_boxed_new( def );
				attach_boxed_to_object( target, tgt_boxed );
			}
			fma_boxed_set_from_boxed( FMA_BOXED( tgt_boxed ), FMA_BOXED( boxed ));
		}
	}

	if( provider ){
		fma_object_set_provider( target, provider );
		if( provider_data ){
			fma_object_set_provider_data( target, provider_data );
		}
	}

	v_copy( target, source );
}

/*
 * fma_factory_object_are_equal:
 * @a: the first (original) #FMAIFactoryObject instance.
 * @b: the second (current) #FMAIFactoryObject isntance.
 *
 * Returns: %TRUE if @a is equal to @b, %FALSE else.
 */
gboolean
fma_factory_object_are_equal( const FMAIFactoryObject *a, const FMAIFactoryObject *b )
{
	static const gchar *thisfn = "fma_factory_object_are_equal";
	gboolean are_equal;
	GList *a_list, *b_list, *ia, *ib;

	are_equal = FALSE;

	a_list = g_object_get_data( G_OBJECT( a ), FMA_IFACTORY_OBJECT_PROP_DATA );
	b_list = g_object_get_data( G_OBJECT( b ), FMA_IFACTORY_OBJECT_PROP_DATA );

	g_debug( "%s: a=%p, b=%p", thisfn, ( void * ) a, ( void * ) b );

	are_equal = TRUE;
	for( ia = a_list ; ia && are_equal ; ia = ia->next ){

		FMADataBoxed *a_boxed = FMA_DATA_BOXED( ia->data );
		const FMADataDef *a_def = fma_data_boxed_get_data_def( a_boxed );
		if( a_def->comparable ){

			FMADataBoxed *b_boxed = fma_ifactory_object_get_data_boxed( b, a_def->name );
			if( b_boxed ){
				are_equal = fma_boxed_are_equal( FMA_BOXED( a_boxed ), FMA_BOXED( b_boxed ));
				if( !are_equal ){
					g_debug( "%s: %s not equal as %s different", thisfn, G_OBJECT_TYPE_NAME( a ), a_def->name );
				}

			} else {
				are_equal = FALSE;
				g_debug( "%s: %s not equal as %s has disappeared", thisfn, G_OBJECT_TYPE_NAME( a ), a_def->name );
			}
		}
	}

	for( ib = b_list ; ib && are_equal ; ib = ib->next ){

		FMADataBoxed *b_boxed = FMA_DATA_BOXED( ib->data );
		const FMADataDef *b_def = fma_data_boxed_get_data_def( b_boxed );
		if( b_def->comparable ){

			FMADataBoxed *a_boxed = fma_ifactory_object_get_data_boxed( a, b_def->name );
			if( !a_boxed ){
				are_equal = FALSE;
				g_debug( "%s: %s not equal as %s was not set", thisfn, G_OBJECT_TYPE_NAME( a ), b_def->name );
			}
		}
	}

	are_equal &= v_are_equal( a, b );

	return( are_equal );
}

/*
 * fma_factory_object_is_valid:
 * @object: the #FMAIFactoryObject instance whose validity is to be checked.
 *
 * Returns: %TRUE if @object is valid, %FALSE else.
 */
gboolean
fma_factory_object_is_valid( const FMAIFactoryObject *object )
{
	static const gchar *thisfn = "fma_factory_object_is_valid";
	gboolean is_valid;
	FMADataGroup *groups;
	GList *list, *iv;

	g_return_val_if_fail( FMA_IS_IFACTORY_OBJECT( object ), FALSE );

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	list = g_object_get_data( G_OBJECT( object ), FMA_IFACTORY_OBJECT_PROP_DATA );
	is_valid = TRUE;

	/* mandatory data must be set
	 */
	NafoValidIter iter_data;
	iter_data.object = ( FMAIFactoryObject * ) object;
	iter_data.is_valid = TRUE;

	groups = v_get_groups( object );
	if( groups ){
		iter_on_data_defs( groups, DATA_DEF_ITER_IS_VALID, ( FMADataDefIterFunc ) is_valid_mandatory_iter, &iter_data );
	}
	is_valid = iter_data.is_valid;

	for( iv = list ; iv && is_valid ; iv = iv->next ){
		is_valid = fma_data_boxed_is_valid( FMA_DATA_BOXED( iv->data ));
	}

	is_valid &= v_is_valid( object );

	return( is_valid );
}

static gboolean
is_valid_mandatory_iter( const FMADataDef *def, NafoValidIter *data )
{
	FMADataBoxed *boxed;

	if( def->mandatory ){
		boxed = fma_ifactory_object_get_data_boxed( data->object, def->name );
		if( !boxed ){
			g_debug( "fma_factory_object_is_valid_mandatory_iter: invalid %s: mandatory but not set", def->name );
			data->is_valid = FALSE;
		}
	}

	/* do not stop iteration while valid */
	return( !data->is_valid );
}

/*
 * fma_factory_object_dump:
 * @object: this #FMAIFactoryObject instance.
 *
 * Dumps the content of @object.
 */
void
fma_factory_object_dump( const FMAIFactoryObject *object )
{
	static const gchar *thisfn = "fma_factory_object_dump";
	static const gchar *prefix = "factory-data-";
	GList *list, *it;
	guint length;
	guint l_prefix;

	length = 0;
	l_prefix = strlen( prefix );
	list = g_object_get_data( G_OBJECT( object ), FMA_IFACTORY_OBJECT_PROP_DATA );

	for( it = list ; it ; it = it->next ){
		FMADataBoxed *boxed = FMA_DATA_BOXED( it->data );
		const FMADataDef *def = fma_data_boxed_get_data_def( boxed );
		length = MAX( length, strlen( def->name ));
	}

	length -= l_prefix;
	length += 1;

	for( it = list ; it ; it = it->next ){
		FMADataBoxed *boxed = FMA_DATA_BOXED( it->data );
		const FMADataDef *def = fma_data_boxed_get_data_def( boxed );
		gchar *value = fma_boxed_get_string( FMA_BOXED( boxed ));
		g_debug( "| %s: %*s=%s", thisfn, length, def->name+l_prefix, value );
		g_free( value );
	}
}

/*
 * fma_factory_object_finalize:
 * @object: the #FMAIFactoryObject being finalized.
 *
 * Clears all data associated with this @object.
 */
void
fma_factory_object_finalize( FMAIFactoryObject *object )
{
	free_data_boxed_list( object );
}

/*
 * fma_factory_object_read_item:
 * @object: this #FMAIFactoryObject instance.
 * @reader: the #FMAIFactoryProvider which is at the origin of this read.
 * @reader_data: reader data.
 * @messages: a pointer to a #GSList list of strings; the implementation
 *  may append messages to this list, but shouldn't reinitialize it.
 *
 * Unserializes the object.
 */
void
fma_factory_object_read_item( FMAIFactoryObject *object, const FMAIFactoryProvider *reader, void *reader_data, GSList **messages )
{
	static const gchar *thisfn = "fma_factory_object_read_item";

	g_return_if_fail( FMA_IS_IFACTORY_OBJECT( object ));
	g_return_if_fail( FMA_IS_IFACTORY_PROVIDER( reader ));

	FMADataGroup *groups = v_get_groups( object );

	if( groups ){
		v_read_start( object, reader, reader_data, messages );

		NafoReadIter *iter = g_new0( NafoReadIter, 1 );
		iter->object = object;
		iter->reader = ( FMAIFactoryProvider * ) reader;
		iter->reader_data = reader_data;
		iter->messages = messages;

		iter_on_data_defs( groups, DATA_DEF_ITER_READ_ITEM, ( FMADataDefIterFunc ) read_data_iter, iter );

		g_free( iter );

		v_read_done( object, reader, reader_data, messages );

	} else {
		g_warning( "%s: class %s doesn't return any FMADataGroup structure",
				thisfn, G_OBJECT_TYPE_NAME( object ));
	}
}

static gboolean
read_data_iter( FMADataDef *def, NafoReadIter *iter )
{
	gboolean stop;

	stop = FALSE;

	FMADataBoxed *boxed = fma_factory_provider_read_data( iter->reader, iter->reader_data, iter->object, def, iter->messages );

	if( boxed ){
		FMADataBoxed *exist = fma_ifactory_object_get_data_boxed( iter->object, def->name );

		if( exist ){
			fma_boxed_set_from_boxed( FMA_BOXED( exist ), FMA_BOXED( boxed ));
			g_object_unref( boxed );

		} else {
			attach_boxed_to_object( iter->object, boxed );
		}
	}

	return( stop );
}

/*
 * fma_factory_object_write_item:
 * @object: this #FMAIFactoryObject instance.
 * @writer: the #FMAIFactoryProvider which is at the origin of this write.
 * @writer_data: writer data.
 * @messages: a pointer to a #GSList list of strings; the implementation
 *  may append messages to this list, but shouldn't reinitialize it.
 *
 * Serializes the object down to the @writer.
 *
 * Returns: a FMAIIOProvider operation return code.
 */
guint
fma_factory_object_write_item( FMAIFactoryObject *object, const FMAIFactoryProvider *writer, void *writer_data, GSList **messages )
{
	static const gchar *thisfn = "fma_factory_object_write_item";
	guint code;
	FMADataGroup *groups;
	gchar *msg;

	g_return_val_if_fail( FMA_IS_IFACTORY_OBJECT( object ), IIO_PROVIDER_CODE_PROGRAM_ERROR );
	g_return_val_if_fail( FMA_IS_IFACTORY_PROVIDER( writer ), IIO_PROVIDER_CODE_PROGRAM_ERROR );

	code = IIO_PROVIDER_CODE_PROGRAM_ERROR;

	groups = v_get_groups( object );

	if( groups ){
		code = v_write_start( object, writer, writer_data, messages );

	} else {
		msg = g_strdup_printf( "%s: class %s doesn't return any FMADataGroup structure",
				thisfn, G_OBJECT_TYPE_NAME( object ));
		g_warning( "%s", msg );
		*messages = g_slist_append( *messages, msg );
	}

	if( code == IIO_PROVIDER_CODE_OK ){

		NafoWriteIter *iter = g_new0( NafoWriteIter, 1 );
		iter->writer = ( FMAIFactoryProvider * ) writer;
		iter->writer_data = writer_data;
		iter->messages = messages;
		iter->code = code;

		fma_factory_object_iter_on_boxed( object, ( FMAFactoryObjectIterBoxedFn ) write_data_iter, iter );

		code = iter->code;
		g_free( iter );
	}

	if( code == IIO_PROVIDER_CODE_OK ){
		code = v_write_done( object, writer, writer_data, messages );
	}

	return( code );
}

static gboolean
write_data_iter( const FMAIFactoryObject *object, FMADataBoxed *boxed, NafoWriteIter *iter )
{
	const FMADataDef *def = fma_data_boxed_get_data_def( boxed );

	if( def->writable ){
		iter->code = fma_factory_provider_write_data( iter->writer, iter->writer_data, object, boxed, iter->messages );
	}

	/* iter while code is ok */
	return( iter->code != IIO_PROVIDER_CODE_OK );
}

/*
 * fma_factory_object_get_as_value:
 * @object: this #FMAIFactoryObject instance.
 * @name: the elementary data id.
 * @value: the #GValue to be set.
 *
 * Set the @value with the current content of the #FMADataBoxed attached
 * to @name.
 *
 * This is to be read as "set value from data element".
 */
void
fma_factory_object_get_as_value( const FMAIFactoryObject *object, const gchar *name, GValue *value )
{
	FMADataBoxed *boxed;

	g_return_if_fail( FMA_IS_IFACTORY_OBJECT( object ));

	g_value_unset( value );

	boxed = fma_ifactory_object_get_data_boxed( object, name );
	if( boxed ){
		fma_boxed_get_as_value( FMA_BOXED( boxed ), value );
	}
}

/*
 * fma_factory_object_get_as_void:
 * @object: this #FMAIFactoryObject instance.
 * @name: the elementary data whose value is to be got.
 *
 * Returns: the searched value.
 *
 * If the type of the value is FMA_DATA_TYPE_STRING or FMA_DATA_TYPE_LOCALE_STRING
 * (resp. FMA_DATA_TYPE_STRING_LIST), then the returned value is a newly allocated
 * string (resp. GSList) and should be g_free() (resp. fma_core_utils_slist_free())
 * by the caller.
 */
void *
fma_factory_object_get_as_void( const FMAIFactoryObject *object, const gchar *name )
{
	void *value;
	FMADataBoxed *boxed;

	g_return_val_if_fail( FMA_IS_IFACTORY_OBJECT( object ), NULL );

	value = NULL;

	boxed = fma_ifactory_object_get_data_boxed( object, name );
	if( boxed ){
		value = fma_boxed_get_as_void( FMA_BOXED( boxed ));
	}

	return( value );
}

/*
 * fma_factory_object_is_set:
 * @object: this #FMAIFactoryObject instance.
 * @name: the elementary data whose value is to be tested.
 *
 * Returns: %TRUE if the value is set (may be %NULL), %FALSE else.
 */
gboolean
fma_factory_object_is_set( const FMAIFactoryObject *object, const gchar *name )
{
	FMADataBoxed *boxed;

	g_return_val_if_fail( FMA_IS_IFACTORY_OBJECT( object ), FALSE );

	boxed = fma_ifactory_object_get_data_boxed( object, name );

	return( boxed != NULL );
}

/*
 * fma_factory_object_set_from_value:
 * @object: this #FMAIFactoryObject instance.
 * @name: the elementary data id.
 * @value: the #GValue whose content is to be got.
 *
 * Get from the @value the content to be set in the #FMADataBoxed
 * attached to @property_id.
 */
void
fma_factory_object_set_from_value( FMAIFactoryObject *object, const gchar *name, const GValue *value )
{
	static const gchar *thisfn = "fma_factory_object_set_from_value";

	g_return_if_fail( FMA_IS_IFACTORY_OBJECT( object ));

	FMADataBoxed *boxed = fma_ifactory_object_get_data_boxed( object, name );
	if( boxed ){
		fma_boxed_set_from_value( FMA_BOXED( boxed ), value );

	} else {
		FMADataDef *def = fma_factory_object_get_data_def( object, name );
		if( !def ){
			g_warning( "%s: unknown FMADataDef %s", thisfn, name );

		} else {
			boxed = fma_data_boxed_new( def );
			fma_boxed_set_from_value( FMA_BOXED( boxed ), value );
			attach_boxed_to_object( object, boxed );
		}
	}
}

/*
 * fma_factory_object_set_from_void:
 * @object: this #FMAIFactoryObject instance.
 * @name: the elementary data whose value is to be set.
 * @data: the value to set.
 *
 * Set the elementary data with the given value.
 */
void
fma_factory_object_set_from_void( FMAIFactoryObject *object, const gchar *name, const void *data )
{
	static const gchar *thisfn = "fma_factory_object_set_from_void";

	g_return_if_fail( FMA_IS_IFACTORY_OBJECT( object ));

	FMADataBoxed *boxed = fma_ifactory_object_get_data_boxed( object, name );
	if( boxed ){
		fma_boxed_set_from_void( FMA_BOXED( boxed ), data );

	} else {
		FMADataDef *def = fma_factory_object_get_data_def( object, name );
		if( !def ){
			g_warning( "%s: unknown FMADataDef %s for %s", thisfn, name, G_OBJECT_TYPE_NAME( object ));

		} else {
			boxed = fma_data_boxed_new( def );
			fma_boxed_set_from_void( FMA_BOXED( boxed ), data );
			attach_boxed_to_object( object, boxed );
		}
	}
}

static FMADataGroup *
v_get_groups( const FMAIFactoryObject *object )
{
	if( FMA_IFACTORY_OBJECT_GET_INTERFACE( object )->get_groups ){
		return( FMA_IFACTORY_OBJECT_GET_INTERFACE( object )->get_groups( object ));
	}

	return( NULL );
}

static void
v_copy( FMAIFactoryObject *target, const FMAIFactoryObject *source )
{
	if( FMA_IFACTORY_OBJECT_GET_INTERFACE( target )->copy ){
		FMA_IFACTORY_OBJECT_GET_INTERFACE( target )->copy( target, source );
	}
}

static gboolean
v_are_equal( const FMAIFactoryObject *a, const FMAIFactoryObject *b )
{
	if( FMA_IFACTORY_OBJECT_GET_INTERFACE( a )->are_equal ){
		return( FMA_IFACTORY_OBJECT_GET_INTERFACE( a )->are_equal( a, b ));
	}

	return( TRUE );
}

static gboolean
v_is_valid( const FMAIFactoryObject *object )
{
	if( FMA_IFACTORY_OBJECT_GET_INTERFACE( object )->is_valid ){
		return( FMA_IFACTORY_OBJECT_GET_INTERFACE( object )->is_valid( object ));
	}

	return( TRUE );
}

static void
v_read_start( FMAIFactoryObject *serializable, const FMAIFactoryProvider *reader, void *reader_data, GSList **messages )
{
	if( FMA_IFACTORY_OBJECT_GET_INTERFACE( serializable )->read_start ){
		FMA_IFACTORY_OBJECT_GET_INTERFACE( serializable )->read_start( serializable, reader, reader_data, messages );
	}
}

static void
v_read_done( FMAIFactoryObject *serializable, const FMAIFactoryProvider *reader, void *reader_data, GSList **messages )
{
	if( FMA_IFACTORY_OBJECT_GET_INTERFACE( serializable )->read_done ){
		FMA_IFACTORY_OBJECT_GET_INTERFACE( serializable )->read_done( serializable, reader, reader_data, messages );
	}
}

static guint
v_write_start( FMAIFactoryObject *serializable, const FMAIFactoryProvider *writer, void *writer_data, GSList **messages )
{
	guint code = IIO_PROVIDER_CODE_OK;

	if( FMA_IFACTORY_OBJECT_GET_INTERFACE( serializable )->write_start ){
		code = FMA_IFACTORY_OBJECT_GET_INTERFACE( serializable )->write_start( serializable, writer, writer_data, messages );
	}

	return( code );
}

static guint
v_write_done( FMAIFactoryObject *serializable, const FMAIFactoryProvider *writer, void *writer_data, GSList **messages )
{
	guint code = IIO_PROVIDER_CODE_OK;

	if( FMA_IFACTORY_OBJECT_GET_INTERFACE( serializable )->write_done ){
		code = FMA_IFACTORY_OBJECT_GET_INTERFACE( serializable )->write_done( serializable, writer, writer_data, messages );
	}

	return( code );
}

static void
attach_boxed_to_object( FMAIFactoryObject *object, FMADataBoxed *boxed )
{
	GList *list = g_object_get_data( G_OBJECT( object ), FMA_IFACTORY_OBJECT_PROP_DATA );
	list = g_list_prepend( list, boxed );
	g_object_set_data( G_OBJECT( object ), FMA_IFACTORY_OBJECT_PROP_DATA, list );
}

static void
free_data_boxed_list( FMAIFactoryObject *object )
{
	GList *list;

	list = g_object_get_data( G_OBJECT( object ), FMA_IFACTORY_OBJECT_PROP_DATA );

	g_list_foreach( list, ( GFunc ) g_object_unref, NULL );
	g_list_free( list );

	g_object_set_data( G_OBJECT( object ), FMA_IFACTORY_OBJECT_PROP_DATA, NULL );
}

/*
 * the iter function must return TRUE to stops the enumeration
 */
static void
iter_on_data_defs( const FMADataGroup *groups, guint mode, FMADataDefIterFunc pfn, void *user_data )
{
	static const gchar *thisfn = "fma_factory_object_iter_on_data_defs";
	FMADataDef *def;
	gboolean stop;

	stop = FALSE;

	while( groups->group && !stop ){

		if( groups->def ){

			def = groups->def;
			while( def->name && !stop ){

				/*g_debug( "serializable_only=%s, def->serializable=%s",
						serializable_only ? "True":"False", def->serializable ? "True":"False" );*/

				switch( mode ){
					case DATA_DEF_ITER_SET_PROPERTIES:
						if( def->has_property ){
							stop = ( *pfn )( def, user_data );
						}
						break;

					case DATA_DEF_ITER_SET_DEFAULTS:
						if( def->default_value ){
							stop = ( *pfn )( def, user_data );
						}
						break;

					case DATA_DEF_ITER_IS_VALID:
						stop = ( *pfn )( def, user_data );
						break;

					case DATA_DEF_ITER_READ_ITEM:
						if( def->readable ){
							stop = ( *pfn )( def, user_data );
						}
						break;

					default:
						g_warning( "%s: unknown mode=%d", thisfn, mode );
				}

				def++;
			}
		}

		groups++;
	}
}
