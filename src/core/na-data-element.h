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

#ifndef __CORE_NA_DATA_ELEMENT_H__
#define __CORE_NA_DATA_ELEMENT_H__

/**
 * SECTION: na_data_element
 * @short_description: #NADataElement class definition.
 * @include: core/na-data-element.h
 *
 * The object which encapsulates an elementary data of #NAIDataFactory.
 * A #NADataElement object has a type and a value.
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define NA_DATA_ELEMENT_TYPE				( na_data_element_get_type())
#define NA_DATA_ELEMENT( object )			( G_TYPE_CHECK_INSTANCE_CAST( object, NA_DATA_ELEMENT_TYPE, NADataElement ))
#define NA_DATA_ELEMENT_CLASS( klass )		( G_TYPE_CHECK_CLASS_CAST( klass, NA_DATA_ELEMENT_TYPE, NADataElementClass ))
#define NA_IS_DATA_ELEMENT( object )		( G_TYPE_CHECK_INSTANCE_TYPE( object, NA_DATA_ELEMENT_TYPE ))
#define NA_IS_DATA_ELEMENT_CLASS( klass )	( G_TYPE_CHECK_CLASS_TYPE(( klass ), NA_DATA_ELEMENT_TYPE ))
#define NA_DATA_ELEMENT_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), NA_DATA_ELEMENT_TYPE, NADataElementClass ))

typedef struct NADataElementPrivate      NADataElementPrivate;

typedef struct {
	GObject               parent;
	NADataElementPrivate *private;
}
	NADataElement;

typedef struct NADataElementClassPrivate NADataElementClassPrivate;

typedef struct {
	GObjectClass               parent;
	NADataElementClassPrivate *private;
}
	NADataElementClass;

GType          na_data_element_get_type( void );

NADataElement *na_data_element_new( guint type );

void           na_data_element_dump( const NADataElement *element, const gchar *name );

void           na_data_element_set             ( NADataElement *element, const NADataElement *value );
/*void           na_data_element_set_from_boolean( NADataElement *element, gboolean value );*/
void           na_data_element_set_from_string ( NADataElement *element, const gchar *value );
/*void           na_data_element_set_from_slist  ( NADataElement *element, GSList *value );*/
void           na_data_element_set_from_value  ( NADataElement *element, const GValue *value );
void           na_data_element_set_from_void   ( NADataElement *element, const void *value );

void          *na_data_element_get             ( const NADataElement *element );
void           na_data_element_set_to_value    ( const NADataElement *element, GValue *value );

gboolean       na_data_element_are_equal       ( const NADataElement *a, const NADataElement *b );

G_END_DECLS

#endif /* __CORE_NA_DATA_ELEMENT_H__ */
