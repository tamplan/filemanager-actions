/*
 * Nautilus ObjectItems
 * A Nautilus extension which offers configurable context menu object_items.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include "na-object-item.h"
#include "na-utils.h"

/* private class data
 */
struct NAObjectItemClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NAObjectItemPrivate {
	gboolean dispose_has_run;

	/* object_item properties
	 */
	gchar   *tooltip;
	gchar   *icon;
};

#define PROP_NAOBJECT_ITEM_TOOLTIP_STR		"na-object-item-tooltip"
#define PROP_NAOBJECT_ITEM_ICON_STR			"na-object-item-icon"

static NAObjectClass *st_parent_class = NULL;

static GType     register_type( void );
static void      class_init( NAObjectItemClass *klass );
static void      instance_init( GTypeInstance *instance, gpointer klass );
static void      instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void      instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void      instance_dispose( GObject *object );
static void      instance_finalize( GObject *object );

static void      object_copy( NAObject *target, const NAObject *source );
static gboolean  object_are_equal( const NAObject *a, const NAObject *b );
static gboolean  object_is_valid( const NAObject *object_item );
static void      object_dump( const NAObject *object_item );

GType
na_object_item_get_type( void )
{
	static GType object_type = 0;

	if( !object_type ){
		object_type = register_type();
	}

	return( object_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "na_object_item_register_type";

	static GTypeInfo info = {
		sizeof( NAObjectItemClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NAObjectItem ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	return( g_type_register_static( NA_OBJECT_TYPE, "NAObjectItem", &info, 0 ));
}

static void
class_init( NAObjectItemClass *klass )
{
	static const gchar *thisfn = "na_object_item_class_init";
	GObjectClass *object_class;
	GParamSpec *spec;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
	object_class->set_property = instance_set_property;
	object_class->get_property = instance_get_property;

	spec = g_param_spec_string(
			PROP_NAOBJECT_ITEM_TOOLTIP_STR,
			"Item tooltip",
			"Context menu tooltip of the item", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_NAOBJECT_ITEM_TOOLTIP, spec );

	spec = g_param_spec_string(
			PROP_NAOBJECT_ITEM_ICON_STR,
			"Icon name",
			"Context menu displayable icon for the item", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_NAOBJECT_ITEM_ICON, spec );

	klass->private = g_new0( NAObjectItemClassPrivate, 1 );

	NA_OBJECT_CLASS( klass )->new = NULL;
	NA_OBJECT_CLASS( klass )->copy = object_copy;
	NA_OBJECT_CLASS( klass )->are_equal = object_are_equal;
	NA_OBJECT_CLASS( klass )->is_valid = object_is_valid;
	NA_OBJECT_CLASS( klass )->dump = object_dump;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	/*static const gchar *thisfn = "na_object_item_instance_init";*/
	NAObjectItem *self;

	/*g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );*/
	g_assert( NA_IS_OBJECT_ITEM( instance ));
	self = NA_OBJECT_ITEM( instance );

	self->private = g_new0( NAObjectItemPrivate, 1 );

	self->private->dispose_has_run = FALSE;

	/* initialize suitable default values
	 */
	self->private->tooltip = g_strdup( "" );
	self->private->icon = g_strdup( "" );
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	NAObjectItem *self;

	g_assert( NA_IS_OBJECT_ITEM( object ));
	self = NA_OBJECT_ITEM( object );

	switch( property_id ){
		case PROP_NAOBJECT_ITEM_TOOLTIP:
			g_value_set_string( value, self->private->tooltip );
			break;

		case PROP_NAOBJECT_ITEM_ICON:
			g_value_set_string( value, self->private->icon );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	NAObjectItem *self;

	g_assert( NA_IS_OBJECT_ITEM( object ));
	self = NA_OBJECT_ITEM( object );

	switch( property_id ){
		case PROP_NAOBJECT_ITEM_TOOLTIP:
			g_free( self->private->tooltip );
			self->private->tooltip = g_value_dup_string( value );
			break;

		case PROP_NAOBJECT_ITEM_ICON:
			g_free( self->private->icon );
			self->private->icon = g_value_dup_string( value );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
			break;
	}
}

static void
instance_dispose( GObject *object )
{
	/*static const gchar *thisfn = "na_object_item_instance_dispose";*/
	NAObjectItem *self;

	/*g_debug( "%s: object=%p", thisfn, ( void * ) object );*/
	g_assert( NA_IS_OBJECT_ITEM( object ));
	self = NA_OBJECT_ITEM( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	/*static const gchar *thisfn = "na_object_item_instance_finalize";*/
	NAObjectItem *self;

	/*g_debug( "%s: object=%p", thisfn, ( void * ) object );*/
	g_assert( NA_IS_OBJECT_ITEM( object ));
	self = NA_OBJECT_ITEM( object );

	g_free( self->private->tooltip );
	g_free( self->private->icon );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * na_object_item_get_tooltip:
 * @item: the #NAObjectItem object to be requested.
 *
 * Returns the tooltip which will be display in the Nautilus context
 * menu item for this @item.
 *
 * Returns: the tooltip of the @item as a newly allocated string. This
 * returned string must be g_free() by the caller.
 */
gchar *
na_object_item_get_tooltip( const NAObjectItem *item )
{
	gchar *tooltip;

	g_assert( NA_IS_OBJECT_ITEM( item ));

	g_object_get( G_OBJECT( item ), PROP_NAOBJECT_ITEM_TOOLTIP_STR, &tooltip, NULL );

	return( tooltip );
}

/**
 * na_object_item_get_icon:
 * @item: the #NAObjectItem object to be requested.
 *
 * Returns the name of the icon attached to the Nautilus context menu
 * item for this @item.
 *
 * Returns: the icon name as a newly allocated string. This returned
 * string must be g_free() by the caller.
 */
gchar *
na_object_item_get_icon( const NAObjectItem *item )
{
	gchar *icon;

	g_assert( NA_IS_OBJECT_ITEM( item ));

	g_object_get( G_OBJECT( item ), PROP_NAOBJECT_ITEM_ICON_STR, &icon, NULL );

	return( icon );
}

/*
 * TODO: remove this function
 */
gchar *
na_object_item_get_verified_icon_name( const NAObjectItem *item )
{
	gchar *icon_name;

	g_assert( NA_IS_OBJECT_ITEM( item ));

	g_object_get( G_OBJECT( item ), PROP_NAOBJECT_ITEM_ICON_STR, &icon_name, NULL );

	if( icon_name[0] == '/' ){
		if( !g_file_test( icon_name, G_FILE_TEST_IS_REGULAR )){
			g_free( icon_name );
			return NULL;
		}
	} else if( strlen( icon_name ) == 0 ){
		g_free( icon_name );
		return NULL;
	}

	return( icon_name );
}

/**
 * na_object_item_set_tooltip:
 * @item: the #NAObjectItem object to be updated.
 * @tooltip: the tooltip to be set.
 *
 * Sets a new tooltip for the @item. Tooltip will be displayed by
 * Nautilus when the user move its mouse over the Nautilus context menu
 * item.
 *
 * #NAObjectItem takes a copy of the provided tooltip. This later may
 * so be g_free() by the caller after this function returns.
 */
void
na_object_item_set_tooltip( NAObjectItem *item, const gchar *tooltip )
{
	g_assert( NA_IS_OBJECT_ITEM( item ));

	g_object_set( G_OBJECT( item ), PROP_NAOBJECT_ITEM_TOOLTIP_STR, tooltip, NULL );
}

/**
 * na_object_item_set_icon:
 * @item: the #NAObjectItem object to be updated.
 * @icon: the icon name to be set.
 *
 * Sets a new icon name for the @item.
 *
 * #NAObjectItem takes a copy of the provided icon name. This later may
 * so be g_free() by the caller after this function returns.
 */
void
na_object_item_set_icon( NAObjectItem *item, const gchar *icon )
{
	g_assert( NA_IS_OBJECT_ITEM( item ));

	g_object_set( G_OBJECT( item ), PROP_NAOBJECT_ITEM_ICON_STR, icon, NULL );
}

void
object_copy( NAObject *target, const NAObject *source )
{
	gchar *tooltip, *icon;

	if( st_parent_class->copy ){
		st_parent_class->copy( target, source );
	}

	g_assert( NA_IS_OBJECT_ITEM( source ));
	g_assert( NA_IS_OBJECT_ITEM( target ));

	g_object_get( G_OBJECT( source ),
			PROP_NAOBJECT_ITEM_TOOLTIP_STR, &tooltip,
			PROP_NAOBJECT_ITEM_ICON_STR, &icon,
			NULL );

	g_object_set( G_OBJECT( target ),
			PROP_NAOBJECT_ITEM_TOOLTIP_STR, tooltip,
			PROP_NAOBJECT_ITEM_ICON_STR, icon,
			NULL );

	g_free( tooltip );
	g_free( icon );
}

static gboolean
object_are_equal( const NAObject *a, const NAObject *b )
{
	NAObjectItem *first, *second;
	gboolean equal = TRUE;

	if( equal ){
		if( st_parent_class->are_equal ){
			equal = st_parent_class->are_equal( a, b );
		}
	}

	if( equal ){
		g_assert( NA_IS_OBJECT_ITEM( a ));
		first = NA_OBJECT_ITEM( a );

		g_assert( NA_IS_OBJECT_ITEM( b ));
		second = NA_OBJECT_ITEM( b );

		equal =
			( g_utf8_collate( first->private->tooltip, second->private->tooltip ) == 0 ) &&
			( g_utf8_collate( first->private->icon, second->private->icon ) == 0 );
	}

	return( equal );
}

gboolean
object_is_valid( const NAObject *item )
{
	gboolean is_valid = TRUE;

	if( is_valid ){
		if( st_parent_class->is_valid ){
			is_valid = st_parent_class->is_valid( item );
		}
	}

	return( is_valid );
}

static void
object_dump( const NAObject *item )
{
	static const gchar *thisfn = "na_object_item_object_dump";
	NAObjectItem *self;

	if( st_parent_class->dump ){
		st_parent_class->dump( item );
	}

	g_assert( NA_IS_OBJECT_ITEM( item ));
	self = NA_OBJECT_ITEM( item );

	g_debug( "%s: tooltip='%s'", thisfn, self->private->tooltip );
	g_debug( "%s:    icon='%s'", thisfn, self->private->icon );
}
