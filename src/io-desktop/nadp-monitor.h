/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2012 Pierre Wieser and others (see AUTHORS)
 *
 * Nautilus-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General  Public  License  as
 * published by the Free Software Foundation; either  version  2  of
 * the License, or (at your option) any later version.
 *
 * Nautilus-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even  the  implied  warranty  of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See  the  GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public  License
 * along with Nautilus-Actions; see the file  COPYING.  If  not,  see
 * <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Frederic Ruaudel <grumz@grumz.net>
 *   Rodrigo Moya <rodrigo@gnome-db.org>
 *   Pierre Wieser <pwieser@trychlos.org>
 *   ... and many others (see AUTHORS)
 */

#ifndef __NADP_MONITOR_H__
#define __NADP_MONITOR_H__

/**
 * SECTION: nadp_monitor
 * @short_description: #NadpMonitor class definition.
 * @include: nadp-monitor.h
 *
 * This class manages monitoring on .desktop files and directories.
 * We also put a monitor on directories which do not exist, to be
 * triggered when a file is dropped there.
 *
 * During tests of GIO monitoring, we don't have found any case where a
 * file monitor would be triggered without the parent directory monitor
 * has been itself triggered. We, so only monitor directories (not files).
 * More, as several events may be triggered for one user modification,
 * we try to factorize all monitor events before advertizing NAPivot.
 */

#include "nadp-desktop-provider.h"

G_BEGIN_DECLS

#define NADP_TYPE_MONITOR                ( nadp_monitor_get_type())
#define NADP_MONITOR( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, NADP_TYPE_MONITOR, NadpMonitor ))
#define NADP_MONITOR_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, NADP_TYPE_MONITOR, NadpMonitorClass ))
#define NADP_IS_MONITOR( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, NADP_TYPE_MONITOR ))
#define NADP_IS_MONITOR_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), NADP_TYPE_MONITOR ))
#define NADP_MONITOR_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), NADP_TYPE_MONITOR, NadpMonitorClass ))

typedef struct _NadpMonitorPrivate       NadpMonitorPrivate;

typedef struct {
	/*< private >*/
	GObject             parent;
	NadpMonitorPrivate *private;
}
	NadpMonitor;

typedef struct _NadpMonitorClassPrivate  NadpMonitorClassPrivate;

typedef struct {
	/*< private >*/
	GObjectClass             parent;
	NadpMonitorClassPrivate *private;
}
	NadpMonitorClass;

GType        nadp_monitor_get_type( void );

NadpMonitor *nadp_monitor_new( const NadpDesktopProvider *provider, const gchar *path );

G_END_DECLS

#endif /* __NADP_MONITOR_H__ */
