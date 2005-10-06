/* Nautilus Actions configuration tool
 * Copyright (C) 2005 The GNOME Foundation
 *
 * Authors:
 *  Frederic Ruaudel (grumz@grumz.net)
 *	 Rodrigo Moya (rodrigo@gnome-db.org)
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

#include <libnautilus-extension/nautilus-file-info.h>

#include "nautilus-actions-test.h"

static int nautilus_actions_test_check_scheme (GSList* schemes2test, NautilusFileInfo* file)
{
	int retv = 0;
	GSList* iter;
	gboolean found = FALSE;
	
	iter = schemes2test; 
	while (iter && !found)
	{
		gchar* scheme = nautilus_file_info_get_uri_scheme (file);
		
		if (g_ascii_strncasecmp (scheme, (gchar*)iter->data, strlen ((gchar*)iter->data)) == 0)
		{
			found = TRUE;
			retv = 1;
		}
		
		g_free (scheme);
		iter = iter->next;
	}

	return retv;
}

gboolean nautilus_actions_test_validate (NautilusActionsConfigAction *action, GList* files)
{
	gboolean retv = FALSE;
	gboolean test_multiple_file = FALSE;
	gboolean test_file_type = FALSE;
	gboolean test_scheme = FALSE;
	gboolean test_basename = FALSE;
	GList* glob_patterns = NULL;
	GSList* iter;
	GList* iter1;
	GList* iter2;
	guint dir_count = 0;
	guint file_count = 0;
	guint total_count = 0;
	guint scheme_ok_count = 0;
	guint glob_ok_count = 0;
	gboolean basename_match_ok = FALSE;

	if (action->basenames && action->basenames->next != NULL && 
			g_ascii_strncasecmp ((gchar*)(action->basenames->data), "*", 1) == 0)
	{
		// if the only pattern is '*' then all files will match, so it is not 
		// necessary to make the test for each of them
		test_basename = TRUE;
	}
	else
	{
		for (iter = action->basenames; iter; iter = iter->next)
		{
			glob_patterns = g_list_append (glob_patterns, g_pattern_spec_new ((gchar*)iter->data));
		}
	}
	
	for (iter1 = files; iter1; iter1 = iter1->next)
	{
		gchar* tmp_filename = nautilus_file_info_get_name ((NautilusFileInfo *)iter1->data);
		
		if (nautilus_file_info_is_directory ((NautilusFileInfo *)iter1->data))
		{
			dir_count++;
		}
		else
		{
			file_count++;
		}

		scheme_ok_count += nautilus_actions_test_check_scheme (action->schemes, (NautilusFileInfo*)iter1->data);

		if (!test_basename) // if it is already ok, skip the test to improve performance
		{
			basename_match_ok = FALSE;
			iter2 = glob_patterns;
			while (iter2 && !basename_match_ok)
			{
				if (g_pattern_match_string ((GPatternSpec*)iter2->data, tmp_filename))
				{
					basename_match_ok = TRUE;
				}
				iter2 = iter2->next;
			}

			if (basename_match_ok)
			{
				glob_ok_count++;
			}
		}

		g_free (tmp_filename);

		total_count++;
	}

	if ((files != NULL) && (files->next == NULL) && (!action->accept_multiple_files))
	{
		test_multiple_file = TRUE;
	}
	else if (action->accept_multiple_files)
	{
		test_multiple_file = TRUE;
	}

	if (action->is_dir && action->is_file)
	{
		if (dir_count > 0 || file_count > 0)
		{
			test_file_type = TRUE;
		}
	}
	else if (action->is_dir && !action->is_file)
	{
		if (file_count == 0)
		{
			test_file_type = TRUE;
		}
	}
	else if (!action->is_dir && action->is_file)
	{
		if (dir_count == 0)
		{
			test_file_type = TRUE;
		}
	}

	if (scheme_ok_count == total_count)
	{
		test_scheme = TRUE;
	}


	if (!test_basename) // if not already tested
	{
		if (glob_ok_count == total_count)
		{
			test_basename = TRUE;
		}
	}

	if (test_basename && test_file_type && test_scheme && test_multiple_file)
	{
		retv = TRUE;
	}
	
	g_list_foreach (glob_patterns, (GFunc) g_pattern_spec_free, NULL);
	g_list_free (glob_patterns);

	return retv;
}

// vim:ts=3:sw=3:tw=1024:ai
