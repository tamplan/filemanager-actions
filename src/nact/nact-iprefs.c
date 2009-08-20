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

#include <common/na-iprefs.h>

#include "nact-iprefs.h"

/* private interface data
 */
struct NactIPrefsInterfacePrivate {
	GConfClient *client;
};

/* key to read/write the last visited folder when browsing for a file
 */
#define IPREFS_ICONDITION_FOLDER_URI			"iconditions-folder-uri"
#define IPREFS_IMPORT_ACTIONS_FOLDER_URI		"import-folder-uri"
#define IPREFS_EXPORT_ACTIONS_FOLDER_URI		"export-folder-uri"

static GType    register_type( void );
static void     interface_base_init( NactIPrefsInterface *klass );
static void     interface_base_finalize( NactIPrefsInterface *klass );

static gchar   *v_get_iprefs_window_id( NactWindow *window );

static GSList  *read_key_listint( NactWindow *window, const gchar *key );
static void     write_key_listint( NactWindow *window, const gchar *key, GSList *list );
static void     listint_to_position( NactWindow *window, GSList *list, gint *x, gint *y, gint *width, gint *height );
static GSList  *position_to_listint( NactWindow *window, gint x, gint y, gint width, gint height );
static void     free_listint( GSList *list );
static gchar   *read_key_str( NactWindow *window, const gchar *key );
static void     save_key_str( NactWindow *window, const gchar *key, const gchar *text );
static gint     read_key_int( NactWindow *window, const gchar *name );
static void     write_key_int( NactWindow *window, const gchar *name, gint value );

GType
nact_iprefs_get_type( void )
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
	static const gchar *thisfn = "nact_iprefs_register_type";
	g_debug( "%s", thisfn );

	static const GTypeInfo info = {
		sizeof( NactIPrefsInterface ),
		( GBaseInitFunc ) interface_base_init,
		( GBaseFinalizeFunc ) interface_base_finalize,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL
	};

	GType type = g_type_register_static( G_TYPE_INTERFACE, "NactIPrefs", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( NactIPrefsInterface *klass )
{
	static const gchar *thisfn = "nact_iprefs_interface_base_init";
	static gboolean initialized = FALSE;

	if( !initialized ){
		g_debug( "%s: klass=%p", thisfn, klass );

		klass->private = g_new0( NactIPrefsInterfacePrivate, 1 );

		klass->private->client = gconf_client_get_default();

		klass->get_iprefs_window_id = NULL;

		initialized = TRUE;
	}
}

static void
interface_base_finalize( NactIPrefsInterface *klass )
{
	static const gchar *thisfn = "nact_iprefs_interface_base_finalize";
	static gboolean finalized = FALSE ;

	if( !finalized ){
		g_debug( "%s: klass=%p", thisfn, klass );

		g_free( klass->private );

		finalized = TRUE;
	}
}

/**
 * Position the specified window on the screen.
 *
 * @window: this NactWindow-derived window.
 *
 * A window position is stored as a list of integers "x,y,width,height".
 */
void
nact_iprefs_position_window( NactWindow *window )
{
	gchar *key = v_get_iprefs_window_id( window );
	if( key ){
		GtkWindow *toplevel = base_window_get_toplevel_dialog( BASE_WINDOW( window ));
		nact_iprefs_position_named_window( window, toplevel, key );
		g_free( key );
	}
}

/**
 * Position the specified window on the screen.
 *
 * @window: this NactWindow-derived window.
 *
 * @name: the name of the window
 */
void
nact_iprefs_position_named_window( NactWindow *window, GtkWindow *toplevel, const gchar *key )
{
	static const gchar *thisfn = "nact_iprefs_position_named_window";

	GSList *list = read_key_listint( window, key );
	if( list ){

		gint x=0, y=0, width=0, height=0;
		listint_to_position( window, list, &x, &y, &width, &height );
		g_debug( "%s: key=%s, x=%d, y=%d, width=%d, height=%d", thisfn, key, x, y, width, height );
		free_listint( list );

		gtk_window_move( toplevel, x, y );
		gtk_window_resize( toplevel, width, height );
	}
}

/**
 * Save the position of the specified window.
 *
 * @window: this NactWindow-derived window.
 *
 * @code: the IPrefs identifiant of the window
 */
void
nact_iprefs_save_window_position( NactWindow *window )
{
	gchar *key = v_get_iprefs_window_id( window );
	if( key ){
		GtkWindow *toplevel = base_window_get_toplevel_dialog( BASE_WINDOW( window ));
		nact_iprefs_save_named_window_position( window, toplevel, key );
		g_free( key );
	}
}

/**
 * Save the position of the specified window.
 *
 * @window: this NactWindow-derived window.
 *
 * @key: the name of the window
 */
void
nact_iprefs_save_named_window_position( NactWindow *window, GtkWindow *toplevel, const gchar *key )
{
	static const gchar *thisfn = "nact_iprefs_save_named_window_position";
	gint x, y, width, height;

	if( GTK_IS_WINDOW( toplevel )){
		gtk_window_get_position( toplevel, &x, &y );
		gtk_window_get_size( toplevel, &width, &height );
		g_debug( "%s: key=%s, x=%d, y=%d, width=%d, height=%d", thisfn, key, x, y, width, height );

		GSList *list = position_to_listint( window, x, y, width, height );
		write_key_listint( window, key, list );
		free_listint( list );
	}
}

/**
 * Save the last visited folder when browsing for command in
 * IConditions interface.
 *
 * @window: this NactWindow-derived window.
 *
 * Returns the last visited folder if any, or NULL.
 * The returned string must be g_free by the caller.
 */
gchar *
nact_iprefs_get_iconditions_folder_uri( NactWindow *window )
{
	return( read_key_str( window, IPREFS_ICONDITION_FOLDER_URI ));
}

void
nact_iprefs_save_iconditions_folder_uri( NactWindow *window, const gchar *uri )
{
	save_key_str( window, IPREFS_ICONDITION_FOLDER_URI, uri );
}

/**
 * Save the last visited folder when importing an action.
 *
 * @window: this NactWindow-derived window.
 *
 * Returns the last visited folder if any, or NULL.
 * The returned string must be g_free by the caller.
 */
gchar *
nact_iprefs_get_import_folder_uri( NactWindow *window )
{
	return( read_key_str( window, IPREFS_IMPORT_ACTIONS_FOLDER_URI ));
}

void
nact_iprefs_save_import_folder_uri( NactWindow *window, const gchar *uri )
{
	save_key_str( window, IPREFS_IMPORT_ACTIONS_FOLDER_URI, uri );
}

/**
 * Save the last visited folder when exporting an action.
 *
 * @window: this NactWindow-derived window.
 *
 * Returns the last visited folder if any, or NULL.
 * The returned string must be g_free by the caller.
 */
gchar *
nact_iprefs_get_export_folder_uri( NactWindow *window )
{
	return( read_key_str( window, IPREFS_EXPORT_ACTIONS_FOLDER_URI ));
}

void
nact_iprefs_save_export_folder_uri( NactWindow *window, const gchar *uri )
{
	save_key_str( window, IPREFS_EXPORT_ACTIONS_FOLDER_URI, uri );
}

/**
 * Get/set a named integer.
 *
 * @window: this NactWindow-derived window.
 */
gint
nact_iprefs_get_int( NactWindow *window, const gchar *name )
{
	return( read_key_int( window, name ));
}

void
nact_iprefs_set_int( NactWindow *window, const gchar *name, gint value )
{
	write_key_int( window, name, value );
}

static gchar *
v_get_iprefs_window_id( NactWindow *window )
{
	g_assert( NACT_IS_IPREFS( window ));

	if( NACT_IPREFS_GET_INTERFACE( window )->get_iprefs_window_id ){
		return( NACT_IPREFS_GET_INTERFACE( window )->get_iprefs_window_id( window ));
	}

	return( NULL );
}

/*
 * returns a list of GConfValue
 */
static GSList *
read_key_listint( NactWindow *window, const gchar *key )
{
	static const gchar *thisfn = "nact_iprefs_read_key_listint";
	GError *error = NULL;
	gchar *path = g_strdup_printf( "%s/%s", NA_GCONF_PREFS_PATH, key );

	GSList *list = gconf_client_get_list(
			NACT_IPREFS_GET_INTERFACE( window )->private->client, path, GCONF_VALUE_INT, &error );

	if( error ){
		g_warning( "%s: %s", thisfn, error->message );
		g_error_free( error );
		list = NULL;
	}

	g_free( path );
	return( list );
}

static void
write_key_listint( NactWindow *window, const gchar *key, GSList *list )
{
	static const gchar *thisfn = "nact_iprefs_write_key_listint";
	GError *error = NULL;
	gchar *path = g_strdup_printf( "%s/%s", NA_GCONF_PREFS_PATH, key );

	gconf_client_set_list(
			NACT_IPREFS_GET_INTERFACE( window )->private->client, path, GCONF_VALUE_INT, list, &error );

	if( error ){
		g_warning( "%s: %s", thisfn, error->message );
		g_error_free( error );
		list = NULL;
	}

	g_free( path );
}

/*
 * extract the position of the window from the list of GConfValue
 */
static void
listint_to_position( NactWindow *window, GSList *list, gint *x, gint *y, gint *width, gint *height )
{
	g_assert( x );
	g_assert( y );
	g_assert( width );
	g_assert( height );
	GSList *il;
	int i;

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
position_to_listint( NactWindow *window, gint x, gint y, gint width, gint height )
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
free_listint( GSList *list )
{
	/*GSList *il;
	for( il = list ; il ; il = il->next ){
		GConfValue *value = ( GConfValue * ) il->data;
		gconf_value_free( value );
	}*/
	g_slist_free( list );
}

static gchar *
read_key_str( NactWindow *window, const gchar *key )
{
	static const gchar *thisfn = "nact_iprefs_read_key_str";
	GError *error = NULL;
	gchar *path = g_strdup_printf( "%s/%s", NA_GCONF_PREFS_PATH, key );

	gchar *text = gconf_client_get_string( NACT_IPREFS_GET_INTERFACE( window )->private->client, path, &error );

	if( error ){
		g_warning( "%s: key=%s, %s", thisfn, key, error->message );
		g_error_free( error );
		text = NULL;
	}

	g_free( path );
	return( text );
}

static void
save_key_str( NactWindow *window, const gchar *key, const gchar *text )
{
	static const gchar *thisfn = "nact_iprefs_save_key_str";
	GError *error = NULL;
	gchar *path = g_strdup_printf( "%s/%s", NA_GCONF_PREFS_PATH, key );

	gconf_client_set_string( NACT_IPREFS_GET_INTERFACE( window )->private->client, path, text, &error );

	if( error ){
		g_warning( "%s: key=%s, %s", thisfn, key, error->message );
		g_error_free( error );
	}

	g_free( path );
}

static gint
read_key_int( NactWindow *window, const gchar *name )
{
	static const gchar *thisfn = "nact_iprefs_read_key_int";
	GError *error = NULL;
	gchar *path = g_strdup_printf( "%s/%s", NA_GCONF_PREFS_PATH, name );

	gint value = gconf_client_get_int( NACT_IPREFS_GET_INTERFACE( window )->private->client, path, &error );

	if( error ){
		g_warning( "%s: name=%s, %s", thisfn, name, error->message );
		g_error_free( error );
	}

	g_free( path );
	return( value );
}

static void
write_key_int( NactWindow *window, const gchar *name, gint value )
{
	static const gchar *thisfn = "nact_iprefs_write_key_int";
	GError *error = NULL;
	gchar *path = g_strdup_printf( "%s/%s", NA_GCONF_PREFS_PATH, name );

	gconf_client_set_int( NACT_IPREFS_GET_INTERFACE( window )->private->client, path, value, &error );

	if( error ){
		g_warning( "%s: name=%s, %s", thisfn, name, error->message );
		g_error_free( error );
	}

	g_free( path );
}
