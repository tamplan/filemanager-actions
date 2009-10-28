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

#include <glib.h>
#include <glib/gi18n.h>
#include <stdlib.h>

#include <common/na-action.h>
#include <common/na-action-profile.h>
#include <common/na-gconf.h>
#include <common/na-iio-provider.h>
#include <common/na-xml-names.h>
#include <common/na-xml-writer.h>

#include "console-utils.h"

static gchar     *label           = "";
static gchar     *tooltip         = "";
static gchar     *icon            = "";
static gboolean   enabled         = TRUE;
static gchar     *command         = "";
static gchar     *parameters      = "";
static gchar    **basenames_array = NULL;
static gboolean   matchcase       = FALSE;
static gchar    **mimetypes_array = NULL;
static gboolean   isfile          = FALSE;
static gboolean   isdir           = FALSE;
static gboolean   accept_multiple = FALSE;
static gchar    **schemes_array   = NULL;
static gchar     *output_dir      = NULL;
static gboolean   output_gconf    = FALSE;

static GOptionEntry entries[] = {

	{ "label"                , 'l', 0, G_OPTION_ARG_STRING      , &label          ,	N_("The label of the menu item (mandatory)"), N_("LABEL") },
	{ "tooltip"              , 't', 0, G_OPTION_ARG_STRING      , &tooltip        , N_("The tooltip of the menu item"), N_("TOOLTIP") },
	{ "icon"                 , 'i', 0, G_OPTION_ARG_STRING      , &icon           , N_("The icon of the menu item (filename or GTK stock ID)"), N_("ICON") },
	{ "enabled"              , 'e', 0, G_OPTION_ARG_NONE        , &enabled        , N_("Whether the action is enabled"), NULL },
	{ "command"              , 'c', 0, G_OPTION_ARG_FILENAME    , &command        , N_("The path of the command"), N_("PATH") },
	{ "parameters"           , 'p', 0, G_OPTION_ARG_STRING      , &parameters     , N_("The parameters of the command"), N_("PARAMS") },
	{ "match"                , 'm', 0, G_OPTION_ARG_STRING_ARRAY, &basenames_array, N_("A pattern to match selected files against. May include wildcards (* or ?) (you must set one option for each pattern you need)"), N_("EXPR") },
	{ "match-case"           , 'C', 0, G_OPTION_ARG_NONE        , &matchcase      , N_("The path of the command"), N_("PATH") },
	{ "mimetypes"            , 'T', 0, G_OPTION_ARG_STRING_ARRAY, &mimetypes_array, N_("A pattern to match selected files' mimetype against. May include wildcards (* or ?) (you must set one option for each pattern you need)"), N_("EXPR") },
	{ "accept-files"         , 'f', 0, G_OPTION_ARG_NONE        , &isfile         , N_("Set it if the selection must only contain files"), NULL },
	{ "accept-dirs"          , 'd', 0, G_OPTION_ARG_NONE        , &isdir          , N_("Set it if the selection must only contain folders. Specify both '--isfile' and '--isdir' options is selection can contain both types of items"), NULL },
	{ "accept-multiple-files", 'M', 0, G_OPTION_ARG_NONE        , &accept_multiple, N_("Set it if the selection can have several items"), NULL },
	{ "scheme"               , 's', 0, G_OPTION_ARG_STRING_ARRAY, &schemes_array  , N_("A valid GVFS scheme where the selected files should be located (you must set one option for each scheme you need)"), N_("SCHEME") },
	{ NULL }
};

static GOptionEntry output_entries[] = {

	{ "output-gconf"         , 'g', 0, G_OPTION_ARG_NONE        , &output_gconf   , N_("Directly import the newly created action in GConf configuration"), NULL },
	{ "output-dir"           , 'o', 0, G_OPTION_ARG_FILENAME    , &output_dir     , N_("The folder where to write the new action as a GConf dump output [default: stdout]"), N_("DIR") },
	{ NULL }
};

static GOptionContext *init_options( void );
static NAAction       *get_action_from_cmdline( void );
static gboolean        write_to_gconf( NAAction *action, gchar **msg );
static void            exit_with_usage( void );

int
main( int argc, char** argv )
{
	int status = EXIT_SUCCESS;
	GOptionContext *context;
	GError *error = NULL;
	NAAction *action;
	gchar *msg = NULL;
	gchar *help;

	g_type_init();
	console_init_log_handler();

	context = init_options();

	if( argc == 1 ){
		g_set_prgname( argv[0] );
		help = g_option_context_get_help( context, FALSE, NULL );
		g_print( "\n%s", help );
		g_free( help );
		exit( status );
	}

	if( !g_option_context_parse( context, &argc, &argv, &error )){
		g_printerr( _("Syntax error: %s\n" ), error->message );
		g_error_free (error);
		exit_with_usage();
	}

	if( !label || !g_utf8_strlen( label, -1 )){
		g_printerr( _( "Error: an action label is mandatory." ));
		exit_with_usage();
	}

	if( output_gconf && output_dir ){
		g_printerr( _( "Error: only one output option may be specified." ));
		exit_with_usage();
	}

	action = get_action_from_cmdline();

	if( output_gconf ){
		if( write_to_gconf( action, &msg )){
			/* i18n: Action <action_label> written to...*/
			g_print( _( "Action '%s' succesfully written to GConf configuration.\n" ), label );
		}

	} else {
		gchar * output_fname = na_xml_writer_export( action, output_dir, FORMAT_GCONFENTRY, &msg );
		if( output_fname ){
			/* i18n: Action <action_label> written to <output_filename>...*/
			g_print( _( "Action '%s' succesfully written to %s, and ready to be imported in NACT.\n" ), label, output_fname );
			g_free( output_fname );
		}
	}

	if( msg ){
		g_printerr( "%s\n", msg );
		g_free( msg );
		status = EXIT_FAILURE;
	}

	g_object_unref( action );
	g_option_context_free( context );

	exit( status );
}

/*
 * init options context
 */
static GOptionContext *
init_options( void )
{
	GOptionContext *context;
	gchar* description;
	GOptionGroup *output_group;

	context = g_option_context_new( _( "Define a new action.\n\n"
			"  The created action defaults to be written to stdout.\n"
			"  It can also be written to an output folder, in a file later suitable for an import in NACT.\n"
			"  Or you may choose to directly write the action into your GConf configuration." ));

#ifdef ENABLE_NLS
	bindtextdomain( GETTEXT_PACKAGE, GNOMELOCALEDIR );
# ifdef HAVE_BIND_TEXTDOMAIN_CODESET
	bind_textdomain_codeset( GETTEXT_PACKAGE, "UTF-8" );
# endif
	textdomain( GETTEXT_PACKAGE );
	g_option_context_add_main_entries( context, entries, GETTEXT_PACKAGE );
#else
	g_option_context_add_main_entries( context, entries, NULL );
#endif

	description = g_strdup_printf( "%s.\n%s", PACKAGE_STRING,
			_( "Bug reports are welcomed at http://bugzilla.gnome.org,"
				" or you may prefer to mail to <maintainer@nautilus-actions.org>.\n" ));

	g_option_context_set_description( context, description );

	g_free( description );

	output_group = g_option_group_new(
			"output", _( "Output of the program" ), _( "Choose where the program creates the action" ), NULL, NULL );

	g_option_group_add_entries( output_group, output_entries );

	g_option_context_add_group( context, output_group );

	return( context );
}

/*
 * allocate a new action, and fill it with values readen from command-line
 */
static NAAction *
get_action_from_cmdline( void )
{
	NAAction *action = na_action_new_with_profile();
	NAActionProfile *profile = NA_ACTION_PROFILE( na_action_get_profiles( action )->data );
	int i = 0;
	GSList *basenames = NULL;
	GSList *mimetypes = NULL;
	GSList *schemes = NULL;

	na_action_set_label( action, label );
	na_action_set_tooltip( action, tooltip );
	na_action_set_icon( action, icon );
	na_action_set_enabled( action, enabled );

	na_action_profile_set_path( profile, command );
	na_action_profile_set_parameters( profile, parameters );

	while( basenames_array != NULL && basenames_array[i] != NULL ){
		basenames = g_slist_append( basenames, g_strdup( basenames_array[i] ));
		i++;
	}
	na_action_profile_set_basenames( profile, basenames );
	g_slist_foreach( basenames, ( GFunc ) g_free, NULL );
	g_slist_free( basenames );

	na_action_profile_set_matchcase( profile, matchcase );

	i = 0;
	while( mimetypes_array != NULL && mimetypes_array[i] != NULL ){
		mimetypes = g_slist_append( mimetypes, g_strdup( mimetypes_array[i] ));
		i++;
	}
	na_action_profile_set_mimetypes( profile, mimetypes );
	g_slist_foreach( mimetypes, ( GFunc ) g_free, NULL );
	g_slist_free( mimetypes );

	if( !isfile && !isdir ){
		isfile = TRUE;
	}
	na_action_profile_set_isfile( profile, isfile );
	na_action_profile_set_isdir( profile, isdir );
	na_action_profile_set_multiple( profile, accept_multiple );

	i = 0;
	while( schemes_array != NULL && schemes_array[i] != NULL ){
		schemes = g_slist_append( schemes, g_strdup( schemes_array[i] ));
		i++;
	}
	na_action_profile_set_schemes( profile, schemes );
	g_slist_foreach( schemes, ( GFunc ) g_free, NULL );
	g_slist_free( schemes );

	return( action );
}

/*
 * initialize GConf as an I/O provider
 * then writes the action
 */
static gboolean
write_to_gconf( NAAction *action, gchar **msg )
{
	NAGConf *gconf;
	guint ret;

	gconf = na_gconf_new( NULL );

	na_action_set_provider( action, NA_IIO_PROVIDER( gconf ));

	ret = na_iio_provider_write_action( NULL, action, msg );

	return( ret == NA_IIO_PROVIDER_WRITE_OK );
}

/*
 * print a help message and exit with failure
 */
static void
exit_with_usage( void )
{
	g_printerr( _("Try %s --help for usage.\n"), g_get_prgname());
	exit( EXIT_FAILURE );
}
