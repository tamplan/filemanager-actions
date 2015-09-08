/*
 * FileManager-Actions
 * A file-manager extension which offers configurable context statusbar actions.
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

#ifndef __UI_NACT_STATUSBAR_H__
#define __UI_NACT_STATUSBAR_H__

/**
 * SECTION: nact_statusbar
 * @short_description: Statusbar class definition.
 * @include: ui/nact-statusbar.h
 *
 * The #NactStatusbar embeds both:
 * - a message bar
 * - a read-only indicator.
 */

#include "nact-main-window-def.h"

G_BEGIN_DECLS

#define NACT_TYPE_STATUSBAR                ( nact_statusbar_get_type())
#define NACT_STATUSBAR( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_TYPE_STATUSBAR, NactStatusbar ))
#define NACT_STATUSBAR_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, NACT_TYPE_STATUSBAR, NactStatusbarClass ))
#define NACT_IS_STATUSBAR( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_TYPE_STATUSBAR ))
#define NACT_IS_STATUSBAR_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), NACT_TYPE_STATUSBAR ))
#define NACT_STATUSBAR_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), NACT_TYPE_STATUSBAR, NactStatusbarClass ))

typedef struct _NactStatusbarPrivate       NactStatusbarPrivate;

typedef struct {
	/*< private >*/
	GtkStatusbar          parent;
	NactStatusbarPrivate *private;
}
	NactStatusbar;

typedef struct {
	/*< private >*/
	GtkStatusbarClass     parent;
}
	NactStatusbarClass;

GType          nact_statusbar_get_type            ( void );

NactStatusbar *nact_statusbar_new                 ( void );

void           nact_statusbar_display_status      ( NactStatusbar *bar,
														const gchar *context,
														const gchar *status );

void           nact_statusbar_display_with_timeout( NactStatusbar *bar,
														const gchar *context,
														const gchar *status );

void           nact_statusbar_hide_status         ( NactStatusbar *bar,
														const gchar *context );

void           nact_statusbar_set_locked          ( NactStatusbar *bar,
														gboolean readonly,
														gint reason );

G_END_DECLS

#endif /* __UI_NACT_STATUSBAR_H__ */
