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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>

#include <api/na-idata-factory.h>
#include <api/na-object-api.h>

#include "na-io-factory.h"
#include "na-data-factory.h"

/* private class data
 */
struct NAObjectMenuClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NAObjectMenuPrivate {
	gboolean dispose_has_run;
};

										/* i18n: default label for a new menu */
#define NEW_NAUTILUS_MENU				N_( "New Nautilus menu" )

extern NadfIdGroup menu_id_groups [];	/* defined in na-item-menu-enum.c */

static NAObjectItemClass *st_parent_class = NULL;

static GType    register_type( void );
static void     class_init( NAObjectMenuClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void     instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void     instance_dispose( GObject *object );
static void     instance_finalize( GObject *object );

static gboolean object_is_valid( const NAObject *object );

static void     idata_factory_iface_init( NAIDataFactoryInterface *iface );
static guint    idata_factory_get_version( const NAIDataFactory *instance );
static gchar   *idata_factory_get_default( const NAIDataFactory *instance, const NadfIdType *iddef );
static void     idata_factory_copy( NAIDataFactory *target, const NAIDataFactory *source );
static gboolean idata_factory_are_equal( const NAIDataFactory *a, const NAIDataFactory *b );
static gboolean idata_factory_is_valid( const NAIDataFactory *object );
static void     idata_factory_read_done( NAIDataFactory *instance, const NAIIOFactory *reader, void *reader_data, GSList **messages );
static void     idata_factory_write_done( NAIDataFactory *instance, const NAIIOFactory *writer, void *writer_data, GSList **messages );

static gboolean menu_is_valid( const NAObjectMenu *menu );
static gboolean is_valid_label( const NAObjectMenu *menu );

GType
na_object_menu_get_type( void )
{
	static GType menu_type = 0;

	if( menu_type == 0 ){

		menu_type = register_type();
	}

	return( menu_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "na_object_menu_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NAObjectMenuClass ),
		NULL,
		NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NAObjectMenu ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	static const GInterfaceInfo idata_factory_iface_info = {
		( GInterfaceInitFunc ) idata_factory_iface_init,
		NULL,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( NA_OBJECT_ITEM_TYPE, "NAObjectMenu", &info, 0 );

	g_type_add_interface_static( type, NA_IDATA_FACTORY_TYPE, &idata_factory_iface_info );

	na_io_factory_register( type, menu_id_groups );

	return( type );
}

static void
class_init( NAObjectMenuClass *klass )
{
	static const gchar *thisfn = "na_object_menu_class_init";
	GObjectClass *object_class;
	NAObjectClass *naobject_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->set_property = instance_set_property;
	object_class->get_property = instance_get_property;
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	naobject_class = NA_OBJECT_CLASS( klass );
	naobject_class->dump = NULL;
	naobject_class->copy = NULL;
	naobject_class->are_equal = NULL;
	naobject_class->is_valid = object_is_valid;

	klass->private = g_new0( NAObjectMenuClassPrivate, 1 );

	na_data_factory_properties( object_class );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "na_object_menu_instance_init";
	NAObjectMenu *self;

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	g_return_if_fail( NA_IS_OBJECT_MENU( instance ));

	self = NA_OBJECT_MENU( instance );

	self->private = g_new0( NAObjectMenuPrivate, 1 );

	na_data_factory_init( NA_IDATA_FACTORY( instance ));
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	g_return_if_fail( NA_IS_OBJECT_MENU( object ));
	g_return_if_fail( NA_IS_IDATA_FACTORY( object ));

	if( !NA_OBJECT_MENU( object )->private->dispose_has_run ){

		na_data_factory_set_value( NA_IDATA_FACTORY( object ), property_id, value, spec );
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	g_return_if_fail( NA_IS_OBJECT_MENU( object ));
	g_return_if_fail( NA_IS_IDATA_FACTORY( object ));

	if( !NA_OBJECT_MENU( object )->private->dispose_has_run ){

		na_data_factory_get_value( NA_IDATA_FACTORY( object ), property_id, value, spec );
	}
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "na_object_menu_instance_dispose";
	NAObjectMenu *self;

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	g_return_if_fail( NA_IS_OBJECT_MENU( object ));

	self = NA_OBJECT_MENU( object );

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
	static const gchar *thisfn = "na_object_menu_instance_finalize";
	NAObjectMenu *self;

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	g_return_if_fail( NA_IS_OBJECT_MENU( object ));

	self = NA_OBJECT_MENU( object );

	g_free( self->private );

	na_data_factory_finalize( NA_IDATA_FACTORY( object ));

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

static gboolean
object_is_valid( const NAObject *object )
{
	g_return_val_if_fail( NA_IS_OBJECT_MENU( object ), FALSE );

	return( menu_is_valid( NA_OBJECT_MENU( object )));
}

static void
idata_factory_iface_init( NAIDataFactoryInterface *iface )
{
	static const gchar *thisfn = "na_object_menu_idata_factory_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->get_version = idata_factory_get_version;
	iface->get_default = idata_factory_get_default;
	iface->copy = idata_factory_copy;
	iface->are_equal = idata_factory_are_equal;
	iface->is_valid = idata_factory_is_valid;
	iface->read_start = NULL;
	iface->read_done = idata_factory_read_done;
	iface->write_start = NULL;
	iface->write_done = idata_factory_write_done;
}

static guint
idata_factory_get_version( const NAIDataFactory *instance )
{
	return( 1 );
}

static gchar *
idata_factory_get_default( const NAIDataFactory *instance, const NadfIdType *iddef )
{
	gchar *value;

	value = NULL;

	switch( iddef->id ){

		case NADF_DATA_LABEL:
			value = g_strdup( NEW_NAUTILUS_MENU );
			break;
	}

	return( value );
}

static void
idata_factory_copy( NAIDataFactory *target, const NAIDataFactory *source )
{
	na_object_item_copy( NA_OBJECT_ITEM( target ), NA_OBJECT_ITEM( source ));
}

static gboolean
idata_factory_are_equal( const NAIDataFactory *a, const NAIDataFactory *b )
{
	return( na_object_item_are_equal( NA_OBJECT_ITEM( a ), NA_OBJECT_ITEM( b )));
}

static gboolean
idata_factory_is_valid( const NAIDataFactory *object )
{
	g_return_val_if_fail( NA_IS_OBJECT_MENU( object ), FALSE );

	return( menu_is_valid( NA_OBJECT_MENU( object )));
}

static void
idata_factory_read_done( NAIDataFactory *instance, const NAIIOFactory *reader, void *reader_data, GSList **messages )
{

}

static void
idata_factory_write_done( NAIDataFactory *instance, const NAIIOFactory *writer, void *writer_data, GSList **messages )
{

}

static gboolean
menu_is_valid( const NAObjectMenu *menu )
{
	gboolean is_valid;
	gint valid_subitems;
	GList *subitems, *ip;

	is_valid = FALSE;

	if( !menu->private->dispose_has_run ){

		is_valid = TRUE;

		if( is_valid ){
			is_valid = is_valid_label( menu );
		}

		if( is_valid ){
			valid_subitems = 0;
			subitems = na_object_get_items( menu );
			for( ip = subitems ; ip && !valid_subitems ; ip = ip->next ){
				if( na_object_is_valid( ip->data )){
					valid_subitems += 1;
				}
			}
			is_valid = ( valid_subitems > 0 );
			if( !is_valid ){
				na_object_debug_invalid( menu, "no valid subitem" );
			}
		}
	}

	return( is_valid );
}

static gboolean
is_valid_label( const NAObjectMenu *menu )
{
	gboolean is_valid;
	gchar *label;

	label = na_object_get_label( menu );
	is_valid = ( label && g_utf8_strlen( label, -1 ) > 0 );
	g_free( label );

	if( !is_valid ){
		na_object_debug_invalid( menu, "label" );
	}

	return( is_valid );
}

/**
 * na_object_menu_new:
 *
 * Allocates a new #NAObjectMenu object.
 *
 * The new #NAObjectMenu object is initialized with suitable default values,
 * but without any profile.
 *
 * Returns: the newly allocated #NAObjectMenu object.
 */
NAObjectMenu *
na_object_menu_new( void )
{
	NAObjectMenu *menu;

	menu = g_object_new( NA_OBJECT_MENU_TYPE, NULL );

	return( menu );
}
