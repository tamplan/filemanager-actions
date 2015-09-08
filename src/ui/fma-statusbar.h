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

#ifndef __UI_FMA_STATUSBAR_H__
#define __UI_FMA_STATUSBAR_H__

/**
 * SECTION: fma_statusbar
 * @short_description: Statusbar class definition.
 * @include: ui/fma-statusbar.h
 *
 * The #FMAStatusbar embeds both:
 * - a message bar
 * - a read-only indicator.
 */

#include "fma-main-window-def.h"

G_BEGIN_DECLS

#define FMA_TYPE_STATUSBAR                ( fma_statusbar_get_type())
#define FMA_STATUSBAR( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_STATUSBAR, FMAStatusbar ))
#define FMA_STATUSBAR_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_STATUSBAR, FMAStatusbarClass ))
#define FMA_IS_STATUSBAR( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_STATUSBAR ))
#define FMA_IS_STATUSBAR_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_STATUSBAR ))
#define FMA_STATUSBAR_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_STATUSBAR, FMAStatusbarClass ))

typedef struct _FMAStatusbarPrivate       FMAStatusbarPrivate;

typedef struct {
	/*< private >*/
	GtkStatusbar         parent;
	FMAStatusbarPrivate *private;
}
	FMAStatusbar;

typedef struct {
	/*< private >*/
	GtkStatusbarClass    parent;
}
	FMAStatusbarClass;

GType         fma_statusbar_get_type            ( void );

FMAStatusbar *fma_statusbar_new                 ( void );

void          fma_statusbar_display_status      ( FMAStatusbar *bar,
														const gchar *context,
														const gchar *status );

void          fma_statusbar_display_with_timeout( FMAStatusbar *bar,
														const gchar *context,
														const gchar *status );

void          fma_statusbar_hide_status         ( FMAStatusbar *bar,
														const gchar *context );

void          fma_statusbar_set_locked          ( FMAStatusbar *bar,
														gboolean readonly,
														gint reason );

G_END_DECLS

#endif /* __UI_FMA_STATUSBAR_H__ */
