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

#ifndef __NA_COMMON_XML_NAMES_H__
#define __NA_COMMON_XML_NAMES_H__

#include <glib/gi18n.h>

G_BEGIN_DECLS

/* import/export formats
 *
 * FORMAT_GCONFSCHEMAFILE_V1: a schema with owner, short and long
 * descriptions ; each action has its own schema addressed by the uuid
 * (historical format up to v1.10.x serie)
 *
 * FORMAT_GCONFSCHEMAFILE_V2: the lightest schema still compatible
 * with gconftool-2 --install-schema-file (no owner, no short nor long
 * descriptions) - introduced in v 1.11
 *
 * FORMAT_GCONFENTRY: not a schema, but a dump of the GConf entry
 * introduced in v 1.11
 */
enum {
	FORMAT_GCONFSCHEMAFILE_V1 = 1,
	FORMAT_GCONFSCHEMAFILE_V2,
	FORMAT_GCONFENTRY,
	FORMAT_GCONFSCHEMA
};

/* XML element names (GConf schema)
 * used in FORMAT_GCONFSCHEMAFILE_V1 and FORMAT_GCONFSCHEMAFILE_V2
 */
#define NACT_GCONF_SCHEMA_ROOT				"gconfschemafile"
#define NACT_GCONF_SCHEMA_LIST				"schemalist"
#define NACT_GCONF_SCHEMA_ENTRY				"schema"
#define NACT_GCONF_SCHEMA_KEY				"key"
#define NACT_GCONF_SCHEMA_APPLYTO			"applyto"
#define NACT_GCONF_SCHEMA_TYPE				"type"
#define NACT_GCONF_SCHEMA_LIST_TYPE			"list_type"
#define NACT_GCONF_SCHEMA_LOCALE			"locale"
#define NACT_GCONF_SCHEMA_DEFAULT			"default"

/* Previouly used keys
 *
 * Up to v 1.10, export used to contain a full schema description,
 * while import only checked for applyto keys (along with locale
 * and default)
 *
 * Starting with 1.11, we have introduced a lighter export schema
 * (mainly without owner and short and long descriptions)
 *
 * only used in FORMAT_GCONFSCHEMAFILE_V1
 */
#define NACT_GCONF_SCHEMA_OWNER				"owner"
#define NACT_GCONF_SCHEMA_SHORT				"short"
#define NACT_GCONF_SCHEMA_LONG				"long"

/* XML element names (GConf dump)
 * used in FORMAT_GCONFENTRY
 */
#define NACT_GCONF_DUMP_ROOT				"gconfentryfile"
#define NACT_GCONF_DUMP_ENTRYLIST			"entrylist"
#define NACT_GCONF_DUMP_ENTRYLIST_BASE		"base"
#define NACT_GCONF_DUMP_ENTRY				"entry"
#define NACT_GCONF_DUMP_KEY					"key"
#define NACT_GCONF_DUMP_VALUE				"value"
#define NACT_GCONF_DUMP_STRING				"string"
#define NACT_GCONF_DUMP_LIST				"list"
#define NACT_GCONF_DUMP_LIST_TYPE			"type"

/* GConf schema descriptions
 */
#define ACTION_LABEL_DESC_SHORT		_( "The label of the menu item" )
#define ACTION_LABEL_DESC_LONG		_( "The label of the menu item that will appear in the Nautilus popup " \
										"menu when the selection matches the appearance condition settings" )
#define ACTION_TOOLTIP_DESC_SHORT	_( "The tooltip of the menu item" )
#define ACTION_TOOLTIP_DESC_LONG	_( "The tooltip of the menu item that will appear in the Nautilus " \
										"statusbar when the user points to the Nautilus popup menu item "\
										"with his/her mouse" )
#define ACTION_ICON_DESC_SHORT		_( "The icon of the menu item" )
#define ACTION_ICON_DESC_LONG		_( "The icon of the menu item that will appear next to the label " \
										"in the Nautilus popup menu when the selection matches the appearance " \
										"conditions settings" )
#define ACTION_ENABLED_DESC_SHORT	_( "Whether the action is enabled" )
#define ACTION_ENABLED_DESC_LONG	_( "If the action is disabled, it will never appear in the Nautilus " \
										"context menu" )
#define ACTION_PROFILE_NAME_DESC_SHORT _( "A description name of the profile" )
#define ACTION_PROFILE_NAME_DESC_LONG  _( "The field is here to give the user a human readable name for a " \
										"profile in the Nact interface. If not set there will be a default " \
										"auto generated string set by default" )
#define ACTION_PATH_DESC_SHORT		_( "The path of the command" )
#define ACTION_PATH_DESC_LONG		_( "The path of the command to start when the user select the menu item " \
										"in the Nautilus popup menu" )
#define ACTION_PARAMETERS_DESC_SHORT _( "The parameters of the command" )
/* too long string for iso c: 665 (max=509) */
#define ACTION_PARAMETERS_DESC_LONG	_( "The parameters of the command to start when the user selects the menu " \
										"item in the Nautilus popup menu.\n\nThe parameters can contain some " \
										"special tokens which are replaced by Nautilus information before starting " \
										"the command:\n\n%d: base folder of the selected file(s)\n%f: the name of " \
										"the selected file or the first one if many are selected\n%h: hostname of " \
										"the URI\n%m: space-separated list of the basenames of the selected " \
										"file(s)/folder(s)\n%M: space-separated list of the selected file(s)/folder(s), " \
										"with their full paths\n%p: port number of the first URI\n%R: space-separated " \
										"list of selected URIs\n%s: scheme of the URI\n%u: URI\n%U: username of the " \
										"URI\n%%: a percent sign" )
#define ACTION_BASENAMES_DESC_SHORT	_( "The list of pattern to match the selected file(s)/folder(s)" )
#define ACTION_BASENAMES_DESC_LONG	_( "A list of strings with joker '*' or '?' to match the name of the selected " \
										"file(s)/folder(s). Each selected items must match at least one of the " \
										"filename patterns for the action to appear" )
#define ACTION_MATCHCASE_DESC_SHORT _( "'true' if the filename patterns have to be case sensitive, 'false' otherwise" )
#define ACTION_MATCHCASE_DESC_LONG	_( "If you need to match a filename in a case-sensitive manner, set this key to " \
										"'true'. If you also want, for example '*.jpg' to match 'photo.JPG', " \
										"set 'false'" )
#define ACTION_MIMETYPES_DESC_SHORT	_( "The list of patterns to match the mimetypes of the selected file(s)" )
#define ACTION_MIMETYPES_DESC_LONG	_( "A list of strings with joker '*' or '?' to match the mimetypes of the selected " \
										"file(s). Each selected items must match at least one of the " \
										"mimetype patterns for the action to appear" )
#define ACTION_ISFILE_DESC_SHORT	_( "'true' if the selection can have files, 'false' otherwise" )
#define ACTION_ISFILE_DESC_LONG		_( "This setting is tied in with the 'isdir' setting. The valid combinations " \
										"are:\n\nisfile=TRUE and isdir=FALSE: the selection may hold only files\n" \
										"isfile=FALSE and isdir=TRUE: the selection may hold only folders\n" \
										"isfile=TRUE and isdir=TRUE: the selection may hold both files and folders\n" \
										"isfile=FALSE and isdir=FALSE: this is an invalid combination (your configuration " \
										"will never appear)" )
#define ACTION_ISDIR_DESC_SHORT		_( "'true' if the selection can have folders, 'false' otherwise" )
#define ACTION_ISDIR_DESC_LONG		_( "This setting is tied in with the 'isfile' setting. The valid combinations " \
										"are:\n\nisfile=TRUE and isdir=FALSE: the selection may hold only files\n" \
										"isfile=FALSE and isdir=TRUE: the selection may hold only folders\n" \
										"isfile=TRUE and isdir=TRUE: the selection may hold both files and folders\n" \
										"isfile=FALSE and isdir=FALSE: this is an invalid combination (your configuration " \
										"will never appear)" )
#define ACTION_MULTIPLE_DESC_SHORT	_( "'true' if the selection can have several items, 'false' otherwise" )
#define ACTION_MULTIPLE_DESC_LONG	_( "If you need one or more files or folders to be selected, set this " \
										"key to 'true'. If you want just one file or folder, set 'false'" )
#define ACTION_SCHEMES_DESC_SHORT	_( "The list of schemes where the selected files should be located" )
/* too long string for iso c: 510 (max=509) */
#define ACTION_SCHEMES_DESC_LONG	_( "Defines the list of valid schemes to be matched against the selected " \
										"items. The scheme is the protocol used to access the files. The " \
										"keyword to use is the one used in the URI.\n\nExamples of valid " \
										"URI include:\nfile:///tmp/foo.txt\nsftp:///root@test.example.net/tmp/foo.txt\n\n" \
										"The most common schemes are:\n\n'file': local files\n'sftp': files accessed via SSH\n" \
										"'ftp': files accessed via FTP\n'smb': files accessed via Samba (Windows share)\n" \
										"'dav': files accessed via WebDAV\n\nAll schemes used by Nautilus can be used here." )
#define ACTION_VERSION_DESC_SHORT	_( "The version of the configuration format" )
#define ACTION_VERSION_DESC_LONG	_( "The version of the configuration format that will be used to manage " \
										"backward compatibility" )

G_END_DECLS

#endif /* __NA_COMMON_XML_NAMES_H__ */
