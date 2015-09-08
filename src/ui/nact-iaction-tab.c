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

#include <glib/gi18n.h>
#include <string.h>

#include "api/fma-core-utils.h"
#include "api/fma-object-api.h"

#include "core/fma-gtk-utils.h"
#include "core/fma-io-provider.h"

#include "fma-application.h"
#include "nact-statusbar.h"
#include "base-gtk-utils.h"
#include "nact-main-tab.h"
#include "nact-main-window.h"
#include "nact-iaction-tab.h"
#include "nact-icon-chooser.h"
#include "nact-tree-view.h"

/* private interface data
 */
struct _NactIActionTabInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* Context identifier, set against the menubar
 */
#define IACTION_TAB_CONTEXT				"nact-iaction-tab-context"

/* data set against the instance
 */
typedef struct {
	gboolean on_selection_change;
}
	IActionData;

#define IACTION_TAB_PROP_DATA			"nact-iaction-tab-data"

static guint st_initializations = 0;	/* interface initialisation count */

static GType        register_type( void );
static void         interface_base_init( NactIActionTabInterface *klass );
static void         interface_base_finalize( NactIActionTabInterface *klass );
static void         initialize_gtk( NactIActionTab *instance );
static void         initialize_window( NactIActionTab *instance );
static void         on_main_item_updated( NactIActionTab *instance, FMAIContext *context, guint data, void *empty );
static void         on_tree_selection_changed( NactTreeView *tview, GList *selected_items, NactIActionTab *instance );
static void         on_target_selection_toggled( GtkToggleButton *button, NactIActionTab *instance );
static void         on_target_location_toggled( GtkToggleButton *button, NactIActionTab *instance );
static void         check_for_label( NactIActionTab *instance, GtkEntry *entry, const gchar *label );
static void         on_label_changed( GtkEntry *entry, NactIActionTab *instance );
static void         set_label_label( NactIActionTab *instance, const gchar *color );
static void         on_target_toolbar_toggled( GtkToggleButton *button, NactIActionTab *instance );
static void         on_toolbar_same_label_toggled( GtkToggleButton *button, NactIActionTab *instance );
static void         toolbar_same_label_set_sensitive( NactIActionTab *instance, FMAObjectItem *item );
static void         setup_toolbar_label( NactIActionTab *instance, FMAObjectItem *item, const gchar *label );
static void         on_toolbar_label_changed( GtkEntry *entry, NactIActionTab *instance );
static void         toolbar_label_set_sensitive( NactIActionTab *instance, FMAObjectItem *item );
static void         on_tooltip_changed( GtkEntry *entry, NactIActionTab *instance );
static void         on_icon_browse( GtkButton *button, NactIActionTab *instance );
static void         on_icon_changed( GtkEntry *entry, NactIActionTab *instance );

static IActionData *get_iaction_data( NactIActionTab *instance );
static void         on_instance_finalized( gpointer user_data, NactIActionTab *instance );

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

	g_type_interface_add_prerequisite( type, GTK_TYPE_APPLICATION_WINDOW );

	return( type );
}

static void
interface_base_init( NactIActionTabInterface *klass )
{
	static const gchar *thisfn = "nact_iaction_tab_interface_base_init";

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NactIActionTabInterfacePrivate, 1 );
	}

	st_initializations += 1;
}

static void
interface_base_finalize( NactIActionTabInterface *klass )
{
	static const gchar *thisfn = "nact_iaction_tab_interface_base_finalize";

	st_initializations -= 1;

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );
	}
}

/**
 * nact_iaction_tab_init:
 * @instance: this #NactIActionTab instance.
 *
 * Initialize the interface
 */
void
nact_iaction_tab_init( NactIActionTab *instance )
{
	static const gchar *thisfn = "nact_iaction_tab_init";
	IActionData *data;

	g_return_if_fail( instance && NACT_IS_IACTION_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	nact_main_tab_init( NACT_MAIN_WINDOW( instance ), TAB_ACTION );
	initialize_gtk( instance );
	initialize_window( instance );

	data = get_iaction_data( instance );
	data->on_selection_change = FALSE;

	g_object_weak_ref( G_OBJECT( instance ), ( GWeakNotify ) on_instance_finalized, NULL );
}

/*
 * GTK_ICON_SIZE_MENU         : 16x16
 * GTK_ICON_SIZE_SMALL_TOOLBAR: 18x18
 * GTK_ICON_SIZE_LARGE_TOOLBAR: 24x24
 * GTK_ICON_SIZE_BUTTON       : 20x20
 * GTK_ICON_SIZE_DND          : 32x32
 * GTK_ICON_SIZE_DIALOG       : 48x48
 *
 * icon is rendered for GTK_ICON_SIZE_MENU (fma_object_item_get_pixbuf)
 *
 * Starting with 3.0.3, the ComboBox is dynamically created into its container.
 * Starting with 3.1.0, the ComboBox is replaced with a GtkEntry (thanks to new
 * Icon Chooser).
 */
static void
initialize_gtk( NactIActionTab *instance )
{
	static const gchar *thisfn = "nact_iaction_tab_initialize_gtk";
	GtkWidget *frame;
	GtkWidget *button;
	gint size;
	GtkRequisition minimal_size, natural_size;

	g_return_if_fail( NACT_IS_IACTION_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	button = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ActionIconBrowseButton" );
	g_return_if_fail( button && GTK_IS_BUTTON( button ));
	frame = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ActionIconFrame" );
	g_return_if_fail( frame && GTK_IS_FRAME( frame ));

	gtk_widget_get_preferred_size( GTK_WIDGET( button ), &minimal_size, &natural_size );
	size = MAX( minimal_size.height, natural_size.height );
	gtk_widget_set_size_request( GTK_WIDGET( frame ), size, size );
	gtk_frame_set_shadow_type( GTK_FRAME( frame ), GTK_SHADOW_IN );
}

static void
initialize_window( NactIActionTab *instance )
{
	static const gchar *thisfn = "nact_iaction_tab_initialize_window";
	NactTreeView *tview;

	g_return_if_fail( NACT_IS_IACTION_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	tview = nact_main_window_get_items_view( NACT_MAIN_WINDOW( instance ));

	g_signal_connect(
			tview, TREE_SIGNAL_SELECTION_CHANGED,
			G_CALLBACK( on_tree_selection_changed ), instance );

	g_signal_connect(
			instance, MAIN_SIGNAL_ITEM_UPDATED,
			G_CALLBACK( on_main_item_updated ), NULL );

	fma_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "ActionTargetSelectionButton",
			"toggled", G_CALLBACK( on_target_selection_toggled ), instance );

	fma_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "ActionTargetLocationButton",
			"toggled", G_CALLBACK( on_target_location_toggled ), instance );

	fma_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "ActionMenuLabelEntry",
			"changed", G_CALLBACK( on_label_changed ), instance );

	fma_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "ActionTargetToolbarButton",
			"toggled", G_CALLBACK( on_target_toolbar_toggled ), instance );

	fma_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "ToolbarSameLabelButton",
			"toggled", G_CALLBACK( on_toolbar_same_label_toggled ), instance );

	fma_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "ActionToolbarLabelEntry",
			"changed", G_CALLBACK( on_toolbar_label_changed ), instance );

	fma_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "ActionTooltipEntry",
			"changed", G_CALLBACK( on_tooltip_changed ), instance );

	fma_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "ActionIconEntry",
			"changed", G_CALLBACK( on_icon_changed ), instance );

	fma_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "ActionIconBrowseButton",
			"clicked", G_CALLBACK( on_icon_browse ), instance );
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

	label_widget = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ActionMenuLabelEntry" );
	label = gtk_entry_get_text( GTK_ENTRY( label_widget ));
	has_label = ( g_utf8_strlen( label, -1 ) > 0 );

	return( has_label );
}

static void
on_main_item_updated( NactIActionTab *instance, FMAIContext *context, guint data, void *empty )
{
	GtkWidget *label_widget;
	gchar *label;

	g_return_if_fail( instance && NACT_IS_IACTION_TAB( instance ));

	if( context && FMA_IS_OBJECT_ITEM( context )){
		label_widget = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ActionMenuLabelEntry" );
		g_return_if_fail( label_widget && GTK_IS_ENTRY( label_widget ));
		label = fma_object_get_label( context );
		gtk_entry_set_text( GTK_ENTRY( label_widget ), label );
		g_free( label );
	}
}

static void
on_tree_selection_changed( NactTreeView *tview, GList *selected_items, NactIActionTab *instance )
{
	static const gchar *thisfn = "nact_iaction_tab_on_tree_selection_changed";
	guint count_selected;
	gboolean enable_tab;
	FMAObjectItem *item;
	gboolean editable;
	gboolean target_selection, target_location, target_toolbar;
	gboolean enable_label;
	gboolean same_label;
	GtkWidget *label_widget, *tooltip_widget, *icon_widget;
	gchar *label, *tooltip, *icon;
	GtkButton *icon_button;
	GtkToggleButton *toggle;
	IActionData *data;

	g_return_if_fail( instance && NACT_IS_MAIN_WINDOW( instance ));
	g_return_if_fail( NACT_IS_IACTION_TAB( instance ));

	count_selected = g_list_length( selected_items );
	g_debug( "%s: tview=%p, selected_items=%p (count=%u), instance=%p (%s)",
			thisfn, ( void * ) tview,
			( void * ) selected_items, count_selected,
			instance, G_OBJECT_TYPE_NAME( instance ));

	enable_tab = ( count_selected == 1 );
	nact_main_tab_enable_page( NACT_MAIN_WINDOW( instance ), TAB_ACTION, enable_tab );

	data = get_iaction_data( instance );
	data->on_selection_change = TRUE;

	g_object_get(
		G_OBJECT( instance ),
		MAIN_PROP_ITEM, &item,
		MAIN_PROP_EDITABLE, &editable,
		NULL );

	target_selection =
			enable_tab &&
			item != NULL &&
			FMA_IS_OBJECT_ACTION( item ) &&
			fma_object_is_target_selection( item );

	target_location =
			enable_tab &&
			item != NULL &&
			FMA_IS_OBJECT_ACTION( item ) &&
			fma_object_is_target_location( item );

	target_toolbar =
			enable_tab &&
			item != NULL &&
			FMA_IS_OBJECT_ACTION( item ) &&
			fma_object_is_target_toolbar( item );

	toggle = GTK_TOGGLE_BUTTON( fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ActionTargetSelectionButton" ));
	gtk_toggle_button_set_active( toggle, target_selection || ( item && FMA_IS_OBJECT_MENU( item )));
	gtk_widget_set_sensitive( GTK_WIDGET( toggle ), item && FMA_IS_OBJECT_ACTION( item ));
	base_gtk_utils_set_editable( G_OBJECT( toggle ), editable );

	toggle = GTK_TOGGLE_BUTTON( fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ActionTargetLocationButton" ));
	gtk_toggle_button_set_active( toggle, target_location || ( item && FMA_IS_OBJECT_MENU( item )));
	gtk_widget_set_sensitive( GTK_WIDGET( toggle ), item && FMA_IS_OBJECT_ACTION( item ));
	base_gtk_utils_set_editable( G_OBJECT( toggle ), editable );

	enable_label = target_selection || target_location || ( item && FMA_IS_OBJECT_MENU( item ));
	label_widget = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ActionMenuLabelEntry" );
	label = item ? fma_object_get_label( item ) : g_strdup( "" );
	label = label ? label : g_strdup( "" );
	gtk_entry_set_text( GTK_ENTRY( label_widget ), label );
	if( item ){
		check_for_label( instance, GTK_ENTRY( label_widget ), label );
	}
	g_free( label );
	gtk_widget_set_sensitive( label_widget, enable_label );
	base_gtk_utils_set_editable( G_OBJECT( label_widget ), editable );

	toggle = GTK_TOGGLE_BUTTON( fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ActionTargetToolbarButton" ));
	gtk_toggle_button_set_active( toggle, target_toolbar );
	gtk_widget_set_sensitive( GTK_WIDGET( toggle ), item && FMA_IS_OBJECT_ACTION( item ));
	base_gtk_utils_set_editable( G_OBJECT( toggle ), editable );

	toggle = GTK_TOGGLE_BUTTON( fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ToolbarSameLabelButton" ));
	same_label = item && FMA_IS_OBJECT_ACTION( item ) ? fma_object_is_toolbar_same_label( item ) : FALSE;
	gtk_toggle_button_set_active( toggle, same_label );
	gtk_widget_set_sensitive( GTK_WIDGET( toggle ), target_toolbar );
	base_gtk_utils_set_editable( G_OBJECT( toggle ), editable );

	label_widget = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ActionToolbarLabelEntry" );
	label = item && FMA_IS_OBJECT_ACTION( item ) ? fma_object_get_toolbar_label( item ) : g_strdup( "" );
	gtk_entry_set_text( GTK_ENTRY( label_widget ), label );
	g_free( label );
	gtk_widget_set_sensitive( label_widget, target_toolbar && !same_label );
	base_gtk_utils_set_editable( G_OBJECT( label_widget ), editable );

	label_widget = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ActionToolbarLabelLabel" );
	gtk_widget_set_sensitive( label_widget, target_toolbar && !same_label );

	tooltip_widget = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ActionTooltipEntry" );
	tooltip = item ? fma_object_get_tooltip( item ) : g_strdup( "" );
	tooltip = tooltip ? tooltip : g_strdup( "" );
	gtk_entry_set_text( GTK_ENTRY( tooltip_widget ), tooltip );
	g_free( tooltip );
	base_gtk_utils_set_editable( G_OBJECT( tooltip_widget ), editable );

	icon_widget = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ActionIconEntry" );
	icon = item ? fma_object_get_icon( item ) : g_strdup( "" );
	icon = icon ? icon : g_strdup( "" );
	gtk_entry_set_text( GTK_ENTRY( icon_widget ), icon );
	g_free( icon );
	base_gtk_utils_set_editable( G_OBJECT( icon_widget ), editable );

	icon_button = GTK_BUTTON( fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ActionIconBrowseButton" ));
	base_gtk_utils_set_editable( G_OBJECT( icon_button ), editable );

	data->on_selection_change = FALSE;
}

static void
on_target_selection_toggled( GtkToggleButton *button, NactIActionTab *instance )
{
	static const gchar *thisfn = "nact_iaction_tab_on_target_selection_toggled";
	FMAObjectItem *item;
	gboolean is_target;
	gboolean editable;
	IActionData *data;

	g_return_if_fail( NACT_IS_IACTION_TAB( instance ));

	data = get_iaction_data( instance );

	if( !data->on_selection_change ){
		g_debug( "%s: button=%p, instance=%p (%s)",
				thisfn,
				( void * ) button,
				( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

		g_object_get(
			G_OBJECT( instance ),
			MAIN_PROP_ITEM, &item,
			MAIN_PROP_EDITABLE, &editable,
			NULL );

		g_debug( "%s: item=%p (%s), editable=%s",
				thisfn, ( void * ) item, item ? G_OBJECT_TYPE_NAME( item ) : "null",
				editable ? "True":"False" );

		if( item && FMA_IS_OBJECT_ACTION( item )){
			is_target = gtk_toggle_button_get_active( button );

			if( editable ){
				fma_object_set_target_selection( item, is_target );
				g_signal_emit_by_name( G_OBJECT( instance ), MAIN_SIGNAL_ITEM_UPDATED, item, 0 );

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
	FMAObjectItem *item;
	gboolean is_target;
	gboolean editable;
	IActionData *data;

	g_return_if_fail( NACT_IS_IACTION_TAB( instance ));

	data = get_iaction_data( instance );

	if( !data->on_selection_change ){
		g_debug( "%s: button=%p, instance=%p (%s)",
				thisfn,
				( void * ) button,
				( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

		g_object_get(
			G_OBJECT( instance ),
			MAIN_PROP_ITEM, &item,
			MAIN_PROP_EDITABLE, &editable,
			NULL );

		g_debug( "%s: item=%p (%s), editable=%s",
				thisfn, ( void * ) item, item ? G_OBJECT_TYPE_NAME( item ) : "null",
				editable ? "True":"False" );

		if( item && FMA_IS_OBJECT_ACTION( item )){
			is_target = gtk_toggle_button_get_active( button );

			if( editable ){
				fma_object_set_target_location( item, is_target );
				g_signal_emit_by_name( G_OBJECT( instance ), MAIN_SIGNAL_ITEM_UPDATED, item, 0 );

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
	FMAObjectItem *item;
	NactStatusbar *bar;

	g_return_if_fail( NACT_IS_IACTION_TAB( instance ));
	g_return_if_fail( GTK_IS_ENTRY( entry ));

	bar = nact_main_window_get_statusbar( NACT_MAIN_WINDOW( instance ));
	nact_statusbar_hide_status( bar, IACTION_TAB_CONTEXT );
	set_label_label( instance, "black" );

	g_object_get( G_OBJECT( instance ), MAIN_PROP_ITEM, &item, NULL );

	if( item && g_utf8_strlen( label, -1 ) == 0 ){

		/* i18n: status bar message when the action label is empty */
		nact_statusbar_display_status(
				bar,
				IACTION_TAB_CONTEXT,
				_( "Caution: a label is mandatory for the action or the menu." ));

		set_label_label( instance, "red" );
	}
}

static void
on_label_changed( GtkEntry *entry, NactIActionTab *instance )
{
	static const gchar *thisfn = "nact_iaction_tab_on_label_changed";
	FMAObjectItem *item;
	const gchar *label;
	IActionData *data;

	g_return_if_fail( NACT_IS_IACTION_TAB( instance ));

	data = get_iaction_data( instance );

	if( !data->on_selection_change ){
		g_debug( "%s: entry=%p, instance=%p (%s)",
				thisfn,
				( void * ) entry,
				( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

		g_object_get(
				G_OBJECT( instance ),
				MAIN_PROP_ITEM, &item,
				NULL );

		if( item ){
			label = gtk_entry_get_text( entry );
			fma_object_set_label( item, label );
			check_for_label( instance, entry, label );

			if( FMA_IS_OBJECT_ACTION( item )){
				setup_toolbar_label( instance, item, label );
			}

			g_signal_emit_by_name( G_OBJECT( instance ), MAIN_SIGNAL_ITEM_UPDATED, item, MAIN_DATA_LABEL );
		}
	}
}

static void
set_label_label( NactIActionTab *instance, const gchar *color_str )
{
	GtkWidget *label;
	GdkRGBA color;

	label = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ActionMenuLabelLabel" );

	gdk_rgba_parse( &color, color_str );
	base_gtk_utils_widget_set_color( label, &color );
}

static void
on_target_toolbar_toggled( GtkToggleButton *button, NactIActionTab *instance )
{
	static const gchar *thisfn = "nact_iaction_tab_on_target_toolbar_toggled";
	FMAObjectAction *item;
	gboolean is_target;
	gboolean editable;
	IActionData *data;

	g_return_if_fail( NACT_IS_IACTION_TAB( instance ));

	data = get_iaction_data( instance );

	if( !data->on_selection_change ){
		g_debug( "%s: button=%p, instance=%p (%s)",
				thisfn,
				( void * ) button,
				( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

		g_object_get(
				G_OBJECT( instance ),
				MAIN_PROP_ITEM, &item,
				MAIN_PROP_EDITABLE, &editable,
				NULL );

		g_debug( "%s: item=%p (%s), editable=%s",
				thisfn, ( void * ) item, item ? G_OBJECT_TYPE_NAME( item ) : "null",
				editable ? "True":"False" );

		if( item && FMA_IS_OBJECT_ACTION( item )){
			is_target = gtk_toggle_button_get_active( button );

			if( editable ){
				fma_object_set_target_toolbar( item, is_target );
				g_signal_emit_by_name( G_OBJECT( instance ), MAIN_SIGNAL_ITEM_UPDATED, item, 0 );
				toolbar_same_label_set_sensitive( instance, FMA_OBJECT_ITEM( item ));
				toolbar_label_set_sensitive( instance, FMA_OBJECT_ITEM( item ));

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
	FMAObjectItem *item;
	gboolean same_label;
	gboolean editable;
	gchar *label;
	GtkWidget *label_widget;
	IActionData *data;

	g_return_if_fail( NACT_IS_IACTION_TAB( instance ));

	data = get_iaction_data( instance );

	if( !data->on_selection_change ){
		g_debug( "%s: button=%p, instance=%p (%s)",
				thisfn,
				( void * ) button,
				( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

		g_object_get(
				G_OBJECT( instance ),
				MAIN_PROP_ITEM, &item,
				MAIN_PROP_EDITABLE, &editable,
				NULL );

		g_debug( "%s: item=%p (%s), editable=%s",
				thisfn, ( void * ) item, item ? G_OBJECT_TYPE_NAME( item ) : "null",
				editable ? "True":"False" );

		if( item && FMA_IS_OBJECT_ACTION( item )){
			same_label = gtk_toggle_button_get_active( button );

			if( editable ){
				fma_object_set_toolbar_same_label( FMA_OBJECT_ACTION( item ), same_label );

				if( same_label ){
					label = fma_object_get_label( item );
					label_widget = base_window_get_widget( BASE_WINDOW( instance ), "ActionToolbarLabelEntry" );
					gtk_entry_set_text( GTK_ENTRY( label_widget ), label );
					g_free( label );
				}

				g_signal_emit_by_name( G_OBJECT( instance ), MAIN_SIGNAL_ITEM_UPDATED, item, 0 );
				toolbar_same_label_set_sensitive( instance, FMA_OBJECT_ITEM( item ));
				toolbar_label_set_sensitive( instance, FMA_OBJECT_ITEM( item ));

			} else {
				g_signal_handlers_block_by_func(( gpointer ) button, on_toolbar_same_label_toggled, instance );
				gtk_toggle_button_set_active( button, !same_label );
				g_signal_handlers_unblock_by_func(( gpointer ) button, on_toolbar_same_label_toggled, instance );
			}
		}
	}
}

static void
toolbar_same_label_set_sensitive( NactIActionTab *instance, FMAObjectItem *item )
{
	GtkToggleButton *toggle;
	gboolean target_toolbar;
	gboolean readonly;

	readonly = item ? fma_object_is_readonly( item ) : FALSE;
	toggle = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( instance ), "ToolbarSameLabelButton" ));
	target_toolbar = item && FMA_IS_OBJECT_ACTION( item ) ? fma_object_is_target_toolbar( FMA_OBJECT_ACTION( item )) : FALSE;
	gtk_widget_set_sensitive( GTK_WIDGET( toggle ), target_toolbar && !readonly );
}

/*
 * setup the label of the toolbar according to the toolbar_same_label flag
 */
static void
setup_toolbar_label( NactIActionTab *instance, FMAObjectItem *item, const gchar *label )
{
	GtkWidget *label_widget;

	if( item && FMA_IS_OBJECT_ACTION( item )){
		if( fma_object_is_toolbar_same_label( item )){
			label_widget = base_window_get_widget( BASE_WINDOW( instance ), "ActionToolbarLabelEntry" );
			gtk_entry_set_text( GTK_ENTRY( label_widget ), label );
		}
	}
}

static void
on_toolbar_label_changed( GtkEntry *entry, NactIActionTab *instance )
{
	static const gchar *thisfn = "nact_iaction_tab_on_toolbar_label_changed";
	FMAObjectItem *item;
	const gchar *label;
	IActionData *data;

	g_return_if_fail( NACT_IS_IACTION_TAB( instance ));

	data = get_iaction_data( instance );

	if( !data->on_selection_change ){
		g_debug( "%s: entry=%p, instance=%p (%s)",
				thisfn,
				( void * ) entry,
				( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

		g_object_get(
				G_OBJECT( instance ),
				MAIN_PROP_ITEM, &item,
				NULL );

		if( item && FMA_IS_OBJECT_ACTION( item )){
			label = gtk_entry_get_text( entry );
			fma_object_set_toolbar_label( FMA_OBJECT_ACTION( item ), label );
			g_signal_emit_by_name( G_OBJECT( instance ), MAIN_SIGNAL_ITEM_UPDATED, item, 0 );
		}
	}
}

static void
toolbar_label_set_sensitive( NactIActionTab *instance, FMAObjectItem *item )
{
	gboolean is_action;
	gboolean same_label;
	GtkWidget *label_widget;

	is_action = item && FMA_IS_OBJECT_ACTION( item );
	same_label = is_action ? fma_object_is_toolbar_same_label( FMA_OBJECT_ACTION( item )) : FALSE;
	label_widget = base_window_get_widget( BASE_WINDOW( instance ), "ActionToolbarLabelEntry" );
	gtk_widget_set_sensitive( label_widget, is_action && !same_label );
}

static void
on_tooltip_changed( GtkEntry *entry, NactIActionTab *instance )
{
	static const gchar *thisfn = "nact_iaction_tab_on_tooltip_changed";
	FMAObjectItem *item;
	IActionData *data;

	g_return_if_fail( NACT_IS_IACTION_TAB( instance ));

	data = get_iaction_data( instance );

	if( !data->on_selection_change ){
		g_debug( "%s: entry=%p, instance=%p (%s)",
				thisfn,
				( void * ) entry,
				( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

		g_object_get(
				G_OBJECT( instance ),
				MAIN_PROP_ITEM, &item,
				NULL );

		if( item ){
			fma_object_set_tooltip( item, gtk_entry_get_text( entry ));
			g_signal_emit_by_name( G_OBJECT( instance ), MAIN_SIGNAL_ITEM_UPDATED, item, 0 );
		}
	}
}

static void
on_icon_browse( GtkButton *button, NactIActionTab *instance )
{
	static const gchar *thisfn = "nact_iaction_tab_on_icon_browse";
	FMAObjectItem *item;
	GtkWidget *icon_entry;
	gchar *icon_name;
	gchar *new_icon_name;

	g_return_if_fail( NACT_IS_IACTION_TAB( instance ));

	g_debug( "%s: button=%p, instance=%p (%s)",
			thisfn,
			( void * ) button,
			( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	g_object_get(
			G_OBJECT( instance ),
			MAIN_PROP_ITEM, &item,
			NULL );

	if( item ){
		icon_name = fma_object_get_icon( item );
		new_icon_name = nact_icon_chooser_choose_icon( NACT_MAIN_WINDOW( instance ), icon_name );

		if( g_utf8_collate( icon_name, new_icon_name ) != 0 ){
			icon_entry = base_window_get_widget( BASE_WINDOW( instance ), "ActionIconEntry" );
			gtk_entry_set_text( GTK_ENTRY( icon_entry ), new_icon_name );
		}

		g_free( icon_name );
		g_free( new_icon_name );
	}
}

static void
on_icon_changed( GtkEntry *icon_entry, NactIActionTab *instance )
{
	static const gchar *thisfn = "nact_iaction_tab_on_icon_changed";
	GtkImage *image;
	FMAObjectItem *item;
	gchar *icon_name;
	IActionData *data;

	g_return_if_fail( NACT_IS_IACTION_TAB( instance ));

	g_debug( "%s: icon_entry=%p, instance=%p (%s)",
			thisfn,
			( void * ) icon_entry,
			( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	icon_name = NULL;
	data = get_iaction_data( instance );

	g_object_get(
			G_OBJECT( instance ),
			MAIN_PROP_ITEM, &item,
			NULL );

	if( item ){
		if( !data->on_selection_change ){
			icon_name = g_strdup( gtk_entry_get_text( icon_entry ));
			fma_object_set_icon( item, icon_name );
			g_signal_emit_by_name( G_OBJECT( instance ), MAIN_SIGNAL_ITEM_UPDATED, item, MAIN_DATA_ICON );

		} else {
			icon_name = fma_object_get_icon( item );
		}
	}

	/* icon_name may be null if there is no current item
	 * in such a case, we blank the image
	 */
	image = GTK_IMAGE( fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ActionIconImage" ));
	base_gtk_utils_render( icon_name, image, GTK_ICON_SIZE_SMALL_TOOLBAR );
	g_free( icon_name );
}

static IActionData *
get_iaction_data( NactIActionTab *instance )
{
	IActionData *data;

	data = ( IActionData * ) g_object_get_data( G_OBJECT( instance ), IACTION_TAB_PROP_DATA );

	if( !data ){
		data = g_new0( IActionData, 1 );
		g_object_set_data( G_OBJECT( instance ), IACTION_TAB_PROP_DATA, data );
	}

	return( data );
}

static void
on_instance_finalized( gpointer user_data, NactIActionTab *instance )
{
	static const gchar *thisfn = "nact_iaction_tab_on_instance_finalized";
	IActionData *data;

	g_debug( "%s: instance=%p, user_data=%p", thisfn, ( void * ) instance, ( void * ) user_data );

	data = get_iaction_data( instance );

	g_free( data );
}
