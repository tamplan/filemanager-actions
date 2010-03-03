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

#include <gio/gio.h>
#include <libxml/tree.h>
#include <string.h>

#include <api/na-core-utils.h>
#include <api/na-data-types.h>
#include <api/na-object-api.h>
#include <api/na-ifactory-provider.h>
#include <api/na-iio-provider.h>

#include <io-gconf/nagp-keys.h>

#include "naxml-formats.h"
#include "naxml-keys.h"
#include "naxml-writer.h"

typedef struct ExportFormatFn ExportFormatFn;

/* private class data
 */
struct NAXMLWriterClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NAXMLWriterPrivate {
	gboolean         dispose_has_run;
	NAIExporter     *provider;
	NAObjectItem    *exported;
	GSList          *messages;

	/* positionning these at document level
	 */
	xmlDocPtr        doc;
	ExportFormatFn  *fn_str;
	gchar           *buffer;

	/* the list node is parent of all entry nodes
	 * it is set in build_xml_doc() to be used as a parent in each
	 * entry node
	 */
	xmlNodePtr       list_node;

	/* nodes created in write_data_schema_v2(), used in write_data_schema_v1()
	 */
	xmlNodePtr       schema_node;
	xmlNodePtr       locale_node;

	/* when exporting a profile
	 * it is set in write_done when exporting a profile
	 */
	NAObjectAction  *action;
};

/* the association between an export format and the functions
 */
struct ExportFormatFn {
	gchar  *format;
	gchar  *root_node;
	gchar  *list_node;
	gchar  *element_node;
	void ( *write_data_fn )( NAXMLWriter *, const NAObjectId *, const NADataBoxed * );
};

static GObjectClass *st_parent_class = NULL;

static GType           register_type( void );
static void            class_init( NAXMLWriterClass *klass );
static void            instance_init( GTypeInstance *instance, gpointer klass );
static void            instance_dispose( GObject *object );
static void            instance_finalize( GObject *object );

static void            write_data_schema_v1( NAXMLWriter *writer, const NAObjectId *object, const NADataBoxed *boxed );
static void            write_data_schema_v2( NAXMLWriter *writer, const NAObjectId *object, const NADataBoxed *boxed );
static void            write_data_dump( NAXMLWriter *writer, const NAObjectId *object, const NADataBoxed *boxed );

static xmlDocPtr       build_xml_doc( NAXMLWriter *writer );
static ExportFormatFn *find_export_format_fn( GQuark format );
static gchar          *get_output_fname( const NAObjectItem *item, const gchar *folder, GQuark format );
static void            output_xml_to_file( const gchar *xml, const gchar *filename, GSList **msg );
static guint           writer_to_buffer( NAXMLWriter *writer );

static ExportFormatFn st_export_format_fn[] = {

	{ NAXML_FORMAT_GCONF_SCHEMA_V1,
					NAXML_KEY_SCHEMA_ROOT,
					NAXML_KEY_SCHEMA_LIST,
					NAXML_KEY_SCHEMA_NODE,
					write_data_schema_v1 },

	{ NAXML_FORMAT_GCONF_SCHEMA_V2,
					NAXML_KEY_SCHEMA_ROOT,
					NAXML_KEY_SCHEMA_LIST,
					NAXML_KEY_SCHEMA_NODE,
					write_data_schema_v2 },

	{ NAXML_FORMAT_GCONF_ENTRY,
					NAXML_KEY_DUMP_ROOT,
					NAXML_KEY_DUMP_LIST,
					NAXML_KEY_DUMP_NODE,
					write_data_dump },

	{ NULL }
};

GType
naxml_writer_get_type( void )
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
	static const gchar *thisfn = "naxml_writer_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NAXMLWriterClass ),
		NULL,
		NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NAXMLWriter ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "NAXMLWriter", &info, 0 );

	return( type );
}

static void
class_init( NAXMLWriterClass *klass )
{
	static const gchar *thisfn = "naxml_writer_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NAXMLWriterClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "naxml_writer_instance_init";
	NAXMLWriter *self;

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );
	g_return_if_fail( NAXML_IS_WRITER( instance ));
	self = NAXML_WRITER( instance );

	self->private = g_new0( NAXMLWriterPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "naxml_writer_instance_dispose";
	NAXMLWriter *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_return_if_fail( NAXML_IS_WRITER( object ));
	self = NAXML_WRITER( object );

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
	NAXMLWriter *self;

	g_return_if_fail( NAXML_IS_WRITER( object ));
	self = NAXML_WRITER( object );

	/* do not release self->private->buffer as the pointer has been
	 * returned to the caller
	 */

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * naxml_writer_export_to_buffer:
 * @instance: this #NAIExporter instance.
 * @parms: a #NAIExporterBufferParms structure.
 *
 * Export the specified 'item' to a newly allocated buffer.
 */
guint
naxml_writer_export_to_buffer( const NAIExporter *instance, NAIExporterBufferParms *parms )
{
	static const gchar *thisfn = "naxml_writer_export_to_buffer";
	NAXMLWriter *writer;
	guint code;

	g_debug( "%s: instance=%p, parms=%p", thisfn, ( void * ) instance, ( void * ) parms );

	code = NA_IEXPORTER_CODE_OK;

	if( !parms->exported || !NA_IS_OBJECT_ITEM( parms->exported )){
		code = NA_IEXPORTER_CODE_INVALID_ITEM;
	}

	if( code == NA_IEXPORTER_CODE_OK ){
		writer = NAXML_WRITER( g_object_new( NAXML_WRITER_TYPE, NULL ));

		writer->private->provider = ( NAIExporter * ) instance;
		writer->private->exported = parms->exported;
		writer->private->messages = parms->messages;
		writer->private->fn_str = find_export_format_fn( parms->format );
		writer->private->action = NULL;
		writer->private->buffer = NULL;

		if( !writer->private->fn_str ){
			code = NA_IEXPORTER_CODE_INVALID_FORMAT;

		} else {
			code = writer_to_buffer( writer );
			if( code == NA_IEXPORTER_CODE_OK ){
				parms->buffer = writer->private->buffer;
			}
		}

		g_object_unref( writer );
	}

	g_debug( "%s: returning code=%u", thisfn, code );
	return( code );
}

/**
 * naxml_writer_export_to_file:
 * @instance: this #NAIExporter instance.
 * @parms: a #NAIExporterFileParms structure.
 *
 * Export the specified 'item' to a newly created file.
 */
guint
naxml_writer_export_to_file( const NAIExporter *instance, NAIExporterFileParms *parms )
{
	static const gchar *thisfn = "naxml_writer_export_to_file";
	NAXMLWriter *writer;
	gchar *filename;
	guint code;

	g_debug( "%s: instance=%p, parms=%p", thisfn, ( void * ) instance, ( void * ) parms );

	code = NA_IEXPORTER_CODE_OK;

	if( !parms->exported || !NA_IS_OBJECT_ITEM( parms->exported )){
		code = NA_IEXPORTER_CODE_INVALID_ITEM;
	}

	if( code == NA_IEXPORTER_CODE_OK ){
		writer = NAXML_WRITER( g_object_new( NAXML_WRITER_TYPE, NULL ));

		writer->private->provider = ( NAIExporter * ) instance;
		writer->private->exported = parms->exported;
		writer->private->messages = parms->messages;
		writer->private->fn_str = find_export_format_fn( parms->format );
		writer->private->action = NULL;
		writer->private->buffer = NULL;

		if( !writer->private->fn_str ){
			code = NA_IEXPORTER_CODE_INVALID_FORMAT;

		} else {
			code = writer_to_buffer( writer );

			if( code == NA_IEXPORTER_CODE_OK ){
				filename = get_output_fname( parms->exported, parms->folder, parms->format );

				if( filename ){
					parms->basename = g_path_get_basename( filename );
					output_xml_to_file(
							writer->private->buffer, filename, parms->messages ? &writer->private->messages : NULL );
					g_free( filename );
				}
			}

			g_free( writer->private->buffer );
		}

		g_object_unref( writer );
	}

	g_debug( "%s: returning code=%u", thisfn, code );
	return( code );
}

static xmlDocPtr
build_xml_doc( NAXMLWriter *writer )
{
	xmlNodePtr root_node;

	writer->private->doc = xmlNewDoc( BAD_CAST( "1.0" ));

	root_node = xmlNewNode( NULL, BAD_CAST( writer->private->fn_str->root_node ));
	xmlDocSetRootElement( writer->private->doc, root_node );

	writer->private->list_node = xmlNewChild( root_node, NULL, BAD_CAST( writer->private->fn_str->list_node ), NULL );

	na_ifactory_provider_write_item(
			NA_IFACTORY_PROVIDER( writer->private->provider ),
			writer,
			NA_IFACTORY_OBJECT( writer->private->exported ),
			writer->private->messages ? & writer->private->messages : NULL );

	return( writer->private->doc );
}

guint
naxml_writer_write_start( const NAIFactoryProvider *provider, void *writer_data, const NAIFactoryObject *object, GSList **messages  )
{
	static const gchar *thisfn = "naxml_writer_write_start";
	NADataBoxed *boxed;
	NAXMLWriter *writer;

	if( NA_IS_OBJECT_ITEM( object )){

		na_object_dump( object );

		boxed = na_ifactory_object_get_data_boxed( object, NAFO_DATA_TYPE );

		if( boxed ){
			writer = NAXML_WRITER( writer_data );
			( *writer->private->fn_str->write_data_fn )( writer, NA_OBJECT_ID( object ), boxed );

		} else {
			g_warning( "%s: unable to get %s databox", thisfn, NAFO_DATA_TYPE );
		}
	}

	return( NA_IIO_PROVIDER_CODE_OK );
}

guint
naxml_writer_write_data( const NAIFactoryProvider *provider, void *writer_data, const NAIFactoryObject *object, const NADataBoxed *boxed, GSList **messages )
{
	guint code;
	NAXMLWriter *writer;

	/*NADataDef *def = na_data_boxed_get_data_def( boxed );
	g_debug( "naxml_writer_write_data: def=%s", def->name );*/

	code = NA_IIO_PROVIDER_CODE_OK;
	writer = NAXML_WRITER( writer_data );

	writer->private->schema_node = NULL;
	writer->private->locale_node = NULL;

	( *writer->private->fn_str->write_data_fn )( writer, NA_OBJECT_ID( object ), boxed );

	return( code );
}

guint
naxml_writer_write_done( const NAIFactoryProvider *provider, void *writer_data, const NAIFactoryObject *object, GSList **messages  )
{
	GList *children, *ic;

	if( NA_IS_OBJECT_ACTION( object )){

		NAXMLWriter *writer = NAXML_WRITER( writer_data );
		writer->private->action = NA_OBJECT_ACTION( object );
		children = na_object_get_items( object );

		for( ic = children ; ic ; ic = ic->next ){
			na_ifactory_provider_write_item( provider, writer, ( NAIFactoryObject * ) ic->data, messages );
		}
	}

	return( NA_IIO_PROVIDER_CODE_OK );
}

static void
write_data_schema_v1( NAXMLWriter *writer, const NAObjectId *object, const NADataBoxed *boxed )
{
	NADataDef *def;

	write_data_schema_v2( writer, object, boxed );

	def = na_data_boxed_get_data_def( boxed );

	if( !writer->private->locale_node ){
		writer->private->locale_node = xmlNewChild( writer->private->schema_node, NULL, BAD_CAST( NAXML_KEY_SCHEMA_NODE_LOCALE ), NULL );
		xmlNewProp( writer->private->locale_node, BAD_CAST( "name" ), BAD_CAST( "C" ));
	}

	xmlNewChild( writer->private->schema_node, NULL, BAD_CAST( NAXML_KEY_SCHEMA_NODE_OWNER ), BAD_CAST( PACKAGE_TARNAME ));

	xmlNewChild( writer->private->locale_node, NULL, BAD_CAST( NAXML_KEY_SCHEMA_NODE_LOCALE_SHORT ), BAD_CAST( def->short_label ));

	xmlNewChild( writer->private->locale_node, NULL, BAD_CAST( NAXML_KEY_SCHEMA_NODE_LOCALE_LONG ), BAD_CAST( def->long_label ));
}

/*
 * <schema>
 *  <key>/schemas/apps/nautilus-actions/configurations/entry</key>
 *  <applyto>/apps/nautilus-actions/configurations/item_id/profile_id/entry</applyto>
 */
static void
write_data_schema_v2( NAXMLWriter *writer, const NAObjectId *object, const NADataBoxed *boxed )
{
	gchar *object_id;
	NADataDef *def;
	xmlChar *content;
	xmlNodePtr parent_value_node;
	gchar *value_str;

	def = na_data_boxed_get_data_def( boxed );
	value_str = na_data_boxed_get_as_string( boxed );

	/* do no export empty values, but for string list
	 */
	if( !value_str || !strlen( value_str )){
		return;
	}

	/* boolean value must be lowercase
	 */
	if( def->type == NAFD_TYPE_BOOLEAN ){
		gchar *tmp = g_ascii_strdown( value_str, -1 );
		g_free( value_str );
		value_str = tmp;
	}

	writer->private->schema_node = xmlNewChild( writer->private->list_node, NULL, BAD_CAST( NAXML_KEY_SCHEMA_NODE ), NULL );

	object_id = na_object_get_id( object );

	if( writer->private->action ){
		gchar *id = na_object_get_id( writer->private->action );
		gchar *tmp = g_strdup_printf( "%s/%s", id, object_id );
		g_free( id );
		g_free( object_id );
		object_id = tmp;
	}

	content = BAD_CAST( g_build_path( "/", NAGP_SCHEMAS_PATH, def->gconf_entry, NULL ));
	xmlNewChild( writer->private->schema_node, NULL, BAD_CAST( NAXML_KEY_SCHEMA_NODE_KEY ), content );
	xmlFree( content );

	content = BAD_CAST( g_build_path( "/", NAGP_CONFIGURATIONS_PATH, object_id, def->gconf_entry, NULL ));
	xmlNewChild( writer->private->schema_node, NULL, BAD_CAST( NAXML_KEY_SCHEMA_NODE_APPLYTO ), content );
	xmlFree( content );

	xmlNewChild( writer->private->schema_node, NULL, BAD_CAST( NAXML_KEY_SCHEMA_NODE_TYPE ), BAD_CAST( na_data_types_get_gconf_dump_key( def->type )));
	if( def->type == NAFD_TYPE_STRING_LIST ){
		xmlNewChild( writer->private->schema_node, NULL, BAD_CAST( NAXML_KEY_SCHEMA_NODE_LISTTYPE ), BAD_CAST( "string" ));
	}

	parent_value_node = writer->private->schema_node;

	if( def->localizable ){
		writer->private->locale_node = xmlNewChild( writer->private->schema_node, NULL, BAD_CAST( NAXML_KEY_SCHEMA_NODE_LOCALE ), NULL );
		xmlNewProp( writer->private->locale_node, BAD_CAST( "name" ), BAD_CAST( "C" ));
		parent_value_node = writer->private->locale_node;
	}

	content = xmlEncodeSpecialChars( writer->private->doc, BAD_CAST( value_str ));
	xmlNewChild( parent_value_node, NULL, BAD_CAST( NAXML_KEY_SCHEMA_NODE_DEFAULT ), content );
	xmlFree( content );

	g_free( value_str );
	g_free( object_id );
}

static void
write_data_dump( NAXMLWriter *writer, const NAObjectId *object, const NADataBoxed *boxed )
{
	xmlNodePtr entry_node;
	gchar *entry;
	NADataDef *def;
	xmlNodePtr value_node;
	xmlNodePtr value_list_node, value_list_value_node;
	GSList *list, *is;
	xmlChar *encoded_content;
	gchar *value_str;

	value_str = NULL;
	def = na_data_boxed_get_data_def( boxed );

	/* do no export empty values, but for string list
	 */
	if( def->type != NAFD_TYPE_STRING_LIST ){
		value_str = na_data_boxed_get_as_string( boxed );
		if( !value_str || !strlen( value_str )){
			return;
		}
	}

	/* boolean value must be lowercase
	 */
	if( def->type == NAFD_TYPE_BOOLEAN ){
		gchar *tmp = g_ascii_strdown( value_str, -1 );
		g_free( value_str );
		value_str = tmp;
	}

	entry_node = xmlNewChild( writer->private->list_node, NULL, BAD_CAST( writer->private->fn_str->element_node ), NULL );

	if( writer->private->action ){
		gchar *id = na_object_get_id( object );
		entry = g_strdup_printf( "%s/%s", id, def->gconf_entry );
		g_free( id );
	} else {
		entry = g_strdup( def->gconf_entry );
	}
	xmlNewChild( entry_node, NULL, BAD_CAST( NAXML_KEY_DUMP_NODE_KEY ), BAD_CAST( entry ));

	value_node = xmlNewChild( entry_node, NULL, BAD_CAST( NAXML_KEY_DUMP_NODE_VALUE ), NULL );

	if( def->type == NAFD_TYPE_STRING_LIST ){
		value_list_node = xmlNewChild( value_node, NULL, BAD_CAST( NAXML_KEY_DUMP_NODE_VALUE_LIST ), NULL );
		xmlNewProp( value_list_node, BAD_CAST( NAXML_KEY_DUMP_NODE_VALUE_LIST_PARM_TYPE ), BAD_CAST( NAXML_KEY_DUMP_NODE_VALUE_TYPE_STRING ));
		value_list_value_node = xmlNewChild( value_list_node, NULL, BAD_CAST( NAXML_KEY_DUMP_NODE_VALUE ), NULL );
		list = ( GSList * ) na_data_boxed_get_as_void( boxed );

		for( is = list ; is ; is = is->next ){
			encoded_content = xmlEncodeSpecialChars( writer->private->doc, BAD_CAST(( gchar * ) is->data ));
			xmlNewChild( value_list_value_node, NULL, BAD_CAST( NAXML_KEY_DUMP_NODE_VALUE_TYPE_STRING ), encoded_content );
			xmlFree( encoded_content );
		}

	} else {
		encoded_content = xmlEncodeSpecialChars( writer->private->doc, BAD_CAST( value_str ));
		xmlNewChild( value_node, NULL, BAD_CAST( na_data_types_get_gconf_dump_key( def->type )), encoded_content );
		xmlFree( encoded_content );
		g_free( value_str );
	}

	g_free( entry );
}

static ExportFormatFn *
find_export_format_fn( GQuark format )
{
	ExportFormatFn *found;
	ExportFormatFn *i;

	found = NULL;
	i = st_export_format_fn;

	while( i->format && !found ){
		if( g_quark_from_string( i->format ) == format ){
			found = i;
		}
		i++;
	}

	return( found );
}

/*
 * get_output_fname:
 * @item: the #NAObjectItme-derived object to be exported.
 * @folder: the path of the directoy where to write the output XML file.
 * @format: the export format.
 *
 * Returns: a filename suitable for writing the output XML.
 *
 * As we don't want overwrite already existing files, the candidate
 * filename is incremented until we find an available filename.
 *
 * The returned string should be g_free() by the caller.
 *
 * Note that this function is always subject to race condition, as it
 * is possible, though unlikely, that the given file be created
 * between our test of inexistance and the actual write.
 */
static gchar *
get_output_fname( const NAObjectItem *item, const gchar *folder, GQuark format )
{
	static const gchar *thisfn = "naxml_writer_get_output_fname";
	gchar *item_id;
	gchar *canonical_fname = NULL;
	gchar *canonical_ext = NULL;
	gchar *candidate_fname;
	gint counter;

	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), NULL );
	g_return_val_if_fail( folder, NULL );
	g_return_val_if_fail( strlen( folder ), NULL );

	item_id = na_object_get_id( item );

	if( format == g_quark_from_string( NAXML_FORMAT_GCONF_SCHEMA_V1 )){
		canonical_fname = g_strdup_printf( "config_%s", item_id );
		canonical_ext = g_strdup( "schemas" );

	} else if( format == g_quark_from_string( NAXML_FORMAT_GCONF_SCHEMA_V2 )){
		canonical_fname = g_strdup_printf( "config-%s", item_id );
		canonical_ext = g_strdup( "schema" );

	} else if( format == g_quark_from_string( NAXML_FORMAT_GCONF_ENTRY )){
		canonical_fname = g_strdup_printf( "%s-%s", NA_IS_OBJECT_ACTION( item ) ? "action" : "menu", item_id );
		canonical_ext = g_strdup( "xml" );

	} else {
		g_warning( "%s: unknown format: %s", thisfn, g_quark_to_string( format ));
	}

	g_free( item_id );
	g_return_val_if_fail( canonical_fname, NULL );

	candidate_fname = g_strdup_printf( "%s/%s.%s", folder, canonical_fname, canonical_ext );

	if( !na_core_utils_file_exists( candidate_fname )){
		g_free( canonical_fname );
		g_free( canonical_ext );
		return( candidate_fname );
	}

	for( counter = 0 ; ; ++counter ){
		g_free( candidate_fname );
		candidate_fname = g_strdup_printf( "%s/%s_%d.%s", folder, canonical_fname, counter, canonical_ext );
		if( !na_core_utils_file_exists( candidate_fname )){
			break;
		}
	}

	g_free( canonical_fname );
	g_free( canonical_ext );

	return( candidate_fname );
}

/**
 * output_xml_to_file:
 * @xml: the xml buffer.
 * @filename: the full path of the output filename.
 * @msg: a GSList to append messages.
 *
 * Exports an item to the given filename.
 */
void
output_xml_to_file( const gchar *xml, const gchar *filename, GSList **msg )
{
	static const gchar *thisfn = "naxml_writer_output_xml_to_file";
	GFile *file;
	GFileOutputStream *stream;
	GError *error = NULL;
	gchar *errmsg;

	g_return_if_fail( xml );
	g_return_if_fail( filename && g_utf8_strlen( filename, -1 ));

	file = g_file_new_for_path( filename );

	stream = g_file_replace( file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, &error );
	if( error ){
		errmsg = g_strdup_printf( "%s: g_file_replace: %s", thisfn, error->message );
		g_warning( "%s", errmsg );
		if( msg ){
			*msg = g_slist_append( *msg, errmsg );
		}
		g_error_free( error );
		if( stream ){
			g_object_unref( stream );
		}
		g_object_unref( file );
		return;
	}

	g_output_stream_write( G_OUTPUT_STREAM( stream ), xml, g_utf8_strlen( xml, -1 ), NULL, &error );
	if( error ){
		errmsg = g_strdup_printf( "%s: g_output_stream_write: %s", thisfn, error->message );
		g_warning( "%s", errmsg );
		if( msg ){
			*msg = g_slist_append( *msg, errmsg );
		}
		g_error_free( error );
		g_object_unref( stream );
		g_object_unref( file );
		return;
	}

	g_output_stream_close( G_OUTPUT_STREAM( stream ), NULL, &error );
	if( error ){
		errmsg = g_strdup_printf( "%s: g_output_stream_close: %s", thisfn, error->message );
		g_warning( "%s", errmsg );
		if( msg ){
			*msg = g_slist_append( *msg, errmsg );
		}
		g_error_free( error );
		g_object_unref( stream );
		g_object_unref( file );
		return;
	}

	g_object_unref( stream );
	g_object_unref( file );
}

static guint
writer_to_buffer( NAXMLWriter *writer )
{
	guint code;
	xmlDocPtr doc;
	xmlChar *text;
	int textlen;

	code = NA_IEXPORTER_CODE_OK;
	doc = build_xml_doc( writer );

	xmlDocDumpFormatMemoryEnc( doc, &text, &textlen, "UTF-8", 1 );
	writer->private->buffer = g_strdup(( const gchar * ) text );

	xmlFree( text );
	xmlFreeDoc (doc);
	xmlCleanupParser();

	return( code );
}

#if 0
static NAXMLWriter *xml_writer_new( const gchar *uuid );
static xmlDocPtr    create_xml_schema( NAXMLWriter *writer, gint format, const NAObjectAction *action );
static void         create_schema_entry(
								NAXMLWriter *writer,
								gint format,
								const gchar *profile_name,
								const gchar *key,
								const gchar *value,
								xmlDocPtr doc,
								xmlNodePtr list_node,
								const gchar *type,
								gboolean is_l10n_value,
								const gchar *short_desc,
								const gchar *long_desc );
static xmlDocPtr    create_xml_dump( NAXMLWriter *writer, gint format, const NAObjectAction *action );
static void         create_dump_entry(
								NAXMLWriter *writer,
								gint format,
								const gchar *profile_name,
								const gchar *key,
								const gchar *value,
								xmlDocPtr doc,
								xmlNodePtr list_node,
								const gchar *type );
static xmlDocPtr    create_gconf_schema( NAXMLWriter *writer );
static void         create_gconf_schema_entry(
								NAXMLWriter *writer,
								const gchar *entry,
								xmlDocPtr doc,
								xmlNodePtr list_node,
								const gchar *type,
								const gchar *short_desc,
								const gchar *long_desc,
								const gchar *default_value,
								gboolean is_i18n );

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	NAXMLWriter *self;

	g_return_if_fail( NA_IS_XML_WRITER( object ));
	self = NA_XML_WRITER( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case XML_WRITER_PROP_UUID_ID:
				g_value_set_string( value, self->private->uuid );
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
	NAXMLWriter *self;

	g_return_if_fail( NA_IS_XML_WRITER( object ));
	self = NA_XML_WRITER( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case XML_WRITER_PROP_UUID_ID:
				g_free( self->private->uuid );
				self->private->uuid = g_value_dup_string( value );
				break;

			default:
				G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
				break;
		}
	}
}

static NAXMLWriter *
writer_new( const gchar *uuid )
{
	return( g_object_new( NAXML_WRITER_TYPE, NULL ));
}

/**
 * na_xml_writer_export:
 * @action: the #NAObjectAction to be exported.
 * Can be NULL when exporting schemas ; in this case, format must be
 * FORMAT_GCONF_SCHEMA.
 * @folder: the directoy where to write the output XML file.
 *  If NULL, the output will be directed to stdout.
 *  If not NULL, must be specified as an URI (e.g. file:///tmp)
 * @format: the export format.
 * @msg: pointer to a #GSList which will receive error messages.
 *  The #GSList should be initialized to an empty list (NULL) before
 *  calling the function.
 *
 * Export the specified action as an XML file.
 *
 * Returns: the written filename, or NULL if written to stdout or errors.
 */
gchar *
na_xml_writer_export( const NAObjectAction *action, const gchar *folder, gint format, GSList **msg )
{
	static const gchar *thisfn = "na_xml_writer_export";
	gchar *filename = NULL;
	gchar *xml_buffer;

	g_debug( "%s: action=%p, format=%u", thisfn, ( void * ) action, format );
	g_return_val_if_fail( action || format == IPREFS_EXPORT_FORMAT_GCONF_SCHEMA, NULL );

	switch( format ){
		case IPREFS_EXPORT_FORMAT_GCONF_SCHEMA_V1:
		case IPREFS_EXPORT_FORMAT_GCONF_SCHEMA_V2:
			filename = na_xml_writer_get_output_fname( action, folder, format );
			break;

		/* this is the format used by nautilus-actions-new utility,
		 * and that's why this option takes care of a NULL folder
		 */
		case IPREFS_EXPORT_FORMAT_GCONF_ENTRY:
			if( folder ){
				filename = na_xml_writer_get_output_fname( action, folder, format );
			}
			break;

		/* this is the format used by nautilus-actions-schemas
		 * utility, and that's why this option takes care of a NULL
		 * folder, or an output filename
		 */
		case IPREFS_EXPORT_FORMAT_GCONF_SCHEMA:
			if( folder ){
				filename = g_strdup( folder );
			}
			break;

		default:
			g_return_val_if_reached( NULL );
	}

	g_assert( filename || folder == NULL );

	xml_buffer = na_xml_writer_get_xml_buffer( action, format );

	if( folder ){
		na_xml_writer_output_xml( xml_buffer, filename, msg );
	} else {
		g_print( "%s", xml_buffer );
	}

	if( *msg ){
		g_free( filename );
		filename = NULL;
	}

	g_free( xml_buffer );

	return( filename );
}

/**
 * na_xml_writer_get_xml_buffer:
 * @action: the #NAObjectAction to be exported.
 * @format: the export format.
 *
 * Returns: a buffer which contains the XML output.
 *
 * The returned string should be g_free() by the caller.
 */
gchar *
na_xml_writer_get_xml_buffer( const NAObjectAction *action, gint format )
{
	gchar *uuid;
	NAXMLWriter *writer;
	xmlDocPtr doc = NULL;
	xmlChar *text;
	int textlen;
	gchar *buffer;

	g_return_val_if_fail( action || format == IPREFS_EXPORT_FORMAT_GCONF_SCHEMA, NULL );

	uuid = action ? na_object_get_id( action ) : NULL;
	writer = xml_writer_new( uuid );
	g_free( uuid );

	switch( format ){
		case IPREFS_EXPORT_FORMAT_GCONF_SCHEMA_V1:
		case IPREFS_EXPORT_FORMAT_GCONF_SCHEMA_V2:
			doc = create_xml_schema( writer, format, action );
			break;

		case IPREFS_EXPORT_FORMAT_GCONF_ENTRY:
			doc = create_xml_dump( writer, format, action );
			break;

		case IPREFS_EXPORT_FORMAT_GCONF_SCHEMA:
			doc = create_gconf_schema( writer );
			break;
	}

	g_assert( doc );

	xmlDocDumpFormatMemoryEnc( doc, &text, &textlen, "UTF-8", 1 );
	buffer = g_strdup(( const gchar * ) text );

	xmlFree( text );
	xmlFreeDoc (doc);
	xmlCleanupParser();
	g_object_unref( writer );

	return( buffer );
}

static xmlDocPtr
create_xml_schema( NAXMLWriter *writer, gint format, const NAObjectAction *action )
{
	xmlDocPtr doc;
	xmlNodePtr root_node, list_node;
	gchar *version, *label, *tooltip, *icon, *text;
	gboolean enabled;
	GList *profiles, *ip;
	NAObjectProfile *profile;
	gchar *profile_dir, *profile_label, *path, *parameters;
	GSList *basenames, *mimetypes, *schemes;
	gboolean match, isfile, isdir, multiple;
	gchar *type;
	GSList *items;
	gboolean target_selection, target_background, target_toolbar;
	gchar *toolbar_label;
	GSList *folders;

	doc = xmlNewDoc( BAD_CAST( "1.0" ));
	root_node = xmlNewNode( NULL, BAD_CAST( NACT_GCONF_SCHEMA_ROOT ));
	xmlDocSetRootElement( doc, root_node );
	list_node = xmlNewChild( root_node, NULL, BAD_CAST( NACT_GCONF_SCHEMA_LIST ), NULL );

	/* version */
	version = na_object_action_get_version( action );
	create_schema_entry( writer, format, NULL, ACTION_VERSION_ENTRY, version, doc, list_node, "string", FALSE, ACTION_VERSION_DESC_SHORT, ACTION_VERSION_DESC_LONG );
	g_free( version );

	/* label */
	label = na_object_get_label( action );
	create_schema_entry( writer, format, NULL, OBJECT_ITEM_LABEL_ENTRY, label, doc, list_node, "string", TRUE, ACTION_LABEL_DESC_SHORT, ACTION_LABEL_DESC_LONG );
	g_free( label );

	/* tooltip */
	tooltip = na_object_get_tooltip( action );
	create_schema_entry( writer, format, NULL, OBJECT_ITEM_TOOLTIP_ENTRY, tooltip, doc, list_node, "string", TRUE, ACTION_TOOLTIP_DESC_SHORT, ACTION_TOOLTIP_DESC_LONG );
	g_free( tooltip );

	/* icon name */
	icon = na_object_get_icon( action );
	create_schema_entry( writer, format, NULL, OBJECT_ITEM_ICON_ENTRY, icon, doc, list_node, "string", FALSE, ACTION_ICON_DESC_SHORT, ACTION_ICON_DESC_LONG );
	g_free( icon );

	/* enabled */
	enabled = na_object_is_enabled( NA_OBJECT_ITEM( action ));
	text = na_utils_boolean_to_schema( enabled );
	create_schema_entry( writer, format, NULL, OBJECT_ITEM_ENABLED_ENTRY, text, doc, list_node, "bool", FALSE, ACTION_ENABLED_DESC_SHORT, ACTION_ENABLED_DESC_LONG );
	g_free( text );

	/* items */
	items = na_object_item_rebuild_items_list( NA_OBJECT_ITEM( action ));
	text = na_utils_gslist_to_schema( items );
	create_schema_entry( writer, format, NULL, OBJECT_ITEM_LIST_ENTRY, text, doc, list_node, "list", FALSE, ACTION_ITEMS_DESC_SHORT, ACTION_ITEMS_DESC_LONG );
	g_free( text );
	na_utils_free_string_list( items );

	/* type */
	type = NA_IS_OBJECT_ACTION( action ) ? g_strdup( "Action" ) : g_strdup( "Menu" );
	create_schema_entry( writer, format, NULL, OBJECT_ITEM_TYPE_ENTRY, type, doc, list_node, "string", FALSE, ACTION_TYPE_DESC_SHORT, ACTION_TYPE_DESC_LONG );
	g_free( type );

	/* target selection */
	target_selection = na_object_action_is_target_selection( action );
	text = na_utils_boolean_to_schema( target_selection );
	create_schema_entry( writer, format, NULL, OBJECT_ITEM_TARGET_SELECTION_ENTRY, text, doc, list_node, "bool", FALSE, ACTION_TARGET_SELECTION_DESC_SHORT, ACTION_TARGET_SELECTION_DESC_LONG );
	g_free( text );

	/* target background */
	target_background = na_object_action_is_target_background( action );
	text = na_utils_boolean_to_schema( target_background );
	create_schema_entry( writer, format, NULL, OBJECT_ITEM_TARGET_BACKGROUND_ENTRY, text, doc, list_node, "bool", FALSE, ACTION_TARGET_BACKGROUND_DESC_SHORT, ACTION_TARGET_BACKGROUND_DESC_LONG );
	g_free( text );

	/* target toolbar */
	target_toolbar = na_object_action_is_target_toolbar( action );
	text = na_utils_boolean_to_schema( target_toolbar );
	create_schema_entry( writer, format, NULL, OBJECT_ITEM_TARGET_TOOLBAR_ENTRY, text, doc, list_node, "bool", FALSE, ACTION_TARGET_TOOLBAR_DESC_SHORT, ACTION_TARGET_TOOLBAR_DESC_LONG );
	g_free( text );

	/* toolbar same label
	 * only used between 2.29.1 and 2.29.4 - removed starting with 2.29.5
	 */
	/*toolbar_same_label = na_object_action_toolbar_use_same_label( action );
	text = na_utils_boolean_to_schema( toolbar_same_label );
	create_schema_entry( writer, format, NULL, OBJECT_ITEM_TOOLBAR_SAME_LABEL_ENTRY, text, doc, list_node, "bool", FALSE, ACTION_TOOLBAR_SAME_LABEL_DESC_SHORT, ACTION_TOOLBAR_SAME_LABEL_DESC_LONG );
	g_free( text );*/

	/* toolbar label
	 * starting with 2.29.5: we always export the toolbar label
	 */
	toolbar_label = na_object_action_toolbar_get_label( action );
	create_schema_entry( writer, format, NULL, OBJECT_ITEM_TOOLBAR_LABEL_ENTRY, toolbar_label, doc, list_node, "string", TRUE, ACTION_TOOLBAR_LABEL_DESC_SHORT, ACTION_TOOLBAR_LABEL_DESC_LONG );
	g_free( toolbar_label );

	profiles = na_object_get_items_list( action );

	for( ip = profiles ; ip ; ip = ip->next ){

		profile = NA_OBJECT_PROFILE( ip->data );
		profile_dir = na_object_get_id( profile );

		/* profile label */
		profile_label = na_object_get_label( profile );
		create_schema_entry( writer, format, profile_dir, ACTION_PROFILE_LABEL_ENTRY, profile_label, doc, list_node, "string", TRUE, ACTION_PROFILE_NAME_DESC_SHORT, ACTION_PROFILE_NAME_DESC_LONG );
		g_free( profile_label );

		/* path */
		path = na_object_profile_get_path( profile );
		create_schema_entry( writer, format, profile_dir, ACTION_PATH_ENTRY, path, doc, list_node, "string", FALSE, ACTION_PATH_DESC_SHORT, ACTION_PATH_DESC_LONG );
		g_free( path );

		/* parameters */
		parameters = na_object_profile_get_parameters( profile );
		create_schema_entry( writer, format, profile_dir, ACTION_PARAMETERS_ENTRY, parameters, doc, list_node, "string", FALSE, ACTION_PARAMETERS_DESC_SHORT, ACTION_PARAMETERS_DESC_LONG );
		g_free( parameters );

		/* basenames */
		basenames = na_object_profile_get_basenames( profile );
		text = na_utils_gslist_to_schema( basenames );
		create_schema_entry( writer, format, profile_dir, ACTION_BASENAMES_ENTRY, text, doc, list_node, "list", FALSE, ACTION_BASENAMES_DESC_SHORT, ACTION_BASENAMES_DESC_LONG );
		g_free( text );
		na_utils_free_string_list( basenames );

		/* match_case */
		match = na_object_profile_get_matchcase( profile );
		text = na_utils_boolean_to_schema( match );
		create_schema_entry( writer, format, profile_dir, ACTION_MATCHCASE_ENTRY, text, doc, list_node, "bool", FALSE, ACTION_MATCHCASE_DESC_SHORT, ACTION_MATCHCASE_DESC_LONG );
		g_free( text );

		/* mimetypes */
		mimetypes = na_object_profile_get_mimetypes( profile );
		text = na_utils_gslist_to_schema( mimetypes );
		create_schema_entry( writer, format, profile_dir, ACTION_MIMETYPES_ENTRY, text, doc, list_node, "list", FALSE, ACTION_MIMETYPES_DESC_SHORT, ACTION_MIMETYPES_DESC_LONG );
		g_free( text );
		na_utils_free_string_list( mimetypes );

		/* is_file */
		isfile = na_object_profile_get_is_file( profile );
		text = na_utils_boolean_to_schema( isfile );
		create_schema_entry( writer, format, profile_dir, ACTION_ISFILE_ENTRY, text, doc, list_node, "bool", FALSE, ACTION_ISFILE_DESC_SHORT, ACTION_ISFILE_DESC_LONG );
		g_free( text );

		/* is_dir */
		isdir = na_object_profile_get_is_dir( profile );
		text = na_utils_boolean_to_schema( isdir );
		create_schema_entry( writer, format, profile_dir, ACTION_ISDIR_ENTRY, text, doc, list_node, "bool", FALSE, ACTION_ISDIR_DESC_SHORT, ACTION_ISDIR_DESC_LONG );
		g_free( text );

		/* accept-multiple-files */
		multiple = na_object_profile_get_multiple( profile );
		text = na_utils_boolean_to_schema( multiple );
		create_schema_entry( writer, format, profile_dir, ACTION_MULTIPLE_ENTRY, text, doc, list_node, "bool", FALSE, ACTION_MULTIPLE_DESC_SHORT, ACTION_MULTIPLE_DESC_LONG );
		g_free( text );

		/* schemes */
		schemes = na_object_profile_get_schemes( profile );
		text = na_utils_gslist_to_schema( schemes );
		create_schema_entry( writer, format, profile_dir, ACTION_SCHEMES_ENTRY, text, doc, list_node, "list", FALSE, ACTION_SCHEMES_DESC_SHORT, ACTION_SCHEMES_DESC_LONG );
		g_free( text );
		na_utils_free_string_list( schemes );

		/* folders */
		folders = na_object_profile_get_folders( profile );
		text = na_utils_gslist_to_schema( folders );
		create_schema_entry( writer, format, profile_dir, ACTION_FOLDERS_ENTRY, text, doc, list_node, "list", FALSE, ACTION_FOLDERS_DESC_SHORT, ACTION_FOLDERS_DESC_LONG );
		g_free( text );
		na_utils_free_string_list( folders );

		g_free( profile_dir );
	}

	return( doc );
}

static void
create_schema_entry( NAXMLWriter *writer,
		gint format,
		const gchar *profile_name, const gchar *key, const gchar *value,
		xmlDocPtr doc, xmlNodePtr list_node, const gchar *type, gboolean is_l10n_value,
		const gchar *short_desc, const gchar *long_desc )
{
	gchar *path = NULL;
	xmlNodePtr schema_node;
	xmlChar *content, *encoded_content;
	xmlNodePtr value_root_node, locale_node;

	if( profile_name ){
		path = g_build_path( "/", NA_GCONF_CONFIG_PATH, writer->private->uuid, profile_name, key, NULL );
	} else {
		path = g_build_path( "/", NA_GCONF_CONFIG_PATH, writer->private->uuid, key, NULL );
	}

	schema_node = xmlNewChild( list_node, NULL, BAD_CAST( NACT_GCONF_SCHEMA_ENTRY ), NULL );

	content = BAD_CAST( g_build_path( "/", NAUTILUS_ACTIONS_GCONF_SCHEMASDIR, path, NULL ));
	xmlNewChild( schema_node, NULL, BAD_CAST( NACT_GCONF_SCHEMA_KEY ), content );
	xmlFree( content );

	xmlNewChild( schema_node, NULL, BAD_CAST( NACT_GCONF_SCHEMA_APPLYTO ), BAD_CAST( path ));

	xmlNewChild( schema_node, NULL, BAD_CAST( NACT_GCONF_SCHEMA_TYPE ), BAD_CAST( type ));
	if( !g_ascii_strcasecmp( type, "list" )){
		xmlNewChild( schema_node, NULL, BAD_CAST( NACT_GCONF_SCHEMA_LIST_TYPE ), BAD_CAST( "string" ));
	}

	/* always creates a 'locale' node,
	 * maybe with the default value if this later is localized
	 */
	value_root_node = schema_node;
	locale_node = xmlNewChild( schema_node, NULL, BAD_CAST( NACT_GCONF_SCHEMA_LOCALE ), NULL );
	xmlNewProp( locale_node, BAD_CAST( "name" ), BAD_CAST( "C" ));
	if( is_l10n_value ){
		value_root_node = locale_node;
	}

	/* encode special chars <, >, &, ...
	 */
	encoded_content = xmlEncodeSpecialChars( doc, BAD_CAST( value ));
	xmlNewChild( value_root_node, NULL, BAD_CAST( NACT_GCONF_SCHEMA_DEFAULT ), encoded_content );
	xmlFree( encoded_content );

	/* fill up the historical format if asked for
	 * add owner and short and long descriptions
	 */
	if( format == IPREFS_EXPORT_FORMAT_GCONF_SCHEMA_V1 ){
		xmlNewChild( schema_node, NULL, BAD_CAST( NACT_GCONF_SCHEMA_OWNER ), BAD_CAST( PACKAGE_TARNAME ));

		xmlNewChild( locale_node, NULL, BAD_CAST( NACT_GCONF_SCHEMA_SHORT ), BAD_CAST( short_desc ));

		xmlNewChild( locale_node, NULL, BAD_CAST( NACT_GCONF_SCHEMA_LONG ), BAD_CAST( long_desc ));
	}

	g_free( path );
}

static xmlDocPtr
create_xml_dump( NAXMLWriter *writer, gint format, const NAObjectAction *action )
{
	xmlDocPtr doc;
	xmlNodePtr root_node, list_node;
	gchar *path;
	gchar *version, *label, *tooltip, *icon, *text;
	gboolean enabled;
	GList *profiles, *ip;
	NAObjectProfile *profile;
	gchar *profile_dir;
	gchar *profile_label, *parameters;
	GSList *basenames, *mimetypes, *schemes;
	gboolean match, isfile, isdir, multiple;
	gchar *type;
	GSList *items;
	gboolean target_selection, target_background, target_toolbar;
	gchar *toolbar_label;
	GSList *folders;

	doc = xmlNewDoc( BAD_CAST( "1.0" ));
	root_node = xmlNewNode( NULL, BAD_CAST( NACT_GCONF_DUMP_ROOT ));
	xmlDocSetRootElement( doc, root_node );

	path = g_build_path( "/", NA_GCONF_CONFIG_PATH, writer->private->uuid, NULL );
	list_node = xmlNewChild( root_node, NULL, BAD_CAST( NACT_GCONF_DUMP_ENTRYLIST ), NULL );
	xmlNewProp( list_node, BAD_CAST( NACT_GCONF_DUMP_ENTRYLIST_BASE ), BAD_CAST( path ));
	g_free( path );

	/* version */
	version = na_object_action_get_version( action );
	create_dump_entry( writer, format, NULL, ACTION_VERSION_ENTRY, version, doc, list_node, "string" );
	g_free( version );

	/* label */
	label = na_object_get_label( action );
	create_dump_entry( writer, format, NULL, OBJECT_ITEM_LABEL_ENTRY, label, doc, list_node, "string" );
	g_free( label );

	/* tooltip */
	tooltip = na_object_get_tooltip( action );
	create_dump_entry( writer, format, NULL, OBJECT_ITEM_TOOLTIP_ENTRY, tooltip, doc, list_node, "string" );
	g_free( tooltip );

	/* icon name */
	icon = na_object_get_icon( action );
	create_dump_entry( writer, format, NULL, OBJECT_ITEM_ICON_ENTRY, icon, doc, list_node, "string" );
	g_free( icon );

	/* enabled */
	enabled = na_object_is_enabled( NA_OBJECT_ITEM( action ));
	text = na_utils_boolean_to_schema( enabled );
	create_dump_entry( writer, format, NULL, OBJECT_ITEM_ENABLED_ENTRY, text, doc, list_node, "bool" );
	g_free( text );

	/* items */
	items = na_object_item_rebuild_items_list( NA_OBJECT_ITEM( action ));
	text = na_utils_gslist_to_schema( items );
	create_dump_entry( writer, format, NULL, OBJECT_ITEM_LIST_ENTRY, text, doc, list_node, "list" );
	g_free( text );
	na_utils_free_string_list( items );

	/* type */
	type = NA_IS_OBJECT_ACTION( action ) ? g_strdup( "Action" ) : g_strdup( "Menu" );
	create_dump_entry( writer, format, NULL, OBJECT_ITEM_TYPE_ENTRY, type, doc, list_node, "string" );
	g_free( type );

	/* target selection */
	target_selection = na_object_action_is_target_selection( action );
	text = na_utils_boolean_to_schema( target_selection );
	create_dump_entry( writer, format, NULL, OBJECT_ITEM_TARGET_SELECTION_ENTRY, text, doc, list_node, "bool" );
	g_free( text );

	/* target background */
	target_background = na_object_action_is_target_background( action );
	text = na_utils_boolean_to_schema( target_background );
	create_dump_entry( writer, format, NULL, OBJECT_ITEM_TARGET_BACKGROUND_ENTRY, text, doc, list_node, "bool" );
	g_free( text );

	/* target toolbar */
	target_toolbar = na_object_action_is_target_toolbar( action );
	text = na_utils_boolean_to_schema( target_toolbar );
	create_dump_entry( writer, format, NULL, OBJECT_ITEM_TARGET_TOOLBAR_ENTRY, text, doc, list_node, "bool" );
	g_free( text );

	/* toolbar same label
	 * only used between 2.29.1 and 2.29.4 - removed starting with 2.29.5
	 */
	/*toolbar_same_label = na_object_action_toolbar_use_same_label( action );
	text = na_utils_boolean_to_schema( toolbar_same_label );
	create_dump_entry( writer, format, NULL, OBJECT_ITEM_TOOLBAR_SAME_LABEL_ENTRY, text, doc, list_node, "bool" );
	g_free( text );*/

	/* toolbar label
	 * starting with 2.29.5: we always export the toolbar label
	 */
	toolbar_label = na_object_action_toolbar_get_label( action );
	create_dump_entry( writer, format, NULL, OBJECT_ITEM_TOOLBAR_LABEL_ENTRY, toolbar_label, doc, list_node, "string" );
	g_free( toolbar_label );

	profiles = na_object_get_items_list( action );

	for( ip = profiles ; ip ; ip = ip->next ){

		profile = NA_OBJECT_PROFILE( ip->data );
		profile_dir = na_object_get_id( profile );

		/* profile label */
		profile_label = na_object_get_label( profile );
		create_dump_entry( writer, format, profile_dir, ACTION_PROFILE_LABEL_ENTRY, profile_label, doc, list_node, "string" );
		g_free( profile_label );

		/* path */
		path = na_object_profile_get_path( profile );
		create_dump_entry( writer, format, profile_dir, ACTION_PATH_ENTRY, path, doc, list_node, "string" );
		g_free( path );

		/* parameters */
		parameters = na_object_profile_get_parameters( profile );
		create_dump_entry( writer, format, profile_dir, ACTION_PARAMETERS_ENTRY, parameters, doc, list_node, "string" );
		g_free( parameters );

		/* basenames */
		basenames = na_object_profile_get_basenames( profile );
		text = na_utils_gslist_to_schema( basenames );
		create_dump_entry( writer, format, profile_dir, ACTION_BASENAMES_ENTRY, text, doc, list_node, "list" );
		g_free( text );
		na_utils_free_string_list( basenames );

		/* match_case */
		match = na_object_profile_get_matchcase( profile );
		text = na_utils_boolean_to_schema( match );
		create_dump_entry( writer, format, profile_dir, ACTION_MATCHCASE_ENTRY, text, doc, list_node, "bool" );
		g_free( text );

		/* mimetypes */
		mimetypes = na_object_profile_get_mimetypes( profile );
		text = na_utils_gslist_to_schema( mimetypes );
		create_dump_entry( writer, format, profile_dir, ACTION_MIMETYPES_ENTRY, text, doc, list_node, "list" );
		g_free( text );
		na_utils_free_string_list( mimetypes );

		/* is_file */
		isfile = na_object_profile_get_is_file( profile );
		text = na_utils_boolean_to_schema( isfile );
		create_dump_entry( writer, format, profile_dir, ACTION_ISFILE_ENTRY, text, doc, list_node, "bool" );
		g_free( text );

		/* is_dir */
		isdir = na_object_profile_get_is_dir( profile );
		text = na_utils_boolean_to_schema( isdir );
		create_dump_entry( writer, format, profile_dir, ACTION_ISDIR_ENTRY, text, doc, list_node, "bool" );
		g_free( text );

		/* accept-multiple-files */
		multiple = na_object_profile_get_multiple( profile );
		text = na_utils_boolean_to_schema( multiple );
		create_dump_entry( writer, format, profile_dir, ACTION_MULTIPLE_ENTRY, text, doc, list_node, "bool" );
		g_free( text );

		/* schemes */
		schemes = na_object_profile_get_schemes( profile );
		text = na_utils_gslist_to_schema( schemes );
		create_dump_entry( writer, format, profile_dir, ACTION_SCHEMES_ENTRY, text, doc, list_node, "list" );
		g_free( text );
		na_utils_free_string_list( schemes );

		/* folders */
		folders = na_object_profile_get_folders( profile );
		text = na_utils_gslist_to_schema( folders );
		create_dump_entry( writer, format, profile_dir, ACTION_FOLDERS_ENTRY, text, doc, list_node, "list" );
		g_free( text );
		na_utils_free_string_list( folders );

		g_free( profile_dir );
	}

	return( doc );
}

static void
create_dump_entry( NAXMLWriter *writer,
		gint format,
		const gchar *profile_name, const gchar *key, const gchar *value,
		xmlDocPtr doc, xmlNodePtr list_node, const gchar *type )
{
	xmlNodePtr entry_node;
	xmlNodePtr value_node;
	xmlNodePtr value_list_node, value_list_value_node;
	gchar *entry = NULL;
	GSList *list, *is;
	xmlChar *encoded_content;

	entry_node = xmlNewChild( list_node, NULL, BAD_CAST( NACT_GCONF_DUMP_ENTRY ), NULL );

	if( profile_name ){
		entry = g_strdup_printf( "%s/%s", profile_name, key );
	} else {
		entry = g_strdup( key );
	}
	xmlNewChild( entry_node, NULL, BAD_CAST( NACT_GCONF_DUMP_KEY ), BAD_CAST( entry ));
	g_free( entry );

	value_node = xmlNewChild( entry_node, NULL, BAD_CAST( NACT_GCONF_DUMP_VALUE ), NULL );

	if( !g_ascii_strcasecmp( type, "list" )){
		value_list_node = xmlNewChild( value_node, NULL, BAD_CAST( NACT_GCONF_DUMP_LIST ), NULL );
		xmlNewProp( value_list_node, BAD_CAST( NACT_GCONF_DUMP_LIST_TYPE ), BAD_CAST( NACT_GCONF_DUMP_STRING ));
		value_list_value_node = xmlNewChild( value_list_node, NULL, BAD_CAST( NACT_GCONF_DUMP_VALUE ), NULL );
		list = na_utils_schema_to_gslist( value );
		for( is = list ; is ; is = is->next ){
			encoded_content = xmlEncodeSpecialChars( doc, BAD_CAST(( gchar * ) is->data ));
			xmlNewChild( value_list_value_node, NULL, BAD_CAST( NACT_GCONF_DUMP_STRING ), encoded_content );
			xmlFree( encoded_content );
		}
	} else {
		encoded_content = xmlEncodeSpecialChars( doc, BAD_CAST( value ));
		xmlNewChild( value_node, NULL, BAD_CAST( NACT_GCONF_DUMP_STRING ), encoded_content );
		xmlFree( encoded_content );
	}
}

static xmlDocPtr
create_gconf_schema( NAXMLWriter *writer )
{
	xmlDocPtr doc;
	xmlNodePtr root_node, list_node;

	doc = xmlNewDoc( BAD_CAST( "1.0" ));
	root_node = xmlNewNode( NULL, BAD_CAST( NACT_GCONF_SCHEMA_ROOT ));
	xmlDocSetRootElement( doc, root_node );
	list_node = xmlNewChild( root_node, NULL, BAD_CAST( NACT_GCONF_SCHEMA_LIST ), NULL );

	create_gconf_schema_entry( writer, ACTION_VERSION_ENTRY       , doc, list_node, "string", ACTION_VERSION_DESC_SHORT     , ACTION_VERSION_DESC_LONG     , NAUTILUS_ACTIONS_CONFIG_VERSION, FALSE );
	create_gconf_schema_entry( writer, OBJECT_ITEM_LABEL_ENTRY    , doc, list_node, "string", ACTION_LABEL_DESC_SHORT       , ACTION_LABEL_DESC_LONG       , "", TRUE );
	create_gconf_schema_entry( writer, OBJECT_ITEM_TOOLTIP_ENTRY  , doc, list_node, "string", ACTION_TOOLTIP_DESC_SHORT     , ACTION_TOOLTIP_DESC_LONG     , "", TRUE );
	create_gconf_schema_entry( writer, OBJECT_ITEM_ICON_ENTRY     , doc, list_node, "string", ACTION_ICON_DESC_SHORT        , ACTION_ICON_DESC_LONG        , "", FALSE );
	create_gconf_schema_entry( writer, OBJECT_ITEM_ENABLED_ENTRY  , doc, list_node, "bool"  , ACTION_ENABLED_DESC_SHORT     , ACTION_ENABLED_DESC_LONG     , "true", FALSE );
	create_gconf_schema_entry( writer, OBJECT_ITEM_LIST_ENTRY     , doc, list_node, "list"  , ACTION_ITEMS_DESC_SHORT       , ACTION_ITEMS_DESC_LONG       , "[]", FALSE );
	create_gconf_schema_entry( writer, OBJECT_ITEM_TYPE_ENTRY     , doc, list_node, "string", ACTION_TYPE_DESC_SHORT        , ACTION_TYPE_DESC_LONG        , "Action", FALSE );
	create_gconf_schema_entry( writer, OBJECT_ITEM_TARGET_SELECTION_ENTRY  , doc, list_node,   "bool", ACTION_TARGET_SELECTION_DESC_SHORT  , ACTION_TARGET_SELECTION_DESC_LONG  ,  "true", FALSE );
	create_gconf_schema_entry( writer, OBJECT_ITEM_TARGET_BACKGROUND_ENTRY , doc, list_node,   "bool", ACTION_TARGET_BACKGROUND_DESC_SHORT , ACTION_TARGET_BACKGROUND_DESC_LONG , "false", FALSE );
	create_gconf_schema_entry( writer, OBJECT_ITEM_TARGET_TOOLBAR_ENTRY    , doc, list_node,   "bool", ACTION_TARGET_TOOLBAR_DESC_SHORT    , ACTION_TARGET_TOOLBAR_DESC_LONG    , "false", FALSE );
	/*create_gconf_schema_entry( writer, OBJECT_ITEM_TOOLBAR_SAME_LABEL_ENTRY, doc, list_node,   "bool", ACTION_TOOLBAR_SAME_LABEL_DESC_SHORT, ACTION_TOOLBAR_SAME_LABEL_DESC_LONG,  "true", FALSE );*/
	create_gconf_schema_entry( writer, OBJECT_ITEM_TOOLBAR_LABEL_ENTRY     , doc, list_node, "string", ACTION_TOOLBAR_LABEL_DESC_SHORT     , ACTION_TOOLBAR_LABEL_DESC_LONG     , "", TRUE );
	create_gconf_schema_entry( writer, ACTION_PROFILE_LABEL_ENTRY , doc, list_node, "string", ACTION_PROFILE_NAME_DESC_SHORT, ACTION_PROFILE_NAME_DESC_LONG, NA_OBJECT_PROFILE_DEFAULT_LABEL, TRUE );
	create_gconf_schema_entry( writer, ACTION_PATH_ENTRY          , doc, list_node, "string", ACTION_PATH_DESC_SHORT        , ACTION_PATH_DESC_LONG        , "", FALSE );
	create_gconf_schema_entry( writer, ACTION_PARAMETERS_ENTRY    , doc, list_node, "string", ACTION_PARAMETERS_DESC_SHORT  , ACTION_PARAMETERS_DESC_LONG  , "", FALSE );
	create_gconf_schema_entry( writer, ACTION_BASENAMES_ENTRY     , doc, list_node, "list"  , ACTION_BASENAMES_DESC_SHORT   , ACTION_BASENAMES_DESC_LONG   , "[*]", FALSE );
	create_gconf_schema_entry( writer, ACTION_MATCHCASE_ENTRY     , doc, list_node, "bool"  , ACTION_MATCHCASE_DESC_SHORT   , ACTION_MATCHCASE_DESC_LONG   , "true", FALSE );
	create_gconf_schema_entry( writer, ACTION_MIMETYPES_ENTRY     , doc, list_node, "list"  , ACTION_MIMETYPES_DESC_SHORT   , ACTION_MIMETYPES_DESC_LONG   , "[*/*]", FALSE );
	create_gconf_schema_entry( writer, ACTION_ISFILE_ENTRY        , doc, list_node, "bool"  , ACTION_ISFILE_DESC_SHORT      , ACTION_ISFILE_DESC_LONG      , "true", FALSE );
	create_gconf_schema_entry( writer, ACTION_ISDIR_ENTRY         , doc, list_node, "bool"  , ACTION_ISDIR_DESC_SHORT       , ACTION_ISDIR_DESC_LONG       , "false", FALSE );
	create_gconf_schema_entry( writer, ACTION_MULTIPLE_ENTRY      , doc, list_node, "bool"  , ACTION_MULTIPLE_DESC_SHORT    , ACTION_MULTIPLE_DESC_LONG    , "false", FALSE );
	create_gconf_schema_entry( writer, ACTION_SCHEMES_ENTRY       , doc, list_node, "list"  , ACTION_SCHEMES_DESC_SHORT     , ACTION_SCHEMES_DESC_LONG     , "[file]", FALSE );
	create_gconf_schema_entry( writer, ACTION_FOLDERS_ENTRY       , doc, list_node, "list"  , ACTION_FOLDERS_DESC_SHORT     , ACTION_FOLDERS_DESC_LONG     , "[]", FALSE );

	return( doc );
}

static void
create_gconf_schema_entry( NAXMLWriter *writer,
		const gchar *entry,
		xmlDocPtr doc, xmlNodePtr list_node, const gchar *type,
		const gchar *short_desc, const gchar *long_desc,
		const gchar *default_value, gboolean is_i18n )
{
	xmlNodePtr schema_node;
	xmlChar *content;
	xmlNodePtr locale_node;

	schema_node = xmlNewChild( list_node, NULL, BAD_CAST( NACT_GCONF_SCHEMA_ENTRY ), NULL );

	content = BAD_CAST( g_build_path( "/", NAUTILUS_ACTIONS_GCONF_SCHEMASDIR, NA_GCONF_CONFIG_PATH, entry, NULL ));
	xmlNewChild( schema_node, NULL, BAD_CAST( NACT_GCONF_SCHEMA_KEY ), content );
	xmlFree( content );

	xmlNewChild( schema_node, NULL, BAD_CAST( NACT_GCONF_SCHEMA_OWNER ), BAD_CAST( PACKAGE_TARNAME ));

	xmlNewChild( schema_node, NULL, BAD_CAST( NACT_GCONF_SCHEMA_TYPE ), BAD_CAST( type ));
	if( !g_ascii_strcasecmp( type, "list" )){
		xmlNewChild( schema_node, NULL, BAD_CAST( NACT_GCONF_SCHEMA_LIST_TYPE ), BAD_CAST( "string" ));
	}

	locale_node = xmlNewChild( schema_node, NULL, BAD_CAST( NACT_GCONF_SCHEMA_LOCALE ), NULL );
	xmlNewProp( locale_node, BAD_CAST( "name" ), BAD_CAST( "C" ));

	content = xmlEncodeSpecialChars( doc, BAD_CAST( default_value ));
	xmlNewChild( schema_node, NULL, BAD_CAST( NACT_GCONF_SCHEMA_DEFAULT ), content );
	if( is_i18n ){
		xmlNewChild( locale_node, NULL, BAD_CAST( NACT_GCONF_SCHEMA_DEFAULT ), content );
	}
	xmlFree( content );

	xmlNewChild( locale_node, NULL, BAD_CAST( NACT_GCONF_SCHEMA_SHORT ), BAD_CAST( short_desc ));

	xmlNewChild( locale_node, NULL, BAD_CAST( NACT_GCONF_SCHEMA_LONG ), BAD_CAST( long_desc ));
}
#endif
