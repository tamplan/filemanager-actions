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

#ifndef __FILEMANAGER_ACTIONS_API_GCONF_MONITOR_H__
#define __FILEMANAGER_ACTIONS_API_GCONF_MONITOR_H__

#ifdef HAVE_GCONF
#ifdef NA_ENABLE_DEPRECATED
/**
 * SECTION: gconf-monitor
 * @title: FMAGConfMonitor
 * @short_description: The GConf Monitoring Class Definition
 * @include: filemanager-actions/fma-gconf-monitor.h
 *
 * This class manages the GConf monitoring.
 * It is used to monitor both the GConf provider and the GConf runtime
 * preferences.
 *
 * Starting with FileManager-Actions 3.1.0, GConf, whether it is used as a
 * preference storage subsystem or as an I/O provider, is deprecated.
 */

#include <gconf/gconf-client.h>

G_BEGIN_DECLS

#define FMA_GCONF_MONITOR_TYPE                ( fma_gconf_monitor_get_type())
#define FMA_GCONF_MONITOR( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_GCONF_MONITOR_TYPE, FMAGConfMonitor ))
#define FMA_GCONF_MONITOR_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_GCONF_MONITOR_TYPE, FMAGConfMonitorClass ))
#define FMA_IS_GCONF_MONITOR( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_GCONF_MONITOR_TYPE ))
#define FMA_IS_GCONF_MONITOR_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_GCONF_MONITOR_TYPE ))
#define FMA_GCONF_MONITOR_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_GCONF_MONITOR_TYPE, FMAGConfMonitorClass ))

typedef struct _FMAGConfMonitorPrivate        FMAGConfMonitorPrivate;

typedef struct {
	/*< private >*/
	GObject                 parent;
	FMAGConfMonitorPrivate *private;
}
	FMAGConfMonitor;

typedef struct _FMAGConfMonitorClassPrivate   FMAGConfMonitorClassPrivate;

typedef struct {
	/*< private >*/
	GObjectClass                 parent;
	FMAGConfMonitorClassPrivate *private;
}
	FMAGConfMonitorClass;

GType            fma_gconf_monitor_get_type( void );

FMAGConfMonitor *fma_gconf_monitor_new( const gchar *path, GConfClientNotifyFunc handler, gpointer user_data );

void             fma_gconf_monitor_release_monitors( GList *monitors );

G_END_DECLS

#endif /* NA_ENABLE_DEPRECATED */
#endif /* HAVE_GCONF */
#endif /* __FILEMANAGER_ACTIONS_API_GCONF_MONITOR_H__ */
