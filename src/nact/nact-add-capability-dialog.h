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

#ifndef __NACT_ADD_CAPABILITY_DIALOG_H__
#define __NACT_ADD_CAPABILITY_DIALOG_H__

/**
 * SECTION: nact_add_capability_dialog
 * @short_description: #NactAddCapabilityDialog class definition.
 * @include: nact/nact-add-capability-dialog.h
 *
 * The dialog let the user pick a capability from the default list
 * when adding a new capability to a profile (resp. an action, a menu).
 */

#include "base-dialog.h"

G_BEGIN_DECLS

#define NACT_ADD_CAPABILITY_DIALOG_TYPE                ( nact_add_capability_dialog_get_type())
#define NACT_ADD_CAPABILITY_DIALOG( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_ADD_CAPABILITY_DIALOG_TYPE, NactAddCapabilityDialog ))
#define NACT_ADD_CAPABILITY_DIALOG_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, NACT_ADD_CAPABILITY_DIALOG_TYPE, NactAddCapabilityDialogClass ))
#define NACT_IS_ADD_CAPABILITY_DIALOG( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_ADD_CAPABILITY_DIALOG_TYPE ))
#define NACT_IS_ADD_CAPABILITY_DIALOG_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), NACT_ADD_CAPABILITY_DIALOG_TYPE ))
#define NACT_ADD_CAPABILITY_DIALOG_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), NACT_ADD_CAPABILITY_DIALOG_TYPE, NactAddCapabilityDialogClass ))

typedef struct _NactAddCapabilityDialogPrivate         NactAddCapabilityDialogPrivate;

typedef struct {
	/*< private >*/
	BaseDialog                      parent;
	NactAddCapabilityDialogPrivate *private;
}
	NactAddCapabilityDialog;

typedef struct _NactAddCapabilityDialogClassPrivate    NactAddCapabilityDialogClassPrivate;

typedef struct {
	/*< private >*/
	BaseDialogClass                      parent;
	NactAddCapabilityDialogClassPrivate *private;
}
	NactAddCapabilityDialogClass;

GType  nact_add_capability_dialog_get_type( void );

gchar *nact_add_capability_dialog_run( BaseWindow *parent, GSList *capabilities );

G_END_DECLS

#endif /* __NACT_ADD_CAPABILITY_DIALOG_H__ */
