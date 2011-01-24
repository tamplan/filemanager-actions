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

#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <api/na-boxed.h>
#include <api/na-core-utils.h>
#include <api/na-timeout.h>

#include "na-settings.h"

/* private class data
 */
struct _NASettingsClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* The characteristics of a configuration file.
 * We manage two configuration files:
 * - the global configuration file handles mandatory preferences;
 * - the user configuration file handles.. well, user preferences.
 */
typedef struct {
	gchar        *fname;
	GKeyFile     *key_file;
	GFileMonitor *monitor;
	gulong        handler;
}
	KeyFile;

/* Each consumer may register a callback function which will be triggered
 * when a key is modified.
 *
 * The monitored key usually is the real key read in the file;
 * as a special case, composite keys are defined:
 * - NA_IPREFS_IO_PROVIDERS_READ_STATUS monitors the 'readable' key for all i/o providers
 *
 * Note that we actually monitor the _user_view_ of the configuration:
 * e.g. if a key has a mandatory value in global conf, then the same
 * key in user conf will just be ignored.
 */
typedef struct {
	gchar    *monitored_key;
	GCallback callback;
	gpointer  user_data;
}
	Consumer;

/* private instance data
 */
struct _NASettingsPrivate {
	gboolean  dispose_has_run;
	KeyFile  *mandatory;
	KeyFile  *user;
	GList    *content;
	GList    *consumers;
	NATimeout timeout;
};

#define GROUP_NACT						"nact"
#define GROUP_RUNTIME					"runtime"

typedef struct {
	const gchar *key;
	const gchar *group;
	gboolean     runtime;	/* whether the key participates to the 'runtime-change' signal */
	guint        type;
	const gchar *default_value;
}
	KeyDef;

static const KeyDef st_def_keys[] = {
	{ NA_IPREFS_ADMIN_PREFERENCES_LOCKED,         GROUP_NACT,    FALSE, NA_BOXED_TYPE_BOOLEAN,     "false" },
	{ NA_IPREFS_ADMIN_IO_PROVIDERS_LOCKED,        GROUP_RUNTIME, FALSE, NA_BOXED_TYPE_BOOLEAN,     "false" },
	{ NA_IPREFS_ASSISTANT_ESC_CONFIRM,            GROUP_NACT,    FALSE, NA_BOXED_TYPE_BOOLEAN,     "true" },
	{ NA_IPREFS_ASSISTANT_ESC_QUIT,               GROUP_NACT,    FALSE, NA_BOXED_TYPE_BOOLEAN,     "true" },
	{ NA_IPREFS_CAPABILITY_ADD_CAPABILITY_WSP,    GROUP_NACT,    FALSE, NA_BOXED_TYPE_UINT_LIST,   "" },
	{ NA_IPREFS_COMMAND_CHOOSER_WSP,              GROUP_NACT,    FALSE, NA_BOXED_TYPE_UINT_LIST,   "" },
	{ NA_IPREFS_COMMAND_CHOOSER_URI,              GROUP_NACT,    FALSE, NA_BOXED_TYPE_STRING,      "file:///bin" },
	{ NA_IPREFS_COMMAND_LEGEND_WSP,               GROUP_NACT,    FALSE, NA_BOXED_TYPE_UINT_LIST,   "" },
	{ NA_IPREFS_WORKING_DIR_WSP,                  GROUP_NACT,    FALSE, NA_BOXED_TYPE_UINT_LIST,   "" },
	{ NA_IPREFS_WORKING_DIR_URI,                  GROUP_NACT,    FALSE, NA_BOXED_TYPE_STRING,      "file:///" },
	{ NA_IPREFS_SHOW_IF_RUNNING_WSP,              GROUP_NACT,    FALSE, NA_BOXED_TYPE_UINT_LIST,   "" },
	{ NA_IPREFS_SHOW_IF_RUNNING_URI,              GROUP_NACT,    FALSE, NA_BOXED_TYPE_STRING,      "file:///bin" },
	{ NA_IPREFS_TRY_EXEC_WSP,                     GROUP_NACT,    FALSE, NA_BOXED_TYPE_UINT_LIST,   "" },
	{ NA_IPREFS_TRY_EXEC_URI,                     GROUP_NACT,    FALSE, NA_BOXED_TYPE_STRING,      "file:///bin" },
	{ NA_IPREFS_EXPORT_ASK_USER_WSP,              GROUP_NACT,    FALSE, NA_BOXED_TYPE_UINT_LIST,   "" },
	{ NA_IPREFS_EXPORT_ASK_USER_LAST_FORMAT,      GROUP_NACT,    FALSE, NA_BOXED_TYPE_STRING,      NA_IPREFS_DEFAULT_EXPORT_FORMAT },
	{ NA_IPREFS_EXPORT_ASK_USER_KEEP_LAST_CHOICE, GROUP_NACT,    FALSE, NA_BOXED_TYPE_BOOLEAN,     "false" },
	{ NA_IPREFS_EXPORT_ASSISTANT_WSP,             GROUP_NACT,    FALSE, NA_BOXED_TYPE_UINT_LIST,   "" },
	{ NA_IPREFS_EXPORT_ASSISTANT_URI,             GROUP_NACT,    FALSE, NA_BOXED_TYPE_STRING,      "file:///tmp" },
	{ NA_IPREFS_EXPORT_PREFERRED_FORMAT,          GROUP_NACT,    FALSE, NA_BOXED_TYPE_STRING,      NA_IPREFS_DEFAULT_EXPORT_FORMAT },
	{ NA_IPREFS_FOLDER_CHOOSER_WSP,               GROUP_NACT,    FALSE, NA_BOXED_TYPE_UINT_LIST,   "" },
	{ NA_IPREFS_FOLDER_CHOOSER_URI,               GROUP_NACT,    FALSE, NA_BOXED_TYPE_STRING,      "file:///" },
	{ NA_IPREFS_IMPORT_ASK_USER_WSP,              GROUP_NACT,    FALSE, NA_BOXED_TYPE_UINT_LIST,   "" },
	{ NA_IPREFS_IMPORT_ASK_USER_LAST_MODE,        GROUP_NACT,    FALSE, NA_BOXED_TYPE_STRING,      NA_IPREFS_DEFAULT_IMPORT_MODE },
	{ NA_IPREFS_IMPORT_ASSISTANT_WSP,             GROUP_NACT,    FALSE, NA_BOXED_TYPE_UINT_LIST,   "" },
	{ NA_IPREFS_IMPORT_ASSISTANT_URI,             GROUP_NACT,    FALSE, NA_BOXED_TYPE_STRING,      "file:///tmp" },
	{ NA_IPREFS_IMPORT_ASK_USER_KEEP_LAST_CHOICE, GROUP_NACT,    FALSE, NA_BOXED_TYPE_BOOLEAN,     "false" },
	{ NA_IPREFS_IMPORT_PREFERRED_MODE,            GROUP_NACT,    FALSE, NA_BOXED_TYPE_STRING,      NA_IPREFS_DEFAULT_IMPORT_MODE },
	{ NA_IPREFS_IO_PROVIDERS_WRITE_ORDER,         GROUP_NACT,    FALSE, NA_BOXED_TYPE_STRING_LIST, "" },
	{ NA_IPREFS_ICON_CHOOSER_URI,                 GROUP_NACT,    FALSE, NA_BOXED_TYPE_STRING,      "file:///" },
	{ NA_IPREFS_ICON_CHOOSER_PANED,               GROUP_NACT,    FALSE, NA_BOXED_TYPE_UINT,        "200" },
	{ NA_IPREFS_ICON_CHOOSER_WSP,                 GROUP_NACT,    FALSE, NA_BOXED_TYPE_UINT_LIST,   "" },
	{ NA_IPREFS_ITEMS_ADD_ABOUT_ITEM,             GROUP_RUNTIME,  TRUE, NA_BOXED_TYPE_BOOLEAN,     "true" },
	{ NA_IPREFS_ITEMS_CREATE_ROOT_MENU,           GROUP_RUNTIME,  TRUE, NA_BOXED_TYPE_BOOLEAN,     "true" },
	{ NA_IPREFS_ITEMS_LEVEL_ZERO_ORDER,           GROUP_RUNTIME,  TRUE, NA_BOXED_TYPE_STRING_LIST, "" },
	{ NA_IPREFS_ITEMS_LIST_ORDER_MODE,            GROUP_RUNTIME,  TRUE, NA_BOXED_TYPE_STRING,      NA_IPREFS_DEFAULT_LIST_ORDER_MODE },
	{ NA_IPREFS_MAIN_PANED,                       GROUP_NACT,    FALSE, NA_BOXED_TYPE_UINT,        "200" },
	{ NA_IPREFS_MAIN_SAVE_AUTO,                   GROUP_NACT,    FALSE, NA_BOXED_TYPE_BOOLEAN,     "false" },
	{ NA_IPREFS_MAIN_SAVE_PERIOD,                 GROUP_NACT,    FALSE, NA_BOXED_TYPE_UINT,        "5" },
	{ NA_IPREFS_MAIN_TOOLBAR_EDIT_DISPLAY,        GROUP_NACT,    FALSE, NA_BOXED_TYPE_BOOLEAN,     "true" },
	{ NA_IPREFS_MAIN_TOOLBAR_FILE_DISPLAY,        GROUP_NACT,    FALSE, NA_BOXED_TYPE_BOOLEAN,     "true" },
	{ NA_IPREFS_MAIN_TOOLBAR_HELP_DISPLAY,        GROUP_NACT,    FALSE, NA_BOXED_TYPE_BOOLEAN,     "true" },
	{ NA_IPREFS_MAIN_TOOLBAR_TOOLS_DISPLAY,       GROUP_NACT,    FALSE, NA_BOXED_TYPE_BOOLEAN,     "false" },
	{ NA_IPREFS_MAIN_WINDOW_WSP,                  GROUP_NACT,    FALSE, NA_BOXED_TYPE_UINT_LIST,   "" },
	{ NA_IPREFS_PREFERENCES_WSP,                  GROUP_NACT,    FALSE, NA_BOXED_TYPE_UINT_LIST,   "" },
	{ NA_IPREFS_RELABEL_DUPLICATE_ACTION,         GROUP_NACT,    FALSE, NA_BOXED_TYPE_BOOLEAN,     "false" },
	{ NA_IPREFS_RELABEL_DUPLICATE_MENU,           GROUP_NACT,    FALSE, NA_BOXED_TYPE_BOOLEAN,     "false" },
	{ NA_IPREFS_RELABEL_DUPLICATE_PROFILE,        GROUP_NACT,    FALSE, NA_BOXED_TYPE_BOOLEAN,     "false" },
	{ NA_IPREFS_SCHEME_ADD_SCHEME_WSP,            GROUP_NACT,    FALSE, NA_BOXED_TYPE_UINT_LIST,   "" },
	{ NA_IPREFS_SCHEME_DEFAULT_LIST,              GROUP_NACT,    FALSE, NA_BOXED_TYPE_STRING_LIST, "" },
	{ NA_IPREFS_IO_PROVIDER_READABLE,             NA_IPREFS_IO_PROVIDER_GROUP,  TRUE, NA_BOXED_TYPE_BOOLEAN, "true" },
	{ NA_IPREFS_IO_PROVIDER_WRITABLE,             NA_IPREFS_IO_PROVIDER_GROUP, FALSE, NA_BOXED_TYPE_BOOLEAN, "true" },
	{ 0 }
};

/* The configuration content is handled as a GList of KeyValue structs.
 * This list is loaded at initialization time, and then compared each
 * time our file monitors signal us that a change has occured.
 */
typedef struct {
	const KeyDef *def;
	const gchar  *group;
	gboolean      mandatory;
	NABoxed      *boxed;
}
	KeyValue;

/* signals
 */
enum {
	RUNTIME_CHANGE,
	UI_CHANGE,
	LAST_SIGNAL
};

static GObjectClass *st_parent_class           = NULL;
static gint          st_burst_timeout          = 100;		/* burst timeout in msec */
static gint          st_signals[ LAST_SIGNAL ] = { 0 };

static GType     register_type( void );
static void      class_init( NASettingsClass *klass );
static void      instance_init( GTypeInstance *instance, gpointer klass );
static void      instance_dispose( GObject *object );
static void      instance_finalize( GObject *object );

static GList    *content_diff( GList *old, GList *new );
static GList    *content_load( NASettings *settings );
static GList    *content_load_keys( NASettings *settings, GList *content, KeyFile *key_file, gboolean mandatory );
static KeyDef   *get_key_def( const gchar *key );
static KeyFile  *key_file_new( NASettings *settings, const gchar *dir );
static void      on_keyfile_changed( GFileMonitor *monitor, GFile *file, GFile *other_file, GFileMonitorEvent event_type, NASettings *settings );
static void      on_keyfile_changed_timeout( NASettings *settings );
static KeyValue *peek_key_value_from_content( GList *content, const gchar *group, const gchar *key );
static KeyValue *read_key_value( NASettings *settings, const gchar *group, const gchar *key, gboolean *found, gboolean *mandatory );
static KeyValue *read_key_value_from_key_file( GKeyFile *key_file, const gchar *group, const gchar *key, const KeyDef *key_def );
static void      release_consumer( Consumer *consumer );
static void      release_key_file( KeyFile *key_file );
static void      release_key_value( KeyValue *value );
static gboolean  set_key_value( NASettings *settings, const gchar *group, const gchar *key, const gchar *string );
static gboolean  write_user_key_file( NASettings *settings );

GType
na_settings_get_type( void )
{
	static GType object_type = 0;

	if( !object_type ){
		object_type = register_type();
	}

	return( object_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "na_settings_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NASettingsClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NASettings ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "NASettings", &info, 0 );

	return( type );
}

static void
class_init( NASettingsClass *klass )
{
	static const gchar *thisfn = "na_settings_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NASettingsClassPrivate, 1 );

	/*
	 * NASettings::settings-runtime-change:
	 *
	 * This signal is sent by NASettings at the end of a burst of
	 * modifications which may affect the way the file manager displays
	 * its context menus.
	 *
	 * The signal is registered without any default handler.
	 */
	st_signals[ RUNTIME_CHANGE ] = g_signal_new(
				SETTINGS_SIGNAL_RUNTIME_CHANGE,
				NA_SETTINGS_TYPE,
				G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
				0,									/* class offset */
				NULL,								/* accumulator */
				NULL,								/* accumulator data */
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE,
				0 );

	/*
	 * NASettings::settings-ui-change:
	 *
	 * This signal is sent by NASettings at the end of a burst of
	 * modifications which only affect the behavior of the NACT
	 * configuration tool.
	 *
	 * The signal is registered without any default handler.
	 */
	st_signals[ UI_CHANGE ] = g_signal_new(
				SETTINGS_SIGNAL_UI_CHANGE,
				NA_SETTINGS_TYPE,
				G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
				0,									/* class offset */
				NULL,								/* accumulator */
				NULL,								/* accumulator data */
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE,
				0 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "na_settings_instance_init";
	NASettings *self;

	g_return_if_fail( NA_IS_SETTINGS( instance ));

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self = NA_SETTINGS( instance );

	self->private = g_new0( NASettingsPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->mandatory = NULL;
	self->private->user = NULL;
	self->private->content = NULL;
	self->private->consumers = NULL;

	self->private->timeout.timeout = st_burst_timeout;
	self->private->timeout.handler = ( NATimeoutFunc ) on_keyfile_changed_timeout;
	self->private->timeout.user_data = self;
	self->private->timeout.source_id = 0;
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "na_settings_instance_dispose";
	NASettings *self;

	g_return_if_fail( NA_IS_SETTINGS( object ));

	self = NA_SETTINGS( object );

	if( !self->private->dispose_has_run ){

		g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

		self->private->dispose_has_run = TRUE;

		release_key_file( self->private->mandatory );
		release_key_file( self->private->user );

		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "na_settings_instance_finalize";
	NASettings *self;

	g_return_if_fail( NA_IS_SETTINGS( object ));

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	self = NA_SETTINGS( object );

	g_list_foreach( self->private->content, ( GFunc ) release_key_value, NULL );
	g_list_free( self->private->content );

	g_list_foreach( self->private->consumers, ( GFunc ) release_consumer, NULL );
	g_list_free( self->private->consumers );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * na_settings_new:
 *
 * Returns: a new #NASettings object which should be g_object_unref()
 * by the caller.
 */
NASettings *
na_settings_new( void )
{
	static const gchar *thisfn = "na_settings_new";
	NASettings *settings;
	gchar *dir;

	settings = g_object_new( NA_SETTINGS_TYPE, NULL );

	dir = g_build_filename( SYSCONFDIR, "xdg", NULL );
	g_debug( "%s: reading mandatory configuration", thisfn );
	settings->private->mandatory = key_file_new( settings, dir );
	g_free( dir );

	dir = g_build_filename( g_get_home_dir(), ".config", NULL );
	g_debug( "%s: reading user configuration", thisfn );
	settings->private->user = key_file_new( settings, dir );
	g_free( dir );

	settings->private->content = content_load( settings );

	return( settings );
}

/**
 * na_settings_register_key_callback:
 * @settings: this #NASettings instance.
 * @key: the key to be monitored.
 * @callback: the function to be called when the value of the key changes.
 * @user_data: data to be passed to the @callback function.
 *
 * Registers a new consumer of the monitoring of the @key.
 *
 * Since: 3.1.0
 */
void
na_settings_register_key_callback( NASettings *settings, const gchar *key, GCallback callback, gpointer user_data )
{
	static const gchar *thisfn = "na_settings_register_key_callback";

	g_return_if_fail( NA_IS_SETTINGS( settings ));

	if( !settings->private->dispose_has_run ){
		g_debug( "%s: settings=%p, key=%s, callback=%p, user_data=%p",
				thisfn, ( void * ) settings, key, ( void * ) callback, ( void * ) user_data );

		Consumer *consumer = g_new0( Consumer, 1 );

		consumer->monitored_key = g_strdup( key );
		consumer->callback = callback;
		consumer->user_data = user_data;
		settings->private->consumers = g_list_prepend( settings->private->consumers, consumer );
	}
}

#if 0
/**
 * na_settings_register_global_callback:
 * @settings: this #NASettings instance.
 * @callback: the function to be called when the value of the key changes.
 * @user_data: data to be passed to the @callback function.
 *
 * Registers a new consumer of the monitoring of the configuration files.
 *
 * Since: 3.1.0
 */
void
na_settings_register_global_callback( NASettings *settings, GCallback callback, gpointer user_data )
{
	g_return_if_fail( NA_IS_SETTINGS( settings ));

	if( !settings->private->dispose_has_run ){

		Consumer *consumer = g_new0( Consumer, 1 );

		consumer->key = NULL;
		consumer->callback = callback;
		consumer->user_data = user_data;
		settings->private->consumers = g_list_prepend( settings->private->consumers, consumer );
	}

}
#endif

/**
 * na_settings_get_boolean:
 * @settings: this #NASettings instance.
 * @key: the key whose value is to be returned.
 * @found: if not %NULL, a pointer to a gboolean in which we will store
 *  whether the searched @key has been found (%TRUE), or if the returned
 *  value comes from default (%FALSE).
 * @mandatory: if not %NULL, a pointer to a gboolean in which we will store
 *  whether the returned value has been read from mandatory preferences
 *  (%TRUE), or from the user preferences (%FALSE). When the @key has not
 *  been found, @mandatory is set to %FALSE.
 *
 * This function should only be called for unambiguous keys; the resultat
 * is otherwise undefined (and rather unpredictable).
 *
 * Returns: the value of the key, of its default value if not found.
 *
 * Since: 3.1.0
 */
gboolean
na_settings_get_boolean( NASettings *settings, const gchar *key, gboolean *found, gboolean *mandatory )
{
	return( na_settings_get_boolean_ex( settings, NULL, key, found, mandatory ));
}

/**
 * na_settings_get_boolean_ex:
 * @settings: this #NASettings instance.
 * @group: the group where the @key is to be searched for. May be %NULL.
 * @key: the key whose value is to be returned.
 * @found: if not %NULL, a pointer to a gboolean in which we will store
 *  whether the searched @key has been found (%TRUE), or if the returned
 *  value comes from default (%FALSE).
 * @mandatory: if not %NULL, a pointer to a gboolean in which we will store
 *  whether the returned value has been read from mandatory preferences
 *  (%TRUE), or from the user preferences (%FALSE). When the @key has not
 *  been found, @mandatory is set to %FALSE.
 *
 * Returns: the value of the key, of its default value if not found.
 *
 * Since: 3.1.0
 */
gboolean
na_settings_get_boolean_ex( NASettings *settings, const gchar *group, const gchar *key, gboolean *found, gboolean *mandatory )
{
	gboolean value;
	KeyValue *key_value;
	KeyDef *key_def;

	value = FALSE;
	key_value = read_key_value( settings, group, key, found, mandatory );

	if( key_value ){
		value = na_boxed_get_boolean( key_value->boxed );
		release_key_value( key_value );

	} else {
		key_def = get_key_def( key );
		if( key_def ){
			value = ( key_def->default_value ? ( strcasecmp( key_def->default_value, "true" ) == 0 || atoi( key_def->default_value ) != 0 ) : FALSE );
		}
	}

	return( value );
}

/**
 * na_settings_get_string:
 * @settings: this #NASettings instance.
 * @key: the key whose value is to be returned.
 * @found: if not %NULL, a pointer to a gboolean in which we will store
 *  whether the searched @key has been found (%TRUE), or if the returned
 *  value comes from default (%FALSE).
 * @mandatory: if not %NULL, a pointer to a gboolean in which we will store
 *  whether the returned value has been read from mandatory preferences
 *  (%TRUE), or from the user preferences (%FALSE). When the @key has not
 *  been found, @mandatory is set to %FALSE.
 *
 * This function should only be called for unambiguous keys; the resultat
 * is otherwise undefined (and rather unpredictable).
 *
 * Returns: the value of the key as a newly allocated string, which should
 * be g_free() by the caller.
 *
 * Since: 3.1.0
 */
gchar *
na_settings_get_string( NASettings *settings, const gchar *key, gboolean *found, gboolean *mandatory )
{
	gchar *value;
	KeyValue *key_value;
	KeyDef *key_def;

	value = NULL;
	key_value = read_key_value( settings, NULL, key, found, mandatory );

	if( key_value ){
		value = na_boxed_get_string( key_value->boxed );
		release_key_value( key_value );

	} else {
		key_def = get_key_def( key );
		if( key_def && key_def->default_value ){
			value = g_strdup( key_def->default_value );
		}
	}

	return( value );
}

/**
 * na_settings_get_string_list:
 * @settings: this #NASettings instance.
 * @key: the key whose value is to be returned.
 * @found: if not %NULL, a pointer to a gboolean in which we will store
 *  whether the searched @key has been found (%TRUE), or if the returned
 *  value comes from default (%FALSE).
 * @mandatory: if not %NULL, a pointer to a gboolean in which we will store
 *  whether the returned value has been read from mandatory preferences
 *  (%TRUE), or from the user preferences (%FALSE). When the @key has not
 *  been found, @mandatory is set to %FALSE.
 *
 * This function should only be called for unambiguous keys; the resultat
 * is otherwise undefined (and rather unpredictable).
 *
 * Returns: the value of the key as a newly allocated list of strings.
 * The returned list should be na_core_utils_slist_free() by the caller.
 *
 * Since: 3.1.0
 */
GSList *
na_settings_get_string_list( NASettings *settings, const gchar *key, gboolean *found, gboolean *mandatory )
{
	GSList *value;
	KeyValue *key_value;
	KeyDef *key_def;

	value = NULL;
	key_value = read_key_value( settings, NULL, key, found, mandatory );

	if( key_value ){
		value = na_boxed_get_string_list( key_value->boxed );
		release_key_value( key_value );

	} else {
		key_def = get_key_def( key );
		if( key_def && key_def->default_value && strlen( key_def->default_value )){
			value = g_slist_append( NULL, g_strdup( key_def->default_value ));
		}
	}

	return( value );
}

/**
 * na_settings_get_uint:
 * @settings: this #NASettings instance.
 * @key: the key whose value is to be returned.
 * @found: if not %NULL, a pointer to a gboolean in which we will store
 *  whether the searched @key has been found (%TRUE), or if the returned
 *  value comes from default (%FALSE).
 * @mandatory: if not %NULL, a pointer to a gboolean in which we will store
 *  whether the returned value has been read from mandatory preferences
 *  (%TRUE), or from the user preferences (%FALSE). When the @key has not
 *  been found, @mandatory is set to %FALSE.
 *
 * This function should only be called for unambiguous keys; the resultat
 * is otherwise undefined (and rather unpredictable).
 *
 * Returns: the value of the key.
 *
 * Since: 3.1.0
 */
guint
na_settings_get_uint( NASettings *settings, const gchar *key, gboolean *found, gboolean *mandatory )
{
	guint value;
	KeyDef *key_def;
	KeyValue *key_value;

	value = 0;
	key_value = read_key_value( settings, NULL, key, found, mandatory );

	if( key_value ){
		value = na_boxed_get_uint( key_value->boxed );
		release_key_value( key_value );

	} else {
		key_def = get_key_def( key );
		if( key_def && key_def->default_value ){
			value = atoi( key_def->default_value );
		}
	}

	return( value );
}

/**
 * na_settings_get_uint_list:
 * @settings: this #NASettings instance.
 * @key: the key whose value is to be returned.
 * @found: if not %NULL, a pointer to a gboolean in which we will store
 *  whether the searched @key has been found (%TRUE), or if the returned
 *  value comes from default (%FALSE).
 * @mandatory: if not %NULL, a pointer to a gboolean in which we will store
 *  whether the returned value has been read from mandatory preferences
 *  (%TRUE), or from the user preferences (%FALSE). When the @key has not
 *  been found, @mandatory is set to %FALSE.
 *
 * This function should only be called for unambiguous keys; the resultat
 * is otherwise undefined (and rather unpredictable).
 *
 * Returns: the value of the key as a newly allocated list of uints.
 * The returned list should be g_list_free() by the caller.
 *
 * Since: 3.1.0
 */
GList *
na_settings_get_uint_list( NASettings *settings, const gchar *key, gboolean *found, gboolean *mandatory )
{
	GList *value;
	KeyDef *key_def;
	KeyValue *key_value;

	value = NULL;
	key_value = read_key_value( settings, NULL, key, found, mandatory );

	if( key_value ){
		value = na_boxed_get_uint_list( key_value->boxed );
		release_key_value( key_value );

	} else {
		key_def = get_key_def( key );
		if( key_def && key_def->default_value ){
			value = g_list_append( NULL, GUINT_TO_POINTER( atoi( key_def->default_value )));
		}
	}

	return( value );
}

/**
 * na_settings_set_boolean:
 * @settings: this #NASettings instance.
 * @key: the key whose value is to be returned.
 * @value: the boolean to be written.
 *
 * This function writes @value as a user preference.
 *
 * This function should only be called for unambiguous keys; the resultat
 * is otherwise undefined (and rather unpredictable).
 *
 * Returns: %TRUE is the writing has been successful, %FALSE else.
 *
 * Since: 3.1.0
 */
gboolean
na_settings_set_boolean( NASettings *settings, const gchar *key, gboolean value )
{
	gchar *string;
	gboolean ok;

	string = g_strdup_printf( "%s", value ? "true" : "false" );
	ok = set_key_value( settings, NULL, key, string );
	g_free( string );

	return( ok );
}

/**
 * na_settings_set_boolean_ex:
 * @settings: this #NASettings instance.
 * @group: the group in the keyed file;
 * @key: the key whose value is to be returned.
 * @value: the boolean to be written.
 *
 * This function writes @value as a user preference.
 *
 * Returns: %TRUE is the writing has been successful, %FALSE else.
 *
 * Since: 3.1.0
 */
gboolean
na_settings_set_boolean_ex( NASettings *settings, const gchar *group, const gchar *key, gboolean value )
{
	gchar *string;
	gboolean ok;

	string = g_strdup_printf( "%s", value ? "true" : "false" );
	ok = set_key_value( settings, group, key, string );
	g_free( string );

	return( ok );
}

/**
 * na_settings_set_string:
 * @settings: this #NASettings instance.
 * @key: the key whose value is to be returned.
 * @value: the string to be written.
 *
 * This function writes @value as a user preference.
 *
 * This function should only be called for unambiguous keys; the resultat
 * is otherwise undefined (and rather unpredictable).
 *
 * Returns: %TRUE is the writing has been successful, %FALSE else.
 *
 * Since: 3.1.0
 */
gboolean
na_settings_set_string( NASettings *settings, const gchar *key, const gchar *value )
{
	return( set_key_value( settings, NULL, key, value ));
}

/**
 * na_settings_set_string_list:
 * @settings: this #NASettings instance.
 * @key: the key whose value is to be returned.
 * @value: the list of strings to be written.
 *
 * This function writes @value as a user preference.
 *
 * This function should only be called for unambiguous keys; the resultat
 * is otherwise undefined (and rather unpredictable).
 *
 * Returns: %TRUE is the writing has been successful, %FALSE else.
 *
 * Since: 3.1.0
 */
gboolean
na_settings_set_string_list( NASettings *settings, const gchar *key, const GSList *value )
{
	GString *string;
	const GSList *it;
	gboolean ok;

	string = g_string_new( "" );
	for( it = value ; it ; it = it->next ){
		g_string_append_printf( string, "%s;", ( const gchar * ) it->data );
	}
	ok = set_key_value( settings, NULL, key, string->str );
	g_string_free( string, TRUE );

	return( ok );
}

/**
 * na_settings_set_uint:
 * @settings: this #NASettings instance.
 * @key: the key whose value is to be returned.
 * @value: the unsigned integer to be written.
 *
 * This function writes @value as a user preference.
 *
 * This function should only be called for unambiguous keys; the resultat
 * is otherwise undefined (and rather unpredictable).
 *
 * Returns: %TRUE is the writing has been successful, %FALSE else.
 *
 * Since: 3.1.0
 */
gboolean
na_settings_set_uint( NASettings *settings, const gchar *key, guint value )
{
	gchar *string;
	gboolean ok;

	string = g_strdup_printf( "%u", value );
	ok = set_key_value( settings, NULL, key, string );
	g_free( string );

	return( ok );
}

/**
 * na_settings_set_uint_list:
 * @settings: this #NASettings instance.
 * @key: the key whose value is to be returned.
 * @value: the list of unsigned integers to be written.
 *
 * This function writes @value as a user preference.
 *
 * This function should only be called for unambiguous keys; the resultat
 * is otherwise undefined (and rather unpredictable).
 *
 * Returns: %TRUE is the writing has been successful, %FALSE else.
 *
 * Since: 3.1.0
 */
gboolean
na_settings_set_uint_list( NASettings *settings, const gchar *key, const GList *value )
{
	GString *string;
	const GList *it;
	gboolean ok;

	string = g_string_new( "" );
	for( it = value ; it ; it = it->next ){
		g_string_append_printf( string, "%u;", GPOINTER_TO_UINT( it->data ));
	}
	ok = set_key_value( settings, NULL, key, string->str );
	g_string_free( string, TRUE );

	return( ok );
}

/**
 * na_settings_get_groups:
 * @settings: this #NASettings instance.
 *
 * Returns: the list of groups in the configuration; this list should be
 * na_core_utils_slist_free() by the caller.
 *
 * This function participates to a rather bad hack to obtain the list of
 * known i/o providers from preferences. We do not care of returning unique
 * or sorted group names.
 *
 * Since: 3.1.0
 */
GSList *
na_settings_get_groups( NASettings *settings )
{
	GSList *groups;
	gchar **array;

	groups = NULL;

	g_return_val_if_fail( NA_IS_SETTINGS( settings ), NULL );

	if( !settings->private->dispose_has_run ){

		array = g_key_file_get_groups( settings->private->mandatory->key_file, NULL );
		if( array ){
			groups = na_core_utils_slist_from_array(( const gchar ** ) array );
			g_strfreev( array );
		}

		array = g_key_file_get_groups( settings->private->user->key_file, NULL );
		if( array ){
			groups = g_slist_concat( groups, na_core_utils_slist_from_array(( const gchar ** ) array ));
			g_strfreev( array );
		}
	}

	return( groups );
}

/*
 * returns a list of modified KeyValue
 * - order in the lists is not signifiant
 * - the mandatory flag is not signifiant
 * - a key is modified:
 *   > if it appears in new
 *   > if it disappears: the value is so reset to its default
 *   > if the value has been modified
 *
 * we return here a new list, with newly allocated KeyValue structs
 * which hold the new value of each modified key
 */
static GList *
content_diff( GList *old, GList *new )
{
	GList *diffs, *io, *in;
	KeyValue *kold, *knew, *kdiff;
	gboolean found;

	diffs = NULL;

	for( io = old ; io ; io = io->next ){
		kold = ( KeyValue * ) io->data;
		found = FALSE;
		for( in = new ; in && !found ; in = in->next ){
			knew = ( KeyValue * ) in->data;
			if( !strcmp( kold->group, knew->group ) && ( gpointer ) kold->def == ( gpointer ) knew->def ){
				found = TRUE;
				if( na_boxed_compare( kold->boxed, knew->boxed ) != 0 ){
					/* a key has been modified */
					kdiff = g_new0( KeyValue, 1 );
					kdiff->group = g_strdup( knew->group );
					kdiff->def = knew->def;
					kdiff->mandatory = knew->mandatory;
					kdiff->boxed = na_boxed_copy( knew->boxed );
					diffs = g_list_prepend( diffs, kdiff );
				}
			}
		}
		if( !found ){
			/* a key has disappeared */
			kdiff = g_new0( KeyValue, 1 );
			kdiff->group = g_strdup( kold->group );
			kdiff->def = kold->def;
			kdiff->mandatory = FALSE;
			kdiff->boxed = na_boxed_new_from_string( kold->def->type, kold->def->default_value );
			diffs = g_list_prepend( diffs, kdiff );
		}
	}

	for( in = new ; in ; in = in->next ){
		knew = ( KeyValue * ) in->data;
		found = FALSE;
		for( io = old ; io && !found ; io = io->next ){
			kold = ( KeyValue * ) io->data;
			if( !strcmp( kold->group, knew->group ) && ( gpointer ) kold->def == ( gpointer ) knew->def ){
				found = TRUE;
			}
		}
		if( !found ){
			/* a key is new */
			kdiff = g_new0( KeyValue, 1 );
			kdiff->group = g_strdup( knew->group );
			kdiff->def = knew->def;
			kdiff->mandatory = knew->mandatory;
			kdiff->boxed = na_boxed_copy( knew->boxed );
			diffs = g_list_prepend( diffs, kdiff );
		}
	}

	return( diffs );
}

/* load the content of the two configuration files (actually of _the_ configuration)
 * taking care of not overriding mandatory preferences with user ones
 */
static GList *
content_load( NASettings *settings )
{
	GList *content;

	content = content_load_keys( settings, NULL, settings->private->mandatory, TRUE );
	content = content_load_keys( settings, content, settings->private->user, FALSE );

	return( content );
}

static GList *
content_load_keys( NASettings *settings, GList *content, KeyFile *key_file, gboolean mandatory )
{
	static const gchar *thisfn = "na_settings_content_load_keys";
	GError *error;
	gchar **groups, **ig;
	gchar **keys, **ik;
	KeyValue *key_value;
	KeyDef *key_def;

	error = NULL;
	if( !g_key_file_load_from_file( key_file->key_file, key_file->fname, G_KEY_FILE_KEEP_COMMENTS, &error )){
		if( error->code != G_FILE_ERROR_NOENT ){
			g_warning( "%s: %s (%d) %s", thisfn, key_file->fname, error->code, error->message );
		}
		g_error_free( error );
		error = NULL;

	} else {
		groups = g_key_file_get_groups( key_file->key_file, NULL );
		ig = groups;
		while( *ig ){
			keys = g_key_file_get_keys( key_file->key_file, *ig, NULL, NULL );
			ik = keys;
			while( *ik ){
				key_def = get_key_def( *ik );
				if( key_def ){
					key_value = peek_key_value_from_content( content, *ig, *ik );
					if( !key_value ){
						key_value = read_key_value_from_key_file( key_file->key_file, *ig, *ik, key_def );
						if( key_value ){
							key_value->mandatory = mandatory;
							content = g_list_prepend( content, key_value );
						}
					}
				}
				ik++;
			}
			g_strfreev( keys );
			ig++;
		}
		g_strfreev( groups );
	}

	return( content );
}

static KeyDef *
get_key_def( const gchar *key )
{
	static const gchar *thisfn = "na_settings_get_key_def";
	KeyDef *found = NULL;
	KeyDef *idef;

	idef = ( KeyDef * ) st_def_keys;
	while( idef->key && !found ){
		if( !strcmp( idef->key, key )){
			found = idef;
		}
		idef++;
	}
	if( !found ){
		g_warning( "%s: no KeyDef found for key=%s", thisfn, key );
	}

	return( found );
}

/*
 * called from na_settings_new
 * allocate and load the key files for global and user preferences
 */
static KeyFile *
key_file_new( NASettings *settings, const gchar *dir )
{
	static const gchar *thisfn = "na_settings_key_file_new";
	KeyFile *key_file;
	GError *error;
	GFile *file;

	key_file = g_new0( KeyFile, 1 );

	key_file->key_file = g_key_file_new();
	key_file->fname = g_strdup_printf( "%s/%s.conf", dir, PACKAGE );

	error = NULL;
	file = g_file_new_for_path( key_file->fname );
	key_file->monitor = g_file_monitor_file( file, 0, NULL, &error );
	if( error ){
		g_warning( "%s: %s: %s", thisfn, key_file->fname, error->message );
		g_error_free( error );
		error = NULL;
	} else {
		key_file->handler = g_signal_connect( key_file->monitor, "changed", ( GCallback ) on_keyfile_changed, settings );
	}
	g_object_unref( file );

	return( key_file );
}

/*
 * one of the two monitored configuration files have changed on the disk
 * we do not try to identify which keys have actually change
 * instead we trigger each registered consumer for the 'global' event
 *
 * consumers which register for the 'global_conf' event are recorded
 * with a NULL key
 */
static void
on_keyfile_changed( GFileMonitor *monitor,
		GFile *file, GFile *other_file, GFileMonitorEvent event_type, NASettings *settings )
{
	g_return_if_fail( NA_IS_SETTINGS( settings ));

	if( !settings->private->dispose_has_run ){

		na_timeout_event( &settings->private->timeout );
	}
}

static void
on_keyfile_changed_timeout( NASettings *settings )
{
	static const gchar *thisfn = "na_settings_on_keyfile_changed_timeout";
	GList *new_content;
	GList *modifs;
	GList *ic, *im;
	KeyValue *changed;
	Consumer *consumer;
	gchar *group_prefix, *key;
	gboolean runtime_change, ui_change;
#ifdef NA_MAINTAINER_MODE
	gchar *value;
#endif

	/* last individual notification is older that the st_burst_timeout
	 * we may so suppose that the burst is terminated
	 */
	new_content = content_load( settings );
	modifs = content_diff( settings->private->content, new_content );
#ifdef NA_MAINTAINER_MODE
	g_debug( "%s: %d found update(s)", thisfn, g_list_length( modifs ));
	for( im = modifs ; im ; im = im->next ){
		changed = ( KeyValue * ) im->data;
		value = na_boxed_get_string( changed->boxed );
		g_debug( "%s: key=%s, value=%s", thisfn, changed->def->key, value );
		g_free( value );
	}
#endif

	runtime_change = FALSE;
	ui_change = FALSE;

	for( ic = settings->private->consumers ; ic ; ic = ic->next ){
		consumer = ( Consumer * ) ic->data;

		group_prefix = NULL;
		if( !strcmp( consumer->monitored_key, NA_IPREFS_IO_PROVIDERS_READ_STATUS )){
			group_prefix = g_strdup_printf( "%s ", NA_IPREFS_IO_PROVIDER_GROUP );
			key = NA_IPREFS_IO_PROVIDER_READABLE;
		} else {
			key = consumer->monitored_key;
		}

		for( im = modifs ; im ; im = im->next ){
			changed = ( KeyValue * ) im->data;
			if(( !group_prefix || g_str_has_prefix( changed->group, group_prefix )) && !strcmp( changed->def->key, key )){
				( *( NASettingsKeyCallback ) consumer->callback )( changed->group, changed->def->key, na_boxed_get_pointer( changed->boxed ), changed->mandatory, consumer->user_data );
			}

			if( changed->def->runtime ){
				runtime_change = TRUE;
			} else {
				ui_change = FALSE;
			}
		}

		g_free( group_prefix );
	}

	if( runtime_change ){
		g_signal_emit_by_name(( gpointer ) settings, SETTINGS_SIGNAL_RUNTIME_CHANGE );
	}

	if( ui_change ){
		g_signal_emit_by_name(( gpointer ) settings, SETTINGS_SIGNAL_UI_CHANGE );
	}

	g_list_foreach( settings->private->content, ( GFunc ) release_key_value, NULL );
	g_list_free( settings->private->content );
	settings->private->content = new_content;

	g_list_foreach( modifs, ( GFunc ) release_key_value, NULL );
	g_list_free( modifs );
}

static KeyValue *
peek_key_value_from_content( GList *content, const gchar *group, const gchar *key )
{
	KeyValue *value, *found;
	GList *ic;

	found = NULL;
	for( ic = content ; ic && !found ; ic = ic->next ){
		value = ( KeyValue * ) ic->data;
		if( !strcmp( value->group, group ) && !strcmp( value->def->key, key )){
			found = value;
		}
	}

	return( found );
}

/* group may be NULL
 */
static KeyValue *
read_key_value( NASettings *settings, const gchar *group, const gchar *key, gboolean *found, gboolean *mandatory )
{
	KeyDef *key_def;
	gboolean has_entry;
	KeyValue *key_value;

	g_return_val_if_fail( NA_IS_SETTINGS( settings ), NULL );

	key_value = NULL;
	if( found ){
		*found = FALSE;
	}
	if( mandatory ){
		*mandatory = FALSE;
	}

	if( !settings->private->dispose_has_run ){

		key_def = get_key_def( key );
		if( key_def ){
			has_entry = FALSE;
			key_value = read_key_value_from_key_file( settings->private->mandatory->key_file, group ? group : key_def->group, key, key_def );
			if( key_value ){
				has_entry = TRUE;
				if( found ){
					*found = TRUE;
				}
				if( mandatory ){
					*mandatory = TRUE;
				}
			}
			if( !has_entry ){
				key_value = read_key_value_from_key_file( settings->private->user->key_file, group ? group : key_def->group, key, key_def );
				if( key_value ){
					has_entry = TRUE;
					if( found ){
						*found = TRUE;
					}
				}
			}
		}
	}

	return( key_value );
}

static KeyValue *
read_key_value_from_key_file( GKeyFile *key_file, const gchar *group, const gchar *key, const KeyDef *key_def )
{
	static const gchar *thisfn = "na_settings_read_key_value_from_key_file";
	KeyValue *value;
	gchar *str;
	GError *error;

	value = NULL;
	error = NULL;

	switch( key_def->type ){

		case NA_BOXED_TYPE_STRING:
		case NA_BOXED_TYPE_STRING_LIST:
		case NA_BOXED_TYPE_UINT:
		case NA_BOXED_TYPE_UINT_LIST:
		case NA_BOXED_TYPE_BOOLEAN:
			str = g_key_file_get_string( key_file, group, key, &error );
			if( error ){
				if( error->code != G_KEY_FILE_ERROR_KEY_NOT_FOUND && error->code != G_KEY_FILE_ERROR_GROUP_NOT_FOUND ){
					g_warning( "%s: %s", thisfn, error->message );
				}
				g_error_free( error );

			/* key exists, but may be empty */
			} else {
				value = g_new0( KeyValue, 1 );
				value->group = g_strdup( group );
				value->def = key_def;
				switch( key_def->type ){
					case NA_BOXED_TYPE_STRING:
					case NA_BOXED_TYPE_UINT:
					case NA_BOXED_TYPE_BOOLEAN:
						value->boxed = na_boxed_new_from_string( key_def->type, str );
						break;
					case NA_BOXED_TYPE_STRING_LIST:
					case NA_BOXED_TYPE_UINT_LIST:
						value->boxed = na_boxed_new_from_string_with_sep( key_def->type, str, ";" );
						break;
				}
			}
			g_free( str );
			break;

		default:
			g_warning( "%s: unmanaged boxed type: %d", thisfn, key_def->type );
			break;
	}

	return( value );
}

/*
 * called from instance_finalize
 * release the list of registered consumers
 */
static void
release_consumer( Consumer *consumer )
{
	g_free( consumer->monitored_key );
	g_free( consumer );
}

/*
 * called from instance_dispose
 * release the opened and monitored GKeyFiles
 */
static void
release_key_file( KeyFile *key_file )
{
	g_key_file_free( key_file->key_file );
	if( key_file->monitor ){
		if( key_file->handler ){
			g_signal_handler_disconnect( key_file->monitor, key_file->handler );
		}
		g_file_monitor_cancel( key_file->monitor );
		g_object_unref( key_file->monitor );
	}
	g_free( key_file->fname );
	g_free( key_file );
}

/*
 * called from instance_finalize
 * release a KeyValue struct
 */
static void
release_key_value( KeyValue *value )
{
	g_free(( gpointer ) value->group );
	na_boxed_free( value->boxed );
	g_free( value );
}

static gboolean
set_key_value( NASettings *settings, const gchar *group, const gchar *key, const gchar *string )
{
	KeyDef *key_def;
	const gchar *wgroup;
	gboolean ok;

	g_return_val_if_fail( NA_IS_SETTINGS( settings ), FALSE );

	ok = FALSE;

	if( !settings->private->dispose_has_run ){

		wgroup = group;
		if( !wgroup ){
			key_def = get_key_def( key );
			if( key_def ){
				wgroup = key_def->group;
			}
		}
		if( wgroup ){
			g_key_file_set_string( settings->private->user->key_file, wgroup, key, string );
			ok = write_user_key_file( settings );
		}
	}

	return( ok );
}

static gboolean
write_user_key_file( NASettings *settings )
{
	static const gchar *thisfn = "na_settings_write_user_key_file";
	gchar *data;
	GFile *file;
	GFileOutputStream *stream;
	GError *error;
	gsize length;

	error = NULL;
	data = g_key_file_to_data( settings->private->user->key_file, &length, NULL );
	file = g_file_new_for_path( settings->private->user->fname );

	stream = g_file_replace( file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, &error );
	if( error ){
		g_warning( "%s: g_file_replace: %s", thisfn, error->message );
		g_error_free( error );
		if( stream ){
			g_object_unref( stream );
		}
		g_object_unref( file );
		g_free( data );
		return( FALSE );
	}

	g_output_stream_write( G_OUTPUT_STREAM( stream ), data, length, NULL, &error );
	if( error ){
		g_warning( "%s: g_output_stream_write: %s", thisfn, error->message );
		g_error_free( error );
		g_object_unref( stream );
		g_object_unref( file );
		g_free( data );
		return( FALSE );
	}

	g_output_stream_close( G_OUTPUT_STREAM( stream ), NULL, &error );
	if( error ){
		g_warning( "%s: g_output_stream_close: %s", thisfn, error->message );
		g_error_free( error );
		g_object_unref( stream );
		g_object_unref( file );
		g_free( data );
		return( FALSE );
	}

	g_object_unref( stream );
	g_object_unref( file );
	g_free( data );

	return( TRUE );
}
