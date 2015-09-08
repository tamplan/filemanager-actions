/*
 * FileManager-Actions
 * A file-manager extension which offers configurable context menu selected_infos.
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

#ifndef __CORE_FMA_SELECTED_INFO_H__
#define __CORE_FMA_SELECTED_INFO_H__

/* @title: FMASelectedInfo
 * @short_description: The #FMASelectedInfo Class Definition
 * @include: core/fma-selected-info.h
 *
 * An object is instantiated for each Nautilus selected item, in order
 * to gather some common properties for the selected item, mainly its
 * mime type for example.
 *
 * This class should be replaced by NautilusFileInfo class, as soon as
 * the required Nautilus version will have the
 * nautilus_file_info_create_for_uri() API (after 2.28)
 */

#include <libnautilus-extension/nautilus-file-info.h>

G_BEGIN_DECLS

#define FMA_TYPE_SELECTED_INFO                ( fma_selected_info_get_type())
#define FMA_SELECTED_INFO( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_SELECTED_INFO, FMASelectedInfo ))
#define FMA_SELECTED_INFO_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_SELECTED_INFO, FMASelectedInfoClass ))
#define FMA_IS_SELECTED_INFO( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_SELECTED_INFO ))
#define FMA_IS_SELECTED_INFO_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_SELECTED_INFO ))
#define FMA_SELECTED_INFO_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_SELECTED_INFO, FMASelectedInfoClass ))

typedef struct _FMASelectedInfoPrivate        FMASelectedInfoPrivate;

typedef struct {
	/*< private >*/
	GObject                 parent;
	FMASelectedInfoPrivate *private;
}
	FMASelectedInfo;

typedef struct _FMASelectedInfoClassPrivate   FMASelectedInfoClassPrivate;

typedef struct {
	/*< private >*/
	GObjectClass                 parent;
	FMASelectedInfoClassPrivate *private;
}
	FMASelectedInfoClass;

GType            fma_selected_info_get_type          ( void );

GList           *fma_selected_info_get_list_from_item( NautilusFileInfo *item );
GList           *fma_selected_info_get_list_from_list( GList *nautilus_selection );
GList           *fma_selected_info_copy_list         ( GList *files );
void             fma_selected_info_free_list         ( GList *files );

gchar           *fma_selected_info_get_basename      ( const FMASelectedInfo *nsi );
gchar           *fma_selected_info_get_dirname       ( const FMASelectedInfo *nsi );
gchar           *fma_selected_info_get_mime_type     ( const FMASelectedInfo *nsi );
gchar           *fma_selected_info_get_path          ( const FMASelectedInfo *nsi );
gchar           *fma_selected_info_get_uri           ( const FMASelectedInfo *nsi );
gchar           *fma_selected_info_get_uri_host      ( const FMASelectedInfo *nsi );
gchar           *fma_selected_info_get_uri_user      ( const FMASelectedInfo *nsi );
guint            fma_selected_info_get_uri_port      ( const FMASelectedInfo *nsi );
gchar           *fma_selected_info_get_uri_scheme    ( const FMASelectedInfo *nsi );
gboolean         fma_selected_info_is_directory      ( const FMASelectedInfo *nsi );
gboolean         fma_selected_info_is_regular        ( const FMASelectedInfo *nsi );
gboolean         fma_selected_info_is_executable     ( const FMASelectedInfo *nsi );
gboolean         fma_selected_info_is_local          ( const FMASelectedInfo *nsi );
gboolean         fma_selected_info_is_owner          ( const FMASelectedInfo *nsi, const gchar *user );
gboolean         fma_selected_info_is_readable       ( const FMASelectedInfo *nsi );
gboolean         fma_selected_info_is_writable       ( const FMASelectedInfo *nsi );

FMASelectedInfo *fma_selected_info_create_for_uri    ( const gchar *uri, const gchar *mimetype, gchar **errmsg );

G_END_DECLS

#endif /* __CORE_FMA_SELECTED_INFO_H__ */
