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

#include <common/na-iprefs.h>

#include <private/na-object-api.h>

#include <runtime/na-pivot.h>

#include "nact-application.h"
#include "nact-assistant-import-ask.h"

/* private class data
 */
struct NactAssistantImportAskClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NactAssistantImportAskPrivate {
	gboolean        dispose_has_run;
	NactMainWindow *parent;
	const gchar    *uri;
	NAObjectAction *action;
	NAObjectItem   *exist;
	gint            mode;
};

static BaseDialogClass *st_parent_class = NULL;

static GType    register_type( void );
static void     class_init( NactAssistantImportAskClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_dispose( GObject *dialog );
static void     instance_finalize( GObject *dialog );

static NactAssistantImportAsk *assistant_import_ask_new( BaseWindow *parent );

static gchar   *base_get_iprefs_window_id( BaseWindow *window );
static gchar   *base_get_dialog_name( BaseWindow *window );
static void     on_base_initial_load_dialog( NactAssistantImportAsk *editor, gpointer user_data );
static void     on_base_runtime_init_dialog( NactAssistantImportAsk *editor, gpointer user_data );
static void     on_base_all_widgets_showed( NactAssistantImportAsk *editor, gpointer user_data );
static void     on_cancel_clicked( GtkButton *button, NactAssistantImportAsk *editor );
static void     on_ok_clicked( GtkButton *button, NactAssistantImportAsk *editor );
static void     get_mode( NactAssistantImportAsk *editor );
static gboolean base_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window );

GType
nact_assistant_import_ask_get_type( void )
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
	static const gchar *thisfn = "nact_assistant_import_ask_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NactAssistantImportAskClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactAssistantImportAsk ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( BASE_DIALOG_TYPE, "NactAssistantImportAsk", &info, 0 );

	return( type );
}

static void
class_init( NactAssistantImportAskClass *klass )
{
	static const gchar *thisfn = "nact_assistant_import_ask_class_init";
	GObjectClass *object_class;
	BaseWindowClass *base_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactAssistantImportAskClassPrivate, 1 );

	base_class = BASE_WINDOW_CLASS( klass );
	base_class->dialog_response = base_dialog_response;
	base_class->get_toplevel_name = base_get_dialog_name;
	base_class->get_iprefs_window_id = base_get_iprefs_window_id;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_assistant_import_ask_instance_init";
	NactAssistantImportAsk *self;

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );
	g_return_if_fail( NACT_IS_ASSISTANT_IMPORT_ASK( instance ));
	self = NACT_ASSISTANT_IMPORT_ASK( instance );

	self->private = g_new0( NactAssistantImportAskPrivate, 1 );

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
}

static void
instance_dispose( GObject *dialog )
{
	static const gchar *thisfn = "nact_assistant_import_ask_instance_dispose";
	NactAssistantImportAsk *self;

	g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );
	g_return_if_fail( NACT_IS_ASSISTANT_IMPORT_ASK( dialog ));
	self = NACT_ASSISTANT_IMPORT_ASK( dialog );

	if( !self->private->dispose_has_run ){

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
	static const gchar *thisfn = "nact_assistant_import_ask_instance_finalize";
	NactAssistantImportAsk *self;

	g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );
	g_return_if_fail( NACT_IS_ASSISTANT_IMPORT_ASK( dialog ));
	self = NACT_ASSISTANT_IMPORT_ASK( dialog );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( dialog );
	}
}

/*
 * Returns a newly allocated NactAssistantImportAsk object.
 */
static NactAssistantImportAsk *
assistant_import_ask_new( BaseWindow *parent )
{
	return( g_object_new( NACT_ASSISTANT_IMPORT_ASK_TYPE, BASE_WINDOW_PROP_PARENT, parent, NULL ));
}

/**
 * nact_assistant_import_ask_run:
 * @parent: the NactMainWindow parent of this dialog.
 *
 * Initializes and runs the dialog.
 *
 * Returns: the mode choosen by the user ; it defaults to NO_IMPORT.
 *
 * When the user selects 'Keep same choice without asking me', this choice
 * becomes his preference import mode.
 */
gint
nact_assistant_import_ask_user( NactMainWindow *parent, const gchar *uri, NAObjectAction *action, NAObjectItem *exist )
{
	static const gchar *thisfn = "nact_assistant_import_ask_run";
	NactApplication *application;
	NAPivot *pivot;
	NactAssistantImportAsk *editor;
	gint mode;

	g_debug( "%s: parent=%p", thisfn, ( void * ) parent );
	g_return_val_if_fail( BASE_IS_WINDOW( parent ), IPREFS_IMPORT_NO_IMPORT );

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( parent )));
	g_return_val_if_fail( NACT_IS_APPLICATION( application ), IPREFS_IMPORT_NO_IMPORT );

	pivot = nact_application_get_pivot( application );
	g_return_val_if_fail( NA_IS_PIVOT( pivot ), IPREFS_IMPORT_NO_IMPORT );

	editor = assistant_import_ask_new( BASE_WINDOW( parent ));
	editor->private->parent = parent;
	editor->private->uri = uri;
	editor->private->action = action;
	editor->private->exist = exist;
	editor->private->mode = na_iprefs_get_import_mode( NA_IPREFS( pivot ), IPREFS_IMPORT_ASK_LAST_MODE );

	base_window_run( BASE_WINDOW( editor ));

	na_iprefs_set_import_mode( NA_IPREFS( pivot ), IPREFS_IMPORT_ASK_LAST_MODE, editor->private->mode );
	mode = editor->private->mode;
	g_object_unref( editor );

	return( mode );
}

static gchar *
base_get_iprefs_window_id( BaseWindow *window )
{
	return( g_strdup( "import-ask-user" ));
}

static gchar *
base_get_dialog_name( BaseWindow *window )
{
	return( g_strdup( "AssistantImportAsk" ));
}

static void
on_base_initial_load_dialog( NactAssistantImportAsk *editor, gpointer user_data )
{
	static const gchar *thisfn = "nact_assistant_import_ask_on_initial_load_dialog";

	g_debug( "%s: editor=%p, user_data=%p", thisfn, ( void * ) editor, ( void * ) user_data );
	g_return_if_fail( NACT_IS_ASSISTANT_IMPORT_ASK( editor ));
}

static void
on_base_runtime_init_dialog( NactAssistantImportAsk *editor, gpointer user_data )
{
	static const gchar *thisfn = "nact_assistant_import_ask_on_runtime_init_dialog";
	gchar *action_label, *exist_label;
	gchar *label;
	GtkWidget *widget;
	GtkWidget *button;

	g_debug( "%s: editor=%p, user_data=%p", thisfn, ( void * ) editor, ( void * ) user_data );
	g_return_if_fail( NACT_IS_ASSISTANT_IMPORT_ASK( editor ));

	action_label = na_object_get_label( editor->private->action );
	exist_label = na_object_get_label( editor->private->exist );

	/* i18n: The action <action_label> imported from <file> has the same id than <existing_label> */
	label = g_strdup_printf(
			_( "The action \"%s\" imported from \"%s\" has the same identifiant than the already existing \"%s\"." ),
			action_label, editor->private->uri, exist_label );

	widget = base_window_get_widget( BASE_WINDOW( editor ), "ImportAskLabel" );
	gtk_label_set_text( GTK_LABEL( widget ), label );
	g_free( label );

	switch( editor->private->mode ){
		case IPREFS_IMPORT_RENUMBER:
			button = base_window_get_widget( BASE_WINDOW( editor ), "AskRenumberButton" );
			break;

		case IPREFS_IMPORT_OVERRIDE:
			button = base_window_get_widget( BASE_WINDOW( editor ), "AskOverrideButton" );
			break;

		case IPREFS_IMPORT_NO_IMPORT:
		default:
			button = base_window_get_widget( BASE_WINDOW( editor ), "AskNoImportButton" );
			break;
	}
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( button ), TRUE );

	button = base_window_get_widget( BASE_WINDOW( editor ), "AskKeepChoiceButton" );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( button ), FALSE );

	base_window_signal_connect_by_name(
			BASE_WINDOW( editor ),
			"CancelButton1",
			"clicked",
			G_CALLBACK( on_cancel_clicked ));

	base_window_signal_connect_by_name(
			BASE_WINDOW( editor ),
			"OKButton1",
			"clicked",
			G_CALLBACK( on_ok_clicked ));
}

static void
on_base_all_widgets_showed( NactAssistantImportAsk *editor, gpointer user_data )
{
	static const gchar *thisfn = "nact_assistant_import_ask_on_all_widgets_showed";

	g_debug( "%s: editor=%p, user_data=%p", thisfn, ( void * ) editor, ( void * ) user_data );
	g_return_if_fail( NACT_IS_ASSISTANT_IMPORT_ASK( editor ));
}

static void
on_cancel_clicked( GtkButton *button, NactAssistantImportAsk *editor )
{
	GtkWindow *toplevel = base_window_get_toplevel( BASE_WINDOW( editor ));
	gtk_dialog_response( GTK_DIALOG( toplevel ), GTK_RESPONSE_CLOSE );
}

static void
on_ok_clicked( GtkButton *button, NactAssistantImportAsk *editor )
{
	GtkWindow *toplevel = base_window_get_toplevel( BASE_WINDOW( editor ));
	gtk_dialog_response( GTK_DIALOG( toplevel ), GTK_RESPONSE_OK );
}

static void
get_mode( NactAssistantImportAsk *editor )
{
	gint import_mode;
	NactApplication *application;
	NAPivot *pivot;
	GtkWidget *button;
	gboolean keep;

	import_mode = IPREFS_IMPORT_NO_IMPORT;
	button = base_window_get_widget( BASE_WINDOW( editor ), "AskRenumberButton" );
	if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button ))){
		import_mode = IPREFS_IMPORT_RENUMBER;
	} else {
		button = base_window_get_widget( BASE_WINDOW( editor ), "AskOverrideButton" );
		if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button ))){
			import_mode = IPREFS_IMPORT_OVERRIDE;
		}
	}

	editor->private->mode = import_mode;

	button = base_window_get_widget( BASE_WINDOW( editor ), "AskKeepChoiceButton" );
	keep = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button ));
	if( keep ){
		application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( editor )));
		pivot = nact_application_get_pivot( application );
		na_iprefs_set_import_mode( NA_IPREFS( pivot ), IPREFS_IMPORT_ACTIONS_IMPORT_MODE, import_mode );
	}
}

static gboolean
base_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window )
{
	static const gchar *thisfn = "nact_assistant_import_ask_on_dialog_response";
	NactAssistantImportAsk *editor;

	g_debug( "%s: dialog=%p, code=%d, window=%p", thisfn, ( void * ) dialog, code, ( void * ) window );
	g_assert( NACT_IS_ASSISTANT_IMPORT_ASK( window ));
	editor = NACT_ASSISTANT_IMPORT_ASK( window );

	switch( code ){
		case GTK_RESPONSE_NONE:
		case GTK_RESPONSE_DELETE_EVENT:
		case GTK_RESPONSE_CLOSE:
		case GTK_RESPONSE_CANCEL:

			editor->private->mode = IPREFS_IMPORT_NO_IMPORT;
			return( TRUE );
			break;

		case GTK_RESPONSE_OK:
			get_mode( editor );
			return( TRUE );
			break;
	}

	return( FALSE );
}
