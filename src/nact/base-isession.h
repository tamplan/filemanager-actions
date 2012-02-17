/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011, 2012 Pierre Wieser and others (see AUTHORS)
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

#ifndef __BASE_ISESSION_H__
#define __BASE_ISESSION_H__

/**
 * SECTION: base_isession
 * @short_description: #BaseISession interface definition.
 * @include: nact/base-isession.h
 *
 * This interface implements the features needed to monitor the end of the
 * session, thus letting the application react if some modifications need
 * to be saved before allowing to quit.
 *
 * This interface is implemented by the BaseApplication class.
 */

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BASE_TYPE_ISESSION                      ( base_isession_get_type())
#define BASE_ISESSION( instance )               ( G_TYPE_CHECK_INSTANCE_CAST( instance, BASE_TYPE_ISESSION, BaseISession ))
#define BASE_IS_ISESSION( instance )            ( G_TYPE_CHECK_INSTANCE_TYPE( instance, BASE_TYPE_ISESSION ))
#define BASE_ISESSION_GET_INTERFACE( instance ) ( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), BASE_TYPE_ISESSION, BaseISessionInterface ))

typedef struct _BaseISession                    BaseISession;
typedef struct _BaseISessionInterfacePrivate    BaseISessionInterfacePrivate;

typedef struct {
	/*< private >*/
	GTypeInterface                parent;
	BaseISessionInterfacePrivate *private;
}
	BaseISessionInterface;

/**
 * Signals defined by the BaseISession interface
 */
#define BASE_SIGNAL_QUIT_REQUESTED			"base-signal-isession-quit-requested"
#define BASE_SIGNAL_QUIT					"base-signal-isession-quit"

GType    base_isession_get_type          ( void );

void     base_isession_init              ( BaseISession *instance );

gboolean base_isession_is_willing_to_quit( const BaseISession *instance );

G_END_DECLS

#endif /* __BASE_ISESSION_H__ */
