/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011, 2012 Pierre Wieser and others (see AUTHORS)
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

#ifndef __BASE_IUNIQUE_H__
#define __BASE_IUNIQUE_H__

/**
 * SECTION: base_iunique
 * @short_description: #BaseIUnique interface definition.
 * @include: nact/base-iunique.h
 *
 * This interface implements the features to make an application
 * unique, i.e. cjheck that we run only one instance of it.
 */

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BASE_IUNIQUE_TYPE                      ( base_iunique_get_type())
#define BASE_IUNIQUE( object )                 ( G_TYPE_CHECK_INSTANCE_CAST( object, BASE_IUNIQUE_TYPE, BaseIUnique ))
#define BASE_IS_IUNIQUE( object )              ( G_TYPE_CHECK_INSTANCE_TYPE( object, BASE_IUNIQUE_TYPE ))
#define BASE_IUNIQUE_GET_INTERFACE( instance ) ( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), BASE_IUNIQUE_TYPE, BaseIUniqueInterface ))

typedef struct _BaseIUnique                    BaseIUnique;
typedef struct _BaseIUniqueInterfacePrivate    BaseIUniqueInterfacePrivate;

typedef struct {
	/*< private >*/
	GTypeInterface               parent;
	BaseIUniqueInterfacePrivate *private;

	/*< public >*/
	/**
	 * get_application_name:
	 * @instance: this #NAIExporter instance.
	 *
	 * Returns: the 'display' name of the application.
	 */
	const gchar * ( *get_application_name )( const BaseIUnique *instance );
}
	BaseIUniqueInterface;

GType    base_iunique_get_type      ( void );

gboolean base_iunique_init_with_name( BaseIUnique *instance, const gchar *unique_app_name );

G_END_DECLS

#endif /* __BASE_IUNIQUE_H__ */
