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

/**
 * SECTION: na_iprefs
 * @short_description: #NAIPrefs interface definition extension.
 * @include: common/na-iprefs.h
 *
 * #NAIPrefs main interface is defined as part of libna-runtime
 * convenience library, and should only be implemented by #NAPivot.
 * Found here is public API not shared by the Nautilus Actions plugin.
 */

#include <runtime/na-iprefs.h>

G_BEGIN_DECLS

/* GConf Preference keys managed by IPrefs interface
 */
#define IPREFS_IMPORT_ACTIONS_IMPORT_MODE	"import-mode"
#define IPREFS_IMPORT_ASK_LAST_MODE			"import-ask-user-last-mode"
#define IPREFS_IMPORT_ASK_KEEP_MODE			"import-ask-user-keep-mode"

/* import mode
 */
enum {
	IPREFS_IMPORT_NO_IMPORT = 1,
	IPREFS_IMPORT_RENUMBER,
	IPREFS_IMPORT_OVERRIDE,
	IPREFS_IMPORT_ASK
};

#define IPREFS_RELABEL_MENUS				"iprefs-relabel-menus"
#define IPREFS_RELABEL_ACTIONS				"iprefs-relabel-actions"
#define IPREFS_RELABEL_PROFILES				"iprefs-relabel-profiles"

void     na_iprefs_migrate_key( NAIPrefs *instance, const gchar *old_key, const gchar *new_key );

gint     na_iprefs_get_import_mode( NAIPrefs *instance, const gchar *pref );
void     na_iprefs_set_import_mode( NAIPrefs *instance, const gchar *pref, gint mode );

G_END_DECLS

#endif /* __NA_IPREFS_H__ */
