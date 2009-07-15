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

#include <common/na-action.h>
#include <common/na-action-profile.h>

#include "nact-application.h"
#include "nact-iprofile-item.h"

/* private interface data
 */
struct NactIProfileItemInterfacePrivate {
};

static GType         register_type( void );
static void          interface_base_init( NactIProfileItemInterface *klass );
static void          interface_base_finalize( NactIProfileItemInterface *klass );

static GObject      *v_get_edited_profile( NactWindow *window );
static void          v_field_modified( NactWindow *window );

static void          on_label_changed( GtkEntry *entry, gpointer user_data );

GType
nact_iprofile_item_get_type( void )
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
	static const gchar *thisfn = "nact_iprofile_item_register_type";
	g_debug( "%s", thisfn );

	static const GTypeInfo info = {
		sizeof( NactIProfileItemInterface ),
		( GBaseInitFunc ) interface_base_init,
		( GBaseFinalizeFunc ) interface_base_finalize,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL
	};

	GType type = g_type_register_static( G_TYPE_INTERFACE, "NactIProfileItem", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( NactIProfileItemInterface *klass )
{
	static const gchar *thisfn = "nact_iprofile_item_interface_base_init";
	static gboolean initialized = FALSE;

	if( !initialized ){
		g_debug( "%s: klass=%p", thisfn, klass );

		klass->private = g_new0( NactIProfileItemInterfacePrivate, 1 );

		klass->get_edited_profile = NULL;
		klass->field_modified = NULL;

		initialized = TRUE;
	}
}

static void
interface_base_finalize( NactIProfileItemInterface *klass )
{
	static const gchar *thisfn = "nact_iprofile_item_interface_base_finalize";
	static gboolean finalized = FALSE ;

	if( !finalized ){
		g_debug( "%s: klass=%p", thisfn, klass );

		g_free( klass->private );

		finalized = TRUE;
	}
}

void
nact_iprofile_item_initial_load( NactWindow *dialog, NAActionProfile *profile )
{
	static const gchar *thisfn = "nact_iprofile_item_initial_load";
	g_debug( "%s: dialog=%p, profile=%p", thisfn, dialog, profile );

	/*BaseApplication *appli = BASE_APPLICATION( base_window_get_application( BASE_WINDOW( dialog )));
	GtkWindow *toplevel = base_application_get_dialog( appli, "MenuItemWindow" );
	GtkWidget *vbox = base_application_search_for_widget( appli, toplevel, "MenuItemVBox" );
	GtkWidget *dest = base_application_get_widget( appli, BASE_WINDOW( dialog ), "MenuItemVBox" );
	gtk_widget_reparent( vbox, dest );*/
}

void
nact_iprofile_item_size_labels( NactWindow *window, GObject *size_group )
{
	g_assert( NACT_IS_WINDOW( window ));
	g_assert( GTK_IS_SIZE_GROUP( size_group ));

	GtkWidget *label = base_window_get_widget( BASE_WINDOW( window ), "ProfileLabelLabel" );
	gtk_size_group_add_widget( GTK_SIZE_GROUP( size_group ), label );
}

void
nact_iprofile_item_size_buttons( NactWindow *window, GObject *size_group )
{
	g_assert( NACT_IS_WINDOW( window ));
	g_assert( GTK_IS_SIZE_GROUP( size_group ));
}

void
nact_iprofile_item_runtime_init( NactWindow *dialog, NAActionProfile *profile )
{
	static const gchar *thisfn = "nact_iprofile_item_runtime_init";
	g_debug( "%s: dialog=%p, profile=%p", thisfn, dialog, profile );

	GtkWidget *label_widget = base_window_get_widget( BASE_WINDOW( dialog ), "ProfileLabelEntry" );
	nact_window_signal_connect( dialog, G_OBJECT( label_widget ), "changed", G_CALLBACK( on_label_changed ));
	gchar *label = na_action_profile_get_label( profile );
	gtk_entry_set_text( GTK_ENTRY( label_widget ), label );
	g_free( label );
}

/**
 * A good place to set focus to the first visible field.
 */
void
nact_iprofile_item_all_widgets_showed( NactWindow *dialog )
{
	GtkWidget *label_widget = base_window_get_widget( BASE_WINDOW( dialog ), "ProfileLabelEntry" );
	gtk_widget_grab_focus( label_widget );
}

/**
 * A profile can only be saved if it has at least a label.
 * Returns TRUE if the label of the profile is not empty.
 */
gboolean
nact_iprofile_item_has_label( NactWindow *window )
{
	GtkWidget *label_widget = base_window_get_widget( BASE_WINDOW( window ), "ProfileLabelEntry" );
	const gchar *label = gtk_entry_get_text( GTK_ENTRY( label_widget ));
	return( g_utf8_strlen( label, -1 ) > 0 );
}

void
nact_iprofile_item_dispose( NactWindow *dialog )
{
	static const gchar *thisfn = "nact_iprofile_item_dispose";
	g_debug( "%s: dialog=%p", thisfn, dialog );

	/*BaseApplication *appli = BASE_APPLICATION( base_window_get_application( BASE_WINDOW( dialog )));
	GtkWindow *toplevel = base_application_get_dialog( appli, "MenuItemWindow" );
	GtkWidget *vbox = base_application_get_widget( appli, BASE_WINDOW( dialog ), "MenuItemVBox" );
	gtk_widget_reparent( vbox, GTK_WIDGET( toplevel ));*/
}

static GObject *
v_get_edited_profile( NactWindow *window )
{
	g_assert( NACT_IS_IPROFILE_ITEM( window ));

	if( NACT_IPROFILE_ITEM_GET_INTERFACE( window )->get_edited_profile ){
		return( NACT_IPROFILE_ITEM_GET_INTERFACE( window )->get_edited_profile( window ));
	}

	return( NULL );
}

static void
v_field_modified( NactWindow *window )
{
	g_assert( NACT_IS_IPROFILE_ITEM( window ));

	if( NACT_IPROFILE_ITEM_GET_INTERFACE( window )->field_modified ){
		NACT_IPROFILE_ITEM_GET_INTERFACE( window )->field_modified( window );
	}
}

static void
on_label_changed( GtkEntry *entry, gpointer user_data )
{
	g_assert( NACT_IS_WINDOW( user_data ));
	NactWindow *dialog = NACT_WINDOW( user_data );

	NAActionProfile *edited = NA_ACTION_PROFILE( v_get_edited_profile( dialog ));
	na_action_profile_set_label( edited, gtk_entry_get_text( entry ));

	v_field_modified( dialog );
}
