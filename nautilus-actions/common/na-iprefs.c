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

#include "na-iprefs.h"

#define DEFAULT_IMPORT_MODE_INT				IPREFS_IMPORT_NO_IMPORT
#define DEFAULT_IMPORT_MODE_STR				"NoImport"

static GConfEnumStringPair import_mode_table[] = {
	{ IPREFS_IMPORT_NO_IMPORT,				DEFAULT_IMPORT_MODE_STR },
	{ IPREFS_IMPORT_RENUMBER,				"Renumber" },
	{ IPREFS_IMPORT_OVERRIDE,				"Override" },
	{ IPREFS_IMPORT_ASK,					"Ask" },
	{ 0, NULL }
};

#define DEFAULT_EXPORT_FORMAT_INT			IPREFS_EXPORT_FORMAT_GCONF_ENTRY
#define DEFAULT_EXPORT_FORMAT_STR			"GConfEntry"

static GConfEnumStringPair export_format_table[] = {
	{ IPREFS_EXPORT_FORMAT_GCONF_SCHEMA_V1,	"GConfSchemaV1" },
	{ IPREFS_EXPORT_FORMAT_GCONF_SCHEMA_V2,	"GConfSchemaV2" },
	{ IPREFS_EXPORT_FORMAT_GCONF_ENTRY,		DEFAULT_EXPORT_FORMAT_STR },
	{ IPREFS_EXPORT_FORMAT_ASK,				"Ask" },
	{ 0, NULL }
};

static GConfValue *get_value( GConfClient *client, const gchar *path, const gchar *entry );
static void        set_value( GConfClient *client, const gchar *path, const gchar *entry, GConfValue *value );

/**
 * na_iprefs_migrate_key:
 * @instance: the #NAIPrefs implementor.
 * @old_key: the old preference entry.
 * @new_key: the new preference entry.
 *
 * Migrates the content of an entry from an obsoleted key to a new one.
 * Removes the old key, along with the schema associated to it,
 * considering that the version which asks for this migration has
 * installed a schema corresponding to the new key.
 */
void
na_iprefs_migrate_key( NAIPrefs *instance, const gchar *old_key, const gchar *new_key )
{
	static const gchar *thisfn = "na_iprefs_migrate_key";
	GConfValue *value;

	g_debug( "%s: instance=%p, old_key=%s, new_key=%s", thisfn, ( void * ) instance, old_key, new_key );
	g_return_if_fail( NA_IS_IPREFS( instance ));

	value = get_value( na_iprefs_get_gconf_client( instance ), NA_GCONF_PREFS_PATH, new_key );
	if( !value ){
		value = get_value( na_iprefs_get_gconf_client( instance ), NA_GCONF_PREFS_PATH, old_key );
		if( value ){
			set_value( na_iprefs_get_gconf_client( instance ), NA_GCONF_PREFS_PATH, new_key, value );
			gconf_value_free( value );
		}
	}

	/* do not remove entries which may be always used by another,
	 * while older, version of NACT
	 */
	/*remove_entry( BASE_IPREFS_GET_INTERFACE( window )->private->client, NA_GCONF_PREFS_PATH, old_key );*/
	/*remove_entry( BASE_IPREFS_GET_INTERFACE( window )->private->client, BASE_IPREFS_SCHEMAS_PATH, old_key );*/
}

/**
 * na_iprefs_get_export_format:
 * @instance: this #NAIPrefs interface instance.
 * @name: name of the export format key to be readen
 *
 * Returns: the export format currently set.
 *
 * Note: this function returns a suitable default value even if the key
 * is not found in GConf preferences or no schema has been installed.
 *
 * Note: please take care of keeping the default value synchronized with
 * those defined in schemas.
 */
gint
na_iprefs_get_export_format( NAIPrefs *instance, const gchar *name )
{
	gint export_format = DEFAULT_EXPORT_FORMAT_INT;
	gint format_int;
	gchar *format_str;

	g_return_val_if_fail( NA_IS_IPREFS( instance ), DEFAULT_EXPORT_FORMAT_INT );

	format_str = na_iprefs_read_string(
			instance,
			name,
			DEFAULT_EXPORT_FORMAT_STR );

	if( gconf_string_to_enum( export_format_table, format_str, &format_int )){
		export_format = format_int;
	}

	g_free( format_str );

	return( export_format );
}

/**
 * na_iprefs_get_import_mode:
 * @instance: this #NAIPrefs interface instance.
 * @name: name of the import key to be readen
 *
 * Returns: the import mode currently set.
 *
 * Note: this function returns a suitable default value even if the key
 * is not found in GConf preferences or no schema has been installed.
 *
 * Note: please take care of keeping the default value synchronized with
 * those defined in schemas.
 */
gint
na_iprefs_get_import_mode( NAIPrefs *instance, const gchar *name )
{
	gint import_mode = DEFAULT_IMPORT_MODE_INT;
	gint import_int;
	gchar *import_str;

	g_return_val_if_fail( NA_IS_IPREFS( instance ), DEFAULT_IMPORT_MODE_INT );

	import_str = na_iprefs_read_string(
			instance,
			name,
			DEFAULT_IMPORT_MODE_STR );

	if( gconf_string_to_enum( import_mode_table, import_str, &import_int )){
		import_mode = import_int;
	}

	g_free( import_str );

	return( import_mode );
}

/**
 * na_iprefs_set_export_format:
 * @instance: this #NAIPrefs interface instance.
 * @format: the new value to be written.
 *
 * Writes the current status of 'import/export format' to the GConf
 * preference system.
 */
void
na_iprefs_set_export_format( NAIPrefs *instance, const gchar *name, gint format )
{
	const gchar *format_str;

	g_return_if_fail( NA_IS_IPREFS( instance ));

	format_str = gconf_enum_to_string( export_format_table, format );

	na_iprefs_write_string(
			instance,
			name,
			format_str ? format_str : DEFAULT_EXPORT_FORMAT_STR );
}

/**
 * na_iprefs_set_import_mode:
 * @instance: this #NAIPrefs interface instance.
 * @mode: the new value to be written.
 *
 * Writes the current status of 'import mode' to the GConf
 * preference system.
 */
void
na_iprefs_set_import_mode( NAIPrefs *instance, const gchar *name, gint mode )
{
	const gchar *import_str;

	g_return_if_fail( NA_IS_IPREFS( instance ));

	import_str = gconf_enum_to_string( import_mode_table, mode );

	na_iprefs_write_string(
			instance,
			name,
			import_str ? import_str : DEFAULT_IMPORT_MODE_STR );
}

static GConfValue *
get_value( GConfClient *client, const gchar *path, const gchar *entry )
{
	static const gchar *thisfn = "na_iprefs_get_value";
	GError *error = NULL;
	gchar *fullpath;
	GConfValue *value;

	fullpath = gconf_concat_dir_and_key( path, entry );

	value = gconf_client_get_without_default( client, fullpath, &error );

	if( error ){
		g_warning( "%s: key=%s, %s", thisfn, fullpath, error->message );
		g_error_free( error );
		if( value ){
			gconf_value_free( value );
			value = NULL;
		}
	}

	g_free( fullpath );

	return( value );
}

static void
set_value( GConfClient *client, const gchar *path, const gchar *entry, GConfValue *value )
{
	static const gchar *thisfn = "na_iprefs_set_value";
	GError *error = NULL;
	gchar *fullpath;

	g_return_if_fail( value );

	fullpath = gconf_concat_dir_and_key( path, entry );

	gconf_client_set( client, fullpath, value, &error );

	if( error ){
		g_warning( "%s: key=%s, %s", thisfn, fullpath, error->message );
		g_error_free( error );
	}

	g_free( fullpath );
}
