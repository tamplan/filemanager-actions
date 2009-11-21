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
#include <glib.h>
#include <glib/gi18n.h>
#include <stdlib.h>

#include <runtime/na-iprefs.h>
#include <runtime/na-utils.h>
#include <runtime/na-xml-names.h>
#include <runtime/na-xml-writer.h>

#include "console-utils.h"

/*static gchar     *output_fname = NULL;
static gboolean   output_gconf = FALSE;*/
static gboolean   output_stdout = FALSE;
static gboolean   version       = FALSE;

static GOptionEntry entries[] = {

	/*{ "output-gconf"         , 'g', 0, G_OPTION_ARG_NONE    , &output_gconf   , N_("Writes the Nautilus Actions schema in GConf"), NULL },
	{ "output-filename"      , 'o', 0, G_OPTION_ARG_FILENAME, &output_fname   , N_("The file where to write the GConf schema ('-' for stdout)"), N_("FILENAME") },*/
	{ "stdout", 's', 0, G_OPTION_ARG_NONE, &output_stdout, N_("Output the schema on stdout"), NULL },
	{ NULL }
};

static GOptionEntry misc_entries[] = {

	{ "version"              , 'v', 0, G_OPTION_ARG_NONE        , &version,
			N_("Output the version number"), NULL },
	{ NULL }
};

static GOptionContext *init_options( void );
/*static gboolean        write_to_gconf( gchar **msg );
static gboolean        write_schema( GConfClient *gconf, const gchar *prefix, GConfValueType type, const gchar *entry, const gchar *dshort, const gchar *dlong, const gchar *default_value, gchar **msg );*/
static void            exit_with_usage( void );

int
main( int argc, char** argv )
{
	int status = EXIT_SUCCESS;
	GOptionContext *context;
	gchar *help;
	GError *error = NULL;
	GSList *msg = NULL;
	GSList *im;

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

	if( version ){
		na_utils_print_version();
		exit( status );
	}

	/*if( output_gconf && output_fname ){
		g_printerr( _( "Error: only one output option may be specified." ));
		exit_with_usage();
	}*/

	/*if( output_gconf ){
		if( write_to_gconf( &msg )){
			g_print( _( "Nautilus Actions schema succesfully written to GConf.\n" ));
		}

	} else {*/
		na_xml_writer_export( NULL, NULL, IPREFS_EXPORT_FORMAT_GCONF_SCHEMA, &msg );
		/*if( !msg ){
			g_print( _( "Nautilus Actions schema succesfully written to %s.\n" ), output_fname );
			g_free( output_fname );
		}*/
	/*}*/

	if( msg ){
		for( im = msg ; im ; im = im->next ){
			g_printerr( "%s\n", ( gchar * ) im->data );
		}
		na_utils_free_string_list( msg );
		status = EXIT_FAILURE;
	}

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
	gchar *description;
	GOptionGroup *misc_group;

	context = g_option_context_new( _( "Output the Nautilus Actions GConf schema on stdout." ));
			/*"  The schema can be written to stdout.\n"
			"  It can also be written to an output file, in a file later suitable for an installation via gconftool-2.\n"
			"  Or you may choose to directly write the schema into the GConf configuration." ));*/

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

	misc_group = g_option_group_new(
			"misc", _( "Miscellaneous options" ), _( "Miscellaneous options" ), NULL, NULL );
	g_option_group_add_entries( misc_group, misc_entries );
	g_option_context_add_group( context, misc_group );

	return( context );
}

/*
 * writes the schema via GConfClient
 */
/*static gboolean
write_to_gconf( gchar **msg )
{
	GConfClient *gconf = gconf_client_get_default();

	gchar *prefix_config = g_strdup_printf( "%s%s", NAUTILUS_ACTIONS_GCONF_SCHEMASDIR, NA_GCONF_CONFIG_PATH );
	gchar *prefix_prefs = g_strdup_printf( "%s%s/%s", NAUTILUS_ACTIONS_GCONF_SCHEMASDIR, NAUTILUS_ACTIONS_GCONF_BASEDIR, NA_GCONF_PREFERENCES );

	gboolean ret =
		write_schema( gconf, prefix_config, GCONF_VALUE_STRING, ACTION_VERSION_ENTRY, ACTION_VERSION_DESC_SHORT, ACTION_VERSION_DESC_LONG, NAUTILUS_ACTIONS_CONFIG_VERSION, msg ) &&
		write_schema( gconf, prefix_config, GCONF_VALUE_STRING, ACTION_LABEL_ENTRY, ACTION_LABEL_DESC_SHORT, ACTION_LABEL_DESC_LONG, "", msg ) &&
		write_schema( gconf, prefix_config, GCONF_VALUE_STRING, ACTION_TOOLTIP_ENTRY, ACTION_TOOLTIP_DESC_SHORT, ACTION_TOOLTIP_DESC_LONG, "", msg ) &&
		write_schema( gconf, prefix_config, GCONF_VALUE_STRING, ACTION_ICON_ENTRY, ACTION_ICON_DESC_SHORT, ACTION_ICON_DESC_LONG, "", msg ) &&
		write_schema( gconf, prefix_config, GCONF_VALUE_STRING, ACTION_PROFILE_LABEL_ENTRY, ACTION_PROFILE_NAME_DESC_SHORT, ACTION_PROFILE_NAME_DESC_LONG, NA_ACTION_PROFILE_DEFAULT_LABEL, msg ) &&
		write_schema( gconf, prefix_config, GCONF_VALUE_STRING, ACTION_PATH_ENTRY, ACTION_PATH_DESC_SHORT, ACTION_PATH_DESC_LONG, "", msg ) &&
		write_schema( gconf, prefix_config, GCONF_VALUE_STRING, ACTION_PARAMETERS_ENTRY, ACTION_PARAMETERS_DESC_SHORT, ACTION_PARAMETERS_DESC_LONG, "", msg ) &&
		write_schema( gconf, prefix_config, GCONF_VALUE_LIST, ACTION_BASENAMES_ENTRY, ACTION_BASENAMES_DESC_SHORT, ACTION_BASENAMES_DESC_LONG, "*", msg ) &&
		write_schema( gconf, prefix_config, GCONF_VALUE_BOOL, ACTION_MATCHCASE_ENTRY, ACTION_MATCHCASE_DESC_SHORT, ACTION_MATCHCASE_DESC_LONG, "true", msg ) &&
		write_schema( gconf, prefix_config, GCONF_VALUE_LIST, ACTION_MIMETYPES_ENTRY, ACTION_MIMETYPES_DESC_SHORT, ACTION_MIMETYPES_DESC_LONG, "*
		/
		 *", msg ) &&
		write_schema( gconf, prefix_config, GCONF_VALUE_BOOL, ACTION_ISFILE_ENTRY, ACTION_ISFILE_DESC_SHORT, ACTION_ISFILE_DESC_LONG, "true", msg ) &&
		write_schema( gconf, prefix_config, GCONF_VALUE_BOOL, ACTION_ISDIR_ENTRY, ACTION_ISDIR_DESC_SHORT, ACTION_ISDIR_DESC_LONG, "false", msg ) &&
		write_schema( gconf, prefix_config, GCONF_VALUE_BOOL, ACTION_MULTIPLE_ENTRY, ACTION_MULTIPLE_DESC_SHORT, ACTION_MULTIPLE_DESC_LONG, "false", msg ) &&
		write_schema( gconf, prefix_config, GCONF_VALUE_LIST, ACTION_SCHEMES_ENTRY, ACTION_SCHEMES_DESC_SHORT, ACTION_SCHEMES_DESC_LONG, "file", msg );

	g_free( prefix_prefs );
	g_free( prefix_config );

	gconf_client_suggest_sync( gconf, NULL );
	return( ret );
}

static gboolean
write_schema( GConfClient *gconf, const gchar *prefix, GConfValueType type, const gchar *entry, const gchar *dshort, const gchar *dlong, const gchar *default_value, gchar **message )
{
	gchar *path = g_strdup_printf( "%s/%s", prefix, entry );
	g_debug( "write_schema: path=%s", path );
	gboolean ret = TRUE;
	GError *error = NULL;

	GConfSchema *schema = gconf_schema_new();
	gconf_schema_set_owner( schema, PACKAGE );
	gconf_schema_set_type( schema, type );
*/
	/* FIXME: if we write the schema with a 'C' locale, how will it be
	 * localized ?? but get_language_names return a list. Do we have to
	 * write a locale for each element of the list ? for the first one ?
	 */
	/*gconf_schema_set_locale( schema, "C" );

	gconf_schema_set_short_desc( schema, dshort );
	gconf_schema_set_long_desc( schema, dlong );


	GConfValue *value = NULL;
	if( type == GCONF_VALUE_LIST ){
		gconf_schema_set_list_type( schema, GCONF_VALUE_STRING );

		GConfValue *first = gconf_value_new_from_string( GCONF_VALUE_STRING, default_value, &error );
		GSList *list = NULL;
		list = g_slist_append( list, first );
		value = gconf_value_new( GCONF_VALUE_LIST );
		gconf_value_set_list_type( value, GCONF_VALUE_STRING );
		gconf_value_set_list( value, list );
		g_slist_free( list );

	} else {
		value = gconf_value_new_from_string( type, default_value, &error );
		if( error ){
			*message = g_strdup( error->message );
			g_error_free( error );
			ret = FALSE;
		}
	}

	if( ret ){
		gconf_schema_set_default_value( schema, value );

		if( !gconf_client_set_schema( gconf, path, schema, &error )){
			*message = g_strdup( error->message );
			g_error_free( error );
			ret = FALSE;
		}
	}

	gconf_schema_free( schema );
	g_free( path );
	return( ret );
}*/

/*
 * print a help message and exit with failure
 */
static void
exit_with_usage( void )
{
	g_printerr( _("Try %s --help for usage.\n"), g_get_prgname());
	exit( EXIT_FAILURE );
}
