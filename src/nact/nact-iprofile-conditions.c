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

#include <common/na-action.h>
#include <common/na-action-profile.h>

#include "nact-iprofile-conditions.h"

/* private interface data
 */
struct NactIProfileConditionsInterfacePrivate {
};

static GType      register_type( void );
static void       interface_base_init( NactIProfileConditionsInterface *klass );
static void       interface_base_finalize( NactIProfileConditionsInterface *klass );

GType
nact_iprofile_conditions_get_type( void )
{
	static GType iface_type = 0;

	if( !iface_type ){
		iface_type = register_type();
	}

	return( iface_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "nact_iprofile_conditions_register_type";
	g_debug( "%s", thisfn );

	static const GTypeInfo info = {
		sizeof( NactIProfileConditionsInterface ),
		( GBaseInitFunc ) interface_base_init,
		( GBaseFinalizeFunc ) interface_base_finalize,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL
	};

	GType type = g_type_register_static( G_TYPE_INTERFACE, "NactIProfileConditions", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( NactIProfileConditionsInterface *klass )
{
	static const gchar *thisfn = "nact_iprofile_conditions_interface_base_init";
	static gboolean initialized = FALSE;

	if( !initialized ){
		g_debug( "%s: klass=%p", thisfn, klass );

		klass->private = g_new0( NactIProfileConditionsInterfacePrivate, 1 );

		initialized = TRUE;
	}
}

static void
interface_base_finalize( NactIProfileConditionsInterface *klass )
{
	static const gchar *thisfn = "nact_iprofile_conditions_interface_base_finalize";
	static gboolean finalized = FALSE ;

	if( !finalized ){
		g_debug( "%s: klass=%p", thisfn, klass );

		g_free( klass->private );

		finalized = TRUE;
	}
}

void
nact_iprofile_conditions_initial_load( NactWindow *dialog, NAAction *action )
{
}

void
nact_iprofile_conditions_runtime_init( NactWindow *dialog, NAAction *action )
{
}

void
nact_iprofile_conditions_all_widgets_showed( NactWindow *dialog )
{
	static const gchar *thisfn = "nact_iprofile_conditions_all_widgets_showed";
	g_debug( "%s: dialog=%p", thisfn, dialog );

	GtkNotebook *notebook = GTK_NOTEBOOK( base_window_get_widget( BASE_WINDOW( dialog ), "notebook2" ));
	gtk_notebook_set_current_page( notebook, 0 );
}
