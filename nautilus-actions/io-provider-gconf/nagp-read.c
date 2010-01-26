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

#include <nautilus-actions/api/na-iio-provider.h>
#include <nautilus-actions/api/na-object-api.h>

/* only possible because we are an internal plugin */
#include <runtime/na-gconf-utils.h>

#include "nagp-gconf-provider.h"
#include "nagp-keys.h"
#include "nagp-read.h"
#include "nagp-write.h"

static NAObjectItem  *read_item( NagpGConfProvider *provider, const gchar *path );
static void           read_item_action( NagpGConfProvider *provider, const gchar *path, NAObjectAction *action );
static void           read_item_action_properties( NagpGConfProvider *provider, GSList *entries, NAObjectAction *action );
static void           read_item_action_properties_v1( NagpGConfProvider *gconf, GSList *entries, NAObjectAction *action );
static void           read_item_action_profile( NagpGConfProvider *provider, NAObjectAction *action, const gchar *path );
static void           read_item_action_profile_properties( NagpGConfProvider *provider, GSList *entries, NAObjectProfile *profile );
static void           read_item_menu( NagpGConfProvider *provider, const gchar *path, NAObjectMenu *menu );
static void           read_item_menu_properties( NagpGConfProvider *provider, GSList *entries, NAObjectMenu *menu );
static void           read_object_item_properties( NagpGConfProvider *provider, GSList *entries, NAObjectItem *item );

static GSList        *get_subdirs( GConfClient *gconf, const gchar *path );
static void           free_subdirs( GSList *subdirs );
static gboolean       is_key_writable( NagpGConfProvider *gconf, const gchar *key );
static gboolean       has_entry( GConfClient *gconf, const gchar *path, const gchar *entry );
static GSList        *remove_from_gslist( GSList *list, const gchar *str );
static void           free_gslist( GSList *list );

/*
 * nagp_iio_provider_read_items:
 *
 * Note that whatever be the version of the readen action, it will be
 * stored as a #NAObjectAction and its set of #NAObjectProfile of the same,
 * latest, version of these classes.
 */
GList *
nagp_iio_provider_read_items( const NAIIOProvider *provider, GSList **messages )
{
	static const gchar *thisfn = "nagp_gconf_provider_iio_provider_read_items";
	NagpGConfProvider *self;
	GList *items_list = NULL;
	GSList *listpath, *ip;
	NAObjectItem *item;

	g_debug( "%s: provider=%p, messages=%p", thisfn, ( void * ) provider, ( void * ) messages );

	g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider ), NULL );
	g_return_val_if_fail( NAGP_IS_GCONF_PROVIDER( provider ), NULL );
	self = NAGP_GCONF_PROVIDER( provider );

	if( !self->private->dispose_has_run ){

		listpath = get_subdirs( self->private->gconf, NA_GCONF_CONFIG_PATH );

		for( ip = listpath ; ip ; ip = ip->next ){

			item = read_item( self, ( const gchar * ) ip->data );
			if( item ){
				items_list = g_list_prepend( items_list, item );
			}
		}

		free_subdirs( listpath );
	}

	return( items_list );
}

static NAObjectItem *
read_item( NagpGConfProvider *provider, const gchar *path )
{
	static const gchar *thisfn = "nagp_gconf_provider_read_item";
	NAObjectItem *item;
	gboolean have_type;
	gchar *full_path;
	gchar *type;

	g_debug( "%s: provider=%p, path=%s", thisfn, ( void * ) provider, path );
	g_return_val_if_fail( NAGP_IS_GCONF_PROVIDER( provider ), NULL );
	g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider ), NULL );
	g_return_val_if_fail( !provider->private->dispose_has_run, NULL );

	have_type = has_entry( provider->private->gconf, path, OBJECT_ITEM_TYPE_ENTRY );
	full_path = gconf_concat_dir_and_key( path, OBJECT_ITEM_TYPE_ENTRY );
	type = na_gconf_utils_read_string( provider->private->gconf, full_path, TRUE, OBJECT_ITEM_TYPE_ACTION );
	g_free( full_path );
	item = NULL;

	/* a menu has a type='Menu'
	 */
	if( have_type && !strcmp( type, OBJECT_ITEM_TYPE_MENU )){
		item = NA_OBJECT_ITEM( na_object_menu_new());
		read_item_menu( provider, path, NA_OBJECT_MENU( item ));

	/* else this should be an action (no type, or type='Action')
	 */
	} else if( !have_type || !strcmp( type, OBJECT_ITEM_TYPE_ACTION )){
		item = NA_OBJECT_ITEM( na_object_action_new());
		read_item_action( provider, path, NA_OBJECT_ACTION( item ));

	} else {
		g_warning( "%s: unknown type '%s' at %s", thisfn, type, path );
	}

	g_free( type );

	return( item );
}

/*
 * load and set the properties of the specified action
 * at least we must have a label, as all other entries can have
 * suitable default values
 *
 * we have to deal with successive versions of action schema :
 *
 * - version = '1.0'
 *   action+= uuid+label+tooltip+icon
 *   action+= path+parameters+basenames+isdir+isfile+multiple+schemes
 *
 * - version > '1.0'
 *   action+= matchcase+mimetypes
 *
 * - version = '2.0' which introduces the 'profile' notion
 *   profile += name+label
 *
 * Profiles are kept in the order specified in 'items' entry if it exists.
 */
static void
read_item_action( NagpGConfProvider *provider, const gchar *path, NAObjectAction *action )
{
	static const gchar *thisfn = "nagp_gconf_provider_read_item_action";
	gchar *uuid;
	GSList *entries, *list_profiles, *ip;
	GSList *order;
	gchar *profile_path;

	g_debug( "%s: provider=%p, path=%s, action=%p",
			thisfn, ( void * ) provider, path, ( void * ) action );
	g_return_if_fail( NA_IS_OBJECT_ACTION( action ));

	uuid = na_gconf_utils_path_to_key( path );
	na_object_set_id( action, uuid );
	g_free( uuid );

	entries = na_gconf_utils_get_entries( provider->private->gconf, path );
	read_item_action_properties( provider, entries, action  );

	order = na_object_item_get_items_string_list( NA_OBJECT_ITEM( action ));
	list_profiles = get_subdirs( provider->private->gconf, path );

	if( list_profiles ){

		/* read profiles in the specified order
		 */
		for( ip = order ; ip ; ip = ip->next ){
			profile_path = gconf_concat_dir_and_key( path, ( gchar * ) ip->data );
			read_item_action_profile( provider, action, profile_path );
			list_profiles = remove_from_gslist( list_profiles, profile_path );
			g_free( profile_path );
		}

		/* read other profiles
		 */
		for( ip = list_profiles ; ip ; ip = ip->next ){
			profile_path = g_strdup(( gchar * ) ip->data );
			read_item_action_profile( provider, action, profile_path );
			g_free( profile_path );
		}

	/* if there is no subdir, this may be a valid v1 or an invalid v2
	 * at least try to read some properties
	 */
	} else {
		read_item_action_properties_v1( provider, entries, action );
	}

	free_gslist( order );
	free_subdirs( list_profiles );
	na_gconf_utils_free_entries( entries );
}

/*
 * set the item properties into the action, dealing with successive
 * versions
 */
static void
read_item_action_properties( NagpGConfProvider *provider, GSList *entries, NAObjectAction *action )
{
	gchar *version;
	gboolean target_selection, target_background, target_toolbar;
	gboolean toolbar_same_label;
	gchar *toolbar_label;

	read_object_item_properties( provider, entries, NA_OBJECT_ITEM( action ) );

	if( na_gconf_utils_get_string_from_entries( entries, ACTION_VERSION_ENTRY, &version )){
		na_object_action_set_version( action, version );
		g_free( version );
	}

	if( na_gconf_utils_get_bool_from_entries( entries, OBJECT_ITEM_TARGET_SELECTION_ENTRY, &target_selection )){
		na_object_action_set_target_selection( action, target_selection );
	}

	if( na_gconf_utils_get_bool_from_entries( entries, OBJECT_ITEM_TARGET_BACKGROUND_ENTRY, &target_background )){
		na_object_action_set_target_background( action, target_background );
	}

	if( na_gconf_utils_get_bool_from_entries( entries, OBJECT_ITEM_TARGET_TOOLBAR_ENTRY, &target_toolbar )){
		na_object_action_set_target_toolbar( action, target_toolbar );
	}

	if( na_gconf_utils_get_bool_from_entries( entries, OBJECT_ITEM_TOOLBAR_SAME_LABEL_ENTRY, &toolbar_same_label )){
		na_object_action_toolbar_set_same_label( action, toolbar_same_label );

	} else {
		toolbar_same_label = na_object_action_toolbar_use_same_label( action );
	}

	if( na_gconf_utils_get_string_from_entries( entries, OBJECT_ITEM_TOOLBAR_LABEL_ENTRY, &toolbar_label )){
		na_object_action_toolbar_set_label( action, toolbar_label );
		g_free( toolbar_label );

	} else if( toolbar_same_label ){
		toolbar_label = na_object_get_label( action );
		na_object_action_toolbar_set_label( action, toolbar_label );
		g_free( toolbar_label );
	}
}

/*
 * version is marked as less than "2.0"
 * we handle so only one profile, which is already loaded
 * action+= path+parameters+basenames+isdir+isfile+multiple+schemes
 * if version greater than "1.0", we have also matchcase+mimetypes
 */
static void
read_item_action_properties_v1( NagpGConfProvider *provider, GSList *entries, NAObjectAction *action )
{
	NAObjectProfile *profile = na_object_profile_new();

	na_object_action_attach_profile( action, profile );

	read_item_action_profile_properties( provider, entries, profile );
}

static void
read_item_action_profile( NagpGConfProvider *provider, NAObjectAction *action, const gchar *path )
{
	NAObjectProfile *profile;
	gchar *name;
	GSList *entries;

	g_return_if_fail( NA_IS_OBJECT_ACTION( action ));

	profile = na_object_profile_new();

	name = na_gconf_utils_path_to_key( path );
	na_object_set_id( profile, name );
	g_free( name );

	entries = na_gconf_utils_get_entries( provider->private->gconf, path );
	read_item_action_profile_properties( provider, entries, profile );
	na_gconf_utils_free_entries( entries );

	na_object_action_attach_profile( action, profile );
}

static void
read_item_action_profile_properties( NagpGConfProvider *provider, GSList *entries, NAObjectProfile *profile )
{
	/*static const gchar *thisfn = "nagp_gconf_provider_read_item_action_profile_properties";*/
	gchar *label, *path, *parameters;
	GSList *basenames, *schemes, *mimetypes;
	gboolean isfile, isdir, multiple, matchcase;
	GSList *folders;

	if( na_gconf_utils_get_string_from_entries( entries, ACTION_PROFILE_LABEL_ENTRY, &label )){
		na_object_set_label( profile, label );
		g_free( label );
	}

	if( na_gconf_utils_get_string_from_entries( entries, ACTION_PATH_ENTRY, &path )){
		na_object_profile_set_path( profile, path );
		g_free( path );
	}

	if( na_gconf_utils_get_string_from_entries( entries, ACTION_PARAMETERS_ENTRY, &parameters )){
		na_object_profile_set_parameters( profile, parameters );
		g_free( parameters );
	}

	if( na_gconf_utils_get_string_list_from_entries( entries, ACTION_BASENAMES_ENTRY, &basenames )){
		na_object_profile_set_basenames( profile, basenames );
		free_gslist( basenames );
	}

	if( na_gconf_utils_get_bool_from_entries( entries, ACTION_ISFILE_ENTRY, &isfile )){
		na_object_profile_set_isfile( profile, isfile );
	}

	if( na_gconf_utils_get_bool_from_entries( entries, ACTION_ISDIR_ENTRY, &isdir )){
		na_object_profile_set_isdir( profile, isdir );
	}

	if( na_gconf_utils_get_bool_from_entries( entries, ACTION_MULTIPLE_ENTRY, &multiple )){
		na_object_profile_set_multiple( profile, multiple );
	}

	if( na_gconf_utils_get_string_list_from_entries( entries, ACTION_SCHEMES_ENTRY, &schemes )){
		na_object_profile_set_schemes( profile, schemes );
		free_gslist( schemes );
	}

	/* handle matchcase+mimetypes
	 * note that default values for 1.0 version have been set
	 * in na_object_profile_instance_init
	 */
	if( na_gconf_utils_get_bool_from_entries( entries, ACTION_MATCHCASE_ENTRY, &matchcase )){
		na_object_profile_set_matchcase( profile, matchcase );
	}

	if( na_gconf_utils_get_string_list_from_entries( entries, ACTION_MIMETYPES_ENTRY, &mimetypes )){
		na_object_profile_set_mimetypes( profile, mimetypes );
		free_gslist( mimetypes );
	}

	if( na_gconf_utils_get_string_list_from_entries( entries, ACTION_FOLDERS_ENTRY, &folders )){
		na_object_profile_set_folders( profile, folders );
		free_gslist( folders );
	}
}

static void
read_item_menu( NagpGConfProvider *provider, const gchar *path, NAObjectMenu *menu )
{
	static const gchar *thisfn = "nagp_gconf_provider_read_item_menu";
	gchar *uuid;
	GSList *entries;

	g_debug( "%s: provider=%p, path=%s, menu=%p",
			thisfn, ( void * ) provider, path, ( void * ) menu );
	g_return_if_fail( NA_IS_OBJECT_MENU( menu ));

	uuid = na_gconf_utils_path_to_key( path );
	na_object_set_id( menu, uuid );
	g_free( uuid );

	entries = na_gconf_utils_get_entries( provider->private->gconf, path );
	read_item_menu_properties( provider, entries, menu  );
	na_gconf_utils_free_entries( entries );
}

static void
read_item_menu_properties( NagpGConfProvider *provider, GSList *entries, NAObjectMenu *menu )
{
	read_object_item_properties( provider, entries, NA_OBJECT_ITEM( menu ) );
}

/*
 * set the properties into the NAObjectItem
 *
 * The NAObjectItem is set to 'read-only' if at least one the entries is
 * not writable ; in other words, a writable NAObjectItem has all its
 * entries writable.
 */
static void
read_object_item_properties( NagpGConfProvider *provider, GSList *entries, NAObjectItem *item )
{
	static const gchar *thisfn = "nagp_gconf_provider_read_object_item_properties";
	gchar *id, *label, *tooltip, *icon;
	gboolean enabled;
	GSList *subitems;
	GSList *ie;
	GConfEntry *gconf_entry;
	const gchar *key;
	gboolean writable;

	if( !na_gconf_utils_get_string_from_entries( entries, OBJECT_ITEM_LABEL_ENTRY, &label )){
		id = na_object_get_id( item );
		g_warning( "%s: no label found for NAObjectItem %s", thisfn, id );
		g_free( id );
		label = g_strdup( "" );
	}
	na_object_set_label( item, label );
	g_free( label );

	if( na_gconf_utils_get_string_from_entries( entries, OBJECT_ITEM_TOOLTIP_ENTRY, &tooltip )){
		na_object_set_tooltip( item, tooltip );
		g_free( tooltip );
	}

	if( na_gconf_utils_get_string_from_entries( entries, OBJECT_ITEM_ICON_ENTRY, &icon )){
		na_object_set_icon( item, icon );
		g_free( icon );
	}

	if( na_gconf_utils_get_bool_from_entries( entries, OBJECT_ITEM_ENABLED_ENTRY, &enabled )){
		na_object_set_enabled( item, enabled );
	}

	if( na_gconf_utils_get_string_list_from_entries( entries, OBJECT_ITEM_LIST_ENTRY, &subitems )){
		na_object_item_set_items_string_list( item, subitems );
		free_gslist( subitems );
	}

	writable = TRUE;
	for( ie = entries ; ie && writable ; ie = ie->next ){
		gconf_entry = ( GConfEntry * ) ie->data;
		key = gconf_entry_get_key( gconf_entry );
		writable = is_key_writable( provider, key );
	}
	na_object_set_readonly( item, !writable );
}

/*
 * key must be an existing entry (not a dir) to get a relevant return
 * value ; else we get FALSE
 */
static gboolean
is_key_writable( NagpGConfProvider *gconf, const gchar *key )
{
	static const gchar *thisfn = "nagp_read_is_key_writable";
	GError *error = NULL;
	gboolean is_writable;

	is_writable = gconf_client_key_is_writable( gconf->private->gconf, key, &error );
	if( error ){
		g_warning( "%s: gconf_client_key_is_writable: %s", thisfn, error->message );
		g_error_free( error );
		error = NULL;
		is_writable = FALSE;
	}

	return( is_writable );
}

/*
 * get_subdirs:
 * @gconf: a  #GConfClient instance.
 * @path: a full path to be readen.
 *
 * Returns: a GSList of full path subdirectories.
 *
 * The returned list should be na_gconf_utils_free_subdirs() by the
 * caller.
 */
static GSList *
get_subdirs( GConfClient *gconf, const gchar *path )
{
	static const gchar *thisfn = "nagp_read_get_subdirs";
	GError *error = NULL;
	GSList *list_subdirs;

	list_subdirs = gconf_client_all_dirs( gconf, path, &error );

	if( error ){
		g_warning( "%s: path=%s, error=%s", thisfn, path, error->message );
		g_error_free( error );
		return(( GSList * ) NULL );
	}

	return( list_subdirs );
}

/*
 * free_subdirs:
 * @subdirs: a list of subdirs as returned by get_subdirs().
 *
 * Release the list of subdirs.
 */
static void
free_subdirs( GSList *subdirs )
{
	free_gslist( subdirs );
}

/*
 * na_gconf_utils_have_subdir:
 * @gconf: a  #GConfClient instance.
 * @path: a full path to be readen.
 *
 * Returns: %TRUE if the specified path has at least one subdirectory,
 * %FALSE else.
 */
/*static gboolean
na_gconf_utils_have_subdir( GConfClient *gconf, const gchar *path )
{
	GSList *listpath;
	gboolean have_subdir;

	listpath = get_subdirs( gconf, path );
	have_subdir = ( listpath && g_slist_length( listpath ));
	free_subdirs( listpath );

	return( have_subdir );
}*/

/*
 * has_entry:
 * @gconf: a  #GConfClient instance.
 * @path: the full path of a key.
 * @entry: the entry to be tested.
 *
 * Returns: %TRUE if the given @entry exists for the given @path,
 * %FALSE else.
 */
static gboolean
has_entry( GConfClient *gconf, const gchar *path, const gchar *entry )
{
	static const gchar *thisfn = "has_entry";
	gboolean have_entry = FALSE;
	GError *error = NULL;
	gchar *key;
	GConfValue *value;

	key = g_strdup_printf( "%s/%s", path, entry );

	value = gconf_client_get_without_default( gconf, key, &error );

	if( error ){
		g_warning( "%s: key=%s, error=%s", thisfn, key, error->message );
		g_error_free( error );
		if( value ){
			gconf_value_free( value );
			value = NULL;
		}
	}

	if( value ){
		have_entry = TRUE;
		gconf_value_free( value );
	}

	g_free( key );

	return( have_entry );
}

/*
 * remove_from_gslist:
 * @list: the GSList to be updated.
 * @str: the string to be removed.
 *
 * Removes from the @list the item which has a string which is equal to
 * @str.
 *
 * Returns: the new @list start position.
 */
static GSList *
remove_from_gslist( GSList *list, const gchar *str )
{
	GSList *is;

	for( is = list ; is ; is = is->next ){
		const gchar *istr = ( const gchar * ) is->data;
		if( !g_utf8_collate( str, istr )){
			g_free( is->data );
			list = g_slist_delete_link( list, is );
			break;
		}
	}

	return( list );
}

/*
 * free_gslist:
 * @list: the GSList to be freed.
 *
 * Frees a GSList of strings.
 */
static void
free_gslist( GSList *list )
{
	g_slist_foreach( list, ( GFunc ) g_free, NULL );
	g_slist_free( list );
}
