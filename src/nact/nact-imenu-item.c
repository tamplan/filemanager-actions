/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
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

#include <glib/gi18n.h>
#include <string.h>

#include <common/na-action.h>
#include <common/na-action-profile.h>

#include "nact-imenu-item.h"

/* private interface data
 */
struct NactIMenuItemInterfacePrivate {
};

/* columns in the icon combobox
 */
enum {
	ICON_STOCK_COLUMN = 0,
	ICON_LABEL_COLUMN,
	ICON_N_COLUMN
};

static GType         register_type( void );
static void          interface_base_init( NactIMenuItemInterface *klass );
static void          interface_base_finalize( NactIMenuItemInterface *klass );

static void          icon_combo_list_fill( GtkComboBoxEntry* combo );
static GtkTreeModel *create_stock_icon_model( void );
static gint          sort_stock_ids( gconstpointer a, gconstpointer b );
static gchar        *strip_underscore( const gchar *text );
static void          on_label_changed( GtkEntry *entry, gpointer user_data );
static void          on_tooltip_changed( GtkEntry *entry, gpointer user_data );
static void          on_icon_changed( GtkEntry *entry, gpointer user_data );

static void          record_signal( NactWindow *window, GObject *instance, const gchar *signal, GCallback fn, gpointer user_data );
static void          v_signal_connected( NactWindow *window, gpointer instance, gulong handler_id );

GType
nact_imenu_item_get_type( void )
{
	static GType iface_type = 0;

	if( !iface_type ){
		iface_type = register_type();
	}

	return( iface_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "nact_imenu_item_register_type";
	g_debug( "%s", thisfn );

	static const GTypeInfo info = {
		sizeof( NactIMenuItemInterface ),
		( GBaseInitFunc ) interface_base_init,
		( GBaseFinalizeFunc ) interface_base_finalize,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL
	};

	GType type = g_type_register_static( G_TYPE_INTERFACE, "NactIMenuItem", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( NactIMenuItemInterface *klass )
{
	static const gchar *thisfn = "nact_imenu_item_interface_base_init";
	static gboolean initialized = FALSE;

	if( !initialized ){
		g_debug( "%s: klass=%p", thisfn, klass );

		klass->private = g_new0( NactIMenuItemInterfacePrivate, 1 );

		klass->signal_connected = NULL;

		initialized = TRUE;
	}
}

static void
interface_base_finalize( NactIMenuItemInterface *klass )
{
	static const gchar *thisfn = "nact_imenu_item_interface_base_finalize";
	static gboolean finalized = FALSE ;

	if( !finalized ){
		g_debug( "%s: klass=%p", thisfn, klass );

		g_free( klass->private );

		finalized = TRUE;
	}
}

void
nact_imenu_item_initial_load( NactWindow *dialog, NAAction *action )
{
	static const gchar *thisfn = "nact_imenu_item_initial_load";
	g_debug( "%s: dialog=%p, action=%p", thisfn, dialog, action );

	GtkWidget *icon_widget = base_window_get_widget( BASE_WINDOW( dialog ), "MenuIconComboBoxEntry" );

	gtk_combo_box_set_model( GTK_COMBO_BOX( icon_widget ), create_stock_icon_model());

	icon_combo_list_fill( GTK_COMBO_BOX_ENTRY( icon_widget ));
}

void
nact_imenu_item_runtime_init( NactWindow *dialog, NAAction *action )
{
	static const gchar *thisfn = "nact_imenu_item_runtime_init";
	g_debug( "%s: dialog=%p, action=%p", thisfn, dialog, action );

	GtkWidget *label_widget = base_window_get_widget( BASE_WINDOW( dialog ), "MenuLabelEntry" );
	record_signal( dialog, G_OBJECT( label_widget ), "changed", G_CALLBACK( on_label_changed ), dialog );
	gchar *label = na_action_get_label( action );
	gtk_entry_set_text( GTK_ENTRY( label_widget ), label );
	g_free( label );

	GtkWidget *tooltip_widget = base_window_get_widget( BASE_WINDOW( dialog ), "MenuTooltipEntry" );
	record_signal( dialog, G_OBJECT( tooltip_widget ), "changed", G_CALLBACK( on_tooltip_changed ), dialog );
	gchar *tooltip = na_action_get_tooltip( action );
	gtk_entry_set_text( GTK_ENTRY( tooltip_widget ), tooltip );
	g_free( tooltip );

	GtkWidget *icon_widget = base_window_get_widget( BASE_WINDOW( dialog ), "MenuIconComboBoxEntry" );
	record_signal( dialog, G_OBJECT( GTK_BIN( icon_widget )->child ), "changed", G_CALLBACK( on_icon_changed ), dialog );
	gchar *icon = na_action_get_icon( action );
	gtk_entry_set_text( GTK_ENTRY( GTK_BIN( icon_widget )->child ), icon );
	g_free( icon );
}

static void
icon_combo_list_fill( GtkComboBoxEntry* combo )
{
	GtkCellRenderer *cell_renderer_pix;
	GtkCellRenderer *cell_renderer_text;

	if( gtk_combo_box_entry_get_text_column( combo ) == -1 ){
		gtk_combo_box_entry_set_text_column( combo, ICON_STOCK_COLUMN );
	}
	gtk_cell_layout_clear( GTK_CELL_LAYOUT( combo ));

	cell_renderer_pix = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start( GTK_CELL_LAYOUT( combo ), cell_renderer_pix, FALSE );
	gtk_cell_layout_add_attribute( GTK_CELL_LAYOUT( combo ), cell_renderer_pix, "stock-id", ICON_STOCK_COLUMN );

	cell_renderer_text = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start( GTK_CELL_LAYOUT( combo ), cell_renderer_text, TRUE );
	gtk_cell_layout_add_attribute( GTK_CELL_LAYOUT( combo ), cell_renderer_text, "text", ICON_LABEL_COLUMN );

	gtk_combo_box_set_active( GTK_COMBO_BOX( combo ), 0 );
}

static GtkTreeModel *
create_stock_icon_model( void )
{
	GtkStockItem stock_item;
	gchar* label;

	GtkListStore *model = gtk_list_store_new( ICON_N_COLUMN, G_TYPE_STRING, G_TYPE_STRING );

	GtkTreeIter row;
	gtk_list_store_append( model, &row );

	/* i18n notes: when no icon is selected in the drop-down list */
	gtk_list_store_set( model, &row, ICON_STOCK_COLUMN, "", ICON_LABEL_COLUMN, _( "None" ), -1 );

	GSList *stock_list = gtk_stock_list_ids();
	GtkIconTheme *icon_theme = gtk_icon_theme_get_default();
	stock_list = g_slist_sort( stock_list, ( GCompareFunc ) sort_stock_ids );

	GSList *iter;
	for( iter = stock_list ; iter ; iter = iter->next ){
		GtkIconInfo *icon_info = gtk_icon_theme_lookup_icon( icon_theme, ( gchar * ) iter->data, GTK_ICON_SIZE_MENU, GTK_ICON_LOOKUP_FORCE_SVG );
		if( icon_info ){
			if( gtk_stock_lookup(( gchar * ) iter->data, &stock_item )){
				gtk_list_store_append( model, &row );
				label = strip_underscore( stock_item.label );
				gtk_list_store_set( model, &row, ICON_STOCK_COLUMN, ( gchar * ) iter->data, ICON_LABEL_COLUMN, label, -1 );
				g_free( label );
			}
			gtk_icon_info_free( icon_info );
		}
	}

	g_slist_foreach( stock_list, ( GFunc ) g_free, NULL );
	g_slist_free( stock_list );

	return( GTK_TREE_MODEL( model ));
}

static gint
sort_stock_ids( gconstpointer a, gconstpointer b )
{
	GtkStockItem stock_item_a;
	GtkStockItem stock_item_b;
	gchar *label_a, *label_b;
	gboolean is_a, is_b;
	int retv = 0;

	is_a = gtk_stock_lookup(( gchar * ) a, &stock_item_a );
	is_b = gtk_stock_lookup(( gchar * ) b, &stock_item_b );

	if( is_a && !is_b ){
		retv = 1;

	} else if( !is_a && is_b ){
		retv = -1;

	} else if( !is_a && !is_b ){
		retv = 0;

	} else {
		label_a = strip_underscore( stock_item_a.label );
		label_b = strip_underscore( stock_item_b.label );
		retv = g_utf8_collate( label_a, label_b );
		g_free( label_a );
		g_free( label_b );
	}

	return( retv );
}

static gchar *
strip_underscore( const gchar *text )
{
	/* Code from gtk-demo */
	gchar *p, *q, *result;

	result = g_strdup( text );
	p = q = result;
	while( *p ){
		if( *p != '_' ){
			*q = *p;
			q++;
		}
		p++;
	}
	*q = '\0';

	return( result );
}

static void
on_label_changed( GtkEntry *entry, gpointer user_data )
{
	g_assert( NACT_IS_WINDOW( user_data ));
	/*NactWindow *dialog = NACT_WINDOW( user_data );*/
}

static void
on_tooltip_changed( GtkEntry *entry, gpointer user_data )
{
	g_assert( NACT_IS_WINDOW( user_data ));
	/*NactWindow *dialog = NACT_WINDOW( user_data );*/
}

static void
on_icon_changed( GtkEntry *icon_entry, gpointer user_data )
{
	static const gchar *thisfn = "nact_imenu_item_on_icon_changed";

	g_assert( NACT_IS_WINDOW( user_data ));
	NactWindow *dialog = NACT_WINDOW( user_data );

	GtkWidget *image = base_window_get_widget( BASE_WINDOW( dialog ), "IconImage" );
	g_assert( GTK_IS_WIDGET( image ));
	const gchar *icon_name = gtk_entry_get_text( icon_entry );
	g_debug( "%s: icon_name=%s", thisfn, icon_name );

	GtkStockItem stock_item;
	GdkPixbuf *icon = NULL;

	if( icon_name && strlen( icon_name ) > 0 ){

		/* TODO: code should be mutualized with those IActionsList */
		if( gtk_stock_lookup( icon_name, &stock_item )){
			g_debug( "%s: gtk_stock_lookup", thisfn );
			gtk_image_set_from_stock( GTK_IMAGE( image ), icon_name, GTK_ICON_SIZE_MENU );
			gtk_widget_show( image );

		} else if( g_file_test( icon_name, G_FILE_TEST_EXISTS ) &&
					g_file_test( icon_name, G_FILE_TEST_IS_REGULAR )){
			g_debug( "%s: g_file_test", thisfn );
			gint width;
			gint height;
			GError *error = NULL;

			gtk_icon_size_lookup( GTK_ICON_SIZE_MENU, &width, &height );
			icon = gdk_pixbuf_new_from_file_at_size( icon_name, width, height, &error );
			if( error ){
				g_warning( "%s: gdk_pixbuf_new_from_file_at_size:%s", thisfn, error->message );
				icon = NULL;
				g_error_free( error );
			}
			gtk_image_set_from_pixbuf( GTK_IMAGE( image ), icon );
			gtk_widget_show( image );

		} else {
			g_debug( "%s: not stock, nor file", thisfn );
			gtk_widget_hide( image );
		}
	} else {
		gtk_widget_hide( image );
	}
}

static void
record_signal( NactWindow *window, GObject *instance, const gchar *signal, GCallback fn, gpointer user_data )
{
	gulong handler_id = g_signal_connect( instance, signal, fn, user_data );
	v_signal_connected( window, instance, handler_id );
}

static void
v_signal_connected( NactWindow *window, gpointer instance, gulong handler_id )
{
	g_assert( NACT_IS_IMENU_ITEM( window ));

	if( NACT_IMENU_ITEM_GET_INTERFACE( window )->signal_connected ){
		NACT_IMENU_ITEM_GET_INTERFACE( window )->signal_connected( window, instance, handler_id );
	}
}
