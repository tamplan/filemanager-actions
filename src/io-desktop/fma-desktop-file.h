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

#ifndef __IO_DESKTOP_FMA_DESKTOP_FILE_H__
#define __IO_DESKTOP_FMA_DESKTOP_FILE_H__

/**
 * SECTION: fma_desktop_file
 * @short_description: #FMADesktopFile class definition.
 * @include: fma-desktop-file.h
 *
 * This class encapŝulates the EggDesktopFile structure, adding some
 * private properties. An instance of this class is associated with
 * every #FMAObjectItem for this provider.
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define FMA_TYPE_DESKTOP_FILE                ( fma_desktop_file_get_type())
#define FMA_DESKTOP_FILE( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_DESKTOP_FILE, FMADesktopFile ))
#define FMA_DESKTOP_FILE_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_DESKTOP_FILE, FMADesktopFileClass ))
#define FMA_IS_DESKTOP_FILE( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_DESKTOP_FILE ))
#define FMA_IS_DESKTOP_FILE_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_DESKTOP_FILE ))
#define FMA_DESKTOP_FILE_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_DESKTOP_FILE, FMADesktopFileClass ))

typedef struct _FMADesktopFilePrivate        FMADesktopFilePrivate;

typedef struct {
	/*< private >*/
	GObject                parent;
	FMADesktopFilePrivate *private;
}
	FMADesktopFile;

typedef struct _FMADesktopFileClassPrivate   FMADesktopFileClassPrivate;

typedef struct {
	/*< private >*/
	GObjectClass                parent;
	FMADesktopFileClassPrivate *private;
}
	FMADesktopFileClass;

/* standard suffix for desktop files
 */
#define FMA_DESKTOP_FILE_SUFFIX		".desktop"

GType           fma_desktop_file_get_type         ( void );

FMADesktopFile *fma_desktop_file_new              ( void );
FMADesktopFile *fma_desktop_file_new_from_path    ( const gchar *path );
FMADesktopFile *fma_desktop_file_new_from_uri     ( const gchar *uri );
FMADesktopFile *fma_desktop_file_new_for_write    ( const gchar *path );

GKeyFile       *fma_desktop_file_get_key_file     ( const FMADesktopFile *ndf );
gchar          *fma_desktop_file_get_key_file_uri ( const FMADesktopFile *ndf );
gboolean        fma_desktop_file_write            ( FMADesktopFile *ndf );

gchar          *fma_desktop_file_get_file_type    ( const FMADesktopFile *ndf );
gchar          *fma_desktop_file_get_id           ( const FMADesktopFile *ndf );
GSList         *fma_desktop_file_get_profiles     ( const FMADesktopFile *ndf );

gboolean        fma_desktop_file_has_profile      ( const FMADesktopFile *ndf, const gchar *profile_id );

void            fma_desktop_file_remove_key       ( const FMADesktopFile *ndf, const gchar *group, const gchar *key );
void            fma_desktop_file_remove_profile   ( const FMADesktopFile *ndf, const gchar *profile_id );

gboolean        fma_desktop_file_get_boolean      ( const FMADesktopFile *ndf, const gchar *group, const gchar *key, gboolean *key_found, gboolean default_value );
gchar          *fma_desktop_file_get_locale_string( const FMADesktopFile *ndf, const gchar *group, const gchar *key, gboolean *key_found, const gchar *default_value );
gchar          *fma_desktop_file_get_string       ( const FMADesktopFile *ndf, const gchar *group, const gchar *key, gboolean *key_found, const gchar *default_value );
GSList         *fma_desktop_file_get_string_list  ( const FMADesktopFile *ndf, const gchar *group, const gchar *key, gboolean *key_found, const gchar *default_value );
guint           fma_desktop_file_get_uint         ( const FMADesktopFile *ndf, const gchar *group, const gchar *key, gboolean *key_found, guint default_value );

void            fma_desktop_file_set_boolean      ( const FMADesktopFile *ndf, const gchar *group, const gchar *key, gboolean value );
void            fma_desktop_file_set_locale_string( const FMADesktopFile *ndf, const gchar *group, const gchar *key, const gchar *value );
void            fma_desktop_file_set_string       ( const FMADesktopFile *ndf, const gchar *group, const gchar *key, const gchar *value );
void            fma_desktop_file_set_string_list  ( const FMADesktopFile *ndf, const gchar *group, const gchar *key, GSList *value );
void            fma_desktop_file_set_uint         ( const FMADesktopFile *ndf, const gchar *group, const gchar *key, guint value );

G_END_DECLS

#endif /* __IO_DESKTOP_FMA_DESKTOP_FILE_H__ */
