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

#ifndef __NACT_MENUBAR_H__
#define __NACT_MENUBAR_H__

/*
 * SECTION: nact-menubar
 * @title: NactMenubar
 * @short_description: The Menubar class definition
 * @include: nact-menubar.h
 *
 * This is a convenience class which embeds the menubar of the application.
 *
 * There is one object (because there is one menubar). It is created by
 * the main window at initialization time.
 *
 * Attaching the object to the window let us connect easily to all 'window'-
 * class messages, thus reducing the count of needed public functions.
 *
 * The #NactMenubar object connects to BASE_SIGNAL_INITIALIZE_WINDOW signal
 * at instanciation time. The caller (usually a #NactMainWindow) should take
 * care to connect itself to this signal before creating the #NactMenubar
 * object if it wants its callback be triggered first.
 *
 * The #NactMenubar object maintains as private data the indicators needed
 * in order to rightly update menu items sensitivity.
 * It is up to the application to update these indicators.
 * Each time an indicator is updated, it triggers an update of all relevant
 * menu items sensitivities.
 *
 * Toolbar and sort buttons are also driven by this menubar.
 */

#include "base-window.h"

G_BEGIN_DECLS

#define NACT_TYPE_MENUBAR                ( nact_menubar_get_type())
#define NACT_MENUBAR( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_TYPE_MENUBAR, NactMenubar ))
#define NACT_MENUBAR_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, NACT_TYPE_MENUBAR, NactMenubarClass ))
#define NACT_IS_MENUBAR( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_TYPE_MENUBAR ))
#define NACT_IS_MENUBAR_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), NACT_TYPE_MENUBAR ))
#define NACT_MENUBAR_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), NACT_TYPE_MENUBAR, NactMenubarClass ))

typedef struct _NactMenubarPrivate       NactMenubarPrivate;

typedef struct {
	/*< private >*/
	GObject             parent;
	NactMenubarPrivate *private;
}
	NactMenubar;

typedef struct _NactMenubarClassPrivate  NactMenubarClassPrivate;

typedef struct {
	/*< private >*/
	GObjectClass             parent;
	NactMenubarClassPrivate *private;
}
	NactMenubarClass;

GType        nact_menubar_get_type( void );

NactMenubar *nact_menubar_new       ( BaseWindow *window );

void         nact_menubar_save_items( BaseWindow *window );

/* *** */
gboolean nact_menubar_is_level_zero_order_changed( const BaseWindow *window );
/* *** */

G_END_DECLS

#endif /* __NACT_MENUBAR_H__ */
