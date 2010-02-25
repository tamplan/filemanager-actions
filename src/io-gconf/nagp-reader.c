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

#include <string.h>

#include <api/na-data-def.h>
#include <api/na-data-types.h>
#include <api/na-ifactory-provider.h>
#include <api/na-iio-provider.h>
#include <api/na-object-api.h>
#include <api/na-core-utils.h>
#include <api/na-gconf-utils.h>

#include "nagp-gconf-provider.h"
#include "nagp-keys.h"
#include "nagp-reader.h"

typedef struct {
	gchar        *path;
	NAObjectItem *parent;
}
	ReaderData;

static NAObjectItem  *read_item( NagpGConfProvider *provider, const gchar *path, GSList **messages );

static NADataBoxed   *read_data_item_properties( const NAIFactoryProvider *provider, NAObjectItem *item, ReaderData *reader_data, const NADataDef *def );
static NADataBoxed   *read_data_profile_properties( const NAIFactoryProvider *provider, NAObjectProfile *profile, ReaderData *reader_data, const NADataDef *def );
static void           read_done_item( const NAIFactoryProvider *provider, NAObjectItem *item, ReaderData *data, GSList **messages );
static void           read_done_action( const NAIFactoryProvider *provider, NAObjectAction *action, ReaderData *data, GSList **messages );
static void           read_done_action_load_profile( const NAIFactoryProvider *provider, ReaderData *data, const gchar *path, GSList **messages );
static void           read_done_profile( const NAIFactoryProvider *provider, NAObjectProfile *profile, ReaderData *data, GSList **messages );
#if 0
static void           read_item_action( NagpGConfProvider *provider, const gchar *path, NAObjectAction *action );
static void           read_item_action_properties( NagpGConfProvider *provider, GSList *entries, NAObjectAction *action );
static void           read_item_action_properties_v1( NagpGConfProvider *gconf, GSList *entries, NAObjectAction *action );
static void           read_item_action_profile( NagpGConfProvider *provider, NAObjectAction *action, const gchar *path );
static void           read_item_action_profile_properties( NagpGConfProvider *provider, GSList *entries, NAObjectProfile *profile );
static void           read_item_menu( NagpGConfProvider *provider, const gchar *path, NAObjectMenu *menu );
static void           read_item_menu_properties( NagpGConfProvider *provider, GSList *entries, NAObjectMenu *menu );
static void           read_object_item_properties( NagpGConfProvider *provider, GSList *entries, NAObjectItem *item );
#endif
static NADataBoxed   *get_boxed_from_path( const NagpGConfProvider *provider, const gchar *path, ReaderData *reader_data, const NADataDef *def );
static gboolean       is_key_writable( NagpGConfProvider *gconf, const gchar *key );

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

		listpath = na_gconf_utils_get_subdirs( self->private->gconf, NAGP_CONFIGURATIONS_PATH );

		for( ip = listpath ; ip ; ip = ip->next ){

			item = read_item( self, ( const gchar * ) ip->data, messages );
			if( item ){
				items_list = g_list_prepend( items_list, item );
			}
		}

		na_gconf_utils_free_subdirs( listpath );
	}

	g_debug( "%s: count=%d", thisfn, g_list_length( items_list ));
	return( items_list );
}

/*
 * path is here the full path to an item
 */
static NAObjectItem *
read_item( NagpGConfProvider *provider, const gchar *path, GSList **messages )
{
	static const gchar *thisfn = "nagp_gconf_provider_read_item";
	NAObjectItem *item;
	gboolean have_type;
	gchar *full_path;
	gchar *type;
	gchar *id;
	ReaderData *data;

	g_debug( "%s: provider=%p, path=%s", thisfn, ( void * ) provider, path );
	g_return_val_if_fail( NAGP_IS_GCONF_PROVIDER( provider ), NULL );
	g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider ), NULL );
	g_return_val_if_fail( !provider->private->dispose_has_run, NULL );

	have_type = na_gconf_utils_has_entry( provider->private->gconf, path, NAGP_ENTRY_TYPE );
	full_path = gconf_concat_dir_and_key( path, NAGP_ENTRY_TYPE );
	type = na_gconf_utils_read_string( provider->private->gconf, full_path, TRUE, NAGP_VALUE_TYPE_ACTION );
	g_free( full_path );
	item = NULL;

	/* a menu has a type='Menu'
	 */
	if( have_type && !strcmp( type, NAGP_VALUE_TYPE_MENU )){
		item = NA_OBJECT_ITEM( na_object_menu_new());
#if 0
		read_item_menu( provider, path, NA_OBJECT_MENU( item ));
#endif

	/* else this should be an action (no type, or type='Action')
	 */
	} else if( !have_type || !strcmp( type, NAGP_VALUE_TYPE_ACTION )){
		item = NA_OBJECT_ITEM( na_object_action_new());
#if 0
		read_item_action( provider, path, NA_OBJECT_ACTION( item ));
#endif

	} else {
		g_warning( "%s: unknown type '%s' at %s", thisfn, type, path );
	}

	g_free( type );

	if( item ){
		id = g_path_get_basename( path );
		na_object_set_id( item, id );
		g_free( id );

		data = g_new0( ReaderData, 1 );
		data->path = ( gchar * ) path;

		na_ifactory_provider_read_item(
				NA_IFACTORY_PROVIDER( provider ),
				data,
				NA_IFACTORY_OBJECT( item ),
				messages );

		g_free( data );
	}

	return( item );
}

NADataBoxed *
nagp_reader_read_data( const NAIFactoryProvider *provider, void *reader_data, const NAIFactoryObject *object, const NADataDef *def, GSList **messages )
{
	static const gchar *thisfn = "nagp_reader_read_data";
	NADataBoxed *boxed;

	g_return_val_if_fail( NA_IS_IFACTORY_PROVIDER( provider ), NULL );
	g_return_val_if_fail( NA_IS_IFACTORY_OBJECT( object ), NULL );

	g_debug( "%s: reader_data=%p, object=%p (%s), data=%s",
			thisfn,
			( void * ) reader_data,
			( void * ) object, G_OBJECT_TYPE_NAME( object ),
			def->name );

	if( !def->gconf_entry || !strlen( def->gconf_entry )){
		g_warning( "%s: GConf entry is not set for NADataDef %s", thisfn, def->name );
		return( NULL );
	}

	boxed = NULL;

	if( NA_IS_OBJECT_ITEM( object )){
		boxed = read_data_item_properties( provider, NA_OBJECT_ITEM( object ), ( ReaderData * ) reader_data, def );
	}

	if( NA_IS_OBJECT_PROFILE( object )){
		boxed = read_data_profile_properties( provider, NA_OBJECT_PROFILE( object ), ( ReaderData * ) reader_data, def );
	}

	return( boxed );
}

static NADataBoxed *
read_data_item_properties( const NAIFactoryProvider *provider, NAObjectItem *item, ReaderData *reader_data, const NADataDef *def )
{
	return( get_boxed_from_path( NAGP_GCONF_PROVIDER( provider ), reader_data->path, reader_data, def ));
}

static NADataBoxed *
read_data_profile_properties( const NAIFactoryProvider *provider, NAObjectProfile *profile, ReaderData *reader_data, const NADataDef *def )
{
	return( get_boxed_from_path( NAGP_GCONF_PROVIDER( provider ), reader_data->path, reader_data, def ));
}

void
nagp_reader_read_done( const NAIFactoryProvider *provider, void *reader_data, const NAIFactoryObject *object, GSList **messages  )
{
	static const gchar *thisfn = "nagp_reader_read_done";

	g_return_if_fail( NA_IS_IFACTORY_PROVIDER( provider ));
	g_return_if_fail( NA_IS_IFACTORY_OBJECT( object ));

	g_debug( "%s: provider=%p, reader_data=%p, object=%p (%s), messages=%p",
			thisfn,
			( void * ) provider,
			( void * ) reader_data,
			( void * ) object, G_OBJECT_TYPE_NAME( object ),
			( void * ) messages );

	if( NA_IS_OBJECT_ITEM( object )){
		read_done_item( provider, NA_OBJECT_ITEM( object ), ( ReaderData * ) reader_data, messages );
	}

	if( NA_IS_OBJECT_ACTION( object )){
		read_done_action( provider, NA_OBJECT_ACTION( object ), ( ReaderData * ) reader_data, messages );
	}

	if( NA_IS_OBJECT_PROFILE( object )){
		read_done_profile( provider, NA_OBJECT_PROFILE( object ), ( ReaderData * ) reader_data, messages );
	}

	g_debug( "quitting nagp_read_done for %s at %p", G_OBJECT_TYPE_NAME( object ), ( void * ) object );
}

static void
read_done_item( const NAIFactoryProvider *provider, NAObjectItem *item, ReaderData *data, GSList **messages )
{
	GSList *entries, *ie;
	gboolean writable;
	GConfEntry *gconf_entry;
	const gchar *key;

	data->parent = item;

	entries = na_gconf_utils_get_entries( NAGP_GCONF_PROVIDER( provider )->private->gconf, data->path );

	writable = TRUE;
	for( ie = entries ; ie && writable ; ie = ie->next ){
		gconf_entry = ( GConfEntry * ) ie->data;
		key = gconf_entry_get_key( gconf_entry );
		writable = is_key_writable( NAGP_GCONF_PROVIDER( provider ), key );
	}

	g_debug( "nagp_reader_read_done_item: writable=%s", writable ? "True":"False" );
	na_object_set_readonly( item, !writable );
}

static void
read_done_action( const NAIFactoryProvider *provider, NAObjectAction *action, ReaderData *data, GSList **messages )
{
	GSList *order;
	GSList *list_profiles;
	GSList *ip;
	gchar *profile_path;

	order = na_object_get_items_slist( action );
	list_profiles = na_gconf_utils_get_subdirs( NAGP_GCONF_PROVIDER( provider )->private->gconf, data->path );

	/* read profiles in the specified order
	 */
	for( ip = order ; ip ; ip = ip->next ){
		profile_path = gconf_concat_dir_and_key( data->path, ( gchar * ) ip->data );
		g_debug( "nagp_reader_read_done_action: loading profile=%s", ( gchar * ) ip->data );
		read_done_action_load_profile( provider, data, profile_path, messages );
		list_profiles = na_core_utils_slist_remove_ascii( list_profiles, profile_path );
		g_free( profile_path );
	}

	/* append other profiles
	 * this is mandatory for pre-2.29 actions which introduced order of profiles
	 */
	for( ip = list_profiles ; ip ; ip = ip->next ){
		g_debug( "nagp_reader_read_done_action: loading profile=%s", ( gchar * ) ip->data );
		read_done_action_load_profile( provider, data, ( const gchar * ) ip->data, messages );
	}
}

static void
read_done_action_load_profile( const NAIFactoryProvider *provider, ReaderData *data, const gchar *path, GSList **messages )
{
	gchar *id;
	ReaderData *profile_data;

	NAObjectProfile *profile = na_object_profile_new();

	id = g_path_get_basename( path );
	na_object_set_id( profile, id );
	g_free( id );

	profile_data = g_new0( ReaderData, 1 );
	profile_data->parent = data->parent;
	profile_data->path = ( gchar * ) path;

	na_ifactory_provider_read_item(
			NA_IFACTORY_PROVIDER( provider ),
			profile_data,
			NA_IFACTORY_OBJECT( profile ),
			messages );

	g_free( profile_data );
}

static void
read_done_profile( const NAIFactoryProvider *provider, NAObjectProfile *profile, ReaderData *data, GSList **messages )
{
	na_object_attach_profile( data->parent, profile );
}

#if 0
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

	uuid = g_path_get_basename( path );
	na_object_set_id( action, uuid );
	g_free( uuid );

	entries = na_gconf_utils_get_entries( provider->private->gconf, path );
	read_item_action_properties( provider, entries, action  );

	order = na_object_get_items_slist( action );
	list_profiles = na_gconf_utils_get_subdirs( provider->private->gconf, path );

	if( list_profiles ){

		/* read profiles in the specified order
		 */
		for( ip = order ; ip ; ip = ip->next ){
			profile_path = gconf_concat_dir_and_key( path, ( gchar * ) ip->data );
			read_item_action_profile( provider, action, profile_path );
			list_profiles = na_core_utils_slist_remove_ascii( list_profiles, profile_path );
			g_free( profile_path );
		}

		/* append other profiles
		 * but this may be an inconvenient for the runtime plugin ?
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

	na_core_utils_slist_free( order );
	na_gconf_utils_free_subdirs( list_profiles );
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
	gchar *action_label;
	gchar *toolbar_label;

	read_object_item_properties( provider, entries, NA_OBJECT_ITEM( action ) );

	if( na_gconf_utils_get_string_from_entries( entries, NAGP_ENTRY_VERSION, &version )){
		na_object_set_version( action, version );
		g_free( version );
	}

	if( na_gconf_utils_get_bool_from_entries( entries, NAGP_ENTRY_TARGET_SELECTION, &target_selection )){
		na_object_set_target_selection( action, target_selection );
	}

	if( na_gconf_utils_get_bool_from_entries( entries, NAGP_ENTRY_TARGET_BACKGROUND, &target_background )){
		na_object_set_target_background( action, target_background );
	}

	if( na_gconf_utils_get_bool_from_entries( entries, NAGP_ENTRY_TARGET_TOOLBAR, &target_toolbar )){
		na_object_set_target_toolbar( action, target_toolbar );
	}

	/* toolbar label is the same that action if empty */
	action_label = na_object_get_label( action );
	toolbar_same_label = FALSE;
	na_gconf_utils_get_string_from_entries( entries, NAGP_ENTRY_TOOLBAR_LABEL, &toolbar_label );
	if( !toolbar_label || !g_utf8_strlen( toolbar_label, -1 ) || !g_utf8_collate( toolbar_label, action_label )){
		toolbar_same_label = TRUE;
	}
	if( toolbar_same_label ){
		g_free( toolbar_label );
		toolbar_label = g_strdup( action_label );
	}
	na_object_set_toolbar_label( action, toolbar_label );
	na_object_set_toolbar_same_label( action, toolbar_same_label );
	g_free( action_label );
	g_free( toolbar_label );
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

	na_object_attach_profile( action, profile );

	read_item_action_profile_properties( provider, entries, profile );
}

static void
read_item_action_profile( NagpGConfProvider *provider, NAObjectAction *action, const gchar *path )
{
	/*static const gchar *thisfn = "nagp_reader_read_item_action_profile";*/
	NAObjectProfile *profile;
	gchar *name;
	GSList *entries;

	g_return_if_fail( NA_IS_OBJECT_ACTION( action ));

	profile = na_object_profile_new();

	name = g_path_get_basename( path );
	na_object_set_id( profile, name );
	g_free( name );

	entries = na_gconf_utils_get_entries( provider->private->gconf, path );
	read_item_action_profile_properties( provider, entries, profile );
	na_gconf_utils_free_entries( entries );

	na_object_attach_profile( action, profile );
}

static void
read_item_action_profile_properties( NagpGConfProvider *provider, GSList *entries, NAObjectProfile *profile )
{
	/*static const gchar *thisfn = "nagp_gconf_provider_read_item_action_profile_properties";*/
	gchar *label, *path, *parameters;
	GSList *basenames, *schemes, *mimetypes;
	gboolean isfile, isdir, multiple, matchcase;
	GSList *folders;

	if( na_gconf_utils_get_string_from_entries( entries, NAGP_ENTRY_PROFILE_LABEL, &label )){
		na_object_set_label( profile, label );
		g_free( label );
	}

	if( na_gconf_utils_get_string_from_entries( entries, NAGP_ENTRY_PATH, &path )){
		na_object_set_path( profile, path );
		g_free( path );
	}

	if( na_gconf_utils_get_string_from_entries( entries, NAGP_ENTRY_PARAMETERS, &parameters )){
		na_object_set_parameters( profile, parameters );
		g_free( parameters );
	}

	if( na_gconf_utils_get_string_list_from_entries( entries, NAGP_ENTRY_BASENAMES, &basenames )){
		na_object_set_basenames( profile, basenames );
		na_core_utils_slist_free( basenames );
	}

	if( na_gconf_utils_get_bool_from_entries( entries, NAGP_ENTRY_ISFILE, &isfile )){
		na_object_set_isfile( profile, isfile );
	}

	if( na_gconf_utils_get_bool_from_entries( entries, NAGP_ENTRY_ISDIR, &isdir )){
		na_object_set_isdir( profile, isdir );
	}

	if( na_gconf_utils_get_bool_from_entries( entries, NAGP_ENTRY_MULTIPLE, &multiple )){
		na_object_set_multiple( profile, multiple );
	}

	if( na_gconf_utils_get_string_list_from_entries( entries, NAGP_ENTRY_SCHEMES, &schemes )){
		na_object_set_schemes( profile, schemes );
		na_core_utils_slist_free( schemes );
	}

	/* handle matchcase+mimetypes
	 * note that default values for 1.0 version have been set
	 * in na_object_profile_instance_init
	 */
	if( na_gconf_utils_get_bool_from_entries( entries, NAGP_ENTRY_MATCHCASE, &matchcase )){
		na_object_set_matchcase( profile, matchcase );
	}

	if( na_gconf_utils_get_string_list_from_entries( entries, NAGP_ENTRY_MIMETYPES, &mimetypes )){
		na_object_set_mimetypes( profile, mimetypes );
		na_core_utils_slist_free( mimetypes );
	}

	if( na_gconf_utils_get_string_list_from_entries( entries, NAGP_ENTRY_FOLDERS, &folders )){
		na_object_set_folders( profile, folders );
		na_core_utils_slist_free( folders );
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

	uuid = g_path_get_basename( path );
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

	if( !na_gconf_utils_get_string_from_entries( entries, NAGP_ENTRY_LABEL, &label )){
		id = na_object_get_id( item );
		g_warning( "%s: no label found for NAObjectItem %s", thisfn, id );
		g_free( id );
		label = g_strdup( "" );
	}
	na_object_set_label( item, label );
	g_free( label );

	if( na_gconf_utils_get_string_from_entries( entries, NAGP_ENTRY_TOOLTIP, &tooltip )){
		na_object_set_tooltip( item, tooltip );
		g_free( tooltip );
	}

	if( na_gconf_utils_get_string_from_entries( entries, NAGP_ENTRY_ICON, &icon )){
		na_object_set_icon( item, icon );
		g_free( icon );
	}

	if( na_gconf_utils_get_bool_from_entries( entries, NAGP_ENTRY_ENABLED, &enabled )){
		na_object_set_enabled( item, enabled );
	}

	if( na_gconf_utils_get_string_list_from_entries( entries, NAGP_ENTRY_ITEMS_LIST, &subitems )){
		na_object_set_items_slist( item, subitems );
		na_core_utils_slist_free( subitems );
	}

	writable = TRUE;
	for( ie = entries ; ie && writable ; ie = ie->next ){
		gconf_entry = ( GConfEntry * ) ie->data;
		key = gconf_entry_get_key( gconf_entry );
		writable = is_key_writable( provider, key );
	}
	na_object_set_readonly( item, !writable );
}
#endif

static NADataBoxed *
get_boxed_from_path( const NagpGConfProvider *provider, const gchar *path, ReaderData *reader_data, const NADataDef *def )
{
	static const gchar *thisfn = "nagp_reader_get_boxed_from_path";
	NADataBoxed *boxed;
	gboolean have_entry;
	gchar *str_value;
	gboolean bool_value;
	GSList *slist_value;
	gint int_value;

	boxed = NULL;
	have_entry = na_gconf_utils_has_entry( provider->private->gconf, path, def->gconf_entry );

	if( have_entry ){
		boxed = na_data_boxed_new( def );
		gchar *entry_path = gconf_concat_dir_and_key( path, def->gconf_entry );

		switch( def->type ){

			case NAFD_TYPE_STRING:
			case NAFD_TYPE_LOCALE_STRING:
				str_value = na_gconf_utils_read_string( provider->private->gconf, entry_path, FALSE, NULL );
				na_data_boxed_set_from_string( boxed, str_value );
				g_free( str_value );
				break;

			case NAFD_TYPE_BOOLEAN:
				bool_value = na_gconf_utils_read_bool( provider->private->gconf, entry_path, FALSE, FALSE );
				na_data_boxed_set_from_void( boxed, GUINT_TO_POINTER( bool_value ));
				break;

			case NAFD_TYPE_STRING_LIST:
				slist_value = na_gconf_utils_read_string_list( provider->private->gconf, entry_path );
				na_data_boxed_set_from_void( boxed, slist_value );
				na_core_utils_slist_free( slist_value );
				break;

			case NAFD_TYPE_UINT:
				int_value = na_gconf_utils_read_int( provider->private->gconf, entry_path, FALSE, 0 );
				na_data_boxed_set_from_void( boxed, GUINT_TO_POINTER( int_value ));
				break;

			default:
				g_warning( "%s: unknown type=%u for %s", thisfn, def->type, def->name );
				g_free( boxed );
				boxed = NULL;
		}

		g_free( entry_path );
	}

	return( boxed );
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
