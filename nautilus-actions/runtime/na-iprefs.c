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

#include "na-gconf-utils.h"
#include "na-iprefs.h"

/* private interface data
 */
struct NAIPrefsInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private data initialized the first time an instance calls a function
 * of the public api
 * it is set as a data association against the calling GObject
 */
typedef struct {
	GConfClient *client;
}
	NAIPrefsPrivate;

#define DEFAULT_ORDER_MODE_INT			IPREFS_ORDER_ALPHA_ASCENDING
#define DEFAULT_ORDER_MODE_STR			"AscendingOrder"

static GConfEnumStringPair order_mode_table[] = {
	{ IPREFS_ORDER_ALPHA_ASCENDING ,	"AscendingOrder" },
	{ IPREFS_ORDER_ALPHA_DESCENDING,	"DescendingOrder" },
	{ IPREFS_ORDER_MANUAL          ,	"ManualOrder" },
	{ 0, NULL }
};

#define NA_IPREFS_PRIVATE_DATA			"na-runtime-iprefs-private-data"

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

static gboolean st_initialized = FALSE;
static gboolean st_finalized = FALSE;

static GType       register_type( void );
static void        interface_base_init( NAIPrefsInterface *klass );
static void        interface_base_finalize( NAIPrefsInterface *klass );

static void        setup_private_data( NAIPrefs *instance );
static GConfValue *get_value( GConfClient *client, const gchar *path, const gchar *entry );
static void        set_value( GConfClient *client, const gchar *path, const gchar *entry, GConfValue *value );

GType
na_iprefs_get_type( void )
{
	static GType iface_type = 0;

	if( !iface_type ){
		iface_type = register_type();
	}

	return( iface_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "na_iprefs_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NAIPrefsInterface ),
		( GBaseInitFunc ) interface_base_init,
		( GBaseFinalizeFunc ) interface_base_finalize,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_INTERFACE, "NAIPrefs", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( NAIPrefsInterface *klass )
{
	static const gchar *thisfn = "na_iprefs_interface_base_init";

	if( !st_initialized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NAIPrefsInterfacePrivate, 1 );

		st_initialized = TRUE;
	}
}

static void
interface_base_finalize( NAIPrefsInterface *klass )
{
	static const gchar *thisfn = "na_iprefs_interface_base_finalize";

	if( !st_finalized ){

		st_finalized = TRUE;

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );
	}
}

/**
 * na_iprefs_get_level_zero_items:
 * @instance: this #NAIPrefs interface instance.
 *
 * Returns: the ordered list of UUID's of items which are to be
 * displayed at level zero of the hierarchy.
 *
 * The returned list should be na_utils_free_string_list() by the caller.
 */
GSList *
na_iprefs_get_level_zero_items( NAIPrefs *instance )
{
	static const gchar *thisfn = "na_iprefs_get_level_zero_items";
	GSList *level_zero = NULL;

	g_debug( "%s: instance=%p (%s)", thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));
	g_return_val_if_fail( NA_IS_IPREFS( instance ), NULL );

	if( st_initialized && !st_finalized ){

		level_zero = na_iprefs_read_string_list( instance, IPREFS_LEVEL_ZERO_ITEMS, NULL );
	}

	return( level_zero );
}

/**
 * na_iprefs_set_level_zero_items:
 * @instance: this #NAIPrefs interface instance.
 * @order: the ordered #GSList of item UUIDs.
 *
 * Writes the order and the content of the level-zero UUID's.
 */
void
na_iprefs_set_level_zero_items( NAIPrefs *instance, GSList *order )
{
	g_return_if_fail( NA_IS_IPREFS( instance ));

	if( st_initialized && !st_finalized ){

		na_iprefs_write_string_list( instance, IPREFS_LEVEL_ZERO_ITEMS, order );
	}
}

/**
 * na_iprefs_get_order_mode:
 * @instance: this #NAIPrefs interface instance.
 *
 * Returns: the order mode currently set.
 *
 * Note: this function returns a suitable default value even if the key
 * is not found in GConf preferences or no schema has been installed.
 *
 * Note: please take care of keeping the default value synchronized with
 * those defined in schemas.
 */
gint
na_iprefs_get_order_mode( NAIPrefs *instance )
{
	gint alpha_order = DEFAULT_ORDER_MODE_INT;
	gint order_int;
	gchar *order_str;

	g_return_val_if_fail( NA_IS_IPREFS( instance ), DEFAULT_ORDER_MODE_INT );

	if( st_initialized && !st_finalized ){

		order_str = na_iprefs_read_string(
				instance,
				IPREFS_DISPLAY_ALPHABETICAL_ORDER,
				DEFAULT_ORDER_MODE_STR );

		if( gconf_string_to_enum( order_mode_table, order_str, &order_int )){
			alpha_order = order_int;
		}

		g_free( order_str );
	}

	return( alpha_order );
}

/**
 * na_iprefs_set_order_mode:
 * @instance: this #NAIPrefs interface instance.
 * @mode: the new value to be written.
 *
 * Writes the current status of 'alphabetical order' to the GConf
 * preference system.
 */
void
na_iprefs_set_order_mode( NAIPrefs *instance, gint mode )
{
	const gchar *order_str;

	g_return_if_fail( NA_IS_IPREFS( instance ));

	if( st_initialized && !st_finalized ){

		order_str = gconf_enum_to_string( order_mode_table, mode );

		na_iprefs_write_string(
				instance,
				IPREFS_DISPLAY_ALPHABETICAL_ORDER,
				order_str ? order_str : DEFAULT_ORDER_MODE_STR );
	}
}

/**
 * na_iprefs_should_add_about_item:
 * @instance: this #NAIPrefs interface instance.
 *
 * Returns: #TRUE if an "About Nautilus Actions" item should be added to
 * the first level of Nautilus context submenus (if any), #FALSE else.
 *
 * Note: this function returns a suitable default value if the key is
 * not found in GConf preferences.
 *
 * Note: please take care of keeping the default value synchronized with
 * those defined in schemas.
 */
gboolean
na_iprefs_should_add_about_item( NAIPrefs *instance )
{
	gboolean about = FALSE;

	g_return_val_if_fail( NA_IS_IPREFS( instance ), FALSE );

	if( st_initialized && !st_finalized ){

		about = na_iprefs_read_bool( instance, IPREFS_ADD_ABOUT_ITEM, TRUE );
	}

	return( about );
}

/**
 * na_iprefs_set_add_about_item:
 * @instance: this #NAIPrefs interface instance.
 * @enabled: the new value to be written.
 *
 * Writes the new value to the GConf preference system.
 */
void
na_iprefs_set_add_about_item( NAIPrefs *instance, gboolean enabled )
{
	g_return_if_fail( NA_IS_IPREFS( instance ));

	if( st_initialized && !st_finalized ){

		na_iprefs_write_bool( instance, IPREFS_ADD_ABOUT_ITEM, enabled );
	}
}

/**
 * na_iprefs_should_create_root_menu:
 * @instance: this #NAIPrefs interface instance.
 *
 * Returns: #TRUE if a root submenu should be created in the Nautilus
 * context menus, #FALSE else.
 *
 * Note: this function returns a suitable default value if the key is
 * not found in GConf preferences.
 *
 * Note: please take care of keeping the default value synchronized with
 * those defined in schemas.
 */
gboolean
na_iprefs_should_create_root_menu( NAIPrefs *instance )
{
	gboolean create = FALSE;

	g_return_val_if_fail( NA_IS_IPREFS( instance ), FALSE );

	if( st_initialized && !st_finalized ){

		create = na_iprefs_read_bool( instance, IPREFS_CREATE_ROOT_MENU, FALSE );
	}

	return( create );
}

/**
 * na_iprefs_set_create_root_menu:
 * @instance: this #NAIPrefs interface instance.
 * @enabled: the new value to be written.
 *
 * Writes the new value to the GConf preference system.
 */
void
na_iprefs_set_create_root_menu( NAIPrefs *instance, gboolean enabled )
{
	g_return_if_fail( NA_IS_IPREFS( instance ));

	if( st_initialized && !st_finalized ){

		na_iprefs_write_bool( instance, IPREFS_CREATE_ROOT_MENU, enabled );
	}
}

/**
 * na_iprefs_get_gconf_client:
 * @instance: this #NAIPrefs interface instance.
 *
 * Returns: a GConfClient object.
 */
GConfClient *
na_iprefs_get_gconf_client( NAIPrefs *instance )
{
	NAIPrefsPrivate *ipp;

	g_return_val_if_fail( NA_IS_IPREFS( instance ), NULL );

	setup_private_data( instance );
	ipp = ( NAIPrefsPrivate * ) g_object_get_data( G_OBJECT( instance ), NA_IPREFS_PRIVATE_DATA );
	g_return_val_if_fail( ipp, NULL );

	return( ipp->client );
}

/**
 * na_iprefs_read_bool:
 * @instance: this #NAIPrefs interface instance.
 * @name: the name of the preference entry.
 * @default_value: default value to be returned if the entry is not found,
 * no default value is available in the schema, of there is no schema at
 * all.
 *
 * Returns: the boolean value.
 */
gboolean
na_iprefs_read_bool( NAIPrefs *instance, const gchar *name, gboolean default_value )
{
	gchar *path;
	gboolean ret;

	g_return_val_if_fail( NA_IS_IPREFS( instance ), FALSE );

	path = gconf_concat_dir_and_key( NA_GCONF_PREFS_PATH, name );
	ret = na_gconf_utils_read_bool( na_iprefs_get_gconf_client( instance ), path, TRUE, default_value );
	g_free( path );

	return( ret );
}

/**
 * na_iprefs_read_string:
 * @instance: this #NAIPrefs interface instance.
 * @name: the preference key.
 * @default_value: the default value, used if entry is not found and
 * there is no schema.
 *
 * Returns: the value, as a newly allocated string which should be
 * g_free() by the caller.
 */
gchar *
na_iprefs_read_string( NAIPrefs *instance, const gchar *name, const gchar *default_value )
{
	gchar *path;
	gchar *value;

	g_return_val_if_fail( NA_IS_IPREFS( instance ), NULL );

	path = gconf_concat_dir_and_key( NA_GCONF_PREFS_PATH, name );
	value = na_gconf_utils_read_string( na_iprefs_get_gconf_client( instance ), path, TRUE, default_value );
	g_free( path );

	return( value );
}

/**
 * na_iprefs_read_string_list:
 * @instance: this #NAIPrefs interface instance.
 * @name: the preference key.
 * @default_value: a default value, used if entry is not found, or there
 * is no default value in the schema, of there is no schema at all.
 *
 * Returns: the list value, which should be na_utils_free_string_list()
 * by the caller.
 */
GSList *
na_iprefs_read_string_list( NAIPrefs *instance, const gchar *name, const gchar *default_value )
{
	gchar *path;
	GSList *list;

	g_return_val_if_fail( NA_IS_IPREFS( instance ), NULL );

	path = gconf_concat_dir_and_key( NA_GCONF_PREFS_PATH, name );
	list = na_gconf_utils_read_string_list( na_iprefs_get_gconf_client( instance ), path );
	g_free( path );

	if(( !list || !g_slist_length( list )) && default_value ){
		g_slist_free( list );
		list = g_slist_append( NULL, g_strdup( default_value ));
	}

	return( list );
}

/**
 * na_iprefs_write_bool:
 * @instance: this #NAIPrefs interface instance.
 * @name: the preference entry.
 * @value: the value to be written.
 *
 * Writes the given boolean value.
 */
void
na_iprefs_write_bool( NAIPrefs *instance, const gchar *name, gboolean value )
{
	gchar *path;

	g_return_if_fail( NA_IS_IPREFS( instance ));

	path = gconf_concat_dir_and_key( NA_GCONF_PREFS_PATH, name );
	na_gconf_utils_write_bool( na_iprefs_get_gconf_client( instance ), path, value, NULL );
	g_free( path );
}

/**
 * na_iprefs_write_string:
 * @instance: this #NAIPrefs interface instance.
 * @name: the preference key.
 * @value: the value to be written.
 *
 * Writes the value as the given GConf preference.
 */
void
na_iprefs_write_string( NAIPrefs *instance, const gchar *name, const gchar *value )
{
	gchar *path;

	g_return_if_fail( NA_IS_IPREFS( instance ));

	path = gconf_concat_dir_and_key( NA_GCONF_PREFS_PATH, name );
	na_gconf_utils_write_string( na_iprefs_get_gconf_client( instance ), path, value, NULL );
	g_free( path );
}

/**
 * na_iprefs_write_string_list
 * @instance: this #NAIPrefs interface instance.
 * @name: the preference key.
 * @value: the value to be written.
 *
 * Writes the value as the given GConf preference.
 */
void
na_iprefs_write_string_list( NAIPrefs *instance, const gchar *name, GSList *list )
{
	gchar *path;

	g_return_if_fail( NA_IS_IPREFS( instance ));

	path = gconf_concat_dir_and_key( NA_GCONF_PREFS_PATH, name );
	na_gconf_utils_write_string_list( na_iprefs_get_gconf_client( instance ), path, list, NULL );
	g_free( path );
}

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

static void
setup_private_data( NAIPrefs *instance )
{
	NAIPrefsPrivate *ipp;

	ipp = ( NAIPrefsPrivate * ) g_object_get_data( G_OBJECT( instance ), NA_IPREFS_PRIVATE_DATA );
	if( !ipp ){
		ipp = g_new0( NAIPrefsPrivate, 1 );
		ipp->client = gconf_client_get_default();
		g_object_set_data( G_OBJECT( instance ), NA_IPREFS_PRIVATE_DATA, ipp );
	}
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
