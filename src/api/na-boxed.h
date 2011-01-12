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

#ifndef __NAUTILUS_ACTIONS_API_NA_BOXED_H__
#define __NAUTILUS_ACTIONS_API_NA_BOXED_H__

/**
 * SECTION: boxed
 * @title: NABoxed
 * @short_description: The NABoxed Structure
 * @include: nautilus-actions/na-boxed.h
 *
 * The NABoxed structure is a way of handling various types of data in an
 * opaque structure.
 *
 * Since: 3.1.0
 */

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * NABoxedType:
 * @NA_BOXED_TYPE_STRING:        an ASCII string
 * @NA_BOXED_TYPE_LOCALE_STRING: a localized UTF-8 string
 * @NA_BOXED_TYPE_BOOLEAN:       a boolean
 * @NA_BOXED_TYPE_STRING_LIST:   a list of ASCII strings
 * @NA_BOXED_TYPE_POINTER:       a ( void * ) pointer
 * @NA_BOXED_TYPE_UINT:          an unsigned integer
 * @NA_BOXED_TYPE_UINT_LIST:     a list of unsigned integers
 *
 * Each #NABoxed structure is typed at creation time with one of these
 * elementary types. A #NABoxed structure as so at least a defined type,
 * even if it does not yet have a value.
 *
 * <note>
 *   <para>
 * Please note that this enumeration may be compiled in by the extensions.
 * They must so remain fixed, unless you want see strange effects (e.g.
 * an extension has been compiled with %NA_BOXED_TYPE_STRING = 2, while
 * you have inserted another element, making it to 3 !) - or you know what
 * you are doing...
 *   </para>
 *   <para>
 *     So, only add new items at the end of the enum. You have been warned!
 *   </para>
 * </note>
 *
 * Since: 3.1.0
 */
typedef enum {
	NA_BOXED_TYPE_STRING = 1,
	NA_BOXED_TYPE_LOCALE_STRING,
	NA_BOXED_TYPE_BOOLEAN,
	NA_BOXED_TYPE_STRING_LIST,
	NA_BOXED_TYPE_POINTER,
	NA_BOXED_TYPE_UINT,
	NA_BOXED_TYPE_UINT_LIST,
	/*< private >*/
	/* the count of defined NABoxed types */
	NA_BOXED_TYPE_N
}
	NABoxedType;

typedef struct _NABoxed NABoxed;

int           na_boxed_compare                 ( const NABoxed *a, const NABoxed *b );
NABoxed      *na_boxed_copy                    ( const NABoxed *value );
void          na_boxed_free                    ( NABoxed *value );
NABoxed      *na_boxed_new_from_string         ( guint type, const gchar *string );
NABoxed      *na_boxed_new_from_string_with_sep( guint type, const gchar *string, const gchar *sep );

gboolean      na_boxed_get_boolean             ( const NABoxed *boxed );
gconstpointer na_boxed_get_pointer             ( const NABoxed *boxed );
GSList       *na_boxed_get_string_list         ( const NABoxed *boxed );

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_API_NA_BOXED_H__ */
