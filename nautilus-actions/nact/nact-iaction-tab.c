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

#include <private/na-object-api.h>

#include "base-window.h"
#include "nact-application.h"
#include "nact-main-statusbar.h"
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

/* IActionTab properties, set against the GObject instance
 */
#define IACTION_TAB_PROP_STATUS_CONTEXT		"nact-iaction-tab-status-context"

static gboolean st_initialized = FALSE;
static gboolean st_finalized = FALSE;

static GType         register_type( void );
static void          interface_base_init( NactIActionTabInterface *klass );
static void          interface_base_finalize( NactIActionTabInterface *klass );

static void          on_iactions_list_column_edited( NactIActionTab *instance, NAObject *object, gchar *text, gint column );
static void          on_tab_updatable_selection_changed( NactIActionTab *instance, gint count_selected );

static void          on_target_selection_toggled( GtkToggleButton *button, NactIActionTab *instance );
static void          on_target_background_toggled( GtkToggleButton *button, NactIActionTab *instance );

static void          check_for_label( NactIActionTab *instance, GtkEntry *entry, const gchar *label );
static void          on_label_changed( GtkEntry *entry, NactIActionTab *instance );
static void          set_label_label( NactIActionTab *instance, const gchar *color );

static void          on_target_toolbar_toggled( GtkToggleButton *button, NactIActionTab *instance );

static void          on_toolbar_same_label_toggled( GtkToggleButton *button, NactIActionTab *instance );
static void          toolbar_same_label_set_sensitive( NactIActionTab *instance, NAObjectItem *item );

static void          on_toolbar_label_changed( GtkEntry *entry, NactIActionTab *instance );
static void          toolbar_label_set_sensitive( NactIActionTab *instance, NAObjectItem *item );

static void          on_tooltip_changed( GtkEntry *entry, NactIActionTab *instance );

static GtkTreeModel *create_stock_icon_model( void );
static void          display_icon( NactIActionTab *instance, GtkWidget *image, gboolean display );
static void          icon_combo_list_fill( GtkComboBoxEntry* combo );
static void          on_icon_browse( GtkButton *button, NactIActionTab *instance );
static void          on_icon_changed( GtkEntry *entry, NactIActionTab *instance );
static gint          sort_stock_ids( gconstpointer a, gconstpointer b );
static gchar        *strip_underscore( const gchar *text );
static void          release_icon_combobox( NactIActionTab *instance );

static GtkButton    *get_enabled_button( NactIActionTab *instance );
static void          on_enabled_toggled( GtkToggleButton *button, NactIActionTab *instance );

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

	if( !st_finalized ){
		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );

		st_finalized = TRUE;
	}
}

void
nact_iaction_tab_initial_load_toplevel( NactIActionTab *instance )
{
	static const gchar *thisfn = "nact_iaction_tab_initial_load_toplevel";
	GtkWidget *icon_widget;
	GtkTreeModel *model;

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_IACTION_TAB( instance ));

	if( st_initialized && !st_finalized ){

		icon_widget = base_window_get_widget( BASE_WINDOW( instance ), "ActionIconComboBoxEntry" );
		model = create_stock_icon_model();
		gtk_combo_box_set_model( GTK_COMBO_BOX( icon_widget ), model );
		g_object_unref( model );
		icon_combo_list_fill( GTK_COMBO_BOX_ENTRY( icon_widget ));
	}
}

void
nact_iaction_tab_runtime_init_toplevel( NactIActionTab *instance )
{
	static const gchar *thisfn = "nact_iaction_tab_runtime_init_toplevel";
	GtkWidget *label_widget, *tooltip_widget, *icon_widget;
	GtkWidget *button;
	GtkButton *enabled_button;

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_IACTION_TAB( instance ));

	if( st_initialized && !st_finalized ){

		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( instance ),
				TAB_UPDATABLE_SIGNAL_SELECTION_CHANGED,
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

		button = base_window_get_widget( BASE_WINDOW( instance ), "ActionTargetBackgroundButton" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( button ),
				"toggled",
				G_CALLBACK( on_target_background_toggled ));

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

		label_widget = base_window_get_widget( BASE_WINDOW( instance ), "ActionIconLabelEntry" );
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

		icon_widget = base_window_get_widget( BASE_WINDOW( instance ), "ActionIconComboBoxEntry" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( GTK_BIN( icon_widget )->child ),
				"changed",
				G_CALLBACK( on_icon_changed ));

		button = base_window_get_widget( BASE_WINDOW( instance ), "ActionIconBrowseButton" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( button ),
				"clicked",
				G_CALLBACK( on_icon_browse ));

		enabled_button = get_enabled_button( instance );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( enabled_button ),
				"toggled",
				G_CALLBACK( on_enabled_toggled ));
	}
}

void
nact_iaction_tab_all_widgets_showed( NactIActionTab *instance )
{
	static const gchar *thisfn = "nact_iaction_tab_all_widgets_showed";

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_IACTION_TAB( instance ));

	if( st_initialized && !st_finalized ){
	}
}

void
nact_iaction_tab_dispose( NactIActionTab *instance )
{
	static const gchar *thisfn = "nact_iaction_tab_dispose";

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_IACTION_TAB( instance ));

	if( st_initialized && !st_finalized ){

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

		if( NA_IS_OBJECT_ACTION( object )){
			label_widget = base_window_get_widget( BASE_WINDOW( instance ), "ActionMenuLabelEntry" );
			gtk_entry_set_text( GTK_ENTRY( label_widget ), text );
		}
	}
}

static void
on_tab_updatable_selection_changed( NactIActionTab *instance, gint count_selected )
{
	static const gchar *thisfn = "nact_iaction_tab_on_tab_updatable_selection_changed";
	NAObjectItem *item;
	gboolean enable_tab;
	gboolean target_selection, target_background, target_toolbar;
	gboolean enable_label;
	gboolean same_label;
	GtkWidget *label_widget, *tooltip_widget, *icon_widget, *title_widget;
	gchar *label, *tooltip, *icon;
	GtkButton *enabled_button;
	gboolean enabled_item;
	GtkToggleButton *toggle;

	g_debug( "%s: instance=%p, count_selected=%d", thisfn, ( void * ) instance, count_selected );
	g_return_if_fail( BASE_IS_WINDOW( instance ));
	g_return_if_fail( NACT_IS_IACTION_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_object_get(
				G_OBJECT( instance ),
				TAB_UPDATABLE_PROP_EDITED_ACTION, &item,
				NULL );

		g_return_if_fail( !item || NA_IS_OBJECT_ITEM( item ));

		enable_tab = ( count_selected == 1 );
		nact_main_tab_enable_page( NACT_MAIN_WINDOW( instance ), TAB_ACTION, enable_tab );

		target_selection = ( item && (
				( NA_IS_OBJECT_ACTION( item ) && na_object_action_is_target_selection( NA_OBJECT_ACTION( item ))) ||
				( NA_IS_OBJECT_MENU( item ))));

		target_background = ( item && (
				( NA_IS_OBJECT_ACTION( item ) && na_object_action_is_target_background( NA_OBJECT_ACTION( item ))) ||
				( NA_IS_OBJECT_MENU( item ))));

		target_toolbar = (item && (
				( NA_IS_OBJECT_ACTION( item ) && na_object_action_is_target_toolbar( NA_OBJECT_ACTION( item )))));

		toggle = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( instance ), "ActionTargetSelectionButton" ));
		gtk_toggle_button_set_active( toggle, target_selection );
		gtk_widget_set_sensitive( GTK_WIDGET( toggle ), item && NA_IS_OBJECT_ACTION( item ));

		toggle = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( instance ), "ActionTargetBackgroundButton" ));
		gtk_toggle_button_set_active( toggle, target_background );
		gtk_widget_set_sensitive( GTK_WIDGET( toggle ), item && NA_IS_OBJECT_ACTION( item ));

		enable_label = ( item && ( NA_IS_OBJECT_MENU( item ) || target_selection || target_background ));
		label_widget = base_window_get_widget( BASE_WINDOW( instance ), "ActionMenuLabelEntry" );
		label = enable_label ? na_object_get_label( item ) : g_strdup( "" );
		gtk_entry_set_text( GTK_ENTRY( label_widget ), label );
		if( item ){
			check_for_label( instance, GTK_ENTRY( label_widget ), label );
		}
		g_free( label );
		gtk_widget_set_sensitive( label_widget, enable_label );

		toggle = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( instance ), "ActionTargetToolbarButton" ));
		gtk_toggle_button_set_active( toggle, target_toolbar );
		gtk_widget_set_sensitive( GTK_WIDGET( toggle ), item && NA_IS_OBJECT_ACTION( item ));

		toggle = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( instance ), "ToolbarSameLabelButton" ));
		same_label = item && NA_IS_OBJECT_ACTION( item ) ? na_object_action_toolbar_use_same_label( NA_OBJECT_ACTION( item )) : FALSE;
		gtk_toggle_button_set_active( toggle, same_label );
		toolbar_same_label_set_sensitive( instance, item );

		label_widget = base_window_get_widget( BASE_WINDOW( instance ), "ActionIconLabelEntry" );
		label = item && NA_IS_OBJECT_ACTION( item ) ? na_object_action_toolbar_get_label( NA_OBJECT_ACTION( item )) : g_strdup( "" );
		gtk_entry_set_text( GTK_ENTRY( label_widget ), label );
		g_free( label );
		toolbar_label_set_sensitive( instance, item );

		tooltip_widget = base_window_get_widget( BASE_WINDOW( instance ), "ActionTooltipEntry" );
		tooltip = item ? na_object_get_tooltip( item ) : g_strdup( "" );
		gtk_entry_set_text( GTK_ENTRY( tooltip_widget ), tooltip );
		g_free( tooltip );

		icon_widget = base_window_get_widget( BASE_WINDOW( instance ), "ActionIconComboBoxEntry" );
		icon = item ? na_object_get_icon( item ) : g_strdup( "" );
		gtk_entry_set_text( GTK_ENTRY( GTK_BIN( icon_widget )->child ), icon );
		g_free( icon );

		title_widget = base_window_get_widget( BASE_WINDOW( instance ), "ActionPropertiesTitle" );
		if( item && NA_IS_OBJECT_MENU( item )){
			gtk_label_set_markup( GTK_LABEL( title_widget ), _( "<b>Menu properties</b>" ));
		} else {
			gtk_label_set_markup( GTK_LABEL( title_widget ), _( "<b>Action properties</b>" ));
		}

		enabled_button = get_enabled_button( instance );
		enabled_item = item ? na_object_is_enabled( NA_OBJECT_ITEM( item )) : FALSE;
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( enabled_button ), enabled_item );

		label_widget = base_window_get_widget( BASE_WINDOW( instance ), "ActionItemID" );
		label = item ? na_object_get_id( item ) : g_strdup( "" );
		gtk_label_set_text( GTK_LABEL( label_widget ), label );
		g_free( label );

		/* TODO: manage read-only flag */
	}
}

static void
on_target_selection_toggled( GtkToggleButton *button, NactIActionTab *instance )
{
	static const gchar *thisfn = "nact_iaction_tab_on_target_selection_toggled";
	NAObjectAction *action;
	gboolean is_target;

	g_debug( "%s: button=%p, instance=%p", thisfn, ( void * ) button, ( void * ) instance );

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, &action,
			NULL );

	if( action && NA_IS_OBJECT_ACTION( action )){

		is_target = gtk_toggle_button_get_active( button );
		na_object_action_set_target_selection( action, is_target );

		g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ENABLE_TAB, action );
		g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, action, FALSE );
	}
}

static void
on_target_background_toggled( GtkToggleButton *button, NactIActionTab *instance )
{
	static const gchar *thisfn = "nact_iaction_tab_on_target_background_toggled";
	NAObjectAction *action;
	gboolean is_target;

	g_debug( "%s: button=%p, instance=%p", thisfn, ( void * ) button, ( void * ) instance );

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, &action,
			NULL );

	if( action && NA_IS_OBJECT_ACTION( action )){

		is_target = gtk_toggle_button_get_active( button );
		na_object_action_set_target_background( action, is_target );

		g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ENABLE_TAB, action );
		g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, action, FALSE );
	}
}

static void
check_for_label( NactIActionTab *instance, GtkEntry *entry, const gchar *label )
{
	NAObjectItem *edited;

	g_return_if_fail( NACT_IS_IACTION_TAB( instance ));
	g_return_if_fail( GTK_IS_ENTRY( entry ));

	if( st_initialized && !st_finalized ){

		nact_main_statusbar_hide_status(
				NACT_MAIN_WINDOW( instance ),
				IACTION_TAB_PROP_STATUS_CONTEXT );

		set_label_label( instance, "black" );

		g_object_get(
				G_OBJECT( instance ),
				TAB_UPDATABLE_PROP_EDITED_ACTION, &edited,
				NULL );

		if( edited && g_utf8_strlen( label, -1 ) == 0 ){

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
	NAObjectItem *edited;
	const gchar *label;
	gboolean target_toolbar;
	gboolean toolbar_same_label;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, &edited,
			NULL );

	if( edited ){
		label = gtk_entry_get_text( entry );
		na_object_set_label( edited, label );
		check_for_label( instance, entry, label );

		if( NA_IS_OBJECT_ACTION( edited )){
			target_toolbar = na_object_action_is_target_toolbar( NA_OBJECT_ACTION( edited ));
			if( target_toolbar ){
				toolbar_same_label = na_object_action_toolbar_use_same_label( NA_OBJECT_ACTION( edited ));
				if( toolbar_same_label ){
					na_object_action_toolbar_set_label( NA_OBJECT_ACTION( edited ), label );
				}
			}
		}

		g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, edited, TRUE );
	}
}

static void
set_label_label( NactIActionTab *instance, const gchar *color_str )
{
	GtkWidget *label;
	GdkColor color;

	label = base_window_get_widget( BASE_WINDOW( instance ), "ActionMenuLabelLabel" );
	gdk_color_parse( color_str, &color );
	gtk_widget_modify_fg( label, GTK_STATE_NORMAL, &color );
}

static void
on_target_toolbar_toggled( GtkToggleButton *button, NactIActionTab *instance )
{
	static const gchar *thisfn = "nact_iaction_tab_on_target_toolbar_toggled";
	NAObjectAction *action;
	gboolean is_target;

	g_debug( "%s: button=%p, instance=%p", thisfn, ( void * ) button, ( void * ) instance );

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, &action,
			NULL );

	if( action && NA_IS_OBJECT_ACTION( action )){

		is_target = gtk_toggle_button_get_active( button );
		na_object_action_set_target_toolbar( action, is_target );

		g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ENABLE_TAB, action );
		g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, action, FALSE );
	}

	toolbar_same_label_set_sensitive( instance, NA_OBJECT_ITEM( action ));
	toolbar_label_set_sensitive( instance, NA_OBJECT_ITEM( action ));
}

static void
on_toolbar_same_label_toggled( GtkToggleButton *button, NactIActionTab *instance )
{
	NAObjectItem *edited;
	gboolean same_label;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, &edited,
			NULL );

	if( edited && NA_IS_OBJECT_ACTION( edited )){

		same_label = gtk_toggle_button_get_active( button );
		na_object_action_toolbar_set_same_label( NA_OBJECT_ACTION( edited ), same_label );

		g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, edited, FALSE );
	}

	toolbar_same_label_set_sensitive( instance, NA_OBJECT_ITEM( edited ));
	toolbar_label_set_sensitive( instance, NA_OBJECT_ITEM( edited ));
}

static void
toolbar_same_label_set_sensitive( NactIActionTab *instance, NAObjectItem *item )
{
	GtkToggleButton *toggle;
	gboolean target_toolbar;

	toggle = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( instance ), "ToolbarSameLabelButton" ));
	target_toolbar = item && NA_IS_OBJECT_ACTION( item ) ? na_object_action_is_target_toolbar( NA_OBJECT_ACTION( item )) : FALSE;
	gtk_widget_set_sensitive( GTK_WIDGET( toggle ), target_toolbar );
}

static void
on_toolbar_label_changed( GtkEntry *entry, NactIActionTab *instance )
{
	NAObjectItem *edited;
	const gchar *label;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, &edited,
			NULL );

	if( edited && NA_IS_OBJECT_ACTION( edited )){

		label = gtk_entry_get_text( entry );
		na_object_action_toolbar_set_label( NA_OBJECT_ACTION( edited ), label );

		g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, edited, FALSE );
	}
}

static void
toolbar_label_set_sensitive( NactIActionTab *instance, NAObjectItem *item )
{
	gboolean is_action;
	gboolean same_label;
	GtkWidget *label_widget;

	is_action = item && NA_IS_OBJECT_ACTION( item );
	same_label = is_action ? na_object_action_toolbar_use_same_label( NA_OBJECT_ACTION( item )) : FALSE;
	label_widget = base_window_get_widget( BASE_WINDOW( instance ), "ActionIconLabelEntry" );
	gtk_widget_set_sensitive( label_widget, is_action && !same_label );
}

static void
on_tooltip_changed( GtkEntry *entry, NactIActionTab *instance )
{
	NAObjectItem *edited;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, &edited,
			NULL );

	if( edited ){
		na_object_item_set_tooltip( edited, gtk_entry_get_text( entry ));
		g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, edited, FALSE );
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

static void
display_icon( NactIActionTab *instance, GtkWidget *image, gboolean show )
{
	GtkFrame *frame = GTK_FRAME( base_window_get_widget( BASE_WINDOW( instance ), "ActionIconFrame" ));

	if( show ){
		gtk_widget_show( image );
		gtk_frame_set_shadow_type( frame, GTK_SHADOW_NONE );

	} else {
		gtk_widget_hide( image );
		gtk_frame_set_shadow_type( frame, GTK_SHADOW_IN );
	}
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

static void
on_icon_browse( GtkButton *button, NactIActionTab *instance )
{
	GtkWidget *dialog;
	GtkWindow *toplevel;
	gchar *filename;
	GtkWidget *icon_widget;

	toplevel = base_window_get_toplevel( BASE_WINDOW( instance ));

	dialog = gtk_file_chooser_dialog_new(
			_( "Choosing an icon" ),
			toplevel,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			NULL
			);

	if( gtk_dialog_run( GTK_DIALOG( dialog )) == GTK_RESPONSE_ACCEPT ){
		filename = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER( dialog ));
		icon_widget = base_window_get_widget( BASE_WINDOW( instance ), "ActionIconComboBoxEntry" );
		gtk_entry_set_text( GTK_ENTRY( GTK_BIN( icon_widget )->child ), filename );
	    g_free (filename);
	  }

	gtk_widget_destroy( dialog );
}

static void
on_icon_changed( GtkEntry *icon_entry, NactIActionTab *instance )
{
	/*static const gchar *thisfn = "nact_iaction_tab_on_icon_changed";*/
	GtkWidget *image;
	GdkPixbuf *icon;
	NAObjectItem *edited;
	const gchar *icon_name;

	image = base_window_get_widget( BASE_WINDOW( instance ), "ActionIconImage" );
	g_assert( GTK_IS_WIDGET( image ));
	display_icon( instance, image, FALSE );

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, &edited,
			NULL );

	if( edited ){
		icon_name = gtk_entry_get_text( icon_entry );
		na_object_item_set_icon( edited, icon_name );
		g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, edited, TRUE );

		if( icon_name && strlen( icon_name ) > 0 ){
			icon = na_object_item_get_pixbuf( edited, image );
			gtk_image_set_from_pixbuf( GTK_IMAGE( image ), icon );
			display_icon( instance, image, TRUE );
		}
	}
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
release_icon_combobox( NactIActionTab *instance )
{
	GtkWidget *combo;
	GtkTreeModel *model;

	combo = base_window_get_widget( BASE_WINDOW( instance ), "ActionIconComboBoxEntry" );
	model = gtk_combo_box_get_model( GTK_COMBO_BOX( combo ));
	gtk_list_store_clear( GTK_LIST_STORE( model ));
}

static GtkButton *
get_enabled_button( NactIActionTab *instance )
{
	return( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( instance ), "ActionEnabledButton" )));
}

static void
on_enabled_toggled( GtkToggleButton *button, NactIActionTab *instance )
{
	NAObjectItem *edited;
	gboolean enabled;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, &edited,
			NULL );

	if( edited && NA_IS_OBJECT_ITEM( edited )){
		enabled = gtk_toggle_button_get_active( button );
		na_object_item_set_enabled( edited, enabled );
		g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, edited, FALSE );
	}
}
