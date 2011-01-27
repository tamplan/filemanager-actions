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
#include <glib/gi18n.h>
#include <libintl.h>

#include <api/na-core-utils.h>

#include <core/na-settings.h>

#include "nact-add-capability-dialog.h"

/* private class data
 */
struct _NactAddCapabilityDialogClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _NactAddCapabilityDialogPrivate {
	gboolean dispose_has_run;
	GSList  *already_used;
	gchar   *capability;
};

/* column ordering in the model
 */
enum {
	CAPABILITY_KEYWORD_COLUMN = 0,
	CAPABILITY_DESC_COLUMN,
	CAPABILITY_ALREADY_USED_COLUMN,
	CAPABILITY_N_COLUMN
};

typedef struct {
	gchar *keyword;
	gchar *desc;
}
	CapabilityTextStruct;

static CapabilityTextStruct st_caps[] = {
		{ "Owner",      N_( "User is the owner of the item" ) },
		{ "Readable",   N_( "Item is readable by the user" ) },
		{ "Writable",   N_( "Item is writable by the user" ) },
		{ "Executable", N_( "Item is executable by the user" ) },
		{ "Local",      N_( "Item is local" ) },
		{ NULL },
};

static const gchar  *st_xmlui_filename = PKGDATADIR "/nact-add-capability.ui";
static const gchar  *st_toplevel_name  = "AddCapabilityDialog";

static GObjectClass *st_parent_class   = NULL;

static GType    register_type( void );
static void     class_init( NactAddCapabilityDialogClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_dispose( GObject *dialog );
static void     instance_finalize( GObject *dialog );

static void     on_base_initialize_gtk_toplevel( NactAddCapabilityDialog *editor, GtkDialog *toplevel );
static void     on_base_initialize_base_window( NactAddCapabilityDialog *editor );
static gchar   *on_base_get_wsp_id( const BaseWindow *window );
static void     on_base_all_widgets_showed( NactAddCapabilityDialog *editor );
static gboolean on_button_press_event( GtkWidget *widget, GdkEventButton *event, NactAddCapabilityDialog *editor );
static void     on_cancel_clicked( GtkButton *button, NactAddCapabilityDialog *editor );
static void     on_ok_clicked( GtkButton *button, NactAddCapabilityDialog *editor );
static void     on_selection_changed( GtkTreeSelection *selection, BaseWindow *window );
static void     display_keyword( GtkTreeViewColumn *column, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, BaseWindow *window );
static void     display_description( GtkTreeViewColumn *column, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, BaseWindow *window );
static void     display_label( GtkTreeViewColumn *column, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, BaseWindow *window, guint column_id );
static gboolean setup_values_iter( GtkTreeModel *model, GtkTreePath *path, GtkTreeIter* iter, GSList *capabilities );
static void     try_for_send_ok( NactAddCapabilityDialog *dialog );
static void     send_ok( NactAddCapabilityDialog *dialog );
static void     on_dialog_ok( BaseDialog *dialog );

GType
nact_add_capability_dialog_get_type( void )
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
	static const gchar *thisfn = "nact_add_capability_dialog_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NactAddCapabilityDialogClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactAddCapabilityDialog ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( BASE_DIALOG_TYPE, "NactAddCapabilityDialog", &info, 0 );

	return( type );
}

static void
class_init( NactAddCapabilityDialogClass *klass )
{
	static const gchar *thisfn = "nact_add_capability_dialog_class_init";
	GObjectClass *object_class;
	BaseWindowClass *base_class;
	BaseDialogClass *dialog_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactAddCapabilityDialogClassPrivate, 1 );

	base_class = BASE_WINDOW_CLASS( klass );
	base_class->get_wsp_id = on_base_get_wsp_id;

	dialog_class = BASE_DIALOG_CLASS( klass );
	dialog_class->ok = on_dialog_ok;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_add_capability_dialog_instance_init";
	NactAddCapabilityDialog *self;

	g_return_if_fail( NACT_IS_ADD_CAPABILITY_DIALOG( instance ));

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );

	self = NACT_ADD_CAPABILITY_DIALOG( instance );

	self->private = g_new0( NactAddCapabilityDialogPrivate, 1 );

	base_window_signal_connect( BASE_WINDOW( instance ),
			G_OBJECT( instance ), BASE_SIGNAL_INITIALIZE_GTK, G_CALLBACK( on_base_initialize_gtk_toplevel ));

	base_window_signal_connect( BASE_WINDOW( instance ),
			G_OBJECT( instance ), BASE_SIGNAL_INITIALIZE_WINDOW, G_CALLBACK( on_base_initialize_base_window ));

	base_window_signal_connect( BASE_WINDOW( instance ),
			G_OBJECT( instance ), BASE_SIGNAL_ALL_WIDGETS_SHOWED, G_CALLBACK( on_base_all_widgets_showed));

	self->private->dispose_has_run = FALSE;
	self->private->capability = NULL;
}

static void
instance_dispose( GObject *dialog )
{
	static const gchar *thisfn = "nact_add_capability_dialog_instance_dispose";
	NactAddCapabilityDialog *self;
	GtkTreeView *listview;
	GtkTreeModel *model;
	GtkTreeSelection *selection;

	g_return_if_fail( NACT_IS_ADD_CAPABILITY_DIALOG( dialog ));

	self = NACT_ADD_CAPABILITY_DIALOG( dialog );

	if( !self->private->dispose_has_run ){
		g_debug( "%s: dialog=%p (%s)", thisfn, ( void * ) dialog, G_OBJECT_TYPE_NAME( dialog ));

		self->private->dispose_has_run = TRUE;

		listview = GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( dialog ), "CapabilitiesTreeView" ));
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
	static const gchar *thisfn = "nact_add_capability_dialog_instance_finalize";
	NactAddCapabilityDialog *self;

	g_return_if_fail( NACT_IS_ADD_CAPABILITY_DIALOG( dialog ));

	g_debug( "%s: dialog=%p (%s)", thisfn, ( void * ) dialog, G_OBJECT_TYPE_NAME( dialog ));

	self = NACT_ADD_CAPABILITY_DIALOG( dialog );

	na_core_utils_slist_free( self->private->already_used );
	g_free( self->private->capability );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( dialog );
	}
}

/**
 * nact_add_capability_dialog_run:
 * @parent: the BaseWindow parent of this dialog
 *  (usually the NactMainWindow).
 * @capabilities: list of already used capabilities.
 *
 * Initializes and runs the dialog.
 *
 * Returns: the selected capability, as a newly allocated string which should
 * be g_free() by the caller, or NULL.
 */
gchar *
nact_add_capability_dialog_run( BaseWindow *parent, GSList *capabilities )
{
	static const gchar *thisfn = "nact_add_capability_dialog_run";
	NactAddCapabilityDialog *dialog;
	gchar *capability;

	g_debug( "%s: parent=%p", thisfn, ( void * ) parent );

	g_return_val_if_fail( BASE_IS_WINDOW( parent ), NULL );

	dialog = g_object_new( NACT_ADD_CAPABILITY_DIALOG_TYPE,
					BASE_PROP_PARENT,         parent,
					BASE_PROP_XMLUI_FILENAME, st_xmlui_filename,
					BASE_PROP_TOPLEVEL_NAME,  st_toplevel_name,
					NULL );

	dialog->private->already_used = na_core_utils_slist_duplicate( capabilities );
	capability = NULL;

	if( base_window_run( BASE_WINDOW( dialog )) == GTK_RESPONSE_OK ){
		capability = g_strdup( dialog->private->capability );
	}

	g_object_unref( dialog );

	return( capability );
}

static void
on_base_initialize_gtk_toplevel( NactAddCapabilityDialog *dialog, GtkDialog *toplevel )
{
	static const gchar *thisfn = "nact_add_capability_dialog_on_base_initialize_gtk_toplevel";
	GtkTreeView *listview;
	GtkTreeModel *model;
	GtkTreeViewColumn *column;
	GtkCellRenderer *text_cell;
	GtkTreeSelection *selection;

	g_return_if_fail( NACT_IS_ADD_CAPABILITY_DIALOG( dialog ));

	if( !dialog->private->dispose_has_run ){
		g_debug( "%s: dialog=%p, toplevel=%p", thisfn, ( void * ) dialog, ( void * ) toplevel );

		listview = GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( dialog ), "CapabilitiesTreeView" ));

		model = GTK_TREE_MODEL( gtk_list_store_new( CAPABILITY_N_COLUMN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN ));
		gtk_tree_view_set_model( listview, GTK_TREE_MODEL( model ));
		g_object_unref( model );

		text_cell = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(
				"capability-keyword",
				text_cell,
				"text", CAPABILITY_KEYWORD_COLUMN,
				NULL );
		gtk_tree_view_append_column( listview, column );
		gtk_tree_sortable_set_sort_column_id( GTK_TREE_SORTABLE( model ), CAPABILITY_KEYWORD_COLUMN, GTK_SORT_ASCENDING );
		gtk_tree_view_column_set_cell_data_func(
				column, text_cell, ( GtkTreeCellDataFunc ) display_keyword, dialog, NULL );

		text_cell = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(
				"capability-description",
				text_cell,
				"text", CAPABILITY_DESC_COLUMN,
				NULL );
		gtk_tree_view_append_column( listview, column );
		gtk_tree_view_column_set_cell_data_func(
				column, text_cell, ( GtkTreeCellDataFunc ) display_description, dialog, NULL );

		gtk_tree_view_set_headers_visible( listview, FALSE );

		selection = gtk_tree_view_get_selection( listview );
		gtk_tree_selection_set_mode( selection, GTK_SELECTION_BROWSE );
	}
}

static void
on_base_initialize_base_window( NactAddCapabilityDialog *dialog )
{
	static const gchar *thisfn = "nact_add_capability_dialog_on_base_initialize_base_window";
	GtkTreeView *listview;
	GtkListStore *model;
	GtkTreeIter row;
	guint i;

	g_return_if_fail( NACT_IS_ADD_CAPABILITY_DIALOG( dialog ));

	if( !dialog->private->dispose_has_run ){
		g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );

		listview = GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( dialog ), "CapabilitiesTreeView" ));
		model = GTK_LIST_STORE( gtk_tree_view_get_model( listview ));

		for( i = 0 ; st_caps[i].keyword ; i = i+1 ){
			gtk_list_store_append( model, &row );
			gtk_list_store_set( model, &row,
					CAPABILITY_KEYWORD_COLUMN, st_caps[i].keyword,
					CAPABILITY_DESC_COLUMN, gettext( st_caps[i].desc ),
					CAPABILITY_ALREADY_USED_COLUMN, FALSE,
					-1 );
		}

		gtk_tree_model_foreach( GTK_TREE_MODEL( model ), ( GtkTreeModelForeachFunc ) setup_values_iter, dialog->private->already_used );

		/* catch double-click */
		base_window_signal_connect( BASE_WINDOW( dialog ),
				G_OBJECT( listview ), "button-press-event", G_CALLBACK( on_button_press_event ));

		base_window_signal_connect( BASE_WINDOW( dialog ),
				G_OBJECT( gtk_tree_view_get_selection( listview )), "changed", G_CALLBACK( on_selection_changed ));

		base_window_signal_connect_by_name( BASE_WINDOW( dialog ),
				"CancelButton", "clicked", G_CALLBACK( on_cancel_clicked ));

		base_window_signal_connect_by_name( BASE_WINDOW( dialog ),
				"OKButton", "clicked", G_CALLBACK( on_ok_clicked ));
	}
}

static gchar *
on_base_get_wsp_id( const BaseWindow *window )
{
	return( g_strdup( NA_IPREFS_CAPABILITY_ADD_CAPABILITY_WSP ));
}

static void
on_base_all_widgets_showed( NactAddCapabilityDialog *dialog )
{
	static const gchar *thisfn = "nact_add_capability_dialog_on_base_all_widgets_showed";
	GtkTreeView *listview;
	GtkTreePath *path;
	GtkTreeSelection *selection;

	g_return_if_fail( NACT_IS_ADD_CAPABILITY_DIALOG( dialog ));

	if( !dialog->private->dispose_has_run ){
		g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );

		listview = GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( dialog ), "CapabilitiesTreeView" ));
		path = gtk_tree_path_new_first();
		selection = gtk_tree_view_get_selection( listview );
		gtk_tree_selection_select_path( selection, path );
		gtk_tree_path_free( path );
	}
}

static gboolean
on_button_press_event( GtkWidget *widget, GdkEventButton *event, NactAddCapabilityDialog *dialog )
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
on_cancel_clicked( GtkButton *button, NactAddCapabilityDialog *dialog )
{
	GtkWindow *toplevel = base_window_get_gtk_toplevel( BASE_WINDOW( dialog ));

	gtk_dialog_response( GTK_DIALOG( toplevel ), GTK_RESPONSE_CLOSE );
}

static void
on_ok_clicked( GtkButton *button, NactAddCapabilityDialog *dialog )
{
	send_ok( dialog );
}

static void
on_selection_changed( GtkTreeSelection *selection, BaseWindow *window )
{
	GList *rows;
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	gboolean used;
	GtkWidget *button;

	rows = gtk_tree_selection_get_selected_rows( selection, &model );
	used = FALSE;

	if( g_list_length( rows ) == 1 ){
		path = ( GtkTreePath * ) rows->data;
		gtk_tree_model_get_iter( model, &iter, path );
		gtk_tree_model_get( model, &iter, CAPABILITY_ALREADY_USED_COLUMN, &used, -1 );
	}

	button = base_window_get_widget( window, "OKButton" );
	gtk_widget_set_sensitive( button, !used );
}

static void
display_keyword( GtkTreeViewColumn *column, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, BaseWindow *window )
{
	display_label( column, cell, model, iter, window, CAPABILITY_KEYWORD_COLUMN );
}

static void
display_description( GtkTreeViewColumn *column, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, BaseWindow *window )
{
	display_label( column, cell, model, iter, window, CAPABILITY_DESC_COLUMN );
}

static void
display_label( GtkTreeViewColumn *column, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, BaseWindow *window, guint column_id )
{
	gboolean used;

	gtk_tree_model_get( model, iter, CAPABILITY_ALREADY_USED_COLUMN, &used, -1 );
	g_object_set( cell, "style-set", FALSE, NULL );

	if( used ){
		g_object_set( cell, "style", PANGO_STYLE_ITALIC, "style-set", TRUE, NULL );
	}
}

static gboolean
setup_values_iter( GtkTreeModel *model, GtkTreePath *path, GtkTreeIter* iter, GSList *capabilities )
{
	gchar *keyword;
	gchar *description, *new_description;

	gtk_tree_model_get( model, iter, CAPABILITY_KEYWORD_COLUMN, &keyword, CAPABILITY_DESC_COLUMN, &description, -1 );

	if( na_core_utils_slist_find_negated( capabilities, keyword )){
		/* i18n: add a comment when a capability is already used by current item */
		new_description = g_strdup_printf( _( "%s (already inserted)"), description );
		gtk_list_store_set( GTK_LIST_STORE( model ), iter, CAPABILITY_DESC_COLUMN, new_description, CAPABILITY_ALREADY_USED_COLUMN, TRUE, -1 );
		g_free( new_description );
	}

	g_free( description );
	g_free( keyword );

	return( FALSE ); /* don't stop looping */
}

static void
try_for_send_ok( NactAddCapabilityDialog *dialog )
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
send_ok( NactAddCapabilityDialog *dialog )
{
	GtkWindow *toplevel = base_window_get_gtk_toplevel( BASE_WINDOW( dialog ));

	gtk_dialog_response( GTK_DIALOG( toplevel ), GTK_RESPONSE_OK );
}

static void
on_dialog_ok( BaseDialog *dialog )
{
	NactAddCapabilityDialog *editor;
	GtkTreeView *listview;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *rows;
	GtkTreePath *path;
	GtkTreeIter iter;

	editor = NACT_ADD_CAPABILITY_DIALOG( dialog );

	listview = GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( editor ), "CapabilitiesTreeView" ));
	selection = gtk_tree_view_get_selection( listview );
	rows = gtk_tree_selection_get_selected_rows( selection, &model );

	if( g_list_length( rows ) == 1 ){
		path = ( GtkTreePath * ) rows->data;
		gtk_tree_model_get_iter( model, &iter, path );
		gtk_tree_model_get( model, &iter, CAPABILITY_KEYWORD_COLUMN, &editor->private->capability, -1 );
	}
}
