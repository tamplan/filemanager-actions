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
#include <uuid/uuid.h>

#include "na-iduplicable.h"
#include "na-object-api.h"
#include "na-object-item-class.h"
#include "na-object-item-fn.h"
#include "na-utils.h"

/* private class data
 */
struct NAObjectItemClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NAObjectItemPrivate {
	gboolean       dispose_has_run;

	/* object_item properties
	 */
	gchar         *tooltip;
	gchar         *icon;
	gboolean       enabled;

	/* list of NAObjectId subitems
	 */
	GList         *items;

	/* the original provider
	 * required to be able to edit/delete the item
	 */
	NAIIOProvider *provider;
};

/* object properties
 */
enum {
	NAOBJECT_ITEM_PROP_TOOLTIP_ID = 1,
	NAOBJECT_ITEM_PROP_ICON_ID,
	NAOBJECT_ITEM_PROP_ENABLED_ID,
	NAOBJECT_ITEM_PROP_PROVIDER_ID,
	NAOBJECT_ITEM_PROP_ITEMS_ID
};

#define NAOBJECT_ITEM_PROP_TOOLTIP		"na-object-item-tooltip"
#define NAOBJECT_ITEM_PROP_ICON			"na-object-item-icon"
#define NAOBJECT_ITEM_PROP_ENABLED		"na-object-item-enabled"
#define NAOBJECT_ITEM_PROP_PROVIDER		"na-object-item-provider"
#define NAOBJECT_ITEM_PROP_ITEMS		"na-object-item-items"

static NAObjectIdClass *st_parent_class = NULL;

static GType    register_type( void );
static void     class_init( NAObjectItemClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void     instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void     instance_dispose( GObject *object );
static void     instance_finalize( GObject *object );

static void     object_dump( const NAObject *object );
static void     object_ref( const NAObject *action );
static void     object_copy( NAObject *target, const NAObject *source );
static gboolean object_are_equal( const NAObject *a, const NAObject *b );
static gboolean object_is_valid( const NAObject *object );
static GList   *object_get_childs( const NAObject *object );

static gchar   *object_id_new_id( const NAObjectId *object );

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

	return( g_type_register_static( NA_OBJECT_ID_TYPE, "NAObjectItem", &info, 0 ));
}

static void
class_init( NAObjectItemClass *klass )
{
	static const gchar *thisfn = "na_object_item_class_init";
	GObjectClass *object_class;
	NAObjectClass *naobject_class;
	NAObjectIdClass *objectid_class;
	GParamSpec *spec;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
	object_class->set_property = instance_set_property;
	object_class->get_property = instance_get_property;

	spec = g_param_spec_string(
			NAOBJECT_ITEM_PROP_TOOLTIP,
			"Item tooltip",
			"Context menu tooltip of the item", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, NAOBJECT_ITEM_PROP_TOOLTIP_ID, spec );

	spec = g_param_spec_string(
			NAOBJECT_ITEM_PROP_ICON,
			"Icon name",
			"Context menu displayable icon for the item", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, NAOBJECT_ITEM_PROP_ICON_ID, spec );

	spec = g_param_spec_boolean(
			NAOBJECT_ITEM_PROP_ENABLED,
			"Enabled",
			"Whether this item, and recursively its subitems, is/are enabled", TRUE,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, NAOBJECT_ITEM_PROP_ENABLED_ID, spec );

	spec = g_param_spec_pointer(
			NAOBJECT_ITEM_PROP_PROVIDER,
			"Original provider",
			"Original provider of the NAObjectItem",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, NAOBJECT_ITEM_PROP_PROVIDER_ID, spec );

	klass->private = g_new0( NAObjectItemClassPrivate, 1 );

	naobject_class = NA_OBJECT_CLASS( klass );
	naobject_class->dump = object_dump;
	naobject_class->get_clipboard_id = NULL;
	naobject_class->ref = object_ref;
	naobject_class->new = NULL;
	naobject_class->copy = object_copy;
	naobject_class->are_equal = object_are_equal;
	naobject_class->is_valid = object_is_valid;
	naobject_class->get_childs = object_get_childs;

	objectid_class = NA_OBJECT_ID_CLASS( klass );
	objectid_class->new_id = object_id_new_id;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	/*static const gchar *thisfn = "na_object_item_instance_init";*/
	NAObjectItem *self;

	/*g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );*/
	g_return_if_fail( NA_IS_OBJECT_ITEM( instance ));
	self = NA_OBJECT_ITEM( instance );

	self->private = g_new0( NAObjectItemPrivate, 1 );

	self->private->dispose_has_run = FALSE;

	/* initialize suitable default values
	 */
	self->private->tooltip = g_strdup( "" );
	self->private->icon = g_strdup( "" );
	self->private->enabled = TRUE;
	self->private->provider = NULL;
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	NAObjectItem *self;

	g_return_if_fail( NA_IS_OBJECT_ITEM( object ));
	self = NA_OBJECT_ITEM( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case NAOBJECT_ITEM_PROP_TOOLTIP_ID:
				g_value_set_string( value, self->private->tooltip );
				break;

			case NAOBJECT_ITEM_PROP_ICON_ID:
				g_value_set_string( value, self->private->icon );
				break;

			case NAOBJECT_ITEM_PROP_ENABLED_ID:
				g_value_set_boolean( value, self->private->enabled );
				break;

			case NAOBJECT_ITEM_PROP_PROVIDER_ID:
				g_value_set_pointer( value, self->private->provider );
				break;

			default:
				G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
				break;
		}
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	NAObjectItem *self;

	g_return_if_fail( NA_IS_OBJECT_ITEM( object ));
	self = NA_OBJECT_ITEM( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case NAOBJECT_ITEM_PROP_TOOLTIP_ID:
				g_free( self->private->tooltip );
				self->private->tooltip = g_value_dup_string( value );
				break;

			case NAOBJECT_ITEM_PROP_ICON_ID:
				g_free( self->private->icon );
				self->private->icon = g_value_dup_string( value );
				break;

			case NAOBJECT_ITEM_PROP_ENABLED_ID:
				self->private->enabled = g_value_get_boolean( value );
				break;

			case NAOBJECT_ITEM_PROP_PROVIDER_ID:
				self->private->provider = g_value_get_pointer( value );
				break;

			default:
				G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
				break;
		}
	}
}

static void
instance_dispose( GObject *object )
{
	/*static const gchar *thisfn = "na_object_item_instance_dispose";*/
	NAObjectItem *self;

	/*g_debug( "%s: object=%p", thisfn, ( void * ) object );*/
	g_return_if_fail( NA_IS_OBJECT_ITEM( object ));
	self = NA_OBJECT_ITEM( object );

	if( !self->private->dispose_has_run ){

		na_object_item_free_items( self->private->items );

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
	g_return_if_fail( NA_IS_OBJECT_ITEM( object ));
	self = NA_OBJECT_ITEM( object );

	g_free( self->private->tooltip );
	g_free( self->private->icon );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/*
 * TODO: remove this function
 */
gchar *
na_object_item_get_verified_icon_name( const NAObjectItem *item )
{
	gchar *icon_name;

	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), NULL );
	g_return_val_if_fail( !item->private->dispose_has_run, NULL );

	g_object_get( G_OBJECT( item ), NAOBJECT_ITEM_PROP_ICON, &icon_name, NULL );

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
 * na_object_item_get_pixbuf:
 * @item: this #NAObjectItem.
 * @widget: the widget for which the icon must be rendered.
 *
 * Returns the #GdkPixbuf image corresponding to the icon.
 * The image has a size of %GTK_ICON_SIZE_MENU.
 */
GdkPixbuf *na_object_item_get_pixbuf( const NAObjectItem *item, GtkWidget *widget )
{
	static const gchar *thisfn = "na_object_item_get_pixbuf";
	gchar *iconname;
	GtkStockItem stock_item;
	GdkPixbuf* icon = NULL;
	gint width, height;
	GError* error = NULL;

	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), NULL );

	if( !item->private->dispose_has_run ){

		iconname = na_object_item_get_icon( item );

		/* TODO: use the same algorythm than Nautilus to find and
		 * display an icon + move the code to NAAction class +
		 * remove na_action_get_verified_icon_name
		 */
		if( iconname ){
			if( gtk_stock_lookup( iconname, &stock_item )){
				icon = gtk_widget_render_icon( widget, iconname, GTK_ICON_SIZE_MENU, NULL );

			} else if( g_file_test( iconname, G_FILE_TEST_EXISTS )
				   && g_file_test( iconname, G_FILE_TEST_IS_REGULAR )){

				gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &width, &height);
				icon = gdk_pixbuf_new_from_file_at_size( iconname, width, height, &error );
				if( error ){
					g_warning( "%s: iconname=%s, error=%s", thisfn, iconname, error->message );
					g_error_free( error );
					error = NULL;
					icon = NULL;
				}
			}
		}

		g_free( iconname );
	}

	return( icon );
}

/**
 * na_object_item_is_enabled:
 * @item: the #NAObjectItem object to be requested.
 *
 * Is the specified item enabled ?
 * When disabled, the item, not its subitems if any, is/are never
 * candidate to any selection.
 *
 * Returns: %TRUE if the item is enabled, %FALSE else.
 */
gboolean
na_object_item_is_enabled( const NAObjectItem *item )
{
	gboolean enabled = FALSE;

	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), FALSE );

	if( !item->private->dispose_has_run ){
		g_object_get( G_OBJECT( item ), NAOBJECT_ITEM_PROP_ENABLED, &enabled, NULL );
	}

	return( enabled );
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
	g_return_if_fail( NA_IS_OBJECT_ITEM( item ));

	if( !item->private->dispose_has_run ){
		g_object_set( G_OBJECT( item ), NAOBJECT_ITEM_PROP_TOOLTIP, tooltip, NULL );
	}
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
	g_return_if_fail( NA_IS_OBJECT_ITEM( item ));

	if( !item->private->dispose_has_run ){
		g_object_set( G_OBJECT( item ), NAOBJECT_ITEM_PROP_ICON, icon, NULL );
	}
}

/**
 * na_object_item_set_enabled:
 * @item: the #NAObjectItem object to be updated.
 * @enabled: the indicator to be set.
 *
 * Sets whether the item, and its subitems if any, is/are enabled or not.
 */
void
na_object_item_set_enabled( NAObjectItem *item, gboolean enabled )
{
	g_return_if_fail( NA_IS_OBJECT_ITEM( item ));

	if( !item->private->dispose_has_run ){
		g_object_set( G_OBJECT( item ), NAOBJECT_ITEM_PROP_ENABLED, enabled, NULL );
	}
}

/**
 * na_object_item_set_provider:
 * @item: the #NAObjectItem object to be updated.
 * @provider: the #NAIIOProvider to be set.
 *
 * Sets the I/O provider for this #NAObjectItem.
 */
void
na_object_item_set_provider( NAObjectItem *item, const NAIIOProvider *provider )
{
	g_return_if_fail( NA_IS_OBJECT_ITEM( item ));

	if( !item->private->dispose_has_run ){
		g_object_set( G_OBJECT( item ), NAOBJECT_ITEM_PROP_PROVIDER, provider, NULL );
	}
}

/**
 * na_object_item_set_items:
 * @item: the #NAObjectItem whose subitems have to be set.
 * @list: a #GList list of #NAObject subitems to be installed.
 *
 * Sets the list of the subitems for the @item.
 *
 * The previously existing list is removed and replaced by the provided
 * one. As we create here a new list with a new reference on provided
 * subitems, the provided list can be safely na_object_items_free_items()
 * by the caller.
 */
void
na_object_item_set_items( NAObjectItem *item, GList *items )
{
	GList *it;

	g_return_if_fail( NA_IS_OBJECT_ITEM( item ));

	if( !item->private->dispose_has_run ){

		na_object_item_free_items( item->private->items );
		item->private->items = NULL;

		for( it = items ; it ; it = it->next ){
			item->private->items = g_list_prepend( item->private->items, g_object_ref( it->data ));
		}

		item->private->items = g_list_reverse( item->private->items );
	}
}

/**
 * na_object_item_append_item:
 * @item: the #NAObjectItem to which add the subitem.
 * @object: a #NAObject to be added to list of subitems.
 *
 * Appends a new @object to the list of subitems of @item.
 *
 * Doesn't modify the reference count on @object.
 */
void
na_object_item_append_item( NAObjectItem *item, const NAObject *object )
{
	g_return_if_fail( NA_IS_OBJECT_ITEM( item ));
	g_return_if_fail( NA_IS_OBJECT( object ));

	if( !item->private->dispose_has_run ){

		if( !g_list_find( item->private->items, ( gpointer ) object )){
			item->private->items = g_list_append( item->private->items, ( gpointer ) object );
		}
	}
}

/**
 * na_object_item_insert_item:
 * @item: the #NAObjectItem to which add the subitem.
 * @object: a #NAObject to be inserted in the list of subitems.
 * @before: the #NAObject before which the @object should be inserted.
 *
 * Inserts a new @object in the list of subitems of @item.
 *
 * Doesn't modify the reference count on @object.
 */
void
na_object_item_insert_item( NAObjectItem *item, const NAObject *object, const NAObject *before )
{
	GList *before_list;

	g_return_if_fail( NA_IS_OBJECT_ITEM( item ));
	g_return_if_fail( NA_IS_OBJECT( object ));
	g_return_if_fail( NA_IS_OBJECT( before ));

	if( !item->private->dispose_has_run ){

		if( !g_list_find( item->private->items, ( gpointer ) object )){
			before_list = g_list_find( item->private->items, ( gconstpointer ) before );
			if( before_list ){
				item->private->items = g_list_insert_before( item->private->items, before_list, ( gpointer ) object );
			} else {
				item->private->items = g_list_prepend( item->private->items, ( gpointer ) object );
			}
		}
	}
}

/**
 * na_object_item_remove_item:
 * @item: the #NAObjectItem from which the subitem must be removed.
 * @object: a #NAObject to be removed from the list of subitems.
 *
 * Removes an @object from the list of subitems of @item.
 *
 * Doesn't modify the reference count on @object.
 */
void
na_object_item_remove_item( NAObjectItem *item, const NAObject *object )
{
	g_return_if_fail( NA_IS_OBJECT_ITEM( item ));
	g_return_if_fail( NA_IS_OBJECT( object ));

	if( !item->private->dispose_has_run ){

		if( g_list_find( item->private->items, ( gconstpointer ) object )){
			item->private->items = g_list_remove( item->private->items, ( gconstpointer ) object );

			/* don't understand why !?
			 * it appears as if embedded actions and menus would have one sur-ref
			 * that profiles don't have
			 */
			if( NA_IS_OBJECT_ITEM( object )){
				g_object_unref(( gpointer ) object );
			}
		}
	}
}

static void
object_dump( const NAObject *item )
{
	static const gchar *thisfn = "na_object_item_object_dump";

	g_return_if_fail( NA_IS_OBJECT_ITEM( item ));

	if( !NA_OBJECT_ITEM( item )->private->dispose_has_run ){

		g_debug( "%s:  tooltip='%s'", thisfn, NA_OBJECT_ITEM( item )->private->tooltip );
		g_debug( "%s:     icon='%s'", thisfn, NA_OBJECT_ITEM( item )->private->icon );
		g_debug( "%s:  enabled='%s'", thisfn, NA_OBJECT_ITEM( item )->private->enabled ? "True" : "False" );
		g_debug( "%s: provider=%p", thisfn, ( void * ) NA_OBJECT_ITEM( item )->private->provider );

		/* dump subitems */
		g_debug( "%s: %d subitem(s) at %p",
				thisfn,
				NA_OBJECT_ITEM( item )->private->items ? g_list_length( NA_OBJECT_ITEM( item )->private->items ) : 0,
				( void * ) NA_OBJECT_ITEM( item )->private->items );

		/* do not recurse here, as this is actually dealt with by
		 * na_object_dump() api ; else, we would have the action body
		 * being dumped after its childs
		 */
	}
}

static void
object_ref( const NAObject *item )
{
	g_list_foreach( NA_OBJECT_ITEM( item )->private->items, ( GFunc ) g_object_ref, NULL );
}

static void
object_copy( NAObject *target, const NAObject *source )
{
	gchar *tooltip, *icon;
	gboolean enabled;
	gpointer provider;
	GList *subitems, *it;

	g_return_if_fail( NA_IS_OBJECT_ITEM( target ));
	g_return_if_fail( NA_IS_OBJECT_ITEM( source ));

	if( !NA_OBJECT_ITEM( target )->private->dispose_has_run &&
		!NA_OBJECT_ITEM( source )->private->dispose_has_run ){

		g_object_get( G_OBJECT( source ),
				NAOBJECT_ITEM_PROP_TOOLTIP, &tooltip,
				NAOBJECT_ITEM_PROP_ICON, &icon,
				NAOBJECT_ITEM_PROP_ENABLED, &enabled,
				NAOBJECT_ITEM_PROP_PROVIDER, &provider,
				NULL );

		g_object_set( G_OBJECT( target ),
				NAOBJECT_ITEM_PROP_TOOLTIP, tooltip,
				NAOBJECT_ITEM_PROP_ICON, icon,
				NAOBJECT_ITEM_PROP_ENABLED, enabled,
				NAOBJECT_ITEM_PROP_PROVIDER, provider,
				NULL );

		g_free( tooltip );
		g_free( icon );

		subitems = NULL;
		for( it = NA_OBJECT_ITEM( source )->private->items ; it ; it = it->next ){
			subitems = g_list_prepend( subitems, na_object_duplicate( it->data ));
		}
		subitems = g_list_reverse( subitems );
		na_object_set_items( target, subitems );
		na_object_free_items( subitems );
	}
}

/*
 * @a: original object.
 * @b: the object which has been initially duplicated from @a, and is
 * being checked for modification status.
 *
 * note 1: The provider is not considered as pertinent here
 *
 * note 2: as a particular case, this function is not recursive
 * because the equality test will stop as soon as it fails, and we so
 * cannot be sure to even come here.
 *
 * The recursivity of na_object_check_edition_status() is directly
 * dealt with by the main entry api function.
 *
 * More, the modification status of subitems doesn't have any
 * impact on this object itself, provided that subitems lists are
 * themselves identical
 *
 * note 3: #NAObjectAction is considered as modified when at least one
 * of the profiles is itself modified (because they are saved as a
 * whole). See #NAObjectAction.
 */
static gboolean
object_are_equal( const NAObject *a, const NAObject *b )
{
	gboolean equal = TRUE;
	GList *it;
	gchar *first_id, *second_id;
	NAObject *first_obj, *second_obj;
	gint first_pos, second_pos;
	GList *second_list;

	g_return_val_if_fail( NA_IS_OBJECT_ITEM( a ), FALSE );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( b ), FALSE );

	if( !NA_OBJECT_ITEM( a )->private->dispose_has_run &&
		!NA_OBJECT_ITEM( b )->private->dispose_has_run ){

		if( equal ){
			equal =
				( g_utf8_collate( NA_OBJECT_ITEM( a )->private->tooltip, NA_OBJECT_ITEM( b )->private->tooltip ) == 0 ) &&
				( g_utf8_collate( NA_OBJECT_ITEM( a )->private->icon, NA_OBJECT_ITEM( b )->private->icon ) == 0 );
		}

		if( equal ){
			equal = ( NA_OBJECT_ITEM( a )->private->enabled && NA_OBJECT_ITEM( b )->private->enabled ) ||
					( !NA_OBJECT_ITEM( a )->private->enabled && !NA_OBJECT_ITEM( b )->private->enabled );
		}

		if( equal ){
			equal = ( g_list_length( NA_OBJECT_ITEM( a )->private->items ) == g_list_length( NA_OBJECT_ITEM( b )->private->items ));
		}

		if( equal ){
			for( it = NA_OBJECT_ITEM( a )->private->items ; it && equal ; it = it->next ){
				first_id = na_object_get_id( it->data );
				second_obj = na_object_get_item( b, first_id );
				if( second_obj ){
					first_pos = g_list_position( NA_OBJECT_ITEM( a )->private->items, it );
					second_list = g_list_find( NA_OBJECT_ITEM( b )->private->items, second_obj );
					second_pos = g_list_position( NA_OBJECT_ITEM( b )->private->items, second_list );
#if NA_IDUPLICABLE_EDITION_STATUS_DEBUG
					g_debug( "na_object_item_object_are_equal: first_pos=%u, second_pos=%u", first_pos, second_pos );
#endif
					if( first_pos != second_pos ){
						equal = FALSE;
					}
				} else {
#if NA_IDUPLICABLE_EDITION_STATUS_DEBUG
					g_debug( "na_object_item_object_are_equal: id=%s not found in b", first_id );
#endif
					equal = FALSE;
				}
				g_free( first_id );
			}
		}

		if( equal ){
			for( it = NA_OBJECT_ITEM( b )->private->items ; it && equal ; it = it->next ){
				second_id = na_object_get_id( it->data );
				first_obj = na_object_get_item( a, second_id );
				if( !first_obj ){
#if NA_IDUPLICABLE_EDITION_STATUS_DEBUG
					g_debug( "na_object_item_object_are_equal: id=%s not found in a", second_id );
#endif
					equal = FALSE;
				}
				g_free( second_id );
			}
		}

#if NA_IDUPLICABLE_EDITION_STATUS_DEBUG
		g_debug( "na_object_item_object_are_equal: a=%p (%s), b=%p (%s), are_equal=%s",
				( void * ) a, G_OBJECT_TYPE_NAME( a ),
				( void * ) b, G_OBJECT_TYPE_NAME( b ),
				equal ? "True":"False" );
#endif
	}

	return( equal );
}

/*
 * from NAObjectItem point of view, all objects are valid
 */
static gboolean
object_is_valid( const NAObject *object )
{
	gboolean valid = TRUE;

	g_return_val_if_fail( NA_IS_OBJECT_ITEM( object ), FALSE );

	if( !NA_OBJECT_ITEM( object )->private->dispose_has_run ){

		/* nothing to check here */
	}

	return( valid );
}

static GList *
object_get_childs( const NAObject *object )
{
	GList *childs = NULL;

	g_return_val_if_fail( NA_IS_OBJECT_ITEM( object ), NULL );

	if( !NA_OBJECT_ITEM( object )->private->dispose_has_run ){
		childs = NA_OBJECT_ITEM( object )->private->items;
	}

	return( childs );
}

static gchar *
object_id_new_id( const NAObjectId *item )
{
	GList *it;
	uuid_t uuid;
	gchar uuid_str[64];
	gchar *new_uuid = NULL;

	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), NULL );

	if( !NA_OBJECT_ITEM( item )->private->dispose_has_run ){

		for( it = NA_OBJECT_ITEM( item )->private->items ; it ; it = it->next ){
			if( NA_IS_OBJECT_ITEM( it->data )){
				na_object_set_new_id( it->data );
			}
		}

		uuid_generate( uuid );
		uuid_unparse_lower( uuid, uuid_str );
		new_uuid = g_strdup( uuid_str );
	}

	return( new_uuid );
}
