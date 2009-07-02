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

#ifndef __NACT_IPROFILE_CONDITIONS_H__
#define __NACT_IPROFILE_CONDITIONS_H__

/*
 * NactIProfileConditions interface definition.
 *
 * This interface implements all the widgets which define the
 * conditions for the action.
 */

#include <common/na-action-profile.h>

#include "nact-window.h"

G_BEGIN_DECLS

#define NACT_IPROFILE_CONDITIONS_TYPE						( nact_iprofile_conditions_get_type())
#define NACT_IPROFILE_CONDITIONS( object )					( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_IPROFILE_CONDITIONS_TYPE, NactIProfileConditions ))
#define NACT_IS_IPROFILE_CONDITIONS( object )				( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_IPROFILE_CONDITIONS_TYPE ))
#define NACT_IPROFILE_CONDITIONS_GET_INTERFACE( instance )	( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NACT_IPROFILE_CONDITIONS_TYPE, NactIProfileConditionsInterface ))

typedef struct NactIProfileConditions NactIProfileConditions;

typedef struct NactIProfileConditionsInterfacePrivate NactIProfileConditionsInterfacePrivate;

typedef struct {
	GTypeInterface                          parent;
	NactIProfileConditionsInterfacePrivate *private;

	/* api */
	GObject * ( *get_edited_profile )( NactWindow *window );
	void      ( *field_modified )    ( NactWindow *window );
}
	NactIProfileConditionsInterface;

GType    nact_iprofile_conditions_get_type( void );

void     nact_iprofile_conditions_initial_load( NactWindow *dialog, NAActionProfile *profile );
void     nact_iprofile_conditions_size_labels( NactWindow *window, GObject *size_group );
void     nact_iprofile_conditions_size_buttons( NactWindow *window, GObject *size_group );

void     nact_iprofile_conditions_runtime_init( NactWindow *dialog, NAActionProfile *profile );
void     nact_iprofile_conditions_all_widgets_showed( NactWindow *dialog );

void     nact_iprofile_conditions_dispose( NactWindow *dialog );

G_END_DECLS

#endif /* __NACT_IPROFILE_CONDITIONS_H__ */
