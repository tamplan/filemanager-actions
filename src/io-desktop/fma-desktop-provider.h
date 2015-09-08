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

#ifndef __IO_DESKTOP_FMA_DESKTOP_PROVIDER_H__
#define __IO_DESKTOP_FMA_DESKTOP_PROVIDER_H__

/**
 * SECTION: fma_desktop_provider
 * @short_description: #FMADesktopProvider class definition.
 * @include: fma-desktop-provider.h
 *
 * This class manages .desktop files I/O storage subsystem, or, in
 * other words, .desktop files as FMAIIOProvider providers. As this, it
 * should only be used through the FMAIIOProvider interface.
 */

#include <glib-object.h>

#include <api/fma-object-item.h>
#include <api/fma-timeout.h>

#include "fma-desktop-file.h"

G_BEGIN_DECLS

#define FMA_TYPE_DESKTOP_PROVIDER                ( fma_desktop_provider_get_type())
#define FMA_DESKTOP_PROVIDER( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_DESKTOP_PROVIDER, FMADesktopProvider ))
#define FMA_DESKTOP_PROVIDER_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_DESKTOP_PROVIDER, FMADesktopProviderClass ))
#define FMA_IS_DESKTOP_PROVIDER( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_DESKTOP_PROVIDER ))
#define FMA_IS_DESKTOP_PROVIDER_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_DESKTOP_PROVIDER ))
#define FMA_DESKTOP_PROVIDER_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_DESKTOP_PROVIDER, FMADesktopProviderClass ))

/* private instance data
 */
typedef struct _FMADesktopProviderPrivate {
	/*< private >*/
	gboolean   dispose_has_run;
	GList     *monitors;
	FMATimeout timeout;
}
	FMADesktopProviderPrivate;

typedef struct {
	/*< private >*/
	GObject                    parent;
	FMADesktopProviderPrivate *private;
}
	FMADesktopProvider;

typedef struct _FMADesktopProviderClassPrivate   FMADesktopProviderClassPrivate;

typedef struct {
	/*< private >*/
	GObjectClass                    parent;
	FMADesktopProviderClassPrivate *private;
}
	FMADesktopProviderClass;

/* this is a ':'-separated list of XDG_DATA_DIRS/subdirs searched for
 * menus or actions .desktop files.
 */
#define FMA_DESKTOP_PROVIDER_SUBDIRS	"file-manager/actions"

GType fma_desktop_provider_get_type        ( void );
void  fma_desktop_provider_register_type   ( GTypeModule *module );

void  fma_desktop_provider_add_monitor     ( FMADesktopProvider *provider, const gchar *dir );
void  fma_desktop_provider_on_monitor_event( FMADesktopProvider *provider );
void  fma_desktop_provider_release_monitors( FMADesktopProvider *provider );

G_END_DECLS

#endif /* __IO_DESKTOP_FMA_DESKTOP_PROVIDER_H__ */
