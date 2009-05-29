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

#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <unistd.h>
#include <common/nautilus-actions-config.h>
#include <common/nautilus-actions-config-schema-writer.h>
#include "nautilus-actions-tools-utils.h"

static gchar* label = "";
static gchar* tooltip = "";
static gchar* icon = "";
static gchar* command = "";
static gchar* params = "";
static gchar** matches = NULL;
static gboolean match_case = FALSE;
static gchar** mimetypes = NULL;
static gboolean isfile = FALSE;
static gboolean isdir = FALSE;
static gboolean accept_multiple_files = FALSE;
static gchar** schemes = NULL;
static gchar* output_file = NULL;

static GOptionEntry entries[] =
{
	{ "label", 'l', 0, G_OPTION_ARG_STRING, &label, N_("The label of the menu item"), N_("LABEL") },
	{ "tooltip", 't', 0, G_OPTION_ARG_STRING, &tooltip, N_("The tooltip of the menu item"), N_("TOOLTIP") },
	{ "icon", 'i', 0, G_OPTION_ARG_STRING, &icon, N_("The icon of the menu item (filename or GTK stock ID)"), N_("ICON") },
	{ "command", 'c', 0, G_OPTION_ARG_FILENAME, &command, N_("The path of the command"), N_("PATH") },
	{ "parameters", 'p', 0, G_OPTION_ARG_STRING, &params, N_("The parameters of the command"), N_("PARAMS") },
	{ "match", 'm', 0, G_OPTION_ARG_STRING_ARRAY, &matches, N_("A pattern to match selected files against. May include wildcards (* or ?) (you must set one option for each pattern you need)"), N_("EXPR") },
	{ "match-case", 'C', 0, G_OPTION_ARG_NONE, &match_case, N_("The path of the command"), N_("PATH") },
	{ "mimetypes", 'T', 0, G_OPTION_ARG_STRING_ARRAY, &mimetypes, N_("A pattern to match selected files' mimetype against. May include wildcards (* or ?) (you must set one option for each pattern you need)"), N_("EXPR") },
	{ "accept-files", 'f', 0, G_OPTION_ARG_NONE, &isfile, N_("Set it if the selection can contain files"), NULL },
	{ "accept-dirs", 'd', 0, G_OPTION_ARG_NONE, &isdir, N_("Set it if the selection can contain folders"), NULL },
	{ "accept-multiple-files", 'M', 0, G_OPTION_ARG_NONE, &accept_multiple_files, N_("Set it if the selection can have several items"), NULL },
	{ "scheme", 's', 0, G_OPTION_ARG_STRING_ARRAY, &schemes, N_("A GnomeVFS scheme where the selected files should be located (you must set it for each scheme you need)"), N_("SCHEME") },
	{ "output-file", 'o', 0, G_OPTION_ARG_FILENAME, &output_file, N_("The path of the file where to save the new GConf schema definition file [default: /tmp/config_UUID.schemas]"), N_("PATH") },
	{ NULL }
};

int main (int argc, char** argv)
{
	/*GSList* iter;*/
	GError * error = NULL;
	GOptionContext* context;
	gchar* path;
	gboolean success = FALSE;
	gchar* contents = NULL;
	gsize length = 0;
	NautilusActionsConfigAction* action;
	NautilusActionsConfigActionProfile* action_profile;
	GSList* basenames = NULL;
	GSList* mimetypes_list = NULL;
	GSList* schemes_list = NULL;
	int i;

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
		g_error_free (error);
		exit (EXIT_FAILURE);
	}

	action = nautilus_actions_config_action_new_default ();
	action_profile = nautilus_actions_config_action_get_profile (action, NULL);
	nautilus_actions_config_action_set_label (action, label);
	nautilus_actions_config_action_set_tooltip (action, tooltip);
	nautilus_actions_config_action_set_icon (action, icon);
	nautilus_actions_config_action_profile_set_path (action_profile, command);
	nautilus_actions_config_action_profile_set_parameters (action_profile, params);

	i = 0;
	while (matches != NULL && matches[i] != NULL)
	{
		basenames = g_slist_append (basenames, g_strdup (matches[i]));
		i++;
	}
	nautilus_actions_config_action_profile_set_basenames (action_profile, basenames);
	g_slist_foreach (basenames, (GFunc) g_free, NULL);
	g_slist_free (basenames);

	nautilus_actions_config_action_profile_set_match_case (action_profile, match_case);

	i = 0;
	while (mimetypes != NULL && mimetypes[i] != NULL)
	{
		mimetypes_list = g_slist_append (mimetypes_list, g_strdup (mimetypes[i]));
		i++;
	}
	nautilus_actions_config_action_profile_set_mimetypes (action_profile, mimetypes_list);
	g_slist_foreach (mimetypes_list, (GFunc) g_free, NULL);
	g_slist_free (mimetypes_list);

	nautilus_actions_config_action_profile_set_is_file (action_profile, isfile);
	nautilus_actions_config_action_profile_set_is_dir (action_profile, isdir);
	nautilus_actions_config_action_profile_set_accept_multiple (action_profile, accept_multiple_files);

	i = 0;
	while (schemes != NULL && schemes[i] != NULL)
	{
		schemes_list = g_slist_append (schemes_list, g_strdup (schemes[i]));
		i++;
	}
	nautilus_actions_config_action_profile_set_schemes (action_profile, schemes_list);
	g_slist_foreach (schemes_list, (GFunc) g_free, NULL);
	g_slist_free (schemes_list);

	NautilusActionsConfigSchemaWriter* schema_configs = nautilus_actions_config_schema_writer_get ();
	g_object_set (G_OBJECT (schema_configs), "save-path", "/tmp", NULL);

	printf (_("Creating %s..."), action->label);
	if (nautilus_actions_config_add_action (NAUTILUS_ACTIONS_CONFIG (schema_configs), action, &error))
	{
		success = TRUE;
		path = nautilus_actions_config_schema_writer_get_saved_filename (schema_configs, action->uuid);
		if (output_file)
		{
			/* Copy the content of the temporary file into the one asked by the user */
			if ((success = g_file_get_contents (path, &contents, &length, &error)))
			{
				success = nautilus_actions_file_set_contents (output_file, contents, length, &error);
				g_free (contents);
			}

			/* --> Remove the temporary file */
			g_unlink (path);

			if (!success)
			{
				printf (_(" Failed: Can't create %s: %s\n"), output_file, error->message);
				g_error_free (error);
			}
			g_free (path);
			path = output_file;
		}

		if (success)
		{
			printf (_("  OK, saved in %s\n"), path);
		}
	}
	else
	{
		printf (_(" Failed: %s\n"), error->message);
		g_error_free (error);
	}

	nautilus_actions_config_action_free (action);
	g_option_context_free (context);

	exit (EXIT_SUCCESS);
}
