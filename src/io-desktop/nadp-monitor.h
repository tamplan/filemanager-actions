/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011 Pierre Wieser and others (see AUTHORS)
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

#define NADP_MONITOR_TYPE					( nadp_monitor_get_type())
#define NADP_MONITOR( object )				( G_TYPE_CHECK_INSTANCE_CAST( object, NADP_MONITOR_TYPE, NadpMonitor ))
#define NADP_MONITOR_CLASS( klass )			( G_TYPE_CHECK_CLASS_CAST( klass, NADP_MONITOR_TYPE, NadpMonitorClass ))
#define NADP_IS_MONITOR( object )			( G_TYPE_CHECK_INSTANCE_TYPE( object, NADP_MONITOR_TYPE ))
#define NADP_IS_MONITOR_CLASS( klass )		( G_TYPE_CHECK_CLASS_TYPE(( klass ), NADP_MONITOR_TYPE ))
#define NADP_MONITOR_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), NADP_MONITOR_TYPE, NadpMonitorClass ))

typedef struct NadpMonitorPrivate      NadpMonitorPrivate;

typedef struct {
	GObject             parent;
	NadpMonitorPrivate *private;
}
	NadpMonitor;

typedef struct NadpMonitorClassPrivate NadpMonitorClassPrivate;

typedef struct {
	GObjectClass             parent;
	NadpMonitorClassPrivate *private;
}
	NadpMonitorClass;

GType        nadp_monitor_get_type( void );

NadpMonitor *nadp_monitor_new( const NadpDesktopProvider *provider, const gchar *path );

G_END_DECLS

#endif /* __NADP_MONITOR_H__ */
