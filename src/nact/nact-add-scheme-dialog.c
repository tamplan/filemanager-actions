/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011 Pierre Wieser and others (see AUTHORS)
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

#include "nact-schemes-list.h"
#include "nact-add-scheme-dialog.h"

/* private class data
 */
struct NactAddSchemeDialogClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NactAddSchemeDialogPrivate {
	gboolean dispose_has_run;
	GSList  *already_used;
	gchar   *scheme;
};

static GObjectClass *st_parent_class = NULL;

static GType    register_type( void );
static void     class_init( NactAddSchemeDialogClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_dispose( GObject *dialog );
static void     instance_finalize( GObject *dialog );

static NactAddSchemeDialog *add_scheme_dialog_new( BaseWindow *parent );

static gchar   *base_get_iprefs_window_id( const BaseWindow *window );
static gchar   *base_get_dialog_name( const BaseWindow *window );
static gchar   *base_get_ui_filename( const BaseWindow *dialog );
static void     on_base_initial_load_dialog( NactAddSchemeDialog *editor, gpointer user_data );
static void     on_base_runtime_init_dialog( NactAddSchemeDialog *editor, gpointer user_data );
static void     on_base_all_widgets_showed( NactAddSchemeDialog *editor, gpointer user_data );
static gboolean on_button_press_event( GtkWidget *widget, GdkEventButton *event, NactAddSchemeDialog *dialog );
static void     on_cancel_clicked( GtkButton *button, NactAddSchemeDialog *editor );
static void     on_ok_clicked( GtkButton *button, NactAddSchemeDialog *editor );
static void     on_selection_changed( const gchar *scheme, gboolean used, NactAddSchemeDialog *dialog );
static void     try_for_send_ok( NactAddSchemeDialog *dialog );
static void     send_ok( NactAddSchemeDialog *dialog );
static void     validate_dialog( NactAddSchemeDialog *editor );
static gboolean base_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window );

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

	type = g_type_register_static( BASE_DIALOG_TYPE, "NactAddSchemeDialog", &info, 0 );

	return( type );
}

static void
class_init( NactAddSchemeDialogClass *klass )
{
	static const gchar *thisfn = "nact_add_scheme_dialog_class_init";
	GObjectClass *object_class;
	BaseWindowClass *base_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactAddSchemeDialogClassPrivate, 1 );

	base_class = BASE_WINDOW_CLASS( klass );
	base_class->dialog_response = base_dialog_response;
	base_class->get_toplevel_name = base_get_dialog_name;
	base_class->get_iprefs_window_id = base_get_iprefs_window_id;
	base_class->get_ui_filename = base_get_ui_filename;
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

	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			BASE_WINDOW_SIGNAL_INITIAL_LOAD,
			G_CALLBACK( on_base_initial_load_dialog ));

	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			BASE_WINDOW_SIGNAL_RUNTIME_INIT,
			G_CALLBACK( on_base_runtime_init_dialog ));

	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			BASE_WINDOW_SIGNAL_ALL_WIDGETS_SHOWED,
			G_CALLBACK( on_base_all_widgets_showed));

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
	g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );
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
	g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );
	self = NACT_ADD_SCHEME_DIALOG( dialog );

	na_core_utils_slist_free( self->private->already_used );
	g_free( self->private->scheme );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( dialog );
	}
}

/*
 * Returns a newly allocated NactAddSchemeDialog object.
 *
 * @parent: the BaseWindow parent of this dialog (usually, the main
 * toplevel window of the application).
 */
static NactAddSchemeDialog *
add_scheme_dialog_new( BaseWindow *parent )
{
	return( g_object_new( NACT_ADD_SCHEME_DIALOG_TYPE, BASE_WINDOW_PROP_PARENT, parent, NULL ));
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

	dialog = add_scheme_dialog_new( parent );
	dialog->private->already_used = na_core_utils_slist_duplicate( schemes );

	base_window_run( BASE_WINDOW( dialog ));

	scheme = g_strdup( dialog->private->scheme );

	g_object_unref( dialog );

	return( scheme );
}

static gchar *
base_get_iprefs_window_id( const BaseWindow *window )
{
	return( g_strdup( "scheme-add-scheme-wsp" ));
}

static gchar *
base_get_dialog_name( const BaseWindow *window )
{
	return( g_strdup( "AddSchemeDialog" ));
}

static gchar *
base_get_ui_filename( const BaseWindow *dialog )
{
	return( g_strdup( PKGDATADIR "/nact-add-scheme.ui" ));
}

static void
on_base_initial_load_dialog( NactAddSchemeDialog *dialog, gpointer user_data )
{
	static const gchar *thisfn = "nact_add_scheme_dialog_on_initial_load_dialog";
	GtkTreeView *listview;

	g_return_if_fail( NACT_IS_ADD_SCHEME_DIALOG( dialog ));

	if( !dialog->private->dispose_has_run ){
		g_debug( "%s: dialog=%p, user_data=%p", thisfn, ( void * ) dialog, ( void * ) user_data );

		listview = GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( dialog ), "SchemesTreeView" ));
		nact_schemes_list_create_model( listview, SCHEMES_LIST_FOR_ADD_FROM_DEFAULTS );
	}
}

static void
on_base_runtime_init_dialog( NactAddSchemeDialog *dialog, gpointer user_data )
{
	static const gchar *thisfn = "nact_add_scheme_dialog_on_runtime_init_dialog";
	GtkTreeView *listview;

	g_return_if_fail( NACT_IS_ADD_SCHEME_DIALOG( dialog ));

	if( !dialog->private->dispose_has_run ){
		g_debug( "%s: dialog=%p, user_data=%p", thisfn, ( void * ) dialog, ( void * ) user_data );

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
on_base_all_widgets_showed( NactAddSchemeDialog *dialog, gpointer user_data )
{
	static const gchar *thisfn = "nact_add_scheme_dialog_on_all_widgets_showed";

	g_return_if_fail( NACT_IS_ADD_SCHEME_DIALOG( dialog ));

	if( !dialog->private->dispose_has_run ){
		g_debug( "%s: dialog=%p, user_data=%p", thisfn, ( void * ) dialog, ( void * ) user_data );

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
	GtkWindow *toplevel = base_window_get_toplevel( BASE_WINDOW( dialog ));

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

/* gtk_widget_is_sensitive() appears with Gtk+ 2.17.5 released on 2009-07-18
 * see http://git.gnome.org/browse/gtk+/commit/?id=8f6017622937770082f7b49dfbe135fae5608704
 * GTK_WIDGET_IS_SENSITIVE macro is deprecated since 2.19.7 released on 2010-03-09
 * see http://git.gnome.org/browse/gtk+/commit/?id=a27d5a2c9eba7af5b056de32ff9b2b4dd1eb97e1
 */
#if GTK_CHECK_VERSION( 2, 17, 5 )
	is_sensitive = gtk_widget_is_sensitive( button );
#else
	is_sensitive = GTK_WIDGET_IS_SENSITIVE( button );
#endif

	if( is_sensitive ){
		send_ok( dialog );
	}
}

static void
send_ok( NactAddSchemeDialog *dialog )
{
	GtkWindow *toplevel = base_window_get_toplevel( BASE_WINDOW( dialog ));

	gtk_dialog_response( GTK_DIALOG( toplevel ), GTK_RESPONSE_OK );
}

static void
validate_dialog( NactAddSchemeDialog *dialog )
{
	dialog->private->scheme = nact_schemes_list_get_current_scheme( BASE_WINDOW( dialog ));
}

static gboolean
base_dialog_response( GtkDialog *dialog_box, gint code, BaseWindow *window )
{
	static const gchar *thisfn = "nact_add_scheme_dialog_on_dialog_response";
	NactAddSchemeDialog *dialog;

	g_return_val_if_fail( NACT_IS_ADD_SCHEME_DIALOG( window ), FALSE );

	dialog = NACT_ADD_SCHEME_DIALOG( window );

	if( !dialog->private->dispose_has_run ){
		g_debug( "%s: dialog_box=%p, code=%d, window=%p", thisfn, ( void * ) dialog_box, code, ( void * ) window );

		switch( code ){
			case GTK_RESPONSE_OK:
				validate_dialog( dialog );

			case GTK_RESPONSE_NONE:
			case GTK_RESPONSE_DELETE_EVENT:
			case GTK_RESPONSE_CLOSE:
			case GTK_RESPONSE_CANCEL:
				return( TRUE );
				break;
		}
	}

	return( FALSE );
}
