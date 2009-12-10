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
#include <string.h>

#include <api/na-object-api.h>

#include <runtime/na-pivot.h>
#include <runtime/na-utils.h>

#include <tracker/na-tracker.h>
#include <tracker/na-tracker-dbus.h>

#include "console-utils.h"
#include "nautilus-actions-run-bindings.h"

static gchar     *id               = "";
static gchar     *parms            = "";
static gchar    **targets_array    = NULL;
static gboolean   current          = FALSE;
static gboolean   version          = FALSE;

static GOptionEntry entries[] = {

	{ "id"                   , 'i', 0, G_OPTION_ARG_STRING      , &id,
			N_( "The internal identifiant of the action to be launched" ), N_( "<STRING>" ) },
	{ "parameters"           , 'p', 0, G_OPTION_ARG_STRING      , &parms,
			N_( "The parameters to be applied to the action" ), N_( "<STRING>" ) },
	{ "target"               , 't', 0, G_OPTION_ARG_STRING      , &targets_array,
			N_( "A target of the action. More than one options may be specified. Incompatible with '--current' option" ), N_( "<STRING>" ) },
	{ "current"              , 'c', 0, G_OPTION_ARG_NONE        , &current,
			N_( "Whether the action should be executed on current Nautilus selection [default]. Incompatible with '--target' option" ), NULL },
	{ NULL }
};

static GOptionEntry misc_entries[] = {

	{ "version"              , 'v', 0, G_OPTION_ARG_NONE        , &version,
			N_( "Output the version number" ), NULL },
	{ NULL }
};

static GOptionContext *init_options( void );
static NAObjectAction *get_action( const gchar *id );
static GList          *get_current_selection( void );
static GList          *targets_from_commandline( void );
static void            dump_targets( GList *targets );
static void            free_targets( GList *targets );
static void            exit_with_usage( void );

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

	if( current && targets_array ){
		g_printerr( _( "Error: '--target' and '--current' options cannot both be specified.\n" ));
		errors += 1;
	}

	if( !current && !targets_array ){
		current = TRUE;
	}

	action = get_action( id );
	if( !action ){
		errors += 1;
	}
	g_debug( "%s: action %s have been found, and is enabled and valid", thisfn, id );

	if( errors ){
		exit_with_usage();
	}

	if( current ){
		targets = get_current_selection();
	} else {
		targets = targets_from_commandline();
	}

	dump_targets( targets );
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

static GList *
get_current_selection( void )
{
	static const gchar *thisfn = "nautilus_actions_run_get_current_selection";
	GList *selection;
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
		selection = g_list_prepend( selection, g_strdup( *iter ));
		iter++;
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
	GList *targets;
	gchar **iter;

	targets = NULL;
	iter = targets_array;
	while( *iter ){
		targets = g_list_prepend( targets, g_strdup( *iter ));
		iter++;
	}

	targets = g_list_reverse( targets );

	return( targets );
}

/*
 *
 */
static void
dump_targets( GList *targets )
{
	GList *it;

	for( it = targets ; it ; it = it->next ){
		g_print( "%s\n", ( gchar * ) it->data );
	}
}

/*
 *
 */
static void
free_targets( GList *targets )
{
	GList *it;

	for( it = targets ; it ; it = it->next ){
		g_free(( gchar * ) it->data );
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
