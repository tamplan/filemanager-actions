#include <libnautilus-extension/nautilus-file-info.h>

#include "nautilus-actions-test.h"

int nautilus_actions_test_check_scheme (GList* schemes2test, NautilusFileInfo* file)
{
	int retv = 0;
	GList* iter;
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

gboolean nautilus_actions_test_validate (ConfigActionTest *action_test, GList* files)
{
	gboolean retv = FALSE;
	gboolean test_multiple_file = FALSE;
	gboolean test_file_type = FALSE;
	gboolean test_scheme = FALSE;
	gboolean test_basename = FALSE;
	GPatternSpec* glob_pattern = g_pattern_spec_new (action_test->basename);
	GList* iter;
	int dir_count = 0;
	int file_count = 0;
	int total_count = 0;
	int scheme_ok_count = 0;
	int glob_ok_count = 0;

	if ((files != NULL) && (files->next == NULL) && (!action_test->accept_multiple_file))
	{
		test_multiple_file = TRUE;
	}
	else if (action_test->accept_multiple_file)
	{
		test_multiple_file = TRUE;
	}

	for (iter = files; iter; iter = iter->next)
	{
		gchar* tmp_filename = nautilus_file_info_get_name ((NautilusFileInfo *)iter->data);
		
		if (nautilus_file_info_is_directory ((NautilusFileInfo *)iter->data))
		{
			dir_count++;
		}
		else
		{
			file_count++;
		}

		scheme_ok_count += nautilus_actions_test_check_scheme (action_test->schemes, (NautilusFileInfo*)iter->data);


		//if (g_pattern_match (glob_pattern, strlen (tmp_filename), tmp_filename, g_strreverse (tmp_filename)))
		if (g_pattern_match_string (glob_pattern, tmp_filename))
		{
			glob_ok_count++;
		}

		g_free (tmp_filename);

		total_count++;
	}

	if (action_test->isdir && action_test->isfile)
	{
		if (dir_count > 0 || file_count > 0)
		{
			test_file_type = TRUE;
		}
	}
	else if (action_test->isdir && !action_test->isfile)
	{
		if (file_count == 0)
		{
			test_file_type = TRUE;
		}
	}
	else if (!action_test->isdir && action_test->isfile)
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


	if (glob_ok_count == total_count)
	{
		test_basename = TRUE;
	}

	if (test_basename && test_file_type && test_scheme && test_multiple_file)
	{
		retv = TRUE;
	}
	
	g_pattern_spec_free (glob_pattern);

	return retv;
}

// vim:ts=3:sw=3:tw=1024:ai
