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

#ifndef __BASE_IPREFS_H__
#define __BASE_IPREFS_H__

/*
 * BaseIPrefs interface definition.
 *
 * This interface may be implemented by all dialogs which wish take
 * benefit of preferences management.
 */

#include "base-window.h"

G_BEGIN_DECLS

#define BASE_IPREFS_TYPE						( base_iprefs_get_type())
#define BASE_IPREFS( object )					( G_TYPE_CHECK_INSTANCE_CAST( object, BASE_IPREFS_TYPE, BaseIPrefs ))
#define BASE_IS_IPREFS( object )				( G_TYPE_CHECK_INSTANCE_TYPE( object, BASE_IPREFS_TYPE ))
#define BASE_IPREFS_GET_INTERFACE( instance )	( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), BASE_IPREFS_TYPE, BaseIPrefsInterface ))

typedef struct BaseIPrefs BaseIPrefs;

typedef struct BaseIPrefsInterfacePrivate BaseIPrefsInterfacePrivate;

typedef struct {
	GTypeInterface              parent;
	BaseIPrefsInterfacePrivate *private;

	/* api */
	gchar * ( *iprefs_get_window_id )( BaseWindow *window );
}
	BaseIPrefsInterface;

GType    base_iprefs_get_type( void );

void     base_iprefs_position_window( BaseWindow *window );
void     base_iprefs_position_named_window( BaseWindow *window, GtkWindow *toplevel, const gchar *name );

void     base_iprefs_save_window_position( BaseWindow *window );
void     base_iprefs_save_named_window_position( BaseWindow *window, GtkWindow *toplevel, const gchar *name );

gboolean base_iprefs_get_bool( BaseWindow *window, const gchar *key );
void     base_iprefs_set_bool( BaseWindow *window, const gchar *key, gboolean value );

gint     base_iprefs_get_int( BaseWindow *window, const gchar *key );
void     base_iprefs_set_int( BaseWindow *window, const gchar *key, gint value );

gchar   *base_iprefs_get_string( BaseWindow *window, const gchar *name );
void     base_iprefs_set_string( BaseWindow *window, const gchar *name, const gchar *string );

G_END_DECLS

#endif /* __BASE_IPREFS_H__ */
