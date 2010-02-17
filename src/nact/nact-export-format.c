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

#include <core/na-exporter.h>
#include <core/na-export-format.h>

#include "nact-export-format.h"

#define EXPORT_FORMAT_PROP_QUARK_ID		"nact-export-format-prop-quark-id"
#define EXPORT_FORMAT_PROP_BUTTON		"nact-export-format-prop-button"
#define EXPORT_FORMAT_PROP_LABEL		"nact-export-format-prop-label"
#define EXPORT_FORMAT_PROP_DESCRIPTION	"nact-export-format-prop-description"

#define ASKME_LABEL						N_( "Ask me" )
#define ASKME_DESCRIPTION				N_( "You will be asked each time an item is about to be exported." )

/* structure used when iterating on container's children
 */
typedef struct {
	GQuark   format;
	gboolean found;
	gchar   *label;
	gchar   *prop;
}
	NactExportFormatStr;

static void draw_in_vbox( const NAExportFormat *format, GtkWidget *vbox, guint mode );
static void select_default_iter( GtkWidget *widget, NactExportFormatStr *str );
static void get_selected_iter( GtkWidget *widget, NactExportFormatStr *str );
static void get_label_iter( GtkWidget *widget, NactExportFormatStr *str );

/**
 * nact_export_format_display:
 * @pivot: the #NAPivot instance.
 * @vbox: the #GtkVBox in which the display must be drawn.
 * @mode: the type of the display.
 *
 * Displays the available export formats in the VBox
 */
void
nact_export_format_display( const NAPivot *pivot, GtkWidget *vbox, guint mode )
{
	GList *formats, *ifmt;

	formats = na_exporter_get_formats( pivot );

	for( ifmt = formats ; ifmt ; ifmt = ifmt->next ){
		draw_in_vbox( NA_EXPORT_FORMAT( ifmt->data ), vbox, mode );
	}

	na_exporter_free_formats( formats );
}

/*
 * container
 *  +- vbox
 *  |   +- hbox
 *  |   |   +- radio button
 *  |   |   +- label
 *  |   +- hbox
 *  |   |   +- description
 */
static void
draw_in_vbox( const NAExportFormat *format, GtkWidget *container, guint mode )
{
	static const gchar *thisfn = "nact_export_format_draw_in_vbox";
	static GtkRadioButton *first_button = NULL;
	GtkVBox *vbox;
	gchar *description;
	GtkHBox *hbox1, *hbox2;
	GtkRadioButton *button;
	guint size, spacing;
	GtkLabel *radio_label;
	gchar *markup, *label;
	GtkLabel *desc_label;

	vbox = GTK_VBOX( gtk_vbox_new( FALSE, 0 ));
	gtk_box_pack_start( GTK_BOX( container ), GTK_WIDGET( vbox ), FALSE, TRUE, 0 );
	description = na_export_format_get_description( format );
	g_object_set( G_OBJECT( vbox ), "tooltip-text", description, NULL );

	hbox1 = GTK_HBOX( gtk_hbox_new( FALSE, 0 ));
	gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( hbox1 ), FALSE, TRUE, 0 );

	button = GTK_RADIO_BUTTON( gtk_radio_button_new( NULL ));
	if( first_button ){
		g_object_set( G_OBJECT( button ), "group", first_button, NULL );
	} else {
		first_button = button;
	}
	gtk_box_pack_start( GTK_BOX( hbox1 ), GTK_WIDGET( button ), FALSE, TRUE, 0 );

	radio_label = GTK_LABEL( gtk_label_new( NULL ));
	label = NULL;
	markup = NULL;
	switch( mode ){

		case EXPORT_FORMAT_DISPLAY_ASK:
			label = na_export_format_get_ask_label( format );
			markup = g_markup_printf_escaped( "%s", label );
			break;

		case EXPORT_FORMAT_DISPLAY_ASSISTANT:
			label = na_export_format_get_label( format );
			markup = g_markup_printf_escaped( "<b>%s</b>", label );
			break;

		default:
			g_warning( "%s: mode=%d: unknown mode", thisfn, mode );
	}
	if( markup ){
		gtk_label_set_markup( radio_label, markup );
		g_free( markup );
	}
	gtk_box_pack_start( GTK_BOX( hbox1 ), GTK_WIDGET( radio_label ), FALSE, TRUE, 0 );

	desc_label = NULL;
	switch( mode ){

		case EXPORT_FORMAT_DISPLAY_ASSISTANT:
			gtk_widget_style_get( GTK_WIDGET( button ), "indicator-size", &size, NULL );
			gtk_widget_style_get( GTK_WIDGET( button ), "indicator-spacing", &spacing, NULL );
			size += 2*spacing;
			hbox2 = GTK_HBOX( gtk_hbox_new( TRUE, 0 ));
			gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( hbox2 ), FALSE, TRUE, 0 );
			desc_label = GTK_LABEL( gtk_label_new( description ));
			g_object_set( G_OBJECT( desc_label ), "xpad", size, NULL );
			gtk_box_pack_start( GTK_BOX( hbox2 ), GTK_WIDGET( desc_label ), TRUE, TRUE, 4 );
			break;
	}

	g_object_set_data( G_OBJECT( vbox ), EXPORT_FORMAT_PROP_QUARK_ID, GUINT_TO_POINTER( na_export_format_get_id( format )));
	g_object_set_data( G_OBJECT( vbox ), EXPORT_FORMAT_PROP_BUTTON, button );
	g_object_set_data( G_OBJECT( vbox ), EXPORT_FORMAT_PROP_LABEL, radio_label );
	g_object_set_data( G_OBJECT( vbox ), EXPORT_FORMAT_PROP_DESCRIPTION, desc_label );

	g_free( label );
	g_free( description );
}

/**
 * nact_export_format_select:
 * @container: the #GtkVBox in which the display has been drawn.
 * @format: the #GQuark which must be used as a default value.
 *
 * Select the default value.
 */
void
nact_export_format_select( const GtkWidget *container, GQuark format )
{
	NactExportFormatStr *str;

	str = g_new0( NactExportFormatStr, 1 );
	str->format = format;
	str->found = FALSE;

	gtk_container_foreach( GTK_CONTAINER( container ), ( GtkCallback ) select_default_iter, str );

	g_free( str );
}

static void
select_default_iter( GtkWidget *widget, NactExportFormatStr *str )
{
	GQuark format;
	GtkRadioButton *button;

	if( !str->found ){
		format = ( GQuark ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( widget ), EXPORT_FORMAT_PROP_QUARK_ID ));
		if( format == str->format ){
			str->found = TRUE;
			button = GTK_RADIO_BUTTON( g_object_get_data( G_OBJECT( widget ), EXPORT_FORMAT_PROP_BUTTON ));
			gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( button ), TRUE );
		}
	}
}

/**
 * nact_export_format_get_select:
 * @container: the #GtkVBox in which the display has been drawn.
 *
 * Returns: the currently selected value, as a #GQuark.
 */
GQuark
nact_export_format_get_select( const GtkWidget *container )
{
	GQuark format;
	NactExportFormatStr *str;

	format = 0;

	str = g_new0( NactExportFormatStr, 1 );
	str->format = 0;
	str->found = FALSE;

	gtk_container_foreach( GTK_CONTAINER( container ), ( GtkCallback ) get_selected_iter, str );

	if( str->found ){
		format = str->format;
	}

	g_free( str );

	return( format );
}

static void
get_selected_iter( GtkWidget *widget, NactExportFormatStr *str )
{
	GtkRadioButton *button;

	if( !str->found ){
		button = GTK_RADIO_BUTTON( g_object_get_data( G_OBJECT( widget ), EXPORT_FORMAT_PROP_BUTTON ));
		if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button ))){
			str->found = TRUE;
			str->format = ( GQuark ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( widget ), EXPORT_FORMAT_PROP_QUARK_ID ));
		}
	}
}

/**
 * nact_export_format_get_label:
 * @format: the #GQuark selected format.
 *
 * Returns: the label of the @format, as a newly allocated string which
 * should be g_free() by the caller.
 */
gchar *
nact_export_format_get_label( const GtkWidget *container, GQuark format )
{
	gchar *label;
	NactExportFormatStr *str;

	label = NULL;

	str = g_new0( NactExportFormatStr, 1 );
	str->format = format;
	str->found = FALSE;
	str->prop = EXPORT_FORMAT_PROP_LABEL;

	gtk_container_foreach( GTK_CONTAINER( container ), ( GtkCallback ) get_label_iter, str );

	if( str->found ){
		label = str->label;
	}

	g_free( str );

	return( label );
}

/**
 * nact_export_format_get_description:
 * @format: the #GQuark selected format.
 *
 * Returns: the description of the @format, as a newly allocated string which
 * should be g_free() by the caller.
 */
gchar *
nact_export_format_get_description( const GtkWidget *container, GQuark format )
{
	gchar *label;
	NactExportFormatStr *str;

	label = NULL;

	str = g_new0( NactExportFormatStr, 1 );
	str->format = format;
	str->found = FALSE;
	str->prop = EXPORT_FORMAT_PROP_DESCRIPTION;

	gtk_container_foreach( GTK_CONTAINER( container ), ( GtkCallback ) get_label_iter, str );

	if( str->found ){
		label = str->label;
	}

	g_free( str );

	return( label );
}

static void
get_label_iter( GtkWidget *widget, NactExportFormatStr *str )
{
	GQuark format;
	GtkLabel *gtk_label;

	if( !str->found ){
		format = ( GQuark ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( widget ), EXPORT_FORMAT_PROP_QUARK_ID ));
		if( format == str->format ){
			str->found = TRUE;
			gtk_label = GTK_LABEL( g_object_get_data( G_OBJECT( widget ), str->prop ));
			str->label = g_strdup( gtk_label_get_text( gtk_label ));
		}
	}
}
