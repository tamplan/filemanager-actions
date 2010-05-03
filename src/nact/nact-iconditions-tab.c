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
#include <string.h>

#include <api/na-core-utils.h>
#include <api/na-object-api.h>

#include "base-iprefs.h"
#include "nact-main-window.h"
#include "nact-main-tab.h"
#include "nact-gtk-utils.h"
#include "nact-iconditions-tab.h"

/* private interface data
 */
struct NactIConditionsTabInterfacePrivate {
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
		{ N_( "<" ), N_( "(strictly lesser than)" ) },
		{ N_( "=" ), N_( "(equal to)" ) },
		{ N_( ">" ), N_( "(strictly greater than)" ) },
		{ NULL }
};

static gboolean st_initialized = FALSE;
static gboolean st_finalized = FALSE;
static gboolean st_on_selection_change = FALSE;

static GType      register_type( void );
static void       interface_base_init( NactIConditionsTabInterface *klass );
static void       interface_base_finalize( NactIConditionsTabInterface *klass );

static void       on_tab_updatable_selection_changed( NactIConditionsTab *instance, gint count_selected );
static void       on_tab_updatable_enable_tab( NactIConditionsTab *instance, NAObjectItem *item );
static gboolean   tab_set_sensitive( NactIConditionsTab *instance );

static GtkWidget *get_basenames_entry( NactIConditionsTab *instance );
static GtkButton *get_both_button( NactIConditionsTab *instance );
static GtkButton *get_isdir_button( NactIConditionsTab *instance );
static GtkButton *get_isfile_button( NactIConditionsTab *instance );
static GtkButton *get_matchcase_button( NactIConditionsTab *instance );
static GtkWidget *get_mimetypes_entry( NactIConditionsTab *instance );
static GtkButton *get_multiple_button( NactIConditionsTab *instance );
static void       on_basenames_changed( GtkEntry *entry, NactIConditionsTab *instance );
static void       on_isfiledir_toggled( GtkToggleButton *button, NactIConditionsTab *instance );
static void       on_matchcase_toggled( GtkToggleButton *button, NactIConditionsTab *instance );
static void       on_mimetypes_changed( GtkEntry *entry, NactIConditionsTab *instance );

static void       on_multiple_toggled( GtkToggleButton *button, NactIConditionsTab *instance );
static void       init_count_combobox( NactIConditionsTab *instance );
static void       dispose_count_combobox( NactIConditionsTab *instance );

static void       set_isfiledir( NactIConditionsTab *instance, gboolean isfile, gboolean isdir, gboolean readonly );

GType
nact_iconditions_tab_get_type( void )
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
	static const gchar *thisfn = "nact_iconditions_tab_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NactIConditionsTabInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "NactIConditionsTab", &info, 0 );

	g_type_interface_add_prerequisite( type, BASE_WINDOW_TYPE );

	return( type );
}

static void
interface_base_init( NactIConditionsTabInterface *klass )
{
	static const gchar *thisfn = "nact_iconditions_tab_interface_base_init";

	if( !st_initialized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NactIConditionsTabInterfacePrivate, 1 );

		st_initialized = TRUE;
	}
}

static void
interface_base_finalize( NactIConditionsTabInterface *klass )
{
	static const gchar *thisfn = "nact_iconditions_tab_interface_base_finalize";

	if( st_initialized && !st_finalized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		st_finalized = TRUE;

		g_free( klass->private );
	}
}

void
nact_iconditions_tab_initial_load_toplevel( NactIConditionsTab *instance )
{
	static const gchar *thisfn = "nact_iconditions_tab_initial_load_toplevel";

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_ICONDITIONS_TAB( instance ));

	if( st_initialized && !st_finalized ){

		init_count_combobox( instance );
	}
}

void
nact_iconditions_tab_runtime_init_toplevel( NactIConditionsTab *instance )
{
	static const gchar *thisfn = "nact_iconditions_tab_runtime_init_toplevel";
	GtkWidget *basenames_widget;
	GtkButton *matchcase_button;
	GtkWidget *mimetypes_widget;
	GtkButton *isfile_button, *isdir_button, *both_button;
	GtkButton *multiple_button;

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_ICONDITIONS_TAB( instance ));

	if( st_initialized && !st_finalized ){

		basenames_widget = get_basenames_entry( instance );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( basenames_widget ),
				"changed",
				G_CALLBACK( on_basenames_changed ));

		matchcase_button = get_matchcase_button( instance );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( matchcase_button ),
				"toggled",
				G_CALLBACK( on_matchcase_toggled ));

		mimetypes_widget = get_mimetypes_entry( instance );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( mimetypes_widget ),
				"changed",
				G_CALLBACK( on_mimetypes_changed ));

		isfile_button = get_isfile_button( instance );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( isfile_button ),
				"toggled",
				G_CALLBACK( on_isfiledir_toggled ));

		isdir_button = get_isdir_button( instance );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( isdir_button ),
				"toggled",
				G_CALLBACK( on_isfiledir_toggled ));

		both_button = get_both_button( instance );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( both_button ),
				"toggled",
				G_CALLBACK( on_isfiledir_toggled ));

		multiple_button = get_multiple_button( instance );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( multiple_button ),
				"toggled",
				G_CALLBACK( on_multiple_toggled ));

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
	}
}

void
nact_iconditions_tab_all_widgets_showed( NactIConditionsTab *instance )
{
	static const gchar *thisfn = "nact_iconditions_tab_all_widgets_showed";

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_ICONDITIONS_TAB( instance ));

	if( st_initialized && !st_finalized ){
	}
}

void
nact_iconditions_tab_dispose( NactIConditionsTab *instance )
{
	static const gchar *thisfn = "nact_iconditions_tab_dispose";

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_ICONDITIONS_TAB( instance ));

	if( st_initialized && !st_finalized ){

		dispose_count_combobox( instance );
	}
}

void
nact_iconditions_tab_get_isfiledir( NactIConditionsTab *instance, gboolean *isfile, gboolean *isdir )
{
	gboolean both;

	g_return_if_fail( NACT_IS_ICONDITIONS_TAB( instance ));
	g_return_if_fail( isfile );
	g_return_if_fail( isdir );

	if( st_initialized && !st_finalized ){

		both = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( get_both_button( instance )));
		if( both ){
			*isfile = TRUE;
			*isdir = TRUE;
		} else {
			*isfile = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( get_isfile_button( instance )));
			*isdir = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( get_isdir_button( instance )));
		}
	}
}

gboolean
nact_iconditions_tab_get_multiple( NactIConditionsTab *instance )
{
	gboolean multiple = FALSE;
	GtkButton *button;

	g_return_val_if_fail( NACT_IS_ICONDITIONS_TAB( instance ), FALSE );

	if( st_initialized && !st_finalized ){
		button = get_multiple_button( instance );
		multiple = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button ));
	}

	return( multiple );
}

static void
on_tab_updatable_selection_changed( NactIConditionsTab *instance, gint count_selected )
{
	static const gchar *thisfn = "nact_iconditions_tab_on_tab_updatable_selection_changed";
	NAObjectItem *item;
	NAObjectProfile *profile;
	gboolean enable_tab;
	GtkWidget *basenames_widget, *mimetypes_widget;
	GSList *basenames, *mimetypes;
	gchar *basenames_text, *mimetypes_text;
	GtkButton *matchcase_button;
	gboolean matchcase;
	gboolean isfile, isdir;
	GtkButton *multiple_button;
	gboolean multiple;
	gboolean editable;

	g_debug( "%s: instance=%p, count_selected=%d", thisfn, ( void * ) instance, count_selected );
	g_return_if_fail( NACT_IS_ICONDITIONS_TAB( instance ));

	if( st_initialized && !st_finalized ){

		st_on_selection_change = TRUE;

		g_object_get(
				G_OBJECT( instance ),
				TAB_UPDATABLE_PROP_EDITED_ACTION, &item,
				TAB_UPDATABLE_PROP_EDITED_PROFILE, &profile,
				TAB_UPDATABLE_PROP_EDITABLE, &editable,
				NULL );

		enable_tab = tab_set_sensitive( instance );

		basenames_widget = get_basenames_entry( instance );
		basenames = profile ? na_object_get_basenames( profile ) : NULL;
		basenames_text = profile ? na_core_utils_slist_to_text( basenames ) : g_strdup( "" );
		gtk_entry_set_text( GTK_ENTRY( basenames_widget ), basenames_text );
		g_free( basenames_text );
		na_core_utils_slist_free( basenames );
		gtk_widget_set_sensitive( basenames_widget, item != NULL );
		nact_gtk_utils_set_editable( GTK_OBJECT( basenames_widget ), editable );

		matchcase_button = get_matchcase_button( instance );
		matchcase = profile ? na_object_get_matchcase( profile ) : FALSE;
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( matchcase_button ), matchcase );
		gtk_widget_set_sensitive( GTK_WIDGET( matchcase_button ), item != NULL );
		nact_gtk_utils_set_editable( GTK_OBJECT( matchcase_button ), editable );

		mimetypes_widget = get_mimetypes_entry( instance );
		mimetypes = profile ? na_object_get_mimetypes( profile ) : NULL;
		mimetypes_text = profile ? na_core_utils_slist_to_text( mimetypes ) : g_strdup( "" );
		gtk_entry_set_text( GTK_ENTRY( mimetypes_widget ), mimetypes_text );
		g_free( mimetypes_text );
		na_core_utils_slist_free( mimetypes );
		gtk_widget_set_sensitive( mimetypes_widget, item != NULL );
		nact_gtk_utils_set_editable( GTK_OBJECT( mimetypes_widget ), editable );

		isfile = profile ? na_object_is_file( profile ) : FALSE;
		isdir = profile ? na_object_is_dir( profile ) : FALSE;
		set_isfiledir( instance, isfile, isdir, editable );

		multiple_button = get_multiple_button( instance );
		multiple = profile ? na_object_is_multiple( profile ) : FALSE;
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( multiple_button ), multiple );
		gtk_widget_set_sensitive( GTK_WIDGET( multiple_button ), item != NULL );
		nact_gtk_utils_set_editable( GTK_OBJECT( multiple_button ), editable );

		st_on_selection_change = FALSE;
	}
}

static void
on_tab_updatable_enable_tab( NactIConditionsTab *instance, NAObjectItem *item )
{
	static const gchar *thisfn = "nact_iconditions_tab_on_tab_updatable_enable_tab";

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p, item=%p", thisfn, ( void * ) instance, ( void * ) item );
		g_return_if_fail( NACT_IS_ICONDITIONS_TAB( instance ));

		tab_set_sensitive( instance );
	}
}

static gboolean
tab_set_sensitive( NactIConditionsTab *instance )
{
	NAObjectItem *item;
	NAObjectProfile *profile;
	gboolean enable_tab;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, &item,
			TAB_UPDATABLE_PROP_EDITED_PROFILE, &profile,
			NULL );

	enable_tab = ( profile != NULL && na_object_is_target_selection( NA_OBJECT_ACTION( item )));
	nact_main_tab_enable_page( NACT_MAIN_WINDOW( instance ), TAB_CONDITIONS, enable_tab );

	return( enable_tab );
}

static GtkWidget *
get_basenames_entry( NactIConditionsTab *instance )
{
	return( base_window_get_widget( BASE_WINDOW( instance ), "ConditionsFilenamesEntry" ));
}

static GtkButton *
get_both_button( NactIConditionsTab *instance )
{
	return( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( instance ), "ConditionsBothButton" )));
}

static GtkButton *
get_isdir_button( NactIConditionsTab *instance )
{
	return( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( instance ), "ConditionsOnlyFoldersButton" )));
}

static GtkButton *
get_isfile_button( NactIConditionsTab *instance )
{
	return( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( instance ), "ConditionsOnlyFilesButton" )));
}

static GtkButton *
get_matchcase_button( NactIConditionsTab *instance )
{
	return( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( instance ), "ConditionsMatchcaseButton" )));
}

static GtkWidget *
get_mimetypes_entry( NactIConditionsTab *instance )
{
	return( base_window_get_widget( BASE_WINDOW( instance ), "ConditionsMimetypesEntry" ));
}

static GtkButton *
get_multiple_button( NactIConditionsTab *instance )
{
	return( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( instance ), "ConditionsMultipleButton" )));
}

static void
on_basenames_changed( GtkEntry *entry, NactIConditionsTab *instance )
{
	NAObjectProfile *edited;
	const gchar *text;
	GSList *basenames;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_PROFILE, &edited,
			NULL );

	if( edited ){
		text = gtk_entry_get_text( entry );
		basenames = na_core_utils_slist_from_split( text, ";" );
		na_object_set_basenames( edited, basenames );
		na_core_utils_slist_free( basenames );
		g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, edited, FALSE );
	}
}

/*
 * Note that this callback is triggered twice: first, for the
 * deactivated button, then a second time for the newly activated one.
 * To avoid a double execution, we only run the code when the button
 * becomes active
 */
static void
on_isfiledir_toggled( GtkToggleButton *button, NactIConditionsTab *instance )
{
	/*static const gchar *thisfn = "nact_iconditions_tab_on_isfiledir_toggled";*/
	NAObjectProfile *edited;
	gboolean isfile, isdir;
	gboolean editable;

	if( !st_on_selection_change ){

		if( gtk_toggle_button_get_active( button )){

			g_object_get(
					G_OBJECT( instance ),
					TAB_UPDATABLE_PROP_EDITED_PROFILE, &edited,
					TAB_UPDATABLE_PROP_EDITABLE, &editable,
					NULL );

			g_return_if_fail( NA_IS_OBJECT_PROFILE( edited ));

			if( editable ){
				nact_iconditions_tab_get_isfiledir( instance, &isfile, &isdir );
				na_object_set_isfile( edited, isfile );
				na_object_set_isdir( edited, isdir );
				g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, edited, FALSE );

			} else if( gtk_toggle_button_get_active( button )){
				g_signal_handlers_block_by_func(( gpointer ) button, on_isfiledir_toggled, instance );
				isfile = na_object_is_file( edited );
				isdir = na_object_is_dir( edited );
				set_isfiledir( instance, isfile, isdir, !editable );
				g_signal_handlers_unblock_by_func(( gpointer ) button, on_isfiledir_toggled, instance );
			}
		}
	}
}

static void
on_matchcase_toggled( GtkToggleButton *button, NactIConditionsTab *instance )
{
	NAObjectProfile *edited;
	gboolean matchcase;
	gboolean editable;

	if( !st_on_selection_change ){

		g_object_get(
				G_OBJECT( instance ),
				TAB_UPDATABLE_PROP_EDITED_PROFILE, &edited,
				TAB_UPDATABLE_PROP_EDITABLE, &editable,
				NULL );

		if( edited ){

			matchcase = gtk_toggle_button_get_active( button );

			if( editable ){
				na_object_set_matchcase( edited, matchcase );
				g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, edited, FALSE );

			} else {
				g_signal_handlers_block_by_func(( gpointer ) button, on_matchcase_toggled, instance );
				gtk_toggle_button_set_active( button, !matchcase );
				g_signal_handlers_unblock_by_func(( gpointer ) button, on_matchcase_toggled, instance );
			}
		}
	}
}

static void
on_mimetypes_changed( GtkEntry *entry, NactIConditionsTab *instance )
{
	NAObjectProfile *edited;
	const gchar *text;
	GSList *mimetypes;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_PROFILE, &edited,
			NULL );

	if( edited ){
		text = gtk_entry_get_text( entry );
		mimetypes = na_core_utils_slist_from_split( text, ";" );
		na_object_set_mimetypes( edited, mimetypes );
		na_core_utils_slist_free( mimetypes );
		g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, edited, FALSE );
	}
}

static void
on_multiple_toggled( GtkToggleButton *button, NactIConditionsTab *instance )
{
	NAObjectProfile *edited;
	gboolean multiple;
	gboolean editable;

	if( !st_on_selection_change ){

		g_object_get(
				G_OBJECT( instance ),
				TAB_UPDATABLE_PROP_EDITED_PROFILE, &edited,
				TAB_UPDATABLE_PROP_EDITABLE, &editable,
				NULL );

		if( edited ){
			multiple = gtk_toggle_button_get_active( button );

			if( editable ){
				na_object_set_multiple( edited, multiple );
				g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, edited, FALSE );

			} else {
				g_signal_handlers_block_by_func(( gpointer ) button, on_multiple_toggled, instance );
				gtk_toggle_button_set_active( button, !multiple );
				g_signal_handlers_unblock_by_func(( gpointer ) button, on_multiple_toggled, instance );
			}
		}
	}
}

static void
init_count_combobox( NactIConditionsTab *instance )
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
dispose_count_combobox( NactIConditionsTab *instance )
{
	GtkComboBox *combo;
	GtkTreeModel *model;

	combo = GTK_COMBO_BOX( base_window_get_widget( BASE_WINDOW( instance ), "ConditionsCountSigneCombobox" ));
	model = gtk_combo_box_get_model( combo );
	gtk_list_store_clear( GTK_LIST_STORE( model ));
}

static void
set_isfiledir( NactIConditionsTab *instance, gboolean isfile, gboolean isdir, gboolean readonly )
{
	GtkButton *both_button;
	GtkButton *file_button;
	GtkButton *dirs_button;

	both_button = get_both_button( instance );
	file_button = get_isfile_button( instance );
	dirs_button = get_isdir_button( instance );

	if( isfile && isdir ){
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( both_button ), TRUE );

	} else if( isfile ){
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( file_button ), TRUE );

	} else if( isdir ){
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( dirs_button ), TRUE );
	}

	nact_gtk_utils_set_editable( GTK_OBJECT( both_button ), !readonly );
	nact_gtk_utils_set_editable( GTK_OBJECT( file_button ), !readonly );
	nact_gtk_utils_set_editable( GTK_OBJECT( dirs_button ), !readonly );
}
