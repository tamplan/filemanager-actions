/* Nautilus Actions configuration tool
 * Copyright (C) 2005 The GNOME Foundation
 *
 * Authors:
 *  Frederic Ruaudel (grumz@grumz.net)
 *
 * This Program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this Library; see the file COPYING.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <glib/gi18n.h>
#include "nact-prefs.h"

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
#define PREFS_ICON_PATH		"nact_icon_last_browsed_dir"
#define PREFS_PATH_PATH		"nact_path_last_browsed_dir"

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
	NactPreferences* prefs = (NactPreferences*)user_data;

	if (user_data != NULL)
	{
		g_print ("Key changed : %s\n", entry->key);
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
			prefs->schemes = g_slist_append (prefs->schemes, g_strdup (_("file|Local Files")));
			prefs->schemes = g_slist_append (prefs->schemes, g_strdup (_("sftp|SSH Files")));
			prefs->schemes = g_slist_append (prefs->schemes, g_strdup (_("smb|Windows Files")));
			prefs->schemes = g_slist_append (prefs->schemes, g_strdup (_("ftp|FTP Files")));
			prefs->schemes = g_slist_append (prefs->schemes, g_strdup (_("dav|Webdav Files")));
		}
		prefs->main_size_width  = get_prefs_int_key (prefs->client, PREFS_MAIN_W);
		prefs->main_size_height = get_prefs_int_key (prefs->client, PREFS_MAIN_H);
		prefs->edit_size_width  = get_prefs_int_key (prefs->client, PREFS_EDIT_W);
		prefs->edit_size_height = get_prefs_int_key (prefs->client, PREFS_EDIT_H);
		prefs->main_position_x  = get_prefs_int_key (prefs->client, PREFS_MAIN_X);
		prefs->main_position_y  = get_prefs_int_key (prefs->client, PREFS_MAIN_Y);
		prefs->edit_position_x  = get_prefs_int_key (prefs->client, PREFS_EDIT_X);
		prefs->edit_position_y  = get_prefs_int_key (prefs->client, PREFS_EDIT_Y);
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

void nact_prefs_set_schemes_list (const GSList* schemes)
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
	set_prefs_int_key (prefs->client, PREFS_MAIN_X, prefs->main_position_x);
	set_prefs_int_key (prefs->client, PREFS_MAIN_Y, prefs->main_position_y);
	set_prefs_int_key (prefs->client, PREFS_EDIT_X, prefs->edit_position_x);
	set_prefs_int_key (prefs->client, PREFS_EDIT_Y, prefs->edit_position_y);
	set_prefs_string_key (prefs->client, PREFS_ICON_PATH, prefs->icon_last_browsed_dir);
	set_prefs_string_key (prefs->client, PREFS_PATH_PATH, prefs->path_last_browsed_dir);
	
	nact_prefs_free_preferences (prefs);
}

// vim:ts=3:sw=3:tw=1024:cin
