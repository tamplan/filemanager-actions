/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2014 Pierre Wieser and others (see AUTHORS)
 *
 * Nautilus-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Nautilus-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nautilus-Actions; see the file COPYING. If not, see
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

#include "core/na-gtk-utils.h"
#include "core/na-io-provider.h"

#include "base-gtk-utils.h"
#include "nact-statusbar.h"

struct _NactStatusbarPrivate {
	gboolean   dispose_has_run;
	GtkWidget *image;
};

typedef struct {
	guint          event_source_id;
	guint          context_id;
	NactStatusbar *bar;
}
	StatusbarTimeoutDisplayStruct;

#define LOCKED_IMAGE					PKGUIDIR "/locked.png"

static GObjectClass *st_parent_class    = NULL;

static GType    register_type( void );
static void     class_init( NactStatusbarClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_dispose( GObject *application );
static void     instance_finalize( GObject *application );
static void     init_bar( NactStatusbar *bar );
static gboolean display_timeout( StatusbarTimeoutDisplayStruct *stds );
static void     display_timeout_free( StatusbarTimeoutDisplayStruct *stds );

GType
nact_statusbar_get_type( void )
{
	static GType type = 0;

	if( !type ){
		type = register_type();
	}

	return( type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "nact_statusbar_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NactStatusbarClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactStatusbar ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( GTK_TYPE_STATUSBAR, "NactStatusbar", &info, 0 );

	return( type );
}

static void
class_init( NactStatusbarClass *klass )
{
	static const gchar *thisfn = "nact_statusbar_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_statusbar_instance_init";
	NactStatusbar *self;

	g_return_if_fail( NACT_IS_STATUSBAR( instance ));

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self = NACT_STATUSBAR( instance );

	self->private = g_new0( NactStatusbarPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "nact_statusbar_instance_dispose";
	NactStatusbarPrivate *priv;

	g_return_if_fail( NACT_IS_STATUSBAR( object ));

	priv = NACT_STATUSBAR( object )->private;

	if( !priv->dispose_has_run ){

		g_debug( "%s: object=%p (%s)",
				thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

		priv->dispose_has_run = TRUE;

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *instance )
{
	static const gchar *thisfn = "nact_statusbar_instance_finalize";
	NactStatusbar *self;

	g_return_if_fail( NACT_IS_STATUSBAR( instance ));

	g_debug( "%s: instance=%p (%s)", thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	self = NACT_STATUSBAR( instance );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( instance );
	}
}

/**
 * nact_statusbar_new:
 *
 * Returns: a new #NactStatusbar object.
 */
NactStatusbar *
nact_statusbar_new( void )
{
	NactStatusbar *bar;

	bar = g_object_new( NACT_TYPE_STATUSBAR, NULL );

	init_bar( bar );

	return( bar );
}

static void
init_bar( NactStatusbar *bar )
{
	GtkWidget *frame, *image;

	frame = gtk_aspect_frame_new( NULL, 0.5, 0.5, 1, FALSE );
	gtk_box_pack_end( GTK_BOX( bar ), frame, FALSE, FALSE, 4 );

	image = gtk_image_new();
	gtk_container_add( GTK_CONTAINER( frame ), image );
	bar->private->image = image;
}

/**
 * nact_statusbar_display_status:
 * @bar: this #NactStatusbar instance.
 * @context: the context to be displayed.
 * @status: the message.
 *
 * Push a message.
 */
void
nact_statusbar_display_status( NactStatusbar *bar, const gchar *context, const gchar *status )
{
	static const gchar *thisfn = "nact_statusbar_display_status";

	g_debug( "%s: bar=%p, context=%s, status=%s",
			thisfn, ( void * ) bar, context, status );

	if( !status || !g_utf8_strlen( status, -1 )){
		return;
	}

	guint context_id = gtk_statusbar_get_context_id( GTK_STATUSBAR( bar ), context );
	gtk_statusbar_push( GTK_STATUSBAR( bar ), context_id, status );
}

/**
 * nact_statusbar_display_with_timeout:
 * @bar: this #NactStatusbar instance.
 * @context: the context to be displayed.
 * @status: the message.
 *
 * Push a message.
 * Automatically pop it after a timeout.
 * The timeout is not suspended when another message is pushed onto the
 * previous one.
 */
void
nact_statusbar_display_with_timeout( NactStatusbar *bar, const gchar *context, const gchar *status )
{
	static const gchar *thisfn = "nact_statusbar_display_with_timeout";
	StatusbarTimeoutDisplayStruct *stds;

	g_debug( "%s: bar=%p, context=%s, status=%s",
			thisfn, ( void * ) bar, context, status );

	if( !status || !g_utf8_strlen( status, -1 )){
		return;
	}

	guint context_id = gtk_statusbar_get_context_id( GTK_STATUSBAR( bar ), context );
	gtk_statusbar_push( GTK_STATUSBAR( bar ), context_id, status );

	stds = g_new0( StatusbarTimeoutDisplayStruct, 1 );
	stds->context_id = context_id;
	stds->bar = bar;
	stds->event_source_id = g_timeout_add_seconds_full(
			G_PRIORITY_DEFAULT,
			10,
			( GSourceFunc ) display_timeout,
			stds,
			( GDestroyNotify ) display_timeout_free );
}

/**
 * nact_statusbar_hide_status:
 * @bar: this #NactStatusbar instance.
 * @context: the context to be hidden.
 *
 * Hide the specified context.
 */
void
nact_statusbar_hide_status( NactStatusbar *bar, const gchar *context )
{
	guint context_id = gtk_statusbar_get_context_id( GTK_STATUSBAR( bar ), context );
	gtk_statusbar_pop( GTK_STATUSBAR( bar ), context_id );
}

/**
 * nact_statusbar_set_locked:
 * @bar: this #NactStatusbar instance.
 * @provider: whether the current provider is locked (read-only).
 * @item: whether the current item is locked (read-only).
 *
 * Displays the writability status of the current item as an image.
 * Installs the corresponding tooltip.
 */
void
nact_statusbar_set_locked( NactStatusbar *bar, gboolean readonly, gint reason )
{
	static const gchar *thisfn = "nact_statusbar_set_locked";
	NactStatusbarPrivate *priv;
	gchar *tooltip;
	gboolean set_pixbuf;

	g_debug( "%s: bar=%p, readonly=%s, reason=%d",
			thisfn, ( void * ) bar, readonly ? "True":"False", reason );

	priv = bar->private;
	if( !priv->dispose_has_run ){

		set_pixbuf = TRUE;
		tooltip = g_strdup( "" );

		if( readonly ){
			gtk_image_set_from_file( GTK_IMAGE( priv->image ), LOCKED_IMAGE );
			set_pixbuf = FALSE;
			g_free( tooltip );
			tooltip = na_io_provider_get_readonly_tooltip( reason );
		}

		gtk_widget_set_tooltip_text( priv->image, tooltip );
		g_free( tooltip );

		if( set_pixbuf ){
			base_gtk_utils_render( NULL, GTK_IMAGE( priv->image ), GTK_ICON_SIZE_MENU );
		}
	}
}

static gboolean
display_timeout( StatusbarTimeoutDisplayStruct *stds )
{
	gboolean keep_source = FALSE;

	gtk_statusbar_pop( GTK_STATUSBAR( stds->bar ), stds->context_id );

	return( keep_source );
}

static void
display_timeout_free( StatusbarTimeoutDisplayStruct *stds )
{
	g_debug( "nact_statusbar_display_timeout_free: stds=%p", ( void * ) stds );

	g_free( stds );
}
