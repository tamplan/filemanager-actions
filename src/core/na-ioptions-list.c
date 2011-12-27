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

#include <api/na-core-utils.h>

#include "na-gtk-utils.h"
#include "na-ioptions-list.h"

/* private interface data
 */
struct _NAIOptionsListInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* column ordering in the tree view mode
 */
enum {
	IMAGE_COLUMN = 0,
	LABEL_COLUMN,
	TOOLTIP_COLUMN,
	OBJECT_COLUMN,
	N_COLUMN
};

#define IOPTIONS_LIST_DATA_CONTAINER		"ioptions-list-data-container"
#define IOPTIONS_LIST_DATA_EDITABLE			"ioptions-list-data-editable"
#define IOPTIONS_LIST_DATA_INITIALIZED		"ioptions-list-data-initialized"
#define IOPTIONS_LIST_DATA_OPTION			"ioptions-list-data-option"
#define IOPTIONS_LIST_DATA_PARENT			"ioptions-list-data-parent"
#define IOPTIONS_LIST_DATA_SENSITIVE		"ioptions-list-data-sensitive"
#define IOPTIONS_LIST_DATA_WITH_ASK			"ioptions-list-data-with-ask"

static gboolean st_ioptions_list_iface_initialized = FALSE;
static gboolean st_ioptions_list_iface_finalized   = FALSE;

static GType      register_type( void );
static void       interface_base_init( NAIOptionsListInterface *iface );
static void       interface_base_finalize( NAIOptionsListInterface *iface );
static guint      ioptions_list_get_version( const NAIOptionsList *instance );
static void       ioptions_list_free_options( const NAIOptionsList *instance, GList *options );
static void       ioptions_list_free_ask_option( const NAIOptionsList *instance, NAIOption *option );
static GList     *options_list_get_options( const NAIOptionsList *instance );
static void       options_list_free_options( const NAIOptionsList *instance, GList *options );
static NAIOption *options_list_get_ask_option( const NAIOptionsList *instance );
static void       options_list_free_ask_option( const NAIOptionsList *instance, NAIOption *ask_option );
static NAIOption *options_list_get_container_option( GtkWidget *container );
static void       options_list_set_container_option( GtkWidget *container, const NAIOption *option );
static GtkWidget *options_list_get_container_parent( NAIOptionsList *instance );
static void       options_list_set_container_parent( NAIOptionsList *instance, GtkWidget *parent );
static gboolean   options_list_get_editable( NAIOptionsList *instance );
static void       options_list_set_editable( NAIOptionsList *instance, gboolean editable );
static gboolean   options_list_get_initialized( NAIOptionsList *instance );
static void       options_list_set_initialized( NAIOptionsList *instance, gboolean initialized );
static NAIOption *options_list_get_option( NAIOptionsList *instance );
static void       options_list_set_option( NAIOptionsList *instance, NAIOption *option );
static gboolean   options_list_get_sensitive( NAIOptionsList *instance );
static void       options_list_set_sensitive( NAIOptionsList *instance, gboolean sensitive );
static gboolean   options_list_get_with_ask( NAIOptionsList *instance );
static void       options_list_set_with_ask( NAIOptionsList *instance, gboolean with_ask );
static void       check_for_initialized_instance( NAIOptionsList *instance );
static void       on_instance_finalized( gpointer user_data, GObject *instance );
static void       radio_button_create_group( NAIOptionsList *instance );
static void       radio_button_draw_vbox( GtkWidget *container_parent, const NAIOption *option );
static void       radio_button_weak_notify( NAIOption *option, GObject *vbox );
static void       tree_view_create_model( NAIOptionsList *instance );
static void       tree_view_populate( NAIOptionsList *instance );
static void       tree_view_add_item( GtkTreeView *listview, GtkTreeModel *model, const NAIOption *option );
static void       tree_view_weak_notify( GtkTreeModel *model, GObject *tree_view );
static void       radio_button_select_iter( GtkWidget *container_option, NAIOptionsList *instance );
static gboolean   tree_view_select_iter( GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, NAIOptionsList *instance );

/**
 * na_ioptions_list_get_type:
 *
 * Returns: the #GType type of this interface.
 */
GType
na_ioptions_list_get_type( void )
{
	static GType type = 0;

	if( !type ){
		type = register_type();
	}

	return( type );
}

/*
 * na_ioptions_list_register_type:
 *
 * Registers this interface.
 */
static GType
register_type( void )
{
	static const gchar *thisfn = "na_ioptions_list_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NAIOptionsListInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "NAIOptionsList", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( NAIOptionsListInterface *iface )
{
	static const gchar *thisfn = "na_ioptions_list_interface_base_init";

	if( !st_ioptions_list_iface_initialized ){

		g_debug( "%s: iface=%p (%s)", thisfn, ( void * ) iface, G_OBJECT_CLASS_NAME( iface ));

		iface->private = g_new0( NAIOptionsListInterfacePrivate, 1 );

		iface->get_version = ioptions_list_get_version;
		iface->get_options = NULL;
		iface->free_options = ioptions_list_free_options;
		iface->get_ask_option = NULL;
		iface->free_ask_option = ioptions_list_free_ask_option;

		st_ioptions_list_iface_initialized = TRUE;
	}
}

static void
interface_base_finalize( NAIOptionsListInterface *iface )
{
	static const gchar *thisfn = "na_ioptions_list_interface_base_finalize";

	if( st_ioptions_list_iface_initialized && !st_ioptions_list_iface_finalized ){

		g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

		st_ioptions_list_iface_finalized = TRUE;

		g_free( iface->private );
	}
}

/*
 * defaults implemented by the interface
 */
static guint
ioptions_list_get_version( const NAIOptionsList *instance )
{
	return( 1 );
}

static void
ioptions_list_free_options( const NAIOptionsList *instance, GList *options )
{
	static const gchar *thisfn = "na_ioptions_list_free_options";

	g_debug( "%s: instance=%p, options=%p", thisfn, ( void * ) instance, ( void * ) options );

	g_list_foreach( options, ( GFunc ) g_object_unref, NULL );
	g_list_free( options );
}

static void
ioptions_list_free_ask_option( const NAIOptionsList *instance, NAIOption *ask_option )
{
	static const gchar *thisfn = "na_ioptions_list_free_ask_option";

	g_debug( "%s: instance=%p, ask_option=%p", thisfn, ( void * ) instance, ( void * ) ask_option );

	g_object_unref( ask_option );
}

/*
 * call these functions will trigger either the implementation method
 * or the default provided by the interface
 */
static GList *
options_list_get_options( const NAIOptionsList *instance )
{
	GList *options;

	options = NULL;

	if( NA_IOPTIONS_LIST_GET_INTERFACE( instance )->get_options ){
		options = NA_IOPTIONS_LIST_GET_INTERFACE( instance )->get_options( instance );
	}

	return( options );
}

static void
options_list_free_options( const NAIOptionsList *instance, GList *options )
{
	if( NA_IOPTIONS_LIST_GET_INTERFACE( instance )->free_options ){
		NA_IOPTIONS_LIST_GET_INTERFACE( instance )->free_options( instance, options );
	}
}

static NAIOption *
options_list_get_ask_option( const NAIOptionsList *instance )
{
	NAIOption *option;

	option = NULL;

	if( NA_IOPTIONS_LIST_GET_INTERFACE( instance )->get_ask_option ){
		option = NA_IOPTIONS_LIST_GET_INTERFACE( instance )->get_ask_option( instance );
	}

	return( option );
}

static void
options_list_free_ask_option( const NAIOptionsList *instance, NAIOption *ask_option )
{
	if( NA_IOPTIONS_LIST_GET_INTERFACE( instance )->free_ask_option ){
		NA_IOPTIONS_LIST_GET_INTERFACE( instance )->free_ask_option( instance, ask_option );
	}
}

/*
 * get/set properties
 */
/* the option associated to this 'option' container
 */
static NAIOption *
options_list_get_container_option( GtkWidget *container )
{
	NAIOption *option;

	option = ( NAIOption * ) g_object_get_data( G_OBJECT( container ), IOPTIONS_LIST_DATA_CONTAINER );

	return( option );
}

static void
options_list_set_container_option( GtkWidget *container, const NAIOption *option )
{
	g_object_set_data( G_OBJECT( container ), IOPTIONS_LIST_DATA_CONTAINER, ( gpointer ) option );
}

/* the global parent container, i.e. a GtkVBox or a GtkTreeView
 *
 * the container parent is provided when calling na_ioptions_list_display_init()
 * function; storing it as a data of the instance just let us to know it without
 * having to require it in subsequent function calls
 */
static GtkWidget *
options_list_get_container_parent( NAIOptionsList *instance )
{
	GtkWidget *parent;

	parent = ( GtkWidget * ) g_object_get_data( G_OBJECT( instance ), IOPTIONS_LIST_DATA_PARENT );

	return( parent );
}

static void
options_list_set_container_parent( NAIOptionsList *instance, GtkWidget *parent )
{
	g_object_set_data( G_OBJECT( instance ), IOPTIONS_LIST_DATA_PARENT, parent );
}

/* whether the selectable user's preference is editable
 *
 * most of the time, a user's preference is not editable if it is set as mandatory,
 * or if the whole user's preference are not writable
 */
static gboolean
options_list_get_editable( NAIOptionsList *instance )
{
	gboolean editable;

	editable = ( gboolean ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( instance ), IOPTIONS_LIST_DATA_EDITABLE ));

	return( editable );
}

static void
options_list_set_editable( NAIOptionsList *instance, gboolean editable )
{
	g_object_set_data( G_OBJECT( instance ), IOPTIONS_LIST_DATA_EDITABLE, GUINT_TO_POINTER( editable ));
}

/* whether the instance has been initialized
 *
 * initializing the instance let us register a 'weak notify' signal on the instance
 * we will so be able to free any allocated resources when the instance will be
 * finalized
 */
static gboolean
options_list_get_initialized( NAIOptionsList *instance )
{
	gboolean initialized;

	initialized = ( gboolean ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( instance ), IOPTIONS_LIST_DATA_INITIALIZED ));

	return( initialized );
}

static void
options_list_set_initialized( NAIOptionsList *instance, gboolean initialized )
{
	g_object_set_data( G_OBJECT( instance ), IOPTIONS_LIST_DATA_INITIALIZED, GUINT_TO_POINTER( initialized ));
}

/* the current option
 */
static NAIOption *
options_list_get_option( NAIOptionsList *instance )
{
	NAIOption *option;

	option = ( NAIOption * ) g_object_get_data( G_OBJECT( instance ), IOPTIONS_LIST_DATA_OPTION );

	return( option );
}

static void
options_list_set_option( NAIOptionsList *instance, NAIOption *option )
{
	g_object_set_data( G_OBJECT( instance ), IOPTIONS_LIST_DATA_OPTION, option );
}

/* whether the selectable user's preference is sensitive
 *
 * an option should be made insensitive when it is not relevant in
 * the considered case
 */
static gboolean
options_list_get_sensitive( NAIOptionsList *instance )
{
	gboolean sensitive;

	sensitive = ( gboolean ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( instance ), IOPTIONS_LIST_DATA_SENSITIVE ));

	return( sensitive );
}

static void
options_list_set_sensitive( NAIOptionsList *instance, gboolean sensitive )
{
	g_object_set_data( G_OBJECT( instance ), IOPTIONS_LIST_DATA_SENSITIVE, GUINT_TO_POINTER( sensitive ));
}

/* whether the options list must include an 'Ask me' option
 */
static gboolean
options_list_get_with_ask( NAIOptionsList *instance )
{
	gboolean with_ask;

	with_ask = ( gboolean ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( instance ), IOPTIONS_LIST_DATA_WITH_ASK ));

	return( with_ask );
}

static void
options_list_set_with_ask( NAIOptionsList *instance, gboolean with_ask )
{
	g_object_set_data( G_OBJECT( instance ), IOPTIONS_LIST_DATA_WITH_ASK, GUINT_TO_POINTER( with_ask ));
}

/*
 * na_ioptions_list_instance_init:
 * @instance: the object which implements this #NAIOptionsList interface.
 *
 * Initialize all #NAIOptionsList-relative properties of the implementation
 * object at instanciation time.
 */
static void
check_for_initialized_instance( NAIOptionsList *instance )
{
	static const gchar *thisfn = "na_ioptions_list_check_for_initialized_instance";

	if( !options_list_get_initialized( instance )){

		g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

		g_object_weak_ref( G_OBJECT( instance ), ( GWeakNotify ) on_instance_finalized, NULL );

		options_list_set_initialized( instance, TRUE );
	}
}

static void
on_instance_finalized( gpointer user_data, GObject *instance )
{
	static const gchar *thisfn = "na_ioptions_list_on_instance_finalized";

	g_debug( "%s: user_data=%p, instance=%p", thisfn, ( void * ) user_data, ( void * ) instance );
}

/*
 * na_ioptions_list_display_init:
 * @instance: the object which implements this #NAIOptionsList interface.
 * @parent: the #GtkWidget container_parent is which we are goint to setup the items
 *  list. @parent may be a #GtkVBox or a #GtkTreeView.
 * @with_ask: whether we should also display an 'Ask me' option.
 * @editable: whether the selectable user's preference is editable;
 *  Most of time, a user's preference is not editable if it is set as mandatory,
 *  or if the whole user's preferences are not writable.
 * @sensitive: whether the radio button group is to be sensitive;
 *  a widget should not be sensitive if the selectable option is not relevant
 *  in the considered case.
 *
 * Initialize the gtk objects which will be used to display the selection.
 */
void
na_ioptions_list_display_init( NAIOptionsList *instance,
		GtkWidget *parent, gboolean with_ask, gboolean editable, gboolean sensitive )
{
	static const gchar *thisfn = "na_ioptions_list_display_init";

	g_return_if_fail( st_ioptions_list_iface_initialized && !st_ioptions_list_iface_finalized );

	check_for_initialized_instance( instance );

	g_debug( "%s: instance=%p (%s), parent=%p (%s), with_ask=%s, editable=%s, sensitive=%s",
			thisfn,
			( void * ) instance, G_OBJECT_TYPE_NAME( instance ),
			( void * ) parent, G_OBJECT_TYPE_NAME( parent ),
			with_ask ? "True":"False",
			editable ? "True":"False",
			sensitive ? "True":"False" );

	options_list_set_container_parent( instance, parent );
	options_list_set_with_ask( instance, with_ask );
	options_list_set_editable( instance, editable );
	options_list_set_sensitive( instance, sensitive );

	if( GTK_IS_BOX( parent )){
		radio_button_create_group( instance );

	} else if( GTK_IS_TREE_VIEW( parent )){
		tree_view_create_model( instance );
		tree_view_populate( instance );

	} else {
		g_warning( "%s: unknown parent type: %s", thisfn, G_OBJECT_TYPE_NAME( parent ));
	}
}

static void
radio_button_create_group( NAIOptionsList *instance )
{
	static const gchar *thisfn = "na_ioptions_list_create_radio_button_group";
	GList *options, *iopt;
	NAIOption *option;
	GtkWidget *parent;

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

	parent = options_list_get_container_parent( instance );
	options = options_list_get_options( instance );

	/* draw the formats as a group of radio buttons
	 */
	for( iopt = options ; iopt ; iopt = iopt->next ){
		radio_button_draw_vbox( parent, NA_IOPTION( iopt->data ));
	}

	options_list_free_options( instance, options );

	/* eventually add the 'Ask me' mode
	 */
	if( options_list_get_with_ask( instance )){
		option = options_list_get_ask_option( instance );
		radio_button_draw_vbox( parent, option );
		options_list_free_ask_option( instance, option );
	}
}

/*
 * @container_parent_parent used to be a glade-defined GtkVBox in which we dynamically
 * add for each mode:
 *  +- vbox
 *  |   +- radio button + label
 *  |   +- hbox
 *  |   |   +- description (assistant mode only)
 *
 *  Starting with Gtk 3.2, container_parent is a GtkGrid attached to the
 *  glade-defined GtkVBox. For each mode, we are defining:
 *  +- grid
 *  |   +- radio button
 *  |   +- description (assistant mode only)
 *
 *  id=-1 but for the 'Ask me' mode
 */
static void
radio_button_draw_vbox( GtkWidget *container_parent, const NAIOption *option )
{
#if 0
	static const gchar *thisfn = "na_ioptions_list_radio_button_draw_vbox";
#endif
	static GtkRadioButton *first_button = NULL;
	GtkWidget *container_option;
	gchar *description;
	GtkRadioButton *button;
	gchar *label;

#if GTK_CHECK_VERSION( 3,2,0 )
	/* create a grid container_parent which will embed two lines */
	container_option = gtk_grid_new();
#else
	/* create a vbox which will embed two children */
	container_option = gtk_vbox_new( FALSE, 0 );
#endif
	gtk_box_pack_start( GTK_BOX( container_parent ), container_option, FALSE, TRUE, 0 );
	description = na_ioption_get_description( option );
	g_object_set( G_OBJECT( container_option ), "tooltip-text", description, NULL );
	g_free( description );

	/* first line/child is the radio button
	 */
	button = GTK_RADIO_BUTTON( gtk_radio_button_new( NULL ));
	if( !first_button ){
		first_button = button;
	}
	g_object_set( G_OBJECT( button ), "group", first_button, NULL );

#if GTK_CHECK_VERSION( 3, 2, 0 )
	gtk_grid_attach( GTK_GRID( container_option ), GTK_WIDGET( button ), 0, 0, 1, 1 );
#else
	gtk_box_pack_start( GTK_BOX( container_option ), GTK_WIDGET( button ), FALSE, TRUE, 0 );
#endif

	label = na_ioption_get_label( option );
	gtk_button_set_label( GTK_BUTTON( button ), label );
	g_free( label );
	gtk_button_set_use_underline( GTK_BUTTON( button ), TRUE );

#if 0
	vbox_data = g_new0( VBoxData, 1 );
	g_debug( "%s: container_option=%p, allocating VBoxData at %p",
			thisfn, ( void * ) container_option, ( void * ) vbox_data );

	vbox_data->container_parent_vbox = container_parent;
	vbox_data->button = button;
	vbox_data->format = g_object_ref(( gpointer ) format );

	g_object_set_data( G_OBJECT( container_option ), EXPORT_FORMAT_PROP_VBOX_DATA, vbox_data );
	g_object_weak_ref( G_OBJECT( container_option ), ( GWeakNotify ) format_weak_notify, ( gpointer ) vbox_data );
#endif

	options_list_set_container_option( container_option, option );
	g_object_weak_ref( G_OBJECT( container_option ), ( GWeakNotify ) radio_button_weak_notify, ( gpointer ) option );
}

/*
 * release the data structure attached to each individual 'option' container
 * when destroying the window
 */
static void
radio_button_weak_notify( NAIOption *option, GObject *vbox )
{
	static const gchar *thisfn = "na_iptions_list_radio_button_weak_notify";

	g_debug( "%s: option=%p, vbox=%p", thisfn, ( void * ) option, ( void * ) vbox );

	na_ioption_free_option( option );
}

static void
tree_view_create_model( NAIOptionsList *instance )
{
	static const gchar *thisfn = "na_ioptions_list_tree_view_create_model";
	GtkWidget *parent;
	GtkListStore *model;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;

	parent = options_list_get_container_parent( instance );
	g_return_if_fail( GTK_IS_TREE_VIEW( parent ));

	g_debug( "%s: instance=%p, parent=%p", thisfn, ( void * ) instance, ( void * ) parent );

	model = gtk_list_store_new( N_COLUMN, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_OBJECT );
	gtk_tree_view_set_model( GTK_TREE_VIEW( parent ), GTK_TREE_MODEL( model ));
	g_object_unref( model );

	/* create visible columns on the tree view
	 */
	column = gtk_tree_view_column_new_with_attributes(
			"image",
			gtk_cell_renderer_pixbuf_new(),
			"pixbuf", IMAGE_COLUMN,
			NULL );
	gtk_tree_view_append_column( GTK_TREE_VIEW( parent ), column );

	column = gtk_tree_view_column_new_with_attributes(
			"label",
			gtk_cell_renderer_text_new(),
			"text", LABEL_COLUMN,
			NULL );
	gtk_tree_view_append_column( GTK_TREE_VIEW( parent ), column );

	g_object_set( G_OBJECT( parent ), "tooltip-column", TOOLTIP_COLUMN, NULL );

	selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( parent ));
	gtk_tree_selection_set_mode( selection, GTK_SELECTION_BROWSE );

	g_object_weak_ref( G_OBJECT( parent ), ( GWeakNotify ) tree_view_weak_notify, ( gpointer ) model );
}

static void
tree_view_populate( NAIOptionsList *instance )
{
	static const gchar *thisfn = "nact_export_format_tree_view_populate";
	GtkWidget *parent;
	GtkTreeModel *model;
	NAIOption *option;
	GList *options, *iopt;

	parent = options_list_get_container_parent( instance );
	g_return_if_fail( GTK_IS_TREE_VIEW( parent ));

	g_debug( "%s: instance=%p, parent=%p", thisfn, ( void * ) instance, ( void * ) parent );

	model = gtk_tree_view_get_model( GTK_TREE_VIEW( parent ));
	options = options_list_get_options( instance );

	for( iopt = options ; iopt ; iopt = iopt->next ){
		option = NA_IOPTION( iopt->data );
		tree_view_add_item( GTK_TREE_VIEW( parent ), model, option );
	}

	/* eventually add the 'Ask me' mode
	 */
	if( options_list_get_with_ask( instance )){
		option = options_list_get_ask_option( instance );
		tree_view_add_item( GTK_TREE_VIEW( parent ), model, option );
		options_list_free_ask_option( instance, option );
	}
}

static void
tree_view_add_item( GtkTreeView *listview, GtkTreeModel *model, const NAIOption *option )
{
	GtkTreeIter iter;
	gchar *label, *label2, *description;
	GdkPixbuf *pixbuf;

	label = na_ioption_get_label( option );
	label2 = na_core_utils_str_remove_char( label, "_" );
	description = na_ioption_get_description( option );
	pixbuf = na_ioption_get_pixbuf( option );
	gtk_list_store_append( GTK_LIST_STORE( model ), &iter );
	gtk_list_store_set(
			GTK_LIST_STORE( model ),
			&iter,
			IMAGE_COLUMN, pixbuf,
			LABEL_COLUMN, label2,
			TOOLTIP_COLUMN, description,
			OBJECT_COLUMN, option,
			-1 );
	if( pixbuf ){
		g_object_unref( pixbuf );
	}
	g_free( label );
	g_free( label2 );
	g_free( description );
}

/*
 * release the data structure attached to each individual 'option' container
 * when destroying the window
 */
static void
tree_view_weak_notify( GtkTreeModel *model, GObject *tree_view )
{
	static const gchar *thisfn = "na_iptions_list_tree_view_weak_notify";

	g_debug( "%s: model=%p, tree_view=%p", thisfn, ( void * ) model, ( void * ) tree_view );

	gtk_list_store_clear( GTK_LIST_STORE( model ));
}

/*
 * na_ioptions_list_set_default:
 * @instance: the object which implements this #NAIOptionsList interface.
 * @default_option: the #NAIOption to be set as the default.
 *
 * Set the default, either of the radio button group or of the tree view.
 */
void
na_ioptions_list_set_default( NAIOptionsList *instance, NAIOption *default_option )
{
	static const gchar *thisfn = "na_ioptions_list_set_default";
	GtkWidget *parent;
	GtkTreeModel *model;

	if( st_ioptions_list_iface_initialized && !st_ioptions_list_iface_finalized ){
		g_debug( "%s: instance=%p (%s), default_option=%p (%s)",
				thisfn,
				( void * ) instance, G_OBJECT_TYPE_NAME( instance ),
				( void * ) default_option, G_OBJECT_TYPE_NAME( default_option ));

		parent = options_list_get_container_parent( instance );
		g_return_if_fail( GTK_IS_WIDGET( parent ));

		options_list_set_option( instance, default_option );

		if( GTK_IS_BOX( parent )){
			gtk_container_foreach( GTK_CONTAINER( parent ),
					( GtkCallback ) radio_button_select_iter, instance );

		} else if( GTK_IS_TREE_VIEW( parent )){
			model = gtk_tree_view_get_model( GTK_TREE_VIEW( parent ));
			gtk_tree_model_foreach( model, ( GtkTreeModelForeachFunc ) tree_view_select_iter, instance );

		} else {
			g_warning( "%s: unknown parent type: %s", thisfn, G_OBJECT_TYPE_NAME( parent ));
		}
	}
}

/*
 * container_mode is a GtkVBox, or a GtkGrid starting with Gtk 3.2
 *
 * iterating through each radio button of the group:
 * - connecting to 'toggled' signal
 * - activating the button which holds our default value
 */
static void
radio_button_select_iter( GtkWidget *container_option, NAIOptionsList *instance )
{
	NAIOption *option;
	NAIOption *default_option;
	GtkWidget *button;
	gboolean editable, sensitive;

	button = NULL;
	default_option = options_list_get_option( instance );
	option = options_list_get_container_option( container_option );

	if( option == default_option ){
		button = na_gtk_utils_find_widget_by_type( GTK_CONTAINER( container_option ), GTK_TYPE_RADIO_BUTTON );
		g_return_if_fail( GTK_IS_RADIO_BUTTON( button ));
		editable = options_list_get_editable( instance );
		sensitive = options_list_get_sensitive( instance );
		na_gtk_utils_radio_set_initial_state( GTK_RADIO_BUTTON( button ),
				NULL, NULL, editable, sensitive );
	}
}

/*
 * walks through the rows of the liststore until the function returns %TRUE
 */
static gboolean
tree_view_select_iter( GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, NAIOptionsList *instance )
{
	gboolean stop;
	GtkTreeView *tree_view;
	NAIOption *default_option, *option;
	GtkTreeSelection *selection;

	stop = FALSE;
	tree_view = ( GtkTreeView * ) options_list_get_container_parent( instance );
	g_return_val_if_fail( GTK_IS_TREE_VIEW( tree_view ), TRUE );
	default_option = options_list_get_option( instance );

	gtk_tree_model_get( model, iter, OBJECT_COLUMN, &option, -1 );
	g_object_unref( option );

	if( option == default_option ){
		selection = gtk_tree_view_get_selection( tree_view );
		gtk_tree_selection_select_iter( selection, iter );
		stop = TRUE;
	}

	return( stop );
}
