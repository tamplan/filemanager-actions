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

#ifndef __NA_IPREFS_H__
#define __NA_IPREFS_H__

/*
 * NAIPrefs interface definition.
 *
 * This interface is to be implemented by all modules which wish take
 * benefit of preferences management.
 */

#include <glib-object.h>

#include "na-gconf-keys.h"

G_BEGIN_DECLS

#define NA_IPREFS_TYPE						( na_iprefs_get_type())
#define NA_IPREFS( object )					( G_TYPE_CHECK_INSTANCE_CAST( object, NA_IPREFS_TYPE, NAIPrefs ))
#define NA_IS_IPREFS( object )				( G_TYPE_CHECK_INSTANCE_TYPE( object, NA_IPREFS_TYPE ))
#define NA_IPREFS_GET_INTERFACE( instance )	( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NA_IPREFS_TYPE, NAIPrefsInterface ))

typedef struct NAIPrefs NAIPrefs;

typedef struct NAIPrefsInterfacePrivate NAIPrefsInterfacePrivate;

typedef struct {
	GTypeInterface            parent;
	NAIPrefsInterfacePrivate *private;
}
	NAIPrefsInterface;

GType    na_iprefs_get_type( void );

gboolean na_iprefs_get_bool( NAIPrefs *instance, const gchar *key );
void     na_iprefs_set_bool( NAIPrefs *instance, const gchar *key, gboolean value );

/* GConf general information
 */
#define NA_GCONF_PREFS_PATH		NAUTILUS_ACTIONS_CONFIG_GCONF_BASEDIR "/" NA_GCONF_SCHEMA_PREFERENCES

/* Preference keys managed by IPrefs interface
 */
#define PREFS_DISPLAY_AS_SUBMENU			"display-as-submenu"

G_END_DECLS

#endif /* __NA_IPREFS_H__ */
