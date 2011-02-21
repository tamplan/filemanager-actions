/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011 Pierre Wieser and others (see AUTHORS)
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

#include <dbus/dbus-glib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <glibtop/proclist.h>
#include <glibtop/procstate.h>

#include <libnautilus-extension/nautilus-file-info.h>

#include <api/na-core-utils.h>
#include <api/na-object-api.h>

#include "na-gnome-vfs-uri.h"
#include "na-selected-info.h"

/* private interface data
 */
struct _NAIContextInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

static const gchar *st_mimetype_all    = "*/*";

static gboolean     st_initialized     = FALSE;
static gboolean     st_finalized       = FALSE;

static GType        register_type( void );
static void         interface_base_init( NAIContextInterface *klass );
static void         interface_base_finalize( NAIContextInterface *klass );

static gboolean     v_is_candidate( NAIContext *object, guint target, GList *selection );

static gboolean     is_candidate_for_target( const NAIContext *object, guint target, GList *files );
static gboolean     is_candidate_for_show_in( const NAIContext *object, guint target, GList *files );
static const gchar *get_running_environment( void );
static gboolean     is_candidate_for_try_exec( const NAIContext *object, guint target, GList *files );
static gboolean     is_candidate_for_show_if_registered( const NAIContext *object, guint target, GList *files );
static gboolean     is_candidate_for_show_if_true( const NAIContext *object, guint target, GList *files );
static gboolean     is_candidate_for_show_if_running( const NAIContext *object, guint target, GList *files );
static gboolean     is_candidate_for_mimetypes( const NAIContext *object, guint target, GList *files );
static gboolean     is_mimetype_of( const gchar *file_type, const gchar *group, const gchar *subgroup );
static void         split_mimetype( const gchar *mimetype, gchar **group, gchar **subgroup );
static gboolean     is_candidate_for_basenames( const NAIContext *object, guint target, GList *files );
static gboolean     is_candidate_for_selection_count( const NAIContext *object, guint target, GList *files );
static gboolean     is_candidate_for_schemes( const NAIContext *object, guint target, GList *files );
static gboolean     is_compatible_scheme( const gchar *pattern, const gchar *scheme );
static gboolean     is_candidate_for_folders( const NAIContext *object, guint target, GList *files );
static gboolean     is_candidate_for_capabilities( const NAIContext *object, guint target, GList *files );

static gboolean     is_valid_basenames( const NAIContext *object );
static gboolean     is_valid_mimetypes( const NAIContext *object );
static gboolean     is_valid_schemes( const NAIContext *object );
static gboolean     is_valid_folders( const NAIContext *object );

static gboolean     is_positive_assertion( const gchar *assertion );

/**
 * na_icontext_get_type:
 *
 * Returns: the #GType type of this interface.
 */
GType
na_icontext_get_type( void )
{
	static GType type = 0;

	if( !type ){
		type = register_type();
	}

	return( type );
}

/*
 * na_icontext_register_type:
 *
 * Registers this interface.
 */
static GType
register_type( void )
{
	static const gchar *thisfn = "na_icontext_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NAIContextInterface ),
		( GBaseInitFunc ) interface_base_init,
		( GBaseFinalizeFunc ) interface_base_finalize,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_INTERFACE, "NAIContext", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( NAIContextInterface *klass )
{
	static const gchar *thisfn = "na_icontext_interface_base_init";

	if( !st_initialized ){

		g_debug( "%s: klass%p (%s)", thisfn, ( void * ) klass, G_OBJECT_CLASS_NAME( klass ));

		klass->private = g_new0( NAIContextInterfacePrivate, 1 );

		st_initialized = TRUE;
	}
}

static void
interface_base_finalize( NAIContextInterface *klass )
{
	static const gchar *thisfn = "na_icontext_interface_base_finalize";

	if( st_initialized && !st_finalized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		st_finalized = TRUE;

		g_free( klass->private );
	}
}

/**
 * na_icontext_is_candidate:
 * @context: a #NAIContext to be checked.
 * @target: the current target.
 * @selection: the currently selected items, as a #GList of NASelectedInfo items.
 *
 * Determines if the given object may be candidate to be displayed in
 * the Nautilus context menu, depending of the list of currently selected
 * items.
 *
 * This function is called by <methodname>build_nautilus_menus</methodname>
 * plugin function for each item found in NAPivot items list, and, when this
 * an action, for each profile of this action.
 *
 * Returns: %TRUE if this @context succeeds to all tests and is so a
 * valid candidate to be displayed in Nautilus context menu, %FALSE
 * else.
 *
 * Since: 2.30
 */
gboolean
na_icontext_is_candidate( const NAIContext *context, guint target, GList *selection )
{
	static const gchar *thisfn = "na_icontext_is_candidate";
	gboolean is_candidate;

	g_return_val_if_fail( NA_IS_ICONTEXT( context ), FALSE );

	g_debug( "%s: object=%p (%s), target=%d, selection=%p (count=%d)",
			thisfn, ( void * ) context, G_OBJECT_TYPE_NAME( context ), target, (void * ) selection, g_list_length( selection ));

	is_candidate = v_is_candidate( NA_ICONTEXT( context ), target, selection );

	if( is_candidate ){
		is_candidate =
				is_candidate_for_target( context, target, selection ) &&
				is_candidate_for_show_in( context, target, selection ) &&
				is_candidate_for_try_exec( context, target, selection ) &&
				is_candidate_for_show_if_registered( context, target, selection ) &&
				is_candidate_for_show_if_true( context, target, selection ) &&
				is_candidate_for_show_if_running( context, target, selection ) &&
				is_candidate_for_mimetypes( context, target, selection ) &&
				is_candidate_for_basenames( context, target, selection ) &&
				is_candidate_for_selection_count( context, target, selection ) &&
				is_candidate_for_schemes( context, target, selection ) &&
				is_candidate_for_folders( context, target, selection ) &&
				is_candidate_for_capabilities( context, target, selection );
	}

	return( is_candidate );
}

/**
 * na_icontext_is_valid:
 * @context: the #NAObjectProfile to be checked.
 *
 * Returns: %TRUE if this @context is valid, %FALSE else.
 *
 * This function is part of <methodname>NAIDuplicable::check_status</methodname>
 * and is called by #NAIDuplicable objects which also implement #NAIContext
 * interface. It so doesn't make sense of asking the object for its
 * validity status as it has already been checked before calling the
 * function.
 *
 * Since: 2.30
 */
gboolean
na_icontext_is_valid( const NAIContext *context )
{
	gboolean is_valid;

	g_return_val_if_fail( NA_IS_ICONTEXT( context ), FALSE );

	is_valid =
		is_valid_basenames( context ) &&
		is_valid_mimetypes( context ) &&
		is_valid_schemes( context ) &&
		is_valid_folders( context );

	return( is_valid );
}

/**
 * na_icontext_check_mimetypes:
 * @context: the #NAIContext object to be checked.
 *
 * Check the current list of mimetypes to see if it covers all mimetypes,
 * or all regular files, or something else.
 *
 * Since: 2.30
 */
void
na_icontext_check_mimetypes( const NAIContext *context )
{
	static const gchar *thisfn = "na_icontext_check_mimetypes";
	gboolean is_all;
	GSList *mimetypes, *im;

	g_return_val_if_fail( NA_IS_ICONTEXT( context ), FALSE );

	is_all = TRUE;
	mimetypes = na_object_get_mimetypes( context );

	for( im = mimetypes ; im ; im = im->next ){
		if( !im->data || !strlen( im->data )){
			g_warning( "%s: empty mimetype for context=%p", thisfn, ( void * ) context );
			continue;
		}
		const gchar *imtype = ( const gchar * ) im->data;
		if( !strcmp( imtype, "*" ) ||
			!strcmp( imtype, st_mimetype_all ) ||
			!strcmp( imtype, "*/all" ) ||
			!strcmp( imtype, "all" ) ||
			!strcmp( imtype, "all/*" ) ||
			!strcmp( imtype, "all/all" )){
				continue;
		}
		is_all = FALSE;
		/* do not break here so that we are able to check all mimetypes */
	}

	na_object_set_all_mimetypes( context, is_all );

	na_core_utils_slist_free( mimetypes );
}

/**
 * na_icontext_read_done:
 * @context: the #NAIContext to be prepared.
 *
 * Prepares the specified #NAIContext just after it has been read.
 *
 * <itemizedlist>
 *   <listitem>
 *     <para>
 *       This setup an internal flag when mimetypes is like 'all/all'
 *       in order to optimize computation time;
 *     </para>
 *   </listitem>
 * </itemizedlist>
 *
 * Since: 2.30
 */
void
na_icontext_read_done( NAIContext *context )
{
	na_object_check_mimetypes( context );
}

/**
 * na_icontext_set_scheme:
 * @context: the #NAIContext to be updated.
 * @scheme: name of the scheme.
 * @selected: whether this scheme is candidate to this @context.
 *
 * Sets the status of a @scheme relative to this @context.
 *
 * Since: 2.30
 */
void
na_icontext_set_scheme( NAIContext *context, const gchar *scheme, gboolean selected )
{
	GSList *schemes;

	g_return_if_fail( NA_IS_ICONTEXT( context ));

	schemes = na_object_get_schemes( context );
	schemes = na_core_utils_slist_setup_element( schemes, scheme, selected );
	na_object_set_schemes( context, schemes );
	na_core_utils_slist_free( schemes );
}

/**
 * na_icontext_set_only_desktop:
 * @context: the #NAIContext to be updated.
 * @desktop: name of the desktop environment.
 * @selected: whether this @desktop is candidate to this @context.
 *
 * Sets the status of the @desktop relative to this @context for the OnlyShowIn list.
 *
 * Since: 2.30
 */
void
na_icontext_set_only_desktop( NAIContext *context, const gchar *desktop, gboolean selected )
{
	GSList *desktops;

	g_return_if_fail( NA_IS_ICONTEXT( context ));

	desktops = na_object_get_only_show_in( context );
	desktops = na_core_utils_slist_setup_element( desktops, desktop, selected );
	na_object_set_only_show_in( context, desktops );
	na_core_utils_slist_free( desktops );
}

/**
 * na_icontext_set_only_desktop:
 * @context: the #NAIContext to be updated.
 * @desktop: name of the desktop environment.
 * @selected: whether this @desktop is candidate to this @context.
 *
 * Sets the status of the @desktop relative to this @context for the NotShowIn list.
 *
 * Since: 2.30
 */
void
na_icontext_set_not_desktop( NAIContext *context, const gchar *desktop, gboolean selected )
{
	GSList *desktops;

	g_return_if_fail( NA_IS_ICONTEXT( context ));

	desktops = na_object_get_not_show_in( context );
	desktops = na_core_utils_slist_setup_element( desktops, desktop, selected );
	na_object_set_not_show_in( context, desktops );
	na_core_utils_slist_free( desktops );
}

/**
 * na_icontext_replace_folder:
 * @context: the #NAIContext to be updated.
 * @old: the old uri.
 * @new: the new uri.
 *
 * Replaces the @old URI by the @new one.
 *
 * Since: 2.30
 */
void
na_icontext_replace_folder( NAIContext *context, const gchar *old, const gchar *new )
{
	GSList *folders;

	g_return_if_fail( NA_IS_ICONTEXT( context ));

	folders = na_object_get_folders( context );
	folders = na_core_utils_slist_remove_utf8( folders, old );
	folders = g_slist_append( folders, ( gpointer ) g_strdup( new ));
	na_object_set_folders( context, folders );
	na_core_utils_slist_free( folders );
}

#if 0
static const gchar *st_mimetype_notdir = "!inode/directory";

/*
 * Convert 'all/allfiles' mimetype to 'all/all' + 'file' scheme.
 * This takes into account
 * - all/allfiles, allfiles, allfiles/ * and allfiles/all
 * - negated assertions.
 */
static void
convert_allfiles_mimetype( NAIContext *context )
{
	static const gchar *thisfn = "na_icontext_convert_allfiles_mimetype";
	GSList *mimetypes, *im, *new_mimetypes;
	const gchar *prev_type;

	mimetypes = na_object_get_mimetypes( context );
	new_mimetypes = NULL;
	prev_type = NULL;

	for( im = mimetypes ; im ; im = im->next ){
		if( !im->data || !strlen( im->data )){
			continue;
		}

		const gchar *imtype = ( const gchar * ) im->data;

		if( !strcmp( imtype, "allfiles" ) ||
			!strcmp( imtype, "*/allfiles" ) ||
			!strcmp( imtype, "allfiles/*" ) ||
			!strcmp( imtype, "allfiles/all" ) ||
			!strcmp( imtype, "all/allfiles" )){

				new_mimetypes = g_slist_prepend( new_mimetypes, g_strdup( st_mimetype_all ));
				new_mimetypes = g_slist_prepend( new_mimetypes, g_strdup( st_mimetype_notdir ));
				prev_type = imtype;

		} else if( strcmp( imtype, st_mimetype_all ) && strcmp( imtype, st_mimetype_notdir )){

				new_mimetypes = g_slist_prepend( new_mimetypes, g_strdup( imtype ));
		}
	}

	if( mimetypes ){
		if( prev_type ){
			g_debug( "%s: changing %s to %s;%s", thisfn, prev_type, st_mimetype_all, st_mimetype_notdir );
		}
		na_object_set_mimetypes( context, g_slist_reverse( new_mimetypes ));
	}

	na_core_utils_slist_free( mimetypes );
	na_core_utils_slist_free( new_mimetypes );
}
#endif

static gboolean
v_is_candidate( NAIContext *context, guint target, GList *selection )
{
	gboolean is_candidate;

	is_candidate = TRUE;

	if( NA_ICONTEXT_GET_INTERFACE( context )->is_candidate ){
		is_candidate = NA_ICONTEXT_GET_INTERFACE( context )->is_candidate( context, target, selection );
	}

	return( is_candidate );
}

/*
 * whether the given NAIContext object is candidate for this target
 * target is context menu for location, context menu for selection or toolbar for location
 * only actions are concerned by this check
 */
static gboolean
is_candidate_for_target( const NAIContext *object, guint target, GList *files )
{
	static const gchar *thisfn = "na_icontext_is_candidate_for_target";
	gboolean ok = TRUE;

	if( NA_IS_OBJECT_ACTION( object )){
		switch( target ){
			case ITEM_TARGET_LOCATION:
				ok = na_object_is_target_location( object );
				break;

			case ITEM_TARGET_TOOLBAR:
				ok = na_object_is_target_toolbar( object );
				break;

			case ITEM_TARGET_SELECTION:
				ok = na_object_is_target_selection( object );
				break;

			case ITEM_TARGET_ANY:
				ok = TRUE;
				break;

			default:
				g_warning( "%s: unknonw target=%d", thisfn, target );
				ok = FALSE;
		}
	}

	if( !ok ){
		g_debug( "%s: object is not candidate because target doesn't match (asked=%d)", thisfn, target );
		/*na_object_dump( object );*/
	}

	return( ok );
}

/*
 * only show in / not show in
 * only one of these two data may be set
 */
static gboolean
is_candidate_for_show_in( const NAIContext *object, guint target, GList *files )
{
	static const gchar *thisfn = "na_icontext_is_candidate_for_show_in";
	gboolean ok = TRUE;
	GSList *only_in = na_object_get_only_show_in( object );
	GSList *not_in = na_object_get_not_show_in( object );
	static const gchar *environment = NULL;

	if( !environment ){
		environment = get_running_environment();
		g_debug( "%s: found %s desktop", thisfn, environment );
	}

	if( only_in && g_slist_length( only_in )){
		ok = ( na_core_utils_slist_count( only_in, environment ) > 0 );
	} else if( not_in && g_slist_length( not_in )){
		ok = ( na_core_utils_slist_count( not_in, environment ) == 0 );
	}

	if( !ok ){
		gchar *only_str = na_core_utils_slist_to_text( only_in );
		gchar *not_str = na_core_utils_slist_to_text( not_in );
		g_debug( "%s: object is not candidate because OnlyShowIn=%s, NotShowIn=%s", thisfn, only_str, not_str );
		g_free( not_str );
		g_free( only_str );
	}

	na_core_utils_slist_free( not_in );
	na_core_utils_slist_free( only_in );

	return( ok );
}

/*
 * Have asked on xdg-list how to identify the currently running desktop environment
 * (see)
 * For now, just reproduce the xdg-open algorythm from xdg-utils 1.0
 */
#define DESKTOP_KDE   "KDE"
#define DESKTOP_GNOME "GNOME"
#define DESKTOP_XFCE  "XFCE"
#define DESKTOP_ROX   "ROX"
#define DESKTOP_LXDE  "LXDE"
#define DESKTOP_OLD   "Old"

static const gchar *
get_running_environment( void )
{
	static const gchar *thisfn = "na_icontext_get_running_environment";
	const gchar *value;
	gchar *output_str, *error_str;
	gint exit_status;
	GError *error;
	gboolean ok;

	value = g_getenv( "KDE_FULL_SESSION" );
	if( value && !strcmp( value, "true" )){
		return( DESKTOP_KDE );
	}

	value = g_getenv( "GNOME_DESKTOP_SESSION_ID" );
	if( value && strlen( value )){
		return( DESKTOP_GNOME );
	}

	output_str = NULL;
	error_str = NULL;
	error = NULL;
	if( g_spawn_command_line_sync(
			"dbus-send --print-reply --dest=org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus.GetNameOwner string:org.gnome.SessionManager",
			&output_str, &error_str, &exit_status, &error )){
		ok = ( exit_status == 0 && output_str && strlen( output_str ) && ( !error_str || !strlen( error_str )));
		g_free( output_str );
		g_free( error_str );
		if( ok ){
			return( DESKTOP_GNOME );
		}
	}
	if( error ){
		g_warning( "%s: dbus-send: %s", thisfn, error->message );
		g_error_free( error );
	}

	output_str = NULL;
	error_str = NULL;
	error = NULL;
	if( g_spawn_command_line_sync(
			"xprop -root _DT_SAVE_MODE", &output_str, &error_str, &exit_status, &error )){
		ok = ( exit_status == 0 && output_str && strlen( output_str ) && ( !error_str || !strlen( error_str )));
		if( ok ){
			ok = ( g_strstr_len( output_str, -1, "xfce" ) != NULL );
		}
		g_free( output_str );
		g_free( error_str );
		if( ok ){
			return( DESKTOP_XFCE );
		}
	}
	if( error ){
		g_warning( "%s: xprop: %s", thisfn, error->message );
		g_error_free( error );
	}

	/* do not know how to identify ROX or LXDE (Hong Jen Yee <pcman.tw (at) gmail.com>)
	 * environments; so other desktops are just identified as 'Old' (legacy systems)
	 */
	return( DESKTOP_OLD );
}

/*
 * if the data is set, it should be the path of an executable file
 */
static gboolean
is_candidate_for_try_exec( const NAIContext *object, guint target, GList *files )
{
	static const gchar *thisfn = "na_icontext_is_candidate_for_try_exec";
	gboolean ok = TRUE;
	GError *error = NULL;
	gchar *tryexec = na_object_get_try_exec( object );

	if( tryexec && strlen( tryexec )){
		ok = FALSE;
		GFile *file = g_file_new_for_path( tryexec );
		GFileInfo *info = g_file_query_info( file, G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE, G_FILE_QUERY_INFO_NONE, NULL, &error );
		if( error ){
			g_debug( "%s: %s", thisfn, error->message );
			g_error_free( error );

		} else {
			ok = g_file_info_get_attribute_boolean( info, G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE );
		}

		if( info ){
			g_object_unref( info );
		}

		g_object_unref( file );
	}

	if( !ok ){
		g_debug( "%s: object is not candidate because TryExec=%s", thisfn, tryexec );
	}

	g_free( tryexec );

	return( ok );
}

static gboolean
is_candidate_for_show_if_registered( const NAIContext *object, guint target, GList *files )
{
	static const gchar *thisfn = "na_icontext_is_candidate_for_show_if_registered";
	gboolean ok = TRUE;
	gchar *name = na_object_get_show_if_registered( object );

	if( name && strlen( name )){
		ok = FALSE;
		GError *error = NULL;
		DBusGConnection *connection = dbus_g_bus_get( DBUS_BUS_SESSION, &error );

		if( !connection ){
			if( error ){
				g_warning( "%s: %s", thisfn, error->message );
				g_error_free( error );
			}

		} else {
			DBusGProxy *proxy = dbus_g_proxy_new_for_name( connection, name, NULL, NULL );
			ok = ( proxy != NULL );
			dbus_g_connection_unref( connection );
		}
	}

	if( !ok ){
		g_debug( "%s: object is not candidate because ShowIfRegistered=%s", thisfn, name );
	}

	g_free( name );

	return( ok );
}

static gboolean
is_candidate_for_show_if_true( const NAIContext *object, guint target, GList *files )
{
	static const gchar *thisfn = "na_icontext_is_candidate_for_show_if_true";
	gboolean ok = TRUE;
	gchar *command = na_object_get_show_if_true( object );

	if( command && strlen( command )){
		ok = FALSE;
		gchar *stdout = NULL;
		g_spawn_command_line_sync( command, &stdout, NULL, NULL, NULL );

		if( stdout && !strcmp( stdout, "true" )){
			ok = TRUE;
		}

		g_free( stdout );
	}

	if( !ok ){
		g_debug( "%s: object is not candidate because ShowIfTrue=%s", thisfn, command );
	}

	g_free( command );

	return( ok );
}

static gboolean
is_candidate_for_show_if_running( const NAIContext *object, guint target, GList *files )
{
	static const gchar *thisfn = "na_icontext_is_candidate_for_show_if_running";
	gboolean ok = TRUE;
	gchar *searched;
	glibtop_proclist proclist;
	glibtop_proc_state procstate;
	pid_t *pid_list;
	guint i;
	gchar *running = na_object_get_show_if_running( object );

	if( running && strlen( running )){
		ok = FALSE;
		searched = g_path_get_basename( running );
		pid_list = glibtop_get_proclist( &proclist, GLIBTOP_KERN_PROC_ALL, 0 );

		for( i=0 ; i<proclist.number && !ok ; ++i ){
			glibtop_get_proc_state( &procstate, pid_list[i] );
			/*g_debug( "%s: i=%d, cmd=%s", thisfn, i, procstate.cmd );*/
			if( strcmp( procstate.cmd, searched ) == 0 ){
				g_debug( "%s: i=%d, cmd=%s", thisfn, i, procstate.cmd );
				ok = TRUE;
			}
		}

		g_free( pid_list );
		g_free( searched );
	}

	if( !ok ){
		g_debug( "%s: object is not candidate because ShowIfRunning=%s", thisfn, running );
	}

	g_free( running );

	return( ok );
}

/*
 * object may embed a list of - possibly negated - mimetypes
 * each file of the selection must satisfy all conditions of this list
 * an empty list is considered the same as '*' or '* / *'
 *
 * most time, we will just have '*' - so try to optimize the code to
 * be as fast as possible when we don't filter on mimetype
 *
 * mimetypes are of type : "image/ *; !image/jpeg"
 * file mimetype must be compatible with at least one positive assertion
 * (they are ORed), while not being of any negative assertions (they are
 * ANDed)
 *
 * here, for each mimetype of the selection, do
 * . match is FALSE while this mimetype has not matched a positive assertion
 * . nomatch is FALSE while this mimetype has not matched a negative assertion
 * we are going to check each mimetype filter
 *  while we have not found a match among positive conditions (i.e. while !match)
 *  we have to check all negative conditions to verify that the current
 *  examined mimetype never match these
 */
static gboolean
is_candidate_for_mimetypes( const NAIContext *object, guint target, GList *files )
{
	static const gchar *thisfn = "na_icontext_is_candidate_for_mimetypes";
	gboolean ok = TRUE;
	gboolean all = na_object_get_all_mimetypes( object );

	g_debug( "%s: all=%s", thisfn, all ? "True":"False" );

	if( !all ){
		GSList *mimetypes = na_object_get_mimetypes( object );
		GSList *im;
		GList *it;

		for( it = files ; it && ok ; it = it->next ){
			gchar *ftype, *fgroup, *fsubgroup;
			gboolean match, positive;

			match = FALSE;
			ftype = na_selected_info_get_mime_type( NA_SELECTED_INFO( it->data ));

			if( ftype ){
				split_mimetype( ftype, &fgroup, &fsubgroup );

				for( im = mimetypes ; im && ok ; im = im->next ){
					const gchar *imtype = ( const gchar * ) im->data;
					positive = is_positive_assertion( imtype );

					if( !positive || !match ){
						if( is_mimetype_of( positive ? imtype : imtype+1, fgroup, fsubgroup )){
							g_debug( "%s: condition=%s, positive=%s, ftype=%s, fgroup=%s, fsubgroup=%s, matched",
									thisfn, imtype, positive ? "True":"False", ftype, fgroup, fsubgroup );
							if( positive ){
								match = TRUE;
							} else {
								ok = FALSE;
							}
						}
					}
				}

				if( !match ){
					gchar *mimetypes_str = na_core_utils_slist_to_text( mimetypes );
					g_debug( "%s: no positive match found for Mimetypes=%s", thisfn, mimetypes_str );
					g_free( mimetypes_str );
					ok = FALSE;
				}

			} else {
				gchar *uri = na_selected_info_get_uri( NA_SELECTED_INFO( it->data ));
				g_debug( "%s: null mimetype found for %s", thisfn, uri );
				g_free( uri );
				ok = FALSE;
			}

			g_free( fsubgroup );
			g_free( fgroup );
			g_free( ftype );
		}

		na_core_utils_slist_free( mimetypes );
	}

	return( ok );
}

/*
 * does the file fgroup/fsubgroup have a mimetype which is 'a sort of'
 *  mimetype specified one ?
 * for example, "image/jpeg" is clearly a sort of "image/ *"
 * but how to check if "msword/xml" is a sort of "application/xml" ??
 */
static gboolean
is_mimetype_of( const gchar *mimetype, const gchar *fgroup, const gchar *fsubgroup )
{
	static const gchar *thisfn = "na_icontext_is_mimetype_of";
	gboolean is_type_of;
	gchar *mgroup, *msubgroup;
	gchar *file_type;
	gchar *file_content_type, *def_content_type;
	gboolean is_a;

	file_type = g_strdup_printf( "%s/%s", fgroup, fsubgroup );
	file_content_type = g_content_type_from_mime_type( file_type );
	if( !file_content_type ){
		g_debug( "%s: mimetype=%s content_type=null", thisfn, file_type );
	}
	def_content_type = g_content_type_from_mime_type( mimetype );
	if( !def_content_type ){
		g_debug( "%s: mimetype=%s content_type=null", thisfn, mimetype );
	}
	if( file_content_type && def_content_type ){
		is_a = g_content_type_is_a( file_content_type, def_content_type );
		g_debug( "%s: def_mimetype=%s content_type=%s file_mimetype=%s content_type=%s is_a=%s",
				thisfn, mimetype, def_content_type, file_type, file_content_type,
				is_a ? "True":"False" );
	}

	g_free( file_type );
	g_free( file_content_type );
	g_free( def_content_type );

	if( !strcmp( mimetype, "*" ) || !strcmp( mimetype, st_mimetype_all )){
		return( TRUE );
	}

	split_mimetype( mimetype, &mgroup, &msubgroup );

	is_type_of = ( strcmp( fgroup, mgroup ) == 0 );
	if( is_type_of ){
		if( strcmp( msubgroup, "*" ) != 0 ){
			is_type_of = ( strcmp( fsubgroup, msubgroup ) == 0 );
		}
	}

	g_free( mgroup );
	g_free( msubgroup );

	return( is_type_of );
}

/*
 * split the given mimetype to its group and subgroup components
 */
static void
split_mimetype( const gchar *mimetype, gchar **group, gchar **subgroup )
{
	GSList *slist = na_core_utils_slist_from_split( mimetype, "/" );
	GSList *is = slist;
	*group = g_strdup(( const gchar * ) is->data );
	is = is->next;
	*subgroup = g_strdup(( const gchar * ) is->data );
	na_core_utils_slist_free( slist );
}

static gboolean
is_candidate_for_basenames( const NAIContext *object, guint target, GList *files )
{
	static const gchar *thisfn = "na_icontext_is_candidate_for_basenames";
	gboolean ok = TRUE;
	GSList *basenames = na_object_get_basenames( object );

	if( basenames ){
		if( strcmp( basenames->data, "*" ) != 0 || g_slist_length( basenames ) > 1 ){
			gboolean matchcase = na_object_get_matchcase( object );
			GSList *ib;
			GList *it;
			gchar *tmp;

			for( it = files ; it && ok ; it = it->next ){
				gchar *pattern, *bname, *bname_utf8;
				gboolean match, positive;
				gchar *pattern_utf8;

				bname = na_selected_info_get_basename( NA_SELECTED_INFO( it->data ));
				bname_utf8 = g_filename_to_utf8( bname, -1, NULL, NULL, NULL );
				if( !matchcase ){
					tmp = g_utf8_strdown( bname_utf8, -1 );
					g_free( bname_utf8 );
					bname_utf8 = tmp;
				}
				match = FALSE;

				for( ib = basenames ; ib && ok ; ib = ib->next ){
					pattern = matchcase ?
						g_strdup(( gchar * ) ib->data ) :
						g_utf8_strdown(( gchar * ) ib->data, -1 );
					positive = is_positive_assertion( pattern );
					pattern_utf8 = g_filename_to_utf8( positive ? pattern : pattern+1, -1, NULL, NULL, NULL );

					if( !positive || !match ){
						if( g_pattern_match_simple( pattern_utf8, bname_utf8 )){
							g_debug( "%s: condition=%s, positive=%s, basename=%s: matched",
									thisfn, pattern_utf8, positive ? "True":"False", bname_utf8 );
							if( positive ){
								match = TRUE;
							} else {
								ok = FALSE;
							}
						}
					}

					g_free( pattern_utf8 );
					g_free( pattern );
				}

				if( !match ){
					gchar *basenames_str = na_core_utils_slist_to_text( basenames );
					g_debug( "%s: no positive match found for Basenames=%s", thisfn, basenames_str );
					g_free( basenames_str );
					ok = FALSE;
				}

				g_free( bname_utf8 );
				g_free( bname );
			}
		}

		na_core_utils_slist_free( basenames );
	}

	return( ok );
}

static gboolean
is_candidate_for_selection_count( const NAIContext *object, guint target, GList *files )
{
	static const gchar *thisfn = "na_icontext_is_candidate_for_selection_count";
	gboolean ok = TRUE;
	gint limit;
	guint count;
	gchar *selection_count = na_object_get_selection_count( object );

	if( selection_count && strlen( selection_count )){
		limit = atoi( selection_count+1 );
		count = g_list_length( files );
		ok = FALSE;

		switch( selection_count[0] ){
			case '<':
				ok = ( count < limit );
				break;
			case '=':
				ok = ( count == limit );
				break;
			case '>':
				ok = ( count > limit );
				break;
			default:
				break;
		}
	}

	if( !ok ){
		g_debug( "%s: object is not candidate because SelectionCount=%s", thisfn, selection_count );
	}

	g_free( selection_count );

	return( ok );
}

/*
 * it is likely that all selected items have the same scheme, because they
 * are all in the same location and the scheme mainly depends on location
 * so we have here a great possible optimization by only testing the
 * first selected item.
 * note that this optimization may be wrong, for example when ran from the
 * command-line with a random set of pseudo-selected items
 * so we take care of only checking _distincts_ schemes of provided selection
 * against schemes conditions.
 */
static gboolean
is_candidate_for_schemes( const NAIContext *object, guint target, GList *files )
{
	static const gchar *thisfn = "na_icontext_is_candidate_for_schemes";
	gboolean ok = TRUE;
	GSList *schemes = na_object_get_schemes( object );

	if( schemes ){
		if( strcmp( schemes->data, "*" ) != 0 || g_slist_length( schemes ) > 1 ){
			GSList *distincts = NULL;
			GList *it;

			for( it = files ; it && ok ; it = it->next ){
				gchar *scheme = na_selected_info_get_uri_scheme( NA_SELECTED_INFO( it->data ));

				if( na_core_utils_slist_count( distincts, scheme ) == 0 ){
					GSList *is;
					gchar *pattern;
					gboolean match, positive;

					match = FALSE;
					distincts = g_slist_prepend( distincts, g_strdup( scheme ));

					for( is = schemes ; is && ok ; is = is->next ){
						pattern = ( gchar * ) is->data;
						positive = is_positive_assertion( pattern );

						if( !positive || !match ){
							if( is_compatible_scheme( positive ? pattern : pattern+1, scheme )){
								if( positive ){
									match = TRUE;
								} else {
									ok = FALSE;
								}
							}
						}
					}

					ok &= match;
				}

				g_free( scheme );
			}

			na_core_utils_slist_free( distincts );
		}

		if( !ok ){
			gchar *schemes_str = na_core_utils_slist_to_text( schemes );
			g_debug( "%s: object is not candidate because Schemes=%s", thisfn, schemes_str );
			g_free( schemes_str );
		}

		na_core_utils_slist_free( schemes );
	}

	return( ok );
}

static gboolean
is_compatible_scheme( const gchar *pattern, const gchar *scheme )
{
	gboolean compatible;

	compatible = FALSE;

	if( strcmp( pattern, "*" )){
		compatible = TRUE;
	} else {
		compatible = ( strcmp( pattern, scheme ) == 0 );
	}

	return( compatible );
}

/*
 * assumuing here the same sort of optimization than for schemes
 * i.e. we assume that all selected items are must probably located
 * in the same dirname
 * so we take care of only checking _distinct_ dirnames against folder
 * conditions
 */
static gboolean
is_candidate_for_folders( const NAIContext *object, guint target, GList *files )
{
	static const gchar *thisfn = "na_icontext_is_candidate_for_folders";
	gboolean ok = TRUE;
	GSList *folders = na_object_get_folders( object );

	if( folders ){
		if( strcmp( folders->data, "/" ) != 0 || g_slist_length( folders ) > 1 ){
			GSList *distincts = NULL;
			GList *it;

			for( it = files ; it && ok ; it = it->next ){
				gchar *dirname = na_selected_info_get_dirname( NA_SELECTED_INFO( it->data ));

				if( na_core_utils_slist_count( distincts, dirname ) == 0 ){
					g_debug( "%s: distinct dirname=%s", thisfn, dirname );

					GSList *id;
					gchar *dirname_utf8, *pattern_utf8;
					const gchar *pattern;
					gboolean match, positive;
					gboolean has_pattern;

					distincts = g_slist_prepend( distincts, g_strdup( dirname ));
					dirname_utf8 = g_filename_to_utf8( dirname, -1, NULL, NULL, NULL );
					match = FALSE;

					for( id = folders ; id && ok ; id = id->next ){
						pattern = ( const gchar * ) id->data;
						positive = is_positive_assertion( pattern );
						pattern_utf8 = g_filename_to_utf8( positive ? pattern : pattern+1, -1, NULL, NULL, NULL );
						has_pattern = ( g_strstr_len( pattern_utf8, -1, "*" ) != NULL );

						if( !positive || !match ){
							if(( has_pattern && g_pattern_match_simple( pattern_utf8, dirname_utf8 )) || g_str_has_prefix( dirname_utf8, pattern_utf8 )){
								g_debug( "%s: condition=%s, positive=%s: matched",
										thisfn, pattern, positive ? "True":"False" );
								if( positive ){
									match = TRUE;
								} else {
									ok = FALSE;
								}
							/*} else {
								g_debug( "%s: condition=%s, positive=%s: not matched",
										thisfn, pattern_utf8, positive ? "True":"False" );*/
							}
						}

						g_free( pattern_utf8 );
					}

					ok &= match;

					g_free( dirname_utf8 );
				}

				g_free( dirname );
			}

			na_core_utils_slist_free( distincts );
		}

		if( !ok ){
			gchar *folders_str = na_core_utils_slist_to_text( folders );
			g_debug( "%s: object is not candidate because Folders=%s", thisfn, folders_str );
			g_free( folders_str );
		}

		na_core_utils_slist_free( folders );
	}

	return( ok );
}

static gboolean
is_candidate_for_capabilities( const NAIContext *object, guint target, GList *files )
{
	static const gchar *thisfn = "na_icontext_is_candidate_for_capabilities";
	gboolean ok = TRUE;
	GSList *capabilities = na_object_get_capabilities( object );

	if( capabilities ){
		GSList *ic;
		GList *it;
		const gchar *cap;
		gboolean match, positive;

		for( it = files ; it && ok ; it = it->next ){
			for( ic = capabilities ; ic && ok ; ic = ic->next ){
				cap = ( const gchar * ) ic->data;
				positive = is_positive_assertion( cap );
				match = FALSE;

				if( !strcmp( positive ? cap : cap+1, "Owner" )){
					match = na_selected_info_is_owner( NA_SELECTED_INFO( it->data ), getlogin());

				} else if( !strcmp( positive ? cap : cap+1, "Readable" )){
					match = na_selected_info_is_readable( NA_SELECTED_INFO( it->data ));

				} else if( !strcmp( positive ? cap : cap+1, "Writable" )){
					match = na_selected_info_is_writable( NA_SELECTED_INFO( it->data ));

				} else if( !strcmp( positive ? cap : cap+1, "Executable" )){
					match = na_selected_info_is_executable( NA_SELECTED_INFO( it->data ));

				} else if( !strcmp( positive ? cap : cap+1, "Local" )){
					match = na_selected_info_is_local( NA_SELECTED_INFO( it->data ));

				} else {
					g_warning( "%s: unknown capability %s", thisfn, cap );
				}

				ok &= (( positive && match ) || ( !positive && !match ));
			}
		}

		if( !ok ){
			gchar *capabilities_str = na_core_utils_slist_to_text( capabilities );
			g_debug( "%s: object is not candidate because Capabilities=%s", thisfn, capabilities_str );
			g_free( capabilities_str );
		}

		na_core_utils_slist_free( capabilities );
	}

	return( ok );
}

static gboolean
is_valid_basenames( const NAIContext *object )
{
	gboolean valid;
	GSList *basenames;

	basenames = na_object_get_basenames( object );
	valid = basenames && g_slist_length( basenames ) > 0;
	na_core_utils_slist_free( basenames );

	if( !valid ){
		na_object_debug_invalid( object, "basenames" );
	}

	return( valid );
}

static gboolean
is_valid_mimetypes( const NAIContext *object )
{
	gboolean valid;
	GSList *mimetypes;

	mimetypes = na_object_get_mimetypes( object );
	valid = mimetypes && g_slist_length( mimetypes ) > 0;
	na_core_utils_slist_free( mimetypes );

	if( !valid ){
		na_object_debug_invalid( object, "mimetypes" );
	}

	return( valid );
}

static gboolean
is_valid_schemes( const NAIContext *object )
{
	gboolean valid;
	GSList *schemes;

	schemes = na_object_get_schemes( object );
	valid = schemes && g_slist_length( schemes ) > 0;
	na_core_utils_slist_free( schemes );

	if( !valid ){
		na_object_debug_invalid( object, "schemes" );
	}

	return( valid );
}

static gboolean
is_valid_folders( const NAIContext *object )
{
	gboolean valid;
	GSList *folders;

	folders = na_object_get_folders( object );
	valid = folders && g_slist_length( folders ) > 0;
	na_core_utils_slist_free( folders );

	if( !valid ){
		na_object_debug_invalid( object, "folders" );
	}

	return( valid );
}

/*
 * "image/ *" is a positive assertion
 * "!image/jpeg" is a negative one
 */
static gboolean
is_positive_assertion( const gchar *assertion )
{
	gboolean positive = TRUE;

	if( assertion ){
		gchar *dupped = g_strdup( assertion );
		const gchar *stripped = g_strstrip( dupped );
		if( stripped ){
			positive = ( stripped[0] != '!' );
		}
		g_free( dupped );
	}

	return( positive );
}
