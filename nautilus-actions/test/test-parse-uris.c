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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib-object.h>
#include <glib/gprintf.h>
#include <glib/gi18n.h>
#include <stdlib.h>

#include <runtime/na-gnome-vfs-uri.h>

static const gchar *uris[] = {
		"http://robert:azerty01@mon.domain.com/path/to/a/document?query#anchor",
		"ssh://pwi.dyndns.biz:2207",
		"sftp://kde.org:1234/pub/kde",
		"/usr/bin/nautilus-actions-config-tool",
		NULL
};

int
main( int argc, char** argv )
{
	int i;

	g_type_init();
	g_printf( _( "URIs parsing test.\n\n" ));

	for( i = 0 ; uris[i] ; ++i ){
		NAGnomeVFSURI *vfs = g_new0( NAGnomeVFSURI, 1 );
		na_gnome_vfs_uri_parse( vfs, uris[i] );
		g_printf( "original  uri=%s\n", uris[i] );
		g_printf( "vfs       uri=%s\n", vfs->uri );
		g_printf( "vfs    scheme=%s\n", vfs->scheme );
		g_printf( "vfs host_name=%s\n", vfs->host_name );
		g_printf( "vfs host_port=%d\n", vfs->host_port );
		g_printf( "vfs user_name=%s\n", vfs->user_name );
		g_printf( "vfs  password=%s\n", vfs->password );
		g_printf( "\n" );
	}

	return( EXIT_SUCCESS );
}
