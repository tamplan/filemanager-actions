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

#ifndef __NAUTILUS_ACTIONS_API_NA_GCONF_MONITOR_H__
#define __NAUTILUS_ACTIONS_API_NA_GCONF_MONITOR_H__

/**
 * SECTION: na_gconf_monitor
 * @short_description: #NAGConfMonitor class definition.
 * @include: nautilus-actions/na-gconf-monitor.h
 *
 * This class manages the GConf monitoring.
 * It is used to monitor both the GConf provider and the GConf runtime
 * preferences.
 */

#include <gconf/gconf-client.h>

G_BEGIN_DECLS

#define NA_GCONF_MONITOR_TYPE					( na_gconf_monitor_get_type())
#define NA_GCONF_MONITOR( object )				( G_TYPE_CHECK_INSTANCE_CAST( object, NA_GCONF_MONITOR_TYPE, NAGConfMonitor ))
#define NA_GCONF_MONITOR_CLASS( klass )			( G_TYPE_CHECK_CLASS_CAST( klass, NA_GCONF_MONITOR_TYPE, NAGConfMonitorClass ))
#define NA_IS_GCONF_MONITOR( object )			( G_TYPE_CHECK_INSTANCE_TYPE( object, NA_GCONF_MONITOR_TYPE ))
#define NA_IS_GCONF_MONITOR_CLASS( klass )		( G_TYPE_CHECK_CLASS_TYPE(( klass ), NA_GCONF_MONITOR_TYPE ))
#define NA_GCONF_MONITOR_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), NA_GCONF_MONITOR_TYPE, NAGConfMonitorClass ))

typedef struct NAGConfMonitorPrivate NAGConfMonitorPrivate;

typedef struct {
	GObject                parent;
	NAGConfMonitorPrivate *private;
}
	NAGConfMonitor;

typedef struct NAGConfMonitorClassPrivate NAGConfMonitorClassPrivate;

typedef struct {
	GObjectClass                parent;
	NAGConfMonitorClassPrivate *private;
}
	NAGConfMonitorClass;

GType           na_gconf_monitor_get_type( void );

NAGConfMonitor *na_gconf_monitor_new( const gchar *path, GConfClientNotifyFunc handler, gpointer user_data );

void            na_gconf_monitor_release_monitors( GList *monitors );

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_API_NA_GCONF_MONITOR_H__ */
