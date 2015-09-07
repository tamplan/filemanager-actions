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

#ifndef __FILE_MANAGER_ACTIONS_API_CORE_UTILS_H__
#define __FILE_MANAGER_ACTIONS_API_CORE_UTILS_H__

/**
 * SECTION: core-utils
 * @title: Core Misc
 * @short_description: The Core Library Utilities
 * @include: file-manager-actions/fma-core-utils.h
 */

#include <glib.h>

G_BEGIN_DECLS

/* boolean manipulation
 */
gboolean fma_core_utils_boolean_from_string( const gchar *string );

/* string manipulation
 */
#ifdef NA_ENABLE_DEPRECATED
gchar   *fma_core_utils_str_add_prefix( const gchar *prefix, const gchar *str );
#endif
int      fma_core_utils_str_collate( const gchar *str1, const gchar *str2 );
gchar   *fma_core_utils_str_remove_char( const gchar *string, const gchar *to_remove );
gchar   *fma_core_utils_str_remove_suffix( const gchar *string, const gchar *suffix );
void     fma_core_utils_str_split_first_word( const gchar *string, gchar **first, gchar **other );
gchar   *fma_core_utils_str_subst( const gchar *pattern, const gchar *key, const gchar *subst );

/* some functions to get or set GSList list of strings
 */
void     fma_core_utils_slist_add_message( GSList **list, const gchar *format, ... );
GSList  *fma_core_utils_slist_duplicate( GSList *slist );
void     fma_core_utils_slist_dump( const gchar *prefix, GSList *list );
GSList  *fma_core_utils_slist_from_array( const gchar **str_array );
GSList  *fma_core_utils_slist_from_split( const gchar *text, const gchar *separator );
gchar   *fma_core_utils_slist_join_at_end( GSList *slist, const gchar *link );
GSList  *fma_core_utils_slist_remove_ascii( GSList *slist, const gchar *text );
GSList  *fma_core_utils_slist_remove_utf8( GSList *slist, const gchar *text );
gchar  **fma_core_utils_slist_to_array( GSList *slist );
gchar   *fma_core_utils_slist_to_text( GSList *slist );
GSList  *fma_core_utils_slist_setup_element( GSList *list, const gchar *element, gboolean set );
guint    fma_core_utils_slist_count( GSList *list, const gchar *str );
gboolean fma_core_utils_slist_find_negated( GSList *list, const gchar *str );
gboolean fma_core_utils_slist_are_equal( GSList *a, GSList *b );
void     fma_core_utils_slist_free( GSList *slist );

/* some functions for GString manipulations.
 */
gchar   *fma_core_utils_gstring_joinv( const gchar *start, const gchar *separator, gchar **list );

/* selection count
 */
void     fma_core_utils_selcount_get_ope_int( const gchar *selection_count, gchar **ope, gchar **uint );

/* directory management
 */
gboolean fma_core_utils_dir_is_writable_path( const gchar *path );
gboolean fma_core_utils_dir_is_writable_uri ( const gchar *uri );
void     fma_core_utils_dir_list_perms      ( const gchar *path, const gchar *message );
void     fma_core_utils_dir_split_ext       ( const gchar *string, gchar **first, gchar **ext );

/* file management
 */
gboolean fma_core_utils_file_delete       ( const gchar *path );
gboolean fma_core_utils_file_exists       ( const gchar *uri );
gboolean fma_core_utils_file_is_loadable  ( const gchar *uri );
void     fma_core_utils_file_list_perms   ( const gchar *path, const gchar *message );
gchar   *fma_core_utils_file_load_from_uri( const gchar *uri, gsize *length );

/* miscellaneous
 */
void     fma_core_utils_print_version( void );

G_END_DECLS

#endif /* __FILE_MANAGER_ACTIONS_API_CORE_UTILS_H__ */
