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

#include <core/na-iprefs.h>
#include <core/na-factory-object.h>
#include <core/na-tokens.h>

#include "base-window.h"
#include "base-iprefs.h"
#include "nact-application.h"
#include "nact-main-statusbar.h"
#include "nact-gtk-utils.h"
#include "nact-iprefs.h"
#include "nact-iactions-list.h"
#include "nact-main-tab.h"
#include "nact-icommand-tab.h"
#include "nact-ischemes-tab.h"

/* private interface data
 */
struct NactICommandTabInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* the GConf key used to read/write size and position of auxiliary dialogs
 */
#define IPREFS_LEGEND_DIALOG				"icommand-legend-dialog"
#define IPREFS_COMMAND_CHOOSER				"icommand-command-chooser"
#define IPREFS_FOLDER_URI					"icommand-folder-uri"
#define IPREFS_WORKING_DIR_DIALOG			"icommand-working-dir-dialog"
#define IPREFS_WORKING_DIR_URI				"icommand-working-dir-uri"

/* a data set in the LegendDialog GObject
 */
#define ICOMMAND_TAB_LEGEND_VISIBLE			"nact-icommand-tab-legend-dialog-visible"
#define ICOMMAND_TAB_STATUSBAR_CONTEXT		"nact-icommand-tab-statusbar-context"

static gboolean  st_initialized = FALSE;
static gboolean  st_finalized = FALSE;
static gboolean  st_on_selection_change = FALSE;
static NATokens *st_tokens = NULL;

static GType      register_type( void );
static void       interface_base_init( NactICommandTabInterface *klass );
static void       interface_base_finalize( NactICommandTabInterface *klass );

static void       on_iactions_list_column_edited( NactICommandTab *instance, NAObject *object, gchar *text, gint column );
static void       on_tab_updatable_selection_changed( NactICommandTab *instance, gint count_selected );

static GtkWidget *get_label_entry( NactICommandTab *instance );
static GtkButton *get_legend_button( NactICommandTab *instance );
static GtkWindow *get_legend_dialog( NactICommandTab *instance );
static GtkWidget *get_parameters_entry( NactICommandTab *instance );
static GtkButton *get_path_button( NactICommandTab *instance );
static GtkWidget *get_path_entry( NactICommandTab *instance );
static void       legend_dialog_show( NactICommandTab *instance );
static void       legend_dialog_hide( NactICommandTab *instance );
static void       on_label_changed( GtkEntry *entry, NactICommandTab *instance );
static void       on_legend_clicked( GtkButton *button, NactICommandTab *instance );
static gboolean   on_legend_dialog_deleted( GtkWidget *dialog, GdkEvent *event, NactICommandTab *instance );
static void       on_parameters_changed( GtkEntry *entry, NactICommandTab *instance );
static void       on_path_browse( GtkButton *button, NactICommandTab *instance );
static void       on_path_changed( GtkEntry *entry, NactICommandTab *instance );
static void       on_wdir_browse( GtkButton *button, NactICommandTab *instance );
static void       on_wdir_changed( GtkEntry *entry, NactICommandTab *instance );
static gchar     *parse_parameters( NactICommandTab *instance );
static void       update_example_label( NactICommandTab *instance, NAObjectProfile *profile );

GType
nact_icommand_tab_get_type( void )
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
	static const gchar *thisfn = "nact_icommand_tab_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NactICommandTabInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "NactICommandTab", &info, 0 );

	g_type_interface_add_prerequisite( type, BASE_WINDOW_TYPE );

	return( type );
}

static void
interface_base_init( NactICommandTabInterface *klass )
{
	static const gchar *thisfn = "nact_icommand_tab_interface_base_init";

	if( !st_initialized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NactICommandTabInterfacePrivate, 1 );

		st_initialized = TRUE;
	}
}

static void
interface_base_finalize( NactICommandTabInterface *klass )
{
	static const gchar *thisfn = "nact_icommand_tab_interface_base_finalize";

	if( st_initialized && !st_finalized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		st_finalized = TRUE;

		g_free( klass->private );
	}
}

/**
 * nact_icommand_tab_initial_load:
 * @window: this #NactICommandTab instance.
 *
 * Initializes the tab widget at initial load.
 *
 * The GConf preference keys used in this tab were misnamed from v1.11.1
 * up to and including v1.12.0. Starting with v1.12.1, these are migrated
 * here, so that the normal code only makes use of 'good' keys.
 */
void
nact_icommand_tab_initial_load_toplevel( NactICommandTab *instance )
{
	static const gchar *thisfn = "nact_icommand_tab_initial_load_toplevel";

	g_return_if_fail( NACT_IS_ICOMMAND_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

		nact_iprefs_migrate_key( BASE_WINDOW( instance ), "iconditions-legend-dialog", IPREFS_LEGEND_DIALOG );
		nact_iprefs_migrate_key( BASE_WINDOW( instance ), "iconditions-command-chooser", IPREFS_COMMAND_CHOOSER );
		nact_iprefs_migrate_key( BASE_WINDOW( instance ), "iconditions-folder-uri", IPREFS_FOLDER_URI );
	}
}

/**
 * nact_icommand_tab_runtime_init:
 * @window: this #NactICommandTab instance.
 *
 * Initializes the tab widget at each time the widget will be displayed.
 * Connect signals and setup runtime values.
 */
void
nact_icommand_tab_runtime_init_toplevel( NactICommandTab *instance )
{
	static const gchar *thisfn = "nact_icommand_tab_runtime_init_toplevel";
	GtkWindow *legend_dialog;
	GtkWidget *label_entry, *path_entry, *parameters_entry, *wdir_entry;
	GtkButton *path_button, *legend_button, *wdir_button;

	g_return_if_fail( NACT_IS_ICOMMAND_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

		label_entry = get_label_entry( instance );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( label_entry ),
				"changed",
				G_CALLBACK( on_label_changed ));

		path_entry = get_path_entry( instance );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( path_entry ),
				"changed",
				G_CALLBACK( on_path_changed ));

		path_button = get_path_button( instance );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( path_button ),
				"clicked",
				G_CALLBACK( on_path_browse ));

		parameters_entry = get_parameters_entry( instance );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( parameters_entry ),
				"changed",
				G_CALLBACK( on_parameters_changed ));

		legend_button = get_legend_button( instance );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( legend_button ),
				"clicked",
				G_CALLBACK( on_legend_clicked ));

		legend_dialog = get_legend_dialog( instance );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( legend_dialog ),
				"delete-event",
				G_CALLBACK( on_legend_dialog_deleted ));

		wdir_entry = base_window_get_widget( BASE_WINDOW( instance ), "WorkingDirectoryEntry" );
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( wdir_entry ),
				"changed",
				G_CALLBACK( on_wdir_changed ));

		wdir_button = GTK_BUTTON( base_window_get_widget( BASE_WINDOW( instance ), "CommandWorkingDirectoryButton" ));
		base_window_signal_connect(
				BASE_WINDOW( instance ),
				G_OBJECT( wdir_button ),
				"clicked",
				G_CALLBACK( on_wdir_browse ));

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

		/* allocate a static fake NATokens object which will be user to build
		 * the example label - this object will be unreffed on dispose
		 */
		if( !st_tokens ){
			st_tokens = na_tokens_new_for_example();
		}
	}
}

void
nact_icommand_tab_all_widgets_showed( NactICommandTab *instance )
{
	static const gchar *thisfn = "nact_icommand_tab_all_widgets_showed";

	g_return_if_fail( NACT_IS_ICOMMAND_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	}
}

/**
 * nact_icommand_tab_dispose:
 * @window: this #NactICommandTab instance.
 *
 * Called at instance_dispose time.
 */
void
nact_icommand_tab_dispose( NactICommandTab *instance )
{
	static const gchar *thisfn = "nact_icommand_tab_dispose";

	g_return_if_fail( NACT_IS_ICOMMAND_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

		legend_dialog_hide( instance );

		if( st_tokens ){
			g_object_unref( st_tokens );
		}
	}
}

static void
on_iactions_list_column_edited( NactICommandTab *instance, NAObject *object, gchar *text, gint column )
{
	GtkWidget *label_widget;

	g_return_if_fail( NACT_IS_ICOMMAND_TAB( instance ));

	if( st_initialized && !st_finalized ){

		if( NA_IS_OBJECT_PROFILE( object )){
			label_widget = get_label_entry( instance );
			gtk_entry_set_text( GTK_ENTRY( label_widget ), text );
		}
	}
}

static void
on_tab_updatable_selection_changed( NactICommandTab *instance, gint count_selected )
{
	static const gchar *thisfn = "nact_icommand_tab_on_tab_updatable_selection_changed";
	NAObjectProfile *profile;
	gboolean editable;
	gboolean enable_tab;
	GtkWidget *label_entry, *path_entry, *parameters_entry, *wdir_entry;
	gchar *label, *path, *parameters, *wdir;
	GtkButton *path_button, *wdir_button;
	GtkButton *legend_button;

	g_return_if_fail( NACT_IS_ICOMMAND_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p, count_selected=%d", thisfn, ( void * ) instance, count_selected );

		g_object_get(
				G_OBJECT( instance ),
				TAB_UPDATABLE_PROP_SELECTED_PROFILE, &profile,
				TAB_UPDATABLE_PROP_EDITABLE, &editable,
				NULL );

		enable_tab = ( profile != NULL );
		nact_main_tab_enable_page( NACT_MAIN_WINDOW( instance ), TAB_COMMAND, enable_tab );

		st_on_selection_change = TRUE;

		label_entry = get_label_entry( instance );
		label = profile ? na_object_get_label( profile ) : g_strdup( "" );
		label = label ? label : g_strdup( "" );
		gtk_entry_set_text( GTK_ENTRY( label_entry ), label );
		g_free( label );
		gtk_widget_set_sensitive( label_entry, profile != NULL );
		nact_gtk_utils_set_editable( GTK_WIDGET( label_entry ), editable );

		path_entry = get_path_entry( instance );
		path = profile ? na_object_get_path( profile ) : g_strdup( "" );
		path = path ? path : g_strdup( "" );
		gtk_entry_set_text( GTK_ENTRY( path_entry ), path );
		g_free( path );
		gtk_widget_set_sensitive( path_entry, profile != NULL );
		nact_gtk_utils_set_editable( GTK_WIDGET( path_entry ), editable );

		path_button = get_path_button( instance );
		gtk_widget_set_sensitive( GTK_WIDGET( path_button ), profile != NULL );
		nact_gtk_utils_set_editable( GTK_WIDGET( path_button ), editable );

		parameters_entry = get_parameters_entry( instance );
		parameters = profile ? na_object_get_parameters( profile ) : g_strdup( "" );
		parameters = parameters ? parameters : g_strdup( "" );
		gtk_entry_set_text( GTK_ENTRY( parameters_entry ), parameters );
		g_free( parameters );
		gtk_widget_set_sensitive( parameters_entry, profile != NULL );
		nact_gtk_utils_set_editable( GTK_WIDGET( parameters_entry ), editable );

		legend_button = get_legend_button( instance );
		gtk_widget_set_sensitive( GTK_WIDGET( legend_button ), profile != NULL );

		update_example_label( instance, profile );

		wdir_entry = base_window_get_widget( BASE_WINDOW( instance ), "WorkingDirectoryEntry" );
		wdir = profile ? na_object_get_working_dir( profile ) : g_strdup( "" );
		wdir = wdir ? wdir : g_strdup( "" );
		gtk_entry_set_text( GTK_ENTRY( wdir_entry ), wdir );
		g_free( wdir );
		gtk_widget_set_sensitive( wdir_entry, profile != NULL );
		nact_gtk_utils_set_editable( GTK_WIDGET( wdir_entry ), editable );

		wdir_button = GTK_BUTTON( base_window_get_widget( BASE_WINDOW( instance ), "CommandWorkingDirectoryButton" ));
		gtk_widget_set_sensitive( GTK_WIDGET( wdir_button ), profile != NULL );
		nact_gtk_utils_set_editable( GTK_WIDGET( wdir_button ), editable );

		st_on_selection_change = FALSE;
	}
}

static GtkWidget *
get_label_entry( NactICommandTab *instance )
{
	return( base_window_get_widget( BASE_WINDOW( instance ), "ProfileLabelEntry" ));
}

static GtkButton *
get_legend_button( NactICommandTab *instance )
{
	return( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( instance ), "CommandLegendButton" )));
}

static GtkWindow *
get_legend_dialog( NactICommandTab *instance )
{
	return( base_window_get_named_toplevel( BASE_WINDOW( instance ), "LegendDialog" ));
}

static GtkWidget *
get_parameters_entry( NactICommandTab *instance )
{
	return( base_window_get_widget( BASE_WINDOW( instance ), "CommandParametersEntry" ));
}

static GtkButton *
get_path_button( NactICommandTab *instance )
{
	return( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( instance ), "CommandPathButton" )));
}

static GtkWidget *
get_path_entry( NactICommandTab *instance )
{
	return( base_window_get_widget( BASE_WINDOW( instance ), "CommandPathEntry" ));
}

static void
legend_dialog_hide( NactICommandTab *instance )
{
	GtkWindow *legend_dialog;
	GtkButton *legend_button;
	gboolean is_visible;

	legend_dialog = get_legend_dialog( instance );
	is_visible = ( gboolean ) GPOINTER_TO_INT(
			g_object_get_data( G_OBJECT( legend_dialog ), ICOMMAND_TAB_LEGEND_VISIBLE ));

	if( is_visible ){
		g_return_if_fail( GTK_IS_WINDOW( legend_dialog ));
		base_iprefs_save_named_window_position( BASE_WINDOW( instance ), legend_dialog, IPREFS_LEGEND_DIALOG );
		gtk_widget_hide( GTK_WIDGET( legend_dialog ));

		/* set the legend button state consistent for when the dialog is
		 * hidden by another way (eg. close the edit profile dialog)
		 */
		legend_button = get_legend_button( instance );
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( legend_button ), FALSE );

		g_object_set_data( G_OBJECT( legend_dialog ), ICOMMAND_TAB_LEGEND_VISIBLE, GINT_TO_POINTER( FALSE ));
	}
}

static void
legend_dialog_show( NactICommandTab *instance )
{
	GtkWindow *legend_dialog;
	GtkWindow *toplevel;

	legend_dialog = get_legend_dialog( instance );
	gtk_window_set_deletable( legend_dialog, FALSE );

	toplevel = base_window_get_toplevel( BASE_WINDOW( instance ));
	gtk_window_set_transient_for( GTK_WINDOW( legend_dialog ), toplevel );

	base_iprefs_position_named_window( BASE_WINDOW( instance ), legend_dialog, IPREFS_LEGEND_DIALOG );
	gtk_widget_show( GTK_WIDGET( legend_dialog ));

	g_object_set_data( G_OBJECT( legend_dialog ), ICOMMAND_TAB_LEGEND_VISIBLE, GINT_TO_POINTER( TRUE ));
}

static void
on_label_changed( GtkEntry *entry, NactICommandTab *instance )
{
	NAObjectProfile *profile;
	const gchar *label;

	if( !st_on_selection_change ){
		g_object_get(
				G_OBJECT( instance ),
				TAB_UPDATABLE_PROP_SELECTED_PROFILE, &profile,
				NULL );

		if( profile ){
			label = gtk_entry_get_text( entry );
			na_object_set_label( profile, label );
			g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, profile, TRUE );
		}
	}
}

static void
on_legend_clicked( GtkButton *button, NactICommandTab *instance )
{
	if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button ))){
		legend_dialog_show( instance );

	} else {
		legend_dialog_hide( instance );
	}
}

static gboolean
on_legend_dialog_deleted( GtkWidget *dialog, GdkEvent *event, NactICommandTab *instance )
{
	/*g_debug( "on_legend_dialog_deleted" );*/
	legend_dialog_hide( instance );
	return( TRUE );
}

static void
on_parameters_changed( GtkEntry *entry, NactICommandTab *instance )
{
	NAObjectProfile *profile;

	if( !st_on_selection_change ){
		g_object_get(
				G_OBJECT( instance ),
				TAB_UPDATABLE_PROP_SELECTED_PROFILE, &profile,
				NULL );

		if( profile ){
			na_object_set_parameters( profile, gtk_entry_get_text( entry ));
			g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, profile, FALSE );
			update_example_label( instance, profile );
		}
	}
}

static void
on_path_browse( GtkButton *button, NactICommandTab *instance )
{
	nact_gtk_utils_select_file(
			BASE_WINDOW( instance ),
			_( "Choosing a command" ), IPREFS_COMMAND_CHOOSER,
			get_path_entry( instance ), IPREFS_FOLDER_URI, "file:///bin" );
}

static void
on_path_changed( GtkEntry *entry, NactICommandTab *instance )
{
	NAObjectProfile *profile;

	if( !st_on_selection_change ){
		g_object_get(
				G_OBJECT( instance ),
				TAB_UPDATABLE_PROP_SELECTED_PROFILE, &profile,
				NULL );

		if( profile ){
			na_object_set_path( profile, gtk_entry_get_text( entry ));
			g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, profile, FALSE );
			update_example_label( instance, profile );
		}
	}
}

static void
on_wdir_browse( GtkButton *button, NactICommandTab *instance )
{
	GtkWidget *wdir_entry;
	NAObjectProfile *profile;
	gchar *default_value;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_SELECTED_PROFILE, &profile,
			NULL );

	if( profile ){
		default_value = na_factory_object_get_default( NA_IFACTORY_OBJECT( profile ), NAFO_DATA_WORKING_DIR );
		wdir_entry = base_window_get_widget( BASE_WINDOW( instance ), "WorkingDirectoryEntry" );

		nact_gtk_utils_select_dir(
				BASE_WINDOW( instance ),
				_( "Choosing a working directory" ), IPREFS_WORKING_DIR_DIALOG,
				wdir_entry, IPREFS_WORKING_DIR_URI, default_value );

		g_free( default_value );
	}
}

static void
on_wdir_changed( GtkEntry *entry, NactICommandTab *instance )
{
	NAObjectProfile *profile;

	if( !st_on_selection_change ){
		g_object_get(
				G_OBJECT( instance ),
				TAB_UPDATABLE_PROP_SELECTED_PROFILE, &profile,
				NULL );

		if( profile ){
			na_object_set_working_dir( profile, gtk_entry_get_text( entry ));
			g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, profile, FALSE );
		}
	}
}

/*
 * See core/na-tokens.c for valid parameters
 */
static gchar *
parse_parameters( NactICommandTab *instance )
{
	const gchar *command = gtk_entry_get_text( GTK_ENTRY( get_path_entry( instance )));
	const gchar *param_template = gtk_entry_get_text( GTK_ENTRY( get_parameters_entry( instance )));
	gchar *exec, *returned;

	exec = g_strdup_printf( "%s %s", command, param_template );
	returned = na_tokens_parse_parameters( st_tokens, exec, FALSE );
	g_free( exec );

	return( returned );
}

static void
update_example_label( NactICommandTab *instance, NAObjectProfile *profile )
{
	/*static const char *thisfn = "nact_iconditions_update_example_label";*/
	gchar *newlabel;
	gchar *parameters;
	GtkWidget *example_widget;

	example_widget = base_window_get_widget( BASE_WINDOW( instance ), "CommandExampleLabel" );

	if( profile ){
		parameters = parse_parameters( instance );
		/*g_debug( "%s: parameters=%s", thisfn, parameters );*/

		/* convert special xml chars (&, <, >,...) to avoid warnings
		 * generated by Pango parser
		 */
		/* i18n: command-line example: e.g., /bin/ls file1.txt file2.txt */
		newlabel = g_markup_printf_escaped(
				"<i><b><span size=\"small\">%s %s</span></b></i>", _( "e.g.," ), parameters );

		g_free( parameters );

	} else {
		newlabel = g_strdup( "" );
	}

	gtk_label_set_label( GTK_LABEL( example_widget ), newlabel );
	g_free( newlabel );
}
