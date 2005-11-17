/* Nautilus Actions conversion tool
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
#include <stdlib.h>
#include <stdio.h>
#include <glib/gi18n.h>
#include <libnautilus-actions/nautilus-actions-config.h>
#include <libnautilus-actions/nautilus-actions-config-xml.h>
#include <libnautilus-actions/nautilus-actions-config-schema-writer.h>

static gchar* input_file = NULL;
static gchar* output_file = NULL;
static gchar* output_dir = "/tmp";
static gboolean convert_all = FALSE;

static GOptionEntry entries[] =
{
	{ "input-file", 'i', 0, G_OPTION_ARG_FILENAME, &input_file, N_("The old xml config file to convert"), N_("FILE") },
	{ "output-file", 'o', 0, G_OPTION_ARG_FILENAME, &output_file, N_("The name of the new converted GConf schema file"), N_("FILE") },
	{ "all", 'a', 0, G_OPTION_ARG_NONE, &convert_all, N_("Convert all old xml config files from previous installations [default]"), NULL },
	{ "output-dir", 'd', 0, G_OPTION_ARG_FILENAME, &output_dir, N_("The folder where the new GConf schema files will be saved if -a options is set [default=/tmp]"), N_("DIR") },
	{ NULL }
};

int main (int argc, char** argv)
{
	GSList* iter;
	GError * error = NULL;
	GOptionContext* context;
	gchar* path;
	gboolean success = FALSE;
	gchar* contents = NULL;
	gssize length = 0;

	g_type_init ();

	context = g_option_context_new ("");

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
# ifdef HAVE_BIND_TEXTDOMAIN_CODESET
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
# endif
	textdomain (GETTEXT_PACKAGE);
	g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
#else
	g_option_context_add_main_entries (context, entries, NULL);
#endif

	g_option_context_parse (context, &argc, &argv, &error);

	if (error != NULL)
	{
		fprintf (stderr, _("Syntax error:\n\t- %s\nTry %s --help\n"), error->message, g_get_prgname ());
		exit (EXIT_FAILURE);
	}

	if (convert_all && (input_file || output_file))
	{
		fprintf (stderr, _("Syntax error:\n\t-i and -o options are mutually exclusive with -a option\nTry %s --help\n"), g_get_prgname ());
		exit (EXIT_FAILURE);
	}

	if (output_file && !input_file)
	{
		fprintf (stderr, _("Syntax error:\n\t-i option is mandatory when using -o option\nTry %s --help\n"), g_get_prgname ());
		exit (EXIT_FAILURE);
	}

	NautilusActionsConfigXml* xml_configs = nautilus_actions_config_xml_get ();
	NautilusActionsConfigSchemaWriter* schema_configs = nautilus_actions_config_schema_writer_get ();
	g_object_set (G_OBJECT (schema_configs), "save-path", output_dir, NULL);

	if (input_file)
	{
		if (!nautilus_actions_config_xml_parse_file (xml_configs, input_file))
		{
			fprintf (stderr, _("Error:\n\t- Can't parse %s\n"), input_file);
			exit (EXIT_FAILURE);
		}
	}
	else
	{
		nautilus_actions_config_xml_load_list (xml_configs);
	}

	GSList* actions = nautilus_actions_config_get_actions (NAUTILUS_ACTIONS_CONFIG (xml_configs));
	
	for (iter = actions; iter; iter = iter->next)
	{
		NautilusActionsConfigAction* action = (NautilusActionsConfigAction*)(iter->data);
		printf (_("Converting %s ..."), action->label);
		if (nautilus_actions_config_add_action (NAUTILUS_ACTIONS_CONFIG (schema_configs), action))
		{
			success = TRUE;
			path = nautilus_actions_config_schema_writer_get_saved_filename (schema_configs, action->uuid);
			if (output_file)
			{
				// Copy the content of the temporary file into the one asked by the user
				if (success = g_file_get_contents (path, &contents, &length, &error))
				{
					success = nautilus_actions_file_set_contents (output_file, contents, length, &error);
					
					g_free (contents);
				}

				//--> Remove the temporary file
				g_unlink (path);
				
				if (!success)
				{
					printf (_(" Failed: Can't create %s : %s\n"), output_file, error->message);
				}
				g_free (path);
				path = output_file;
			}

			if (success)
			{
				printf (_("  Ok, saved in %s\n"), path);
			}
		}
		else
		{
			printf (_("  Failed\n"));
		}
	}

	nautilus_actions_config_free_actions_list (actions);
	g_option_context_free (context);

	exit (EXIT_SUCCESS);
}

// vim:ts=3:sw=3:tw=1024:cin
