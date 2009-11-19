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

#include <string.h>

#include <nautilus-actions/api/na-iio-provider.h>
#include <nautilus-actions/api/na-object-api.h>
#include <nautilus-actions/api/na-gconf-monitor.h>

/* only possible because we are an internal plugin */
#include <runtime/na-gconf-utils.h>
#include <runtime/na-utils.h>

#include "nagp-gconf-provider.h"
#include "nagp-keys.h"

/* private class data
 */
struct NagpGConfProviderClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NagpGConfProviderPrivate {
	gboolean     dispose_has_run;
	GConfClient *gconf;
	GList       *monitors;
};

static GType         st_module_type = 0;
static GObjectClass *st_parent_class = NULL;

static void           class_init( NagpGConfProviderClass *klass );
static void           iio_provider_iface_init( NAIIOProviderInterface *iface );
static void           instance_init( GTypeInstance *instance, gpointer klass );
static void           instance_dispose( GObject *object );
static void           instance_finalize( GObject *object );

static GList         *install_monitors( NagpGConfProvider *provider );
static void           config_path_changed_cb( GConfClient *client, guint cnxn_id, GConfEntry *entry, NagpGConfProvider *provider );
#if 0
static NAPivotNotify *entry_to_notify( const GConfEntry *entry );
#endif

static GList         *iio_provider_read_items( const NAIIOProvider *provider, GSList **messages );
static NAObjectItem  *read_item( NagpGConfProvider *provider, const gchar *path );
static void           read_item_action( NagpGConfProvider *provider, const gchar *path, NAObjectAction *action );
static void           read_item_action_properties( NagpGConfProvider *provider, GSList *entries, NAObjectAction *action );
static void           read_item_action_properties_v1( NagpGConfProvider *gconf, GSList *entries, NAObjectAction *action );
static void           read_item_action_profile( NagpGConfProvider *provider, NAObjectAction *action, const gchar *path );
static void           read_item_action_profile_properties( NagpGConfProvider *provider, GSList *entries, NAObjectProfile *profile );
static void           read_item_menu( NagpGConfProvider *provider, const gchar *path, NAObjectMenu *menu );
static void           read_item_menu_properties( NagpGConfProvider *provider, GSList *entries, NAObjectMenu *menu );
static void           read_object_item_properties( NagpGConfProvider *provider, GSList *entries, NAObjectItem *item );

static gboolean       iio_provider_is_willing_to_write( const NAIIOProvider *provider );

static gboolean       iio_provider_is_writable( const NAIIOProvider *provider, const NAObjectItem *item );

static guint          iio_provider_write_item( const NAIIOProvider *provider, const NAObjectItem *item, GSList **message );
static gboolean       write_item_action( NagpGConfProvider *gconf, const NAObjectAction *action, GSList **message );
static gboolean       write_item_menu( NagpGConfProvider *gconf, const NAObjectMenu *menu, GSList **message );
static gboolean       write_object_item( NagpGConfProvider *gconf, const NAObjectItem *item, GSList **message );

static guint          iio_provider_delete_item( const NAIIOProvider *provider, const NAObjectItem *item, GSList **message );

static gboolean       key_is_writable( NagpGConfProvider *gconf, const gchar *path );

static gboolean       write_str( NagpGConfProvider *gconf, const gchar *uuid, const gchar *name, const gchar *key, gchar *value, GSList **message );
static gboolean       write_bool( NagpGConfProvider *gconf, const gchar *uuid, const gchar *name, const gchar *key, gboolean value, GSList **message );
static gboolean       write_list( NagpGConfProvider *gconf, const gchar *uuid, const gchar *name, const gchar *key, GSList *value, GSList **message );

static GSList  *na_gconf_utils_get_subdirs( GConfClient *gconf, const gchar *path );
static void     na_gconf_utils_free_subdirs( GSList *subdirs );
/*static gboolean na_gconf_utils_have_subdir( GConfClient *gconf, const gchar *path );*/
static gboolean na_gconf_utils_have_entry( GConfClient *gconf, const gchar *path, const gchar *entry );

GType
nagp_gconf_provider_get_type( void )
{
	return( st_module_type );
}

void
nagp_gconf_provider_register_type( GTypeModule *module )
{
	static const gchar *thisfn = "nagp_gconf_provider_register_type";

	static GTypeInfo info = {
		sizeof( NagpGConfProviderClass ),
		NULL,
		NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NagpGConfProvider ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	static const GInterfaceInfo iio_provider_iface_info = {
		( GInterfaceInitFunc ) iio_provider_iface_init,
		NULL,
		NULL
	};

	g_debug( "%s", thisfn );

	st_module_type = g_type_module_register_type( module, G_TYPE_OBJECT, "NagpGConfProvider", &info, 0 );

	g_type_module_add_interface( module, st_module_type, NA_IIO_PROVIDER_TYPE, &iio_provider_iface_info );
}

static void
class_init( NagpGConfProviderClass *klass )
{
	static const gchar *thisfn = "nagp_gconf_provider_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NagpGConfProviderClassPrivate, 1 );
}

static void
iio_provider_iface_init( NAIIOProviderInterface *iface )
{
	static const gchar *thisfn = "nagp_gconf_provider_iio_provider_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->read_items = iio_provider_read_items;
	iface->is_willing_to_write = iio_provider_is_willing_to_write;
	iface->is_writable = iio_provider_is_writable;
	iface->write_item = iio_provider_write_item;
	iface->delete_item = iio_provider_delete_item;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nagp_gconf_provider_instance_init";
	NagpGConfProvider *self;

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );
	g_return_if_fail( NAGP_IS_GCONF_PROVIDER( instance ));
	self = NAGP_GCONF_PROVIDER( instance );

	self->private = g_new0( NagpGConfProviderPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->gconf = gconf_client_get_default();
	self->private->monitors = install_monitors( self );
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "nagp_gconf_provider_instance_dispose";
	NagpGConfProvider *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_return_if_fail( NAGP_IS_GCONF_PROVIDER( object ));
	self = NAGP_GCONF_PROVIDER( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* release the GConf monitoring */
		na_gconf_monitor_release_monitors( self->private->monitors );

		/* release the GConf connexion */
		g_object_unref( self->private->gconf );

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	NagpGConfProvider *self;

	g_assert( NAGP_IS_GCONF_PROVIDER( object ));
	self = NAGP_GCONF_PROVIDER( object );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

static GList *
install_monitors( NagpGConfProvider *provider )
{
	GList *list = NULL;

	g_return_val_if_fail( NAGP_IS_GCONF_PROVIDER( provider ), NULL );
	g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider ), NULL );
	g_return_val_if_fail( !provider->private->dispose_has_run, NULL );

	/* monitor the configurations/ directory which contains all menus,
	 * actions and profiles definitions
	 */
	list = g_list_prepend( list,
			na_gconf_monitor_new(
					NA_GCONF_CONFIG_PATH,
					( GConfClientNotifyFunc ) config_path_changed_cb,
					provider ));

	return( list );
}

/*
 * this callback is triggered each time a value is changed under our
 * configurations/ directory ; as each object has several entries which
 * describe it, this callback is triggered several times for each object
 * update
 *
 * up to and including 1.10.1, the user interface took care of writing
 * a special key in GConf at the end of each update operations ;
 * as GConf monitored only this special key, it triggered this callback
 * once for each global update operation
 *
 * this special key was of the form xxx:yyyyyyyy-yyyy-yyyy-..., where :
 *    xxx was a sequential number (inside of the ui session)
 *    yyyyyyyy-yyyy-yyyy-... was the uuid of the involved action
 *
 * this was a sort of hack which simplified a lot the notification
 * system, but didn't take into account any modification which might
 * come from outside of the ui
 *
 * if the modification is made elsewhere (an action is imported as a
 * xml file in gconf, or gconf is directly edited), we'd have to rely
 * only on the standard monitor (GConf watch) mechanism
 *
 * this is what we do below, thus triggering the NAIIOProvider interface
 * for each and every modification in the GConf underlying system ; this
 * is the prerogative of NAIIOProvider to decide what to do with them
 */
static void
config_path_changed_cb( GConfClient *client, guint cnxn_id, GConfEntry *entry, NagpGConfProvider *provider )
{
	/*static const gchar *thisfn = "nagp_gconf_provider_config_path_changed_cb";*/
#if 0
	NAPivotNotify *npn;
#endif

	/*g_debug( "%s: client=%p, cnxnid=%u, entry=%p, provider=%p",
			thisfn, ( void * ) client, cnxn_id, ( void * ) entry, ( void * ) provider );*/

	g_return_if_fail( NAGP_IS_GCONF_PROVIDER( provider ));
	g_return_if_fail( NA_IS_IIO_PROVIDER( provider ));

	if( !provider->private->dispose_has_run ){

#if 0
		npn = entry_to_notify( entry );
		g_signal_emit_by_name( provider->private->pivot, NA_IIO_PROVIDER_SIGNAL_ACTION_CHANGED, npn );
#endif
		na_iio_provider_config_changed( NA_IIO_PROVIDER( provider ));
	}
}

/*
 * convert a GConfEntry to a structure suitable to notify NAIIOProvider
 * interface
 *
 * when created or modified, the entry can be of the forms :
 *  key=path/uuid/parm
 *  key=path/uuid/profile/parm with a not null value
 *
 * but when removing an entry, it will be of the form :
 *  key=path/uuid
 *  key=path/uuid/parm
 *  key=path/uuid/profile
 *  key=path/uuid/profile/parm with a null value
 *
 * I don't see any way to choose between key/parm and key/profile (*)
 * as the entry no more exists in GConf and thus cannot be tested
 * -> we will set this as key/parm, letting the interface try to
 *    interpret it
 *
 * (*) other than assuming that a profile name begins with 'profile-'
 * (see action-profile.h)
 */
#if 0
static NAPivotNotify *
entry_to_notify( const GConfEntry *entry )
{
	/*static const gchar *thisfn = "nagp_gconf_entry_to_notify";*/
	GSList *listvalues, *iv, *strings;
	NAPivotNotify *npn;
	gchar **split;
	const gchar *path;
	const gchar *subpath;
	const GConfValue *value;

	g_assert( entry );
	path = gconf_entry_get_key( entry );
	g_assert( path );

	npn = g_new0( NAPivotNotify, 1 );

	subpath = path + strlen( NA_GCONF_CONFIG_PATH ) + 1;
	split = g_strsplit( subpath, "/", -1 );
	/*g_debug( "%s: [0]=%s, [1]=%s", thisfn, split[0], split[1] );*/
	npn->uuid = g_strdup( split[0] );

	if( g_strv_length( split ) == 2 ){
		npn->parm = g_strdup( split[1] );

	} else if( g_strv_length( split ) == 3 ){
		npn->profile = g_strdup( split[1] );
		npn->parm = g_strdup( split[2] );
	}

	g_strfreev( split );

	value = gconf_entry_get_value( entry );

	if( value ){
		switch( value->type ){

			case GCONF_VALUE_STRING:
				npn->type = NA_PIVOT_STR;
				npn->data = ( gpointer ) g_strdup( gconf_value_get_string( value ));
				break;

			case GCONF_VALUE_BOOL:
				npn->type = NA_PIVOT_BOOL;
				npn->data = GINT_TO_POINTER( gconf_value_get_bool( value ));
				break;

			case GCONF_VALUE_LIST:
				listvalues = gconf_value_get_list( value );
				strings = NULL;
				for( iv = listvalues ; iv != NULL ; iv = iv->next ){
					strings = g_slist_prepend( strings,
							( gpointer ) gconf_value_get_string(( GConfValue * ) iv->data ));
				}

				npn->type = NA_PIVOT_STRLIST;
				npn->data = ( gpointer ) na_utils_duplicate_string_list( strings );
				/*na_utils_free_string_list( strings );*/
				break;

			default:
				g_assert_not_reached();
				break;
		}
	}
	return( npn );
}
#endif

/**
 * iio_provider_read_items:
 *
 * Note that whatever be the version of the readen action, it will be
 * stored as a #NAObjectAction and its set of #NAObjectProfile of the same,
 * latest, version of these classes.
 */
static GList *
iio_provider_read_items( const NAIIOProvider *provider, GSList **messages )
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

		listpath = na_gconf_utils_get_subdirs( self->private->gconf, NA_GCONF_CONFIG_PATH );

		for( ip = listpath ; ip ; ip = ip->next ){

			const gchar *path = ( const gchar * ) ip->data;
			g_debug( "path=%s", path );
			item = read_item( self, path );
			if( item ){
				items_list = g_list_prepend( items_list, item );
			}
		}

		na_gconf_utils_free_subdirs( listpath );
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

	have_type = na_gconf_utils_have_entry( provider->private->gconf, path, OBJECT_ITEM_TYPE_ENTRY );
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
	list_profiles = na_gconf_utils_get_subdirs( provider->private->gconf, path );

	if( list_profiles ){

		/* read profiles in the specified order
		 */
		for( ip = order ; ip ; ip = ip->next ){
			profile_path = gconf_concat_dir_and_key( path, ( gchar * ) ip->data );
			read_item_action_profile( provider, action, profile_path );
			list_profiles = na_utils_remove_from_string_list( list_profiles, profile_path );
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

	na_utils_free_string_list( order );
	na_gconf_utils_free_subdirs( list_profiles );
	na_gconf_utils_free_entries( entries );

	na_object_action_set_readonly( action, !key_is_writable( provider, path ));
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
		na_utils_free_string_list( basenames );
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
		na_utils_free_string_list( schemes );
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
		na_utils_free_string_list( mimetypes );
	}

	if( na_gconf_utils_get_string_list_from_entries( entries, ACTION_FOLDERS_ENTRY, &folders )){
		na_object_profile_set_folders( profile, folders );
		na_utils_free_string_list( folders );
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
 */
static void
read_object_item_properties( NagpGConfProvider *provider, GSList *entries, NAObjectItem *item )
{
	static const gchar *thisfn = "nagp_gconf_provider_read_object_item_properties";
	gchar *id, *label, *tooltip, *icon;
	gboolean enabled;
	GSList *subitems;

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
		na_utils_free_string_list( subitems );
	}
}

static gboolean
iio_provider_is_willing_to_write( const NAIIOProvider *provider )
{
	NagpGConfProvider *self;
	gboolean willing_to = FALSE;

	g_return_val_if_fail( NAGP_IS_GCONF_PROVIDER( provider ), FALSE );
	g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider ), FALSE );
	self = NAGP_GCONF_PROVIDER( provider );

	/* TODO: nagp_gconf_provider_iio_provider_is_willing_to_write */
	if( !self->private->dispose_has_run ){
		willing_to = TRUE;
	}

	return( willing_to );
}

static gboolean
iio_provider_is_writable( const NAIIOProvider *provider, const NAObjectItem *item )
{
	NagpGConfProvider *self;
	gboolean willing_to = FALSE;

	g_return_val_if_fail( NAGP_IS_GCONF_PROVIDER( provider ), FALSE );
	g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider ), FALSE );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), FALSE );
	self = NAGP_GCONF_PROVIDER( provider );

	/* TODO: nagp_gconf_provider_iio_provider_is_writable */
	if( !self->private->dispose_has_run ){
		willing_to = TRUE;
	}

	return( willing_to );
}

static guint
iio_provider_write_item( const NAIIOProvider *provider, const NAObjectItem *item, GSList **messages )
{
	static const gchar *thisfn = "nagp_gconf_provider_iio_provider_write_item";
	NagpGConfProvider *self;

	g_debug( "%s: provider=%p, item=%p (%s), messages=%p",
			thisfn, ( void * ) provider,
			( void * ) item, G_OBJECT_TYPE_NAME( item ), ( void * ) messages );
	g_return_val_if_fail( NAGP_IS_GCONF_PROVIDER( provider ), NA_IIO_PROVIDER_PROGRAM_ERROR );
	g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider ), NA_IIO_PROVIDER_PROGRAM_ERROR );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), NA_IIO_PROVIDER_PROGRAM_ERROR );

	self = NAGP_GCONF_PROVIDER( provider );

	if( self->private->dispose_has_run ){
		return( NA_IIO_PROVIDER_NOT_WILLING_TO_WRITE );
	}

	if( NA_IS_OBJECT_ACTION( item )){
		if( !write_item_action( self, NA_OBJECT_ACTION( item ), messages )){
			return( NA_IIO_PROVIDER_WRITE_ERROR );
		}
	}

	if( NA_IS_OBJECT_MENU( item )){
		if( !write_item_menu( self, NA_OBJECT_MENU( item ), messages )){
			return( NA_IIO_PROVIDER_WRITE_ERROR );
		}
	}

	gconf_client_suggest_sync( self->private->gconf, NULL );

	return( NA_IIO_PROVIDER_WRITE_OK );
}

static gboolean
write_item_action( NagpGConfProvider *provider, const NAObjectAction *action, GSList **messages )
{
	gchar *uuid, *name;
	gboolean ret;
	GList *profiles, *ip;
	NAObjectProfile *profile;

	uuid = na_object_get_id( action );

	ret =
		write_object_item( provider, NA_OBJECT_ITEM( action ), messages ) &&
		write_str( provider, uuid, NULL, ACTION_VERSION_ENTRY, na_object_action_get_version( action ), messages ) &&
		write_bool( provider, uuid, NULL, OBJECT_ITEM_TARGET_SELECTION_ENTRY, na_object_action_is_target_selection( action ), messages ) &&
		write_bool( provider, uuid, NULL, OBJECT_ITEM_TARGET_BACKGROUND_ENTRY, na_object_action_is_target_background( action ), messages ) &&
		write_bool( provider, uuid, NULL, OBJECT_ITEM_TARGET_TOOLBAR_ENTRY, na_object_action_is_target_toolbar( action ), messages ) &&
		write_bool( provider, uuid, NULL, OBJECT_ITEM_TOOLBAR_SAME_LABEL_ENTRY, na_object_action_toolbar_use_same_label( action ), messages ) &&
		write_str( provider, uuid, NULL, OBJECT_ITEM_TOOLBAR_LABEL_ENTRY, na_object_action_toolbar_get_label( action ), messages ) &&
		write_str( provider, uuid, NULL, OBJECT_ITEM_TYPE_ENTRY, g_strdup( OBJECT_ITEM_TYPE_ACTION ), messages );

	profiles = na_object_get_items_list( action );

	for( ip = profiles ; ip && ret ; ip = ip->next ){

		profile = NA_OBJECT_PROFILE( ip->data );
		name = na_object_get_id( profile );

		ret =
			write_str( provider, uuid, name, ACTION_PROFILE_LABEL_ENTRY, na_object_get_label( profile ), messages ) &&
			write_str( provider, uuid, name, ACTION_PATH_ENTRY, na_object_profile_get_path( profile ), messages ) &&
			write_str( provider, uuid, name, ACTION_PARAMETERS_ENTRY, na_object_profile_get_parameters( profile ), messages ) &&
			write_list( provider, uuid, name, ACTION_BASENAMES_ENTRY, na_object_profile_get_basenames( profile ), messages ) &&
			write_bool( provider, uuid, name, ACTION_MATCHCASE_ENTRY, na_object_profile_get_matchcase( profile ), messages ) &&
			write_list( provider, uuid, name, ACTION_MIMETYPES_ENTRY, na_object_profile_get_mimetypes( profile ), messages ) &&
			write_bool( provider, uuid, name, ACTION_ISFILE_ENTRY, na_object_profile_get_is_file( profile ), messages ) &&
			write_bool( provider, uuid, name, ACTION_ISDIR_ENTRY, na_object_profile_get_is_dir( profile ), messages ) &&
			write_bool( provider, uuid, name, ACTION_MULTIPLE_ENTRY, na_object_profile_get_multiple( profile ), messages ) &&
			write_list( provider, uuid, name, ACTION_SCHEMES_ENTRY, na_object_profile_get_schemes( profile ), messages ) &&
			write_list( provider, uuid, name, ACTION_FOLDERS_ENTRY, na_object_profile_get_folders( profile ), messages );

		g_free( name );
	}

	g_free( uuid );

	return( ret );
}

static gboolean
write_item_menu( NagpGConfProvider *provider, const NAObjectMenu *menu, GSList **messages )
{
	gboolean ret;
	gchar *uuid;

	uuid = na_object_get_id( menu );

	ret =
		write_object_item( provider, NA_OBJECT_ITEM( menu ), messages ) &&
		write_str( provider, uuid, NULL, OBJECT_ITEM_TYPE_ENTRY, g_strdup( OBJECT_ITEM_TYPE_MENU ), messages );

	g_free( uuid );

	return( ret );
}

static gboolean
write_object_item( NagpGConfProvider *provider, const NAObjectItem *item, GSList **messages )
{
	gchar *uuid;
	gboolean ret;

	uuid = na_object_get_id( NA_OBJECT( item ));

	ret =
		write_str( provider, uuid, NULL, OBJECT_ITEM_LABEL_ENTRY, na_object_get_label( NA_OBJECT( item )), messages ) &&
		write_str( provider, uuid, NULL, OBJECT_ITEM_TOOLTIP_ENTRY, na_object_get_tooltip( item ), messages ) &&
		write_str( provider, uuid, NULL, OBJECT_ITEM_ICON_ENTRY, na_object_get_icon( item ), messages ) &&
		write_bool( provider, uuid, NULL, OBJECT_ITEM_ENABLED_ENTRY, na_object_is_enabled( item ), messages ) &&
		write_list( provider, uuid, NULL, OBJECT_ITEM_LIST_ENTRY, na_object_item_rebuild_items_list( item ), messages );

	g_free( uuid );
	return( ret );
}

/*
 * also delete the schema which may be directly attached to this action
 * cf. http://bugzilla.gnome.org/show_bug.cgi?id=325585
 */
static guint
iio_provider_delete_item( const NAIIOProvider *provider, const NAObjectItem *item, GSList **messages )
{
	static const gchar *thisfn = "nagp_gconf_provider_iio_provider_delete_item";
	NagpGConfProvider *self;
	guint ret;
	gchar *uuid, *path;
	GError *error = NULL;

	g_debug( "%s: provider=%p, item=%p (%s), messages=%p",
			thisfn, ( void * ) provider,
			( void * ) item, G_OBJECT_TYPE_NAME( item ), ( void * ) messages );

	g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider ), NA_IIO_PROVIDER_PROGRAM_ERROR );
	g_return_val_if_fail( NAGP_IS_GCONF_PROVIDER( provider ), NA_IIO_PROVIDER_PROGRAM_ERROR );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), NA_IIO_PROVIDER_PROGRAM_ERROR );

	self = NAGP_GCONF_PROVIDER( provider );

	if( self->private->dispose_has_run ){
		return( NA_IIO_PROVIDER_NOT_WILLING_TO_WRITE );
	}

	ret = NA_IIO_PROVIDER_WRITE_OK;
	uuid = na_object_get_id( NA_OBJECT( item ));

	path = g_strdup_printf( "%s%s/%s", NAUTILUS_ACTIONS_GCONF_SCHEMASDIR, NA_GCONF_CONFIG_PATH, uuid );
	gconf_client_recursive_unset( self->private->gconf, path, 0, &error );
	if( error ){
		g_warning( "%s: path=%s, error=%s", thisfn, path, error->message );
		*messages = g_slist_append( *messages, g_strdup( error->message ));
		g_error_free( error );
		error = NULL;
	}
	g_free( path );


	path = g_strdup_printf( "%s/%s", NA_GCONF_CONFIG_PATH, uuid );
	if( !gconf_client_recursive_unset( self->private->gconf, path, 0, &error )){
		g_warning( "%s: path=%s, error=%s", thisfn, path, error->message );
		*messages = g_slist_append( *messages, g_strdup( error->message ));
		g_error_free( error );
		error = NULL;
		ret = NA_IIO_PROVIDER_WRITE_ERROR;

	} else {
		gconf_client_suggest_sync( self->private->gconf, NULL );
	}
	g_free( path );

	g_free( uuid );

	return( ret );
}

/*
 * gconf_client_key_is_writable doesn't work as I expect: it returns
 * FALSE without error for our keys !
 * So I have to actually try a fake write to the key to get the real
 * writability status...
 *
 * TODO: having a NAPivot as Nautilus extension and another NAPivot in
 * the UI leads to a sort of notification loop: extension's pivot is
 * not aware of UI's one, and so get notifications from GConf, reloads
 * the list of actions, retrying to test the writability.
 * In the meanwhile, UI's pivot has reauthorized notifications, and is
 * notified of extension's pivot tests, and so on......
 *
 * -> Nautilus extension's pivot should not test for writability, as it
 *    uses actions as read-only, reloading the whole list when one
 *    action is modified ; this can break the loop
 *
 * -> the UI may use the pivot inside of Nautilus extension via a sort
 *    of API, falling back to its own pivot, when the extension is not
 *    present.
 */
static gboolean
key_is_writable( NagpGConfProvider *gconf, const gchar *path )
{
	/*static const gchar *thisfn = "nagp_gconf_provider_key_is_writable";
	GError *error = NULL;

	remove_gconf_watched_dir( gconf );

	gboolean ret_gconf = gconf_client_key_is_writable( gconf->private->gconf, path, &error );
	if( error ){
		g_warning( "%s: gconf_client_key_is_writable: %s", thisfn, error->message );
		g_error_free( error );
		error = NULL;
	}
	gboolean ret_try = FALSE;
	gchar *path_try = g_strdup_printf( "%s/%s", path, "fake_key" );
	ret_try = gconf_client_set_string( gconf->private->gconf, path_try, "fake_value", &error );
	if( error ){
		g_warning( "%s: gconf_client_set_string: %s", thisfn, error->message );
		g_error_free( error );
		error = NULL;
	}
	if( ret_try ){
		gconf_client_unset( gconf->private->gconf, path_try, NULL );
	}
	g_free( path_try );
	g_debug( "%s: ret_gconf=%s, ret_try=%s", thisfn, ret_gconf ? "True":"False", ret_try ? "True":"False" );

	install_gconf_watched_dir( gconf );
	return( ret_try );*/

	return( TRUE );
}

static gboolean
write_str( NagpGConfProvider *provider, const gchar *uuid, const gchar *name, const gchar *key, gchar *value, GSList **messages )
{
	gchar *path;
	gboolean ret;
	gchar *msg;

	if( name && strlen( name )){
		path = g_strdup_printf( "%s/%s/%s/%s", NA_GCONF_CONFIG_PATH, uuid, name, key );
	} else {
		path = g_strdup_printf( "%s/%s/%s", NA_GCONF_CONFIG_PATH, uuid, key );
	}

	msg = NULL;
	ret = na_gconf_utils_write_string( provider->private->gconf, path, value, &msg );
	if( msg ){
		*messages = g_slist_append( *messages, g_strdup( msg ));
		g_free( msg );
	}

	g_free( value );
	g_free( path );

	return( ret );
}

static gboolean
write_bool( NagpGConfProvider *provider, const gchar *uuid, const gchar *name, const gchar *key, gboolean value, GSList **messages )
{
	gboolean ret;
	gchar *path;
	gchar *msg;

	if( name && strlen( name )){
		path = g_strdup_printf( "%s/%s/%s/%s", NA_GCONF_CONFIG_PATH, uuid, name, key );
	} else {
		path = g_strdup_printf( "%s/%s/%s", NA_GCONF_CONFIG_PATH, uuid, key );
	}

	msg = NULL;
	ret = na_gconf_utils_write_bool( provider->private->gconf, path, value, &msg );
	if( msg ){
		*messages = g_slist_append( *messages, g_strdup( msg ));
		g_free( msg );
	}

	g_free( path );

	return( ret );
}

static gboolean
write_list( NagpGConfProvider *provider, const gchar *uuid, const gchar *name, const gchar *key, GSList *value, GSList **messages )
{
	gboolean ret;
	gchar *path;
	gchar *msg;

	if( name && strlen( name )){
		path = g_strdup_printf( "%s/%s/%s/%s", NA_GCONF_CONFIG_PATH, uuid, name, key );
	} else {
		path = g_strdup_printf( "%s/%s/%s", NA_GCONF_CONFIG_PATH, uuid, key );
	}

	msg = NULL;
	ret = na_gconf_utils_write_string_list( provider->private->gconf, path, value, &msg );
	if( msg ){
		*messages = g_slist_append( *messages, g_strdup( msg ));
		g_free( msg );
	}

	na_utils_free_string_list( value );
	g_free( path );

	return( ret );
}

/**
 * na_gconf_utils_get_subdirs:
 * @gconf: a  #GConfClient instance.
 * @path: a full path to be readen.
 *
 * Loads the subdirs of the given path.
 *
 * Returns: a GSList of full path subdirectories.
 *
 * The returned list should be na_gconf_utils_free_subdirs() by the
 * caller.
 */
static GSList *
na_gconf_utils_get_subdirs( GConfClient *gconf, const gchar *path )
{
	static const gchar *thisfn = "na_gconf_utils_get_subdirs";
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

/**
 * na_gconf_utils_free_subdirs:
 * @subdirs: a list of subdirs as returned by na_gconf_utils_get_subdirs().
 *
 * Release the list of subdirs.
 */
static void
na_gconf_utils_free_subdirs( GSList *subdirs )
{
	na_utils_free_string_list( subdirs );
}

/**
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

	listpath = na_gconf_utils_get_subdirs( gconf, path );
	have_subdir = ( listpath && g_slist_length( listpath ));
	na_gconf_utils_free_subdirs( listpath );

	return( have_subdir );
}*/

/**
 * na_gconf_utils_have_entry:
 * @gconf: a  #GConfClient instance.
 * @path: the full path of a key.
 * @entry: the entry to be tested.
 *
 * Returns: %TRUE if the given @entry exists for the given @path,
 * %FALSE else.
 */
static gboolean
na_gconf_utils_have_entry( GConfClient *gconf, const gchar *path, const gchar *entry )
{
	static const gchar *thisfn = "na_gconf_utils_have_entry";
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
