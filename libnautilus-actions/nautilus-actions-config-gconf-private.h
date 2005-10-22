/* Nautilus Actions configuration tool
 * Copyright (C) 2005 The GNOME Foundation
 *
 * Authors:
 *  Frederic Ruaudel (grumz@grumz.net)
 *	 Rodrigo Moya (rodrigo@gnome-db.org)
 *
 * This Program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this Library; see the file COPYING.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

// GConf general information
#define ACTIONS_SCHEMA_PREFIX  "/schemas"
#define ACTIONS_CONFIG_DIR     NAUTILUS_ACTIONS_CONFIG_GCONF_BASEDIR "/configurations"
#define ACTIONS_SCHEMA_OWNER	 "nautilus-actions"

// GConf key names :
#define ACTION_LABEL_ENTRY     "label"
#define ACTION_TOOLTIP_ENTRY   "tooltip"
#define ACTION_ICON_ENTRY      "icon"
#define ACTION_PATH_ENTRY      "path"
#define ACTION_PARAMS_ENTRY    "parameters"
#define ACTION_BASENAMES_ENTRY "basenames"
#define ACTION_ISFILE_ENTRY    "isfile"
#define ACTION_ISDIR_ENTRY     "isdir"
#define ACTION_MULTIPLE_ENTRY  "accept-multiple-files"
#define ACTION_SCHEMES_ENTRY   "schemes"
#define ACTION_VERSION_ENTRY   "version"

// GConf description strings :
#define ACTION_LABEL_DESC_SHORT		"The label of the menu item"
#define ACTION_LABEL_DESC_LONG		"The label of the menu item that will appear in the Nautilus contextual menu when the selection match the test options"
#define ACTION_TOOLTIP_DESC_SHORT	"The tooltip of the menu item"
#define ACTION_TOOLTIP_DESC_LONG		"The tooltip of the menu item that will appear in the Nautilus statusbar when the user passes over the contextual menu item with his mouse"
#define ACTION_ICON_DESC_SHORT	"The icon of the menu item"
#define ACTION_ICON_DESC_LONG		"The icon of the menu item that will appear next to the label in the Nautilus contextual menu when the selection match the test options"
#define ACTION_PATH_DESC_SHORT		"The path of the command"
#define ACTION_PATH_DESC_LONG			"The path of the command to start when the user select the menu item in the Nautilus contextual menu"
#define ACTION_PARAMS_DESC_SHORT		"The parameters of the command"
#define ACTION_PARAMS_DESC_LONG		"The parameters of the command to start when the user select the menu item in the Nautilus contextual menu.\n\nThe parameters can contains some special token which is replaced by nautilus informations before starting the command :\n\n%p : parent directory of the selected file(s)\n%d : base dir of the selected file(s)\n%f : the name of the selected file or the 1st one if many are selected\n%m : list of the basename of the selected files/directories separated\n%M : list of the selected files/directories with their full path\n%u : gnome-vfs URI\n%s : scheme of the gnome-vfs URI\n%h : hostname of the gnome-vfs URI\n%U : username of the gnome-vfs URI\n%% : a percent sign"
#define ACTION_BASENAMES_DESC_SHORT	"The list of pattern to match selected files"
#define ACTION_BASENAMES_DESC_LONG	"A list of string with joker * or ? to match the selected files. Each selected files schould match at least one of the pattern to be valid."
#define ACTION_ISFILE_DESC_SHORT		"True if the selection must have files, false otherwise"
#define ACTION_ISFILE_DESC_LONG		"This options is tied with isdir options. Here is the valid combination :\n\n- isfile is true and isdir is false : selection holds only files\n- isfile is false and isdir is true : selection holds only folders\n- isfile is true and isdir is true : selection holds both files and folders\n- isfile is false and isdir is false : invalid combination"
#define ACTION_ISDIR_DESC_SHORT		"True if the selection must have folders, false otherwise"
#define ACTION_ISDIR_DESC_LONG		"This options is tied with isfile options. Here is the valid combination :\n\n- isfile is true and isdir is false : selection holds only files\n- isfile is false and isdir is true : selection holds only folders\n- isfile is true and isdir is true : selection holds both files and folders\n- isfile is false and isdir is false : invalid combination"
#define ACTION_MULTIPLE_DESC_SHORT	"True if the selection must have several items, false otherwise"
#define ACTION_MULTIPLE_DESC_LONG	"If you want to activate this config if several files or folders are selected, set 'true' to this key. If you want just one file or folder, set 'false'"
#define ACTION_SCHEMES_DESC_SHORT	"The list of GnomeVFS schemes where the selected files should be located"
#define ACTION_SCHEMES_DESC_LONG		"Define the list of valid GnomeVFS scheme to match the selected items. The GnomeVFS scheme is the protocol used to access the files. The keyword to use is the one use in GnomeVFS URI.\n\nexample of GnomeVFS URI : \nfile:///tmp/foo.txt\nsftp:///root@test.example.net/tmp/foo.txt\n\nThe most common schemes are :\n\nfile : local files\nsftp : files accessed via SSH\nftp : files accessed via FTP\nsmb : files accessed via Samba (Windows share)\ndav : files accessed via WebDav\n\nAll valid GnomeVFS scheme can be used here."
#define ACTION_VERSION_DESC_SHORT	"The version of the configuration format"
#define ACTION_VERSION_DESC_LONG		"The version of the configuration format that will be used to manage backward compatibility"
