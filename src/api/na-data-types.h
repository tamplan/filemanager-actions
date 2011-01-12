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

#ifndef __NAUTILUS_ACTIONS_API_NA_FACTORY_DATA_TYPES_H__
#define __NAUTILUS_ACTIONS_API_NA_FACTORY_DATA_TYPES_H__

/**
 * SECTION: data-type
 * @title: NADataType
 * @short_description: The Data Factory Type Definitions
 * @include: nautilus-actions/na-data-types.h
 */

#include <glib.h>

G_BEGIN_DECLS

/**
 * NAFactoryDataType:
 * @NAFD_TYPE_STRING:        an ASCII string
 * @NAFD_TYPE_LOCALE_STRING: a localized UTF-8 string
 * @NAFD_TYPE_BOOLEAN:       a boolean
 *                           can be initialized with "true" or "false" (case insensitive)
 * @NAFD_TYPE_STRING_LIST:   a list of ASCII strings
 * @NAFD_TYPE_POINTER:       a ( void * ) pointer
 *                           should be initialized to NULL
 * @NAFD_TYPE_UINT:          an unsigned integer
 *
 * Each elementary factory data must be typed as one of these
 * IFactoryProvider implementations should provide a primitive for reading
 * (resp. writing) a value for each of these elementary data types.
 *
 * <note>
 *   <para>
 * Please note that this enumeration may be compiled in by the extensions.
 * They must so remain fixed, unless you want see strange effects (e.g.
 * an extension has been compiled with %NAFD_TYPE_STRING = 2, while you
 * have inserted another element, making it to 3 !) - or you know what
 * you are doing...
 *   </para>
 *   <para>
 *     So, only add new items at the end of the enum. You have been warned!
 *   </para>
 * </note>
 *
 * Starting with version 3.1.0, #NAFactoryDataType is deprecated in favour
 * of #NABoxed structure. New code should only use #NABoxed structure and
 * accessors.
 *
 * Deprecated: 3.1.0
 */
typedef enum {
	NAFD_TYPE_STRING = 1,
	NAFD_TYPE_LOCALE_STRING,
	NAFD_TYPE_BOOLEAN,
	NAFD_TYPE_STRING_LIST,
	NAFD_TYPE_POINTER,
	NAFD_TYPE_UINT
}
	NAFactoryDataType;

const gchar *na_data_types_get_gconf_dump_key( guint type );

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_API_NA_FACTORY_DATA_TYPES_H__ */
