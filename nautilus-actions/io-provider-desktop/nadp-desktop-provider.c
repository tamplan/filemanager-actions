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

#include <string.h>

#include <nautilus-actions/api/na-iio-provider.h>

#include "nadp-desktop-provider.h"
#include "nadp-read.h"
#include "nadp-write.h"

/* private class data
 */
struct NadpDesktopProviderClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
typedef struct NadpDesktopProviderPrivate {
	gboolean dispose_has_run;
}
	NadpDesktopProviderPrivate;

static GType         st_module_type = 0;
static GObjectClass *st_parent_class = NULL;

static void           class_init( NadpDesktopProviderClass *klass );
static void           iio_provider_iface_init( NAIIOProviderInterface *iface );
static void           instance_init( GTypeInstance *instance, gpointer klass );
static void           instance_dispose( GObject *object );
static void           instance_finalize( GObject *object );

GType
nadp_desktop_provider_get_type( void )
{
	return( st_module_type );
}

void
nadp_desktop_provider_register_type( GTypeModule *module )
{
	static const gchar *thisfn = "nadp_desktop_provider_register_type";

	static GTypeInfo info = {
		sizeof( NadpDesktopProviderClass ),
		NULL,
		NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NadpDesktopProvider ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	static const GInterfaceInfo iio_provider_iface_info = {
		( GInterfaceInitFunc ) iio_provider_iface_init,
		NULL,
		NULL
	};

	g_debug( "%s", thisfn );

	st_module_type = g_type_module_register_type( module, G_TYPE_OBJECT, "NadpDesktopProvider", &info, 0 );

	g_type_module_add_interface( module, st_module_type, NA_IIO_PROVIDER_TYPE, &iio_provider_iface_info );
}

static void
class_init( NadpDesktopProviderClass *klass )
{
	static const gchar *thisfn = "nadp_desktop_provider_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NadpDesktopProviderClassPrivate, 1 );
}

static void
iio_provider_iface_init( NAIIOProviderInterface *iface )
{
	static const gchar *thisfn = "nadp_desktop_provider_iio_provider_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->read_items = nadp_iio_provider_read_items;
	iface->is_willing_to_write = nadp_iio_provider_is_willing_to_write;
	iface->is_writable = nadp_iio_provider_is_writable;
	iface->write_item = nadp_iio_provider_write_item;
	iface->delete_item = nadp_iio_provider_delete_item;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nadp_desktop_provider_instance_init";
	NadpDesktopProvider *self;

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );
	g_return_if_fail( NADP_IS_DESKTOP_PROVIDER( instance ));
	self = NADP_DESKTOP_PROVIDER( instance );

	self->private = g_new0( NadpDesktopProviderPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "nadp_desktop_provider_instance_dispose";
	NadpDesktopProvider *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_return_if_fail( NADP_IS_DESKTOP_PROVIDER( object ));
	self = NADP_DESKTOP_PROVIDER( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	NadpDesktopProvider *self;

	g_assert( NADP_IS_DESKTOP_PROVIDER( object ));
	self = NADP_DESKTOP_PROVIDER( object );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}
