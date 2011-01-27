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

#include "base-dialog.h"

/* private class data
 */
struct _BaseDialogClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _BaseDialogPrivate {
	gboolean dispose_has_run;
};

static BaseWindowClass *st_parent_class = NULL;

static GType    register_type( void );
static void     class_init( BaseDialogClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_dispose( GObject *application );
static void     instance_finalize( GObject *application );

static int      do_run( BaseWindow *window, GtkWindow *toplevel );
static gboolean terminate_dialog( BaseDialog *window, GtkDialog *toplevel, int *code );
static void     dialog_cancel( BaseDialog *window );
static void     dialog_ok( BaseDialog *window );

GType
base_dialog_get_type( void )
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
	static const gchar *thisfn = "base_dialog_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( BaseDialogClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( BaseDialog ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( BASE_WINDOW_TYPE, "BaseDialog", &info, 0 );

	return( type );
}

static void
class_init( BaseDialogClass *klass )
{
	static const gchar *thisfn = "base_dialog_class_init";
	GObjectClass *object_class;
	BaseWindowClass *base_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	base_class = BASE_WINDOW_CLASS( klass );
	base_class->run = do_run;

	klass->private = g_new0( BaseDialogClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "base_dialog_instance_init";
	BaseDialog *self;

	g_return_if_fail( BASE_IS_DIALOG( instance ));

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self = BASE_DIALOG( instance );

	self->private = g_new0( BaseDialogPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *window )
{
	static const gchar *thisfn = "base_dialog_instance_dispose";
	BaseDialog *self;

	g_return_if_fail( BASE_IS_DIALOG( window ));

	self = BASE_DIALOG( window );

	if( !self->private->dispose_has_run ){

		g_debug( "%s: window=%p (%s)", thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ));

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( window );
		}
	}
}

static void
instance_finalize( GObject *window )
{
	static const gchar *thisfn = "base_dialog_instance_finalize";
	BaseDialog *self;

	g_return_if_fail( BASE_IS_DIALOG( window ));

	g_debug( "%s: window=%p (%s)", thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ));

	self = BASE_DIALOG( window );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( window );
	}
}

/*
 * returns the response ID of the dialog box
 */
static int
do_run( BaseWindow *window, GtkWindow *toplevel )
{
	static const gchar *thisfn = "base_dialog_do_run";
	int code;

	g_return_val_if_fail( BASE_IS_DIALOG( window ), BASE_EXIT_CODE_PROGRAM );
	g_return_val_if_fail( GTK_IS_DIALOG( toplevel ), BASE_EXIT_CODE_PROGRAM );

	code = BASE_EXIT_CODE_INIT_FAIL;

	if( !BASE_DIALOG( window )->private->dispose_has_run ){
		g_debug( "%s: window=%p (%s), toplevel=%p (%s), starting gtk_dialog_run",
				thisfn,
				( void * ) window, G_OBJECT_TYPE_NAME( window ),
				( void * ) toplevel, G_OBJECT_TYPE_NAME( toplevel ));
		do {
			code = gtk_dialog_run( GTK_DIALOG( toplevel ));
		}
		while( !terminate_dialog( BASE_DIALOG( window ), GTK_DIALOG( toplevel ), &code ));
	}

	return( code );
}

/*
 * returns %TRUE to quit the dialog loop
 */
static gboolean
terminate_dialog( BaseDialog *window, GtkDialog *toplevel, int *code )
{
	gboolean quit = FALSE;

	switch( *code ){
		case GTK_RESPONSE_NONE:
		case GTK_RESPONSE_DELETE_EVENT:
		case GTK_RESPONSE_CLOSE:
		case GTK_RESPONSE_CANCEL:
			dialog_cancel( window );
			*code = GTK_RESPONSE_CANCEL;
			quit = TRUE;
			break;

		case GTK_RESPONSE_OK:
			dialog_ok( window );
			quit = TRUE;
			break;
	}

	return( quit );
}

static void
dialog_cancel( BaseDialog *window )
{
	if( BASE_DIALOG_GET_CLASS( window )->cancel ){
		BASE_DIALOG_GET_CLASS( window )->cancel( window );
	}
}

static void
dialog_ok( BaseDialog *window )
{
	if( BASE_DIALOG_GET_CLASS( window )->ok ){
		BASE_DIALOG_GET_CLASS( window )->ok( window );
	}
}
