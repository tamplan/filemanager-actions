/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010 Pierre Wieser and others (see AUTHORS)
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

#ifndef __NAUTILUS_ACTIONS_API_NA_IFACTORY_OBJECT_ENUM_H__
#define __NAUTILUS_ACTIONS_API_NA_IFACTORY_OBJECT_ENUM_H__

#include <glib.h>

/**
 * SECTION: na_ifactory_object
 * @short_description: Enumeration of all serializable elementary datas.
 * @include: nautilus-actions/na-ifactory-object-enum.h
 */

G_BEGIN_DECLS

/*
 * IMPORTANT NOTE
 * Please note that this enumeration may  be compiled in by extensions.
 * They must so remain fixed, unless you want see strange effects (e.g.
 * an extension has been compiled with NADF_TYPE_STRING = 2, while you
 * have inserted another element, making it to 3 !) - or you know what
 * you are doing...
 */

enum {
	NA_FACTORY_OBJECT_ID_GROUP = 1,
	NADF_DATA_ID,
	NADF_DATA_LABEL,
	NADF_DATA_PARENT,

	NA_FACTORY_OBJECT_ITEM_GROUP = 20,
	NADF_DATA_TOOLTIP,
	NADF_DATA_ICON,
	NADF_DATA_DESCRIPTION,
	NADF_DATA_SUBITEMS,
	NADF_DATA_SUBITEMS_SLIST,
	NADF_DATA_ENABLED,
	NADF_DATA_READONLY,
	NADF_DATA_PROVIDER,
	NADF_DATA_PROVIDER_DATA,

	NA_FACTORY_OBJECT_ACTION_GROUP = 40,
	NADF_DATA_VERSION,
	NADF_DATA_TARGET_SELECTION,
	NADF_DATA_TARGET_BACKGROUND,
	NADF_DATA_TARGET_TOOLBAR,
	NADF_DATA_TOOLBAR_LABEL,
	NADF_DATA_TOOLBAR_SAME_LABEL,
	NADF_DATA_LAST_ALLOCATED,

	NA_FACTORY_OBJECT_MENU_GROUP = 60,

	NA_FACTORY_OBJECT_PROFILE_GROUP = 80,
	NADF_DATA_PATH,
	NADF_DATA_PARAMETERS,
	NADF_DATA_BASENAMES,
	NADF_DATA_MATCHCASE,
	NADF_DATA_MIMETYPES,
	NADF_DATA_ISFILE,
	NADF_DATA_ISDIR,
	NADF_DATA_MULTIPLE,
	NADF_DATA_SCHEMES,
	NADF_DATA_FOLDERS,

	NA_FACTORY_OBJECT_CONDITIONS_GROUP = 100,
};

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_API_NA_IFACTORY_OBJECT_ENUM_H__ */
