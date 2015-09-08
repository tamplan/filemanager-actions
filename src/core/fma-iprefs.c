/*
 * FileManager-Actions
 * A file-manager extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
 *
 * FileManager-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * FileManager-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FileManager-Actions; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
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

#include <api/fma-core-utils.h>

#include "fma-importer.h"
#include "fma-iprefs.h"
#include "fma-settings.h"

typedef struct {
	guint        id;
	const gchar *str;
}
	EnumMap;

/* sort mode of the items in the file manager context menu
 * enum is defined in core/fma-iprefs.h
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

static EnumMap st_tabs_pos[] = {
	{ 1+GTK_POS_LEFT,   "Left" },
	{ 1+GTK_POS_RIGHT,  "Right" },
	{ 1+GTK_POS_TOP,    "Top" },
	{ 1+GTK_POS_BOTTOM, "Bottom" },
	{ 0 }
};

static const gchar *enum_map_string_from_id( const EnumMap *map, guint id );
static guint        enum_map_id_from_string( const EnumMap *map, const gchar *str );

/*
 * fma_iprefs_get_order_mode:
 * @mandatory: if not %NULL, a pointer to a boolean which will receive the
 *  mandatory property.
 *
 * Returns: the order mode currently set.
 */
guint
fma_iprefs_get_order_mode( gboolean *mandatory )
{
	gchar *order_mode_str;
	guint order_mode;

	order_mode_str = fma_settings_get_string( IPREFS_ITEMS_LIST_ORDER_MODE, NULL, mandatory );
	order_mode = enum_map_id_from_string( st_order_mode, order_mode_str );
	g_free( order_mode_str );

	return( order_mode );
}

/*
 * fma_iprefs_get_order_mode_by_label:
 * @label: the label.
 *
 * This function converts a label (e.g. 'ManualOrder') stored in user preferences
 * into the corresponding integer internally used. This is needed e.g. when
 * monitoring the preferences changes.
 *
 * Returns: the order mode currently set.
 */
guint
fma_iprefs_get_order_mode_by_label( const gchar *label )
{
	guint order_mode;

	order_mode = enum_map_id_from_string( st_order_mode, label );

	return( order_mode );
}

/*
 * fma_iprefs_set_order_mode:
 * @mode: the new value to be written.
 *
 * Writes the current status of 'alphabetical order' to the GConf
 * preference system.
 */
void
fma_iprefs_set_order_mode( guint mode )
{
	const gchar *order_str;

	order_str = enum_map_string_from_id( st_order_mode, mode );
	fma_settings_set_string( IPREFS_ITEMS_LIST_ORDER_MODE, order_str );
}

/*
 * fma_iprefs_get_tabs_pos:
 * @mandatory: if not %NULL, a pointer to a boolean which will receive the
 *  mandatory property.
 *
 * Returns: the tabs position of the main window.
 */
guint
fma_iprefs_get_tabs_pos( gboolean *mandatory )
{
	gchar *tabs_pos_str;
	guint tabs_pos;

	tabs_pos_str = fma_settings_get_string( IPREFS_MAIN_TABS_POS, NULL, mandatory );
	tabs_pos = enum_map_id_from_string( st_tabs_pos, tabs_pos_str );
	g_free( tabs_pos_str );

	return( tabs_pos-1 );
}

/*
 * fma_iprefs_set_tabs_pos:
 * @position: the new value to be written.
 *
 * Writes the current status of 'tabs position' to the preference system.
 */
void
fma_iprefs_set_tabs_pos( guint position )
{
	const gchar *tabs_pos_str;

	tabs_pos_str = enum_map_string_from_id( st_tabs_pos, 1+position );
	fma_settings_set_string( IPREFS_MAIN_TABS_POS, tabs_pos_str );
}

/*
 * fma_iprefs_write_level_zero:
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
 * If there is no error (fma_iprefs_write_level_zero() returns %TRUE), then
 * the caller may safely assume that @messages is returned in the same
 * state that it has been provided.
 */
gboolean
fma_iprefs_write_level_zero( const GList *items, GSList **messages )
{
	gboolean written;
	const GList *it;
	gchar *id;
	GSList *content;

	written = FALSE;
	content = NULL;

	for( it = items ; it ; it = it->next ){
		id = fma_object_get_id( it->data );
		content = g_slist_prepend( content, id );
	}
	content = g_slist_reverse( content );

	written = fma_settings_set_string_list( IPREFS_ITEMS_LEVEL_ZERO_ORDER, content );

	fma_core_utils_slist_free( content );

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
