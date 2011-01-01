/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011 Pierre Wieser and others (see AUTHORS)
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

#include <api/na-core-utils.h>
#include <api/na-object-api.h>

#include <core/na-io-provider.h>

#include "base-iprefs.h"
#include "base-window.h"
#include "nact-application.h"
#include "nact-iprefs.h"
#include "nact-main-statusbar.h"
#include "nact-gtk-utils.h"
#include "nact-iactions-list.h"
#include "nact-main-tab.h"
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

#define IPREFS_ICONS_DIALOG					"icons-chooser"
#define IPREFS_ICONS_PATH					"icons-path"

/* IActionTab properties, set against the GObject instance
 */
#define IACTION_TAB_PROP_STATUS_CONTEXT		"nact-iaction-tab-status-context"

static gboolean st_initialized = FALSE;
static gboolean st_finalized = FALSE;
static gboolean st_on_selection_change = FALSE;

static GType         register_type( void );
static void          interface_base_init( NactIActionTabInterface *klass );
static void          interface_base_finalize( NactIActionTabInterface *klass );

static void          on_iactions_list_column_edited( NactIActionTab *instance, NAObject *object, gchar *text, gint column );
static void          on_tab_updatable_selection_changed( NactIActionTab *instance, gint count_selected );

static void          on_target_selection_toggled( GtkToggleButton *button, NactIActionTab *instance );
static void          on_target_location_toggled( GtkToggleButton *button, NactIActionTab *instance );
static void          check_for_label( NactIActionTab *instance, GtkEntry *entry, const gchar *label );
static void          on_label_changed( GtkEntry *entry, NactIActionTab *instance );
static void          set_label_label( NactIActionTab *instance, const gchar *color );
static void          on_target_toolbar_toggled( GtkToggleButton *button, NactIActionTab *instance );
static void          on_toolbar_same_label_toggled( GtkToggleButton *button, NactIActionTab *instance );
static void          toolbar_same_label_set_sensitive( NactIActionTab *instance, NAObjectItem *item );
static void          setup_toolbar_label( NactIActionTab *instance, NAObjectItem *item, const gchar *label );
static void          on_toolbar_label_changed( GtkEntry *entry, NactIActionTab *instance );
static void          toolbar_label_set_sensitive( NactIActionTab *instance, NAObjectItem *item );
static void          on_tooltip_changed( GtkEntry *entry, NactIActionTab *instance );
static GtkTreeModel *create_stock_icon_model( void );
static void          icon_combo_list_set_layout( GtkComboBox* combo );
static void          on_icon_browse( GtkButton *button, NactIActionTab *instance );
static void          on_icon_changed( GtkEntry *entry, NactIActionTab *instance );
static void          icon_preview_cb( GtkFileChooser *dialog, GtkWidget *preview );
static gint          sort_stock_ids_by_label( gconstpointer a, gconstpointer b );
static gchar        *strip_underscore( const gchar *text );
static void          release_icon_combobox( NactIActionTab *instance );
static GtkWidget    *get_icon_combo_box( NactIActionTab *instance );

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

	g_type_interface_add_prerequisite( type, BASE_WINDOW_TYPE );

	return( type );
}

static void
interface_base_init( NactIActionTabInterface *klass )
{
	static const gchar *thisfn = "nact_iaction_tab_interface_base_init";

	if( !st_initialized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NactIActionTabInterfacePrivate, 1 );

		st_initialized = TRUE;
	}
}

static void
interface_base_finalize( NactIActionTabInterface *klass )
{
	static const gchar *thisfn = "nact_iaction_tab_interface_base_finalize";

	if( st_initialized && !st_finalized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		st_finalized = TRUE;

		g_free( klass->private );
	}
}

/*
 * GTK_ICON_SIZE_MENU         : 16x16
 * GTK_ICON_SIZE_SMALL_TOOLBAR: 18x18
 * GTK_ICON_SIZE_LARGE_TOOLBAR: 24x24
 * GTK_ICON_SIZE_BUTTON       : 20x20
 * GTK_ICON_SIZE_DND          : 32x32
 * GTK_ICON_SIZE_DIALOG       : 48x48
 *
 * icon is rendered for GTK_ICON_SIZE_MENU (na_object_item_get_pixbuf)
 *
 * Starting with 3.0.3, the ComboBox is dynamically created into its container.
 */
void
nact_iaction_tab_initial_load_toplevel( NactIActionTab *instance )
{
	static const gchar *thisfn = "nact_iaction_tab_initial_load_toplevel";
	GtkFrame *frame;
	GtkButton *button;
	gint size;
#if(( GTK_MAJOR_VERSION >= 2 && GTK_MINOR_VERSION >= 91 ) || GTK_MAJOR_VERSION >= 3 )
	GtkRequisition minimal_size, natural_size;
#else
	GtkRequisition requisition;
#endif
	GtkWidget *container;
	GtkWidget *icon_combo;
	GtkTreeModel *model;

	g_return_if_fail( NACT_IS_IACTION_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

		button = GTK_BUTTON( base_window_get_widget( BASE_WINDOW( instance ), "ActionIconBrowseButton" ));
		frame = GTK_FRAME( base_window_get_widget( BASE_WINDOW( instance ), "ActionIconFrame" ));
#if(( GTK_MAJOR_VERSION >= 2 && GTK_MINOR_VERSION >= 91 ) || GTK_MAJOR_VERSION >= 3 )
		gtk_widget_get_preferred_size( GTK_WIDGET( button ), &minimal_size, &natural_size );
		size = MAX( minimal_size.height, natural_size.height );
#else
		gtk_widget_size_request( GTK_WIDGET( button ), &requisition );
		size = requisition.height;
#endif
		gtk_widget_set_size_request( GTK_WIDGET( frame ), size, size );
		gtk_frame_set_shadow_type( frame, GTK_SHADOW_IN );

		model = create_stock_icon_model();
#if(( GTK_MAJOR_VERSION >= 2 && GTK_MINOR_VERSION >= 24 ) || GTK_MAJOR_VERSION >= 3 )
		icon_combo = gtk_combo_box_new_with_model_and_entry( model );
		gtk_combo_box_set_entry_text_column( GTK_COMBO_BOX( icon_combo ), ICON_STOCK_COLUMN );
#else
		icon_combo = gtk_combo_box_entry_new_with_model( model, ICON_STOCK_COLUMN );
#endif
		icon_combo_list_set_layout( GTK_COMBO_BOX( icon_combo ));
		g_object_unref( model );
		container = base_window_get_widget( BASE_WINDOW( instance ), "ActionIconHBox" );
		gtk_box_pack_start( GTK_BOX( container ), icon_combo, TRUE, TRUE, 0 );
	}
}

void
nact_iaction_tab_runtime_init_toplevel( NactIActionTab *instance )
{
	static const gchar *thisfn = "nact_iaction_tab_runtime_init_toplevel";
	GtkWidget *label_widget, *tooltip_widget, *icon_widget;
	GtkWidget *button;

	g_return_if_fail( NACT_IS_IACTION_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( instance ),
				MAIN_WINDOW_SIGNAL_SELECTION_CHANGED,
				G_CALLBACK( on_tab_updatable_selection_changed ));

		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( instance ),
				IACTIONS_LIST_SIGNAL_COLUMN_EDITED,
				G_CALLBACK( on_iactions_list_column_edited ));

		button = base_window_get_widget( BASE_WINDOW( instance ), "ActionTargetSelectionButton" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( button ),
				"toggled",
				G_CALLBACK( on_target_selection_toggled ));

		button = base_window_get_widget( BASE_WINDOW( instance ), "ActionTargetLocationButton" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( button ),
				"toggled",
				G_CALLBACK( on_target_location_toggled ));

		label_widget = base_window_get_widget( BASE_WINDOW( instance ), "ActionMenuLabelEntry" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( label_widget ),
				"changed",
				G_CALLBACK( on_label_changed ));

		button = base_window_get_widget( BASE_WINDOW( instance ), "ActionTargetToolbarButton" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( button ),
				"toggled",
				G_CALLBACK( on_target_toolbar_toggled ));

		button = base_window_get_widget( BASE_WINDOW( instance ), "ToolbarSameLabelButton" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( button ),
				"toggled",
				G_CALLBACK( on_toolbar_same_label_toggled ));

		label_widget = base_window_get_widget( BASE_WINDOW( instance ), "ActionToolbarLabelEntry" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( label_widget ),
				"changed",
				G_CALLBACK( on_toolbar_label_changed ));

		tooltip_widget = base_window_get_widget( BASE_WINDOW( instance ), "ActionTooltipEntry" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( tooltip_widget ),
				"changed",
				G_CALLBACK( on_tooltip_changed ));

		icon_widget = get_icon_combo_box( instance );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( gtk_bin_get_child( GTK_BIN( icon_widget ))),
				"changed",
				G_CALLBACK( on_icon_changed ));

		button = base_window_get_widget( BASE_WINDOW( instance ), "ActionIconBrowseButton" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( button ),
				"clicked",
				G_CALLBACK( on_icon_browse ));
	}
}

void
nact_iaction_tab_all_widgets_showed( NactIActionTab *instance )
{
	static const gchar *thisfn = "nact_iaction_tab_all_widgets_showed";

	g_return_if_fail( NACT_IS_IACTION_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	}
}

void
nact_iaction_tab_dispose( NactIActionTab *instance )
{
	static const gchar *thisfn = "nact_iaction_tab_dispose";

	g_return_if_fail( NACT_IS_IACTION_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

		release_icon_combobox( instance );
	}
}

/**
 * nact_iaction_tab_has_label:
 * @window: this #NactIActionTab instance.
 *
 * An action or a menu can only be written if it has at least a label.
 *
 * Returns %TRUE if the label of the action or of the menu is not empty.
 */
gboolean
nact_iaction_tab_has_label( NactIActionTab *instance )
{
	GtkWidget *label_widget;
	const gchar *label;
	gboolean has_label = FALSE;

	g_return_val_if_fail( NACT_IS_IACTION_TAB( instance ), FALSE );

	if( st_initialized && !st_finalized ){

		label_widget = base_window_get_widget( BASE_WINDOW( instance ), "ActionMenuLabelEntry" );
		label = gtk_entry_get_text( GTK_ENTRY( label_widget ));
		has_label = ( g_utf8_strlen( label, -1 ) > 0 );
	}

	return( has_label );
}

static void
on_iactions_list_column_edited( NactIActionTab *instance, NAObject *object, gchar *text, gint column )
{
	GtkWidget *label_widget;

	g_return_if_fail( NACT_IS_IACTION_TAB( instance ));

	if( st_initialized && !st_finalized ){

		if( object && NA_IS_OBJECT_ITEM( object )){
			label_widget = base_window_get_widget( BASE_WINDOW( instance ), "ActionMenuLabelEntry" );
			gtk_entry_set_text( GTK_ENTRY( label_widget ), text );
		}
	}
}

static void
on_tab_updatable_selection_changed( NactIActionTab *instance, gint count_selected )
{
	static const gchar *thisfn = "nact_iaction_tab_on_tab_updatable_selection_changed";
	gboolean enable_tab;
	NAObjectItem *item;
	gboolean editable;
	gboolean target_selection, target_location, target_toolbar;
	gboolean enable_label;
	gboolean same_label;
	GtkWidget *label_widget, *tooltip_widget, *icon_widget;
	gchar *label, *tooltip, *icon;
	GtkButton *icon_button;
	GtkToggleButton *toggle;

	g_return_if_fail( BASE_IS_WINDOW( instance ));
	g_return_if_fail( NACT_IS_IACTION_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p, count_selected=%d", thisfn, ( void * ) instance, count_selected );

		enable_tab = ( count_selected == 1 );
		nact_main_tab_enable_page( NACT_MAIN_WINDOW( instance ), TAB_ACTION, enable_tab );

		st_on_selection_change = TRUE;

		g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_SELECTED_ITEM, &item,
			TAB_UPDATABLE_PROP_EDITABLE, &editable,
			NULL );

		target_selection =
				enable_tab &&
				item != NULL &&
				NA_IS_OBJECT_ACTION( item ) &&
				na_object_is_target_selection( item );

		target_location =
				enable_tab &&
				item != NULL &&
				NA_IS_OBJECT_ACTION( item ) &&
				na_object_is_target_location( item );

		target_toolbar =
				enable_tab &&
				item != NULL &&
				NA_IS_OBJECT_ACTION( item ) &&
				na_object_is_target_toolbar( item );

		toggle = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( instance ), "ActionTargetSelectionButton" ));
		gtk_toggle_button_set_active( toggle, target_selection || ( item && NA_IS_OBJECT_MENU( item )));
		gtk_widget_set_sensitive( GTK_WIDGET( toggle ), item && NA_IS_OBJECT_ACTION( item ));
		nact_gtk_utils_set_editable( G_OBJECT( toggle ), editable );

		toggle = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( instance ), "ActionTargetLocationButton" ));
		gtk_toggle_button_set_active( toggle, target_location || ( item && NA_IS_OBJECT_MENU( item )));
		gtk_widget_set_sensitive( GTK_WIDGET( toggle ), item && NA_IS_OBJECT_ACTION( item ));
		nact_gtk_utils_set_editable( G_OBJECT( toggle ), editable );

		enable_label = target_selection || target_location || ( item && NA_IS_OBJECT_MENU( item ));
		label_widget = base_window_get_widget( BASE_WINDOW( instance ), "ActionMenuLabelEntry" );
		label = item ? na_object_get_label( item ) : g_strdup( "" );
		label = label ? label : g_strdup( "" );
		gtk_entry_set_text( GTK_ENTRY( label_widget ), label );
		if( item ){
			check_for_label( instance, GTK_ENTRY( label_widget ), label );
		}
		g_free( label );
		gtk_widget_set_sensitive( label_widget, enable_label );
		nact_gtk_utils_set_editable( G_OBJECT( label_widget ), editable );

		toggle = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( instance ), "ActionTargetToolbarButton" ));
		gtk_toggle_button_set_active( toggle, target_toolbar );
		gtk_widget_set_sensitive( GTK_WIDGET( toggle ), item && NA_IS_OBJECT_ACTION( item ));
		nact_gtk_utils_set_editable( G_OBJECT( toggle ), editable );

		toggle = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( instance ), "ToolbarSameLabelButton" ));
		same_label = item && NA_IS_OBJECT_ACTION( item ) ? na_object_is_toolbar_same_label( item ) : FALSE;
		gtk_toggle_button_set_active( toggle, same_label );
		gtk_widget_set_sensitive( GTK_WIDGET( toggle ), target_toolbar );
		nact_gtk_utils_set_editable( G_OBJECT( toggle ), editable );

		label_widget = base_window_get_widget( BASE_WINDOW( instance ), "ActionToolbarLabelEntry" );
		label = item && NA_IS_OBJECT_ACTION( item ) ? na_object_get_toolbar_label( item ) : g_strdup( "" );
		gtk_entry_set_text( GTK_ENTRY( label_widget ), label );
		g_free( label );
		gtk_widget_set_sensitive( label_widget, target_toolbar && !same_label );
		nact_gtk_utils_set_editable( G_OBJECT( label_widget ), editable );

		label_widget = base_window_get_widget( BASE_WINDOW( instance ), "ActionToolbarLabelLabel" );
		gtk_widget_set_sensitive( label_widget, target_toolbar && !same_label );

		tooltip_widget = base_window_get_widget( BASE_WINDOW( instance ), "ActionTooltipEntry" );
		tooltip = item ? na_object_get_tooltip( item ) : g_strdup( "" );
		tooltip = tooltip ? tooltip : g_strdup( "" );
		gtk_entry_set_text( GTK_ENTRY( tooltip_widget ), tooltip );
		g_free( tooltip );
		nact_gtk_utils_set_editable( G_OBJECT( tooltip_widget ), editable );

		icon_widget = get_icon_combo_box( instance );
		icon = item ? na_object_get_icon( item ) : g_strdup( "" );
		icon = icon ? icon : g_strdup( "" );
		gtk_entry_set_text( GTK_ENTRY( gtk_bin_get_child( GTK_BIN( icon_widget ))), icon );
		g_free( icon );
		nact_gtk_utils_set_editable( G_OBJECT( icon_widget ), editable );

		icon_button = GTK_BUTTON( base_window_get_widget( BASE_WINDOW( instance ), "ActionIconBrowseButton" ));
		nact_gtk_utils_set_editable( G_OBJECT( icon_button ), editable );

		st_on_selection_change = FALSE;
	}
}

static void
on_target_selection_toggled( GtkToggleButton *button, NactIActionTab *instance )
{
	static const gchar *thisfn = "nact_iaction_tab_on_target_selection_toggled";
	NAObjectItem *item;
	gboolean is_target;
	gboolean editable;

	if( !st_on_selection_change ){

		g_debug( "%s: button=%p, instance=%p", thisfn, ( void * ) button, ( void * ) instance );

		g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_SELECTED_ITEM, &item,
			TAB_UPDATABLE_PROP_EDITABLE, &editable,
			NULL );

		g_debug( "%s: item=%p (%s), editable=%s",
				thisfn, ( void * ) item, item ? G_OBJECT_TYPE_NAME( item ) : "null",
				editable ? "True":"False" );

		if( item && NA_IS_OBJECT_ACTION( item )){
			is_target = gtk_toggle_button_get_active( button );

			if( editable ){
				na_object_set_target_selection( item, is_target );
				g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, item, FALSE );

			} else {
				g_signal_handlers_block_by_func(( gpointer ) button, on_target_selection_toggled, instance );
				gtk_toggle_button_set_active( button, !is_target );
				g_signal_handlers_unblock_by_func(( gpointer ) button, on_target_selection_toggled, instance );
			}
		}
	}
}

static void
on_target_location_toggled( GtkToggleButton *button, NactIActionTab *instance )
{
	static const gchar *thisfn = "nact_iaction_tab_on_target_location_toggled";
	NAObjectItem *item;
	gboolean is_target;
	gboolean editable;

	if( !st_on_selection_change ){

		g_debug( "%s: button=%p, instance=%p", thisfn, ( void * ) button, ( void * ) instance );

		g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_SELECTED_ITEM, &item,
			TAB_UPDATABLE_PROP_EDITABLE, &editable,
			NULL );

		g_debug( "%s: item=%p (%s), editable=%s",
				thisfn, ( void * ) item, item ? G_OBJECT_TYPE_NAME( item ) : "null",
				editable ? "True":"False" );

		if( item && NA_IS_OBJECT_ACTION( item )){
			is_target = gtk_toggle_button_get_active( button );

			if( editable ){
				na_object_set_target_location( item, is_target );
				g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, item, FALSE );

			} else {
				g_signal_handlers_block_by_func(( gpointer ) button, on_target_location_toggled, instance );
				gtk_toggle_button_set_active( button, !is_target );
				g_signal_handlers_unblock_by_func(( gpointer ) button, on_target_location_toggled, instance );
			}
		}
	}
}

static void
check_for_label( NactIActionTab *instance, GtkEntry *entry, const gchar *label )
{
	NAObjectItem *item;

	g_return_if_fail( NACT_IS_IACTION_TAB( instance ));
	g_return_if_fail( GTK_IS_ENTRY( entry ));

	if( st_initialized && !st_finalized ){

		nact_main_statusbar_hide_status(
				NACT_MAIN_WINDOW( instance ),
				IACTION_TAB_PROP_STATUS_CONTEXT );

		set_label_label( instance, "black" );

		g_object_get(
				G_OBJECT( instance ),
				TAB_UPDATABLE_PROP_SELECTED_ITEM, &item,
				NULL );

		if( item && g_utf8_strlen( label, -1 ) == 0 ){

			/* i18n: status bar message when the action label is empty */
			nact_main_statusbar_display_status(
					NACT_MAIN_WINDOW( instance ),
					IACTION_TAB_PROP_STATUS_CONTEXT,
					_( "Caution: a label is mandatory for the action or the menu." ));

			set_label_label( instance, "red" );
		}
	}
}

static void
on_label_changed( GtkEntry *entry, NactIActionTab *instance )
{
	NAObjectItem *item;
	const gchar *label;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_SELECTED_ITEM, &item,
			NULL );

	if( item ){
		label = gtk_entry_get_text( entry );
		na_object_set_label( item, label );
		check_for_label( instance, entry, label );

		if( NA_IS_OBJECT_ACTION( item )){
			setup_toolbar_label( instance, item, label );
		}

		g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, item, TRUE );
	}
}

static void
set_label_label( NactIActionTab *instance, const gchar *color_str )
{
	GtkWidget *label;

	label = base_window_get_widget( BASE_WINDOW( instance ), "ActionMenuLabelLabel" );

	/* gtk_widget_modify_fg() is deprecated as of Gtk+ 3.0
	 */
#if(( GTK_MAJOR_VERSION >= 2 && GTK_MINOR_VERSION >= 91 ) || GTK_MAJOR_VERSION >= 3 )
	GdkRGBA color;
	gdk_rgba_parse( &color, color_str );
	gtk_widget_override_color( label, GTK_STATE_FLAG_ACTIVE, &color );
#else
	GdkColor color;
	gdk_color_parse( color_str, &color );
	gtk_widget_modify_fg( label, GTK_STATE_NORMAL, &color );
#endif
}

static void
on_target_toolbar_toggled( GtkToggleButton *button, NactIActionTab *instance )
{
	static const gchar *thisfn = "nact_iaction_tab_on_target_toolbar_toggled";
	NAObjectAction *item;
	gboolean is_target;
	gboolean editable;

	if( !st_on_selection_change ){

		g_debug( "%s: button=%p, instance=%p", thisfn, ( void * ) button, ( void * ) instance );

		g_object_get(
				G_OBJECT( instance ),
				TAB_UPDATABLE_PROP_SELECTED_ITEM, &item,
				TAB_UPDATABLE_PROP_EDITABLE, &editable,
				NULL );

		g_debug( "%s: item=%p (%s), editable=%s",
				thisfn, ( void * ) item, item ? G_OBJECT_TYPE_NAME( item ) : "null",
				editable ? "True":"False" );

		if( item && NA_IS_OBJECT_ACTION( item )){
			is_target = gtk_toggle_button_get_active( button );

			if( editable ){
				na_object_set_target_toolbar( item, is_target );
				g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, item, FALSE );
				toolbar_same_label_set_sensitive( instance, NA_OBJECT_ITEM( item ));
				toolbar_label_set_sensitive( instance, NA_OBJECT_ITEM( item ));

			} else {
				g_signal_handlers_block_by_func(( gpointer ) button, on_target_toolbar_toggled, instance );
				gtk_toggle_button_set_active( button, !is_target );
				g_signal_handlers_unblock_by_func(( gpointer ) button, on_target_toolbar_toggled, instance );
			}
		}
	}
}

static void
on_toolbar_same_label_toggled( GtkToggleButton *button, NactIActionTab *instance )
{
	static const gchar *thisfn = "nact_iaction_tab_on_toolbar_same_label_toggled";
	NAObjectItem *item;
	gboolean same_label;
	gboolean editable;
	gchar *label;
	GtkWidget *label_widget;

	if( !st_on_selection_change ){
		g_debug( "%s: button=%p, instance=%p", thisfn, ( void * ) button, ( void * ) instance );

		g_object_get(
				G_OBJECT( instance ),
				TAB_UPDATABLE_PROP_SELECTED_ITEM, &item,
				TAB_UPDATABLE_PROP_EDITABLE, &editable,
				NULL );

		g_debug( "%s: item=%p (%s), editable=%s",
				thisfn, ( void * ) item, item ? G_OBJECT_TYPE_NAME( item ) : "null",
				editable ? "True":"False" );

		if( item && NA_IS_OBJECT_ACTION( item )){
			same_label = gtk_toggle_button_get_active( button );

			if( editable ){
				na_object_set_toolbar_same_label( NA_OBJECT_ACTION( item ), same_label );

				if( same_label ){
					label = na_object_get_label( item );
					label_widget = base_window_get_widget( BASE_WINDOW( instance ), "ActionToolbarLabelEntry" );
					gtk_entry_set_text( GTK_ENTRY( label_widget ), label );
					g_free( label );
				}

				g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, item, FALSE );
				toolbar_same_label_set_sensitive( instance, NA_OBJECT_ITEM( item ));
				toolbar_label_set_sensitive( instance, NA_OBJECT_ITEM( item ));

			} else {
				g_signal_handlers_block_by_func(( gpointer ) button, on_toolbar_same_label_toggled, instance );
				gtk_toggle_button_set_active( button, !same_label );
				g_signal_handlers_unblock_by_func(( gpointer ) button, on_toolbar_same_label_toggled, instance );
			}
		}
	}
}

static void
toolbar_same_label_set_sensitive( NactIActionTab *instance, NAObjectItem *item )
{
	GtkToggleButton *toggle;
	gboolean target_toolbar;
	gboolean readonly;

	readonly = item ? na_object_is_readonly( item ) : FALSE;
	toggle = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( instance ), "ToolbarSameLabelButton" ));
	target_toolbar = item && NA_IS_OBJECT_ACTION( item ) ? na_object_is_target_toolbar( NA_OBJECT_ACTION( item )) : FALSE;
	gtk_widget_set_sensitive( GTK_WIDGET( toggle ), target_toolbar && !readonly );
}

/*
 * setup the label of the toolbar according to the toolbar_same_label flag
 */
static void
setup_toolbar_label( NactIActionTab *instance, NAObjectItem *item, const gchar *label )
{
	GtkWidget *label_widget;

	if( item && NA_IS_OBJECT_ACTION( item )){
		if( na_object_is_toolbar_same_label( item )){
			label_widget = base_window_get_widget( BASE_WINDOW( instance ), "ActionToolbarLabelEntry" );
			gtk_entry_set_text( GTK_ENTRY( label_widget ), label );
		}
	}
}

static void
on_toolbar_label_changed( GtkEntry *entry, NactIActionTab *instance )
{
	NAObjectItem *item;
	const gchar *label;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_SELECTED_ITEM, &item,
			NULL );

	if( item && NA_IS_OBJECT_ACTION( item )){
		label = gtk_entry_get_text( entry );
		na_object_set_toolbar_label( NA_OBJECT_ACTION( item ), label );

		g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, item, FALSE );
	}
}

static void
toolbar_label_set_sensitive( NactIActionTab *instance, NAObjectItem *item )
{
	gboolean is_action;
	gboolean same_label;
	GtkWidget *label_widget;

	is_action = item && NA_IS_OBJECT_ACTION( item );
	same_label = is_action ? na_object_is_toolbar_same_label( NA_OBJECT_ACTION( item )) : FALSE;
	label_widget = base_window_get_widget( BASE_WINDOW( instance ), "ActionToolbarLabelEntry" );
	gtk_widget_set_sensitive( label_widget, is_action && !same_label );
}

static void
on_tooltip_changed( GtkEntry *entry, NactIActionTab *instance )
{
	NAObjectItem *item;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_SELECTED_ITEM, &item,
			NULL );

	if( item ){
		na_object_set_tooltip( item, gtk_entry_get_text( entry ));
		g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, item, FALSE );
	}
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
	stock_list = g_slist_sort( stock_list, ( GCompareFunc ) sort_stock_ids_by_label );

	for( iter = stock_list ; iter ; iter = iter->next ){
		icon_info = gtk_icon_theme_lookup_icon( icon_theme, ( gchar * ) iter->data, GTK_ICON_SIZE_MENU, GTK_ICON_LOOKUP_GENERIC_FALLBACK );
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

static void
icon_combo_list_set_layout( GtkComboBox *combo )
{
	GtkCellRenderer *cell_renderer_pix;
	GtkCellRenderer *cell_renderer_text;

	gtk_cell_layout_clear( GTK_CELL_LAYOUT( combo ));

	cell_renderer_pix = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start( GTK_CELL_LAYOUT( combo ), cell_renderer_pix, FALSE );
	gtk_cell_layout_add_attribute( GTK_CELL_LAYOUT( combo ), cell_renderer_pix, "stock-id", ICON_STOCK_COLUMN );

	cell_renderer_text = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start( GTK_CELL_LAYOUT( combo ), cell_renderer_text, TRUE );
	gtk_cell_layout_add_attribute( GTK_CELL_LAYOUT( combo ), cell_renderer_text, "text", ICON_LABEL_COLUMN );

	gtk_combo_box_set_active( GTK_COMBO_BOX( combo ), 0 );
}

static void
on_icon_browse( GtkButton *button, NactIActionTab *instance )
{
	nact_gtk_utils_select_file_with_preview(
			BASE_WINDOW( instance ),
			_( "Choosing an icon" ),
			IPREFS_ICONS_DIALOG,
			gtk_bin_get_child( GTK_BIN( get_icon_combo_box( instance ))),
			IPREFS_ICONS_PATH,
			"",
			G_CALLBACK( icon_preview_cb )
	);
}

static void
on_icon_changed( GtkEntry *icon_entry, NactIActionTab *instance )
{
	static const gchar *thisfn = "nact_iaction_tab_on_icon_changed";
	GtkImage *image;
	NAObjectItem *item;
	const gchar *icon_name;

	g_debug( "%s: entry=%p, instance=%p", thisfn, ( void * ) icon_entry, ( void * ) instance );

	icon_name = NULL;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_SELECTED_ITEM, &item,
			NULL );

	if( item ){
		icon_name = gtk_entry_get_text( icon_entry );
		na_object_set_icon( item, icon_name );
		g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, item, TRUE );
	}

	/* icon_name may be null if there is no current item
	 * in such a case, we blank the image
	 */
	image = GTK_IMAGE( base_window_get_widget( BASE_WINDOW( instance ), "ActionIconImage" ));
	nact_gtk_utils_render( icon_name, image, GTK_ICON_SIZE_SMALL_TOOLBAR );
}

static void
icon_preview_cb( GtkFileChooser *dialog, GtkWidget *preview )
{
	char *filename;
	GdkPixbuf *pixbuf;
	gboolean have_preview;

	filename = gtk_file_chooser_get_preview_filename( dialog );
	pixbuf = gdk_pixbuf_new_from_file_at_size( filename, 128, 128, NULL );
	have_preview = ( pixbuf != NULL );
	g_free( filename );

	if( have_preview ){
		gtk_image_set_from_pixbuf( GTK_IMAGE( preview ), pixbuf );
		g_object_unref( pixbuf );
	}

	gtk_file_chooser_set_preview_widget_active( dialog, have_preview );
}

static gint
sort_stock_ids_by_label( gconstpointer a, gconstpointer b )
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
		retv = na_core_utils_str_collate( label_a, label_b );
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
release_icon_combobox( NactIActionTab *instance )
{
	GtkWidget *combo;
	GtkTreeModel *model;

	combo = get_icon_combo_box( instance );
	model = gtk_combo_box_get_model( GTK_COMBO_BOX( combo ));
	gtk_list_store_clear( GTK_LIST_STORE( model ));
}

static GtkWidget *
get_icon_combo_box( NactIActionTab *instance )
{
	GtkWidget *icon_box, *icon_combo;

	icon_box = base_window_get_widget( BASE_WINDOW( instance ), "ActionIconHBox" );
	icon_combo = GTK_WIDGET( gtk_container_get_children( GTK_CONTAINER( icon_box ))->data );

	return( icon_combo );
}
