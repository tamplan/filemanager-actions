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
 *
 * This preference defines what to do when an imported item has the same
 * identifier that an already existing one. Default value is defined in
 * core/na-settings.h.
 *
 * Returns: the import mode currently set.
 */
guint
na_iprefs_get_import_mode( const NAPivot *pivot, const gchar *pref )
{
	gchar *import_mode_str;
	guint import_mode;
	NASettings *settings;

	settings = na_pivot_get_settings( pivot );
	import_mode_str = na_settings_get_string( settings, pref, NULL, NULL );
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
 *
 * Returns: the order mode currently set.
 */
gint
na_iprefs_get_order_mode( const NAPivot *pivot )
{
	gchar *order_mode_str;
	guint order_mode;
	NASettings *settings;

	settings = na_pivot_get_settings( pivot );
	order_mode_str = na_settings_get_string( settings, NA_IPREFS_ITEMS_LIST_ORDER_MODE, NULL, NULL );
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
na_iprefs_set_order_mode( const NAPivot *pivot, gint mode )
{
	const gchar *order_str;
	NASettings *settings;

	settings = na_pivot_get_settings( pivot );
	order_str = enum_map_string_from_id( st_order_mode, mode );
	na_settings_set_string( settings, NA_IPREFS_ITEMS_LIST_ORDER_MODE, order_str );
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
		if( !strcmp( i->str == str )){
			return( i->id );
		}
		i++;
	}
	return( map->id );
}
