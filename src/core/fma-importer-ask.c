/*
 * FileManager-Actions
 * A file-manager extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
 *
 * FileManager-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * FileManager-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FileManager-Actions; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
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

#include <api/fma-iimporter.h>
#include <api/fma-object-api.h>

#include "fma-gtk-utils.h"
#include "fma-import-mode.h"
#include "fma-importer.h"
#include "fma-importer-ask.h"
#include "fma-ioptions-list.h"
#include "fma-settings.h"

/* private class data
 */
struct _FMAImporterAskClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _FMAImporterAskPrivate {
	gboolean                 dispose_has_run;
	GtkWindow               *toplevel;
	FMAObjectItem           *importing;
	FMAObjectItem           *existing;
	FMAImporterAskUserParms *parms;
	guint                    mode;
};

static GObjectClass   *st_parent_class = NULL;
static FMAImporterAsk *st_dialog       = NULL;
static const gchar    *st_uixml        = PKGIMPORTERASKDIR "/fma-importer-ask.ui";

static GType          register_type( void );
static void           class_init( FMAImporterAskClass *klass );
static void           ioptions_list_iface_init( FMAIOptionsListInterface *iface, void *user_data );
static GList         *ioptions_list_get_modes( const FMAIOptionsList *instance, GtkWidget *container );
static void           ioptions_list_free_modes( const FMAIOptionsList *instance, GtkWidget *container, GList *modes );
static void           instance_init( GTypeInstance *instance, gpointer klass );
static void           instance_dispose( GObject *dialog );
static void           instance_finalize( GObject *dialog );
static FMAImporterAsk *import_ask_new( GtkWindow *parent );
static void           initialize_gtk( FMAImporterAsk *dialog, GtkWindow *toplevel );
static void           initialize_window( FMAImporterAsk *dialog, GtkWindow *toplevel );
static void           get_selected_mode( FMAImporterAsk *editor );
static void           on_destroy_toplevel( GtkWindow *toplevel, FMAImporterAsk *dialog );
static gboolean       on_dialog_response( FMAImporterAsk *editor, gint code );

GType
fma_importer_ask_get_type( void )
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
	static const gchar *thisfn = "fma_importer_ask_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( FMAImporterAskClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( FMAImporterAsk ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	static const GInterfaceInfo ioptions_list_iface_info = {
		( GInterfaceInitFunc ) ioptions_list_iface_init,
		NULL,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "FMAImporterAsk", &info, 0 );

	g_type_add_interface_static( type, FMA_TYPE_IOPTIONS_LIST, &ioptions_list_iface_info );

	return( type );
}

static void
class_init( FMAImporterAskClass *klass )
{
	static const gchar *thisfn = "fma_importer_ask_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( FMAImporterAskClassPrivate, 1 );
}

static void
ioptions_list_iface_init( FMAIOptionsListInterface *iface, void *user_data )
{
	static const gchar *thisfn = "fma_importer_ask_ioptions_list_iface_init";

	g_debug( "%s: iface=%p, user_data=%p", thisfn, ( void * ) iface, ( void * ) user_data );

	iface->get_options = ioptions_list_get_modes;
	iface->free_options = ioptions_list_free_modes;
}

static GList *
ioptions_list_get_modes( const FMAIOptionsList *instance, GtkWidget *container )
{
	GList *modes;

	g_return_val_if_fail( FMA_IS_IMPORTER_ASK( instance ), NULL );

	modes = fma_importer_get_modes();

	return( modes );
}

static void
ioptions_list_free_modes( const FMAIOptionsList *instance, GtkWidget *container, GList *modes )
{
	g_return_if_fail( FMA_IS_IMPORTER_ASK( instance ));

	fma_importer_free_modes( modes );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "fma_importer_ask_instance_init";
	FMAImporterAsk *self;

	g_return_if_fail( FMA_IS_IMPORTER_ASK( instance ));

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self = FMA_IMPORTER_ASK( instance );

	self->private = g_new0( FMAImporterAskPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *dialog )
{
	static const gchar *thisfn = "fma_importer_ask_instance_dispose";
	FMAImporterAsk *self;

	g_return_if_fail( FMA_IS_IMPORTER_ASK( dialog ));

	self = FMA_IMPORTER_ASK( dialog );

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
	static const gchar *thisfn = "fma_importer_ask_instance_finalize";
	FMAImporterAsk *self;

	g_return_if_fail( FMA_IS_IMPORTER_ASK( dialog ));

	g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );

	self = FMA_IMPORTER_ASK( dialog );

	if( self->private->toplevel ){
		gtk_widget_destroy( GTK_WIDGET( self->private->toplevel ));
	}

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( dialog );
	}
}

/*
 * Returns a newly allocated FMAImporterAsk object.
 */
static FMAImporterAsk *
import_ask_new( GtkWindow *parent )
{
	FMAImporterAsk *dialog;
	GtkBuilder *builder;
	GError *error;
	GtkWindow *toplevel;

	if( st_dialog ){
		dialog = st_dialog;

	} else {
		dialog = g_object_new( FMA_TYPE_IMPORTER_ASK, NULL );

		builder = gtk_builder_new();
		error = NULL;
		gtk_builder_add_from_file( builder, st_uixml, &error );
		if( error ){
			gtk_message_dialog_new( parent, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", error->message );
			g_error_free( error );
			g_object_unref( dialog );
			dialog = NULL;

		} else {
			toplevel = GTK_WINDOW( gtk_builder_get_object( builder, "ImporterAskDialog" ));
			if( !toplevel ){
				/* l10n: 'ImporterAskDialog' is the dialog name: do not translate */
				gtk_message_dialog_new( parent, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _( "Unable to load 'ImporterAskDialog' from %s" ), st_uixml );
				g_object_unref( dialog );
				dialog = NULL;

			} else {
				dialog->private->toplevel = toplevel;

				if( parent ){
					gtk_window_set_transient_for( dialog->private->toplevel, parent );
					gtk_window_set_destroy_with_parent( dialog->private->toplevel, TRUE );
					g_signal_connect(
							G_OBJECT( dialog->private->toplevel ),
							"destroy",
							G_CALLBACK( on_destroy_toplevel ),
							dialog );
					st_dialog = dialog;
				}

				initialize_gtk( dialog, toplevel );
			}
		}
		g_object_unref( builder );
	}

	return( dialog );
}

/*
 * fma_importer_ask_user:
 * @importing: the #FMAObjectItem-derived object being currently imported.
 * @existing: the #FMAObjectItem-derived already existing object with the same ID.
 * @parms: a #FMAIImporterUriParms structure.
 *
 * Ask the user for what to do when an imported item has the same ID
 * that an already existing one.
 *
 * If a parent is specified, then we allocate a new FMAImporterAsk from
 * GtkBuilder, hiding and showing it for each invocation of the dialog
 * by the parent, as long as the parent exists.
 * When the parent is destroyed, this (maybe hidden) FMAImporterAsk dialog
 * is also destroyed.
 *
 * If there is no specified parent, then we recreate a new FMAImporterAsk
 * dialog at each invocation, destroying it after on_dialog_response()
 * returns.
 *
 * Returns: the definitive import mode.
 *
 * When the user selects 'Keep same choice without asking me', this choice
 * becomes his preference import mode.
 */
guint
fma_importer_ask_user( const FMAObjectItem *importing, const FMAObjectItem *existing, FMAImporterAskUserParms *parms )
{
	static const gchar *thisfn = "fma_importer_ask_user";
	FMAImporterAsk *dialog;
	guint mode;
	gint code;

	g_return_val_if_fail( FMA_IS_OBJECT_ITEM( importing ), IMPORTER_MODE_NO_IMPORT );
	g_return_val_if_fail( FMA_IS_OBJECT_ITEM( existing ), IMPORTER_MODE_NO_IMPORT );

	g_debug( "%s: importing=%p, existing=%p, parms=%p",
			thisfn, ( void * ) importing, ( void * ) existing, ( void * ) parms );

	mode = IMPORTER_MODE_ASK;
	dialog = import_ask_new( parms->parent );

	if( dialog ){

		dialog->private->importing = ( FMAObjectItem * ) importing;
		dialog->private->existing = ( FMAObjectItem * ) existing;
		dialog->private->parms = parms;

		initialize_window( dialog, dialog->private->toplevel );

		do {
			code = gtk_dialog_run( GTK_DIALOG( dialog->private->toplevel ));
		} while ( !on_dialog_response( dialog, code ));

		mode = dialog->private->mode;
		fma_gtk_utils_save_window_position( dialog->private->toplevel, IPREFS_IMPORT_ASK_USER_WSP );

		if( parms->parent ){
			gtk_widget_hide( GTK_WIDGET( dialog->private->toplevel ));

		} else {
			g_object_unref( dialog );
		}
	}

	return( mode );
}
static void
initialize_gtk( FMAImporterAsk *dialog, GtkWindow *toplevel )
{
	static const gchar *thisfn = "fma_importer_ask_initialize_gtk";
	GtkWidget *container;

	g_debug( "%s: dialog=%p, toplevel=%p", thisfn, ( void * ) dialog, ( void * ) toplevel );

	g_return_if_fail( FMA_IS_IMPORTER_ASK( dialog ));

	container = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( toplevel ), "AskModeBox" );
	g_return_if_fail( container && GTK_IS_CONTAINER( container ));

	fma_ioptions_list_gtk_init( FMA_IOPTIONS_LIST( dialog ), container, FALSE );
}

static void
initialize_window( FMAImporterAsk *editor, GtkWindow *toplevel )
{
	static const gchar *thisfn = "fma_importer_ask_initialize_window";
	gchar *imported_label, *existing_label;
	gchar *label;
	GtkWidget *widget;
	GtkWidget *button;
	gchar *mode_id;

	g_return_if_fail( FMA_IS_IMPORTER_ASK( editor ));

	g_debug( "%s: editor=%p, toplevel=%p", thisfn, ( void * ) editor, ( void * ) toplevel );

	imported_label = fma_object_get_label( editor->private->importing );
	existing_label = fma_object_get_label( editor->private->existing );

	if( FMA_IS_OBJECT_ACTION( editor->private->importing )){
		/* i18n: The action <action_label> imported from <file> has the same id than <existing_label> */
		label = g_strdup_printf(
				_( "The action \"%s\" imported from \"%s\" has the same identifier than the already existing \"%s\"." ),
				imported_label, editor->private->parms->uri, existing_label );

	} else {
		/* i18n: The menu <menu_label> imported from <file> has the same id than <existing_label> */
		label = g_strdup_printf(
				_( "The menu \"%s\" imported from \"%s\" has the same identifier than the already existing \"%s\"." ),
				imported_label, editor->private->parms->uri, existing_label );
	}

	widget = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( toplevel ), "ImporterAskLabel" );
	gtk_label_set_text( GTK_LABEL( widget ), label );
	g_free( label );

	widget = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( toplevel ), "AskModeBox" );
	mode_id = fma_settings_get_string( IPREFS_IMPORT_ASK_USER_LAST_MODE, NULL, NULL );
	fma_ioptions_list_set_default( FMA_IOPTIONS_LIST( editor ), widget, mode_id );
	g_free( mode_id );

	button = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( toplevel ), "AskKeepChoiceButton" );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( button ), editor->private->parms->keep_choice );

	fma_gtk_utils_restore_window_position( toplevel, IPREFS_IMPORT_ASK_USER_WSP );
	gtk_widget_show_all( GTK_WIDGET( toplevel ));
}

static void
get_selected_mode( FMAImporterAsk *editor )
{
	GtkWidget *widget;
	FMAIOption *mode;
	gchar *mode_id;
	GtkWidget *button;
	gboolean keep;

	widget = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( editor->private->toplevel ), "AskModeBox" );
	g_return_if_fail( widget && GTK_IS_CONTAINER( widget ));

	mode = fma_ioptions_list_get_selected( FMA_IOPTIONS_LIST( editor ), widget );

	mode_id = fma_ioption_get_id( mode );
	fma_settings_set_string( IPREFS_IMPORT_ASK_USER_LAST_MODE, mode_id );
	g_free( mode_id );

	editor->private->mode = fma_import_mode_get_id( FMA_IMPORT_MODE( mode ));

	button = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( editor->private->toplevel ), "AskKeepChoiceButton" );
	keep = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button ));
	fma_settings_set_boolean( IPREFS_IMPORT_ASK_USER_KEEP_LAST_CHOICE, keep );
}

/*
 * destroy signal is only connected if the FMAImporterAsk has been created
 * with a parent window; it has been defined with 'destroy_with_parent'
 * and so we have yet to unref the FMAImporterAsk object itself
 */
static void
on_destroy_toplevel( GtkWindow *toplevel, FMAImporterAsk *dialog )
{
	static const gchar *thisfn = "fma_importer_ask_on_destroy_toplevel";

	g_debug( "%s: toplevel=%p, dialog=%p",
			thisfn, ( void * ) toplevel, ( void * ) dialog );

	g_return_if_fail( FMA_IS_IMPORTER_ASK( dialog ));
	g_return_if_fail( toplevel == dialog->private->toplevel );

	if( !dialog->private->dispose_has_run ){

		dialog->private->toplevel = NULL;
		g_object_unref( dialog );
	}

	st_dialog = NULL;
}

static gboolean
on_dialog_response( FMAImporterAsk *editor, gint code )
{
	static const gchar *thisfn = "fma_importer_ask_on_dialog_response";

	g_return_val_if_fail( FMA_IS_IMPORTER_ASK( editor ), FALSE );

	g_debug( "%s: editor=%p, code=%d", thisfn, ( void * ) editor, code );

	switch( code ){
		case GTK_RESPONSE_NONE:
		case GTK_RESPONSE_DELETE_EVENT:
		case GTK_RESPONSE_CLOSE:
		case GTK_RESPONSE_CANCEL:
			editor->private->mode = IMPORTER_MODE_NO_IMPORT;
			return( TRUE );
			break;

		case GTK_RESPONSE_OK:
			get_selected_mode( editor );
			return( TRUE );
			break;
	}

	return( FALSE );
}
