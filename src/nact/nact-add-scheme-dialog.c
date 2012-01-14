/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011, 2012 Pierre Wieser and others (see AUTHORS)
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

#include <gdk/gdkkeysyms.h>

#include <api/na-core-utils.h>

#include <core/na-settings.h>

#include "nact-schemes-list.h"
#include "nact-add-scheme-dialog.h"

/* private class data
 */
struct _NactAddSchemeDialogClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _NactAddSchemeDialogPrivate {
	gboolean dispose_has_run;
	GSList  *already_used;
	gchar   *scheme;
};

static const gchar  *st_xmlui_filename = PKGDATADIR "/nact-add-scheme.ui";
static const gchar  *st_toplevel_name  = "AddSchemeDialog";
static const gchar  *st_wsp_name       = NA_IPREFS_SCHEME_ADD_SCHEME_WSP;

static GObjectClass *st_parent_class   = NULL;

static GType    register_type( void );
static void     class_init( NactAddSchemeDialogClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_dispose( GObject *dialog );
static void     instance_finalize( GObject *dialog );

static void     on_base_initialize_gtk_toplevel( NactAddSchemeDialog *editor, GtkDialog *toplevel );
static void     on_base_initialize_base_window( NactAddSchemeDialog *editor );
static void     on_base_all_widgets_showed( NactAddSchemeDialog *editor );
static gboolean on_button_press_event( GtkWidget *widget, GdkEventButton *event, NactAddSchemeDialog *dialog );
static void     on_cancel_clicked( GtkButton *button, NactAddSchemeDialog *editor );
static void     on_ok_clicked( GtkButton *button, NactAddSchemeDialog *editor );
static void     on_selection_changed( const gchar *scheme, gboolean used, NactAddSchemeDialog *dialog );
static void     try_for_send_ok( NactAddSchemeDialog *dialog );
static void     send_ok( NactAddSchemeDialog *dialog );
static void     on_dialog_ok( BaseDialog *dialog );

GType
nact_add_scheme_dialog_get_type( void )
{
	static GType dialog_type = 0;

	if( !dialog_type ){
		dialog_type = register_type();
	}

	return( dialog_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "nact_add_scheme_dialog_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NactAddSchemeDialogClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactAddSchemeDialog ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( BASE_TYPE_DIALOG, "NactAddSchemeDialog", &info, 0 );

	return( type );
}

static void
class_init( NactAddSchemeDialogClass *klass )
{
	static const gchar *thisfn = "nact_add_scheme_dialog_class_init";
	GObjectClass *object_class;
	BaseDialogClass *dialog_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactAddSchemeDialogClassPrivate, 1 );

	dialog_class = BASE_DIALOG_CLASS( klass );
	dialog_class->ok = on_dialog_ok;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_add_scheme_dialog_instance_init";
	NactAddSchemeDialog *self;

	g_return_if_fail( NACT_IS_ADD_SCHEME_DIALOG( instance ));

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );

	self = NACT_ADD_SCHEME_DIALOG( instance );

	self->private = g_new0( NactAddSchemeDialogPrivate, 1 );

	base_window_signal_connect( BASE_WINDOW( instance ),
			G_OBJECT( instance ), BASE_SIGNAL_INITIALIZE_GTK, G_CALLBACK( on_base_initialize_gtk_toplevel ));

	base_window_signal_connect( BASE_WINDOW( instance ),
			G_OBJECT( instance ), BASE_SIGNAL_INITIALIZE_WINDOW, G_CALLBACK( on_base_initialize_base_window ));

	base_window_signal_connect( BASE_WINDOW( instance ),
			G_OBJECT( instance ), BASE_SIGNAL_SHOW_WIDGETS, G_CALLBACK( on_base_all_widgets_showed));

	self->private->dispose_has_run = FALSE;
	self->private->scheme = NULL;
}

static void
instance_dispose( GObject *dialog )
{
	static const gchar *thisfn = "nact_add_scheme_dialog_instance_dispose";
	NactAddSchemeDialog *self;
	GtkTreeView *listview;
	GtkTreeModel *model;
	GtkTreeSelection *selection;

	g_return_if_fail( NACT_IS_ADD_SCHEME_DIALOG( dialog ));

	g_debug( "%s: dialog=%p (%s)", thisfn, ( void * ) dialog, G_OBJECT_TYPE_NAME( dialog ));

	self = NACT_ADD_SCHEME_DIALOG( dialog );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		listview = GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( dialog ), "SchemesTreeView" ));
		model = gtk_tree_view_get_model( listview );
		selection = gtk_tree_view_get_selection( listview );
		gtk_tree_selection_unselect_all( selection );
		gtk_list_store_clear( GTK_LIST_STORE( model ));

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( dialog );
		}
	}
}

static void
instance_finalize( GObject *dialog )
{
	static const gchar *thisfn = "nact_add_scheme_dialog_instance_finalize";
	NactAddSchemeDialog *self;

	g_return_if_fail( NACT_IS_ADD_SCHEME_DIALOG( dialog ));

	g_debug( "%s: dialog=%p (%s)", thisfn, ( void * ) dialog, G_OBJECT_TYPE_NAME( dialog ));

	self = NACT_ADD_SCHEME_DIALOG( dialog );

	na_core_utils_slist_free( self->private->already_used );
	g_free( self->private->scheme );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( dialog );
	}
}

/**
 * nact_add_scheme_dialog_run:
 * @parent: the BaseWindow parent of this dialog
 *  (usually the NactMainWindow).
 * @schemes: list of already used schemes.
 *
 * Initializes and runs the dialog.
 *
 * Returns: the selected scheme, as a newly allocated string which should
 * be g_free() by the caller, or NULL.
 */
gchar *
nact_add_scheme_dialog_run( BaseWindow *parent, GSList *schemes )
{
	static const gchar *thisfn = "nact_add_scheme_dialog_run";
	NactAddSchemeDialog *dialog;
	gchar *scheme;

	g_debug( "%s: parent=%p", thisfn, ( void * ) parent );

	g_return_val_if_fail( BASE_IS_WINDOW( parent ), NULL );

	dialog = g_object_new( NACT_TYPE_ADD_SCHEME_DIALOG,
			BASE_PROP_PARENT,         parent,
			BASE_PROP_XMLUI_FILENAME, st_xmlui_filename,
			BASE_PROP_TOPLEVEL_NAME,  st_toplevel_name,
			BASE_PROP_WSP_NAME,       st_wsp_name,
			NULL );

	dialog->private->already_used = na_core_utils_slist_duplicate( schemes );
	scheme = NULL;

	if( base_window_run( BASE_WINDOW( dialog )) == GTK_RESPONSE_OK ){
		scheme = g_strdup( dialog->private->scheme );
	}

	g_object_unref( dialog );

	return( scheme );
}

static void
on_base_initialize_gtk_toplevel( NactAddSchemeDialog *dialog, GtkDialog *toplevel )
{
	static const gchar *thisfn = "nact_add_scheme_dialog_on_base_initialize_gtk_toplevel";
	GtkTreeView *listview;

	g_return_if_fail( NACT_IS_ADD_SCHEME_DIALOG( dialog ));

	if( !dialog->private->dispose_has_run ){
		g_debug( "%s: dialog=%p, toplevel=%p", thisfn, ( void * ) dialog, ( void * ) toplevel );

		listview = GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( dialog ), "SchemesTreeView" ));
		nact_schemes_list_create_model( listview, SCHEMES_LIST_FOR_ADD_FROM_DEFAULTS );

#if !GTK_CHECK_VERSION( 2,22,0 )
		gtk_dialog_set_has_separator( toplevel, FALSE );
#endif
	}
}

static void
on_base_initialize_base_window( NactAddSchemeDialog *dialog )
{
	static const gchar *thisfn = "nact_add_scheme_dialog_on_base_initialize_base_window";
	GtkTreeView *listview;

	g_return_if_fail( NACT_IS_ADD_SCHEME_DIALOG( dialog ));

	if( !dialog->private->dispose_has_run ){
		g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );

		listview = GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( dialog ), "SchemesTreeView" ));
		nact_schemes_list_init_view( listview, BASE_WINDOW( dialog ), ( pf_new_selection_cb ) on_selection_changed, ( void * ) dialog );

		nact_schemes_list_setup_values( BASE_WINDOW( dialog ), dialog->private->already_used );

		/* catch double-click */
		base_window_signal_connect(
				BASE_WINDOW( dialog ),
				G_OBJECT( listview ),
				"button-press-event",
				G_CALLBACK( on_button_press_event ));

		/* dialog buttons
		 */
		base_window_signal_connect_by_name(
				BASE_WINDOW( dialog ),
				"CancelButton",
				"clicked",
				G_CALLBACK( on_cancel_clicked ));

		base_window_signal_connect_by_name(
				BASE_WINDOW( dialog ),
				"OKButton",
				"clicked",
				G_CALLBACK( on_ok_clicked ));
	}
}

static void
on_base_all_widgets_showed( NactAddSchemeDialog *dialog )
{
	static const gchar *thisfn = "nact_add_scheme_dialog_on_base_all_widgets_showed";

	g_return_if_fail( NACT_IS_ADD_SCHEME_DIALOG( dialog ));

	if( !dialog->private->dispose_has_run ){
		g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );

		nact_schemes_list_show_all( BASE_WINDOW( dialog ));
	}
}

static gboolean
on_button_press_event( GtkWidget *widget, GdkEventButton *event, NactAddSchemeDialog *dialog )
{
	gboolean stop = FALSE;

	/* double-click of left button */
	if( event->type == GDK_2BUTTON_PRESS && event->button == 1 ){
		try_for_send_ok( dialog );
		stop = TRUE;
	}

	return( stop );
}

static void
on_cancel_clicked( GtkButton *button, NactAddSchemeDialog *dialog )
{
	GtkWindow *toplevel = base_window_get_gtk_toplevel( BASE_WINDOW( dialog ));

	gtk_dialog_response( GTK_DIALOG( toplevel ), GTK_RESPONSE_CLOSE );
}

static void
on_ok_clicked( GtkButton *button, NactAddSchemeDialog *dialog )
{
	send_ok( dialog );
}

/*
 * this function is a callback, called from nact-schemes-list:on_selection_changed
 * this let us validate/invalidate the OK button
 */
static void
on_selection_changed( const gchar *scheme, gboolean used, NactAddSchemeDialog *dialog )
{
	GtkWidget *button;

	button = base_window_get_widget( BASE_WINDOW( dialog ), "OKButton" );
	gtk_widget_set_sensitive( button, !used );
}

static void
try_for_send_ok( NactAddSchemeDialog *dialog )
{
	GtkWidget *button;
	gboolean is_sensitive;

	button = base_window_get_widget( BASE_WINDOW( dialog ), "OKButton" );

	is_sensitive = gtk_widget_is_sensitive( button );

	if( is_sensitive ){
		send_ok( dialog );
	}
}

static void
send_ok( NactAddSchemeDialog *dialog )
{
	GtkWindow *toplevel = base_window_get_gtk_toplevel( BASE_WINDOW( dialog ));

	gtk_dialog_response( GTK_DIALOG( toplevel ), GTK_RESPONSE_OK );
}

static void
on_dialog_ok( BaseDialog *dialog )
{
	NactAddSchemeDialog *editor = NACT_ADD_SCHEME_DIALOG( dialog );
	editor->private->scheme = nact_schemes_list_get_current_scheme( BASE_WINDOW( dialog ));
}
