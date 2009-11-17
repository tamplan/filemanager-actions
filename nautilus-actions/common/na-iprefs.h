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
#define IPREFS_EXPORT_FORMAT				"export-format"
#define IPREFS_EXPORT_ASK_LAST_FORMAT		"export-ask-user-last-format"
#define IPREFS_IMPORT_ACTIONS_IMPORT_MODE	"import-mode"
#define IPREFS_IMPORT_ASK_LAST_MODE			"import-ask-user-last-mode"

#define IPREFS_ASSIST_ESC_QUIT				"assistant-esc-quit"
#define IPREFS_ASSIST_ESC_CONFIRM			"assistant-esc-confirm"

/* import mode
 */
enum {
	IPREFS_IMPORT_NO_IMPORT = 1,
	IPREFS_IMPORT_RENUMBER,
	IPREFS_IMPORT_OVERRIDE,
	IPREFS_IMPORT_ASK
};

/* import/export formats
 *
 * FORMAT_GCONF_SCHEMA_V1: a schema with owner, short and long
 * descriptions ; each action has its own schema addressed by the uuid
 * (historical format up to v1.10.x serie)
 *
 * FORMAT_GCONF_SCHEMA_V2: the lightest schema still compatible
 * with gconftool-2 --install-schema-file (no owner, no short nor long
 * descriptions) - introduced in v 1.11
 *
 * FORMAT_GCONF_SCHEMA: exports a full schema, not an action
 *
 * FORMAT_GCONF_ENTRY: not a schema, but a dump of the GConf entry
 * introduced in v 1.11
 */
enum {
	IPREFS_EXPORT_NO_EXPORT = 1,
	IPREFS_EXPORT_FORMAT_GCONF_SCHEMA_V1,
	IPREFS_EXPORT_FORMAT_GCONF_SCHEMA_V2,
	IPREFS_EXPORT_FORMAT_GCONF_SCHEMA,
	IPREFS_EXPORT_FORMAT_GCONF_ENTRY,
	IPREFS_EXPORT_FORMAT_ASK
};

#define IPREFS_RELABEL_MENUS				"iprefs-relabel-menus"
#define IPREFS_RELABEL_ACTIONS				"iprefs-relabel-actions"
#define IPREFS_RELABEL_PROFILES				"iprefs-relabel-profiles"

void na_iprefs_migrate_key( NAIPrefs *instance, const gchar *old_key, const gchar *new_key );

gint na_iprefs_get_export_format( NAIPrefs *instance, const gchar *pref );
gint na_iprefs_get_import_mode( NAIPrefs *instance, const gchar *pref );

void na_iprefs_set_export_format( NAIPrefs *instance, const gchar *pref, gint format );
void na_iprefs_set_import_mode( NAIPrefs *instance, const gchar *pref, gint mode );

G_END_DECLS

#endif /* __NA_IPREFS_H__ */
