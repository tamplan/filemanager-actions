/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010 Pierre Wieser and others (see AUTHORS)
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

#ifndef __NADP_DESKTOP_FILE_H__
#define __NADP_DESKTOP_FILE_H__

/**
 * SECTION: nadp_desktop_file
 * @short_description: #NadpDesktopFile class definition.
 * @include: nadp-desktop-file.h
 *
 * This class encapŝulates the EggDesktopFile structure, adding some
 * private properties. An instance of this class is associated with
 * every #NAObjectItem for this provider.
 */

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define NADP_DESKTOP_FILE_TYPE					( nadp_desktop_file_get_type())
#define NADP_DESKTOP_FILE( object )				( G_TYPE_CHECK_INSTANCE_CAST( object, NADP_DESKTOP_FILE_TYPE, NadpDesktopFile ))
#define NADP_DESKTOP_FILE_CLASS( klass )		( G_TYPE_CHECK_CLASS_CAST( klass, NADP_DESKTOP_FILE_TYPE, NadpDesktopFileClass ))
#define NADP_IS_DESKTOP_FILE( object )			( G_TYPE_CHECK_INSTANCE_TYPE( object, NADP_DESKTOP_FILE_TYPE ))
#define NADP_IS_DESKTOP_FILE_CLASS( klass )		( G_TYPE_CHECK_CLASS_TYPE(( klass ), NADP_DESKTOP_FILE_TYPE ))
#define NADP_DESKTOP_FILE_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), NADP_DESKTOP_FILE_TYPE, NadpDesktopFileClass ))

typedef struct NadpDesktopFilePrivate NadpDesktopFilePrivate;

typedef struct {
	GObject                 parent;
	NadpDesktopFilePrivate *private;
}
	NadpDesktopFile;

typedef struct NadpDesktopFileClassPrivate NadpDesktopFileClassPrivate;

typedef struct {
	GObjectClass                 parent;
	NadpDesktopFileClassPrivate *private;
}
	NadpDesktopFileClass;

GType            nadp_desktop_file_get_type( void );

NadpDesktopFile *nadp_desktop_file_new_for_write( const gchar *path );
NadpDesktopFile *nadp_desktop_file_new_from_path( const gchar *path );

gchar           *nadp_desktop_file_get_key_file_path     ( const NadpDesktopFile *ndf );

gchar           *nadp_desktop_file_get_file_type         ( const NadpDesktopFile *ndf );
gchar           *nadp_desktop_file_get_id                ( const NadpDesktopFile *ndf );
gchar           *nadp_desktop_file_get_name              ( const NadpDesktopFile *ndf );
gchar           *nadp_desktop_file_get_tooltip           ( const NadpDesktopFile *ndf );
gchar           *nadp_desktop_file_get_icon              ( const NadpDesktopFile *ndf );
gboolean         nadp_desktop_file_get_enabled           ( const NadpDesktopFile *ndf );
GSList          *nadp_desktop_file_get_items_list        ( const NadpDesktopFile *ndf );
gboolean         nadp_desktop_file_get_target_context    ( const NadpDesktopFile *ndf );
gboolean         nadp_desktop_file_get_target_toolbar    ( const NadpDesktopFile *ndf );
gchar           *nadp_desktop_file_get_toolbar_label     ( const NadpDesktopFile *ndf );
GSList          *nadp_desktop_file_get_profiles_list     ( const NadpDesktopFile *ndf );
GSList          *nadp_desktop_file_get_profile_group_list( const NadpDesktopFile *ndf );

gchar           *nadp_desktop_file_get_profile_name      ( const NadpDesktopFile *ndf, const gchar *profile_id );
gchar           *nadp_desktop_file_get_profile_exec      ( const NadpDesktopFile *ndf, const gchar *profile_id );
GSList          *nadp_desktop_file_get_basenames         ( const NadpDesktopFile *ndf, const gchar *profile_id );
gboolean         nadp_desktop_file_get_matchcase         ( const NadpDesktopFile *ndf, const gchar *profile_id );
GSList          *nadp_desktop_file_get_mimetypes         ( const NadpDesktopFile *ndf, const gchar *profile_id );
GSList          *nadp_desktop_file_get_schemes           ( const NadpDesktopFile *ndf, const gchar *profile_id );
GSList          *nadp_desktop_file_get_folders           ( const NadpDesktopFile *ndf, const gchar *profile_id );

void             nadp_desktop_file_set_label( NadpDesktopFile *ndf, const gchar *label );
void             nadp_desktop_file_set_tooltip( NadpDesktopFile *ndf, const gchar *tooltip );
void             nadp_desktop_file_set_icon( NadpDesktopFile *ndf, const gchar *icon );
void             nadp_desktop_file_set_enabled( NadpDesktopFile *ndf, gboolean enabled );

gboolean         nadp_desktop_file_write( NadpDesktopFile *ndf );

G_END_DECLS

#endif /* __NADP_DESKTOP_FILE_H__ */
