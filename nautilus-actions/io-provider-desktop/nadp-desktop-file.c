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

#include "nadp-desktop-file.h"
#include "nadp-utils.h"

/* private class data
 */
struct NadpDesktopFileClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NadpDesktopFilePrivate {
	gboolean        dispose_has_run;
	gchar          *id;
	EggDesktopFile *edf;
};

static GObjectClass *st_parent_class = NULL;

static GType register_type( void );
static void  class_init( NadpDesktopFileClass *klass );
static void  instance_init( GTypeInstance *instance, gpointer klass );
static void  instance_dispose( GObject *object );
static void  instance_finalize( GObject *object );

GType
nadp_desktop_file_get_type( void )
{
	static GType class_type = 0;

	if( !class_type ){
		class_type = register_type();
	}

	return( class_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "nadp_desktop_file_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NadpDesktopFileClass ),
		NULL,
		NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NadpDesktopFile ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "NadpDesktopFile", &info, 0 );

	return( type );
}

static void
class_init( NadpDesktopFileClass *klass )
{
	static const gchar *thisfn = "nadp_desktop_file_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NadpDesktopFileClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nadp_desktop_file_instance_init";
	NadpDesktopFile *self;

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );
	g_return_if_fail( NADP_IS_DESKTOP_FILE( instance ));
	self = NADP_DESKTOP_FILE( instance );

	self->private = g_new0( NadpDesktopFilePrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "nadp_desktop_file_instance_dispose";
	NadpDesktopFile *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_return_if_fail( NADP_IS_DESKTOP_FILE( object ));
	self = NADP_DESKTOP_FILE( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		if( self->private->edf ){
			egg_desktop_file_free( self->private->edf );
			self->private->edf = NULL;
		}

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	NadpDesktopFile *self;

	g_assert( NADP_IS_DESKTOP_FILE( object ));
	self = NADP_DESKTOP_FILE( object );

	if( self->private->id ){
		g_free( self->private->id );
		self->private->id = NULL;
	}

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * nadp_desktop_file_new_from_path:
 * @path: the full pathname of a .desktop file.
 *
 * Retuns: a newly allocated #NadpDesktopFile object.
 */
NadpDesktopFile *
nadp_desktop_file_new_from_path( const gchar *path )
{
	static const gchar *thisfn = "nadp_desktop_file_new_from_path";
	NadpDesktopFile *ndf;
	EggDesktopFile *edf;
	GError *error;

	ndf = NULL;
	g_debug( "%s: path=%s", thisfn, path );
	g_return_val_if_fail( path && g_utf8_strlen( path, -1 ), ndf );

	error = NULL;
	edf = egg_desktop_file_new( path, &error );
	if( error ){
		g_warning( "%s: %s: %s", thisfn, path, error->message );
		g_error_free( error );
		return( NULL );
	}

	ndf = g_object_new( NADP_DESKTOP_FILE_TYPE, NULL );
	ndf->private->edf = edf;
	ndf->private->id = nadp_utils_path2id( path );

	return( ndf );
}

/**
 * nadp_desktop_file_get_id:
 * @ndf: the #NadpDesktopFile instance.
 *
 * Returns: the id of the file, as a newly allocated string which should
 * be g_free() by the caller.
 */
gchar *
nadp_desktop_file_get_id( const NadpDesktopFile *ndf )
{
	gchar *id;

	id = NULL;
	g_return_val_if_fail( NADP_IS_DESKTOP_FILE( ndf ), id );

	if( !ndf->private->dispose_has_run ){
		id = ndf->private->id;
	}

	return( id );
}

/**
 * nadp_desktop_file_get_egg_desktop_file:
 * @ndf: the #NadpDesktopFile instance.
 */
EggDesktopFile *
nadp_desktop_file_get_egg_desktop_file( const NadpDesktopFile *ndf )
{
	EggDesktopFile *edf;

	edf = NULL;
	g_return_val_if_fail( NADP_IS_DESKTOP_FILE( ndf ), edf );

	if( !ndf->private->dispose_has_run ){
		edf = ndf->private->edf;
	}

	return( edf );
}
