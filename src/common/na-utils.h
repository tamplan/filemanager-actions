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

#ifndef __NA_UTILS_H__
#define __NA_UTILS_H__

#include <glib.h>

G_BEGIN_DECLS

/*
 * Some functions to ease the GSList list manipulations.
 */
gboolean na_utils_find_in_list( GSList *list, const gchar *str );
gboolean na_utils_string_lists_are_equal( GSList *first, GSList *second );
GSList  *na_utils_duplicate_string_list( GSList *list );
GSList  *na_utils_remove_ascii_from_string_list( GSList *list, const gchar *text );
void     na_utils_free_string_list( GSList *list );
gchar   *na_utils_string_list_to_text( GSList *list );
GSList  *na_utils_text_to_string_list( const gchar *text );

/*
 * Some functions for GString manipulations.
 */
gchar *  na_utils_gstring_joinv( const gchar *start, const gchar *separator, gchar **list );

G_END_DECLS

#endif /* __NA_UTILS_H__ */
