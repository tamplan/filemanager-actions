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

#include <common/na-gconf-keys-schemas.h>
#include <common/na-gconf-utils.h>
#include <common/na-iprefs.h>

#include "base-iprefs.h"

/* private interface data
 */
struct BaseIPrefsInterfacePrivate {
	GConfClient *client;
};

#define BASE_IPREFS_SCHEMAS_PATH		NAUTILUS_ACTIONS_GCONF_SCHEMASDIR NA_GCONF_PREFS_PATH

static gboolean st_initialized = FALSE;
static gboolean st_finalized = FALSE;

static GType       register_type( void );
static void        interface_base_init( BaseIPrefsInterface *klass );
static void        interface_base_finalize( BaseIPrefsInterface *klass );

static gchar      *v_iprefs_get_window_id( BaseWindow *window );

static GConfValue *get_value( GConfClient *client, const gchar *path, const gchar *entry );
static void        set_value( GConfClient *client, const gchar *path, const gchar *entry, GConfValue *value );
static gboolean    remove_entry( GConfClient *client, const gchar *path, const gchar *entry );

static gboolean    read_bool( BaseWindow *window, const gchar *name );
static gint        read_int( BaseWindow *window, const gchar *name );
static GSList     *read_int_list( BaseWindow *window, const gchar *key );
static gchar      *read_str( BaseWindow *window, const gchar *key );

static void        write_bool( BaseWindow *window, const gchar *name, gboolean value );
static void        write_int( BaseWindow *window, const gchar *name, gint value );
static void        write_int_list( BaseWindow *window, const gchar *key, GSList *list );
static void        write_str( BaseWindow *window, const gchar *key, const gchar *text );

static void        int_list_to_position( BaseWindow *window, GSList *list, gint *x, gint *y, gint *width, gint *height );
static GSList     *position_to_int_list( BaseWindow *window, gint x, gint y, gint width, gint height );
static void        free_int_list( GSList *list );

GType
base_iprefs_get_type( void )
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
	static const gchar *thisfn = "base_iprefs_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( BaseIPrefsInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "BaseIPrefs", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( BaseIPrefsInterface *klass )
{
	static const gchar *thisfn = "base_iprefs_interface_base_init";

	if( !st_initialized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( BaseIPrefsInterfacePrivate, 1 );
		klass->private->client = gconf_client_get_default();

		klass->iprefs_get_window_id = NULL;

		st_initialized = TRUE;
	}
}

static void
interface_base_finalize( BaseIPrefsInterface *klass )
{
	static const gchar *thisfn = "base_iprefs_interface_base_finalize";

	if( !st_finalized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );

		st_finalized = TRUE;
	}
}

/**
 * base_iprefs_migrate_key:
 * @window: this #BaseWindow-derived window.
 * @old_key: the old preference entry.
 * @new_key: the new preference entry.
 *
 * Migrates the content of an entry from an obsoleted key to a new one.
 * Removes the old key, along with the schema associated to it,
 * considering that the version which asks for this migration has
 * installed a schema corresponding to the new key.
 */
void
base_iprefs_migrate_key( BaseWindow *window, const gchar *old_key, const gchar *new_key )
{
	static const gchar *thisfn = "base_iprefs_migrate_key";
	GConfValue *value;

	g_debug( "%s: window=%p, old_key=%s, new_key=%s", thisfn, ( void * ) window, old_key, new_key );
	g_return_if_fail( BASE_IS_WINDOW( window ));
	g_return_if_fail( BASE_IS_IPREFS( window ));

	if( st_initialized && !st_finalized ){

		value = get_value( BASE_IPREFS_GET_INTERFACE( window )->private->client, NA_GCONF_PREFS_PATH, old_key );

		if( value ){
			set_value( BASE_IPREFS_GET_INTERFACE( window )->private->client, NA_GCONF_PREFS_PATH, new_key, value );
			gconf_value_free( value );
		}

		remove_entry( BASE_IPREFS_GET_INTERFACE( window )->private->client, NA_GCONF_PREFS_PATH, old_key );
		remove_entry( BASE_IPREFS_GET_INTERFACE( window )->private->client, BASE_IPREFS_SCHEMAS_PATH, old_key );
	}
}

/**
 * base_iprefs_position_window:
 * @window: this #BaseWindow-derived window.
 *
 * Position the specified window on the screen.
 *
 * A window position is stored as a list of integers "x,y,width,height".
 */
void
base_iprefs_position_window( BaseWindow *window )
{
	GtkWindow *toplevel;
	gchar *key;

	g_return_if_fail( BASE_IS_WINDOW( window ));
	g_return_if_fail( BASE_IS_IPREFS( window ));

	if( st_initialized && !st_finalized ){

		key = v_iprefs_get_window_id( window );
		if( key ){
			toplevel = base_window_get_toplevel( BASE_WINDOW( window ));
			base_iprefs_position_named_window( window, toplevel, key );
			g_free( key );
		}
	}
}

/**
 * base_iprefs_position_named_window:
 * @window: this #BaseWindow-derived window.
 * @toplevel: the toplevel #GtkWindow whose size and position are to be
 * set.
 * @key: the string id of this toplevel.
 *
 * Positions the specified window on the screen, maximizing it by the
 * actual current screen size. Note that this is a rough approximation
 * as some of the screen is reserved by deskbars and so...
 */
void
base_iprefs_position_named_window( BaseWindow *window, GtkWindow *toplevel, const gchar *key )
{
	static const gchar *thisfn = "base_iprefs_position_named_window";
	GSList *list;
	gint x=0, y=0, width=0, height=0;
	GdkDisplay *display;
	GdkScreen *screen;
	gint screen_width, screen_height;

	g_return_if_fail( BASE_IS_WINDOW( window ));
	g_return_if_fail( BASE_IS_IPREFS( window ));

	if( st_initialized && !st_finalized ){

		list = read_int_list( window, key );
		if( list ){

			int_list_to_position( window, list, &x, &y, &width, &height );
			g_debug( "%s: key=%s, x=%d, y=%d, width=%d, height=%d", thisfn, key, x, y, width, height );
			free_int_list( list );

			display = gdk_display_get_default();
			screen = gdk_display_get_screen( display, 0 );
			screen_width = gdk_screen_get_width( screen );
			screen_height = gdk_screen_get_height( screen );

			if(( x+width > screen_width ) ||
				( y+height > screen_height )){
				gtk_window_get_default_size( toplevel, &width, &height );
			}

			gtk_window_move( toplevel, x, y );
			gtk_window_resize( toplevel, width, height );
		}
	}
}

/**
 * base_iprefs_save_window_position:
 * @window: this #BaseWindow-derived window.
 *
 * Save the size and position of the specified window.
 */
void
base_iprefs_save_window_position( BaseWindow *window )
{
	GtkWindow *toplevel;
	gchar *key;

	g_return_if_fail( BASE_IS_WINDOW( window ));
	g_return_if_fail( BASE_IS_IPREFS( window ));

	if( st_initialized && !st_finalized ){

		key = v_iprefs_get_window_id( window );
		if( key ){
			toplevel = base_window_get_toplevel( BASE_WINDOW( window ));
			base_iprefs_save_named_window_position( window, toplevel, key );
			g_free( key );
		}
	}
}

/**
 * base_iprefs_save_named_window_position:
 * @window: this #BaseWindow-derived window.
 * @toplevel: the #GtkWindow whose size and position are to be saved.
 * @key: the name of the window.
 *
 * Save size and position of the specified window.
 */
void
base_iprefs_save_named_window_position( BaseWindow *window, GtkWindow *toplevel, const gchar *key )
{
	static const gchar *thisfn = "base_iprefs_save_named_window_position";
	gint x, y, width, height;
	GSList *list;

	g_return_if_fail( BASE_IS_WINDOW( window ));
	g_return_if_fail( BASE_IS_IPREFS( window ));

	if( st_initialized && !st_finalized ){

		if( GTK_IS_WINDOW( toplevel )){
			gtk_window_get_position( toplevel, &x, &y );
			gtk_window_get_size( toplevel, &width, &height );
			g_debug( "%s: key=%s, x=%d, y=%d, width=%d, height=%d", thisfn, key, x, y, width, height );

			list = position_to_int_list( window, x, y, width, height );
			write_int_list( window, key, list );
			free_int_list( list );
		}
	}
}

/**
 * base_iprefs_get_bool:
 * @window: this #BaseWindow-derived window.
 * @name: the entry to be readen.
 *
 * Returns: the named boolean.
 */
gboolean
base_iprefs_get_bool( BaseWindow *window, const gchar *name )
{
	gboolean ret = FALSE;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), FALSE );
	g_return_val_if_fail( BASE_IS_IPREFS( window ), FALSE );

	if( st_initialized && !st_finalized ){
		ret = read_bool( window, name );
	}

	return( ret );
}

/**
 * base_iprefs_set_bool:
 * @window: this #BaseWindow-derived window.
 * @name: the entry to be readen.
 * @value: the value to be set.
 *
 * Writes the named boolean in GConf.
 */
void
base_iprefs_set_bool( BaseWindow *window, const gchar *name, gboolean value )
{
	g_return_if_fail( BASE_IS_WINDOW( window ));
	g_return_if_fail( BASE_IS_IPREFS( window ));

	if( st_initialized && !st_finalized ){
		write_bool( window, name, value );
	}
}

/**
 * base_iprefs_get_int:
 * @window: this BaseWindow-derived window.
 * @name: the entry to be readen.
 *
 * Returns: the named integer.
 */
gint
base_iprefs_get_int( BaseWindow *window, const gchar *name )
{
	gint ret = 0;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), 0 );
	g_return_val_if_fail( BASE_IS_IPREFS( window ), 0 );

	if( st_initialized && !st_finalized ){
		ret = read_int( window, name );
	}

	return( ret );
}

/**
 * base_iprefs_set_int:
 * @window: this BaseWindow-derived window.
 * @name: the entry to be written.
 * @value: the integer to be set.
 *
 * Writes an integer in the GConf system.
 */
void
base_iprefs_set_int( BaseWindow *window, const gchar *name, gint value )
{
	g_return_if_fail( BASE_IS_WINDOW( window ));
	g_return_if_fail( BASE_IS_IPREFS( window ));

	if( st_initialized && !st_finalized ){
		write_int( window, name, value );
	}
}

/**
 * base_iprefs_get_string:
 * @window: this #BaseWindow-derived window.
 * @name: entry of the string value.
 *
 * Returns the required string if any, or NULL.
 * The returned string must be g_free() by the caller.
 */
gchar *
base_iprefs_get_string( BaseWindow *window, const gchar *name )
{
	gchar *string = NULL;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), NULL );
	g_return_val_if_fail( BASE_IS_IPREFS( window ), NULL );

	if( st_initialized && !st_finalized ){
		string = read_str( window, name );
	}

	return( string );
}

/**
 * base_iprefs_set_string:
 * @window: this #BaseWindow-derived window.
 * @name: entry of the string value.
 * @string: value to save.
 *
 * Saves the required string.
 */
void
base_iprefs_set_string( BaseWindow *window, const gchar *name, const gchar *string )
{
	g_return_if_fail( BASE_IS_WINDOW( window ));
	g_return_if_fail( BASE_IS_IPREFS( window ));

	if( st_initialized && !st_finalized ){
		write_str( window, name, string );
	}
}

static gchar *
v_iprefs_get_window_id( BaseWindow *window )
{
	g_return_val_if_fail( BASE_IS_IPREFS( window ), NULL );

	if( BASE_IPREFS_GET_INTERFACE( window )->iprefs_get_window_id ){
		return( BASE_IPREFS_GET_INTERFACE( window )->iprefs_get_window_id( window ));
	}

	return( NULL );
}

static GConfValue *
get_value( GConfClient *client, const gchar *path, const gchar *entry )
{
	static const gchar *thisfn = "base_iprefs_get_value";
	GError *error = NULL;
	gchar *fullpath;
	GConfValue *value;

	fullpath = g_strdup_printf( "%s/%s", path, entry );

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
	static const gchar *thisfn = "base_iprefs_set_value";
	GError *error = NULL;
	gchar *fullpath;

	g_return_if_fail( value );

	fullpath = g_strdup_printf( "%s/%s", path, entry );

	gconf_client_set( client, fullpath, value, &error );

	if( error ){
		g_warning( "%s: key=%s, %s", thisfn, fullpath, error->message );
		g_error_free( error );
	}

	g_free( fullpath );
}

static gboolean
remove_entry( GConfClient *client, const gchar *path, const gchar *entry )
{
	static const gchar *thisfn = "base_iprefs_remove_entry";
	GError *error = NULL;
	gchar *fullpath;
	gboolean ret;

	fullpath = g_strdup_printf( "%s/%s", path, entry );

	ret = gconf_client_unset( client, fullpath, &error );

	if( error ){
		g_warning( "%s: key=%s, %s", thisfn, fullpath, error->message );
		g_error_free( error );
	}

	g_free( fullpath );

	return( ret );
}

static gboolean
read_bool( BaseWindow *window, const gchar *name )
{
	gchar *path;
	gint value;

	path = g_strdup_printf( "%s/%s", NA_GCONF_PREFS_PATH, name );

	value = na_gconf_utils_read_bool( BASE_IPREFS_GET_INTERFACE( window )->private->client, path, TRUE, FALSE );

	g_free( path );
	return( value );
}

static gint
read_int( BaseWindow *window, const gchar *name )
{
	static const gchar *thisfn = "base_iprefs_read_key_int";
	GError *error = NULL;
	gchar *path;
	gint value;

	path = g_strdup_printf( "%s/%s", NA_GCONF_PREFS_PATH, name );

	value = gconf_client_get_int( BASE_IPREFS_GET_INTERFACE( window )->private->client, path, &error );

	if( error ){
		g_warning( "%s: name=%s, %s", thisfn, name, error->message );
		g_error_free( error );
	}

	g_free( path );
	return( value );
}

/*
 * returns a list of int
 */
static GSList *
read_int_list( BaseWindow *window, const gchar *key )
{
	static const gchar *thisfn = "base_iprefs_read_int_list";
	GError *error = NULL;
	gchar *path;
	GSList *list;

	path = g_strdup_printf( "%s/%s", NA_GCONF_PREFS_PATH, key );

	list = gconf_client_get_list(
			BASE_IPREFS_GET_INTERFACE( window )->private->client, path, GCONF_VALUE_INT, &error );

	if( error ){
		g_warning( "%s: path=%s, error=%s", thisfn, path, error->message );
		g_error_free( error );
		list = NULL;
	}

	g_free( path );
	return( list );
}

static gchar *
read_str( BaseWindow *window, const gchar *key )
{
	static const gchar *thisfn = "base_iprefs_read_str";
	GError *error = NULL;
	gchar *path;
	gchar *text;

	path = g_strdup_printf( "%s/%s", NA_GCONF_PREFS_PATH, key );

	text = gconf_client_get_string( BASE_IPREFS_GET_INTERFACE( window )->private->client, path, &error );

	if( error ){
		g_warning( "%s: key=%s, %s", thisfn, key, error->message );
		g_error_free( error );
		text = NULL;
	}

	g_free( path );
	return( text );
}

static void
write_bool( BaseWindow *window, const gchar *name, gboolean value )
{
	gchar *path;

	path = g_strdup_printf( "%s/%s", NA_GCONF_PREFS_PATH, name );

	na_gconf_utils_write_bool( BASE_IPREFS_GET_INTERFACE( window )->private->client, path, value, NULL );

	g_free( path );
}

static void
write_int( BaseWindow *window, const gchar *name, gint value )
{
	static const gchar *thisfn = "base_iprefs_write_int";
	GError *error = NULL;
	gchar *path;

	path = g_strdup_printf( "%s/%s", NA_GCONF_PREFS_PATH, name );

	gconf_client_set_int( BASE_IPREFS_GET_INTERFACE( window )->private->client, path, value, &error );

	if( error ){
		g_warning( "%s: name=%s, %s", thisfn, name, error->message );
		g_error_free( error );
	}

	g_free( path );
}

static void
write_int_list( BaseWindow *window, const gchar *key, GSList *list )
{
	static const gchar *thisfn = "base_iprefs_write_int_list";
	GError *error = NULL;
	gchar *path;

	path = g_strdup_printf( "%s/%s", NA_GCONF_PREFS_PATH, key );

	gconf_client_set_list(
			BASE_IPREFS_GET_INTERFACE( window )->private->client, path, GCONF_VALUE_INT, list, &error );

	if( error ){
		g_warning( "%s: %s", thisfn, error->message );
		g_error_free( error );
		list = NULL;
	}

	g_free( path );
}

static void
write_str( BaseWindow *window, const gchar *key, const gchar *text )
{
	static const gchar *thisfn = "base_iprefs_write_str";
	GError *error = NULL;
	gchar *path;

	path = g_strdup_printf( "%s/%s", NA_GCONF_PREFS_PATH, key );

	gconf_client_set_string( BASE_IPREFS_GET_INTERFACE( window )->private->client, path, text, &error );

	if( error ){
		g_warning( "%s: key=%s, %s", thisfn, key, error->message );
		g_error_free( error );
	}

	g_free( path );
}

/*
 * extract the position of the window from the list of GConfValue
 */
static void
int_list_to_position( BaseWindow *window, GSList *list, gint *x, gint *y, gint *width, gint *height )
{
	GSList *il;
	int i;

	g_assert( x );
	g_assert( y );
	g_assert( width );
	g_assert( height );

	for( il=list, i=0 ; il ; il=il->next, i+=1 ){
		switch( i ){
			case 0:
				*x = GPOINTER_TO_INT( il->data );
				break;
			case 1:
				*y = GPOINTER_TO_INT( il->data );
				break;
			case 2:
				*width = GPOINTER_TO_INT( il->data );
				break;
			case 3:
				*height = GPOINTER_TO_INT( il->data );
				break;
		}
	}
}

static GSList *
position_to_int_list( BaseWindow *window, gint x, gint y, gint width, gint height )
{
	GSList *list = NULL;

	list = g_slist_append( list, GINT_TO_POINTER( x ));
	list = g_slist_append( list, GINT_TO_POINTER( y ));
	list = g_slist_append( list, GINT_TO_POINTER( width ));
	list = g_slist_append( list, GINT_TO_POINTER( height ));

	return( list );
}

/*
 * free the list of int
 */
static void
free_int_list( GSList *list )
{
	/*GSList *il;
	for( il = list ; il ; il = il->next ){
		GConfValue *value = ( GConfValue * ) il->data;
		gconf_value_free( value );
	}*/
	g_slist_free( list );
}
