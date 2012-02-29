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

#ifndef __NAUTILUS_ACTIONS_API_NA_GCONF_MONITOR_H__
#define __NAUTILUS_ACTIONS_API_NA_GCONF_MONITOR_H__

#ifdef HAVE_GCONF
#ifdef NA_ENABLE_DEPRECATED
/**
 * SECTION: gconf-monitor
 * @title: NAGConfMonitor
 * @short_description: The GConf Monitoring Class Definition
 * @include: nautilus-actions/na-gconf-monitor.h
 *
 * This class manages the GConf monitoring.
 * It is used to monitor both the GConf provider and the GConf runtime
 * preferences.
 *
 * Starting with Nautilus-Actions 3.1.0, GConf, whether it is used as a
 * preference storage subsystem or as an I/O provider, is deprecated.
 */

#include <gconf/gconf-client.h>

G_BEGIN_DECLS

#define NA_GCONF_MONITOR_TYPE                ( na_gconf_monitor_get_type())
#define NA_GCONF_MONITOR( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, NA_GCONF_MONITOR_TYPE, NAGConfMonitor ))
#define NA_GCONF_MONITOR_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, NA_GCONF_MONITOR_TYPE, NAGConfMonitorClass ))
#define NA_IS_GCONF_MONITOR( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, NA_GCONF_MONITOR_TYPE ))
#define NA_IS_GCONF_MONITOR_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), NA_GCONF_MONITOR_TYPE ))
#define NA_GCONF_MONITOR_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), NA_GCONF_MONITOR_TYPE, NAGConfMonitorClass ))

typedef struct _NAGConfMonitorPrivate        NAGConfMonitorPrivate;

typedef struct {
	/*< private >*/
	GObject                parent;
	NAGConfMonitorPrivate *private;
}
	NAGConfMonitor;

typedef struct _NAGConfMonitorClassPrivate   NAGConfMonitorClassPrivate;

typedef struct {
	/*< private >*/
	GObjectClass                parent;
	NAGConfMonitorClassPrivate *private;
}
	NAGConfMonitorClass;

GType           na_gconf_monitor_get_type( void );

NAGConfMonitor *na_gconf_monitor_new( const gchar *path, GConfClientNotifyFunc handler, gpointer user_data );

void            na_gconf_monitor_release_monitors( GList *monitors );

G_END_DECLS

#endif /* NA_ENABLE_DEPRECATED */
#endif /* HAVE_GCONF */
#endif /* __NAUTILUS_ACTIONS_API_NA_GCONF_MONITOR_H__ */
