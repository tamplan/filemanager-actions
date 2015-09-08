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
#include <libintl.h>
#include <stdlib.h>
#include <string.h>

#include "api/fma-core-utils.h"
#include "api/fma-object-api.h"

#include "core/fma-gtk-utils.h"
#include "core/fma-desktop-environment.h"
#include "core/fma-settings.h"

#include "base-gtk-utils.h"
#include "fma-main-tab.h"
#include "nact-main-window.h"
#include "fma-ienvironment-tab.h"

/* private interface data
 */
struct _FMAIEnvironmentTabInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* columns in the selection count combobox
 */
enum {
	COUNT_SIGN_COLUMN = 0,
	COUNT_LABEL_COLUMN,
	COUNT_N_COLUMN
};

typedef struct {
	gchar *sign;
	gchar *label;
}
	SelectionCountStruct;

/* i18n notes: selection count symbol, respectively 'less than', 'equal to' and 'greater than' */
static SelectionCountStruct st_counts[] = {
		{ "<", N_( "(strictly lesser than)" ) },
		{ "=", N_( "(equal to)" ) },
		{ ">", N_( "(strictly greater than)" ) },
		{ NULL }
};

/* column ordering in the OnlyShowIn/NotShowIn listview
 */
enum {
	ENV_BOOL_COLUMN = 0,
	ENV_LABEL_COLUMN,
	ENV_KEYWORD_COLUMN,
	N_COLUMN
};

/* Pseudo-property, set against the instance
 */
typedef struct {
	gboolean on_selection_change;
}
	IEnvironData;

#define IENVIRON_TAB_PROP_DATA				"fma-ienviron-tab-data"

static guint st_initializations = 0;		/* interface initialization count */

static GType         register_type( void );
static void          interface_base_init( FMAIEnvironmentTabInterface *klass );
static void          interface_base_finalize( FMAIEnvironmentTabInterface *klass );
static void          initialize_gtk( FMAIEnvironmentTab *instance );
static void          initialize_window( FMAIEnvironmentTab *instance );
static void          on_tree_selection_changed( NactTreeView *tview, GList *selected_items, FMAIEnvironmentTab *instance );
static void          on_selcount_ope_changed( GtkComboBox *combo, FMAIEnvironmentTab *instance );
static void          on_selcount_int_changed( GtkEntry *entry, FMAIEnvironmentTab *instance );
static void          on_selection_count_changed( FMAIEnvironmentTab *instance );
static void          on_show_always_toggled( GtkToggleButton *togglebutton, FMAIEnvironmentTab *instance );
static void          on_only_show_toggled( GtkToggleButton *togglebutton, FMAIEnvironmentTab *instance );
static void          on_do_not_show_toggled( GtkToggleButton *togglebutton, FMAIEnvironmentTab *instance );
static void          on_desktop_toggled( GtkCellRendererToggle *renderer, gchar *path, FMAIEnvironmentTab *instance );
static void          on_try_exec_changed( GtkEntry *entry, FMAIEnvironmentTab *instance );
static void          on_try_exec_browse( GtkButton *button, FMAIEnvironmentTab *instance );
static void          on_show_if_registered_changed( GtkEntry *entry, FMAIEnvironmentTab *instance );
static void          on_show_if_true_changed( GtkEntry *entry, FMAIEnvironmentTab *instance );
static void          on_show_if_running_changed( GtkEntry *entry, FMAIEnvironmentTab *instance );
static void          on_show_if_running_browse( GtkButton *button, FMAIEnvironmentTab *instance );
static void          init_selection_count_combobox( FMAIEnvironmentTab *instance );
static gchar        *get_selection_count_selection( FMAIEnvironmentTab *instance );
static void          set_selection_count_selection( FMAIEnvironmentTab *instance, const gchar *ope, const gchar *uint );
static void          dispose_selection_count_combobox( FMAIEnvironmentTab *instance );
static void          init_desktop_listview( FMAIEnvironmentTab *instance );
static void          raz_desktop_listview( FMAIEnvironmentTab *instance );
static void          setup_desktop_listview( FMAIEnvironmentTab *instance, GSList *show );
static void          dispose_desktop_listview( FMAIEnvironmentTab *instance );
static IEnvironData *get_ienviron_data( FMAIEnvironmentTab *instance );
static void          on_instance_finalized( gpointer user_data, FMAIEnvironmentTab *instance );

GType
fma_ienvironment_tab_get_type( void )
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
	static const gchar *thisfn = "fma_ienvironment_tab_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( FMAIEnvironmentTabInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "FMAIEnvironmentTab", &info, 0 );

	g_type_interface_add_prerequisite( type, GTK_TYPE_APPLICATION_WINDOW );

	return( type );
}

static void
interface_base_init( FMAIEnvironmentTabInterface *klass )
{
	static const gchar *thisfn = "fma_ienvironment_tab_interface_base_init";

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( FMAIEnvironmentTabInterfacePrivate, 1 );
	}

	st_initializations += 1;
}

static void
interface_base_finalize( FMAIEnvironmentTabInterface *klass )
{
	static const gchar *thisfn = "fma_ienvironment_tab_interface_base_finalize";

	st_initializations -= 1;

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );
	}
}

/**
 * fma_ienvironment_tab_init:
 * @instance: this #FMAIEnvironmentTab instance.
 *
 * Initialize the interface
 * Connect to #BaseWindow signals
 */
void
fma_ienvironment_tab_init( FMAIEnvironmentTab *instance )
{
	static const gchar *thisfn = "fma_ienvironment_tab_init";
	IEnvironData *data;

	g_return_if_fail( FMA_IS_IENVIRONMENT_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn,
			( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	fma_main_tab_init( NACT_MAIN_WINDOW( instance ), TAB_ENVIRONMENT );
	initialize_gtk( instance );
	initialize_window( instance );

	data = get_ienviron_data( instance );
	data->on_selection_change = FALSE;

	g_object_weak_ref( G_OBJECT( instance ), ( GWeakNotify ) on_instance_finalized, NULL );
}

/*
 * on_base_initialize_gtk:
 * @window: this #FMAIEnvironmentTab instance.
 *
 * Initializes the tab widget at initial load.
 */
static void
initialize_gtk( FMAIEnvironmentTab *instance )
{
	static const gchar *thisfn = "fma_ienvironment_tab_initialize_gtk";

	g_return_if_fail( FMA_IS_IENVIRONMENT_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	init_selection_count_combobox( instance );
	init_desktop_listview( instance );
}

/*
 * on_base_initialize_window:
 * @window: this #FMAIEnvironmentTab instance.
 *
 * Initializes the tab widget at each time the widget will be displayed.
 * Connect signals and setup runtime values.
 */
static void
initialize_window( FMAIEnvironmentTab *instance )
{
	static const gchar *thisfn = "fma_ienvironment_tab_initialize_window";
	GtkTreeView *listview;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeViewColumn *column;
	GList *renderers;
	guint i;
	const FMADesktopEnv *desktops;
	NactTreeView *tview;

	g_return_if_fail( FMA_IS_IENVIRONMENT_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	tview = nact_main_window_get_items_view( NACT_MAIN_WINDOW( instance ));

	g_signal_connect(
			tview, TREE_SIGNAL_SELECTION_CHANGED,
			G_CALLBACK( on_tree_selection_changed ), instance );

	fma_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "SelectionCountSigneCombobox",
			"changed", G_CALLBACK( on_selcount_ope_changed ), instance );

	fma_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "SelectionCountNumberEntry",
			"changed", G_CALLBACK( on_selcount_int_changed ), instance );

	fma_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "ShowAlwaysButton",
			"toggled", G_CALLBACK( on_show_always_toggled ), instance );

	fma_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "OnlyShowButton",
			"toggled", G_CALLBACK( on_only_show_toggled ), instance );

	fma_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "DoNotShowButton",
			"toggled", G_CALLBACK( on_do_not_show_toggled ), instance );

	listview = GTK_TREE_VIEW(
			fma_gtk_utils_find_widget_by_name(
					GTK_CONTAINER( instance ), "EnvironmentsDesktopTreeView" ));
	model = gtk_tree_view_get_model( listview );

	desktops = fma_desktop_environment_get_known_list();

	for( i = 0 ; desktops[i].id ; ++i ){
		gtk_list_store_append( GTK_LIST_STORE( model ), &iter );
		gtk_list_store_set(
				GTK_LIST_STORE( model ),
				&iter,
				ENV_BOOL_COLUMN, FALSE,
				ENV_LABEL_COLUMN, gettext( desktops[i].label ),
				ENV_KEYWORD_COLUMN, desktops[i].id,
				-1 );
	}

	column = gtk_tree_view_get_column( listview, ENV_BOOL_COLUMN );
	renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( column ));

	g_signal_connect(
			renderers->data,
			"toggled", G_CALLBACK( on_desktop_toggled ), instance );

	fma_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "TryExecEntry",
			"changed", G_CALLBACK( on_try_exec_changed ), instance );

	fma_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "TryExecButton",
			"clicked", G_CALLBACK( on_try_exec_browse ), instance );

	fma_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "ShowIfRegisteredEntry",
			"changed", G_CALLBACK( on_show_if_registered_changed ), instance );

	fma_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "ShowIfTrueEntry",
			"changed", G_CALLBACK( on_show_if_true_changed ), instance );

	fma_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "ShowIfRunningEntry",
			"changed", G_CALLBACK( on_show_if_running_changed ), instance );

	fma_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "ShowIfRunningButton",
			"clicked", G_CALLBACK( on_show_if_running_browse ), instance );
}

static void
on_tree_selection_changed( NactTreeView *tview, GList *selected_items, FMAIEnvironmentTab *instance )
{
	static const gchar *thisfn = "fma_ienvironment_tab_on_tree_selection_changed";
	FMAIContext *context;
	gboolean editable;
	gboolean enable_tab;
	gchar *sel_count, *selcount_ope, *selcount_int;
	GtkWidget *combo, *entry;
	GtkTreeView *listview;
	GtkTreePath *path;
	GtkTreeSelection *selection;
	GtkWidget *always_button, *show_button, *notshow_button;
	GtkWidget *browse_button;
	GSList *desktops;
	gchar *text;
	IEnvironData *data;

	g_return_if_fail( FMA_IS_IENVIRONMENT_TAB( instance ));

	g_debug( "%s: tview=%p, selected_items=%p (count=%d), instance=%p (%s)",
			thisfn, tview,
			( void * ) selected_items, g_list_length( selected_items ),
			( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	g_object_get( G_OBJECT( instance ),
			MAIN_PROP_CONTEXT, &context, MAIN_PROP_EDITABLE, &editable,
			NULL );

	enable_tab = ( context != NULL );
	fma_main_tab_enable_page( NACT_MAIN_WINDOW( instance ), TAB_ENVIRONMENT, enable_tab );

	data = get_ienviron_data( instance );
	data->on_selection_change = TRUE;

	/* selection count
	 */
	sel_count = context ? fma_object_get_selection_count( context ) : g_strdup( "" );
	fma_core_utils_selcount_get_ope_int( sel_count, &selcount_ope, &selcount_int );
	set_selection_count_selection( instance, selcount_ope, selcount_int );
	g_free( selcount_int );
	g_free( selcount_ope );
	g_free( sel_count );

	combo = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "SelectionCountSigneCombobox" );
	base_gtk_utils_set_editable( G_OBJECT( combo ), editable );

	entry = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "SelectionCountNumberEntry" );
	base_gtk_utils_set_editable( G_OBJECT( entry ), editable );

	/* desktop environment
	 */
	raz_desktop_listview( instance );

	always_button = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ShowAlwaysButton" );
	show_button = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "OnlyShowButton" );
	notshow_button = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "DoNotShowButton" );

	desktops = context ? fma_object_get_only_show_in( context ) : NULL;
	listview = GTK_TREE_VIEW( fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "EnvironmentsDesktopTreeView" ));
	gtk_toggle_button_set_inconsistent( GTK_TOGGLE_BUTTON( always_button ), context == NULL );

	if( desktops && g_slist_length( desktops )){
		base_gtk_utils_radio_set_initial_state(
				GTK_RADIO_BUTTON( show_button ),
				G_CALLBACK( on_only_show_toggled ), instance, editable, ( context != NULL ));
		gtk_widget_set_sensitive( GTK_WIDGET( listview ), TRUE );

	} else {
		desktops = context ? fma_object_get_not_show_in( context ) : NULL;

		if( desktops && g_slist_length( desktops )){
			base_gtk_utils_radio_set_initial_state(
					GTK_RADIO_BUTTON( notshow_button ),
					G_CALLBACK( on_do_not_show_toggled ), instance, editable, ( context != NULL ));
			gtk_widget_set_sensitive( GTK_WIDGET( listview ), TRUE );

		} else {
			base_gtk_utils_radio_set_initial_state(
					GTK_RADIO_BUTTON( always_button ),
					G_CALLBACK( on_show_always_toggled ), instance, editable, ( context != NULL ));
			gtk_widget_set_sensitive( GTK_WIDGET( listview ), FALSE );
			desktops = NULL;
		}
	}

	setup_desktop_listview( instance, desktops );

	/* execution environment
	 */
	entry = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "TryExecEntry" );
	text = context ? fma_object_get_try_exec( context ) : g_strdup( "" );
	text = text && strlen( text ) ? text : g_strdup( "" );
	gtk_entry_set_text( GTK_ENTRY( entry ), text );
	g_free( text );
	base_gtk_utils_set_editable( G_OBJECT( entry ), editable );

	browse_button = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "TryExecButton" );
	base_gtk_utils_set_editable( G_OBJECT( browse_button ), editable );

	entry = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ShowIfRegisteredEntry" );
	text = context ? fma_object_get_show_if_registered( context ) : g_strdup( "" );
	text = text && strlen( text ) ? text : g_strdup( "" );
	gtk_entry_set_text( GTK_ENTRY( entry ), text );
	g_free( text );
	base_gtk_utils_set_editable( G_OBJECT( entry ), editable );

	entry = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ShowIfTrueEntry" );
	text = context ? fma_object_get_show_if_true( context ) : g_strdup( "" );
	text = text && strlen( text ) ? text : g_strdup( "" );
	gtk_entry_set_text( GTK_ENTRY( entry ), text );
	g_free( text );
	base_gtk_utils_set_editable( G_OBJECT( entry ), editable );

	entry = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ShowIfRunningEntry" );
	text = context ? fma_object_get_show_if_running( context ) : g_strdup( "" );
	text = text && strlen( text ) ? text : g_strdup( "" );
	gtk_entry_set_text( GTK_ENTRY( entry ), text );
	g_free( text );
	base_gtk_utils_set_editable( G_OBJECT( entry ), editable );

	browse_button = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ShowIfRunningButton" );
	base_gtk_utils_set_editable( G_OBJECT( browse_button ), editable );

	data->on_selection_change = FALSE;

	path = gtk_tree_path_new_first();
	if( path ){
		selection = gtk_tree_view_get_selection( listview );
		gtk_tree_selection_select_path( selection, path );
		gtk_tree_path_free( path );
	}
}

static void
on_selcount_ope_changed( GtkComboBox *combo, FMAIEnvironmentTab *instance )
{
	on_selection_count_changed( instance );
}

static void
on_selcount_int_changed( GtkEntry *entry, FMAIEnvironmentTab *instance )
{
	on_selection_count_changed( instance );
}

static void
on_selection_count_changed( FMAIEnvironmentTab *instance )
{
	FMAIContext *context;
	gchar *selcount;
	IEnvironData *data;

	data = get_ienviron_data( instance );

	if( !data->on_selection_change ){
		g_object_get( G_OBJECT( instance ), MAIN_PROP_CONTEXT, &context, NULL );

		if( context ){
			selcount = get_selection_count_selection( instance );
			fma_object_set_selection_count( context, selcount );
			g_free( selcount );

			g_signal_emit_by_name( G_OBJECT( instance ), MAIN_SIGNAL_ITEM_UPDATED, context, 0 );
		}
	}
}

/*
 * the behavior coded here as one main drawback:
 * - user will usually try to go through the radio buttons (always show,
 *   only show in, not show in) just to see what are their effects
 * - but each time we toggle one of these buttons, the list of desktop is raz :(
 *
 * this behavior is inherent because we have to save each modification in the
 * context as soon as this modification is made in the UI, so that user do not
 * have to save each modification before going to another tab/context/item
 *
 * as far as I know, this case is the only which has this drawback...
 */
static void
on_show_always_toggled( GtkToggleButton *toggle_button, FMAIEnvironmentTab *instance )
{
	static const gchar *thisfn = "fma_ienvironment_tab_on_show_always_toggled";
	FMAIContext *context;
	gboolean editable;
	gboolean active;
	GtkTreeView *listview;

	g_return_if_fail( FMA_IS_IENVIRONMENT_TAB( instance ));

	g_debug( "%s: toggle_button=%p (active=%s), instance=%p",
			thisfn,
			( void * ) toggle_button, gtk_toggle_button_get_active( toggle_button ) ? "True":"False",
			( void * ) instance );

	g_object_get( G_OBJECT( instance ),
			MAIN_PROP_CONTEXT, &context, MAIN_PROP_EDITABLE, &editable,
			NULL );

	if( context ){
		active = gtk_toggle_button_get_active( toggle_button );

		if( editable ){
			listview = GTK_TREE_VIEW( fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "EnvironmentsDesktopTreeView" ));
			gtk_widget_set_sensitive( GTK_WIDGET( listview ), !active );

			if( active ){
				raz_desktop_listview( instance );
				fma_object_set_only_show_in( context, NULL );
				fma_object_set_not_show_in( context, NULL );
				g_signal_emit_by_name( G_OBJECT( instance ), MAIN_SIGNAL_ITEM_UPDATED, context, 0 );
			}

		} else {
			base_gtk_utils_radio_reset_initial_state(
					GTK_RADIO_BUTTON( toggle_button ), G_CALLBACK( on_show_always_toggled ));
		}
	}
}

static void
on_only_show_toggled( GtkToggleButton *toggle_button, FMAIEnvironmentTab *instance )
{
	static const gchar *thisfn = "fma_ienvironment_tab_on_only_show_toggled";
	FMAIContext *context;
	gboolean editable;
	gboolean active;
	GSList *show;

	g_debug( "%s: toggle_button=%p (active=%s), instance=%p",
			thisfn,
			( void * ) toggle_button, gtk_toggle_button_get_active( toggle_button ) ? "True":"False",
			( void * ) instance );

	g_object_get( G_OBJECT( instance ),
			MAIN_PROP_CONTEXT, &context, MAIN_PROP_EDITABLE, &editable,
			NULL );

	if( context ){
		active = gtk_toggle_button_get_active( toggle_button );

		if( editable ){
			if( active ){
				raz_desktop_listview( instance );
				show = fma_object_get_only_show_in( context );
				if( show && g_slist_length( show )){
					setup_desktop_listview( instance, show );
				}

			} else {
				fma_object_set_only_show_in( context, NULL );
				g_signal_emit_by_name( G_OBJECT( instance ), MAIN_SIGNAL_ITEM_UPDATED, context, 0 );
			}

		} else {
			base_gtk_utils_radio_reset_initial_state(
					GTK_RADIO_BUTTON( toggle_button ), G_CALLBACK( on_only_show_toggled ));
		}
	}
}

static void
on_do_not_show_toggled( GtkToggleButton *toggle_button, FMAIEnvironmentTab *instance )
{
	static const gchar *thisfn = "fma_ienvironment_tab_on_do_not_show_toggled";
	FMAIContext *context;
	gboolean editable;
	gboolean active;
	GSList *show;

	g_debug( "%s: toggle_button=%p (active=%s), instance=%p",
			thisfn,
			( void * ) toggle_button, gtk_toggle_button_get_active( toggle_button ) ? "True":"False",
			( void * ) instance );

	g_object_get( G_OBJECT( instance ),
			MAIN_PROP_CONTEXT, &context, MAIN_PROP_EDITABLE, &editable,
			NULL );

	if( context ){
		active = gtk_toggle_button_get_active( toggle_button );

		if( editable ){
			if( active ){
				raz_desktop_listview( instance );
				show = fma_object_get_not_show_in( context );
				if( show && g_slist_length( show )){
					setup_desktop_listview( instance, show );
				}

			} else {
				fma_object_set_not_show_in( context, NULL );
				g_signal_emit_by_name( G_OBJECT( instance ), MAIN_SIGNAL_ITEM_UPDATED, context, 0 );
			}

		} else {
			base_gtk_utils_radio_reset_initial_state(
					GTK_RADIO_BUTTON( toggle_button ), G_CALLBACK( on_do_not_show_toggled ));
		}
	}
}

static void
on_desktop_toggled( GtkCellRendererToggle *renderer, gchar *path, FMAIEnvironmentTab *instance )
{
	static const gchar *thisfn = "fma_ienvironment_tab_on_desktop_toggled";
	FMAIContext *context;
	gboolean editable;
	GtkTreeView *listview;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *tree_path;
	gboolean state;
	gchar *desktop;
	GtkWidget *show_button;
	IEnvironData *data;

	g_debug( "%s: renderer=%p, path=%s, instance=%p",
			thisfn, ( void * ) renderer, path, ( void * ) instance );

	data = get_ienviron_data( instance );

	if( !data->on_selection_change ){
		g_object_get( G_OBJECT( instance ),
				MAIN_PROP_CONTEXT, &context, MAIN_PROP_EDITABLE, &editable,
				NULL );

		if( context ){
			if( editable ){
				listview = GTK_TREE_VIEW( fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "EnvironmentsDesktopTreeView" ));
				model = gtk_tree_view_get_model( listview );
				tree_path = gtk_tree_path_new_from_string( path );
				gtk_tree_model_get_iter( model, &iter, tree_path );
				gtk_tree_path_free( tree_path );
				gtk_tree_model_get( model, &iter, ENV_BOOL_COLUMN, &state, ENV_KEYWORD_COLUMN, &desktop, -1 );
				gtk_list_store_set( GTK_LIST_STORE( model ), &iter, ENV_BOOL_COLUMN, !state, -1 );

				show_button = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "OnlyShowButton" );
				if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( show_button ))){
					fma_object_set_only_desktop( context, desktop, !state );
				} else {
					fma_object_set_not_desktop( context, desktop, !state );
				}

				g_signal_emit_by_name( G_OBJECT( instance ), MAIN_SIGNAL_ITEM_UPDATED, context, 0 );

				g_free( desktop );

			} else {
				g_signal_handlers_block_by_func(( gpointer ) renderer, on_desktop_toggled, instance );
				gtk_cell_renderer_toggle_set_active( renderer, state );
				g_signal_handlers_unblock_by_func(( gpointer ) renderer, on_desktop_toggled, instance );
			}
		}
	}
}

static void
on_try_exec_changed( GtkEntry *entry, FMAIEnvironmentTab *instance )
{
	FMAIContext *context;
	const gchar *text;

	g_object_get( G_OBJECT( instance ), MAIN_PROP_CONTEXT, &context, NULL );

	if( context ){
		text = gtk_entry_get_text( entry );
		fma_object_set_try_exec( context, text );
		g_signal_emit_by_name( G_OBJECT( instance ), MAIN_SIGNAL_ITEM_UPDATED, context, 0 );
	}
}

static void
on_try_exec_browse( GtkButton *button, FMAIEnvironmentTab *instance )
{
	GtkWidget *entry;

	entry = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "TryExecEntry" );

	base_gtk_utils_select_file(
			GTK_APPLICATION_WINDOW( instance ),
			_( "Choosing an executable" ), IPREFS_TRY_EXEC_WSP,
			entry, IPREFS_TRY_EXEC_URI );
}

static void
on_show_if_registered_changed( GtkEntry *entry, FMAIEnvironmentTab *instance )
{
	FMAIContext *context;
	const gchar *text;

	g_object_get( G_OBJECT( instance ), MAIN_PROP_CONTEXT, &context, NULL );

	if( context ){
		text = gtk_entry_get_text( entry );
		fma_object_set_show_if_registered( context, text );
		g_signal_emit_by_name( G_OBJECT( instance ), MAIN_SIGNAL_ITEM_UPDATED, context, 0 );
	}
}

static void
on_show_if_true_changed( GtkEntry *entry, FMAIEnvironmentTab *instance )
{
	FMAIContext *context;
	const gchar *text;

	g_object_get( G_OBJECT( instance ), MAIN_PROP_CONTEXT, &context, NULL );

	if( context ){
		text = gtk_entry_get_text( entry );
		fma_object_set_show_if_true( context, text );
		g_signal_emit_by_name( G_OBJECT( instance ), MAIN_SIGNAL_ITEM_UPDATED, context, 0 );
	}
}

static void
on_show_if_running_changed( GtkEntry *entry, FMAIEnvironmentTab *instance )
{
	FMAIContext *context;
	const gchar *text;

	g_object_get( G_OBJECT( instance ), MAIN_PROP_CONTEXT, &context, NULL );

	if( context ){
		text = gtk_entry_get_text( entry );
		fma_object_set_show_if_running( context, text );
		g_signal_emit_by_name( G_OBJECT( instance ), MAIN_SIGNAL_ITEM_UPDATED, context, 0 );
	}
}

static void
on_show_if_running_browse( GtkButton *button, FMAIEnvironmentTab *instance )
{
	GtkWidget *entry;

	entry = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ShowIfRunningEntry" );

	base_gtk_utils_select_file(
			GTK_APPLICATION_WINDOW( instance ),
			_( "Choosing an executable" ), IPREFS_SHOW_IF_RUNNING_WSP,
			entry, IPREFS_SHOW_IF_RUNNING_URI );
}

static void
init_selection_count_combobox( FMAIEnvironmentTab *instance )
{
	GtkTreeModel *model;
	guint i;
	GtkTreeIter row;
	GtkComboBox *combo;
	GtkCellRenderer *cell_renderer_text;

	model = GTK_TREE_MODEL( gtk_list_store_new( COUNT_N_COLUMN, G_TYPE_STRING, G_TYPE_STRING ));
	i = 0;
	while( st_counts[i].sign ){
		gtk_list_store_append( GTK_LIST_STORE( model ), &row );
		gtk_list_store_set( GTK_LIST_STORE( model ), &row, COUNT_SIGN_COLUMN, st_counts[i].sign, -1 );
		gtk_list_store_set( GTK_LIST_STORE( model ), &row, COUNT_LABEL_COLUMN, gettext( st_counts[i].label ), -1 );
		i += 1;
	}

	combo = GTK_COMBO_BOX( fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "SelectionCountSigneCombobox" ));
	gtk_combo_box_set_model( combo, model );
	g_object_unref( model );

	gtk_cell_layout_clear( GTK_CELL_LAYOUT( combo ));

	cell_renderer_text = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start( GTK_CELL_LAYOUT( combo ), cell_renderer_text, FALSE );
	gtk_cell_layout_add_attribute( GTK_CELL_LAYOUT( combo ), cell_renderer_text, "text", COUNT_SIGN_COLUMN );

	cell_renderer_text = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start( GTK_CELL_LAYOUT( combo ), cell_renderer_text, TRUE );
	g_object_set( G_OBJECT( cell_renderer_text ), "xalign", ( gdouble ) 0.0, "style", PANGO_STYLE_ITALIC, "style-set", TRUE, NULL );
	gtk_cell_layout_add_attribute( GTK_CELL_LAYOUT( combo ), cell_renderer_text, "text", COUNT_LABEL_COLUMN );

	gtk_combo_box_set_active( GTK_COMBO_BOX( combo ), 0 );
}

static gchar *
get_selection_count_selection( FMAIEnvironmentTab *instance )
{
	GtkComboBox *combo;
	GtkEntry *entry;
	gint index;
	gchar *uints, *selcount;
	guint uinti;

	combo = GTK_COMBO_BOX( fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "SelectionCountSigneCombobox" ));
	index = gtk_combo_box_get_active( combo );
	if( index == -1 ){
		return( NULL );
	}

	entry = GTK_ENTRY( fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "SelectionCountNumberEntry" ));
	uinti = abs( atoi( gtk_entry_get_text( entry )));
	uints = g_strdup_printf( "%d", uinti );
	gtk_entry_set_text( entry, uints );
	g_free( uints );

	selcount = g_strdup_printf( "%s%d", st_counts[index].sign, uinti );

	return( selcount );
}

static void
set_selection_count_selection( FMAIEnvironmentTab *instance, const gchar *ope, const gchar *uint )
{
	GtkComboBox *combo;
	GtkEntry *entry;
	gint i, index;

	combo = GTK_COMBO_BOX( fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "SelectionCountSigneCombobox" ));

	index = -1;
	for( i=0 ; st_counts[i].sign && index==-1 ; ++i ){
		if( !strcmp( st_counts[i].sign, ope )){
			index = i;
		}
	}
	gtk_combo_box_set_active( combo, index );

	entry = GTK_ENTRY( fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "SelectionCountNumberEntry" ));
	gtk_entry_set_text( entry, uint );
}

static void
dispose_selection_count_combobox( FMAIEnvironmentTab *instance )
{
	GtkWidget *combo;
	GtkTreeModel *model;

	combo = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "SelectionCountSigneCombobox" );
	if( GTK_IS_COMBO_BOX( combo )){
		model = gtk_combo_box_get_model( GTK_COMBO_BOX( combo ));
		gtk_list_store_clear( GTK_LIST_STORE( model ));
	}
}

static void
init_desktop_listview( FMAIEnvironmentTab *instance )
{
	GtkTreeView *listview;
	GtkListStore *model;
	GtkCellRenderer *check_cell, *text_cell;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;

	listview = GTK_TREE_VIEW( fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "EnvironmentsDesktopTreeView" ));
	model = gtk_list_store_new( N_COLUMN, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING );
	gtk_tree_view_set_model( listview, GTK_TREE_MODEL( model ));
	g_object_unref( model );

	check_cell = gtk_cell_renderer_toggle_new();
	column = gtk_tree_view_column_new_with_attributes(
			"boolean",
			check_cell,
			"active", ENV_BOOL_COLUMN,
			NULL );
	gtk_tree_view_append_column( listview, column );

	text_cell = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
			"label",
			text_cell,
			"text", ENV_LABEL_COLUMN,
			NULL );
	gtk_tree_view_append_column( listview, column );

	gtk_tree_view_set_headers_visible( listview, FALSE );

	selection = gtk_tree_view_get_selection( listview );
	gtk_tree_selection_set_mode( selection, GTK_SELECTION_BROWSE );
}

static void
raz_desktop_listview( FMAIEnvironmentTab *instance )
{
	GtkTreeView *listview;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean next_ok;

	listview = GTK_TREE_VIEW( fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "EnvironmentsDesktopTreeView" ));
	model = gtk_tree_view_get_model( listview );

	if( gtk_tree_model_get_iter_first( model, &iter )){
		next_ok = TRUE;
		while( next_ok ){
			gtk_list_store_set( GTK_LIST_STORE( model ), &iter, ENV_BOOL_COLUMN, FALSE, -1 );
			next_ok = gtk_tree_model_iter_next( model, &iter );
		}
	}
}

static void
setup_desktop_listview( FMAIEnvironmentTab *instance, GSList *show )
{
	static const gchar *thisfn = "fma_ienvironment_tab_setup_desktop_listview";
	GtkTreeView *listview;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean next_ok, found;
	GSList *ic;
	gchar *keyword;

	listview = GTK_TREE_VIEW( fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "EnvironmentsDesktopTreeView" ));
	model = gtk_tree_view_get_model( listview );

	for( ic = show ; ic ; ic = ic->next ){
		if( strlen( ic->data )){
			found = FALSE;
			if( gtk_tree_model_get_iter_first( model, &iter )){
				next_ok = TRUE;
				while( next_ok && !found ){
					gtk_tree_model_get( model, &iter, ENV_KEYWORD_COLUMN, &keyword, -1 );
					if( !strcmp( keyword, ic->data )){
						gtk_list_store_set( GTK_LIST_STORE( model ), &iter, ENV_BOOL_COLUMN, TRUE, -1 );
						found = TRUE;
					}
					g_free( keyword );
					if( !found ){
						next_ok = gtk_tree_model_iter_next( model, &iter );
					}
				}
			}
			if( !found ){
				g_warning( "%s: unable to set %s environment", thisfn, ( const gchar * ) ic->data );
			}
		}
	}
}

static void
dispose_desktop_listview( FMAIEnvironmentTab *instance )
{
	GtkWidget *listview;
	GtkTreeModel *model;
	GtkTreeSelection *selection;

	listview = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "EnvironmentsDesktopTreeView" );
	if( GTK_IS_TREE_VIEW( listview )){
		model = gtk_tree_view_get_model( GTK_TREE_VIEW( listview ));
		selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( listview ));
		gtk_tree_selection_unselect_all( selection );
		gtk_list_store_clear( GTK_LIST_STORE( model ));
	}
}

static IEnvironData *
get_ienviron_data( FMAIEnvironmentTab *instance )
{
	IEnvironData *data;

	data = ( IEnvironData * ) g_object_get_data( G_OBJECT( instance ), IENVIRON_TAB_PROP_DATA );

	if( !data ){
		data = g_new0( IEnvironData, 1 );
		g_object_set_data( G_OBJECT( instance ), IENVIRON_TAB_PROP_DATA, data );
	}

	return( data );
}

static void
on_instance_finalized( gpointer user_data, FMAIEnvironmentTab *instance )
{
	static const gchar *thisfn = "fma_ienvironment_tab_on_instance_finalized";
	IEnvironData *data;

	g_debug( "%s: instance=%p, user_data=%p", thisfn, ( void * ) instance, ( void * ) user_data );

	data = get_ienviron_data( instance );
	data->on_selection_change = TRUE;
	dispose_selection_count_combobox( instance );
	dispose_desktop_listview( instance );

	g_free( data );
}
