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

#include <glib/gi18n.h>
#include <libxml/tree.h>
#include <stdarg.h>
#include <string.h>
#include <uuid/uuid.h>

#include <runtime/na-gconf-provider-keys.h>
#include <runtime/na-gconf-utils.h>

#include <common/na-iprefs.h>
#include <common/na-object-api.h>
#include <common/na-utils.h>
#include <common/na-xml-names.h>

#include "nact-application.h"
#include "nact-main-window.h"
#include "nact-assistant-import-ask.h"
#include "nact-xml-reader.h"

/* private class data
 */
struct NactXMLReaderClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 * we allocate one instance for each imported file, and each imported
 * file should contain one and only one action
 * follow here the import flow
 */
struct NactXMLReaderPrivate {
	gboolean         dispose_has_run;
	BaseWindow      *window;
	gint             import_mode;
	const gchar     *uri;
	GList           *auxiliaries;
	NAObjectAction  *action;			/* the action that we will return, or NULL */
	GSList          *messages;
	gboolean         uuid_set;			/* set at first uuid, then checked against */

	/* following values are reset at each schema/entry node
	 */
	NAObjectProfile *profile;			/* profile */
	gboolean         locale_waited;		/* does this require a locale ? */
	gboolean         profile_waited;	/* does this entry apply to a profile ? */
	gboolean         list_waited;
	gchar           *entry;
	gchar           *value;				/* found value */
	GSList          *list_value;
};

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

#define ERR_UNABLE_PARSE_XML_FILE	_( "Unable to parse XML file: %s." )
#define ERR_ROOT_ELEMENT			_( "Invalid XML root element: waited for '%s' or '%s', found '%s' at line %d." )
#define ERR_WAITED_IGNORED_NODE		_( "Waited for '%s' node, found (ignored) '%s' at line %d." )
#define ERR_IGNORED_NODE			_( "Unexpected (ignored) '%s' node found at line %d." )
#define ERR_IGNORED_SCHEMA			_( "Schema is ignored at line %d." )
#define ERR_UNEXPECTED_NODE			_( "Unexpected '%s' node found at line %d." )
#define ERR_UNEXPECTED_ENTRY		_( "Unexpected '%s' entry found at line %d." )
#define ERR_NODE_NOT_FOUND			_( "Mandatory node '%s' not found." )
#define ERR_NO_VALUE_FOUND			_( "No value found." )
#define ERR_INVALID_UUID			_( "Invalid UUID: waited for %s, found %s at line %d." )
#define ERR_INVALID_KEY_PREFIX		_( "Invalid content: waited for %s prefix, found %s at line %d." )
#define ERR_NOT_AN_UUID				_( "Invalid UUID %s found at line %d." )
#define ERR_UUID_ALREADY_EXISTS		_( "Already existing action (UUID: %s)." )
#define ERR_VALUE_ALREADY_SET		_( "Value '%s' already set: new value ignored at line %d." )
#define ERR_UUID_NOT_FOUND			_( "UUID not found." )
#define ERR_ACTION_LABEL_NOT_FOUND	_( "Action label not found." )

static GObjectClass *st_parent_class = NULL;

static GType          register_type( void );
static void           class_init( NactXMLReaderClass *klass );
static void           instance_init( GTypeInstance *instance, gpointer klass );
static void           instance_dispose( GObject *object );
static void           instance_finalize( GObject *object );

static NactXMLReader *gconf_reader_new( void );

static void           gconf_reader_parse_schema_root( NactXMLReader *reader, xmlNode *root );
static void           gconf_reader_parse_schemalist( NactXMLReader *reader, xmlNode *schemalist );
static gboolean       gconf_reader_parse_schema( NactXMLReader *reader, xmlNode *schema );
static gboolean       gconf_reader_parse_applyto( NactXMLReader *reader, xmlNode *node );
static gboolean       gconf_reader_check_for_entry( NactXMLReader *reader, xmlNode *node, const char *entry );
static gboolean       gconf_reader_parse_locale( NactXMLReader *reader, xmlNode *node );
static void           gconf_reader_parse_default( NactXMLReader *reader, xmlNode *node );
static gchar         *get_profile_name_from_schema_key( const gchar *key, const gchar *uuid );

static void           gconf_reader_parse_dump_root( NactXMLReader *reader, xmlNode *root );
static void           gconf_reader_parse_entrylist( NactXMLReader *reader, xmlNode *entrylist );
static gboolean       gconf_reader_parse_entry( NactXMLReader *reader, xmlNode *entry );
static gboolean       gconf_reader_parse_dump_key( NactXMLReader *reader, xmlNode *key );
static void           gconf_reader_parse_dump_value( NactXMLReader *reader, xmlNode *key );
static void           gconf_reader_parse_dump_value_list( NactXMLReader *reader, xmlNode *key );
static gchar         *get_profile_name_from_dump_key( const gchar *key );

static void           apply_values( NactXMLReader *reader );
static void           add_message( NactXMLReader *reader, const gchar *format, ... );
static int            strxcmp( const xmlChar *a, const char *b );
static gchar         *get_uuid_from_key( NactXMLReader *reader, const gchar *key, guint line );
static gboolean       is_uuid_valid( const gchar *uuid );
static gchar         *get_entry_from_key( const gchar *key );
static void           free_reader_values( NactXMLReader *reader );
static gboolean       manage_import_mode( NactXMLReader *reader );
static NAObjectItem  *search_in_auxiliaries( NactXMLReader *reader, const gchar *uuid );
static void           relabel( NactXMLReader *reader );

GType
nact_xml_reader_get_type( void )
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
	static const gchar *thisfn = "nact_xml_reader_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NactXMLReaderClass ),
		NULL,
		NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactXMLReader ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "NactXMLReader", &info, 0 );

	return( type );
}

static void
class_init( NactXMLReaderClass *klass )
{
	static const gchar *thisfn = "nact_xml_reader_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactXMLReaderClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_xml_reader_instance_init";
	NactXMLReader *self;

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );
	g_return_if_fail( NACT_IS_XML_READER( instance ));
	self = NACT_XML_READER( instance );

	self->private = g_new0( NactXMLReaderPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->action = NULL;
	self->private->messages = NULL;
	self->private->uuid_set = FALSE;
	self->private->profile = NULL;
	self->private->locale_waited = FALSE;
	self->private->entry = NULL;
	self->private->value = NULL;
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "nact_xml_reader_instance_dispose";
	NactXMLReader *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_return_if_fail( NACT_IS_XML_READER( object ));
	self = NACT_XML_READER( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		if( self->private->action ){
			g_return_if_fail( NA_IS_OBJECT_ACTION( self->private->action ));
			g_object_unref( self->private->action );
		}

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "nact_xml_reader_instance_finalize";
	NactXMLReader *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_return_if_fail( NACT_IS_XML_READER( object ));
	self = NACT_XML_READER( object );

	na_utils_free_string_list( self->private->messages );
	free_reader_values( self );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

static NactXMLReader *
gconf_reader_new( void )
{
	return( g_object_new( NACT_XML_READER_TYPE, NULL ));
}

/**
 * nact_xml_reader_import:
 * @window: the #NactAssistantImport instance.
 * @items: an auxiliary list of NAObjectItems in which the existancy of
 *  the imported action could be checked ; this typically correspond to
 *  actions which were previously imported in the assistant
 * @uri: the uri of the file to import.
 * @import_mode: the import mode.
 * @msg: a list of error messages which may be set by this function.
 *
 * Import the specified file as an NAAction XML description.
 *
 * Returns: the imported action, or NULL.
 */
NAObjectAction *
nact_xml_reader_import( BaseWindow *window, GList *items, const gchar *uri, gint import_mode, GSList **msg )
{
	static const gchar *thisfn = "nact_xml_reader_import";
	NAObjectAction *action = NULL;
	NactXMLReader *reader;
	xmlDoc *doc;

	g_debug( "%s: window=%p, uri=%s, msg=%p", thisfn, ( void * ) window, uri, ( void * ) msg );
	g_return_val_if_fail( BASE_IS_WINDOW( window ), NULL );

	reader = gconf_reader_new();
	reader->private->window = window;
	reader->private->import_mode = import_mode;
	reader->private->uri = uri;
	reader->private->auxiliaries = items;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), NULL );

	doc = xmlParseFile( uri );

	if( !doc ){
		xmlErrorPtr error = xmlGetLastError();
		add_message( reader,
				ERR_UNABLE_PARSE_XML_FILE, error->message );
		xmlResetError( error );

	} else {
		xmlNode *root_node = xmlDocGetRootElement( doc );

		if( strxcmp( root_node->name, NACT_GCONF_SCHEMA_ROOT ) &&
			strxcmp( root_node->name, NACT_GCONF_DUMP_ROOT )){
				add_message( reader,
						ERR_ROOT_ELEMENT,
						NACT_GCONF_SCHEMA_ROOT, NACT_GCONF_DUMP_ROOT, ( const char * ) root_node->name, root_node->line );

		} else if( !strxcmp( root_node->name, NACT_GCONF_SCHEMA_ROOT )){
			gconf_reader_parse_schema_root( reader, root_node );

		} else {
			g_assert( !strxcmp( root_node->name, NACT_GCONF_DUMP_ROOT ));
			gconf_reader_parse_dump_root( reader, root_node );
		}

		xmlFreeDoc (doc);
	}

	xmlCleanupParser();

	if( reader->private->action ){
		g_assert( NA_IS_OBJECT_ACTION( reader->private->action ));
		if( manage_import_mode( reader )){
			action = g_object_ref( reader->private->action );
		}
	}

	*msg = na_utils_duplicate_string_list( reader->private->messages );
	g_object_unref( reader );

	return( action );
}

static void
gconf_reader_parse_schema_root( NactXMLReader *reader, xmlNode *root )
{
	static const gchar *thisfn = "gconf_reader_parse_schema_root";
	xmlNodePtr iter;
	gboolean found = FALSE;

	g_debug( "%s: reader=%p, root=%p", thisfn, ( void * ) reader, ( void * ) root );

	for( iter = root->children ; iter ; iter = iter->next ){

		if( iter->type != XML_ELEMENT_NODE ){
			continue;
		}

		if( strxcmp( iter->name, NACT_GCONF_SCHEMA_LIST )){
			add_message( reader,
					ERR_WAITED_IGNORED_NODE,
					NACT_GCONF_SCHEMA_LIST, ( const char * ) iter->name, iter->line );
			continue;
		}

		if( found ){
			add_message( reader, ERR_IGNORED_NODE, ( const char * ) iter->name, iter->line );
			continue;
		}

		found = TRUE;
		gconf_reader_parse_schemalist( reader, iter );
	}
}

/*
 * iter points to the 'schemalist' node (already checked)
 * children should only be 'schema' nodes ; other nodes are warned,
 * but not fatal
 */
static void
gconf_reader_parse_schemalist( NactXMLReader *reader, xmlNode *schema )
{
	static const gchar *thisfn = "gconf_reader_parse_schemalist";
	xmlNode *iter;
	gboolean ok = TRUE;

	g_debug( "%s: reader=%p, schema=%p", thisfn, ( void * ) reader, ( void * ) schema );

	reader->private->action = na_object_action_new();
	reader->private->uuid_set = FALSE;

	for( iter = schema->children ; iter ; iter = iter->next ){

		if( iter->type != XML_ELEMENT_NODE ){
			continue;
		}

		if( strxcmp( iter->name, NACT_GCONF_SCHEMA_ENTRY )){
			add_message( reader,
					ERR_WAITED_IGNORED_NODE,
					NACT_GCONF_SCHEMA_ENTRY, ( const char * ) iter->name, iter->line );
			continue;
		}

		if( !gconf_reader_parse_schema( reader, iter )){
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
gconf_reader_parse_schema( NactXMLReader *reader, xmlNode *schema )
{
	static const gchar *thisfn = "gconf_reader_parse_schema";
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

		if( strxcmp( iter->name, NACT_GCONF_SCHEMA_KEY ) &&
			strxcmp( iter->name, NACT_GCONF_SCHEMA_APPLYTO ) &&
			strxcmp( iter->name, NACT_GCONF_SCHEMA_OWNER ) &&
			strxcmp( iter->name, NACT_GCONF_SCHEMA_TYPE ) &&
			strxcmp( iter->name, NACT_GCONF_SCHEMA_LIST_TYPE ) &&
			strxcmp( iter->name, NACT_GCONF_SCHEMA_LOCALE ) &&
			strxcmp( iter->name, NACT_GCONF_SCHEMA_DEFAULT )){

				add_message( reader, ERR_UNEXPECTED_NODE, ( const char * ) iter->name, iter->line );
				ret = FALSE;
				continue;
		}

		if( !strxcmp( iter->name, NACT_GCONF_SCHEMA_KEY ) ||
			!strxcmp( iter->name, NACT_GCONF_SCHEMA_OWNER ) ||
			!strxcmp( iter->name, NACT_GCONF_SCHEMA_TYPE ) ||
			!strxcmp( iter->name, NACT_GCONF_SCHEMA_LIST_TYPE )){

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

		if( !strxcmp( iter->name, NACT_GCONF_SCHEMA_APPLYTO )){

			if( applyto ){
				add_message( reader, ERR_UNEXPECTED_NODE, ( const char * ) iter->name, iter->line );
				ret = FALSE;
			}

			applyto = TRUE;
			ret = gconf_reader_parse_applyto( reader, iter );
		}
	}

	if( !applyto ){
		g_assert( ret );
		add_message( reader, ERR_NODE_NOT_FOUND, NACT_GCONF_SCHEMA_APPLYTO );
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

		if( !strxcmp( iter->name, NACT_GCONF_SCHEMA_LOCALE )){

			if( locale_found ){
				add_message( reader, ERR_UNEXPECTED_NODE, ( const char * ) iter->name, iter->line );
				ret = FALSE;

			} else {
				locale_found = TRUE;
				if( reader->private->locale_waited ){
					ret = gconf_reader_parse_locale( reader, iter );
				}
			}

		} else if( !strxcmp( iter->name, NACT_GCONF_SCHEMA_DEFAULT )){

			if( default_found ){
				add_message( reader, ERR_UNEXPECTED_NODE, ( const char * ) iter->name, iter->line );
				ret = FALSE;

			} else {
				default_found = TRUE;
				if( !reader->private->locale_waited ||
						( pre_v1_11 && !strcmp( reader->private->entry, ACTION_PROFILE_LABEL_ENTRY ))){
					gconf_reader_parse_default( reader, iter );
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
gconf_reader_parse_applyto( NactXMLReader *reader, xmlNode *node )
{
	static const gchar *thisfn = "gconf_reader_parse_applyto";
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

		ret = gconf_reader_check_for_entry( reader, node, entry );
	}

	g_free( entry );
	g_free( profile );
	g_free( uuid );
	xmlFree( text );

	return( ret );
}

static gboolean
gconf_reader_check_for_entry( NactXMLReader *reader, xmlNode *node, const char *entry )
{
	static const gchar *thisfn = "gconf_reader_check_for_entry";
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
gconf_reader_parse_locale( NactXMLReader *reader, xmlNode *locale )
{
	static const gchar *thisfn = "gconf_reader_parse_locale";
	gboolean ret = TRUE;
	xmlNode *iter;
	gboolean default_found = FALSE;

	g_debug( "%s: reader=%p, locale=%p", thisfn, ( void * ) reader, ( void * ) locale );

	for( iter = locale->children ; iter ; iter = iter->next ){

		if( iter->type != XML_ELEMENT_NODE ){
			continue;
		}

		if( strxcmp( iter->name, NACT_GCONF_SCHEMA_SHORT ) &&
			strxcmp( iter->name, NACT_GCONF_SCHEMA_LONG ) &&
			strxcmp( iter->name, NACT_GCONF_SCHEMA_DEFAULT )){

				add_message( reader, ERR_UNEXPECTED_NODE, ( const char * ) iter->name, iter->line );
				ret = FALSE;
				continue;
		}

		if( !strxcmp( iter->name, NACT_GCONF_SCHEMA_SHORT ) ||
			!strxcmp( iter->name, NACT_GCONF_SCHEMA_LONG )){
				continue;
		}

		if( default_found ){
			add_message( reader, ERR_UNEXPECTED_NODE, ( const char * ) iter->name, iter->line );
			ret = FALSE;
			continue;
		}

		g_assert( ret );
		default_found = TRUE;
		gconf_reader_parse_default( reader, iter );
	}

	return( ret );
}

static void
gconf_reader_parse_default( NactXMLReader *reader, xmlNode *node )
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
	/*g_debug( "gconf_reader_parse_default: set value=%s", reader->private->value );*/
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

static void
gconf_reader_parse_dump_root( NactXMLReader *reader, xmlNode *root )
{
	static const gchar *thisfn = "gconf_reader_parse_dump_root";
	xmlNodePtr iter;
	gboolean found = FALSE;

	g_debug( "%s: reader=%p, root=%p", thisfn, ( void * ) reader, ( void * ) root );

	for( iter = root->children ; iter ; iter = iter->next ){

		if( iter->type != XML_ELEMENT_NODE ){
			continue;
		}
		if( strxcmp( iter->name, NACT_GCONF_DUMP_ENTRYLIST )){
			add_message( reader,
					ERR_WAITED_IGNORED_NODE,
					NACT_GCONF_DUMP_ENTRYLIST, ( const char * ) iter->name, iter->line );
			continue;
		}
		if( found ){
			add_message( reader, ERR_IGNORED_NODE, ( const char * ) iter->name, iter->line );
			continue;
		}

		found = TRUE;
		gconf_reader_parse_entrylist( reader, iter );
	}
}

/*
 * iter points to the 'entrylist' node (already checked)
 * children should only be 'entry' nodes ; other nodes are warned,
 * but not fatal
 */
static void
gconf_reader_parse_entrylist( NactXMLReader *reader, xmlNode *entrylist )
{
	static const gchar *thisfn = "gconf_reader_parse_entrylist";
	xmlChar *path;
	gchar *uuid, *label;
	gboolean ok;

	g_debug( "%s: reader=%p, entrylist=%p", thisfn, ( void * ) reader, ( void * ) entrylist );

	reader->private->action = na_object_action_new();
	path = xmlGetProp( entrylist, ( const xmlChar * ) NACT_GCONF_DUMP_ENTRYLIST_BASE );
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

			if( strxcmp( iter->name, NACT_GCONF_DUMP_ENTRY )){
				add_message( reader,
						ERR_WAITED_IGNORED_NODE,
						NACT_GCONF_DUMP_ENTRY, ( const char * ) iter->name, iter->line );
				continue;
			}

			if( !gconf_reader_parse_entry( reader, iter )){
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
gconf_reader_parse_entry( NactXMLReader *reader, xmlNode *entry )
{
	static const gchar *thisfn = "gconf_reader_parse_entry";
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
		if( strxcmp( iter->name, NACT_GCONF_DUMP_KEY ) &&
			strxcmp( iter->name, NACT_GCONF_DUMP_VALUE )){

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
		if( !strxcmp( iter->name, NACT_GCONF_DUMP_KEY )){

			if( key_found ){
				add_message( reader, ERR_UNEXPECTED_NODE, ( const char * ) iter->name, iter->line );
				ret = FALSE;

			} else {
				key_found = TRUE;
				ret = gconf_reader_parse_dump_key( reader, iter );
			}
		}
	}

	if( !key_found ){
		g_assert( ret );
		add_message( reader, ERR_NODE_NOT_FOUND, NACT_GCONF_DUMP_KEY );
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
		if( !strxcmp( iter->name, NACT_GCONF_DUMP_VALUE )){

			if( value_found ){
				add_message( reader, ERR_UNEXPECTED_NODE, ( const char * ) iter->name, iter->line );
				ret = FALSE;

			} else {
				value_found = TRUE;
				gconf_reader_parse_dump_value( reader, iter );
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
gconf_reader_parse_dump_key( NactXMLReader *reader, xmlNode *node )
{
	static const gchar *thisfn = "gconf_reader_parse_dump_key";
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

		ret = gconf_reader_check_for_entry( reader, node, entry );
	}

	g_free( entry );
	g_free( profile );
	xmlFree( text );

	return( ret );
}

static void
gconf_reader_parse_dump_value( NactXMLReader *reader, xmlNode *node )
{
	xmlNode *iter;
	for( iter = node->children ; iter ; iter = iter->next ){

		if( iter->type != XML_ELEMENT_NODE ){
			continue;
		}
		if( strxcmp( iter->name, NACT_GCONF_DUMP_LIST ) &&
			strxcmp( iter->name, NACT_GCONF_DUMP_STRING )){
				add_message( reader,
					ERR_IGNORED_NODE, ( const char * ) iter->name, iter->line );
			continue;
		}
		if( !strxcmp( iter->name, NACT_GCONF_DUMP_STRING )){

			xmlChar *text = xmlNodeGetContent( iter );

			if( reader->private->list_waited ){
				reader->private->list_value = g_slist_append( reader->private->list_value, g_strdup(( const gchar * ) text ));
			} else {
				reader->private->value = g_strdup(( const gchar * ) text );
			}

			xmlFree( text );
			continue;
		}
		gconf_reader_parse_dump_value_list( reader, iter );
	}
}

static void
gconf_reader_parse_dump_value_list( NactXMLReader *reader, xmlNode *list_node )
{
	xmlNode *iter;
	for( iter = list_node->children ; iter ; iter = iter->next ){

		if( iter->type != XML_ELEMENT_NODE ){
			continue;
		}
		if( strxcmp( iter->name, NACT_GCONF_DUMP_VALUE )){
				add_message( reader,
					ERR_IGNORED_NODE, ( const char * ) iter->name, iter->line );
			continue;
		}
		gconf_reader_parse_dump_value( reader, iter );
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
apply_values( NactXMLReader *reader )
{
	static const gchar *thisfn = "nact_xml_reader_apply_values";

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
			na_object_action_toolbar_set_same_label( reader->private->action, na_utils_schema_to_boolean( reader->private->value, TRUE ));

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

static void
add_message( NactXMLReader *reader, const gchar *format, ... )
{
	va_list va;
	gchar *tmp;

	g_debug( "nact_xml_reader_add_message: format=%s", format );

	va_start( va, format );
	tmp = g_markup_vprintf_escaped( format, va );
	va_end( va );
	reader->private->messages = g_slist_append( reader->private->messages, tmp );
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

static gchar *
get_uuid_from_key( NactXMLReader *reader, const gchar *key, guint line )
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

static void
free_reader_values( NactXMLReader *reader )
{
	int i;

	reader->private->profile = NULL;
	reader->private->locale_waited = FALSE;

	g_free( reader->private->entry );
	reader->private->entry = NULL;

	g_free( reader->private->value );
	reader->private->value = NULL;

	na_utils_free_string_list( reader->private->list_value );
	reader->private->list_value = NULL;

	for( i=0 ; reader_str[i].entry ; ++i ){
		reader_str[i].entry_found = FALSE;
	}
}

/*
 * returns TRUE if we can safely insert the action
 * - the uuid doesn't already exist
 * - the uuid already exist, but import mode is renumber
 * - the uuid already exists, but import mode is override
 */
static gboolean
manage_import_mode( NactXMLReader *reader )
{
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
}

static NAObjectItem *
search_in_auxiliaries( NactXMLReader *reader, const gchar *uuid )
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
relabel( NactXMLReader *reader )
{
	gchar *label, *tmp;

	label = na_object_get_label( reader->private->action );

	/* i18n: the action has been renumbered during import operation */
	tmp = g_strdup_printf( "%s %s", label, _( "(renumbered)" ));

	na_object_set_label( reader->private->action, tmp );

	g_free( tmp );
	g_free( label );
}
