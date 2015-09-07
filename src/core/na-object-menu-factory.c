/*
 * FileManager-Actions
 * A file-manager extension which offers configurable context menu actions.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <api/fma-ifactory-object-data.h>
#include <api/fma-data-def.h>
#include <api/fma-data-types.h>

extern FMADataDef data_def_id [];			/* defined in na-object-id-factory.c */
extern FMADataDef data_def_item [];			/* defined in na-object-item-factory.c */
extern FMADataDef data_def_conditions [];	/* defined in fma-icontext-factory.c */

/*
 * As of 3.2 non copyables data are:
 * - n/a
 */

static FMADataDef data_def_menu [] = {
	{ NULL },
};

FMADataGroup menu_data_groups [] = {
	{ NA_FACTORY_OBJECT_ID_GROUP,         data_def_id },
	{ NA_FACTORY_OBJECT_ITEM_GROUP,       data_def_item },
	{ NA_FACTORY_OBJECT_MENU_GROUP,       data_def_menu },
	{ NA_FACTORY_OBJECT_CONDITIONS_GROUP, data_def_conditions },
	{ NULL }
};
