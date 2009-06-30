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

#ifndef __NACT_PREFS_H__
#define __NACT_PREFS_H__

/*
 * NactIPrefs interface definition.
 *
 * This interface should be implemented by all dialogs which wish take
 * benefit of preferences management.
 */

#include <glib-object.h>

#include "nact-window.h"

G_BEGIN_DECLS

#define NACT_IPREFS_TYPE						( nact_iprefs_get_type())
#define NACT_IPREFS( object )					( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_IPREFS_TYPE, NactIPrefs ))
#define NACT_IS_IPREFS( object )				( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_IPREFS_TYPE ))
#define NACT_IPREFS_GET_INTERFACE( instance )	( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NACT_IPREFS_TYPE, NactIPrefsInterface ))

typedef struct NactIPrefs NactIPrefs;

typedef struct NactIPrefsInterfacePrivate NactIPrefsInterfacePrivate;

typedef struct {
	GTypeInterface              parent;
	NactIPrefsInterfacePrivate *private;

	/* api */
	gchar * ( *get_iprefs_window_id )( NactWindow *window );
}
	NactIPrefsInterface;

GType nact_iprefs_get_type( void );

void  nact_iprefs_position_window( NactWindow *window );
void  nact_iprefs_save_window_position( NactWindow *window );

/* .. */
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>

typedef struct _NactPreferences NactPreferences;

struct _NactPreferences {
	GSList* schemes;
	gint main_size_width;
	gint main_size_height;
	gint main_position_x;
	gint main_position_y;
	gint edit_size_width;
	gint edit_size_height;
	gint edit_position_x;
	gint edit_position_y;
	gint im_ex_size_width;
	gint im_ex_size_height;
	gint im_ex_position_x;
	gint im_ex_position_y;
	gchar* icon_last_browsed_dir;
	gchar* path_last_browsed_dir;
	gchar* import_last_browsed_dir;
	gchar* export_last_browsed_dir;
	GConfClient* client;
	guint prefs_notify_id;
};

GSList* nact_prefs_get_schemes_list (void);

void nact_prefs_set_schemes_list (GSList* schemes);

gboolean nact_prefs_get_main_dialog_size (gint* width, gint* height);
void nact_prefs_set_main_dialog_size (GtkWindow* dialog);

gboolean nact_prefs_get_edit_dialog_size (gint* width, gint* height);
void nact_prefs_set_edit_dialog_size (GtkWindow* dialog);

gboolean nact_prefs_get_im_ex_dialog_size (gint* width, gint* height);
void nact_prefs_set_im_ex_dialog_size (GtkWindow* dialog);


gboolean nact_prefs_get_main_dialog_position (gint* x, gint* y);
void nact_prefs_set_main_dialog_position (GtkWindow* dialog);

gboolean nact_prefs_get_edit_dialog_position (gint* x, gint* y);
void nact_prefs_set_edit_dialog_position (GtkWindow* dialog);

gboolean nact_prefs_get_im_ex_dialog_position (gint* x, gint* y);
void nact_prefs_set_im_ex_dialog_position (GtkWindow* dialog);


gchar* nact_prefs_get_icon_last_browsed_dir (void);
void nact_prefs_set_icon_last_browsed_dir (const gchar* path);

gchar* nact_prefs_get_path_last_browsed_dir (void);
void nact_prefs_set_path_last_browsed_dir (const gchar* path);

gchar* nact_prefs_get_import_last_browsed_dir (void);
void nact_prefs_set_import_last_browsed_dir (const gchar* path);

gchar* nact_prefs_get_export_last_browsed_dir (void);
void nact_prefs_set_export_last_browsed_dir (const gchar* path);


void nact_prefs_save_preferences (void);

G_END_DECLS

#endif /* __NACT_IPREFS_H__ */
