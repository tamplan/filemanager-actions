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

#ifndef __NA_GNOME_VFS_URI_H__
#define __NA_GNOME_VFS_URI_H__

/*
 * pwi 2009-07-29
 * shamelessly pull out of GnomeVFS (gnome-vfs-uri and consorts)
 */

/* gnome-vfs-uri.h - URI handling for the GNOME Virtual File System.

   Copyright (C) 1999 Free Software Foundation

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Ettore Perazzoli <ettore@comm2000.it> */

#include <glib.h>

G_BEGIN_DECLS

typedef struct {
	gchar *uri;
	gchar *scheme;
	gchar *host_name;
	guint  host_port;
	gchar *user_name;
	gchar *password;
}
	NAGnomeVFSURI;

/**
 * GNOME_VFS_URI_MAGIC_CHR:
 *
 * The character used to divide location from
 * extra "arguments" passed to the method.
 **/
/**
 * GNOME_VFS_URI_MAGIC_STR:
 *
 * The character used to divide location from
 * extra "arguments" passed to the method.
 **/
#define GNOME_VFS_URI_MAGIC_CHR	'#'
#define GNOME_VFS_URI_MAGIC_STR "#"

/**
 * GNOME_VFS_URI_PATH_CHR:
 *
 * Defines the path seperator character.
 **/
/**
 * GNOME_VFS_URI_PATH_STR:
 *
 * Defines the path seperator string.
 **/
#define GNOME_VFS_URI_PATH_CHR '/'
#define GNOME_VFS_URI_PATH_STR "/"

void na_gnome_vfs_uri_parse( NAGnomeVFSURI *vfs, const gchar *uri );

void na_gnome_vfs_uri_free( NAGnomeVFSURI *vfs );

G_END_DECLS

#endif /* __NA_GNOME_VFS_URI_H__ */
