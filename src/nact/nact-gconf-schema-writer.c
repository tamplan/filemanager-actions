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

#include <libxml/tree.h>

#include "nact-gconf-schema-writer.h"

/* private class data
 */
struct NactGConfSchemaWriterClassPrivate {
};

/* private instance data
 */
struct NactGConfSchemaWriterPrivate {
	gboolean  dispose_has_run;
	gchar    *uuid;
};

static GObjectClass *st_parent_class = NULL;

static GType                  register_type( void );
static void                   class_init( NactGConfSchemaWriterClass *klass );
static void                   instance_init( GTypeInstance *instance, gpointer klass );
static void                   instance_dispose( GObject *object );
static void                   instance_finalize( GObject *object );

static NactGConfSchemaWriter *gconf_schema_writer_new( void );
static xmlDocPtr              create_xml( NactGConfSchemaWriter *writer, NAAction *action );
static void                   create_schema_entry(
										NactGConfSchemaWriter *writer,
										const gchar *profile_name,
										const gchar *key,
										const gchar *value,
										xmlDocPtr doc,
										xmlNodePtr list_node,
										const gchar *type,
										gboolean is_l10n_value );

GType
nact_gconf_schema_writer_get_type( void )
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
	static GTypeInfo info = {
		sizeof( NactGConfSchemaWriterClass ),
		NULL,
		NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactGConfSchemaWriter ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	GType type = g_type_register_static( G_TYPE_OBJECT, "NactGConfSchemaWriter", &info, 0 );

	return( type );
}

static void
class_init( NactGConfSchemaWriterClass *klass )
{
	static const gchar *thisfn = "nact_gconf_schema_writer_class_init";
	g_debug( "%s: klass=%p", thisfn, klass );

	st_parent_class = g_type_class_peek_parent( klass );

	GObjectClass *object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactGConfSchemaWriterClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_gconf_schema_writer_instance_init";
	g_debug( "%s: instance=%p, klass=%p", thisfn, instance, klass );

	g_assert( NACT_IS_GCONF_SCHEMA_WRITER( instance ));
	NactGConfSchemaWriter *self = NACT_GCONF_SCHEMA_WRITER( instance );

	self->private = g_new0( NactGConfSchemaWriterPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *object )
{
	g_assert( NACT_IS_GCONF_SCHEMA_WRITER( object ));
	NactGConfSchemaWriter *self = NACT_GCONF_SCHEMA_WRITER( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		G_OBJECT_CLASS( st_parent_class )->dispose( object );
	}
}

static void
instance_finalize( GObject *object )
{
	g_assert( NACT_IS_GCONF_SCHEMA_WRITER( object ));
	NactGConfSchemaWriter *self = NACT_GCONF_SCHEMA_WRITER( object );

	g_free( self->private->uuid );

	g_free( self->private );

	/* chain call to parent class */
	if( st_parent_class->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

static NactGConfSchemaWriter *
gconf_schema_writer_new( void )
{
	return( g_object_new( NACT_GCONF_SCHEMA_WRITER_TYPE, NULL ));
}

static void
gconf_schema_writer_set_uuid( NactGConfSchemaWriter *writer, const gchar *uuid )
{
	g_free( writer->private->uuid );
	writer->private->uuid = g_strdup( uuid );
}

/**
 * Export the specified action as a GConf schema.
 */
void
nact_gconf_schema_writer_export( NAAction *action, const gchar *folder, gchar **msg )
{
	NactGConfSchemaWriter *writer = gconf_schema_writer_new();

	gchar *uuid = na_action_get_uuid( action );
	gconf_schema_writer_set_uuid( writer, uuid );
	g_free( uuid );

	xmlDocPtr doc = create_xml( writer, action );

	/* generate the filename name and save the schema into it */
	gchar *filename = g_strdup_printf( "%s/%s.schema", folder, writer->private->uuid );
	xmlSaveFormatFileEnc( filename, doc, "UTF-8", 1 );
	g_free( filename );

	xmlFreeDoc (doc);
	xmlCleanupParser();
}

static xmlDocPtr
create_xml( NactGConfSchemaWriter *writer, NAAction *action )
{
	xmlDocPtr doc = xmlNewDoc( BAD_CAST( "1.0" ));
	xmlNodePtr root_node = xmlNewNode( NULL, BAD_CAST( NACT_GCONF_XML_ROOT ));
	xmlDocSetRootElement( doc, root_node );
	xmlNodePtr list_node = xmlNewChild( root_node, NULL, BAD_CAST( NACT_GCONF_XML_SCHEMA_LIST ), NULL );

	/* version */
	gchar *version = na_action_get_version( action );
	create_schema_entry( writer, NULL, ACTION_VERSION_ENTRY, version, doc, list_node, "string", FALSE );
	g_free( version );

	/* label */
	gchar *label = na_action_get_label( action );
	create_schema_entry( writer, NULL, ACTION_LABEL_ENTRY, label, doc, list_node, "string", TRUE );
	g_free( label );

	/* tooltip */
	gchar *tooltip = na_action_get_tooltip( action );
	create_schema_entry( writer, NULL, ACTION_TOOLTIP_ENTRY, tooltip, doc, list_node, "string", TRUE );
	g_free( tooltip );

	/* icon name */
	gchar *icon = na_action_get_icon( action );
	create_schema_entry( writer, NULL, ACTION_ICON_ENTRY, icon, doc, list_node, "string", FALSE );
	g_free( icon );

	GSList *profiles = na_action_get_profiles( action );
	GSList *ip;

	for( ip = profiles ; ip ; ip = ip->next ){

		NAActionProfile *profile = NA_ACTION_PROFILE( ip->data );
		gchar *profile_dir = na_action_profile_get_name( profile );

		/* profile label */
		gchar *profile_label = na_action_profile_get_label( profile );
		create_schema_entry( writer, profile_dir, ACTION_PROFILE_LABEL_ENTRY, profile_label, doc, list_node, "string", TRUE );
		g_free( profile_label );

		/* path */
		gchar *path = na_action_profile_get_path( profile );
		create_schema_entry( writer, profile_dir, ACTION_PATH_ENTRY, path, doc, list_node, "string", FALSE );
		g_free( path );

		/* parameters */
		gchar *parameters = na_action_profile_get_parameters( profile );
		create_schema_entry( writer, profile_dir, ACTION_PARAMETERS_ENTRY, parameters, doc, list_node, "string", FALSE );
		g_free( parameters );

		/* basenames */
		GSList *basenames = na_action_profile_get_basenames( profile );
		gchar *text = na_utils_gslist_to_schema( basenames );
		create_schema_entry( writer, profile_dir, ACTION_BASENAMES_ENTRY, text, doc, list_node, "list", FALSE );
		g_free( text );
		na_utils_free_string_list( basenames );

		/* match_case */
		gboolean match = na_action_profile_get_matchcase( profile );
		text = na_utils_boolean_to_schema( match );
		create_schema_entry( writer, profile_dir, ACTION_MATCHCASE_ENTRY, text, doc, list_node, "bool", FALSE );
		g_free( text );

		/* mimetypes */
		GSList *mimetypes = na_action_profile_get_mimetypes( profile );
		text = na_utils_gslist_to_schema( mimetypes );
		create_schema_entry( writer, profile_dir, ACTION_MIMETYPES_ENTRY, text, doc, list_node, "list", FALSE );
		g_free( text );
		na_utils_free_string_list( mimetypes );

		/* is_file */
		gboolean isfile = na_action_profile_get_is_file( profile );
		text = na_utils_boolean_to_schema( isfile );
		create_schema_entry( writer, profile_dir, ACTION_ISFILE_ENTRY, text, doc, list_node, "bool", FALSE );
		g_free( text );

		/* is_dir */
		gboolean isdir = na_action_profile_get_is_dir( profile );
		text = na_utils_boolean_to_schema( isdir );
		create_schema_entry( writer, profile_dir, ACTION_ISDIR_ENTRY, text, doc, list_node, "bool", FALSE );
		g_free( text );

		/* accept-multiple-files */
		gboolean mutiple = na_action_profile_get_multiple( profile );
		text = na_utils_boolean_to_schema( mutiple );
		create_schema_entry( writer, profile_dir, ACTION_MULTIPLE_ENTRY, text, doc, list_node, "bool", FALSE );
		g_free( text );

		/* schemes */
		GSList *schemes = na_action_profile_get_schemes( profile );
		text = na_utils_gslist_to_schema( schemes );
		create_schema_entry( writer, profile_dir, ACTION_SCHEMES_ENTRY, text, doc, list_node, "list", FALSE );
		g_free( text );
		na_utils_free_string_list( schemes );

		g_free( profile_dir );
	}

	return( doc );
}

static void
create_schema_entry( NactGConfSchemaWriter *writer,
		const gchar *profile_name, const gchar *key, const gchar *value,
		xmlDocPtr doc, xmlNodePtr list_node, const gchar *type, gboolean is_l10n_value )
{
	gchar *path = NULL;
	if( profile_name ){
		path = g_build_path( "/", NA_GCONF_CONFIG_PATH, writer->private->uuid, profile_name, key, NULL );
	} else {
		path = g_build_path( "/", NA_GCONF_CONFIG_PATH, writer->private->uuid, key, NULL );
	}

	xmlNodePtr schema_node = xmlNewChild( list_node, NULL, BAD_CAST( NACT_GCONF_XML_SCHEMA_ENTRY ), NULL );

	xmlChar *content = BAD_CAST( g_build_path( "/", NACT_GCONF_SCHEMA_PREFIX, path, NULL ));
	xmlNewChild( schema_node, NULL, BAD_CAST( NACT_GCONF_XML_SCHEMA_KEY ), content );
	xmlFree( content );

	xmlNewChild( schema_node, NULL, BAD_CAST( NACT_GCONF_XML_SCHEMA_APPLYTO ), BAD_CAST( path ));

	xmlNewChild( schema_node, NULL, BAD_CAST( NACT_GCONF_XML_SCHEMA_TYPE ), BAD_CAST( type ));

	if( !g_ascii_strcasecmp( type, "list" )){
		xmlNewChild( schema_node, NULL, BAD_CAST( NACT_GCONF_XML_SCHEMA_LIST_TYPE ), BAD_CAST( "string" ));
	}

	/* if the default value must be localized, put it in the <locale> element
	 */
	xmlNodePtr value_root_node = schema_node;
	if( is_l10n_value ){
		xmlNodePtr locale_node = xmlNewChild( schema_node, NULL, BAD_CAST( NACT_GCONF_XML_SCHEMA_LOCALE ), NULL );
		xmlNewProp( locale_node, BAD_CAST( "name" ), BAD_CAST( "C" ));
		value_root_node = locale_node;
	}

	/* encode special chars <, >, &, ...
	 */
	xmlChar *encoded_content = xmlEncodeSpecialChars( doc, BAD_CAST( value ));
	xmlNewChild( value_root_node, NULL, BAD_CAST( NACT_GCONF_XML_SCHEMA_DFT ), encoded_content );
	xmlFree( encoded_content );

	g_free( path );
}
