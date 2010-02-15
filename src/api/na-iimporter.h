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

#ifndef __NAUTILUS_ACTIONS_API_NA_IIMPORTER_H__
#define __NAUTILUS_ACTIONS_API_NA_IIMPORTER_H__

/**
 * SECTION: na_iimporter
 * @short_description: #NAIImporter interface definition.
 * @include: nautilus-actions/na-iimporter.h
 *
 * The #NAIImporter interface imports  items from the outside world.
 *
 * Nautilus-Actions v 2.30 - API version:  1
 */

#include "na-object-item.h"

G_BEGIN_DECLS

#define NA_IIMPORTER_TYPE						( na_iimporter_get_type())
#define NA_IIMPORTER( instance )				( G_TYPE_CHECK_INSTANCE_CAST( instance, NA_IIMPORTER_TYPE, NAIImporter ))
#define NA_IS_IIMPORTER( instance )				( G_TYPE_CHECK_INSTANCE_TYPE( instance, NA_IIMPORTER_TYPE ))
#define NA_IIMPORTER_GET_INTERFACE( instance )	( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NA_IIMPORTER_TYPE, NAIImporterInterface ))

typedef struct NAIImporter                 NAIImporter;

typedef struct NAIImporterInterfacePrivate NAIImporterInterfacePrivate;

typedef struct {
	GTypeInterface               parent;
	NAIImporterInterfacePrivate *private;

	/**
	 * get_version:
	 * @instance: the #NAIImporter provider.
	 *
	 * Returns: the version of this interface supported by the I/O provider.
	 *
	 * Defaults to 1.
	 */
	guint          ( *get_version )( const NAIImporter *instance );
}
	NAIImporterInterface;

GType na_iimporter_get_type( void );

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_API_NA_IIMPORTER_H__ */
