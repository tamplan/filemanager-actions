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

#include <glib/gi18n.h>
#include <libxml/tree.h>
#include <string.h>

#include <api/na-core-utils.h>
#include <api/na-ifactory-provider.h>
#include <api/na-object-api.h>

#include <io-gconf/nagp-keys.h>

#include "naxml-keys.h"
#include "naxml-reader.h"

/* private class data
 */
struct NAXMLReaderClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* the association between a document root node key and the functions
 */
typedef struct {
	gchar     *root_key;
	gchar     *list_key;
	gchar     *element_key;
	guint   ( *fn_root_parms )     ( NAXMLReader *, xmlNode * );
	guint   ( *fn_list_parms )     ( NAXMLReader *, xmlNode * );
	guint   ( *fn_element_parms )  ( NAXMLReader *, xmlNode * );
	guint   ( *fn_element_content )( NAXMLReader *, xmlNode * );
	gchar * ( *fn_get_value )      ( NAXMLReader *, xmlNode *, const NADataDef *def );
}
	RootNodeStr;

/* private instance data
 * main naxml_reader_import_from_uri() function is called once for each file
 * to import. We thus have one NAXMLReader object per import operation.
 */
struct NAXMLReaderPrivate {
	gboolean          dispose_has_run;

	/* data provided by the caller
	 */
	NAIImporter      *importer;
	NAIImporterParms *parms;

	/* data dynamically set during the import operation
	 */
	gboolean      type_found;
	GList        *nodes;
	RootNodeStr  *root_node_str;
	gchar        *item_id;

	/* following values are reset and reused while iterating on each
	 * element nodes of the imported item (cf. reset_node_data())
	 */
	gboolean      node_ok;
};

#define SCHEMA_PATH_ID_IDX		4		/* index of item id in a GConf schema key path */

extern NAXMLKeyStr naxml_schema_key_schema_str[];

static GObjectClass *st_parent_class = NULL;

static GType         register_type( void );
static void          class_init( NAXMLReaderClass *klass );
static void          instance_init( GTypeInstance *instance, gpointer klass );
static void          instance_dispose( GObject *object );
static void          instance_finalize( GObject *object );

static NAXMLReader  *reader_new( void );

static guint         schema_parse_schema_content( NAXMLReader *reader, xmlNode *node );
static void          schema_check_for_id( NAXMLReader *reader, xmlNode *iter );
static void          schema_check_for_type( NAXMLReader *reader, xmlNode *iter );
static gchar        *schema_get_value( xmlNode *node );
static gchar        *schema_get_locale_value( xmlNode *node );
static gchar        *schema_read_value( NAXMLReader *reader, xmlNode *node, const NADataDef *def );

static guint         dump_parse_list_parms( NAXMLReader *reader, xmlNode *node );
static guint         dump_parse_entry_content( NAXMLReader *reader, xmlNode *node );
static gchar        *dump_read_value( NAXMLReader *reader, xmlNode *node, const NADataDef *def );

static RootNodeStr st_root_node_str[] = {

	{ NAXML_KEY_SCHEMA_ROOT,
			NAXML_KEY_SCHEMA_LIST,
			NAXML_KEY_SCHEMA_NODE,
			NULL,
			NULL,
			NULL,
			schema_parse_schema_content,
			schema_read_value },

	{ NAXML_KEY_DUMP_ROOT,
			NAXML_KEY_DUMP_LIST,
			NAXML_KEY_DUMP_NODE,
			NULL,
			dump_parse_list_parms,
			NULL,
			dump_parse_entry_content,
			dump_read_value },

	{ NULL }
};

#if 0
typedef struct {
	char    *entry;
	gboolean entry_found;
	gboolean locale_waited;
	gboolean profile_waited;
	gboolean list_waited;
}
	GConfReaderStruct;

static GConfReaderStruct reader_str[] = {
	{ ACTION_VERSION_ENTRY      , FALSE, FALSE, FALSE, FALSE },
	{ OBJECT_ITEM_LABEL_ENTRY   , FALSE,  TRUE, FALSE, FALSE },
	{ OBJECT_ITEM_TOOLTIP_ENTRY , FALSE,  TRUE, FALSE, FALSE },
	{ OBJECT_ITEM_ICON_ENTRY    , FALSE, FALSE, FALSE, FALSE },
	{ OBJECT_ITEM_ENABLED_ENTRY , FALSE, FALSE, FALSE, FALSE },
	{ OBJECT_ITEM_LIST_ENTRY    , FALSE, FALSE, FALSE, FALSE },
	{ OBJECT_ITEM_TYPE_ENTRY    , FALSE, FALSE, FALSE, FALSE },
	{ OBJECT_ITEM_TARGET_SELECTION_ENTRY  , FALSE, FALSE, FALSE, FALSE },
	{ OBJECT_ITEM_TARGET_BACKGROUND_ENTRY , FALSE, FALSE, FALSE, FALSE },
	{ OBJECT_ITEM_TARGET_TOOLBAR_ENTRY    , FALSE, FALSE, FALSE, FALSE },
	{ OBJECT_ITEM_TOOLBAR_SAME_LABEL_ENTRY, FALSE, FALSE, FALSE, FALSE },
	{ OBJECT_ITEM_TOOLBAR_LABEL_ENTRY     ,  TRUE, FALSE, FALSE, FALSE },
	{ ACTION_PROFILE_LABEL_ENTRY, FALSE,  TRUE,  TRUE, FALSE },
	{ ACTION_PATH_ENTRY         , FALSE, FALSE,  TRUE, FALSE },
	{ ACTION_PARAMETERS_ENTRY   , FALSE, FALSE,  TRUE, FALSE },
	{ ACTION_BASENAMES_ENTRY    , FALSE, FALSE,  TRUE,  TRUE },
	{ ACTION_MATCHCASE_ENTRY    , FALSE, FALSE,  TRUE, FALSE },
	{ ACTION_ISFILE_ENTRY       , FALSE, FALSE,  TRUE, FALSE },
	{ ACTION_ISDIR_ENTRY        , FALSE, FALSE,  TRUE, FALSE },
	{ ACTION_MULTIPLE_ENTRY     , FALSE, FALSE,  TRUE, FALSE },
	{ ACTION_MIMETYPES_ENTRY    , FALSE, FALSE,  TRUE,  TRUE },
	{ ACTION_SCHEMES_ENTRY      , FALSE, FALSE,  TRUE,  TRUE },
	{ ACTION_FOLDERS_ENTRY      , FALSE, FALSE,  TRUE,  TRUE },
	{                       NULL, FALSE, FALSE, FALSE, FALSE },
};
#endif

#define ERR_XMLDOC_UNABLE_TOPARSE	_( "Unable to parse XML file: %s." )
#define ERR_ROOT_UNKNOWN			_( "Invalid XML root element %s found at line %d while waiting for %s." )
#define ERR_NODE_UNKNOWN			_( "Unknown element %s found at line %d while waiting for %s." )
#define ERR_NODE_ALREADY_FOUND		_( "Element %s at line %d already found, ignored." )
/* i18n: do not translate keywords 'Action' nor 'Menu' */
#define ERR_NODE_UNKNOWN_TYPE		_( "Unknown type %s found at line %d, while waiting for Action or Menu." )
#define ERR_PATH_LENGTH				_( "Too many elements in key path %s." )
#define ERR_MENU_UNWAITED			_( "Unwaited key path %s while importing a menu." )
#define ERR_ITEM_ID_NOT_FOUND		_( "Item ID not found." )
#define ERR_NODE_INVALID_ID			_( "Invalid Item ID: waited for %s, found %s at line %d." )

#if 0
#define ERR_ITEM_LABEL_NOT_FOUND	_( "Item label not found." )
#define ERR_IGNORED_SCHEMA			_( "Schema is ignored at line %d." )
#define ERR_UNEXPECTED_NODE			_( "Unexpected '%s' node found at line %d." )
#define ERR_UNEXPECTED_ENTRY		_( "Unexpected '%s' entry found at line %d." )
#define ERR_NODE_NOT_FOUND			_( "Mandatory node '%s' not found." )
#define ERR_NO_VALUE_FOUND			_( "No value found." )
#define ERR_INVALID_KEY_PREFIX		_( "Invalid content: waited for %s prefix, found %s at line %d." )
#define ERR_NOT_AN_UUID				_( "Invalid UUID %s found at line %d." )
#define ERR_UUID_ALREADY_EXISTS		_( "Already existing action (UUID: %s)." )
#define ERR_VALUE_ALREADY_SET		_( "Value '%s' already set: new value ignored at line %d." )
#endif

static void          factory_provider_read_done_action( NAXMLReader *reader, GSList **messages );

static guint         reader_parse_xmldoc( NAXMLReader *reader );
static guint         iter_on_root_children( NAXMLReader *reader, xmlNode *root );
static guint         iter_on_list_children( NAXMLReader *reader, xmlNode *first );

static void          add_message( NAXMLReader *reader, const gchar *format, ... );
static gchar        *build_key_node_list( NAXMLKeyStr *strlist );
static gchar        *build_root_node_list( void );
static void          reset_node_data( NAXMLReader *reader );
static xmlNode      *search_for_child_node( xmlNode *node, const gchar *key );
static int           strxcmp( const xmlChar *a, const char *b );


#if 0
static void          iter_on_list_children_run( NAXMLReader *reader, xmlNode *list );
static void          iter_on_elements_list( NAXMLReader *reader );
#endif
#if 0
static void          set_schema_applyto_value( NAXMLReader *reader, xmlNode *node, const gchar *entry );
#endif


#if 0
static void          reader_parse_schemalist( NAXMLReader *reader, xmlNode *schemalist );
static gboolean      reader_parse_schema( NAXMLReader *reader, xmlNode *schema );
static gboolean      reader_parse_applyto( NAXMLReader *reader, xmlNode *node );
static gboolean      reader_check_for_entry( NAXMLReader *reader, xmlNode *node, const char *entry );
static gboolean      reader_parse_locale( NAXMLReader *reader, xmlNode *node );
static void          reader_parse_default( NAXMLReader *reader, xmlNode *node );
static gchar        *get_profile_name_from_schema_key( const gchar *key, const gchar *uuid );

static void          reader_parse_entrylist( NAXMLReader *reader, xmlNode *entrylist );
static gboolean      reader_parse_entry( NAXMLReader *reader, xmlNode *entry );
static gboolean      reader_parse_dump_key( NAXMLReader *reader, xmlNode *key );
static void          reader_parse_dump_value( NAXMLReader *reader, xmlNode *key );
static void          reader_parse_dump_value_list( NAXMLReader *reader, xmlNode *key );
static gchar        *get_profile_name_from_dump_key( const gchar *key );

static void          apply_values( NAXMLReader *reader );
static gchar        *get_uuid_from_key( NAXMLReader *reader, const gchar *key, guint line );
static gboolean      is_uuid_valid( const gchar *uuid );
static gchar        *get_entry_from_key( const gchar *key );

#endif
static guint      manage_import_mode( NAXMLReader *reader );
#if 0
static void          propagate_default_values( NAXMLReader *reader );
static NAObjectItem *search_in_auxiliaries( NAXMLReader *reader, const gchar *uuid );
static void          relabel( NAXMLReader *reader );
#endif

GType
naxml_reader_get_type( void )
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
	static const gchar *thisfn = "naxml_reader_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NAXMLReaderClass ),
		NULL,
		NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NAXMLReader ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "NAXMLReader", &info, 0 );

	return( type );
}

static void
class_init( NAXMLReaderClass *klass )
{
	static const gchar *thisfn = "naxml_reader_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NAXMLReaderClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "naxml_reader_instance_init";
	NAXMLReader *self;

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );
	g_return_if_fail( NAXML_IS_READER( instance ));
	self = NAXML_READER( instance );

	self->private = g_new0( NAXMLReaderPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->importer = NULL;
	self->private->parms = NULL;
	self->private->type_found = FALSE;
	self->private->nodes = NULL;
	self->private->root_node_str = NULL;
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "naxml_reader_instance_dispose";
	NAXMLReader *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_return_if_fail( NAXML_IS_READER( object ));
	self = NAXML_READER( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		g_list_free( self->private->nodes );

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "naxml_reader_instance_finalize";
	NAXMLReader *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_return_if_fail( NAXML_IS_READER( object ));
	self = NAXML_READER( object );

	g_free( self->private->item_id );

	reset_node_data( self );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

static NAXMLReader *
reader_new( void )
{
	return( g_object_new( NAXML_READER_TYPE, NULL ));
}

/**
 * naxml_reader_import_uri:
 * @instance: the #NAIImporter provider.
 * @parms: a #NAIImporterParms structure.
 *
 * Imports an item.
 *
 * Returns: the import operation code.
 */
guint
naxml_reader_import_from_uri( const NAIImporter *instance, NAIImporterParms *parms )
{
	static const gchar *thisfn = "naxml_reader_import_from_uri";
	NAXMLReader *reader;
	guint code;

	g_debug( "%s: instance=%p, parms=%p", thisfn, ( void * ) instance, ( void * ) parms );

	g_return_val_if_fail( NA_IS_IIMPORTER( instance ), IMPORTER_CODE_PROGRAM_ERROR );

	reader = reader_new();
	reader->private->importer = ( NAIImporter * ) instance;
	reader->private->parms = parms;

	parms->item = NULL;

	code = reader_parse_xmldoc( reader );

	if( code == IMPORTER_CODE_OK ){
		g_assert( NA_IS_OBJECT_ITEM( reader->private->parms->item ));
#if 0
		propagate_default_values( reader );
#endif
		code = manage_import_mode( reader );
	}

	g_object_unref( reader );

	return( code );
}

/*
 * check that the file is a valid XML document
 * and that the root node can be identified as a schema or a dump
 */
static guint
reader_parse_xmldoc( NAXMLReader *reader )
{
	RootNodeStr *istr;
	gboolean found;
	guint code;

	xmlDoc *doc = xmlParseFile( reader->private->parms->uri );

	if( !doc ){
		xmlErrorPtr error = xmlGetLastError();
		add_message( reader,
				ERR_XMLDOC_UNABLE_TOPARSE, error->message );
		xmlResetError( error );
		code = IMPORTER_CODE_NOT_WILLING_TO;

	} else {
		xmlNode *root_node = xmlDocGetRootElement( doc );

		istr = st_root_node_str;
		found = FALSE;

		while( istr->root_key && !found ){
			if( !strxcmp( root_node->name, istr->root_key )){
				found = TRUE;
				reader->private->root_node_str = istr;
				code = iter_on_root_children( reader, root_node );
			}
			istr++;
		}

		if( !found ){
			gchar *node_list = build_root_node_list();
			add_message( reader,
						ERR_ROOT_UNKNOWN,
						( const char * ) root_node->name, root_node->line, node_list );
			g_free( node_list );
			code = IMPORTER_CODE_NOT_WILLING_TO;
		}

		xmlFreeDoc (doc);
	}

	xmlCleanupParser();
	return( code );
}

/*
 * parse a XML tree
 * - must have one child on the named 'first_child' key (others are warned)
 * - then iter on child nodes of this previous first named which must ne 'next_child'
 */
static guint
iter_on_root_children( NAXMLReader *reader, xmlNode *root )
{
	static const gchar *thisfn = "naxml_reader_iter_on_root_children";
	xmlNodePtr iter;
	gboolean found;
	guint code;

	g_debug( "%s: reader=%p, root=%p", thisfn, ( void * ) reader, ( void * ) root );

	code = IMPORTER_CODE_OK;

	/* deal with properties attached to the root node
	 */
	if( reader->private->root_node_str->fn_root_parms ){
		code = ( *reader->private->root_node_str->fn_root_parms )( reader, root );
	}

	/* iter through the first level of children (list)
	 * we must have only one occurrence of this first 'list' child
	 */
	found = FALSE;
	for( iter = root->children ; iter && code == IMPORTER_CODE_OK ; iter = iter->next ){

		if( iter->type != XML_ELEMENT_NODE ){
			continue;
		}

		if( strxcmp( iter->name, reader->private->root_node_str->list_key )){
			add_message( reader,
					ERR_NODE_UNKNOWN,
					( const char * ) iter->name, iter->line, reader->private->root_node_str->list_key );
			continue;
		}

		if( found ){
			add_message( reader, ERR_NODE_ALREADY_FOUND, ( const char * ) iter->name, iter->line );
			continue;
		}

		found = TRUE;
		code = iter_on_list_children( reader, iter );
	}

	return( code );
}

/*
 * iter on 'schema/entry' element nodes
 * each node should correspond to an elementary data of the imported item
 * other nodes are warned (and ignored)
 *
 * we have to iterate a first time through all nodes to be sure to find
 * a potential 'type' indication - this is needed in order to allocate an
 * action or a menu - if not found at the end of this first pass, we default
 * to allocate an action
 *
 * this first pass is also used to check nodes
 *
 * - for each node, check that
 *   > 'schema/entry' childs are in the list of known schema/entry child nodes
 *   > 'schema/entry' childs appear only once per node
 *     -> this requires a per-node 'found' flag which is reset for each node
 *   > schema has an 'applyto' child node
 *     -> only checkable at the end of the schema
 *
 * - check that each data, identified by the 'applyto' value, appears only once
 *   applyto node -> elementary data + id item + (optionally) id profile
 *   elementary data -> group (action,  menu, profile)
 *   -> this requires a 'found' flag for each group+data reset at item level
 *      as the item may not be allocated yet, we cannot check that data
 *      is actually relevant with the to-be-imported item
 *
 * each schema 'applyto' node let us identify a data and its value
 */
static guint
iter_on_list_children( NAXMLReader *reader, xmlNode *list )
{
	static const gchar *thisfn = "naxml_reader_iter_on_list_children";
	guint code;
	xmlNode *iter;

	g_debug( "%s: reader=%p, list=%p", thisfn, ( void * ) reader, ( void * ) list );

	code = IMPORTER_CODE_OK;

	/* deal with properties attached to the list node
	 */
	if( reader->private->root_node_str->fn_list_parms ){
		code = ( *reader->private->root_node_str->fn_list_parms )( reader, list );
	}

	/* each occurrence should correspond to an elementary data
	 * we run first to determine the type, and allocate the object
	 * we then rely on NAIFactoryProvider to actually read the data
	 */
	for( iter = list->children ; iter && code == IMPORTER_CODE_OK ; iter = iter->next ){

		if( iter->type != XML_ELEMENT_NODE ){
			continue;
		}

		if( strxcmp( iter->name, reader->private->root_node_str->element_key )){
			add_message( reader,
					ERR_NODE_UNKNOWN,
					( const char * ) iter->name, iter->line, reader->private->root_node_str->element_key );
			continue;
		}

		reset_node_data( reader );

		if( reader->private->root_node_str->fn_element_parms ){
			code = ( *reader->private->root_node_str->fn_element_parms )( reader, iter );
			if( code != IMPORTER_CODE_OK ){
				continue;
			}
		}

		if( reader->private->root_node_str->fn_element_content ){
			code = ( *reader->private->root_node_str->fn_element_content )( reader, iter );
			if( code != IMPORTER_CODE_OK ){
				continue;
			}
		}

		if( reader->private->node_ok ){
			reader->private->nodes = g_list_prepend( reader->private->nodes, iter );
		}
	}

	/* check that we have a not empty id
	 */
	if( code == IMPORTER_CODE_OK ){
		if( !reader->private->item_id || !strlen( reader->private->item_id )){
			add_message( reader, ERR_ITEM_ID_NOT_FOUND );
			code = 	IMPORTER_CODE_NO_ITEM_ID;
		}
	}

	/* if type not found, then suppose that we have an action
	 */
	if( code == IMPORTER_CODE_OK ){

		if( !reader->private->type_found ){
			reader->private->parms->item = NA_OBJECT_ITEM( na_object_action_new());
		}

		/* now load the data
		 */
		na_object_set_id( reader->private->parms->item, reader->private->item_id );

		na_ifactory_provider_read_item(
				NA_IFACTORY_PROVIDER( reader->private->importer ),
				reader,
				NA_IFACTORY_OBJECT( reader->private->parms->item ),
				&reader->private->parms->messages );
	}

	return( code );
}

#if 0
/*
 * iter on list child nodes
 * each 'schema/entry' node should correspond to an elementary data of
 * the imported item - other nodes are warned (and ignored)
 * invalid nodes are just ignored
 * the id of the item must have been set during this run
 */
static void
iter_on_list_children_run( NAXMLReader *reader, xmlNode *list )
{
	xmlNode *iter;

	for( iter = list->children ; iter ; iter = iter->next ){

		if( iter->type != XML_ELEMENT_NODE ){
			continue;
		}

		if( strxcmp( iter->name, reader->private->root_node_str->element_key )){
			add_message( reader,
					ERR_NODE_UNKNOWN,
					( const char * ) iter->name, iter->line, reader->private->root_node_str->element_key );
			continue;
		}

		reset_node_data( reader );

		if( reader->private->root_node_str->fn_element_parms ){
			( *reader->private->root_node_str->fn_element_parms )( reader, iter );
		}

		if( reader->private->root_node_str->fn_element_content ){
			( *reader->private->root_node_str->fn_element_content )( reader, iter );
		}

		if( reader->private->node_ok ){

			NAXMLElementStr *str = g_new0( NAXMLElementStr, 1 );
			str->node = reader->private->node_ptr;
			str->key = g_strdup( reader->private->node_key );
			reader->private->elements = g_list_prepend( reader->private->elements, str );
		}
	}
}

/*
 * parse a XML schema
 * root = "gconfschemafile" (already tested)
 *  +- have one descendant node "schemalist"
 *  |   +- have one descendant node per key "schema"
 */
static void
reader_parse_schema_root( NAXMLReader *reader, xmlNode *root )
{
	static const gchar *thisfn = "naxml_reader_parse_schema_root";
	xmlNodePtr iter;
	gboolean found = FALSE;

	g_debug( "%s: reader=%p, root=%p", thisfn, ( void * ) reader, ( void * ) root );

	iter_on_tree_nodes(
			reader, root,
			NAXML_KEY_SCHEMA_LIST, NAXML_KEY_SCHEMA_ENTRY, reader_parse_schema_schema );

	for( iter = root->children ; iter ; iter = iter->next ){

		if( iter->type != XML_ELEMENT_NODE ){
			continue;
		}

		if( strxcmp( iter->name, NAXML_KEY_SCHEMA_LIST )){
			add_message( reader,
					ERR_WAITED_IGNORED_NODE,
					NAXML_KEY_SCHEMA_LIST, ( const char * ) iter->name, iter->line );
			continue;
		}

		if( found ){
			add_message( reader, ERR_IGNORED_NODE, ( const char * ) iter->name, iter->line );
			continue;
		}

		found = TRUE;
		reader_parse_schemalist( reader, iter );
	}
}
#endif

#if 0
static void
iter_on_elements_list( NAXMLReader *reader )
{
	GList *ielt;
	gboolean idset;
	gboolean err;

	idset = FALSE;
	err = FALSE;

	for( ielt = reader->private->elements ; ielt && !err ; ielt = ielt->next ){

		NAXMLElementStr *str = ( NAXMLElementStr * ) ielt->data;
		GSList *path_slist = na_core_utils_slist_from_split(  str->key_path, "/" );
		gchar **path_elts = g_strsplit( str->key_path, "/", -1 );
		guint path_length = g_slist_length( path_slist );
		g_debug( "path=%s, length=%d", str->key_path, path_length );
		/* path=/apps/nautilus-actions/configurations/0af5a47e-96d9-441c-a3b8-d1185ced0351/version, length=6 */
		/* path=/apps/nautilus-actions/configurations/0af5a47e-96d9-441c-a3b8-d1185ced0351/profile-main/schemes, length=7 */

		if( !idset ){
			na_object_set_id( reader->private->item, path_elts[ PATH_ID_IDX ] );
			idset = TRUE;
			gchar *id = na_object_get_id( reader->private->item );
			g_debug( "id=%s", id );
			g_free( id );
		}

		g_debug( "iddef=%p, name=%s, value=%s",
				( void * ) str->iddef, str->iddef ? str->iddef->name : "(null)", str->key_value );

		/* this is for the action or menu body
		 */
		if( path_length == 2+PATH_ID_IDX ){
			na_ifactory_object_set_from_string( NA_IFACTORY_OBJECT( reader->private->item ), str->iddef->id, str->key_value );

		/* this is for a profile
		 */
		} else {
			if( path_length != 3+PATH_ID_IDX ){
				add_message( reader, ERR_PATH_LENGTH, str->key_path );
				err = TRUE;

			} else if( !NA_IS_OBJECT_ACTION( reader->private->item )){
				add_message( reader, ERR_MENU_UNWAITED, str->key_path );
				err = TRUE;

			} else {
				gchar *profile_name = g_strdup( path_elts[ 1+PATH_ID_IDX] );
				NAObjectProfile *profile = ( NAObjectProfile * ) na_object_get_item( reader->private->item, profile_name );
				if( !profile ){
					profile = na_object_profile_new();
					na_object_set_id( profile, profile_name );
					na_object_action_attach_profile( NA_OBJECT_ACTION( reader->private->item ), profile );
				}
				na_ifactory_object_set_from_string( NA_IFACTORY_OBJECT( profile ), str->iddef->id, str->key_value );
				g_free( profile_name );
			}
		}

		na_core_utils_slist_free( path_slist );
		g_strfreev( path_elts );
	}

	if( err ){
		g_object_unref( reader->private->item );
		reader->private->item = NULL;
	}
}
#endif

/*
 * this callback function is called by NAIFactoryObject once for each
 * serializable data for the object
 */
NADataBoxed *
naxml_reader_read_data( const NAIFactoryProvider *provider, void *reader_data, const NAIFactoryObject *object, const NADataDef *def, GSList **messages )
{
	static const gchar *thisfn = "naxml_reader_read_data";
	GList *ielt;

	g_return_val_if_fail( NA_IS_IFACTORY_PROVIDER( provider ), NULL );
	g_return_val_if_fail( NA_IS_IFACTORY_OBJECT( object ), NULL );

	g_debug( "%s: data=%s", thisfn, def->name );

	if( !def->gconf_entry || !strlen( def->gconf_entry )){
		g_warning( "%s: GConf entry is not set for NADataDef %s", thisfn, def->name );
		return( NULL );
	}

	gboolean found = FALSE;
	NADataBoxed *boxed = NULL;
	NAXMLReader *reader = NAXML_READER( reader_data );

	for( ielt = reader->private->nodes ; ielt && !found ; ielt = ielt->next ){

		xmlNode *schema = ( xmlNode * ) ielt->data;
		xmlNode *applyto = search_for_child_node( schema, NAXML_KEY_SCHEMA_NODE_APPLYTO );

		if( !applyto ){
			g_warning( "%s: no 'applyto' node in schema at line %u", thisfn, schema->line );

		} else {
			xmlChar *text = xmlNodeGetContent( applyto );
			gchar *entry = g_path_get_basename(( const gchar * ) text );

			if( !strcmp( entry, def->gconf_entry )){
				found = TRUE;

				if( reader->private->root_node_str->fn_get_value ){
					gchar *value = ( *reader->private->root_node_str->fn_get_value )( reader, schema, def );
					boxed = na_data_boxed_new( def );
					na_data_boxed_set_from_string( boxed, value );
					g_free( value );
				}
			}

			g_free( entry );
			xmlFree( text );
		}
	}

	return( boxed );
}

/*
 * all serializable data of the object has been readen
 */
void
naxml_reader_read_done( const NAIFactoryProvider *provider, void *reader_data, const NAIFactoryObject *object, GSList **messages  )
{
	static const gchar *thisfn = "naxml_reader_read_done";

	g_return_if_fail( NA_IS_IFACTORY_PROVIDER( provider ));
	g_return_if_fail( NA_IS_IFACTORY_OBJECT( object ));

	g_debug( "%s: provider=%p, reader_data=%p, object=%p, messages=%p",
			thisfn, ( void * ) provider, ( void * ) reader_data, ( void * ) object, ( void * ) messages );

	if( NA_IS_OBJECT_ACTION( object )){
		factory_provider_read_done_action( NAXML_READER( reader_data ), messages );
	}
}

static void
factory_provider_read_done_action( NAXMLReader *reader, GSList **messages )
{
	/* do we have a pre-v2 action ?
	 */

}
/*
 * 'key' and 'applyto' keys: check the id
 * 'applyto' key: check for type
 * returns set node_ok if:
 * - each key appears is known and appears only once
 * - there is an applyto key
 */
static guint
schema_parse_schema_content( NAXMLReader *reader, xmlNode *schema )
{
	xmlNode *iter;
	NAXMLKeyStr *str;
	int i;
	guint code;

	code = IMPORTER_CODE_OK;

	for( iter = schema->children ; iter && code == IMPORTER_CODE_OK ; iter = iter->next ){

		if( iter->type != XML_ELEMENT_NODE ){
			continue;
		}

		str = NULL;
		for( i = 0 ; naxml_schema_key_schema_str[i].key && !str ; ++i ){
			if( !strxcmp( iter->name, naxml_schema_key_schema_str[i].key )){
				str = naxml_schema_key_schema_str+i;
			}
		}

		if( !str ){
			gchar *node_list = build_key_node_list( naxml_schema_key_schema_str );
			add_message( reader,
					ERR_NODE_UNKNOWN,
					( const char * ) iter->name, iter->line, node_list );
			g_free( node_list );
			reader->private->node_ok = FALSE;
			continue;
		}

		if( str->reader_found ){
			add_message( reader,
					ERR_NODE_ALREADY_FOUND,
					( const char * ) iter->name, iter->line );
			reader->private->node_ok = FALSE;
			continue;
		}

		str->reader_found = TRUE;

		/* set the item id the first time, check after
		 */
		if( !strxcmp( iter->name, NAXML_KEY_SCHEMA_NODE_KEY ) ||
			!strxcmp( iter->name, NAXML_KEY_SCHEMA_NODE_APPLYTO )){

			schema_check_for_id( reader, iter );

			if( !reader->private->node_ok ){
				continue;
			}
		}

		/* search for the type of the item
		 */
		if( !strxcmp( iter->name, NAXML_KEY_SCHEMA_NODE_APPLYTO )){

			schema_check_for_type( reader, iter );

			if( !reader->private->node_ok ){
				continue;
			}
		}
	}

	return( code );
}

/*
 * check the id on 'key' and 'applyto' keys
 */
static void
schema_check_for_id( NAXMLReader *reader, xmlNode *iter )
{
	guint idx = 1;

	if( !strxcmp( iter->name, NAXML_KEY_SCHEMA_NODE_KEY )){
		idx = 2;
	}

	xmlChar *text = xmlNodeGetContent( iter );
	gchar **path_elts = g_strsplit(( const gchar * ) text, "/", -1 );
	gchar *id = g_strdup( path_elts[ SCHEMA_PATH_ID_IDX+idx-1 ] );
	g_strfreev( path_elts );
	xmlFree( text );

	if( reader->private->item_id ){
		if( strcmp( reader->private->item_id, id ) != 0 ){
			add_message( reader,
					ERR_NODE_INVALID_ID,
					reader->private->item_id, id, iter->line );
			reader->private->node_ok = FALSE;
		}
	} else {
		reader->private->item_id = g_strdup( id );
	}

	g_free( id );
}

/*
 * check 'applyto' key for 'Type'
 */
static void
schema_check_for_type( NAXMLReader *reader, xmlNode *iter )
{
	xmlChar *text = xmlNodeGetContent( iter );

	gchar *entry = g_path_get_basename(( const gchar * ) text );

	if( !strcmp( entry, NAGP_ENTRY_TYPE )){
		reader->private->type_found = TRUE;
		gchar *type = schema_get_value( iter->parent );

		if( !strcmp( type, NAGP_VALUE_TYPE_ACTION )){
			reader->private->parms->item = NA_OBJECT_ITEM( na_object_action_new());

		} else if( !strcmp( type, NAGP_VALUE_TYPE_MENU )){
			reader->private->parms->item = NA_OBJECT_ITEM( na_object_menu_new());

		} else {
			add_message( reader, ERR_NODE_UNKNOWN_TYPE, type, iter->line );
			reader->private->node_ok = FALSE;
		}

		g_free( type );
	}

	g_free( entry );
	xmlFree( text );
}

static gchar *
schema_get_value( xmlNode *node )
{
	gchar *value = NULL;

	xmlNode *default_node = search_for_child_node( node, NAXML_KEY_SCHEMA_NODE_DEFAULT );
	if( default_node ){
		xmlChar *default_value = xmlNodeGetContent( default_node );
		if( default_value ){
			value = g_strdup(( const char * ) default_value );
			xmlFree( default_value );
		}
	}

	return( value );
}

static gchar *
schema_get_locale_value( xmlNode *node )
{
	gchar *value = NULL;

	xmlNode *locale = search_for_child_node( node, NAXML_KEY_SCHEMA_NODE_LOCALE );
	if( locale ){
		xmlNode *default_node = search_for_child_node( locale, NAXML_KEY_SCHEMA_NODE_LOCALE_DEFAULT );
		if( default_node ){
			xmlChar *default_value = xmlNodeGetContent( default_node );
			if( default_value ){
				value = g_strdup(( const char * ) default_value );
				xmlFree( default_value );
			}
		}
	}

	return( value );
}

static gchar *
schema_read_value( NAXMLReader *reader, xmlNode *node, const NADataDef *def )
{
	gchar *value;

	if( def->localizable ){
		value = schema_get_locale_value( node );
	} else {
		value = schema_get_value( node );
	}

	return( value );
}

/*
 * first run: do nothing
 * second run: get the id
 */
static guint
dump_parse_list_parms( NAXMLReader *reader, xmlNode *node )
{
	guint code;

	code = IMPORTER_CODE_OK;

	return( code );
}

/*
 * first_run: only search for a 'Type' key, and allocate the item
 * second run: load data
 */
static guint
dump_parse_entry_content( NAXMLReader *reader, xmlNode *node )
{
	guint code;

	code = IMPORTER_CODE_OK;

	return( code );
}

static gchar *
dump_read_value( NAXMLReader *reader, xmlNode *node, const NADataDef *def )
{
	return( NULL );
}

#if 0
/*
 * iter points to the 'schemalist' node (already checked)
 * children should only be 'schema' nodes ; other nodes are warned,
 * but not fatal
 */
static void
reader_parse_schemalist( NAXMLReader *reader, xmlNode *schema )
{
	static const gchar *thisfn = "reader_parse_schemalist";
	xmlNode *iter;
	gboolean ok = TRUE;

	g_debug( "%s: reader=%p, schema=%p", thisfn, ( void * ) reader, ( void * ) schema );

	reader->private->action = na_object_action_new();
	reader->private->uuid_set = FALSE;

	for( iter = schema->children ; iter ; iter = iter->next ){

		if( iter->type != XML_ELEMENT_NODE ){
			continue;
		}

		if( strxcmp( iter->name, NAXML_KEY_SCHEMA_ENTRY )){
			add_message( reader,
					ERR_WAITED_IGNORED_NODE,
					NAXML_KEY_SCHEMA_ENTRY, ( const char * ) iter->name, iter->line );
			continue;
		}

		if( !reader_parse_schema( reader, iter )){
			add_message( reader, ERR_IGNORED_SCHEMA, iter->line );
		}
	}

	if( !reader->private->uuid_set ){
		add_message( reader, ERR_UUID_NOT_FOUND );
		ok = FALSE;
	}

	if( ok ){
		gchar *label = na_object_get_label( reader->private->action );
		if( !label || !g_utf8_strlen( label, -1 )){
			add_message( reader, ERR_ACTION_LABEL_NOT_FOUND );
		}
		g_free( label );
	}

	if( !ok ){
		g_object_unref( reader->private->action );
		reader->private->action = NULL;
	}
}

/*
 * iter points to a 'schema' node (already checked)
 *
 * we can have
 * - schema
 *   +- locale
 *      +- default
 * or
 * - schema
 *   +- default
 * depending of the key's entry.
 *
 * data found in schema node is imported into the action if and only if
 * the whole node is correct ; else the error is warned (but not fatal)
 *
 * note also that versions previous to 1.11 used to export profile label
 * as if it were not localized (which is a bug, though not signaled)
 * so if the profile label is not found inside of locale node, we search
 * for it outside
 *
 * Returns TRUE if the node has been successfully parsed, FALSE else.
 */
static gboolean
reader_parse_schema( NAXMLReader *reader, xmlNode *schema )
{
	static const gchar *thisfn = "reader_parse_schema";
	xmlNode *iter;
	gboolean ret = TRUE;
	gboolean applyto = FALSE;
	gboolean pre_v1_11 = FALSE;
	gboolean locale_found = FALSE;
	gboolean default_found = FALSE;

	g_debug( "%s: reader=%p, schema=%p", thisfn, ( void * ) reader, ( void * ) schema );

	free_reader_values( reader );

	/* check for the children of the 'schema' node
	 * we must only found known keys
	 */
	for( iter = schema->children ; iter ; iter = iter->next ){

		if( iter->type != XML_ELEMENT_NODE ){
			continue;
		}

		if( strxcmp( iter->name, NAXML_KEY_SCHEMA_KEY ) &&
			strxcmp( iter->name, NAXML_KEY_SCHEMA_APPLYTO ) &&
			strxcmp( iter->name, NAXML_KEY_SCHEMA_OWNER ) &&
			strxcmp( iter->name, NAXML_KEY_SCHEMA_TYPE ) &&
			strxcmp( iter->name, NAXML_KEY_SCHEMA_LIST_TYPE ) &&
			strxcmp( iter->name, NAXML_KEY_SCHEMA_LOCALE ) &&
			strxcmp( iter->name, NAXML_KEY_SCHEMA_DEFAULT )){

				add_message( reader, ERR_UNEXPECTED_NODE, ( const char * ) iter->name, iter->line );
				ret = FALSE;
				continue;
		}

		if( !strxcmp( iter->name, NAXML_KEY_SCHEMA_KEY ) ||
			!strxcmp( iter->name, NAXML_KEY_SCHEMA_OWNER ) ||
			!strxcmp( iter->name, NAXML_KEY_SCHEMA_TYPE ) ||
			!strxcmp( iter->name, NAXML_KEY_SCHEMA_LIST_TYPE )){

				pre_v1_11 = TRUE;
				continue;
		}
	}

	if( !ret ){
		return( ret );
	}

	g_debug( "%s: pre_v1_11=%s", thisfn, pre_v1_11 ? "True":"False" );

	/* check for an 'applyto' node
	 * is mandatory
	 * will determine if we are waiting for locale+default or only default
	 */
	reader->private->locale_waited = FALSE;

	for( iter = schema->children ; iter ; iter = iter->next ){

		if( iter->type != XML_ELEMENT_NODE ){
			continue;
		}

		if( !strxcmp( iter->name, NAXML_KEY_SCHEMA_APPLYTO )){

			if( applyto ){
				add_message( reader, ERR_UNEXPECTED_NODE, ( const char * ) iter->name, iter->line );
				ret = FALSE;
			}

			applyto = TRUE;
			ret = reader_parse_applyto( reader, iter );
		}
	}

	if( !applyto ){
		g_assert( ret );
		add_message( reader, ERR_NODE_NOT_FOUND, NAXML_KEY_SCHEMA_APPLYTO );
		ret = FALSE;
	}

	if( !ret ){
		return( ret );
	}

	/* check for and parse locale+default or locale depending of the
	 * previously found 'applyto' node
	 */
	for( iter = schema->children ; iter ; iter = iter->next ){

		if( iter->type != XML_ELEMENT_NODE ){
			continue;
		}

		if( !strxcmp( iter->name, NAXML_KEY_SCHEMA_LOCALE )){

			if( locale_found ){
				add_message( reader, ERR_UNEXPECTED_NODE, ( const char * ) iter->name, iter->line );
				ret = FALSE;

			} else {
				locale_found = TRUE;
				if( reader->private->locale_waited ){
					ret = reader_parse_locale( reader, iter );
				}
			}

		} else if( !strxcmp( iter->name, NAXML_KEY_SCHEMA_DEFAULT )){

			if( default_found ){
				add_message( reader, ERR_UNEXPECTED_NODE, ( const char * ) iter->name, iter->line );
				ret = FALSE;

			} else {
				default_found = TRUE;
				if( !reader->private->locale_waited ||
						( pre_v1_11 && !strcmp( reader->private->entry, ACTION_PROFILE_LABEL_ENTRY ))){
					reader_parse_default( reader, iter );
				}
			}
		}
	}

	if( !reader->private->value && !g_slist_length( reader->private->list_value )){
		g_assert( ret );
		add_message( reader, ERR_NO_VALUE_FOUND );
		ret = FALSE;
	}

	if( ret ){
		apply_values( reader );
	}

	return( ret );
}

static gboolean
reader_parse_applyto( NAXMLReader *reader, xmlNode *node )
{
	static const gchar *thisfn = "reader_parse_applyto";
	gboolean ret = TRUE;
	xmlChar *text;
	gchar *uuid;
	gchar *profile = NULL;
	gchar *entry = NULL;

	g_debug( "%s: reader=%p, node=%p", thisfn, ( void * ) reader, ( void * ) node );

	text = xmlNodeGetContent( node );
	uuid = get_uuid_from_key( reader, ( const gchar * ) text, node->line );

	if( !uuid ){
		ret = FALSE;
	}

	if( ret ){
		if( !reader->private->uuid_set ){
			na_object_set_id( reader->private->action, uuid );
			reader->private->uuid_set = TRUE;

		} else {
			gchar *ref = na_object_get_id( reader->private->action );
			if( g_ascii_strcasecmp(( const gchar * ) uuid, ( const gchar * ) ref )){
				add_message( reader, ERR_INVALID_UUID, ref, uuid, node->line );
				ret = FALSE;
			}
			g_free( ref );
		}
	}

	if( ret ){
		profile = get_profile_name_from_schema_key(( const gchar * ) text, uuid );

		if( profile ){
			reader->private->profile = NA_OBJECT_PROFILE( na_object_get_item( reader->private->action, profile ));

			if( !reader->private->profile ){
				reader->private->profile = na_object_profile_new();
				na_object_set_id( reader->private->profile, profile );
				na_object_action_attach_profile( reader->private->action, reader->private->profile );
			}
		}

		entry = get_entry_from_key(( const gchar * ) text );
		g_assert( entry && strlen( entry ));

		ret = reader_check_for_entry( reader, node, entry );
	}

	g_free( entry );
	g_free( profile );
	g_free( uuid );
	xmlFree( text );

	return( ret );
}

static gboolean
reader_check_for_entry( NAXMLReader *reader, xmlNode *node, const char *entry )
{
	static const gchar *thisfn = "reader_check_for_entry";
	gboolean ret = TRUE;
	gboolean found = FALSE;
	int i;

	g_debug( "%s: reader=%p, node=%p, entry=%s", thisfn, ( void * ) reader, ( void * ) node, entry );

	for( i=0 ; reader_str[i].entry ; ++i ){

		if( !strcmp( reader_str[i].entry, entry )){

			found = TRUE;

			if( reader_str[i].entry_found ){
				add_message( reader, ERR_UNEXPECTED_ENTRY, entry, node->line );
				ret = FALSE;

			} else {
				reader_str[i].entry_found = TRUE;
				reader->private->entry  = g_strdup( reader_str[i].entry );
				reader->private->locale_waited = reader_str[i].locale_waited;
				reader->private->profile_waited = reader_str[i].profile_waited;
				reader->private->list_waited = reader_str[i].list_waited;
			}
		}
	}

	if( !found ){
		g_assert( ret );
		add_message( reader, ERR_UNEXPECTED_ENTRY, entry, node->line );
		ret = FALSE;
	}

	return( ret );
}

/*
 * we only parse 'locale' when we are waiting for a value inside of
 * this node
 */
static gboolean
reader_parse_locale( NAXMLReader *reader, xmlNode *locale )
{
	static const gchar *thisfn = "reader_parse_locale";
	gboolean ret = TRUE;
	xmlNode *iter;
	gboolean default_found = FALSE;

	g_debug( "%s: reader=%p, locale=%p", thisfn, ( void * ) reader, ( void * ) locale );

	for( iter = locale->children ; iter ; iter = iter->next ){

		if( iter->type != XML_ELEMENT_NODE ){
			continue;
		}

		if( strxcmp( iter->name, NAXML_KEY_SCHEMA_SHORT ) &&
			strxcmp( iter->name, NAXML_KEY_SCHEMA_LONG ) &&
			strxcmp( iter->name, NAXML_KEY_SCHEMA_DEFAULT )){

				add_message( reader, ERR_UNEXPECTED_NODE, ( const char * ) iter->name, iter->line );
				ret = FALSE;
				continue;
		}

		if( !strxcmp( iter->name, NAXML_KEY_SCHEMA_SHORT ) ||
			!strxcmp( iter->name, NAXML_KEY_SCHEMA_LONG )){
				continue;
		}

		if( default_found ){
			add_message( reader, ERR_UNEXPECTED_NODE, ( const char * ) iter->name, iter->line );
			ret = FALSE;
			continue;
		}

		g_assert( ret );
		default_found = TRUE;
		reader_parse_default( reader, iter );
	}

	return( ret );
}

static void
reader_parse_default( NAXMLReader *reader, xmlNode *node )
{
	xmlChar *text;
	gchar *value;

	if( reader->private->value ){
		add_message( reader, ERR_VALUE_ALREADY_SET, reader->private->value, node->line );
		return;
	}

	text = xmlNodeGetContent( node );
	value = g_strdup(( const gchar * ) text );

	if( reader->private->list_waited ){
		reader->private->list_value = na_utils_schema_to_gslist( value );
		g_free( value );

	} else {
		reader->private->value = value;
	}

	xmlFree( text );
	/*g_debug( "reader_parse_default: set value=%s", reader->private->value );*/
}

/*
 * prefix was already been checked when extracting the uuid
 */
static gchar *
get_profile_name_from_schema_key( const gchar *key, const gchar *uuid )
{
	gchar *prefix = g_strdup_printf( "%s/%s/%s", NA_GCONF_CONFIG_PATH, uuid, OBJECT_PROFILE_PREFIX );
	gchar *profile_name = NULL;
	gchar *pos;

	if( g_str_has_prefix( key, prefix )){
		profile_name = g_strdup( key + strlen( prefix ));
		pos = g_strrstr( profile_name, "/" );
		if( pos != NULL ){
			*pos = '\0';
		}
	}

	g_free( prefix );
	return( profile_name );
}

/*
 * parse a XML gconf dump
 * root = "gconfentryfile" (already tested)
 *  +- have one descendant node "entrylist"
 *  |   +- have one descendant node per key "entry"
 */
static void
reader_parse_dump_root( NAXMLReader *reader, xmlNode *root )
{
	static const gchar *thisfn = "reader_parse_dump_root";
	xmlNodePtr iter;
	gboolean found = FALSE;

	g_debug( "%s: reader=%p, root=%p", thisfn, ( void * ) reader, ( void * ) root );

	for( iter = root->children ; iter ; iter = iter->next ){

		if( iter->type != XML_ELEMENT_NODE ){
			continue;
		}
		if( strxcmp( iter->name, NAXML_KEY_DUMP_ENTRYLIST )){
			add_message( reader,
					ERR_WAITED_IGNORED_NODE,
					NAXML_KEY_DUMP_ENTRYLIST, ( const char * ) iter->name, iter->line );
			continue;
		}
		if( found ){
			add_message( reader, ERR_IGNORED_NODE, ( const char * ) iter->name, iter->line );
			continue;
		}

		found = TRUE;
		reader_parse_entrylist( reader, iter );
	}
}

/*
 * iter points to the 'entrylist' node (already checked)
 * children should only be 'entry' nodes ; other nodes are warned,
 * but not fatal
 */
static void
reader_parse_entrylist( NAXMLReader *reader, xmlNode *entrylist )
{
	static const gchar *thisfn = "reader_parse_entrylist";
	xmlChar *path;
	gchar *uuid, *label;
	gboolean ok;

	g_debug( "%s: reader=%p, entrylist=%p", thisfn, ( void * ) reader, ( void * ) entrylist );

	reader->private->action = na_object_action_new();
	path = xmlGetProp( entrylist, ( const xmlChar * ) NAXML_KEY_DUMP_ENTRYLIST_BASE );
	uuid = na_gconf_utils_path_to_key(( const gchar * ) path );
	/*g_debug( "%s: uuid=%s", thisfn, uuid );*/

	if( is_uuid_valid( uuid )){
		na_object_set_id( reader->private->action, uuid );
		reader->private->uuid_set = TRUE;

	} else {
		add_message( reader, ERR_NOT_AN_UUID, uuid, entrylist->line );
	}

	g_free( uuid );
	xmlFree( path );
	ok = TRUE;

	if( reader->private->uuid_set ){
		xmlNode *iter;
		for( iter = entrylist->children ; iter ; iter = iter->next ){

			if( iter->type != XML_ELEMENT_NODE ){
				continue;
			}

			if( strxcmp( iter->name, NAXML_KEY_DUMP_ENTRY )){
				add_message( reader,
						ERR_WAITED_IGNORED_NODE,
						NAXML_KEY_DUMP_ENTRY, ( const char * ) iter->name, iter->line );
				continue;
			}

			if( !reader_parse_entry( reader, iter )){
				add_message( reader, ERR_IGNORED_SCHEMA, iter->line );
			}
		}

		label = na_object_get_label( reader->private->action );
		if( !label || !g_utf8_strlen( label, -1 )){
			add_message( reader, ERR_ACTION_LABEL_NOT_FOUND );
			ok = FALSE;
		}
		g_free( label );

	} else {
		g_object_unref( reader->private->action );
		reader->private->action = NULL;
	}
}

static gboolean
reader_parse_entry( NAXMLReader *reader, xmlNode *entry )
{
	static const gchar *thisfn = "reader_parse_entry";
	xmlNode *iter;
	gboolean ret = TRUE;
	gboolean key_found = FALSE;
	gboolean value_found = FALSE;

	g_debug( "%s: reader=%p, entry=%p", thisfn, ( void * ) reader, ( void * ) entry );

	free_reader_values( reader );

	/* check for the children of the 'entry' node
	 * we must only found known keys
	 */
	for( iter = entry->children ; iter ; iter = iter->next ){

		if( iter->type != XML_ELEMENT_NODE ){
			continue;
		}
		if( strxcmp( iter->name, NAXML_KEY_DUMP_KEY ) &&
			strxcmp( iter->name, NAXML_KEY_DUMP_VALUE )){

				add_message( reader, ERR_UNEXPECTED_NODE, ( const char * ) iter->name, iter->line );
				ret = FALSE;
				continue;
		}
	}
	if( !ret ){
		return( ret );
	}

	/* check for one and only one 'key' node
	 */
	for( iter = entry->children ; iter ; iter = iter->next ){

		if( iter->type != XML_ELEMENT_NODE ){
			continue;
		}
		if( !strxcmp( iter->name, NAXML_KEY_DUMP_KEY )){

			if( key_found ){
				add_message( reader, ERR_UNEXPECTED_NODE, ( const char * ) iter->name, iter->line );
				ret = FALSE;

			} else {
				key_found = TRUE;
				ret = reader_parse_dump_key( reader, iter );
			}
		}
	}

	if( !key_found ){
		g_assert( ret );
		add_message( reader, ERR_NODE_NOT_FOUND, NAXML_KEY_DUMP_KEY );
		ret = FALSE;
	}

	if( !ret ){
		return( ret );
	}

	/* check for one and only one 'value' node
	 */
	for( iter = entry->children ; iter ; iter = iter->next ){

		if( iter->type != XML_ELEMENT_NODE ){
			continue;
		}
		if( !strxcmp( iter->name, NAXML_KEY_DUMP_VALUE )){

			if( value_found ){
				add_message( reader, ERR_UNEXPECTED_NODE, ( const char * ) iter->name, iter->line );
				ret = FALSE;

			} else {
				value_found = TRUE;
				reader_parse_dump_value( reader, iter );
			}
		}
	}

	if( !value_found ){
		g_assert( ret );
		add_message( reader, ERR_NO_VALUE_FOUND );
		ret = FALSE;
	}

	if( ret ){
		apply_values( reader );
	}

	return( ret );
}

static gboolean
reader_parse_dump_key( NAXMLReader *reader, xmlNode *node )
{
	static const gchar *thisfn = "reader_parse_dump_key";
	gboolean ret = TRUE;
	xmlChar *text;
	gchar *profile = NULL;
	gchar *entry = NULL;

	g_debug( "%s: reader=%p, node=%p", thisfn, ( void * ) reader, ( void * ) node );

	text = xmlNodeGetContent( node );

	if( ret ){
		profile = get_profile_name_from_dump_key(( const gchar * ) text );

		if( profile ){
			reader->private->profile = NA_OBJECT_PROFILE( na_object_get_item( reader->private->action, profile ));

			if( !reader->private->profile ){
				reader->private->profile = na_object_profile_new();
				na_object_set_id( reader->private->profile, profile );
				na_object_action_attach_profile( reader->private->action, reader->private->profile );
			}
		}

		entry = get_entry_from_key(( const gchar * ) text );
		g_assert( entry && strlen( entry ));

		ret = reader_check_for_entry( reader, node, entry );
	}

	g_free( entry );
	g_free( profile );
	xmlFree( text );

	return( ret );
}

static void
reader_parse_dump_value( NAXMLReader *reader, xmlNode *node )
{
	xmlNode *iter;
	for( iter = node->children ; iter ; iter = iter->next ){

		if( iter->type != XML_ELEMENT_NODE ){
			continue;
		}
		if( strxcmp( iter->name, NAXML_KEY_DUMP_LIST ) &&
			strxcmp( iter->name, NAXML_KEY_DUMP_STRING )){
				add_message( reader,
					ERR_IGNORED_NODE, ( const char * ) iter->name, iter->line );
			continue;
		}
		if( !strxcmp( iter->name, NAXML_KEY_DUMP_STRING )){

			xmlChar *text = xmlNodeGetContent( iter );

			if( reader->private->list_waited ){
				reader->private->list_value = g_slist_append( reader->private->list_value, g_strdup(( const gchar * ) text ));
			} else {
				reader->private->value = g_strdup(( const gchar * ) text );
			}

			xmlFree( text );
			continue;
		}
		reader_parse_dump_value_list( reader, iter );
	}
}

static void
reader_parse_dump_value_list( NAXMLReader *reader, xmlNode *list_node )
{
	xmlNode *iter;
	for( iter = list_node->children ; iter ; iter = iter->next ){

		if( iter->type != XML_ELEMENT_NODE ){
			continue;
		}
		if( strxcmp( iter->name, NAXML_KEY_DUMP_VALUE )){
				add_message( reader,
					ERR_IGNORED_NODE, ( const char * ) iter->name, iter->line );
			continue;
		}
		reader_parse_dump_value( reader, iter );
	}
}

static gchar *
get_profile_name_from_dump_key( const gchar *key )
{
	gchar *profile_name = NULL;
	gchar **split = g_strsplit( key, "/", -1 );
	guint count = g_strv_length( split );

	g_assert( count );

	if( count > 1 ){
		profile_name = g_strdup( split[0] );
	}

	g_strfreev( split );

	return( profile_name );
}

static void
apply_values( NAXMLReader *reader )
{
	static const gchar *thisfn = "naxml_reader_apply_values";

	g_debug( "%s: reader=%p, entry=%s, value=%s",
			thisfn, ( void * ) reader, reader->private->entry, reader->private->value );

	if( reader->private->entry && strlen( reader->private->entry )){
		if( !strcmp( reader->private->entry, ACTION_VERSION_ENTRY )){
			na_object_action_set_version( reader->private->action, reader->private->value );

		} else if( !strcmp( reader->private->entry, OBJECT_ITEM_LABEL_ENTRY )){
			na_object_set_label( reader->private->action, reader->private->value );

		} else if( !strcmp( reader->private->entry, OBJECT_ITEM_TOOLTIP_ENTRY )){
			na_object_set_tooltip( reader->private->action, reader->private->value );

		} else if( !strcmp( reader->private->entry, OBJECT_ITEM_ICON_ENTRY )){
			na_object_set_icon( reader->private->action, reader->private->value );

		} else if( !strcmp( reader->private->entry, OBJECT_ITEM_ENABLED_ENTRY )){
			na_object_set_enabled( NA_OBJECT_ITEM( reader->private->action ), na_utils_schema_to_boolean( reader->private->value, TRUE ));

		} else if( !strcmp( reader->private->entry, OBJECT_ITEM_LIST_ENTRY )){
			na_object_item_set_items_string_list( NA_OBJECT_ITEM( reader->private->action ), reader->private->list_value );

		} else if( !strcmp( reader->private->entry, OBJECT_ITEM_TYPE_ENTRY )){
			/* type (Action, Menu) is not set here, because this must has been
			 * decided at object construction (it is so too late !)
			 */
			;

		} else if( !strcmp( reader->private->entry, OBJECT_ITEM_TARGET_SELECTION_ENTRY )){
			na_object_action_set_target_selection( reader->private->action, na_utils_schema_to_boolean( reader->private->value, TRUE ));

		} else if( !strcmp( reader->private->entry, OBJECT_ITEM_TARGET_BACKGROUND_ENTRY )){
			na_object_action_set_target_background( reader->private->action, na_utils_schema_to_boolean( reader->private->value, FALSE ));

		} else if( !strcmp( reader->private->entry, OBJECT_ITEM_TARGET_TOOLBAR_ENTRY )){
			na_object_action_set_target_toolbar( reader->private->action, na_utils_schema_to_boolean( reader->private->value, FALSE ));

		} else if( !strcmp( reader->private->entry, OBJECT_ITEM_TOOLBAR_SAME_LABEL_ENTRY )){
			/* only used between 2.29.1 and 2.29.4, removed starting with 2.29.5
			na_object_action_toolbar_set_same_label( reader->private->action, na_utils_schema_to_boolean( reader->private->value, TRUE ));
			*/
			;

		} else if( !strcmp( reader->private->entry, OBJECT_ITEM_TOOLBAR_LABEL_ENTRY )){
			na_object_action_toolbar_set_label( reader->private->action, reader->private->value );

		} else if( !strcmp( reader->private->entry, ACTION_PROFILE_LABEL_ENTRY )){
			na_object_set_label( reader->private->profile, reader->private->value );

		} else if( !strcmp( reader->private->entry, ACTION_PATH_ENTRY )){
			na_object_profile_set_path( reader->private->profile, reader->private->value );

		} else if( !strcmp( reader->private->entry, ACTION_PARAMETERS_ENTRY )){
			na_object_profile_set_parameters( reader->private->profile, reader->private->value );

		} else if( !strcmp( reader->private->entry, ACTION_BASENAMES_ENTRY )){
			na_object_profile_set_basenames( reader->private->profile, reader->private->list_value );

		} else if( !strcmp( reader->private->entry, ACTION_MATCHCASE_ENTRY )){
			na_object_profile_set_matchcase( reader->private->profile, na_utils_schema_to_boolean( reader->private->value, TRUE ));

		} else if( !strcmp( reader->private->entry, ACTION_ISFILE_ENTRY )){
			na_object_profile_set_isfile( reader->private->profile, na_utils_schema_to_boolean( reader->private->value, TRUE ));

		} else if( !strcmp( reader->private->entry, ACTION_ISDIR_ENTRY )){
			na_object_profile_set_isdir( reader->private->profile, na_utils_schema_to_boolean( reader->private->value, FALSE ));

		} else if( !strcmp( reader->private->entry, ACTION_MULTIPLE_ENTRY )){
			na_object_profile_set_multiple( reader->private->profile, na_utils_schema_to_boolean( reader->private->value, FALSE ));

		} else if( !strcmp( reader->private->entry, ACTION_MIMETYPES_ENTRY )){
			na_object_profile_set_mimetypes( reader->private->profile, reader->private->list_value );

		} else if( !strcmp( reader->private->entry, ACTION_SCHEMES_ENTRY )){
			na_object_profile_set_schemes( reader->private->profile, reader->private->list_value );

		} else if( !strcmp( reader->private->entry, ACTION_FOLDERS_ENTRY )){
			na_object_profile_set_folders( reader->private->profile, reader->private->list_value );

		} else {
			g_warning( "%s: unprocessed entry %s", thisfn, reader->private->entry );
		}
	}
}
#endif

static void
add_message( NAXMLReader *reader, const gchar *format, ... )
{
	va_list va;
	gchar *tmp;

	va_start( va, format );
	tmp = g_markup_vprintf_escaped( format, va );
	va_end( va );

	reader->private->parms->messages = g_slist_append( reader->private->parms->messages, tmp );
}

static gchar *
build_key_node_list( NAXMLKeyStr *strlist )
{
	NAXMLKeyStr *next;

	NAXMLKeyStr *istr = strlist;
	GString *string = g_string_new( "" );

	while( istr->key ){
		next = istr+1;
		if( string->len ){
			if( next->key ){
				string = g_string_append( string, ", " );
			} else {
				string = g_string_append( string, " or " );
			}
		}
		string = g_string_append( string, istr->key );
		istr++;
	}

	return( g_string_free( string, FALSE ));
}

static gchar *
build_root_node_list( void )
{
	RootNodeStr *next;

	RootNodeStr *istr = st_root_node_str;
	GString *string = g_string_new( "" );

	while( istr->root_key ){
		next = istr+1;
		if( string->len ){
			if( next->root_key ){
				string = g_string_append( string, ", " );
			} else {
				string = g_string_append( string, " or " );
			}
		}
		string = g_string_append( string, istr->root_key );
		istr++;
	}

	return( g_string_free( string, FALSE ));
}

#if 0
static void
free_naxml_element_str( NAXMLElementStr *str, void *data )
{
	g_free( str->key );
	g_free( str );
}
#endif

/*
 * data are reset before first run on nodes for an item
 */
static void
reset_node_data( NAXMLReader *reader )
{
	int i;

	for( i=0 ; naxml_schema_key_schema_str[i].key ; ++i ){
		naxml_schema_key_schema_str[i].reader_found = FALSE;
	}

	reader->private->node_ok = TRUE;
}

#if 0
/*
 * reset here static data which should be reset before each object
 * (so, obviously, not reader data as a new reader is reallocated
 *  for each import operation)
 */
static void
reset_item_data( NAXMLReader *reader )
{
}
#endif

static xmlNode *
search_for_child_node( xmlNode *node, const gchar *key )
{
	xmlNode *iter;

	for( iter = node->children ; iter ; iter = iter->next ){
		if( iter->type == XML_ELEMENT_NODE ){
			if( !strxcmp( iter->name, key )){
				return( iter );
			}
		}
	}

	return( NULL );
}

/*
 * note that up to v 1.10 included, key check was made via a call to
 * g_ascii_strncasecmp, which was doubly wrong:
 * - because XML is case sensitive by definition
 * - because this did not detect a key longer that the reference.
 */
static int
strxcmp( const xmlChar *a, const char *b )
{
	xmlChar *xb = xmlCharStrdup( b );
	int ret = xmlStrcmp( a, xb );
	xmlFree( xb );
	return( ret );
}

#if 0
static gchar *
get_uuid_from_key( NAXMLReader *reader, const gchar *key, guint line )
{
	gchar *uuid, *pos;

	if( !g_str_has_prefix( key, NA_GCONF_CONFIG_PATH )){
		add_message( reader,
				ERR_INVALID_KEY_PREFIX, NA_GCONF_CONFIG_PATH, key, line );
		return( NULL );
	}

	uuid = g_strdup( key + strlen( NA_GCONF_CONFIG_PATH "/" ));
	pos = g_strstr_len( uuid, strlen( uuid ), "/" );
	if( pos != NULL ){
		*pos = '\0';
	}

	if( !is_uuid_valid( uuid )){
		add_message( reader, ERR_NOT_AN_UUID, uuid, line );
		g_free( uuid );
		uuid = NULL;
	}

	return( uuid );
}

static gboolean
is_uuid_valid( const gchar *uuid )
{
	uuid_t uu;
	return( uuid_parse( uuid, uu ) == 0 );
}

static gchar *
get_entry_from_key( const gchar *key )
{
	gchar *pos = g_strrstr( key, "/" );
	gchar *entry = pos ? g_strdup( pos+1 ) : g_strdup( key );
	return( entry );
}
#endif

/*
 * returns IMPORTER_CODE_OK if we can safely insert the action
 * - the id doesn't already exist
 * - the id already exist, but import mode is renumber
 * - the id already exists, but import mode is override
 */
static guint
manage_import_mode( NAXMLReader *reader )
{
#if 0
	NAObjectItem *exists;
	gchar *uuid;
	BaseApplication *appli;
	NactMainWindow *main_window;
	gboolean ret;
	gint mode;

	appli = base_window_get_application( BASE_WINDOW( reader->private->window ));
	main_window = NACT_MAIN_WINDOW( base_application_get_main_window( appli ));

	uuid = na_object_get_id( reader->private->action );
	exists = nact_main_window_get_item( main_window, uuid );
	if( !exists ){
		exists = search_in_auxiliaries( reader, uuid );
	}

	if( !exists ){
		g_free( uuid );
		return( TRUE );
	}

	if( reader->private->import_mode == IPREFS_IMPORT_ASK ){
		mode = nact_assistant_import_ask_user( main_window, reader->private->uri, reader->private->action, exists );
	} else {
		mode = reader->private->import_mode;
	}

	switch( mode ){
		case IPREFS_IMPORT_RENUMBER:
			na_object_set_new_id( reader->private->action, NULL );
			relabel( reader );
			if( reader->private->import_mode == IPREFS_IMPORT_ASK ){
				add_message( reader, "%s", _( "Action was renumbered due to user request." ));
			}
			ret = TRUE;
			break;

		case IPREFS_IMPORT_OVERRIDE:
			if( reader->private->import_mode == IPREFS_IMPORT_ASK ){
				add_message( reader, "%s", _( "Existing action was overriden due to user request." ));
			}
			ret = TRUE;
			break;

		case IPREFS_IMPORT_NO_IMPORT:
		default:
			add_message( reader, ERR_UUID_ALREADY_EXISTS, uuid );
			if( reader->private->import_mode == IPREFS_IMPORT_ASK ){
				add_message( reader, "%s", _( "Import was canceled due to user request." ));
			}
			ret = FALSE;
	}

	g_free( uuid );
	return( ret );
#endif
	return( IMPORTER_CODE_OK );
}

#if 0
static void
propagate_default_values( NAXMLReader *reader )
{
	gchar *action_label, *toolbar_label;
	gboolean same_label;

	/* between 2.29.1 and 2.29.4, we use to have a toolbar_same_label indicator
	 * starting with 2.29.5, we no more have this flag
	 */
	same_label = FALSE;
	action_label = na_object_get_label( reader->private->action );
	toolbar_label = na_object_action_toolbar_get_label( reader->private->action );
	if( !toolbar_label || !g_utf8_strlen( toolbar_label, -1 ) || !g_utf8_collate( toolbar_label, action_label )){
		same_label = TRUE;
		na_object_action_toolbar_set_label( reader->private->action, action_label );
	}
	na_object_action_toolbar_set_same_label( reader->private->action, same_label );
	g_free( toolbar_label );
	g_free( action_label );
}

static NAObjectItem *
search_in_auxiliaries( NAXMLReader *reader, const gchar *uuid )
{
	NAObjectItem *action;
	gchar *aux_uuid;
	GList *it;

	action = NULL;
	for( it = reader->private->auxiliaries ; it && !action ; it = it->next ){
		aux_uuid = na_object_get_id( it->data );
		if( !strcmp( aux_uuid, uuid )){
			action = NA_OBJECT_ITEM( it->data );
		}
		g_free( aux_uuid );
	}
	return( action );
}

/*
 * set a new label because the action has been renumbered
 */
static void
relabel( NAXMLReader *reader )
{
	gchar *label, *tmp;

	label = na_object_get_label( reader->private->action );

	/* i18n: the action has been renumbered during import operation */
	tmp = g_strdup_printf( "%s %s", label, _( "(renumbered)" ));

	na_object_set_label( reader->private->action, tmp );

	g_free( tmp );
	g_free( label );
}
#endif
