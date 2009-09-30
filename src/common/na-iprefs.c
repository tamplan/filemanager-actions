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

#include "na-gconf-utils.h"
#include "na-iprefs.h"

/* private interface data
 */
struct NAIPrefsInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private data initialized the first time an instance calls a function
 * of the public api
 */
typedef struct {
	GConfClient *client;
}
	NAIPrefsPrivate;

#define DEFAULT_ORDER_MODE_INT			PREFS_ORDER_ALPHA_ASCENDING
#define DEFAULT_ORDER_MODE_STR			"AscendingOrder"

static GConfEnumStringPair order_mode_table[] = {
	{ PREFS_ORDER_ALPHA_ASCENDING , "AscendingOrder" },
	{ PREFS_ORDER_ALPHA_DESCENDING, "DescendingOrder" },
	{ PREFS_ORDER_MANUAL          , "ManualOrder" },
	{ 0, NULL }
};

#define NA_IPREFS_PRIVATE_DATA			"na-iprefs-private-data"

static gboolean st_initialized = FALSE;
static gboolean st_finalized = FALSE;

static GType        register_type( void );
static void         interface_base_init( NAIPrefsInterface *klass );
static void         interface_base_finalize( NAIPrefsInterface *klass );

static gboolean     read_bool( NAIPrefs *instance, const gchar *name, gboolean default_value );
/*static gint         read_int( NAIPrefs *instance, const gchar *name, gint default_value );*/
static gchar       *read_string( NAIPrefs *instance, const gchar *name, const gchar *default_value );
static GSList      *read_string_list( NAIPrefs *instance, const gchar *name );
static void         write_bool( NAIPrefs *instance, const gchar *name, gboolean value );
/*static void         write_int( NAIPrefs *instance, const gchar *name, gint value );*/
static void         write_string( NAIPrefs *instance, const gchar *name, const gchar *value );
static void         write_string_list( NAIPrefs *instance, const gchar *name, GSList *list );

static void         setup_private_data( NAIPrefs *instance );
static GConfClient *get_gconf_client( NAIPrefs *instance );

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
	GSList *level_zero = NULL;

	g_return_val_if_fail( NA_IS_IPREFS( instance ), NULL );

	if( st_initialized && !st_finalized ){
		setup_private_data( instance );
		level_zero = read_string_list( instance, PREFS_LEVEL_ZERO_ITEMS );
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
		setup_private_data( instance );
		write_string_list( instance, PREFS_LEVEL_ZERO_ITEMS, order );
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

		setup_private_data( instance );
		order_str = read_string( instance, PREFS_DISPLAY_ALPHABETICAL_ORDER, DEFAULT_ORDER_MODE_STR );
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
		setup_private_data( instance );
		order_str = gconf_enum_to_string( order_mode_table, mode );
		write_string( instance, PREFS_DISPLAY_ALPHABETICAL_ORDER, order_str ? order_str : DEFAULT_ORDER_MODE_STR );
	}
}

/**
 * na_iprefs_should_add_about_item:
 * @instance: this #NAIPrefs interface instance.
 *
 * Returns: #TRUE if an "About Nautilus Actions" item may be added to
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
		setup_private_data( instance );
		about = read_bool( instance, PREFS_ADD_ABOUT_ITEM, TRUE );
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
		setup_private_data( instance );
		write_bool( instance, PREFS_ADD_ABOUT_ITEM, enabled );
	}
}

static gboolean
read_bool( NAIPrefs *instance, const gchar *name, gboolean default_value )
{
	gchar *path;
	gboolean ret;

	path = gconf_concat_dir_and_key( NA_GCONF_PREFS_PATH, name );
	ret = na_gconf_utils_read_bool( get_gconf_client( instance ), path, TRUE, default_value );
	g_free( path );

	return( ret );
}

/*static gint
read_int( NAIPrefs *instance, const gchar *name, gint default_value )
{
	gchar *path;
	gint ret;

	path = gconf_concat_dir_and_key( NA_GCONF_PREFS_PATH, name );
	ret = na_gconf_utils_read_int( get_gconf_client( instance ), path, TRUE, default_value );
	g_free( path );

	return( ret );
}*/

static gchar *
read_string( NAIPrefs *instance, const gchar *name, const gchar *default_value )
{
	gchar *path;
	gchar *value;

	path = gconf_concat_dir_and_key( NA_GCONF_PREFS_PATH, name );
	value = na_gconf_utils_read_string( get_gconf_client( instance ), path, TRUE, default_value );
	g_free( path );

	return( value );
}

static GSList *
read_string_list( NAIPrefs *instance, const gchar *name )
{
	gchar *path;
	GSList *list;

	path = gconf_concat_dir_and_key( NA_GCONF_PREFS_PATH, name );
	list = na_gconf_utils_read_string_list( get_gconf_client( instance ), path );
	g_free( path );

	return( list );
}

static void
write_bool( NAIPrefs *instance, const gchar *name, gboolean value )
{
	gchar *path;

	path = gconf_concat_dir_and_key( NA_GCONF_PREFS_PATH, name );
	na_gconf_utils_write_bool( get_gconf_client( instance ), path, value, NULL );
	g_free( path );
}

/*static void
write_int( NAIPrefs *instance, const gchar *name, gint value )
{
	gchar *path;

	path = gconf_concat_dir_and_key( NA_GCONF_PREFS_PATH, name );
	na_gconf_utils_write_int( get_gconf_client( instance ), path, value, NULL );
	g_free( path );
}*/

static void
write_string( NAIPrefs *instance, const gchar *name, const gchar *value )
{
	gchar *path;

	path = gconf_concat_dir_and_key( NA_GCONF_PREFS_PATH, name );
	na_gconf_utils_write_string( get_gconf_client( instance ), path, value, NULL );
	g_free( path );
}

static void
write_string_list( NAIPrefs *instance, const gchar *name, GSList *list )
{
	gchar *path;

	path = gconf_concat_dir_and_key( NA_GCONF_PREFS_PATH, name );
	na_gconf_utils_write_string_list( get_gconf_client( instance ), path, list, NULL );
	g_free( path );
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

static GConfClient *
get_gconf_client( NAIPrefs *instance )
{
	NAIPrefsPrivate *ipp;

	ipp = ( NAIPrefsPrivate * ) g_object_get_data( G_OBJECT( instance ), NA_IPREFS_PRIVATE_DATA );
	g_return_val_if_fail( ipp, NULL );

	return( ipp->client );
}
