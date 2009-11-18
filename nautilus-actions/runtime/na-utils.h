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

#ifndef __NA_RUNTIME_UTILS_H__
#define __NA_RUNTIME_UTILS_H__

#include <glib.h>

G_BEGIN_DECLS

/* some functions to ease the GSList list manipulations
 */
void     na_utils_dump_string_list( GSList *list );
GSList  *na_utils_duplicate_string_list( GSList *list );
gboolean na_utils_find_in_list( GSList *list, const gchar *str );
void     na_utils_free_string_list( GSList *list );
GSList  *na_utils_lines_to_string_list( const gchar *text );
GSList  *na_utils_remove_ascii_from_string_list( GSList *list, const gchar *text );
GSList  *na_utils_remove_from_string_list( GSList *list, const gchar *str );
gboolean na_utils_string_lists_are_equal( GSList *first, GSList *second );
gchar   *na_utils_string_list_to_text( GSList *list );
GSList  *na_utils_text_to_string_list( const gchar *text );

/* a GSList to a string [aaaa,bbbb,cccc]
 */
gchar   *na_utils_gslist_to_schema( GSList *list );
GSList  *na_utils_schema_to_gslist( const gchar *value );

gchar   *na_utils_boolean_to_schema( gboolean b );
gboolean na_utils_schema_to_boolean( const gchar *value, gboolean default_value );

/*
 * Some functions for GString manipulations.
 */
gchar   *na_utils_gstring_joinv( const gchar *start, const gchar *separator, gchar **list );

/* string manipulations
 */
gchar   *na_utils_get_first_word( const gchar *string );
gchar   *na_utils_prefix_strings( const gchar *prefix, const gchar *str );

/* path manipulations
 */
gboolean na_utils_exist_file( const gchar *uri );
gboolean na_utils_is_writable_dir( const gchar *uri );
gchar   *na_utils_path_extract_last_dir( const gchar *path );
gchar   *na_utils_remove_last_level_from_path( const gchar *path );

G_END_DECLS

#endif /* __NA_RUNTIME_UTILS_H__ */
