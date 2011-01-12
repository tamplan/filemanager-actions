/*
 * Nautilus Actions
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

#include "base-iprefs.h"
#include "nact-application.h"
#include "nact-gtk-utils.h"
#include "nact-icon-chooser.h"

/* private class data
 */
struct _NactIconChooserClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _NactIconChooserPrivate {
	gboolean      dispose_has_run;
	BaseWindow   *main_window;
	const gchar  *initial_icon;
	gchar        *current_icon;
	GtkWidget    *path_preview;
};

#define IPREFS_PANED_WIDTH				"item-icon-chooser-paned-width"
#define IPREFS_LAST_URI					"item-icon-chooser-last-file-uri"
#define IPREFS_WSP						"item-icon-chooser-wsp"

#define VIEW_ICON_SIZE					GTK_ICON_SIZE_DND
#define VIEW_ICON_DEFAULT_WIDTH			32	/* width of the GTK_ICON_SIZE_DND icon size */
#define PREVIEW_ICON_SIZE				GTK_ICON_SIZE_DIALOG
#define PREVIEW_ICON_WIDTH				64
#define CURRENT_ICON_SIZE				GTK_ICON_SIZE_DIALOG

/* column ordering in the Stock model
 */
enum {
	STOCK_NAME_COLUMN = 0,
	STOCK_LABEL_COLUMN,
	STOCK_PIXBUF_COLUMN,
	STOCK_N_COLUMN
};

/* column ordering in the ThemeContext model
 * this is the list store on the left which lets the user select the context
 */
enum {
	THEME_CONTEXT_LABEL_COLUMN = 0,
	THEME_CONTEXT_STORE_COLUMN,
	THEME_CONTEXT_LAST_SELECTED_COLUMN,
	THEME_CONTEXT_N_COLUMN
};

/* column ordering in the ThemeIconView model
 * foreach selected context, we display in the icon view the list of
 * corresponding icons
 */
enum {
	THEME_ICON_LABEL_COLUMN = 0,
	THEME_ICON_PIXBUF_COLUMN,
	THEME_ICON_N_COLUMN
};

static BaseDialogClass *st_parent_class = NULL;

static GType         register_type( void );
static void          class_init( NactIconChooserClass *klass );
static void          instance_init( GTypeInstance *instance, gpointer klass );
static void          instance_dispose( GObject *dialog );
static void          instance_finalize( GObject *dialog );

static NactIconChooser *icon_chooser_new( BaseWindow *parent );

static gchar        *base_get_iprefs_window_id( const BaseWindow *window );
static gchar        *base_get_dialog_name( const BaseWindow *window );
static gchar        *base_get_ui_filename( const BaseWindow *dialog );
static void          on_base_initial_load_dialog( NactIconChooser *editor, gpointer user_data );
static void          do_initialize_themed_icons( NactIconChooser *editor );
static void          do_initialize_icons_by_path( NactIconChooser *editor );
static void          on_base_runtime_init_dialog( NactIconChooser *editor, gpointer user_data );
static void          fillup_themed_icons( NactIconChooser *editor );
static void          fillup_icons_by_path( NactIconChooser *editor );
static void          on_base_all_widgets_showed( NactIconChooser *editor, gpointer user_data );
static void          on_cancel_clicked( GtkButton *button, NactIconChooser *editor );
static void          on_ok_clicked( GtkButton *button, NactIconChooser *editor );
static gboolean      base_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window );
static void          on_current_icon_changed( const NactIconChooser *editor );
static gboolean      on_destroy( GtkWidget *widget, GdkEvent *event, void *foo );
static gboolean      on_icon_view_button_press_event( GtkWidget *widget, GdkEventButton *event, NactIconChooser *editor );
static gboolean      on_key_pressed_event( GtkWidget *widget, GdkEventKey *event, NactIconChooser *editor );
static void          on_themed_context_changed( GtkTreeSelection *selection, NactIconChooser *editor );
static void          on_themed_icon_changed( GtkIconView *icon_view, NactIconChooser *editor );
static void          on_themed_apply_button_clicked( GtkButton *button, NactIconChooser *editor );
static void          on_themed_apply_triggered( NactIconChooser *editor );
static void          on_path_selection_changed( GtkFileChooser *chooser, NactIconChooser *editor );
static void          on_path_update_preview( GtkFileChooser *chooser, NactIconChooser *editor );
static void          on_path_apply_button_clicked( GtkButton *button, NactIconChooser *editor );
static GtkListStore *theme_context_load_icons( NactIconChooser *editor, const gchar *context );

GType
nact_icon_chooser_get_type( void )
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
	static const gchar *thisfn = "nact_icon_chooser_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NactIconChooserClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactIconChooser ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( BASE_DIALOG_TYPE, "NactIconChooser", &info, 0 );

	return( type );
}

static void
class_init( NactIconChooserClass *klass )
{
	static const gchar *thisfn = "nact_icon_chooser_class_init";
	GObjectClass *object_class;
	BaseWindowClass *base_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactIconChooserClassPrivate, 1 );

	base_class = BASE_WINDOW_CLASS( klass );
	base_class->dialog_response = base_dialog_response;
	base_class->get_toplevel_name = base_get_dialog_name;
	base_class->get_iprefs_window_id = base_get_iprefs_window_id;
	base_class->get_ui_filename = base_get_ui_filename;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_icon_chooser_instance_init";
	NactIconChooser *self;

	g_return_if_fail( NACT_IS_ICON_CHOOSER( instance ));

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self = NACT_ICON_CHOOSER( instance );

	self->private = g_new0( NactIconChooserPrivate, 1 );

	base_window_signal_connect(
			BASE_WINDOW( self ),
			G_OBJECT( self ),
			BASE_WINDOW_SIGNAL_INITIAL_LOAD,
			G_CALLBACK( on_base_initial_load_dialog ));

	base_window_signal_connect(
			BASE_WINDOW( self ),
			G_OBJECT( self ),
			BASE_WINDOW_SIGNAL_RUNTIME_INIT,
			G_CALLBACK( on_base_runtime_init_dialog ));

	base_window_signal_connect(
			BASE_WINDOW( self ),
			G_OBJECT( self ),
			BASE_WINDOW_SIGNAL_ALL_WIDGETS_SHOWED,
			G_CALLBACK( on_base_all_widgets_showed));

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *dialog )
{
	static const gchar *thisfn = "nact_icon_chooser_instance_dispose";
	NactIconChooser *self;
	guint pos;
	GtkWidget *paned;

	g_return_if_fail( NACT_IS_ICON_CHOOSER( dialog ));

	g_debug( "%s: dialog=%p (%s)", thisfn, ( void * ) dialog, G_OBJECT_TYPE_NAME( dialog ));

	self = NACT_ICON_CHOOSER( dialog );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		paned = base_window_get_widget( BASE_WINDOW( self ), "IconPaned" );
		pos = gtk_paned_get_position( GTK_PANED( paned ));
		base_iprefs_set_int( BASE_WINDOW( self ), IPREFS_PANED_WIDTH, pos );

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( dialog );
		}
	}
}

static void
instance_finalize( GObject *dialog )
{
	static const gchar *thisfn = "nact_icon_chooser_instance_finalize";
	NactIconChooser *self;

	g_return_if_fail( NACT_IS_ICON_CHOOSER( dialog ));

	g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );

	self = NACT_ICON_CHOOSER( dialog );

	g_free( self->private->current_icon );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( dialog );
	}
}

/*
 * Returns a newly allocated NactIconChooser object.
 */
static NactIconChooser *
icon_chooser_new( BaseWindow *parent )
{
	return( g_object_new( NACT_ICON_CHOOSER_TYPE, BASE_WINDOW_PROP_PARENT, parent, NULL ));
}

/**
 * nact_icon_chooser_choose_icon:
 * @parent: the #BaseWindow parent of this dialog.
 * @icon_name: the current icon at startup.
 *
 * Initializes and runs the dialog.
 *
 * This dialog lets the user choose an icon, either as the name of a
 * themed icon, or as the path of an image.
 *
 * Returns: the selected icon, as a new string which should be g_free()
 * by the caller.
 */
gchar *
nact_icon_chooser_choose_icon( BaseWindow *parent, const gchar *icon_name )
{
	static const gchar *thisfn = "nact_icon_chooser_choose_icon";
	NactIconChooser *editor;
	gchar *new_name;

	g_return_val_if_fail( BASE_IS_WINDOW( parent ), NULL );

	g_debug( "%s: parent=%p, icon_name=%s", thisfn, ( void * ) parent, icon_name );

	editor = icon_chooser_new( parent );
	editor->private->main_window = parent;
	editor->private->initial_icon = icon_name;

	new_name = g_strdup( editor->private->initial_icon );

	if( base_window_run( BASE_WINDOW( editor ))){
		g_free( new_name );
		new_name = g_strdup( editor->private->current_icon );
	}

	g_object_unref( editor );

	return( new_name );
}

static gchar *
base_get_iprefs_window_id( const BaseWindow *window )
{
	return( g_strdup( IPREFS_WSP ));
}

static gchar *
base_get_dialog_name( const BaseWindow *window )
{
	return( g_strdup( "IconChooserDialog" ));
}

static gchar *
base_get_ui_filename( const BaseWindow *dialog )
{
	return( g_strdup( PKGDATADIR "/nact-icon-chooser.ui" ));
}

static void
on_base_initial_load_dialog( NactIconChooser *editor, gpointer user_data )
{
	static const gchar *thisfn = "nact_icon_chooser_on_initial_load_dialog";

	g_return_if_fail( NACT_IS_ICON_CHOOSER( editor ));

	g_debug( "%s: editor=%p, user_data=%p", thisfn, ( void * ) editor, ( void * ) user_data );

	/* initialize the notebook
	 */
	do_initialize_themed_icons( editor );
	do_initialize_icons_by_path( editor );

	/* destroy event
	 * there is here that we are going to release our stores
	 */
	GtkDialog *dialog = GTK_DIALOG( base_window_get_toplevel( BASE_WINDOW( editor )));
	g_signal_connect( G_OBJECT( dialog ), "destroy", G_CALLBACK( on_destroy ), NULL );
}

/*
 * initialize the themed icon tab
 * first, the listview which handles the context list
 * each context carries a list store which handles the corresponding icons
 * this store is initialized the first time the context is selected
 */
static void
do_initialize_themed_icons( NactIconChooser *editor )
{
	GtkTreeView *context_view;
	GtkTreeModel *context_model;
	GtkCellRenderer *text_cell;
	GtkTreeViewColumn *column;
	GtkIconView *icon_view;
	GtkTreeSelection *selection;
	GtkIconTheme *icon_theme;
	GList *theme_contexts, *it;
	const gchar *context_label;
	GtkTreeIter iter;

	context_view = GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( editor ), "ThemedTreeView" ));
	context_model = GTK_TREE_MODEL(
			gtk_list_store_new( THEME_CONTEXT_N_COLUMN,
					G_TYPE_STRING, G_TYPE_OBJECT, G_TYPE_STRING ));
	gtk_tree_view_set_model( context_view, context_model );
	gtk_tree_view_set_headers_visible( context_view, FALSE );

	text_cell = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
			"theme-context",
			text_cell,
			"text", THEME_CONTEXT_LABEL_COLUMN,
			NULL );
	gtk_tree_view_append_column( context_view, column );

	icon_view = GTK_ICON_VIEW( base_window_get_widget( BASE_WINDOW( editor ), "ThemedIconView" ));
	gtk_icon_view_set_text_column( icon_view, THEME_ICON_LABEL_COLUMN );
	gtk_icon_view_set_pixbuf_column( icon_view, THEME_ICON_PIXBUF_COLUMN );
	gtk_icon_view_set_selection_mode( icon_view, GTK_SELECTION_BROWSE );

	selection = gtk_tree_view_get_selection( context_view );
	gtk_tree_selection_set_mode( selection, GTK_SELECTION_BROWSE );

	icon_theme = gtk_icon_theme_get_default();
	theme_contexts = g_list_sort(
			gtk_icon_theme_list_contexts( icon_theme ), ( GCompareFunc ) g_utf8_collate );

	for( it = theme_contexts ; it ; it = it->next ){
		context_label = ( const gchar *) it->data;
		gtk_list_store_append( GTK_LIST_STORE( context_model ), &iter );
		gtk_list_store_set( GTK_LIST_STORE( context_model ), &iter,
				THEME_CONTEXT_LABEL_COLUMN, context_label,
				THEME_CONTEXT_STORE_COLUMN, NULL,
				-1 );
	}
	g_list_foreach( theme_contexts, ( GFunc ) g_free, NULL );
	g_list_free( theme_contexts );

	g_object_unref( context_model );
}

static void
do_initialize_icons_by_path( NactIconChooser *editor )
{
	GtkFileChooser *file_chooser;

	file_chooser = GTK_FILE_CHOOSER( base_window_get_widget( BASE_WINDOW( editor ), "FileChooser" ));
	gtk_file_chooser_set_action( file_chooser, GTK_FILE_CHOOSER_ACTION_OPEN );
	gtk_file_chooser_set_select_multiple( file_chooser, FALSE );
}

static void
on_base_runtime_init_dialog( NactIconChooser *editor, gpointer user_data )
{
	static const gchar *thisfn = "nact_icon_chooser_on_runtime_init_dialog";
	guint pos;
	GtkWidget *paned;

	g_return_if_fail( NACT_IS_ICON_CHOOSER( editor ));

	g_debug( "%s: editor=%p, user_data=%p", thisfn, ( void * ) editor, ( void * ) user_data );

	pos = base_iprefs_get_int( BASE_WINDOW( editor ), IPREFS_PANED_WIDTH );
	if( pos ){
		paned = base_window_get_widget( BASE_WINDOW( editor ), "IconPaned" );
		gtk_paned_set_position( GTK_PANED( paned ), pos );
	}

	/* setup the initial icon
	 */
	editor->private->current_icon = g_strdup( editor->private->initial_icon );
	on_current_icon_changed( editor );

	/* fillup the icon stores
	 */
	fillup_themed_icons( editor );
	fillup_icons_by_path( editor );

	/*  intercept Escape key: we do not quit on Esc.
	 */
	base_window_signal_connect(
					BASE_WINDOW( editor ),
					G_OBJECT( base_window_get_toplevel( BASE_WINDOW( editor ))),
					"key-press-event",
					G_CALLBACK( on_key_pressed_event ));

	/* OK/Cancel buttons
	 */
	base_window_signal_connect_by_name(
			BASE_WINDOW( editor ), "CancelButton", "clicked", G_CALLBACK( on_cancel_clicked ));

	base_window_signal_connect_by_name(
			BASE_WINDOW( editor ), "OKButton", "clicked", G_CALLBACK( on_ok_clicked ));
}

static void
fillup_themed_icons( NactIconChooser *editor )
{
	GtkTreeView *context_view;
	GtkTreeSelection *selection;
	GtkTreePath *path;
	GtkIconView *icon_view;

	icon_view = GTK_ICON_VIEW( base_window_get_widget( BASE_WINDOW( editor ), "ThemedIconView" ));
	base_window_signal_connect(
			BASE_WINDOW( editor ), G_OBJECT( icon_view ), "selection-changed", G_CALLBACK( on_themed_icon_changed ));
	/* catch double-click */
	base_window_signal_connect(
			BASE_WINDOW( editor ), G_OBJECT( icon_view ), "button-press-event", G_CALLBACK( on_icon_view_button_press_event ));

	context_view = GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( editor ), "ThemedTreeView" ));
	selection = gtk_tree_view_get_selection( context_view );
	base_window_signal_connect(
			BASE_WINDOW( editor ), G_OBJECT( selection ), "changed", G_CALLBACK( on_themed_context_changed ));

	path = gtk_tree_path_new_first();
	gtk_tree_selection_select_path( selection, path );
	gtk_tree_path_free( path );

	base_window_signal_connect_by_name(
			BASE_WINDOW( editor ), "ThemedApplyButton", "clicked", G_CALLBACK( on_themed_apply_button_clicked ));
}

static void
fillup_icons_by_path( NactIconChooser *editor )
{
	GtkFileChooser *file_chooser;
	NactApplication *application;
	NAUpdater *updater;
	NASettings *settings;
	gchar *uri;

	file_chooser = GTK_FILE_CHOOSER( base_window_get_widget( BASE_WINDOW( editor ), "FileChooser" ));
	editor->private->path_preview = gtk_image_new();
	gtk_file_chooser_set_preview_widget( file_chooser, editor->private->path_preview );

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( editor )));
	updater = nact_application_get_updater( application );
	settings = na_pivot_get_settings( NA_PIVOT( updater ));

	gtk_file_chooser_unselect_all( file_chooser );

	uri = na_settings_get_string( settings, IPREFS_LAST_URI, NULL, NULL );
	if( uri ){
		gtk_file_chooser_set_uri( file_chooser, uri );
		g_free( uri );
	} else if( editor->private->current_icon ){
		gtk_file_chooser_set_filename( file_chooser, editor->private->current_icon );
	}

	base_window_signal_connect(
				BASE_WINDOW( editor ), G_OBJECT( file_chooser ), "selection-changed", G_CALLBACK( on_path_selection_changed ));
	base_window_signal_connect(
				BASE_WINDOW( editor ), G_OBJECT( file_chooser ), "update-preview", G_CALLBACK( on_path_update_preview ));

	base_window_signal_connect_by_name(
				BASE_WINDOW( editor ), "PathApplyButton", "clicked", G_CALLBACK( on_path_apply_button_clicked ));
}

static void
on_base_all_widgets_showed( NactIconChooser *editor, gpointer user_data )
{
	static const gchar *thisfn = "nact_icon_chooser_on_all_widgets_showed";
	GtkWidget *about_button;

	g_return_if_fail( NACT_IS_ICON_CHOOSER( editor ));

	g_debug( "%s: editor=%p, user_data=%p", thisfn, ( void * ) editor, ( void * ) user_data );

	/* hide about button not used here
	 */
	about_button = base_window_get_widget( BASE_WINDOW( editor ), "AboutButton" );
	gtk_widget_hide( about_button );
}

static void
on_cancel_clicked( GtkButton *button, NactIconChooser *editor )
{
	GtkWindow *toplevel = base_window_get_toplevel( BASE_WINDOW( editor ));
	gtk_dialog_response( GTK_DIALOG( toplevel ), GTK_RESPONSE_CLOSE );
}

static void
on_ok_clicked( GtkButton *button, NactIconChooser *editor )
{
	GtkWindow *toplevel = base_window_get_toplevel( BASE_WINDOW( editor ));
	gtk_dialog_response( GTK_DIALOG( toplevel ), GTK_RESPONSE_OK );
}

static gboolean
base_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window )
{
	static const gchar *thisfn = "nact_icon_chooser_on_dialog_response";
	NactIconChooser *editor;

	g_return_val_if_fail( NACT_IS_ICON_CHOOSER( window ), FALSE );

	g_debug( "%s: dialog=%p, code=%d, window=%p", thisfn, ( void * ) dialog, code, ( void * ) window );

	editor = NACT_ICON_CHOOSER( window );

	switch( code ){
		case GTK_RESPONSE_NONE:
		case GTK_RESPONSE_DELETE_EVENT:
		case GTK_RESPONSE_CLOSE:
		case GTK_RESPONSE_CANCEL:

			g_free( editor->private->current_icon );
			editor->private->current_icon = g_strdup( editor->private->initial_icon );
			return( TRUE );
			break;

		case GTK_RESPONSE_OK:
			return( TRUE );
			break;
	}

	return( FALSE );
}

/*
 * display at the top of the dialog the icon addressed in @icon
 * this is this icon which will be returned if the user validates
 * this dialog
 */
static void
on_current_icon_changed( const NactIconChooser *editor )
{
	GtkImage *image;
	gchar *icon_label;
	GtkLabel *label;

	image = GTK_IMAGE( base_window_get_widget( BASE_WINDOW( editor ), "IconImage" ));
	nact_gtk_utils_render( editor->private->current_icon, image, CURRENT_ICON_SIZE );

	if( editor->private->current_icon ){
		if( g_path_is_absolute( editor->private->current_icon )){
			icon_label = g_filename_to_utf8( editor->private->current_icon, -1, NULL, NULL, NULL );
		} else {
			icon_label = g_strdup( editor->private->current_icon );
		}
		label = GTK_LABEL( base_window_get_widget( BASE_WINDOW( editor ), "IconLabel" ));
		gtk_label_set_label( label, icon_label );
		g_free( icon_label );
	}
}

static gboolean
on_destroy( GtkWidget *widget, GdkEvent *event, void *foo )
{
	static const gchar *thisfn = "nact_icon_chooser_on_destroy";
	GtkTreeView *context_view;
	GtkListStore *context_store;
	GtkTreeIter context_iter;
	GtkListStore *icon_store;
	gchar *context_label;

	g_debug( "%s: widget=%p", thisfn, ( void * ) widget );

	/* clear the various models
	 */
	context_view = GTK_TREE_VIEW( base_window_peek_widget( GTK_WINDOW( widget ), "ThemedTreeView" ));
	context_store = GTK_LIST_STORE( gtk_tree_view_get_model( context_view ));

	if( gtk_tree_model_get_iter_first( GTK_TREE_MODEL( context_store ), &context_iter )){
		while( TRUE ){

			gtk_tree_model_get( GTK_TREE_MODEL( context_store ), &context_iter,
					THEME_CONTEXT_LABEL_COLUMN, &context_label,
					THEME_CONTEXT_STORE_COLUMN, &icon_store,
					-1 );
			if( icon_store ){
				g_debug( "%s: context=%s, clearing store=%p", thisfn, context_label, ( void * ) icon_store );
				gtk_list_store_clear( icon_store );
				g_object_unref( icon_store );
			}

			g_free( context_label );

			if( !gtk_tree_model_iter_next( GTK_TREE_MODEL( context_store ), &context_iter )){
				break;
			}
		}
	}

	gtk_list_store_clear( context_store );

	/* let other handlers get this message */
	return( FALSE );
}

/*
 * mouse click on the themed icons icon view
 */
static gboolean
on_icon_view_button_press_event( GtkWidget *widget, GdkEventButton *event, NactIconChooser *editor )
{
	gboolean stop = FALSE;

	/* double-click of left button
	 * > triggers a 'Apply' action
	 */
	if( event->type == GDK_2BUTTON_PRESS && event->button == 1 ){
		on_themed_apply_triggered( editor );
		stop = TRUE;
	}

	return( stop );
}

static gboolean
on_key_pressed_event( GtkWidget *widget, GdkEventKey *event, NactIconChooser *editor )
{
	gboolean stop = FALSE;

	g_return_val_if_fail( NACT_IS_ICON_CHOOSER( editor ), FALSE );

	if( !editor->private->dispose_has_run ){

		/* inhibit Escape key */
		if( event->keyval == GDK_Escape ){
			stop = TRUE;
		}
	}

	return( stop );
}

static void
on_themed_context_changed( GtkTreeSelection *selection, NactIconChooser *editor )
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkListStore *store;
	gchar *context, *last_path;
	GtkTreePath *path;
	GtkWidget *preview_image, *preview_label;

	if( gtk_tree_selection_get_selected( selection, &model, &iter )){
		gtk_tree_model_get( model, &iter,
				THEME_CONTEXT_LABEL_COLUMN, &context,
				THEME_CONTEXT_STORE_COLUMN, &store,
				THEME_CONTEXT_LAST_SELECTED_COLUMN, &last_path,
				-1 );

		if( !store ){
			store = theme_context_load_icons( editor, context );
			gtk_list_store_set( GTK_LIST_STORE( model ), &iter, THEME_CONTEXT_STORE_COLUMN, store, -1 );
		}

		GtkIconView *iconview = GTK_ICON_VIEW( base_window_get_widget( BASE_WINDOW( editor ), "ThemedIconView" ));
		gtk_icon_view_set_model( iconview, GTK_TREE_MODEL( store ));

		if( last_path ){
			path = gtk_tree_path_new_from_string( last_path );
			gtk_icon_view_select_path( iconview, path );
			gtk_tree_path_free( path );

		} else {
			preview_image = base_window_get_widget( BASE_WINDOW( editor ), "ThemedIconImage" );
			gtk_image_set_from_pixbuf( GTK_IMAGE( preview_image ), NULL );
			preview_label = base_window_get_widget( BASE_WINDOW( editor ), "ThemedIconName" );
			gtk_label_set_text( GTK_LABEL( preview_label ), "" );
		}

		g_free( last_path );
		g_free( context );
		g_object_unref( store );
	}
}

static void
on_themed_icon_changed( GtkIconView *icon_view, NactIconChooser *editor )
{
	GList *selected;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *label;
	GtkWidget *preview_image, *preview_label;
	GtkTreeView *context_view;
	GtkListStore *context_store;
	GtkTreeSelection *context_selection;
	GtkTreeIter context_iter;
	gchar *icon_path;

	selected = gtk_icon_view_get_selected_items( icon_view );
	if( selected ){
		model = gtk_icon_view_get_model( icon_view );

		if( gtk_tree_model_get_iter( model, &iter, ( GtkTreePath * ) selected->data )){
			gtk_tree_model_get( model, &iter,
					THEME_ICON_LABEL_COLUMN, &label,
					-1 );

			preview_image = base_window_get_widget( BASE_WINDOW( editor ), "ThemedIconImage" );
			nact_gtk_utils_render( label, GTK_IMAGE( preview_image ), PREVIEW_ICON_SIZE );
			preview_label = base_window_get_widget( BASE_WINDOW( editor ), "ThemedIconName" );
			gtk_label_set_text( GTK_LABEL( preview_label ), label );

			/* record in context tree view the path to the last selected icon
			 */
			context_view = GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( editor ), "ThemedTreeView" ));
			context_selection = gtk_tree_view_get_selection( context_view );
			if( gtk_tree_selection_get_selected( context_selection, ( GtkTreeModel ** ) &context_store, &context_iter )){
				icon_path = gtk_tree_model_get_string_from_iter( model, &iter );
				gtk_list_store_set( context_store, &context_iter, THEME_CONTEXT_LAST_SELECTED_COLUMN, icon_path, -1 );
				g_free( icon_path );
			}

			g_free( label );
		}

		g_list_foreach( selected, ( GFunc ) gtk_tree_path_free, NULL );
		g_list_free( selected );
	}
}

static void
on_themed_apply_button_clicked( GtkButton *button, NactIconChooser *editor )
{
	on_themed_apply_triggered( editor );
}

static void
on_themed_apply_triggered( NactIconChooser *editor )
{
	GtkWidget *icon_label;
	const gchar *icon_name;

	icon_label = base_window_get_widget( BASE_WINDOW( editor ), "ThemedIconName" );
	icon_name = gtk_label_get_text( GTK_LABEL( icon_label ));

	g_free( editor->private->current_icon );
	editor->private->current_icon = g_strdup( icon_name );
	on_current_icon_changed( editor );
}

static void
on_path_selection_changed( GtkFileChooser *file_chooser, NactIconChooser *editor )
{
	gchar *uri;
	NactApplication *application;
	NAUpdater *updater;
	NASettings *settings;

	uri = gtk_file_chooser_get_uri( file_chooser );
	if( uri ){
		application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( editor )));
		updater = nact_application_get_updater( application );
		settings = na_pivot_get_settings( NA_PIVOT( updater ));
		na_settings_set_string( settings, IPREFS_LAST_URI, uri );
		g_free( uri );
	}
}

static void
on_path_update_preview( GtkFileChooser *file_chooser, NactIconChooser *editor )
{
	static const gchar *thisfn = "nact_icon_chooser_on_path_update_preview";
	char *filename;
	GdkPixbuf *pixbuf;
	gboolean have_preview;
	gint width, height;

	if( !gtk_icon_size_lookup( PREVIEW_ICON_SIZE, &width, &height )){
		width = PREVIEW_ICON_WIDTH;
		height = PREVIEW_ICON_WIDTH;
	}

	have_preview = FALSE;
	filename = gtk_file_chooser_get_preview_filename( file_chooser );
	g_debug( "%s: file_chooser=%p, editor=%p, filename=%s",
			thisfn, ( void * ) file_chooser, ( void * ) editor, filename );

	if( filename ){
		pixbuf = gdk_pixbuf_new_from_file_at_size( filename, width, height, NULL );
		have_preview = ( pixbuf != NULL );
		g_free( filename );
	}

	if( have_preview ){
		gtk_image_set_from_pixbuf( GTK_IMAGE( editor->private->path_preview ), pixbuf );
		g_object_unref( pixbuf );
	}

	gtk_file_chooser_set_preview_widget_active( file_chooser, TRUE );
}

static void
on_path_apply_button_clicked( GtkButton *button, NactIconChooser *editor )
{
	GtkFileChooser *file_chooser = GTK_FILE_CHOOSER( base_window_get_widget( BASE_WINDOW( editor ), "FileChooser" ));

	/* this is a filename in the character set specified by the G_FILENAME_ENCODING
	 * environment variable
	 */
	g_free( editor->private->current_icon );
	editor->private->current_icon = gtk_file_chooser_get_filename( file_chooser );
	on_current_icon_changed( editor );
}

static GtkListStore *
theme_context_load_icons( NactIconChooser *editor, const gchar *context )
{
	static const gchar *thisfn = "nact_icon_chooser_theme_context_load_icons";
	GtkTreeIter iter;
	GList *ic;
	GError *error;
	gint width, height;

	g_debug( "%s: editor=%p, context=%s", thisfn, ( void * ) editor, context );

	GtkIconTheme *icon_theme = gtk_icon_theme_get_default();
	GtkListStore *store = gtk_list_store_new( THEME_ICON_N_COLUMN, G_TYPE_STRING, GDK_TYPE_PIXBUF );

	GList *icon_list = g_list_sort( gtk_icon_theme_list_icons( icon_theme, context ), ( GCompareFunc ) g_utf8_collate );

	if( !gtk_icon_size_lookup( VIEW_ICON_SIZE, &width, &height )){
		width = VIEW_ICON_DEFAULT_WIDTH;
	}
	g_debug( "%s: width=%d", thisfn, width );

	for( ic = icon_list ; ic ; ic = ic->next ){
		const gchar *icon_name = ( const gchar * ) ic->data;
		error = NULL;
		GdkPixbuf *pixbuf = gtk_icon_theme_load_icon(
				icon_theme, icon_name, width, GTK_ICON_LOOKUP_GENERIC_FALLBACK, &error );
		if( error ){
			g_warning( "%s: %s", thisfn, error->message );
			g_error_free( error );
		} else {
			gtk_list_store_append( store, &iter );
			gtk_list_store_set( store, &iter,
					THEME_ICON_LABEL_COLUMN, icon_name,
					THEME_ICON_PIXBUF_COLUMN, pixbuf,
					-1 );
			g_object_unref( pixbuf );
		}
	}
	g_debug( "%s: %d loaded icons in store=%p", thisfn, g_list_length( icon_list ), ( void * ) store );
	g_list_foreach( icon_list, ( GFunc ) g_free, NULL );
	g_list_free( icon_list );

	return( store );
}
