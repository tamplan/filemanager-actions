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

#ifndef __NACT_ISTATUSBAR_H__
#define __NACT_ISTATUSBAR_H__

/*
 * NactIActionTab interface definition.
 *
 * This interface implements the "Nautilus Menu Item" box.
 */

#include "nact-window.h"

G_BEGIN_DECLS

#define NACT_ISTATUSBAR_TYPE						( nact_istatusbar_get_type())
#define NACT_ISTATUSBAR( object )					( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_ISTATUSBAR_TYPE, NactIStatusbar ))
#define NACT_IS_ISTATUSBAR( object )				( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_ISTATUSBAR_TYPE ))
#define NACT_ISTATUSBAR_GET_INTERFACE( instance )	( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NACT_ISTATUSBAR_TYPE, NactIStatusbarInterface ))

typedef struct NactIStatusbar NactIStatusbar;

typedef struct NactIStatusbarInterfacePrivate NactIStatusbarInterfacePrivate;

typedef struct {
	GTypeInterface                  parent;
	NactIStatusbarInterfacePrivate *private;

	/* api */
	GtkStatusbar * ( *get_statusbar )( NactWindow *window );
}
	NactIStatusbarInterface;

GType nact_istatusbar_get_type( void );

void  nact_istatusbar_display_status( NactWindow *window, const gchar *context, const gchar *status );
void  nact_istatusbar_hide_status( NactWindow *window, const gchar *context );

G_END_DECLS

#endif /* __NACT_ISTATUSBAR_H__ */
