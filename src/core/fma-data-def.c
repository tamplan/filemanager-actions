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

#include <string.h>

#include <api/fma-data-def.h>

/**
 * fma_data_def_get_data_def:
 * @group: a #FMADataGroup structure array.
 * @group_name: the searched group name.
 * @name: the searched data name.
 *
 * Returns: a pointer to the #FMADataDef structure, or %NULL if not found.
 *
 * Since: 2.30
 */
const FMADataDef *
fma_data_def_get_data_def( const FMADataGroup *group, const gchar *group_name, const gchar *name )
{
	FMADataGroup *igroup;
	FMADataDef *idef;

	igroup = ( FMADataGroup * ) group;
	while( igroup->group ){
		if( !strcmp( igroup->group, group_name )){
			idef = igroup->def;
			while( idef->name ){
				if( !strcmp( idef->name, name )){
					return( idef );
				}
				idef++;
			}
		}
		igroup++;
	}

	return( NULL );
}
