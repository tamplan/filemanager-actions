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

#include <api/na-data-types.h>
#include <api/na-core-utils.h>

typedef struct {
	guint  type;
	gchar *label;
	gchar *gconf_dump_key;
}
	FactoryType;

static FactoryType st_factory_type[] = {
		{ NAFD_TYPE_STRING,        "string",        "string" },
		{ NAFD_TYPE_LOCALE_STRING, "locale string", "string" },
		{ NAFD_TYPE_BOOLEAN,       "bool",          "bool" },
		{ NAFD_TYPE_STRING_LIST,   "string list",   "list" },
		{ NAFD_TYPE_POINTER,       "pointer",        NULL },
		{ NAFD_TYPE_UINT,          "uint",          "int" },
		{ NAFD_TYPE_MAP,           "map",           "string" },
		{ 0 }
};

/**
 * na_data_types_copy:
 * @value: the value to be duplicated.
 * @type: the FactoryData type.
 *
 * Returns: a new occurrence of the value which should be na_data_types_free()
 * by the caller.
 *
 * Since: 3.1.0
 */
gpointer
na_data_types_copy( gpointer value, guint type )
{
	static const gchar *thisfn = "na_data_types_copy";
	gpointer new_value;

	switch( type ){

		case NAFD_TYPE_STRING:
		case NAFD_TYPE_LOCALE_STRING:
		case NAFD_TYPE_MAP:
			new_value = g_strdup(( const gchar * ) value );
			break;

		case NAFD_TYPE_BOOLEAN:
		case NAFD_TYPE_UINT:
			new_value = GUINT_TO_POINTER( GPOINTER_TO_UINT( value ));
			break;

		case NAFD_TYPE_POINTER:
			g_warning( "%s: unmanaged data type: %s", thisfn, na_data_types_get_label( type ));
			new_value = NULL;
			break;

		case NAFD_TYPE_STRING_LIST:
			new_value = na_core_utils_slist_duplicate(( GSList * ) value );
			break;

		default:
			g_warning( "%s: unknown data type: %d", thisfn, type );
			new_value = NULL;
	}

	return( new_value );
}

/**
 * na_data_types_free:
 * @value: the value to be released.
 * @type: the FactoryData type.
 *
 * Release the value.
 *
 * Since: 3.1.0
 */
void
na_data_types_free( gpointer value, guint type )
{
	static const gchar *thisfn = "na_data_types_free";

	switch( type ){

		case NAFD_TYPE_STRING:
		case NAFD_TYPE_LOCALE_STRING:
		case NAFD_TYPE_MAP:
			g_free( value );
			break;

		case NAFD_TYPE_BOOLEAN:
		case NAFD_TYPE_UINT:
			break;

		case NAFD_TYPE_POINTER:
			g_warning( "%s: unmanaged data type: %s", thisfn, na_data_types_get_label( type ));
			break;

		case NAFD_TYPE_STRING_LIST:
			na_core_utils_slist_free(( GSList * ) value );
			break;

		default:
			g_warning( "%s: unknown data type: %d", thisfn, type );
	}
}

/**
 * na_data_types_get_gconf_dump_key:
 * @type: the FactoryData type.
 *
 * Returns: the GConf key suitable for this type.
 *
 * The returned key is owned by the factory data management system, and
 * should not be released by the caller.
 *
 * Since: 2.30
 */
const gchar *
na_data_types_get_gconf_dump_key( guint type )
{
	static const gchar *thisfn = "na_data_types_get_gconf_dump_key";
	FactoryType *str;

	str = st_factory_type;
	while( str->type ){
		if( str->type == type ){
			return( str->gconf_dump_key );
		}
		str++;
	}

	g_warning( "%s: unknown data type: %d", thisfn, type );
	return( NULL );
}

/**
 * na_data_types_get_label:
 * @type: the FactoryData type.
 *
 * Returns: the label of this type.
 *
 * The returned label is owned by the factory data management system, and
 * should not be released by the caller.
 *
 * Since: 3.1.0
 */
const gchar *
na_data_types_get_label( guint type )
{
	static const gchar *thisfn = "na_data_types_get_label";
	FactoryType *str;

	str = st_factory_type;
	while( str->type ){
		if( str->type == type ){
			return( str->label );
		}
		str++;
	}

	g_warning( "%s: unknown data type: %d", thisfn, type );
	return( NULL );
}
