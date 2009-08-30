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

#include <gconf/gconf.h>
#include <gconf/gconf-client.h>

#include "na-iprefs.h"

/* private interface data
 */
struct NAIPrefsInterfacePrivate {
	GConfClient *client;
};

static GType    register_type( void );
static void     interface_base_init( NAIPrefsInterface *klass );
static void     interface_base_finalize( NAIPrefsInterface *klass );

static gboolean read_key_bool( NAIPrefs *instance, const gchar *name, gboolean default_value );
static void     write_key_bool( NAIPrefs *instance, const gchar *name, gboolean value );

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
	static gboolean initialized = FALSE;

	if( !initialized ){
		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NAIPrefsInterfacePrivate, 1 );

		klass->private->client = gconf_client_get_default();

		initialized = TRUE;
	}
}

static void
interface_base_finalize( NAIPrefsInterface *klass )
{
	static const gchar *thisfn = "na_iprefs_interface_base_finalize";
	static gboolean finalized = FALSE ;

	if( !finalized ){
		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );

		finalized = TRUE;
	}
}

/**
 * na_iprefs_get_alphabetical_order:
 * @instance: this #NAIPrefs interface instance.
 *
 * Returns: #TRUE if the actions are to be maintained in alphabetical
 * order of their label, #FALSE else.
 *
 * Note: this function returns a suitable default value if the key is
 * not found in GConf preferences.
 *
 * Note: please take care of keeping the default value synchronized with
 * those defined in schemas.
 */
gboolean na_iprefs_get_alphabetical_order( NAIPrefs *instance )
{
	return( read_key_bool( instance, PREFS_DISPLAY_ALPHABETICAL_ORDER, TRUE ));
}

/**
 * na_iprefs_get_add_about_item:
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
gboolean na_iprefs_get_add_about_item( NAIPrefs *instance )
{
	return( read_key_bool( instance, PREFS_ADD_ABOUT_ITEM, TRUE ));
}

/**
 * Get a named boolean.
 * @instance: this #NAIPrefs interface instance.
 * @name: the name of the key to be read.
 *
 * Returns: the boolean attached to the @name key.
 *
 * Note that this returns #FALSE if the key doesn't exist.
 * See na_iprefs_get_alphabetical_order() and
 * na_iprefs_get_add_about_item() to get suitable default values.
 */
gboolean
na_iprefs_get_bool( NAIPrefs *instance, const gchar *name )
{
	return( read_key_bool( instance, name, FALSE ));
}

/**
 * Set a named boolean.
 * @instance: this #NAIPrefs interface instance.
 * @name: the name of the key to be read.
 *
 * Records the specified boolean in the GConf preferences.
 */
void
na_iprefs_set_bool( NAIPrefs *instance, const gchar *name, gboolean value )
{
	write_key_bool( instance, name, value );
}

/*
 * note that don't rely on having correctly installed the schema for the key
 */
static gboolean
read_key_bool( NAIPrefs *instance, const gchar *name, gboolean default_value )
{
	static const gchar *thisfn = "na_iprefs_read_key_bool";
	GError *error = NULL;
	gchar *path;
	GConfValue *value;
	gboolean ret;

	ret = default_value;

	path = g_strdup_printf( "%s/%s", NA_GCONF_PREFS_PATH, name );

	value = gconf_client_get_without_default( NA_IPREFS_GET_INTERFACE( instance )->private->client, path, &error );
	/*g_debug( "%s: path=%s, value=%p", thisfn, path, ( void * ) value );*/

	if( error ){
		g_warning( "%s: name=%s, %s", thisfn, name, error->message );
		g_error_free( error );
		if( value ){
			gconf_value_free( value );
			value = NULL;
		}
	}

	if( value ){
		ret = gconf_value_get_bool( value );
		gconf_value_free( value );
	}

	g_free( path );
	return( ret );
}

static void
write_key_bool( NAIPrefs *instance, const gchar *name, gboolean value )
{
	static const gchar *thisfn = "na_iprefs_write_key_bool";
	GError *error = NULL;
	gchar *path;

	path = g_strdup_printf( "%s/%s", NA_GCONF_PREFS_PATH, name );

	gconf_client_set_bool( NA_IPREFS_GET_INTERFACE( instance )->private->client, path, value, &error );

	if( error ){
		g_warning( "%s: name=%s, %s", thisfn, name, error->message );
		g_error_free( error );
	}

	g_free( path );
}
