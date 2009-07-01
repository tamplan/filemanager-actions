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

#include "nact-iprefs.h"

/* private interface data
 */
struct NactIPrefsInterfacePrivate {
	GConfClient *client;
};

/* GConf general information
 */
#define NA_GCONF_PREFS_PATH		NAUTILUS_ACTIONS_CONFIG_GCONF_BASEDIR "/preferences"

/* key to read/write the last visited folder when browsing for command
 */
#define IPREFS_IPROFILE_CONDITION_FOLDER_URI	"iprofile-conditions-folder-uri"

static GType   register_type( void );
static void    interface_base_init( NactIPrefsInterface *klass );
static void    interface_base_finalize( NactIPrefsInterface *klass );

static gchar  *v_get_iprefs_window_id( NactWindow *window );

static GSList *read_key_listint( NactWindow *window, const gchar *key );
static void    write_key_listint( NactWindow *window, const gchar *key, GSList *list );
static void    listint_to_position( NactWindow *window, GSList *list, gint *x, gint *y, gint *width, gint *height );
static GSList *position_to_listint( NactWindow *window, gint x, gint y, gint width, gint height );
static void    free_listint( GSList *list );

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

/**
 * Save the last visited folder when browsing for command in
 * IProfileConditions interface.
 *
 * @window: this NactWindow-derived window.
 *
 * Returns the last visited folder if any, or NULL.
 * The returned string must be g_free by the caller.
 */
gchar *
nact_iprefs_get_iprofile_conditions_folder_uri( NactWindow *window )
{
	static const gchar *thisfn = "nact_iprefs_get_iprofile_conditions_folder_uri";
	GError *error = NULL;
	gchar *path = g_strdup_printf( "%s/%s", NA_GCONF_PREFS_PATH, IPREFS_IPROFILE_CONDITION_FOLDER_URI );

	gchar *uri = gconf_client_get_string( NACT_IPREFS_GET_INTERFACE( window )->private->client, path, &error );

	if( error ){
		g_warning( "%s: %s", thisfn, error->message );
		g_error_free( error );
		uri = NULL;
	}

	g_free( path );
	return( uri );
}

void
nact_iprefs_save_iprofile_conditions_folder_uri( NactWindow *window, const gchar *uri )
{
	static const gchar *thisfn = "nact_iprefs_save_iprofile_conditions_folder_uri";
	GError *error = NULL;
	gchar *path = g_strdup_printf( "%s/%s", NA_GCONF_PREFS_PATH, IPREFS_IPROFILE_CONDITION_FOLDER_URI );

	gconf_client_set_string( NACT_IPREFS_GET_INTERFACE( window )->private->client, path, uri, &error );

	if( error ){
		g_warning( "%s: %s", thisfn, error->message );
		g_error_free( error );
	}

	g_free( path );
}

/* ... */
#include <glib/gi18n.h>

/* List of gconf keys */
#define PREFS_SCHEMES 		"nact_schemes_list"
#define PREFS_MAIN_X  		"nact_main_dialog_position_x"
#define PREFS_MAIN_Y  		"nact_main_dialog_position_y"
#define PREFS_MAIN_W  		"nact_main_dialog_size_width"
#define PREFS_MAIN_H  		"nact_main_dialog_size_height"
#define PREFS_EDIT_X  		"nact_edit_dialog_position_x"
#define PREFS_EDIT_Y  		"nact_edit_dialog_position_y"
#define PREFS_EDIT_W  		"nact_edit_dialog_size_width"
#define PREFS_EDIT_H  		"nact_edit_dialog_size_height"
#define PREFS_IM_EX_X  		"nact_im_ex_dialog_position_x"
#define PREFS_IM_EX_Y  		"nact_im_ex_dialog_position_y"
#define PREFS_IM_EX_W  		"nact_im_ex_dialog_size_width"
#define PREFS_IM_EX_H  		"nact_im_ex_dialog_size_height"
#define PREFS_ICON_PATH		"nact_icon_last_browsed_dir"
#define PREFS_PATH_PATH		"nact_path_last_browsed_dir"
#define PREFS_IMPORT_PATH	"nact_import_last_browsed_dir"
#define PREFS_EXPORT_PATH	"nact_export_last_browsed_dir"

static GSList *
get_prefs_list_key (GConfClient *client, const gchar *key)
{
	gchar *fullkey;
	GSList *l;

	fullkey = g_strdup_printf ("%s/%s", NAUTILUS_ACTIONS_CONFIG_GCONF_BASEDIR, key);
	l = gconf_client_get_list (client, fullkey, GCONF_VALUE_STRING, NULL);

	g_free (fullkey);

	return l;
}

static gchar *
get_prefs_string_key (GConfClient *client, const gchar *key)
{
	gchar *fullkey, *s;

	fullkey = g_strdup_printf ("%s/%s", NAUTILUS_ACTIONS_CONFIG_GCONF_BASEDIR, key);
	s = gconf_client_get_string (client, fullkey, NULL);

	g_free (fullkey);

	return s;
}

static int
get_prefs_int_key (GConfClient *client, const gchar *key)
{
	gchar *fullkey;
	gint i = -1;
	GConfValue* value;

	fullkey = g_strdup_printf ("%s/%s", NAUTILUS_ACTIONS_CONFIG_GCONF_BASEDIR, key);
	value = gconf_client_get (client, fullkey, NULL);

	if (value != NULL)
	{
		i = gconf_value_get_int (value);
	}

	g_free (fullkey);

	return i;
}

static gboolean
set_prefs_list_key (GConfClient *client, const gchar *key, GSList* value)
{
	gchar *fullkey;
	gboolean retv;

	fullkey = g_strdup_printf ("%s/%s", NAUTILUS_ACTIONS_CONFIG_GCONF_BASEDIR, key);
	retv = gconf_client_set_list (client, fullkey, GCONF_VALUE_STRING, value, NULL);

	g_free (fullkey);

	return retv;
}

static gboolean
set_prefs_string_key (GConfClient *client, const gchar *key, const gchar* value)
{
	gchar *fullkey;
	gboolean retv;

	fullkey = g_strdup_printf ("%s/%s", NAUTILUS_ACTIONS_CONFIG_GCONF_BASEDIR, key);
	retv = gconf_client_set_string (client, fullkey, value, NULL);

	g_free (fullkey);

	return retv;
}

static gboolean
set_prefs_int_key (GConfClient *client, const gchar *key, gint value)
{
	gchar *fullkey;
	gboolean retv;

	fullkey = g_strdup_printf ("%s/%s", NAUTILUS_ACTIONS_CONFIG_GCONF_BASEDIR, key);
	retv = gconf_client_set_int (client, fullkey, value, NULL);

	g_free (fullkey);

	return retv;
}

static void prefs_changed_cb (GConfClient *client,
										guint cnxn_id,
									 	GConfEntry *entry,
									 	gpointer user_data)
{
	/*NactPreferences* prefs = (NactPreferences*)user_data;*/

	if (user_data != NULL)
	{
		/*g_print ("Key changed : %s\n", entry->key);*/
	}
}

static NactPreferences* nact_prefs_get_preferences (void)
{
	static NactPreferences* prefs = NULL;
	gchar* tmp;

	if (!prefs)
	{
		prefs = g_new0 (NactPreferences, 1);

		prefs->client = gconf_client_get_default ();

		prefs->schemes = get_prefs_list_key (prefs->client, PREFS_SCHEMES);
		if (!prefs->schemes)
		{
			/* initialize the default schemes */
			/* i18n notes : description of 'file' scheme */
			prefs->schemes = g_slist_append (prefs->schemes, g_strdup_printf (_("%sLocal Files"), "file|"));
			/* i18n notes : description of 'sftp' scheme */
			prefs->schemes = g_slist_append (prefs->schemes, g_strdup_printf (_("%sSSH Files"), "sftp|"));
			/* i18n notes : description of 'smb' scheme */
			prefs->schemes = g_slist_append (prefs->schemes, g_strdup_printf (_("%sWindows Files"), "smb|"));
			/* i18n notes : description of 'ftp' scheme */
			prefs->schemes = g_slist_append (prefs->schemes, g_strdup_printf (_("%sFTP Files"), "ftp|"));
			/* i18n notes : description of 'dav' scheme */
			prefs->schemes = g_slist_append (prefs->schemes, g_strdup_printf (_("%sWebdav Files"), "dav|"));
		}
		prefs->main_size_width  = get_prefs_int_key (prefs->client, PREFS_MAIN_W);
		prefs->main_size_height = get_prefs_int_key (prefs->client, PREFS_MAIN_H);
		prefs->edit_size_width  = get_prefs_int_key (prefs->client, PREFS_EDIT_W);
		prefs->edit_size_height = get_prefs_int_key (prefs->client, PREFS_EDIT_H);
		prefs->im_ex_size_width  = get_prefs_int_key (prefs->client, PREFS_IM_EX_W);
		prefs->im_ex_size_height = get_prefs_int_key (prefs->client, PREFS_IM_EX_H);
		prefs->main_position_x  = get_prefs_int_key (prefs->client, PREFS_MAIN_X);
		prefs->main_position_y  = get_prefs_int_key (prefs->client, PREFS_MAIN_Y);
		prefs->edit_position_x  = get_prefs_int_key (prefs->client, PREFS_EDIT_X);
		prefs->edit_position_y  = get_prefs_int_key (prefs->client, PREFS_EDIT_Y);
		prefs->im_ex_position_x  = get_prefs_int_key (prefs->client, PREFS_IM_EX_X);
		prefs->im_ex_position_y  = get_prefs_int_key (prefs->client, PREFS_IM_EX_Y);
		tmp = get_prefs_string_key (prefs->client, PREFS_ICON_PATH);
		if (!tmp)
		{
			tmp = g_strdup ("/usr/share/pixmaps");
		}
		prefs->icon_last_browsed_dir = tmp;

		tmp = get_prefs_string_key (prefs->client, PREFS_PATH_PATH);
		if (!tmp)
		{
			tmp = g_strdup ("/usr/bin");
		}
		prefs->path_last_browsed_dir = tmp;

		tmp = get_prefs_string_key (prefs->client, PREFS_IMPORT_PATH);
		if (!tmp)
		{
			tmp = g_strdup ("/tmp");
		}
		prefs->import_last_browsed_dir = tmp;

		tmp = get_prefs_string_key (prefs->client, PREFS_EXPORT_PATH);
		if (!tmp)
		{
			tmp = g_strdup ("/tmp");
		}
		prefs->export_last_browsed_dir = tmp;

		gconf_client_add_dir (prefs->client, NAUTILUS_ACTIONS_CONFIG_GCONF_BASEDIR, GCONF_CLIENT_PRELOAD_NONE, NULL);
		prefs->prefs_notify_id = gconf_client_notify_add (prefs->client, NAUTILUS_ACTIONS_CONFIG_GCONF_BASEDIR,
									  (GConfClientNotifyFunc) prefs_changed_cb, prefs,
									  NULL, NULL);
	}

	return prefs;
}

static void
copy_to_list (gpointer value, gpointer user_data)
{
	GSList **list = user_data;

	(*list) = g_slist_append ((*list), g_strdup ((gchar*)value));
}

GSList* nact_prefs_get_schemes_list (void)
{
	GSList* new_list = NULL;
	NactPreferences* prefs = nact_prefs_get_preferences ();

	g_slist_foreach (prefs->schemes, (GFunc)copy_to_list, &new_list);

	return new_list;
}

void nact_prefs_set_schemes_list (GSList* schemes)
{
	NactPreferences* prefs = nact_prefs_get_preferences ();

	if (prefs->schemes)
	{
		g_slist_foreach (prefs->schemes, (GFunc) g_free, NULL);
		g_slist_free (prefs->schemes);
		prefs->schemes = NULL;
	}

	g_slist_foreach (schemes, (GFunc)copy_to_list, &(prefs->schemes));
}

gboolean nact_prefs_get_main_dialog_size (gint* width, gint* height)
{
	gboolean retv = FALSE;
	NactPreferences* prefs = nact_prefs_get_preferences ();

	if (prefs->main_size_width != -1 && prefs->main_size_height != -1)
	{
		retv = TRUE;
		(*width) = prefs->main_size_width;
		(*height) = prefs->main_size_height;
	}

	return retv;
}

void nact_prefs_set_main_dialog_size (GtkWindow* dialog)
{
	NactPreferences* prefs = nact_prefs_get_preferences ();

	gtk_window_get_size (dialog, &(prefs->main_size_width), &(prefs->main_size_height));
}

gboolean nact_prefs_get_edit_dialog_size (gint* width, gint* height)
{
	gboolean retv = FALSE;
	NactPreferences* prefs = nact_prefs_get_preferences ();

	if (prefs->edit_size_width != -1 && prefs->edit_size_height != -1)
	{
		retv = TRUE;
		(*width) = prefs->edit_size_width;
		(*height) = prefs->edit_size_height;
	}

	return retv;

}

void nact_prefs_set_edit_dialog_size (GtkWindow* dialog)
{
	NactPreferences* prefs = nact_prefs_get_preferences ();

	gtk_window_get_size (dialog, &(prefs->edit_size_width), &(prefs->edit_size_height));
}

gboolean nact_prefs_get_im_ex_dialog_size (gint* width, gint* height)
{
	gboolean retv = FALSE;
	NactPreferences* prefs = nact_prefs_get_preferences ();

	if (prefs->im_ex_size_width != -1 && prefs->im_ex_size_height != -1)
	{
		retv = TRUE;
		(*width) = prefs->im_ex_size_width;
		(*height) = prefs->im_ex_size_height;
	}

	return retv;

}

void nact_prefs_set_im_ex_dialog_size (GtkWindow* dialog)
{
	NactPreferences* prefs = nact_prefs_get_preferences ();

	gtk_window_get_size (dialog, &(prefs->im_ex_size_width), &(prefs->im_ex_size_height));
}

gboolean nact_prefs_get_main_dialog_position (gint* x, gint* y)
{
	gboolean retv = FALSE;
	NactPreferences* prefs = nact_prefs_get_preferences ();

	if (prefs->main_position_x != -1 && prefs->main_position_y != -1)
	{
		retv = TRUE;
		(*x) = prefs->main_position_x;
		(*y) = prefs->main_position_y;
	}

	return retv;
}

void nact_prefs_set_main_dialog_position (GtkWindow* dialog)
{
	NactPreferences* prefs = nact_prefs_get_preferences ();

	gtk_window_get_position (dialog, &(prefs->main_position_x), &(prefs->main_position_y));
}

gboolean nact_prefs_get_edit_dialog_position (gint* x, gint* y)
{
	gboolean retv = FALSE;
	NactPreferences* prefs = nact_prefs_get_preferences ();

	if (prefs->edit_position_x != -1 && prefs->edit_position_y != -1)
	{
		retv = TRUE;
		(*x) = prefs->edit_position_x;
		(*y) = prefs->edit_position_y;
	}

	return retv;
}

void nact_prefs_set_edit_dialog_position (GtkWindow* dialog)
{
	NactPreferences* prefs = nact_prefs_get_preferences ();

	gtk_window_get_position (dialog, &(prefs->edit_position_x), &(prefs->edit_position_y));
}

gboolean nact_prefs_get_im_ex_dialog_position (gint* x, gint* y)
{
	gboolean retv = FALSE;
	NactPreferences* prefs = nact_prefs_get_preferences ();

	if (prefs->im_ex_position_x != -1 && prefs->im_ex_position_y != -1)
	{
		retv = TRUE;
		(*x) = prefs->im_ex_position_x;
		(*y) = prefs->im_ex_position_y;
	}

	return retv;
}

void nact_prefs_set_im_ex_dialog_position (GtkWindow* dialog)
{
	NactPreferences* prefs = nact_prefs_get_preferences ();

	gtk_window_get_position (dialog, &(prefs->im_ex_position_x), &(prefs->im_ex_position_y));
}

gchar* nact_prefs_get_icon_last_browsed_dir (void)
{
	NactPreferences* prefs = nact_prefs_get_preferences ();

	return g_strdup (prefs->icon_last_browsed_dir);
}

void nact_prefs_set_icon_last_browsed_dir (const gchar* path)
{
	NactPreferences* prefs = nact_prefs_get_preferences ();

	if (prefs->icon_last_browsed_dir)
	{
		g_free (prefs->icon_last_browsed_dir);
	}
	prefs->icon_last_browsed_dir = g_strdup (path);
}

gchar* nact_prefs_get_path_last_browsed_dir (void)
{
	NactPreferences* prefs = nact_prefs_get_preferences ();

	return g_strdup (prefs->path_last_browsed_dir);
}

void nact_prefs_set_path_last_browsed_dir (const gchar* path)
{
	NactPreferences* prefs = nact_prefs_get_preferences ();

	if (prefs->path_last_browsed_dir)
	{
		g_free (prefs->path_last_browsed_dir);
	}
	prefs->path_last_browsed_dir = g_strdup (path);
}

gchar* nact_prefs_get_import_last_browsed_dir (void)
{
	NactPreferences* prefs = nact_prefs_get_preferences ();

	return g_strdup (prefs->import_last_browsed_dir);
}

void nact_prefs_set_import_last_browsed_dir (const gchar* path)
{
	NactPreferences* prefs = nact_prefs_get_preferences ();

	if (prefs->import_last_browsed_dir)
	{
		g_free (prefs->import_last_browsed_dir);
	}
	prefs->import_last_browsed_dir = g_strdup (path);
}

gchar* nact_prefs_get_export_last_browsed_dir (void)
{
	NactPreferences* prefs = nact_prefs_get_preferences ();

	return g_strdup (prefs->export_last_browsed_dir);
}

void nact_prefs_set_export_last_browsed_dir (const gchar* path)
{
	NactPreferences* prefs = nact_prefs_get_preferences ();

	if (prefs->export_last_browsed_dir)
	{
		g_free (prefs->export_last_browsed_dir);
	}
	prefs->export_last_browsed_dir = g_strdup (path);
}

static void nact_prefs_free_preferences (NactPreferences* prefs)
{
	if (prefs)
	{
		if (prefs->schemes)
		{
			g_slist_foreach (prefs->schemes, (GFunc) g_free, NULL);
			g_slist_free (prefs->schemes);
		}

		if (prefs->icon_last_browsed_dir)
		{
			g_free (prefs->icon_last_browsed_dir);
		}

		if (prefs->path_last_browsed_dir)
		{
			g_free (prefs->path_last_browsed_dir);
		}

		if (prefs->import_last_browsed_dir)
		{
			g_free (prefs->import_last_browsed_dir);
		}

		if (prefs->export_last_browsed_dir)
		{
			g_free (prefs->export_last_browsed_dir);
		}

		gconf_client_remove_dir (prefs->client, NAUTILUS_ACTIONS_CONFIG_GCONF_BASEDIR, NULL);
		gconf_client_notify_remove (prefs->client, prefs->prefs_notify_id);
		g_object_unref (prefs->client);

		g_free (prefs);
		prefs = NULL;
	}
}

void nact_prefs_save_preferences (void)
{
	NactPreferences* prefs = nact_prefs_get_preferences ();

	/* Save preferences in GConf */
	set_prefs_list_key (prefs->client, PREFS_SCHEMES, prefs->schemes);
	set_prefs_int_key (prefs->client, PREFS_MAIN_W, prefs->main_size_width);
	set_prefs_int_key (prefs->client, PREFS_MAIN_H, prefs->main_size_height);
	set_prefs_int_key (prefs->client, PREFS_EDIT_W, prefs->edit_size_width);
	set_prefs_int_key (prefs->client, PREFS_EDIT_H, prefs->edit_size_height);
	set_prefs_int_key (prefs->client, PREFS_IM_EX_W, prefs->im_ex_size_width);
	set_prefs_int_key (prefs->client, PREFS_IM_EX_H, prefs->im_ex_size_height);
	set_prefs_int_key (prefs->client, PREFS_MAIN_X, prefs->main_position_x);
	set_prefs_int_key (prefs->client, PREFS_MAIN_Y, prefs->main_position_y);
	set_prefs_int_key (prefs->client, PREFS_EDIT_X, prefs->edit_position_x);
	set_prefs_int_key (prefs->client, PREFS_EDIT_Y, prefs->edit_position_y);
	set_prefs_int_key (prefs->client, PREFS_IM_EX_X, prefs->im_ex_position_x);
	set_prefs_int_key (prefs->client, PREFS_IM_EX_Y, prefs->im_ex_position_y);
	set_prefs_string_key (prefs->client, PREFS_ICON_PATH, prefs->icon_last_browsed_dir);
	set_prefs_string_key (prefs->client, PREFS_PATH_PATH, prefs->path_last_browsed_dir);
	set_prefs_string_key (prefs->client, PREFS_IMPORT_PATH, prefs->import_last_browsed_dir);
	set_prefs_string_key (prefs->client, PREFS_EXPORT_PATH, prefs->export_last_browsed_dir);

	nact_prefs_free_preferences (prefs);
}
