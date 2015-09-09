/*
 * FileManager-Actions
 * A file-manager extension which offers configurable context menu pivots.
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

#ifndef __FILEMANAGER_ACTIONS_API_FM_DEFINES_H__
#define __FILEMANAGER_ACTIONS_API_FM_DEFINES_H__

/* @title: FMDefines
 * @short_description: Definitions suitable for file-managers
 * @include: filemanager-actions/fma-fm-defines.h
 */

#if FMA_TARGET_ID == NAUTILUS_ID
#include <libnautilus-extension/nautilus-extension-types.h>
#include <libnautilus-extension/nautilus-menu-provider.h>
#include <libnautilus-extension/nautilus-file-info.h>
#elif FMA_TARGET_ID == NEMO_ID
#include <libnemo-extension/nemo-extension-types.h>
#include <libnemo-extension/nemo-menu-provider.h>
#include <libnemo-extension/nemo-file-info.h>
#endif

G_BEGIN_DECLS

#if FMA_TARGET_ID == NAUTILUS_ID
#define FILE_MANAGER_TYPE_MENU_PROVIDER                       NAUTILUS_TYPE_MENU_PROVIDER
#define FILE_MANAGER_MENU_PROVIDER                            NAUTILUS_MENU_PROVIDER
#define FILE_MANAGER_IS_MENU                                  NAUTILUS_IS_MENU
#define FILE_MANAGER_MENU_ITEM                                NAUTILUS_MENU_ITEM
#define FILE_MANAGER_FILE_INFO                                NAUTILUS_FILE_INFO
#define FileManagerMenuProviderIface                          NautilusMenuProviderIface
#define FileManagerMenuProvider                               NautilusMenuProvider
#define FileManagerMenuItem                                   NautilusMenuItem
#define FileManagerMenu                                       NautilusMenu
#define FileManagerFileInfo                                   NautilusFileInfo
#define file_manager_menu_new                                 nautilus_menu_new
#define file_manager_menu_append_item                         nautilus_menu_append_item
#define file_manager_menu_item_new                            nautilus_menu_item_new
#define file_manager_menu_item_set_submenu                    nautilus_menu_item_set_submenu
#define file_manager_menu_item_list_free                      nautilus_menu_item_list_free
#define file_manager_file_info_get_uri                        nautilus_file_info_get_uri
#define file_manager_file_info_get_mime_type                  nautilus_file_info_get_mime_type
#define file_manager_menu_provider_emit_items_updated_signal  nautilus_menu_provider_emit_items_updated_signal
#elif FMA_TARGET_ID == NEMO_ID
#define FILE_MANAGER_TYPE_MENU_PROVIDER                       NEMO_TYPE_MENU_PROVIDER
#define FILE_MANAGER_MENU_PROVIDER                            NEMO_MENU_PROVIDER
#define FILE_MANAGER_IS_MENU                                  NEMO_IS_MENU
#define FILE_MANAGER_MENU_ITEM                                NEMO_MENU_ITEM
#define FILE_MANAGER_FILE_INFO                                NEMO_FILE_INFO
#define FileManagerMenuProviderIface                          NemoMenuProviderIface
#define FileManagerMenuProvider                               NemoMenuProvider
#define FileManagerMenuItem                                   NemoMenuItem
#define FileManagerMenu                                       NemoMenu
#define FileManagerFileInfo                                   NemoFileInfo
#define file_manager_menu_new                                 nemo_menu_new
#define file_manager_menu_append_item                         nemo_menu_append_item
#define file_manager_menu_item_new                            nemo_menu_item_new
#define file_manager_menu_item_set_submenu                    nemo_menu_item_set_submenu
#define file_manager_menu_item_list_free                      nemo_menu_item_list_free
#define file_manager_file_info_get_uri                        nemo_file_info_get_uri
#define file_manager_file_info_get_mime_type                  nemo_file_info_get_mime_type
#define file_manager_menu_provider_emit_items_updated_signal  nemo_menu_provider_emit_items_updated_signal
#endif

G_END_DECLS

#endif /* __FILEMANAGER_ACTIONS_API_FM_DEFINES_H__ */
