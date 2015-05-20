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

#ifndef __NACT_CONFIRM_LOGOUT_H__
#define __NACT_CONFIRM_LOGOUT_H__

/**
 * SECTION: nact_preferences_editor
 * @short_description: #NactConfirmLogout class definition.
 * @include: nact/nact-preferences-editor.h
 *
 * This class is derived from NactWindow.
 * It encapsulates the "PreferencesDialog" widget dialog.
 */

#include "base-dialog.h"
#include "nact-main-window-def.h"

G_BEGIN_DECLS

#define NACT_TYPE_CONFIRM_LOGOUT                ( nact_confirm_logout_get_type())
#define NACT_CONFIRM_LOGOUT( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_TYPE_CONFIRM_LOGOUT, NactConfirmLogout ))
#define NACT_CONFIRM_LOGOUT_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, NACT_TYPE_CONFIRM_LOGOUT, NactConfirmLogoutClass ))
#define NACT_IS_CONFIRM_LOGOUT( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_TYPE_CONFIRM_LOGOUT ))
#define NACT_IS_CONFIRM_LOGOUT_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), NACT_TYPE_CONFIRM_LOGOUT ))
#define NACT_CONFIRM_LOGOUT_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), NACT_TYPE_CONFIRM_LOGOUT, NactConfirmLogoutClass ))

typedef struct _NactConfirmLogoutPrivate        NactConfirmLogoutPrivate;

typedef struct {
	/*< private >*/
	BaseDialog                parent;
	NactConfirmLogoutPrivate *private;
}
	NactConfirmLogout;

typedef struct {
	/*< private >*/
	BaseDialogClass           parent;
}
	NactConfirmLogoutClass;

GType    nact_confirm_logout_get_type( void );

gboolean nact_confirm_logout_run     ( NactMainWindow *parent );

G_END_DECLS

#endif /* __NACT_CONFIRM_LOGOUT_H__ */
