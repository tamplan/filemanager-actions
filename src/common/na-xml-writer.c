/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009 Pierre Wieser and others (see AUTHORS)
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

#include "na-object-api.h"
#include "na-object-action.h"
#include "na-object-profile.h"
#include "na-gconf-provider-keys.h"
#include "na-utils.h"
#include "na-xml-names.h"
#include "na-xml-writer.h"

/* private class data
 */
struct NAXMLWriterClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NAXMLWriterPrivate {
	gboolean  dispose_has_run;
	gchar    *uuid;
};

/* instance properties
 */
enum {
	XML_WRITER_PROP_UUID_ID = 1
};

#define XLM_WRITER_PROP_UUID		"na-xml-writer-uuid"

static GObjectClass *st_parent_class = NULL;

static GType        register_type( void );
static void         class_init( NAXMLWriterClass *klass );
static void         instance_init( GTypeInstance *instance, gpointer klass );
static void         instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void         instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void         instance_dispose( GObject *object );
static void         instance_finalize( GObject *object );

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

GType
na_xml_writer_get_type( void )
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
	static const gchar *thisfn = "na_xml_writer_register_type";
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
	static const gchar *thisfn = "na_xml_writer_class_init";
	GObjectClass *object_class;
	GParamSpec *spec;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
	object_class->get_property = instance_get_property;
	object_class->set_property = instance_set_property;

	spec = g_param_spec_string(
			XLM_WRITER_PROP_UUID,
			XLM_WRITER_PROP_UUID,
			"UUID of the action for which we invoke this XMLWriter", "",
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, XML_WRITER_PROP_UUID_ID, spec );

	klass->private = g_new0( NAXMLWriterClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "na_xml_writer_instance_init";
	NAXMLWriter *self;

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );
	g_return_if_fail( NA_IS_XML_WRITER( instance ));
	self = NA_XML_WRITER( instance );

	self->private = g_new0( NAXMLWriterPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

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

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "na_xml_writer_instance_dispose";
	NAXMLWriter *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_return_if_fail( NA_IS_XML_WRITER( object ));
	self = NA_XML_WRITER( object );

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

	g_return_if_fail( NA_IS_XML_WRITER( object ));
	self = NA_XML_WRITER( object );

	g_free( self->private->uuid );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

static NAXMLWriter *
xml_writer_new( const gchar *uuid )
{
	return( g_object_new( NA_XML_WRITER_TYPE, XLM_WRITER_PROP_UUID, uuid, NULL ));
}

/**
 * na_xml_writer_export:
 * @action: the #NAObjectAction to be exported.
 * Can be NULL when exporting schemas ; in this case, format must be
 * FORMAT_GCONFSCHEMA.
 * @folder: the directoy where to write the output XML file.
 * If NULL, the output will be directed to stdout.
 * @format: the export format.
 * @msg: pointer to a buffer which will receive error messages.
 *
 * Export the specified action as an XML file.
 *
 * Returns: the written filename, or NULL if written to stdout.
 */
gchar *
na_xml_writer_export( const NAObjectAction *action, const gchar *folder, gint format, gchar **msg )
{
	static const gchar *thisfn = "na_xml_writer_export";
	gchar *filename = NULL;
	gchar *xml_buffer;

	g_debug( "%s: action=%p, format=%u", thisfn, ( void * ) action, format );
	g_return_val_if_fail( action || format == FORMAT_GCONFSCHEMA, NULL );

	switch( format ){
		case FORMAT_GCONFSCHEMAFILE_V1:
		case FORMAT_GCONFSCHEMAFILE_V2:
			filename = na_xml_writer_get_output_fname( action, folder, format );
			break;

		/* this is the format used by nautilus-actions-new utility,
		 * and that's why this option takes care of a NULL folder
		 */
		case FORMAT_GCONFENTRY:
			if( folder ){
				filename = na_xml_writer_get_output_fname( action, folder, format );
			}
			break;

		/* this is the format used by nautilus-actions-install-schema
		 * utility, and that's why this option takes care of a NULL
		 * folder, or an output filename
		 */
		case FORMAT_GCONFSCHEMA:
			if( folder ){
				filename = g_strdup( folder );
			}
			break;
	}

	g_assert( filename || folder == NULL );

	xml_buffer = na_xml_writer_get_xml_buffer( action, format );

	if( folder ){
		na_xml_writer_output_xml( xml_buffer, filename );
	} else {
		g_print( "%s", xml_buffer );
	}

	g_free( xml_buffer );

	return( filename );
}

/**
 * na_xml_writer_get_output_fname:
 * @action: the #NAObjectAction to be exported.
 * @folder: the uri of the directoy where to write the output XML file.
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
 * is possible, though very unlikely, that the given file be created
 * between our test of inexistance and the actual write.
 */
gchar *
na_xml_writer_get_output_fname( const NAObjectAction *action, const gchar *folder, gint format )
{
	gchar *uuid;
	gchar *canonical_fname = NULL;
	gchar *canonical_ext = NULL;
	gchar *candidate_fname;
	gint counter;

	g_return_val_if_fail( action, NULL );
	g_return_val_if_fail( folder, NULL );
	g_return_val_if_fail( strlen( folder ), NULL );

	uuid = na_object_get_id( action );

	switch( format ){
		case FORMAT_GCONFSCHEMAFILE_V1:
			canonical_fname = g_strdup_printf( "config_%s", uuid );
			canonical_ext = g_strdup( "schemas" );
			break;

		case FORMAT_GCONFSCHEMAFILE_V2:
			canonical_fname = g_strdup_printf( "config-%s", uuid );
			canonical_ext = g_strdup( "schema" );
			break;

		case FORMAT_GCONFENTRY:
			canonical_fname = g_strdup_printf( "action-%s", uuid );
			canonical_ext = g_strdup( "xml" );
			break;
	}

	g_free( uuid );
	g_return_val_if_fail( canonical_fname, NULL );

	candidate_fname = g_strdup_printf( "%s/%s.%s", folder, canonical_fname, canonical_ext );

	if( !na_utils_exist_file( candidate_fname )){
		g_free( canonical_fname );
		g_free( canonical_ext );
		return( candidate_fname );
	}

	for( counter = 0 ; ; ++counter ){
		g_free( candidate_fname );
		candidate_fname = g_strdup_printf( "%s/%s_%d.%s", folder, canonical_fname, counter, canonical_ext );
		if( !na_utils_exist_file( candidate_fname )){
			break;
		}
	}

	g_free( canonical_fname );
	g_free( canonical_ext );

	return( candidate_fname );
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

	g_return_val_if_fail( action || format == FORMAT_GCONFSCHEMA, NULL );

	uuid = action ? na_object_get_id( action ) : NULL;
	writer = xml_writer_new( uuid );
	g_free( uuid );

	switch( format ){
		case FORMAT_GCONFSCHEMAFILE_V1:
		case FORMAT_GCONFSCHEMAFILE_V2:
			doc = create_xml_schema( writer, format, action );
			break;

		case FORMAT_GCONFENTRY:
			doc = create_xml_dump( writer, format, action );
			break;

		case FORMAT_GCONFSCHEMA:
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

/**
 * na_xml_writer_output_xml:
 * @action: the #NAObjectAction to be exported.
 * @filename: the uri of the output filename
 *
 * Exports an action to the given filename.
 */
void
na_xml_writer_output_xml( const gchar *xml, const gchar *filename )
{
	static const gchar *thisfn = "na_xml_writer_output_xml";
	GFile *file;
	GFileOutputStream *stream;
	GError *error = NULL;

	g_return_if_fail( xml );
	g_return_if_fail( filename && g_utf8_strlen( filename, -1 ));

	file = g_file_new_for_uri( filename );

	stream = g_file_create( file, G_FILE_CREATE_REPLACE_DESTINATION, NULL, &error );
	if( error ){
		g_warning( "%s: %s", thisfn, error->message );
		g_error_free( error );
		g_object_unref( stream );
		g_object_unref( file );
		return;
	}

	g_output_stream_write( G_OUTPUT_STREAM( stream ), xml, g_utf8_strlen( xml, -1 ), NULL, &error );
	if( error ){
		g_warning( "%s: %s", thisfn, error->message );
		g_error_free( error );
		g_object_unref( stream );
		g_object_unref( file );
		return;
	}

	g_output_stream_close( G_OUTPUT_STREAM( stream ), NULL, &error );
	if( error ){
		g_warning( "%s: %s", thisfn, error->message );
		g_error_free( error );
		g_object_unref( stream );
		g_object_unref( file );
		return;
	}

	g_object_unref( stream );
	g_object_unref( file );
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

	profiles = na_object_get_items( action );

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

		g_free( profile_dir );
	}

	na_object_free_items( profiles );

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
	if( format == FORMAT_GCONFSCHEMAFILE_V1 ){
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

	profiles = na_object_get_items( action );

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

		g_free( profile_dir );
	}

	na_object_free_items( profiles );

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
