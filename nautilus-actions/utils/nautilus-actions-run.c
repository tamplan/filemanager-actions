/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010 Pierre Wieser and others (see AUTHORS)
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
#include <string.h>

#include <api/na-dbus.h>
#include <api/na-object-api.h>

#include <runtime/na-pivot.h>
#include <runtime/na-utils.h>

#include <tracker/na-tracker.h>
#include <tracker/na-tracker-dbus.h>

#include "console-utils.h"
#include "nautilus-actions-run-bindings.h"

static gchar     *id               = "";
static gchar    **targets_array    = NULL;
static gboolean   version          = FALSE;

static GOptionEntry entries[] = {

	{ "id"                   , 'i', 0, G_OPTION_ARG_STRING        , &id,
			N_( "The internal identifiant of the action to be launched" ), N_( "<STRING>" ) },
	{ "target"               , 't', 0, G_OPTION_ARG_FILENAME_ARRAY, &targets_array,
			N_( "A target, file or folder, for the action. More than one options may be specified" ), N_( "<URI>" ) },
	{ NULL }
};

static GOptionEntry misc_entries[] = {

	{ "version"              , 'v', 0, G_OPTION_ARG_NONE        , &version,
			N_( "Output the version number" ), NULL },
	{ NULL }
};

static GOptionContext  *init_options( void );
static NAObjectAction  *get_action( const gchar *id );
static GList           *targets_from_selection( void );
static GList           *targets_from_commandline( void );
static NAObjectProfile *get_profile_for_targets( NAObjectAction *action, GList *targets );
static void             execute_action( NAObjectAction *action, NAObjectProfile *profile, GList *targets );
static void             dump_targets( GList *targets );
static void             free_targets( GList *targets );
static void             exit_with_usage( void );

int
main( int argc, char** argv )
{
	static const gchar *thisfn = "nautilus_actions_run_main";
	int status = EXIT_SUCCESS;
	GOptionContext *context;
	GError *error = NULL;
	gchar *help;
	gint errors;
	NAObjectAction *action;
	NAObjectProfile *profile;
	GList *targets;

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

	g_option_context_free( context );

	if( version ){
		na_utils_print_version();
		exit( status );
	}

	errors = 0;

	if( !id || !strlen( id )){
		g_printerr( _( "Error: action id is mandatory.\n" ));
		errors += 1;
	}

	action = get_action( id );
	if( !action ){
		errors += 1;
	} else {
		g_debug( "%s: action %s have been found, and is enabled and valid", thisfn, id );
	}

	if( errors ){
		exit_with_usage();
	}

	if( targets_array ){
		targets = targets_from_commandline();

	} else {
		targets = targets_from_selection();
	}

	dump_targets( targets );

	if( g_list_length( targets ) == 0 ){
		g_print( "No current selection. Nothing to do. Exiting...\n" );
		exit( status );
	}

	profile = get_profile_for_targets( action, targets );
	if( !profile ){
		g_print( "No valid profile is candidate to execution. Exiting...\n" );
		exit( status );
	}
	g_debug( "%s: profile %p found", thisfn, ( void * ) profile );

	execute_action( action, profile, targets );

	free_targets( targets );
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
	GOptionGroup *misc_group;

	context = g_option_context_new( _( "Execute an action on the specified target." ));

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
 * search for the action in the repository
 */
static NAObjectAction *
get_action( const gchar *id )
{
	NAObjectAction *action;
	NAPivot *pivot;

	action = NULL;

	pivot = na_pivot_new();
	na_pivot_load_items( pivot );
	action = ( NAObjectAction * ) na_pivot_get_item( pivot, id );

	if( !action ){
		g_printerr( _( "Error: action '%s' doesn't exist.\n" ), id );
	}

	if( action ){
		if( !na_object_is_enabled( action )){
			g_printerr( _( "Error: action '%s' is disabled.\n" ), id );
			g_object_unref( action );
			action = NULL;
		}
	}

	if( action ){
		if( !na_object_is_valid( action )){
			g_printerr( _( "Error: action '%s' is not valid.\n" ), id );
			g_object_unref( action );
			action = NULL;
		}
	}

	return( action );
}

/*
 * the DBus.Tracker.Status interface returns a list of strings
 * each item has two slots in this list:
 * - uri
 * - mimetype
 */
static GList *
targets_from_selection( void )
{
	static const gchar *thisfn = "nautilus_actions_run_targets_from_selection";
	GList *selection;
	NATrackedItem *tracked;
	DBusGConnection *connection;
	DBusGProxy *proxy;
	GError *error;
	gchar **paths, **iter;

	selection = NULL;
	error = NULL;
	proxy = NULL;
	paths = NULL;

	connection = dbus_g_bus_get( DBUS_BUS_SESSION, &error );

	if( !connection ){
		if( error ){
			g_printerr( _( "Error: unable to get a connection to session DBus: %s" ), error->message );
			g_error_free( error );
		}
		return( NULL );
	}
	g_debug( "%s: connection is ok", thisfn );

	proxy = dbus_g_proxy_new_for_name( connection,
			NAUTILUS_ACTIONS_DBUS_SERVICE, NA_TRACKER_DBUS_TRACKER_PATH, NA_TRACKER_DBUS_TRACKER_INTERFACE );

	if( !proxy ){
		g_printerr( _( "Error: unable to get a proxy on %s service" ), NAUTILUS_ACTIONS_DBUS_SERVICE );
		dbus_g_connection_unref( connection );
		return( NULL );
	}
	g_debug( "%s: proxy is ok", thisfn );

	if( !dbus_g_proxy_call( proxy, "GetSelectedPaths", &error,
			G_TYPE_INVALID,
			G_TYPE_STRV, &paths, G_TYPE_INVALID )){

		g_printerr( _( "Error on GetSelectedPaths call: %s" ), error->message );
		g_error_free( error );
		/* TODO: unref proxy */
		dbus_g_connection_unref( connection );
		return( NULL );
	}
	g_debug( "%s: function call is ok", thisfn );

	iter = paths;
	while( *iter ){
		tracked = g_new0( NATrackedItem, 1 );
		tracked->uri = g_strdup( *iter );
		iter++;
		tracked->mimetype = g_strdup( *iter );
		iter++;
		selection = g_list_prepend( selection, tracked );
	}
	g_strfreev( paths );
	selection = g_list_reverse( selection );

	/* TODO: unref proxy */
	dbus_g_connection_unref( connection );

	return( selection );
}

/*
 * get targets from command-line
 */
static GList *
targets_from_commandline( void )
{
	static const gchar *thisfn = "nautilus_actions_run_targets_from_commandline";
	GList *targets;
	NATrackedItem *tracked;
	gchar **iter;

	g_debug( "%s", thisfn );

	targets = NULL;
	iter = targets_array;

	while( *iter ){
		tracked = g_new0( NATrackedItem, 1 );
		tracked->uri = g_strdup( *iter );
		iter++;
		targets = g_list_prepend( targets, tracked );
	}

	targets = g_list_reverse( targets );

	return( targets );
}

/*
 * find a profile candidate to be executed for the given uris
 */
static NAObjectProfile *
get_profile_for_targets( NAObjectAction *action, GList *targets )
{
	/*static const gchar *thisfn = "nautilus_actions_run_get_profile_for_targets";*/
	NAObjectProfile *candidate;
	GList *profiles, *ip;

	candidate = NULL;
	profiles = na_object_get_items_list( action );
	for( ip = profiles ; ip && !candidate ; ip = ip->next ){

		NAObjectProfile *profile = NA_OBJECT_PROFILE( ip->data );
		if( na_object_profile_is_candidate_for_tracked( profile, targets )){
			candidate = profile;
		}
	}

	return( candidate );
}

static void
execute_action( NAObjectAction *action, NAObjectProfile *profile, GList *targets )
{
	static const gchar *thisfn = "nautilus_action_run_execute_action";
	GString *cmd;
	gchar *param, *path;

	path = na_object_profile_get_path( profile );
	cmd = g_string_new( path );

	param = na_object_profile_parse_parameters_for_tracked( profile, targets );

	if( param != NULL ){
		g_string_append_printf( cmd, " %s", param );
		g_free( param );
	}

	g_debug( "%s: executing '%s'", thisfn, cmd->str );
	g_spawn_command_line_async( cmd->str, NULL );

	g_string_free (cmd, TRUE);
	g_free( path );
}

/*
 *
 */
static void
dump_targets( GList *targets )
{
	NATrackedItem *tracked;
	GList *it;

	for( it = targets ; it ; it = it->next ){
		tracked = ( NATrackedItem * ) it->data;
		g_print( "%s\t[%s]\n", tracked->uri, tracked->mimetype );
	}
}

/*
 *
 */
static void
free_targets( GList *targets )
{
	NATrackedItem *tracked;
	GList *it;

	for( it = targets ; it ; it = it->next ){
		tracked = ( NATrackedItem * ) it->data;
		g_free( tracked->uri );
		g_free( tracked->mimetype );
		g_free( tracked );
	}

	g_list_free( targets );
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
