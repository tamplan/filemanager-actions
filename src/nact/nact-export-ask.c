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

#include <api/na-object-api.h>

#include <core/na-exporter.h>
#include <core/na-iprefs.h>

#include "nact-application.h"
#include "nact-export-format.h"
#include "nact-export-ask.h"
#include "base-gtk-utils.h"

/* private class data
 */
struct _NactExportAskClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _NactExportAskPrivate {
	gboolean      dispose_has_run;
	BaseWindow   *parent;
	gboolean      preferences_locked;
	NAObjectItem *item;
	GQuark        format;
	gboolean      format_mandatory;
	gboolean      keep_last_choice;
	gboolean      keep_last_choice_mandatory;
};

static const gchar     *st_xmlui_filename = PKGDATADIR "/nact-assistant-export.ui";
static const gchar     *st_toplevel_name  = "ExportAskDialog";
static const gchar     *st_wsp_name       = NA_IPREFS_EXPORT_ASK_USER_WSP;

static BaseDialogClass *st_parent_class   = NULL;

static GType    register_type( void );
static void     class_init( NactExportAskClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_dispose( GObject *dialog );
static void     instance_finalize( GObject *dialog );

static void     on_base_initialize_gtk_toplevel( NactExportAsk *editor, GtkDialog *toplevel );
static void     on_base_initialize_base_window( NactExportAsk *editor );
static void     keep_choice_on_toggled( GtkToggleButton *button, NactExportAsk *editor );
static void     on_cancel_clicked( GtkButton *button, NactExportAsk *editor );
static void     on_ok_clicked( GtkButton *button, NactExportAsk *editor );
static GQuark   get_export_format( NactExportAsk *editor );

GType
nact_export_ask_get_type( void )
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
	static const gchar *thisfn = "nact_export_ask_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NactExportAskClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactExportAsk ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( BASE_DIALOG_TYPE, "NactExportAsk", &info, 0 );

	return( type );
}

static void
class_init( NactExportAskClass *klass )
{
	static const gchar *thisfn = "nact_export_ask_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactExportAskClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_export_ask_instance_init";
	NactExportAsk *self;

	g_return_if_fail( NACT_IS_EXPORT_ASK( instance ));

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self = NACT_EXPORT_ASK( instance );

	self->private = g_new0( NactExportAskPrivate, 1 );

	base_window_signal_connect( BASE_WINDOW( instance ),
			G_OBJECT( instance ), BASE_SIGNAL_INITIALIZE_GTK, G_CALLBACK( on_base_initialize_gtk_toplevel ));

	base_window_signal_connect( BASE_WINDOW( instance ),
			G_OBJECT( instance ), BASE_SIGNAL_INITIALIZE_WINDOW, G_CALLBACK( on_base_initialize_base_window ));

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *dialog )
{
	static const gchar *thisfn = "nact_export_ask_instance_dispose";
	NactExportAsk *self;

	g_return_if_fail( NACT_IS_EXPORT_ASK( dialog ));

	self = NACT_EXPORT_ASK( dialog );

	if( !self->private->dispose_has_run ){
		g_debug( "%s: dialog=%p (%s)", thisfn, ( void * ) dialog, G_OBJECT_TYPE_NAME( dialog ));

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( dialog );
		}
	}
}

static void
instance_finalize( GObject *dialog )
{
	static const gchar *thisfn = "nact_export_ask_instance_finalize";
	NactExportAsk *self;

	g_return_if_fail( NACT_IS_EXPORT_ASK( dialog ));

	g_debug( "%s: dialog=%p (%s)", thisfn, ( void * ) dialog, G_OBJECT_TYPE_NAME( dialog ));

	self = NACT_EXPORT_ASK( dialog );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( dialog );
	}
}

/**
 * nact_export_ask_user:
 * @parent: the NactAssistant parent of this dialog.
 * @item: the NAObjectItem to be exported.
 * @first: whether this is the first call of a serie.
 *  On a first call, the user is really asked for his choice.
 *  The next times, the 'keep-last-choice' flag will be considered.
 *
 * Initializes and runs the dialog.
 *
 * This is a small dialog which is to be ran during export operations,
 * when the set export format is 'Ask me'. Each exported file runs this
 * dialog, unless the user selects the 'keep same choice' box.
 *
 * Returns: the mode chosen by the user as a #GQuark which identifies
 * the export mode.
 * The function defaults to returning NA_IPREFS_DEFAULT_EXPORT_FORMAT.
 *
 * When the user selects 'Keep same choice without asking me', this choice
 * becomes his new preferred export format.
 */
GQuark
nact_export_ask_user( BaseWindow *parent, NAObjectItem *item, gboolean first )
{
	static const gchar *thisfn = "nact_export_ask_user";
	NactExportAsk *editor;
	gboolean are_locked, mandatory;
	gboolean keep, keep_mandatory;

	GQuark format = g_quark_from_static_string( NA_IPREFS_DEFAULT_EXPORT_FORMAT );

	g_return_val_if_fail( BASE_IS_WINDOW( parent ), format );

	g_debug( "%s: parent=%p, item=%p (%s), first=%s",
			thisfn, ( void * ) parent, ( void * ) item, G_OBJECT_TYPE_NAME( item ), first ? "True":"False" );

	format = na_iprefs_get_export_format( NA_IPREFS_EXPORT_ASK_USER_LAST_FORMAT, &mandatory );
	keep = na_settings_get_boolean( NA_IPREFS_EXPORT_ASK_USER_KEEP_LAST_CHOICE, NULL, &keep_mandatory );

	if( first || !keep ){

		editor = g_object_new( NACT_EXPORT_ASK_TYPE,
				BASE_PROP_PARENT,         parent,
				BASE_PROP_XMLUI_FILENAME, st_xmlui_filename,
				BASE_PROP_TOPLEVEL_NAME,  st_toplevel_name,
				BASE_PROP_WSP_NAME,       st_wsp_name,
				NULL );

		editor->private->format = format;
		editor->private->format_mandatory = mandatory;
		editor->private->keep_last_choice = keep;
		editor->private->keep_last_choice_mandatory = keep_mandatory;

		editor->private->parent = parent;
		editor->private->item = item;

		are_locked = na_settings_get_boolean( NA_IPREFS_ADMIN_PREFERENCES_LOCKED, NULL, &mandatory );
		editor->private->preferences_locked = are_locked && mandatory;

		if( base_window_run( BASE_WINDOW( editor )) == GTK_RESPONSE_OK ){
			format = get_export_format( editor );
		}

		g_object_unref( editor );
	}

	return( format );
}

static void
on_base_initialize_gtk_toplevel( NactExportAsk *editor, GtkDialog *toplevel )
{
	static const gchar *thisfn = "nact_export_ask_on_base_initialize_gtk_toplevel";
	NactApplication *application;
	NAUpdater *updater;
	GtkWidget *container;

	g_return_if_fail( NACT_IS_EXPORT_ASK( editor ));

	if( !editor->private->dispose_has_run ){
		g_debug( "%s: editor=%p, toplevel=%p", thisfn, ( void * ) editor, ( void * ) toplevel );

		application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( editor )));
		updater = nact_application_get_updater( application );
		container = base_window_get_widget( BASE_WINDOW( editor ), "ExportFormatAskVBox" );

		nact_export_format_init_display( container,
				NA_PIVOT( updater ), EXPORT_FORMAT_DISPLAY_ASK, !editor->private->preferences_locked );
	}
}

static void
on_base_initialize_base_window( NactExportAsk *editor )
{
	static const gchar *thisfn = "nact_export_ask_on_base_initialize_base_window";
	GtkWidget *container;
	gchar *item_label, *label;
	GtkWidget *widget;

	g_return_if_fail( NACT_IS_EXPORT_ASK( editor ));

	if( !editor->private->dispose_has_run ){
		g_debug( "%s: editor=%p", thisfn, ( void * ) editor );

		item_label = na_object_get_label( editor->private->item );

		if( NA_IS_OBJECT_ACTION( editor->private->item )){
			/* i18n: The action <label> is about to be exported */
			label = g_strdup_printf( _( "The action \"%s\" is about to be exported." ), item_label );
		} else {
			/* i18n: The menu <label> is about to be exported */
			label = g_strdup_printf( _( "The menu \"%s\" is about to be exported." ), item_label );
		}

		widget = base_window_get_widget( BASE_WINDOW( editor ), "ExportAskLabel1" );
		gtk_label_set_text( GTK_LABEL( widget ), label );
		g_free( label );
		g_free( item_label );

		container = base_window_get_widget( BASE_WINDOW( editor ), "ExportFormatAskVBox" );
		nact_export_format_select( container, !editor->private->format_mandatory, editor->private->format );

		base_gtk_utils_toggle_set_initial_state( BASE_WINDOW( editor ),
				"AskKeepChoiceButton", G_CALLBACK( keep_choice_on_toggled ),
				editor->private->keep_last_choice,
				!editor->private->keep_last_choice_mandatory, !editor->private->preferences_locked );

		base_window_signal_connect_by_name( BASE_WINDOW( editor ),
				"CancelButton", "clicked", G_CALLBACK( on_cancel_clicked ));

		base_window_signal_connect_by_name( BASE_WINDOW( editor ),
				"OKButton", "clicked", G_CALLBACK( on_ok_clicked ));
	}
}

static void
keep_choice_on_toggled( GtkToggleButton *button, NactExportAsk *editor )
{
	gboolean editable;

	editable = ( gboolean ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( button ), NACT_PROP_TOGGLE_EDITABLE ));

	if( editable ){
		editor->private->keep_last_choice = gtk_toggle_button_get_active( button );

	} else {
		base_gtk_utils_toggle_reset_initial_state( button );
	}
}

static void
on_cancel_clicked( GtkButton *button, NactExportAsk *editor )
{
	GtkWindow *toplevel = base_window_get_gtk_toplevel( BASE_WINDOW( editor ));
	gtk_dialog_response( GTK_DIALOG( toplevel ), GTK_RESPONSE_CLOSE );
}

static void
on_ok_clicked( GtkButton *button, NactExportAsk *editor )
{
	GtkWindow *toplevel = base_window_get_gtk_toplevel( BASE_WINDOW( editor ));
	gtk_dialog_response( GTK_DIALOG( toplevel ), GTK_RESPONSE_OK );
}

/*
 * we have come here because preferred_export_format was 'Ask'
 * in all cases, the chosen format is kept in 'ask_user_last_chosen_format'
 * and so this will be the default format which will be proposed the next
 * time we come here
 * if the 'remember_my_choice' is cheecked, then we do not even ask the user
 * but returns as soon as we enter
 *
 * not overrinding the preferred export format (i.e. letting it to 'Ask')
 * let the user go back in ask dialog box the next time he will have some
 * files to export
 */
static GQuark
get_export_format( NactExportAsk *editor )
{
	GtkWidget *container;
	NAExportFormat *format;
	GQuark format_quark;

	container = base_window_get_widget( BASE_WINDOW( editor ), "ExportFormatAskVBox" );
	format = nact_export_format_get_selected( container );
	format_quark = na_export_format_get_quark( format );

	if( !editor->private->keep_last_choice_mandatory ){
		na_settings_set_boolean( NA_IPREFS_EXPORT_ASK_USER_KEEP_LAST_CHOICE, editor->private->keep_last_choice );
	}

	na_iprefs_set_export_format( NA_IPREFS_EXPORT_ASK_USER_LAST_FORMAT, format_quark );

	return( format_quark );
}
