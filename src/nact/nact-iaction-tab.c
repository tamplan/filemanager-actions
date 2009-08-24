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

#include "nact-application.h"
#include "nact-statusbar.h"
#include "nact-iaction-tab.h"

/* private interface data
 */
struct NactIActionTabInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* columns in the icon combobox
 */
enum {
	ICON_STOCK_COLUMN = 0,
	ICON_LABEL_COLUMN,
	ICON_N_COLUMN
};

/* IActionTab properties, set on the main window
 */
#define PROP_IACTION_TAB_STATUS_CONTEXT	"nact-iaction-tab-status-context"

static GType         register_type( void );
static void          interface_base_init( NactIActionTabInterface *klass );
static void          interface_base_finalize( NactIActionTabInterface *klass );

static NAObject     *v_get_selected( NactWindow *window );
static NAAction     *v_get_edited_action( NactWindow *window );
static void          v_field_modified( NactWindow *window );

static void          on_label_changed( GtkEntry *entry, gpointer user_data );
static void          check_for_label( NactWindow *window, GtkEntry *entry, const gchar *label );
static void          set_label_label( NactWindow *window, const gchar *color );
static void          on_tooltip_changed( GtkEntry *entry, gpointer user_data );
static void          on_icon_changed( GtkEntry *entry, gpointer user_data );
static void          on_icon_browse( GtkButton *button, gpointer user_data );
static void          icon_combo_list_fill( GtkComboBoxEntry* combo );
static GtkTreeModel *create_stock_icon_model( void );
static gint          sort_stock_ids( gconstpointer a, gconstpointer b );
static gchar        *strip_underscore( const gchar *text );
static void          display_icon( NactWindow *window, GtkWidget *image, gboolean display );
static void          on_enabled_toggled( GtkToggleButton *button, gpointer user_data );
static GtkButton    *get_enabled_button( NactWindow *window );

GType
nact_iaction_tab_get_type( void )
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
	static const gchar *thisfn = "nact_iaction_tab_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NactIActionTabInterface ),
		( GBaseInitFunc ) interface_base_init,
		( GBaseFinalizeFunc ) interface_base_finalize,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_INTERFACE, "NactIActionTab", &info, 0 );

	g_type_interface_add_prerequisite( type, NACT_WINDOW_TYPE );

	return( type );
}

static void
interface_base_init( NactIActionTabInterface *klass )
{
	static const gchar *thisfn = "nact_iaction_tab_interface_base_init";
	static gboolean initialized = FALSE;

	if( !initialized ){
		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NactIActionTabInterfacePrivate, 1 );

		klass->get_edited_action = NULL;
		klass->field_modified = NULL;

		initialized = TRUE;
	}
}

static void
interface_base_finalize( NactIActionTabInterface *klass )
{
	static const gchar *thisfn = "nact_iaction_tab_interface_base_finalize";
	static gboolean finalized = FALSE ;

	if( !finalized ){
		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );

		finalized = TRUE;
	}
}

void
nact_iaction_tab_initial_load( NactWindow *dialog )
{
	static const gchar *thisfn = "nact_iaction_tab_initial_load";
	GtkWidget *icon_widget;

	g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );

	icon_widget = base_window_get_widget( BASE_WINDOW( dialog ), "ActionIconComboBoxEntry" );
	gtk_combo_box_set_model( GTK_COMBO_BOX( icon_widget ), create_stock_icon_model());
	icon_combo_list_fill( GTK_COMBO_BOX_ENTRY( icon_widget ));
}

void
nact_iaction_tab_runtime_init( NactWindow *dialog )
{
	static const gchar *thisfn = "nact_iaction_tab_runtime_init";
	GtkWidget *label_widget, *tooltip_widget, *icon_widget;
	GtkWidget *button;
	GtkButton *enabled_button;

	g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );

	label_widget = base_window_get_widget( BASE_WINDOW( dialog ), "ActionLabelEntry" );
	nact_window_signal_connect( dialog, G_OBJECT( label_widget ), "changed", G_CALLBACK( on_label_changed ));

	tooltip_widget = base_window_get_widget( BASE_WINDOW( dialog ), "ActionTooltipEntry" );
	nact_window_signal_connect( dialog, G_OBJECT( tooltip_widget ), "changed", G_CALLBACK( on_tooltip_changed ));

	icon_widget = base_window_get_widget( BASE_WINDOW( dialog ), "ActionIconComboBoxEntry" );
	nact_window_signal_connect( dialog, G_OBJECT( GTK_BIN( icon_widget )->child ), "changed", G_CALLBACK( on_icon_changed ));

	button = base_window_get_widget( BASE_WINDOW( dialog ), "ActionIconBrowseButton" );
	nact_window_signal_connect( dialog, G_OBJECT( button ), "clicked", G_CALLBACK( on_icon_browse ));

	enabled_button = get_enabled_button( dialog );
	nact_window_signal_connect( dialog, G_OBJECT( enabled_button ), "toggled", G_CALLBACK( on_enabled_toggled ));
}

/**
 * A good place to set focus to the first visible field.
 */
void
nact_iaction_tab_all_widgets_showed( NactWindow *dialog )
{
	static const gchar *thisfn = "nact_iaction_tab_all_widgets_showed";

	g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );
}

void
nact_iaction_tab_dispose( NactWindow *dialog )
{
	static const gchar *thisfn = "nact_iaction_tab_dispose";

	g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );
}

/*
 * disable the tab if current row is a profile and the action has more
 * than one profile
 */
void
nact_iaction_tab_set_action( NactWindow *dialog, const NAAction *action )
{
	/*static const gchar *thisfn = "nact_iaction_tab_set_action";
	g_debug( "%s: dialog=%p, action=%p", thisfn, dialog, action );*/

	NAObject *current;
	gboolean enabled;
	GtkWidget *label_widget, *tooltip_widget, *icon_widget, *button;
	gchar *label, *tooltip, *icon;
	GtkButton *enabled_button;
	gboolean enabled_action;

	current = v_get_selected( dialog );
	enabled = ( action != NULL );
	if( NA_IS_ACTION_PROFILE( current)){
		if( na_action_get_profiles_count( action ) > 1 ){
			enabled = FALSE;
		}
	}

	label_widget = base_window_get_widget( BASE_WINDOW( dialog ), "ActionLabelEntry" );
	label = action ? na_action_get_label( action ) : g_strdup( "" );
	gtk_entry_set_text( GTK_ENTRY( label_widget ), label );
	gtk_widget_set_sensitive( label_widget, enabled );
	if( action ){
		check_for_label( dialog, GTK_ENTRY( label_widget ), label );
	}
	g_free( label );

	tooltip_widget = base_window_get_widget( BASE_WINDOW( dialog ), "ActionTooltipEntry" );
	tooltip = action ? na_action_get_tooltip( action ) : g_strdup( "" );
	gtk_entry_set_text( GTK_ENTRY( tooltip_widget ), tooltip );
	gtk_widget_set_sensitive( tooltip_widget, enabled );
	g_free( tooltip );

	icon_widget = base_window_get_widget( BASE_WINDOW( dialog ), "ActionIconComboBoxEntry" );
	icon = action ? na_action_get_icon( action ) : g_strdup( "" );
	gtk_entry_set_text( GTK_ENTRY( GTK_BIN( icon_widget )->child ), icon );
	gtk_widget_set_sensitive( icon_widget, enabled );
	g_free( icon );

	button = base_window_get_widget( BASE_WINDOW( dialog ), "ActionIconBrowseButton" );
	gtk_widget_set_sensitive( button, enabled );

	enabled_button = get_enabled_button( dialog );
	enabled_action = action ? na_action_is_enabled( action ) : FALSE;
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( enabled_button ), enabled_action );
	gtk_widget_set_sensitive( GTK_WIDGET( enabled_button ), enabled );
}

/**
 * An action can only be written if it has at least a label.
 * Returns TRUE if the label of the action is not empty.
 */
gboolean
nact_iaction_tab_has_label( NactWindow *window )
{
	GtkWidget *label_widget;
	const gchar *label;

	label_widget = base_window_get_widget( BASE_WINDOW( window ), "ActionLabelEntry" );
	label = gtk_entry_get_text( GTK_ENTRY( label_widget ));
	return( g_utf8_strlen( label, -1 ) > 0 );
}

static NAObject *
v_get_selected( NactWindow *window )
{
	g_assert( NACT_IS_IACTION_TAB( window ));

	if( NACT_IACTION_TAB_GET_INTERFACE( window )->get_selected ){
		return( NACT_IACTION_TAB_GET_INTERFACE( window )->get_selected( window ));
	}

	return( NULL );
}

static NAAction *
v_get_edited_action( NactWindow *window )
{
	g_assert( NACT_IS_IACTION_TAB( window ));

	if( NACT_IACTION_TAB_GET_INTERFACE( window )->get_edited_action ){
		return( NACT_IACTION_TAB_GET_INTERFACE( window )->get_edited_action( window ));
	}

	return( NULL );
}

static void
v_field_modified( NactWindow *window )
{
	g_assert( NACT_IS_IACTION_TAB( window ));

	if( NACT_IACTION_TAB_GET_INTERFACE( window )->field_modified ){
		NACT_IACTION_TAB_GET_INTERFACE( window )->field_modified( window );
	}
}

static void
on_label_changed( GtkEntry *entry, gpointer user_data )
{
	NactWindow *dialog;
	NAAction *edited;
	const gchar *label;

	g_assert( NACT_IS_WINDOW( user_data ));
	dialog = NACT_WINDOW( user_data );

	edited = v_get_edited_action( dialog );

	if( edited ){
		label = gtk_entry_get_text( entry );
		na_action_set_label( edited, label );
		v_field_modified( dialog );
		check_for_label( dialog, entry, label );
	}

	/* 2009-07-20: about 900-1200 usec for ten loops */
	/*int i;
	GTimeVal begin, end;
	g_get_current_time( &begin );
	for( i=0 ; i<10 ; ++i ){
		v_field_modified( dialog );
	}
	g_get_current_time( &end );
	g_debug( "on_label_changed: %ld usec", ( 1000000 * ( end.tv_sec - begin.tv_sec )) + end.tv_usec - begin.tv_usec );*/
}

static void
check_for_label( NactWindow *window, GtkEntry *entry, const gchar *label )
{
	NAAction *edited;

	nact_statusbar_hide_status( NACT_MAIN_WINDOW( window ), PROP_IACTION_TAB_STATUS_CONTEXT );
	set_label_label( window, "black" );

	edited = v_get_edited_action( window );

	if( edited && g_utf8_strlen( label, -1 ) == 0 ){
		/* i18n: status bar message when the action label is empty */
		nact_statusbar_display_status( NACT_MAIN_WINDOW( window ), PROP_IACTION_TAB_STATUS_CONTEXT, _( "Caution: a label is mandatory for the action." ));
		set_label_label( window, "red" );
	}
}

static void
set_label_label( NactWindow *window, const gchar *color )
{
	GtkWidget *label;
	gchar *text;

	label = base_window_get_widget( BASE_WINDOW( window ), "ActionLabelLabel" );
	/* i18n: label in front of the GtkEntry where user enters the action label */
	text = g_markup_printf_escaped( "<span color=\"%s\">%s</span>", color, _( "_Label :" ));
	gtk_label_set_markup_with_mnemonic( GTK_LABEL( label ), text );
}

static void
on_tooltip_changed( GtkEntry *entry, gpointer user_data )
{
	NactWindow *dialog;
	NAAction *edited;

	g_assert( NACT_IS_WINDOW( user_data ));
	dialog = NACT_WINDOW( user_data );

	edited = v_get_edited_action( dialog );

	if( edited ){
		na_action_set_tooltip( edited, gtk_entry_get_text( entry ));
		v_field_modified( dialog );
	}
}

static void
on_icon_changed( GtkEntry *icon_entry, gpointer user_data )
{
	static const gchar *thisfn = "nact_iaction_tab_on_icon_changed";
	NactWindow *dialog;
	GtkWidget *image;
	const gchar *icon_name;
	GtkStockItem stock_item;
	GdkPixbuf *icon = NULL;
	gint width, height;
	GError *error = NULL;
	NAAction *edited;

	g_assert( NACT_IS_WINDOW( user_data ));
	dialog = NACT_WINDOW( user_data );

	image = base_window_get_widget( BASE_WINDOW( dialog ), "ActionIconImage" );
	g_assert( GTK_IS_WIDGET( image ));
	display_icon( dialog, image, FALSE );

	icon_name = gtk_entry_get_text( icon_entry );
	/*g_debug( "%s: icon_name=%s", thisfn, icon_name );*/

	if( icon_name && strlen( icon_name ) > 0 ){

		/* TODO: code should be mutualized with those IActionsList */
		if( gtk_stock_lookup( icon_name, &stock_item )){
			g_debug( "%s: gtk_stock_lookup", thisfn );
			gtk_image_set_from_stock( GTK_IMAGE( image ), icon_name, GTK_ICON_SIZE_MENU );
			display_icon( dialog, image, TRUE );

		} else if( g_file_test( icon_name, G_FILE_TEST_EXISTS ) &&
					g_file_test( icon_name, G_FILE_TEST_IS_REGULAR )){
			g_debug( "%s: g_file_test", thisfn );

			gtk_icon_size_lookup( GTK_ICON_SIZE_MENU, &width, &height );
			icon = gdk_pixbuf_new_from_file_at_size( icon_name, width, height, &error );
			if( error ){
				g_warning( "%s: gdk_pixbuf_new_from_file_at_size:%s", thisfn, error->message );
				icon = NULL;
				g_error_free( error );
			}
			gtk_image_set_from_pixbuf( GTK_IMAGE( image ), icon );
			display_icon( dialog, image, TRUE );

		}
	}

	edited = v_get_edited_action( dialog );

	if( edited ){
		na_action_set_icon( edited, icon_name );
		v_field_modified( dialog );
	}
}

/* TODO: replace with a fds-compliant icon chooser */
static void
on_icon_browse( GtkButton *button, gpointer user_data )
{
	GtkWidget *dialog;
	gchar *filename;
	GtkWidget *icon_widget;

	g_assert( NACT_IS_IACTION_TAB( user_data ));

	dialog = gtk_file_chooser_dialog_new(
			_( "Choosing an icon" ),
			NULL,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			NULL
			);

	if( gtk_dialog_run( GTK_DIALOG( dialog )) == GTK_RESPONSE_ACCEPT ){
		filename = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER( dialog ));
		icon_widget = base_window_get_widget( BASE_WINDOW( user_data ), "MenuIconComboBoxEntry" );
		gtk_entry_set_text( GTK_ENTRY( GTK_BIN( icon_widget )->child ), filename );
	    g_free (filename);
	  }

	gtk_widget_destroy( dialog );
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
	GtkListStore *model;
	GtkTreeIter row;
	GSList *stock_list, *iter;
	GtkIconTheme *icon_theme;
	GtkIconInfo *icon_info;

	model = gtk_list_store_new( ICON_N_COLUMN, G_TYPE_STRING, G_TYPE_STRING );

	gtk_list_store_append( model, &row );

	/* i18n notes: when no icon is selected in the drop-down list */
	gtk_list_store_set( model, &row, ICON_STOCK_COLUMN, "", ICON_LABEL_COLUMN, _( "None" ), -1 );

	stock_list = gtk_stock_list_ids();
	icon_theme = gtk_icon_theme_get_default();
	stock_list = g_slist_sort( stock_list, ( GCompareFunc ) sort_stock_ids );

	for( iter = stock_list ; iter ; iter = iter->next ){
		icon_info = gtk_icon_theme_lookup_icon( icon_theme, ( gchar * ) iter->data, GTK_ICON_SIZE_MENU, GTK_ICON_LOOKUP_FORCE_SVG );
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
display_icon( NactWindow *window, GtkWidget *image, gboolean show )
{
	GtkFrame *frame = GTK_FRAME( base_window_get_widget( BASE_WINDOW( window ), "ActionIconFrame" ));

	if( show ){
		gtk_widget_show( image );
		gtk_frame_set_shadow_type( frame, GTK_SHADOW_NONE );

	} else {
		gtk_widget_hide( image );
		gtk_frame_set_shadow_type( frame, GTK_SHADOW_IN );
	}
}

static void
on_enabled_toggled( GtkToggleButton *button, gpointer user_data )
{
	NactWindow *dialog;
	NAAction *edited;
	gboolean enabled;

	g_assert( NACT_IS_WINDOW( user_data ));
	dialog = NACT_WINDOW( user_data );

	edited = v_get_edited_action( dialog );
	if( edited ){
		enabled = gtk_toggle_button_get_active( button );
		na_action_set_enabled( edited, enabled );
		v_field_modified( dialog );
	}
}

static GtkButton *
get_enabled_button( NactWindow *window )
{
	return( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "ActionEnabledButton" )));
}
