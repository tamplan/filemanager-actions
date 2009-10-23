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

#include <common/na-object-api.h>
#include <common/na-utils.h>

#include "base-iprefs.h"
#include "base-window.h"
#include "nact-main-tab.h"
#include "nact-ibackground-tab.h"

/* private interface data
 */
struct NactIBackgroundTabInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

static gboolean st_initialized = FALSE;
static gboolean st_finalized = FALSE;

static GType    register_type( void );
static void     interface_base_init( NactIBackgroundTabInterface *klass );
static void     interface_base_finalize( NactIBackgroundTabInterface *klass );

static void     on_tab_updatable_selection_changed( NactIBackgroundTab *instance, gint count_selected );
static void     on_tab_updatable_enable_tab( NactIBackgroundTab *instance, NAObjectItem *item );
static gboolean tab_set_sensitive( NactIBackgroundTab *instance );

static void     on_toolbar_same_label_toggled( GtkToggleButton *button, NactIBackgroundTab *instance );

static void     on_toolbar_label_changed( GtkEntry *entry, NactIBackgroundTab *instance );

GType
nact_ibackground_tab_get_type( void )
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
	static const gchar *thisfn = "nact_ibackground_tab_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NactIBackgroundTabInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "NactIBackgroundTab", &info, 0 );

	g_type_interface_add_prerequisite( type, BASE_WINDOW_TYPE );

	return( type );
}

static void
interface_base_init( NactIBackgroundTabInterface *klass )
{
	static const gchar *thisfn = "nact_ibackground_tab_interface_base_init";

	if( !st_initialized ){
		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NactIBackgroundTabInterfacePrivate, 1 );

		st_initialized = TRUE;
	}
}

static void
interface_base_finalize( NactIBackgroundTabInterface *klass )
{
	static const gchar *thisfn = "nact_ibackground_tab_interface_base_finalize";

	if( !st_finalized ){
		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );

		st_finalized = TRUE;
	}
}

void
nact_ibackground_tab_initial_load_toplevel( NactIBackgroundTab *instance )
{
	static const gchar *thisfn = "nact_ibackground_tab_initial_load_toplevel";

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_IBACKGROUND_TAB( instance ));

	if( st_initialized && !st_finalized ){
	}
}

void
nact_ibackground_tab_runtime_init_toplevel( NactIBackgroundTab *instance )
{
	static const gchar *thisfn = "nact_ibackground_tab_runtime_init_toplevel";
	GtkWidget *label_widget;
	GtkWidget *toolbar_same_label_button;

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_IBACKGROUND_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_signal_connect(
				G_OBJECT( instance ),
				TAB_UPDATABLE_SIGNAL_SELECTION_CHANGED,
				G_CALLBACK( on_tab_updatable_selection_changed ),
				instance );

		g_signal_connect(
				G_OBJECT( instance ),
				TAB_UPDATABLE_SIGNAL_ENABLE_TAB,
				G_CALLBACK( on_tab_updatable_enable_tab ),
				instance );

		toolbar_same_label_button = base_window_get_widget( BASE_WINDOW( instance ), "ToolbarSameLabelButton" );
		g_signal_connect(
				G_OBJECT( toolbar_same_label_button ),
				"toggled",
				G_CALLBACK( on_toolbar_same_label_toggled ),
				instance );

		label_widget = base_window_get_widget( BASE_WINDOW( instance ), "ToolbarLabelEntry" );
		g_signal_connect(
				G_OBJECT( label_widget ),
				"changed",
				G_CALLBACK( on_toolbar_label_changed ),
				instance );
	}
}

void
nact_ibackground_tab_all_widgets_showed( NactIBackgroundTab *instance )
{
	static const gchar *thisfn = "nact_ibackground_tab_all_widgets_showed";

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_IBACKGROUND_TAB( instance ));

	if( st_initialized && !st_finalized ){
	}
}

void
nact_ibackground_tab_dispose( NactIBackgroundTab *instance )
{
	static const gchar *thisfn = "nact_ibackground_tab_dispose";

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_IBACKGROUND_TAB( instance ));

	if( st_initialized && !st_finalized ){
	}
}

static void
on_tab_updatable_selection_changed( NactIBackgroundTab *instance, gint count_selected )
{
	static const gchar *thisfn = "nact_ibackground_tab_on_tab_updatable_selection_changed";
	NAObjectItem *item;
	gboolean enable_tab;
	GtkToggleButton *same_label_button;
	gboolean same_label;
	GtkWidget *short_label_widget;
	gchar *short_label;

	g_debug( "%s: instance=%p, count_selected=%d", thisfn, ( void * ) instance, count_selected );
	g_return_if_fail( NACT_IS_IBACKGROUND_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_object_get(
				G_OBJECT( instance ),
				TAB_UPDATABLE_PROP_EDITED_ACTION, &item,
				NULL );

		g_return_if_fail( !item || NA_IS_OBJECT_ITEM( item ));

		enable_tab = tab_set_sensitive( instance );

		same_label_button = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( instance ), "ToolbarSameLabelButton" ));
		same_label = enable_tab && NA_IS_OBJECT_ACTION( item ) ? na_object_action_toolbar_use_same_label( NA_OBJECT_ACTION( item )) : FALSE;
		gtk_toggle_button_set_active( same_label_button, same_label );

		short_label_widget = base_window_get_widget( BASE_WINDOW( instance ), "ToolbarLabelEntry" );
		short_label = enable_tab && NA_IS_OBJECT_ACTION( item ) ? na_object_action_toolbar_get_label( NA_OBJECT_ACTION( item )) : g_strdup( "" );
		gtk_entry_set_text( GTK_ENTRY( short_label_widget ), short_label );
		g_free( short_label );
	}
}

static void
on_tab_updatable_enable_tab( NactIBackgroundTab *instance, NAObjectItem *item )
{
	static const gchar *thisfn = "nact_ibackground_tab_on_tab_updatable_enable_tab";

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p, item=%p", thisfn, ( void * ) instance, ( void * ) item );
		g_return_if_fail( NACT_IS_IBACKGROUND_TAB( instance ));

		tab_set_sensitive( instance );
	}
}

static gboolean
tab_set_sensitive( NactIBackgroundTab *instance )
{
	NAObjectItem *item;
	gboolean enable_tab;
	gboolean target_toolbar;
	gboolean enable_toolbar_frame;
	GtkWidget *toolbar_frame_widget;
	gboolean enable_toolbar_label_entry;
	GtkWidget *toolbar_label_entry;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, &item,
			NULL );

	target_toolbar = ( item && na_object_is_target_toolbar( item ));
	enable_tab = ( item &&
			( na_object_is_target_background( item ) || target_toolbar ));
	nact_main_tab_enable_page( NACT_MAIN_WINDOW( instance ), TAB_BACKGROUND, enable_tab );

	enable_toolbar_frame = ( enable_tab && NA_IS_OBJECT_ACTION( item ) && target_toolbar );
	toolbar_frame_widget = base_window_get_widget( BASE_WINDOW( instance ), "BackgroundToolbarFrame" );
	gtk_widget_set_sensitive( toolbar_frame_widget, enable_toolbar_frame );

	enable_toolbar_label_entry = ( enable_toolbar_frame && !na_object_action_toolbar_use_same_label( NA_OBJECT_ACTION( item )));
	toolbar_label_entry = base_window_get_widget( BASE_WINDOW( instance ), "ToolbarLabelEntry" );
	gtk_widget_set_sensitive( toolbar_label_entry, enable_toolbar_label_entry );

	return( enable_tab );
}

static void
on_toolbar_same_label_toggled( GtkToggleButton *button, NactIBackgroundTab *instance )
{
	NAObjectItem *edited;
	gboolean same_label;
	GtkWidget *label_widget;
	gchar *text;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, &edited,
			NULL );

	if( edited ){
		g_return_if_fail( NA_IS_OBJECT_ACTION( edited ));

		same_label = gtk_toggle_button_get_active( button );
		na_object_action_toolbar_set_same_label( NA_OBJECT_ACTION( edited ), same_label );

		tab_set_sensitive( instance );

		if( same_label ){
			label_widget = base_window_get_widget( BASE_WINDOW( instance ), "ToolbarLabelEntry" );
			text = na_object_get_label( edited );
			gtk_entry_set_text( GTK_ENTRY( label_widget ), text );
			g_free( text );
		}

		g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, edited, FALSE );
	}
}

static void
on_toolbar_label_changed( GtkEntry *entry, NactIBackgroundTab *instance )
{
	NAObjectItem *edited;
	const gchar *label;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, &edited,
			NULL );

	if( edited ){
		g_return_if_fail( NA_IS_OBJECT_ACTION( edited ));

		label = gtk_entry_get_text( entry );
		na_object_action_toolbar_set_label( NA_OBJECT_ACTION( edited ), label );

		g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, edited, FALSE );
	}
}
