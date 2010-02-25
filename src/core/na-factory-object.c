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
#include <string.h>

#include <api/na-core-utils.h>
#include <api/na-data-types.h>
#include <api/na-ifactory-provider.h>
#include <api/na-data-boxed.h>

#include "na-factory-object.h"
#if 0
#include "na-ifactory-object-priv.h"
#endif
#include "na-factory-provider.h"

typedef gboolean ( *NADataDefIterFunc )( NADataDef *def, void *user_data );

/* while iterating on read/write item
 */
typedef struct {
	NAIFactoryObject   *object;
	NAIFactoryProvider *reader;
	void               *reader_data;
	GSList            **messages;
}
	NafoRWIter;

/* while iterating on is_valid
 */
typedef struct {
	NAIFactoryObject  *object;
	gboolean           is_valid;
}
	NafoValidIter;

#if 0
/* while iterating on set defaults
 */
typedef struct {
	NAIFactoryObject *object;
	gboolean          creation;
}
	NadfNewIter;
#endif

#define NA_IFACTORY_OBJECT_PROP_DATA				"na-ifactory-object-prop-data"

extern gboolean                   ifactory_object_initialized;
extern gboolean                   ifactory_object_finalized;
#if 0
extern NAIFactoryObjectInterface *ifactory_object_klass;
#endif

static gboolean     define_class_properties_iter( const NADataDef *def, GObjectClass *class );
static gboolean     factory_object_is_valid_mandatory_iter( const NADataDef *def, NafoValidIter *data );
static gboolean     factory_object_read_data_iter( NADataDef *def, NafoRWIter *iter );
static gboolean     factory_object_write_data_iter( NADataDef *def, NafoRWIter *iter );

static NADataGroup *v_get_groups( const NAIFactoryObject *object );
static void         v_copy( NAIFactoryObject *target, const NAIFactoryObject *source );
static gboolean     v_are_equal( const NAIFactoryObject *a, const NAIFactoryObject *b );
static gboolean     v_is_valid( const NAIFactoryObject *object );
static void         v_read_start( NAIFactoryObject *serializable, const NAIFactoryProvider *reader, void *reader_data, GSList **messages );
static void         v_read_done( NAIFactoryObject *serializable, const NAIFactoryProvider *reader, void *reader_data, GSList **messages );
static void         v_write_start( NAIFactoryObject *serializable, const NAIFactoryProvider *reader, void *reader_data, GSList **messages );
static void         v_write_done( NAIFactoryObject *serializable, const NAIFactoryProvider *reader, void *reader_data, GSList **messages );

static void         attach_boxed_to_object( NAIFactoryObject *object, NADataBoxed *boxed );
static NADataBoxed *data_boxed_from_name( const NAIFactoryObject *object, const gchar *name );
static void         free_data_boxed_list( NAIFactoryObject *object );
static void         iter_on_data_defs( const NADataGroup *idgroups, gboolean serializable_only, NADataDefIterFunc pfn, void *user_data );

#if 0
static gboolean     factory_object_init_instance_iter( const NADataDef *def, NAIFactoryObject *object );
static gchar       *v_get_default( const NAIFactoryObject *object, const NADataDef *def );
#endif

#if 0
/**
 * na_factory_object_register_type:
 * @type: the #GType of the implementation class.
 * @groups: a table of #NADataGroup structures which defines the
 *  serializable properties which will be attached to each instance of
 *  this class.
 *
 * Registers the implementation #GType @type.
 */
void
na_factory_object_register_type( GType type, const NADataGroup *groups )
{
	static const gchar *thisfn = "na_factory_object_register_type";
	NADataImplement *known;
	NADataGroup *registered;

	if( ifactory_object_initialized && !ifactory_object_finalized ){

		g_debug( "%s: type=%lu, groups=%p",
				thisfn, ( unsigned long ) type, ( void * ) groups );

		g_return_if_fail( groups != NULL );

		registered = na_factory_object_get_data_groups_from_type( type );
		if( registered ){
			g_warning( "%s: type=%lu: already registered", thisfn, ( unsigned long ) type );

		} else {
			/* register the implementation
			 */
			known = g_new0( NADataImplement, 1 );
			known->type = type;
			known->groups = ( NADataGroup * ) groups;

			ifactory_object_klass->private->registered =
					g_list_prepend( ifactory_object_klass->private->registered, known );
		}
	}
}
#endif

/**
 * na_factory_object_define_properties:
 * @class: the #GObjectClass.
 * @groups: the list of #NADataGroup structure which define the data of the class.
 *
 * Initializes all the properties for the class.
 */
void
na_factory_object_define_properties( GObjectClass *class, const NADataGroup *groups )
{
	static const gchar *thisfn = "na_factory_object_define_properties";

	if( ifactory_object_initialized && !ifactory_object_finalized ){

		g_debug( "%s: class=%p (%s)",
				thisfn, ( void * ) class, G_OBJECT_CLASS_NAME( class ));

		g_return_if_fail( G_IS_OBJECT_CLASS( class ));

		/* define class properties
		 */
		iter_on_data_defs( groups, FALSE, ( NADataDefIterFunc ) &define_class_properties_iter, class );
	}
}

static gboolean
define_class_properties_iter( const NADataDef *def, GObjectClass *class )
{
	static const gchar *thisfn = "na_factory_object_define_class_properties_iter";
	gboolean stop;
	GParamSpec *spec;

	g_debug( "%s: def=%p (%s)", thisfn, ( void * ) def, def->name );

	stop = FALSE;

	if( !def->obsoleted ){
		spec = na_data_boxed_get_param_spec( def );

		if( spec ){
			g_object_class_install_property( class, g_quark_from_string( def->name ), spec );

		} else {
			g_warning( "%s: type=%d: unable to get a spec", thisfn, def->type );
		}
	}

	return( stop );
}

/**
 * na_factory_object_get_data_def:
 * @object: this #NAIFactoryObject object.
 * @name: the searched name.
 *
 * Returns: the #NADataDef structure which describes this @name, or %NULL.
 */
NADataDef *
na_factory_object_get_data_def( const NAIFactoryObject *object, const gchar *name )
{
	NADataDef *def;

	g_return_val_if_fail( NA_IS_IFACTORY_OBJECT( object ), NULL );

	def = NULL;

	if( ifactory_object_initialized && !ifactory_object_finalized ){

		NADataGroup *groups = v_get_groups( object );
		while( groups->group ){

			NADataDef *def = groups->def;
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
	}

	return( def );
}

/**
 * na_factory_object_get_data_groups:
 * @object: the #NAIFactoryObject instance.
 *
 * Returns: a pointer to the list of #NADataGroup which define the data.
 */
NADataGroup *
na_factory_object_get_data_groups( const NAIFactoryObject *object )
{
	NADataGroup *groups;

	g_return_val_if_fail( NA_IS_IFACTORY_OBJECT( object ), NULL );

	groups = NULL;

	if( ifactory_object_initialized && !ifactory_object_finalized ){

		groups = v_get_groups( object );
	}

	return( groups );
}

/**
 * na_factory_object_iter_on_boxed:
 * @object: this #NAIFactoryObject object.
 * @fn: the function to be called.
 * @user_data: data to be provided to the user function.
 *
 * Iterate on each #NADataBoxed attached to the @object.
 *
 * The @fn called function may return %TRUE to stop the iteration.
 */
void
na_factory_object_iter_on_boxed( const NAIFactoryObject *object, NAFactoryObjectIterBoxedFn pfn, void *user_data )
{
	GList *list, *ibox;
	gboolean stop;

	g_return_if_fail( NA_IS_IFACTORY_OBJECT( object ));

	if( ifactory_object_initialized && !ifactory_object_finalized ){

		list = g_object_get_data( G_OBJECT( object ), NA_IFACTORY_OBJECT_PROP_DATA );
		/*g_debug( "list=%p (count=%u)", ( void * ) list, g_list_length( list ));*/
		stop = FALSE;

		for( ibox = list ; ibox && !stop ; ibox = ibox->next ){
			stop = ( *pfn )( object, NA_DATA_BOXED( ibox->data ), user_data );
		}
	}
}

/**
 * na_factory_object_attach_with_default:
 * @object: this #NAIFactoryObject object.
 * @name: a #NADataDef name.
 *
 * Attach a new #NADataBoxed element to this @object, initializing it
 * with its default value.
 */
void
na_factory_object_attach_with_default( NAIFactoryObject *object, const gchar *name )
{
	static const gchar *thisfn = "na_factory_object_attach_with_default";

	g_return_if_fail( NA_IS_IFACTORY_OBJECT( object ));

	if( ifactory_object_initialized && !ifactory_object_finalized ){

		NADataDef *def = na_factory_object_get_data_def( object, name );

		if( !def ){
			g_warning( "%s: no NADataDef found for %s", thisfn, name );

		} else {
			NADataBoxed *boxed = na_data_boxed_new( def );
			na_data_boxed_set_default( boxed );
			attach_boxed_to_object( object, boxed );
		}
	}
}

/**
 * na_factory_object_move_boxed:
 * @target: the target #NAIFactoryObject instance.
 * @source: the source #NAIFactoryObject instance.
 * @boxed: a #NADataBoxed.
 *
 * Move the @boxed from @source to @target, detaching from @source list
 * to be attached to @target one.
 */
void
na_factory_object_move_boxed( NAIFactoryObject *target, const NAIFactoryObject *source, NADataBoxed *boxed )
{
	g_return_if_fail( NA_IS_IFACTORY_OBJECT( target ));
	g_return_if_fail( NA_IS_IFACTORY_OBJECT( source ));

	if( ifactory_object_initialized && !ifactory_object_finalized ){

		GList *src_list = g_object_get_data( G_OBJECT( source ), NA_IFACTORY_OBJECT_PROP_DATA );

		if( g_list_find( src_list, boxed )){
			src_list = g_list_remove( src_list, boxed );
			g_object_set_data( G_OBJECT( source ), NA_IFACTORY_OBJECT_PROP_DATA, src_list );

			attach_boxed_to_object( target, boxed );

			NADataDef *src_def = na_data_boxed_get_data_def( boxed );
			NADataDef *tgt_def = na_factory_object_get_data_def( target, src_def->name );
			na_data_boxed_set_data_def( boxed, tgt_def );
		}
	}
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

	free_data_boxed_list( target );

	src_list = g_object_get_data( G_OBJECT( source ), NA_IFACTORY_OBJECT_PROP_DATA );

	for( isrc = src_list ; isrc ; isrc = isrc->next ){

		NADataBoxed *src_boxed = NA_DATA_BOXED( isrc->data );
		NADataDef *def = na_data_boxed_get_data_def( src_boxed );
		if( def->copyable ){

			NADataBoxed *tgt_boxed = na_data_boxed_new( def );
			na_data_boxed_set_from_boxed( tgt_boxed, src_boxed );
			attach_boxed_to_object( target, tgt_boxed );
		}
	}

	v_copy( target, source );
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

	are_equal = FALSE;

	a_list = g_object_get_data( G_OBJECT( a ), NA_IFACTORY_OBJECT_PROP_DATA );
	b_list = g_object_get_data( G_OBJECT( b ), NA_IFACTORY_OBJECT_PROP_DATA );

	if( g_list_length( a_list ) == g_list_length( b_list )){

		are_equal = TRUE;
		for( ia = a_list ; ia && are_equal ; ia = ia->next ){

			NADataBoxed *a_boxed = NA_DATA_BOXED( ia->data );
			NADataDef *a_def = na_data_boxed_get_data_def( a_boxed );
			if( a_def->comparable ){

				NADataBoxed *b_boxed = data_boxed_from_name( b, a_def->name );
				if( b_boxed ){
					are_equal = na_data_boxed_are_equal( a_boxed, b_boxed );

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

/**
 * na_factory_object_is_valid:
 * @object: the #NAIFactoryObject instance whose validity is to be checked.
 *
 * Returns: %TRUE if @object is valid, %FALSE else.
 */
gboolean
na_factory_object_is_valid( const NAIFactoryObject *object )
{
	gboolean is_valid;
	NADataGroup *groups;
	GList *list, *iv;

	g_return_val_if_fail( NA_IS_IFACTORY_OBJECT( object ), FALSE );

	list = g_object_get_data( G_OBJECT( object ), NA_IFACTORY_OBJECT_PROP_DATA );
	is_valid = TRUE;

	/* mndatory data must be set
	 */
	NafoValidIter iter_data;
	iter_data.object = ( NAIFactoryObject * ) object;
	iter_data.is_valid = TRUE;

	groups = v_get_groups( object );
	if( groups ){
		iter_on_data_defs( groups, FALSE, ( NADataDefIterFunc ) factory_object_is_valid_mandatory_iter, &iter_data );
	}
	is_valid = iter_data.is_valid;

	for( iv = list ; iv && is_valid ; iv = iv->next ){
		is_valid = na_data_boxed_is_valid( NA_DATA_BOXED( iv->data ));
	}

	if( is_valid ){
		is_valid = v_is_valid( object );
	}

	return( is_valid );
}

static gboolean
factory_object_is_valid_mandatory_iter( const NADataDef *def, NafoValidIter *data )
{
	NADataBoxed *boxed;

	if( def->mandatory && !def->obsoleted ){
		boxed = data_boxed_from_name( data->object, def->name );
		if( !boxed ){
			g_debug( "na_factory_object_is_valid_mandatory_iter: invalid %s: mandatory but not set", def->name );
			data->is_valid = FALSE;
		}
	}

	/* do not stop iteration while valid */
	return( !data->is_valid );
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
	static const gchar *thisfn = "na_factory_object_dump";
	static const gchar *prefix = "na-factory-data-";
	GList *list, *it;
	guint length;
	guint l_prefix;

	length = 0;
	l_prefix = strlen( prefix );
	list = g_object_get_data( G_OBJECT( object ), NA_IFACTORY_OBJECT_PROP_DATA );

	for( it = list ; it ; it = it->next ){
		NADataBoxed *boxed = NA_DATA_BOXED( it->data );
		NADataDef *def = na_data_boxed_get_data_def( boxed );
		length = MAX( length, strlen( def->name ));
	}

	length -= l_prefix;
	length += 1;

	for( it = list ; it ; it = it->next ){
		/*na_data_boxed_dump( NA_DATA_BOXED( it->data ));*/
		NADataBoxed *boxed = NA_DATA_BOXED( it->data );
		NADataDef *def = na_data_boxed_get_data_def( boxed );
		gchar *value = na_data_boxed_get_as_string( boxed );
		g_debug( "%s: %*s=%s", thisfn, length, def->name+l_prefix, value );
		g_free( value );
	}
}

#if 0
/**
 * na_factory_object_init_instance:
 * @object: the #NAIFactoryObject being initialized.
 *
 * Initializes properties attached to the @object.
 *
 * This essentially consists of creating a #NADataBoxed for each
 * defined elementary data, initializing it to its default value.
 */
void
na_factory_object_init_instance( NAIFactoryObject *object )
{
	static const gchar *thisfn = "na_factory_object_init_instance";
	NADataGroup *groups;

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));
	g_return_if_fail( NA_IS_IFACTORY_OBJECT( object ));

	/* do not allocate any NADataBoxed now
	 * just ensure that the GList which will host them is initialized
	 */
	g_object_set_data( G_OBJECT( object ), NA_IFACTORY_OBJECT_PROP_DATA, NULL );

	if( FALSE ){
		groups = na_factory_object_get_data_groups_from_type( G_OBJECT_TYPE( object ));
		if( groups ){
			iter_on_data_defs( groups, FALSE, ( NADataDefIterFunc ) &factory_object_init_instance_iter, object );

		} else {
			g_warning( "%s: class=%s: groups not found", thisfn, G_OBJECT_TYPE_NAME( object ));
		}
	}
}

static gboolean
factory_object_init_instance_iter( const NADataDef *def, NAIFactoryObject *object )
{
	gboolean stop;
	GList *list;
	NADataBoxed *element;
	NadfDataValue *data;
	gchar *default_value;

	stop = FALSE;

	default_value = v_get_default( object, def );
	element = na_data_boxed_new( def );
	na_data_boxed_set_from_string( element, ( const void * )( default_value ? default_value : def->default_value ));
	g_free( default_value );

	data = g_new0( NadfDataValue, 1 );
	data->def = ( NADataDef * ) def;
	data->element = element;

	list = g_object_get_data( G_OBJECT( object ), NA_IFACTORY_OBJECT_PROP_DATA );
	list = g_list_prepend( list, data );
	g_object_set_data( G_OBJECT( object ), NA_IFACTORY_OBJECT_PROP_DATA, list );

	return( stop );
}
#endif

/**
 * na_factory_object_finalize_instance:
 * @object: the #NAIFactoryObject being finalized.
 *
 * Clears all data associated with this @object.
 */
void
na_factory_object_finalize_instance( NAIFactoryObject *object )
{
	free_data_boxed_list( object );
}

/**
 * na_factory_object_read_item:
 * @serializable: this #NAIFactoryObject instance.
 * @reader: the #NAIFactoryProvider which is at the origin of this read.
 * @reader_data: reader data.
 * @messages: a pointer to a #GSList list of strings; the implementation
 *  may append messages to this list, but shouldn't reinitialize it.
 *
 * Unserializes the object.
 */
void
na_factory_object_read_item( NAIFactoryObject *serializable, const NAIFactoryProvider *reader, void *reader_data, GSList **messages )
{
	static const gchar *thisfn = "na_factory_object_read_item";

	if( ifactory_object_initialized && !ifactory_object_finalized ){

		g_return_if_fail( NA_IS_IFACTORY_OBJECT( serializable ));
		g_return_if_fail( NA_IS_IFACTORY_PROVIDER( reader ));

		NADataGroup *groups = v_get_groups( serializable );

		if( groups ){
			v_read_start( serializable, reader, reader_data, messages );

			NafoRWIter *iter = g_new0( NafoRWIter, 1 );
			iter->object = serializable;
			iter->reader = ( NAIFactoryProvider * ) reader;
			iter->reader_data = reader_data;
			iter->messages = messages;

			iter_on_data_defs( groups, TRUE, ( NADataDefIterFunc ) &factory_object_read_data_iter, iter );

			g_free( iter );

			v_read_done( serializable, reader, reader_data, messages );

		} else {
			g_warning( "%s: class %s doesn't return any NADataGroup structure",
					thisfn, G_OBJECT_TYPE_NAME( serializable ));
		}
	}
}

static gboolean
factory_object_read_data_iter( NADataDef *def, NafoRWIter *iter )
{
	gboolean stop;

	stop = FALSE;

	NADataBoxed *boxed = na_factory_provider_read_data( iter->reader, iter->reader_data, iter->object, def, iter->messages );

	if( boxed ){
		NADataBoxed *exist = data_boxed_from_name( iter->object, def->name );

		if( exist ){
			na_data_boxed_set_from_boxed( exist, boxed );
			g_object_unref( boxed );

		} else {
			attach_boxed_to_object( iter->object, boxed );
		}
	}

	return( stop );
}

/**
 * na_factory_object_write_item:
 * @serializable: this #NAIFactoryObject instance.
 * @writer: the #NAIFactoryProvider which is at the origin of this write.
 * @writer_data: writer data.
 * @messages: a pointer to a #GSList list of strings; the implementation
 *  may append messages to this list, but shouldn't reinitialize it.
 *
 * Serializes the object down to the @writer.
 */
void
na_factory_object_write_item( NAIFactoryObject *serializable, const NAIFactoryProvider *writer, void *writer_data, GSList **messages )
{
	static const gchar *thisfn = "na_factory_object_write_item";
	NADataGroup *groups;
	gchar *msg;

	g_return_if_fail( NA_IS_IFACTORY_OBJECT( serializable ));
	g_return_if_fail( NA_IS_IFACTORY_PROVIDER( writer ));

	groups = v_get_groups( serializable );
	if( groups ){

		v_write_start( serializable, writer, writer_data, messages );

		NafoRWIter *iter = g_new0( NafoRWIter, 1 );
		iter->object = serializable;
		iter->reader = ( NAIFactoryProvider * ) writer;
		iter->reader_data = writer_data;
		iter->messages = messages;

		iter_on_data_defs( groups, TRUE, ( NADataDefIterFunc ) &factory_object_write_data_iter, iter );

		g_free( iter );

		v_write_done( serializable, writer, writer_data, messages );

	} else {
		msg = g_strdup_printf( "%s: class %s doesn't return any NADataGroup structure",
				thisfn, G_OBJECT_TYPE_NAME( serializable ));
		g_warning( "%s", msg );
		*messages = g_slist_append( *messages, msg );
	}
}

static gboolean
factory_object_write_data_iter( NADataDef *def, NafoRWIter *iter )
{
	gboolean stop;

	stop = FALSE;

	/*na_factory_provider_set_value( iter->reader, iter->reader_data, def, iter->messages );*/

	return( stop );
}

#if 0
/**
 * na_factory_provider_get_data_groups:
 * @type: a previously registered #GType.
 *
 * Returns the #NADataGroups table which has been registered for this @type,
 * or %NULL.
 */
NADataGroup *
na_factory_object_get_data_groups_from_type( GType type )
{
	GList *it;

	if( ifactory_object_initialized && !ifactory_object_finalized ){

		for( it = ifactory_object_klass->private->registered ; it ; it = it->next ){
			if((( NADataImplement * ) it->data )->type == type ){
				return((( NADataImplement * ) it->data )->groups );
			}
		}
	}

	return( NULL );
}
#endif

/**
 * na_factory_object_get_as_value:
 * @object: this #NAIFactoryObject instance.
 * @property_id: the elementary data id.
 * @value: the #GValue to be set.
 * @spec: the #GParamSpec which describes this data.
 *
 * Set the @value with the current content of the #NADataBoxed attached
 * to @property_id.
 *
 * This is to be readen as "set value from data element".
 */
void
na_factory_object_get_as_value( const NAIFactoryObject *object, const gchar *name, GValue *value )
{
	NADataBoxed *boxed;

	g_return_if_fail( NA_IS_IFACTORY_OBJECT( object ));

	g_value_unset( value );

	boxed = data_boxed_from_name( object, name );
	if( boxed ){
		na_data_boxed_get_as_value( boxed, value );
	}
}

/**
 * na_factory_object_get_as_void:
 * @object: this #NAIFactoryObject instance.
 * @name: the elementary data whose value is to be got.
 *
 * Returns: the searched value.
 *
 * If the type of the value is NAFD_TYPE_STRING, NAFD_TYPE_LOCALE_STRING,
 * or NAFD_TYPE_STRING_LIST, then the returned value is a newly allocated
 * one and should be g_free() (resp. na_core_utils_slist_free()) by the
 * caller.
 */
void *
na_factory_object_get_as_void( const NAIFactoryObject *object, const gchar *name )
{
	void *value;
	NADataBoxed *boxed;

	g_return_val_if_fail( NA_IS_IFACTORY_OBJECT( object ), NULL );

	value = NULL;

	boxed = data_boxed_from_name( object, name );
	if( boxed ){
		value = na_data_boxed_get_as_void( boxed );
	}

	return( value );
}

/**
 * na_factory_object_set_from_value:
 * @object: this #NAIFactoryObject instance.
 * @name: the elementary data id.
 * @value: the #GValue whose content is to be got.
 *
 * Get from the @value the content to be set in the #NADataBoxed
 * attached to @property_id.
 */
void
na_factory_object_set_from_value( NAIFactoryObject *object, const gchar *name, const GValue *value )
{
	static const gchar *thisfn = "na_factory_object_set_from_value";

	g_return_if_fail( NA_IS_IFACTORY_OBJECT( object ));

	NADataBoxed *boxed = data_boxed_from_name( object, name );
	if( boxed ){
		na_data_boxed_set_from_value( boxed, value );

	} else {
		NADataDef *def = na_factory_object_get_data_def( object, name );
		if( !def ){
			g_warning( "%s: unknown NADataDef %s", thisfn, name );

		} else {
			boxed = na_data_boxed_new( def );
			na_data_boxed_set_from_value( boxed, value );
			attach_boxed_to_object( object, boxed );
		}
	}
}

/**
 * na_factory_object_set_from_void:
 * @object: this #NAIFactoryObject instance.
 * @name: the elementary data whose value is to be set.
 * @data: the value to set.
 *
 * Set the elementary data with the given value.
 */
void
na_factory_object_set_from_void( NAIFactoryObject *object, const gchar *name, const void *data )
{
	static const gchar *thisfn = "na_factory_object_set_from_void";

	g_return_if_fail( NA_IS_IFACTORY_OBJECT( object ));

	NADataBoxed *boxed = data_boxed_from_name( object, name );
	if( boxed ){
		na_data_boxed_set_from_void( boxed, data );

	} else {
		NADataDef *def = na_factory_object_get_data_def( object, name );
		if( !def ){
			g_warning( "%s: unknown NADataDef %s", thisfn, name );

		} else {
			boxed = na_data_boxed_new( def );
			na_data_boxed_set_from_void( boxed, data );
			attach_boxed_to_object( object, boxed );
		}
	}
}

static NADataGroup *
v_get_groups( const NAIFactoryObject *object )
{
	NADataGroup *groups;

	groups = NULL;

	if( NA_IFACTORY_OBJECT_GET_INTERFACE( object )->get_groups ){
		groups = NA_IFACTORY_OBJECT_GET_INTERFACE( object )->get_groups( object );
	}

	return( groups );
}

static void
v_copy( NAIFactoryObject *target, const NAIFactoryObject *source )
{
	if( NA_IFACTORY_OBJECT_GET_INTERFACE( target )->copy ){
		NA_IFACTORY_OBJECT_GET_INTERFACE( target )->copy( target, source );
	}
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

#if 0
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
	NADataGroup *groups;

	object = NULL;

	/* check that @type has been registered
	 */
	groups = na_factory_object_get_data_groups_from_type( type );
	if( groups ){

		object = NA_IFACTORY_OBJECT( g_object_new( type, NULL ));
	}

	return( object );
}

static gchar *
v_get_default( const NAIFactoryObject *object, const NADataDef *def )
{
	gchar *default_value;

	default_value = NULL;

	if( NA_IFACTORY_OBJECT_GET_INTERFACE( object )->get_default ){
		default_value = NA_IFACTORY_OBJECT_GET_INTERFACE( object )->get_default( object, def );
	}

	return( default_value );
}
#endif

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

#if 0
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

	g_return_if_fail( NA_IS_IFACTORY_OBJECT( object ));

	NADataBoxed *boxed = data_boxed_from_name( object, data_id );
	if( boxed ){
		na_data_boxed_set_from_string( boxed, string );

	} else {
		g_warning( "%s: unknown property id %d", thisfn, data_id );
	}
}
#endif

static void
attach_boxed_to_object( NAIFactoryObject *object, NADataBoxed *boxed )
{
	GList *list = g_object_get_data( G_OBJECT( object ), NA_IFACTORY_OBJECT_PROP_DATA );
	list = g_list_prepend( list, boxed );
	g_object_set_data( G_OBJECT( object ), NA_IFACTORY_OBJECT_PROP_DATA, list );
}

static NADataBoxed *
data_boxed_from_name( const NAIFactoryObject *object, const gchar *name )
{
	GList *list, *ip;

	list = g_object_get_data( G_OBJECT( object ), NA_IFACTORY_OBJECT_PROP_DATA );
	/*g_debug( "list=%p (count=%u)", ( void * ) list, g_list_length( list ));*/

	for( ip = list ; ip ; ip = ip->next ){
		NADataBoxed *boxed = NA_DATA_BOXED( ip->data );
		NADataDef *def = na_data_boxed_get_data_def( boxed );

		if( !strcmp( def->name, name )){
			return( boxed );
		}
	}

	return( NULL );
}

static void
free_data_boxed_list( NAIFactoryObject *object )
{
	GList *list, *it;

	list = g_object_get_data( G_OBJECT( object ), NA_IFACTORY_OBJECT_PROP_DATA );

	for( it = list ; it ; it = it->next ){

		g_object_unref( it->data );
	}

	g_list_free( list );

	g_object_set_data( G_OBJECT( object ), NA_IFACTORY_OBJECT_PROP_DATA, NULL );
}

/*
 * the iter function must return TRUE to stops the enumeration
 */
static void
iter_on_data_defs( const NADataGroup *groups, gboolean serializable_only, NADataDefIterFunc pfn, void *user_data )
{
	NADataDef *def;
	gboolean stop;

	stop = FALSE;

	while( groups->group && !stop ){

		if( groups->def ){

			def = groups->def;
			while( def->name && !stop ){

				/*g_debug( "serializable_only=%s, def->serializable=%s",
						serializable_only ? "True":"False", def->serializable ? "True":"False" );*/

				if( !serializable_only || def->serializable ){
					stop = ( *pfn )( def, user_data );
				}

				def++;
			}
		}

		groups++;
	}
}
