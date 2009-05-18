/*
 * Nautilus Actions
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

#include <config.h>
#include <glib/gi18n.h>

#ifndef __NAUTILUS_ACTIONS_CONFIG_GCONF_PRIVATE_H__
#define __NAUTILUS_ACTIONS_CONFIG_GCONF_PRIVATE_H__

#ifdef N_
#undef N_
#endif
#define N_(String) String

/* GConf general information */
#define ACTIONS_SCHEMA_PREFIX  "/schemas"
#define ACTIONS_CONFIG_DIR     NAUTILUS_ACTIONS_CONFIG_GCONF_BASEDIR "/configurations"
#define ACTIONS_CONFIG_NOTIFY_KEY     ACTIONS_CONFIG_DIR "/action_change_state"
#define ACTIONS_SCHEMA_OWNER	 "nautilus-actions"
#define ACTIONS_PROFILE_PREFIX   "profile-"

/* GConf XML element names */
#define NA_GCONF_XML_ROOT					"gconfschemafile"
#define NA_GCONF_XML_SCHEMA_LIST			"schemalist"
#define NA_GCONF_XML_SCHEMA_ENTRY		"schema"
#define NA_GCONF_XML_SCHEMA_KEY			"key"
#define NA_GCONF_XML_SCHEMA_APPLYTO		"applyto"
#define NA_GCONF_XML_SCHEMA_OWNER		"owner"
#define NA_GCONF_XML_SCHEMA_TYPE			"type"
#define NA_GCONF_XML_SCHEMA_LOCALE		"locale"
#define NA_GCONF_XML_SCHEMA_SHORT		"short"
#define NA_GCONF_XML_SCHEMA_LONG			"long"
#define NA_GCONF_XML_SCHEMA_DFT			"default"
#define NA_GCONF_XML_SCHEMA_LIST_TYPE	"list_type"

/* GConf key names : */
#define ACTION_LABEL_ENTRY     		"label"
#define ACTION_TOOLTIP_ENTRY   		"tooltip"
#define ACTION_ICON_ENTRY      		"icon"
#define ACTION_PROFILE_DESC_NAME_ENTRY	"desc-name"
#define ACTION_PATH_ENTRY      		"path"
#define ACTION_PARAMS_ENTRY    		"parameters"
#define ACTION_BASENAMES_ENTRY 		"basenames"
#define ACTION_MATCHCASE_ENTRY 		"matchcase"
#define ACTION_MIMETYPES_ENTRY 		"mimetypes"
#define ACTION_ISFILE_ENTRY    		"isfile"
#define ACTION_ISDIR_ENTRY     		"isdir"
#define ACTION_MULTIPLE_ENTRY  		"accept-multiple-files"
#define ACTION_SCHEMES_ENTRY   		"schemes"
#define ACTION_VERSION_ENTRY   		"version"

/* GConf description strings : */
#define ACTION_LABEL_DESC_SHORT		_("The label of the menu item")
#define ACTION_LABEL_DESC_LONG		_("The label of the menu item that will appear in the Nautilus popup menu when the selection matches the appearance condition settings")
#define ACTION_TOOLTIP_DESC_SHORT	_("The tooltip of the menu item")
#define ACTION_TOOLTIP_DESC_LONG		_("The tooltip of the menu item that will appear in the Nautilus statusbar when the user points to the Nautilus popup menu item with his/her mouse")
#define ACTION_ICON_DESC_SHORT	_("The icon of the menu item")
#define ACTION_ICON_DESC_LONG		_("The icon of the menu item that will appear next to the label in the Nautilus popup menu when the selection matches the appearance conditions settings")
#define ACTION_PROFILE_NAME_DESC_SHORT	_("A description name of the profile")
#define ACTION_PROFILE_NAME_DESC_LONG	_("The field is here to give the user a human readable name for a profile in the Nact interface. If not set there will be a default auto generated string set by default")
#define ACTION_PATH_DESC_SHORT		_("The path of the command")
#define ACTION_PATH_DESC_LONG			_("The path of the command to start when the user select the menu item in the Nautilus popup menu")
#define ACTION_PARAMS_DESC_SHORT		_("The parameters of the command")
#define ACTION_PARAMS_DESC_LONG		_("The parameters of the command to start when the user selects the menu item in the Nautilus popup menu.\n\nThe parameters can contain some special tokens which are replaced by Nautilus information before starting the command:\n\n%d: base folder of the selected file(s)\n%f: the name of the selected file or the first one if many are selected\n%m: space-separated list of the basenames of the selected file(s)/folder(s)\n%M: space-separated list of the selected file(s)/folder(s), with their full paths\n%u: GnomeVFS URI\n%s: scheme of the GnomeVFS URI\n%h: hostname of the GnomeVFS URI\n%U: username of the GnomeVFS URI\n%%: a percent sign")
#define ACTION_BASENAMES_DESC_SHORT	_("The list of pattern to match the selected file(s)/folder(s)")
#define ACTION_BASENAMES_DESC_LONG	_("A list of strings with joker '*' or '?' to match the name of the selected file(s)/folder(s). Each selected items must match at least one of the filename patterns for the action to appear")
#define ACTION_MATCHCASE_DESC_SHORT _("'true' if the filename patterns have to be case sensitive, 'false' otherwise")
#define ACTION_MATCHCASE_DESC_LONG	_("If you need to match a filename in a case-sensitive manner, set this key to 'true'. If you also want, for example '*.jpg' to match 'photo.JPG', set 'false'")
#define ACTION_MIMETYPES_DESC_SHORT	_("The list of patterns to match the mimetypes of the selected file(s)")
#define ACTION_MIMETYPES_DESC_LONG	_("A list of strings with joker '*' or '?' to match the mimetypes of the selected file(s). Each selected items must match at least one of the mimetype patterns for the action to appear")
#define ACTION_ISFILE_ISDIR_COMBINAITION_DESC_LONG N_("The valid combinations are:\n\nisfile=TRUE and isdir=FALSE: the selection may hold only files\nisfile=FALSE and isdir=TRUE: the selection may hold only folders\nisfile=TRUE and isdir=TRUE: the selection may hold both files and folders\nisfile=FALSE and isdir=FALSE: this is an invalid combination (your configuration will never appear)")
#define ACTION_ISFILE_DESC_SHORT		_("'true' if the selection can have files, 'false' otherwise")
/* i18n notes: The last space is important if your language add a space after a period sign "." because a string is concatenated after this string */
#define ACTION_ISFILE_DESC_LONG		N_("This setting is tied in with the 'isdir' setting. ") ACTION_ISFILE_ISDIR_COMBINAITION_DESC_LONG
#define ACTION_ISDIR_DESC_SHORT		_("'true' if the selection can have folders, 'false' otherwise")
/* i18n notes: The last space is important if your language add a space after a period sign "." because a string is concatenated after this string */
#define ACTION_ISDIR_DESC_LONG		N_("This setting is tied in with the 'isfile' setting. ") ACTION_ISFILE_ISDIR_COMBINAITION_DESC_LONG
#define ACTION_MULTIPLE_DESC_SHORT	_("'true' if the selection can have several items, 'false' otherwise")
#define ACTION_MULTIPLE_DESC_LONG	_("If you need one or more files or folders to be selected, set this key to 'true'. If you want just one file or folder, set 'false'")
#define ACTION_SCHEMES_DESC_SHORT	_("The list of GnomeVFS schemes where the selected files should be located")
#define ACTION_SCHEMES_DESC_LONG		_("Defines the list of valid GnomeVFS schemes to be matched against the selected items. The GnomeVFS scheme is the protocol used to access the files. The keyword to use is the one used in the GnomeVFS URI.\n\nExamples of GnomeVFS URI include: \nfile:///tmp/foo.txt\nsftp:///root@test.example.net/tmp/foo.txt\n\nThe most common schemes are:\n\n'file': local files\n'sftp': files accessed via SSH\n'ftp': files accessed via FTP\n'smb': files accessed via Samba (Windows share)\n'dav': files accessed via WebDav\n\nAll GnomeVFS schemes used by Nautilus can be used here.")
#define ACTION_VERSION_DESC_SHORT	_("The version of the configuration format")
#define ACTION_VERSION_DESC_LONG		_("The version of the configuration format that will be used to manage backward compatibility")

#endif /* __NAUTILUS_ACTIONS_CONFIG_GCONF_PRIVATE_H__ */
