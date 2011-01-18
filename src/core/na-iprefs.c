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

#include <string.h>

#include <api/na-core-utils.h>
#include <api/na-iimporter.h>

#include "na-iprefs.h"
#include "na-settings.h"

typedef struct {
	guint        id;
	const gchar *str;
}
	EnumMap;

/* import mode: what to do when the imported id already exists ?
 * enum is defined in api/na-iimporter.h
 */
#define IMPORT_MODE_NOIMPORT_STR			"NoImport"
#define IMPORT_MODE_RENUMBER_STR			"Renumber"
#define IMPORT_MODE_OVERRIDE_STR			"Override"
#define IMPORT_MODE_ASK_STR					"Ask"

static EnumMap st_import_mode[] = {
	{ IMPORTER_MODE_NO_IMPORT, IMPORT_MODE_NOIMPORT_STR },
	{ IMPORTER_MODE_RENUMBER,  IMPORT_MODE_RENUMBER_STR },
	{ IMPORTER_MODE_OVERRIDE,  IMPORT_MODE_OVERRIDE_STR },
	{ IMPORTER_MODE_ASK,       IMPORT_MODE_ASK_STR },
	{ 0 }
};

/* sort mode of the items in the file manager context menu
 * enum is defined in core/na-iprefs.h
 */
#define ORDER_ALPHA_ASC_STR					"AscendingOrder"
#define ORDER_ALPHA_DESC_STR				"DescendingOrder"
#define ORDER_MANUAL_STR					"ManualOrder"

static EnumMap st_order_mode[] = {
	{ IPREFS_ORDER_ALPHA_ASCENDING,  ORDER_ALPHA_ASC_STR },
	{ IPREFS_ORDER_ALPHA_DESCENDING, ORDER_ALPHA_DESC_STR },
	{ IPREFS_ORDER_MANUAL,           ORDER_MANUAL_STR },
	{ 0 }
};

static const gchar *enum_map_string_from_id( const EnumMap *map, guint id );
static guint        enum_map_id_from_string( const EnumMap *map, const gchar *str );

/*
 * na_iprefs_get_import_mode:
 * @pivot: the #NAPivot application object.
 * @pref: name of the import key to be readen.
 * @mandatory: if not %NULL, a pointer to a boolean which will receive the
 *  mandatory property.
 *
 * This preference defines what to do when an imported item has the same
 * identifier that an already existing one. Default value is defined in
 * core/na-settings.h.
 *
 * Returns: the import mode currently set.
 */
guint
na_iprefs_get_import_mode( const NAPivot *pivot, const gchar *pref, gboolean *mandatory )
{
	gchar *import_mode_str;
	guint import_mode;
	NASettings *settings;

	settings = na_pivot_get_settings( pivot );
	import_mode_str = na_settings_get_string( settings, pref, NULL, mandatory );
	import_mode = enum_map_id_from_string( st_import_mode, import_mode_str );
	g_free( import_mode_str );

	return( import_mode );
}

/*
 * na_iprefs_set_import_mode:
 * @pivot: the #NAPivot application object.
 * @pref: name of the import key to be written.
 * @mode: the new value to be written.
 *
 * Writes the current status of 'import mode' to the preferences system.
 */
void
na_iprefs_set_import_mode( const NAPivot *pivot, const gchar *pref, guint mode )
{
	const gchar *import_str;
	NASettings *settings;

	settings = na_pivot_get_settings( pivot );
	import_str = enum_map_string_from_id( st_import_mode, mode );
	na_settings_set_string( settings, pref, import_str );
}

/*
 * na_iprefs_get_order_mode:
 * @pivot: the #NAPivot application object.
 * @mandatory: if not %NULL, a pointer to a boolean which will receive the
 *  mandatory property.
 *
 * Returns: the order mode currently set.
 */
guint
na_iprefs_get_order_mode( const NAPivot *pivot, gboolean *mandatory )
{
	gchar *order_mode_str;
	guint order_mode;
	NASettings *settings;

	settings = na_pivot_get_settings( pivot );
	order_mode_str = na_settings_get_string( settings, NA_IPREFS_ITEMS_LIST_ORDER_MODE, NULL, mandatory );
	order_mode = enum_map_id_from_string( st_order_mode, order_mode_str );
	g_free( order_mode_str );

	return( order_mode );
}

/*
 * na_iprefs_set_order_mode:
 * @pivot: the #NAPivot application object.
 * @mode: the new value to be written.
 *
 * Writes the current status of 'alphabetical order' to the GConf
 * preference system.
 */
void
na_iprefs_set_order_mode( const NAPivot *pivot, guint mode )
{
	const gchar *order_str;
	NASettings *settings;

	settings = na_pivot_get_settings( pivot );
	order_str = enum_map_string_from_id( st_order_mode, mode );
	na_settings_set_string( settings, NA_IPREFS_ITEMS_LIST_ORDER_MODE, order_str );
}

/**
 * na_iprefs_get_export_format:
 * @pivot: the #NAPivot application object.
 * @name: name of the export format key to be readen
 * @mandatory: if not %NULL, a pointer to a boolean which will receive the
 *  mandatory property.
 *
 * Used to default to export as a GConfEntry.
 * Starting with 3.1.0, defaults to Desktop1 (see. core/na-settings.h)
 *
 * Returns: the export format currently set as a #GQuark.
 */
GQuark
na_iprefs_get_export_format( const NAPivot *pivot, const gchar *name, gboolean *mandatory )
{
	GQuark export_format;
	NASettings *settings;
	gchar *format_str;

	export_format = g_quark_from_static_string( NA_IPREFS_DEFAULT_EXPORT_FORMAT );

	settings = na_pivot_get_settings( pivot );
	format_str = na_settings_get_string( settings, name, NULL, mandatory );

	if( format_str ){
		export_format = g_quark_from_string( format_str );
		g_free( format_str );
	}

	return( export_format );
}

/**
 * na_iprefs_set_export_format:
 * @pivot: the #NAPivot application object.
 * @format: the new value to be written.
 *
 * Writes the preferred export format' to the preference system.
 */
void
na_iprefs_set_export_format( const NAPivot *pivot, const gchar *name, GQuark format )
{
	NASettings *settings;

	settings = na_pivot_get_settings( pivot );
	na_settings_set_string( settings, name, g_quark_to_string( format ));
}

/*
 * na_iprefs_get_io_providers:
 * @pivot: the #NAPivot application object.
 *
 * Searches in preferences system for all mentions of an i/o provider.
 * This does not mean in any way that the i/o provider is active,
 * available or so, but just that is mentioned here.
 *
 * I/o provider identifers returned in the list are not supposed
 * to be unique, nor sorted.
 *
 * Returns: a list of i/o provider identifiers found in preferences
 * system; this list should be na_core_utils_slist_free() by the caller.
 *
 * since: 3.1.0
 */
GSList *
na_iprefs_get_io_providers( const NAPivot *pivot )
{
	GSList *providers;
	NASettings *settings;
	GSList *write_order, *groups;
	GSList *it;
	const gchar *name;
	gchar *group_prefix;
	guint prefix_len;

	providers = NULL;
	settings = na_pivot_get_settings( pivot );

	write_order = na_settings_get_string_list( settings, NA_IPREFS_IO_PROVIDERS_WRITE_ORDER, NULL, NULL );
	for( it = write_order ; it ; it = it->next ){
		name = ( const gchar * ) it->data;
		providers = g_slist_prepend( providers, g_strdup( name ));
	}
	na_core_utils_slist_free( write_order );

	groups = na_settings_get_groups( settings );

	group_prefix = g_strdup_printf( "%s ", NA_IPREFS_IO_PROVIDER_GROUP );
	prefix_len = strlen( group_prefix );
	for( it = groups ; it ; it = it->next ){
		name = ( const gchar * ) it->data;
		if( g_str_has_prefix( name, group_prefix )){
			providers = g_slist_prepend( providers, g_strdup( name+prefix_len ));
		}
	}
	g_free( group_prefix );
	na_core_utils_slist_free( groups );

	return( providers );
}

/*
 * na_iprefs_is_level_zero_writable:
 * @pivot: the #NAPivot application object.
 *
 * The level-zero order may not be writable if:
 * - all the configuration has been locked down by an admin
 * - the preferences has been locked down by an admin
 * - the level-zero order is a mandatory preference
 * - the user configuration file is not writable.
 *
 * All these conditions are subject to runtime modifications. The caller
 * should not keep the result, but rather re-call this function each time
 * it needs this status.
 *
 * Each condition is also subject to race conditions. So the returned
 * status may not be more valid when the caller tries to actually write
 * the level-zero preference.
 *
 * Returns: %TRUE if we are able to update the level-zero list of items,
 * %FALSE else.
 *
 * As of 3.1.0, level-zero order is written as a user preference.
 */
gboolean
na_iprefs_is_level_zero_writable( const NAPivot *pivot )
{
	g_return_val_if_fail( NA_IS_PIVOT( pivot ), FALSE );

	if( na_pivot_is_configuration_locked_by_admin( pivot )){
		return( FALSE );
	}

	return( TRUE );
}

/*
 * na_iprefs_write_level_zero:
 * @pivot: the #NAPivot application object.
 * @items: the #GList of items whose first level is to be written.
 * @messages: a pointer to a #GSList in which we will add happening
 *  error messages;
 *  the pointer may be %NULL;
 *  if not %NULL, the #GSList must have been initialized by the
 *  caller.
 *
 * Rewrite the level-zero items in GConf preferences.
 *
 * Returns: %TRUE if successfully written (i.e. writable, not locked,
 * and so on), %FALSE else.
 *
 * @messages #GSList is only filled up in case of an error has occured.
 * If there is no error (na_iprefs_write_level_zero() returns %TRUE), then
 * the caller may safely assume that @messages is returned in the same
 * state that it has been provided.
 */
gboolean
na_iprefs_write_level_zero( const NAPivot *pivot, const GList *items, GSList **messages )
{
	static const gchar *thisfn = "na_iprefs_write_level_zero";
	gboolean written;
	const GList *it;
	gchar *id;
	GSList *content;

	g_return_val_if_fail( NA_IS_PIVOT( pivot ), FALSE );

	written = FALSE;

	g_debug( "%s: pivot=%p", thisfn, ( void * ) pivot);

	if( na_iprefs_is_level_zero_writable( pivot )){

		content = NULL;
		for( it = items ; it ; it = it->next ){
			id = na_object_get_id( it->data );
			content = g_slist_prepend( content, id );
		}
		content = g_slist_reverse( content );

		na_settings_set_string_list( na_pivot_get_settings( pivot ), NA_IPREFS_ITEMS_LEVEL_ZERO_ORDER, content );
		written = TRUE;

		na_core_utils_slist_free( content );
	}

	return( written );
}

static const gchar *
enum_map_string_from_id( const EnumMap *map, guint id )
{
	const EnumMap *i = map;

	while( i->id ){
		if( i->id == id ){
			return( i->str );
		}
		i++;
	}
	return( map->str );
}

static guint
enum_map_id_from_string( const EnumMap *map, const gchar *str )
{
	const EnumMap *i = map;

	while( i->id ){
		if( !strcmp( i->str, str )){
			return( i->id );
		}
		i++;
	}
	return( map->id );
}
