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

#ifndef __UI_FMA_STATUS_BAR_H__
#define __UI_FMA_STATUS_BAR_H__

/**
 * SECTION: fma_status_bar
 * @short_description: Statusbar class definition.
 * @include: ui/fma-status-bar.h
 *
 * The #FMAStatusBar embeds both:
 * - a message bar
 * - a read-only indicator.
 */

#include "fma-main-window-def.h"

G_BEGIN_DECLS

#define FMA_TYPE_STATUS_BAR                ( fma_status_bar_get_type())
#define FMA_STATUS_BAR( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_STATUS_BAR, FMAStatusBar ))
#define FMA_STATUS_BAR_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_STATUS_BAR, FMAStatusBarClass ))
#define FMA_IS_STATUS_BAR( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_STATUS_BAR ))
#define FMA_IS_STATUS_BAR_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_STATUS_BAR ))
#define FMA_STATUS_BAR_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_STATUS_BAR, FMAStatusBarClass ))

typedef struct _FMAStatusBarPrivate        FMAStatusBarPrivate;

typedef struct {
	/*< private >*/
	GtkStatusbar         parent;
	FMAStatusBarPrivate *private;
}
	FMAStatusBar;

typedef struct {
	/*< private >*/
	GtkStatusbarClass    parent;
}
	FMAStatusBarClass;

GType         fma_status_bar_get_type            ( void );

FMAStatusBar *fma_status_bar_new                 ( void );

void          fma_status_bar_display_status      ( FMAStatusBar *bar,
														const gchar *context,
														const gchar *status );

void          fma_status_bar_display_with_timeout( FMAStatusBar *bar,
														const gchar *context,
														const gchar *status );

void          fma_status_bar_hide_status         ( FMAStatusBar *bar,
														const gchar *context );

void          fma_status_bar_set_locked          ( FMAStatusBar *bar,
														gboolean readonly,
														gint reason );

G_END_DECLS

#endif /* __UI_FMA_STATUS_BAR_H__ */
