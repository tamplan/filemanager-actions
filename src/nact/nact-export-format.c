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

#include <glib/gi18n.h>

#include <core/na-exporter.h>
#include <core/na-export-format.h>

#include "nact-export-format.h"
#include "base-gtk-utils.h"

/*
 * As of Gtk 3.2.0, GtkHBox and GtkVBox are deprecated. It is adviced
 * to replace them with a GtkGrid.
 * In this dialog box, we have a glade-defined VBox, said 'container_vbox',
 * in which we dynamically embed the radio buttons as a hierarchy of
 * VBoxes and HBoxes.
 * While it is still possible to keep the glade-defined VBoxes, we will
 * stay stuck with our VBox 'container_vbox', replacing only dynamically
 * allocated GtkHBoxes and GtkVBoxes with one GtkGrid.
 */
typedef struct {
	GtkWidget      *container_vbox;
	GtkRadioButton *button;
	NAExportFormat *format;
	gulong          handler;	/* 'toggled' signal handler id */
}
	VBoxData;

#define EXPORT_FORMAT_PROP_CONTAINER_FORMAT		"nact-export-format-prop-container-format"
#define EXPORT_FORMAT_PROP_CONTAINER_EDITABLE	"nact-export-format-prop-container-editable"
#define EXPORT_FORMAT_PROP_CONTAINER_SENSITIVE	"nact-export-format-prop-container-sensitive"
#define EXPORT_FORMAT_PROP_VBOX_DATA			"nact-export-format-prop-vbox-data"

#define ASKME_LABEL						N_( "_Ask me" )
#define ASKME_DESCRIPTION				N_( "You will be asked for the format to choose each time an item is about to be exported." )

static const NAIExporterFormat st_ask_str = { "Ask", ASKME_LABEL, ASKME_DESCRIPTION };

static void draw_in_vbox( GtkWidget *container, const NAExportFormat *format, guint mode, gint id );
static void format_weak_notify( VBoxData *vbox_data, GObject *vbox );
static void select_default_iter( GtkWidget *widget, void *quark_ptr );
static void export_format_on_toggled( GtkToggleButton *toggle_button, VBoxData *vbox_data );
static void get_selected_iter( GtkWidget *widget, NAExportFormat **format );

/**
 * nact_export_format_init_display:
 * @container_vbox: the #GtkVBox container in which the display must be drawn.
 * @pivot: the #NAPivot instance.
 * @mode: the type of the display.
 * @sensitive: whether the whole radio button group is sensitive.
 *
 * Displays the available export formats in the VBox.
 *
 * Should only be called once per dialog box instance (e.g. on initial load
 * of the GtkWindow).
 */
void
nact_export_format_init_display( GtkWidget *container_vbox, const NAPivot *pivot, guint mode, gboolean sensitive )
{
	static const gchar *thisfn = "nact_export_format_init_display";
	GList *formats, *ifmt;
	NAExportFormat *format;

	g_debug( "%s: container_vbox=%p, pivot=%p, mode=%u, sensitive=%s",
			thisfn, ( void * ) container_vbox, ( void * ) pivot, mode,
			sensitive ? "True":"False" );

	formats = na_exporter_get_formats( pivot );

	for( ifmt = formats ; ifmt ; ifmt = ifmt->next ){
		draw_in_vbox( container_vbox, NA_EXPORT_FORMAT( ifmt->data ), mode, -1 );
	}

	na_exporter_free_formats( formats );

	switch( mode ){

		/* this is the mode to be used when we are about to export an item
		 * and the user preference is 'Ask me'; obviously, we don't propose
		 * here to ask him another time :)
		 */
		case EXPORT_FORMAT_DISPLAY_ASK:
			break;

		case EXPORT_FORMAT_DISPLAY_PREFERENCES:
		case EXPORT_FORMAT_DISPLAY_ASSISTANT:
			format = na_export_format_new( &st_ask_str, NULL );
			draw_in_vbox( container_vbox, format, mode, IPREFS_EXPORT_FORMAT_ASK );
			g_object_unref( format );
			break;

		default:
			g_warning( "%s: mode=%d: unknown mode", thisfn, mode );
	}

	g_object_set_data( G_OBJECT( container_vbox ), EXPORT_FORMAT_PROP_CONTAINER_FORMAT, GUINT_TO_POINTER( 0 ));
	g_object_set_data( G_OBJECT( container_vbox ), EXPORT_FORMAT_PROP_CONTAINER_SENSITIVE, GUINT_TO_POINTER( sensitive ));
}

/*
 * container used to be a glade-defined GtkVBox in which we dynamically
 * add for each mode:
 *  +- vbox
 *  |   +- radio button
 *  |   +- hbox
 *  |   |   +- description (assistant mode only)
 *
 *  Starting with Gtk 3.2, container is a GtkGrid attached to the
 *  glade-defined GtkVBox. For each mode, we are defining:
 *  +- grid
 *  |   +- radio button
 *  |   +- description (assistant mode only)
 *
 *  id=-1 but for the 'Ask me' mode
 */
static void
draw_in_vbox( GtkWidget *container, const NAExportFormat *format, guint mode, gint id )
{
	static const gchar *thisfn = "nact_export_format_draw_in_vbox";
	static GtkRadioButton *first_button = NULL;
	GtkWidget *container_mode;
	gchar *description;
	GtkRadioButton *button;
	guint size, spacing;
	gint  ypad;
	gfloat yalign;
#if ! GTK_CHECK_VERSION( 3, 2, 0 )
	GtkWidget *hbox;
#endif
	gchar *markup, *label;
	GtkLabel *desc_label;
	VBoxData *vbox_data;

#if GTK_CHECK_VERSION( 3, 2, 0 )
	/* create a grid container which will embed two lines */
	container_mode = gtk_grid_new();
#else
	/* create a vbox which will embed two children */
	container_mode = gtk_vbox_new( FALSE, 0 );
#endif
	gtk_box_pack_start( GTK_BOX( container ), container_mode, FALSE, TRUE, 0 );
	/*g_object_set( G_OBJECT( container_mode ), "spacing", 6, NULL );*/
	description = na_export_format_get_description( format );
	g_object_set( G_OBJECT( container_mode ), "tooltip-text", description, NULL );

	/* first line/children is the radio button
	 */
	button = GTK_RADIO_BUTTON( gtk_radio_button_new( NULL ));
	if( first_button ){
		g_object_set( G_OBJECT( button ), "group", first_button, NULL );
	} else {
		first_button = button;
	}
#if GTK_CHECK_VERSION( 3, 2, 0 )
	gtk_grid_attach( GTK_GRID( container_mode ), GTK_WIDGET( button ), 0, 0, 1, 1 );
#else
	gtk_box_pack_start( GTK_BOX( container_mode ), GTK_WIDGET( button ), FALSE, TRUE, 0 );
#endif

	label = NULL;
	markup = NULL;
	switch( mode ){

		case EXPORT_FORMAT_DISPLAY_ASK:
		case EXPORT_FORMAT_DISPLAY_PREFERENCES:
		case EXPORT_FORMAT_DISPLAY_ASSISTANT:
			label = na_export_format_get_label( format );
			markup = g_markup_printf_escaped( "%s", label );
			gtk_button_set_label( GTK_BUTTON( button ), label );
			gtk_button_set_use_underline( GTK_BUTTON( button ), TRUE );
			break;

		/* this work fine, but it appears that this is not consistant with import assistant */
		/*case EXPORT_FORMAT_DISPLAY_ASSISTANT:
			radio_label = GTK_LABEL( gtk_label_new( NULL ));
			label = na_export_format_get_label( format );
			markup = g_markup_printf_escaped( "<b>%s</b>", label );
			gtk_label_set_markup( radio_label, markup );
			gtk_container_add( GTK_CONTAINER( button ), GTK_WIDGET( radio_label ));
			break;*/
	}

	desc_label = NULL;
	switch( mode ){

		case EXPORT_FORMAT_DISPLAY_ASSISTANT:
			gtk_widget_style_get( GTK_WIDGET( button ), "indicator-size", &size, NULL );
			gtk_widget_style_get( GTK_WIDGET( button ), "indicator-spacing", &spacing, NULL );
			size += 3*spacing;

			desc_label = GTK_LABEL( gtk_label_new( description ));
			gtk_misc_get_padding( GTK_MISC( desc_label ), NULL, &ypad );
			gtk_misc_set_padding( GTK_MISC( desc_label ), size, ypad );
			gtk_misc_get_alignment( GTK_MISC( desc_label ), NULL, &yalign );
			gtk_misc_set_alignment( GTK_MISC( desc_label ), 0, yalign );
			gtk_label_set_line_wrap( desc_label, TRUE );
			gtk_label_set_line_wrap_mode( desc_label, PANGO_WRAP_WORD );

#if GTK_CHECK_VERSION( 3, 2, 0 )
			gtk_grid_attach( GTK_GRID( container_mode ), GTK_WIDGET( desc_label ), 0, 1, 1, 1 );
#else
			hbox = gtk_hbox_new( TRUE, 0 );
			gtk_box_pack_start( GTK_BOX( container_mode ), hbox, FALSE, TRUE, 0 );
			gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( desc_label ), TRUE, TRUE, 4 );
#endif
			break;
	}

	vbox_data = g_new0( VBoxData, 1 );
	g_debug( "%s: container_mode=%p, allocating VBoxData at %p",
			thisfn, ( void * ) container_mode, ( void * ) vbox_data );

	vbox_data->container_vbox = container;
	vbox_data->button = button;
	vbox_data->format = g_object_ref(( gpointer ) format );

	g_object_set_data( G_OBJECT( container_mode ), EXPORT_FORMAT_PROP_VBOX_DATA, vbox_data );
	g_object_weak_ref( G_OBJECT( container_mode ), ( GWeakNotify ) format_weak_notify, ( gpointer ) vbox_data );

	g_free( markup );
	g_free( label );
	g_free( description );
}

static void
format_weak_notify( VBoxData *vbox_data, GObject *vbox )
{
	static const gchar *thisfn = "nact_export_format_weak_notify";

	g_debug( "%s: vbox_data=%p, vbox=%p", thisfn, ( void * ) vbox_data, ( void * ) vbox );

	g_signal_handler_disconnect( vbox_data->button, vbox_data->handler );

	g_object_unref( vbox_data->format );

	g_free( vbox_data );
}

/**
 * nact_export_format_select:
 * @container_vbox: the #GtkVBox in which the display has been drawn.
 * @editable: whether the whole radio button group is activatable.
 * @format: the #GQuark which must be used as a default value.
 *
 * Select the default value.
 *
 * This is to be ran from runtime initialization dialog.
 *
 * Data for each format has been set on the new embedding vbox, previously
 * created in nact_export_format_init_display(). We are iterating on these
 * vbox to setup the initially active radio button.
 *
 * Starting with Gtk 3.2.0, the 'container_vbox' no more contains GtkVBoxes,
 * but a grid (one column, n rows) whose each row contains itself one grid
 * for each mode.
 */
void
nact_export_format_select( const GtkWidget *container_vbox, gboolean editable, GQuark format )
{
	g_object_set_data( G_OBJECT( container_vbox ), EXPORT_FORMAT_PROP_CONTAINER_EDITABLE, GUINT_TO_POINTER( editable ));
	g_object_set_data( G_OBJECT( container_vbox ), EXPORT_FORMAT_PROP_CONTAINER_FORMAT, GUINT_TO_POINTER( format ));

	gtk_container_foreach( GTK_CONTAINER( container_vbox ), ( GtkCallback ) select_default_iter, GUINT_TO_POINTER( format ));
}

/*
 * container_mode is a GtkVBox, or a GtkGrid starting with Gtk 3.2
 */
static void
select_default_iter( GtkWidget *container_mode, void *quark_ptr )
{
	static const gchar *thisfn = "nact_export_format_select_default_iter";
	VBoxData *vbox_data;
	GQuark format_quark;
	GtkRadioButton *button;
	gboolean editable, sensitive;

	vbox_data = ( VBoxData * )
			g_object_get_data( G_OBJECT( container_mode ), EXPORT_FORMAT_PROP_VBOX_DATA );
	g_debug( "%s: container_mode=%p, retrieving VBoxData at %p",
			thisfn, ( void * ) container_mode, ( void * ) vbox_data );

	editable = ( gboolean ) GPOINTER_TO_UINT(
			g_object_get_data( G_OBJECT( vbox_data->container_vbox ), EXPORT_FORMAT_PROP_CONTAINER_EDITABLE ));

	sensitive = ( gboolean ) GPOINTER_TO_UINT(
			g_object_get_data( G_OBJECT( vbox_data->container_vbox ), EXPORT_FORMAT_PROP_CONTAINER_SENSITIVE ));

	vbox_data->handler = g_signal_connect( G_OBJECT( vbox_data->button ), "toggled", G_CALLBACK( export_format_on_toggled ), vbox_data );

	button = NULL;
	format_quark = ( GQuark ) GPOINTER_TO_UINT( quark_ptr );

	if( na_export_format_get_quark( vbox_data->format ) == format_quark ){
		button = vbox_data->button;
	}

	if( button ){
		base_gtk_utils_radio_set_initial_state( button,
				G_CALLBACK( export_format_on_toggled ), vbox_data, editable, sensitive );
	}
}

static void
export_format_on_toggled( GtkToggleButton *toggle_button, VBoxData *vbox_data )
{
	gboolean editable, active;
	GQuark format;

	editable = ( gboolean ) GPOINTER_TO_UINT(
			g_object_get_data( G_OBJECT( vbox_data->container_vbox ), EXPORT_FORMAT_PROP_CONTAINER_EDITABLE ));

	if( editable ){
		active = gtk_toggle_button_get_active( toggle_button );
		if( active ){
			format = na_export_format_get_quark( vbox_data->format );
			g_object_set_data( G_OBJECT( vbox_data->container_vbox ), EXPORT_FORMAT_PROP_CONTAINER_FORMAT, GUINT_TO_POINTER( format ));
		}
	} else {
		base_gtk_utils_radio_reset_initial_state( GTK_RADIO_BUTTON( toggle_button ), NULL );
	}
}

/**
 * nact_export_format_get_selected:
 * @container_vbox: the #GtkVBox in which the display has been drawn.
 *
 * Returns: the currently selected value, as a #NAExportFormat object.
 *
 * The returned #NAExportFormat is owned by #NactExportFormat, and
 * should not be released by the caller.
 */
NAExportFormat *
nact_export_format_get_selected( const GtkWidget *container_vbox )
{
	NAExportFormat *format;

	format = NULL;
	gtk_container_foreach( GTK_CONTAINER( container_vbox ), ( GtkCallback ) get_selected_iter, &format );
	g_debug( "nact_export_format_get_selected: format=%p", ( void * ) format );

	return( format );
}

/*
 * container_mode is a GtkVBox, or a GtkGrid starting with Gtk 3.2
 */
static void
get_selected_iter( GtkWidget *container_mode, NAExportFormat **format )
{
	VBoxData *vbox_data;

	if( !( *format  )){
		vbox_data = ( VBoxData * ) g_object_get_data( G_OBJECT( container_mode ), EXPORT_FORMAT_PROP_VBOX_DATA );
		if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( vbox_data->button ))){
			*format = vbox_data->format;
		}
	}
}
