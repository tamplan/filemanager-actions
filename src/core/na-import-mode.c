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

#include "na-import-mode.h"
#include "na-ioption.h"

/* private class data
 */
struct _NAImportModeClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _NAImportModePrivate {
	gboolean     dispose_has_run;
};

static GObjectClass *st_parent_class = NULL;

static GType  register_type( void );
static void   class_init( NAImportModeClass *klass );
static void   ioption_iface_init( NAIOptionInterface *iface );
static guint  ioption_get_version( const NAIOption *instance );
static void   instance_init( GTypeInstance *instance, gpointer klass );
static void   instance_dispose( GObject *object );
static void   instance_finalize( GObject *object );

GType
na_import_mode_get_type( void )
{
	static GType object_type = 0;

	if( !object_type ){
		object_type = register_type();
	}

	return( object_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "na_import_mode_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NAImportModeClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NAImportMode ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	static const GInterfaceInfo ioption_iface_info = {
		( GInterfaceInitFunc ) ioption_iface_init,
		NULL,
		NULL
	};

	type = g_type_register_static( G_TYPE_OBJECT, "NAImportMode", &info, 0 );

	g_type_add_interface_static( type, NA_IOPTION_TYPE, &ioption_iface_info );

	return( type );
}

static void
class_init( NAImportModeClass *klass )
{
	static const gchar *thisfn = "na_import_mode_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NAImportModeClassPrivate, 1 );
}

static void
ioption_iface_init( NAIOptionInterface *iface )
{
	static const gchar *thisfn = "na_import_mode_ioption_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->get_version = ioption_get_version;
}

static guint
ioption_get_version( const NAIOption *instance )
{
	return( 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "na_import_mode_instance_init";
	NAImportMode *self;

	g_return_if_fail( NA_IS_IMPORT_MODE( instance ));

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );
	self = NA_IMPORT_MODE( instance );

	self->private = g_new0( NAImportModePrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "na_import_mode_instance_dispose";
	NAImportMode *self;

	g_return_if_fail( NA_IS_IMPORT_MODE( object ));

	self = NA_IMPORT_MODE( object );

	if( !self->private->dispose_has_run ){

		g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

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
	static const gchar *thisfn = "na_import_mode_instance_finalize";
	NAImportMode *self;

	g_return_if_fail( NA_IS_IMPORT_MODE( object ));

	g_debug( "%s: object=%p", thisfn, ( void * ) object );

	self = NA_IMPORT_MODE( object );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}
