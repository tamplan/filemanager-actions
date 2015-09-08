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

#ifndef __UI_FMA_ADD_SCHEME_DIALOG_H__
#define __UI_FMA_ADD_SCHEME_DIALOG_H__

/**
 * SECTION: fma_add_scheme_dialog
 * @short_description: #FMAAddSchemeDialog class definition.
 * @include: nact/nact-add-scheme-dialog.h
 *
 * The dialog let the user pick a scheme from the default list
 * when adding a new scheme to a profile (resp. an action, a menu).
 */

#include "base-dialog.h"
#include "nact-main-window-def.h"

G_BEGIN_DECLS

#define FMA_TYPE_ADD_SCHEME_DIALOG                ( fma_add_scheme_dialog_get_type())
#define FMA_ADD_SCHEME_DIALOG( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_ADD_SCHEME_DIALOG, FMAAddSchemeDialog ))
#define FMA_ADD_SCHEME_DIALOG_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_ADD_SCHEME_DIALOG, FMAAddSchemeDialogClass ))
#define FMA_IS_ADD_SCHEME_DIALOG( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_ADD_SCHEME_DIALOG ))
#define FMA_IS_ADD_SCHEME_DIALOG_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_ADD_SCHEME_DIALOG ))
#define FMA_ADD_SCHEME_DIALOG_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_ADD_SCHEME_DIALOG, FMAAddSchemeDialogClass ))

typedef struct _FMAAddSchemeDialogPrivate         FMAAddSchemeDialogPrivate;

typedef struct {
	/*< private >*/
	BaseDialog                  parent;
	FMAAddSchemeDialogPrivate *private;
}
	FMAAddSchemeDialog;

typedef struct {
	/*< private >*/
	BaseDialogClass             parent;
}
	FMAAddSchemeDialogClass;

GType  fma_add_scheme_dialog_get_type( void );

gchar *fma_add_scheme_dialog_run     ( NactMainWindow *parent,
												GSList *schemes );

G_END_DECLS

#endif /* __UI_FMA_ADD_SCHEME_DIALOG_H__ */
