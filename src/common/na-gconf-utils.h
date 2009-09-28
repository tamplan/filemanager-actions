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

#ifndef __NA_GCONF_UTILS_H__
#define __NA_GCONF_UTILS_H__

/**
 * SECTION: na_gconf_utils
 * @short_description: GConf utilities.
 * @include: common/na-gconf-utils.h
 */

#include <gconf/gconf.h>
#include <gconf/gconf-client.h>

GSList  *na_gconf_utils_get_subdirs( GConfClient *gconf, const gchar *path );
void     na_gconf_utils_free_subdirs( GSList *subdirs );
gboolean na_gconf_utils_have_subdir( GConfClient *gconf, const gchar *path );

GSList  *na_gconf_utils_get_entries( GConfClient *gconf, const gchar *path );
void     na_gconf_utils_free_entries( GSList *entries );
gboolean na_gconf_utils_have_entry( GConfClient *gconf, const gchar *path, const gchar *entry );
gboolean na_gconf_utils_get_bool_from_entries( GSList *entries, const gchar *entry, gboolean *value );
gboolean na_gconf_utils_get_string_from_entries( GSList *entries, const gchar *entry, gchar **value );
gboolean na_gconf_utils_get_string_list_from_entries( GSList *entries, const gchar *entry, GSList **value );

gchar   *na_gconf_utils_path_to_key( const gchar *path );

gboolean na_gconf_utils_read_bool( GConfClient *gconf, const gchar *path, gboolean use_schema, gboolean default_value );
gint     na_gconf_utils_read_int( GConfClient *gconf, const gchar *path, gboolean use_schema, gint default_value );
GSList  *na_gconf_utils_read_string_list( GConfClient *gconf, const gchar *path );

gboolean na_gconf_utils_write_bool( GConfClient *gconf, const gchar *path, gboolean value, gchar **message );
gboolean na_gconf_utils_write_int( GConfClient *gconf, const gchar *path, gint value, gchar **message );
gboolean na_gconf_utils_write_string( GConfClient *gconf, const gchar *path, const gchar *value, gchar **message );
gboolean na_gconf_utils_write_string_list( GConfClient *gconf, const gchar *path, GSList *value, gchar **message );

G_END_DECLS

#endif /* __NA_GCONF_UTILS_H__ */
