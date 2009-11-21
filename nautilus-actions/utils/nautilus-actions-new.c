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

#include <api/na-iio-provider.h>
#include <api/na-object-api.h>

#include <io-provider-gconf/nagp-keys.h>

#include <runtime/na-io-provider.h>
#include <runtime/na-iprefs.h>
#include <runtime/na-pivot.h>
#include <runtime/na-utils.h>
#include <runtime/na-xml-names.h>
#include <runtime/na-xml-writer.h>

#include "console-utils.h"

static gchar     *label            = "";
static gchar     *tooltip          = "";
static gchar     *icon             = "";
static gboolean   enabled          = FALSE;
static gboolean   disabled         = FALSE;
static gboolean   target_selection = FALSE;
static gboolean   target_folders   = FALSE;
static gboolean   target_toolbar   = FALSE;
static gchar     *label_toolbar    = "";
static gchar     *command          = "";
static gchar     *parameters       = "";
static gchar    **basenames_array  = NULL;
static gboolean   matchcase        = FALSE;
static gchar    **mimetypes_array  = NULL;
static gboolean   isfile           = FALSE;
static gboolean   isdir            = FALSE;
static gboolean   accept_multiple  = FALSE;
static gchar    **schemes_array    = NULL;
static gchar    **folders_array    = NULL;
static gchar     *output_dir       = NULL;
static gboolean   output_gconf     = FALSE;
static gboolean   version          = FALSE;

static GOptionEntry entries[] = {

	{ "label"                , 'l', 0, G_OPTION_ARG_STRING      , &label,
			N_( "The label of the menu item (mandatory)" ), N_( "<STRING>" ) },
	{ "tooltip"              , 't', 0, G_OPTION_ARG_STRING      , &tooltip,
			N_( "The tooltip of the menu item" ), N_( "<STRING>" ) },
	{ "icon"                 , 'i', 0, G_OPTION_ARG_STRING      , &icon,
			N_( "The icon of the menu item (filename or themed icon)" ), N_( "<PATH|NAME>" ) },
	{ "enabled"              , 'e', 0, G_OPTION_ARG_NONE        , &enabled,
			N_( "Set it if the action should be enabled [default]" ), NULL },
	{ "disabled"             , 'a', 0, G_OPTION_ARG_NONE        , &disabled,
			N_( "Set it if the action should be disabled at creation" ), NULL },
	{ "target-selection"     , 'S', 0, G_OPTION_ARG_NONE        , &target_selection,
			N_( "Set it if the action should be displayed in selection menus" ), NULL },
	{ "target-folders"       , 'F', 0, G_OPTION_ARG_NONE        , &target_folders,
			N_( "Set it if the action should be displayed in folders menus" ), NULL },
	{ "target-toolbar"       , 'O', 0, G_OPTION_ARG_NONE        , &target_toolbar,
			N_( "Set it if the action should be displayed in toolbar" ), NULL },
	{ "label-toolbar"        , 'L', 0, G_OPTION_ARG_STRING      , &label_toolbar,
			N_( "The label of the action item in the toolbar" ), N_( "<STRING>" ) },
	{ "command"              , 'c', 0, G_OPTION_ARG_FILENAME    , &command,
			N_( "The path of the command" ), N_( "<PATH>" ) },
	{ "parameters"           , 'p', 0, G_OPTION_ARG_STRING      , &parameters,
			N_( "The parameters of the command" ), N_( "<PARAMETERS>" ) },
	{ "match"                , 'm', 0, G_OPTION_ARG_STRING_ARRAY, &basenames_array,
			N_( "A pattern to match selected items against. May include wildcards (* or ?). You must set one option for each pattern you need" ), N_( "<EXPR>" ) },
	{ "match-case"           , 'C', 0, G_OPTION_ARG_NONE        , &matchcase,
			N_( "Set it if the previous patterns are case sensitive" ), NULL },
	{ "mimetypes"            , 'T', 0, G_OPTION_ARG_STRING_ARRAY, &mimetypes_array,
			N_( "A pattern to match selected items mimetype against. May include wildcards (* or ?). You must set one option for each pattern you need" ), N_( "<EXPR>" ) },
	{ "accept-files"         , 'f', 0, G_OPTION_ARG_NONE        , &isfile,
			N_( "Set it if the selection must only contain files" ), NULL },
	{ "accept-dirs"          , 'd', 0, G_OPTION_ARG_NONE        , &isdir,
			N_( "Set it if the selection must only contain folders. Specify both '--accept-files' and '--accept-dirs' options if selection can contain both types of items" ), NULL },
	{ "accept-multiple-files", 'M', 0, G_OPTION_ARG_NONE        , &accept_multiple,
			N_( "Set it if the selection can have several items" ), NULL },
	{ "scheme"               , 's', 0, G_OPTION_ARG_STRING_ARRAY, &schemes_array,
			N_( "A valid GIO scheme where the selected files should be located. You must set one option for each scheme you need" ), N_( "<SCHEME>" ) },
	{ "folder"               , 'U', 0, G_OPTION_ARG_STRING_ARRAY, &folders_array,
			N_( "The URI of a directory for which folders or toolbar action will be displayed. You must set one option for each folder you need" ), N_( "<URI>" ) },
	{ NULL }
};

static GOptionEntry output_entries[] = {

	{ "output-gconf"         , 'g', 0, G_OPTION_ARG_NONE        , &output_gconf,
			N_( "Store the newly created action as a GConf configuration" ), NULL },
	{ "output-dir"           , 'o', 0, G_OPTION_ARG_FILENAME    , &output_dir,
			N_( "The URI of the folder where to write the new action as a GConf dump output [default: stdout]" ), N_( "<URI>" ) },
	{ NULL }
};

static GOptionEntry misc_entries[] = {

	{ "version"              , 'v', 0, G_OPTION_ARG_NONE        , &version,
			N_( "Output the version number" ), NULL },
	{ NULL }
};

static GOptionContext *init_options( void );
static NAObjectAction *get_action_from_cmdline( void );
static gboolean        write_to_gconf( NAObjectAction *action, GSList **msg );
static void            exit_with_usage( void );

int
main( int argc, char** argv )
{
	int status = EXIT_SUCCESS;
	GOptionContext *context;
	GError *error = NULL;
	NAObjectAction *action;
	GSList *msg = NULL;
	GSList *im;
	gchar *help;
	gint errors;

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
		g_printerr( _( "Syntax error: %s\n" ), error->message );
		g_error_free (error);
		exit_with_usage();
	}

	if( version ){
		na_utils_print_version();
		exit( status );
	}

	errors = 0;

	if( !label || !g_utf8_strlen( label, -1 )){
		g_printerr( _( "Error: an action label is mandatory.\n" ));
		errors += 1;
	}

	if( enabled && disabled ){
		g_printerr( _( "Error: '--enabled' and '--disabled' options cannot both be specified.\n" ));
		errors += 1;
	} else if( !disabled ){
		enabled = TRUE;
	}

	if( output_gconf && output_dir ){
		g_printerr( _( "Error: only one output option may be specified.\n" ));
		errors += 1;
	}

	if( errors ){
		exit_with_usage();
	}

	action = get_action_from_cmdline();

	if( output_gconf ){
		if( write_to_gconf( action, &msg )){
			/* i18n: Action <action_label> written to...*/
			g_print( _( "Action '%s' succesfully written to GConf configuration.\n" ), label );
		}

	} else {
		gchar *output_fname = na_xml_writer_export( action, output_dir, IPREFS_EXPORT_FORMAT_GCONF_ENTRY, &msg );
		if( output_fname ){
			/* i18n: Action <action_label> written to <output_filename>...*/
			g_print( _( "Action '%s' succesfully written to %s, and ready to be imported in NACT.\n" ), label, output_fname );
			g_free( output_fname );
		}
	}

	if( msg ){
		for( im = msg ; im ; im = im->next ){
			g_printerr( "%s\n", ( gchar * ) im->data );
		}
		na_utils_free_string_list( msg );
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
	GOptionGroup *misc_group;

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

	misc_group = g_option_group_new(
			"misc", _( "Miscellaneous options" ), _( "Miscellaneous options" ), NULL, NULL );
	g_option_group_add_entries( misc_group, misc_entries );
	g_option_context_add_group( context, misc_group );

	return( context );
}

/*
 * allocate a new action, and fill it with values readen from command-line
 */
static NAObjectAction *
get_action_from_cmdline( void )
{
	NAObjectAction *action = na_object_action_new_with_profile();
	GList *profiles;
	NAObjectProfile *profile;
	int i = 0;
	GSList *basenames = NULL;
	GSList *mimetypes = NULL;
	GSList *schemes = NULL;
	GSList *folders = NULL;

	profiles = na_object_get_items_list( action );
	profile = NA_OBJECT_PROFILE( profiles->data );

	na_object_set_label( action, label );
	na_object_set_tooltip( action, tooltip );
	na_object_set_icon( action, icon );
	na_object_set_enabled( NA_OBJECT_ITEM( action ), enabled );
	na_object_action_set_target_selection( action, target_selection );
	na_object_action_set_target_background( action, target_folders );
	na_object_action_set_target_toolbar( action, target_toolbar );

	if( target_toolbar ){
		if( label_toolbar && g_utf8_strlen( label_toolbar, -1 )){
			na_object_action_toolbar_set_same_label( action, FALSE );
			na_object_action_toolbar_set_label( action, label_toolbar );
		} else {
			na_object_action_toolbar_set_same_label( action, TRUE );
			na_object_action_toolbar_set_label( action, label );
		}
	} else {
		na_object_action_toolbar_set_label( action, "" );
	}

	na_object_profile_set_path( profile, command );
	na_object_profile_set_parameters( profile, parameters );

	while( basenames_array != NULL && basenames_array[i] != NULL ){
		basenames = g_slist_append( basenames, g_strdup( basenames_array[i] ));
		i++;
	}
	na_object_profile_set_basenames( profile, basenames );
	g_slist_foreach( basenames, ( GFunc ) g_free, NULL );
	g_slist_free( basenames );

	na_object_profile_set_matchcase( profile, matchcase );

	i = 0;
	while( mimetypes_array != NULL && mimetypes_array[i] != NULL ){
		mimetypes = g_slist_append( mimetypes, g_strdup( mimetypes_array[i] ));
		i++;
	}
	na_object_profile_set_mimetypes( profile, mimetypes );
	g_slist_foreach( mimetypes, ( GFunc ) g_free, NULL );
	g_slist_free( mimetypes );

	if( !isfile && !isdir ){
		isfile = TRUE;
	}
	na_object_profile_set_isfile( profile, isfile );
	na_object_profile_set_isdir( profile, isdir );
	na_object_profile_set_multiple( profile, accept_multiple );

	i = 0;
	while( schemes_array != NULL && schemes_array[i] != NULL ){
		schemes = g_slist_append( schemes, g_strdup( schemes_array[i] ));
		i++;
	}
	na_object_profile_set_schemes( profile, schemes );
	g_slist_foreach( schemes, ( GFunc ) g_free, NULL );
	g_slist_free( schemes );

	i = 0;
	while( folders_array != NULL && folders_array[i] != NULL ){
		folders = g_slist_append( folders, g_strdup( folders_array[i] ));
		i++;
	}
	na_object_profile_set_folders( profile, folders );
	g_slist_foreach( folders, ( GFunc ) g_free, NULL );
	g_slist_free( folders );

	return( action );
}

/*
 * initialize GConf as an I/O provider
 * then writes the action
 */
static gboolean
write_to_gconf( NAObjectAction *action, GSList **msg )
{
	NAPivot *pivot;
	GObject *provider;
	guint ret;

	pivot = na_pivot_new( NULL );
	provider = na_pivot_get_provider( pivot, NA_IIO_PROVIDER_TYPE );

	na_object_set_provider( action, NA_IIO_PROVIDER( provider ));

	ret = na_io_provider_write_item( pivot, NA_OBJECT_ITEM( action ), msg );

	na_pivot_release_provider( provider );
	g_object_unref( pivot );

	return( ret == NA_IIO_PROVIDER_WRITE_OK );
}

/*
 * print a help message and exit with failure
 */
static void
exit_with_usage( void )
{
	g_printerr( _( "Try %s --help for usage.\n" ), g_get_prgname());
	exit( EXIT_FAILURE );
}
