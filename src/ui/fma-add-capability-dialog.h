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

#ifndef __UI_FMA_ADD_CAPABILITY_DIALOG_H__
#define __UI_FMA_ADD_CAPABILITY_DIALOG_H__

/**
 * SECTION: fma_add_capability_dialog
 * @short_description: #FMAAddCapabilityDialog class definition.
 * @include: ui/fma-add-capability-dialog.h
 *
 * The dialog let the user pick a capability from the default list
 * when adding a new capability to a profile (resp. an action, a menu).
 */

#include "base-dialog.h"
#include "fma-main-window-def.h"

G_BEGIN_DECLS

#define FMA_TYPE_ADD_CAPABILITY_DIALOG                ( fma_add_capability_dialog_get_type())
#define FMA_ADD_CAPABILITY_DIALOG( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_ADD_CAPABILITY_DIALOG, FMAAddCapabilityDialog ))
#define FMA_ADD_CAPABILITY_DIALOG_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_ADD_CAPABILITY_DIALOG, FMAAddCapabilityDialogClass ))
#define FMA_IS_ADD_CAPABILITY_DIALOG( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_ADD_CAPABILITY_DIALOG ))
#define FMA_IS_ADD_CAPABILITY_DIALOG_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_ADD_CAPABILITY_DIALOG ))
#define FMA_ADD_CAPABILITY_DIALOG_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_ADD_CAPABILITY_DIALOG, FMAAddCapabilityDialogClass ))

typedef struct _FMAAddCapabilityDialogPrivate         FMAAddCapabilityDialogPrivate;

typedef struct {
	/*< private >*/
	BaseDialog                     parent;
	FMAAddCapabilityDialogPrivate *private;
}
	FMAAddCapabilityDialog;

typedef struct _FMAAddCapabilityDialogClassPrivate    FMAAddCapabilityDialogClassPrivate;

typedef struct {
	/*< private >*/
	BaseDialogClass                     parent;
	FMAAddCapabilityDialogClassPrivate *private;
}
	FMAAddCapabilityDialogClass;

GType  fma_add_capability_dialog_get_type( void );

gchar *fma_add_capability_dialog_run     ( FMAMainWindow *parent,
													GSList *capabilities );

G_END_DECLS

#endif /* __UI_FMA_ADD_CAPABILITY_DIALOG_H__ */
