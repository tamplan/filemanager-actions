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

#include <glib.h>
#include <string.h>
/*#include <libgnomevfs/gnome-vfs-file-info.h>*/
#include <libnautilus-extension/nautilus-file-info.h>
#include <libgnomevfs/gnome-vfs.h>

#include "nautilus-actions-utils.h"

gchar* nautilus_actions_utils_parse_parameter (const gchar* param_template, GList* files)
{
	/*
	 * Valid parameters :
	 *
	 * %u : gnome-vfs URI
	 * %d : base dir of the selected file(s)/folder(s)
	 * %f : the name of the selected file/folder or the 1st one if many are selected
	 * %m : list of the basename of the selected files/directories separated by space.
	 * %M : list of the selected files/directories with their complete path separated by space.
	 * %s : scheme of the gnome-vfs URI
	 * %h : hostname of the gnome-vfs URI
	 * %U : username of the gnome-vfs URI
	 * %% : a percent sign
	 *
	 */
	gchar* retv = NULL;

	if (files != NULL)
	{
		GString* tmp_string = g_string_new ("");
		gchar* iter = g_strdup (param_template);
		gchar* old_iter = iter;
		/*int current_len = strlen (iter);*/
		gchar* uri = nautilus_file_info_get_uri ((NautilusFileInfo*)files->data);
		GnomeVFSURI* gvfs_uri = gnome_vfs_uri_new (uri);
		gchar* filename;
		gchar* dirname;
		gchar* scheme = nautilus_file_info_get_uri_scheme ((NautilusFileInfo*)files->data);
		gchar* hostname = g_strdup (gnome_vfs_uri_get_host_name (gvfs_uri));
		gchar* username = g_strdup (gnome_vfs_uri_get_user_name (gvfs_uri));
		gchar* file_list;
		gchar* path_file_list;
		GList* file_iter = NULL;
		GString* tmp_file_list;
		GString* tmp_path_file_list;
		gchar* tmp;
		gchar* tmp2;

		tmp = gnome_vfs_uri_extract_dirname (gvfs_uri);
		dirname = (gchar*)gnome_vfs_unescape_string ((const gchar*)tmp, "");
		g_free (tmp);

		tmp = nautilus_file_info_get_name ((NautilusFileInfo*)files->data);
		if (!tmp)
		{
			tmp = g_strdup ("");
		}

		filename = g_shell_quote (tmp);
		tmp2 = g_build_path ("/", dirname, tmp, NULL);
		g_free (tmp);
		tmp_file_list = g_string_new (filename);
		tmp = g_shell_quote (tmp2);
		tmp_path_file_list = g_string_new (tmp);
		g_free (tmp2);
		g_free (tmp);

		/* We already have the first item, so we start with the next one if any */
		for (file_iter = files->next; file_iter; file_iter = file_iter->next)
		{
			gchar* tmp_filename = nautilus_file_info_get_name ((NautilusFileInfo*)file_iter->data);
			gchar* tmp_uri = nautilus_file_info_get_uri ((NautilusFileInfo*)file_iter->data);
			GnomeVFSURI* tmp_gvfs_uri = gnome_vfs_uri_new (tmp_uri);
			tmp = gnome_vfs_uri_extract_dirname (tmp_gvfs_uri);
			gchar* tmp_dirname = (gchar*)gnome_vfs_unescape_string ((const gchar*)tmp, "");
			g_free (tmp);

			if (!tmp_filename)
			{
				tmp_filename = g_strdup ("");
			}

			gchar* quoted_tmp_filename = g_shell_quote (tmp_filename);
			g_string_append_printf (tmp_file_list, " %s", quoted_tmp_filename);

			tmp = g_build_path ("/", tmp_dirname, tmp_filename, NULL);
			tmp2 = g_shell_quote (tmp);
			g_string_append_printf (tmp_path_file_list, " %s", tmp2);

			g_free (tmp2);
			g_free (tmp);
			g_free (tmp_filename);
			g_free (quoted_tmp_filename);
			g_free (tmp_dirname);
			g_free (tmp_uri);
			gnome_vfs_uri_unref (tmp_gvfs_uri);
		}
		file_list = g_string_free (tmp_file_list, FALSE);
		path_file_list = g_string_free (tmp_path_file_list, FALSE);


		while ((iter = g_strstr_len (iter, strlen (iter), "%")))
		{
			tmp_string = g_string_append_len (tmp_string, old_iter, strlen (old_iter) - strlen (iter));
			switch (iter[1])
			{
				case 'u': /* gnome-vfs URI */
					tmp_string = g_string_append (tmp_string, uri);
					break;
				case 'd': /* base dir of the selected file(s)/folder(s) */
					tmp = g_shell_quote (dirname);
					tmp_string = g_string_append (tmp_string, tmp);
					g_free (tmp);
					break;
				case 'f': /* the basename of the selected file/folder or the 1st one if many are selected */
					tmp_string = g_string_append (tmp_string, filename);
					break;
				case 'm': /* list of the basename of the selected files/directories separated by space */
					tmp_string = g_string_append (tmp_string, file_list);
					break;
	 			case 'M': /* list of the selected files/directories with their complete path separated by space. */
					tmp_string = g_string_append (tmp_string, path_file_list);
					break;
				case 's': /* scheme of the gnome-vfs URI */
					tmp_string = g_string_append (tmp_string, scheme);
					break;
				case 'h': /* hostname of the gnome-vfs URI */
					tmp_string = g_string_append (tmp_string, hostname);
					break;
				case 'U': /* username of the gnome-vfs URI */
					tmp_string = g_string_append (tmp_string, username);
					break;
				case '%': /* a percent sign */
					tmp_string = g_string_append_c (tmp_string, '%');
					break;
			}
			iter+=2; /* skip the % sign and the character after. */
			old_iter = iter; /* store the new start of the string */
		}
		tmp_string = g_string_append_len (tmp_string, old_iter, strlen (old_iter));

		g_free (uri);
		g_free (dirname);
		g_free (filename);
		g_free (file_list);
		g_free (path_file_list);
		g_free (scheme);
		g_free (hostname);
		g_free (username);
		g_free (iter);
		gnome_vfs_uri_unref (gvfs_uri);

		retv = g_string_free (tmp_string, FALSE); /* return the content of the GString */
	}

	return retv;
}
