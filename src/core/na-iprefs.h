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

#ifndef __CORE_NA_IPREFS_H__
#define __CORE_NA_IPREFS_H__

/*
 * Starting with 3.1.0, NAIPrefs interface is deprecated.
 *
 * Instead, this module is used as an intermediate level between actual
 * settings and the application; it so implements all maps needed to
 * transform an enum used in the code to and from a string stored in
 * preferences.
 */

#include "na-pivot.h"

G_BEGIN_DECLS

/* sort mode of the items in the file manager context menu
 */
enum {
	IPREFS_ORDER_ALPHA_ASCENDING = 1,	/* default */
	IPREFS_ORDER_ALPHA_DESCENDING,
	IPREFS_ORDER_MANUAL
};

void     na_iprefs_set_import_mode        ( const gchar *pref, guint mode );

guint    na_iprefs_get_order_mode         ( gboolean *mandatory );
guint    na_iprefs_get_order_mode_by_label( const gchar *label );
void     na_iprefs_set_order_mode         ( guint mode );

GQuark   na_iprefs_get_export_format      ( const gchar *pref, gboolean *mandatory );
void     na_iprefs_set_export_format      ( const gchar *pref, GQuark format );

GSList  *na_iprefs_get_io_providers       ( void );

gboolean na_iprefs_write_level_zero       ( const GList *items, GSList **messages );

G_END_DECLS

#endif /* __CORE_NA_IPREFS_H__ */
