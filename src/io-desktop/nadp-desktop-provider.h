/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2013 Pierre Wieser and others (see AUTHORS)
 *
 * Nautilus-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Nautilus-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nautilus-Actions; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Frederic Ruaudel <grumz@grumz.net>
 *   Rodrigo Moya <rodrigo@gnome-db.org>
 *   Pierre Wieser <pwieser@trychlos.org>
 *   ... and many others (see AUTHORS)
 */

#ifndef __NADP_DESKTOP_PROVIDER_H__
#define __NADP_DESKTOP_PROVIDER_H__

/**
 * SECTION: nadp_desktop_provider
 * @short_description: #NadpDesktopProvider class definition.
 * @include: nadp-desktop-provider.h
 *
 * This class manages .desktop files I/O storage subsystem, or, in
 * other words, .desktop files as NAIIOProvider providers. As this, it
 * should only be used through the NAIIOProvider interface.
 */

#include <api/na-object-item.h>
#include <api/na-timeout.h>

#include "nadp-desktop-file.h"

G_BEGIN_DECLS

#define NADP_TYPE_DESKTOP_PROVIDER                ( nadp_desktop_provider_get_type())
#define NADP_DESKTOP_PROVIDER( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, NADP_TYPE_DESKTOP_PROVIDER, NadpDesktopProvider ))
#define NADP_DESKTOP_PROVIDER_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, NADP_TYPE_DESKTOP_PROVIDER, NadpDesktopProviderClass ))
#define NADP_IS_DESKTOP_PROVIDER( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, NADP_TYPE_DESKTOP_PROVIDER ))
#define NADP_IS_DESKTOP_PROVIDER_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), NADP_TYPE_DESKTOP_PROVIDER ))
#define NADP_DESKTOP_PROVIDER_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), NADP_TYPE_DESKTOP_PROVIDER, NadpDesktopProviderClass ))

/* private instance data
 */
typedef struct _NadpDesktopProviderPrivate {
	/*< private >*/
	gboolean  dispose_has_run;
	GList    *monitors;
	NATimeout timeout;
}
	NadpDesktopProviderPrivate;

typedef struct {
	/*< private >*/
	GObject                     parent;
	NadpDesktopProviderPrivate *private;
}
	NadpDesktopProvider;

typedef struct _NadpDesktopProviderClassPrivate   NadpDesktopProviderClassPrivate;

typedef struct {
	/*< private >*/
	GObjectClass                     parent;
	NadpDesktopProviderClassPrivate *private;
}
	NadpDesktopProviderClass;

/* this is a ':'-separated list of XDG_DATA_DIRS/subdirs searched for
 * menus or actions .desktop files.
 */
#define NADP_DESKTOP_PROVIDER_SUBDIRS	"file-manager/actions"

GType nadp_desktop_provider_get_type     ( void );
void  nadp_desktop_provider_register_type( GTypeModule *module );

void  nadp_desktop_provider_add_monitor     ( NadpDesktopProvider *provider, const gchar *dir );
void  nadp_desktop_provider_on_monitor_event( NadpDesktopProvider *provider );
void  nadp_desktop_provider_release_monitors( NadpDesktopProvider *provider );

G_END_DECLS

#endif /* __NADP_DESKTOP_PROVIDER_H__ */
