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

#include <common/na-utils.h>

#include "nact-main-window.h"
#include "nact-iconditions-tab.h"
#include "nact-iprefs.h"

/* private interface data
 */
struct NactIConditionsTabInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

static GType            register_type( void );
static void             interface_base_init( NactIConditionsTabInterface *klass );
static void             interface_base_finalize( NactIConditionsTabInterface *klass );

static NAActionProfile *v_get_edited_profile( NactWindow *window );
static void             v_field_modified( NactWindow *window );

static void             on_basenames_changed( GtkEntry *entry, gpointer user_data );
static GtkWidget       *get_basenames_entry( NactWindow *window );
static void             on_matchcase_toggled( GtkToggleButton *button, gpointer user_data );
static GtkButton       *get_matchcase_button( NactWindow *window );
static void             on_mimetypes_changed( GtkEntry *entry, gpointer user_data );
static GtkWidget       *get_mimetypes_entry( NactWindow *window );
static void             on_isfiledir_toggled( GtkToggleButton *button, gpointer user_data );
static void             set_isfiledir( NactWindow *window, gboolean isfile, gboolean isdir );
static GtkButton       *get_isfile_button( NactWindow *window );
static GtkButton       *get_isdir_button( NactWindow *window );
static GtkButton       *get_both_button( NactWindow *window );
static void             on_multiple_toggled( GtkToggleButton *button, gpointer user_data );
static GtkButton       *get_multiple_button( NactWindow *window );

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

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( NactIConditionsTabInterface *klass )
{
	static const gchar *thisfn = "nact_iconditions_tab_interface_base_init";
	static gboolean initialized = FALSE;

	if( !initialized ){
		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NactIConditionsTabInterfacePrivate, 1 );

		klass->get_edited_profile = NULL;
		klass->field_modified = NULL;

		initialized = TRUE;
	}
}

static void
interface_base_finalize( NactIConditionsTabInterface *klass )
{
	static const gchar *thisfn = "nact_iconditions_tab_interface_base_finalize";
	static gboolean finalized = FALSE ;

	if( !finalized ){
		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );

		finalized = TRUE;
	}
}

void
nact_iconditions_tab_initial_load( NactWindow *dialog )
{
	static const gchar *thisfn = "nact_iconditions_tab_initial_load";

	g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );
}

void
nact_iconditions_tab_runtime_init( NactWindow *dialog )
{
	static const gchar *thisfn = "nact_iconditions_tab_runtime_init";
	GtkWidget *basenames_widget;
	GtkButton *matchcase_button;
	GtkWidget *mimetypes_widget;
	GtkButton *isfile_button, *isdir_button, *both_button;
	GtkButton *multiple_button;

	g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );

	basenames_widget = get_basenames_entry( dialog );
	nact_window_signal_connect( dialog, G_OBJECT( basenames_widget ), "changed", G_CALLBACK( on_basenames_changed ));

	matchcase_button = get_matchcase_button( dialog );
	nact_window_signal_connect( dialog, G_OBJECT( matchcase_button ), "toggled", G_CALLBACK( on_matchcase_toggled ));

	mimetypes_widget = get_mimetypes_entry( dialog );
	nact_window_signal_connect( dialog, G_OBJECT( mimetypes_widget ), "changed", G_CALLBACK( on_mimetypes_changed ));

	isfile_button = get_isfile_button( dialog );
	nact_window_signal_connect( dialog, G_OBJECT( isfile_button ), "toggled", G_CALLBACK( on_isfiledir_toggled ));
	isdir_button = get_isdir_button( dialog );
	nact_window_signal_connect( dialog, G_OBJECT( isdir_button ), "toggled", G_CALLBACK( on_isfiledir_toggled ));
	both_button = get_both_button( dialog );
	nact_window_signal_connect( dialog, G_OBJECT( both_button ), "toggled", G_CALLBACK( on_isfiledir_toggled ));

	multiple_button = get_multiple_button( dialog );
	nact_window_signal_connect( dialog, G_OBJECT( multiple_button ), "toggled", G_CALLBACK( on_multiple_toggled ));
}

void
nact_iconditions_tab_all_widgets_showed( NactWindow *dialog )
{
	static const gchar *thisfn = "nact_iconditions_tab_all_widgets_showed";

	g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );
}

void
nact_iconditions_tab_dispose( NactWindow *dialog )
{
	static const gchar *thisfn = "nact_iconditions_tab_dispose";

	g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );
}

void
nact_iconditions_tab_set_profile( NactWindow *dialog, NAActionProfile *profile )
{
	static const gchar *thisfn = "nact_iconditions_tab_set_profile";
	GtkWidget *basenames_widget, *mimetypes_widget;
	GSList *basenames, *mimetypes;
	gchar *basenames_text, *mimetypes_text;
	GtkButton *matchcase_button;
	gboolean matchcase;
	gboolean isfile, isdir;
	GtkButton *files_button, *dir_button, *both_button;
	GtkButton *multiple_button;
	gboolean multiple;

	g_debug( "%s: dialog=%p, profile=%p", thisfn, ( void * ) dialog, ( void * ) profile );

	basenames_widget = get_basenames_entry( dialog );
	basenames = profile ? na_action_profile_get_basenames( profile ) : NULL;
	basenames_text = profile ? na_utils_string_list_to_text( basenames ) : g_strdup( "" );
	gtk_entry_set_text( GTK_ENTRY( basenames_widget ), basenames_text );
	g_free( basenames_text );
	na_utils_free_string_list( basenames );
	gtk_widget_set_sensitive( basenames_widget, profile != NULL );

	matchcase_button = get_matchcase_button( dialog );
	matchcase = profile ? na_action_profile_get_matchcase( profile ) : FALSE;
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( matchcase_button ), matchcase );
	gtk_widget_set_sensitive( GTK_WIDGET( matchcase_button ), profile != NULL );

	mimetypes_widget = get_mimetypes_entry( dialog );
	mimetypes = profile ? na_action_profile_get_mimetypes( profile ) : NULL;
	mimetypes_text = profile ? na_utils_string_list_to_text( mimetypes ) : g_strdup( "" );
	gtk_entry_set_text( GTK_ENTRY( mimetypes_widget ), mimetypes_text );
	g_free( mimetypes_text );
	na_utils_free_string_list( mimetypes );
	gtk_widget_set_sensitive( mimetypes_widget, profile != NULL );

	isfile = profile ? na_action_profile_get_is_file( profile ) : FALSE;
	isdir = profile ? na_action_profile_get_is_dir( profile ) : FALSE;
	set_isfiledir( dialog, isfile, isdir );
	files_button = get_isfile_button( dialog );
	gtk_widget_set_sensitive( GTK_WIDGET( files_button ), profile != NULL );
	dir_button = get_isdir_button( dialog );
	gtk_widget_set_sensitive( GTK_WIDGET( dir_button ), profile != NULL );
	both_button = get_both_button( dialog );
	gtk_widget_set_sensitive( GTK_WIDGET( both_button ), profile != NULL );

	multiple_button = get_multiple_button( dialog );
	multiple = profile ? na_action_profile_get_multiple( profile ) : FALSE;
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( multiple_button ), multiple );
	gtk_widget_set_sensitive( GTK_WIDGET( multiple_button ), profile != NULL );
}

void
nact_iconditions_tab_get_isfiledir( NactWindow *window, gboolean *isfile, gboolean *isdir )
{
	gboolean both;

	g_assert( isfile );
	g_assert( isdir );

	both = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( get_both_button( window )));
	if( both ){
		*isfile = TRUE;
		*isdir = TRUE;
	} else {
		*isfile = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( get_isfile_button( window )));
		*isdir = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( get_isdir_button( window )));
	}
}

gboolean
nact_iconditions_tab_get_multiple( NactWindow *window )
{
	GtkButton *button = get_multiple_button( window );
	return( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button )));
}

static NAActionProfile *
v_get_edited_profile( NactWindow *window )
{
	g_assert( NACT_IS_ICONDITIONS_TAB( window ));

	if( NACT_ICONDITIONS_TAB_GET_INTERFACE( window )->get_edited_profile ){
		return( NACT_ICONDITIONS_TAB_GET_INTERFACE( window )->get_edited_profile( window ));
	}

	return( NULL );
}

static void
v_field_modified( NactWindow *window )
{
	g_assert( NACT_IS_ICONDITIONS_TAB( window ));

	if( NACT_ICONDITIONS_TAB_GET_INTERFACE( window )->field_modified ){
		NACT_ICONDITIONS_TAB_GET_INTERFACE( window )->field_modified( window );
	}
}

static void
on_basenames_changed( GtkEntry *entry, gpointer user_data )
{
	NactWindow *dialog;
	NAActionProfile *edited;
	const gchar *text;
	GSList *basenames;

	g_assert( NACT_IS_WINDOW( user_data ));
	dialog = NACT_WINDOW( user_data );

	edited = NA_ACTION_PROFILE( v_get_edited_profile( dialog ));

	if( edited ){
		text = gtk_entry_get_text( entry );
		basenames = na_utils_text_to_string_list( text );
		na_action_profile_set_basenames( edited, basenames );
		na_utils_free_string_list( basenames );
		v_field_modified( dialog );
	}
}

static GtkWidget *
get_basenames_entry( NactWindow *window )
{
	return( base_window_get_widget( BASE_WINDOW( window ), "ConditionsFilenamesEntry" ));
}

static void
on_matchcase_toggled( GtkToggleButton *button, gpointer user_data )
{
	NactWindow *dialog;
	NAActionProfile *edited;
	gboolean matchcase;

	g_assert( NACT_IS_WINDOW( user_data ));
	dialog = NACT_WINDOW( user_data );

	edited = NA_ACTION_PROFILE( v_get_edited_profile( dialog ));

	if( edited ){
		matchcase = gtk_toggle_button_get_active( button );
		na_action_profile_set_matchcase( edited, matchcase );
		v_field_modified( dialog );
	}
}

static GtkButton *
get_matchcase_button( NactWindow *window )
{
	return( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "ConditionsMatchcaseButton" )));
}

static void
on_mimetypes_changed( GtkEntry *entry, gpointer user_data )
{
	NactWindow *dialog;
	NAActionProfile *edited;
	const gchar *text;
	GSList *mimetypes;

	g_assert( NACT_IS_WINDOW( user_data ));
	dialog = NACT_WINDOW( user_data );

	edited = NA_ACTION_PROFILE( v_get_edited_profile( dialog ));

	if( edited ){
		text = gtk_entry_get_text( entry );
		mimetypes = na_utils_text_to_string_list( text );
		na_action_profile_set_mimetypes( edited, mimetypes );
		na_utils_free_string_list( mimetypes );
		v_field_modified( dialog );
	}
}

static GtkWidget *
get_mimetypes_entry( NactWindow *window )
{
	return( base_window_get_widget( BASE_WINDOW( window ), "ConditionsMimetypesEntry" ));
}

/*
 * Note that this callback is triggered twice: first, for the
 * deactivated button, then a second time for the newly activated one.
 * I don't know what to do to be triggered only once..?
 */
static void
on_isfiledir_toggled( GtkToggleButton *button, gpointer user_data )
{
	/*static const gchar *thisfn = "nact_iconditions_tab_on_isfiledir_toggled";*/
	NactWindow *dialog;
	NAActionProfile *edited;
	gboolean isfile, isdir;

	g_assert( NACT_IS_WINDOW( user_data ));
	dialog = NACT_WINDOW( user_data );

	edited = NA_ACTION_PROFILE( v_get_edited_profile( dialog ));

	if( edited ){
		nact_iconditions_tab_get_isfiledir( dialog, &isfile, &isdir );
		na_action_profile_set_isfiledir( edited, isfile, isdir );
		v_field_modified( dialog );
	}
}

static void
set_isfiledir( NactWindow *window, gboolean isfile, gboolean isdir )
{
	if( isfile && isdir ){
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( get_both_button( window )), TRUE );

	} else if( isfile ){
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( get_isfile_button( window )), TRUE );

	} else if( isdir ){
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( get_isdir_button( window )), TRUE );
	}
}

static GtkButton *
get_isfile_button( NactWindow *window )
{
	return( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "ConditionsOnlyFilesButton" )));
}

static GtkButton *
get_isdir_button( NactWindow *window )
{
	return( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "ConditionsOnlyFoldersButton" )));
}

static GtkButton *
get_both_button( NactWindow *window )
{
	return( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "ConditionsBothButton" )));
}

static void
on_multiple_toggled( GtkToggleButton *button, gpointer user_data )
{
	NactWindow *dialog;
	NAActionProfile *edited;
	gboolean multiple;

	g_assert( NACT_IS_WINDOW( user_data ));
	dialog = NACT_WINDOW( user_data );

	edited = NA_ACTION_PROFILE( v_get_edited_profile( dialog ));

	if( edited ){
		multiple = gtk_toggle_button_get_active( button );
		na_action_profile_set_multiple( edited, multiple );
		v_field_modified( dialog );
	}
}

static GtkButton *
get_multiple_button( NactWindow *window )
{
	return( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( window ), "ConditionsMultipleButton" )));
}
