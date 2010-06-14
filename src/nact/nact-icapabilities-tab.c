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
#include <stdlib.h>

#include <api/na-core-utils.h>
#include <api/na-object-api.h>

#include "nact-gtk-utils.h"
#include "nact-main-tab.h"
#include "nact-match-list.h"
#include "nact-add-capability-dialog.h"
#include "nact-icapabilities-tab.h"

/* private interface data
 */
struct NactICapabilitiesTabInterfacePrivate {
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

#define ITAB_NAME						"capabilities"

static gboolean st_initialized = FALSE;
static gboolean st_finalized = FALSE;

static GType        register_type( void );
static void         interface_base_init( NactICapabilitiesTabInterface *klass );
static void         interface_base_finalize( NactICapabilitiesTabInterface *klass );

static void         runtime_init_connect_signals( NactICapabilitiesTab *instance, GtkTreeView *listview );
static void         on_tab_updatable_selection_changed( NactICapabilitiesTab *instance, gint count_selected );
static void         on_tab_updatable_enable_tab( NactICapabilitiesTab *instance, NAObjectItem *item );
static void         on_selcount_ope_changed( GtkComboBox *combo, NactICapabilitiesTab *instance );
static void         on_selcount_int_changed( GtkEntry *entry, NactICapabilitiesTab *instance );
static void         on_add_clicked( GtkButton *button, BaseWindow *window );
static gboolean     tab_set_sensitive( NactICapabilitiesTab *instance );
static GtkTreeView *get_capabilities_tree_view( NactICapabilitiesTab *instance );
static GSList      *get_capabilities( NAIContext *context );
static void         set_capabilities( NAIContext *context, GSList *list );
static void         init_count_combobox( NactICapabilitiesTab *instance );
static void         set_selection_count_selection( NactICapabilitiesTab *instance, const gchar *ope, const gchar *uint );
static gchar       *get_selection_count_selection( NactICapabilitiesTab *instance );
static void         dispose_count_combobox( NactICapabilitiesTab *instance );

GType
nact_icapabilities_tab_get_type( void )
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
	static const gchar *thisfn = "nact_icapabilities_tab_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NactICapabilitiesTabInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "NactICapabilitiesTab", &info, 0 );

	g_type_interface_add_prerequisite( type, BASE_WINDOW_TYPE );

	return( type );
}

static void
interface_base_init( NactICapabilitiesTabInterface *klass )
{
	static const gchar *thisfn = "nact_icapabilities_tab_interface_base_init";

	if( !st_initialized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NactICapabilitiesTabInterfacePrivate, 1 );

		st_initialized = TRUE;
	}
}

static void
interface_base_finalize( NactICapabilitiesTabInterface *klass )
{
	static const gchar *thisfn = "nact_icapabilities_tab_interface_base_finalize";

	if( st_initialized && !st_finalized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		st_finalized = TRUE;

		g_free( klass->private );
	}
}

void
nact_icapabilities_tab_initial_load_toplevel( NactICapabilitiesTab *instance )
{
	static const gchar *thisfn = "nact_icapabilities_tab_initial_load_toplevel";
	GtkWidget *list, *add, *remove;

	g_return_if_fail( NACT_IS_ICAPABILITIES_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

		init_count_combobox( instance );

		list = base_window_get_widget( BASE_WINDOW( instance ), "CapabilitiesTreeView" );
		add = base_window_get_widget( BASE_WINDOW( instance ), "AddCapabilityButton" );
		remove = base_window_get_widget( BASE_WINDOW( instance ), "RemoveCapabilityButton" );

		nact_match_list_create_model( BASE_WINDOW( instance ),
				ITAB_NAME, TAB_CAPABILITIES,
				list, add, remove,
				( pget_filters ) get_capabilities,
				( pset_filters ) set_capabilities,
				( pon_add_cb ) on_add_clicked,
				MATCH_LIST_MUST_MATCH_ALL_OF,
				_( "Capability filter" ));
	}
}

void
nact_icapabilities_tab_runtime_init_toplevel( NactICapabilitiesTab *instance )
{
	static const gchar *thisfn = "nact_icapabilities_tab_runtime_init_toplevel";
	GtkTreeView *listview;

	g_return_if_fail( NACT_IS_ICAPABILITIES_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

		listview = get_capabilities_tree_view( instance );
		nact_match_list_init_view( BASE_WINDOW( instance ), ITAB_NAME );
		runtime_init_connect_signals( instance, listview );
	}
}

static void
runtime_init_connect_signals( NactICapabilitiesTab *instance, GtkTreeView *listview )
{
	static const gchar *thisfn = "nact_icapabilities_tab_runtime_init_connect_signals";
	GtkWidget *selcount_ope, *selcount_int;

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p, listview=%p", thisfn, ( void * ) instance, ( void * ) listview );
		g_return_if_fail( NACT_IS_ICAPABILITIES_TAB( instance ));

		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( instance ),
				MAIN_WINDOW_SIGNAL_SELECTION_CHANGED,
				G_CALLBACK( on_tab_updatable_selection_changed ));

		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( instance ),
				TAB_UPDATABLE_SIGNAL_ENABLE_TAB,
				G_CALLBACK( on_tab_updatable_enable_tab ));

		selcount_ope = base_window_get_widget( BASE_WINDOW( instance ), "ConditionsCountSigneCombobox" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( selcount_ope ),
				"changed",
				G_CALLBACK( on_selcount_ope_changed ));

		selcount_int = base_window_get_widget( BASE_WINDOW( instance ), "ConditionsCountNumberEntry" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( selcount_int ),
				"changed",
				G_CALLBACK( on_selcount_int_changed ));
	}
}

void
nact_icapabilities_tab_all_widgets_showed( NactICapabilitiesTab *instance )
{
	static const gchar *thisfn = "nact_icapabilities_tab_all_widgets_showed";

	g_return_if_fail( NACT_IS_ICAPABILITIES_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	}
}

void
nact_icapabilities_tab_dispose( NactICapabilitiesTab *instance )
{
	static const gchar *thisfn = "nact_icapabilities_tab_dispose";

	g_return_if_fail( NACT_IS_ICAPABILITIES_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

		dispose_count_combobox( instance );
		nact_match_list_dispose( BASE_WINDOW( instance ), ITAB_NAME );
	}
}

static void
on_tab_updatable_selection_changed( NactICapabilitiesTab *instance, gint count_selected )
{
	static const gchar *thisfn = "nact_icapabilities_tab_on_tab_updatable_selection_changed";
	NAObjectItem *item;
	NAObjectProfile *profile;
	NAIContext *context;
	GSList *capabilities;
	gboolean editable;
	gchar *sel_count;
	gchar *selcount_ope, *selcount_int;
	GtkWidget *combo, *entry;

	capabilities = NULL;
	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p, count_selected=%d", thisfn, ( void * ) instance, count_selected );
		g_return_if_fail( NACT_IS_ICAPABILITIES_TAB( instance ));

		g_object_get(
				G_OBJECT( instance ),
				TAB_UPDATABLE_PROP_EDITED_ACTION, &item,
				TAB_UPDATABLE_PROP_EDITED_PROFILE, &profile,
				TAB_UPDATABLE_PROP_EDITABLE, &editable,
				NULL );

		context = ( profile ? NA_ICONTEXT( profile ) : ( NAIContext * ) item );

		sel_count = context ? na_object_get_selection_count( context ) : g_strdup( ">0" );
		na_core_utils_selcount_get_ope_int( sel_count, &selcount_ope, &selcount_int );
		set_selection_count_selection( instance, selcount_ope, selcount_int );
		g_free( selcount_int );
		g_free( selcount_ope );
		g_free( sel_count );

		combo = base_window_get_widget( BASE_WINDOW( instance ), "ConditionsCountSigneCombobox" );
		gtk_widget_set_sensitive( combo, context != NULL );
		nact_gtk_utils_set_editable( GTK_OBJECT( combo ), editable );

		entry = base_window_get_widget( BASE_WINDOW( instance ), "ConditionsCountNumberEntry" );
		gtk_widget_set_sensitive( entry, context != NULL );
		nact_gtk_utils_set_editable( GTK_OBJECT( entry ), editable );

		nact_match_list_on_selection_changed( BASE_WINDOW( instance ), ITAB_NAME, count_selected );
		tab_set_sensitive( instance );
	}
}

static void
on_tab_updatable_enable_tab( NactICapabilitiesTab *instance, NAObjectItem *item )
{
	static const gchar *thisfn = "nact_icapabilities_tab_on_tab_updatable_enable_tab";

	g_return_if_fail( NACT_IS_ICAPABILITIES_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p, item=%p", thisfn, ( void * ) instance, ( void * ) item );

		tab_set_sensitive( instance );
	}
}

static void
on_selcount_ope_changed( GtkComboBox *combo, NactICapabilitiesTab *instance )
{
	NAObjectItem *edited;
	NAObjectProfile *profile;
	NAIContext *context;
	gboolean editable;
	gchar *selcount;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, &edited,
			TAB_UPDATABLE_PROP_EDITED_PROFILE, &profile,
			TAB_UPDATABLE_PROP_EDITABLE, &editable,
			NULL );

	context = ( profile ? NA_ICONTEXT( profile ) : ( NAIContext * ) edited );

	if( context && editable ){
		selcount = get_selection_count_selection( instance );
		na_object_set_selection_count( context, selcount );
		g_free( selcount );
		g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, edited, FALSE );
	}
}

static void
on_selcount_int_changed( GtkEntry *entry, NactICapabilitiesTab *instance )
{
	NAObjectItem *edited;
	NAObjectProfile *profile;
	NAIContext *context;
	gboolean editable;
	gchar *selcount;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, &edited,
			TAB_UPDATABLE_PROP_EDITED_PROFILE, &profile,
			TAB_UPDATABLE_PROP_EDITABLE, &editable,
			NULL );

	context = ( profile ? NA_ICONTEXT( profile ) : ( NAIContext * ) edited );

	if( context && editable ){
		selcount = get_selection_count_selection( instance );
		na_object_set_selection_count( context, selcount );
		g_free( selcount );
		g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, edited, FALSE );
	}
}

static void
on_add_clicked( GtkButton *button, BaseWindow *window )
{
	NAObjectItem *item;
	NAObjectProfile *profile;
	NAIContext *context;
	GSList *capabilities;
	gchar *new_cap;

	g_object_get(
			G_OBJECT( window ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, &item,
			TAB_UPDATABLE_PROP_EDITED_PROFILE, &profile,
			NULL );

	context = ( profile ? NA_ICONTEXT( profile ) : ( NAIContext * ) item );

	if( context ){
		capabilities = na_object_get_capabilities( context );
		new_cap = nact_add_capability_dialog_run( window, capabilities );
		g_debug( "nact_icapabilities_tab_on_add_clicked: new_cap=%s", new_cap );

		if( new_cap ){
			nact_match_list_insert_row( window, ITAB_NAME, new_cap, FALSE, FALSE );
			g_free( new_cap );
		}
	}
}

static gboolean
tab_set_sensitive( NactICapabilitiesTab *instance )
{
	NAObjectItem *item;
	NAObjectProfile *profile;
	gboolean enable_tab;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, &item,
			TAB_UPDATABLE_PROP_EDITED_PROFILE, &profile,
			NULL );

	enable_tab = ( item != NULL );
	nact_main_tab_enable_page( NACT_MAIN_WINDOW( instance ), TAB_CAPABILITIES, enable_tab );

	return( enable_tab );
}

static GtkTreeView *
get_capabilities_tree_view( NactICapabilitiesTab *instance )
{
	GtkWidget *treeview;

	treeview = base_window_get_widget( BASE_WINDOW( instance ), "CapabilitiesTreeView" );
	g_assert( GTK_IS_TREE_VIEW( treeview ));

	return( GTK_TREE_VIEW( treeview ));
}

static GSList *
get_capabilities( NAIContext *context )
{
	return( na_object_get_capabilities( context ));
}

static void
set_capabilities( NAIContext *context, GSList *list )
{
	na_object_set_capabilities( context, list );
}

static void
init_count_combobox( NactICapabilitiesTab *instance )
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
		gtk_list_store_set( GTK_LIST_STORE( model ), &row, COUNT_LABEL_COLUMN, st_counts[i].label, -1 );
		i += 1;
	}

	combo = GTK_COMBO_BOX( base_window_get_widget( BASE_WINDOW( instance ), "ConditionsCountSigneCombobox" ));
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

static void
set_selection_count_selection( NactICapabilitiesTab *instance, const gchar *ope, const gchar *uint )
{
	GtkComboBox *combo;
	GtkEntry *entry;
	gint i, index;

	combo = GTK_COMBO_BOX( base_window_get_widget( BASE_WINDOW( instance ), "ConditionsCountSigneCombobox" ));

	index = -1;
	for( i=0 ; st_counts[i].sign && index==-1 ; ++i ){
		if( !strcmp( st_counts[i].sign, ope )){
			index = i;
		}
	}
	gtk_combo_box_set_active( combo, index );

	entry = GTK_ENTRY( base_window_get_widget( BASE_WINDOW( instance ), "ConditionsCountNumberEntry" ));
	gtk_entry_set_text( entry, uint );
}

static gchar *
get_selection_count_selection( NactICapabilitiesTab *instance )
{
	GtkComboBox *combo;
	GtkEntry *entry;
	gint index;
	gchar *uints, *selcount;
	guint uinti;

	combo = GTK_COMBO_BOX( base_window_get_widget( BASE_WINDOW( instance ), "ConditionsCountSigneCombobox" ));
	index = gtk_combo_box_get_active( combo );
	if( index == -1 ){
		return( NULL );
	}

	entry = GTK_ENTRY( base_window_get_widget( BASE_WINDOW( instance ), "ConditionsCountNumberEntry" ));
	uinti = abs( atoi( gtk_entry_get_text( entry )));
	uints = g_strdup_printf( "%d", uinti );
	gtk_entry_set_text( entry, uints );
	g_free( uints );

	selcount = g_strdup_printf( "%s%d", st_counts[index].sign, uinti );

	return( selcount );
}

static void
dispose_count_combobox( NactICapabilitiesTab *instance )
{
	GtkComboBox *combo;
	GtkTreeModel *model;

	combo = GTK_COMBO_BOX( base_window_get_widget( BASE_WINDOW( instance ), "ConditionsCountSigneCombobox" ));
	model = gtk_combo_box_get_model( combo );
	gtk_list_store_clear( GTK_LIST_STORE( model ));
}
